#ifndef STATISTICS_H
#define STATISTICS_H

#include "takedamageinfo.h"

#ifdef GAME_DLL
#include "cf_player.h"
#else
#include "c_cf_player.h"
#endif

class CRuneInfo;
class CRunePosition;

// HLP and STP are unsigned shorts, so don't make them > 65535
// HLR and STR are floats
// ATT NRG DEF RES and SPD are chars, so not > 127
// CRT is an unsigned char, so not > 255
#define STAT_HLP_MAX	(4096.0f)
#define STAT_HLP_MIN	(0.0f)
#define STAT_FOP_MAX	(256.0f)
#define STAT_FOP_MIN	(-128.0f)
#define STAT_STP_MAX	(256.0f)
#define STAT_STP_MIN	(-128.0f)
#define STAT_HLR_MAX	(16.0f)
#define STAT_HLR_MIN	(0.0f)
#define STAT_FOR_MAX	(16.0f)
#define STAT_FOR_MIN	(0.0f)
#define STAT_STR_MAX	(64.0f)
#define STAT_STR_MIN	(0.0f)
#define STAT_ATT_MAX	(120.0f)
#define STAT_ATT_MIN	(-80.0f)
#define STAT_NRG_MAX	(120.0f)
#define STAT_NRG_MIN	(-80.0f)
#define STAT_DEF_MAX	(80.0f)
#define STAT_DEF_MIN	(-120.0f)
#define STAT_RES_MAX	(80.0f)
#define STAT_RES_MIN	(-120.0f)
#define STAT_SPD_MAX	(120.0f)
#define STAT_SPD_MIN	(-80.0f)
#define STAT_CRT_MAX	(100.0f)
#define STAT_CRT_MIN	(0.0f)

// What percentage of his total health a player with a 1:1 K/D ratio must sustain to go into overdrive.
#define STAT_OVERDRIVE_DAMAGE		(1.5f)
#define STATISTICS_FRAMETIME		1.0f
#define STATISTICS_DEGRADE_RATE		((float)1/30)

class CStatistics
{
	DECLARE_SIMPLE_DATADESC();
	DECLARE_CLASS_NOBASE( CStatistics );

public:
	CStatistics(CCFPlayer* pPlayer);

	void	InitStats();

	void	StatisticsThink();
	float	m_flNextStatsThink;
	CNetworkVarForDerived( float, m_flLastStatsThink );

	int		TakeDamage(CTakeDamageInfo &info);
	void	Event_Killed( const CTakeDamageInfo &info );
	void	Event_Knockout( const CTakeDamageInfo &info );

	void			AddStatusEffects( const unsigned short iEffects, const float flMagnitude );
	void			RemoveStatusEffects( const unsigned short iEffects, const float flMagnitude );
	unsigned short	GetStatusEffects( ) { return m_iStatusEffects; };
	void			SetEffectFromBitmask( const int iMask, float flMagnitude );
	float&			GetEffectFromBitmaskToModify( const int iMask );
	float			GetEffectFromBitmask( const int iMask );

	void	AddArmamentModifiers();

	float	GetHealth();
	float	GetFocus();
	float	GetStamina();
	float	GetHealthRegen();
	float	GetFocusRegen();
	float	GetStaminaRegen();
	float	GetAttack();
	float	GetEnergy();
	float	GetDefense();
	float	GetResistance();
	float	GetSpeed();
	float	GetCritical();

	float	GetAttackScale();
	float	GetEnergyScale();
	float	GetDefenseScale();
	float	GetResistanceScale();
	float	GetSpeedScale();
	float	GetSpeedInvScale();
	float	GetCriticalRate();

	float	GetDrainHealthRate(int iWeapon, int iRune);
	float	GetDrainFocusRate(int iWeapon, int iRune);
	float	GetDrainStaminaRate(int iWeapon, int iRune);

	int			GetPhysicalAttackDamage(int iWeapon, int iWeaponDamage);
	element_t	GetPhysicalAttackElement(int iWeapon);
	statuseffect_t	GetPhysicalAttackStatusEffect(int iWeapon);
	float		GetPhysicalAttackStatusEffectMagnitude(int iWeapon);

	float		GetMagicalAttackCost(CRunePosition* pRune);
	float		GetMagicalAttackDamage(CRunePosition* pRune);
	element_t	GetMagicalAttackElement(CRunePosition* pRune);
	statuseffect_t	GetMagicalAttackStatusEffect(CRunePosition* pRune);
	float		GetMagicalAttackCastTime(CRunePosition* pRune);
	float		GetMagicalAttackReload(CRunePosition* pRune);

	float	GetElementDefenseScale( int iDamageElemental );

	float	GetPowerJumpScale();

	CRunePosition	GetVengeance();
	float			GetVengeanceRate();
	float			GetVengeanceDamage();
	element_t		GetVengeanceElement();
	statuseffect_t	GetVengeanceStatusEffect();
	float			GetVengeanceStatusMagnitude();

	float	GetBulletBlockRate();

	void	GoOverdrive();
	bool	IsInOverdrive();
	bool	IsOverdriveTimeExpired();
	void	ResetOverdrive();
	void	LowStatIndicators();

	CNetworkVarForDerived( float, m_flFocus );
	CNetworkVarForDerived( short, m_iMaxFocus );

	CNetworkVarForDerived( float, m_flStamina );
	CNetworkVarForDerived( short, m_iMaxStamina );

	// Do not refer to these directly, use the wrapper functions so that you get status effect modifications!
	CNetworkVarForDerived( float, m_flHealthRegen );
	CNetworkVarForDerived( float, m_flFocusRegen );
	CNetworkVarForDerived( float, m_flStaminaRegen );
	CNetworkVarForDerived( float, m_flAttack );
	CNetworkVarForDerived( float, m_flEnergy );
	CNetworkVarForDerived( float, m_flDefense );
	CNetworkVarForDerived( float, m_flResistance );
	CNetworkVarForDerived( float, m_flSpeedStat );
	CNetworkVarForDerived( float, m_flCritical );

	CNetworkVarForDerived( unsigned short, m_iStatusEffects );

	CNetworkVarForDerived( float, m_flDOT );
	CNetworkVarForDerived( float, m_flSlowness );
	CNetworkVarForDerived( float, m_flWeakness );
	CNetworkVarForDerived( float, m_flDisorientation );
	CNetworkVarForDerived( float, m_flBlindness );
	CNetworkVarForDerived( float, m_flAtrophy );
	CNetworkVarForDerived( float, m_flSilence );
	CNetworkVarForDerived( float, m_flRegeneration );
	CNetworkVarForDerived( float, m_flPoison );
	CNetworkVarForDerived( float, m_flHaste );
	CNetworkVarForDerived( float, m_flShield );
	CNetworkVarForDerived( float, m_flBarrier );
	CNetworkVarForDerived( float, m_flReflect );
	CNetworkVarForDerived( float, m_flStealth );
	CNetworkVarForDerived( float, m_flOverdrive );

	float			m_flGoOverdriveTime;

	CCFPlayer*		m_pPlayer;
};

float Perc( float val );

#endif // STATISTICS_H