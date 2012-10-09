//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "gamemovement.h"
#include "cf_gamerules.h"
#include "cf_shareddefs.h"
#include "in_buttons.h"
#include "movevars_shared.h"
#include "particle_parse.h"
#include "statistics.h"
#include "armament.h"
#include "KeyValues.h"
#include "coordsize.h"	// for DIST_EPSILON

#ifdef CLIENT_DLL
	#include "c_cf_player.h"
	#include "prediction.h"
#else
	#include "cf_player.h"
	#include "ndebugoverlay.h"
#endif

extern bool g_bMovementOptimizations;

class CCFGameMovement : public CGameMovement
{
public:
	DECLARE_CLASS( CCFGameMovement, CGameMovement );

	CCFGameMovement();

	virtual void	StartTrackPredictionErrors( CBasePlayer *pPlayer );

	virtual void	FullWalkMove();
	virtual void	PlayerMove(	void );
	virtual void	FullBodyAnimationMove( void );
	virtual void	RushMove( void );
	virtual void	LatchMove( void );
	virtual void	FatalityMove();

	virtual void	FindBestGoalMeleePosition( CCFPlayer* pTarget, float flDistance, Vector& vecPosition );

	// Decompoooooosed gravity
	void			StartGravity( void );
	void			FinishGravity( void );

	virtual bool	CheckJumpButton( void );
	void			PreventBunnyJumping();

	virtual void	PowerjumpThink();

	virtual bool	Powerjump(bool bChargeJump = false);
	virtual void	Latch();
	virtual bool	CanFullLatch();
	virtual void	JumpTo( Vector vecDirection, float flHeight, float flDistance, bool bNoDash = false );

	virtual void	Duck( void ) {};	// Goose

	virtual void	TracePlayerBBox( const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm );

	virtual bool	GameHasLadders() const { return false; }
	virtual bool	CanAccelerate();

	virtual void	CheckParameters( void );

	virtual void	PlayerRoughLandingEffects( float fvol );

protected:
	CCFPlayer*		m_pCFPlayer;
};


// Expose our interface.
static CCFGameMovement g_GameMovement;
IGameMovement *g_pGameMovement = ( IGameMovement * )&g_GameMovement;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CGameMovement, IGameMovement,INTERFACENAME_GAMEMOVEMENT, g_GameMovement );

