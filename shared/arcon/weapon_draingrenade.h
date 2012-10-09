#ifndef WEAPON_DRAINGRENADE_H
#define WEAPON_DRAINGRENADE_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "weapon_cfbase.h"
#include "cf_basegrenade_projectile.h"
#include "runes.h"
#include "armament.h"

#if defined( CLIENT_DLL )

	#define CWeaponDrainGrenade C_WeaponDrainGrenade
	#include "c_cf_player.h"
	
#else

	#include "cf_player.h"

#endif

#include "particle_parse.h"

class CWeaponDrainGrenade : public CBaseGrenadeProjectile
{
public:
	DECLARE_CLASS( CWeaponDrainGrenade, CBaseGrenadeProjectile );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	#if !defined( CLIENT_DLL )
		DECLARE_DATADESC();
	#endif
	
	//CWeaponDrainGrenade();

	#ifdef GAME_DLL
		static CWeaponDrainGrenade* Create(
			const Vector &position,
			const QAngle &angles,
			const Vector &velocity,
			CBaseCombatCharacter *pOwner);

		virtual void Spawn();
		virtual void Precache();
		virtual void BounceSound( void );
		virtual void RemoveMe();
		virtual void Detonate();

		virtual void DetonateTouch( CBaseEntity *pOther );
		virtual void RadiusDamage( const CTakeDamageInfo &inputInfo, const Vector &vecSrcIn, float flRadius );

		virtual int		UpdateTransmitState( void);

	#endif

	#ifdef CLIENT_DLL
		virtual void	OnDataChanged( DataUpdateType_t updateType );

		CUtlVector<CNewParticleEffect*>	m_apEffects;
	#endif

	void					Explode(	CBaseCombatCharacter* pAttacker,
										CBaseEntity* pInflictor,
										CBaseEntity* pIgnore,
										Vector vecPosition,
										float flDamage,
										float flRadius,
										float flDrainFocus );

	virtual float			GetFocusDrain() { return m_flFocusDrain; };
	virtual void			SetFocusDrain(float flFocus) { m_flFocusDrain = flFocus; }; 

	CNetworkVar(float,		m_flRealDamage);
	float					m_flFocusDrain;
	

};

#endif // WEAPON_DRAINGRENADE_H
