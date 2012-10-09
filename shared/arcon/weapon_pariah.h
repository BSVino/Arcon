#ifndef WEAPON_PARIAH_H
#define WEAPON_PARIAH_H

#include "cbase.h"
#include "weapon_cfbasemelee.h"

#if defined( CLIENT_DLL )

	#define CWeaponPariahBlade C_WeaponPariahBlade
	#include "c_cf_player.h"

#else

	#include "cf_player.h"

#endif


class CWeaponPariahBlade : public CCFBaseMeleeWeapon
{
public:
	DECLARE_CLASS( CWeaponPariahBlade, CCFBaseMeleeWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponPariahBlade();

	virtual void 			DefaultTouch( CBaseEntity *pOther );

	virtual CFWeaponID		GetWeaponID( void ) const	{ return WEAPON_PARIAH; }

	static acttable_t s_CommonActions[];
	static acttable_t s_LongswordActions[];
	static acttable_t s_LongswordPistolActions[];
	static acttable_t s_LongswordLongswordActions[];

	acttable_t*				ActivityList( CFWeaponType eSecondary, bool bCommon );
	int						ActivityListCount( CFWeaponType eSecondary, bool bCommon );
	static	acttable_t*		ActivityListStatic( CFWeaponType eSecondary, bool bCommon );
	static	int				ActivityListCountStatic( CFWeaponType eSecondary, bool bCommon );

private:

	CWeaponPariahBlade( const CWeaponPariahBlade & );

};

#endif