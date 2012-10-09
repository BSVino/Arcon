	//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "in_buttons.h"
#include "tempentity.h"
#include "takedamageinfo.h"
#include "shot_manipulator.h"

#if defined( CLIENT_DLL )

	#include "c_cf_player.h"
	#include "cf_in_main.h"

#else

	#include "cf_player.h"
	#include "baseentity.h"
	#include "soundent.h"

#endif

#include "particle_parse.h"
#include "weapon_magic.h"
#include "cf_gamerules.h"
#include "debugoverlay_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponMagic, DT_WeaponMagic )

BEGIN_NETWORK_TABLE( CWeaponMagic, DT_WeaponMagic )
#ifdef GAME_DLL
	SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),
	SendPropTime( SENDINFO(m_flCastTime) ),
	SendPropTime( SENDINFO(m_flLastCast) ),
	SendPropBool( SENDINFO(m_bCharging) ),
	SendPropTime( SENDINFO(m_flChargeStartTime) ),
	SendPropInt( SENDINFO( m_iChargeAttack ), 4 ),
#else
	RecvPropTime( RECVINFO(m_flCastTime)),
	RecvPropTime( RECVINFO(m_flLastCast)),
	RecvPropBool( RECVINFO(m_bCharging)),
	RecvPropTime( RECVINFO(m_flChargeStartTime)),
	RecvPropInt( RECVINFO( m_iChargeAttack ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponMagic )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_magic, CWeaponMagic );
PRECACHE_WEAPON_REGISTER( weapon_magic );

static ConVar mp_magicchargetime( "mp_magicchargetime", "1", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );

acttable_t CWeaponMagic::s_ReadyActions[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_CF_IDLE_NUMEN_READY,				false },
	{ ACT_MP_RUN,						ACT_CF_RUN_NUMEN_READY,					false },
	{ ACT_CF_CHARGE,					ACT_CF_CHARGEUP_NUMEN_ACTIVE,			false },
	{ ACT_CF_CHARGEUP,					ACT_CF_CHARGEUP_NUMEN_ACTIVE,			false },
	{ ACT_CF_CAST,						ACT_CF_CAST_NUMEN_ACTIVE,				false },
	{ ACT_MP_CROUCH_IDLE,				ACT_CF_IDLE_NUMEN_READY,				false },
	{ ACT_MP_CROUCHWALK,				ACT_CF_RUN_NUMEN_READY,					false },
	{ ACT_MP_JUMP_START,				ACT_CF_JUMP_START_NUMEN_READY,			false },
	{ ACT_MP_JUMP_FLOAT,				ACT_CF_JUMP_FLOAT_NUMEN_READY,			false },
	{ ACT_MP_JUMP_LAND,					ACT_CF_JUMP_LAND_NUMEN_READY,			false },
	{ ACT_CF_POWERJUMP_FORWARD,			ACT_CF_POWERJUMP_FORWARD_NUMEN_READY,	false },
	{ ACT_CF_POWERJUMP_UP,				ACT_CF_POWERJUMP_UP_NUMEN_READY,		false },
	{ ACT_CF_POWERJUMP_BACK,			ACT_CF_POWERJUMP_BACK_NUMEN_READY,		false },
	{ ACT_CF_POWERJUMP_LEFT,			ACT_CF_POWERJUMP_LEFT_NUMEN_READY,		false },
	{ ACT_CF_POWERJUMP_RIGHT,			ACT_CF_POWERJUMP_RIGHT_NUMEN_READY,		false },
	{ ACT_CF_LATCH_JUMP_START,			ACT_CF_JUMP_START_NUMEN_READY,			false },
	{ ACT_CF_LATCH_LEFT,				ACT_CF_LATCH_LEFT_NUMEN_READY,			false },
	{ ACT_CF_LATCH_LEFT_CHARGEUP,		ACT_CF_LATCH_LEFT_CHARGEUP_NUMEN_ACTIVE,false },
	{ ACT_CF_LATCH_LEFT_CAST,			ACT_CF_LATCH_LEFT_CAST_NUMEN_ACTIVE,	false },
	{ ACT_CF_LATCH_RIGHT,				ACT_CF_LATCH_RIGHT_NUMEN_READY,			false },
	{ ACT_CF_LATCH_RIGHT_CHARGEUP,		ACT_CF_LATCH_RIGHT_CHARGEUP_NUMEN_ACTIVE,false },
	{ ACT_CF_LATCH_RIGHT_CAST,			ACT_CF_LATCH_RIGHT_CAST_NUMEN_ACTIVE,	false },
	{ ACT_CF_LATCH_BACK,				ACT_CF_LATCH_BACK_NUMEN_READY,			false },
	{ ACT_CF_LATCH_BACK_CHARGEUP,		ACT_CF_LATCH_BACK_CHARGEUP_NUMEN_ACTIVE,false },
	{ ACT_CF_LATCH_BACK_CAST,			ACT_CF_LATCH_BACK_CAST_NUMEN_ACTIVE,	false },
};

