#include "cbase.h"
#include "statistics.h"
#include "armament.h"
#include "runes.h"
#include "cf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar cf_extremeoverdrive("cf_extremeoverdrive", "0", FCVAR_CHEAT, "Overdrive on every hit.");
static ConVar cf_overdrivedebug("cf_overdrivedebug", "0", FCVAR_CHEAT, "Print overdrive debug messages.");

static ConVar mp_slownessstrength("mp_slownessstrength", "60.0", FCVAR_CHEAT|FCVAR_REPLICATED, "Magnitude that slowness reduces the speed stat. (points)", true, 0, false, 0);
static ConVar mp_weaknessstrength("mp_weaknessstrength", "30.0", FCVAR_CHEAT|FCVAR_REPLICATED, "Magnitude that weakness reduces vital stats. (points)", true, 0, false, 0);
static ConVar mp_atrophystrength("mp_atrophystrength", "60.0", FCVAR_CHEAT|FCVAR_REPLICATED, "Magnitude that atrophy reduces attack and defense stats. (points)", true, 0, false, 0);
static ConVar mp_silencestrength("mp_silencestrength", "60.0", FCVAR_CHEAT|FCVAR_REPLICATED, "Magnitude that silence reduces energy and resistance stats. (points)", true, 0, false, 0);
static ConVar mp_poisonrate("mp_poisonrate", "9.0", FCVAR_CHEAT|FCVAR_REPLICATED, "Rate that poison reduces health. (points per second)", true, 0, false, 0);
static ConVar mp_regenrate("mp_regenrate", "9.0", FCVAR_CHEAT|FCVAR_REPLICATED, "Rate that regeneration increases health. (points per second)", true, 0, false, 0);
static ConVar mp_dotrate("mp_dotrate", "40.0", FCVAR_CHEAT|FCVAR_REPLICATED, "Rate that damage over time decreases health. (points per second)", true, 0, false, 0);
static ConVar mp_hastestrength("mp_hastestrength", "30.0", FCVAR_CHEAT|FCVAR_REPLICATED, "Magnitude that haste increases the speed stat. (points)", true, 0, false, 0);
static ConVar mp_stealthtime("mp_stealthtime", "60.0", FCVAR_CHEAT|FCVAR_REPLICATED, "Time that stealth remains activated. (seconds)", true, 0, false, 0);
static ConVar mp_overdrivetime("mp_overdrivetime", "10", FCVAR_CHEAT|FCVAR_REPLICATED, "How long does the player OD.");
static ConVar mp_overdriveinvultime("mp_overdriveinvultime", "1", FCVAR_CHEAT|FCVAR_REPLICATED, "How long is the player invulnerable after getting OD.");
static ConVar mp_overdriveminhp("mp_overdriveminhp", "600", FCVAR_CHEAT, "How long is the player invulnerable after getting OD.");
static ConVar mp_targetlossmagnitude("mp_targetlossmagnitude", "0.4", FCVAR_CHEAT|FCVAR_REPLICATED, "At what magnitude of the proper effect is the victim caused to lose his target lock?");
static ConVar mp_aiminattackbonus("mp_aiminattackbonus", "20.0", FCVAR_CHEAT|FCVAR_REPLICATED, "Magnitude that aiming in with a firearm increases attack. (points)", true, 0, false, 0);

BEGIN_SIMPLE_DATADESC( CStatistics )
	DEFINE_FIELD( m_flFocus, FIELD_FLOAT),
	DEFINE_FIELD( m_iMaxFocus, FIELD_INTEGER),
	DEFINE_FIELD( m_flStamina, FIELD_FLOAT),
	DEFINE_FIELD( m_iMaxStamina, FIELD_INTEGER),
	DEFINE_FIELD( m_flHealthRegen, FIELD_FLOAT),
	DEFINE_FIELD( m_flFocusRegen, FIELD_FLOAT),
	DEFINE_FIELD( m_flStaminaRegen, FIELD_FLOAT),
	DEFINE_FIELD( m_flAttack, FIELD_FLOAT),
	DEFINE_FIELD( m_flEnergy, FIELD_FLOAT),
	DEFINE_FIELD( m_flDefense, FIELD_FLOAT),
	DEFINE_FIELD( m_flResistance, FIELD_FLOAT),
	DEFINE_FIELD( m_flSpeedStat, FIELD_FLOAT),
	DEFINE_FIELD( m_flCritical, FIELD_FLOAT),

	DEFINE_FIELD( m_iStatusEffects, FIELD_INTEGER),

	DEFINE_FIELD( m_flDOT, FIELD_FLOAT),
	DEFINE_FIELD( m_flSlowness, FIELD_FLOAT),
	DEFINE_FIELD( m_flWeakness, FIELD_FLOAT),
	DEFINE_FIELD( m_flDisorientation, FIELD_FLOAT),
	DEFINE_FIELD( m_flBlindness, FIELD_FLOAT),
	DEFINE_FIELD( m_flAtrophy, FIELD_FLOAT),
	DEFINE_FIELD( m_flSilence, FIELD_FLOAT),
	DEFINE_FIELD( m_flRegeneration, FIELD_FLOAT),
	DEFINE_FIELD( m_flPoison, FIELD_FLOAT),
	DEFINE_FIELD( m_flHaste, FIELD_FLOAT),
	DEFINE_FIELD( m_flShield, FIELD_FLOAT),
	DEFINE_FIELD( m_flBarrier, FIELD_FLOAT),
	DEFINE_FIELD( m_flReflect, FIELD_FLOAT),
	DEFINE_FIELD( m_flStealth, FIELD_FLOAT),
	DEFINE_FIELD( m_flOverdrive, FIELD_FLOAT),
END_DATADESC()

CStatistics::CStatistics(CCFPlayer* pPlayer)
{
	m_flNextStatsThink = 0;
	m_flLastStatsThink = 0;
	InitStats();
	m_pPlayer = pPlayer;
}

void CStatistics::InitStats()
{
	m_flFocus = 100;
	m_iMaxFocus = 100;
	m_flStamina = 100;
	m_iMaxStamina = 100;
	m_flHealthRegen = 10;
	m_flFocusRegen = 2.5f;
	m_flStaminaRegen = 10.0f;
	m_flAttack = 0;
	m_flEnergy = 0;
	m_flDefense = 0;
	m_flResistance = 0;
	m_flSpeedStat = 0;
	m_flCritical = 5;

	m_iStatusEffects = 0;

	m_flDOT = 0.0f;
	m_flSlowness = 0.0f;
	m_flWeakness = 0.0f;
	m_flDisorientation = 0.0f;
	m_flBlindness = 0.0f;
	m_flAtrophy = 0.0f;
	m_flSilence = 0.0f;
	m_flRegeneration = 0.0f;
	m_flPoison = 0.0f;
	m_flHaste = 0.0f;
	m_flShield = 0.0f;
	m_flBarrier = 0.0f;
	m_flReflect = 0.0f;
	m_flStealth = 0.0f;
	m_flOverdrive = 0.0f;

	m_flGoOverdriveTime = 0;
}

