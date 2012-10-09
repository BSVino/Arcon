//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "in_buttons.h"
#include "takedamageinfo.h"
#include "weapon_cfbase.h"
#include "ammodef.h"
#include "datacache/imdlcache.h"
#include "weapon_magic.h"
#include "cf_allweapons.h"
#include "weapon_flag.h"

#if defined( CLIENT_DLL )
#include "c_cf_player.h"
#include "fx_impact.h"
#include "prediction.h"
#include "iinput.h"
#include "cf_in_main.h"
#else
#include "cf_player.h"
#endif

#include "engine/ivdebugoverlay.h"

// ----------------------------------------------------------------------------- //
// Global functions.
// ----------------------------------------------------------------------------- //

//--------------------------------------------------------------------------------------------------------
static const char * s_WeaponAliasInfo[] = 
{
	// IMPORTANT: Keep these so that "weapon_%s" is the entity name, or armament buying will fail.
	"none",			// WEAPON_NONE
	"magic",		// WEAPON_MAGIC
	"pistol",		// WEAPON_PISTOL
	"rifle",		// WEAPON_RIFLE
	"rivenblade",	// WEAPON_RIVENBLADE
	"shotgun",		// WEAPON_SHOTGUN
	"valangard",	// WEAPON_VALANGARD
	"pariah",		// WEAPON_PARIAH
	"flag",			// WEAPON_FLAG
	NULL,			// WEAPON_NONE
};

//--------------------------------------------------------------------------------------------------------
//
// Given an alias, return the associated weapon ID
//
CFWeaponID AliasToWeaponID( const char *alias )
{
	if (alias)
	{
		for( int i=0; s_WeaponAliasInfo[i] != NULL; ++i )
			if (!Q_stricmp( s_WeaponAliasInfo[i], alias ))
				return (CFWeaponID)i;

		CFWeaponID iWeapon = (CFWeaponID)atoi(alias);
		if (iWeapon >= WEAPON_NONE && iWeapon < WEAPON_MAX)
			return iWeapon;
	}

	return WEAPON_NONE;
}

//--------------------------------------------------------------------------------------------------------
//
// Given a weapon ID, return its alias
//
const char *WeaponIDToAlias( CFWeaponID id )
{
	if ( (id >= WEAPON_MAX) || (id < 0) )
		return NULL;

	return s_WeaponAliasInfo[id];
}

// ----------------------------------------------------------------------------- //
// CWeaponCFBase tables.
// ----------------------------------------------------------------------------- //

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponCFBase, DT_WeaponCFBase )

BEGIN_NETWORK_TABLE( CWeaponCFBase, DT_WeaponCFBase )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO(m_iPosition)),
	RecvPropInt( RECVINFO(m_bRightHand)),
	RecvPropTime( RECVINFO(m_flDisabledUntil)),
#else
	// world weapon models have no animations
  	SendPropExclude( "DT_AnimTimeMustBeFirst", "m_flAnimTime" ),
	SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),
	SendPropInt( SENDINFO(m_iPosition), 2, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_bRightHand), 1, SPROP_UNSIGNED ),
	SendPropTime( SENDINFO(m_flDisabledUntil) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponCFBase )
	DEFINE_PRED_FIELD( m_flTimeWeaponIdle, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_iPosition, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bRightHand, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flDisabledUntil, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_cf_base, CWeaponCFBase );


#ifdef GAME_DLL

	BEGIN_DATADESC( CWeaponCFBase )

		// New weapon Think and Touch Functions go here..

	END_DATADESC()

#endif

#ifdef CLIENT_DLL
CUtlVector<CWeaponCFBase::CGroupedSound> CWeaponCFBase::s_GroupedSounds;
#endif

bool IsPrimaryWeapon( int id )
{
	switch( id )
	{
		case WEAPON_MAGIC:
		case WEAPON_RIFLE:
		case WEAPON_RIVENBLADE:
		case WEAPON_SHOTGUN:
		case WEAPON_VALANGARD:
		case WEAPON_PARIAH:
		case WEAPON_FLAG:
			return true;
	}

	return false;
}

//--------------------------------------------------------------------------------------------------------
//
// Return true if given weapon ID is a secondary weapon
//
bool IsSecondaryWeapon( int id )
{
	switch( id )
	{
		case WEAPON_MAGIC:
		case WEAPON_PISTOL:
		case WEAPON_RIVENBLADE:
			return true;
	}

	return false;
}

