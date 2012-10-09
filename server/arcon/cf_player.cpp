//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Player for HL1.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "datacache/imdlcache.h"
#include "cf_player.h"
#include "cf_gamerules.h"
#include "weapon_cfbase.h"
#include "predicted_viewmodel.h"
#include "iservervehicle.h"
#include "viewport_panel_names.h"
#include "in_buttons.h"
#include "statistics.h"
#include "cfgui_shared.h"
#include "activitylist.h"
#include "armament.h"
#include "weapon_magic.h"
#include "weapon_pariah.h"
#include "ai_basenpc.h"
#include "cf_team.h"
#include "objectives.h"
#include "takedamageinfo.h"
#include "cf_player_resource.h"
#include "weapon_draingrenade.h"
#include "props.h"

extern int gEvilImpulse101;

void* SendProxy_Statistics( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID );

extern ConVar	sk_player_head;
extern ConVar	sk_player_chest;
extern ConVar	sk_player_stomach;
extern ConVar	sk_player_arm;
extern ConVar	sk_player_leg;

extern ConVar	mp_hpbonustime;
extern ConVar	mp_hpbonusamnt;

ConVar mp_fatalityrespawntimer("mp_fatalityrespawntimer", "10", FCVAR_DEVELOPMENTONLY, "How long after a fatality must a player wait to respawn?" );
ConVar mp_deathhp("mp_deathhp", "-500", FCVAR_DEVELOPMENTONLY, "At what health will a character die instead of being KO'd?" );

ConVar mp_fuse_start_time("mp_fuse_start_time", "3", FCVAR_DEVELOPMENTONLY, "How long will a player go GRAAAAH while becoming Fuse?" );
ConVar mp_pariah_start_time("mp_pariah_start_time", "3", FCVAR_DEVELOPMENTONLY, "How long will a player go GRAAAAH while becoming Pariah?" );

ConVar bot_mimic( "bot_mimic", "0", FCVAR_CHEAT );
ConVar bot_freeze( "bot_freeze", "0", FCVAR_CHEAT );
ConVar bot_crouch( "bot_crouch", "0", FCVAR_CHEAT );
ConVar bot_mimic_yaw_offset( "bot_mimic_yaw_offset", "180", FCVAR_CHEAT );

// -------------------------------------------------------------------------------- //
// Player animation event. Sent to the client when a player fires, jumps, reloads, etc..
// -------------------------------------------------------------------------------- //

class CTEPlayerAnimEvent : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEPlayerAnimEvent, CBaseTempEntity );
	DECLARE_SERVERCLASS();

	CTEPlayerAnimEvent( const char *name ) : CBaseTempEntity( name )
	{
		m_iPlayerIndex = MAX_PLAYERS+1;
	}

	CNetworkVar( int, m_iPlayerIndex );
	CNetworkVar( int, m_iEvent );
	CNetworkVar( int, m_nData );
	CNetworkVar( bool, m_bSecondary );
};

IMPLEMENT_SERVERCLASS_ST_NOBASE( CTEPlayerAnimEvent, DT_TEPlayerAnimEvent )
	SendPropInt( SENDINFO( m_iPlayerIndex ), 7, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iEvent ), Q_log2( PLAYERANIMEVENT_COUNT ) + 1, SPROP_UNSIGNED ),
	// BUGBUG:  ywb  we assume this is either 0 or an animation sequence #, but it could also be an activity, which should fit within this limit, but we're not guaranteed.
	SendPropInt( SENDINFO( m_nData ), ANIMATION_SEQUENCE_BITS ),
	SendPropInt( SENDINFO( m_bSecondary ), 4 ),		// Apparently you need four bits to send a boolean, it didn't work with just one.
END_SEND_TABLE()

static CTEPlayerAnimEvent g_TEPlayerAnimEvent( "PlayerAnimEvent" );

void TE_PlayerAnimEvent( CBasePlayer *pPlayer, PlayerAnimEvent_t event, int nData, bool bSecondary )
{
	CPVSFilter filter( (const Vector&)pPlayer->EyePosition() );
	if ( !IsCustomPlayerAnimEvent( event ) && !IsServerSendableAnimEvent( event, nData ) && ( event != PLAYERANIMEVENT_SNAP_YAW ) && ( event != PLAYERANIMEVENT_VOICE_COMMAND_GESTURE ) )
	{
		filter.RemoveRecipient( pPlayer );
	}
	
	Assert( pPlayer->entindex() >= 1 && pPlayer->entindex() <= MAX_PLAYERS );
	g_TEPlayerAnimEvent.m_iPlayerIndex = pPlayer->entindex();
	g_TEPlayerAnimEvent.m_iEvent = event;
	Assert( nData < (1<<ANIMATION_SEQUENCE_BITS) );
	Assert( (1<<ANIMATION_SEQUENCE_BITS) >= ActivityList_HighestIndex() );
	g_TEPlayerAnimEvent.m_nData = nData;
	g_TEPlayerAnimEvent.m_bSecondary = bSecondary;
	g_TEPlayerAnimEvent.Create( filter, 0 );
}

// -------------------------------------------------------------------------------- //
// Tables.
// -------------------------------------------------------------------------------- //

LINK_ENTITY_TO_CLASS( player, CCFPlayer );
PRECACHE_REGISTER(player);

BEGIN_NETWORK_TABLE_NOBASE( CStatistics, DT_Statistics )
	SendPropTime	(SENDINFO(m_flLastStatsThink)),
	SendPropFloat	(SENDINFO(m_flFocus),			9,	0,				STAT_FOP_MIN, STAT_FOP_MAX),
	SendPropInt		(SENDINFO(m_iMaxFocus),			10,	SPROP_UNSIGNED),
	SendPropFloat	(SENDINFO(m_flStamina),			10,	0,				STAT_STP_MIN, STAT_STP_MAX),
	SendPropInt		(SENDINFO(m_iMaxStamina),		10,	SPROP_UNSIGNED),
	SendPropFloat	(SENDINFO(m_flHealthRegen),		4,	SPROP_UNSIGNED, STAT_HLR_MIN, STAT_HLR_MAX),
	SendPropFloat	(SENDINFO(m_flFocusRegen),		4,	SPROP_UNSIGNED, STAT_HLR_MIN, STAT_HLR_MAX),
	SendPropFloat	(SENDINFO(m_flStaminaRegen),	6,	SPROP_UNSIGNED, STAT_STR_MIN, STAT_STR_MAX),
	SendPropFloat	(SENDINFO(m_flAttack),			8,	0,				STAT_ATT_MIN, STAT_ATT_MAX),
	SendPropFloat	(SENDINFO(m_flEnergy),			8,	0,				STAT_NRG_MIN, STAT_NRG_MAX),
	SendPropFloat	(SENDINFO(m_flDefense),			8,	0,				STAT_DEF_MIN, STAT_DEF_MAX),
	SendPropFloat	(SENDINFO(m_flResistance),		8,	0,				STAT_RES_MIN, STAT_RES_MAX),
	SendPropFloat	(SENDINFO(m_flSpeedStat),		8,	0,				STAT_SPD_MIN, STAT_SPD_MAX),
	SendPropFloat	(SENDINFO(m_flCritical),		7,	SPROP_UNSIGNED, STAT_CRT_MIN, STAT_CRT_MAX),

	SendPropInt		(SENDINFO(m_iStatusEffects),	TOTAL_STATUSEFFECTS,	SPROP_UNSIGNED),

	SendPropFloat	(SENDINFO(m_flDOT),				7,	SPROP_UNSIGNED, 0.0f, 10.0f),
	SendPropFloat	(SENDINFO(m_flSlowness),		7,	SPROP_UNSIGNED, 0.0f, 10.0f),
	SendPropFloat	(SENDINFO(m_flWeakness),		7,	SPROP_UNSIGNED, 0.0f, 10.0f),
	SendPropFloat	(SENDINFO(m_flDisorientation),	7,	SPROP_UNSIGNED, 0.0f, 10.0f),
	SendPropFloat	(SENDINFO(m_flBlindness),		7,	SPROP_UNSIGNED, 0.0f, 10.0f),
	SendPropFloat	(SENDINFO(m_flAtrophy),			7,	SPROP_UNSIGNED, 0.0f, 10.0f),
	SendPropFloat	(SENDINFO(m_flSilence),			7,	SPROP_UNSIGNED, 0.0f, 10.0f),
	SendPropFloat	(SENDINFO(m_flRegeneration),	7,	SPROP_UNSIGNED, 0.0f, 10.0f),
	SendPropFloat	(SENDINFO(m_flPoison),			7,	SPROP_UNSIGNED, 0.0f, 10.0f),
	SendPropFloat	(SENDINFO(m_flHaste),			7,	SPROP_UNSIGNED, 0.0f, 10.0f),
	SendPropFloat	(SENDINFO(m_flShield),			7,	SPROP_UNSIGNED, 0.0f, 10.0f),
	SendPropFloat	(SENDINFO(m_flBarrier),			7,	SPROP_UNSIGNED, 0.0f, 10.0f),
	SendPropFloat	(SENDINFO(m_flReflect),			7,	SPROP_UNSIGNED, 0.0f, 10.0f),
	SendPropTime	(SENDINFO(m_flStealth)),
	SendPropFloat	(SENDINFO(m_flOverdrive),		7,	SPROP_UNSIGNED, -1.0f, 10.0f),
END_NETWORK_TABLE()

BEGIN_SEND_TABLE_NOBASE( CCFPlayer, DT_CFLocalPlayerExclusive )
    SendPropInt( SENDINFO( m_iDirection ), 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iShotsFired ), 8, SPROP_UNSIGNED ),

	SendPropEHandle		( SENDINFO( m_hCameraCinematic ) ),
	SendPropTime		( SENDINFO( m_flCameraCinematicUntil ) ),

	SendPropTime		( SENDINFO( m_flFreezeUntil ) ),
	SendPropFloat		( SENDINFO( m_flFreezeAmount ) ),

	SendPropTime		( SENDINFO( m_flFreezeRotation ) ),
	SendPropFloat		( SENDINFO( m_flFreezeRAmount ) ),
	SendPropAngle		( SENDINFO_VECTORELEM(m_angFreeze, 0), 11 ),
	SendPropAngle		( SENDINFO_VECTORELEM(m_angFreeze, 1), 11 ),
	SendPropAngle		( SENDINFO_VECTORELEM(m_angFreeze, 2), 11 ),

	SendPropTime		( SENDINFO( m_flEnemyFrozenUntil ) ),
	SendPropEHandle		( SENDINFO( m_hEnemyFrozen ) ),

	SendPropTime		( SENDINFO( m_flSuspendGravityAt ) ),
	SendPropTime		( SENDINFO( m_flSuspendGravityUntil ) ),

	SendPropTime		( SENDINFO( m_flLastDashTime ) ),
	SendPropInt			( SENDINFO( m_bWantVelocityMatched ), 1, SPROP_UNSIGNED ),

	SendPropInt			( SENDINFO( m_iAirMeleeAttacks ), 6, SPROP_UNSIGNED ),
	SendPropInt			( SENDINFO( m_iMeleeChain ), 6, SPROP_UNSIGNED ),

	SendPropTime		( SENDINFO( m_flRushDistance ) ),
	SendPropEHandle		( SENDINFO( m_hRushingWeapon ) ),
	SendPropBool		( SENDINFO( m_bDownStrike ) ),

	SendPropFloat		( SENDINFO( m_flNextRespawn ) ),
	SendPropFloat		( SENDINFO( m_flFatalityStart ) ),

	SendPropEHandle		( SENDINFO( m_hLastAttacker ) ),
	SendPropFloat		( SENDINFO( m_flLastAttackedTime ) ),
	SendPropEHandle		( SENDINFO( m_hLastDOTAttacker ) ),

	SendPropFloat		( SENDINFO( m_flStrongAttackJumpTime ) ),
	SendPropBool		( SENDINFO( m_bStrongAttackJump ) ),

	SendPropInt			( SENDINFO( m_iLatchTriggerCount ) ),
	SendPropTime		( SENDINFO( m_flLastLatch ) ),
END_SEND_TABLE()

IMPLEMENT_SERVERCLASS_ST( CCFPlayer, DT_CFPlayer )
	SendPropExclude( "DT_BaseAnimating", "m_flPoseParameter" ),
	SendPropExclude( "DT_BaseAnimating", "m_flPlaybackRate" ),	
	SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),
	SendPropExclude( "DT_BaseEntity", "m_angRotation" ),
	SendPropExclude( "DT_BaseAnimatingOverlay", "overlay_vars" ),
	
	// playeranimstate and clientside animation takes care of these on the client
	SendPropExclude( "DT_ServerAnimationData" , "m_flCycle" ),	
	SendPropExclude( "DT_AnimTimeMustBeFirst" , "m_flAnimTime" ),

	// Data that only gets sent to the local player.
	SendPropDataTable( "cflocaldata", 0, &REFERENCE_SEND_TABLE(DT_CFLocalPlayerExclusive), SendProxy_SendLocalDataTable ),

	SendPropDataTable( "player_statistics_data", 0, &REFERENCE_SEND_TABLE( DT_Statistics ), SendProxy_Statistics ),

	SendPropInt( SENDINFO( m_bInFollowMode ), 1, SPROP_UNSIGNED ),

	SendPropBool( SENDINFO( m_bLatched ) ),
	SendPropBool( SENDINFO( m_bOverdrive ) ),
	SendPropVector( SENDINFO( m_vecLatchPlaneNormal ), -1, SPROP_COORD ),

	SendPropBool (SENDINFO(m_bCanPowerjump)),
	SendPropBool (SENDINFO(m_bPowerjump)),
	SendPropBool (SENDINFO(m_bChargejump)),

	SendPropInt( SENDINFO( m_eLHEffectElements ), TOTAL_ELEMENTS, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_eRHEffectElements ), TOTAL_ELEMENTS, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iLastCombo ), 2, SPROP_UNSIGNED ),

	SendPropInt( SENDINFO( m_iActiveSecondary ), 4, SPROP_UNSIGNED ),

	SendPropArray3	( SENDINFO_ARRAY3(m_hWeapons), SendPropEHandle( SENDINFO_ARRAY(m_hWeapons) ) ),
	SendPropArray3	( SENDINFO_ARRAY3(m_hAlternateWeapons), SendPropEHandle( SENDINFO_ARRAY(m_hAlternateWeapons) ) ),
	SendPropEHandle	( SENDINFO( m_hMagic ) ),

	SendPropBool		( SENDINFO(m_bPhysicalMode) ),

	SendPropInt		( SENDINFO( m_bBecomingFuse ), 1, SPROP_UNSIGNED ),
	SendPropInt		( SENDINFO( m_bIsFuse ), 1, SPROP_UNSIGNED ),
	SendPropFloat	( SENDINFO( m_flFuseStartTime ) ),
	SendPropInt		( SENDINFO( m_bBecomingPariah ), 1, SPROP_UNSIGNED ),
	SendPropInt		( SENDINFO( m_bIsPariah ), 1, SPROP_UNSIGNED ),
	SendPropFloat	( SENDINFO( m_flPariahStartTime ) ),

	SendPropInt		( SENDINFO( m_bIsCaptain ), 1, SPROP_UNSIGNED ),
	SendPropInt		( SENDINFO( m_bIsSergeant ), 1, SPROP_UNSIGNED ),

	SendPropEHandle	( SENDINFO( m_hDirectTarget ) ),
	SendPropEHandle	( SENDINFO( m_hRecursedTarget ) ),

	SendPropInt			( SENDINFO( m_bReviving ), 1, SPROP_UNSIGNED ),
	SendPropEHandle		( SENDINFO( m_hReviver ) ),
	SendPropEHandle		( SENDINFO( m_hReviving ) ),
	SendPropBool		( SENDINFO( m_bIsDecapitated ) ),

	SendPropEHandle		( SENDINFO( m_hObjective ) ),

	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 0), 11 ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 1), 11 ),
	SendPropEHandle( SENDINFO( m_hRagdoll ) ),
