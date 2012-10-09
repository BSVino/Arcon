//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"

#ifdef CLIENT_DLL
	
	#include "iclientvehicle.h"
	#include "c_cf_player.h"
	#include "fx_impact.h"
	#include "cf_in_main.h"
	#include "cfui_gui.h"
	#include "prediction.h"
	#include "c_objectives.h"

#else

	#include "iservervehicle.h"
	#include "cf_player.h"
	#include "te_firebullets.h"
	#include "npcevent.h"
	#include "objectives.h"

#endif

#include "gamevars_shared.h"
#include "takedamageinfo.h"
#include "effect_dispatch_data.h"
#include "engine/ivdebugoverlay.h"
#include "in_buttons.h"
#include "datacache/imdlcache.h"
#include "vprof.h"
#include "weapon_magic.h"
#include "armament.h"
#include "func_latch.h"
#include "movevars_shared.h"
#include "cf_gamerules.h"
#include "ammodef.h"

ConVar sv_showimpacts("sv_showimpacts", "0", FCVAR_CHEAT|FCVAR_REPLICATED, "Shows client (red) and server (blue) bullet impact point" );

ConVar mp_hpbonustime("mp_hpbonustime", "15", FCVAR_CHEAT|FCVAR_REPLICATED, "Amount of time the player can wait for his HP to refill while KO'd" );
ConVar mp_hpbonusamnt("mp_hpbonusamnt", "300", FCVAR_CHEAT|FCVAR_REPLICATED, "Amount of HP the player can regenerate by waiting around" );

ConVar mp_respawntimer("mp_respawntimer", "4", FCVAR_CHEAT|FCVAR_DEVELOPMENTONLY|FCVAR_REPLICATED, "How must a player wait to respawn?" );