void CStatistics::AddStatusEffects( const unsigned short iEffects, const float flMagnitude )
{
	m_iStatusEffects |= iEffects;

	for (int i = 0; i < TOTAL_STATUSEFFECTS; i++)
	{
		int iMask = (1<<i);
		if (iEffects & iMask)
		{
			float flOriginalMagnitude = GetEffectFromBitmask(iMask);

			if (iMask == STATUSEFFECT_STEALTH)
				SetEffectFromBitmask(iMask, gpGlobals->curtime + mp_stealthtime.GetFloat());
			else
				SetEffectFromBitmask(iMask, flMagnitude);

			if (GetEffectFromBitmask(iMask) != flOriginalMagnitude && (iMask & (STATUSEFFECT_SLOWNESS|STATUSEFFECT_HASTE)))
				m_pPlayer->CalculateMovementSpeed();

			if (!flOriginalMagnitude && GetEffectFromBitmask(iMask))
				m_pPlayer->EmitSound(VarArgs("Player.Status_%s", StatusEffectToString((statuseffect_t)iMask)));
		}
	}
}

float& CStatistics::GetEffectFromBitmaskToModify( const int iMask )
{
	static float flNone = 0;
	switch (iMask)
	{
	case STATUSEFFECT_DOT: return m_flDOT.GetForModify();
	case STATUSEFFECT_SLOWNESS: return m_flSlowness.GetForModify();
	case STATUSEFFECT_WEAKNESS: return m_flWeakness.GetForModify();
	case STATUSEFFECT_DISORIENT: return m_flDisorientation.GetForModify();
	case STATUSEFFECT_BLINDNESS: return m_flBlindness.GetForModify();
	case STATUSEFFECT_UNUSED1: return flNone;
	case STATUSEFFECT_UNUSED2: return flNone;
	case STATUSEFFECT_ATROPHY: return m_flAtrophy.GetForModify();
	case STATUSEFFECT_SILENCE: return m_flSilence.GetForModify();
	case STATUSEFFECT_REGEN: return m_flRegeneration.GetForModify();
	case STATUSEFFECT_POISON: return m_flPoison.GetForModify();
	case STATUSEFFECT_HASTE: return m_flHaste.GetForModify();
	case STATUSEFFECT_SHIELD: return m_flShield.GetForModify();
	case STATUSEFFECT_BARRIER: return m_flBarrier.GetForModify();
	case STATUSEFFECT_REFLECT: return m_flReflect.GetForModify();
	case STATUSEFFECT_STEALTH: return m_flStealth.GetForModify();
	default: Assert(0); return flNone;
	}
}

float CStatistics::GetEffectFromBitmask( const int iMask )
{
	switch (iMask)
	{
	case STATUSEFFECT_DOT: return m_flDOT;
	case STATUSEFFECT_SLOWNESS: return m_flSlowness;
	case STATUSEFFECT_WEAKNESS: return m_flWeakness;
	case STATUSEFFECT_DISORIENT: return m_flDisorientation;
	case STATUSEFFECT_BLINDNESS: return m_flBlindness;
	case STATUSEFFECT_UNUSED1: return 0;
	case STATUSEFFECT_UNUSED2: return 0;
	case STATUSEFFECT_ATROPHY: return m_flAtrophy;
	case STATUSEFFECT_SILENCE: return m_flSilence;
	case STATUSEFFECT_REGEN: return m_flRegeneration;
	case STATUSEFFECT_POISON: return m_flPoison;
	case STATUSEFFECT_HASTE: return m_flHaste;
	case STATUSEFFECT_SHIELD: return m_flShield;
	case STATUSEFFECT_BARRIER: return m_flBarrier;
	case STATUSEFFECT_REFLECT: return m_flReflect;
	case STATUSEFFECT_STEALTH: return m_flStealth;
	default: Assert(0); return 0;
	}
}

void CStatistics::SetEffectFromBitmask( const int iMask, float flMagnitude )
{
	GetEffectFromBitmaskToModify(iMask) += flMagnitude;
	if (GetEffectFromBitmask(iMask) < 0)
		GetEffectFromBitmaskToModify(iMask) = 0;

#ifdef GAME_DLL
	// Change this to blindness in the future.
	if (iMask == TARGETLOSS_STATUSEFFECT && GetEffectFromBitmask(iMask) > mp_targetlossmagnitude.GetFloat())
		m_pPlayer->SetDirectTarget(NULL);
#endif
}

void CStatistics::RemoveStatusEffects( const unsigned short iEffects, const float flMagnitude )
{
	for (int i = 0; i < TOTAL_STATUSEFFECTS; i++)
	{
		int iMask = (1<<i);
		if (iEffects & iMask)
		{
			float flOriginalMagnitude = GetEffectFromBitmask(iMask);

			SetEffectFromBitmask(iMask, -flMagnitude);

			if (GetEffectFromBitmask(iMask) <= 0)
				m_iStatusEffects &= ~iMask;

			if (GetEffectFromBitmask(iMask) != flOriginalMagnitude && (iMask & (STATUSEFFECT_SLOWNESS|STATUSEFFECT_HASTE)))
				m_pPlayer->CalculateMovementSpeed();

			if (flOriginalMagnitude && !GetEffectFromBitmask(iMask))
				m_pPlayer->StopSound(VarArgs("Player.Status_%s", StatusEffectToString((statuseffect_t)iMask)));
		}
	}
}