CFWeaponType WeaponClassFromWeaponID( CFWeaponID weaponID )
{
	return CWeaponCFBase::GetWeaponType(weaponID);
}

// ----------------------------------------------------------------------------- //
// CWeaponCSBase implementation. 
// ----------------------------------------------------------------------------- //
CWeaponCFBase::CWeaponCFBase()
{
	SetPredictionEligible( true );

	AddSolidFlags( FSOLID_TRIGGER ); // Nothing collides with these but it gets touches.

	m_iPosition = 0;
	m_bRightHand = true;
	m_bWeaponVisible = true;
	m_flDisabledUntil = 0;
}

const CCFWeaponInfo &CWeaponCFBase::GetCFWpnData() const
{
	const FileWeaponInfo_t *pWeaponInfo = &GetWpnData();
	const CCFWeaponInfo *pCFInfo;

	#ifdef _DEBUG
		pCFInfo = dynamic_cast< const CCFWeaponInfo* >( pWeaponInfo );
		Assert( pCFInfo );
	#else
		pCFInfo = static_cast< const CCFWeaponInfo* >( pWeaponInfo );
	#endif

	return *pCFInfo;
}

void CWeaponCFBase::ItemPreFrame( void )
{
    if ( GetPlayerOwner() && (gpGlobals->curtime < GetPlayerOwner()->m_flNextAttack) )
		return;

	// More stuff here? Maybe later.
}

void CWeaponCFBase::ItemPostFrame( void )
{
	CCFPlayer *pOwner = ToCFPlayer( GetOwner() );
	if (!pOwner)
		return;

    if ( gpGlobals->curtime < pOwner->m_flNextAttack )
		return;

	if (pOwner->m_hReviving != NULL)
		return;

	if ( UsesClipsForAmmo1() )
	{
		CheckReload();
	}

	bool bFired = false;

	// Secondary attack has priority
	if (pOwner->IsPhysicalMode() && (FiringButtons() & SecondaryButtons()) && (m_flNextSecondaryAttack <= gpGlobals->curtime))
	{
		// Clip empty? Or out of ammo on a no-clip weapon?
		if ( !IsMeleeWeapon() &&  
			(( UsesClipsForAmmo1() && m_iClip1 <= 0) || ( !UsesClipsForAmmo1() && pOwner->GetAmmoCount(m_iPrimaryAmmoType)<=0 )) )
		{
			HandleFireOnEmpty();
		}
		else if (pOwner->GetWaterLevel() == 3 && m_bFiresUnderwater == false)
		{
			// This weapon doesn't fire underwater
			WeaponSound(EMPTY);
			DisableForSeconds(0.2);
			return;
		}
		else
		{
			//NOTENOTE: There is a bug with this code with regards to the way machine guns catch the leading edge trigger
			//			on the player hitting the attack key.  It relies on the gun catching that case in the same frame.
			//			However, because the player can also be doing a secondary attack, the edge trigger may be missed.
			//			We really need to hold onto the edge trigger and only clear the condition when the gun has fired its
			//			first shot.  Right now that's too much of an architecture change -- jdw
			
			// If the firing button was just pressed, or the alt-fire just released, reset the firing time
			if ( pOwner && ( (pOwner->m_afButtonPressed & PrimaryButtons()) || (pOwner->m_afButtonReleased & SecondaryButtons()) ) )
			{
				 DisableUntil(gpGlobals->curtime);
			}

			SecondaryAttack();
		}
	}
	
	if (pOwner->IsPhysicalMode() && !bFired && (FiringButtons() & PrimaryButtons()) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		// Clip empty? Or out of ammo on a no-clip weapon?
		if ( !IsMeleeWeapon() &&  
			(( UsesClipsForAmmo1() && m_iClip1 <= 0) || ( !UsesClipsForAmmo1() && pOwner->GetAmmoCount(m_iPrimaryAmmoType)<=0 )) )
		{
			HandleFireOnEmpty();
		}
		else if (pOwner->GetWaterLevel() == 3 && m_bFiresUnderwater == false)
		{
			// This weapon doesn't fire underwater
			WeaponSound(EMPTY);
			DisableForSeconds(0.2);
			return;
		}
		else
		{
			//NOTENOTE: There is a bug with this code with regards to the way machine guns catch the leading edge trigger
			//			on the player hitting the attack key.  It relies on the gun catching that case in the same frame.
			//			However, because the player can also be doing a secondary attack, the edge trigger may be missed.
			//			We really need to hold onto the edge trigger and only clear the condition when the gun has fired its
			//			first shot.  Right now that's too much of an architecture change -- jdw
			
			// If the firing button was just pressed, or the alt-fire just released, reset the firing time
			if ( pOwner && ( (pOwner->m_afButtonPressed & PrimaryButtons()) || (pOwner->m_afButtonReleased & SecondaryButtons()) ) )
			{
				 DisableUntil(gpGlobals->curtime);
			}

			PrimaryAttack();
		}
	}

	// -----------------------
	//  Reload pressed / Clip Empty
	// -----------------------
	if ( pOwner->m_nButtons & IN_RELOAD && UsesClipsForAmmo1() && !m_bInReload ) 
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		Reload();
	}
}