ConVar cf_maxspeed( "cf_maxspeed", "240", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Players move at this speed." );
ConVar cf_aimsinspeed( "cf_aimsinspeed", "120", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Players aiming in with a firearm move at this speed." );
ConVar spec_speed( "spec_speed", "400", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Observers move at this speed." );

ConVar cam_fm_fov("cam_fm_fov", "65", 0);

void DispatchEffect( const char *pName, const CEffectData &data );

// This runs on both the client and the server.
// On the server, it only does the damage calculations.
// On the client, it does all the effects.
void CCFPlayer::FireBullets( 
	const Vector &vecOrigin,
	const QAngle &vecAngles,
	CFWeaponID	iWeaponID,
	int iSeed,
	float flSpread
	)
{
	char szWeaponName[512];
	Q_snprintf(szWeaponName,sizeof(szWeaponName), "weapon_%s", WeaponIDToAlias(iWeaponID));
	const CCFWeaponInfo &pWeaponInfo = *static_cast< const CCFWeaponInfo* >(GetFileWeaponInfoFromHandle( LookupWeaponInfoSlot(szWeaponName) ));

	if (IsAimingIn())
		flSpread /= 2;

#ifndef CLIENT_DLL
	// if this is server code, send the effect over to client as temp entity
	// Dispatch one message for all the bullet impacts and sounds.
	TE_FireBullets(
		entindex(),
		vecOrigin, 
		vecAngles, 
		iWeaponID,
		Primary_Mode,
		iSeed,
		flSpread
		);
#endif

	CWeaponCFBase* pWeapon = NULL;
	if (GetPrimaryWeapon() && GetPrimaryWeapon()->GetWeaponID() == iWeaponID)
		pWeapon = GetPrimaryWeapon();
	else if (GetSecondaryWeapon() && GetSecondaryWeapon()->GetWeaponID() == iWeaponID)
		pWeapon = GetSecondaryWeapon();

	if (!pWeapon)
		return;

	int iAmmoType = pWeaponInfo.iAmmoType;

	iSeed++;

#ifdef CLIENT_DLL
	pWeapon->WeaponSound( SINGLE );

	SetImpactSoundRoute( CWeaponCFBase::ShotgunImpactSoundGroup );

	int iDamage = pWeaponInfo.m_iDamage;
	element_t eElement = ELEMENT_TYPELESS;
	statuseffect_t eStatusEffect = STATUSEFFECT_NONE;
	float flStatusEffectMagnitude = 0;
#else
	char iWeapon = pWeapon->GetPosition();
	int iDamage = m_pStats->GetPhysicalAttackDamage(iWeapon, pWeaponInfo.m_iDamage);
	element_t eElement = m_pStats->GetPhysicalAttackElement(iWeapon);
	statuseffect_t eStatusEffect = m_pStats->GetPhysicalAttackStatusEffect(iWeapon);
	float flStatusEffectMagnitude = m_pStats->GetPhysicalAttackStatusEffectMagnitude(iWeapon);
#endif

#ifdef CLIENT_DLL
	Vector vecMuzzle;
	QAngle angMuzzle;
	pWeapon->GetAttachment("muzzle", vecMuzzle, angMuzzle);
	CPVSFilter filter(vecMuzzle);
	te->DynamicLight( filter, 0.0, &(vecMuzzle), 255, 192, 64, 5, 70, 0.05, 768 );
#endif

	for ( int iBullet=0; iBullet < pWeaponInfo.m_iBullets; iBullet++ )
	{
		RandomSeed( iSeed );	// init random system with this seed

		// Yes, I know this tends to cluster shots in the center
		float flRadius = RandomFloat( 0.0, 1.0 );
		float flTheta = RandomFloat( 0.0, 2*M_PI );

		float flX = flRadius*cos(flTheta);
		float flY = flRadius*sin(flTheta);

		iSeed++; // use new seed for next bullet

		FireBullet(
			vecOrigin,
			vecAngles,
			iWeaponID,
			flSpread,
			iDamage,
			iAmmoType,
			eElement,
			eStatusEffect,
			flStatusEffectMagnitude,
			this,
			flX, flY );
	}

#ifdef CLIENT_DLL
	SetImpactSoundRoute( NULL );
#endif
}

void CCFPlayer::FireBullet( 
						   Vector vecSrc,	// shooting postion
						   const QAngle &shootAngles,  //shooting angle
						   CFWeaponID iWeaponID,
						   float vecSpread, // spread vector
						   int iDamage, // base damage
						   int iBulletType, // ammo type
						   element_t eElements,
						   statuseffect_t eStatusEffects,
						   float flStatusEffectsMagnitude,
						   CBaseEntity *pevAttacker, // shooter
						   float x,	// spread x factor
						   float y	// spread y factor
						   )
{
	float fCurrentDamage = iDamage;   // damage of the bullet at it's current trajectory
	float flCurrentDistance = 0.0;  //distance that the bullet has traveled so far

	Vector vecDirShooting, vecRight, vecUp;
	AngleVectors( shootAngles, &vecDirShooting, &vecRight, &vecUp );

	if ( !pevAttacker )
		pevAttacker = this;  // the default attacker is ourselves

	// add the spray 
	Vector vecDir = vecDirShooting +
		x * vecSpread * vecRight +
		y * vecSpread * vecUp;

	VectorNormalize( vecDir );

	float flMaxRange = 8000;

	Vector vecEnd = vecSrc + vecDir * flMaxRange; // max bullet range is 10000 units
	Vector vecStart = vecSrc;
	CBaseEntity* pIgnore = this;
	float flRange = flMaxRange;

	while (fCurrentDamage > 0)
	{
		trace_t tr; // main enter bullet trace

		UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID|CONTENTS_DEBRIS|CONTENTS_HITBOX, pIgnore, COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction == 1.0f )
			return; // we didn't hit anything, stop tracing shoot

		//calculate the damage based on the distance the bullet travelled.
		flCurrentDistance += tr.fraction * flRange;

		// Reduce for range.
		float flDmgAfterRange = fCurrentDamage * pow ( 0.85f, (flCurrentDistance / 500));

		if (flDmgAfterRange <= 0)
			return;

		int iDamageType = GetAmmoDef()->DamageType(iBulletType) | DMG_NEVERGIB;

		ImpactEffects(tr, iDamageType, vecStart, vecDir, pIgnore, flRange);

		// add damage to entity that we hit

#ifdef GAME_DLL
		CDisablePredictionFiltering disabler;

		ClearMultiDamage();

		CTakeDamageInfo info( pevAttacker, pevAttacker, flDmgAfterRange, iDamageType, iWeaponID );

		info.AddElements( eElements );
		info.AddStatusEffects( eStatusEffects, flStatusEffectsMagnitude );

		CalculateBulletDamageForce( &info, iBulletType, vecDir, tr.endpos );
		tr.m_pEnt->DispatchTraceAttack( info, vecDir, &tr );

		TraceAttackToTriggers( info, tr.startpos, tr.endpos, vecDir );

		ApplyMultiDamage();
#endif

		pIgnore = tr.m_pEnt;

		Vector vecBackwards = tr.endpos + vecDir * (fCurrentDamage/5 + 10);
		Vector vecOldEndPos = tr.endpos;
		if (tr.m_pEnt->IsBSPModel())
			UTIL_TraceLine( vecBackwards, tr.endpos, MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr );
		else
			UTIL_TraceLine( vecBackwards, tr.endpos, CONTENTS_HITBOX, NULL, COLLISION_GROUP_NONE, &tr );

		if (tr.startsolid)
			return;

		// Reduce for bullets which pass through objects.
		fCurrentDamage -= (tr.endpos - vecOldEndPos).Length() * 5;

		// Set up the next trace.
		vecStart = tr.endpos + vecDir;	// One unit in the direction of fire so that we firmly embed ourselves in whatever solid was hit.
		flRange = (vecEnd - vecStart).Length();
	}
}

void CCFPlayer::FireDrainer( 
	const Vector &vecOrigin,
	const QAngle &vecAngles,
	CFWeaponID	iWeaponID,
	int iSeed,
	float flSpread
	)
{
	char szWeaponName[512];
	Q_snprintf(szWeaponName,sizeof(szWeaponName), "weapon_%s", WeaponIDToAlias(iWeaponID));
	const CCFWeaponInfo &pWeaponInfo = *static_cast< const CCFWeaponInfo* >(GetFileWeaponInfoFromHandle( LookupWeaponInfoSlot(szWeaponName) ));

#ifndef CLIENT_DLL
	// if this is server code, send the effect over to client as temp entity
	// Dispatch one message for all the bullet impacts and sounds.
	TE_FireBullets(
		entindex(),
		vecOrigin, 
		vecAngles, 
		iWeaponID,
		Secondary_Mode,
		iSeed,
		flSpread
		);
#endif

	CWeaponCFBase* pWeapon = NULL;
	if (GetPrimaryWeapon() && GetPrimaryWeapon()->GetWeaponID() == iWeaponID)
		pWeapon = GetPrimaryWeapon();
	else if (GetSecondaryWeapon() && GetSecondaryWeapon()->GetWeaponID() == iWeaponID)
		pWeapon = GetSecondaryWeapon();

	iSeed++;

	int iDamage = pWeaponInfo.m_iDrainDamage;
	
	if (pWeapon)
		iDamage = iDamage * pWeapon->m_iClip1 / pWeaponInfo.iMaxClip1;

#ifdef CLIENT_DLL
	if (pWeapon)
		pWeapon->WeaponSound( SINGLE );

	SetImpactSoundRoute( CWeaponCFBase::ShotgunImpactSoundGroup );
#endif

	RandomSeed( iSeed );	// init random system with this seed

	// Yes, I know this tends to cluster shots in the center
	float flRadius = RandomFloat( 0.0, 1.0 );
	float flTheta = RandomFloat( 0.0, 2*M_PI );

	float flX = flRadius*cos(flTheta);
	float flY = flRadius*sin(flTheta);

	Vector vecDirShooting, vecRight, vecUp;
	AngleVectors( vecAngles, &vecDirShooting, &vecRight, &vecUp );

	// add the spray 
	Vector vecDir = vecDirShooting +
		flX * flSpread * vecRight +
		flY * flSpread * vecUp;

	VectorNormalize( vecDir );

	float flMaxRange = 8000;

	Vector vecEnd = vecOrigin + vecDir * flMaxRange; // max bullet range is 10000 units
	Vector vecStart = vecOrigin;
	CBaseEntity* pIgnore = this;
	float flRange = flMaxRange;

	trace_t tr; // main enter bullet trace

	UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID|CONTENTS_DEBRIS|CONTENTS_HITBOX, pIgnore, COLLISION_GROUP_NONE, &tr );

	if ( IsClient() )
	{
		Vector vecMuzzle;
		QAngle angMuzzle;
		if (pWeapon)
			pWeapon->GetAttachment("muzzle", vecMuzzle, angMuzzle);
		else
			vecMuzzle = vecStart;
		DispatchParticleEffect("focusshot", vecMuzzle, tr.endpos, QAngle(0,0,0));
	}

	if ( tr.fraction == 1.0f )
		return; // we didn't hit anything, stop tracing shoot

	int iDamageType = DMG_BULLET | DMG_NEVERGIB;

	ImpactEffects(tr, iDamageType, vecStart, vecDir, pIgnore, flRange);

	if (tr.m_pEnt->IsPlayer())
	{
		// Don't need to actually drain any focus to learn the lesson,
		// just need to see the pretty focus drain graphic.
		Instructor_LessonLearned(HINT_RMB_FOCUS_BLAST);

#ifdef GAME_DLL
		float& flFocus = ToCFPlayer(tr.m_pEnt)->m_pStats->m_flFocus.GetForModify();
		if (flFocus > 0)
			flFocus -= iDamage;

		CDisablePredictionFiltering disabler;

		DispatchParticleEffect("focusdrained", PATTACH_ABSORIGIN_FOLLOW, tr.m_pEnt);
#endif
	}

#ifdef CLIENT_DLL
	SetImpactSoundRoute( NULL );
#endif
}

void CCFPlayer::ImpactEffects( trace_t& tr, int iDamageType, Vector vecStart, Vector vecDir, CBaseEntity* pIgnore, float flRange )
{
	if ( sv_showimpacts.GetBool() )
	{
#ifdef CLIENT_DLL
		// draw red client impact markers
		debugoverlay->AddBoxOverlay( tr.endpos, Vector(-2,-2,-2), Vector(2,2,2), QAngle( 0, 0, 0), 255,0,0,127, 4 );

		if ( tr.m_pEnt && tr.m_pEnt->IsPlayer() )
		{
			C_BasePlayer *player = ToBasePlayer( tr.m_pEnt );
			player->DrawClientHitboxes( 4, true );
		}
#else
		// draw blue server impact markers
		NDebugOverlay::Box( tr.endpos, Vector(-2,-2,-2), Vector(2,2,2), 0,0,255,127, 4 );

		if ( tr.m_pEnt && tr.m_pEnt->IsPlayer() )
		{
			CBasePlayer *player = ToBasePlayer( tr.m_pEnt );
			player->DrawServerHitboxes( 4, true );
		}
#endif
	}

	if( IsClient() )
	{
		// See if the bullet ended up underwater + started out of the water
		if ( enginetrace->GetPointContents( tr.endpos ) & (CONTENTS_WATER|CONTENTS_SLIME) )
		{	
			trace_t waterTrace;
			UTIL_TraceLine( vecStart, tr.endpos, (MASK_SHOT|CONTENTS_WATER|CONTENTS_SLIME), pIgnore, COLLISION_GROUP_NONE, &waterTrace );

			if( waterTrace.allsolid != 1 )
			{
				CEffectData	data;
				data.m_vOrigin = waterTrace.endpos;
				data.m_vNormal = waterTrace.plane.normal;
				data.m_flScale = random->RandomFloat( 8, 12 );

				if ( waterTrace.contents & CONTENTS_SLIME )
				{
					data.m_fFlags |= FX_WATER_IN_SLIME;
				}

				DispatchEffect( "gunshotsplash", data );
			}
		}
		else
		{
			//Do Regular hit effects

			// Don't decal nodraw surfaces
			if ( !( tr.surface.flags & (SURF_SKY|SURF_NODRAW|SURF_HINT|SURF_SKIP) ) )
			{
				CBaseEntity *pEntity = tr.m_pEnt;
				if ( !( !friendlyfire.GetBool() && pEntity && pEntity->IsPlayer() && CFGameRules()->PlayerRelationship(this, pEntity) == GR_TEAMMATE ) )
				{
					UTIL_ImpactTrace( &tr, iDamageType );

					if (tr.fractionleftsolid)
					{
						trace_t oppositeTrace;
						Vector vecBackwardsStart = vecStart + vecDir * (flRange * tr.fractionleftsolid + 2);
						UTIL_TraceLine( vecBackwardsStart, vecStart, MASK_SOLID|CONTENTS_DEBRIS|CONTENTS_HITBOX, NULL, COLLISION_GROUP_NONE, &oppositeTrace );
						UTIL_ImpactTrace( &oppositeTrace, iDamageType );
					}
				}
			}
		}
	}
}

void CCFPlayer::SharedSpawn()
{
	if (GetMagicWeapon())
		GetMagicWeapon()->Reset();

	m_flLastLatch = 0;

	m_flLastDashTime = 0;
	m_bWantVelocityMatched = false;

	m_bCanPowerjump = true;

	m_flRushDistance = 0;
	m_bDownStrike = false;

	BaseClass::SharedSpawn();

	m_iMeleeChain = 0;

	m_flSpawnTime = gpGlobals->curtime;

	m_flLastLandDustSpawn = 0;

	// Unfreeze player.
	FreezePlayer(1);

	m_hLastAttacker = NULL;
	m_hLastDOTAttacker = NULL;

	m_flStrongAttackJumpTime = 0;

	m_flFollowModeStarted = 0;

	if (GetTeamNumber() != TEAM_SPECTATOR)
	{
		if (!m_pCurrentArmament)
			m_pCurrentArmament = new CArmament();
		*m_pCurrentArmament = *m_pArmament;	//Copy

#ifdef CLIENT_DLL
		cfgui::CRootPanel::UpdateArmament(m_pCurrentArmament);
#endif

		m_pCurrentArmament->Buy(this);
	}

	m_iLastMovementButton = 0;
	m_flLastMovementButtonTime = 0;
}

void CCFPlayer::StartLatch(const trace_t &tr, CBaseEntity* pOther)
{
	Vector vecLatchPlaneNormal = tr.plane.normal;
	vecLatchPlaneNormal.NormalizeInPlace();

	float flDot = DotProduct(Vector(0, 0, 1), vecLatchPlaneNormal);

	if (IsInFullBodyAnimation())
		return;

	// This happens sometimes for some reason.
	if (vecLatchPlaneNormal.LengthSqr() == 0)
		return;

	//Anything that cannot be stood on without falling or sliding off can be latched to.
	if (flDot >= 0.7f)
		return;

	// No negative normals!
	if (flDot < 0.0f)
		return;

	if (tr.surface.flags & (SURF_SKY|SURF_NODRAW|SURF_HINT|SURF_SKIP))
		return;

	if (!pOther->IsLatchable())
		return;

	if (m_bLatched)
		return;

	if (CFuncLatch::HasLatches() && m_iLatchTriggerCount <= 0)
		return;

	if (gpGlobals->curtime < m_flLastLatch + LATCH_BREAK_TIME)
		return;

	// Only award the lesson learned if he is spamming the latch button and not treating it like a jump key.
	if (gpGlobals->curtime < m_flLastLatch + 0.5f)
		Instructor_LessonLearned(HINT_SPACE_LATCH);

	StopPowerjump();

	m_bLatched = true;

	DoAnimationEvent( PLAYERANIMEVENT_LATCH );

	m_vecLatchPlaneNormal = vecLatchPlaneNormal;
	VectorNormalize(m_vecLatchPlaneNormal.GetForModify());

	m_hLatchEntity = pOther;
	//SetGroundEntity occurs in CGameMovement::CategorizePosition()

	EmitSound( "Player.Latch" );
	m_bLatchSoundPlaying = true;
}

void CCFPlayer::StopLatch()
{
	if (!m_bLatched)
		return;

	m_bLatched = false;
	m_flLastLatch = gpGlobals->curtime;
	m_Local.m_flLastJump = gpGlobals->curtime;

	m_hLatchEntity = NULL;

	StopSound( "Player.Latch" );
	m_bLatchSoundPlaying = false;
}

void CCFPlayer::LatchThink()
{
	if (m_bLatchSoundPlaying && GetLocalVelocity().LengthSqr() < 1)
	{
		StopSound( "Player.Latch" );
		m_bLatchSoundPlaying = false;
	}
	else if (m_bLatched && !m_bLatchSoundPlaying && GetLocalVelocity().LengthSqr() > 1)
	{
		EmitSound( "Player.Latch" );
		m_bLatchSoundPlaying = true;
	}

#ifdef CLIENT_DLL
	float flSpeed = GetLocalVelocity().Length();
	float bFastEnough = flSpeed > 10;
	if (m_bLatched && bFastEnough && !m_apLatchEffects.Count())
	{
		KeyValues* pKV = GetSequenceKeyValues(GetSequence());

		if (pKV)
		{
			KeyValues* pDebrisKV = pKV->FindKey("debris");
			KeyValues* pAppendageKV = pDebrisKV->GetFirstSubKey();
			while (pAppendageKV)
			{
				if (pAppendageKV->GetInt())
				{
					m_apLatchEffects.AddToTail(ParticleProp()->Create( "latch_debris", PATTACH_POINT_FOLLOW, pAppendageKV->GetName() ));
				}

				pAppendageKV = pAppendageKV->GetNextKey();
			}
		}
	}
	else if ((!m_bLatched || !bFastEnough) && m_apLatchEffects.Count())
	{
		for (int i = 0; i < m_apLatchEffects.Count(); i++)
			ParticleProp()->StopEmission(m_apLatchEffects[i]);

		m_apLatchEffects.RemoveAll();
	}
#endif
}

void CCFPlayer::StopPowerjump()
{
	m_bPowerjump = false;
	m_bChargejump = false;
	SetGravity(1);
}

void CCFPlayer::ItemPreFrame()
{
	// Handle use events
	PlayerUse();

	if ( GetPrimaryWeapon() )
	{
#if defined( CLIENT_DLL )
		// Not predicting this weapon
		if ( GetPrimaryWeapon()->IsPredicted() )
#endif
			GetPrimaryWeapon()->ItemPreFrame( );
	}

	if ( GetSecondaryWeapon() )
	{
#if defined( CLIENT_DLL )
		// Not predicting this weapon
		if ( GetSecondaryWeapon()->IsPredicted() )
#endif
			GetSecondaryWeapon()->ItemPreFrame( );
	}

	CBaseCombatWeapon *pWeapon;

	// Allow all the holstered weapons to update
	for ( int i = 0; i < 3; ++i )
	{
		pWeapon = GetWeapon( i );

		if ( pWeapon == NULL )
			continue;

		if ( GetPrimaryWeapon() == pWeapon )
			continue;

		if ( GetSecondaryWeapon() == pWeapon )
			continue;

		pWeapon->ItemHolsterFrame();
	}
}

void CCFPlayer::ItemPostFrame()
{
	VPROF( "CCFPlayer::ItemPostFrame" );

	// Don't process items while in a vehicle.
	if ( GetVehicle() )
	{
#if defined( CLIENT_DLL )
		IClientVehicle *pVehicle = GetVehicle();
#else
		IServerVehicle *pVehicle = GetVehicle();
#endif

		bool bUsingStandardWeapons = UsingStandardWeaponsInVehicle();

#if defined( CLIENT_DLL )
		if ( pVehicle->IsPredicted() )
#endif
		{
			pVehicle->ItemPostFrame( this );
		}

		if (!bUsingStandardWeapons || !GetVehicle())
			return;
	}


	// check if the player is using something
	if ( GetUseEntity() )
	{
#if !defined( CLIENT_DLL )
		Assert( !IsInAVehicle() );
		ImpulseCommands();// this will call playerUse
#endif
		return;
	}

	if ( GetPrimaryWeapon() )
	{
#if defined( CLIENT_DLL )
		// Not predicting this weapon
		if ( GetPrimaryWeapon()->IsPredicted() )
#endif
			GetPrimaryWeapon()->ItemPostFrame( );
	}

	if ( GetSecondaryWeapon() )
	{
#if defined( CLIENT_DLL )
		// Not predicting this weapon
		if ( GetSecondaryWeapon()->IsPredicted() )
#endif
			GetSecondaryWeapon()->ItemPostFrame( );
	}

	GetMagicWeapon()->ItemPostFrame( );

#if !defined( CLIENT_DLL )
	ImpulseCommands();
#else
	// NOTE: If we ever support full impulse commands on the client,
	// remove this line and call ImpulseCommands instead.
	ResetImpulse();
#endif
}

void CCFPlayer::UpdateStepSound( surfacedata_t *psurface, const Vector &vecOrigin, const Vector &vecVelocity )
{
	// Don't play footsteps here, they are played as animation events.
}

void CCFPlayer::PlayStepSound( Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force )
{
	BaseClass::PlayStepSound(vecOrigin, psurface, fvol, force);
}

// GOOSEMAN : Kick the view..
void CCFPlayer::Recoil(float flPermanentRecoil, float flTemporaryRecoil)
{
#ifdef CLIENT_DLL
	if ( !prediction->IsFirstTimePredicted() )
		return;
#endif

	if (IsInFollowMode())
	{
		flPermanentRecoil /= 2;
		flTemporaryRecoil /= 2;
	}

	float flKickUp;
	float flKickLateral;

	float flUpBase = flTemporaryRecoil;
	float flUpModifier = flTemporaryRecoil/3;
	float flUpMax = flTemporaryRecoil*4;
	float flLateralBase = flUpBase/2;
	float flLateralModifier = flUpModifier/10;
	float flLateralMax = flUpMax/2;

	if (m_iShotsFired == 1) // This is the first round fired
	{
		flKickUp = flUpBase;
		flKickLateral = flLateralBase;
	}
	else
	{
		flKickUp = flUpBase + m_iShotsFired*flUpModifier;
		flKickLateral = flLateralBase + m_iShotsFired*flLateralModifier;
	}


	QAngle angle = GetPunchAngle();

	angle.x -= flKickUp;
	if ( angle.x < -1 * flUpMax )
		angle.x = -1 * flUpMax;
	
	if ( m_iDirection == 1 )
	{
		angle.y += flKickLateral;
		if (angle.y > flLateralMax)
			angle.y = flLateralMax;
	}
	else
	{
		angle.y -= flKickLateral;
		if ( angle.y < -1 * flLateralMax )
			angle.y = -1 * flLateralMax;
	}

	if ( !SharedRandomInt( "KickBack", 0, 5 ) )
		m_iDirection = 1 - m_iDirection;

	SetPunchAngle( angle );

#ifdef CLIENT_DLL
	float flPitchRecoil = flPermanentRecoil;
	float flYawRecoil = flPitchRecoil / 4;

	SetRecoilAmount( flPitchRecoil, flYawRecoil );
#endif
}

void CCFPlayer::SharedTouch( CBaseEntity *pOther )
{
	SetGravity(1);
}

void CCFPlayer::Rush()
{
	if (!GetPrimaryWeapon())
		return;

	if (!GetPrimaryWeapon()->IsMeleeWeapon())
		return;

	Rush(dynamic_cast<CCFBaseMeleeWeapon*>(GetPrimaryWeapon()), 200, 0);
}

void CCFPlayer::Rush(CCFBaseMeleeWeapon* pWeapon, float flDistance, int iAttack)
{
	DoAnimationEvent( PLAYERANIMEVENT_RUSH, iAttack );

	m_hRushingWeapon = pWeapon;
	m_flRushDistance = flDistance;
	m_bStrongAttackJump = false;
}

void CCFPlayer::EndRush()
{
	if (m_hRushingWeapon != NULL)
	{
		// Don't do an extra swing at the bottom when doing down strikes.
		if (!m_bDownStrike && !m_bStrongAttackJump)
			m_hRushingWeapon->SetSwingTime(gpGlobals->curtime);
		m_hRushingWeapon->EndRush();
	}

	m_hRushingWeapon = NULL;
	m_flRushDistance = 0;

	if (m_bStrongAttackJump)
		SuspendGravity(0.3f);
}

Vector CCFPlayer::EyePosition( )
{
	if (IsObserver() || !IsAlive())
		return BaseClass::EyePosition();

	MDLCACHE_CRITICAL_SECTION();
	int iBone = LookupBone( "Valvebiped.Eye_Camera" );
	Vector vecPos;
	QAngle angPos;

#if CLIENT_DLL
	if (IsBoneAccessAllowed())
	{
		SetupBones( NULL, -1, BONE_USED_BY_BONE_MERGE, gpGlobals->curtime );
		MatrixAngles(GetBone( iBone ), angPos, vecPos);

		trace_t trace;
		CTraceFilterNoNPCsOrPlayer traceFilter( this, COLLISION_GROUP_NONE );
		UTIL_TraceHull(GetAbsOrigin() + GetViewOffset(), vecPos, Vector(-5,-5,-5), Vector(5,5,5), MASK_SOLID, &traceFilter, &trace );

		vecPos = trace.endpos;
#else
		GetBonePosition(iBone, vecPos, angPos);
#endif

		return vecPos;
#if CLIENT_DLL
	}
	else
		return BaseClass::EyePosition();
#endif
}

void CCFPlayer::CalcPlayerView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov )
{
#ifdef CLIENT_DLL
	if (ShouldLockFollowModeView())
		CalcFollowModeView( eyeOrigin, eyeAngles, fov );
	else
#endif
		BaseClass::CalcPlayerView( eyeOrigin, eyeAngles, fov );

	if (m_flFreezeRotation == 0 || gpGlobals->curtime < m_flFreezeRotation)
	{
#ifdef CLIENT_DLL
		SetLocalViewAngles( m_angFreeze );
		engine->SetViewAngles( m_angFreeze );
#endif
		eyeAngles = m_angFreeze;
	}
}

