//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef WEAPON_FLAG_H
#define WEAPON_FLAG_H
#ifdef _WIN32
#pragma once
#endif

#include "weapon_cfbase.h"
#include "runes.h"
#include "armament.h"

#if defined( CLIENT_DLL )
	#define CWeaponFlag C_WeaponFlag
#endif

class CWeaponFlag : public CWeaponCFBase
{
public:
	DECLARE_CLASS( CWeaponFlag, CWeaponCFBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponFlag();

	virtual void	Precache();

	virtual bool	DefaultDeploy( char *szViewModel, char *szWeaponModel, int iActivity, char *szAnimExt );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );

	virtual void	ItemPostFrame( void );

	virtual CFWeaponID GetWeaponID( void ) const		{ return WEAPON_FLAG; }

	static acttable_t s_Actions[];

	acttable_t*				ActivityList();
	int						ActivityListCount();
	static	acttable_t*		ActivityListStatic();
	static	int				ActivityListCountStatic();
	Activity				ActivityOverride( Activity baseAct, bool *pRequired );

#ifdef GAME_DLL
	virtual int		UpdateTransmitState( void);
#endif

private:
	CWeaponFlag( const CWeaponFlag & );

};

#endif // WEAPON_FLAG_H
