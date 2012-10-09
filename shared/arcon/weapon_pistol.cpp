//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "weapon_pistol.h"

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponPistol, DT_WeaponPistol )

BEGIN_NETWORK_TABLE( CWeaponPistol, DT_WeaponPistol )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponPistol )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_pistol, CWeaponPistol );
PRECACHE_WEAPON_REGISTER( weapon_pistol );

acttable_t	CWeaponPistol::s_CommonActions[] = 
{
	{ ACT_CF_EXECUTE_ANYTHING,			ACT_CF_EXECUTE_UNARMED_UNARMED,			false },
	{ ACT_CF_EXECUTED_ANYTHING,			ACT_CF_EXECUTED_UNARMED_UNARMED,		false },
	{ ACT_CF_LATCH_JUMP_START,			ACT_CF_LATCH_JUMP_START_PISTOL,			false },
	{ ACT_CF_LATCH_LEFT,				ACT_CF_LATCH_LEFT_PISTOL,				false },
	{ ACT_CF_LATCH_LEFT_RELOAD,			ACT_CF_LATCH_LEFT_RELOAD_PISTOL,		false },
	{ ACT_CF_LATCH_LEFT_RANGE_ATTACK,	ACT_CF_LATCH_LEFT_RANGE_ATTACK_PISTOL,	false },
	{ ACT_CF_LATCH_RIGHT,				ACT_CF_LATCH_RIGHT_PISTOL,				false },
	{ ACT_CF_LATCH_RIGHT_RELOAD,		ACT_CF_LATCH_RIGHT_RELOAD_PISTOL,		false },
	{ ACT_CF_LATCH_RIGHT_RANGE_ATTACK,	ACT_CF_LATCH_RIGHT_RANGE_ATTACK_PISTOL,	false },
	{ ACT_CF_LATCH_BACK,				ACT_CF_LATCH_BACK_PISTOL,				false },
	{ ACT_CF_LATCH_BACK_RELOAD,			ACT_CF_LATCH_BACK_RELOAD_PISTOL,		false },
	{ ACT_CF_LATCH_BACK_RANGE_ATTACK,	ACT_CF_LATCH_BACK_RANGE_ATTACK_PISTOL,	false },
};