END_SEND_TABLE()

BEGIN_DATADESC( CCFPlayer )
	DEFINE_USEFUNC( ReviveUse ),
	DEFINE_AUTO_ARRAY( m_hWeapons, FIELD_EHANDLE ),
	DEFINE_AUTO_ARRAY( m_hAlternateWeapons, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hMagic, FIELD_EHANDLE ),

	DEFINE_FIELD( m_vecLatchPlaneNormal, FIELD_VECTOR ),
	DEFINE_FIELD( m_hLatchEntity, FIELD_EHANDLE ),

	DEFINE_FIELD( m_hObjective, FIELD_EHANDLE ),

	DEFINE_FIELD( m_bPhysicalMode, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_bLatched, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_iLatchTriggerCount, FIELD_INTEGER ),
	DEFINE_FIELD( m_vecLatchPlaneNormal, FIELD_VECTOR ),

	DEFINE_FIELD( m_bCanPowerjump, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bPowerjump, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bChargejump, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_iLastMovementButton, FIELD_INTEGER ),
	DEFINE_FIELD( m_flLastMovementButtonTime, FIELD_TIME ),

	DEFINE_FIELD( m_eLHEffectElements, FIELD_INTEGER ),
	DEFINE_FIELD( m_eRHEffectElements, FIELD_INTEGER ),
	DEFINE_FIELD( m_iLastCombo, FIELD_INTEGER ),
END_DATADESC()

class CCFRagdoll : public CBaseAnimatingOverlay
{
public:
	DECLARE_CLASS( CCFRagdoll, CBaseAnimatingOverlay );
	DECLARE_SERVERCLASS();

	virtual void FadeOut();

	// Transmit ragdolls to everyone.
	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

	void Init( void )
	{
		SetSolid( SOLID_BBOX );
		SetMoveType( MOVETYPE_STEP );
		SetFriction( 1.0f );
		SetCollisionBounds( VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX );
		m_takedamage = DAMAGE_NO;
		SetCollisionGroup( COLLISION_GROUP_DEBRIS );
		SetAbsOrigin( m_hPlayer->GetAbsOrigin() );
		SetAbsVelocity( m_hPlayer->GetAbsVelocity() );
		AddSolidFlags( FSOLID_NOT_SOLID );
		ChangeTeam( m_hPlayer->GetTeamNumber() );
		UseClientSideAnimation();
		m_flFadeStartTime = 0;
	}

public:
	// In case the client has the player entity, we transmit the player index.
	// In case the client doesn't have it, we transmit the player's model index, origin, and angles
	// so they can create a ragdoll in the right place.
	CNetworkHandle( CBaseEntity, m_hPlayer );	// networked entity handle 
	CNetworkVector( m_vecRagdollVelocity );
	CNetworkVector( m_vecRagdollOrigin );
	CNetworkVar(float, m_flFadeStartTime);
};

LINK_ENTITY_TO_CLASS( cf_ragdoll, CCFRagdoll );

IMPLEMENT_SERVERCLASS_ST_NOBASE( CCFRagdoll, DT_CFRagdoll )
	SendPropVector( SENDINFO(m_vecRagdollOrigin), -1,  SPROP_COORD ),
	SendPropEHandle( SENDINFO( m_hPlayer ) ),
	SendPropModelIndex( SENDINFO( m_nModelIndex ) ),
	SendPropInt		( SENDINFO(m_nForceBone), 8, 0 ),
	SendPropVector	( SENDINFO(m_vecForce), -1, SPROP_NOSCALE ),
	SendPropVector( SENDINFO( m_vecRagdollVelocity ) ),
	SendPropFloat( SENDINFO( m_flFadeStartTime ) ),
END_SEND_TABLE()

void CCFRagdoll::FadeOut()
{
	if (m_flFadeStartTime)
		return;

	m_flFadeStartTime = gpGlobals->curtime;
}

// -------------------------------------------------------------------------------- //

void cc_CreatePredictionError_f()
{
	CBaseEntity *pEnt = CBaseEntity::Instance( 1 );
	pEnt->SetAbsOrigin( pEnt->GetAbsOrigin() + Vector( 63, 0, 0 ) );
}

ConCommand cc_CreatePredictionError( "CreatePredictionError", cc_CreatePredictionError_f, "Create a prediction error", FCVAR_CHEAT );


CCFPlayer::CCFPlayer()
{
	m_PlayerAnimState = CreatePlayerAnimState( this );

	UseClientSideAnimation();
	m_angEyeAngles.Init();
	m_iDirection = 0;

	m_flFreezeUntil = .1;
	m_flFreezeAmount = 0;
	m_flFreezeRotation = .1;

	m_flEnemyFrozenUntil = .1;
	m_hEnemyFrozen = NULL;

	m_flSuspendGravityAt = .1;
	m_flSuspendGravityUntil = .1;

	m_bInFollowMode = m_bOldInFollowMode = false;
	m_hDirectTarget = NULL;
	m_hRecursedTarget = NULL;
	m_flNextRespawn = 0;
	m_flRespawnBonus = 0;
	m_bIsMenuOpen = false;

	m_bIsFuse = m_bBecomingFuse = false;
	m_flFuseStartTime = 0;
	m_bIsPariah = m_bBecomingPariah = false;
	m_flPariahStartTime = 0;

	SetViewOffset( CF_PLAYER_VIEW_OFFSET );

	m_flEloScore = CFGameRules()->GetAverageElo(this);

	for (int i = 0; i < 3; i++)
		m_hWeapons.Set( i, NULL );

	for (int i = 0; i < 3; i++)
		m_hAlternateWeapons.Set( i, NULL );

	m_pArmament = new CArmament();
	m_pCurrentArmament = NULL;
	m_pStats = new CStatistics(this);

	m_Randomness.SetSeed((int)this);	// Gotta use something. So long as it's not 0 I don't give a fuck.
}


CCFPlayer::~CCFPlayer()
{
	m_PlayerAnimState->Release();

	delete m_pArmament;
	if (m_pCurrentArmament)
		delete m_pCurrentArmament;
	delete m_pStats;
}

void CCFPlayer::ChangeTeam(int iTeam, bool bAutoTeam, bool bSilent)
{
	// if this is our current team, just abort
	if ( iTeam == GetTeamNumber() )
	{
		return;
	}

	// Remove any grenades the player has floating around.
	CMagicGrenade *pMagicGrenade = NULL;
	while ( (pMagicGrenade = dynamic_cast<CMagicGrenade*>( gEntList.FindEntityByClassname( pMagicGrenade, "magic_grenade" ) )) != NULL )
	{
		if (pMagicGrenade->GetThrower() && ToCFPlayer(pMagicGrenade->GetThrower()) == this)
			UTIL_Remove(pMagicGrenade);
	}

	CWeaponDrainGrenade *pDrainGrenade = NULL;
	while ( (pDrainGrenade = dynamic_cast<CWeaponDrainGrenade*>( gEntList.FindEntityByClassname( pDrainGrenade, "drain_grenade" ) )) != NULL )
	{
		if (pDrainGrenade->GetThrower() && ToCFPlayer(pDrainGrenade->GetThrower()) == this)
			UTIL_Remove(pDrainGrenade);
	}

	BaseClass::ChangeTeam(iTeam, bAutoTeam, bSilent);
}

bool CCFPlayer::ClientCommand( const CCommand &args )
{
	const char *pcmd = args[0];
	
	if ( FStrEq( pcmd, "menuopen" ) )
	{
		SetMenuOpen( true );
		return true;
	}
	else if ( FStrEq( pcmd, "menuclosed" ) )
	{
		SetMenuOpen( false );
		return true;
	}

	return BaseClass::ClientCommand( args );
}

CCFPlayer *CCFPlayer::CreatePlayer( const char *className, edict_t *ed )
{
	CCFPlayer::s_PlayerEdict = ed;
	return (CCFPlayer*)CreateEntityByName( className );
}

bool CCFPlayer::BumpWeapon( CBaseCombatWeapon *pWeapon )
{
	CBaseCombatCharacter *pOwner = pWeapon->GetOwner();
	CWeaponCFBase* pCFWeapon = dynamic_cast<CWeaponCFBase*>(pWeapon);

	// Can I have this weapon type?
	if ( !IsAllowedToPickupWeapons() )
		return false;

	if ( pOwner || !Weapon_CanUse( pWeapon ) || !g_pGameRules->CanHavePlayerItem( this, pWeapon ) )
	{
		if ( gEvilImpulse101 )
		{
			UTIL_Remove( pWeapon );
		}
		return false;
	}

	// Don't let the player fetch weapons through walls (use MASK_SOLID so that you can't pickup through windows)
	if( pWeapon->FVisible( this, MASK_SOLID ) == false && !(GetFlags() & FL_NOTARGET) )
		return false;

	if (pCFWeapon->GetWeaponID() == WEAPON_PARIAH)
		return CFGameRules()->MakePariah(this, dynamic_cast<CWeaponPariahBlade*>(pCFWeapon));

	AssertMsg(false, "Shouldn't be picking up weapons other than the Pariah blade.");

	return false;
}

void CCFPlayer::Weapon_Equip( CBaseCombatWeapon *pWeapon )
{
	// The old way of using weapons is obsolete. Use the version with iSlot now.
	AssertMsg(false, "Old CCFPlayer::Weapon_Equip() called. Not cool!");
}

void CCFPlayer::Weapon_Equip( CWeaponCFBase *pWeapon, int iSlot )
{
	if (GetWeapon(iSlot))
		return;

	SetWeapon( iSlot, pWeapon );

	pWeapon->SetPosition(iSlot);

	// Skip CBasePlayer
	CBaseCombatCharacter::Weapon_Equip( pWeapon );

	Weapon_CalcHands();

	if ( pWeapon->IsActive() )
		Weapon_Switch( pWeapon );
}

void CCFPlayer::Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget, const Vector *pVelocity )
{
	CWeaponCFBase* pCFWeapon = dynamic_cast<CWeaponCFBase*>(pWeapon);

	char iPosition = pCFWeapon->GetPosition();
	SetWeapon(iPosition, NULL);

	// Skip BasePlayer, it just does a bunch of GetActiveWeapon() stuff.
	CBaseCombatCharacter::Weapon_Drop( pWeapon, pvecTarget, pVelocity );
}

void CCFPlayer::Weapon_CalcHands()
{
	CWeaponCFBase* pPrimary = GetWeapon(0);
	if (ShouldPromoteSecondary())
		pPrimary = GetWeapon(SecondaryToPromote());

	bool bPrimaryHand = pPrimary && !pPrimary->GetCFWpnData().m_bPrimaryInLeft;
	if (pPrimary)
		pPrimary->SetHand(bPrimaryHand);

	CWeaponCFBase* pSecondary1 = GetWeapon(1);
	CWeaponCFBase* pSecondary2 = GetWeapon(2);

	if (ShouldPromoteSecondary() && SecondaryToPromote() == 1)
		pSecondary1 = NULL;
	if (ShouldPromoteSecondary() && SecondaryToPromote() == 2)
		pSecondary2 = NULL;

	if (pSecondary1)
		pSecondary1->SetHand(!bPrimaryHand);
	if (pSecondary2)
		pSecondary2->SetHand(!bPrimaryHand);
}

void CCFPlayer::PackDeadPlayerItems( void )
{
	int iWeaponRules;
	int iAmmoRules;
	int i;
	CBaseCombatWeapon *rgpPackWeapons[ 20 ];// 20 hardcoded for now. How to determine exactly how many weapons we have?
	int iPackAmmo[ MAX_AMMO_SLOTS + 1];
	int iPW = 0;// index into packweapons array
	int iPA = 0;// index into packammo array

	memset(rgpPackWeapons, NULL, sizeof(rgpPackWeapons) );
	memset(iPackAmmo, -1, sizeof(iPackAmmo) );

	// get the game rules 
	iWeaponRules = g_pGameRules->DeadPlayerWeapons( this );
 	iAmmoRules = g_pGameRules->DeadPlayerAmmo( this );

	if ( iWeaponRules == GR_PLR_DROP_GUN_NO && iAmmoRules == GR_PLR_DROP_AMMO_NO )
	{
		// nothing to pack. Remove the weapons and return. Don't call create on the box!
		RemoveAllItems( true );
		return;
	}

// go through all of the weapons and make a list of the ones to pack
	for ( i = 0 ; i < 3; i++ )
	{
		// there's a weapon here. Should I pack it?
		CBaseCombatWeapon *pPlayerItem = GetWeapon( i );
		if ( pPlayerItem )
		{
			switch( iWeaponRules )
			{
			case GR_PLR_DROP_GUN_ACTIVE:
				if ( (GetPrimaryWeapon() && pPlayerItem == GetPrimaryWeapon()) ||
					(GetSecondaryWeapon() && pPlayerItem == GetSecondaryWeapon()) )
				{
					// this is the active item. Pack it.
					rgpPackWeapons[ iPW++ ] = pPlayerItem;
				}
				break;

			case GR_PLR_DROP_GUN_ALL:
				rgpPackWeapons[ iPW++ ] = pPlayerItem;
				break;

			default:
				break;
			}
		}
	}

// now go through ammo and make a list of which types to pack.
	if ( iAmmoRules != GR_PLR_DROP_AMMO_NO )
	{
		for ( i = 0 ; i < MAX_AMMO_SLOTS ; i++ )
		{
			if ( GetAmmoCount( i ) > 0 )
			{
				// player has some ammo of this type.
				switch ( iAmmoRules )
				{
				case GR_PLR_DROP_AMMO_ALL:
					iPackAmmo[ iPA++ ] = i;
					break;

				case GR_PLR_DROP_AMMO_ACTIVE:
					// WEAPONTODO: Make this work
					/*
					if ( GetActiveWeapon() && i == GetActiveWeapon()->m_iPrimaryAmmoType ) 
					{
						// this is the primary ammo type for the active weapon
						iPackAmmo[ iPA++ ] = i;
					}
					else if ( GetActiveWeapon() && i == GetActiveWeapon()->m_iSecondaryAmmoType ) 
					{
						// this is the secondary ammo type for the active weapon
						iPackAmmo[ iPA++ ] = i;
					}
					*/
					break;

				default:
					break;
				}
			}
		}
	}

	RemoveAllItems( true );// now strip off everything that wasn't handled by the code above.
}

