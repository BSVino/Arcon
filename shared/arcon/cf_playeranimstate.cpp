//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "tier0/vprof.h"
#include "animation.h"
#include "studio.h"
#include "apparent_velocity_helper.h"
#include "utldict.h"
#include "cf_playeranimstate.h"
#include "base_playeranimstate.h"
#include "datacache/imdlcache.h"
#include "in_buttons.h"
#include "weapon_magic.h"

#ifdef CLIENT_DLL
	#include "c_cf_player.h"
	#include "cf_in_main.h"
	#include "engine/ivdebugoverlay.h"
#else
	#include "cf_player.h"
#endif

#define CF_RUN_SPEED			320.0f
#define CF_CROUCH_SPEED			110.0f

extern ConVar anim_showmainactivity;

ConVar anim_showyaws( "anim_showyaws", "0", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Show some triangles that show the yaw values of the player." );

CCFPlayerAnimState* CreatePlayerAnimState( CCFPlayer* pPlayer )
{
	MDLCACHE_CRITICAL_SECTION();

	// Setup the movement data.
	MultiPlayerMovementData_t movementData;
	movementData.m_flBodyYawRate = 720.0f;
	movementData.m_flRunSpeed = CF_RUN_SPEED;
	movementData.m_flWalkSpeed = CF_CROUCH_SPEED;
	movementData.m_flSprintSpeed = -1.0f;

	// Create animation state for this player.
	CCFPlayerAnimState *pRet = new CCFPlayerAnimState( pPlayer, movementData );

	pRet->InitCF( pPlayer );
	return pRet;
}

// ------------------------------------------------------------------------------------------------ //
// CCFPlayerAnimState implementation.
// ------------------------------------------------------------------------------------------------ //

CCFPlayerAnimState::CCFPlayerAnimState()
{
	m_pCFPlayer = NULL;
}

CCFPlayerAnimState::CCFPlayerAnimState( CBasePlayer *pPlayer, MultiPlayerMovementData_t &movementData )
	: CMultiPlayerAnimState( pPlayer, movementData )
{
	m_pCFPlayer = NULL;
	m_CFParameterData.Init();

	m_bFacingForward = true;

	// Latching.
	m_bLatching = false;
	m_bLatchJumping = false;

	m_bRushing = false;

	m_iFullBodyAnimation = ACT_INVALID;

	m_vecLastAirVelocity = Vector(0,0,0);
}

void CCFPlayerAnimState::InitCF( CCFPlayer *pPlayer )
{
	m_pCFPlayer = pPlayer;
}


void CCFPlayerAnimState::ClearAnimationState()
{
	m_bFacingForward = true;
	m_bLatching = false;
	m_bLatchJumping = false;
	m_iFullBodyAnimation = ACT_INVALID;
	m_vecLastAirVelocity = Vector(0,0,0);
	BaseClass::ClearAnimationState();
}

Activity CCFPlayerAnimState::TranslateActivity( Activity actDesired )
{
	Activity translateActivity = BaseClass::TranslateActivity( actDesired );

	CCFPlayer* pPlayer = GetCFPlayer();

	if ( pPlayer->IsMagicMode() )
	{
		CWeaponMagic* pMagic = pPlayer->GetMagicWeapon();
		bool bActive = (gpGlobals->curtime < pMagic->m_flLastCast + 3) && (!pPlayer->GetPrimaryWeapon() && !pPlayer->GetSecondaryWeapon());

		// Always use active animation sets if we have weapons, they use active activities to translate.
		if (pPlayer->GetPrimaryWeapon() || pPlayer->GetSecondaryWeapon())
			bActive = true;

		if (pMagic->IsCharging())
			bActive = true;

#ifdef CLIENT_DLL
		C_CFPlayer * pLocalPlayer = C_CFPlayer::GetLocalCFPlayer();

		// If we're in first person with the magic and no weapons then automatically use the active set all the time, since it's more visible to the player.
		if ((pPlayer == pLocalPlayer && !CFInput()->CAM_IsThirdPerson() || pLocalPlayer->IsFirstPersonSpectating(pPlayer)) && (!pPlayer->GetPrimaryWeapon() && !pPlayer->GetSecondaryWeapon()))
			bActive = true;
#endif

		translateActivity = pMagic->ActivityOverride( translateActivity, bActive );
	}

	if ( pPlayer->GetPrimaryWeapon() )
	{
		return pPlayer->GetPrimaryWeapon()->ActivityOverride( translateActivity, false );
	}

	return translateActivity;
}

void CCFPlayerAnimState::DoAnimationEvent( PlayerAnimEvent_t event, int mData, bool bSecondary )
{
	switch( event )
	{
	case PLAYERANIMEVENT_POWERJUMP:
		{
			if (mData == ACT_CF_POWERJUMP_LEFT)
				FullBodyAnimation( ACT_CF_POWERJUMP_LEFT, false );
			else if (mData == ACT_CF_POWERJUMP_RIGHT)
				FullBodyAnimation( ACT_CF_POWERJUMP_RIGHT, false );
			else if (mData == ACT_CF_POWERJUMP_BACK)
				FullBodyAnimation( ACT_CF_POWERJUMP_BACK, false );
			else
				// Fall back to the jumping animations.
				BaseClass::DoAnimationEvent( PLAYERANIMEVENT_JUMP, mData );
	
			break;
		}
	case PLAYERANIMEVENT_LATCH:
		{
			// Latch.
			m_bLatching = true;
			m_eLatchDirection = FindInitialLatchingDirection();

			RestartMainSequence();

			break;
		}
	case PLAYERANIMEVENT_JUMP:
		{
			if (m_bLatching)
				m_bLatchJumping = true;

			BaseClass::DoAnimationEvent( event, mData );
	
			break;
		}
	case PLAYERANIMEVENT_ATTACK_PRIMARY:
		{
			if (m_bLatching)
			{
				switch(m_eLatchDirection)
				{
				case LD_LEFT:
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_CF_LATCH_LEFT_RANGE_ATTACK );
					break;

				case LD_RIGHT:
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_CF_LATCH_RIGHT_RANGE_ATTACK );
					break;

				case LD_BACK:
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_CF_LATCH_BACK_RANGE_ATTACK );
					break;
				}
			}
			else if (bSecondary)
				RestartGesture( GESTURE_SLOT_ATTACK_SECONDARY, ACT_CF_S_RANGE_ATTACK );
			else
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_STAND_PRIMARYFIRE );
			break;
		}
	case PLAYERANIMEVENT_RELOAD:
		{
			if (m_bLatching)
			{
				switch(m_eLatchDirection)
				{
				case LD_LEFT:
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_CF_LATCH_LEFT_RELOAD );
					break;

				case LD_RIGHT:
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_CF_LATCH_RIGHT_RELOAD );
					break;

				case LD_BACK:
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_CF_LATCH_BACK_RELOAD );
					break;
				}
			}
			else if ( GetBasePlayer()->GetFlags() & FL_DUCKING )
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_CROUCH );
			//else if ( m_bInSwim )
				//RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_SWIM );
			else if (bSecondary)
				RestartGesture( GESTURE_SLOT_ATTACK_SECONDARY, ACT_CF_S_RELOAD );
			else
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_STAND );
			break;
		}
	case PLAYERANIMEVENT_NUMEN:
		{
			if (m_bLatching)
			{
				switch(m_eLatchDirection)
				{
				case LD_LEFT:
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_CF_LATCH_LEFT_CAST );
					break;

				case LD_RIGHT:
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_CF_LATCH_RIGHT_CAST );
					break;

				case LD_BACK:
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_CF_LATCH_BACK_CAST );
					break;
				}
			}
			else
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_CF_CAST );
			break;
		}
	case PLAYERANIMEVENT_CHARGEUP:
		{
			if (m_bLatching)
			{
				switch(m_eLatchDirection)
				{
				case LD_LEFT:
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_CF_LATCH_LEFT_CHARGEUP );
					break;

				case LD_RIGHT:
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_CF_LATCH_RIGHT_CHARGEUP );
					break;

				case LD_BACK:
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_CF_LATCH_BACK_CHARGEUP );
					break;
				}
			}
			else
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_CF_CHARGEUP );
			break;
		}
	case PLAYERANIMEVENT_RUSH:
		{
			m_bRushing = true;
			m_iRush = 0;
			RestartMainSequence();	// Reset the animation.
			break;
		}
	case PLAYERANIMEVENT_CHARGE:
		{
			if (m_bLatching)
			{
				switch(m_eLatchDirection)
				{
				case LD_LEFT:
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_CF_LATCH_LEFT_CHARGE, false );

				case LD_RIGHT:
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_CF_LATCH_RIGHT_CHARGE, false );

				case LD_BACK:
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_CF_LATCH_BACK_CHARGE, false );
				}
			}
			else
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_CF_CHARGE, false );
			break;
		}
	case PLAYERANIMEVENT_ATTACK:
		{
			// If we are allowed to do any attacks, make sure the flinch is not still playing.
			ResetGestureSlot( GESTURE_SLOT_FLINCH );

			if (mData == ACT_INVALID)
			{
				ResetGestureSlot( GESTURE_SLOT_ATTACK_AND_RELOAD );
				FullBodyAnimation( ACT_INVALID );	// Stop any full-body attack animations going on.
			}
			else if (m_bLatching)
			{
				switch(m_eLatchDirection)
				{
				case LD_LEFT:
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_CF_LATCH_LEFT_WEAK_ATTACK );
					break;

				case LD_RIGHT:
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_CF_LATCH_RIGHT_WEAK_ATTACK );
					break;

				case LD_BACK:
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_CF_LATCH_BACK_WEAK_ATTACK );
					break;
				}
			}
			else if (bSecondary)
				RestartGesture( GESTURE_SLOT_ATTACK_SECONDARY, ACT_CF_S_WEAK_ATTACK );
			else
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_CF_WEAK_ATTACK );
			break;
		}
	case PLAYERANIMEVENT_STRONGATTACK:
		{
			// If we are allowed to do any attacks, make sure the flinch is not still playing.
			ResetGestureSlot( GESTURE_SLOT_FLINCH );

			FullBodyAnimation( ACT_INVALID );	// Stop any full-body attack animations going on so the code doesn't just reset an existing animation.

			if (mData == ACT_INVALID)
			{
				ResetGestureSlot( GESTURE_SLOT_ATTACK_AND_RELOAD );
				FullBodyAnimation( ACT_INVALID );	// Stop any full-body attack animations going on.
			}
			else if (m_bLatching)
			{
				switch(m_eLatchDirection)
				{
				case LD_LEFT:
					FullBodyAnimation( ACT_CF_LATCH_LEFT_STRONG_ATTACK, true, true );
					break;

				case LD_RIGHT:
					FullBodyAnimation( ACT_CF_LATCH_RIGHT_STRONG_ATTACK, true, true );
					break;

				case LD_BACK:
					FullBodyAnimation( ACT_CF_LATCH_BACK_STRONG_ATTACK, true, true );
					break;
				}
			}
			else if (bSecondary)
				FullBodyAnimation( ACT_CF_S_STRONG_ATTACK, true, true );
			else
				FullBodyAnimation( ACT_CF_STRONG_ATTACK, true, true );
			break;
		}
	case PLAYERANIMEVENT_CHARGEATTACK:
		{
			// If we are allowed to do any attacks, make sure the flinch is not still playing.
			ResetGestureSlot( GESTURE_SLOT_FLINCH );

			if (bSecondary)
				FullBodyAnimation( ACT_CF_SC_ATTACK );
			else
				FullBodyAnimation( ACT_CF_C_ATTACK );
			break;
		}
	case PLAYERANIMEVENT_BLOCK:
		{
			if (bSecondary)
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_CF_S_BLOCK );
			else
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_CF_BLOCK );
			break;
		}
	case PLAYERANIMEVENT_BLOCKED:
		{
			if (bSecondary)
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_CF_S_BLOCKED );
			else
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_CF_BLOCKED );
			break;
		}
	case PLAYERANIMEVENT_DIE:
		{
			m_bDying = true;
			FullBodyAnimation( ACT_DIESIMPLE );
			break;
		}
	case PLAYERANIMEVENT_EXECUTE:
		{
			FullBodyAnimation( (Activity)mData );
			break;
		}
	case PLAYERANIMEVENT_EXECUTED:
		{
			FullBodyAnimation( (Activity)mData );
			break;
		}
	case PLAYERANIMEVENT_FLINCH_CHEST:
		{
			if (m_pCFPlayer->IsAlive())
				PlayFlinchGesture( ACT_MP_GESTURE_FLINCH_CHEST );
			break;
		}
	default:
		{
			BaseClass::DoAnimationEvent( event, mData );
			break;
		}
	}
}