acttable_t	CWeaponPistol::s_PistolActions[] = 
{
	{ ACT_MP_RUN,						ACT_CF_RUN_PISTOL,						false },
	{ ACT_MP_STAND_IDLE,				ACT_CF_IDLE_PISTOL,						false },
	{ ACT_MP_CROUCH_IDLE,				ACT_CF_IDLE_CROUCH_PISTOL,				false },
	{ ACT_MP_CROUCHWALK,				ACT_CF_WALK_CROUCH_PISTOL,				false },
	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_CF_GESTURE_RANGE_ATTACK_PISTOL,		false },
	{ ACT_MP_RELOAD_STAND,				ACT_CF_GESTURE_RELOAD_PISTOL,			false },
	{ ACT_MP_JUMP_START,				ACT_CF_JUMP_START_PISTOL,				false },
	{ ACT_MP_JUMP_FLOAT,				ACT_CF_JUMP_FLOAT_PISTOL,				false },
	{ ACT_MP_JUMP_LAND,					ACT_CF_JUMP_LAND_PISTOL,				false },
	{ ACT_CF_POWERJUMP_FORWARD,			ACT_CF_POWERJUMP_FORWARD_PISTOL,		false },
	{ ACT_CF_POWERJUMP_UP,				ACT_CF_POWERJUMP_UP_PISTOL,				false },
	{ ACT_CF_POWERJUMP_LEFT,			ACT_CF_POWERJUMP_LEFT_PISTOL,			false },
	{ ACT_CF_POWERJUMP_RIGHT,			ACT_CF_POWERJUMP_RIGHT_PISTOL,			false },
	{ ACT_CF_POWERJUMP_BACK,			ACT_CF_POWERJUMP_BACK_PISTOL,			false },

	{ ACT_CF_IDLE_NUMEN_ACTIVE,					ACT_CF_IDLE_PISTOL_NUMEN,				false },
	{ ACT_CF_RUN_NUMEN_ACTIVE,					ACT_CF_RUN_PISTOL_NUMEN,				false },
	{ ACT_CF_CHARGEUP_NUMEN_ACTIVE,				ACT_CF_CHARGEUP_PISTOL_NUMEN,			false },
	{ ACT_CF_CAST_NUMEN_ACTIVE,					ACT_CF_CAST_PISTOL_NUMEN,				false },
	{ ACT_CF_IDLE_NUMEN_ACTIVE,					ACT_CF_IDLE_PISTOL_NUMEN,				false },
	{ ACT_CF_RUN_NUMEN_ACTIVE,					ACT_CF_RUN_PISTOL_NUMEN,				false },
	{ ACT_CF_JUMP_START_NUMEN_ACTIVE,			ACT_CF_JUMP_START_PISTOL_NUMEN,			false },
	{ ACT_CF_JUMP_FLOAT_NUMEN_ACTIVE,			ACT_CF_JUMP_FLOAT_PISTOL_NUMEN,			false },
	{ ACT_CF_JUMP_LAND_NUMEN_ACTIVE,			ACT_CF_JUMP_LAND_PISTOL_NUMEN,			false },
	{ ACT_CF_POWERJUMP_FORWARD_NUMEN_ACTIVE,	ACT_CF_POWERJUMP_FORWARD_PISTOL_NUMEN,	false },
	{ ACT_CF_POWERJUMP_UP_NUMEN_ACTIVE,			ACT_CF_POWERJUMP_UP_PISTOL_NUMEN,		false },
	{ ACT_CF_POWERJUMP_BACK_NUMEN_ACTIVE,		ACT_CF_POWERJUMP_BACK_PISTOL_NUMEN,		false },
	{ ACT_CF_POWERJUMP_LEFT_NUMEN_ACTIVE,		ACT_CF_POWERJUMP_LEFT_PISTOL_NUMEN,		false },
	{ ACT_CF_POWERJUMP_RIGHT_NUMEN_ACTIVE,		ACT_CF_POWERJUMP_RIGHT_PISTOL_NUMEN,	false },
	{ ACT_CF_JUMP_START_NUMEN_ACTIVE,			ACT_CF_JUMP_START_PISTOL_NUMEN,			false },
	{ ACT_CF_LATCH_LEFT_NUMEN_ACTIVE,			ACT_CF_LATCH_LEFT_PISTOL_NUMEN,			false },
	{ ACT_CF_LATCH_LEFT_CHARGEUP_NUMEN_ACTIVE,	ACT_CF_LATCH_LEFT_CHARGEUP_PISTOL_NUMEN,false },
	{ ACT_CF_LATCH_LEFT_CAST_NUMEN_ACTIVE,		ACT_CF_LATCH_LEFT_CAST_PISTOL_NUMEN,	false },
	{ ACT_CF_LATCH_RIGHT_NUMEN_ACTIVE,			ACT_CF_LATCH_RIGHT_PISTOL_NUMEN,		false },
	{ ACT_CF_LATCH_RIGHT_CHARGEUP_NUMEN_ACTIVE,	ACT_CF_LATCH_RIGHT_CHARGEUP_PISTOL_NUMEN,false },
	{ ACT_CF_LATCH_RIGHT_CAST_NUMEN_ACTIVE,		ACT_CF_LATCH_RIGHT_CAST_PISTOL_NUMEN,	false },
	{ ACT_CF_LATCH_BACK_NUMEN_ACTIVE,			ACT_CF_LATCH_BACK_PISTOL_NUMEN,			false },
	{ ACT_CF_LATCH_BACK_CHARGEUP_NUMEN_ACTIVE,	ACT_CF_LATCH_BACK_CHARGEUP_PISTOL_NUMEN,false },
	{ ACT_CF_LATCH_BACK_CAST_NUMEN_ACTIVE,		ACT_CF_LATCH_BACK_CAST_PISTOL_NUMEN,	false },
};