bool CCFPlayer::ToggleFollowMode()
{
	const char *pszToggle = "0";
	if (!IsBot())
		pszToggle = engine->GetClientConVarValue( entindex(), "cl_togglefollowmode" );
	return !!atoi(pszToggle);
}

bool CCFPlayer::AutoFollowMode()
{
	const char *pszAuto = "1";
	if (!IsBot())
		pszAuto = engine->GetClientConVarValue( entindex(), "cl_autofollowmode" );
	return !!atoi(pszAuto);
}

void CCFPlayer::StartAutoFollowMode()
{
	m_flAutoFollowModeEnds = gpGlobals->curtime + cvar->FindVar("mp_meleechaintime")->GetFloat();
}

bool CCFPlayer::CanFollowMode()
{
	if (!GetPrimaryWeapon())
		return false;

	if (AutoFollowMode())
	{
		if (gpGlobals->curtime < m_flAutoFollowModeEnds)
			return true;
	}

	if (ToggleFollowMode())
	{
		if (m_bInFollowMode)
		{
			if ((m_afButtonPressed & IN_ALT1) || !IsPhysicalMode())
				return false;
			else
				return true;
		}
		else
		{
			if ((m_afButtonPressed & IN_ALT1) && IsPhysicalMode())
				return true;
			else
				return false;
		}
	}
	else
	{
		if ((m_nButtons & IN_ALT1) && IsPhysicalMode())
			return true;
		else
			return false;
	}
}

void CCFPlayer::FindTarget(bool bAvoidTeammates, bool bNotBehind, bool bBackwards)
{
	Vector vecForward;
	EyeVectors( &vecForward );

	int iFirstClient = 0;

	// If we have a target, use him as the start so that we loop.
	if (GetDirectTarget() != NULL)
		iFirstClient = GetDirectTarget()->entindex();	// Don't subtract 1 so that we start with the next player instead.

	CCFPlayer* pBackup = NULL;

	int iClient = iFirstClient;
	while (true)
	{
		if (bBackwards)
			iClient -= 1;
		else
			iClient += 1;

		if (iClient >= gpGlobals->maxClients)
			iClient = 0;
		else if (iClient < 0)
			iClient = gpGlobals->maxClients - 1;

		if (iClient == iFirstClient)
			break;

		CBaseEntity *pEnt = UTIL_PlayerByIndex(iClient);

		if (!pEnt)
			continue;

		if (entindex() == pEnt->entindex())
			continue;

		CCFPlayer* pPlayer = ToCFPlayer(pEnt);

		// Only one unique target is allowed per follow mode activation.
		// If this isn't the guy we found already, wait for our real target
		// to show up.
		if (m_hFollowModeTarget != NULL && pPlayer != m_hFollowModeTarget)
			continue;

		if (bAvoidTeammates && CFGameRules()->PlayerRelationship(this, pPlayer) == GR_TEAMMATE)
		{
			if (!pBackup)
				pBackup = pPlayer;
			continue;
		}

		// Allow finding teammates through walls.
		if (CFGameRules()->PlayerRelationship(this, pPlayer) == GR_NOTTEAMMATE && !IsVisible(pPlayer))
			continue;

		// ... no point in breaking the target chain just to target something we already had targeted.
		if (GetRecursedTarget() == pPlayer)
			continue;

		if (!pPlayer->IsAlive())
			continue;

		Vector vecDir = pPlayer->GetCentroid() - GetCentroid();
		if (vecDir.LengthSqr() > 2048*2048)
			continue;

		// If he's really close, don't bother trying to find him inside the player's view cone.
		// This lets us accept players that are only just behind us, but that are possibly not
		// very friendly!
		if (!bNotBehind && vecDir.LengthSqr() > 80*80)
		{
			vecDir.NormalizeInPlace();

			if (DotProduct(vecDir, vecForward) < 0.7f)
			{
				if (!pBackup)
					pBackup = pPlayer;
				continue;
			}
		}

		SetDirectTarget(pPlayer);
		break;
	}

	if (pBackup && GetDirectTarget() == NULL)
		SetDirectTarget(pBackup);
}

CCFPlayer* CCFPlayer::FindClosestEnemy()
{
	Vector vecForward;
	EyeVectors( &vecForward );

	CCFPlayer* pClosest = NULL;

	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		CBaseEntity *pEnt = UTIL_PlayerByIndex(i);

		// No ghosts.
		if (!pEnt)
			continue;

		// Not myself.
		if (entindex() == pEnt->entindex())
			continue;

		CCFPlayer* pPlayer = ToCFPlayer(pEnt);

		// No friends.
		if (CFGameRules()->PlayerRelationship(this, pPlayer) == GR_TEAMMATE)
			continue;

		if (!IsVisible(pPlayer))
			continue;

		// No ghouls.
		if (!pPlayer->IsAlive())
			continue;

		// No far away people.
		Vector vecDir = pPlayer->GetCentroid() - GetCentroid();
		if (vecDir.LengthSqr() > 2048*2048)
			continue;

		// If he's really close, don't bother trying to find him inside the player's view cone.
		// This lets us accept players that are only just behind us, but that are possibly not
		// very friendly!
		if (vecDir.LengthSqr() > 80*80)
		{
			Vector vecDirNormalized = vecDir;
			vecDirNormalized.NormalizeInPlace();

			if (DotProduct(vecDirNormalized, vecForward) < 0.7f)
				continue;
		}

		if (!pClosest)
			pClosest = pPlayer;
		else if (vecDir.LengthSqr() < (pClosest->GetCentroid() - GetCentroid()).LengthSqr())
			pClosest = pPlayer;
	}

	return pClosest;
}

void CCFPlayer::SetDirectTarget( CCFPlayer* pTarget )
{
	if (GetDirectTarget() == pTarget)
		return;

	// If I am targeting someone who is already targeting this target,
	// I don't want to target him myself, I just want to kill him.
	if (GetRecursedTarget() == pTarget)
		return;

	if (pTarget == this)
		return;

	if (pTarget && !pTarget->CanBeTargeted())
		return;

	// Don't target dead people unless they are on my team.
	if (pTarget && (!pTarget->IsAlive() || !IsAlive()) && CFGameRules()->PlayerRelationship(this, pTarget) == GR_NOTTEAMMATE)
		return;

	// Can't if I'm too blind (or weak until blindness is in.)
	if (m_pStats->GetEffectFromBitmask(TARGETLOSS_STATUSEFFECT) > cvar->FindVar("mp_targetlossmagnitude")->GetFloat() && pTarget != NULL)
		return;

	CCFPlayer* pDirectTarget = GetDirectTarget();

	// If I am targeting a teammate who has no target, give him this target.
	// (This will recurse down to the last person on my team in the targeting
	// chain, and give that person this target.)
	if (pDirectTarget && CFGameRules()->PlayerRelationship(this, pDirectTarget) == GR_TEAMMATE && pDirectTarget->IsAlive() && !pDirectTarget->GetRecursedTarget() && pTarget != NULL)
	{
		pDirectTarget->SetDirectTarget(pTarget);
		return;
	}

	if (pDirectTarget)
		pDirectTarget->m_hTargeters.FindAndRemove(this);

	m_hDirectTarget = pTarget;

	if (GetDirectTarget())
		GetDirectTarget()->m_hTargeters.AddToTail(this);

	// Clear out anything that might be there right now.
	SetRecursedTarget(NULL);

	if (pTarget && pTarget->GetRecursedTarget() && pTarget->GetRecursedTarget() != this && CFGameRules()->PlayerRelationship(this, pTarget) == GR_TEAMMATE)
	{
		SetRecursedTarget(pTarget->GetRecursedTarget());
		// DevMsg("%s targets %s recurses to %s\n", GetPlayerName(), GetDirectTarget()->GetPlayerName(), GetRecursedTarget()->GetPlayerName());
	}
	else if (pTarget)
	{
		SetRecursedTarget(pTarget);
		// DevMsg("%s targets %s\n", GetPlayerName(), GetDirectTarget()->GetPlayerName());
	}

	m_bAutoTarget = false;

	if (IsVisible(pTarget))
		m_flLastSeenDirectTarget = gpGlobals->curtime;

	if (IsVisible(GetRecursedTarget()))
		m_flLastSeenRecursedTarget = gpGlobals->curtime;

	if (IsInFollowMode() && GetRecursedTarget() && CFGameRules()->PlayerRelationship(GetRecursedTarget(), this) == GR_NOTTEAMMATE)
		m_hFollowModeTarget = GetRecursedTarget();

	if (GetDirectTarget())
	{
		CSingleUserRecipientFilter filter( this );
		EmitSound(filter, entindex(), "HUD.DirectTargetAcquired");
	}
}

void CCFPlayer::SetRecursedTarget( CCFPlayer* pTarget, bool bSignal )
{
	MarkAllPlayers(false);

	SetRecursedTargetInternal(pTarget, bSignal);
}

void CCFPlayer::SetRecursedTargetInternal( CCFPlayer* pTarget, bool bSignal )
{
	// Prevent infinite recursion.
	if (IsMarked())
		return;

	Mark();

	if (GetRecursedTarget() == pTarget)
		return;

	if (this == pTarget)
		return;

	m_hRecursedTarget = pTarget;

	if (bSignal)
	{
		CSingleUserRecipientFilter filter( this );
		EmitSound(filter, entindex(), "HUD.RecursedTargetAcquired");
	}

	// If pTarget is NULL, just tell everybody that I should be their target.
	CCFPlayer* pGivenTarget;
	if (pTarget == NULL)
		pGivenTarget = this;
	else
		pGivenTarget = pTarget;

	// Loop through everybody who directly targets us and tell them they have a new recursed target.
	for (int i = 0; i < m_hTargeters.Count(); i++)
	{
		CCFPlayer* pPlayer = m_hTargeters[i];
		
		Assert(pPlayer);

		// Should always be valid, but it's better for it to not work then to crash.
		if (!pPlayer)
			continue;

		// Don't recurse to players on other teams.
		if (CFGameRules()->PlayerRelationship(this, pPlayer) == GR_NOTTEAMMATE)
			continue;

		pPlayer->SetRecursedTargetInternal(pGivenTarget, true);
	}
}

// Return true if me or any of my friends targeting me can see this player.
bool CCFPlayer::TargetersCanSee(CCFPlayer* pTarget)
{
	MarkAllPlayers(false);

	return TargetersCanSeeInternal(pTarget);
}

bool CCFPlayer::TargetersCanSeeInternal(CCFPlayer* pTarget)
{
	MDLCACHE_CRITICAL_SECTION();

	// Prevent infinite recursion.
	if (IsMarked())
		return false;

	Mark();

	if (IsAlive() && FVisible(pTarget, MASK_OPAQUE))
		return true;

	for (int i = 0; i < m_hTargeters.Count(); i++)
	{
		CCFPlayer* pPlayer = m_hTargeters[i];
		Assert(pPlayer);

		if (!pPlayer)
			continue;

		if (CFGameRules()->PlayerRelationship(this, pPlayer) == GR_NOTTEAMMATE)
			continue;

		if (pPlayer->TargetersCanSeeInternal(pTarget))
			return true;
	}

	return false;
}

// This poorly named function removes the player as a target from all the players who are targeting him.
void CCFPlayer::RemoveTargeters( bool bKeepSameTeam )
{
	int i;
	CUtlVector<CCFPlayer*> apRemove;

	// Build a list of people we must remove from our targeting list.
	// We can't just call pPlayer->SetDirectTarget(NULL) because that
	// call removes an entry from m_hTargeters, which fucks up our
	// iterating in this list.
	for (i = 0; i < m_hTargeters.Count(); i++)
	{
		CCFPlayer* pPlayer = m_hTargeters[i];
		Assert(pPlayer);

		if (!pPlayer)
			continue;

		if (bKeepSameTeam && CFGameRules()->PlayerRelationship(this, pPlayer) == GR_TEAMMATE)
			continue;

		apRemove.AddToTail(pPlayer);
	}

	// Iterate through our list and remove targets safely.
	for (i = 0; i < apRemove.Count(); i++)
	{
		apRemove[i]->SetDirectTarget(NULL);
	}
}

bool CCFPlayer::CanBeTargeted()
{
	if (!IsAlive() && gpGlobals->curtime > m_flDeathTime + 1)
		return false;

	return true;
}

void CCFPlayer::MarkAllPlayers(bool bMark)
{
	for (int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CCFPlayer *pPlayer = ToCFPlayer(UTIL_PlayerByIndex( i ));
		if (pPlayer)
			pPlayer->m_bMark = bMark;
	}
}

bool CCFPlayer::FVisible(CBaseEntity *pEntity, int iTraceMask, CBaseEntity **ppBlocker)
{
	if (pEntity->IsPlayer())
		return IsVisible(dynamic_cast<CCFPlayer*>(pEntity));
	else
		return CBasePlayer::FVisible(pEntity, iTraceMask, ppBlocker);
}

bool CCFPlayer::IsVisible( const Vector &pos, bool testFOV, const CBaseEntity *ignore ) const
{
	// is it in my general viewcone?
	if (testFOV && !const_cast<CCFPlayer*>(this)->FInViewCone( pos ))
		return false;

	// check line of sight
	// Must include CONTENTS_MONSTER to pick up all non-brush objects like barrels
	trace_t result;
	CTraceFilterNoNPCsOrPlayer traceFilter( ignore, COLLISION_GROUP_NONE );
	UTIL_TraceLine( const_cast<CCFPlayer*>(this)->EyePosition(), pos, MASK_OPAQUE, &traceFilter, &result );
	if (result.fraction != 1.0f)
		return false;

	return true;
}