void CCFPlayerAnimState::FullBodyAnimation( Activity iFullBody, bool bResetGestures, bool bAvoidRepeat )
{
	Activity iTranslated = TranslateActivity( iFullBody );

	if (iFullBody == ACT_INVALID)
	{
		if (bResetGestures)
			ResetGestureSlots();
		RestartMainSequence();
		m_iFullBodyAnimation = ACT_INVALID;
		return;
	}

	MDLCACHE_CRITICAL_SECTION();

	// Don't use CCFPlayerAnimState, it just returns m_iPredictedFullBodySequence during full body anims.
	int iNewSequence = CMultiPlayerAnimState::SelectWeightedSequence( iTranslated );
	if (bAvoidRepeat && iNewSequence == m_iPredictedFullBodySequence)
		m_iPredictedFullBodySequence = CMultiPlayerAnimState::SelectWeightedSequence( iTranslated );
	else
		m_iPredictedFullBodySequence = iNewSequence;
	if (m_iPredictedFullBodySequence == -1)
		return;

	m_pCFPlayer->ResetSequence( 0 );	// Make sure ComputeMainSequences doesn't throw this one away.

	// Currently, any full body animation (attacks and such) are always started in the
	// direction that the eye is facing as opposed to the direction of movement.
	m_pCFPlayer->SetAbsAngles(QAngle(0, m_flEyeYaw, 0));

	if (bResetGestures)
		ResetGestureSlots();
	RestartMainSequence();

	m_iFullBodyAnimation = iTranslated;
}