acttable_t CWeaponMagic::s_ActiveActions[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_CF_IDLE_NUMEN_ACTIVE,				false },
	{ ACT_MP_RUN,						ACT_CF_RUN_NUMEN_ACTIVE,				false },
	{ ACT_CF_CHARGE,					ACT_CF_CHARGEUP_NUMEN_ACTIVE,			false },
	{ ACT_CF_CHARGEUP,					ACT_CF_CHARGEUP_NUMEN_ACTIVE,			false },
	{ ACT_CF_CAST,						ACT_CF_CAST_NUMEN_ACTIVE,				false },
	{ ACT_MP_CROUCH_IDLE,				ACT_CF_IDLE_NUMEN_ACTIVE,				false },
	{ ACT_MP_CROUCHWALK,				ACT_CF_RUN_NUMEN_ACTIVE,				false },
	{ ACT_MP_JUMP_START,				ACT_CF_JUMP_START_NUMEN_ACTIVE,			false },
	{ ACT_MP_JUMP_FLOAT,				ACT_CF_JUMP_FLOAT_NUMEN_ACTIVE,			false },
	{ ACT_MP_JUMP_LAND,					ACT_CF_JUMP_LAND_NUMEN_ACTIVE,			false },
	{ ACT_CF_POWERJUMP_FORWARD,			ACT_CF_POWERJUMP_FORWARD_NUMEN_ACTIVE,	false },
	{ ACT_CF_POWERJUMP_UP,				ACT_CF_POWERJUMP_UP_NUMEN_ACTIVE,		false },
	{ ACT_CF_POWERJUMP_BACK,			ACT_CF_POWERJUMP_BACK_NUMEN_ACTIVE,		false },
	{ ACT_CF_POWERJUMP_LEFT,			ACT_CF_POWERJUMP_LEFT_NUMEN_ACTIVE,		false },
	{ ACT_CF_POWERJUMP_RIGHT,			ACT_CF_POWERJUMP_RIGHT_NUMEN_ACTIVE,	false },
	{ ACT_CF_LATCH_JUMP_START,			ACT_CF_JUMP_START_NUMEN_ACTIVE,			false },
	{ ACT_CF_LATCH_LEFT,				ACT_CF_LATCH_LEFT_NUMEN_ACTIVE,			false },
	{ ACT_CF_LATCH_LEFT_CHARGEUP,		ACT_CF_LATCH_LEFT_CHARGEUP_NUMEN_ACTIVE,false },
	{ ACT_CF_LATCH_LEFT_CAST,			ACT_CF_LATCH_LEFT_CAST_NUMEN_ACTIVE,	false },
	{ ACT_CF_LATCH_RIGHT,				ACT_CF_LATCH_RIGHT_NUMEN_ACTIVE,		false },
	{ ACT_CF_LATCH_RIGHT_CHARGEUP,		ACT_CF_LATCH_RIGHT_CHARGEUP_NUMEN_ACTIVE,false },
	{ ACT_CF_LATCH_RIGHT_CAST,			ACT_CF_LATCH_RIGHT_CAST_NUMEN_ACTIVE,	false },
	{ ACT_CF_LATCH_BACK,				ACT_CF_LATCH_BACK_NUMEN_ACTIVE,			false },
	{ ACT_CF_LATCH_BACK_CHARGEUP,		ACT_CF_LATCH_BACK_CHARGEUP_NUMEN_ACTIVE,false },
	{ ACT_CF_LATCH_BACK_CAST,			ACT_CF_LATCH_BACK_CAST_NUMEN_ACTIVE,	false },
};

Activity CWeaponMagic::ActivityOverride( Activity baseAct, bool bActive )
{
	acttable_t *pTable;
	int actCount;
	int i;

	pTable = ActivityListStatic(bActive);
	actCount = ActivityListCountStatic(bActive);

	for ( i = 0; i < actCount; i++, pTable++ )
	{
		if ( baseAct == pTable->baseAct )
		{
			return (Activity)pTable->weaponAct;
		}
	}

	return baseAct;
}

acttable_t* CWeaponMagic::ActivityList(bool bActive)
{
	return ActivityListStatic(bActive);
}

int CWeaponMagic::ActivityListCount(bool bActive)
{
	return ActivityListCountStatic(bActive);
}

acttable_t* CWeaponMagic::ActivityListStatic(bool bActive)
{
	if (bActive)
		return s_ActiveActions;
	else
		return s_ReadyActions;
}

int CWeaponMagic::ActivityListCountStatic(bool bActive)
{
	if (bActive)
		return ARRAYSIZE(s_ActiveActions);
	else
		return ARRAYSIZE(s_ReadyActions);
}

CWeaponMagic::CWeaponMagic()
{
	m_flCastTime = 0;
	m_flLastCast = 0;
	m_iLeftHand = -1;
	m_iRightHand = -1;
	m_bCharging = false;
	m_flChargeStartTime = 0;
	m_iChargeAttack = 0;
	ResetCharge();
}

void CWeaponMagic::Precache( void )
{
	PrecacheScriptSound( "Numen.LightningBullet" );
	PrecacheScriptSound( "Numen.LightningAOE" );
	PrecacheScriptSound( "Numen.LightningBlast" );
	PrecacheScriptSound( "Numen.FireBullet" );
	PrecacheScriptSound( "Numen.FireAOE" );
	PrecacheScriptSound( "Numen.FireBlast" );
	PrecacheScriptSound( "Numen.IceBullet" );
	PrecacheScriptSound( "Numen.IceAOE" );
	PrecacheScriptSound( "Numen.IceBlast" );
	PrecacheScriptSound( "Numen.RestoreBullet" );
	PrecacheScriptSound( "Numen.RestoreAOE" );
	PrecacheScriptSound( "Numen.RestoreBlast" );
	PrecacheScriptSound( "Numen.CastGeneric" );
	PrecacheScriptSound( "Numen.HitGeneric" );
}

void CWeaponMagic::Reset( void )
{
	m_flCastTime = 0;
	m_flLastCast = 0;
	m_flNextPrimaryAttack = 0;
	m_bCharging = false;
	m_flChargeStartTime = 0;
	m_iChargeAttack = 0;
	ResetCharge();
}

void CWeaponMagic::ItemPostFrame( void )
{
	CCFPlayer *pOwner = ToCFPlayer( GetOwner() );
	if (!pOwner)
		return;

	// Don't let people hold charges forever.
	if (IsCharging() && GetChargeStartTime() + 5.0f < gpGlobals->curtime)
	{
		StopCharge(false);
		pOwner->m_pStats->m_flFocus -= 50;
		m_flNextPrimaryAttack = gpGlobals->curtime + 3;
	}

	int iAttack = 0;
	if ((pOwner->m_nButtons & IN_ATTACK) && m_iChargeAttack != 0)
		iAttack = 0;
	else if ((pOwner->m_nButtons & IN_ATTACK2))
		iAttack = 1;

	if (IsCharging() && pOwner->m_pStats->GetFocus() < 0)
		StopCharge(false);

	if (pOwner->IsMagicMode() && (m_flCastTime <= gpGlobals->curtime) &&
		(m_flNextPrimaryAttack <= gpGlobals->curtime) &&
		(pOwner->m_afButtonPressed & (IN_ATTACK|IN_ATTACK2)))
	{
		StartAttack(iAttack);
	}
	else if ( pOwner->IsMagicMode() && IsCharging(false) && !(pOwner->m_nButtons & (IN_ATTACK|IN_ATTACK2))
		&& (m_flNextPrimaryAttack <= gpGlobals->curtime) )
	{
		StartAttack(m_iChargeAttack);
	}
	else if ( pOwner->IsMagicMode()
		&& !IsCharging(false) && (pOwner->m_nButtons & (IN_ATTACK|IN_ATTACK2))
		&& (m_flNextPrimaryAttack <= gpGlobals->curtime) && m_flCastTime <= 0 )
	{
		StartCharge(iAttack);
	}

	if (m_flCastTime != 0 && m_flCastTime <= gpGlobals->curtime)
		Attack();
}

