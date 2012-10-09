//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "takedamageinfo.h"
#include "ammodef.h"
#include "statistics.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern bool g_bFlag = false;

void CTakeDamageInfo::CFInit( CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, const Vector &damageForce, const Vector &damagePosition, const Vector &reportedPosition, float flDamage, int bitsDamageType, int iKillWeapon, bool bPhysical, int iElements, int iStatusEffects, float flStatusEffectMagnitude )
{
	if (!g_bFlag)
	{
		g_bFlag = true;
		Init(pInflictor, pAttacker, pWeapon, damageForce, damagePosition, reportedPosition, flDamage, bitsDamageType, iKillWeapon);
	}

	m_bPhysical = bPhysical;
	m_iElements = iElements;
	m_flStatusEffectMagnitude = flStatusEffectMagnitude;
	m_iStatusEffects = iStatusEffects;

	m_flDrainDamage = 0;
	m_flDrainFocus = 0;
	m_flDrainStamina = 0;

	g_bFlag = false;
}

void CTakeDamageInfo::CFSet( CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType, int iKillWeapon, bool bPhysical, int iElements, int iStatusEffects, float flStatusEffectMagnitude )
{
	CFInit( pInflictor, pAttacker, NULL, vec3_origin, vec3_origin, vec3_origin, flDamage, bitsDamageType, iKillWeapon, bPhysical, iElements, iStatusEffects, flStatusEffectMagnitude );
}

void CTakeDamageInfo::CFSet( CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, float flDamage, int bitsDamageType, int iKillWeapon, bool bPhysical, int iElements, int iStatusEffects, float flStatusEffectMagnitude )
{
	CFInit( pInflictor, pAttacker, pWeapon, vec3_origin, vec3_origin, vec3_origin, flDamage, bitsDamageType, iKillWeapon, bPhysical, iElements, iStatusEffects, flStatusEffectMagnitude );
}

void CTakeDamageInfo::CFSet( CBaseEntity *pInflictor, CBaseEntity *pAttacker, const Vector &damageForce, const Vector &damagePosition, float flDamage, int bitsDamageType, int iKillWeapon, Vector *reportedPosition, bool bPhysical, int iElements, int iStatusEffects, float flStatusEffectMagnitude )
{
	CFSet( pInflictor, pAttacker, NULL, damageForce, damagePosition, flDamage, bitsDamageType, iKillWeapon, reportedPosition, bPhysical, iElements, iStatusEffects, flStatusEffectMagnitude );
}

void CTakeDamageInfo::CFSet( CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, const Vector &damageForce, const Vector &damagePosition, float flDamage, int bitsDamageType, int iKillWeapon, Vector *reportedPosition, bool bPhysical, int iElements, int iStatusEffects, float flStatusEffectMagnitude )
{
	Vector vecReported = vec3_origin;
	if ( reportedPosition )
	{
		vecReported = *reportedPosition;
	}
	CFInit( pInflictor, pAttacker, pWeapon, damageForce, damagePosition, vecReported, flDamage, bitsDamageType, iKillWeapon, bPhysical, iElements, iStatusEffects, flStatusEffectMagnitude );
}

void CTakeDamageInfo::AdjustPlayerDamageInflictedForStatistics()
{
#ifndef CLIENT_DLL
	if (!m_hAttacker.Get() || !m_hAttacker->IsPlayer())
		return;

	CBasePlayer *pPlayer = (CBasePlayer*)m_hAttacker.Get();

	float flScale;

	if (m_bPhysical)
		flScale = pPlayer->m_pStats->GetAttackScale();
	else
		flScale = pPlayer->m_pStats->GetEnergyScale();

	// Quad damage!!!
	if (pPlayer->m_pStats->IsInOverdrive())
		flScale *= 2;

	ScaleDamage( flScale );
#endif
}

void CTakeDamageInfo::AdjustPlayerDamageTakenForStatistics(CBasePlayer* pPlayer)
{
#ifndef CLIENT_DLL
	float flScale;

	if (m_bPhysical)
		flScale = pPlayer->m_pStats->GetDefenseScale();
	else
		flScale = pPlayer->m_pStats->GetResistanceScale();

	ToCFPlayer(pPlayer)->ShowDefense(this, flScale);

	ScaleDamage( flScale );

	flScale = pPlayer->m_pStats->GetElementDefenseScale(GetElements());

	//pPlayer->ShowDefense(false, GetElements(), flScale);

	ScaleDamage( flScale );

	m_flStatusEffectMagnitude *= flScale;
#endif
}

void CTakeDamageInfo::Drain()
{
#ifndef CLIENT_DLL
	if (GetDamage() > 0 && GetAttacker())
	{
		if (GetDrainDamage())
			GetAttacker()->TakeHealth( GetDrainDamage() * GetDamage(), 0 );

		if (GetDrainStamina())
		{
			CBaseCombatCharacter* pAttacker = dynamic_cast<CBaseCombatCharacter*>(GetAttacker());
			if (pAttacker)
				pAttacker->m_pStats->m_flStamina = Approach(pAttacker->m_pStats->m_iMaxStamina, pAttacker->m_pStats->m_flStamina, GetDrainStamina() * GetDamage() / 10);
		}
	}
#endif
}