bool CCFPlayerAnimState::HandleKnockout( Activity &idealActivity )
{
	if (m_bDying && m_pCFPlayer->IsAlive())
	{
		m_bDying = false;
		FullBodyAnimation(ACT_INVALID);
	}

	// Always return false so that HandleFullBodyAnimation takes over our fullbody animation.
	return false;
}

bool CCFPlayerAnimState::HandleFullBodyAnimation( Activity &idealActivity )
{
	if (m_pCFPlayer->GetSequenceActivity(m_pCFPlayer->GetSequence()) == m_iFullBodyAnimation
		&& m_pCFPlayer->IsSequenceFinished() && !m_pCFPlayer->IsSequenceLooping(m_pCFPlayer->GetSequence()))
	{
		m_iFullBodyAnimation = ACT_INVALID;
		RestartMainSequence();
	}

	if ( m_iFullBodyAnimation != ACT_INVALID )
	{
		idealActivity = m_iFullBodyAnimation;
	}

	return m_iFullBodyAnimation != ACT_INVALID;
}

bool CCFPlayerAnimState::HandleRushing( Activity &idealActivity )
{
	if ( m_bRushing )
	{
		if ( GetCFPlayer()->m_flRushDistance <= 0 )
			m_bRushing = false;
	}

	if ( m_bRushing )
	{
		idealActivity = (Activity)(ACT_CF_RUSH1 + m_iRush);
	}

	// Are we still jumping? If so, keep playing the jump animation.
	return m_bRushing;
}