void CStatistics::StatisticsThink()
{
	if (gpGlobals->curtime < m_flNextStatsThink)
		return;

	m_flNextStatsThink += STATISTICS_FRAMETIME;

#ifdef CLIENT_DLL
	LowStatIndicators();
#endif

#ifdef GAME_DLL
		float flRegen = GetHealthRegen() * STATISTICS_FRAMETIME;

	if (m_flDOT)
	{
		CTakeDamageInfo dmg;
		CCFPlayer* hLastDOTAttacker = m_pPlayer->GetLastDOTAttacker();
		if (hLastDOTAttacker != NULL)
			dmg.CFSet( hLastDOTAttacker, hLastDOTAttacker, m_flDOT * mp_dotrate.GetFloat() * STATISTICS_FRAMETIME, DMG_GENERIC, WEAPON_NONE, false, ELEMENT_FIRE );
		else 
			dmg.CFSet( m_pPlayer, m_pPlayer, m_flDOT * mp_dotrate.GetFloat() * STATISTICS_FRAMETIME, DMG_GENERIC, WEAPON_NONE, false, ELEMENT_FIRE );
		m_pPlayer->TakeDamage(dmg);
	}

	if (flRegen < 0)
	{
		CTakeDamageInfo dmg;
		dmg.CFSet( m_pPlayer, m_pPlayer, flRegen, DMG_GENERIC, WEAPON_NONE, false );
		m_pPlayer->TakeDamage(dmg);
	}

	if (!m_flDOT && flRegen > 0)
		m_pPlayer->CFTakeHealth(flRegen, 0, false);

	if (m_flFocus < m_iMaxFocus)
		m_flFocus = Approach( m_iMaxFocus, m_flFocus, GetFocusRegen() * STATISTICS_FRAMETIME );
	if (m_flStamina < m_iMaxStamina)
		m_flStamina = Approach( m_iMaxStamina, m_flStamina, GetStaminaRegen() * STATISTICS_FRAMETIME );

	float flDegrade = STATISTICS_DEGRADE_RATE * STATISTICS_FRAMETIME;

	for (int i = 0; i < TOTAL_STATUSEFFECTS; i++)
		RemoveStatusEffects( (1<<i), flDegrade );

	m_flLastStatsThink = gpGlobals->curtime;
#endif
}

void CStatistics::AddArmamentModifiers()
{
#ifdef GAME_DLL
	// Bump the player's statistics as a result of buying this armament
	CArmament *pArm = m_pPlayer->GetActiveArmament();

	m_pPlayer->m_iMaxHealth.GetForModify() = pArm->GetMaxHealth();
	m_iMaxFocus.GetForModify() = pArm->GetMaxFocus();
	m_iMaxStamina.GetForModify() = pArm->GetMaxStamina();
#endif
}

int CStatistics::TakeDamage(CTakeDamageInfo &info)
{
#ifdef GAME_DLL
	if (!IsInOverdrive() && m_pPlayer->IsAlive() &&
		CFGameRules()->CanGoOverdrive(m_pPlayer) &&
		info.GetAttacker()->IsPlayer() &&
		CFGameRules()->CanGiveOverdrive(ToCFPlayer(info.GetAttacker())))
	{
		float flScore = m_pPlayer->GetEloScore();

		float flHealthForOD = (flScore - CFGameRules()->GetAverageElo() + 300)*5 + 200;
		float flODBonus;
		if (flHealthForOD < mp_overdriveminhp.GetFloat())
			flODBonus = info.GetDamage() / mp_overdriveminhp.GetFloat();	// If score's too low, use 1/2 health so player's OD doesn't trip in one shot.
		else
			flODBonus = info.GetDamage() / flHealthForOD;

		int iPlayers = 0;
		for (int i = 0; i < GetNumberOfTeams(); i++)
		{
			CTeam* pTeam = GetGlobalTeam(i);
			iPlayers += pTeam->GetNumPlayers();
		}

		float flMultiplier = RemapValClamped(iPlayers, 2, 8, 0.3, 1);

		if (cf_overdrivedebug.GetBool() && iPlayers <= 6)
			DevMsg("Overdrive Players: %d OD Multiplier: %f\n", iPlayers, flMultiplier);

		flODBonus *= flMultiplier;

		m_flOverdrive += flODBonus;

		if (cf_overdrivedebug.GetBool())
			DevMsg("Overdrive: %s +%f = %f\n", m_pPlayer->GetPlayerName(), flODBonus, m_flOverdrive.Get());

		if (cf_extremeoverdrive.GetBool())
			m_flOverdrive += 1;

		// Don't go overdrive if the player would be killed anyways by this attack.
		// This way, devastating blows (like a fully charged critical) don't trigger OD.
		if (info.GetDamage() < m_pPlayer->GetHealth())
		{
			if (IsInOverdrive())
			{
				GoOverdrive();
				return 0;
			}
		}
		else
		{
			if (IsInOverdrive() && cf_overdrivedebug.GetBool())
				DevMsg("Overdrive: %s death blocked OD\n", m_pPlayer->GetPlayerName());
		}
	}

	if (m_flGoOverdriveTime != 0 && gpGlobals->curtime < m_flGoOverdriveTime + mp_overdriveinvultime.GetFloat())
		return 0;
#endif

	if (m_pPlayer->IsAlive() && !IsInOverdrive())
	{
#ifdef GAME_DLL
		if (ToCFPlayer(info.GetAttacker()))
			ToCFPlayer(info.GetAttacker())->AwardEloPoints(SCORE_INFLICT_STATUS, m_pPlayer);
#endif
		AddStatusEffects(info.GetStatusEffects(), info.GetStatusEffectMagnitude());
	}

	return info.GetDamage();
}

void CStatistics::Event_Killed( const CTakeDamageInfo &info )
{
	// Stop sounds, kill particle effects
	RemoveStatusEffects((unsigned short)~STATUSEFFECT_NONE, 100);
}

void CStatistics::Event_Knockout( const CTakeDamageInfo &info )
{
	ResetOverdrive();

	// Stop sounds, kill particle effects
	RemoveStatusEffects((unsigned short)~STATUSEFFECT_NONE, 100);
}

void CStatistics::GoOverdrive()
{
#ifdef GAME_DLL
	if (!m_pPlayer->IsAlive())
		return;

	if (m_pPlayer->HasObjective())
	{
		ResetOverdrive();
		return;
	}

	if (cf_overdrivedebug.GetBool())
		DevMsg("Overdrive: %s triggered OD\n");

	m_pPlayer->EmitSound("Player.Overdrive");

	m_pPlayer->m_bOverdrive = true;

	// Unfreeze player.
	m_pPlayer->FreezePlayer(1);

	m_flGoOverdriveTime = gpGlobals->curtime;

	// Heal the player completely.
	m_pPlayer->TakeHealth(m_pPlayer->GetMaxHealth() - m_pPlayer->GetHealth(), 0);
	m_flFocus = m_iMaxFocus;
	m_flStamina = m_iMaxStamina;

	// Remove negative status effects.
	m_flDOT = 0;
	m_flSlowness = 0;
	m_flWeakness = 0;
	m_flDisorientation = 0;
	m_flBlindness = 0;
	m_flAtrophy = 0;
	m_flSilence = 0;
	m_flPoison = 0;
#endif
}