void CWeaponMagic::StartAttack(int iAttack)
{
	if (m_flCastTime)
		return;

	CCFPlayer *pOwner = ToCFPlayer( GetOwner() );
	if (!pOwner)
		return;

	m_CastBind.m_iWeapon = pOwner->GetActiveArmament()->m_aAttackBinds[iAttack].m_iWeapon;
	m_CastBind.m_iRune = pOwner->GetActiveArmament()->m_aAttackBinds[iAttack].m_iRune;
	m_CastBind.m_iMod = pOwner->GetActiveArmament()->m_aAttackBinds[iAttack].m_iMod;

	if (m_CastBind.m_iWeapon < 0 || m_CastBind.m_iRune < 0)
		return;

	if (pOwner->m_pStats->m_flFocus < 0)
		return;

#ifdef GAME_DLL
	float flCastingCost = pOwner->m_pStats->GetMagicalAttackCost(&m_CastBind);
	pOwner->m_pStats->m_flFocus -= flCastingCost * GetChargeMultiplier();
#endif

#ifdef CLIENT_DLL
	pOwner->m_iLastCombo = iAttack;
#endif

	float flCastTime = pOwner->m_pStats->GetMagicalAttackCastTime(&m_CastBind);

	m_flCastTime = gpGlobals->curtime
		+ (flCastTime * pOwner->m_pStats->GetSpeedInvScale());

	pOwner->m_flNextAttack = m_flNextPrimaryAttack = m_flCastTime
		+ (pOwner->m_pStats->GetMagicalAttackReload(&m_CastBind)
		* pOwner->m_pStats->GetSpeedInvScale());

	if (m_iRightHand == -1)
	{
		m_iLeftHand = pOwner->LookupAttachment("lmagic");
		m_iRightHand = pOwner->LookupAttachment("rmagic");
	}

	Vector vecLeft, vecRight;
	pOwner->GetAttachment(m_iLeftHand, vecLeft);
	pOwner->GetAttachment(m_iRightHand, vecRight);

	Vector vecSrc;

	if (pOwner->GetPrimaryWeapon() && !pOwner->GetSecondaryWeapon())
	{
		if (pOwner->GetPrimaryWeapon()->IsMeleeWeapon())
			m_bRightHandCast = true;
		else
			m_bRightHandCast = false;
	}
	else
	{
		if (iAttack == 1)
			m_bRightHandCast = true;
		else
			m_bRightHandCast = false;
	}

	if (m_bRightHandCast)
		vecSrc = vecRight;
	else
		vecSrc = vecLeft;

	m_flLastCast = gpGlobals->curtime;

	if (m_iChargeAttack == iAttack)
	{
		if (IsCharging(false))
			// Only get the multiplier if the player shows actual intent that he wanted to charge.
			// Otherwise spamming the attack button can set this off.
			StopCharge(IsCharging());
		else
			ResetCharge();
	}

	if (pOwner->m_pStats->GetFocus() <= 0)
	{
		if (GetChargeAmount())
		{
			StopAttack();
			return;
		}
	}

	ToCFPlayer(pOwner)->DoAnimationEvent( PLAYERANIMEVENT_CHARGEUP );

#ifdef CLIENT_DLL
	if (C_CFPlayer::GetLocalCFPlayer() == ToCFPlayer(pOwner) && !CFInput()->CAM_IsThirdPerson())
		DispatchNumenEffect("numen_cast_local", PATTACH_CUSTOMORIGIN, pOwner, (vecLeft+vecRight)/2, (vecLeft+vecRight)/2);
	else
#endif
		DispatchNumenEffect("numen_cast", PATTACH_CUSTOMORIGIN, pOwner, vecSrc, vecSrc);

	DispatchNumenCastEffect( pOwner->GetActiveArmament()->SerializeCombo(m_CastBind.m_iWeapon, m_CastBind.m_iRune), pOwner, vecSrc, pOwner->EyeAngles() );

	char* pszSound = "Numen.CastGeneric";
	CSoundParameters params;
	if ( GetParametersForSound( pszSound, params, NULL ) )
	{
		CPASAttenuationFilter filter( GetOwner(), params.soundlevel );
		if ( IsPredicted() )
			filter.UsePredictionRules();
		EmitSound( filter, GetOwner()->entindex(), pszSound, NULL, 0 ); 
	}
}

void CWeaponMagic::StopAttack()
{
	m_flCastTime = 0;
	StopCharge(false);
}

void CWeaponMagic::StartCharge(int iAttack)
{
	if (m_bCharging)
		return;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
		return;

	// If we don't have the juice, don't even bother trying to charge it up.
	if (ToCFPlayer(pOwner)->m_pStats->GetFocus() <= 0)
		return;

	ToCFPlayer(pOwner)->DoAnimationEvent( PLAYERANIMEVENT_CHARGE, 0, !!(pOwner->m_nButtons & (IN_ATTACK2)));

	m_iChargeAttack = iAttack;
	m_flChargeStartTime = gpGlobals->curtime;
	m_bCharging = true;
}

void CWeaponMagic::StopCharge(bool bMultiplier)
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
		return;

	if (bMultiplier)
	{
		m_flChargeAmount = RemapValClamped(gpGlobals->curtime, m_flChargeStartTime, m_flChargeStartTime + mp_magicchargetime.GetFloat(), 0.0f, 1.0f);
		m_flChargeMultiplier = RemapValClamped(m_flChargeAmount, 0, 1, 1.5f, 2.0f);
	}

	// Get rid of the charging animation.
	ToCFPlayer(pOwner)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK, ACT_INVALID );

	// Only award a proper lesson learned if we are sure that the player has not tapped the attack button.
	if (m_flChargeAmount > 0.3f)
		ToCFPlayer(pOwner)->Instructor_LessonLearned(HINT_HOLD_CHARGE);

	m_bCharging = false;
}

void CWeaponMagic::ResetCharge()
{
	m_flChargeMultiplier = 1;
	m_flChargeAmount = 0;
}

bool CWeaponMagic::IsCharging(bool bDelay)
{
	if (bDelay && (gpGlobals->curtime - m_flChargeStartTime) < 0.25f)
		return false;

	return m_bCharging;
}

