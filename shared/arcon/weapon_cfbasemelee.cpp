#include "cbase.h"
#include "weapon_cfbasemelee.h"
#include "in_buttons.h"
#include "statistics.h"
#include "weapon_magic.h"
#include "takedamageinfo.h"
#include "cf_gamerules.h"

#if defined( CLIENT_DLL )
#include "c_cf_player.h"
#include "cf_in_main.h"
#else
#include "cf_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED( CFBaseMeleeWeapon, DT_CFBaseMeleeWeapon )

BEGIN_NETWORK_TABLE( CCFBaseMeleeWeapon, DT_CFBaseMeleeWeapon )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iAttack ) ),
	RecvPropBool( RECVINFO( m_bStrongAttack ) ),
	RecvPropTime( RECVINFO( m_flLastAttackTime ) ),
	RecvPropTime( RECVINFO( m_flChargeStartTime ) ),
	RecvPropTime( RECVINFO( m_flSwingTime ) ),
	RecvPropBool( RECVINFO( m_bCharging ) ),
	RecvPropBool( RECVINFO( m_bAfflictive ) ),
	RecvPropInt( RECVINFO( m_eAttackElement ) ),
#else
	SendPropInt( SENDINFO( m_iAttack ), 4 ),
	SendPropBool( SENDINFO( m_bStrongAttack ) ),
	SendPropTime( SENDINFO( m_flLastAttackTime ) ),
	SendPropTime( SENDINFO( m_flChargeStartTime ) ),
	SendPropTime( SENDINFO( m_flSwingTime ) ),
	SendPropBool( SENDINFO( m_bCharging ) ),
	SendPropBool( SENDINFO( m_bAfflictive ) ),
	SendPropInt( SENDINFO( m_eAttackElement ), TOTAL_ELEMENTS ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CCFBaseMeleeWeapon )
	DEFINE_PRED_FIELD( m_iAttack, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bStrongAttack, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flLastAttackTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flChargeStartTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flSwingTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bCharging, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bAfflictive, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_eAttackElement, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

#define MAX_MELEE_ATTACKS 3
#define MELEE_FREEZE_MULT 0.4f

#ifdef _DEBUG
#define SHOW_MELEE_TRACES "1"
#else
#define SHOW_MELEE_TRACES "0"
#endif

static ConVar mp_chargetime( "mp_chargetime", "1", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
static ConVar mp_maxairattacks( "mp_maxairattacks", "3", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
static ConVar mp_attackstamina( "mp_attackstamina", "15", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
static ConVar mp_strongattackstamina( "mp_strongattackstamina", "30", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
static ConVar sv_showmeleetraces( "sv_showmeleetraces", SHOW_MELEE_TRACES, FCVAR_REPLICATED | FCVAR_CHEAT );
static ConVar mp_meleechaintime( "mp_meleechaintime", "2", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
static ConVar mp_downstrikemultiplier( "mp_downstrikemultiplier", "2", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
static ConVar mp_downstrikefreeze( "mp_downstrikefreeze", "1", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );

// Constructor
//-----------------------------------------------------------------------------
CCFBaseMeleeWeapon::CCFBaseMeleeWeapon()
{
	m_bFiresUnderwater = true;
}

//-----------------------------------------------------------------------------
// Purpose: Spawn the weapon
//-----------------------------------------------------------------------------
void CCFBaseMeleeWeapon::Spawn( void )
{
	m_iAttack = -1;
	m_bStrongAttack = false;
#ifdef CLIENT_DLL
	m_bOldCharging = false;
	m_bOldAfflictive = false;
#endif
	m_bAfflictive = false;
	m_bCharging = false;
	m_flLastAttackTime = 0;
	m_flSwingTime = 0;

	m_iTipAttachment = m_iHiltAttachment = -1;

	//Call base class first
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Precache the weapon
//-----------------------------------------------------------------------------
void CCFBaseMeleeWeapon::Precache( void )
{
	PrecacheScriptSound( "Weapon.Critical" );
	PrecacheScriptSound( "Weapon.SwordHit" );
	PrecacheScriptSound( "Weapon.SwordClash" );

	//Call base class first
	BaseClass::Precache();
}

bool CCFBaseMeleeWeapon::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	StopMeleeAttack(false);
	return BaseClass::Holster(pSwitchingTo);
}

void CCFBaseMeleeWeapon::Drop( const Vector &vecVelocity )
{
	StopMeleeAttack(false);
	BaseClass::Drop(vecVelocity);
}

int CCFBaseMeleeWeapon::GetAttackFromMovement()
{
	if (!GetOwner())
		return 2;		// Horizontal

	if (ToCFPlayer(GetOwner())->m_nButtons & IN_FORWARD)
		return 1;		// Diagonal

	if (ToCFPlayer(GetOwner())->m_nButtons & IN_BACK)
		return 0;		// Vertical

	return 2;
}

//------------------------------------------------------------------------------
// Purpose : Update weapon
//------------------------------------------------------------------------------
void CCFBaseMeleeWeapon::ItemPostFrame( void )
{
	// No call to superclass, we don't want that behavior.

	CCFPlayer *pOwner = ToCFPlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	bool bHasDualMelee = pOwner->HasDualMelee();
	CCFBaseMeleeWeapon* pMeleeSecondary = NULL;
	if (bHasDualMelee)
		pMeleeSecondary = (CCFBaseMeleeWeapon*)pOwner->GetSecondaryWeapon();

	// If this is the secondary and we have two melee weapons, let the primary think for us.
	if (!IsPrimary() && bHasDualMelee)
	{
		if (m_bAfflictive)
			Afflict();

		return;
	}

	// Reset 
	if (!pOwner->PlayerFrozen(mp_meleechaintime.GetFloat()) && gpGlobals->curtime > m_flLastAttackTime + mp_meleechaintime.GetFloat() && !IsCharging(false))
		pOwner->m_iMeleeChain = 0;

	if ((m_bAfflictive || bHasDualMelee && pMeleeSecondary->m_bAfflictive) && gpGlobals->curtime > m_flLastAttackTime + GetFireRate() && !pOwner->m_bDownStrike)
		StopMeleeAttack(true, false);

	if (m_bAfflictive)
		Afflict();

	if (pOwner->m_bDownStrike)
		return;

	// Bail if the player lost the sword mid-swing.
	if (!GetOwner())
		return;

	if ( gpGlobals->curtime < pOwner->m_flNextAttack )
		return;

	if (pOwner->m_hReviving != NULL)
		return;

	if (m_flSwingTime && m_flSwingTime < gpGlobals->curtime)
		Swing();

	// Bail if the player lost the sword mid-swing.
	if (!GetOwner())
		return;

	if ( pOwner->IsPhysicalMode() && (FiringButtons() & AttackButtons()) && (m_flNextPrimaryAttack <= gpGlobals->curtime) )
	{
		CalculateStrongAttack();

		PrimaryAttack();
	}
	else if ( pOwner->IsPhysicalMode() && IsCharging(false) && !(pOwner->m_nButtons & AttackButtons()) && (m_flNextPrimaryAttack <= gpGlobals->curtime) )
	{
		PrimaryAttack();
	}
	else if ( IsCharging() && pOwner->IsMagicMode() && (m_flNextPrimaryAttack <= gpGlobals->curtime) )
	{
		PrimaryAttack();
	}
	else if ( pOwner->IsPhysicalMode()
		&& !IsCharging(false) && (pOwner->m_nButtons & AttackButtons())
		&& (m_flNextPrimaryAttack <= gpGlobals->curtime)
		&& pOwner->m_flRushDistance <= 0 && m_flSwingTime <= 0 )
	{
		StartCharge(GetAttackFromMovement());
	}
	else 
	{
		WeaponIdle();
		return;
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CCFBaseMeleeWeapon::PrimaryAttack()
{
	CCFPlayer *pCFOwner = ToCFPlayer( GetOwner() );

	bool bAllowDownStrike = false;
	bool bPredictSwing = false;

	Vector vecForward, vecTarget;
	if (pCFOwner->GetRecursedTarget())
	{
		vecTarget = pCFOwner->GetRecursedTarget()->GetAbsOrigin() - pCFOwner->GetAbsOrigin();
		vecTarget.z = 0;
		vecTarget.NormalizeInPlace();
		pCFOwner->GetVectors(&vecForward, NULL, NULL);
		vecForward.z = 0;
		vecForward.NormalizeInPlace();
	}

	if (pCFOwner->AutoFollowMode() && pCFOwner->GetRecursedTarget() &&
		CFGameRules()->PlayerRelationship(pCFOwner->GetRecursedTarget(), pCFOwner) == GR_NOTTEAMMATE &&
		DotProduct(vecForward, vecTarget) > 0.9f)
	{
#ifdef GAME_DLL
		pCFOwner->StartAutoFollowMode();
#endif
		bPredictSwing = true;
		bAllowDownStrike = true;
	}

	if (pCFOwner->CanDownStrike(bAllowDownStrike))
	{
		m_iAttack = 0;

		Rush(true);
		return;
	}

	if (bPredictSwing || pCFOwner->IsInFollowMode())
	{
		Rush();
	}
	else
	{
		Swing();
	}
}

// YYZ
void CCFBaseMeleeWeapon::Rush(bool bDownStrike)
{
	CCFPlayer* pPlayer = ToCFPlayer(GetOwner());

	// This is actually wrong, you can never have too much Rush.
	if (pPlayer->m_flRushDistance > 0 || m_flSwingTime > 0)
		return;

	if (bDownStrike)
		pPlayer->Rush(this, 80, m_iAttack);
	else
	{
		if (IsStrongAttack())
			pPlayer->Rush(this, 200, m_iAttack);
		else
			pPlayer->Rush(this, 200, m_iAttack);
	}

	pPlayer->m_bDownStrike = bDownStrike;

	if (bDownStrike)
	{
		m_bAfflictive = true;
		m_flSwingTime = 0;
		Afflict(true);
	}
}

ConVar cf_dfa_damage("cf_dfa_damage", "200", FCVAR_DEVELOPMENTONLY);

void CCFBaseMeleeWeapon::EndRush()
{
#ifdef GAME_DLL
	if (!GetOwner())
		return;

	CCFPlayer* pOwner = ToCFPlayer(GetOwner());

	if (pOwner->m_bDownStrike)
	{
		CTakeDamageInfo info;
		info.CFSet( pOwner, pOwner, cf_dfa_damage.GetFloat(), DMG_SLASH, GetWeaponID(), true,
			pOwner->m_pStats->GetPhysicalAttackElement(GetPosition()),
			pOwner->m_pStats->GetPhysicalAttackStatusEffect(GetPosition()),
			pOwner->m_pStats->GetPhysicalAttackStatusEffectMagnitude(GetPosition())
			);

		RadiusDamage(info, pOwner->GetCentroid(), cf_dfa_damage.GetFloat(), CLASS_NONE, pOwner);

		EmitSound("Weapon.SwordHit");
	}
#endif

	StopMeleeAttack(true, false);
}

//------------------------------------------------------------------------------
// Purpose : Starts the swing of the weapon and determines the animation
// Input   : bIsSecondary - is this a secondary attack?
//------------------------------------------------------------------------------
void CCFBaseMeleeWeapon::Swing()
{
	m_iAttack = GetAttackFromMovement();

	m_flSwingTime = 0;

	// Try a ray
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
		return;

	m_bHitThisSwing = false;

	CCFPlayer *pCFOwner = ToCFPlayer( pOwner );

#ifdef GAME_DLL
	// Don't pick up enemies if we can do a DFA... we might change our DFA target.
	// This might very rarely cause a false positive but I very rarely care.
	if (!pCFOwner->CanDownStrike(true) && !pCFOwner->IsInFollowMode() || CFGameRules()->PlayerRelationship(pCFOwner, pCFOwner->GetRecursedTarget()) == GR_TEAMMATE)
	{
		CCFPlayer* pClosestEnemy = pCFOwner->FindClosestEnemy();
		pCFOwner->SuggestRecursedTarget(pClosestEnemy);

		// Well I tried to pick up this guy I want to attack but for some reason I can't.
		// Maybe I already have another enemy. Screw that though I want this one! If I don't
		// have him by now then MAKE me have him, because I'm swinging at him as hard as
		// I can.
		if (pClosestEnemy && pCFOwner->GetRecursedTarget() != pClosestEnemy)
			pCFOwner->SetDirectTarget(pClosestEnemy);
	}
#endif

	if (pCFOwner->GetGroundEntity())
	{
		pCFOwner->m_iAirMeleeAttacks = 0;
	}
	else
	{
		// Allow down strikes even if the player is out of air attacks.
		if (!pCFOwner->m_bDownStrike)
		{
			if (++pCFOwner->m_iAirMeleeAttacks > mp_maxairattacks.GetInt())
				return;
		}
	}

	if (IsCharging(false))
		// Only get the multiplier if the player shows actual intent that he wanted to charge.
		// Otherwise spamming the attack button can set this off.
		StopCharge(IsCharging(), 1.0f);
	else
		ResetCharge();

	float flChargeAttackStamina;
	if (IsStrongAttack())
		flChargeAttackStamina = mp_strongattackstamina.GetFloat() * GetChargeMultiplier();
	else
		flChargeAttackStamina = mp_attackstamina.GetFloat() * GetChargeMultiplier();

	if (pCFOwner->m_pStats->GetStamina() <= 0)
	{
		if (GetChargeAmount())
		{
			StopMeleeAttack(true, false);
			return;
		}
		else
			return;
	}

	m_bAfflictive = true;

	// For the client to use to draw particle effects later.
	m_eAttackElement = pCFOwner->m_pStats->GetPhysicalAttackElement(GetPosition());

	if (pCFOwner->HasDualMelee())
	{
		CCFBaseMeleeWeapon* pMeleeSecondary = (CCFBaseMeleeWeapon*)pCFOwner->GetSecondaryWeapon();
		pMeleeSecondary->m_iAttack = m_iAttack;
		pMeleeSecondary->m_bAfflictive = true;
		pMeleeSecondary->m_eAttackElement = pCFOwner->m_pStats->GetPhysicalAttackElement(pMeleeSecondary->GetPosition());
	}

	m_flLastAttackTime = gpGlobals->curtime;

	WeaponSound( SINGLE );

	if (GetChargeAmount() || IsStrongAttack() || pCFOwner->m_bDownStrike)
		pOwner->EmitSound("Player.ChargeYell");

	if (!ToCFPlayer(pOwner)->m_bDownStrike)
	{
		if (GetChargeAmount())
			pCFOwner->DoAnimationEvent( PLAYERANIMEVENT_CHARGEATTACK, m_iAttack, pCFOwner->GetSecondaryWeapon() == this );
		else if (IsStrongAttack())
			pCFOwner->DoAnimationEvent( PLAYERANIMEVENT_STRONGATTACK, m_iAttack, pCFOwner->GetSecondaryWeapon() == this );
		else
			pCFOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK, m_iAttack, pCFOwner->GetSecondaryWeapon() == this );
	}

	//Setup our next attack times
	CWeaponMagic* pMagic = pCFOwner->GetMagicWeapon();
	CWeaponCFBase* pPrimary = pCFOwner->GetPrimaryWeapon();
	CWeaponCFBase* pSecondary = pCFOwner->GetSecondaryWeapon();

	pCFOwner->m_flNextAttack = pMagic->m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	if (pPrimary)
		pPrimary->DisableForSeconds(GetFireRate());
	if (pSecondary)
		pSecondary->DisableForSeconds(GetFireRate());

	float flFreezeTime = GetFireRate();
	pCFOwner->FreezePlayer(pCFOwner->IsInFollowMode()?MELEE_FREEZE_MULT/2:MELEE_FREEZE_MULT, flFreezeTime);

	float flSuspendTime = GetFireRate() + 0.25f;
	pCFOwner->SuspendGravity(flSuspendTime);

	Vector vecNewVelocity = pCFOwner->IsInFollowMode()?Vector(0,0,0):pCFOwner->GetAbsVelocity()/2;
	if (vecNewVelocity.z < 0)
		vecNewVelocity.z = 0;

	pCFOwner->SetAbsVelocity(vecNewVelocity);

	Afflict(true);

	if (pCFOwner->HasDualMelee())
		((CCFBaseMeleeWeapon*)pSecondary)->Afflict(true);

	// Bail if the player lost the sword mid-swing.
	if (!GetOwner())
		return;

#ifdef GAME_DLL
	if (pCFOwner->m_pStats->GetStamina() > 0)
		pCFOwner->m_pStats->m_flStamina -= flChargeAttackStamina;
#endif
}

void CCFBaseMeleeWeapon::Afflict(bool bFirst)
{
	if (IsClient())
		return;

	if (bFirst)
	{
		if (m_iTipAttachment == -1)
		{
			m_iTipAttachment = LookupAttachment("tip");
			m_iHiltAttachment = LookupAttachment("hilt");
		}

		m_aHitList.RemoveAll();
		m_iLastFrame = 0;
	}

	// Never simulate a frame more than once.
	if (m_iLastFrame >= gpGlobals->framecount)
		return;

	GetOwner()->InvalidateBoneCache();
	InvalidateBoneCache();

	Vector vecTip, vecHilt;
	GetAttachment(m_iTipAttachment, vecTip);
	GetAttachment(m_iHiltAttachment, vecHilt);

	Trace(vecHilt, vecTip, Color(0,255,0));
	if (!bFirst)
	{

		Trace(m_vecLastHilt, vecHilt, Color(0,0,255));
		Trace(m_vecLastTip, vecTip, Color(0,0,255));
		Trace(m_vecLastTip, vecHilt, Color(0,255,0));
		Trace(m_vecLastHilt, vecTip, Color(0,255,0));
	}

	m_iLastFrame = gpGlobals->framecount;
	m_vecLastTip = vecTip;
	m_vecLastHilt = vecHilt;
}

bool CCFBaseMeleeWeapon::Trace( Vector vecStart, Vector vecEnd, Color d )
{
	trace_t traceHit;

	UTIL_TraceLine( vecStart, vecEnd, MASK_SHOT_HULL, GetOwner(), COLLISION_GROUP_NONE, &traceHit );

	if (sv_showmeleetraces.GetBool())
	{
		if (traceHit.fraction == 1)
			DebugDrawLine( traceHit.startpos, traceHit.endpos, d.r(), d.g(), d.b(), true, 0.5f );
		else
			DebugDrawLine( traceHit.startpos, traceHit.endpos, 255, 0, 0, true, 0.5f );
	}

	if ( traceHit.fraction != 1.0f )
	{
		Hit( traceHit );
		return true;
	}

	return false;
}

//------------------------------------------------------------------------------
// Purpose: Implement impact function
//------------------------------------------------------------------------------
void CCFBaseMeleeWeapon::Hit( trace_t &traceHit )
{
	// Bail if the player lost the sword mid-swing.
	if (!GetOwner())
		return;

	CCFPlayer *pPlayer = ToCFPlayer( GetOwner() );
	
	CBaseEntity	*pHitEntity = traceHit.m_pEnt;

	if ( !pHitEntity )
		return;
	
	if (m_aHitList.IsValidIndex(m_aHitList.Find(pHitEntity->GetRefEHandle())))
		return;

	m_aHitList.AddToHead(pHitEntity->GetRefEHandle());

	Vector vecHitDirection = pHitEntity->WorldSpaceCenter() - pPlayer->WorldSpaceCenter();

	Vector vecHitForward;
	pHitEntity->GetVectors( &vecHitForward, NULL, NULL );
	VectorNormalize( vecHitForward );

#ifndef CLIENT_DLL
	// Turn off suppression for this message, because the client does not run this code.
	CDisablePredictionFiltering disabler;

	// In case StopMeleeAttack() gets called.
	bool bDownStrike = pPlayer->m_bDownStrike;
	int iAttack = m_iAttack;

	int iDamage = pPlayer->m_pStats->GetPhysicalAttackDamage(GetPosition(), IsStrongAttack()?GetCFWpnData().m_iStrongDamage:GetCFWpnData().m_iDamage);

	iDamage *= GetChargeMultiplier();

	CTakeDamageInfo info;
	info.CFSet( GetOwner(), GetOwner(), iDamage, DMG_SLASH, GetWeaponID(), true,
		pPlayer->m_pStats->GetPhysicalAttackElement(GetPosition()),
		pPlayer->m_pStats->GetPhysicalAttackStatusEffect(GetPosition()),
		pPlayer->m_pStats->GetPhysicalAttackStatusEffectMagnitude(GetPosition())
		);

	if( pPlayer && pHitEntity->IsNPC() )
	{
		// If bonking an NPC, adjust damage.
		info.AdjustPlayerDamageInflictedForSkillLevel();
	}

	if (bDownStrike)
		info.SetDamage(info.GetDamage()*mp_downstrikemultiplier.GetFloat());

	if (pHitEntity->IsPlayer())
	{
		if (IsStrongAttack())
			pPlayer->Instructor_LessonLearned(HINT_RMB_STRONG_ATTACK);

		CCFPlayer* pVictim = ToCFPlayer(pHitEntity);
		CWeaponCFBase* pVictimWeapon = pVictim->GetPrimaryWeapon();
		CCFBaseMeleeWeapon* pVictimMeleeWeapon = NULL;
		if (pVictimWeapon && pVictimWeapon->IsMeleeWeapon())
			pVictimMeleeWeapon = dynamic_cast<CCFBaseMeleeWeapon*>(pVictimWeapon);

		// Will the enemy just end up absorbing this damage? Good to know now so we don't do criticals or play painful sounds.
		bool bWillAbsorb = (pVictim->m_pStats->GetElementDefenseScale(info.GetElements()) <= 0);

		bool bBlocked = false;
		bool bVictimRiposte = false;
		if (pVictimMeleeWeapon && DotProduct(vecHitDirection, vecHitForward) < 0)
		{
			if (m_bStrongAttack)
			{
				// If the enemy is also swinging a strong attack, we have a block.
				if (pVictimMeleeWeapon->m_bAfflictive && pVictimMeleeWeapon->m_bStrongAttack)
					bBlocked = true;
				// Strong attacks cut through blocking.
				else if (pVictim->IsBlocking())
					bBlocked = false;
			}
			else
			{
				if (pVictim->IsBlocking())
					bVictimRiposte = bBlocked = true;
				// If the enemy is also swinging a weak attack, we have a block.
				else if (pVictimMeleeWeapon->m_bAfflictive && !pVictimMeleeWeapon->m_bStrongAttack)
					bBlocked = true;
			}
		}

		float flRandom = SharedRandomFloat("critical", 0, 1);
		if (bBlocked)
		{
			info.SetDamage(0);
			pHitEntity->EmitSound( "Weapon.SwordClash" );
		}
		else if (!bWillAbsorb && CFGameRules()->CanReceiveCritical(pVictim, pPlayer) && flRandom < pPlayer->m_pStats->GetCriticalRate() * GetChargeMultiplier() * GetChainMultiplier())
		{
			info.SetDamage(info.GetDamage()*2);
			pHitEntity->EmitSound( "Weapon.Critical" );

			DispatchParticleEffect( "critical", pVictim->GetAbsOrigin(), pVictim->GetAbsAngles(), pHitEntity );

			CSingleUserRecipientFilter user( pPlayer );
			user.MakeReliable();
			UserMessageBegin( user, "CFMessage" );
				WRITE_BYTE( MESSAGE_CRITICAL );
			MessageEnd();
		}
		else
		{
			if (!bWillAbsorb)
				pHitEntity->EmitSound( "Weapon.SwordHit" );
		}

		bool bBlockedSecondary = true;
		CWeaponCFBase* pDefendingWeapon = NULL;

		if (CFGameRules()->CanFreezeVictim(pVictim, pPlayer))
		{
			// In most situations, only freeze the victim long enough to make the next shot a fair game fight.
			float flFreezeTime = pPlayer->m_flFreezeUntil - gpGlobals->curtime + 0.2f;

			if (bDownStrike)
				// Fuck these guys up good.
				flFreezeTime += mp_downstrikefreeze.GetFloat();

			pVictim->SetAbsVelocity(Vector(0,0,0));

			if (pVictimMeleeWeapon)
				pVictimMeleeWeapon->StopMeleeAttack(true, false);

			// Need to get rid of full body animation so velocity changes affect the player.
			pVictim->DoAnimationEvent( PLAYERANIMEVENT_ATTACK, ACT_INVALID );

			if (bDownStrike)
				pVictim->FreezePlayer(0.2f, mp_downstrikefreeze.GetFloat());
			else
				pVictim->FreezePlayer(0.2f, flFreezeTime);

			if (pVictim->GetDefendingWeapon())
			{
				pDefendingWeapon = pVictim->GetDefendingWeapon();
				if (pDefendingWeapon->IsPrimary())
					bBlockedSecondary = false;
				pDefendingWeapon->DisableForSeconds(flFreezeTime, true);
			}

			pPlayer->SetAbsVelocity(Vector(0,0,0));

			if (!bDownStrike)
			{
				StopMeleeAttack(true, false);

				// Need to get rid of full body animation so velocity changes affect the player.
				pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK, ACT_INVALID );

				pPlayer->FreezePlayer(0.2f, flFreezeTime);

				DisableForSeconds(flFreezeTime, true);
			}
		}

		if (bVictimRiposte)
		{
			float flFreezeTime = pPlayer->m_flFreezeUntil - gpGlobals->curtime;

			// I got parried! Freeze me long enough for my enemy to cut in.
			flFreezeTime += pVictimMeleeWeapon->GetFireRate();

			pPlayer->FreezePlayer(0.2f, flFreezeTime);
			DisableForSeconds(flFreezeTime, true);

			// Unfreeze victim so he can strike back immediately.
			pVictim->FreezePlayer(1);
			pVictimMeleeWeapon->DisableForSeconds(0, true);
			pVictimMeleeWeapon->m_flNextPrimaryAttack = 0;
			pVictimMeleeWeapon->m_flNextSecondaryAttack = 0;
			pVictimMeleeWeapon->m_flDisabledUntil = 0;
		}

		if (bBlocked)
		{
			pVictim->DoAnimationEvent( PLAYERANIMEVENT_BLOCK, 0, bBlockedSecondary );
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_BLOCKED, 0, !IsPrimary() );

			if (pVictim->m_nButtons&IN_BACK)
				pVictim->Instructor_LessonLearned( HINT_S_BLOCK );

			if (pDefendingWeapon)
				pDefendingWeapon->ShowBlock();
		}
		else
		{
			// Only flinch if we are holding a melee weapon. Otherwise, flinching is pointless because the player still can shoot.
			CWeaponCFBase* pPrimary = pVictim->GetPrimaryWeapon();
			CWeaponCFBase* pSecondary = pVictim->GetSecondaryWeapon();
			if ((pPrimary && pPrimary->IsMeleeWeapon()) || (pSecondary && pSecondary->IsMeleeWeapon()))
				pVictim->DoAnimationEvent( PLAYERANIMEVENT_FLINCH_CHEST );
		}

		// If you are hit then you can return the favor
		pVictim->m_iAirMeleeAttacks = 0;
 
		// But you can't keep going forever
		//pPlayer->m_iAirMeleeAttacks = 0;

		if (CFGameRules()->CanFreezeVictim(pVictim, pPlayer))
			// Improve damage by 10% for each hit past the first.
			info.SetDamage(info.GetDamage() * GetChainMultiplier());

		if (!CFGameRules()->CanFreezeVictim(pVictim, pPlayer) || bBlocked)
			info.RemoveStatusEffects(~STATUSEFFECT_NONE);

		m_bHitThisSwing = true;
		if (!bBlocked && CFGameRules()->FPlayerCanTakeDamage(pVictim, pPlayer, info))
		{
			pPlayer->m_iMeleeChain++;

			CSingleUserRecipientFilter user( pPlayer );
			user.MakeReliable();
			UserMessageBegin( user, "CFMessage" );
				WRITE_BYTE( MESSAGE_MULTIPLIER );
				WRITE_SHORT( pPlayer->m_iMeleeChain );
			MessageEnd();
		}

		if (CFGameRules()->CanFreezeVictim(pVictim, pPlayer))
		{
			Vector vecPlayerForce = Vector(0,0,0);
			Vector vecVictimForce = Vector(0,0,0);
			float flPowerMultiplier = GetFireRate();
			if (bDownStrike)
			{
				pVictim->SuspendGravity(mp_downstrikefreeze.GetFloat());
			}
/*			else if (bBlocked)
			{
				// Attack was blocked, both players get pushed up a bit.
				vecVictimForce.z = 150;	// 150 is about the min force needed to separate from the ground.

				vecPlayerForce.z = 150;	// 150 is about the min force needed to separate from the ground.

				pVictim->SuspendGravity(GetFireRate()*0.5f);
				pPlayer->SuspendGravity(GetFireRate()*0.5f);
			}*/
			else if (iAttack == 0)
			{
				// Down attack
				vecVictimForce = Vector(0, 0, -1);
				vecVictimForce *= 300;
			}
/*			else if (!IsStrongAttack())
			{
				// Victims of weak attacks get a slight push.
				vecVictimForce = pHitEntity->GetAbsOrigin() - pPlayer->GetAbsOrigin();
				vecVictimForce.z = 0;
				vecVictimForce.NormalizeInPlace();
				vecVictimForce *= 50;
				vecVictimForce.z = 200;

				vecPlayerForce.z = 150;

				pVictim->SuspendGravity(GetFireRate()*0.5f);
				pPlayer->SuspendGravity(GetFireRate()*0.5f);
			}
			else if (bVictimRiposte)
			{
				// Only do a little push, give the victim the chance to get his own strike in now!
				vecVictimForce = pHitEntity->GetAbsOrigin() - pPlayer->GetAbsOrigin();
				vecVictimForce.z = 0;
				vecVictimForce.NormalizeInPlace();
				vecVictimForce *= 105;
				vecVictimForce.z = 150;

				vecPlayerForce = pPlayer->GetAbsOrigin() - pHitEntity->GetAbsOrigin();
				vecPlayerForce.z = 0;
				vecPlayerForce.NormalizeInPlace();
				vecPlayerForce *= 105;
				vecPlayerForce.z = 150;	// 150 is about the min force needed to separate from the ground.

				pVictim->SuspendGravity(GetFireRate()*0.5f);
				pPlayer->SuspendGravity(GetFireRate());
			}*/
			else if (IsStrongAttack())
			{
				// Move the player up 250 and out enough space for a player or two to fit,
				// to allow powerjump-dashing up to the victim.
				Vector vecDirection = pHitEntity->GetAbsOrigin() - pPlayer->GetAbsOrigin();
				vecDirection.z = 0;
				vecDirection.NormalizeInPlace();
				vecDirection *= cvar->FindVar("sv_collisionradiusfollow")->GetFloat() * 2;
				vecVictimForce = Vector(0, 0, 1);
				vecVictimForce *= 350;
				vecVictimForce += vecDirection;

//				vecPlayerForce.z = 150;	// 150 is about the min force needed to separate from the ground.

				pVictim->SuspendGravity(0.33f * flPowerMultiplier, 0.33f * flPowerMultiplier);
				pPlayer->SuspendGravity(GetFireRate()*0.5f);

				pPlayer->m_flStrongAttackJumpTime = gpGlobals->curtime + 1.5f;
			}

			pHitEntity->SetGroundEntity( NULL );
			pPlayer->SetGroundEntity( NULL );

			pVictim->ApplyAbsVelocityImpulse( vecVictimForce );
			pPlayer->ApplyAbsVelocityImpulse( vecPlayerForce );
		}

		if (pVictim->GetPrimaryWeapon() && !pVictim->GetPrimaryWeapon()->IsMeleeWeapon() && CFGameRules()->FPlayerCanTakeDamage(pVictim, pPlayer, info))
			pVictim->SetPunchAngle( QAngle(80 * random->RandomFloat(-1, 1), 80 * random->RandomFloat(-1, 1), 30 * random->RandomFloat(-1, 1)) );
	}

	if ((pHitEntity->IsPlayer() || pHitEntity->IsNPC()) && pHitEntity->IsAlive())
	{
		// Any player or NPC can allow the player to powerjump again after being hit.
		pPlayer->StopPowerjump();
		pPlayer->m_Local.m_flLastJump = gpGlobals->curtime;
	}

	CalculateMeleeDamageForce( &info, vecHitDirection, traceHit.endpos );

	pHitEntity->DispatchTraceAttack( info, vecHitDirection, &traceHit ); 
	ApplyMultiDamage();

	// Now hit all triggers along the ray that... 
	TraceAttackToTriggers( info, traceHit.startpos, traceHit.endpos, vecHitDirection );
#endif
}

void CCFBaseMeleeWeapon::StopMeleeAttack(bool bDualAware, bool bInterruptChain)
{
	m_bAfflictive = false;
	StopCharge(false);
	m_iAttack = -1;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
		return;

	CCFPlayer* pCFOwner = ToCFPlayer(pOwner);

	if (bDualAware && pCFOwner->HasDualMelee())
	{
		((CCFBaseMeleeWeapon*)pCFOwner->GetSecondaryWeapon())->m_bAfflictive = false;
	}

	pCFOwner->m_bDownStrike = false;

	if (!m_bHitThisSwing || bInterruptChain)
		pCFOwner->m_iMeleeChain = 0;

	if (bInterruptChain)
	{
		// Reset the attack animation gesture slot to stop whatever charge/attack/whatever animation we are currently doing.
		pCFOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK, ACT_INVALID );
	}
}

void CCFBaseMeleeWeapon::SetSwingTime(float flTime)
{
	m_flSwingTime = flTime;
}

void CCFBaseMeleeWeapon::CalculateStrongAttack()
{
	if (FiringButtons() & PrimaryButtons())
		m_bStrongAttack = false;
	else if (FiringButtons() & SecondaryButtons())
		m_bStrongAttack = true;

	if (GetOwner() && ToCFPlayer(GetOwner())->HasDualMelee())
		((CCFBaseMeleeWeapon*)ToCFPlayer(GetOwner())->GetSecondaryWeapon())->m_bStrongAttack = m_bStrongAttack;
}

ConVar cf_bulletblockcost("cf_bulletblockcost", "4", FCVAR_DEVELOPMENTONLY);
void CCFBaseMeleeWeapon::BulletBlocked(int iDamageType)
{
	if (!IsCharging())
	{
		ToCFPlayer(GetOwner())->DoAnimationEvent( PLAYERANIMEVENT_BLOCK );
		ShowBlock();
	}
	EmitSound( "Weapon.SwordClash" );

	float flBulletBlockCost = cf_bulletblockcost.GetFloat();
	if (iDamageType & DMG_BUCKSHOT)
		flBulletBlockCost /= 8;

	ToCFPlayer(GetOwner())->m_pStats->m_flStamina -= flBulletBlockCost;
}

void CCFBaseMeleeWeapon::ShowBlock()
{
	Vector vec1, vec2;
	QAngle angRandom;
	float flRandom = 0;
#ifdef GAME_DLL
	if (GetOwner())
		flRandom = ToCFPlayer(GetOwner())->m_Randomness.RandomFloat(0, 1);
#endif

	// Make sure it grabs the proper animation to place the block sparks appropriately.
	GetOwner()->InvalidateBoneCache();

	GetAttachment("hilt", vec1, angRandom);
	GetAttachment("tip", vec2, angRandom);
	Vector vecMiddle = vec1*flRandom + vec2*(1-flRandom);
	DispatchParticleEffect("block", vecMiddle, angRandom);
}

bool CCFBaseMeleeWeapon::IsStrongAttack()
{
	return m_bStrongAttack;
}

void CCFBaseMeleeWeapon::StartCharge(int iAttack)
{
	// Disable for now.
	return;

	if (m_bCharging)
		return;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
		return;

	if (!pOwner->GetGroundEntity())
	{
		if (ToCFPlayer(pOwner)->m_iAirMeleeAttacks >= mp_maxairattacks.GetInt())
			return;
	}

	// If we don't have the juice, don't even bother trying to charge it up.
	if (ToCFPlayer(pOwner)->m_pStats->GetStamina() <= 0)
		return;

	m_iAttack = iAttack;

	ToCFPlayer(pOwner)->DoAnimationEvent( PLAYERANIMEVENT_CHARGE, iAttack, ToCFPlayer(pOwner)->GetSecondaryWeapon() == this );
	m_flChargeStartTime = gpGlobals->curtime;
	m_bCharging = true;

	if (ToCFPlayer(pOwner)->HasDualMelee())
	{
		((CCFBaseMeleeWeapon*)ToCFPlayer(pOwner)->GetSecondaryWeapon())->m_bCharging = true;
		((CCFBaseMeleeWeapon*)ToCFPlayer(pOwner)->GetSecondaryWeapon())->m_iAttack = m_iAttack;
	}

	ToCFPlayer(pOwner)->FreezePlayer(0.5f);
}

void CCFBaseMeleeWeapon::StopCharge(bool bMultiplier, float flFreeze)
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
		return;

	if (bMultiplier)
	{
		m_flChargeAmount = RemapValClamped(gpGlobals->curtime, m_flChargeStartTime, m_flChargeStartTime + mp_chargetime.GetFloat(), 0.0f, 1.0f);
		m_flChargeMultiplier = RemapValClamped(m_flChargeAmount, 0, 1, 1.5f, 2.0f);
	}

	// Only award a proper lesson learned if we are sure that the player has not tapped the attack button.
//	if (m_flChargeAmount > 0.3f)
//		ToCFPlayer(pOwner)->Instructor_LessonLearned(HINT_HOLD_CHARGE_POWERSWING);

	m_bCharging = false;

	if (ToCFPlayer(pOwner)->HasDualMelee())
		((CCFBaseMeleeWeapon*)ToCFPlayer(pOwner)->GetSecondaryWeapon())->m_bCharging = false;

	if (flFreeze)
		ToCFPlayer(pOwner)->FreezePlayer(flFreeze);
}

void CCFBaseMeleeWeapon::ResetCharge()
{
	m_flChargeMultiplier = 1;
	m_flChargeAmount = 0;
}

bool CCFBaseMeleeWeapon::IsCharging(bool bDelay)
{
	if (bDelay && (gpGlobals->curtime - m_flChargeStartTime) < 0.25f)
		return false;

	return m_bCharging;
}

float CCFBaseMeleeWeapon::GetChainMultiplier()
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
		return 1;

	// Player gets 10% more whatever for each successive hit.
	return (10.0f + (float)ToCFPlayer(pOwner)->m_iMeleeChain) / 10.0f;
}

float CCFBaseMeleeWeapon::GetFireRate( void )
{
	return BaseClass::GetFireRate() * (IsStrongAttack()?2:1);
}

#ifdef CLIENT_DLL
void CCFBaseMeleeWeapon::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	ClientThinkList()->SetNextClientThink( GetClientHandle(), CLIENT_THINK_ALWAYS );
}

void CCFBaseMeleeWeapon::ClientThink()
{
	if (m_bCharging != m_bOldCharging)
	{
		if (!m_bOldCharging && m_bCharging)
			CreateChargeEffect();

		m_bOldCharging = m_bCharging;
	}

	if (!m_bCharging && m_pChargeEffect || GetOwner() && !GetOwner()->IsAlive())
	{
		ParticleProp()->StopEmission(m_pChargeEffect);
		m_pChargeEffect = NULL;
	}

	if (m_bAfflictive != m_bOldAfflictive)
	{
		if (!m_bOldAfflictive && m_bAfflictive)
			CreateSwingEffect();

		m_bOldAfflictive = m_bAfflictive;
	}

	if (!m_bAfflictive && m_apSwingEffects.Count() || GetOwner() && !GetOwner()->IsAlive())
	{
		for (int i = 0; i < m_apSwingEffects.Count(); i++)
			ParticleProp()->StopEmission(m_apSwingEffects[i]);
		m_apSwingEffects.RemoveAll();
	}
}

void CCFBaseMeleeWeapon::CreateChargeEffect()
{
	if (m_pChargeEffect)
		ParticleProp()->StopEmissionAndDestroyImmediately(m_pChargeEffect);

	if (m_iTipAttachment == -1)
	{
		m_iTipAttachment = LookupAttachment("tip");
		m_iHiltAttachment = LookupAttachment("hilt");
	}

	m_pChargeEffect = ParticleProp()->Create( "sword_charge", PATTACH_POINT_FOLLOW, m_iHiltAttachment );
	ParticleProp()->AddControlPoint( m_pChargeEffect, 1, this, PATTACH_POINT_FOLLOW, "hilt" );
	ParticleProp()->AddControlPoint( m_pChargeEffect, 2, this, PATTACH_POINT_FOLLOW, "tip" );
}

void CCFBaseMeleeWeapon::CreateSwingEffect()
{
	if (m_apSwingEffects.Count())
	{
		for (int i = 0; i < m_apSwingEffects.Count(); i++)
			ParticleProp()->StopEmission(m_apSwingEffects[i]);
		m_apSwingEffects.RemoveAll();
	}

	if (m_iTipAttachment == -1)
	{
		m_iTipAttachment = LookupAttachment("tip");
		m_iHiltAttachment = LookupAttachment("hilt");
	}

	element_t eElement = m_eAttackElement;

	if (!eElement)
	{
		CNewParticleEffect* pParticle = ParticleProp()->Create( "swing_typeless", PATTACH_POINT_FOLLOW, m_iHiltAttachment );
		ParticleProp()->AddControlPoint( pParticle, 1, this, PATTACH_POINT_FOLLOW, "hilt" );
		ParticleProp()->AddControlPoint( pParticle, 2, this, PATTACH_POINT_FOLLOW, "tip" );
		m_apSwingEffects.AddToTail(pParticle);
	}
	else
	{
		for (int i = 0; i < sizeof(element_t)*4; i++)
		{
			if (!((1<<i) & eElement))
				continue;

			CNewParticleEffect* pParticle = ParticleProp()->Create( VarArgs("swing_%s", ElementToString((element_t)(1<<i))), PATTACH_POINT_FOLLOW, m_iHiltAttachment );
			ParticleProp()->AddControlPoint( pParticle, 1, this, PATTACH_POINT_FOLLOW, "hilt" );
			ParticleProp()->AddControlPoint( pParticle, 2, this, PATTACH_POINT_FOLLOW, "tip" );
			m_apSwingEffects.AddToTail(pParticle);
		}
	}
}

#endif