acttable_t	CWeaponPistol::s_PistolPistolActions[] = 
{
	{ ACT_MP_RUN,						ACT_CF_RUN_PISTOL_PISTOL,						false },
	{ ACT_MP_STAND_IDLE,				ACT_CF_IDLE_PISTOL_PISTOL,						false },
	{ ACT_MP_CROUCH_IDLE,				ACT_CF_IDLE_CROUCH_PISTOL_PISTOL,				false },
	{ ACT_MP_CROUCHWALK,				ACT_CF_WALK_CROUCH_PISTOL_PISTOL,				false },
	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_CF_GESTURE_RANGE_ATTACK_PISTOL_PISTOL,		false },
	{ ACT_MP_RELOAD_STAND,				ACT_CF_GESTURE_RELOAD_PISTOL_PISTOL,			false },
	{ ACT_CF_DRAW_SECONDARY,			ACT_CF_DRAW_SECONDARY_PISTOL_PISTOL,			false },
	{ ACT_CF_S_RANGE_ATTACK,			ACT_CF_S_RANGE_ATTACK_PISTOL_PISTOL,			false },
	{ ACT_CF_S_RELOAD,					ACT_CF_S_RELOAD_PISTOL_PISTOL,					false },
	{ ACT_MP_JUMP_START,				ACT_CF_JUMP_START_PISTOL_PISTOL,				false },
	{ ACT_MP_JUMP_FLOAT,				ACT_CF_JUMP_FLOAT_PISTOL_PISTOL,				false },
	{ ACT_MP_JUMP_LAND,					ACT_CF_JUMP_LAND_PISTOL_PISTOL,					false },
	{ ACT_CF_POWERJUMP_FORWARD,			ACT_CF_POWERJUMP_FORWARD_PISTOL_PISTOL,			false },
	{ ACT_CF_POWERJUMP_UP,				ACT_CF_POWERJUMP_UP_PISTOL_PISTOL,				false },
	{ ACT_CF_POWERJUMP_LEFT,			ACT_CF_POWERJUMP_LEFT_PISTOL_PISTOL,			false },
	{ ACT_CF_POWERJUMP_RIGHT,			ACT_CF_POWERJUMP_RIGHT_PISTOL_PISTOL,			false },
	{ ACT_CF_POWERJUMP_BACK,			ACT_CF_POWERJUMP_BACK_PISTOL_PISTOL,			false },
};

acttable_t *CWeaponPistol::ActivityList( CFWeaponType eSecondary, bool bCommon )
{
	return ActivityListStatic(eSecondary, bCommon);
}

int CWeaponPistol::ActivityListCount( CFWeaponType eSecondary, bool bCommon )
{
	return ActivityListCountStatic(eSecondary, bCommon);
}

acttable_t *CWeaponPistol::ActivityListStatic( CFWeaponType eSecondary, bool bCommon )
{
	if (bCommon)
		return s_CommonActions;

	if (eSecondary == WT_PISTOL)
		return s_PistolPistolActions;

	return s_PistolActions;
}

int CWeaponPistol::ActivityListCountStatic( CFWeaponType eSecondary, bool bCommon )
{
	if (bCommon)
		return ARRAYSIZE(s_CommonActions);

	if (eSecondary == WT_PISTOL)
		return ARRAYSIZE(s_PistolPistolActions);

	return ARRAYSIZE(s_PistolActions);
}

CWeaponPistol::CWeaponPistol()
{
}

bool CWeaponPistol::Deploy( )
{
	CCFPlayer *pPlayer = GetPlayerOwner();
	pPlayer->m_iShotsFired = 0;

	return BaseClass::Deploy();
}

bool CWeaponPistol::Reload( )
{
	CCFPlayer *pPlayer = GetPlayerOwner();

	int iResult = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_IDLE );
	if ( !iResult )
		return false;

	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD, 0, pPlayer->GetSecondaryWeapon() == this );

#ifdef GAME_DLL
	SendReloadEvents();
#endif

#ifndef CLIENT_DLL
	if ((iResult) && (pPlayer->GetFOV() != pPlayer->GetDefaultFOV()))
	{
		pPlayer->SetFOV( pPlayer, pPlayer->GetDefaultFOV() );
	}
#endif

	pPlayer->m_iShotsFired = 0;

	return true;
}

void CWeaponPistol::WeaponIdle()
{
	if (m_flTimeWeaponIdle > gpGlobals->curtime)
		return;

	// only idle if the slid isn't back
	if ( m_iClip1 != 0 )
	{
		SetWeaponIdleTime( gpGlobals->curtime + 5.0f );
	}
}