void CWeaponMagic::Attack()
{
	CCFPlayer *pOwner = ToCFPlayer( GetOwner() );
	if (!pOwner)
		return;

	if (m_CastBind.m_iWeapon < 0 || m_CastBind.m_iRune < 0)
	{
		Assert(!"Invalid cast bind.");
		return;
	}

	m_flCastTime = 0;
	m_flLastCast = gpGlobals->curtime;

	ToCFPlayer(pOwner)->DoAnimationEvent( PLAYERANIMEVENT_NUMEN );

	float flDamage = pOwner->m_pStats->GetMagicalAttackDamage(&m_CastBind) * GetChargeMultiplier();
	element_t eElement = pOwner->m_pStats->GetMagicalAttackElement(&m_CastBind);

	Vector vecLeft, vecRight, vecSrc;
	pOwner->GetAttachment(m_iLeftHand, vecLeft);
	pOwner->GetAttachment(m_iRightHand, vecRight);

	if (m_bRightHandCast)
		vecSrc = vecRight;
	else
		vecSrc = vecLeft;

	Color c = ElementToColor(eElement);

	if (flDamage < 0)
		c = Color(0, 255, 0);

	CPVSFilter filter(vecSrc);
	te->DynamicLight( filter, 0.0, &(vecSrc), c.r(), c.g(), c.b(), 5, 70, 0.5f, 50 );

	CArmament* pArm = pOwner->GetActiveArmament();

	switch (pArm->GetDominantForce(m_CastBind.m_iWeapon, m_CastBind.m_iRune))
	{
	case FE_BULLET:
		AttackBulletForce(
			flDamage,
			eElement
			);
		break;
	case FE_AOE:
		AttackAreaEffectForce(
			flDamage,
			eElement
			);
		break;
	case FE_BLAST:
		AttackBlastForce(
			flDamage,
			eElement
			);
		break;
	case FE_PROJECTILE:
		AttackProjectileForce(
			flDamage,
			eElement
			);
		break;
	case FE_SOFTNADE:
		AttackSoftNadeForce(
			flDamage,
			eElement
			);
		break;
	default:
		AssertMsg(false, "Unrecognized dominant force rune.");
		break;
	}
}

#ifdef _DEBUG
#define SHOW_MAGIC_TRACES "1"
#else
#define SHOW_MAGIC_TRACES "0"
#endif

ConVar sv_showmagictraces("sv_showmagictraces", SHOW_MAGIC_TRACES, FCVAR_REPLICATED|FCVAR_CHEAT);

void CWeaponMagic::AttackBulletForce(float flDamage, element_t iElement)
{
	CCFPlayer *pOwner = ToCFPlayer( GetOwner() );
	if (!pOwner)
		return;

	Vector vecLeft, vecRight, vecSrc, vecEye, vecDir;
	pOwner->GetAttachment(m_iLeftHand, vecLeft);
	pOwner->GetAttachment(m_iRightHand, vecRight);
	vecEye = pOwner->EyePosition();

	if (m_bRightHandCast)
		vecSrc = vecRight;
	else
		vecSrc = vecLeft;

	AngleVectors(pOwner->EyeAngles(), &vecDir);

	int iBeamWidth = 10;
	Vector vecBeamMax = Vector(iBeamWidth, iBeamWidth, iBeamWidth)/2;

	trace_t		tr;
	UTIL_TraceHull( vecSrc, vecEye + vecDir * 1500, -vecBeamMax, vecBeamMax, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &tr);

#ifdef GAME_DLL
	if (sv_showmagictraces.GetBool())
	{
		if (tr.fraction == 1.0f)
			NDebugOverlay::SweptBox(vecSrc, tr.endpos, -vecBeamMax, vecBeamMax, QAngle(1, 0, 0), 0, 0, 255, 255, 20);
		else
			NDebugOverlay::SweptBox(vecSrc, tr.endpos, -vecBeamMax, vecBeamMax, QAngle(1, 0, 0), 255, 0, 0, 255, 20);
	}
#endif

	if (iElement & ELEMENT_FIRE)
	{
		trace_t		linetr;
		UTIL_TraceLine ( vecSrc, tr.endpos + -tr.plane.normal*iBeamWidth, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &linetr);
		UTIL_DecalTrace(&linetr, "Numen.FireBurnMark");
	}

	DispatchNumenEffect(iElement, pOwner->GetActiveArmament()->SerializeCombo(m_CastBind.m_iWeapon, m_CastBind.m_iRune),
		FE_BULLET, this, vecSrc, tr.endpos);

	AttackSound("Bullet", iElement);

	if ( tr.fraction == 1.0 )
		return;
	
	if ( !tr.m_pEnt )
		return;

	//flDamage -= (vecSrc - tr.endpos).Length();

	CTakeDamageInfo info;
	info.CFSet( this, pOwner, vec3_origin, tr.endpos, flDamage, DMG_BLAST, WEAPON_MAGIC, &tr.endpos, false, iElement );

	info.AddStatusEffects( pOwner->m_pStats->GetMagicalAttackStatusEffect(&m_CastBind), pOwner->GetActiveArmament()->GetMagicalAttackStatusMagnitude(&m_CastBind));
	info.SetNumen( pOwner->GetActiveArmament()->SerializeCombo(m_CastBind.m_iWeapon, m_CastBind.m_iRune) );
	info.SetDrainDamage( pOwner->m_pStats->GetDrainHealthRate(m_CastBind.m_iWeapon, m_CastBind.m_iRune) );
	info.SetDrainFocus( pOwner->m_pStats->GetDrainFocusRate(m_CastBind.m_iWeapon, m_CastBind.m_iRune) );
	info.SetDrainStamina( pOwner->m_pStats->GetDrainStaminaRate(m_CastBind.m_iWeapon, m_CastBind.m_iRune) );

	CalculateExplosiveDamageForce( &info, vecDir, vecSrc );

	if (info.GetDamage() < 0)
	{
#ifdef GAME_DLL
		HealPlayer(ToCFPlayer(tr.m_pEnt), ToCFPlayer(info.GetAttacker()), -info.GetDamage());
#endif
	}
	else
	{
		ClearMultiDamage();
		tr.m_pEnt->DispatchTraceAttack( info, vecDir, &tr );
		ApplyMultiDamage();
	}
}