bool CWeaponCFBase::PlayEmptySound()
{
	CPASAttenuationFilter filter( this );
	filter.UsePredictionRules();

	EmitSound( filter, entindex(), "Default.ClipEmpty_Rifle" );
	
	return 0;
}

CCFPlayer* CWeaponCFBase::GetPlayerOwner() const
{
	return dynamic_cast< CCFPlayer* >( GetOwner() );
}

#ifdef GAME_DLL

void CWeaponCFBase::SendReloadEvents()
{
	CCFPlayer *pPlayer = dynamic_cast< CCFPlayer* >( GetOwner() );
	if ( !pPlayer )
		return;

	// Send a message to any clients that have this entity to play the reload.
/*	CPASFilter filter( pPlayer->GetAbsOrigin() );
	filter.RemoveRecipient( pPlayer );

	UserMessageBegin( filter, "ReloadEffect" );
	WRITE_SHORT( pPlayer->entindex() );
	MessageEnd();*/

	// Make the player play his reload animation.
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD, 0, pPlayer->GetSecondaryWeapon() == this );
}

#endif

bool CWeaponCFBase::Deploy( )
{
	MDLCACHE_CRITICAL_SECTION();
	return DefaultDeploy( NULL, (char*)GetWorldModel(), NULL, (char*)GetAnimPrefix() );
}

void CWeaponCFBase::DefaultTouch( CBaseEntity *pOther )
{
	// Don't pick up weapons lying on the ground.
}

void CWeaponCFBase::SetPickupTouch()
{
#ifdef GAME_DLL
	// The superclass calls SUB_Remove in 30 seconds. That's no good.
	// This clever function disappears us when nobody is looking. Nice!
	// We don't want players to think they can pick it up.

	SetThink( &CWeaponCFBase::SUB_Vanish );
	SetNextThink( gpGlobals->curtime + 5.0f );
#endif
}

void CWeaponCFBase::SetWeaponVisible( bool bVisible )
{
	m_bWeaponVisible = bVisible;

	if ( bVisible )
		RemoveEffects( EF_NODRAW );
	else if (GetPlayerOwner() && (GetPlayerOwner()->IsPariah() || GetPlayerOwner()->HasObjective()))
		AddEffects( EF_NODRAW );
}

bool CWeaponCFBase::IsWeaponVisible()
{
	return !IsEffectActive(EF_NODRAW);
}

void CWeaponCFBase::PrimaryAttack()
{
	const CCFWeaponInfo &pWeaponInfo = GetCFWpnData();
	CCFPlayer *pPlayer = GetPlayerOwner();

	float flCycleTime = GetFireRate();

	float flSpread = pWeaponInfo.m_flAccuracy;

	// more spread when jumping
	if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
		flSpread = 0.05f;
	
	pPlayer->m_iShotsFired++;

	// Out of ammo?
	if ( m_iClip1 <= 0 )
	{
		if (m_bFireOnEmpty)
		{
			PlayEmptySound();
			DisableForSeconds(0.2);
		}
	}

	m_iClip1--;

	DispatchParticleEffect("muzzle_flash", PATTACH_POINT_FOLLOW, this, "muzzle");

	// player "shoot" animation
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY, 0, pPlayer->GetSecondaryWeapon() == this );

	Vector vecOrigin = pPlayer->Weapon_ShootPosition();
	QAngle vecAngles = pPlayer->EyeAngles() + pPlayer->GetPunchAngle();

	int iSeed = CBaseEntity::GetPredictionRandomSeed() & 255;

	pPlayer->FireBullets(
		vecOrigin,
		vecAngles,
		GetWeaponID(),
		iSeed,
		flSpread );

	pPlayer->Recoil( pWeaponInfo.m_flPermRecoil, pWeaponInfo.m_flTempRecoil );

	pPlayer->SetNextAttack(gpGlobals->curtime + flCycleTime);
	DisableForSeconds(flCycleTime);

	// start idle animation in 5 seconds
	SetWeaponIdleTime( gpGlobals->curtime + 5.0 );
}

