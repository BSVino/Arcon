	//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "in_buttons.h"
#include "tempentity.h"

#if defined( CLIENT_DLL )

	#include "c_cf_player.h"
	#include "cf_in_main.h"

#else

	#include "cf_player.h"
	#include "baseentity.h"

#endif

#include "weapon_flag.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponFlag, DT_WeaponFlag )

BEGIN_NETWORK_TABLE( CWeaponFlag, DT_WeaponFlag )
#ifdef GAME_DLL
	SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),
#else
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponFlag )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_flag, CWeaponFlag );
PRECACHE_WEAPON_REGISTER( weapon_flag );

acttable_t CWeaponFlag::s_Actions[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_CF_IDLE_FLAG,				false },
	{ ACT_MP_RUN,						ACT_CF_RUN_FLAG,				false },
	{ ACT_MP_CROUCH_IDLE,				ACT_CF_IDLE_FLAG,				false },
	{ ACT_MP_CROUCHWALK,				ACT_CF_RUN_FLAG,				false },
	{ ACT_MP_JUMP_START,				ACT_CF_JUMP_START_FLAG,			false },
	{ ACT_MP_JUMP_FLOAT,				ACT_CF_JUMP_FLOAT_FLAG,			false },
	{ ACT_MP_JUMP_LAND,					ACT_CF_JUMP_LAND_FLAG,			false },
	{ ACT_CF_LATCH_LEFT,				ACT_CF_LATCH_LEFT_FLAG,			false },
	{ ACT_CF_LATCH_RIGHT,				ACT_CF_LATCH_RIGHT_FLAG,		false },
	{ ACT_CF_LATCH_BACK,				ACT_CF_LATCH_BACK_FLAG,			false },
};

Activity CWeaponFlag::ActivityOverride( Activity baseAct, bool *pRequired )
{
	acttable_t *pTable;
	int actCount;
	int i;

	pTable = ActivityListStatic();
	actCount = ActivityListCountStatic();

	for ( i = 0; i < actCount; i++, pTable++ )
	{
		if ( baseAct == pTable->baseAct )
		{
			return (Activity)pTable->weaponAct;
		}
	}

	return baseAct;
}

acttable_t* CWeaponFlag::ActivityList()
{
	return ActivityListStatic();
}

int CWeaponFlag::ActivityListCount()
{
	return ActivityListCountStatic();
}

acttable_t* CWeaponFlag::ActivityListStatic()
{
	return s_Actions;
}

int CWeaponFlag::ActivityListCountStatic()
{
	return ARRAYSIZE(s_Actions);
}

CWeaponFlag::CWeaponFlag()
{
}

void CWeaponFlag::Precache( void )
{
	BaseClass::Precache();
}

bool CWeaponFlag::DefaultDeploy( char *szViewModel, char *szWeaponModel, int iActivity, char *szAnimExt )
{
	// Nothing!
	return true;
}

bool CWeaponFlag::Holster( CBaseCombatWeapon *pSwitchingTo )
{ 
	return true;
}


void CWeaponFlag::ItemPostFrame( void )
{
	CCFPlayer *pOwner = ToCFPlayer( GetOwner() );
	if (!pOwner)
		return;

#ifdef GAME_DLL
	if (pOwner->m_afButtonPressed & (IN_ATTACK|IN_ATTACK2))
		pOwner->GetObjective()->Drop();
#endif
}

#ifdef GAME_DLL
int CWeaponFlag::UpdateTransmitState( void)
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}
#endif