bool CCFPlayer::IsVisible(CCFPlayer *pPlayer, bool testFOV, unsigned char *visParts) const
{
	if (!pPlayer)
		return false;

	// optimization - assume if center is not in FOV, nothing is
	// we're using WorldSpaceCenter instead of GUT so we can skip GetPartPosition below - that's
	// the most expensive part of this, and if we can skip it, so much the better.
	if (testFOV && !(const_cast<CCFPlayer*>(this)->FInViewCone( pPlayer->WorldSpaceCenter() )))
	{
		return false;
	}

	unsigned char testVisParts = VIS_NONE;

	// check gut
	Vector partPos = GetPartPosition( pPlayer, VIS_GUT );

	// finish gut check
	if (IsVisible( partPos, testFOV ))
	{
		if (visParts == NULL)
			return true;

		testVisParts |= VIS_GUT;
	}


	// check top of head
	partPos = GetPartPosition( pPlayer, VIS_HEAD );
	if (IsVisible( partPos, testFOV ))
	{
		if (visParts == NULL)
			return true;

		testVisParts |= VIS_HEAD;
	}

	// check feet
	partPos = GetPartPosition( pPlayer, VIS_FEET );
	if (IsVisible( partPos, testFOV ))
	{
		if (visParts == NULL)
			return true;

		testVisParts |= VIS_FEET;
	}

	// check "edges"
	partPos = GetPartPosition( pPlayer, VIS_LEFT_SIDE );
	if (IsVisible( partPos, testFOV ))
	{
		if (visParts == NULL)
			return true;

		testVisParts |= VIS_LEFT_SIDE;
	}

	partPos = GetPartPosition( pPlayer, VIS_RIGHT_SIDE );
	if (IsVisible( partPos, testFOV ))
	{
		if (visParts == NULL)
			return true;

		testVisParts |= VIS_RIGHT_SIDE;
	}

	if (visParts)
		*visParts = testVisParts;

	if (testVisParts)
		return true;

	return false;
}

Vector CCFPlayer::GetPartPosition(CCFPlayer* player, VisiblePartType part) const
{
	// which PartInfo corresponds to the given player
	PartInfo* info = &m_partInfo[ player->entindex() % MAX_PLAYERS ];

	if (gpGlobals->framecount > info->m_validFrame)
	{
		// update part positions
		const_cast<CCFPlayer*>(this)->ComputePartPositions( player );
		info->m_validFrame = gpGlobals->framecount;
	}

	// return requested part position
	switch( part )
	{
		default:
		{
			AssertMsg( false, "GetPartPosition: Invalid part" );
			// fall thru to GUT
		}

		case VIS_GUT:
			return info->m_gutPos;

		case VIS_HEAD:
			return info->m_headPos;
			
		case VIS_FEET:
			return info->m_feetPos;

		case VIS_LEFT_SIDE:
			return info->m_leftSidePos;
			
		case VIS_RIGHT_SIDE:
			return info->m_rightSidePos;
	}
}

CCFPlayer::PartInfo CCFPlayer::m_partInfo[ MAX_PLAYERS ];

//--------------------------------------------------------------------------------------------------------------
/**
 * Compute part positions from bone location.
 */
void CCFPlayer::ComputePartPositions( CCFPlayer *player )
{
	const int headBox = 1;
	const int gutBox = 3;
	const int leftElbowBox = 11;
	const int rightElbowBox = 9;
	//const int hipBox = 0;
	//const int leftFootBox = 4;
	//const int rightFootBox = 8;
	const int maxBoxIndex = leftElbowBox;

	// which PartInfo corresponds to the given player
	PartInfo *info = &m_partInfo[ player->entindex() % MAX_PLAYERS ];

	// always compute feet, since it doesn't rely on bones
	info->m_feetPos = player->GetAbsOrigin();
	info->m_feetPos.z += 5.0f;

	// get bone positions for interesting points on the player
	MDLCACHE_CRITICAL_SECTION();
	CStudioHdr *studioHdr = player->GetModelPtr();
	if (studioHdr)
	{
		mstudiohitboxset_t *set = studioHdr->pHitboxSet( player->GetHitboxSet() );
		if (set && maxBoxIndex < set->numhitboxes)
		{
			QAngle angles;
			mstudiobbox_t *box;

			// gut
			box = set->pHitbox( gutBox );
			player->GetBonePosition( box->bone, info->m_gutPos, angles );	

			// head
			box = set->pHitbox( headBox );
			player->GetBonePosition( box->bone, info->m_headPos, angles );

			Vector forward, right;
			AngleVectors( angles, &forward, &right, NULL );

			// in local bone space
			const float headForwardOffset = 4.0f;
			const float headRightOffset = 2.0f;
			info->m_headPos += headForwardOffset * forward + headRightOffset * right;

			/// @todo Fix this hack - lower the head target because it's a bit too high for the current T model
			info->m_headPos.z -= 2.0f;


			// left side
			box = set->pHitbox( leftElbowBox );
			player->GetBonePosition( box->bone, info->m_leftSidePos, angles );	

			// right side
			box = set->pHitbox( rightElbowBox );
			player->GetBonePosition( box->bone, info->m_rightSidePos, angles );	

			return;
		}
	}


	// default values if bones are not available
	info->m_headPos = player->GetCentroid();
	info->m_gutPos = info->m_headPos;
	info->m_leftSidePos = info->m_headPos;
	info->m_rightSidePos = info->m_headPos;
}

CCFPlayer* CCFPlayer::FindClosestFriend(float flMaxDistance, bool bFOV)
{
	CCFPlayer* pClosest = NULL;
	for (int i = 1; i < gpGlobals->maxClients; i++)
	{
		CCFPlayer* pPlayer = ToCFPlayer(UTIL_PlayerByIndex(i));

		if (!pPlayer)
			continue;

		if (pPlayer == this)
			continue;

		if (CFGameRules()->PlayerRelationship(this, pPlayer) == GR_NOTTEAMMATE)
			continue;

		if (!IsVisible(pPlayer, bFOV))
			continue;

		float flDistance = (GetAbsOrigin() - pPlayer->GetAbsOrigin()).Length();
		if (flDistance > flMaxDistance)
			continue;

		if (pClosest && (pClosest->GetAbsOrigin() - GetAbsOrigin()).Length() > flDistance)
			continue;

		pClosest = pPlayer;
	}

	return pClosest;
}

void CCFPlayer::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr )
{
	// Ignore damage from players if we are dying.
	// Reviving/fatality damage bypasses this by calling TakeDamage() and TakeHealth() directly.
	if (IsKnockedOut())
		return;

	Vector vecForward;
	EyeVectors(&vecForward);

	float flChance = m_pStats->GetBulletBlockRate();
	float flBulletBlockAngle = -0.75f;

	if ((inputInfo.GetDamageType()&DMG_BULLET) && DotProduct(vecDir, vecForward) < flBulletBlockAngle && m_Randomness.RandomFloat(0, 1) < flChance && CFGameRules()->PlayerRelationship(this, inputInfo.GetAttacker()) == GR_NOTTEAMMATE)
	{
		CWeaponCFBase* pWeapon = NULL;

		// Primary first, because with dual rivenblades, it is in front.
		if (CanBlockBulletsWithWeapon(GetPrimaryWeapon()))
			pWeapon = GetPrimaryWeapon();

		else if (CanBlockBulletsWithWeapon(GetSecondaryWeapon()))
			pWeapon = GetSecondaryWeapon();

		if (pWeapon)
		{
			pWeapon->BulletBlocked(inputInfo.GetDamageType());
			return;
		}
	}

	if ( m_takedamage )
	{
		CTakeDamageInfo info = inputInfo;

		if ( info.GetAttacker() )
		{
			// Prevent team damage here so blood doesn't appear
			if ( info.GetAttacker()->IsPlayer() )
			{
				if ( !CFGameRules()->FPlayerCanTakeDamage( this, info.GetAttacker(), info ) )
					return;
			}
		}

		SetLastHitGroup( ptr->hitgroup );
		m_nForceBone = ptr->physicsbone;		// save this bone for physics forces

		if ( info.GetDamage() > 0 && m_pStats->GetElementDefenseScale(inputInfo.GetElements()) > 0 )
		{
			SpawnBlood(ptr->endpos, vecDir, BloodColor(), info.GetDamage());// a little surface blood.
			TraceBleed( info.GetDamage(), vecDir, ptr, info.GetDamageType() );
		}

		AddMultiDamage( info, this );
	}
}

void CCFPlayer::SetLastAttacker( CCFPlayer* hLastAttacker )
{
	if(hLastAttacker != NULL)
	{
		if (hLastAttacker->IsPlayer() && hLastAttacker != this)
		{
			m_hLastAttacker = hLastAttacker;
			m_flLastAttackedTime = gpGlobals->curtime;
		}
	}
}

void CCFPlayer::SetLastDOTAttacker( CCFPlayer* hLastDOTAttacker )
{
	if(hLastDOTAttacker != NULL)
	{
		if (hLastDOTAttacker->IsPlayer() && hLastDOTAttacker != this)
		{
			m_hLastDOTAttacker = hLastDOTAttacker;
		}
	}
}

int CCFPlayer::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	CTakeDamageInfo info(inputInfo);

	// Prevent damage here instead of CBasePlayer::OnTakeDamage() for CFGameRules version of this function signature.
	if ( info.GetAttacker() && info.GetAttacker()->IsPlayer() )
	{
		if ( !CFGameRules()->FPlayerCanTakeDamage( this, info.GetAttacker(), info ) )
			return 0;
	}

	if (IsBecomingFuse() || IsBecomingPariah())
		return 0;

	if ( info.GetDamageType() & (DMG_DROWN | DMG_FALL) )
		return 0;

	info.AdjustPlayerDamageInflictedForStatistics();
	info.AdjustPlayerDamageTakenForStatistics(this);

	// Now that we know exactly how much damage is about to be done, do the draining.
	info.Drain();

	if (info.GetDamage() <= 0)
	{
		if (info.GetDamage() < 0)
			TakeHealth(-info.GetDamage(), 0);
		return 0;
	}

	if (!info.IsPhysical())
	{
		// Magical damage fucks up melee attacks pretty good.
		StopMeleeAttack();

		// If we have a sword, it fucks with our movement.
		if (GetPrimaryWeapon() && GetPrimaryWeapon()->IsMeleeWeapon())
		{
			if (info.GetStatusEffects() & STATUSEFFECT_SLOWNESS)
				FreezePlayer(0.2f, 0.5f);
			else
				FreezePlayer(0.6f, 0.5f);
		}

		// Screw with powerjumping.
		if (m_bPowerjump)
		{
			if (!((info.GetAttacker() == GetLastDOTAttacker()) && (info.GetElements() & ELEMENT_FIRE) && (info.GetAttacker() == info.GetInflictor())))
			{
				SetAbsVelocity(GetAbsVelocity()/2);
				SetGravity(1);
			}
		}

		bool bAttackerIsPlayer = false;
		if (info.GetAttacker() && info.GetAttacker()->IsPlayer())
			bAttackerIsPlayer = true;

		char* pszSound = "Numen.HitGeneric";
		CSoundParameters params;
		if ( GetParametersForSound( pszSound, params, NULL ) )
		{
			CPASAttenuationFilter filter( this, params.soundlevel );
			if (bAttackerIsPlayer)
				filter.RemoveRecipient(ToCFPlayer(info.GetAttacker()));
			EmitSound( filter, entindex(), pszSound ); 

			pszSound = "Numen.HitGenericAttacker";
			if (bAttackerIsPlayer)
			{
				CSingleUserRecipientFilter filter2(ToCFPlayer(info.GetAttacker()));
				EmitSound( filter2, entindex(), pszSound );
			}
		}
	}

	if (info.GetDamageType() & DMG_BULLET)
	{
		GetMagicWeapon()->StopAttack();
	}

	info.SetDamage(m_pStats->TakeDamage(info));

	int iDamage = BaseClass::OnTakeDamage( info );

	CCFPlayer* pAttacker = ToCFPlayer(info.GetAttacker());

	if (!((pAttacker == GetLastDOTAttacker()) && (info.GetElements() & ELEMENT_FIRE) && (pAttacker == info.GetInflictor())))
		StopRevive();

	if (info.GetStatusEffects() & STATUSEFFECT_DOT)
		SetLastDOTAttacker( pAttacker );

	SetLastAttacker( pAttacker );

	// Keep this after SetLastAttacker. Event_Killed uses it.
	if (IsKnockedOut())
	{
		if (GetHealth() < mp_deathhp.GetFloat())
		{
			Event_Killed( info, false );
		}
	}

	if (iDamage && pAttacker)
	{
		// Auto-target my attacker.
		SuggestRecursedTarget(pAttacker);

		// Have my attacker auto-target me.
		pAttacker->SuggestRecursedTarget(this);

		pAttacker->AwardEloPoints(SCORE_DAMAGE, this);
	}

	// Send the damage message to the client for the hud damage indicator
	// Don't do this for damage types that don't use the indicator
	if ( CFGameRules()->ShouldShowDamage(this, pAttacker) && iDamage && !(inputInfo.GetDamageType() & (DMG_DROWN | DMG_FALL | DMG_BURN) ) )
	{
		// Try and figure out where the damage is coming from
		Vector vecDamageDirection = info.GetReportedPosition();

		// If we didn't get an origin to use, try using the attacker's origin
		if ( vecDamageDirection == vec3_origin && info.GetInflictor() )
		{
			vecDamageDirection = info.GetInflictor()->GetAbsOrigin();
		}

		CSingleUserRecipientFilter filter( this );
		UserMessageBegin( filter, "Damage" );
			WRITE_BYTE( entindex() );
			WRITE_BYTE( (int)info.GetDamage() );
			WRITE_BYTE( false );
			WRITE_SHORT( info.GetElements() );
			WRITE_VEC3COORD( vecDamageDirection );
			WRITE_VEC3COORD( GetCentroid() );
		MessageEnd();
	}

	return iDamage;
}

int CCFPlayer::TakeHealth( float flHealth, int bitsDamageType )
{
	// Has a different name because bool bNotify has a default value and gives a compiler error for ambiguousness.
	// Is there a better way to do this? Probably.
	return CFTakeHealth(flHealth, bitsDamageType, true);
}

int CCFPlayer::CFTakeHealth( float flHealth, int bitsDamageType, bool bNotify )
{
	int iResult = BaseClass::TakeHealth (flHealth, bitsDamageType);

	if (bNotify)
	{
		CPVSFilter filter( GetAbsOrigin() );
		UserMessageBegin( filter, "Damage" );
			WRITE_BYTE( entindex() );
			WRITE_BYTE( (int)iResult );
			WRITE_BYTE( true );
			WRITE_SHORT( ELEMENT_TYPELESS );
			WRITE_VEC3COORD( Vector() );
			WRITE_VEC3COORD( GetCentroid() );
		MessageEnd();
	}

	return iResult;
}