bool CCFPlayerAnimState::HandleDFA( Activity &idealActivity )
{
	// Finish my fullbody animation first, whatever it is.
	if ( IsInFullBodyAnimation() )
		return false;

	if ( GetCFPlayer()->m_bDownStrike )
	{
		idealActivity = TranslateActivity(ACT_CF_DFA_ATTACK);

		return true;
	}

	bool bCanDownStrike = true;
	if (GetCFPlayer()->GetRecursedTarget())
	{
		Vector vecForward, vecTarget;
		vecTarget = GetCFPlayer()->GetRecursedTarget()->GetAbsOrigin() - GetCFPlayer()->GetAbsOrigin();
		vecTarget.z = 0;
		vecTarget.NormalizeInPlace();
		GetCFPlayer()->GetVectors(&vecForward, NULL, NULL);
		vecForward.z = 0;
		vecForward.NormalizeInPlace();
		bCanDownStrike = (DotProduct(vecForward, vecTarget) > 0.9f);
	}

	if ( GetCFPlayer()->CanDownStrike(bCanDownStrike) )
	{
		idealActivity = TranslateActivity(ACT_CF_DFA_READY);

		return true;
	}

	return false;
}

bool CCFPlayerAnimState::HandleJumping( Activity &idealActivity )
{
	if ( !(GetBasePlayer()->GetFlags() & FL_ONGROUND) )
	{
		// Ran off a ledge.
		idealActivity = ACT_MP_JUMP_FLOAT;
		m_bJumping = true;

		m_vecLastAirVelocity = GetBasePlayer()->GetAbsVelocity();
	}

	if ( m_bJumping )
	{
		if ( m_bFirstJumpFrame )
		{
			m_bFirstJumpFrame = false;
			RestartMainSequence();	// Reset the animation.
		}

		// Reset if we hit water and start swimming.
		if ( GetBasePlayer()->GetWaterLevel() >= WL_Waist )
		{
			m_bJumping = false;
			RestartMainSequence();
		}
		// Don't check if he's on the ground for a sec.. sometimes the client still has the
		// on-ground flag set right when the message comes in.
		else if ( gpGlobals->curtime - m_flJumpStartTime > 0.2f )
		{
			if ( GetBasePlayer()->GetFlags() & FL_ONGROUND )
			{
				m_bJumping = false;
				RestartMainSequence();	// Reset the animation.
				RestartGesture( GESTURE_SLOT_JUMP, ACT_MP_JUMP_LAND );
				float flMaxWeight = 1;
#ifdef CLIENT_DLL
				if (!input->CAM_IsThirdPerson())
					flMaxWeight = 0.5f;
#endif
				GetGestureSlot(GESTURE_SLOT_JUMP)->m_pAnimLayer->m_flWeight = RemapValClamped(m_vecLastAirVelocity.z, -200, -1000, 0.2f, flMaxWeight);
			}
		}

		// if we're still jumping
		if ( m_bJumping )
		{
			if ( gpGlobals->curtime - m_flJumpStartTime > 0.5 )
			{
				idealActivity = ACT_MP_JUMP_FLOAT;
			}
			else
			{
				if ( GetCFPlayer()->m_bPowerjump )
				{
					if ( GetCFPlayer()->m_bChargejump )
						idealActivity = ACT_CF_POWERJUMP_UP;
					else
						idealActivity = ACT_CF_POWERJUMP_FORWARD;
				}
				else
					idealActivity = ACT_MP_JUMP_START;
			}
		}
	}

	// Are we still jumping? If so, keep playing the jump animation.
	return m_bJumping;
}

bool CCFPlayerAnimState::HandleSwimming( Activity &idealActivity )
{
	// We look down on swimming in this game.
	
	return false;
}

latchdir_t CCFPlayerAnimState::FindInitialLatchingDirection()
{
	Assert( m_bLatching );

	QAngle angPlane;
	VectorAngles(GetCFPlayer()->m_vecLatchPlaneNormal, angPlane);
	float flRelativeAimYaw = AngleNormalize( angPlane[YAW] - m_flEyeYaw );

	if (flRelativeAimYaw < 0 && flRelativeAimYaw > -135)
		return LD_LEFT;

	if (flRelativeAimYaw > 0 && flRelativeAimYaw < 135)
		return LD_RIGHT;

	return LD_BACK;
}

latchdir_t CCFPlayerAnimState::FindLatchingDirection()
{
	Assert( m_bLatching );

	QAngle angPlane;
	VectorAngles(GetCFPlayer()->m_vecLatchPlaneNormal, angPlane);
 	float flRelativeAimYaw = AngleNormalize( angPlane[YAW] - m_flEyeYaw );

	if (fabs(flRelativeAimYaw) > 135 && m_eLatchDirection != LD_BACK)
		return LD_BACK;

	if (flRelativeAimYaw > 45 && flRelativeAimYaw < 90 && m_eLatchDirection != LD_RIGHT)
		return LD_RIGHT;

	if (flRelativeAimYaw < -45 && flRelativeAimYaw > -90 && m_eLatchDirection != LD_LEFT)
		return LD_LEFT;

	return m_eLatchDirection;
}

