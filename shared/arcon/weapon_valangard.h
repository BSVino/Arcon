#ifndef WEAPON_VALANGARD_H
#define WEAPON_VALANGARD_H

#include "cbase.h"
#include "weapon_cfbasemelee.h"

#if defined( CLIENT_DLL )

	#define CWeaponValangard C_WeaponValangard
	#include "c_cf_player.h"

#else

	#include "cf_player.h"

#endif


class CWeaponValangard : public CCFBaseMeleeWeapon
{
public:
	DECLARE_CLASS( CWeaponValangard, CCFBaseMeleeWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponValangard();

	virtual void PrimaryAttack();

	virtual CFWeaponID		GetWeaponID( void ) const	{ return WEAPON_VALANGARD; }

	static acttable_t s_CommonActions[];
	static acttable_t s_BusterActions[];

	acttable_t*				ActivityList( CFWeaponType eSecondary, bool bCommon );
	int						ActivityListCount( CFWeaponType eSecondary, bool bCommon );
	static	acttable_t*		ActivityListStatic( CFWeaponType eSecondary, bool bCommon );
	static	int				ActivityListCountStatic( CFWeaponType eSecondary, bool bCommon );

private:

	CWeaponValangard( const CWeaponValangard & );

};

#endif