void CCFPlayer::ShowDefense( CTakeDamageInfo* pDmg, float flScale )
{
	// Only show defense against physical if we have some kind of armor.
	if (flScale >= 1)
		return;

	CEffectData data;
	data.m_nEntIndex = entindex();
	data.m_nMaterial = pDmg->IsPhysical();
	data.m_nDamageType = pDmg->GetElements();
	data.m_flMagnitude = flScale;
	if (pDmg->GetInflictor())
	{
		if (pDmg->GetInflictor()->IsPlayer())
			data.m_vOrigin = ToCFPlayer(pDmg->GetInflictor())->GetCentroid();
		else
			data.m_vOrigin = pDmg->GetInflictor()->GetAbsOrigin();
	}
	else
		data.m_vOrigin = pDmg->GetDamagePosition();

	DispatchEffect("ShowDefense", data);
}

int CCFPlayer::OnTakeDamage_Dying( const CTakeDamageInfo &info )
{
	// do the damage
	if ( m_takedamage != DAMAGE_EVENTS_ONLY )
	{
		m_iHealth -= info.GetDamage();
	}

	return 1;
}

void CCFPlayer::SuggestRecursedTarget(CCFPlayer* pTarget)
{
	if (!pTarget)
		return;

	MarkAllPlayers(false);

	SuggestRecursedTargetInternal(pTarget);
}

void CCFPlayer::SuggestRecursedTargetInternal(CCFPlayer* pTarget)
{
	if (IsMarked())
		return;

	Mark();

	CCFPlayer* pDirect = GetDirectTarget();

	if (!pDirect)
		SetDirectTarget(pTarget);
		// If my direct target is on my team and has already been marked,
		// then we're looking at a circular team-only targeting chain, so we should
		// take the opportunity to pick up this target instead.
	else if (pDirect && CFGameRules()->PlayerRelationship(this, pDirect) == GR_TEAMMATE && pDirect->IsMarked())
		SetDirectTarget(pTarget);
	else if (pDirect && CFGameRules()->PlayerRelationship(this, pDirect) == GR_TEAMMATE)
		pDirect->SuggestRecursedTargetInternal(pTarget);
}

void CCFPlayer::LeaveVehicle( const Vector &vecExitPoint, const QAngle &vecExitAngles )
{
	BaseClass::LeaveVehicle( vecExitPoint, vecExitAngles );

	//teleport physics shadow too
	// Vector newPos = GetAbsOrigin();
	// QAngle newAng = GetAbsAngles();

	// Teleport( &newPos, &newAng, &vec3_origin );
}

void CCFPlayer::PreThink(void)
{
	if ( g_fGameOver )
		return;         // intermission or finale

	m_bPhysicalMode = IsPhysicalMode();

	if (IsBecomingFuse() && gpGlobals->curtime > m_flFuseStartTime + mp_fuse_start_time.GetFloat())
		m_bBecomingFuse = false;

	if (IsBecomingPariah() && gpGlobals->curtime > m_flPariahStartTime + mp_pariah_start_time.GetFloat())
		m_bBecomingPariah = false;

	if (IsVisible(GetDirectTarget()))
		m_flLastSeenDirectTarget = gpGlobals->curtime;

	if (IsVisible(GetRecursedTarget()))
		m_flLastSeenRecursedTarget = gpGlobals->curtime;

	if (AutoFollowMode() && m_flAutoFollowModeEnds)
	{
		// Disengage when time has run out.
		if (gpGlobals->curtime > m_flAutoFollowModeEnds)
			m_flAutoFollowModeEnds = 0;

		if (m_flAutoFollowModeEnds)
		{
			// If we are charging, extend the expiration time for the length of the charge.
			if (IsCharging())
				m_flAutoFollowModeEnds = gpGlobals->curtime + cvar->FindVar("mp_meleechaintime")->GetFloat();

			// Disengage if we lose our target.
			if (!GetRecursedTarget())
				m_flAutoFollowModeEnds = 0;
			// Disengage if our target becomes friendly, for some reason.
			else if (CFGameRules()->PlayerRelationship(GetRecursedTarget(), this) == GR_TEAMMATE)
				m_flAutoFollowModeEnds = 0;
			// Disengage if the player presses the follow mode button. (But only when he actually releases it... so he can hold it down to keep auto on.)
			else if ((m_afButtonReleased & IN_ALT1) && IsPhysicalMode())
				m_flAutoFollowModeEnds = 0;
		}
	}

	if (GetDirectTarget() && !GetDirectTarget()->CanBeTargeted())
		SetDirectTarget(NULL);

	if (!GetDirectTarget())
	{
		// Only pick up people closeby, encourage sticking together!
		CCFPlayer* pClosest = FindClosestFriend(200, true);

		if (pClosest)
		{
			SetDirectTarget(pClosest);
			m_bAutoTarget = true;
		}
	}

	if (GetDirectTarget() && m_bAutoTarget)
	{
		CCFPlayer* pClosest = FindClosestFriend(200, true);

		// If our friend has wandered away and there is another friend much closer, switch over to him.
		float flDistanceToTarget = (GetAbsOrigin() - GetDirectTarget()->GetAbsOrigin()).Length();
		if (GetDirectTarget() != pClosest && pClosest && flDistanceToTarget > 400 && (GetAbsOrigin() - pClosest->GetAbsOrigin()).Length() < flDistanceToTarget/4)
		{
			SetDirectTarget(pClosest);
			m_bAutoTarget = true;
		}
		// Disengage if our target wanders far away.
		else if (flDistanceToTarget > 800)
			SetDirectTarget(NULL);
		// Disengage if it's been more than three seconds since we've had LOS to our target.
		else if (gpGlobals->curtime > m_flLastSeenDirectTarget + 3)
			SetDirectTarget(NULL);
	}

	// If he doesn't stick his head up again in 3 seconds, give up on him.
	if (gpGlobals->curtime > m_flLastSeenDirectTarget + 3)
		m_hFollowModeTarget = NULL;

	FollowMode();

	LatchThink();

	// Riding a vehicle?
	if ( IsInAVehicle() )	
	{
		// make sure we update the client, check for timed damage and update suit even if we are in a vehicle
		UpdateClientData();		
		CheckTimeBasedDamage();

		// Allow the suit to recharge when in the vehicle.
		CheckSuitUpdate();
		
		WaterMove();	
		return;
	}

	if (m_lifeState == LIFE_ALIVE)
		m_pStats->StatisticsThink();

	BaseClass::PreThink();

	if (m_hReviving != NULL && m_bReviving)
		m_hReviving->TakeHealth(1, 0);	// Overflow with the healing effect.

	//If we're reviving/fataliting, if the current time has extended past the duration of a fatality -> Stop.  
	if (m_hReviving != NULL && gpGlobals->curtime >= m_flFatalityStart + FATALITY_TIME)
		StopFatality(true);

	if (gpGlobals->curtime > m_flNextRevive)
		m_flNextRevive = gpGlobals->curtime + 0.1f;

	CalculateHandMagic();

	if (m_lifeState >= LIFE_DYING)
		return;

	if (m_pStats->IsInOverdrive() && m_pStats->IsOverdriveTimeExpired())
		m_pStats->ResetOverdrive();

	CCFPlayer* pTarget = GetDirectTarget();
	if (pTarget && CFGameRules()->PlayerRelationship(this, pTarget) == GR_NOTTEAMMATE)
	{
		// If we can't see the guy anymore, we've lost our lock.
		if ( !TargetersCanSee( pTarget ) )
		{
			SetDirectTarget(NULL);
		}
	}

	//Reset the last known attacker if the attack occurred more than 15 seconds ago.
	if (GetLastAttacker() != NULL && m_flLastAttackedTime < (gpGlobals->curtime - 15.0f))
	{
		m_hLastAttacker = NULL;
	}
}


