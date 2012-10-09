//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "weapon_cfbasemelee.h"

#include "weapon_pariah.h"

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponPariahBlade, DT_WeaponPariahBlade )

BEGIN_NETWORK_TABLE( CWeaponPariahBlade, DT_WeaponPariahBlade )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponPariahBlade )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_pariah, CWeaponPariahBlade );
PRECACHE_WEAPON_REGISTER( weapon_pariah );

acttable_t CWeaponPariahBlade::s_CommonActions[] = 
{
	{ ACT_CF_EXECUTE_ANYTHING,			ACT_CF_EXECUTE_UNARMED_UNARMED,			false },
	{ ACT_CF_EXECUTED_ANYTHING,			ACT_CF_EXECUTED_UNARMED_UNARMED,		false },
	{ ACT_CF_LATCH_JUMP_START,			ACT_CF_LATCH_JUMP_START_PARIAHBLADE,	false },
	{ ACT_CF_LATCH_LEFT,				ACT_CF_LATCH_LEFT_PARIAHBLADE,			false },
	{ ACT_CF_LATCH_LEFT_CHARGE,			ACT_CF_LATCH_LEFT_CHARGE_PARIAHBLADE,	false },
	{ ACT_CF_LATCH_LEFT_WEAK_ATTACK,	ACT_CF_LATCH_LEFT_WEAK_ATTACK_PARIAHBLADE,false },
	{ ACT_CF_LATCH_LEFT_STRONG_ATTACK,	ACT_CF_LATCH_LEFT_STRONG_ATTACK_PARIAHBLADE,false },
	{ ACT_CF_LATCH_RIGHT,				ACT_CF_LATCH_RIGHT_PARIAHBLADE,			false },
	{ ACT_CF_LATCH_RIGHT_CHARGE,		ACT_CF_LATCH_RIGHT_CHARGE_PARIAHBLADE,	false },
	{ ACT_CF_LATCH_RIGHT_WEAK_ATTACK,	ACT_CF_LATCH_RIGHT_WEAK_ATTACK_PARIAHBLADE,false },
	{ ACT_CF_LATCH_RIGHT_STRONG_ATTACK,	ACT_CF_LATCH_RIGHT_STRONG_ATTACK_PARIAHBLADE,false },
	{ ACT_CF_LATCH_BACK,				ACT_CF_LATCH_BACK_PARIAHBLADE,			false },
	{ ACT_CF_LATCH_BACK_CHARGE,			ACT_CF_LATCH_BACK_CHARGE_PARIAHBLADE,	false },
	{ ACT_CF_LATCH_BACK_WEAK_ATTACK,	ACT_CF_LATCH_BACK_WEAK_ATTACK_PARIAHBLADE,false },
	{ ACT_CF_LATCH_BACK_STRONG_ATTACK,	ACT_CF_LATCH_BACK_STRONG_ATTACK_PARIAHBLADE,false },
};

acttable_t CWeaponPariahBlade::s_LongswordActions[] = 
{
	{ ACT_MP_RUN,						ACT_CF_RUN_PARIAHBLADE,					false },
	{ ACT_MP_STAND_IDLE,				ACT_CF_IDLE_PARIAHBLADE,				false },
	{ ACT_MP_CROUCH_IDLE,				ACT_CF_IDLE_CROUCH_PARIAHBLADE,			false },
	{ ACT_MP_CROUCHWALK,				ACT_CF_WALK_CROUCH_PARIAHBLADE,			false },
	{ ACT_CF_CHARGE,					ACT_CF_CHARGE_PARIAHBLADE,				false },
	{ ACT_CF_WEAK_ATTACK,				ACT_CF_WEAK_ATTACK_PARIAHBLADE,			false },
	{ ACT_CF_STRONG_ATTACK,				ACT_CF_STRONG_ATTACK_PARIAHBLADE,		false },
	{ ACT_CF_C_ATTACK,					ACT_CF_C_ATTACK_PARIAHBLADE,			false },
	{ ACT_CF_BLOCK,						ACT_CF_BLOCK_PARIAHBLADE,				false },
	{ ACT_CF_BLOCKED,					ACT_CF_BLOCKED_PARIAHBLADE,				false },
	{ ACT_CF_DFA_READY,					ACT_CF_DFA_READY_PARIAHBLADE,			false },
	{ ACT_CF_DFA_ATTACK,				ACT_CF_DFA_ATTACK_PARIAHBLADE,			false },
	{ ACT_MP_JUMP_START,				ACT_CF_JUMP_START_PARIAHBLADE,			false },
	{ ACT_MP_JUMP_FLOAT,				ACT_CF_JUMP_FLOAT_PARIAHBLADE,			false },
	{ ACT_MP_JUMP_LAND,					ACT_CF_JUMP_LAND_PARIAHBLADE,			false },
	{ ACT_CF_POWERJUMP_FORWARD,			ACT_CF_POWERJUMP_FORWARD_PARIAHBLADE,	false },
	{ ACT_CF_POWERJUMP_UP,				ACT_CF_POWERJUMP_UP_PARIAHBLADE,		false },
	{ ACT_CF_POWERJUMP_LEFT,			ACT_CF_POWERJUMP_LEFT_PARIAHBLADE,		false },
	{ ACT_CF_POWERJUMP_RIGHT,			ACT_CF_POWERJUMP_RIGHT_PARIAHBLADE,		false },
	{ ACT_CF_POWERJUMP_BACK,			ACT_CF_POWERJUMP_BACK_PARIAHBLADE,		false },
};

acttable_t *CWeaponPariahBlade::ActivityList( CFWeaponType eSecondary, bool bCommon )
{
	return ActivityListStatic(eSecondary, bCommon);
}

int CWeaponPariahBlade::ActivityListCount( CFWeaponType eSecondary, bool bCommon )
{
	return ActivityListCountStatic(eSecondary, bCommon);
}

acttable_t *CWeaponPariahBlade::ActivityListStatic( CFWeaponType eSecondary, bool bCommon )
{
	if (bCommon)
		return s_CommonActions;

	return s_LongswordActions;
}

int CWeaponPariahBlade::ActivityListCountStatic( CFWeaponType eSecondary, bool bCommon )
{
	if (bCommon)
		return ARRAYSIZE(s_CommonActions);

	return ARRAYSIZE(s_LongswordActions);
}

CWeaponPariahBlade::CWeaponPariahBlade()
{
#ifdef GAME_DLL
	AddSpawnFlags(SF_NORESPAWN);
#endif
}

void CWeaponPariahBlade::DefaultTouch( CBaseEntity *pOther )
{
	// Skip CWeaponCFBase's version, which disables this functionality.
	CBaseCombatWeapon::DefaultTouch(pOther);
}