bool CCFPlayerAnimState::HandleLatching( Activity &idealActivity )
{
	if ( m_bLatchJumping )
	{
		if ( m_bFirstJumpFrame )
		{
			m_bFirstJumpFrame = false;
			RestartMainSequence();	// Reset the animation.
		}

		// Reset if we hit water and start swimming.
		if ( GetBasePlayer()->GetWaterLevel() >= WL_Waist )
		{
			m_bLatchJumping = false;
			RestartMainSequence();
		}
		// Don't check if he's on the ground for a sec.. sometimes the client still has the
		// on-ground flag set right when the message comes in.
		else if ( gpGlobals->curtime - m_flJumpStartTime > 0.2f )
		{
			if ( GetBasePlayer()->GetFlags() & FL_ONGROUND )
			{
				m_bLatchJumping = false;
				RestartMainSequence();	// Reset the animation.
				RestartGesture( GESTURE_SLOT_JUMP, ACT_MP_JUMP_LAND );					
				GetGestureSlot(GESTURE_SLOT_JUMP)->m_pAnimLayer->m_flWeight = RemapValClamped(m_vecLastAirVelocity.z, -200, -1000, 0.2f, 1.0f);
			}
		}

		// if we're still jumping
		if ( m_bLatchJumping )
		{
			if ( gpGlobals->curtime - m_flJumpStartTime > 0.5 )
			{
				m_bJumping = true;
				m_bLatchJumping = false;
				// HandleJumping() should take over from here.
			}
			else
			{
				idealActivity = ACT_CF_LATCH_JUMP_START;
			}
		}
	}

	if ( m_bLatching && !GetCFPlayer()->m_bLatched)
	{
		m_bLatching = false;
		RestartMainSequence();
	}

	if ( m_bLatching )
	{
		latchdir_t eNewDirection = FindLatchingDirection();

		if (m_eLatchDirection != eNewDirection)
		{
			m_eLatchDirection = eNewDirection;
			RestartMainSequence();
		}

		switch(m_eLatchDirection)
		{
		case LD_LEFT:
			idealActivity = ACT_CF_LATCH_LEFT;
			break;

		case LD_RIGHT:
			idealActivity = ACT_CF_LATCH_RIGHT;
			break;

		case LD_BACK:
			idealActivity = ACT_CF_LATCH_BACK;
			break;
		}
	}

	return m_bLatching || m_bLatchJumping;
}

Activity CCFPlayerAnimState::CalcMainActivity()
{
	Activity idealActivity = ACT_MP_STAND_IDLE;

	if ( HandleKnockout( idealActivity ) ||
		HandleDFA( idealActivity ) ||
		HandleFullBodyAnimation( idealActivity ) ||
		HandleRushing( idealActivity ) ||
		HandleLatching( idealActivity ) ||
		HandleJumping( idealActivity ) ||
//		HandleDucking( idealActivity ) ||
		HandleSwimming( idealActivity ) ||
		HandleDying( idealActivity ) )
	{
		// intentionally blank
	}
	else
	{
		HandleMoving( idealActivity );
	}

	ShowDebugInfo();

#ifdef CLIENT_DLL
	// Client specific.
	if ( anim_showmainactivity.GetBool() )
	{
		DebugShowActivity( idealActivity );
	}

	if ( anim_showyaws.GetBool() )
	{
		// Draw a red triangle on the ground for the eye yaw.
		float flBaseSize = 10;
		float flHeight = 80;
		Vector vBasePos = GetBasePlayer()->GetAbsOrigin() + Vector( 0, 0, 3 );
		QAngle angles( 0, 0, 0 );
		angles[YAW] = m_flEyeYaw;
		Vector vForward, vRight, vUp;
		AngleVectors( angles, &vForward, &vRight, &vUp );
		debugoverlay->AddTriangleOverlay( vBasePos+vRight*flBaseSize/2, vBasePos-vRight*flBaseSize/2, vBasePos+vForward*flHeight, 255, 0, 0, 255, false, 0.01 );

		// Draw a blue triangle on the ground for the body yaw.
		angles[YAW] = m_angRender[YAW];
		AngleVectors( angles, &vForward, &vRight, &vUp );
		debugoverlay->AddTriangleOverlay( vBasePos+vRight*flBaseSize/2, vBasePos-vRight*flBaseSize/2, vBasePos+vForward*flHeight, 0, 0, 255, 255, false, 0.01 );	
	}
#endif

	return idealActivity;
}

bool CCFPlayerAnimState::SetupPoseParameters( CStudioHdr *pStudioHdr )
{
	// Check to see if this has already been done.
	if ( m_bPoseParameterInit )
		return true;

	// Save off the pose parameter indices.
	if ( !pStudioHdr )
		return false;

	// Look for the aim pitch blender.
	m_CFParameterData.m_iHeadPitch = GetBasePlayer()->LookupPoseParameter( pStudioHdr, "head_pitch" );
	if ( m_CFParameterData.m_iHeadPitch < 0 )
		return false;

	// Look for aim yaw blender.
	m_CFParameterData.m_iHeadYaw = GetBasePlayer()->LookupPoseParameter( pStudioHdr, "head_yaw" );
	if ( m_CFParameterData.m_iHeadYaw < 0 )
		return false;

	return BaseClass::SetupPoseParameters(pStudioHdr);
}