void CCFPlayer::SetAnimation( PLAYER_ANIM playerAnim )
{
	// Do nothing. CMultiPlayerAnimState takes care of animation.
}

bool CCFPlayer::IsInFullBodyAnimation( )
{
	return m_PlayerAnimState->IsInFullBodyAnimation();
}

bool CCFPlayer::ShouldUseForwardFacingAnimationStyle()
{
	if (IsInFollowMode())
		return true;

#ifdef CLIENT_DLL
	CCFPlayer* pLocal = CCFPlayer::GetLocalCFPlayer();

	// If the player is in first person, third person body movements jerk the weapon model around a lot,
	// so use forward facing animations to reduce this.
	if (this == pLocal && !CFInput()->CAM_IsThirdPerson())
		return true;

	if (pLocal && pLocal->IsFirstPersonSpectating(this))
		return true;
#endif

	return false;
}

// What a monstrous hack. I love it.
#ifdef CLIENT_DLL
void CCFPlayer::FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options )
{
	int iEvent = event;
	const char* pszOptions = options;
#else
void CCFPlayer::HandleAnimEvent( animevent_t *pEvent )
{
	int iEvent = pEvent->event;
	const char* pszOptions = pEvent->options;
#endif

	switch (iEvent)
	{
	case AE_CF_ENDAFFLICT:
		if (FStrEq(pszOptions, "primary") && GetPrimaryWeapon() && GetPrimaryWeapon()->IsMeleeWeapon())
			GetPrimaryWeapon()->StopMeleeAttack(false, false);
		else if (FStrEq(pszOptions, "secondary") && GetSecondaryWeapon() && GetSecondaryWeapon()->IsMeleeWeapon())
			GetSecondaryWeapon()->StopMeleeAttack(false, false);
		break;

#ifdef GAME_DLL
	case AE_CF_ENDFATALITY:
		if (m_hReviver)
			m_hReviver->StopFatality(true);
		break;

	case AE_CF_DECAPITATE:
		Decapitate();

		if (m_hReviver)
			m_hReviver->StopFatality(true);
		break;
#endif

	case AE_CF_STEP_LEFT:
	case AE_CF_STEP_RIGHT:
	{
		// Don't play footsteps if we're walking.
		if (m_nButtons & IN_WALK)
			break;

		if (IsAimingIn())
			break;

		Vector vecFootPosition;
		QAngle angWhatever;
		int iFootAttachment = LookupAttachment( pszOptions );

		if( iFootAttachment == -1 )
			break;

		GetAttachment( iFootAttachment, vecFootPosition, angWhatever );

		trace_t tr;
		UTIL_TraceLine( vecFootPosition, vecFootPosition - Vector(0,0,48.0f), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr );
		if( tr.fraction < 1.0 && tr.m_pEnt )
		{
			surfacedata_t *psurf = physprops->GetSurfaceData( tr.surface.surfaceProps );
			if( psurf )
				PlayStepSound( vecFootPosition, psurf, VOL_NORM * 0.5f, false );
		}
		break;
	}

	default:
#ifdef CLIENT_DLL
		BaseClass::FireEvent(origin, angles, event, options);
#else
		BaseClass::HandleAnimEvent(pEvent);
#endif
	}
}

void CCFPlayer::FollowMode()
{
#ifdef GAME_DLL
	bool bWantsFollowMode = CanFollowMode();
	bool bInFollowMode;

	if (bWantsFollowMode && !m_bInFollowMode)
	{
		bInFollowMode = true;

		// Don't auto-find new targets if this is an auto-follow mode due to swinging or watev.
		if (!m_flAutoFollowModeEnds)
			FindTarget(true, true);
	}
	else
		bInFollowMode = IsInFollowMode();

#else
	bool bInFollowMode = m_bInFollowMode;
#endif

#ifdef GAME_DLL
	float flCamSwitchTime;
	if (IsBot())
		flCamSwitchTime = 0.2;
	else
		flCamSwitchTime = atof(engine->GetClientConVarValue( entindex(), "cam_switchtime" )) + 0.2f;
#endif

	if (bInFollowMode != m_bOldInFollowMode)
	{
		if (bInFollowMode)
		{
			m_flFollowModeStarted = gpGlobals->curtime;

#ifdef GAME_DLL
			if (GetRecursedTarget() && CFGameRules()->PlayerRelationship(GetRecursedTarget(), this) == GR_NOTTEAMMATE)
				m_hFollowModeTarget = GetRecursedTarget();
			else
				m_hFollowModeTarget = NULL;
			SetFOV(this, cam_fm_fov.GetInt(), flCamSwitchTime);
#endif
		}
		else
		{
#ifdef GAME_DLL
			// Only train on the follow mode skill if the player held follow mode for a nontrivial amount of time.
			if (gpGlobals->curtime > m_flAutoFollowModeEnds && gpGlobals->curtime - m_flFollowModeStarted > 0.5f)
			{
				if (GetPrimaryWeapon())
				{
					if (GetPrimaryWeapon()->IsMeleeWeapon())
						Instructor_LessonLearned(HINT_RMB_FOLLOW, true);
					else
						Instructor_LessonLearned(HINT_SHIFT_AIMIN, true);
				}
			}
#endif

#ifdef GAME_DLL
			m_hFollowModeTarget = NULL;
			SetFOV(this, 0, flCamSwitchTime);
#endif
		}

		m_bOldInFollowMode = m_bInFollowMode = bInFollowMode;
		CalculateMovementSpeed();
	}

	if (!m_bInFollowMode)
		return;

#ifdef GAME_DLL
	// If we are in follow mode and don't have a target, maybe we lost them or
	// maybe one will jump into the picture soon, so be on constant lookout.
	if (!GetRecursedTarget() || CFGameRules()->PlayerRelationship(GetRecursedTarget(), this) == GR_TEAMMATE)
		FindTarget(true, true);

	// Once we pick up a target in follow mode, remember him so that we only
	// pick him up.
	if (GetRecursedTarget() && m_hFollowModeTarget == NULL && CFGameRules()->PlayerRelationship(GetRecursedTarget(), this) == GR_NOTTEAMMATE)
		m_hFollowModeTarget = GetRecursedTarget();

	// If our follow mode target has died, then we don't care about him anymore
	// and we can pick up someone else, no problem.
	if (m_hFollowModeTarget != NULL && !m_hFollowModeTarget->CanBeTargeted())
		m_hFollowModeTarget = NULL;
#endif

	// Target locking follow mode.
	CCFPlayer* pTarget = GetRecursedTarget();

	if (!pTarget)
		return;

	Vector vecDir = pTarget->WorldSpaceCenter() - EyePosition();

	if (ShouldLockFollowModeView())
		// Update the current command with our view angles.
		VectorAngles( vecDir, m_pCurrentCommand->viewangles );

	// This is kind of a hack to keep the player facing towards his enemy during full-body animations like sword swinging.
	if (IsInFullBodyAnimation())
		SetLocalAngles(m_pCurrentCommand->viewangles);
}

bool CCFPlayer::IsInFollowMode()
{
#ifdef CLIENT_DLL
	return m_bInFollowMode;
#else
	if (CanFollowMode())
		return true;
	else
		return false;
#endif
}

bool CCFPlayer::IsAimingIn()
{
	return IsInFollowMode() && GetPrimaryWeapon() && !GetPrimaryWeapon()->IsMeleeWeapon();
}

bool CCFPlayer::ShouldLockFollowModeView()
{
	if (!GetPrimaryWeapon() || !GetPrimaryWeapon()->IsMeleeWeapon())
		return false;

	return IsInFollowMode() && GetRecursedTarget() && CFGameRules()->PlayerRelationship(this, GetRecursedTarget()) == GR_NOTTEAMMATE;
}

bool CCFPlayer::CanDownStrike(bool bAllowNoFollowMode)
{
	// If I'm in follow mode in the air with an enemy targeted below me, I can do a downward strike.

	if (!IsInFollowMode() && !bAllowNoFollowMode)
		return false;

	if (!GetRecursedTarget())
		return false;

	if (CFGameRules()->PlayerRelationship(this, GetRecursedTarget()) == GR_TEAMMATE)
		return false;

	if (GetGroundEntity())
		return false;

	float flHeightAboveTarget = GetAbsOrigin().z - GetRecursedTarget()->GetAbsOrigin().z;
	if (flHeightAboveTarget < 60)
		return false;

	if (((GetAbsOrigin() - GetRecursedTarget()->GetAbsOrigin()).Length2D() / flHeightAboveTarget) > 0.5f)
		return false;

	if (!GetPrimaryWeapon())
		return false;

	if (!GetPrimaryWeapon()->IsMeleeWeapon())
		return false;

	if (GetPrimaryWeapon()->IsDisableFlagged())
		return false;

	if (IsMagicMode())
		return false;

	if (m_bDownStrike)
		return false;

	return true;
}

void CCFPlayer::StartGroundContact( CBaseEntity *ground )
{
	BaseClass::StartGroundContact(ground);
	m_iAirMeleeAttacks = 0;
}

void CCFPlayer::EndGroundContact( CBaseEntity *ground )
{
	BaseClass::EndGroundContact(ground);
	m_Local.m_flLastJump = gpGlobals->curtime;
}

CArmament* CCFPlayer::GetActiveArmament()
{
	if (IsPariah())
		return CArmament::GetPariahArmament();

	if (m_pCurrentArmament && (IsAlive() || IsKnockedOut()))
		return m_pCurrentArmament;
	else
		return m_pArmament;
}

CBaseCombatWeapon *CCFPlayer::GetActiveWeapon() const
{
	AssertMsg(false, "CCFPlayer::GetActiveWeapon() call not allowed!");
	return NULL;
}

bool CCFPlayer::ShouldUseAlternateWeaponSet() const
{
	return IsPariah() || HasObjective();
}

bool CCFPlayer::HasDualMelee() const
{
	return GetPrimaryWeapon() && GetPrimaryWeapon()->IsMeleeWeapon() && GetSecondaryWeapon() && GetSecondaryWeapon()->IsMeleeWeapon();
}

bool CCFPlayer::HasAFirearm() const
{
	return (GetPrimaryWeapon() && !GetPrimaryWeapon()->IsMeleeWeapon()) || (GetSecondaryWeapon() && !GetSecondaryWeapon()->IsMeleeWeapon());
}

bool CCFPlayer::HasAMelee() const
{
	return (GetPrimaryWeapon() && GetPrimaryWeapon()->IsMeleeWeapon()) || (GetSecondaryWeapon() && GetSecondaryWeapon()->IsMeleeWeapon());
}

bool CCFPlayer::HasAMagicCombo() const
{
	return m_pCurrentArmament->HasBindableCombo();
}

void CCFPlayer::SetNextAttack(float flNextAttack)
{
	// If we have two firearms on us, don't bother setting this so that we can fire both weapons at once.
	if (GetPrimaryWeapon() && !GetPrimaryWeapon()->IsMeleeWeapon() && GetSecondaryWeapon() && !GetSecondaryWeapon()->IsMeleeWeapon())
		return;

	m_flNextAttack = flNextAttack;
}

CWeaponCFBase* CCFPlayer::GetWeapon(int i) const
{
	if (i < 0 || i >= 3)
		return NULL;

	if (ShouldUseAlternateWeaponSet())
		return m_hAlternateWeapons[i];
	else
		return m_hWeapons[i];
}

void CCFPlayer::SetWeapon(int i, CWeaponCFBase* pWeapon)
{
#if GAME_DLL
	if (ShouldUseAlternateWeaponSet())
		m_hAlternateWeapons.Set(i, pWeapon);
	else
		m_hWeapons.Set(i, pWeapon);
#else
	AssertMsg(false, "Can't SetWeapon on client.");
#endif
}

CWeaponCFBase* CCFPlayer::GetPrimaryWeapon(bool bFallback) const
{
	if (GetWeapon(0))
		return GetWeapon(0);

	// If the caller asks for it (by default) and the user has two secondaries and no primary,
	// the first secondary can pretend it is a primary.
	if (bFallback && ShouldPromoteSecondary())
		return GetWeapon(SecondaryToPromote());
	else
		return NULL;
}

CWeaponCFBase* CCFPlayer::GetSecondaryWeapon(bool bFallback) const
{
	// If we've promoted the first return the second one if it's active.
	if (bFallback && ShouldPromoteSecondary())
	{
		if (m_iActiveSecondary == SecondaryToPromote(true))
			return GetWeapon(SecondaryToPromote(true));
		else
			return NULL;
	}

	return GetWeapon(m_iActiveSecondary);
}

bool CCFPlayer::ShouldPromoteSecondary() const
{
	// If there is no primary, a secondary gets promoted.
	if (!GetWeapon(0) && (GetWeapon(1) || GetWeapon(2)))
		return true;
	else
		return false;
}

int CCFPlayer::SecondaryToPromote(bool bOther) const
{
	Assert(ShouldPromoteSecondary());

	// If there is only one secondary, 
	if (GetWeapon(1) && !GetWeapon(2))
		return (!bOther)?1:2;
	if (GetWeapon(2) && !GetWeapon(1))
		return (!bOther)?2:1;

	// If the second secondary has a higher weight than the first, promote it.
	if (GetWeapon(2)->GetWpnData().iWeight > GetWeapon(1)->GetWpnData().iWeight)
		return (!bOther)?2:1;
	else
		return (!bOther)?1:2;
}

bool CCFPlayer::Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex ) 
{
	MDLCACHE_CRITICAL_SECTION();

	if (pWeapon == GetPrimaryWeapon())
	{
		// Skip CBasePlayer
		return CBaseCombatCharacter::Weapon_Switch( pWeapon, viewmodelindex );
	}

	if ( pWeapon == NULL )
	{
		if (GetSecondaryWeapon())
			return GetSecondaryWeapon()->Holster( pWeapon );
		else
			return false;
	}

	// Already have it out?
	if ( GetSecondaryWeapon() == pWeapon )
	{
		if ( !GetSecondaryWeapon()->IsActive() )
			return GetSecondaryWeapon()->Deploy( );
		return false;
	}

	if (!pWeapon->CanDeploy())
	{
		return false;
	}

	if ( GetSecondaryWeapon() )
	{
		if ( !GetSecondaryWeapon()->Holster( pWeapon ) )
			return false;
	}

	return pWeapon->Deploy( );
}