bool CStatistics::IsOverdriveTimeExpired()
{
	return m_flGoOverdriveTime == 0 || gpGlobals->curtime > m_flGoOverdriveTime + mp_overdrivetime.GetFloat();
}

void CStatistics::ResetOverdrive()
{
#ifdef GAME_DLL
	if (m_pPlayer->m_bOverdrive)
		m_flOverdrive = -1;	// He just had one? Make it twice as long before he can get the next one.
	else
		m_flOverdrive = 0;

	m_flGoOverdriveTime = 0;

	m_pPlayer->m_bOverdrive = false;
#endif
}

float CStatistics::GetAttack()
{
	float flAttack = m_flAttack;
	CArmament *pArm = m_pPlayer->GetActiveArmament();

	for (int i = 0; i < 5; i++)
	{
		if (pArm->GetWeaponData(i))
			flAttack += pArm->GetWeaponData(i)->m_flAttack;
		else if (pArm->GetArmamentData(i))
			flAttack += pArm->GetArmamentData(i)->m_iAttack;

		if (pArm->GetWeaponData(i) ||
			pArm->GetArmamentData(i))
		{
			for (int j = 0; j < MAX_RUNES; j++)
			{
				for (int k = 0; k < MAX_RUNES; k++)
				{
					CRuneData *pRune = pArm->m_aWeapons[i].m_apRuneData[j][k];
					if (!pRune)
						continue;
						
					if (pRune->m_eEffect == RE_SPELL || pRune->m_eEffect == RE_RESTORE)
						flAttack += pRune->m_iAttack;
				}
			}
		}
	}

	flAttack -= m_flWeakness * mp_weaknessstrength.GetFloat();
	flAttack -= m_flAtrophy * mp_atrophystrength.GetFloat();

	if (m_pPlayer->IsAimingIn())
		flAttack += mp_aiminattackbonus.GetFloat();

	if (flAttack < STAT_ATT_MIN)
		flAttack = STAT_ATT_MIN;

	if (flAttack > STAT_ATT_MAX)
		flAttack = STAT_ATT_MAX;

	return flAttack;
}

float CStatistics::GetAttackScale()
{
	return Perc(GetAttack()) + 1;
}

float CStatistics::GetEnergy()
{
	float flEnergy = m_flEnergy;
	CArmament *pArm = m_pPlayer->GetActiveArmament();

	for (int i = 0; i < 5; i++)
	{
		if (pArm->GetWeaponData(i))
			flEnergy += pArm->GetWeaponData(i)->m_flEnergy;
		else if (pArm->GetArmamentData(i))
			flEnergy += pArm->GetArmamentData(i)->m_iEnergy;

		if (pArm->GetWeaponData(i) ||
			pArm->GetArmamentData(i))
		{
			for (int j = 0; j < MAX_RUNES; j++)
			{
				for (int k = 0; k < MAX_RUNES; k++)
				{
					CRuneData *pRune = pArm->m_aWeapons[i].m_apRuneData[j][k];
					if (!pRune)
						continue;

					if (pRune->m_eEffect == RE_SPELL || pRune->m_eEffect == RE_RESTORE)
						flEnergy += pRune->m_iEnergy;
				}
			}
		}
	}

	flEnergy -= m_flWeakness * mp_weaknessstrength.GetFloat();
	flEnergy -= m_flSilence * mp_silencestrength.GetFloat();

	if (flEnergy < STAT_NRG_MIN)
		flEnergy = STAT_NRG_MIN;

	if (flEnergy > STAT_NRG_MAX)
		flEnergy = STAT_NRG_MAX;
	
	return flEnergy;
}

float CStatistics::GetEnergyScale()
{
	return Perc(GetEnergy()) + 1;
}

float CStatistics::GetDefense()
{
	float flDefense = m_flDefense;
	CArmament *pArm = m_pPlayer->GetActiveArmament();

	for (int i = 0; i < 5; i++)
	{
		CArmamentData* pArmamentData = pArm->GetArmamentData(i);
		if (pArmamentData)
			flDefense += pArmamentData->m_iDefense;

		if (pArm->GetWeaponData(i) ||
			pArm->GetArmamentData(i))
		{
			for (int j = 0; j < MAX_RUNES; j++)
			{
				for (int k = 0; k < MAX_RUNES; k++)
				{
					CRuneData *pRune = pArm->m_aWeapons[i].m_apRuneData[j][k];
					if (!pRune)
						continue;

					if (pRune->m_eEffect == RE_SPELL || pRune->m_eEffect == RE_RESTORE)
						flDefense += pRune->m_iDefense;
				}
			}
		}
	}

	flDefense -= m_flWeakness * mp_weaknessstrength.GetFloat();
	flDefense -= m_flAtrophy * mp_atrophystrength.GetFloat();

	if (IsInOverdrive())
		flDefense += 25;

	if (m_pPlayer->IsFuse())
		flDefense += 20;

	if (flDefense < STAT_DEF_MIN)
		flDefense = STAT_DEF_MIN;

	if (flDefense > STAT_DEF_MAX)
		flDefense = STAT_DEF_MAX;
	
	return flDefense;
}

float CStatistics::GetDefenseScale()
{
	return 1 - Perc(GetDefense());
}

float CStatistics::GetResistance()
{
	float flResistance = m_flResistance;
	CArmament *pArm = m_pPlayer->GetActiveArmament();

	for (int i = 0; i < 5; i++)
	{
		CArmamentData* pArmamentData = pArm->GetArmamentData(i);
		if (pArmamentData)
			flResistance += pArmamentData->m_iResistance;

		if (pArm->GetWeaponData(i) ||
			pArm->GetArmamentData(i))
		{
			for (int j = 0; j < MAX_RUNES; j++)
			{
				for (int k = 0; k < MAX_RUNES; k++)
				{
					CRuneData *pRune = pArm->m_aWeapons[i].m_apRuneData[j][k];
					if (!pRune)
						continue;

					if (pRune->m_eEffect == RE_SPELL || pRune->m_eEffect == RE_RESTORE)
						flResistance += pRune->m_iResistance;
				}
			}
		}
	}

	flResistance -= m_flWeakness * mp_weaknessstrength.GetFloat();
	flResistance -= m_flSilence * mp_silencestrength.GetFloat();

	if (IsInOverdrive())
		flResistance += 25;

	if (m_pPlayer->IsFuse())
		flResistance += 20;

	if (flResistance < STAT_RES_MIN)
		flResistance = STAT_RES_MIN;

	if (flResistance > STAT_RES_MAX)
		flResistance = STAT_RES_MAX;

	return flResistance;
}