void CWeaponMagic::AttackBlastForce(float flDamage, element_t iElement)
{
	CCFPlayer *pOwner = ToCFPlayer( GetOwner() );
	if (!pOwner)
		return;

	trace_t		tr;

	Vector vecLeft, vecRight, vecSrc, vecEye, vecForward;
	pOwner->GetAttachment(m_iLeftHand, vecLeft);
	pOwner->GetAttachment(m_iRightHand, vecRight);
	vecEye = pOwner->EyePosition();

	if (m_bRightHandCast)
		vecSrc = vecRight;
	else
		vecSrc = vecLeft;

	AngleVectors(pOwner->EyeAngles(), &vecForward);

	ClearMultiDamage();

	Vector vecEndpoint = vecEye + vecForward * 256;

	UTIL_TraceLine ( vecSrc, vecEndpoint, MASK_SHOT_HULL|CONTENTS_HITBOX, pOwner, COLLISION_GROUP_NONE, &tr);

	DispatchNumenEffect(iElement, pOwner->GetActiveArmament()->SerializeCombo(m_CastBind.m_iWeapon, m_CastBind.m_iRune),
		FE_BLAST, this, vecSrc, tr.endpos);

	Vector vecMin;
	Vector vecMax;
	VectorMin(vecSrc, vecEndpoint, vecMin);
	VectorMax(vecSrc, vecEndpoint, vecMax);

	CBaseEntity* list[256];
	int count = UTIL_EntitiesInBox( list, 256, vecMin, vecMax, 0 );

	for( int i = 0 ; i < count ; i++ )
	{
		if (!list[i]->IsPlayer())
			continue;

		CCFPlayer* pVictim = ToCFPlayer(list[i]);

		if (pVictim == pOwner)
			continue;

		Vector vecToTarget = pVictim->GetCentroid() - vecSrc;

		VectorNormalize(vecToTarget);

		// Using dot product for a cone isn't really accurate, but eh, it's an improvement.
		float flDot = DotProduct(vecForward, vecToTarget);
		if (flDot < 0.94f)
			continue;

		UTIL_TraceLine ( vecSrc, pVictim->GetCentroid(), MASK_SHOT_HULL|CONTENTS_HITBOX, pOwner, COLLISION_GROUP_NONE, &tr);
	
		if ( tr.fraction == 1.0 )
			continue;
	
		if ( !tr.m_pEnt )
			continue;

		if ( tr.m_pEnt != list[i] )
			continue;

		CTakeDamageInfo info;
		info.CFSet( this, pOwner, vec3_origin, tr.endpos, flDamage, DMG_BLAST, WEAPON_MAGIC, &tr.endpos, false, iElement );

		info.AddStatusEffects( pOwner->m_pStats->GetMagicalAttackStatusEffect(&m_CastBind), pOwner->GetActiveArmament()->GetMagicalAttackStatusMagnitude(&m_CastBind));
		info.SetNumen( pOwner->GetActiveArmament()->SerializeCombo(m_CastBind.m_iWeapon, m_CastBind.m_iRune) );
		info.SetDrainDamage( pOwner->m_pStats->GetDrainHealthRate(m_CastBind.m_iWeapon, m_CastBind.m_iRune) );
		info.SetDrainFocus( pOwner->m_pStats->GetDrainFocusRate(m_CastBind.m_iWeapon, m_CastBind.m_iRune) );
		info.SetDrainStamina( pOwner->m_pStats->GetDrainStaminaRate(m_CastBind.m_iWeapon, m_CastBind.m_iRune) );

		CalculateExplosiveDamageForce( &info, vecForward, vecSrc );

		if (info.GetDamage() < 0)
		{
#ifdef GAME_DLL
			HealPlayer(ToCFPlayer(tr.m_pEnt), ToCFPlayer(info.GetAttacker()), -info.GetDamage());
#endif
		}
		else
		{
			tr.m_pEnt->DispatchTraceAttack( info, vecForward, &tr );
		}
	}

	ApplyMultiDamage();

	AttackSound("Blast", iElement);
}

void CWeaponMagic::AttackAreaEffectForce(float flDamage, element_t iElement)
{
	CCFPlayer *pOwner = ToCFPlayer( GetOwner() );
	if (!pOwner)
		return;

	CArmament* pArm = pOwner->GetActiveArmament();

	Vector vecSrc = pOwner->GetCentroid();

	float flRadius = fabs(flDamage) * pArm->GetRadiusMultiplier(&m_CastBind);

	Explode( pOwner, this, pArm->GetMagicalAttackPositive(&m_CastBind)?NULL:pOwner, vecSrc, flDamage, flRadius, iElement,
		pOwner->GetActiveArmament()->GetMagicalAttackStatusEffect(&m_CastBind),
		pOwner->GetActiveArmament()->GetMagicalAttackStatusMagnitude(&m_CastBind),
		FE_AOE,
		pOwner->m_pCurrentArmament->SerializeCombo(m_CastBind.m_iWeapon, m_CastBind.m_iRune),
		pOwner->m_pStats->GetDrainHealthRate(m_CastBind.m_iWeapon, m_CastBind.m_iRune),
		pOwner->m_pStats->GetDrainFocusRate(m_CastBind.m_iWeapon, m_CastBind.m_iRune),
		pOwner->m_pStats->GetDrainStaminaRate(m_CastBind.m_iWeapon, m_CastBind.m_iRune));

	AttackSound("AoE", iElement);
}

void CWeaponMagic::AttackProjectileForce(float flDamage, element_t iElement)
{
	CCFPlayer *pOwner = ToCFPlayer( GetOwner() );
	if (!pOwner)
		return;

	ThrowGrenade(flDamage, pOwner->GetActiveArmament()->SerializeCombo(m_CastBind.m_iWeapon, m_CastBind.m_iRune), iElement, FE_PROJECTILE,
		pOwner->GetActiveArmament()->GetMagicalAttackStatusEffect(&m_CastBind),
		pOwner->GetActiveArmament()->GetMagicalAttackStatusMagnitude(&m_CastBind),
		pOwner->m_pStats->GetDrainHealthRate(m_CastBind.m_iWeapon, m_CastBind.m_iRune),
		pOwner->m_pStats->GetDrainFocusRate(m_CastBind.m_iWeapon, m_CastBind.m_iRune),
		pOwner->m_pStats->GetDrainStaminaRate(m_CastBind.m_iWeapon, m_CastBind.m_iRune));
}

void CWeaponMagic::AttackSoftNadeForce(float flDamage, element_t iElement)
{
	CCFPlayer *pOwner = ToCFPlayer( GetOwner() );
	if (!pOwner)
		return;

	ThrowGrenade(flDamage, pOwner->GetActiveArmament()->SerializeCombo(m_CastBind.m_iWeapon, m_CastBind.m_iRune), iElement, FE_AOE,
		pOwner->GetActiveArmament()->GetMagicalAttackStatusEffect(&m_CastBind),
		pOwner->GetActiveArmament()->GetMagicalAttackStatusMagnitude(&m_CastBind),
		pOwner->m_pStats->GetDrainHealthRate(m_CastBind.m_iWeapon, m_CastBind.m_iRune),
		pOwner->m_pStats->GetDrainFocusRate(m_CastBind.m_iWeapon, m_CastBind.m_iRune),
		pOwner->m_pStats->GetDrainStaminaRate(m_CastBind.m_iWeapon, m_CastBind.m_iRune));
}