void CWeaponCFBase::SecondaryAttack()
{
	const CCFWeaponInfo &pWeaponInfo = GetCFWpnData();
	CCFPlayer *pPlayer = GetPlayerOwner();

	float flCycleTime = GetFireRate();

	float flSpread = pWeaponInfo.m_flAccuracy;

	// more spread when jumping
	if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
		flSpread = 0.05f;
	
	pPlayer->m_iShotsFired++;

	// Out of ammo?
	if ( m_iClip1 <= 0 )
	{
		if (m_bFireOnEmpty)
		{
			PlayEmptySound();
			DisableForSeconds(0.2);
		}
	}

	DispatchParticleEffect("muzzle_flash", PATTACH_POINT_FOLLOW, this, "muzzle");

	// player "shoot" animation
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY, 0, pPlayer->GetSecondaryWeapon() == this );

	Vector vecOrigin = pPlayer->Weapon_ShootPosition();
	QAngle vecAngles = pPlayer->EyeAngles() + pPlayer->GetPunchAngle();

	int iSeed = CBaseEntity::GetPredictionRandomSeed() & 255;

	pPlayer->FireDrainer(
		vecOrigin,
		vecAngles,
		GetWeaponID(),
		iSeed,
		flSpread );

	pPlayer->Recoil( pWeaponInfo.m_flPermRecoil*2, pWeaponInfo.m_flTempRecoil*2 );

	pPlayer->m_flNextAttack = gpGlobals->curtime + flCycleTime;
	DisableForSeconds(flCycleTime);

	// start idle animation in 5 seconds
	SetWeaponIdleTime( gpGlobals->curtime + 5.0 );

	m_iClip1 = 0;
}

bool CWeaponCFBase::DefaultReload( int iClipSize1, int iClipSize2, int iActivity )
{
	if (!CBaseCombatWeapon::DefaultReload(iClipSize1, iClipSize2, iActivity))
		return false;

	CCFPlayer* pPlayer = ToCFPlayer(GetOwner());
	CWeaponMagic* pMagic = pPlayer->GetMagicWeapon();

	float flSequenceEndTime = gpGlobals->curtime + ((CBasePlayer*)GetOwner())->SequenceDuration();
	pMagic->m_flNextPrimaryAttack = flSequenceEndTime;
	pPlayer->SetNextAttack(flSequenceEndTime);
	DisableUntil(flSequenceEndTime);

	pPlayer->Instructor_LessonLearned(HINT_R_RELOAD);

	return true;
}

// Flagging tells the user that his weapon is disabled in the HUD
void CWeaponCFBase::DisableForSeconds(float flTime, bool bFlag)
{
	DisableUntil(gpGlobals->curtime + flTime, bFlag);
}

void CWeaponCFBase::DisableUntil(float flTime, bool bFlag)
{
	if (m_flNextPrimaryAttack < flTime)
		m_flNextPrimaryAttack = flTime;

	if (m_flNextSecondaryAttack < flTime)
		m_flNextSecondaryAttack = flTime;

	if (bFlag)
	{
		m_flDisabledUntil = flTime;
	}
}

bool CWeaponCFBase::IsDisableFlagged()
{
	return m_flDisabledUntil > gpGlobals->curtime;
}

int CWeaponCFBase::UpdateClientData( CBasePlayer *pPlayer )
{
	int iNewState = WEAPON_IS_CARRIED_BY_PLAYER;

	if ( IsWeaponVisible() )
		iNewState = WEAPON_IS_ACTIVE;

	if ( m_iState != iNewState )
	{
		m_iState = iNewState;
	}
	return 1;
}

