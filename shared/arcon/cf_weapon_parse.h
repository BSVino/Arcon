//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef CF_WEAPON_PARSE_H
#define CF_WEAPON_PARSE_H
#ifdef _WIN32
#pragma once
#endif


#include "weapon_parse.h"
#include "networkvar.h"
#include "cf_shareddefs.h"

//--------------------------------------------------------------------------------------------------------
class CCFWeaponInfo : public FileWeaponInfo_t
{
public:
	DECLARE_CLASS_GAMEROOT( CCFWeaponInfo, FileWeaponInfo_t );
	
	CCFWeaponInfo();
	
	virtual void Parse( ::KeyValues *pKeyValuesData, const char *szWeaponName );

	static enum weapontype_e	StringToWeaponType( const char* szString );
	static const char*			WeaponTypeToString( weapontype_t eWeapon );
	
	// Parameters for FX_FireBullets:
	int		m_iDamage;
	int		m_iBullets;
	float	m_flCycleTime;
	int		m_iDrainDamage;

	int		m_iStrongDamage;
	float	m_flStrongCycleTime;

	bool	m_bPrimaryInLeft;

	enum weapontype_e		m_eWeaponType;
	unsigned int			m_iCost;
	float					m_flAttack;
	float					m_flEnergy;
	int						m_iRuneSlots[MAX_RUNES];
	float					m_flAccuracy;
	float					m_flSpeed;
	float					m_flFocusRegen;
	float					m_flStaminaRegen;
	float					m_flCritical;
	float					m_flBulletBlock;

	float					m_flPermRecoil;
	float					m_flTempRecoil;
};


#endif // CF_WEAPON_PARSE_H