float CStatistics::GetResistanceScale()
{
	return 1 - Perc(GetResistance());
}

float CStatistics::GetCritical()
{
	float flCritical = m_flCritical;
	CArmament *pArm = m_pPlayer->GetActiveArmament();

	for (int i = 0; i < 5; i++)
	{
		if (pArm->GetWeaponData(i))
			flCritical += pArm->GetWeaponData(i)->m_flCritical;
		else if (pArm->GetArmamentData(i))
			flCritical += pArm->GetArmamentData(i)->m_iCritical;

		if (pArm->GetWeaponData(i) ||
			pArm->GetArmamentData(i))
		{
			for (int j = 0; j < MAX_RUNES; j++)
			{
				for (int k = 0; k < MAX_RUNES; k++)
				{
					CRuneData *pRune = pArm->m_aWeapons[i].m_apRuneData[j][k];
					if (!pRune)
						continue;

					if (pRune->m_eEffect == RE_SPELL || pRune->m_eEffect == RE_RESTORE)
						flCritical += pRune->m_iCritical;
				}
			}
		}
	}

	if (IsInOverdrive())
		flCritical += 25;

	if (flCritical < STAT_CRT_MIN)
		flCritical = STAT_CRT_MIN;

	if (flCritical > STAT_CRT_MAX)
		flCritical = STAT_CRT_MAX;

	return flCritical;
}

float CStatistics::GetCriticalRate()
{
	return Perc(GetCritical());
}

// This isn't the REAL health, don't use it for anything important! It's only there to look good.
float CStatistics::GetHealth()
{
	float flTimeSinceLast = max(gpGlobals->curtime - m_flLastStatsThink, 0);

	if (m_pPlayer->GetHealth() > m_pPlayer->m_iMaxHealth)
		return m_pPlayer->GetHealth();

	return min(m_pPlayer->m_iMaxHealth, m_pPlayer->GetHealth() + flTimeSinceLast * GetHealthRegen());
}

float CStatistics::GetFocus()
{
	float flTimeSinceLast = max(gpGlobals->curtime - m_flLastStatsThink, 0);

	return min(m_iMaxFocus, m_flFocus + flTimeSinceLast * GetFocusRegen());
}

float CStatistics::GetStamina()
{
	float flTimeSinceLast = max(gpGlobals->curtime - m_flLastStatsThink, 0);

	return min(m_iMaxStamina, m_flStamina + flTimeSinceLast * GetStaminaRegen());
}

float CStatistics::GetHealthRegen()
{
	float flHealthRegen = m_flHealthRegen;
	CArmament *pArm = m_pPlayer->GetActiveArmament();
	int i;

	for (i = 0; i < 5; i++)
	{
		if (pArm->GetArmamentData(i))
			flHealthRegen += pArm->GetArmamentData(i)->m_iHealthRegen;
	}

	for (i = 0; i < 5; i++)
	{
		if (pArm->GetWeaponData(i) ||
			pArm->GetArmamentData(i))
		{
			for (int j = 0; j < MAX_RUNES; j++)
			{
				for (int k = 0; k < MAX_RUNES; k++)
				{
					CRuneData *pRune = pArm->m_aWeapons[i].m_apRuneData[j][k];
					if (!pRune)
						continue;
	
					if (pRune->m_eEffect == RE_SPELL || pRune->m_eEffect == RE_RESTORE)
						flHealthRegen *= Perc(pRune->m_iHealthRegen) + 1;
				}
			}
		}
	}

	flHealthRegen += m_flRegeneration * mp_regenrate.GetFloat();
	flHealthRegen -= m_flPoison * mp_poisonrate.GetFloat();

	if (IsInOverdrive())
		flHealthRegen += 50;

	if (m_pPlayer->IsFuse())
		flHealthRegen += 100;

	if (flHealthRegen < STAT_HLR_MIN)
		flHealthRegen = STAT_HLR_MIN;

	if (flHealthRegen > STAT_HLR_MAX*10)
		flHealthRegen = STAT_HLR_MAX*10;

	return flHealthRegen;
}

float CStatistics::GetFocusRegen()
{
	float flFocusRegen = m_flFocusRegen;
	CArmament *pArm = m_pPlayer->GetActiveArmament();
	int i;

	for (i = 0; i < 5; i++)
	{
		if (pArm->GetWeaponData(i))
			flFocusRegen += pArm->GetWeaponData(i)->m_flFocusRegen;
		else if (pArm->GetArmamentData(i))
			flFocusRegen += pArm->GetArmamentData(i)->m_iFocusRegen;
	}

	for (i = 0; i < 5; i++)
	{
		if (pArm->GetWeaponData(i) ||
			pArm->GetArmamentData(i))
		{
			for (int j = 0; j < MAX_RUNES; j++)
			{
				for (int k = 0; k < MAX_RUNES; k++)
				{
					CRuneData *pRune = pArm->m_aWeapons[i].m_apRuneData[j][k];
					if (!pRune)
						continue;

					if (pRune->m_eEffect == RE_SPELL || pRune->m_eEffect == RE_RESTORE)
						flFocusRegen *= Perc(pRune->m_iFocusRegen) + 1;
				}
			}
		}
	}

	if (IsInOverdrive())
		flFocusRegen += 5;

	if (m_pPlayer->IsFuse())
		flFocusRegen += 10;

	if (flFocusRegen < STAT_FOR_MIN)
		flFocusRegen = STAT_FOR_MIN;

	if (flFocusRegen > STAT_FOR_MAX)
		flFocusRegen = STAT_FOR_MAX;

	return flFocusRegen;
}