#ifdef CLIENT_DLL
void CWeaponCFBase::NotifyShouldTransmit( ShouldTransmitState_t state )
{
	// skip CBaseCombatWeapon
	CBaseAnimating::NotifyShouldTransmit(state);

	if (state == SHOULDTRANSMIT_END)
	{
		if (m_iState == WEAPON_IS_ACTIVE)
		{
			m_iState = WEAPON_IS_CARRIED_BY_PLAYER;
		}
	}
	else if( state == SHOULDTRANSMIT_START )
	{
		if( m_iState == WEAPON_IS_CARRIED_BY_PLAYER )
		{
			if( IsWeaponVisible() )
			{
				// Restore the Activeness of the weapon if we client-twiddled it off in the first case above.
				m_iState = WEAPON_IS_ACTIVE;
			}
		}
	}
}
#endif

void CWeaponCFBase::Equip( CBaseCombatCharacter *pOwner )
{
	// Attach the weapon to an owner
	SetAbsVelocity( vec3_origin );
	RemoveSolidFlags( FSOLID_TRIGGER );
	AddSolidFlags( FSOLID_NOT_SOLID );
	FollowEntity( pOwner );
	SetOwner( pOwner );
	SetOwnerEntity( pOwner );

	// Break any constraint I might have to the world.
	RemoveEffects( EF_ITEM_BLINK );

#if !defined( CLIENT_DLL )
	if ( GetConstraint() )
	{
		RemoveSpawnFlags( SF_WEAPON_START_CONSTRAINED );
		physenv->DestroyConstraint( GetConstraint() );
		ResetConstraint();
	}
#endif

	DisableUntil(gpGlobals->curtime);
	SetTouch( NULL );
	SetThink( NULL );
#if !defined( CLIENT_DLL )
	VPhysicsDestroyObject();
#endif

	// Make the weapon ready as soon as any NPC picks it up.
	DisableUntil(gpGlobals->curtime);
	SetModel( GetWorldModel() );
}

// Called by the ImpactSound function.
void CWeaponCFBase::ShotgunImpactSoundGroup( const char *pSoundName, const Vector &vEndPos )
{
#ifdef CLIENT_DLL
	int i;
	// Don't play the sound if it's too close to another impact sound.
	for ( i=0; i < s_GroupedSounds.Count(); i++ )
	{
		CGroupedSound *pSound = &s_GroupedSounds[i];

		if ( vEndPos.DistToSqr( pSound->m_vPos ) < 300*300 )
		{
			if ( Q_stricmp( pSound->m_SoundName, pSoundName ) == 0 )
				return;
		}
	}

	// Ok, play the sound and add it to the list.
	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound( filter, NULL, pSoundName, &vEndPos );

	i = s_GroupedSounds.AddToTail();
	s_GroupedSounds[i].m_SoundName = pSoundName;
	s_GroupedSounds[i].m_vPos = vEndPos;
#endif
}

unsigned int CWeaponCFBase::FiringButtons()
{
	CCFPlayer* pPlayer = ToCFPlayer(GetOwner());

	Assert(pPlayer);

	if (!pPlayer)
		return 0;

	if (IsFullAuto())
		return pPlayer->m_nButtons;
	else
		return pPlayer->m_afButtonPressed; // Semi auto only triggers if the button is pressed.
}

unsigned int CWeaponCFBase::PrimaryButtons()
{
	CCFPlayer* pPlayer = ToCFPlayer(GetOwner());

	Assert(pPlayer);

	if (!pPlayer)
		return IN_ATTACK;

	// If the player has two melee weapons, all signals go to the first one.
	if (pPlayer->HasDualMelee())
	{
		if (IsPrimary())
			return IN_ATTACK;
		else
			return 0;
	}

	// Solitary primary weapons always use attack1
	if (!pPlayer->GetSecondaryWeapon())
		return IN_ATTACK;

	bool bPrimaryInLeft = false;
	if (pPlayer->GetPrimaryWeapon())
		bPrimaryInLeft = pPlayer->GetPrimaryWeapon()->GetCFWpnData().m_bPrimaryInLeft;
	else
		bPrimaryInLeft = GetCFWpnData().m_bPrimaryInLeft;

	int iPrimary = IN_ATTACK2;
	int iSecondary = IN_ATTACK;

	if (bPrimaryInLeft)
	{
		iPrimary = IN_ATTACK;
		iSecondary = IN_ATTACK2;
	}

	if (IsPrimary())
		return iPrimary;
	else
		return iSecondary;
}