CWeaponCFBase* CCFPlayer::GetInactiveSecondaryWeapon(int* pIndex) const
{
	int iIndex;
	if (ShouldPromoteSecondary())
	{
		if (m_iActiveSecondary == SecondaryToPromote(true))
		{
			// If we're currently on the non-promoted secondary, then switch out to nothing.
			iIndex = SecondaryToPromote();	// m_iActiveSecondary == SecondaryToPromote() means the secondary is nothing.
			return NULL;
		}
		else
			// If we're not on the non-promoted secondary, then bring it out.
			iIndex = SecondaryToPromote(true);
	}
	else
	{
		if (m_iActiveSecondary == 1)
			iIndex = 2;
		else
			iIndex = 1;
	}

	if (pIndex)
		*pIndex = iIndex;

	return GetWeapon(iIndex);
}

void CCFPlayer::Weapon_SwitchSecondaries() 
{
	if (HasObjective())
	{
#ifdef GAME_DLL
		GetObjective()->Drop();
#endif
		return;
	}

	int iNewSecondary;
	if (Weapon_Switch(GetInactiveSecondaryWeapon(&iNewSecondary)))
		m_iActiveSecondary = iNewSecondary;

	Instructor_LessonLearned(HINT_Q_SWITCH_WEAPONS);
}