void DrawDebugSphere(Vector vecCenter, float flRadius)
{
	int iLatitudes = 10;
	int iLongitudes = 10;

	QAngle angCoordinate(0, 0, 0);
	for (int i = 0; i < iLatitudes; i++)
	{
		for (int j = 0; j < iLongitudes; j++)
		{
			QAngle angCoordinate = QAngle(RemapVal(j, 0, iLongitudes, -90, 90), RemapVal(i, 0, iLatitudes, 0, 360), 0);
			QAngle angNextLongitude = QAngle(RemapVal(j+1, 0, iLongitudes, -90, 90), RemapVal(i, 0, iLatitudes, 0, 360), 0);
			QAngle angNextLatitude = QAngle(RemapVal(j, 0, iLongitudes, -90, 90), RemapVal(i+1, 0, iLatitudes, 0, 360), 0);

			Vector vecCoordinate, vecNextLongitude, vecNextLatitude;

			AngleVectors(angCoordinate, &vecCoordinate);
			AngleVectors(angNextLongitude, &vecNextLongitude);
			AngleVectors(angNextLatitude, &vecNextLatitude);

			NDebugOverlay::Line(vecCenter + vecCoordinate*flRadius, vecCenter + vecNextLongitude*flRadius, 0, 0, 255, false, 20);
			NDebugOverlay::Line(vecCenter + vecCoordinate*flRadius, vecCenter + vecNextLatitude*flRadius, 0, 0, 255, false, 20);
		}
	}
}

void CWeaponMagic::Explode( CBaseCombatCharacter* pAttacker,
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
						   float flDrainStamina )
{
	DispatchNumenEffect(iElement, iNumen, eForce, pAttacker, vecPosition, Vector(0,0,0), flRadius);

#if !defined( CLIENT_DLL )
	CSoundEnt::InsertSound ( SOUND_COMBAT, vecPosition, 1024, 3.0 );

	// Use the caster's position as the reported position
	Vector vecReported = pAttacker ? pAttacker->GetAbsOrigin() : vecPosition;
	
	CTakeDamageInfo info;
	info.CFSet( pInflictor, pAttacker, vec3_origin, vecPosition, flDamage, DMG_BLAST, WEAPON_MAGIC, &vecPosition, false, iElement );

	info.AddStatusEffects( eStatusEffects, flStatusMagnitude);
	info.SetNumen( iNumen );
	info.SetDrainDamage( flDrainHealth );
	info.SetDrainFocus( flDrainFocus );
	info.SetDrainStamina( flDrainStamina );

	RadiusDamage( info, vecPosition, flRadius, CLASS_NONE, pIgnore );

	if (sv_showmagictraces.GetBool())
		DrawDebugSphere(vecPosition, flRadius);

//	UTIL_ScreenShake( vecPosition, flDamage/20, flDamage/3, 1.0, flDamage*1.2, SHAKE_START );
#endif
}

void CWeaponMagic::AttackSound(char* pszType, element_t iElement)
{
	// Play all of the appropriate sounds.
	for (int i = 0; i < sizeof(element_t)*4; i++)
	{
		if (!((1<<i) & iElement))
			continue;

		char* pszSound;
		pszSound = VarArgs("Numen.%s%s", ElementToString((element_t)(1<<i)), pszType);

		CSoundParameters params;
		if ( GetParametersForSound( pszSound, params, NULL ) )
		{
			CPASAttenuationFilter filter( GetOwner(), params.soundlevel );
			if ( IsPredicted() )
			{
				filter.UsePredictionRules();
			}
			EmitSound( filter, GetOwner()->entindex(), pszSound, NULL, 0 ); 
		}
	}
}

void CWeaponMagic::HealPlayer(CCFPlayer* pTarget, CCFPlayer* pAttacker, float flHeal)
{
#ifdef GAME_DLL
	if (!pTarget)
		return;
	
	if (CFGameRules()->PlayerRelationship(pTarget, pAttacker) == GR_TEAMMATE)
		pTarget->TakeHealth(flHeal, 0);
	
#endif
}

#ifdef GAME_DLL
int CWeaponMagic::UpdateTransmitState( void)
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}
#endif

void DispatchNumenCastEffect( long iSerializedCombo, CBaseEntity *pEntity, Vector vecPos, QAngle angDir )
{
	Assert(pEntity);

	CEffectData	data;

	data.m_nHitBox = iSerializedCombo;

#ifdef CLIENT_DLL
	data.m_hEntity = pEntity;
#else
	data.m_nEntIndex = pEntity->entindex();
#endif

	data.m_vStart = vecPos;
	data.m_vAngles = angDir;

	DispatchEffect( "NumenCastEffect", data );
}

void DispatchNumenEffect( element_t eElement, long iNumen, forceeffect_t eForce, CBaseEntity *pEntity, Vector vecCP1, Vector vecCP2, float flC1, float flC2, float flC3 )
{
	const char* pszForceEffect = CRuneData::ForceEffectToString( eForce );

	// Deploy elementals
	for (int i = 0; i < sizeof(element_t)*4; i++)
	{
		if (!((1<<i) & eElement))
			continue;

		DispatchNumenEffect(VarArgs("numen_%s_%s", ElementToString((element_t)(1<<i)), pszForceEffect), PATTACH_CUSTOMORIGIN, pEntity, vecCP1, vecCP2, flC1, flC2, flC3);
	}

	// Deploy typeless bases.
	for (int i = 0; i < sizeof(iNumen)*4; i++)
	{
		CRuneData* pData;

		if (!((1<<i) & iNumen))
			continue;

		pData = CRuneData::GetData(i);

		if (pData->m_eType == RUNETYPE_BASE && pData->m_eElement == ELEMENT_TYPELESS)
			DispatchNumenEffect(VarArgs("numen_%s_%s", pData->m_szName, pszForceEffect), PATTACH_CUSTOMORIGIN, pEntity, vecCP1, vecCP2, flC1, flC2, flC3);
	}
}

void DispatchNumenEffect( const char *pszParticleName, ParticleAttachment_t iAttachType, CBaseEntity *pEntity, Vector vecCP1, Vector vecCP2, float flC1, float flC2, float flC3 )
{
	CEffectData	data;

	data.m_nHitBox = GetParticleSystemIndex( pszParticleName );
	if ( pEntity )
	{
#ifdef CLIENT_DLL
		data.m_hEntity = pEntity;
#else
		data.m_nEntIndex = pEntity->entindex();
#endif
		data.m_fFlags |= PARTICLE_DISPATCH_FROM_ENTITY;
	}
	data.m_nDamageType = iAttachType;

	data.m_vStart = vecCP1;
	data.m_vOrigin = vecCP2;
	Vector vecForward = vecCP2 - vecCP1;
	QAngle angForward;
	VectorAngles(vecCP2 - vecCP1, angForward);
	data.m_vAngles = angForward;

	data.m_flScale = flC1;
	data.m_flMagnitude = flC2;
	data.m_flRadius = flC3;

	DispatchEffect( "NumenEffect", data );
}

void StopNumenEffects( CBaseEntity *pEntity )
{
	CEffectData	data;

	if ( pEntity )
	{
#ifdef CLIENT_DLL
		data.m_hEntity = pEntity;
#else
		data.m_nEntIndex = pEntity->entindex();
#endif
	}

	DispatchEffect( "NumenEffectStop", data );
}