unsigned int CWeaponCFBase::SecondaryButtons()
{
	CCFPlayer* pPlayer = ToCFPlayer(GetOwner());

	Assert(pPlayer);

	if (!pPlayer)
		return 0;

	// If the player has two melee weapons, all signals go to the first one.
	if (pPlayer->HasDualMelee())
	{
		if (IsPrimary())
			return IN_ATTACK2;
		else
			return 0;
	}

	// No secondary if latched.
	if (pPlayer->m_bLatched)
		return 0;

	if (IsPrimary())
	{
		// If we are a primary and there is no secondary, then we can use attack2. Otherwise no.
		if (!pPlayer->GetSecondaryWeapon())
			return IN_ATTACK2;
	}

	return 0;
}

unsigned int CWeaponCFBase::AttackButtons()
{
	return PrimaryButtons() | SecondaryButtons();
}

Activity CWeaponCFBase::ActivityOverride( Activity baseAct, bool *pRequired )
{
	if (!GetOwner())
		return baseAct;

	CCFPlayer* pPlayer = ToCFPlayer(GetOwner());

	CWeaponCFBase* pWeapon = pPlayer->GetSecondaryWeapon();

	return ActivityOverride( GetWeaponID(), pWeapon?pWeapon->GetWeaponType():WT_NONE, baseAct, pRequired );
}

Activity CWeaponCFBase::ActivityOverride( CFWeaponID ePrimary, CFWeaponType eSecondary, Activity baseAct, bool *pRequired )
{
	acttable_t *pTable;
	int actCount;
	int i;

	pTable = ActivityList(ePrimary, eSecondary, true);
	actCount = ActivityListCount(ePrimary, eSecondary, true);

	for ( i = 0; i < actCount; i++, pTable++ )
	{
		if ( baseAct == pTable->baseAct )
		{
			if (pRequired)
			{
				*pRequired = pTable->required;
			}
			return (Activity)pTable->weaponAct;
		}
	}

	pTable = ActivityList(ePrimary, eSecondary, false);
	actCount = ActivityListCount(ePrimary, eSecondary, false);

	for ( i = 0; i < actCount; i++, pTable++ )
	{
		if ( baseAct == pTable->baseAct )
		{
			if (pRequired)
			{
				*pRequired = pTable->required;
			}
			return (Activity)pTable->weaponAct;
		}
	}
	return baseAct;
}

acttable_t* CWeaponCFBase::ActivityList( CFWeaponID ePrimary, CFWeaponType eSecondary, bool bCommon )
{
	// Is this really worth it?

	switch (ePrimary)
	{
	case WEAPON_PARIAH:
		return CWeaponPariahBlade::ActivityListStatic(eSecondary, bCommon);
	case WEAPON_PISTOL:
		return CWeaponPistol::ActivityListStatic(eSecondary, bCommon);
	case WEAPON_RIFLE:
		return CWeaponRifle::ActivityListStatic(eSecondary, bCommon);
	case WEAPON_RIVENBLADE:
		return CWeaponRivenBlade::ActivityListStatic(eSecondary, bCommon);
	case WEAPON_SHOTGUN:
		return CWeaponShotgun::ActivityListStatic(eSecondary, bCommon);
	case WEAPON_VALANGARD:
		return CWeaponValangard::ActivityListStatic(eSecondary, bCommon);
	case WEAPON_FLAG:
		return CWeaponFlag::ActivityListStatic();
	}

	return NULL;
}

int CWeaponCFBase::ActivityListCount( CFWeaponID ePrimary, CFWeaponType eSecondary, bool bCommon )
{
	switch (ePrimary)
	{
	case WEAPON_PARIAH:
		return CWeaponPariahBlade::ActivityListCountStatic(eSecondary, bCommon);
	case WEAPON_PISTOL:
		return CWeaponPistol::ActivityListCountStatic(eSecondary, bCommon);
	case WEAPON_RIFLE:
		return CWeaponRifle::ActivityListCountStatic(eSecondary, bCommon);
	case WEAPON_RIVENBLADE:
		return CWeaponRivenBlade::ActivityListCountStatic(eSecondary, bCommon);
	case WEAPON_SHOTGUN:
		return CWeaponShotgun::ActivityListCountStatic(eSecondary, bCommon);
	case WEAPON_VALANGARD:
		return CWeaponValangard::ActivityListCountStatic(eSecondary, bCommon);
	case WEAPON_FLAG:
		return CWeaponFlag::ActivityListCountStatic();
	}

	return 0;
}

bool CWeaponCFBase::IsPredicted() const
{
#ifdef CLIENT_DLL
	return prediction->InPrediction();
#else
	return true;
#endif
}