void CCFPlayer::PostThink()
{
	MDLCACHE_CRITICAL_SECTION();

	if ( !g_fGameOver && !IsPlayerLockedInPlace() && IsAlive() && (GetFlags() & FL_ONGROUND) && m_bPowerjump )
		StopPowerjump();

	// Needs to happen before ItemPostFrame where the melee calculations are done.
	QAngle angles = GetLocalAngles();
	angles[PITCH] = 0;
	SetLocalAngles( angles );
	
	BaseClass::PostThink();

	// Store the eye angles pitch so the client can compute its animation state correctly.
	if (m_hReviver == NULL && m_hReviving == NULL)
		m_angEyeAngles = EyeAngles();
	else
	{
		Vector vecTarget;
		if (m_hReviving != NULL)
			vecTarget = m_hReviving->GetAbsOrigin() - GetAbsOrigin();
		else if (m_hReviver != NULL)
			vecTarget = m_hReviver->GetAbsOrigin() - GetAbsOrigin();
		VectorAngles(vecTarget, m_angEyeAngles.GetForModify());
		m_angEyeAngles.SetX(0);
		m_angEyeAngles.SetZ(0);
	}

    m_PlayerAnimState->Update( m_angEyeAngles[YAW], m_angEyeAngles[PITCH] );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CCFPlayer::IsReadyToPlay( void )
{
	if ( GetTeamNumber() <= LAST_SHARED_TEAM )
		return false;
	
	if ( IsMenuOpen() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CCFPlayer::IsReadyToSpawn( void )
{
	if ( IsMenuOpen() )
		return false;

	if (gpGlobals->curtime < m_flNextRespawn)
		return false;

	if (gpGlobals->curtime < (GetDeathTime() + cvar->FindVar("mp_respawntimer")->GetFloat()))
		return false;

	return ( !IsKnockedOut() );
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this player should be allowed to instantly spawn
//			when they next finish arming themselves.
//-----------------------------------------------------------------------------
bool CCFPlayer::ShouldGainInstantSpawn( void )
{
	return ( IsMenuOpen() );
}

bool CCFPlayer::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	if ( collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT )
		return false;

	return BaseClass::ShouldCollide( collisionGroup, contentsMask );
}

void CCFPlayer::Touch( CBaseEntity *pOther )
{
	SharedTouch(pOther);
}

int CCFPlayer::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	CBaseEntity *pRecipientEntity = CBaseEntity::Instance( pInfo->m_pClientEnt );
	Assert( pRecipientEntity->IsPlayer() );
	CCFPlayer *pRecipientPlayer = static_cast<CCFPlayer*>( pRecipientEntity );

	// Always transmit my targets to me so the targeting reticles are updated.
	// If the target is obscured then it is removed elsewhere.
	if ( pRecipientPlayer->GetRecursedTarget() == this )
		return FL_EDICT_ALWAYS;

	if ( pRecipientPlayer->GetDirectTarget() == this )
		return FL_EDICT_ALWAYS;

	// Always transmit the Fuse to his teammates so that they can see where he is through the objective indicators.
	if ( IsFuse() && CFGameRules()->PlayerRelationship(this, pRecipientPlayer) == GR_TEAMMATE )
		return FL_EDICT_ALWAYS;

	return BaseClass::ShouldTransmit(pInfo);
}

void CCFPlayer::Precache()
{
	PrecacheModel( PLAYER_MODEL_MACHINDO );
	PrecacheModel( PLAYER_MODEL_NUMENI );
	PrecacheModel( PLAYER_MODEL_PARIAH );
	PrecacheModel( PLAYER_MODEL_SHADE );

	PrecacheModel( "models/player/machindomale_head.mdl" );
	PrecacheModel( "models/player/numenimale_head.mdl" );

	PrecacheModel( "models/magic/barrier.mdl" );

	PrecacheParticleSystem( "sword_charge" );
	PrecacheParticleSystem( "latch_debris" );
	PrecacheParticleSystem( "powerjump" );
	PrecacheParticleSystem( "overdrive" );
	PrecacheParticleSystem( "critical" );
	PrecacheParticleSystem( "fuse" );
	PrecacheParticleSystem( "pariah" );
	PrecacheParticleSystem( "muzzle_flash" );
	PrecacheParticleSystem( "healed" );
	PrecacheParticleSystem( "focusdrained" );
	PrecacheParticleSystem( "focusshot" );
	PrecacheParticleSystem( "kod" );
	PrecacheParticleSystem( "block" );
	PrecacheParticleSystem( "land_dust" );

	PrecacheScriptSound( "Numeni.RoundStart" );
	PrecacheScriptSound( "Machindo.RoundStart" );

	BaseClass::Precache();
}

void CCFPlayer::Spawn()
{
	MDLCACHE_CRITICAL_SECTION();	// For SetModel and DoAnimationEvent

	SetModel( GetCFModelName() );

	m_nSkin = m_iDesiredSkin;
	m_bIsDecapitated = false;

	SetBodygroup(1, 0);
	SetBodygroup(2, 0);
	SetBodygroup(3, 0);

	SetMoveType( MOVETYPE_WALK );
	RemoveSolidFlags( FSOLID_NOT_SOLID );

	// Force this to clear out.
	SetGroundEntity(NULL);

	BaseClass::Spawn();

	if (ShouldPromoteSecondary())
		m_iActiveSecondary = SecondaryToPromote(true);
	else
		m_iActiveSecondary = 1;	// Should default to the first secondary.

	// Remove all enemy targeters.
	RemoveTargeters(true);
	m_bAutoTarget = false;
	m_flLastSeenDirectTarget = 0;
	m_flLastSeenRecursedTarget = 0;

	m_flAutoFollowModeEnds = 0;

	m_iDirection = 0;

	for( int p=0; p<MAX_PLAYERS; ++p )
	{
		m_partInfo[p].m_validFrame = 0;
	}

	CalculateMovementSpeed();

	DoAnimationEvent( PLAYERANIMEVENT_SPAWN );

	UnlockPlayer();
	m_flNextRevive = 0;

	// Prevent firing for a second so players don't blow their faces off
	SetNextAttack( gpGlobals->curtime + 1.0 );
	
	CSingleUserRecipientFilter filter( this );
	filter.MakeReliable();

	// DO IT
	UserMessageBegin( filter, "Respawn" );
	MessageEnd();
}

void CCFPlayer::InitialSpawn( void )
{
	BaseClass::InitialSpawn();

	m_bPhysicalMode = true;
	m_iDesiredSkin = 0;

	ChangeTeam(TEAM_SPECTATOR);
}

extern CBaseEntity* g_pLastSpawn;
extern CBaseEntity *FindPlayerStart(const char *pszClassName);

CBaseEntity *CCFPlayer::EntSelectSpawnPoint()
{
	CBaseEntity *pSpot;
	edict_t		*player;

	player = edict();

	if ( GetTeamNumber() < FIRST_GAME_TEAM )
	{
		pSpot = FindPlayerStart( "info_player_start" );
		if ( pSpot )
			return pSpot;
	}

	char* pszSpawnEntName = "info_player_teamspawn";

	pSpot = g_pLastSpawn;
	// Randomize the start spot
	for ( int i = random->RandomInt(1,5); i > 0; i-- )
		pSpot = gEntList.FindEntityByClassname( pSpot, pszSpawnEntName );
	if ( !pSpot )  // skip over the null point
		pSpot = gEntList.FindEntityByClassname( pSpot, pszSpawnEntName );

	CBaseEntity* pFirstSpot = pSpot;
	CBaseEntity* pDesiredSpot = NULL;

	while ((pSpot = gEntList.FindEntityByClassname( pSpot, pszSpawnEntName )) != pFirstSpot)
	{
		if ( !pSpot )
			continue;

		// If it doesn't pass g_pGameRules->IsSpawnPointValid() we can still use it as a fallback in case no points pass.
		if (!pDesiredSpot && pSpot->GetTeamNumber() == GetTeamNumber())
			pDesiredSpot = pSpot;

		// check if pSpot is valid
		if ( g_pGameRules->IsSpawnPointValid( pSpot, this ) )
		{
			if ( pSpot->GetLocalOrigin() == vec3_origin )
			{
				pSpot = gEntList.FindEntityByClassname( pSpot, pszSpawnEntName );
				continue;
			}

			// if so, go to pSpot
			pDesiredSpot = pSpot;
			break;
		}
	}

	if (!pDesiredSpot)
	{
		Warning( "PutClientInServer: no info_player_teamspawn on level\n");
		return CBaseEntity::Instance( INDEXENT( 0 ) );
	}

	g_pLastSpawn = pDesiredSpot;
	return pDesiredSpot;
}

void CCFPlayer::PlayerDeathThink(void)
{
	SetNextThink( gpGlobals->curtime + 0.1f );

	float flRespawnTimer = 0;
	if (cvar->FindVar("mp_respawntimer"))
		flRespawnTimer = cvar->FindVar("mp_respawntimer")->GetFloat();

	if (IsKnockedOut())
		m_flRespawnBonus = RemapValClamped(gpGlobals->curtime, m_flDeathTime + flRespawnTimer, m_flDeathTime + flRespawnTimer + mp_hpbonustime.GetFloat(), 0, mp_hpbonusamnt.GetFloat());

	if (m_iHealth >= m_iMaxHealth/3)
	{
		Event_Revived();
		return;
	}

	if (m_iHealth > mp_deathhp.GetFloat())
		return;

	BaseClass::PlayerDeathThink();
}

void CCFPlayer::BecomeFuse()
{
	m_pStats->ResetOverdrive();

	StopMeleeAttack();

	FreezePlayer(0, mp_fuse_start_time.GetFloat());

	m_flNextAttack = gpGlobals->curtime + mp_fuse_start_time.GetFloat();

	m_bIsFuse = m_bBecomingFuse = true;
	m_flFuseStartTime = gpGlobals->curtime;

	if (GetPrimaryWeapon())
		GetPrimaryWeapon()->DisableForSeconds(mp_fuse_start_time.GetFloat());
	if (GetSecondaryWeapon())
		GetSecondaryWeapon()->DisableForSeconds(mp_fuse_start_time.GetFloat());

	SetBodygroup(2, 1);

	EmitSound("Player.BecomeFuse");
}

void CCFPlayer::DemoteFuse()
{
	if (!IsFuse())
		return;

	m_bIsFuse = m_bBecomingFuse = false;
	m_flFuseStartTime = 0;

	SetBodygroup(2, 0);
}

void CCFPlayer::BecomePariah(CWeaponPariahBlade* pBlade)
{
	RemoveTargeters(true);
	m_pStats->ResetOverdrive();

	StopMeleeAttack();

	FreezePlayer(0, mp_pariah_start_time.GetFloat());

	m_flNextAttack = gpGlobals->curtime + mp_fuse_start_time.GetFloat();

	// Put away all existing weapons, don't delete them though.
	for (int i = 0; i < m_hWeapons.Count(); i++)
		if (GetWeapon(i))
			GetWeapon(i)->Holster();

	m_bIsPariah = m_bBecomingPariah = true;
	m_flPariahStartTime = gpGlobals->curtime;

	SetModel( GetCFModelName() );

	pBlade->AddSolidFlags( FSOLID_NOT_SOLID );
	pBlade->AddEffects( EF_NODRAW );

	Weapon_Equip( pBlade, 0 );

	// Switch over to the Pariah's armament.
	GetActiveArmament()->Buy(this, false);

	if (GetPrimaryWeapon())
		GetPrimaryWeapon()->DisableForSeconds(mp_fuse_start_time.GetFloat());
	if (GetSecondaryWeapon())
		GetSecondaryWeapon()->DisableForSeconds(mp_fuse_start_time.GetFloat());

	EmitSound("Player.BecomePariah");

	CSingleUserRecipientFilter filter(this);
	EmitSound(filter, entindex(), "CFMusic.Pariah");
}

void CCFPlayer::DemotePariah()
{
	int i;

	if (!IsPariah())
		return;

	SetModel( GetCFModelName() );

	for (i = 0; i < 3; i++)
	{
		CWeaponCFBase* pCFWeapon = GetWeapon(i);

		if (!pCFWeapon)
			continue;

		CWeaponPariahBlade* pWeapon = dynamic_cast<CWeaponPariahBlade*>(pCFWeapon);

		if (!pWeapon)
			continue;

		Weapon_Drop(pWeapon);
	}

	m_bIsPariah = m_bBecomingPariah = false;
	m_flPariahStartTime = 0;

	SetModel( GetCFModelName() );

	// Switch over to the normal armament.
	GetActiveArmament()->Buy(this, false);

	for (i = 0; i < m_hWeapons.Count(); i++)
		if (GetWeapon(i))
			GetWeapon(i)->Deploy();

	StopSound("CFMusic.Pariah");
}

void CCFPlayer::BecomeNormal()
{
	CFGameRules()->RemoveFuse(this);
	CFGameRules()->RemovePariah(this);
}

void CCFPlayer::PromoteToCaptain()
{
	m_bIsCaptain = true;
	m_bIsSergeant = false;

	SetBodygroup(3, 1);

	CSingleUserRecipientFilter filter(this);
	filter.MakeReliable();
	EmitSound(filter, entindex(), "CFMusic.Promoted");
}

void CCFPlayer::PromoteToSergeant()
{
	m_bIsSergeant = true;
	m_bIsCaptain = false;

	SetBodygroup(3, 2);

	CSingleUserRecipientFilter filter(this);
	filter.MakeReliable();
	EmitSound(filter, entindex(), "CFMusic.Promoted");
}

void CCFPlayer::DemoteRank()
{
	m_bIsCaptain = m_bIsSergeant = false;

	SetBodygroup(3, 0);
}

void CCFPlayer::ReviveUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (!pActivator->IsPlayer())
		return;

	CCFPlayer *pOther = ToCFPlayer(pActivator);

	if (CFGameRules()->PlayerRelationship(this, pOther) == GR_NOTTEAMMATE)
	{
		if (useType == USE_ON && IsKnockedOut())
			pOther->StartFatality(this);
	}
	else
	{
		if (useType == USE_ON && IsKnockedOut())
			pOther->StartRevive(this);
		else
			pOther->StopRevive();
	}
}

void CCFPlayer::StartRevive(CCFPlayer *pPlayer)
{
	if (!IsAlive())
		return;

	//Ensure that we're not stealing someone else's revive.
	if (m_hReviver != NULL)
		return;

	// If for some reason player is currently doing a fatality, clear it out.
	if (!m_bReviving && m_hReviving != NULL)
		StopFatality();

	// If the player is reviving someone else, clear it out.
	if (m_hReviving != NULL)
		StopRevive();

	LockPlayerInPlace();
	pPlayer->m_hReviver = this;
	m_hReviving = pPlayer;
	m_bReviving = true;
	pPlayer->m_bReviving = true;
	m_flFatalityStart = gpGlobals->curtime;
}

void CCFPlayer::StopRevive()
{
	if (m_hReviving == NULL)
		return;

	// So you can call StopRevive() whether reviving or fatality-ing and it will always do the right thing.
	if (!m_bReviving)
	{
		StopFatality();
		return;
	}

	if (m_hReviving->IsAlive())
		return;

	UnlockPlayer();

	if (m_hReviving->IsKnockedOut() && gpGlobals->curtime >= m_flFatalityStart + FATALITY_TIME)
	{
		m_hReviving->TakeHealth(m_hReviving->GetMaxHealth()/3 - m_hReviving->GetHealth() + m_hReviving->m_flRespawnBonus, 0);
		m_hReviving->Event_Revived();

		Instructor_LessonLearned(HINT_E_REVIVE, true);

		AwardEloPoints(SCORE_REVIVAL);
	}

	m_hReviving->m_hReviver = NULL;
	m_hReviving = NULL;
}

// FINISH HIM!!!
void CCFPlayer::StartFatality(CCFPlayer *pPlayer)
{
	if (!IsAlive())
		return;

	if (m_hReviving != NULL)
		return;

	//Ensure that we're not stealing someone else's fatality.
	if (pPlayer->m_hReviver != NULL)
		return;

	// If for some reason player is currently reviving someone, clear it out.
	if (m_bReviving && m_hReviving != NULL)
		StopRevive();

	// If the player is already doing a fatality on someone else, clear it out.
	if (m_hReviving.Get() != NULL && m_hReviving.Get() != pPlayer)
		StopFatality();

	m_bReviving = false;
	pPlayer->m_bReviving = false;
	pPlayer->m_hReviver = this;
	m_hReviving = pPlayer;
	m_flFatalityStart = gpGlobals->curtime;

	SetMoveType( MOVETYPE_NONE );

	Vector vecTarget = m_hReviving->GetAbsOrigin() - GetAbsOrigin();
	QAngle angToVictim;
	VectorAngles(vecTarget, angToVictim);

	// Face the combatants towards each other.
	m_PlayerAnimState->OverrideEyeYaw(angToVictim.y);
	m_hReviving->m_PlayerAnimState->OverrideEyeYaw(180+angToVictim.y);

	Activity iExecution = m_PlayerAnimState->TranslateActivity(ACT_CF_EXECUTE_ANYTHING);
	DoAnimationEvent( PLAYERANIMEVENT_EXECUTE, iExecution );
	m_hReviving->DoAnimationEvent( PLAYERANIMEVENT_EXECUTED, iExecution+1 );	// Kinda hacky.
}

void CCFPlayer::StopFatality(bool bKill)
{
	if (m_hReviving == NULL)
		return;

	// So you can call StopFatality() whether reviving or fatality-ing and it will always do the right thing.
	if (m_bReviving)
	{
		StopRevive();
		return;
	}

	if (m_hReviving->IsKnockedOut() && bKill)
	{
		CCFPlayer* pReviving = m_hReviving;

		// You attack his weak point for MASSIVE DAMAGE!!!
		CTakeDamageInfo info(this, this, m_hReviving->m_iMaxHealth*2, 0, 0);
		pReviving->OnTakeDamage( info );

		// Make sure they go into observer mode before we can fatality them again.
		pReviving->PlayerDeathThink();

		Instructor_LessonLearned(HINT_E_FATALITY, true);

		AwardEloPoints(SCORE_FATALITY);
	}
	else
	{
		// Get rid of the animation
		DoAnimationEvent(PLAYERANIMEVENT_CUSTOM_SEQUENCE, -1);
		m_hReviving->DoAnimationEvent(PLAYERANIMEVENT_CUSTOM_SEQUENCE, -1);
	}

	SetMoveType( MOVETYPE_WALK );

	// May have been done already by a recursive call to StopFatality() inside Player_Killed().
	if (m_hReviving != NULL)
	{
		m_hReviving->m_hReviver = NULL;
		m_hReviving = NULL;
	}
}

void CCFPlayer::Decapitate( )
{
	// Remove the head.
	m_bIsDecapitated = true;
	SetBodygroup(1, 1);
}

void CCFPlayer::Event_Revived( )
{
	m_lifeState = LIFE_ALIVE;
	m_takedamage = DAMAGE_YES;
	pl.deadflag = false;

	SetNextThink( TICK_NEVER_THINK );
	SetMoveType( MOVETYPE_WALK );
	SetSolid( SOLID_BBOX );
	RemoveSolidFlags( FSOLID_NOT_SOLID );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetFriction( 1.0f );
	SetUse( NULL );

	m_flRespawnBonus = 0;

	m_pStats->ResetOverdrive();

	if (m_hReviver != NULL)
		m_hReviver->StopRevive();

	IPhysicsObject *pObject = VPhysicsGetObject();
	if ( pObject )
	{
		pObject->RecheckContactPoints();
	}

	m_nRenderFX = kRenderFxNone;

	if ( GetPrimaryWeapon() )
		GetPrimaryWeapon()->Deploy();

	if ( GetSecondaryWeapon() )
		GetSecondaryWeapon()->Deploy();

	CPASAttenuationFilter filter( this );
	CBaseEntity::EmitSound( filter, entindex(), "Player.ReviveBreath" );

	// Make sure we don't do a stats think immediately and jump up 50hp instantaneously.
	m_pStats->m_flNextStatsThink = gpGlobals->curtime + STATISTICS_FRAMETIME;
}

void CCFPlayer::Event_Killed( const CTakeDamageInfo &info )
{
	Event_Knockout(info);
}

void CCFPlayer::Event_Killed( const CTakeDamageInfo &info, bool bKnockout )
{
	if (!bKnockout)
	{
		if (m_hReviver != NULL)
		{
			IGameEvent * event = gameeventmanager->CreateEvent( "player_death" );
			if ( event )
			{
				event->SetInt("userid", GetUserID() );
				event->SetInt("attacker", m_hReviver->GetUserID() );
				event->SetString("weapon", "fatality" );
				event->SetInt("priority", 7 );	// HLTV event priority, not transmitted
				event->SetInt("numen", 0 );	// HLTV event priority, not transmitted

				gameeventmanager->FireEvent( event );
			}

			// I swear there needs to be a global register for this.
			int iClients = 0;
			for (int i = 0; i < gpGlobals->maxClients; i++)
				if (UTIL_PlayerByIndex(i))
					iClients++;

			// Make the fatality respawn penalty smaller if there's less than eight people in the game.
			m_flNextRespawn = gpGlobals->curtime + RemapValClamped(iClients, 4, 8, 5, mp_fatalityrespawntimer.GetFloat());

			// Just in case.
			m_hReviver->StopFatality();
		}

		Vector vecForce = CalcDamageForceVector( info );

		// Drop any weapon that I own
		for (int i = 0; i < 3; i++)
		{
			CWeaponCFBase* pWeapon = GetWeapon(i);

			if (!pWeapon)
				continue;

			if ( VPhysicsGetObject() )
			{
				Vector weaponForce = vecForce * VPhysicsGetObject()->GetInvMass();
				Weapon_Drop( pWeapon, NULL, &weaponForce );
			}
			else
			{
				Weapon_Drop( pWeapon );
			}
		}

		CreateRagdollEntity();

		m_pStats->Event_Killed(info);

		// Whoever fatalitied me or attacked me last (in case of extreme damage instant death)
		// is who I am most interested in watching while spectating.
		SetObserverTarget(GetLastAttacker());

		return;
	}

	float flVengeanceRate = m_pStats->GetVengeanceRate();
	if (flVengeanceRate && random->RandomFloat(0, 1) < flVengeanceRate)
	{
		// Not so fast...
		CWeaponMagic::Explode( this, this, this, GetAbsOrigin(),
			m_pStats->GetVengeanceDamage(),
			m_pStats->GetVengeanceDamage(),
			m_pStats->GetVengeanceElement(),
			m_pStats->GetVengeanceStatusEffect(),
			m_pStats->GetVengeanceStatusMagnitude(),
			FE_EXPLODE,
			GetActiveArmament()->SerializeCombo(m_pStats->GetVengeance().m_iWeapon, m_pStats->GetVengeance().m_iRune),
			0, 0, 0);
		// TAKE THAT BITCHES!!!
	}

	StopLatch();
	StopPowerjump();
	StopRevive();

	if ( GetPrimaryWeapon() )
	{
		GetPrimaryWeapon()->Holster();
	}

	if ( GetSecondaryWeapon() )
	{
		GetSecondaryWeapon()->Holster();
	}

	DoAnimationEvent( PLAYERANIMEVENT_DIE );

	// Note: since we're dead, it won't draw us on the client, but we don't set EF_NODRAW
	// because we still want to transmit to the clients in our PVS.

	CCFPlayer* pTarget = GetDirectTarget();
	if (pTarget && CFGameRules()->PlayerRelationship(this, pTarget) == GR_NOTTEAMMATE)
	{
		// If the player dies, he will lose any enemy he has targeted.
		SetDirectTarget(NULL);
	}

	UnlockPlayer();

	m_pStats->Event_Knockout(info);

	// Send info on this player to the victim so that it can be displayed in the killer info panel.
	if ( info.GetAttacker()->IsPlayer() )
		SendPlayerInfo( ToCFPlayer(info.GetAttacker()) );
	else
		SendPlayerInfo( NULL );

	BaseClass::Event_Killed( info );
}

void CCFPlayer::Event_Dying( )
{
	SetUse(&CCFPlayer::ReviveUse);

	BaseClass::Event_Dying( );
}

// Override CBasePlayer::DeathSound to get rid of that horrible flatline sound.
void CCFPlayer::DeathSound( const CTakeDamageInfo &info )
{
	// Did we die from falling?
	if ( m_bitsDamageType & DMG_FALL )
	{
		// They died in the fall. Play a splat sound.
		EmitSound( "Player.FallGib" );
	}
	else
	{
		EmitSound( "Player.Death" );
	}
}

void CCFPlayer::CommitSuicide( bool bExplode /*= false*/, bool bForce /*= false*/ )
{
	// Code stolen from CBasePlayer::CommitSuicide()
	MDLCACHE_CRITICAL_SECTION();

	if( !IsAlive() && !IsKnockedOut() )
		return;
		
	if (GetTeamNumber() == TEAM_UNASSIGNED || GetTeamNumber() == TEAM_SPECTATOR)
		return;

	// prevent suiciding too often
	if ( m_fNextSuicideTime > gpGlobals->curtime && !bForce )
		return;

	// don't let them suicide for 5 seconds after suiciding
	m_fNextSuicideTime = gpGlobals->curtime + 5;

	int fDamage = DMG_PREVENT_PHYSICS_FORCE | ( bExplode ? ( DMG_BLAST | DMG_ALWAYSGIB ) : DMG_NEVERGIB );

	// have the player kill themself with enough damage to make sure he can't be revived.
	CTakeDamageInfo info( this, this, GetHealth() + mp_deathhp.GetFloat() + 1, fDamage );
	m_iHealth = mp_deathhp.GetFloat() - 1;
	Event_Killed( info );
	Event_Killed( info, false );
	Event_Dying( );
	PlayerDeathThink();
	m_iSuicideCustomKillFlags = 0;
}

bool CCFPlayer::IsCharging()
{
	CWeaponCFBase* pPrimaryWeapon = GetPrimaryWeapon();
	CWeaponCFBase* pSecondaryWeapon = GetSecondaryWeapon();
	if (pPrimaryWeapon && pPrimaryWeapon->IsMeleeWeapon() && ((CCFBaseMeleeWeapon*)pPrimaryWeapon)->IsCharging())
		return true;
	if (pSecondaryWeapon && pSecondaryWeapon->IsMeleeWeapon() && ((CCFBaseMeleeWeapon*)pSecondaryWeapon)->IsCharging())
		return true;
	return false;
}

CWeaponCFBase* CCFPlayer::GetBlockingWeapon()
{
	CWeaponCFBase* pWeapon = GetPrimaryWeapon();
	if (pWeapon && pWeapon->IsMeleeWeapon() &&
			gpGlobals->curtime > pWeapon->m_flNextPrimaryAttack)
		return pWeapon;

	pWeapon = GetSecondaryWeapon();
	if (pWeapon && pWeapon->IsMeleeWeapon() &&
			gpGlobals->curtime > pWeapon->m_flNextPrimaryAttack)
		return pWeapon;

	return NULL;
}

CWeaponCFBase* CCFPlayer::GetDefendingWeapon()
{
	if (GetBlockingWeapon())
		return GetBlockingWeapon();

	CWeaponCFBase* pWeapon = GetPrimaryWeapon();
	if (pWeapon && pWeapon->IsMeleeWeapon())
		return pWeapon;

	pWeapon = GetSecondaryWeapon();
	if (pWeapon && pWeapon->IsMeleeWeapon())
		return pWeapon;

	return NULL;
}

bool CCFPlayer::IsBlocking()
{
	if (!(m_nButtons&IN_BACK))
		return false;

	return GetBlockingWeapon() != NULL;
}

void CCFPlayer::StopMeleeAttack()
{
	if (GetPrimaryWeapon())
		GetPrimaryWeapon()->StopMeleeAttack(false);
	if (GetSecondaryWeapon())
		GetSecondaryWeapon()->StopMeleeAttack(false);
}

bool CCFPlayer::CanBlockBulletsWithWeapon(CWeaponCFBase* pWeapon)
{
	if (!IsAlive())
		return false;

	if (IsMagicMode())
		return false;

	if (!pWeapon)
		return false;

	if (IsBecomingFuse() || IsBecomingPariah())
		return false;

	if (m_hReviving != NULL)
		return false;

	return pWeapon->CanBlockBullets();
}

float CCFPlayer::GetEloProbability( CCFPlayer* pOpponent )
{
	return 1/(1+(pow(10, ((pOpponent->m_flEloScore - m_flEloScore)/400))));
}

float CCFPlayer::GetEloProbability( int iTeam )
{
	int iEnemyTeam = (iTeam == TEAM_NUMENI) ? TEAM_MACHINDO : TEAM_NUMENI;
	return 1/(1+(pow(10, ((CFGameRules()->GetAverageElo(NULL, iEnemyTeam) - m_flEloScore)/400))));
}

float g_aflPointKTable[] =
{
	0,		// SCORE_NOTHING = 0,
	32,		// SCORE_KO,
	8,		// SCORE_FATALITY,
	16,		// SCORE_REVIVAL,
	4,		// SCORE_DAMAGE,
	4,		// SCORE_HEAL,
	2,		// SCORE_INFLICT_STATUS,
	2,		// SCORE_HEAL_STATUS,
	128,	// SCORE_CAPTURE_FLAG,
	16,		// SCORE_KILL_ENEMY_FLAG_CARRIER,
	16,		// SCORE_RETURN_FLAG,
	128,	// SCORE_CAPTURE_POINT,
	4,		// SCORE_BLOCK_CAPTURE,
};

void CCFPlayer::AwardEloPoints( int iScoreType, CCFPlayer* pOpponent )
{
	float flProbability;
	if (pOpponent)
		flProbability = GetEloProbability(pOpponent);
	else
		flProbability = GetEloProbability(GetTeamNumber());

	float flPoints = (1 - flProbability) * g_aflPointKTable[iScoreType];

	m_flEloScore += flPoints;

	if (pOpponent)
		pOpponent->m_flEloScore -= flPoints;
	else
	{
		CCFTeam* pTeam = GetGlobalCFTeam( GetTeamNumber() == TEAM_NUMENI?TEAM_MACHINDO:TEAM_NUMENI );
		Assert(pTeam);
		if (pTeam)
		{
			float flPointsPerEnemy = flPoints / pTeam->GetNumPlayers();
			for (int i = 0; i < pTeam->GetNumPlayers(); i++)
			{
				CBasePlayer* pEnemy = pTeam->GetPlayer(i);
				Assert(pEnemy);
				if (!pEnemy)
					continue;

				ToCFPlayer(pEnemy)->m_flEloScore -= flPointsPerEnemy;
			}
		}
	}
}

// We completely override CBasePlayer because it calls GetActiveWeapon().
void CCFPlayer::ModifyOrAppendPlayerCriteria( AI_CriteriaSet& set )
{
	// Append our health
	set.AppendCriteria( "playerhealth", UTIL_VarArgs( "%i", GetHealth() ) );
	float healthfrac = 0.0f;
	if ( GetMaxHealth() > 0 )
	{
		healthfrac = (float)GetHealth() / (float)GetMaxHealth();
	}

	set.AppendCriteria( "playerhealthfrac", UTIL_VarArgs( "%.3f", healthfrac ) );

	CBaseCombatWeapon *pWeapon = GetPrimaryWeapon();
	if ( pWeapon )
	{
		set.AppendCriteria( "playerweapon", pWeapon->GetClassname() );
	}
	else
	{
		set.AppendCriteria( "playerweapon", "none" );
	}

	pWeapon = GetSecondaryWeapon();
	if ( pWeapon )
	{
		set.AppendCriteria( "playersecondaryweapon", pWeapon->GetClassname() );
	}
	else
	{
		set.AppendCriteria( "playersecondaryweapon", "none" );
	}

	// Append current activity name
	set.AppendCriteria( "playeractivity", CAI_BaseNPC::GetActivityName( GetActivity() ) );

	set.AppendCriteria( "playerspeed", UTIL_VarArgs( "%.3f", GetAbsVelocity().Length() ) );

	// CF stuff
	set.AppendCriteria( "magicmode", IsMagicMode()?"1":"0" );
	set.AppendCriteria( "chargingmelee", IsCharging()?"1":"0" );
	set.AppendCriteria( "waitingforplayers", (CFGameRules()->IsInWaitingForPlayers() || CFGameRules()->IsInPreMatch()) ? "1" : "0" );
	set.AppendCriteria( "hasobjective", HasObjective() ? "1" : "0" );

	set.AppendCriteria( "targetingfriendly", (GetRecursedTarget() && CFGameRules()->PlayerRelationship(this, GetRecursedTarget()) == GR_TEAMMATE) ? "1" : "0" );
	set.AppendCriteria( "targetingenemy", (GetRecursedTarget() && CFGameRules()->PlayerRelationship(this, GetRecursedTarget()) == GR_NOTTEAMMATE) ? "1" : "0" );

	set.AppendCriteria( "commitingfatality", (m_hReviving != NULL && !m_bReviving) ? "1" : "0" );
	set.AppendCriteria( "commitingrevival", (m_hReviving != NULL && m_bReviving) ? "1" : "0" );

	set.AppendCriteria( "knockedout", (IsKnockedOut()) ? "1" : "0" );

	if (IsFuse())
		set.AppendCriteria( "playerrole", "fuse" );
	else if (IsPariah())
		set.AppendCriteria( "playerrole", "pariah" );
	else
		set.AppendCriteria( "playerrole", "normal" );

	set.AppendCriteria( "playerteam", GetTeam()->GetName() );

	CCFAreaCapture *pAreaTrigger = GetControlPointStandingOn();
	if ( pAreaTrigger )
	{
		CTeamControlPoint *pCP = pAreaTrigger->GetControlPoint();
		if ( pCP )
		{
			if ( pCP->GetOwner() == GetTeamNumber() )
			{
				set.AppendCriteria( "OnFriendlyControlPoint", "1" );
			}
			else 
			{
				if ( TeamplayGameRules()->TeamMayCapturePoint( GetTeamNumber(), pCP->GetPointIndex() ) && 
					 TeamplayGameRules()->PlayerMayCapturePoint( this, pCP->GetPointIndex() ) )
				{
					set.AppendCriteria( "OnCappableControlPoint", "1" );
				}
			}
		}
	}

	if (CFGameRules()->GetGameMode() == CF_GAME_CTF)
		set.AppendCriteria( "gamemode", "ctf" );
	else if (CFGameRules()->GetGameMode() == CF_GAME_PARIAH)
		set.AppendCriteria( "gamemode", "pariah" );
	else
		set.AppendCriteria( "gamemode", "tdm" );

	AppendContextToCriteria( set, "player" );
}

CCFAreaCapture* CCFPlayer::GetControlPointStandingOn( void )
{
	touchlink_t *root = ( touchlink_t * )GetDataObject( TOUCHLINK );
	if ( root )
	{
		for ( touchlink_t *link = root->nextLink; link != root; link = link->nextLink )
		{
			CBaseEntity *pTouch = link->entityTouched;
			if ( pTouch && pTouch->IsSolidFlagSet( FSOLID_TRIGGER ) && pTouch->IsBSPModel() )
			{
				CCFAreaCapture *pAreaTrigger = dynamic_cast<CCFAreaCapture*>(pTouch);
				if ( pAreaTrigger )
					return pAreaTrigger;
			}
		}
	}

	return NULL;
}

bool CCFPlayer::GetInVehicle(IServerVehicle *pVehicle, int nRole)
{
	bool bResult = BaseClass::GetInVehicle(pVehicle, nRole);

	if (bResult)
	{
		StopPowerjump();
		StopLatch();
	}

	return bResult;
}

bool CCFPlayer::StartObserverMode(int mode)
{
    if ( GetPrimaryWeapon() )
		GetPrimaryWeapon()->Holster();
    if ( GetSecondaryWeapon() )
		GetSecondaryWeapon()->Holster();

	return BaseClass::StartObserverMode(mode);
}

void CCFPlayer::UpdateOnRemove( void )
{
	if ( m_hRagdoll )
	{
		UTIL_RemoveImmediate( m_hRagdoll );
		m_hRagdoll = NULL;
	}


	for (int i = 0; i < 3; i++) 
	{
		if (m_hWeapons[i])
			UTIL_Remove( m_hWeapons[i] );
		if (m_hAlternateWeapons[i])
			UTIL_Remove( m_hAlternateWeapons[i] );
	}

	BaseClass::UpdateOnRemove();
}

void CCFPlayer::CreateRagdollEntity()
{
	if ( m_hRagdoll )
	{
		UTIL_RemoveImmediate( m_hRagdoll );
		m_hRagdoll = NULL;
	}

	CCFRagdoll *pRagdoll = dynamic_cast< CCFRagdoll* >( m_hRagdoll.Get() );

	if ( !pRagdoll )
	{
		// create a new one
		pRagdoll = dynamic_cast< CCFRagdoll* >( CreateEntityByName( "cf_ragdoll" ) );
	}

	if ( pRagdoll )
	{
		pRagdoll->m_hPlayer = this;
		pRagdoll->m_vecRagdollOrigin = GetAbsOrigin();
		pRagdoll->m_vecRagdollVelocity = GetAbsVelocity();
		pRagdoll->m_nModelIndex = m_nModelIndex;
		pRagdoll->m_nSkin = m_nSkin;
		pRagdoll->m_nForceBone = m_nForceBone;
		pRagdoll->m_vecForce = m_vecForce;
		pRagdoll->Init();
		pRagdoll->SetBodygroup(1, m_bIsDecapitated);
	}

	if (m_bIsDecapitated)
	{
		CPhysicsProp *pProp = dynamic_cast< CPhysicsProp * >( CreateEntityByName( "physics_prop" ) );
		if ( pProp )
		{
			Vector vecHead;
			QAngle angHead;

			// Futile attempt to make the below code works. Emphasis in futile. The below code (SetupBones and GetBonePosition)
			// is broken because it spawns the head in the wrong position. Needs to be replaced with proper ragdolls.
			QAngle angLocal = GetLocalAngles();
			angLocal.y = m_PlayerAnimState->GetEyeYaw();
			angLocal.z = 0;
			SetLocalAngles(angLocal);

			int iBone = LookupBone( "Valvebiped.Bip01_Head1" );
			matrix3x4_t bonetoworld[MAXSTUDIOBONES];
			SetupBones( bonetoworld, BONE_USED_BY_ANYTHING );
			GetBonePosition(iBone, vecHead, angHead);

			Vector vecForward;
			AngleVectors(angHead, &vecForward);

			vecForward *= 200;
			vecForward.z = 200;

			char buf[512];
			// Pass in standard key values
			Q_snprintf( buf, sizeof(buf), "%.10f %.10f %.10f", vecHead.x, vecHead.y, vecHead.z );
			pProp->KeyValue( "origin", buf );
			Q_snprintf( buf, sizeof(buf), "%.10f %.10f %.10f", angHead.x, angHead.y, angHead.z );
			pProp->KeyValue( "angles", buf );
			pProp->KeyValue( "model", GetTeamNumber()==TEAM_MACHINDO?"models/player/machindomale_head.mdl":"models/player/numenimale_head.mdl" );
			pProp->KeyValue( "fademindist", "-1" );
			pProp->KeyValue( "fademaxdist", "0" );
			pProp->KeyValue( "fadescale", "1" );
			pProp->KeyValue( "inertiaScale", "1.0" );
			pProp->KeyValue( "physdamagescale", "0.1" );
			pProp->Precache();
			DispatchSpawn( pProp );
			pProp->Activate();
			pProp->ApplyAbsVelocityImpulse(vecForward);
			pProp->SetThink( &CPhysicsProp::SUB_Vanish );
			pProp->SetNextThink( gpGlobals->curtime + 10.0f );
		}
	}

	// ragdolls will be removed on round restart automatically
	m_hRagdoll = pRagdoll;
}

void CCFPlayer::FadeRagdoll()
{
	if (m_hRagdoll == NULL)
		return;

	((CCFRagdoll*)m_hRagdoll.Get())->FadeOut();
}

void CCFPlayer::DoAnimationEvent( PlayerAnimEvent_t event, int mData, bool bSecondary )
{
	m_PlayerAnimState->DoAnimationEvent( event, mData, bSecondary );
	TE_PlayerAnimEvent( this, event, mData, bSecondary );	// Send to any clients who can see this guy.
}

CWeaponCFBase* CCFPlayer::GetActiveCFWeapon() const
{
	return dynamic_cast< CWeaponCFBase* >( GetActiveWeapon() );
}

void CCFPlayer::Instructor_LessonLearned(int iLesson, bool bForceSend)
{
	CSingleUserRecipientFilter filter( this );
	filter.MakeReliable();

	CDisablePredictionFiltering disabler(bForceSend);

	if ( te->GetSuppressHost() )
	{
		if ( !filter.IgnorePredictionCull() )
			filter.RemoveRecipient( (CBasePlayer *)te->GetSuppressHost()  );
	}

	if ( !filter.GetRecipientCount() )
		return;

	UserMessageBegin( filter, "LessonLearned" );
		WRITE_BYTE( iLesson );
	MessageEnd();
}

void CCFPlayer::CheatImpulseCommands( int iImpulse )
{
	if ( !sv_cheats->GetBool() )
	{
		return;
	}

	if ( iImpulse != 101 )
	{
		BaseClass::CheatImpulseCommands( iImpulse );
		return ;
	}
	gEvilImpulse101 = true;

	EquipSuit();

	// Give the player everything!
	GiveAmmo( 90, AMMO_BULLETS );
	GiveAmmo( 5, AMMO_GRENADE );
	GiveAmmo( 36, AMMO_BUCKSHOT );
	
	TakeHealth( m_iMaxHealth, DMG_GENERIC );
	m_pStats->m_flFocus = m_pStats->m_iMaxFocus;
	m_pStats->m_flStamina = m_pStats->m_iMaxStamina;

	gEvilImpulse101		= false;
}

void CCFPlayer::RemoveAllItems( bool removeSuit )
{
	if (GetPrimaryWeapon())
		GetPrimaryWeapon()->Holster( );

	if (GetSecondaryWeapon())
		GetSecondaryWeapon()->Holster( );

	for (int i = 0; i < 3; i++)
	{
		m_hWeapons.Set( i, NULL );
		m_hAlternateWeapons.Set( i, NULL );
	}
	
	BaseClass::RemoveAllItems(removeSuit);
}

void CCFPlayer::SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways )
{
	// Skip this work if we're already marked for transmission.
	if ( pInfo->m_pTransmitEdict->Get( entindex() ) )
		return;

	BaseClass::SetTransmit( pInfo, bAlways );

	bool bLocalPlayer = ( pInfo->m_pClientEnt == edict() );

	if ( bLocalPlayer )
	{
		for ( int i=0; i < 3; i++ )
		{
			CBaseCombatWeapon *pWeapon = GetWeapon(i);
			if ( !pWeapon )
				continue;

			// The local player is sent all of his weapons.
			pWeapon->SetTransmit( pInfo, bAlways );
		}
	}
	else
	{
		// The check for EF_NODRAW is useless because the weapon will be networked anyway. In CBaseCombatWeapon::
		// UpdateTransmitState all weapons with owners will transmit to clients in the PVS.
		if ( GetPrimaryWeapon() && !GetPrimaryWeapon()->IsEffectActive( EF_NODRAW ) )
			GetPrimaryWeapon()->SetTransmit( pInfo, bAlways );

		if ( GetSecondaryWeapon() && !GetSecondaryWeapon()->IsEffectActive( EF_NODRAW ) )
			GetSecondaryWeapon()->SetTransmit( pInfo, bAlways );
	}
}

