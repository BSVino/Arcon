#ifndef WEAPON_PISTOL_H
#define WEAPON_PISTOL_H

#include "cbase.h"
#include "weapon_cfbase.h"

#if defined( CLIENT_DLL )

	#define CWeaponPistol C_WeaponPistol
	#include "c_cf_player.h"

#else

	#include "cf_player.h"

#endif


class CWeaponPistol : public CWeaponCFBase
{
public:
	DECLARE_CLASS( CWeaponPistol, CWeaponCFBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponPistol();

	virtual bool IsFullAuto() { return false; };

	virtual bool Deploy();
	virtual bool Reload();
	virtual void WeaponIdle();

	virtual CFWeaponID		GetWeaponID( void ) const	{ return WEAPON_PISTOL; }

	static acttable_t s_CommonActions[];
	static acttable_t s_PistolActions[];
	static acttable_t s_PistolPistolActions[];

	acttable_t*				ActivityList( CFWeaponType eSecondary, bool bCommon );
	int						ActivityListCount( CFWeaponType eSecondary, bool bCommon );
	static	acttable_t*		ActivityListStatic( CFWeaponType eSecondary, bool bCommon );
	static	int				ActivityListCountStatic( CFWeaponType eSecondary, bool bCommon );

private:

	CWeaponPistol( const CWeaponPistol & );

	void Fire( float flSpread );
};

#endif
