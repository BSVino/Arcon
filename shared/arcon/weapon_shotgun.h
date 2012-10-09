#ifndef WEAPON_SHOTGUN_H
#define WEAPON_SHOTGUN_H

#include "cbase.h"
#include "weapon_cfbase.h"

#if defined( CLIENT_DLL )

	#define CWeaponShotgun C_WeaponShotgun
	#include "c_cf_player.h"

#else

	#include "cf_player.h"

#endif


class CWeaponShotgun : public CWeaponCFBase
{
public:
	DECLARE_CLASS( CWeaponShotgun, CWeaponCFBase );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	
	CWeaponShotgun();

	virtual bool IsFullAuto() { return false; };

	virtual bool Deploy();
	virtual bool Reload();
	virtual void WeaponIdle();
	virtual void PrimaryAttack();

	virtual void SecondaryAttack( void );
	virtual void FireGrenade( void );

	#ifdef GAME_DLL
		virtual class CWeaponDrainGrenade* EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel );
	#endif

	virtual CFWeaponID		GetWeaponID( void ) const	{ return WEAPON_SHOTGUN; }

	static acttable_t s_CommonActions[];
	static acttable_t s_ShotgunActions[];
	static acttable_t s_ShotgunPistolActions[];

	acttable_t*				ActivityList( CFWeaponType eSecondary, bool bCommon );
	int						ActivityListCount( CFWeaponType eSecondary, bool bCommon );
	static	acttable_t*		ActivityListStatic( CFWeaponType eSecondary, bool bCommon );
	static	int				ActivityListCountStatic( CFWeaponType eSecondary, bool bCommon );

private:

	CWeaponShotgun( const CWeaponShotgun & );

	void Fire( float flSpread );
};

#endif
