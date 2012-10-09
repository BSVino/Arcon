#ifndef WEAPON_RIVENBLADE_H
#define WEAPON_RIVENBLADE_H

#include "cbase.h"
#include "weapon_cfbasemelee.h"

#if defined( CLIENT_DLL )

	#define CWeaponRivenBlade C_WeaponRivenBlade
	#include "c_cf_player.h"

#else

	#include "cf_player.h"

#endif


class CWeaponRivenBlade : public CCFBaseMeleeWeapon
{
public:
	DECLARE_CLASS( CWeaponRivenBlade, CCFBaseMeleeWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponRivenBlade();

	virtual void PrimaryAttack();

	virtual CFWeaponID		GetWeaponID( void ) const	{ return WEAPON_RIVENBLADE; }

	static acttable_t s_CommonActions[];
	static acttable_t s_LongswordActions[];
	static acttable_t s_LongswordPistolActions[];
	static acttable_t s_LongswordLongswordActions[];

	acttable_t*				ActivityList( CFWeaponType eSecondary, bool bCommon );
	int						ActivityListCount( CFWeaponType eSecondary, bool bCommon );
	static	acttable_t*		ActivityListStatic( CFWeaponType eSecondary, bool bCommon );
	static	int				ActivityListCountStatic( CFWeaponType eSecondary, bool bCommon );

private:

	CWeaponRivenBlade( const CWeaponRivenBlade & );

};

#endif