float CStatistics::GetStaminaRegen()
{
	float flStaminaRegen = m_flStaminaRegen;
	CArmament *pArm = m_pPlayer->GetActiveArmament();
	int i;

	for (i = 0; i < 5; i++)
	{
		if (pArm->GetWeaponData(i))
			flStaminaRegen += pArm->GetWeaponData(i)->m_flStaminaRegen;
		else if (pArm->GetArmamentData(i))
			flStaminaRegen += pArm->GetArmamentData(i)->m_iStaminaRegen;
	}

	for (i = 0; i < 5; i++)
	{
		if (pArm->GetWeaponData(i) ||
			pArm->GetArmamentData(i))
		{
			for (int j = 0; j < MAX_RUNES; j++)
			{
				for (int k = 0; k < MAX_RUNES; k++)
				{
					CRuneData *pRune = pArm->m_aWeapons[i].m_apRuneData[j][k];
					if (!pRune)
						continue;

					if (pRune->m_eEffect == RE_SPELL || pRune->m_eEffect == RE_RESTORE)
						flStaminaRegen *= Perc(pRune->m_iStaminaRegen) + 1;
				}
			}
		}
	}

	if (IsInOverdrive())
		flStaminaRegen += 5;

	if (m_pPlayer->IsFuse())
		flStaminaRegen += 10;

	if (flStaminaRegen < STAT_STR_MIN)
		flStaminaRegen = STAT_STR_MIN;

	if (flStaminaRegen > STAT_STR_MAX)
		flStaminaRegen = STAT_STR_MAX;

	return flStaminaRegen;
}

float CStatistics::GetSpeed()
{
	CArmament *pArm = m_pPlayer->GetActiveArmament();
	float flSpeed = m_flSpeedStat;
	int i;

	for (i = 0; i < 5; i++)
	{
		CArmamentData* pArmamentData = pArm->GetArmamentData(i);
		if (pArmamentData)
			flSpeed += pArmamentData->m_iSpeed;

		CCFWeaponInfo* pWeaponData = pArm->GetWeaponData(i);
		if (pWeaponData)
			flSpeed += pWeaponData->m_flSpeed;

		if (pArm->GetWeaponData(i) ||
			pArm->GetArmamentData(i))
		{
			for (int j = 0; j < MAX_RUNES; j++)
			{
				for (int k = 0; k < MAX_RUNES; k++)
				{
					CRuneData *pRune = pArm->m_aWeapons[i].m_apRuneData[j][k];
					if (!pRune)
						continue;

					if (pRune->m_eEffect == RE_SPELL || pRune->m_eEffect == RE_RESTORE)
						flSpeed += pRune->m_iSpeed;
				}
			}
		}
	}

	flSpeed -= m_flSlowness * mp_slownessstrength.GetFloat();
	flSpeed += m_flHaste * mp_hastestrength.GetFloat();

	if (flSpeed < STAT_SPD_MIN)
		flSpeed = STAT_SPD_MIN;

	if (flSpeed > STAT_SPD_MAX)
		flSpeed = STAT_SPD_MAX;

	return flSpeed;
}

float CStatistics::GetSpeedScale()
{
	return Perc(GetSpeed()) + 1;
}

float CStatistics::GetSpeedInvScale()
{
	return 1 / GetSpeedScale();
}

float CStatistics::GetPowerJumpScale()
{
	float flPowerjump = GetSpeedScale();

	if (flPowerjump < -1)
		flPowerjump = -1;

	return flPowerjump;
}

int CStatistics::GetPhysicalAttackDamage(int iWeapon, int iWeaponDamage)
{
	return m_pPlayer->GetActiveArmament()->GetPhysicalAttackDamage(iWeapon, iWeaponDamage);
}

element_t CStatistics::GetPhysicalAttackElement(int iWeapon)
{
	// When in overdrive, all attacks are typeless.
	if (IsInOverdrive())
		return ELEMENT_TYPELESS;

	return m_pPlayer->GetActiveArmament()->GetPhysicalAttackElement(iWeapon);
}

statuseffect_t CStatistics::GetPhysicalAttackStatusEffect(int iWeapon)
{
	return m_pPlayer->GetActiveArmament()->GetPhysicalAttackStatusEffect(iWeapon);
}

float CStatistics::GetPhysicalAttackStatusEffectMagnitude(int iWeapon)
{
	return m_pPlayer->GetActiveArmament()->GetPhysicalAttackStatusEffectMagnitude(iWeapon);
}

float CStatistics::GetMagicalAttackCost(CRunePosition* pRune)
{
	return m_pPlayer->GetActiveArmament()->GetMagicalAttackCost(pRune);
}

float CStatistics::GetMagicalAttackDamage(CRunePosition* pRune)
{
	return m_pPlayer->GetActiveArmament()->GetMagicalAttackDamage(pRune);
}

element_t CStatistics::GetMagicalAttackElement(CRunePosition* pRune)
{
	return m_pPlayer->GetActiveArmament()->GetMagicalAttackElement(pRune);
}

statuseffect_t CStatistics::GetMagicalAttackStatusEffect(CRunePosition* pRune)
{
	return m_pPlayer->GetActiveArmament()->GetMagicalAttackStatusEffect(pRune);
}

float CStatistics::GetMagicalAttackCastTime(CRunePosition* pRune)
{
	return m_pPlayer->GetActiveArmament()->GetMagicalAttackCastTime(pRune);
}

float CStatistics::GetMagicalAttackReload(CRunePosition* pRune)
{
	return m_pPlayer->GetActiveArmament()->GetMagicalAttackReload(pRune);
}

float CStatistics::GetElementDefenseScale(int iDamageElemental)
{
	return m_pPlayer->GetActiveArmament()->GetElementDefenseScale(iDamageElemental);
}

float CStatistics::GetDrainHealthRate(int iWeapon, int iRune)
{
	float iHealthDrain = 0;

	CArmament *pArm = m_pPlayer->GetActiveArmament();
	for (int i = 0; i < MAX_RUNE_MODS; i++)
	{
		CRuneData *pRune = pArm->m_aWeapons[iWeapon].m_apRuneData[iRune][i];

		if (!pRune || pRune->m_eEffect != RE_SPELL || pRune->m_eEffect != RE_RESTORE)
			continue;

		for (int j = 0; j < MAX_RUNE_MODS; j++)
		{
//			CRuneData *pMod = pArm->m_aWeapons[iWeapon].m_apRuneData[iRune][j];
//			if (pMod && pMod->m_eEffect == RE_DRAIN)
//				iHealthDrain += pArm->GetLevelData(iWeapon, iRune, j)->m_iHealthDrain;
		}
	}

	return Perc(iHealthDrain);
}

float CStatistics::GetDrainFocusRate(int iWeapon, int iRune)
{
	float iFocusDrain = 0;

	CArmament *pArm = m_pPlayer->GetActiveArmament();
	if (pArm->GetBaseLevel(iWeapon, iRune) == 0)
		return iFocusDrain;

	for (int i = 0; i < MAX_RUNE_MODS; i++)
	{
//		CRuneData *pMod = pArm->m_aWeapons[iWeapon].m_apRuneData[iRune][i];
//		if (pMod && pMod->m_eEffect == RE_DRAIN)
//			iFocusDrain += pArm->GetLevelData(iWeapon, iRune, i)->m_iFocusDrain;
	}

	return Perc(iFocusDrain);
}