static ConVar mp_rushspeed( "mp_rushspeed", "1200", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
static ConVar mp_rushrspeed( "mp_rushrspeed", "50", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );

static ConVar mp_dashtime( "mp_dashtime", "0.2", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
static ConVar mp_dashspeed( "mp_dashspeed", "960", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );

static ConVar sv_collisionradius( "sv_collisionradius", "23", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
static ConVar sv_collisionradiusfollow( "sv_collisionradiusfollow", "40", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );

static ConVar mp_halflatchspeed( "mp_halflatchspeed", "120", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );

// ---------------------------------------------------------------------------------------- //
// CCFGameMovement.
// ---------------------------------------------------------------------------------------- //

CCFGameMovement::CCFGameMovement()
{
	//m_vecViewOffsetNormal = CF_PLAYER_VIEW_OFFSET;
}

void CCFGameMovement::StartTrackPredictionErrors( CBasePlayer *pPlayer )
{
	m_pCFPlayer = ToCFPlayer(pPlayer);
	BaseClass::StartTrackPredictionErrors(pPlayer);
}

void CCFGameMovement::PlayerMove( void )
{
	if (m_pCFPlayer->PlayerFrozen())
	{
		mv->m_flForwardMove *= m_pCFPlayer->m_flFreezeAmount;
		mv->m_flSideMove *= m_pCFPlayer->m_flFreezeAmount;
		mv->m_flUpMove *= m_pCFPlayer->m_flFreezeAmount;
		mv->m_nImpulseCommand = 0;
	}

	if (m_pCFPlayer->m_nButtons & IN_WALK)
	{
		float frac = 0.33333333f;
		mv->m_flForwardMove	*= frac;
		mv->m_flSideMove	*= frac;
		mv->m_flUpMove		*= frac;
	}

	CCFPlayer* pTarget = m_pCFPlayer->GetRecursedTarget();

	if (m_pCFPlayer->m_bWantVelocityMatched && pTarget && (pTarget->GetAbsOrigin() - m_pCFPlayer->GetAbsOrigin()).Length2DSqr() < 64*64)
	{
		Vector vecPlayerVelNorm = mv->m_vecVelocity;
		Vector vecTargetVelNorm = pTarget->GetAbsVelocity();
		vecPlayerVelNorm.z = 0;
		vecTargetVelNorm.z = 0;
		vecPlayerVelNorm.NormalizeInPlace();
		vecTargetVelNorm.NormalizeInPlace();

		float flDot = DotProduct(vecPlayerVelNorm, vecTargetVelNorm);

		// If the enemy is moving backwards compared to this player, just stop.
		if (flDot < 0)
			flDot = 0;

		// Try to match the enemy's velocity, but don't actually do any turning.
		mv->m_vecVelocity = mv->m_vecVelocity*flDot;

		m_pCFPlayer->m_bWantVelocityMatched = false;
	}

	// Give the player some time before the velocity matching is no longer effective.
	if (m_pCFPlayer->m_bWantVelocityMatched && gpGlobals->curtime > m_pCFPlayer->m_flLastDashTime + mp_dashtime.GetFloat()*2)	
		m_pCFPlayer->m_bWantVelocityMatched = false;

	if (gpGlobals->curtime > m_pCFPlayer->m_flLastDashTime + mp_dashtime.GetFloat())
		m_pCFPlayer->SetGravity(1);

	BaseClass::PlayerMove();

	// Can't put this in FullWalkMove, that never gets called during fatalities. It's fine here.
	if (m_pCFPlayer->m_hReviver != NULL || m_pCFPlayer->m_hReviving != NULL)
	{
		FatalityMove();
	}
}

void CCFGameMovement::FullWalkMove( )
{
	if ( !CheckWater() )
	{
		StartGravity();
	}

	// If we are leaping out of the water, just update the counters.
	if (player->m_flWaterJumpTime)
	{
		WaterJump();
		TryPlayerMove();
		// See if we are still in water?
		CheckWater();
		return;
	}

	// If we are swimming in the water, see if we are nudging against a place we can jump up out
	//  of, and, if so, start out jump.  Otherwise, if we are not moving up, then reset jump timer to 0
	if ( player->GetWaterLevel() >= WL_Waist ) 
	{
		if ( player->GetWaterLevel() == WL_Waist )
		{
			CheckWaterJump();
		}

			// If we are falling again, then we must not trying to jump out of water any more.
		if ( mv->m_vecVelocity[2] < 0 && 
			 player->m_flWaterJumpTime )
		{
			player->m_flWaterJumpTime = 0;
		}

		// Was jump button pressed?
		if (mv->m_nButtons & IN_JUMP)
		{
			CheckJumpButton();
		}
		else
		{
			mv->m_nOldButtons &= ~IN_JUMP;
		}

		// Perform regular water movement
		WaterMove();

		// Redetermine position vars
		CategorizePosition();

		// If we are on ground, no downward velocity.
		if ( player->GetGroundEntity() != NULL )
		{
			mv->m_vecVelocity[2] = 0;			
		}
	}
	else
	// Not fully underwater
	{
		Latch();

		// Was jump button pressed?
		if (mv->m_nButtons & IN_JUMP)
		{
 			CheckJumpButton();
		}
		else
		{
			mv->m_nOldButtons &= ~IN_JUMP;
		}

		PowerjumpThink();

		// Fricion is handled before we add in any base velocity. That way, if we are on a conveyor, 
		//  we don't slow when standing still, relative to the conveyor.
		if (player->GetGroundEntity() != NULL && !m_pCFPlayer->m_bLatched)
		{
			mv->m_vecVelocity[2] = 0.0;
			Friction();
		}

		// Make sure velocity is valid.
		CheckVelocity();

		if (m_pCFPlayer->IsInFullBodyAnimation())
		{
			FullBodyAnimationMove();
		}
		else if (m_pCFPlayer->m_flRushDistance > 0)
		{
			RushMove();
		}
		else if (m_pCFPlayer->m_bLatched)
		{
			LatchMove();
		}
		else if (player->GetGroundEntity() != NULL)
		{
			WalkMove();
		}
		else
		{
			AirMove();  // Take into account movement when in air.
		}

		// Set final flags.
		CategorizePosition();

		// Make sure velocity is valid.
		CheckVelocity();

		// Add any remaining gravitational component.
		if ( !CheckWater() )
		{
			FinishGravity();
		}

		// If we are on ground, no downward velocity.
		// Except for latched people, they might be latched to something that's moving up or down.
		if ( player->GetGroundEntity() != NULL )
		{
			mv->m_vecVelocity[2] = 0;
		}
		CheckFalling();
	}

	if  ( ( m_nOldWaterLevel == WL_NotInWater && player->GetWaterLevel() != WL_NotInWater ) ||
		  ( m_nOldWaterLevel != WL_NotInWater && player->GetWaterLevel() == WL_NotInWater ) )
	{
		PlaySwimSound();
#if !defined( CLIENT_DLL )
		player->Splash();
#endif
	}
}


void CCFGameMovement::FullBodyAnimationMove( void )
{
	trace_t pm;

	bool bFinished;
	Vector vecNewPosition;
	QAngle angNewAngles;

	float flFrameTime;
#ifdef CLIENT_DLL
	if (m_pCFPlayer->GetCycle() == 0)
	{
		m_pCFPlayer->m_flLastAnimFrameTime = gpGlobals->curtime;
		flFrameTime = gpGlobals->frametime;
	}
	else
		flFrameTime = gpGlobals->curtime - m_pCFPlayer->m_flLastAnimFrameTime;
	m_pCFPlayer->m_flLastAnimFrameTime = gpGlobals->curtime;
#else
	flFrameTime = gpGlobals->frametime;
#endif

	m_pCFPlayer->SetAbsAngles(m_pCFPlayer->m_angEyeAngles);

	m_pCFPlayer->GetIntervalMovement(flFrameTime, bFinished, vecNewPosition, angNewAngles);

	if (bFinished || (m_pCFPlayer->GetLocalOrigin() - vecNewPosition).LengthSqr() == 0)
	{
		mv->m_vecVelocity = Vector(0,0,0);
		return;
	}

	Vector vecDelta, vecInternalDelta = vecNewPosition - m_pCFPlayer->GetLocalOrigin();
	float flPitch = -mv->m_vecAbsViewAngles[0];

	while (flPitch > 180)
		flPitch -= 360;
	while (flPitch < -180)
		flPitch += 360;

	float flLength = vecInternalDelta.Length2D();	// Only need 2d because z is not extracted from anim data.
	float flZ = sin(DEG2RAD(flPitch))*flLength;		// Find z based on where the player is looking.

	// Modify it based on the player's direction of movement. (Looking up but moving backwards actually means moving down.)
	Vector vecForward, vecForward2D, vecInternalDeltaNormalized = vecInternalDelta;
	m_pCFPlayer->GetVectors(&vecForward, NULL, NULL);
	vecForward2D = vecForward;
	vecForward2D.z = vecInternalDeltaNormalized.z = 0;
	VectorNormalize(vecForward2D);
	VectorNormalize(vecInternalDeltaNormalized);

	float flNewLength = RemapVal(fabs(DotProduct(vecInternalDeltaNormalized, vecForward2D)), 0, 1, flLength, flZ / tan(DEG2RAD(flPitch)));

	vecDelta = vecInternalDelta;
	VectorNormalize(vecDelta);
	vecDelta *= flNewLength;
	vecDelta.z = flZ * DotProduct(vecInternalDeltaNormalized, vecForward2D);

	Vector vecDestination = mv->GetAbsOrigin()+vecDelta;

	mv->m_vecVelocity = vecDelta/gpGlobals->frametime;

	// first try moving directly to the next spot
	TracePlayerBBox( mv->GetAbsOrigin(), vecDestination, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, pm );

	if ( pm.fraction == 1 )
	{
		mv->SetAbsOrigin( pm.endpos );
		return;
	}

	// We are using the animation's velocity to force velocity consistent with the animation.
	mv->m_vecVelocity += player->GetBaseVelocity();

	StepMove( vecDestination, pm );

	// Now pull the base velocity back out.
	mv->m_vecVelocity -= player->GetBaseVelocity();
}

void CCFGameMovement::RushMove( void )
{
	trace_t pm;

	Vector vecTarget;

	if (m_pCFPlayer->m_bDownStrike)
	{
		if (m_pCFPlayer->GetGroundEntity())
		{
			m_pCFPlayer->EndRush();
			return;
		}
	}

	if (m_pCFPlayer->GetRecursedTarget())
	{
		float flDistance = sv_collisionradiusfollow.GetFloat();
		if (m_pCFPlayer->m_hRushingWeapon != NULL && m_pCFPlayer->m_hRushingWeapon->m_bStrongAttack)
			flDistance *= 2;
		FindBestGoalMeleePosition(m_pCFPlayer->GetRecursedTarget(), flDistance, vecTarget);
	}
	else
	{
		m_pCFPlayer->EndRush();
		return;
	}

	Vector vecDirection = vecTarget - mv->GetAbsOrigin();
	vecDirection.NormalizeInPlace();

	mv->m_vecVelocity = vecDirection * mp_rushspeed.GetFloat();

	Vector vecDelta = mv->m_vecVelocity * gpGlobals->frametime;

	Vector vecDestination = mv->GetAbsOrigin()+vecDelta;

	// first try moving directly to the next spot
	TracePlayerBBox( mv->GetAbsOrigin(), vecDestination, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, pm );

	if ( pm.fraction == 1 )
	{
		mv->SetAbsOrigin( pm.endpos );

		// Down strikes can rush down forever so long as the way is clear.
		// Also they don't do an extra swing at the bottom.
		if (!m_pCFPlayer->m_bDownStrike)
		{
			m_pCFPlayer->m_flRushDistance -= vecDelta.Length();

			if (m_pCFPlayer->m_hRushingWeapon != NULL && m_pCFPlayer->m_flRushDistance <= 0 && !m_pCFPlayer->m_bStrongAttackJump)
				m_pCFPlayer->m_hRushingWeapon->SetSwingTime(gpGlobals->curtime);
		}
	}
	else
	{
		// We are using the animation's velocity to force velocity consistent with the animation.
		mv->m_vecVelocity += player->GetBaseVelocity();

		Vector vecOldOrigin = mv->GetAbsOrigin();

		StepMove( vecDestination, pm );

		m_pCFPlayer->m_flRushDistance -= max((mv->GetAbsOrigin() - vecOldOrigin).Length(), 2);	// Always decrease at least a little bit.

		// Now pull the base velocity back out.
		mv->m_vecVelocity -= player->GetBaseVelocity();
	}

	bool bRushOver = false;

	// Down strikes go forever until they hit the ground.
	if (!m_pCFPlayer->m_bDownStrike)
	{
		float flDistanceToTarget = (mv->GetAbsOrigin() - vecTarget).Length2DSqr();

		if (flDistanceToTarget < vecDelta.Length2DSqr())
			bRushOver = true;

		float flDistanceToVictim = (mv->GetAbsOrigin() - m_pCFPlayer->GetRecursedTarget()->GetAbsOrigin()).LengthSqr();
		float flIdealDistanceSqr = sv_collisionradiusfollow.GetFloat() * sv_collisionradiusfollow.GetFloat();

		if (flDistanceToVictim < flIdealDistanceSqr)
			bRushOver = true;

		if (m_pCFPlayer->m_flRushDistance <= 0)
			bRushOver = true;

		if (!bRushOver)
		{
			Vector vecDirectionNormalized = vecDirection;
			vecDirectionNormalized.z = 0;
			vecDirectionNormalized.NormalizeInPlace();

			// Add in vecDelta so we're simulating where we will be next frame.
			// If we're going to be overshooting it next frame, we want to end it now.
			Vector vecToTargetNormalized = vecTarget - (m_pCFPlayer->GetAbsOrigin() + vecDelta);
			vecToTargetNormalized.z = 0;
			vecToTargetNormalized.NormalizeInPlace();

			if (DotProduct(vecDirectionNormalized, vecToTargetNormalized) < 0.5f)
				bRushOver = true;
		}
	}

	if (m_pCFPlayer->m_hRushingWeapon != NULL && bRushOver)
	{
		m_pCFPlayer->EndRush();
		mv->m_vecVelocity = Vector(0,0,0);
	}
}

void CCFGameMovement::LatchMove( void )
{
	// If the player is not moving in z, it looks really bad if he is sliding slideways.
	if (fabs(mv->m_vecVelocity.z) > 1)
	{
		mv->m_vecVelocity.x = Approach(0, mv->m_vecVelocity.x, 400.0f * gpGlobals->frametime);
		mv->m_vecVelocity.y = Approach(0, mv->m_vecVelocity.y, 400.0f * gpGlobals->frametime);
	}
	else
	{
		mv->m_vecVelocity.x = 0;
		mv->m_vecVelocity.y = 0;
	}

	if (CanFullLatch())
		mv->m_vecVelocity.z = Approach(0, mv->m_vecVelocity.z, 600.0f * gpGlobals->frametime);
	else if (mv->m_vecVelocity.z < -mp_halflatchspeed.GetFloat())
		mv->m_vecVelocity.z = Approach(-mp_halflatchspeed.GetFloat(), mv->m_vecVelocity.z, 400.0f * gpGlobals->frametime);

	// Add in any base velocity to the current velocity.
	VectorAdd(mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity );

	TryPlayerMove();

	// Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
	VectorSubtract( mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity );
}

#define FATALITY_DISTANCE 40

void CCFGameMovement::FatalityMove( void )
{
	CCFPlayer* pTarget = m_pCFPlayer->m_hReviving;
	if (!pTarget)
		pTarget = m_pCFPlayer->m_hReviver;

	// Try to get up to the proper distance for a fatality, FATALITY_DISTANCE units.

	Vector vecTargetDirection = (pTarget->GetAbsOrigin() - m_pCFPlayer->GetAbsOrigin());

	if (!(m_pCFPlayer->GetFlags() & FL_ONGROUND))
		vecTargetDirection.z = 0;	// Try to level off the two players, but the one in the air has to do all the work.

	VectorNormalize(vecTargetDirection);
	Vector vecGoalPosition = pTarget->GetAbsOrigin() - vecTargetDirection*FATALITY_DISTANCE;

	if (m_pCFPlayer->GetFlags() & FL_ONGROUND)
		vecGoalPosition.z = m_pCFPlayer->GetAbsOrigin().z;	// If I'm already on the ground then don't try going up, and of course down won't work either.
	else
		vecGoalPosition.z = pTarget->GetAbsOrigin().z;	// I'm not on the ground so I'll go down to his level.

	if ((m_pCFPlayer->GetAbsOrigin() - vecGoalPosition).Length2D() > 1)
	{
		Vector vecGoalDirection = vecGoalPosition - m_pCFPlayer->GetAbsOrigin();
		VectorNormalize(vecGoalDirection);

		// This should get us there in about .5 seconds.
		Vector vecEndPoint = m_pCFPlayer->GetAbsOrigin() + vecGoalDirection*gpGlobals->frametime*FATALITY_DISTANCE*2;

		trace_t tr;
		UTIL_TraceHull(m_pCFPlayer->GetAbsOrigin(), vecEndPoint, GetPlayerMins(), GetPlayerMaxs(), MASK_PLAYERSOLID, m_pCFPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &tr);
		mv->SetAbsOrigin(tr.endpos);
		mv->m_vecVelocity = Vector(0,0,0);
	}
}

void CCFGameMovement::FindBestGoalMeleePosition( CCFPlayer* pTarget, float flClosest, Vector& vecPosition )
{
	Vector vecTarget = pTarget->GetAbsOrigin() - m_pCFPlayer->GetAbsOrigin();

	float flDistance = vecTarget.Length();

	vecTarget.z = 0;				// We want to be next to him, not up or down.
	vecTarget.NormalizeInPlace();
	vecTarget *= flClosest;			// We want to be this far from him.
	vecPosition = pTarget->GetAbsOrigin() - vecTarget;

	Vector vecTargetVelocity = pTarget->GetAbsVelocity();

	// Cap it so it doesn't go crazy when people are flying around with rush and whatever.
	if (vecTargetVelocity.LengthSqr() > 250*250)
	{
		vecTargetVelocity.NormalizeInPlace();
		vecTargetVelocity *= 250;
	}

	// Add a little for where we think our target is going to be in the time it takes to travel there.
	vecPosition += vecTargetVelocity * (flDistance/mp_dashspeed.GetFloat());

#if defined(GAME_DLL) && defined(_DEBUG)
//	NDebugOverlay::Box( vecPosition, Vector(-2,-2,-2), Vector(2,2,2), 255,0,255,127, 20 );
//	NDebugOverlay::Box( m_pCFPlayer->GetAbsOrigin(), Vector(-2,-2,-2), Vector(2,2,2), 0,0,255,127, 20 );
//	NDebugOverlay::Box( pTarget->GetAbsOrigin(), Vector(-2,-2,-2), Vector(2,2,2), 255,0,0,127, 20 );
#endif
}

void CCFGameMovement::StartGravity( void )
{
	if (CanFullLatch() && m_pCFPlayer->m_bLatched)
		return;

	if (m_pCFPlayer->IsGravitySuspended())
		return;

	float flGravity;
	
	if (m_pCFPlayer->GetGravity())
		flGravity = m_pCFPlayer->GetGravity();
	else
		flGravity = 1.0;

	// Add gravity so they'll be in the correct position during movement
	// yes, this 0.5 looks wrong, but it's not.  
	mv->m_vecVelocity[2] -= (flGravity * sv_gravity.GetFloat() * 0.5 * gpGlobals->frametime );
	mv->m_vecVelocity[2] += player->GetBaseVelocity()[2] * gpGlobals->frametime;

	Vector temp = player->GetBaseVelocity();
	temp[ 2 ] = 0;
	player->SetBaseVelocity( temp );

	CheckVelocity();
}

void CCFGameMovement::FinishGravity( void )
{
	if (CanFullLatch() && m_pCFPlayer->m_bLatched)
		return;

	if (m_pCFPlayer->IsGravitySuspended())
		return;

	float flGravity;

	if ( player->m_flWaterJumpTime )
		return;

	if ( m_pCFPlayer->GetGravity() )
		flGravity = m_pCFPlayer->GetGravity();
	else
		flGravity = 1.0;

	// Get the correct velocity for the end of the dt 
  	mv->m_vecVelocity[2] -= (flGravity * sv_gravity.GetFloat() * gpGlobals->frametime * 0.5);

	CheckVelocity();}

bool CCFGameMovement::CheckJumpButton()
{
	int buttonsChanged	= ( mv->m_nOldButtons ^ mv->m_nButtons );	// These buttons have changed this frame
	int buttonsPressed	=  buttonsChanged & mv->m_nButtons;			// The changed ones still down are "pressed"

	if (m_pCFPlayer->IsInFullBodyAnimation())
		return false;

	if (m_pCFPlayer->pl.deadflag)
	{
		mv->m_nOldButtons |= IN_JUMP ;	// don't jump again until released
		return false;
	}

	// See if we are waterjumping.  If so, decrement count and return.
	if (m_pCFPlayer->m_flWaterJumpTime)
	{
		m_pCFPlayer->m_flWaterJumpTime -= gpGlobals->frametime;
		if (m_pCFPlayer->m_flWaterJumpTime < 0)
			m_pCFPlayer->m_flWaterJumpTime = 0;
		
		return false;
	}

	// If we are in the water most of the way...
	if ( m_pCFPlayer->GetWaterLevel() >= WL_Eyes )
	{	
		// swimming, not jumping
		SetGroundEntity( NULL );

		if(m_pCFPlayer->GetWaterType() == CONTENTS_WATER)    // We move up a certain amount
			mv->m_vecVelocity[2] = 100;
		else if (m_pCFPlayer->GetWaterType() == CONTENTS_SLIME)
			mv->m_vecVelocity[2] = 80;
		
		// play swiming sound
		if ( m_pCFPlayer->m_flSwimSoundTime <= 0 )
		{
			// Don't play sound again for 1 second
			m_pCFPlayer->m_flSwimSoundTime = 1000;
			PlaySwimSound();
		}

		return false;
	}

	// No more effect
 	if (m_pCFPlayer->GetGroundEntity() == NULL && (buttonsPressed & IN_JUMP))
	{
		if ((mv->m_nButtons & (IN_BACK|IN_FORWARD|IN_MOVELEFT|IN_MOVERIGHT)) == 0)
			Powerjump(true);
		else
			Powerjump();
		mv->m_nOldButtons |= IN_JUMP;
		return false;		// in air, so no effect
	}

	if (m_pCFPlayer->m_flStrongAttackJumpTime > 0.0f && m_pCFPlayer->m_flStrongAttackJumpTime > gpGlobals->curtime)
	{
		m_pCFPlayer->m_flStrongAttackJumpTime = 0;
		m_pCFPlayer->Rush();
		m_pCFPlayer->m_bStrongAttackJump = true;
		mv->m_nOldButtons |= IN_JUMP;
		return false;
	}

	if ( mv->m_nOldButtons & IN_JUMP )
		return false;		// don't pogo stick

	// Cannot jump will in the unduck transition.
	if ( m_pCFPlayer->m_Local.m_bDucking && (  m_pCFPlayer->GetFlags() & FL_DUCKING ) )
		return false;

	// Still updating the eye position.
	if ( m_pCFPlayer->m_Local.m_flDuckJumpTime > 0.0f )
		return false;

	// In the air now.
    SetGroundEntity( NULL );
	
	m_pCFPlayer->PlayStepSound( (Vector &)mv->GetAbsOrigin(), m_pCFPlayer->m_pSurfaceData, 1.0, true );
	
	MoveHelper()->PlayerSetAnimation( PLAYER_JUMP );

	float flGroundFactor = 1.0f;
	if (m_pCFPlayer->m_pSurfaceData)
	{
		flGroundFactor = m_pCFPlayer->m_pSurfaceData->game.jumpFactor; 
	}

	float flMul;
	if ( g_bMovementOptimizations )
	{
		Assert( sv_gravity.GetFloat() == 650.0f );
		flMul = 270.0f;
	}
	else
	{
		flMul = sqrt(2 * sv_gravity.GetFloat() * GAMEMOVEMENT_JUMP_HEIGHT);
	}

	// Acclerate upward
	// If we are ducking...
	float startz = mv->m_vecVelocity[2];
	if ( (  m_pCFPlayer->m_Local.m_bDucking ) || (  m_pCFPlayer->GetFlags() & FL_DUCKING ) )
	{
		// d = 0.5 * g * t^2		- distance traveled with linear accel
		// t = sqrt(2.0 * 45 / g)	- how long to fall 45 units
		// v = g * t				- velocity at the end (just invert it to jump up that high)
		// v = g * sqrt(2.0 * 45 / g )
		// v^2 = g * g * 2.0 * 45 / g
		// v = sqrt( g * 2.0 * 45 )
		mv->m_vecVelocity[2] = flGroundFactor * flMul;  // 2 * gravity * height
	}
	else
	{
		mv->m_vecVelocity[2] += flGroundFactor * flMul;  // 2 * gravity * height
	}

	FinishGravity();

	// CheckV( player->CurrentCommandNumber(), "CheckJump", mv->m_vecVelocity );

	mv->m_outJumpVel.z += mv->m_vecVelocity[2] - startz;
	mv->m_outStepHeight += 0.15f;

	// Set jump time.
	if ( gpGlobals->maxClients == 1 )
	{
		m_pCFPlayer->m_Local.m_flJumpTime = GAMEMOVEMENT_JUMP_TIME;
		m_pCFPlayer->m_Local.m_bInDuckJump = true;
	}

	PreventBunnyJumping();

	m_pCFPlayer->DoAnimationEvent( PLAYERANIMEVENT_JUMP );

	// Flag that we jumped.
	mv->m_nOldButtons |= IN_JUMP;	// don't jump again until released
	return true;
}

// Only allow bunny jumping up to 1.2x server / player maxspeed setting
#define BUNNYJUMP_MAX_SPEED_FACTOR 1.2f

void CCFGameMovement::PreventBunnyJumping()
{
	// Speed at which bunny jumping is limited
	float maxscaledspeed = BUNNYJUMP_MAX_SPEED_FACTOR * m_pCFPlayer->MaxSpeed();
	if ( maxscaledspeed <= 0.0f )
		return;

	// Current player speed
	float spd = mv->m_vecVelocity.Length();
	if ( spd <= maxscaledspeed )
		return;

	// Apply this cropping fraction to velocity
	float fraction = ( maxscaledspeed / spd );

	mv->m_vecVelocity *= fraction;
}

#ifdef CLIENT_DLL
ConVar cl_doubletapdodges("cl_doubletapdodges", "1", FCVAR_USERINFO|FCVAR_ARCHIVE, "Turn on the double-tap dodging feature.");
#endif

void CCFGameMovement::PowerjumpThink()
{
	int buttonsChanged	= ( mv->m_nOldButtons ^ mv->m_nButtons );	// These buttons have changed this frame
	int buttonsPressed	=  buttonsChanged & mv->m_nButtons;			// The changed ones still down are "pressed"

	bool bUseDoubleTap;

#ifdef CLIENT_DLL
	bUseDoubleTap = cl_doubletapdodges.GetBool();
#else
	const char *pszDoubleTap = "0";
	if (!m_pCFPlayer->IsBot())
		pszDoubleTap = engine->GetClientConVarValue( m_pCFPlayer->entindex(), "cl_doubletapdodges" );
	bUseDoubleTap = !!atoi(pszDoubleTap);
#endif

	if (gpGlobals->curtime < m_pCFPlayer->m_flLastMovementButtonTime + 0.2f)
	{
		if (m_pCFPlayer->m_iLastMovementButton == IN_FORWARD && (buttonsPressed & IN_FORWARD))
		{
			if (bUseDoubleTap)
			{
				Powerjump();
				m_pCFPlayer->Instructor_LessonLearned(HINT_DOUBLETAP_POWERJUMP);
			}
		}
		else if (m_pCFPlayer->m_iLastMovementButton == IN_MOVELEFT && (buttonsPressed & IN_MOVELEFT))
		{
			if (bUseDoubleTap)
			{
				Powerjump();
				m_pCFPlayer->Instructor_LessonLearned(HINT_DOUBLETAP_POWERJUMP);
			}
		}
		else if (m_pCFPlayer->m_iLastMovementButton == IN_MOVERIGHT && (buttonsPressed & IN_MOVERIGHT))
		{
			if (bUseDoubleTap)
			{
				Powerjump();
				m_pCFPlayer->Instructor_LessonLearned(HINT_DOUBLETAP_POWERJUMP);
			}
		}
		else if (m_pCFPlayer->m_iLastMovementButton == IN_BACK && (buttonsPressed & IN_BACK))
		{
			if (bUseDoubleTap)
			{
				Powerjump();
				m_pCFPlayer->Instructor_LessonLearned(HINT_DOUBLETAP_POWERJUMP);
			}
		}
	}

	if (buttonsPressed & IN_FORWARD)
	{
		m_pCFPlayer->m_flLastMovementButtonTime = gpGlobals->curtime;
		m_pCFPlayer->m_iLastMovementButton = IN_FORWARD;
	}
	else if (buttonsPressed & IN_MOVELEFT)
	{
		m_pCFPlayer->m_flLastMovementButtonTime = gpGlobals->curtime;
		m_pCFPlayer->m_iLastMovementButton = IN_MOVELEFT;
	}
	else if (buttonsPressed & IN_MOVERIGHT)
	{
		m_pCFPlayer->m_flLastMovementButtonTime = gpGlobals->curtime;
		m_pCFPlayer->m_iLastMovementButton = IN_MOVERIGHT;
	}
	else if (buttonsPressed & IN_BACK)
	{
		m_pCFPlayer->m_flLastMovementButtonTime = gpGlobals->curtime;
		m_pCFPlayer->m_iLastMovementButton = IN_BACK;
	}
}

ConVar cf_powerjumpstamina("cf_powerjumpstamina", "25", FCVAR_DEVELOPMENTONLY);
ConVar cf_dodgestamina("cf_dodgestamina", "12", FCVAR_DEVELOPMENTONLY);
bool CCFGameMovement::Powerjump(bool bChargeJump)
{
	if (m_pCFPlayer->m_bPowerjump && bChargeJump && !m_pCFPlayer->m_bChargejump)
	{
		// This is only here to emit the noise. Otherwise it is redundant to the tests below.
		m_pCFPlayer->EmitSound( "Player.PowerjumpFailed" );
		return false;
	}

	if (m_pCFPlayer->m_bPowerjump && !m_pCFPlayer->m_bChargejump)
		return false;

	if (m_pCFPlayer->m_bChargejump && bChargeJump)
		return false;

	if (m_pCFPlayer->m_bLatched)
		return false;

	// Prevent PJ when spamming latch button.
	if (gpGlobals->curtime < m_pCFPlayer->m_flLastLatch + LATCH_BREAK_TIME)
		return false;

	if (!m_pCFPlayer->m_bCanPowerjump)
	{
		m_pCFPlayer->EmitSound( "Player.PowerjumpFailed" );
		return false;
	}

	// Limit on how long after jumping one can powerjump. ie no jumping down a huge canyon and powerjumping just before you hit the bottom.
	if (m_pCFPlayer->GetGroundEntity() == NULL && gpGlobals->curtime - m_pCFPlayer->m_Local.m_flLastJump > 2)
	{
		m_pCFPlayer->EmitSound( "Player.PowerjumpFailed" );
		return false;
	}

	if (m_pCFPlayer->m_pStats->GetStamina() < 0)
	{
		m_pCFPlayer->EmitSound( "Player.PowerjumpFailed" );
		return false;
	}

	//m_pCFPlayer->SetGravity(0.85);

	float flPowerjumpDistance = m_pCFPlayer->GetActiveArmament()->GetPowerjumpDistance() * m_pCFPlayer->m_pStats->GetPowerJumpScale();

	if (flPowerjumpDistance < 40)
		return false;

	if (m_pCFPlayer->IsFuse())
	{
		m_pCFPlayer->EmitSound( "Player.PowerjumpFailed" );
		return false;
	}

	// You double tap space bar to do a charge jump, so we'll count that as lesson learned.
	if (bChargeJump)
		m_pCFPlayer->Instructor_LessonLearned(HINT_DOUBLETAP_POWERJUMP);

	if (flPowerjumpDistance > 8192)
		flPowerjumpDistance = 8192;

	Vector vecDirection;
	if (bChargeJump)
		vecDirection = Vector(0, 0, 1);	// Straight up, yo.
	else if (m_pCFPlayer->ShouldLockFollowModeView())
	{
		CCFPlayer* pTarget = m_pCFPlayer->GetRecursedTarget();
		Vector vecTarget = pTarget->GetAbsOrigin() - m_pCFPlayer->GetAbsOrigin();
		float flDistance2DSqr = vecTarget.Length2DSqr();
		float flRadius = sv_collisionradiusfollow.GetFloat();
		if (flDistance2DSqr > flRadius*flRadius)
		{
			FindBestGoalMeleePosition(pTarget, flRadius, vecTarget);
			vecDirection = vecTarget - m_pCFPlayer->GetAbsOrigin();
		}
		else
		{
			vecTarget.x = 0;
			vecTarget.y = 0;
			vecDirection = vecTarget;
		}
		vecDirection.NormalizeInPlace();
	}
	else
		AngleVectors( mv->m_vecViewAngles, &vecDirection );

	// Jump up or down about 1/2 as much as up.
	float flDot = fabs(DotProduct( vecDirection, Vector(0, 0, 1) ));
	flPowerjumpDistance *= (1-flDot) + 0.5*flDot;

	JumpTo(vecDirection, 100, flPowerjumpDistance, bChargeJump);

	// Charge-jumps aren't real powerjumps, so you can do a powerjump out of them. They still take normal stamina though.
	if (bChargeJump)
	{
		m_pCFPlayer->m_Local.m_flLastJump = gpGlobals->curtime;
		m_pCFPlayer->m_bChargejump = true;
	}
	else
		m_pCFPlayer->m_bChargejump = false;

	m_pCFPlayer->m_bPowerjump = true;

	int iType = 0;
	if (m_pCFPlayer->m_nButtons & IN_MOVELEFT)
		iType = ACT_CF_POWERJUMP_LEFT;
	else if (m_pCFPlayer->m_nButtons & IN_MOVERIGHT)
		iType = ACT_CF_POWERJUMP_RIGHT;
	else if (m_pCFPlayer->m_nButtons & IN_BACK)
		iType = ACT_CF_POWERJUMP_BACK;

	m_pCFPlayer->DoAnimationEvent( PLAYERANIMEVENT_POWERJUMP, iType );

#ifdef CLIENT_DLL
	if (prediction->IsFirstTimePredicted())
#endif
	{
		CPASAttenuationFilter filter( m_pCFPlayer );
		filter.UsePredictionRules();
		m_pCFPlayer->EmitSound( filter, m_pCFPlayer->entindex(), "Player.Powerjump" );

		QAngle angDirection;
		VectorAngles(vecDirection, angDirection);
		DispatchParticleEffect( "powerjump", m_pCFPlayer->GetAbsOrigin(), angDirection, m_pCFPlayer );
	}

#ifdef GAME_DLL
	if (mv->m_nButtons & (IN_MOVELEFT|IN_MOVERIGHT|IN_BACK) || m_pCFPlayer->m_bChargejump)
		m_pCFPlayer->m_pStats->m_flStamina -= cf_dodgestamina.GetFloat();
	else
		m_pCFPlayer->m_pStats->m_flStamina -= cf_powerjumpstamina.GetFloat();
#endif

	return true;
}

void CCFGameMovement::JumpTo( Vector vecDirection, float flHeight, float flDistance, bool bNoDash )
{
	if (m_pCFPlayer->ShouldLockFollowModeView() && !bNoDash)
	{
		m_pCFPlayer->m_flLastDashTime = gpGlobals->curtime;
		m_pCFPlayer->m_bWantVelocityMatched = true;

		m_pCFPlayer->SetGravity(0.01f);

		mv->m_vecVelocity = vecDirection * mp_dashspeed.GetFloat();

		SetGroundEntity( NULL );

		return;
	}

	float flGravity = sv_gravity.GetFloat() * m_pCFPlayer->GetGravity();

	// Find where the player is pointing at
	trace_t tr;
	UTIL_TraceLine( mv->GetAbsOrigin() + m_pCFPlayer->GetViewOffset(), mv->GetAbsOrigin() + m_pCFPlayer->GetViewOffset() + vecDirection * flDistance,
					MASK_PLAYERSOLID_BRUSHONLY, m_pCFPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &tr);

	Vector vecEndPoint = tr.endpos;

	float flPlayerRadius = sv_collisionradius.GetFloat();

	// Don't jump if target is very close
	if ((vecEndPoint - mv->GetAbsOrigin()).Length() < flPlayerRadius + GAMEMOVEMENT_LATCH_DISTANCE - 1)
		return;

	// get a rough idea of how high to launch
	Vector vecMidPoint = mv->GetAbsOrigin() + (vecEndPoint - mv->GetAbsOrigin())/2;

	UTIL_TraceLine( vecMidPoint, vecMidPoint + Vector(0, 0, flHeight),
		MASK_PLAYERSOLID_BRUSHONLY, m_pCFPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &tr);

	vecMidPoint = vecMidPoint + (tr.endpos - vecMidPoint)/2;

	// How high should we travel to reach the apex
 	float distance1 = fabs(vecMidPoint.z - mv->GetAbsOrigin().z);
	float distance2 = fabs(vecMidPoint.z - vecEndPoint.z);

	// How long will it take to travel this distance
	float time1 = sqrt( distance1 / (flGravity/2) );
	float time2 = sqrt( distance2 / (flGravity/2) );
	if (time1+time2 < 0.1)
		return;

	// how hard to launch to get there in time.
	Vector vecTargetVel = (vecEndPoint - mv->GetAbsOrigin()) / (time1 + time2);
	vecTargetVel.z += flGravity * (time1+time2)/2;

	mv->m_vecVelocity = vecTargetVel;

	SetGroundEntity(NULL);
}

ConVar cf_latchstamina("cf_latchstamina", "0.15", FCVAR_DEVELOPMENTONLY);
void CCFGameMovement::Latch()
{
	int buttonsChanged	= ( mv->m_nOldButtons ^ mv->m_nButtons );	// These buttons have changed this frame
	int buttonsReleased	=  buttonsChanged & mv->m_nOldButtons;		// The changed ones which were previously down are "released"

	if (m_pCFPlayer->m_bLatched)
	{
#ifdef GAME_DLL
		// Don't lose your ability to slide just because you're out of stamina.
		if (m_pCFPlayer->m_pStats->m_flStamina > 0)
			m_pCFPlayer->m_pStats->m_flStamina -= cf_latchstamina.GetFloat();
#endif
	}

	if (m_pCFPlayer->m_bLatched && m_pCFPlayer->GetFlags() & FL_ONGROUND)
		m_pCFPlayer->StopLatch();
	else if (buttonsReleased & (IN_FORWARD|IN_BACK|IN_MOVELEFT|IN_MOVERIGHT) && m_pCFPlayer->m_bLatched)
		m_pCFPlayer->StopLatch();
	else if (buttonsReleased & IN_JUMP && m_pCFPlayer->m_bLatched)
	{
		m_pCFPlayer->DoAnimationEvent(PLAYERANIMEVENT_JUMP);

		m_pCFPlayer->StopLatch();

		float flJumpVelocity = CanFullLatch()?200:160;
		float flLadderVelocity = CanFullLatch()?300:260;

		//Clear this out here so that Friction() isn't run.
		SetGroundEntity( NULL );

#ifdef GAME_DLL
		// Just because we can't slide when we're out of stamina doesn't mean we can't jump.
		if (m_pCFPlayer->m_pStats->GetStamina() < 0)
			return;

		m_pCFPlayer->m_pStats->m_flStamina -= cf_powerjumpstamina.GetFloat()/2;
#endif

		QAngle angForward;
		Vector vecForward = m_vecForward;

		VectorAngles(m_vecForward, angForward);

		if (AngleNormalize(angForward.x) > 0)
		{
			angForward.x = 0;
			AngleVectors(angForward, &vecForward);
		}

		if (mv->m_nButtons & (IN_MOVERIGHT|IN_MOVELEFT|IN_FORWARD|IN_BACK))
		{
			//Jump as if on a ladder
			float forward = 0, right = 0;

			if ( mv->m_nButtons & IN_BACK )
				forward -= flLadderVelocity;
			if ( mv->m_nButtons & IN_FORWARD )
				forward += flLadderVelocity;
			if ( mv->m_nButtons & IN_MOVELEFT )
				right -= flLadderVelocity;
			if ( mv->m_nButtons & IN_MOVERIGHT )
				right += flLadderVelocity;

			if ( forward != 0 || right != 0 )
			{
				Vector vecCross, vecLateral;
				float flNormal;

				// Calculate player's intended velocity
				Vector vecVelocity = (forward * vecForward) + (right * m_vecRight);

				// If the player is trying to get away from the wall, pop him directly off the wall.
				if (DotProduct(vecVelocity, m_pCFPlayer->m_vecLatchPlaneNormal) > 0.2f)
				{
					JumpTo(m_pCFPlayer->m_vecLatchPlaneNormal, 80, flJumpVelocity);
					return;
				}

				// Parallel to the latch plane
				Vector vecParallel = CrossProduct( Vector(0,0,1), m_pCFPlayer->m_vecLatchPlaneNormal );

				// decompose velocity into ladder plane
				flNormal = DotProduct( vecVelocity, m_pCFPlayer->m_vecLatchPlaneNormal );
				// This is the velocity into the face of the ladder
				vecCross = m_pCFPlayer->m_vecLatchPlaneNormal * flNormal;

				// This is the player's additional velocity
				vecLateral = vecVelocity - vecCross;

				// This turns the velocity into the face of the ladder into velocity that
				// is roughly vertically perpendicular to the face of the ladder.
				// NOTE: It IS possible to face up and move down or face down and move up
				// because the velocity is a sum of the directional velocity and the converted
				// velocity through the face of the ladder -- by design.
				mv->m_vecVelocity = vecLateral - CrossProduct( m_pCFPlayer->m_vecLatchPlaneNormal, vecParallel ) * flNormal;
			}
		}
		else
		{
 			JumpTo(vecForward, 80, flJumpVelocity);
		}
	}
	else if (mv->m_nButtons & (IN_JUMP|IN_FORWARD|IN_BACK|IN_MOVELEFT|IN_MOVERIGHT) && !(m_pCFPlayer->GetFlags() & FL_ONGROUND))
	{
		bool bHit = false;
		Vector vecStart = mv->GetAbsOrigin();
		trace_t tr;

		if (mv->m_nButtons & IN_JUMP)
		{
			Vector vecTry[] =
			{
				Vector( GAMEMOVEMENT_LATCH_DISTANCE,  GAMEMOVEMENT_LATCH_DISTANCE, 0),
				Vector(-GAMEMOVEMENT_LATCH_DISTANCE, -GAMEMOVEMENT_LATCH_DISTANCE, 0),
				Vector( GAMEMOVEMENT_LATCH_DISTANCE, -GAMEMOVEMENT_LATCH_DISTANCE, 0),
				Vector(-GAMEMOVEMENT_LATCH_DISTANCE,  GAMEMOVEMENT_LATCH_DISTANCE, 0)
			};

			for (int i = 0; i < 4; i++)
			{
				UTIL_TraceHull(vecStart, vecStart + vecTry[i], VEC_HULL_MIN, VEC_HULL_MAX,
					MASK_SOLID_BRUSHONLY, m_pCFPlayer, COLLISION_GROUP_NONE, &tr );

				if (tr.fraction != 1.0)
				{
					bHit = true;
					break;
				}
			}
		}
		else if (gpGlobals->curtime - m_pCFPlayer->m_Local.m_flLastJump > 0.5f)
		{
			float forward = 0, right = 0;

			if ( mv->m_nButtons & IN_BACK )
				forward -= GAMEMOVEMENT_LATCH_DISTANCE;
			if ( mv->m_nButtons & IN_FORWARD )
				forward += GAMEMOVEMENT_LATCH_DISTANCE;
			if ( mv->m_nButtons & IN_MOVELEFT )
				right -= GAMEMOVEMENT_LATCH_DISTANCE;
			if ( mv->m_nButtons & IN_MOVERIGHT )
				right += GAMEMOVEMENT_LATCH_DISTANCE;

			Vector vecDirection = (forward * m_vecForward) + (right * m_vecRight);

			UTIL_TraceHull(vecStart, vecStart + vecDirection, VEC_HULL_MIN, VEC_HULL_MAX,
				MASK_SOLID_BRUSHONLY, m_pCFPlayer, COLLISION_GROUP_NONE, &tr );

			if (tr.fraction != 1.0f)
				bHit = true;
		}

		if (bHit && !m_pCFPlayer->m_bLatched)
			m_pCFPlayer->StartLatch(tr, tr.m_pEnt);
		else if (!bHit && m_pCFPlayer->m_bLatched)
			m_pCFPlayer->StopLatch();
	}

	if (mv->m_vecVelocity.z > 0 && !m_pCFPlayer->m_bLatched && gpGlobals->curtime < m_pCFPlayer->m_flLastLatch + LATCH_BREAK_TIME)
	{
		// Latch plane normal should be lying around still from the last plane we latched to.
		trace_t tr;
		UTIL_TraceHull(mv->GetAbsOrigin(), mv->GetAbsOrigin() - m_pCFPlayer->m_vecLatchPlaneNormal, VEC_HULL_MIN, VEC_HULL_MAX,
			MASK_SOLID_BRUSHONLY, m_pCFPlayer, COLLISION_GROUP_NONE, &tr );

		// So, if we were just latching, and now we have some clear area in front of us, nudge us in that direction so that we catch the lip of whatever wall we just latched up to.
		if (tr.fraction == 1.0f)
			mv->m_vecVelocity -= m_pCFPlayer->m_vecLatchPlaneNormal;
	}
}

void CCFGameMovement::PlayerRoughLandingEffects( float fvol )
{
	BaseClass::PlayerRoughLandingEffects( fvol );

	if ( fvol > 0.0 )
	{
		if (gpGlobals->curtime > m_pCFPlayer->m_flLastLandDustSpawn + 1)
		{
			m_pCFPlayer->m_flLastLandDustSpawn = gpGlobals->curtime;
			DispatchParticleEffect("land_dust", PATTACH_ABSORIGIN, player);
		}
	}
}

void CCFGameMovement::TracePlayerBBox( const Vector& vecStart, const Vector& vecEnd, unsigned int fMask, int collisionGroup, trace_t& pm )
{
	VPROF( "CCFGameMovement::TracePlayerBBox" );

	Vector vecHackedStart = vecStart;
	Vector vecHackedPlayerMins = GetPlayerMins();
	Vector vecHackedPlayerMaxs = GetPlayerMaxs();

#ifdef CLIENT_DLL
	UTIL_TraceHull(vecStart, vecEnd, GetPlayerMins(), GetPlayerMaxs(), fMask, mv->m_nPlayerHandle.Get(), collisionGroup, &pm);

	// If the start position is stuck in a wall, make ourselves a tiny bit smaller to see if network inaccuracies have made us get stuck.
	if (pm.startsolid)
	{
		vecHackedStart.z += DIST_EPSILON;
		vecHackedPlayerMins.x += DIST_EPSILON;
		vecHackedPlayerMins.y += DIST_EPSILON;
		vecHackedPlayerMins.z += DIST_EPSILON;
		vecHackedPlayerMaxs.x -= DIST_EPSILON;
		vecHackedPlayerMaxs.y -= DIST_EPSILON;
		vecHackedPlayerMaxs.z -= DIST_EPSILON;
	}
#endif

	Ray_t ray;
	ray.Init( vecHackedStart, vecEnd, vecHackedPlayerMins, vecHackedPlayerMaxs );
	UTIL_TraceRay( ray, fMask, mv->m_nPlayerHandle.Get(), collisionGroup, &pm );

	float flDistance;

	if (collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT)
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
			if ( !pPlayer )
				continue;

			CCFPlayer* pCFPlayer = ToCFPlayer(pPlayer);

			if (m_pCFPlayer == pCFPlayer)
				continue;

			if (CFGameRules()->PlayerRelationship(m_pCFPlayer, pCFPlayer) == GR_TEAMMATE)
				continue;

			if (pCFPlayer == m_pCFPlayer->GetRecursedTarget() && m_pCFPlayer->IsInFollowMode())
			{
				// Don't do this check if I'm in follow mode against this target.
				// I need to keep my distance no matter what the height difference,
				// so that the follow mode camera doesn't fuck up.
			}
			else
			{
				if (fabs(vecEnd.z - pPlayer->GetAbsOrigin().z) > GetPlayerMaxs().z - GetPlayerMins().z)
					continue;
			}

			if (!pCFPlayer->IsAlive())
				continue;

			if (m_pCFPlayer->ShouldLockFollowModeView() || pCFPlayer->IsInFollowMode())
				flDistance = sv_collisionradiusfollow.GetFloat();
			else
				flDistance = sv_collisionradius.GetFloat();

			float flDistanceSqr = flDistance*flDistance;

			// For brevity sake.
			Vector v0 = pPlayer->GetAbsOrigin();
			Vector v1 = vecStart;
			Vector v2 = vecEnd;

			// Zero out all the z's so they don't interfere.
			v0.z = 0;
			v1.z = 0;
			v2.z = 0;

			if ((v1 - v0).Length2DSqr() < flDistanceSqr)
			{
				if (DotProduct(v2-v1, v1-v0) < -0.01)
				{
					pm.fraction = 0;
					pm.endpos = v1;
					pm.hitgroup = 0;
					pm.hitbox = 0;
					pm.m_pEnt = pPlayer;

					Vector vecNormal = pm.endpos - v0;
					vecNormal.z = 0;
					vecNormal.NormalizeInPlace();

					pm.plane.normal = vecNormal;
					return;
				}
				else
				{
					continue;
				}
			}

			float a, b, c;
			float bb4ac;
			Vector vd = v2 - v1;
			a = vd.Length2DSqr();
			b = 2 * (vd.x * (v1.x - v0.x) + vd.y * (v1.y - v0.y));
			c = v0.Length2DSqr() + v1.Length2DSqr();
			c -= 2 * (v0.x * v1.x + v0.y * v1.y);
			c -= flDistance*flDistance;
			bb4ac = b * b - 4 * a * c;
			if (fabs(a) == 0 || bb4ac < 0)
				continue;

			float flFraction = (-b + sqrt(bb4ac)) / (2 * a);
			if (flFraction < pm.fraction && flFraction > 0 && flFraction < 1)
			{
				pm.fraction = flFraction;
				pm.endpos = vecStart + flFraction*(vecEnd-vecStart);
				pm.hitgroup = 0;
				pm.hitbox = 0;
				pm.m_pEnt = pPlayer;

				Vector vecNormal = pm.endpos - pPlayer->GetAbsOrigin();
				vecNormal.z = 0;
				vecNormal.NormalizeInPlace();

				pm.plane.normal = vecNormal;
				// I don't think pm.plane.dist will be used.
			}
		}
	}
}

void CCFGameMovement::CheckParameters( void )
{
	BaseClass::CheckParameters();

	if ( m_pCFPlayer->m_bLatched )
	{
		mv->m_flForwardMove = 0;
		mv->m_flSideMove    = 0;
		mv->m_flUpMove      = 0;
	}
}

bool CCFGameMovement::CanFullLatch()
{
	// Only the space bar can do a full latch. It's antithetical for movement keys to slow the player down.
	return m_pCFPlayer->GetActiveArmament()->CanFullLatch() && (mv->m_nButtons & IN_JUMP);
}

bool CCFGameMovement::CanAccelerate()
{
	if ( player->IsObserver() )
	{
		return true;
	}

	return BaseClass::CanAccelerate();
}
