#ifndef CFBASEMELEEWEAPON_H
#define CFBASEMELEEWEAPON_H

#include "weapon_cfbase.h"
#include "cf_shareddefs.h"

#if defined( CLIENT_DLL )
#define CCFBaseMeleeWeapon C_CFBaseMeleeWeapon
#endif

class CCFBaseMeleeWeapon : public CWeaponCFBase
{
	DECLARE_CLASS( CCFBaseMeleeWeapon, CWeaponCFBase );

public:
	CCFBaseMeleeWeapon();

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual	void	Spawn( void );
	virtual	void	Precache( void );
	
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void	Drop( const Vector &vecVelocity );

	//Attack functions
	virtual	void	PrimaryAttack( void );

	virtual int		GetAttackFromMovement();

	virtual void	SetSwingTime( float flTime );
	virtual void	StopMeleeAttack(bool bDualAware = false, bool bInterruptChain = true);
	virtual void	CalculateStrongAttack();

	virtual void	StartCharge(int iAttack);
	virtual void	StopCharge(bool bMultiplier = true, float flFreeze = 0.0f);
	virtual void	ResetCharge();
	virtual float	GetChargeAmount() { return m_flChargeAmount; };
	virtual float	GetChargeMultiplier() { return m_flChargeMultiplier; };
	virtual float	GetChargeStartTime() { return m_flChargeStartTime; };
	virtual bool	IsCharging(bool bDelay = true);

	virtual float	GetChainMultiplier();

	virtual void	ItemPostFrame( void );

	virtual bool	CanBlockBullets() { return true; };
	virtual void	BulletBlocked(int iDamageType);
	virtual void	ShowBlock();
	virtual bool	IsStrongAttack();
	virtual void	EndRush();

#ifdef CLIENT_DLL
	virtual void	ClientThink();
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	CreateChargeEffect();
	virtual void	CreateSwingEffect();
	
	bool			m_bOldCharging;
	bool			m_bOldAfflictive;
#endif

	virtual float	GetFireRate( void );
	virtual bool	IsFullAuto() { return false; };

	CCFBaseMeleeWeapon( const CCFBaseMeleeWeapon & );

	CNetworkVar(int, m_iAttack);
	CNetworkVar(bool, m_bStrongAttack);

	CNetworkVar(float, m_flLastAttackTime);
	CNetworkVar(float, m_flChargeStartTime);
	CNetworkVar(float, m_flSwingTime);
	CNetworkVar(bool, m_bCharging);
	CNetworkVar(bool, m_bAfflictive);
	CNetworkVar(element_t, m_eAttackElement);
	float			m_flChargeMultiplier;
	float			m_flChargeAmount;

protected:
	virtual void	Rush( bool bDownStrike = false );
	virtual void	Swing( );
	virtual void	Afflict( bool bFirst = false );
	virtual bool	Trace( Vector vecStart, Vector vecEnd, Color d );
	virtual void	Hit( trace_t &traceHit );

	int				m_iLastFrame;
	Vector			m_vecLastTip;
	Vector			m_vecLastHilt;

	bool			m_bHitThisSwing;	// Did we hit something during this swing?

	int				m_iTipAttachment;
	int				m_iHiltAttachment;

	CUtlVector< CHandle< CBasePlayer > >	m_aHitList;

#ifdef CLIENT_DLL
	CNewParticleEffect*	m_pChargeEffect;
	CUtlVector< CNewParticleEffect* > m_apSwingEffects;
#endif
};

#endif // CFBASEMELEEWEAPON_H