bool CCFPlayer::IsPhysicalMode()
{
#ifdef CLIENT_DLL
	C_CFPlayer* pLocal = C_CFPlayer::GetLocalCFPlayer();
	if (this != pLocal)
		return m_bPhysicalMode;
#endif

	if (HasObjective())
		return true;

	bool bHasNumen = (GetActiveArmament()->m_aAttackBinds[0].m_iWeapon != -1) || (GetActiveArmament()->m_aAttackBinds[1].m_iWeapon != -1);

	if (!GetPrimaryWeapon() && bHasNumen)
		return false;

	if (!bHasNumen)
		return true;

	return !(m_nButtons & IN_ALT2);
}

char* CCFPlayer::GetCFModelName()
{
	if ( IsPariah() )
		return PLAYER_MODEL_PARIAH;
	else
		return (GetTeamNumber() == TEAM_NUMENI) ? PLAYER_MODEL_NUMENI : PLAYER_MODEL_MACHINDO;
}

void CCFPlayer::FreezePlayer(float flAmount, float flTime)
{
	m_flFreezeAmount = flAmount;

	if (m_pStats->IsInOverdrive())
		m_flFreezeAmount = RemapVal(m_flFreezeAmount, 0, 1, 0.5, 1);

	if (flAmount == 1.0f)
		m_flFreezeUntil = gpGlobals->curtime;
	else if (flTime < 0)
		m_flFreezeUntil = 0;
	else
		m_flFreezeUntil = gpGlobals->curtime + flTime;
}