void CCFPlayerAnimState::ComputePoseParam_MoveYaw( CStudioHdr *pStudioHdr )
{
	// Get the estimated movement yaw.
	EstimateYaw();

	// Get the view yaw.
	float flAngle = AngleNormalize( m_flEyeYaw );

	// Calc side to side turning - the view vs. movement yaw.
	float flYaw = flAngle - m_PoseParameterData.m_flEstimateYaw;
	flYaw = AngleNormalize( -flYaw );

	// Get the current speed the character is running.
	bool bIsMoving;
	float flPlaybackRate = CalcMovementPlaybackRate( &bIsMoving );

	// Setup the 9-way blend parameters based on our speed and direction.
	Vector2D vecCurrentMoveYaw( 0.0f, 0.0f );
	if ( bIsMoving )
	{
		if (GetCFPlayer()->ShouldUseForwardFacingAnimationStyle())
		{
			vecCurrentMoveYaw.x = cos( DEG2RAD( flYaw ) ) * flPlaybackRate;
			vecCurrentMoveYaw.y = -sin( DEG2RAD( flYaw ) ) * flPlaybackRate;
		}
		else
		{
			vecCurrentMoveYaw.x = cos( DEG2RAD( m_PoseParameterData.m_flEstimateYaw ) ) * flPlaybackRate;
			vecCurrentMoveYaw.y = -sin( DEG2RAD( m_PoseParameterData.m_flEstimateYaw ) ) * flPlaybackRate;
		}
	}

	// Set the 9-way blend movement pose parameters.
	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveX, vecCurrentMoveYaw.x );
	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveY, vecCurrentMoveYaw.y );

	m_DebugAnimData.m_vecMoveYaw = vecCurrentMoveYaw;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCFPlayerAnimState::EstimateYaw( void )
{
	// Get the frame time.
	float flDeltaTime = gpGlobals->frametime;
	if ( flDeltaTime == 0.0f )
		return;

	// Get the player's velocity and angles.
	Vector vecEstVelocity;
	GetOuterAbsVelocity( vecEstVelocity );
	QAngle angles = GetBasePlayer()->GetLocalAngles();

	// If we are not moving, sync up the feet and eyes slowly.
	if ( vecEstVelocity.Length2DSqr() < 1.0f )
	{
		float flYawDelta = angles[YAW] - m_PoseParameterData.m_flEstimateYaw;
		flYawDelta = AngleNormalize( flYawDelta );

		if ( flDeltaTime < 0.25f )
		{
			flYawDelta *= ( flDeltaTime * 4.0f );
		}
		else
		{
			flYawDelta *= flDeltaTime;
		}

		m_PoseParameterData.m_flEstimateYaw += flYawDelta;
		m_PoseParameterData.m_flEstimateYaw = AngleNormalize( m_PoseParameterData.m_flEstimateYaw );
	}
	else if (GetCFPlayer()->ShouldUseForwardFacingAnimationStyle())
	{
		m_PoseParameterData.m_flEstimateYaw = ( atan2( vecEstVelocity.y, vecEstVelocity.x ) * 180.0f / M_PI );
		m_PoseParameterData.m_flEstimateYaw = clamp( m_PoseParameterData.m_flEstimateYaw, -180.0f, 180.0f );
	}
	else
	{
		QAngle angDir;
		VectorAngles(vecEstVelocity, angDir);

		if (fabs(AngleNormalize(angDir[YAW] - m_flEyeYaw)) <= 90)
			m_bFacingForward = true;
		else if (fabs(AngleNormalize(angDir[YAW] - m_flEyeYaw)) >= 91)
			m_bFacingForward = false;

		float flYawDelta = AngleNormalize(m_flGoalFeetYaw - m_flCurrentFeetYaw);

		if (m_bFacingForward)
			m_PoseParameterData.m_flEstimateYaw = flYawDelta;
		else
			m_PoseParameterData.m_flEstimateYaw = 180-flYawDelta;
	}
}

void CCFPlayerAnimState::ComputePoseParam_AimPitch( CStudioHdr *pStudioHdr )
{
	BaseClass::ComputePoseParam_AimPitch(pStudioHdr);

	ComputePoseParam_HeadPitch( pStudioHdr );
}

void CCFPlayerAnimState::ComputePoseParam_AimYaw( CStudioHdr *pStudioHdr )
{
	// Get the movement velocity.
	Vector vecVelocity;
	GetOuterAbsVelocity( vecVelocity );

	// Check to see if we are moving.
	bool bMoving = ( vecVelocity.Length2DSqr() > 1.0f ) ? true : false;

	if ( IsInFullBodyAnimation() )
	{
		// Follow the eyes. If it should be locked into one direction, call FreezePlayer().
		m_flGoalFeetYaw = m_flEyeYaw;
	}
	else if ( m_bLatching )
	{
		QAngle angPlane;
		VectorAngles(GetCFPlayer()->m_vecLatchPlaneNormal, angPlane);

		float flYawOffset;
		if (m_eLatchDirection == LD_LEFT)
			flYawOffset = 45;
		else if (m_eLatchDirection == LD_RIGHT)
			flYawOffset = -45;
		else
			flYawOffset = 180;
		m_flGoalFeetYaw = AngleNormalize( angPlane[YAW] + flYawOffset );
	}
	// If we are moving or are prone and undeployed.
	else if ( bMoving || m_bForceAimYaw )
	{
		if (GetCFPlayer()->ShouldUseForwardFacingAnimationStyle() || gpGlobals->curtime < GetCFPlayer()->m_flLastLatch + 0.5f)
		{
			// The feet match the eye direction when moving - the move yaw takes care of the rest.
			m_flGoalFeetYaw = m_flEyeYaw;
		}
		else
		{
			QAngle angDir;
			VectorAngles(vecVelocity, angDir);

			if (m_bFacingForward)
			{
				m_flGoalFeetYaw = angDir[YAW];
			}
			else
			{
				m_flGoalFeetYaw = AngleNormalize(angDir[YAW] + 180);
			}
		}
	}
	// Else if we are not moving.
	else
	{
		// Initialize the feet.
		if ( m_PoseParameterData.m_flLastAimTurnTime <= 0.0f )
		{
			m_flGoalFeetYaw	= m_flEyeYaw;
			m_flCurrentFeetYaw = m_flEyeYaw;
			m_PoseParameterData.m_flLastAimTurnTime = gpGlobals->curtime;
		}
		// Make sure the feet yaw isn't too far out of sync with the eye yaw.
		// TODO: Do something better here!
		else
		{
			float flYawDelta = AngleNormalize(  m_flGoalFeetYaw - m_flEyeYaw );

			if ( fabs( flYawDelta ) > 90.0f )
			{
				float flSide = ( flYawDelta > 0.0f ) ? -1.0f : 1.0f;
				m_flGoalFeetYaw += ( 90.0f * flSide );
			}
		}
	}

	// Fix up the feet yaw.
	m_flGoalFeetYaw = AngleNormalize( m_flGoalFeetYaw );
	if ( m_flGoalFeetYaw != m_flCurrentFeetYaw )
	{
		if ( m_bForceAimYaw || m_bLatching )
		{
			m_flCurrentFeetYaw = m_flGoalFeetYaw;
		}
		else
		{
			ConvergeYawAngles( m_flGoalFeetYaw, 720.0f, gpGlobals->frametime, m_flCurrentFeetYaw );
			m_flLastAimTurnTime = gpGlobals->curtime;
		}
	}

	// Rotate the body into position.
	m_angRender[YAW] = m_flCurrentFeetYaw;

	// Find the aim(torso) yaw base on the eye and feet yaws.
	float flAimYaw = m_flEyeYaw - m_flCurrentFeetYaw;
	flAimYaw = AngleNormalize( flAimYaw );

	// Set the aim yaw and save.
	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iAimYaw, -flAimYaw );
	m_DebugAnimData.m_flAimYaw	= flAimYaw;

	ComputePoseParam_HeadYaw( pStudioHdr );

	// Turn off a force aim yaw - either we have already updated or we don't need to.
	m_bForceAimYaw = false;

