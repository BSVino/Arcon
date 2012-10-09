#ifndef WEAPON_RIFLE_H
#define WEAPON_RIFLE_H

#include "cbase.h"
#include "weapon_cfbase.h"

#if defined( CLIENT_DLL )

	#define CWeaponRifle C_WeaponRifle
	#include "c_cf_player.h"
	
#else

	#include "cf_player.h"

#endif

class CWeaponRifle : public CWeaponCFBase
{
public:
	DECLARE_CLASS( CWeaponRifle, CWeaponCFBase );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	
	CWeaponRifle();

	virtual void Precache();

	virtual bool IsFullAuto() { return true; };

	virtual bool Deploy();
	virtual bool Reload();
	virtual void WeaponIdle();
	virtual void PrimaryAttack();

	virtual void SecondaryAttack( void );
	virtual void FireGrenade( void );

	#ifdef GAME_DLL
		virtual class CWeaponDrainGrenade* EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel );
	#endif

	virtual CFWeaponID		GetWeaponID( void ) const	{ return WEAPON_RIFLE; }

	static acttable_t s_CommonActions[];
	static acttable_t s_RifleActions[];
	static acttable_t s_RiflePistolActions[];

	acttable_t*				ActivityList( CFWeaponType eSecondary, bool bCommon );
	int						ActivityListCount( CFWeaponType eSecondary, bool bCommon );
	static	acttable_t*		ActivityListStatic( CFWeaponType eSecondary, bool bCommon );
	static	int				ActivityListCountStatic( CFWeaponType eSecondary, bool bCommon );

private:

	CWeaponRifle( const CWeaponRifle & );

	void Fire( float flSpread );
};

#endif
