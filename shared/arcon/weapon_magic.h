//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef WEAPON_MAGIC_H
#define WEAPON_MAGIC_H
#ifdef _WIN32
#pragma once
#endif

#include "weapon_cfbase.h"
#include "cf_basegrenade_projectile.h"
#include "runes.h"
#include "armament.h"

#ifdef GAME_DLL
#include "te_effect_dispatch.h"
#include "networkstringtable_gamedll.h"
#else
#include "c_te_effect_dispatch.h"
#include "networkstringtable_clientdll.h"
#endif
#include "particle_parse.h"

void DispatchNumenCastEffect( long iSerializedCombo, CBaseEntity *pEntity, Vector vecPos, QAngle angDir );
void DispatchNumenEffect( element_t eElement, long iNumen, forceeffect_t eForce, CBaseEntity *pEntity, Vector vecCP1, Vector vecCP2 = Vector(0,0,0), float flC1 = 0, float flC2 = 0, float flC3 = 0 );
void DispatchNumenEffect( const char *pszParticleName, ParticleAttachment_t iAttachType, CBaseEntity *pEntity, Vector vecCP1, Vector vecCP2 = Vector(0,0,0), float flC1 = 0, float flC2 = 0, float flC3 = 0 );
void StopNumenEffects( CBaseEntity *pEntity );

#if defined( CLIENT_DLL )
	#define CWeaponMagic C_WeaponMagic
#endif

class CWeaponMagic : public CWeaponCFBase
{
public:
	DECLARE_CLASS( CWeaponMagic, CWeaponCFBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponMagic();

	virtual void	Precache();

	virtual void	Reset();

	virtual void	ItemPostFrame( void );

	virtual void	StartAttack(int iAttack);
	virtual void	StopAttack();

	virtual void	StartCharge(int iAttack);
	virtual void	StopCharge(bool bMultiplier = true);
	virtual void	ResetCharge();
	virtual float	GetChargeAmount() { return m_flChargeAmount; };
	virtual float	GetChargeMultiplier() { return m_flChargeMultiplier; };
	virtual float	GetChargeStartTime() { return m_flChargeStartTime; };
	virtual bool	IsCharging(bool bDelay = true);

	virtual void	Attack();
	virtual void	AttackBulletForce(float flDamage, element_t iElement);
	virtual void	AttackAreaEffectForce(float flDamage, element_t iElement);
	virtual void	AttackBlastForce(float flDamage, element_t iElement);
	virtual void	AttackProjectileForce(float flDamage, element_t iElement);
	virtual void	AttackSoftNadeForce(float flDamage, element_t iElement);
	virtual void	AttackSound(char* pszType, element_t iElement);

	static void		HealPlayer(CCFPlayer* pTarget, CCFPlayer* pAttacker, float flHeal);

	virtual void	ThrowGrenade(float flDamage, long iNumen, element_t iElement, forceeffect_t eForceEffect, statuseffect_t eStatusEffect, float flStatusMagnitude, float flHealthDrain, float flStaminaDrain, float flFocusDrain);
	static void		Explode(CBaseCombatCharacter* pAttacker,
		CBaseEntity* pInflictor,
		CBaseEntity* pIgnore,
		Vector vecPosition,
		float flDamage,
		float flRadius,
		element_t iElement,
		statuseffect_t eStatusEffects,
		float flStatusMagnitude,
		forceeffect_t eForce,
		long iNumen,
		float flDrainHealth,
		float flDrainFocus,
		float flDrainStamina);
#ifdef GAME_DLL
	virtual class CMagicGrenade*	EmitGrenade(Vector vecSrc, QAngle vecAngles, Vector vecVel, AngularImpulse angImpulse);
#endif

	virtual CFWeaponID GetWeaponID( void ) const		{ return WEAPON_MAGIC; }

	static acttable_t s_ReadyActions[];
	static acttable_t s_ActiveActions[];

	acttable_t*				ActivityList(bool bActive);
	int						ActivityListCount(bool bActive);
	static	acttable_t*		ActivityListStatic(bool bActive);
	static	int				ActivityListCountStatic(bool bActive);
	static Activity			ActivityOverride( Activity baseAct, bool bActive );

#ifdef GAME_DLL
	virtual int		UpdateTransmitState( void);
#endif

	CNetworkVar(float, 		m_flCastTime);
	CNetworkVar(float, 		m_flLastCast);

	CNetworkVar(bool,		m_bCharging);
	CNetworkVar(float,		m_flChargeStartTime);
	CNetworkVar(int,		m_iChargeAttack);
	float					m_flChargeMultiplier;
	float					m_flChargeAmount;

	CRunePosition	m_CastBind;
	bool			m_bRightHandCast;

	int				m_iLeftHand;
	int				m_iRightHand;

private:

	CWeaponMagic( const CWeaponMagic & );

};

#if defined( CLIENT_DLL )
	#define CMagicGrenade C_MagicGrenade
#endif

class CMagicGrenade : public CBaseGrenadeProjectile
{
public:
	DECLARE_CLASS( CMagicGrenade, CBaseGrenadeProjectile );

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();
#endif

#ifdef GAME_DLL
	static CMagicGrenade* Create(
		const Vector &position,
		const QAngle &angles,
		const Vector &velocity,
		const AngularImpulse &angVelocity,
		CBaseCombatCharacter *pOwner,
		float flTimer );