#ifndef CLIENT_DLL
	QAngle angle = GetBasePlayer()->GetAbsAngles();
	angle[YAW] = m_flCurrentFeetYaw;

	GetBasePlayer()->SetAbsAngles( angle );
#endif
}

void CCFPlayerAnimState::ConvergeYawAngles( float flGoalYaw, float flYawRate, float flDeltaTime, float &flCurrentYaw )
{
#define FADE_TURN_DEGREES 60.0f

	// Find the yaw delta.
	float flDeltaYaw = flGoalYaw - flCurrentYaw;
	float flDeltaYawAbs = fabs( flDeltaYaw );
	flDeltaYaw = AngleNormalize( flDeltaYaw );

	// Always do at least a bit of the turn (1%).
	float flScale = 1.0f;
	flScale = flDeltaYawAbs / FADE_TURN_DEGREES;
	flScale = clamp( flScale, 0.01f, 1.0f );

	float flYaw = flYawRate * flDeltaTime * flScale;
	if ( flDeltaYawAbs < flYaw )
	{
		flCurrentYaw = flGoalYaw;
	}
	else
	{
		// Always turn in the forward direction. Never loop around and turn backwards.
		float flRelativeGoalYaw = AngleNormalize( flGoalYaw - m_flEyeYaw );
		float flRelativeCurrentYaw = AngleNormalize( flCurrentYaw - m_flEyeYaw );
		float flSide = ( flRelativeCurrentYaw < flRelativeGoalYaw ) ? 1.0f : -1.0f;
		flCurrentYaw += ( flYaw * flSide );
	}

	flCurrentYaw = AngleNormalize( flCurrentYaw );

#undef FADE_TURN_DEGREES
}

void CCFPlayerAnimState::ComputePoseParam_HeadPitch( CStudioHdr *pStudioHdr )
{
	float flGoalPitch;

	if (ShouldLookAtTarget())
	{
		Assert(GetCFPlayer()->GetRecursedTarget());
		if (!GetCFPlayer()->GetRecursedTarget())
			return;

		Vector vecTargetDirection = GetCFPlayer()->GetRecursedTarget()->GetAbsOrigin() - GetCFPlayer()->GetAbsOrigin();
		vecTargetDirection.NormalizeInPlace();

		QAngle angTarget;
		VectorAngles(vecTargetDirection, angTarget);

		flGoalPitch = AngleNormalize(-angTarget[PITCH]);
	}
	else
	{
		flGoalPitch = -m_flEyePitch;
	}

	m_CFParameterData.m_flHeadPitch = Approach(flGoalPitch, m_CFParameterData.m_flHeadPitch, gpGlobals->frametime*500);
	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_CFParameterData.m_iHeadPitch, m_CFParameterData.m_flHeadPitch );
}

void CCFPlayerAnimState::ComputePoseParam_HeadYaw( CStudioHdr *pStudioHdr )
{
	float flGoalYaw;

	if (ShouldLookAtTarget())
	{
		Assert(GetCFPlayer()->GetRecursedTarget());
		if (!GetCFPlayer()->GetRecursedTarget())
			return;

		Vector vecTargetDirection = GetCFPlayer()->GetRecursedTarget()->GetAbsOrigin() - GetCFPlayer()->GetAbsOrigin();
		vecTargetDirection.NormalizeInPlace();

		QAngle angTarget;
		VectorAngles(vecTargetDirection, angTarget);

		flGoalYaw = AngleNormalize(m_flCurrentFeetYaw - angTarget[YAW]);
	}
	else
	{
		float flAimYaw = m_flEyeYaw - m_flCurrentFeetYaw;
		flGoalYaw = -AngleNormalize( flAimYaw );
	}

	m_CFParameterData.m_flHeadYaw = Approach(flGoalYaw, m_CFParameterData.m_flHeadYaw, gpGlobals->frametime*500);
	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_CFParameterData.m_iHeadYaw, m_CFParameterData.m_flHeadYaw );
}