bool CCFPlayer::PlayerFrozen(float flPadFrozenTime)
{
	// m_flFreezeUntil == 0 means to freeze for an indefinite amount of time.
	// Otherwise it means freeze until curtime >= m_flFreezeUntil.
	return (m_flFreezeUntil == 0) || (gpGlobals->curtime < m_flFreezeUntil + flPadFrozenTime);
}

int CCFPlayer::AdjustFrozenButtons(int nButtons)
{
	// Sometimes we really want to use buttons that the FreezePlayer() function has limited.
	// FreezePlayer() is used for the melee logic, so it doesn't really take gun and magic
	// users into consideration. This function restores the buttons for non-melee events.
	// It must be done here instead of in FreezePlayer() because the player may switch 
	// weapons or something in an attempt to circumvent the limitation.

	if (IsPhysicalMode())
	{
		// Allow shooting of the primary weapon if it is a gun.
		if (GetPrimaryWeapon() && !GetPrimaryWeapon()->IsMeleeWeapon())
			nButtons |= GetPrimaryWeapon()->PrimaryButtons();

		// Allow shooting of the secondary weapon if it is a gun.
		if (GetSecondaryWeapon() && !GetSecondaryWeapon()->IsMeleeWeapon())
			nButtons |= GetSecondaryWeapon()->PrimaryButtons();
	}
	else
	{
		// Never block these buttons in magic mode, always allow magic casting.
		nButtons |= (IN_ATTACK|IN_ATTACK2);
	}

	return nButtons;
}

