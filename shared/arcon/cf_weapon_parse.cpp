//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include <KeyValues.h>
#include "cf_weapon_parse.h"
#include "cf_shareddefs.h"

FileWeaponInfo_t* CreateWeaponInfo()
{
	return new CCFWeaponInfo;
}


CCFWeaponInfo::CCFWeaponInfo()
{
	m_eWeaponType = INVALID_WEAPONTYPE;
	m_iDrainDamage = 0;
	m_iCost = 0;
	m_flAttack = 0;
	m_flEnergy = 0;
	for (int i = 0; i < MAX_RUNES; i++)
		m_iRuneSlots[i] = 0;
	m_flAccuracy = 0;
	m_flFocusRegen = 0;
	m_flStaminaRegen = 0;
	m_flCritical = 0;
	m_flBulletBlock = 0;
}


void CCFWeaponInfo::Parse( KeyValues *pKeyValuesData, const char *szWeaponName )
{
	BaseClass::Parse( pKeyValuesData, szWeaponName );

	m_iDamage		= pKeyValuesData->GetInt( "Damage", 42 ); // Douglas Adams 1952 - 2001
	m_iBullets		= pKeyValuesData->GetInt( "Bullets", 1 );
	m_flCycleTime	= pKeyValuesData->GetFloat( "CycleTime", 0.15 );
	m_iDrainDamage	= pKeyValuesData->GetInt( "DrainDamage", 42 );

	m_iStrongDamage		= pKeyValuesData->GetInt( "StrongDamage", 84 ); // 2 * Douglas Adams 1952 - 2001
	m_flStrongCycleTime	= pKeyValuesData->GetFloat( "StrongCycleTime", 0.3 );

	m_bPrimaryInLeft	= !!pKeyValuesData->GetInt( "PrimaryInLeft", 0 );

	m_iCost = pKeyValuesData->GetInt( "cost", 0 );
	m_flAttack = pKeyValuesData->GetFloat( "attack", 1 );
	m_flEnergy = pKeyValuesData->GetFloat( "energy", 1 );
	for (int i = 0; i < MAX_RUNES; i++)
		m_iRuneSlots[i] = pKeyValuesData->GetInt( VarArgs("rune%d_slots", i+1), 0 );
	m_flAccuracy = pKeyValuesData->GetFloat( "accuracy", 0 );
	m_flSpeed = pKeyValuesData->GetFloat( "speed", 0 );
	m_flFocusRegen = pKeyValuesData->GetFloat( "focus", 0 );
	m_flStaminaRegen = pKeyValuesData->GetFloat( "stamina", 0 );
	m_flCritical = pKeyValuesData->GetFloat( "critical", 0 );
	m_flBulletBlock = pKeyValuesData->GetFloat( "bulletblock", 0 );

	const char *pszWeaponType = pKeyValuesData->GetString( "weapon_type", "Default" );
	Assert( strcmp("Default", pszWeaponType) );
	m_eWeaponType = StringToWeaponType( pszWeaponType );

	m_flPermRecoil				= pKeyValuesData->GetFloat( "permrecoil", 99.0 );
	m_flTempRecoil				= pKeyValuesData->GetFloat( "temprecoil", 99.0 );
}

weapontype_t CCFWeaponInfo::StringToWeaponType( const char* szString )
{
	for (int i = 0; i < NUMBER_WEAPONTYPES; i++)
	{
		if (Q_strcmp(szString, g_szWeaponTypes[i]) == 0)
			return (weapontype_t)i;
	}
	return INVALID_WEAPONTYPE;
}

const char* CCFWeaponInfo::WeaponTypeToString( weapontype_t eWeapon )
{
	if (eWeapon == INVALID_WEAPONTYPE)
		return "invalid";

	if (eWeapon < 0 || eWeapon >= NUMBER_WEAPONTYPES)
		return "invalid";

	return g_szWeaponTypes[eWeapon];
}