bool CCFPlayerAnimState::ShouldLookAtTarget( )
{
#ifdef CLIENT_DLL
	// If the local player is in first person, don't look at targets,
	// my head is for looking at where I am looking at.
	if (GetCFPlayer() == C_CFPlayer::GetLocalCFPlayer() && !CFInput()->CAM_IsThirdPerson())
		return false;

	CCFPlayer* pTarget = GetCFPlayer()->GetRecursedTarget();
	// If we're in third person and we have a target, look at it.
	if (pTarget)
	{
		Vector vecTargetDirection = pTarget->GetAbsOrigin() - GetCFPlayer()->GetAbsOrigin();
		vecTargetDirection.NormalizeInPlace();

		Vector vecForward;
		
		// Get a fake body angle that has the current feet yaw instead of the eyes.
		QAngle angBody = GetCFPlayer()->GetAbsAngles();
		angBody[YAW] = m_flCurrentFeetYaw;

		AngleVectors(angBody, &vecForward);

		// Don't look if he's behind where the player's body is facing.
		if (DotProduct(vecForward, vecTargetDirection) < 0.1f)
			return false;
		else
			return true;
	}
	else
		return false;
#else
	// Server never looks at the target so that other clients see client looking where he is actually looking.
	return false;
#endif
}

void CCFPlayerAnimState::RestartMainSequence()
{
	CBaseAnimatingOverlay *pPlayer = GetCFPlayer();

	// If the animation system sends a non-fullbody animation event while a fullbody is playing,
	// it resets the fullbody which causes it to play again from the beginning, which obviously
	// causes a lot of problems.
	if (GetCFPlayer()->IsInFullBodyAnimation())
	{
		m_iFullBodyAnimation = ACT_INVALID;
	}

	pPlayer->m_flAnimTime = gpGlobals->curtime;
	pPlayer->SetCycle( 0 );
}

int CCFPlayerAnimState::SelectWeightedSequence( Activity activity )
{
	if (IsInFullBodyAnimation())
		return m_iPredictedFullBodySequence;

	return GetBasePlayer()->SelectWeightedSequence( activity );
}

void CCFPlayerAnimState::ComputeSequences( CStudioHdr *pStudioHdr )
{
#ifdef CLIENT_DLL
	// Completely replace ComputeSequences on the client so that we can do cycling properly.

	VPROF( "CBasePlayerAnimState::ComputeSequences" );

	// Lower body (walk/run/idle).
	ComputeMainSequence();

	// The groundspeed interpolator uses the main sequence info.
	UpdateInterpolators();		

	// Update all active gesture layers.
	for ( int iGesture = 0; iGesture < GESTURE_SLOT_COUNT; ++iGesture )
	{
		if ( !m_aGestureSlots[iGesture].m_bActive )
			continue;

		// Sanity check.
		if ( !pStudioHdr )
			continue;

		CBasePlayer *pPlayer = GetBasePlayer();
		if( !pPlayer )
			continue;

		// Get the current cycle.
		float flCycle = m_aGestureSlots[iGesture].m_pAnimLayer->m_flCycle;
		flCycle += pPlayer->GetSequenceCycleRate( pStudioHdr, m_aGestureSlots[iGesture].m_pAnimLayer->m_nSequence ) * gpGlobals->frametime;

		m_aGestureSlots[iGesture].m_pAnimLayer->m_flPrevCycle =	m_aGestureSlots[iGesture].m_pAnimLayer->m_flCycle;
		m_aGestureSlots[iGesture].m_pAnimLayer->m_flCycle = flCycle;

		if( flCycle > 1.0f )
		{
			RunGestureSlotAnimEventsToCompletion( &m_aGestureSlots[iGesture] );

			if ( m_aGestureSlots[iGesture].m_bAutoKill )
			{
				ResetGestureSlot( m_aGestureSlots[iGesture].m_iGestureSlot );
				continue;
			}
			else if (pPlayer->IsSequenceLooping( pStudioHdr, m_aGestureSlots[iGesture].m_pAnimLayer->m_nSequence))
				m_aGestureSlots[iGesture].m_pAnimLayer->m_flCycle -= 1.0f;
			else
				m_aGestureSlots[iGesture].m_pAnimLayer->m_flCycle = 1.0f;
		}

	}

#else
	CMultiPlayerAnimState::ComputeSequences(pStudioHdr);
#endif
}

void CCFPlayerAnimState::RestartGesture( int iGestureSlot, Activity iGestureActivity, bool bAutoKill )
{
#ifdef GAME_DLL
	bool bAdded = !IsGestureSlotPlaying( iGestureSlot, iGestureActivity );
#endif

	CMultiPlayerAnimState::RestartGesture(iGestureSlot, iGestureActivity, bAutoKill);

#ifdef GAME_DLL
	// This was taken out of the super, I don't know why. CF wants it for charging so here it is.
	if (bAdded)
		m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_bLooping = ( ( GetSequenceFlags( GetCFPlayer()->GetModelPtr(), m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_nSequence ) & STUDIO_LOOPING ) != 0);
#endif
}