void CCFPlayer::FreezeRotation(float flAmount, float flTime)
{
	m_flFreezeRAmount = flAmount;
	m_angFreeze = m_angEyeAngles;

	if (flAmount == 1.0f)
		m_flFreezeRotation = gpGlobals->curtime;
	else if (flTime < 0)
		m_flFreezeRotation = 0;
	else
		m_flFreezeRotation = gpGlobals->curtime + flTime;
}

void CCFPlayer::EnemyFrozen(CCFPlayer* pPlayer, float flTime)
{
	m_hEnemyFrozen = pPlayer;
	m_flEnemyFrozenUntil = gpGlobals->curtime + flTime;
}

float CCFPlayer::GetEnemyFrozenUntil()
{
	return m_flEnemyFrozenUntil;
}

CCFPlayer* CCFPlayer::GetEnemyFrozen()
{
	return m_hEnemyFrozen;
}

void CCFPlayer::SuspendGravity(float flTime, float flAt)
{
	if (flTime < 0)
		m_flSuspendGravityUntil = 0;
	else
	{
		m_flSuspendGravityAt = gpGlobals->curtime + flAt;

		m_flSuspendGravityUntil = m_flSuspendGravityAt + flTime;
	}
}

bool CCFPlayer::IsGravitySuspended() const
{
	return gpGlobals->curtime > m_flSuspendGravityAt && gpGlobals->curtime < m_flSuspendGravityUntil;
}