void CCFPlayer::SetWeaponMode(bool bPhysical)
{
	CArmament* pArm = GetActiveArmament();
	if ( pArm )
	{
		if ( !pArm->HasBindableCombo() )
		{
			// If the player has no bindable combos, don't let him go to magic mode.
			m_bPhysicalMode = true;
			return;
		}

		if ( (pArm->m_aAttackBinds[0].m_iWeapon == -1) && (pArm->m_aAttackBinds[1].m_iWeapon == -1) )
		{
			// If the player has no combos bound to mouse1 or mouse2, don't let him go to magic mode.
			m_bPhysicalMode = true;
			return;
		}
	}

	m_bPhysicalMode = bPhysical;
}

void CCFPlayer::FlashlightTurnOn( void )
{
	AddEffects( EF_DIMLIGHT );
}

void CCFPlayer::FlashlightTurnOff( void )
{
	RemoveEffects( EF_DIMLIGHT );
}

int CCFPlayer::FlashlightIsOn( void )
{
	return IsEffectActive( EF_DIMLIGHT );
}

void CCFPlayer::SetMenuOpen( bool bOpen )
{
	m_bIsMenuOpen = bOpen;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CCFPlayer::IsMenuOpen( void )
{
	return m_bIsMenuOpen;
}

void CCFPlayer::ShowCFPanel( CFPanel ePanel, bool bShow, bool bCloseAfter )
{
	CSingleUserRecipientFilter filter( this );
	filter.MakeReliable();

	UserMessageBegin( filter, "CFPanel" );
		WRITE_BYTE( ePanel );
		WRITE_BYTE( bShow?1:0 );
		WRITE_BYTE( bCloseAfter?1:0 );
	MessageEnd();
}

void CCFPlayer::SendPlayerInfo( CCFPlayer* pPlayer )
{
	if (!pPlayer)
	{
		CSingleUserRecipientFilter filter( this );
		filter.MakeReliable();

		UserMessageBegin( filter, "PlayerInfo" );
			WRITE_BYTE( 0 );
			WRITE_SHORT( 0 );
			WRITE_BYTE( 0 );
			WRITE_BYTE( 0 );
		MessageEnd();
	}
	else if (pPlayer == this)	// Suicide or accidental self-kill
	{
		CSingleUserRecipientFilter filter( this );
		filter.MakeReliable();

		UserMessageBegin( filter, "PlayerInfo" );
			WRITE_BYTE( entindex() );
			WRITE_SHORT( 0 );
			WRITE_BYTE( 0 );
			WRITE_BYTE( 0 );
		MessageEnd();
	}
	else
	{
		// Player is weak to melee attacks if he has no melee weapons and no magic combos to defend himself.
		bool bWeakToMelee = !pPlayer->HasAMelee() && !pPlayer->HasAMagicCombo();

		// Player is weak to firearms if he has no firearms and no sword to defend himself.
		bool bWeakToFirearms = !pPlayer->HasAFirearm() && !pPlayer->HasAMelee();

		// Player is weak to magic if he has no magic and no firearm to defend himself.
		bool bWeakToMagic = !pPlayer->HasAMagicCombo() && !pPlayer->HasAFirearm();

		// Player is weak to physical damage if he has a low physical defense armor rating.
		bool bWeakToPhyDmg = (pPlayer->m_pStats->GetDefense() < 15);	// TODO: Switch this over to the armament version once that is created.

		// Player is weak to magical damage if he has a low magical defense armor rating.
		bool bWeakToMagDmg = (pPlayer->m_pStats->GetResistance() < 15);	// TODO: Switch this over to the armament version once that is created.

		// Player is strong against melee attacks if he has a melee weapon or some magic.
		bool bStrongToMelee = pPlayer->HasAMelee() || pPlayer->HasAMagicCombo();

		// Player is strong against firearms if he has a firearm or a melee weapon.
		bool bStrongToFirearms = pPlayer->HasAFirearm() || pPlayer->HasAMelee();

		// Player is strong against magic if he has some magic or a firearm.
		bool bStrongToMagic = pPlayer->HasAMagicCombo() || pPlayer->HasAFirearm();

		// Player is strong against physical damage if he has a good physical defense armor rating.
		bool bStrongToPhyDmg = (pPlayer->m_pStats->GetDefense() >= 15);	// TODO: Switch this over to the armament version once that is created.

		// Player is strong against magical damage if he has a good magical defense armor rating.
		bool bStrongToMagDmg = (pPlayer->m_pStats->GetResistance() >= 15);	// TODO: Switch this over to the armament version once that is created.

		element_t eStrongElements = pPlayer->m_pArmament->GetElementsDefended();
		element_t eWeakElements = (element_t)(ELEMENTS_ALL ^ eStrongElements);

		int iPlayerInfo = (bWeakToMelee<<9)|(bWeakToFirearms<<8)|(bWeakToMagic<<7)|(bWeakToPhyDmg<<6)|(bWeakToMagDmg<<5)|(bStrongToMelee<<4)|(bStrongToFirearms<<3)|(bStrongToMagic<<2)|(bStrongToPhyDmg<<1)|(bStrongToMagDmg<<0);

		CSingleUserRecipientFilter filter( this );
		filter.MakeReliable();

		// If this fails, bump WRITE_BYTE up to WRITE_SHORT for the elements.
		Assert(TOTAL_ELEMENTS <= (1<<(sizeof(char)*4)));

		UserMessageBegin( filter, "PlayerInfo" );
			WRITE_BYTE( pPlayer->entindex() );
			WRITE_SHORT( iPlayerInfo );
			WRITE_BYTE( eStrongElements );
			WRITE_BYTE( eWeakElements );
		MessageEnd();
	}

	ShowCFPanel( CF_KILLERINFO, true );
}

bool CCFPlayer::RunMimicCommand( CUserCmd& cmd )
{
	if ( !IsBot() )
		return false;

	int iMimic = abs( bot_mimic.GetInt() );
	if ( iMimic > gpGlobals->maxClients )
		return false;

	CBasePlayer *pPlayer = UTIL_PlayerByIndex( iMimic );
	if ( !pPlayer )
		return false;

	if ( !pPlayer->GetLastUserCommand() )
		return false;

	cmd = *pPlayer->GetLastUserCommand();
	cmd.viewangles[YAW] += bot_mimic_yaw_offset.GetFloat();

	pl.fixangle = FIXANGLE_NONE;

	return true;
}

inline bool CCFPlayer::IsReloading( void ) const
{
	CWeaponCFBase *pPrimary = GetPrimaryWeapon();
	CWeaponCFBase *pSecondary = GetSecondaryWeapon();

	if (pPrimary && pPrimary->m_bInReload)
		return true;

	return pSecondary && pSecondary->m_bInReload;
}

CCFTeam* CCFPlayer::GetCFTeam()
{
	return dynamic_cast<CCFTeam*>(GetTeam());
}

void *SendProxy_Statistics( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
{
	CStatistics *pStats = ((CBasePlayer*)pData)->m_pStats;
	Assert( pStats );
	pRecipients->SetAllRecipients();
	return pStats;
}