void CWeaponMagic::ThrowGrenade(float flDamage, long iNumen, element_t eElement, forceeffect_t eForceEffect, statuseffect_t eStatusEffect, float flStatusMagnitude, float flHealthDrain, float flStaminaDrain, float flFocusDrain)
{
#ifdef GAME_DLL
	CCFPlayer *pOwner = ToCFPlayer( GetOwner() );
	if (!pOwner)
		return;

	QAngle angThrow = pOwner->LocalEyeAngles();

	Vector vForward, vRight, vUp;

	if (angThrow.x < 90 )
		angThrow.x = -10 + angThrow.x * ((90 + 10) / 90.0);
	else
	{
		angThrow.x = 360.0f - angThrow.x;
		angThrow.x = -10 + angThrow.x * -((90 - 10) / 90.0);
	}

	AngleVectors( angThrow, &vForward, &vRight, &vUp );

	Vector vecSrc = pOwner->GetAbsOrigin() + pOwner->GetViewOffset();

	vecSrc += vForward * 16;

	Vector vecVelocity = ( vForward * 900 ) + ( vUp * 100.0f ) + ( random->RandomFloat( -10.0f, 10.0f ) * vRight ) +		
		( random->RandomFloat( -10.0f, 10.0f ) * vUp );

	CMagicGrenade* pGrenade = EmitGrenade( vecSrc, vec3_angle, vecVelocity, AngularImpulse(600,random->RandomInt(-1200,1200),0) );

	pGrenade->SetNumen(iNumen);
	pGrenade->SetElement(eElement);
	pGrenade->SetStatusEffect(eStatusEffect);
	pGrenade->SetStatusMagnitude(flStatusMagnitude);
	pGrenade->SetForceEffect(eForceEffect);
	pGrenade->SetDrainage(flHealthDrain, flStaminaDrain, flFocusDrain);

	CArmament* pArm = pOwner->GetActiveArmament();
	float flRadius = fabs(flDamage) * pArm->GetRadiusMultiplier(&m_CastBind);
	pGrenade->SetDamageRadius(flRadius);
	pGrenade->SetDamage(flDamage);
#endif
}

#ifdef GAME_DLL
CMagicGrenade* CWeaponMagic::EmitGrenade(Vector vecSrc, QAngle vecAngles, Vector vecVel, AngularImpulse angImpulse)
{
	CCFPlayer *pOwner = ToCFPlayer( GetOwner() );
	if (!pOwner)
		return NULL;

	return CMagicGrenade::Create( vecSrc, vecAngles, vecVel, angImpulse, pOwner, 2 );
}
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( MagicGrenade, DT_MagicGrenade )

#ifdef GAME_DLL
BEGIN_DATADESC( CMagicGrenade )
	DEFINE_ENTITYFUNC(DetonateTouch),
END_DATADESC()
#endif

LINK_ENTITY_TO_CLASS( magic_grenade, CMagicGrenade );

BEGIN_NETWORK_TABLE( CMagicGrenade, DT_MagicGrenade )
#ifdef CLIENT_DLL
	RecvPropFloat( RECVINFO( m_flRealDamage ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_iNumen), RecvPropInt( RECVINFO( m_iNumen[ 0 ] ))),
	RecvPropInt( RECVINFO( m_eElement ) ),
	RecvPropInt( RECVINFO( m_eStatusEffect ) ),
#else
	SendPropFloat( SENDINFO( m_flRealDamage ), 10, SPROP_ROUNDDOWN, -2048.0, 2048.0 ),
	SendPropArray3(SENDINFO_ARRAY3(m_iNumen), SendPropInt( SENDINFO_ARRAY( m_iNumen ), 1, SPROP_UNSIGNED ) ),
	SendPropInt( SENDINFO( m_eElement ), TOTAL_ELEMENTS ),
	SendPropInt( SENDINFO( m_eStatusEffect ), TOTAL_STATUSEFFECTS ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CMagicGrenade )
END_PREDICTION_DATA()

#ifdef GAME_DLL

CMagicGrenade* CMagicGrenade::Create( 
	const Vector &position, 
	const QAngle &angles, 
	const Vector &velocity, 
	const AngularImpulse &angVelocity, 
	CBaseCombatCharacter *pOwner, 
	float timer )
{
	CMagicGrenade *pGrenade = (CMagicGrenade*)CBaseEntity::Create( "magic_grenade", position, angles, pOwner );
	
	// Set the timer for 1 second less than requested. We're going to issue a SOUND_DANGER
	// one second before detonation.

	pGrenade->SetDetonateTimerLength( 1.5 );
	pGrenade->SetAbsVelocity( velocity );
	pGrenade->SetupInitialTransmittedGrenadeVelocity( velocity );
	pGrenade->SetThrower( pOwner ); 

	pGrenade->SetGravity( BaseClass::GetGrenadeGravity() );
	pGrenade->SetFriction( BaseClass::GetGrenadeFriction() );
	pGrenade->SetElasticity( BaseClass::GetGrenadeElasticity() );

	pGrenade->SetDamage(100);
	pGrenade->SetDamageRadius(pGrenade->GetDamage() * 3.5f);
	pGrenade->ChangeTeam( pOwner->GetTeamNumber() );
	pGrenade->ApplyLocalAngularVelocityImpulse( angVelocity );	

	// make NPCs afaid of it while in the air
	pGrenade->SetThink( &CMagicGrenade::DangerSoundThink );
	pGrenade->SetNextThink( gpGlobals->curtime );

	return pGrenade;
}

void CMagicGrenade::Spawn()
{
	BaseClass::Spawn();

	SetTouch( &CMagicGrenade::DetonateTouch );
}

void CMagicGrenade::Precache()
{
	PrecacheScriptSound( "Numen.GrenadeBounce" );

	BaseClass::Precache();
}

void CMagicGrenade::BounceSound( void )
{
	EmitSound( "Numen.GrenadeBounce" );
}

void CMagicGrenade::DetonateTouch( CBaseEntity *pOther )
{
	if ( pOther == GetThrower() )
		return;

	// Verify a correct "other."
	if ( !pOther->IsSolid() || pOther->IsSolidFlagSet( FSOLID_VOLUME_CONTENTS ) )
		return;

	// Handle hitting skybox (disappear).
	trace_t pTrace;
	Vector velDir = GetAbsVelocity();
	VectorNormalize( velDir );
	Vector vecSpot = GetAbsOrigin() - velDir * 32;
	UTIL_TraceLine( vecSpot, vecSpot + velDir * 64, MASK_SOLID, this, COLLISION_GROUP_NONE, &pTrace );

	if ( pTrace.fraction < 1.0 && pTrace.surface.flags & SURF_SKY )
	{
		UTIL_Remove( this );
		return;
	}

	if (GetForceEffect() == FE_PROJECTILE)
	{
		DirectDamage(pOther);
		return;
	}

	// Blow up if we hit an enemy we can damage
	if ( CFGameRules()->PlayerRelationship(GetThrower(), pOther) == GR_NOTTEAMMATE && pOther->m_takedamage != DAMAGE_NO )
	{
		SetThink( &CMagicGrenade::Detonate );
		SetNextThink( gpGlobals->curtime );
	}
}