float CStatistics::GetDrainStaminaRate(int iWeapon, int iRune)
{
	float iStaminaDrain = 0;

	CArmament *pArm = m_pPlayer->GetActiveArmament();
	if (pArm->GetBaseLevel(iWeapon, iRune) == 0)
		return iStaminaDrain;

	for (int i = 0; i < MAX_RUNE_MODS; i++)
	{
//		CRuneData *pMod = pArm->m_aWeapons[iWeapon].m_apRuneData[iRune][i];
//		if (pMod && pMod->m_eEffect == RE_DRAIN)
//			iStaminaDrain += pArm->GetLevelData(iWeapon, iRune, i)->m_iStaminaDrain;
	}

	return Perc(iStaminaDrain);
}

CRunePosition CStatistics::GetVengeance()
{
	// Maybe this could be cached?
	static CRunePosition BestVengeance;
	BestVengeance.m_iWeapon = -1;

	CArmament *pArm = m_pPlayer->GetActiveArmament();
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < MAX_RUNES; j++)
		{
			unsigned int iLevel = pArm->GetBaseLevel(i, j);
			if (iLevel == 0)
				continue;

			for (int k = 0; k < MAX_RUNE_MODS; k++)
			{
				CRuneData *pMod = pArm->m_aWeapons[i].m_apRuneData[j][k];
				if (pMod && pMod->m_eEffect == RE_VENGEANCE)
				{
					if (BestVengeance.m_iWeapon == -1 ||
						pArm->GetRuneAmount(pArm->GetRuneID(i, j, k), i, j) > pArm->GetRuneAmount(pArm->GetRuneID(BestVengeance.m_iWeapon, BestVengeance.m_iRune, BestVengeance.m_iMod), BestVengeance.m_iWeapon, BestVengeance.m_iRune))
					{
						BestVengeance.m_iWeapon = i;
						BestVengeance.m_iRune = j;
						BestVengeance.m_iMod = k;
					}
				}
			}
		}
	}

	return BestVengeance;
}

float CStatistics::GetVengeanceRate()
{
	CRunePosition BestVengeance = GetVengeance();
	if (BestVengeance.m_iWeapon == -1)
		return 0;
	return Perc(m_pPlayer->GetActiveArmament()->GetLevelData(BestVengeance.m_iWeapon, BestVengeance.m_iRune, BestVengeance.m_iMod)->m_iChance);
}

float CStatistics::GetVengeanceDamage()
{
	CRunePosition BestVengeance = GetVengeance();
	if (BestVengeance.m_iWeapon == -1)
	{
		AssertMsg(BestVengeance.m_iWeapon != -1, "GetVengeanceDamage() called, but Vengeance could not be found on the player.");
		return 0;
	}
	return m_pPlayer->GetActiveArmament()->GetLevelData(BestVengeance.m_iWeapon, BestVengeance.m_iRune, BestVengeance.m_iMod)->m_flDamage;
}

element_t CStatistics::GetVengeanceElement()
{
	CRunePosition BestVengeance = GetVengeance();
	if (BestVengeance.m_iWeapon == -1)
	{
		Assert("GetVengeanceElement() called, but Vengeance could not be found on the player.");
		return ELEMENT_TYPELESS;
	}
	return GetMagicalAttackElement(&BestVengeance);
}

statuseffect_t CStatistics::GetVengeanceStatusEffect()
{
	CRunePosition BestVengeance = GetVengeance();
	if (BestVengeance.m_iWeapon == -1)
	{
		Assert("GetVengeanceStatusEffect() called, but Vengeance could not be found on the player.");
		return STATUSEFFECT_NONE;
	}
	return GetMagicalAttackStatusEffect(&BestVengeance);
}

float CStatistics::GetVengeanceStatusMagnitude()
{
	CRunePosition BestVengeance = GetVengeance();
	if (BestVengeance.m_iWeapon == -1)
	{
		Assert("GetVengeanceStatusMagnitude() called, but Vengeance could not be found on the player.");
		return STATUSEFFECT_NONE;
	}
	return m_pPlayer->GetActiveArmament()->GetMagicalAttackStatusMagnitude(&BestVengeance);
}

float CStatistics::GetBulletBlockRate()
{
	float flBulletBlock = 0;
	CArmament *pArm = m_pPlayer->GetActiveArmament();

	for (int i = 0; i < 5; i++)
	{
		if (pArm->GetWeaponData(i))
			flBulletBlock += pArm->GetWeaponData(i)->m_flBulletBlock;
	}

	if (m_flStamina < 0)
		flBulletBlock /= 2;

	return flBulletBlock;
}

bool CStatistics::IsInOverdrive()
{
	return m_flOverdrive >= 1;
}

float Perc( float flVal )
{
	return flVal/100;
}

#ifdef GAME_DLL
void CC_AddStatus_f(const CCommand &args)
{
	CCFPlayer *pPlayer = ToCFPlayer( UTIL_GetCommandClient() );
	if ( !pPlayer )
		return;

	if (args.ArgC() <= 2)
		return;

	pPlayer->m_pStats->AddStatusEffects(1<<atoi( args[1] ), atof( args[2] ));
}

static ConCommand addstatus("addstatus", CC_AddStatus_f, "Add a status effect to the player.", FCVAR_CHEAT);