	virtual void Spawn();
	virtual void Precache();
	virtual void BounceSound( void );
	virtual void Detonate();
	virtual void DirectDamage(CBaseEntity* pHitEntity);
	virtual void RemoveMe();

	virtual void DetonateTouch( CBaseEntity *pOther );
	virtual void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );

	virtual int		UpdateTransmitState( void);
#endif

#ifdef CLIENT_DLL
	virtual void	OnDataChanged( DataUpdateType_t updateType );

	virtual ShadowType_t	ShadowCastType() { return SHADOWS_NONE; };

	bool			m_bOldCharging;
	bool			m_bOldAfflictive;

	CUtlVector<CNewParticleEffect*>	m_apEffects;
#endif

	virtual void			SetDamage(float flDamage) { m_flRealDamage = flDamage; };
	virtual float			GetDamage() { return m_flRealDamage; };
	virtual void			SetNumen(long iNumen);
	virtual long			GetNumen();
	virtual void			SetElement(element_t eElement) {m_eElement = eElement;};
	virtual element_t		GetElement() {return m_eElement;};
	virtual void			SetStatusEffect(statuseffect_t eStatusEffect) {m_eStatusEffect = eStatusEffect;};
	virtual statuseffect_t	GetStatusEffect() {return m_eStatusEffect;};
	virtual void			SetStatusMagnitude(float flStatusMagnitude) {m_flStatusMagnitude = flStatusMagnitude;};
	virtual float			GetStatusMagnitude() {return m_flStatusMagnitude;};
	virtual void			SetForceEffect(forceeffect_t eForceEffect) {m_eForceEffect = eForceEffect;};
	virtual forceeffect_t	GetForceEffect() {return m_eForceEffect;};
	virtual void			SetDrainage(float flHealth, float flStamina, float flFocus) { m_flHealthDrain = flHealth; m_flStaminaDrain = flStamina; m_flFocusDrain = flFocus; };
	virtual float			GetHealthDrainage() { return m_flHealthDrain; };
	virtual float			GetStaminaDrainage() { return m_flStaminaDrain; };
	virtual float			GetFocusDrainage() { return m_flFocusDrain; };

	CNetworkVar(float,		m_flRealDamage);	// Base class damage doesn't support negative values.
	CNetworkArray( bool, m_iNumen, MAX_RUNEEFFECTS );	// This isn't entirely accurate, because the number of runes parsed from
														// runes.txt is not necessarily less than MAX_RUNEFFECTS, but probably is.
	CNetworkVar(element_t,	m_eElement);
	CNetworkVar(statuseffect_t,	m_eStatusEffect);
	float					m_flStatusMagnitude;
	forceeffect_t			m_eForceEffect;
	float					m_flHealthDrain;
	float					m_flStaminaDrain;
	float					m_flFocusDrain;
};

#endif // WEAPON_MAGIC_H