void CMagicGrenade::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	BaseClass::VPhysicsCollision( index, pEvent );

	int otherIndex = !index;
	CBaseEntity *pHitEntity = pEvent->pEntities[otherIndex];

	if ( !pHitEntity )
		return;

	if (GetForceEffect() == FE_PROJECTILE)
	{
		DirectDamage(pHitEntity);
		return;
	}

	// Blow up if we hit an enemy we can damage
	if ( CFGameRules()->PlayerRelationship(GetThrower(), pHitEntity) == GR_NOTTEAMMATE && pHitEntity->m_takedamage != DAMAGE_NO )
	{
		SetThink( &CMagicGrenade::Detonate );
		SetNextThink( gpGlobals->curtime );
	}
}

void CMagicGrenade::DirectDamage(CBaseEntity* pHitEntity)
{
	Vector vecAbsOrigin = GetAbsOrigin();

	CTakeDamageInfo info;
	info.CFSet( this, GetThrower(), vec3_origin, vecAbsOrigin, GetDamage(), DMG_BLAST, WEAPON_MAGIC, &vecAbsOrigin, false, GetElement() );

	info.AddStatusEffects( GetStatusEffect(), GetStatusMagnitude());
	info.SetNumen( GetNumen() );
	info.SetDrainDamage( GetHealthDrainage() );
	info.SetDrainFocus( GetFocusDrainage() );
	info.SetDrainStamina( GetStaminaDrainage() );

	CalculateExplosiveDamageForce( &info, pHitEntity->GetAbsOrigin()-GetAbsOrigin(), GetAbsOrigin() );

	if (info.GetDamage() < 0)
#ifdef GAME_DLL
		CWeaponMagic::HealPlayer(ToCFPlayer(pHitEntity), ToCFPlayer(info.GetAttacker()), -info.GetDamage());
#endif
	else
	{
		// Do a token trace just so that we have something to pass into DispatchTraceAttack().
		trace_t tr;
		UTIL_TraceLine(GetAbsOrigin(), pHitEntity->GetAbsOrigin(), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr);

		ClearMultiDamage();
		pHitEntity->DispatchTraceAttack( info, pHitEntity->GetAbsOrigin()-GetAbsOrigin(), &tr );
		ApplyMultiDamage();
	}

	RemoveMe();
}

void CMagicGrenade::Detonate()
{
	trace_t		tr;
	Vector		vecSpot;// trace starts here!

	SetThink( NULL );

	vecSpot = GetAbsOrigin() + Vector ( 0 , 0 , 8 );
	UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -32 ), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, & tr);

	if( tr.startsolid )
	{
		// Since we blindly moved the explosion origin vertically, we may have inadvertently moved the explosion into a solid,
		// in which case nothing is going to be harmed by the grenade's explosion because all subsequent traces will startsolid.
		// If this is the case, we do the downward trace again from the actual origin of the grenade. (sjb) 3/8/2007  (for ep2_outland_09)
		UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + Vector( 0, 0, -32), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr );
	}

#if !defined( CLIENT_DLL )
	if (GetForceEffect() != FE_PROJECTILE)
	{
		CWeaponMagic::Explode( GetThrower(), this, NULL, tr.endpos, GetDamage(), GetDamageRadius(),
			GetElement(),
			GetStatusEffect(),
			GetStatusMagnitude(),
			GetForceEffect(),
			GetNumen(),
			GetHealthDrainage(),
			GetStaminaDrainage(),
			GetFocusDrainage());
		CSoundEnt::InsertSound ( SOUND_COMBAT, GetAbsOrigin(), BASEGRENADE_EXPLOSION_VOLUME, 3.0 );
	}
#endif

	RemoveMe();
}

void CMagicGrenade::RemoveMe()
{
	SetModelName( NULL_STRING );//invisible
	AddSolidFlags( FSOLID_NOT_SOLID );

	m_takedamage = DAMAGE_NO;

	SetThink( &CBaseGrenade::SUB_Remove );
	SetTouch( NULL );
	SetSolid( SOLID_NONE );
	
	AddEffects( EF_NODRAW );
	SetAbsVelocity( vec3_origin );

	SetNextThink( gpGlobals->curtime + 0.1 );

#if defined( CLIENT_DLL )
	for (int i = 0; i < m_apEffects.Count(); i++)
		ParticleProp()->StopEmissionAndDestroyImmediately(m_apEffects[i]);
	m_apEffects.RemoveAll();
#endif
}

int CMagicGrenade::UpdateTransmitState( void)
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}
#endif

#ifdef CLIENT_DLL
void CMagicGrenade::OnDataChanged( DataUpdateType_t updateType )
{
	if (updateType == DATA_UPDATE_CREATED)
	{
		for (int i = 0; i < sizeof(element_t)*4; i++)
		{
			if (!((1<<i) & GetElement()))
				continue;

			m_apEffects.AddToTail(ParticleProp()->Create( VarArgs("numen_%s_projectile", ElementToString((element_t)(1<<i))), PATTACH_ABSORIGIN_FOLLOW ));
		}

		// Deploy typeless bases.
		for (int i = 0; i < sizeof(GetNumen())*4; i++)
		{
			CRuneData* pData;

			if (!((1<<i) & GetNumen()))
				continue;

			pData = CRuneData::GetData(i);

			if (pData->m_eType == RUNETYPE_BASE && pData->m_eElement == ELEMENT_TYPELESS)
				m_apEffects.AddToTail(ParticleProp()->Create( VarArgs("numen_%s_projectile", pData->m_szName), PATTACH_ABSORIGIN_FOLLOW ));
		}
	}
}
#endif

void CMagicGrenade::SetNumen( long iNumen )
{
	for (long i = 0; i < CRuneData::TotalRunes(); i++)
	{
		m_iNumen.Set(i, !!(iNumen&(1<<i)));
	}
}

long CMagicGrenade::GetNumen( )
{
	long iNumen = 0;

	for (long i = 0; i < CRuneData::TotalRunes(); i++)
		iNumen |= m_iNumen.Get(i)<<i;

	return iNumen;
}