float CCFPlayer::GetGravity() const
{
	if (IsGravitySuspended())
		return 0;

	if (gpGlobals->curtime < m_flSuspendGravityAt)
		return 1;

	return RemapValClamped(gpGlobals->curtime - m_flSuspendGravityUntil, 0, 1, 0, 1);
}

void CCFPlayer::CalculateMovementSpeed()
{
	// Spectators can move while in Classic Observer mode
	if ( IsObserver() )
	{
		if ( GetObserverMode() == OBS_MODE_ROAMING )
			SetMaxSpeed( spec_speed.GetFloat() );
		else
			SetMaxSpeed( 0 );
		return;
	}

	// Check for any reason why they can't move at all
	if ( GameRules()->InRoundRestart() )
	{
		SetAbsVelocity( vec3_origin );
		SetMaxSpeed( 1 );
		return;
	}

	// First, get their max class speed
	float flMaxSpeed = cf_maxspeed.GetFloat();

	if (IsAimingIn())
		flMaxSpeed = cf_aimsinspeed.GetFloat();

	flMaxSpeed *= m_pStats->GetSpeedScale();

	// Set the speed
	SetMaxSpeed( flMaxSpeed );
}

void CCFPlayer::CalculateHandMagic()
{
	CRunePosition* pPrimary = &m_pCurrentArmament->m_aAttackBinds[0];
	CRunePosition* pSecondary = &m_pCurrentArmament->m_aAttackBinds[1];
	m_eLHEffectElements = ELEMENT_TYPELESS;
	m_eRHEffectElements = ELEMENT_TYPELESS;

	// His fire has been extinguished.
	if (!IsAlive())
		return;

	if (m_pStats->m_flFocus < 0)
		return;

	if (IsMagicMode())
	{
		// If we have either no weapons or two weapons, use both hands. Otherwise use only the off hand.
		bool bBothHands = !(GetPrimaryWeapon() && !GetSecondaryWeapon());

		if (bBothHands)
		{
			if (pPrimary->m_iWeapon >= 0)
				m_eLHEffectElements = m_pCurrentArmament->GetMagicalAttackElement(pPrimary);

			if (pSecondary->m_iWeapon >= 0)
				m_eRHEffectElements = m_pCurrentArmament->GetMagicalAttackElement(pSecondary);
		}
		else
		{
			CRunePosition* pLastCombo = &m_pCurrentArmament->m_aAttackBinds[m_iLastCombo];
			if (pLastCombo->m_iWeapon >= 0)
			{
				element_t eAttackElements = m_pCurrentArmament->GetMagicalAttackElement(pLastCombo);
				Assert(GetPrimaryWeapon());
				if (GetPrimaryWeapon()->IsMeleeWeapon())
					m_eRHEffectElements = eAttackElements;
				else
					m_eLHEffectElements = eAttackElements;
			}
		}
	}
}

int CCFPlayer::ObjectCaps( void )
{
	int iCaps = BaseClass::ObjectCaps();
	if (IsKnockedOut())
		iCaps |= FCAP_ONOFF_USE|FCAP_USE_IN_RADIUS;
	return iCaps;
}

Vector CCFPlayer::GetCentroid( ) const
{
	Vector centroid = GetAbsOrigin();

	const Vector &mins = WorldAlignMins();
	const Vector &maxs = WorldAlignMaxs();

	centroid.z += (maxs.z - mins.z)/2.0f;

	//centroid.z += HalfHumanHeight;

	return centroid;
}

void CCFPlayer::CreateViewModel( int viewmodelindex )
{
	AssertMsg(false, "Tried to CreateViewModel(). It shouldn't exist in CF!");
}

CBaseViewModel* CCFPlayer::GetViewModel( int index )
{
	AssertMsg(false, "Tried to GetViewModel(). It doesn't exist in CF!");
	return NULL;
}

bool CCFPlayer::IsKnockedOut()
{
	return m_lifeState == LIFE_DYING;
}

Vector CInfoObjective::GetHoldingPosition(CCFPlayer* pHolder)
{
	if (!pHolder)
		return GetAbsOrigin();

	Vector vecLHand, vecRHand, vecForward;
	pHolder->GetAttachment("lhand", vecLHand);
	pHolder->GetAttachment("rhand", vecRHand);
	pHolder->GetVectors(&vecForward, NULL, NULL);
	return (vecLHand + vecRHand)/2;// + vecForward*10 + Vector(0, 0, -5);
}