CFWeaponType CWeaponCFBase::GetWeaponType() const
{
	return GetWeaponType(GetWeaponID());
}

CFWeaponType CWeaponCFBase::GetWeaponType(CFWeaponID eWeapon)
{
	switch (eWeapon)
	{
	case WEAPON_PARIAH:
		return WT_LONGSWORD;

	case WEAPON_PISTOL:
		return WT_PISTOL;

	case WEAPON_RIFLE:
		return WT_LONGGUN;

	case WEAPON_RIVENBLADE:
		return WT_LONGSWORD;

	case WEAPON_SHOTGUN:
		return WT_LONGGUN;

	case WEAPON_VALANGARD:
		return WT_BUSTER;

	case WEAPON_FLAG:
		return WT_NONE;
	}

	return WT_NONE;
}

bool CWeaponCFBase::IsPrimary()
{
	if (GetPosition() == 0)
		return true;

	return (GetOwner() && GetOwner()->IsPlayer() &&
		ToCFPlayer(GetOwner())->ShouldPromoteSecondary() &&
		ToCFPlayer(GetOwner())->SecondaryToPromote() == GetPosition());
}

bool CWeaponCFBase::IsActive()
{
	if (IsPrimary())
		return true;

	return (GetOwner() && GetOwner()->IsPlayer() && ToCFPlayer(GetOwner())->GetActiveSecondary() == GetPosition());
}

bool CWeaponCFBase::IsMeleeWeapon() const
{
	return GetCFWpnData().m_eWeaponType == WEAPONTYPE_MELEE || GetCFWpnData().m_eWeaponType == WEAPONTYPE_MAGIC;
}

char* CWeaponCFBase::GetBoneMergeName(int iBone)
{
	if (!IsActive())
	{
		switch (GetWeaponType())
		{
		default:
			Assert(false);
		case WT_PISTOL:
			if (m_bRightHand)
				return "ValveBiped.Weapon_R_Leg";
			else
				return "ValveBiped.Weapon_L_Leg";
		case WT_LONGSWORD:
			if (m_bRightHand)
				return "ValveBiped.Weapon_R_Backside";
			else
				return "ValveBiped.Weapon_L_Backside";
		}
	}

	if (m_bRightHand)
		return "ValveBiped.Weapon_R_Hand";
	else
		return "ValveBiped.Weapon_L_Hand";
}

float CWeaponCFBase::GetFireRate( void )
{
	const CCFWeaponInfo &pWeaponInfo = GetCFWpnData();
	CCFPlayer *pPlayer = GetPlayerOwner();

	return pWeaponInfo.m_flCycleTime * pPlayer->m_pStats->GetSpeedInvScale();
}

#ifdef CLIENT_DLL
int CWeaponCFBase::DrawModel( int flags )
{
	if ( !IsVisible() )
		return 0;

	// Don't draw me if the player has the flag.
	// This will take place for weapon_flag too, but that has no model anyway, the regular flag model is used.
	if (GetPlayerOwner() && GetPlayerOwner()->HasObjective())
		return 0;

	// Skip CBaseCombatWeapon, it doesn't draw us if we're spectating in first person, which we don't want.
	return BASECOMBATWEAPON_DERIVED_FROM::DrawModel( flags );
}

bool CWeaponCFBase::OnInternalDrawModel( ClientModelRenderInfo_t *pInfo )
{
	// if local player is spectating this player in first person mode, don't draw it
	C_CFPlayer * pPlayer = CCFPlayer::GetLocalCFPlayer();

	CMatRenderContextPtr pRenderContext( materials );

	if ((!input->CAM_IsThirdPerson() && pPlayer == GetPlayerOwner())
		|| pPlayer->IsFirstPersonSpectating(GetPlayerOwner()))
		pRenderContext->DepthRange(0.0f, 0.01f);

	return BaseClass::OnInternalDrawModel(pInfo);
}

bool CWeaponCFBase::OnPostInternalDrawModel( ClientModelRenderInfo_t *pInfo )
{
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->DepthRange(0.0f, 1.0f);

	return BaseClass::OnPostInternalDrawModel(pInfo);
}

ShadowType_t CWeaponCFBase::ShadowCastType()
{
	if ( IsEffectActive( EF_NOSHADOW ) )
		return SHADOWS_NONE;

	return SHADOWS_RENDER_TO_TEXTURE;
}
#endif