void CC_Show_Statistics(const CCommand &args)
{
	CCFPlayer *pPlayer;
	if (args.ArgC() == 1)
		pPlayer = ToCFPlayer( UTIL_GetCommandClient() ); 
	else
		pPlayer = ToCFPlayer( UTIL_PlayerByIndex(atoi(args[1])) );

	if ( !pPlayer )
		return;

	CCFPlayer *pPrinter = ToCFPlayer( UTIL_GetCommandClient() );;

	CStatistics* pStats = pPlayer->m_pStats;
	if ( !pStats )
		return;

	CArmament* pArm = pPlayer->GetActiveArmament();
	if ( !pArm )
		return;

	ClientPrint( pPrinter, HUD_PRINTCONSOLE, "Statistics report:\n" );

	ClientPrint( pPrinter, HUD_PRINTCONSOLE, VarArgs(" Health: %d/%d\n", pPlayer->m_iHealth, pPlayer->m_iMaxHealth) );
	ClientPrint( pPrinter, HUD_PRINTCONSOLE, VarArgs(" Atma: %d/%d\n", (int)pStats->m_flFocus, (int)pStats->m_iMaxFocus) );
	ClientPrint( pPrinter, HUD_PRINTCONSOLE, VarArgs(" Stamina: %d/%d\n", (int)pStats->m_flStamina, (int)pStats->m_iMaxStamina) );
	ClientPrint( pPrinter, HUD_PRINTCONSOLE, VarArgs(" Health Regen: %f\n", pStats->GetHealthRegen()) );
	ClientPrint( pPrinter, HUD_PRINTCONSOLE, VarArgs(" Atma Regen: %f\n", pStats->GetFocusRegen()) );
	ClientPrint( pPrinter, HUD_PRINTCONSOLE, VarArgs(" Stamina Regen: %f\n", pStats->GetStaminaRegen()) );
	ClientPrint( pPrinter, HUD_PRINTCONSOLE, VarArgs(" Attack: %f\n", pStats->GetAttack()) );
	ClientPrint( pPrinter, HUD_PRINTCONSOLE, VarArgs(" Energy: %f\n", pStats->GetEnergy()) );
	ClientPrint( pPrinter, HUD_PRINTCONSOLE, VarArgs(" Defense: %f\n", pStats->GetDefense()) );
	ClientPrint( pPrinter, HUD_PRINTCONSOLE, VarArgs(" Resistance: %f\n", pStats->GetResistance()) );
	ClientPrint( pPrinter, HUD_PRINTCONSOLE, VarArgs(" Speed: %d%%\n", (int)(pStats->GetSpeedScale()*100)) );
	ClientPrint( pPrinter, HUD_PRINTCONSOLE, VarArgs(" Critical: %d%%\n", (int)(pStats->GetCriticalRate()*100)) );
	ClientPrint( pPrinter, HUD_PRINTCONSOLE, VarArgs(" Overdrive: %d%%\n", (int)(pStats->m_flOverdrive*100)) );

	ClientPrint( pPrinter, HUD_PRINTCONSOLE, VarArgs(" Powerjump Scale: %d%%\n", (int)(pStats->GetPowerJumpScale()*100)) );

	if (pStats->GetVengeanceRate())
	{
		ClientPrint( pPrinter, HUD_PRINTCONSOLE, " Vengeance:\n" );
		ClientPrint( pPrinter, HUD_PRINTCONSOLE, VarArgs("  Rate: %d%%\n", (int)(pStats->GetVengeanceRate()*100)) );
		ClientPrint( pPrinter, HUD_PRINTCONSOLE, VarArgs("  Damage: %f\n", pStats->GetVengeanceDamage()) );
		ClientPrint( pPrinter, HUD_PRINTCONSOLE, VarArgs("  Element: %s\n", ElementToString(pStats->GetVengeanceElement())) );
	}

	for (int i = 0; i < 5; i++)
	{
		if (!pArm->GetWeaponData(i) && !pArm->GetArmamentData(i))
			continue;

		if (pArm->GetWeaponData(i))
		{
			ClientPrint( pPrinter, HUD_PRINTCONSOLE, VarArgs(" Weapon %s:\n", WeaponIDToAlias(pArm->m_aWeapons[i].m_iWeapon)) );
			ClientPrint( pPrinter, HUD_PRINTCONSOLE, VarArgs("  Attack Damage: %d%%\n", pStats->GetPhysicalAttackDamage(i, 100)) );
			ClientPrint( pPrinter, HUD_PRINTCONSOLE, VarArgs("  Attack Element: %s\n", ElementToString(pStats->GetPhysicalAttackElement(i))) );
			ClientPrint( pPrinter, HUD_PRINTCONSOLE, VarArgs("  Status Effect: %s\n", StatusEffectToString(pStats->GetPhysicalAttackStatusEffect(i))) );
		}

		if (pArm->GetArmamentData(i))
		{
			ClientPrint( pPrinter, HUD_PRINTCONSOLE, VarArgs(" %s %s:\n", (i==3)?"Armor":"Accessory", pArm->GetArmamentData(i)->m_szName) );
		}

		for (int j = 0; j < MAX_RUNES; j++)
		{
			unsigned int iLevel = pArm->GetBaseLevel(i, j);

			if (iLevel == 0)
				continue;

			ClientPrint( pPrinter, HUD_PRINTCONSOLE, VarArgs("  Spell:\n") );

			CRuneInfo RuneInfo;
			RuneInfo.m_iWeapon = i;
			RuneInfo.m_iRune = j;

			ClientPrint( pPrinter, HUD_PRINTCONSOLE, VarArgs("   Elements: %s\n", ElementToString(pStats->GetMagicalAttackElement(&RuneInfo)) ) );
			ClientPrint( pPrinter, HUD_PRINTCONSOLE, VarArgs("   Attack Damage: %f\n", pStats->GetMagicalAttackDamage(&RuneInfo)) );
			ClientPrint( pPrinter, HUD_PRINTCONSOLE, VarArgs("   Casting Cost: %f\n", pStats->GetMagicalAttackCost(&RuneInfo)) );
			ClientPrint( pPrinter, HUD_PRINTCONSOLE, VarArgs("   Casting Time: %f\n", pStats->GetMagicalAttackCastTime(&RuneInfo)) );
			ClientPrint( pPrinter, HUD_PRINTCONSOLE, VarArgs("   Cast Reload Time: %f\n", pStats->GetMagicalAttackReload(&RuneInfo)) );
			ClientPrint( pPrinter, HUD_PRINTCONSOLE, VarArgs("   Health Drain Rate: %d%%\n", (int)(pStats->GetDrainHealthRate(i, j)*100)) );
			ClientPrint( pPrinter, HUD_PRINTCONSOLE, VarArgs("   Stamina Drain Rate: %d%%\n", (int)(pStats->GetDrainStaminaRate(i, j)*100)) );
		}
	}

	ClientPrint( pPrinter, HUD_PRINTCONSOLE, "End of report.\n" );
}

static ConCommand showstats("showstats", CC_Show_Statistics, "Show a player's statistics.", FCVAR_CHEAT);
#endif


void CStatistics::LowStatIndicators()
{
#ifdef CLIENT_DLL
	if (( m_flStamina / m_iMaxStamina ) < .15 )   //if stamina is below 15%, play a low stamina warning.
	{
		m_pPlayer->EmitSound("Player.LowStamina");
	}
#endif
}