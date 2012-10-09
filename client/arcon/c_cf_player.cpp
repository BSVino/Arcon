//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "c_cf_player.h"
#include "weapon_cfbase.h"
#include "c_basetempentity.h"
#include "in_buttons.h"
#include "prediction.h"
#include "ivieweffects.h"
#include "view_scene.h"
#include "iinput.h"
#include "tier0/vprof.h"
#include "cfui_gui.h"
#include "cfui_menu.h"
#include "armament.h"
#include "statistics.h"
#include "bone_setup.h"
#include "cf_gamerules.h"
#include "ClientEffectPrecacheSystem.h"
#include "cf_in_main.h"
#include "ProxyEntity.h"
#include "materialsystem/IMaterial.h"
#include "materialsystem/IMaterialVar.h"
#include "weapon_magic.h"
#include "choreo/c_cf_choreo.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if defined( CCFPlayer )
	#undef CCFPlayer
#endif

CLIENTEFFECT_REGISTER_BEGIN( CFPlayerEffects )
CLIENTEFFECT_MATERIAL( "effects/target_friend" )
CLIENTEFFECT_MATERIAL( "effects/target_enemy" )
CLIENTEFFECT_MATERIAL( "effects/targeting_friend" )
CLIENTEFFECT_MATERIAL( "effects/targeting_enemy" )
CLIENTEFFECT_REGISTER_END()

ConVar cl_drawlocalplayer("cl_drawlocalplayer", "1", FCVAR_CHEAT);
ConVar cl_togglefollowmode("cl_togglefollowmode", "0", FCVAR_USERINFO|FCVAR_ARCHIVE, "Turn me on to use clap-on, clap-off instead of click hold follow mode.");
ConVar cl_autofollowmode("cl_autofollowmode", "1", FCVAR_USERINFO|FCVAR_ARCHIVE, "Automatically engage and disengage follow mode.");

void RecvProxy_Statistics( const RecvProp *pProp, void **pOut, void *pData, int objectID );

// -------------------------------------------------------------------------------- //
// Player animation event. Sent to the client when a player fires, jumps, reloads, etc..
// -------------------------------------------------------------------------------- //

class C_TEPlayerAnimEvent : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEPlayerAnimEvent, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

	virtual void PostDataUpdate( DataUpdateType_t updateType )
	{
		VPROF( "C_TEPlayerAnimEvent::PostDataUpdate" );

		// Create the effect.
		if ( m_iPlayerIndex == MAX_PLAYERS+1 )
			return;

		EHANDLE hPlayer = cl_entitylist->GetNetworkableHandle( m_iPlayerIndex );
		if ( !hPlayer )
			return;

		C_CFPlayer *pPlayer = dynamic_cast< C_CFPlayer* >( hPlayer.Get() );
		if ( pPlayer && !pPlayer->IsDormant() )
		{
			pPlayer->DoAnimationEvent( (PlayerAnimEvent_t)m_iEvent.Get(), m_nData, m_bSecondary );
		}	
	}

public:
	CNetworkVar( int, m_iPlayerIndex );
	CNetworkVar( int, m_iEvent );
	CNetworkVar( int, m_nData );
	CNetworkVar( bool, m_bSecondary );
};

IMPLEMENT_CLIENTCLASS_EVENT( C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent, CTEPlayerAnimEvent );

//-----------------------------------------------------------------------------
// Data tables and prediction tables.
//-----------------------------------------------------------------------------
BEGIN_RECV_TABLE_NOBASE( C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent )
	RecvPropInt( RECVINFO( m_iPlayerIndex ) ),
	RecvPropInt( RECVINFO( m_iEvent ) ),
	RecvPropInt( RECVINFO( m_nData ) ),
	RecvPropInt( RECVINFO( m_bSecondary ) ),
END_RECV_TABLE()

BEGIN_NETWORK_TABLE_NOBASE( CStatistics, DT_Statistics )
	RecvPropTime	(RECVINFO(m_flLastStatsThink)),
	RecvPropFloat	(RECVINFO(m_flFocus)),
	RecvPropInt		(RECVINFO(m_iMaxFocus)),
	RecvPropFloat	(RECVINFO(m_flStamina)),
	RecvPropInt		(RECVINFO(m_iMaxStamina)),
	RecvPropFloat	(RECVINFO(m_flHealthRegen)),
	RecvPropFloat	(RECVINFO(m_flFocusRegen)),
	RecvPropFloat	(RECVINFO(m_flStaminaRegen)),
	RecvPropFloat	(RECVINFO(m_flAttack)),
	RecvPropFloat	(RECVINFO(m_flEnergy)),
	RecvPropFloat	(RECVINFO(m_flDefense)),
	RecvPropFloat	(RECVINFO(m_flResistance)),
	RecvPropFloat	(RECVINFO(m_flSpeedStat)),
	RecvPropFloat	(RECVINFO(m_flCritical)),

	RecvPropInt		(RECVINFO(m_iStatusEffects)),

	RecvPropFloat	(RECVINFO(m_flDOT)),
	RecvPropFloat	(RECVINFO(m_flSlowness)),
	RecvPropFloat	(RECVINFO(m_flWeakness)),
	RecvPropFloat	(RECVINFO(m_flDisorientation)),
	RecvPropFloat	(RECVINFO(m_flBlindness)),
	RecvPropFloat	(RECVINFO(m_flAtrophy)),
	RecvPropFloat	(RECVINFO(m_flSilence)),
	RecvPropFloat	(RECVINFO(m_flRegeneration)),
	RecvPropFloat	(RECVINFO(m_flPoison)),
	RecvPropFloat	(RECVINFO(m_flHaste)),
	RecvPropFloat	(RECVINFO(m_flShield)),
	RecvPropFloat	(RECVINFO(m_flBarrier)),
	RecvPropFloat	(RECVINFO(m_flReflect)),
	RecvPropTime	(RECVINFO(m_flStealth)),
	RecvPropFloat	(RECVINFO(m_flOverdrive)),
END_NETWORK_TABLE()

BEGIN_RECV_TABLE_NOBASE( C_CFPlayer, DT_CFLocalPlayerExclusive )
	RecvPropInt( RECVINFO( m_iDirection ) ),
	RecvPropInt( RECVINFO( m_iShotsFired ) ),

	RecvPropEHandle		( RECVINFO( m_hCameraCinematic ) ),
	RecvPropTime		( RECVINFO( m_flCameraCinematicUntil ) ),

	RecvPropTime		( RECVINFO( m_flFreezeUntil ) ),
	RecvPropFloat		( RECVINFO( m_flFreezeAmount ) ),

	RecvPropTime		( RECVINFO( m_flFreezeRotation ) ),
	RecvPropFloat		( RECVINFO( m_flFreezeRAmount ) ),
	RecvPropFloat		( RECVINFO( m_angFreeze[0] ) ),
	RecvPropFloat		( RECVINFO( m_angFreeze[1] ) ),
	RecvPropFloat		( RECVINFO( m_angFreeze[2] ) ),

	RecvPropTime		( RECVINFO( m_flEnemyFrozenUntil ) ),
	RecvPropEHandle		( RECVINFO( m_hEnemyFrozen ) ),

	RecvPropTime		( RECVINFO( m_flSuspendGravityAt ) ),
	RecvPropTime		( RECVINFO( m_flSuspendGravityUntil ) ),

	RecvPropFloat		( RECVINFO( m_flLastDashTime ) ),
	RecvPropInt			( RECVINFO( m_bWantVelocityMatched ) ),

	RecvPropInt			( RECVINFO( m_iAirMeleeAttacks ) ),
	RecvPropInt			( RECVINFO( m_iMeleeChain ) ),

	RecvPropFloat		( RECVINFO( m_flRushDistance ) ),
	RecvPropEHandle		( RECVINFO( m_hRushingWeapon ) ),
	RecvPropBool		( RECVINFO( m_bDownStrike ) ),

	RecvPropFloat		( RECVINFO( m_flNextRespawn ) ),
	RecvPropFloat		( RECVINFO( m_flFatalityStart ) ),

	RecvPropEHandle		( RECVINFO( m_hLastAttacker ) ),
	RecvPropFloat		( RECVINFO( m_flLastAttackedTime ) ),
	RecvPropEHandle		( RECVINFO( m_hLastDOTAttacker ) ),

	RecvPropFloat		( RECVINFO( m_flStrongAttackJumpTime ) ),
	RecvPropBool		( RECVINFO( m_bStrongAttackJump ) ),

	RecvPropInt			( RECVINFO(m_iLatchTriggerCount) ),
	RecvPropTime		( RECVINFO( m_flLastLatch ) ),
END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_DT( C_CFPlayer, DT_CFPlayer, CCFPlayer )
	RecvPropDataTable( "cflocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_CFLocalPlayerExclusive) ),

	RecvPropDataTable( "player_statistics_data", 0, 0, &REFERENCE_RECV_TABLE( DT_Statistics ), RecvProxy_Statistics ),

	RecvPropInt( RECVINFO( m_bInFollowMode ) ),

	RecvPropBool( RECVINFO( m_bLatched ) ),
	RecvPropBool( RECVINFO( m_bOverdrive ) ),
	RecvPropVector( RECVINFO( m_vecLatchPlaneNormal ) ),

	RecvPropBool (RECVINFO(m_bCanPowerjump)),
	RecvPropBool (RECVINFO(m_bPowerjump)),
	RecvPropBool (RECVINFO(m_bChargejump)),

	RecvPropInt( RECVINFO( m_eLHEffectElements ) ),
	RecvPropInt( RECVINFO( m_eRHEffectElements ) ),
	RecvPropInt( RECVINFO( m_iLastCombo ) ),

	RecvPropInt( RECVINFO( m_iActiveSecondary ) ),

	RecvPropArray3	( RECVINFO_ARRAY(m_hWeapons), RecvPropEHandle( RECVINFO( m_hWeapons[0] ) ) ),
	RecvPropArray3	( RECVINFO_ARRAY(m_hAlternateWeapons), RecvPropEHandle( RECVINFO( m_hAlternateWeapons[0] ) ) ),
	RecvPropEHandle	( RECVINFO(m_hMagic) ),

	RecvPropBool		( RECVINFO(m_bPhysicalMode) ),

	RecvPropInt		( RECVINFO( m_bBecomingFuse ) ),
	RecvPropInt		( RECVINFO( m_bIsFuse ) ),
	RecvPropFloat	( RECVINFO( m_flFuseStartTime ) ),
	RecvPropInt		( RECVINFO( m_bBecomingPariah ) ),
	RecvPropInt		( RECVINFO( m_bIsPariah ) ),
	RecvPropFloat	( RECVINFO( m_flPariahStartTime ) ),

	RecvPropInt		( RECVINFO( m_bIsCaptain ) ),
	RecvPropInt		( RECVINFO( m_bIsSergeant ) ),

	RecvPropEHandle	( RECVINFO( m_hDirectTarget ) ),
	RecvPropEHandle	( RECVINFO( m_hRecursedTarget ) ),

	RecvPropInt			( RECVINFO( m_bReviving ) ),
	RecvPropEHandle		( RECVINFO( m_hReviver ) ),
	RecvPropEHandle		( RECVINFO( m_hReviving ) ),
	RecvPropBool		( RECVINFO( m_bIsDecapitated ) ),

	RecvPropEHandle		( RECVINFO( m_hObjective ) ),

	RecvPropFloat( RECVINFO( m_angEyeAngles[0] ) ),
	RecvPropFloat( RECVINFO( m_angEyeAngles[1] ) ),
	RecvPropEHandle( RECVINFO( m_hRagdoll ) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_CFPlayer )
	DEFINE_PRED_FIELD( m_nSkin, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE ),
	DEFINE_PRED_FIELD( m_nBody, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE ),
	DEFINE_PRED_FIELD( m_nSequence, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_flPlaybackRate, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_flCycle, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_iShotsFired, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),   
	DEFINE_PRED_FIELD( m_iDirection, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),   

	DEFINE_PRED_FIELD( m_flLastAnimFrameTime, FIELD_FLOAT, 0 ),   

	DEFINE_PRED_FIELD( m_flFreezeUntil, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),   
	DEFINE_PRED_FIELD( m_flFreezeAmount, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),   
	DEFINE_PRED_FIELD( m_flFreezeRotation, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),   
	DEFINE_PRED_FIELD( m_flFreezeRAmount, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),   
	DEFINE_PRED_FIELD_TOL( m_angFreeze, FIELD_VECTOR, FTYPEDESC_INSENDTABLE, 0.125f ),

	DEFINE_PRED_FIELD( m_flEnemyFrozenUntil, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),   
	DEFINE_PRED_FIELD( m_hEnemyFrozen, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD( m_flSuspendGravityAt, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),   
	DEFINE_PRED_FIELD( m_flSuspendGravityUntil, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),   

	DEFINE_PRED_FIELD( m_bInFollowMode, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),   
	DEFINE_PRED_FIELD( m_flFollowModeStarted, FIELD_FLOAT, 0 ),   

	DEFINE_PRED_ARRAY( m_hWeapons, FIELD_EHANDLE, 3, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_ARRAY( m_hAlternateWeapons, FIELD_EHANDLE, 3, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_hMagic, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD( m_hDirectTarget, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_hRecursedTarget, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD( m_bOverdrive, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD( m_bPhysicalMode, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD( m_hReviver, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_hReviving, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bReviving, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flNextRespawn, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),   
	DEFINE_PRED_FIELD( m_flFatalityStart, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),   
	DEFINE_PRED_FIELD( m_bIsDecapitated, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD( m_flRushDistance, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),   
	DEFINE_PRED_FIELD( m_hRushingWeapon, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bDownStrike, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD( m_hObjective, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD( m_bLatched, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_iLatchTriggerCount, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),   
	DEFINE_PRED_FIELD( m_flLastLatch, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),   
	DEFINE_PRED_FIELD( m_vecLatchPlaneNormal, FIELD_VECTOR, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_hLatchEntity, FIELD_EHANDLE, 0 ),

	DEFINE_PRED_FIELD( m_bCanPowerjump, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bPowerjump, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bChargejump, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD( m_eLHEffectElements, FIELD_INTEGER, FTYPEDESC_PRIVATE ),   // Visual items. Prob don't want them changing all the time.
	DEFINE_PRED_FIELD( m_eRHEffectElements, FIELD_INTEGER, FTYPEDESC_PRIVATE ),
	DEFINE_PRED_FIELD( m_iLastCombo, FIELD_INTEGER, FTYPEDESC_PRIVATE ),

	DEFINE_PRED_FIELD( m_iLastMovementButton, FIELD_INTEGER, 0 ),   
	DEFINE_PRED_FIELD( m_flLastMovementButtonTime, FIELD_FLOAT, 0 ),   
END_PREDICTION_DATA()

// Global Savedata for player
BEGIN_DATADESC( C_CFPlayer )

	DEFINE_FIELD( m_hDirectTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hRecursedTarget, FIELD_EHANDLE ),

END_DATADESC()

ConVar cl_ragdoll_physics_enable( "cl_ragdoll_physics_enable", "1", 0, "Enable/disable ragdoll physics." );

class C_CFRagdoll : public C_BaseAnimatingOverlay
{
public:
	DECLARE_CLASS( C_CFRagdoll, C_BaseAnimatingOverlay );
	DECLARE_CLIENTCLASS();

	C_CFRagdoll();
	~C_CFRagdoll();

	virtual void OnDataChanged( DataUpdateType_t type );

	int GetPlayerEntIndex() const;
	IRagdoll* GetIRagdoll() const;

private:

	C_CFRagdoll( const C_CFRagdoll & ) {}

	void Interp_Copy( C_BaseAnimatingOverlay *pSourceEntity );
	void ImpactTrace( trace_t *pTrace, int iDamageType, char *pCustomImpactName );

	void CreateRagdoll();

	void ClientThink();

private:

	EHANDLE	m_hPlayer;
	CNetworkVector( m_vecRagdollVelocity );
	CNetworkVector( m_vecRagdollOrigin );
	CNetworkVar(float, m_flFadeStartTime);
};


IMPLEMENT_CLIENTCLASS_DT_NOBASE( C_CFRagdoll, DT_CFRagdoll, CCFRagdoll )
	RecvPropVector( RECVINFO(m_vecRagdollOrigin) ),
	RecvPropEHandle( RECVINFO( m_hPlayer ) ),
	RecvPropInt( RECVINFO( m_nModelIndex ) ),
	RecvPropInt( RECVINFO(m_nForceBone) ),
	RecvPropVector( RECVINFO(m_vecForce) ),
	RecvPropVector( RECVINFO( m_vecRagdollVelocity ) ),
	RecvPropFloat( RECVINFO( m_flFadeStartTime ) ),
END_RECV_TABLE()


C_CFRagdoll::C_CFRagdoll()
{
}

C_CFRagdoll::~C_CFRagdoll()
{
	PhysCleanupFrictionSounds( this );
}

void C_CFRagdoll::Interp_Copy( C_BaseAnimatingOverlay *pSourceEntity )
{
	if ( !pSourceEntity )
		return;
	
	VarMapping_t *pSrc = pSourceEntity->GetVarMapping();
	VarMapping_t *pDest = GetVarMapping();
    	
	// Find all the VarMapEntry_t's that represent the same variable.
	for ( int i = 0; i < pDest->m_Entries.Count(); i++ )
	{
		VarMapEntry_t *pDestEntry = &pDest->m_Entries[i];
		for ( int j=0; j < pSrc->m_Entries.Count(); j++ )
		{
			VarMapEntry_t *pSrcEntry = &pSrc->m_Entries[j];
			if ( !Q_strcmp( pSrcEntry->watcher->GetDebugName(),
				pDestEntry->watcher->GetDebugName() ) )
			{
				pDestEntry->watcher->Copy( pSrcEntry->watcher );
				break;
			}
		}
	}
}

void C_CFRagdoll::ImpactTrace( trace_t *pTrace, int iDamageType, char *pCustomImpactName )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();

	if( !pPhysicsObject )
		return;

	Vector dir = pTrace->endpos - pTrace->startpos;

	if ( iDamageType == DMG_BLAST )
	{
		dir *= 4000;  // adjust impact strenght
				
		// apply force at object mass center
		pPhysicsObject->ApplyForceCenter( dir );
	}
	else
	{
		Vector hitpos;  
	
		VectorMA( pTrace->startpos, pTrace->fraction, dir, hitpos );
		VectorNormalize( dir );

		dir *= 4000;  // adjust impact strenght

		// apply force where we hit it
		pPhysicsObject->ApplyForceOffset( dir, hitpos );	

		// Blood spray!
		//FX_BloodSpray( hitpos, dir, 10 );
	}

	m_pRagdoll->ResetRagdollSleepAfterTime();
}

void C_CFRagdoll::CreateRagdoll()
{
	// First, initialize all our data. If we have the player's entity on our client,
	// then we can make ourselves start out exactly where the player is.
	C_CFPlayer *pPlayer = dynamic_cast< C_CFPlayer* >( m_hPlayer.Get() );

	if ( pPlayer && !pPlayer->IsDormant() )
	{
		// move my current model instance to the ragdoll's so decals are preserved.
		pPlayer->SnatchModelInstance( this );

		VarMapping_t *varMap = GetVarMapping();

		// Copy all the interpolated vars from the player entity.
		// The entity uses the interpolated history to get bone velocity.
		bool bRemotePlayer = (pPlayer != C_BasePlayer::GetLocalPlayer());			
		if ( bRemotePlayer )
		{
			Interp_Copy( pPlayer );

			SetAbsAngles( pPlayer->GetRenderAngles() );
			GetRotationInterpolator().Reset();

			m_flAnimTime = pPlayer->m_flAnimTime;
			SetSequence( pPlayer->GetSequence() );
			m_flPlaybackRate = pPlayer->GetPlaybackRate();
		}
		else
		{
			// This is the local player, so set them in a default
			// pose and slam their velocity, angles and origin
			SetAbsOrigin( m_vecRagdollOrigin );

			SetAbsAngles( pPlayer->GetRenderAngles() );

			SetAbsVelocity( m_vecRagdollVelocity );

			int iSeq = LookupSequence( "idle" );
			if ( iSeq == -1 )
			{
				Assert( false );	// missing walk_lower?
				iSeq = 0;
			}

			SetSequence( iSeq );	// walk_lower, basic pose
			SetCycle( 0.0 );

			Interp_Reset( varMap );
		}		
	}
	else
	{
		// overwrite network origin so later interpolation will
		// use this position
		SetNetworkOrigin( m_vecRagdollOrigin );

		SetAbsOrigin( m_vecRagdollOrigin );
		SetAbsVelocity( m_vecRagdollVelocity );

		Interp_Reset( GetVarMapping() );

	}

	if (pPlayer)
	{
		SetModelIndex( m_nModelIndex );
		m_nSkin = pPlayer->m_nSkin;
		SetBodygroup(1, pPlayer->m_bIsDecapitated);
	}

	// Turn it into a ragdoll.
	if ( cl_ragdoll_physics_enable.GetBool() )
	{
		// Make us a ragdoll..
		m_nRenderFX = kRenderFxRagdoll;

		matrix3x4_t boneDelta0[MAXSTUDIOBONES];
		matrix3x4_t boneDelta1[MAXSTUDIOBONES];
		matrix3x4_t currentBones[MAXSTUDIOBONES];
		const float boneDt = 0.05f;

		// We have to make sure that we're initting this client ragdoll off of the same model.
		// GetRagdollInitBoneArrays uses the *player* Hdr, which may be a different model than
		// the ragdoll Hdr, if we try to create a ragdoll in the same frame that the player
		// changes their player model.
		CStudioHdr *pRagdollHdr = GetModelPtr();
		CStudioHdr *pPlayerHdr = NULL;
		if ( pPlayer )
			pPlayerHdr = pPlayer->GetModelPtr();

		bool bChangedModel = false;

		if ( pRagdollHdr && pPlayerHdr )
		{
			bChangedModel = pRagdollHdr->GetVirtualModel() != pPlayerHdr->GetVirtualModel();

			Assert( !bChangedModel && "C_CFRagdoll::CreateRagdoll: Trying to create ragdoll with a different model than the player it's based on" );
		}

		if ( pPlayer && !pPlayer->IsDormant() && !bChangedModel && pPlayerHdr )
		{
			pPlayer->GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
		}
		else
		{
			GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
		}

		InitAsClientRagdoll( boneDelta0, boneDelta1, currentBones, boneDt );
	}
	SetNextClientThink( CLIENT_THINK_ALWAYS );
}


void C_CFRagdoll::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		CreateRagdoll();
	}

	if (m_pRagdoll)
		m_pRagdoll->ResetRagdollSleepAfterTime();
}

IRagdoll* C_CFRagdoll::GetIRagdoll() const
{
	return m_pRagdoll;
}

void C_CFRagdoll::ClientThink()
{
	float flFadeTime = 3.0f;

	float dt = gpGlobals->curtime - m_flFadeStartTime - 3;
	if ( dt > 0 && dt < flFadeTime )
	{
		SetRenderMode(kRenderTransTexture);
		SetRenderColorA( (1.0f - dt / flFadeTime) * 255.0f );
	}
}

C_BaseAnimating * C_CFPlayer::BecomeRagdollOnClient()
{
	// Let the C_CSRagdoll entity do this.
	// m_builtRagdoll = true;
	return NULL;
}


IRagdoll* C_CFPlayer::GetRepresentativeRagdoll() const
{
	if ( m_hRagdoll.Get() )
	{
		C_CFRagdoll *pRagdoll = (C_CFRagdoll*)m_hRagdoll.Get();

		return pRagdoll->GetIRagdoll();
	}
	else
	{
		return NULL;
	}
}


class CShieldVisibilityProxy : public CEntityMaterialProxy
{
public:
						CShieldVisibilityProxy( void );
	virtual				~CShieldVisibilityProxy( void );
	virtual bool		Init( IMaterial *pMaterial, KeyValues* pKeyValues );
	virtual void		OnBind( C_BaseEntity *pC_BaseEntity );
	virtual IMaterial *	GetMaterial();

private:
	IMaterialVar*		m_AlphaVar;
};

CShieldVisibilityProxy::CShieldVisibilityProxy()
{
	m_AlphaVar = NULL;
}

CShieldVisibilityProxy::~CShieldVisibilityProxy()
{
}

bool CShieldVisibilityProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	bool foundVar;
	m_AlphaVar = pMaterial->FindVar( "$refractamount", &foundVar, false );
	return foundVar;
}

ConVar cf_barrierrefract("cf_barrierrefract", "0.4", FCVAR_CHEAT);
ConVar cf_shieldrefract("cf_shieldrefract", "0.8", FCVAR_CHEAT);

void CShieldVisibilityProxy::OnBind( C_BaseEntity *pEnt )
{
	if (!pEnt->GetOwnerEntity())
		return;

	if (!pEnt->GetOwnerEntity()->IsPlayer())
		return;

	C_CFPlayer* pPlayer = ToCFPlayer(pEnt->GetOwnerEntity());

	if (m_AlphaVar)
	{
		float flRefractAmount;
		if (pPlayer->m_bShieldPhysical)
			flRefractAmount = cf_barrierrefract.GetFloat();
		else
			flRefractAmount = cf_shieldrefract.GetFloat();

		m_AlphaVar->SetFloatValue( ((float)pEnt->m_clrRender->a) * flRefractAmount / 255 );
	}
}

IMaterial *CShieldVisibilityProxy::GetMaterial()
{
	if ( !m_AlphaVar )
		return NULL;

	return m_AlphaVar->GetOwningMaterial();
}

EXPOSE_INTERFACE( CShieldVisibilityProxy, IMaterialProxy, "ShieldVisibility" IMATERIAL_PROXY_INTERFACE_VERSION );


class CStatusEffectSkinProxy : public CEntityMaterialProxy
{
public:
						CStatusEffectSkinProxy( void );
	virtual				~CStatusEffectSkinProxy( void );
	virtual bool		Init( IMaterial *pMaterial, KeyValues* pKeyValues );
	virtual void		OnBind( C_BaseEntity *pC_BaseEntity );
	virtual IMaterial *	GetMaterial();

private:
	IMaterialVar*		m_pDetail;
	IMaterialVar*		m_pDetailScale;
	IMaterialVar*		m_pDetailBlend;
	IMaterialVar*		m_pDetailMode;
};

CStatusEffectSkinProxy::CStatusEffectSkinProxy()
{
	m_pDetail = NULL;
	m_pDetailScale = NULL;
	m_pDetailBlend = NULL;
	m_pDetailMode = NULL;
}

CStatusEffectSkinProxy::~CStatusEffectSkinProxy()
{
}

bool CStatusEffectSkinProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	bool bFoundDetail, bFoundScale, bFoundBlend, bFoundMode;

	m_pDetail = pMaterial->FindVar( "$detail", &bFoundDetail, false );
	m_pDetailScale = pMaterial->FindVar( "$detailscale", &bFoundScale, false );
	m_pDetailBlend = pMaterial->FindVar( "$detailblendfactor", &bFoundBlend, false );
	m_pDetailMode = pMaterial->FindVar( "$detailblendmode", &bFoundMode, false );

	return bFoundDetail && bFoundScale && bFoundBlend && bFoundMode;
}

void CStatusEffectSkinProxy::OnBind( C_BaseEntity *pEnt )
{
	float flEffectMagnitude;

	if (pEnt->IsPlayer() && ToCFPlayer(pEnt))
	{
		C_CFPlayer* pPlayer = ToCFPlayer(pEnt);
		flEffectMagnitude = pPlayer->m_pStats->GetEffectFromBitmask(STATUSEFFECT_SLOWNESS);
	}
	else if (pEnt->IsNPC() && dynamic_cast<C_CFActor*>(pEnt))
	{
		flEffectMagnitude = dynamic_cast<C_CFActor*>(pEnt)->m_flEffectMagnitude;
	}
	else
		return;

	if (m_pDetailBlend)
	{
		float flCurrent = m_pDetailBlend->GetFloatValue();
		float flGoal = RemapValClamped(flEffectMagnitude, 0.0f, 1.0f, 0.3f, 1.0f );

		if (flEffectMagnitude < 0.01)
			flGoal = 0.0f;

		m_pDetailBlend->SetFloatValue( Approach(flGoal, flCurrent, gpGlobals->frametime/10) );
	}
}

IMaterial *CStatusEffectSkinProxy::GetMaterial()
{
	if ( !m_pDetail )
		return NULL;

	return m_pDetail->GetOwningMaterial();
}

EXPOSE_INTERFACE( CStatusEffectSkinProxy, IMaterialProxy, "StatusLevel" IMATERIAL_PROXY_INTERFACE_VERSION );


C_CFPlayer::C_CFPlayer() : 
	m_iv_angEyeAngles( "C_CFPlayer::m_iv_angEyeAngles" )
{
	m_PlayerAnimState = CreatePlayerAnimState( this );

	m_angEyeAngles.Init();
	AddVar( &m_angEyeAngles, &m_iv_angEyeAngles, LATCH_SIMULATION_VAR );

	m_iDirection = 0;

	m_flPitchRecoilAccumulator = 0.0;
	m_flYawRecoilAccumulator = 0.0;
	m_flRecoilTimeRemaining = 0.0;

	m_bInFollowMode = m_bOldInFollowMode = false;

	m_pOverdriveEffect = NULL;
	m_pKnockoutEffect = NULL;

	m_pBarrier = NULL;

	m_flLastRespawn = -1;

	m_pArmament = new CArmament();
	m_pCurrentArmament = NULL;
	m_pStats = new CStatistics(this);

	m_flLastCameraTargetTime = 0;
	m_vecLastCameraTarget = Vector(0,0,0);
	m_vecLastTargetPosition = Vector(0,0,0);
}


C_CFPlayer::~C_CFPlayer()
{
	m_PlayerAnimState->Release();

	delete m_pArmament;
	if (m_pCurrentArmament)
		delete m_pCurrentArmament;
	delete m_pStats;
}

void C_CFPlayer::Spawn()
{
	BaseClass::Spawn();

	if (GetTeamNumber() == TEAM_SPECTATOR || GetTeamNumber() == TEAM_UNASSIGNED)
		cfgui::CRootPanel::SetArmament(m_pArmament);
	else
		cfgui::CRootPanel::SetArmament(m_pCurrentArmament);

	if (IsLocalPlayer())
	{
		switch (cvar->FindVar("cl_thirdperson")->GetInt())
		{
		case 0:
			CFInput()->CAM_ToFirstPerson();
			break;
		case 1:
			CFInput()->CAM_ToThirdPerson();
			break;
		case 2:
			if (GetActiveArmament()->GetWeaponData(0) && GetActiveArmament()->GetWeaponData(0)->m_eWeaponType == WEAPONTYPE_MELEE)
				CFInput()->CAM_ToThirdPerson();
			else
				CFInput()->CAM_ToFirstPerson();
			break;
		}
	}

	m_flLastRespawn = gpGlobals->curtime;
	m_flLastEnemySeen = 0;

	m_flLastCameraTargetTime = 0;

	Instructor_Respawn();

	m_hDrawingDirectTarget = NULL;
	m_hDrawingRecursedTarget = NULL;
	m_flReceivedDirectTarget = 0;
	m_flReceivedRecursedTarget = 0;

	m_eLHEffectElementsOld = ELEMENT_TYPELESS;
	m_eRHEffectElementsOld = ELEMENT_TYPELESS;
	m_iLastCombo = 0;

	m_pLHChargeEffect = m_pRHChargeEffect = NULL;

	for (int i = 0; i < m_apLHComboEffects.Count(); i++)
	{
		ParticleProp()->StopEmission(m_apLHComboEffects[i]);
		ParticleProp()->StopEmission(m_apRHComboEffects[i]);
	}
	m_apLHComboEffects.RemoveAll();
	m_apRHComboEffects.RemoveAll();

	if (!m_pBarrier)
	{
		m_pBarrier = new C_BaseAnimatingOverlay();
		m_pBarrier->InitializeAsClientEntity( "models/magic/barrier.mdl", RENDER_GROUP_OPAQUE_ENTITY );
		m_pBarrier->UseClientSideAnimation();
		m_pBarrier->SetModel("models/magic/barrier.mdl");
		m_pBarrier->AddEffects( EF_NODRAW ); // don't let the renderer draw the model normally
		m_pBarrier->SetOwnerEntity(this);
	}

	m_flShieldTime = 0;
	m_flShieldStrength = 0;
}

void C_CFPlayer::PreThink()
{
	FollowMode();

	LatchThink();

	// Recoil
	QAngle viewangles;
	engine->GetViewAngles( viewangles );

	float flYawRecoil;
	float flPitchRecoil;
	GetRecoilToAddThisFrame( flPitchRecoil, flYawRecoil );

	// Recoil
	if( flPitchRecoil > 0 )
	{
		//add the recoil pitch
		viewangles[PITCH] -= flPitchRecoil;
		viewangles[YAW] += flYawRecoil;
	}

	engine->SetViewAngles( viewangles );

	BaseClass::PreThink();

	if (m_afButtonPressed & IN_ALT2)
		Instructor_LessonLearned(HINT_SHIFT_MAGIC);

	CalculateHandMagic();
}

void C_CFPlayer::PostThink()
{
	if ( IsAlive() && (GetFlags() & FL_ONGROUND) && m_bPowerjump )
		StopPowerjump();
	
	BaseClass::PostThink();

	Instructor_Think();

	for (int i = 1; i < gpGlobals->maxClients; i++)
	{
		C_BasePlayer *pPlayer =	UTIL_PlayerByIndex( i );

		if (!pPlayer)
			continue;

		C_CFPlayer* pCFPlayer = ToCFPlayer(pPlayer);

		if (CFGameRules()->PlayerRelationship(this, pCFPlayer) == GR_TEAMMATE)
			continue;

		// Far enemies are not important.
		if ((EyePosition() - pCFPlayer->WorldSpaceCenter()).Length() > 500)
			continue;

		trace_t result;
		CTraceFilterNoNPCsOrPlayer traceFilter( NULL, COLLISION_GROUP_NONE );
		UTIL_TraceLine( EyePosition(), pCFPlayer->WorldSpaceCenter(), MASK_VISIBLE_AND_NPCS, &traceFilter, &result );
		if (result.fraction != 1.0f)
		//if (!pPlayer->IsVisible(pCFTarget))	// This is unfortunately a server-only function, though I'd love to use it here.
			continue;

		m_flLastEnemySeen = gpGlobals->curtime;
		break;
	}

	if (!IsInFollowMode() || !ShouldLockFollowModeView())
	{
		Vector vecForward;
		GetVectors(&vecForward, NULL, NULL);

		if (m_flLastCameraTargetTime == 0)
		{
			if (!GetRecursedTarget())
				m_hLastCameraTarget = NULL;
			m_vecLastCameraTarget = m_vecLastTargetPosition = EyePosition() + vecForward*100;
		}
	}
}

void C_CFPlayer::ClientThink()
{
	C_CFPlayer* pLocal = C_CFPlayer::GetLocalCFPlayer();
	bool bShouldDisplayEffects = true;

	// Don't show the particle effects for a local player in first person.
	if (this == pLocal && !CFInput()->CAM_IsThirdPerson() || pLocal->IsFirstPersonSpectating(this))
		bShouldDisplayEffects = false;

	if (!IsAlive() && !IsKnockedOut())
	{
		for (int i = 0; i < TOTAL_STATUSEFFECTS; i++)
		{
			if (m_apStatusEffects[i])
			{
				ParticleProp()->StopEmissionAndDestroyImmediately(m_apStatusEffects[i]);
				m_apStatusEffects[i] = NULL;
			}
		}
	}
	else
	{
		for (int i = 0; i < TOTAL_STATUSEFFECTS; i++)
		{
			if ((m_pStats->m_iStatusEffects&(1<<i)) && !m_apStatusEffects[i] && bShouldDisplayEffects)
			{
				CNewParticleEffect* pParticle = ParticleProp()->Create( VarArgs("status_%s", StatusEffectToString((statuseffect_t)(1<<i))), PATTACH_ABSORIGIN_FOLLOW );
				ParticleProp()->AddControlPoint( pParticle, 1, this, PATTACH_POINT_FOLLOW, "lhand" );
				ParticleProp()->AddControlPoint( pParticle, 2, this, PATTACH_POINT_FOLLOW, "rhand" );
				ParticleProp()->AddControlPoint( pParticle, 3, this, PATTACH_POINT_FOLLOW, "lfoot" );
				ParticleProp()->AddControlPoint( pParticle, 4, this, PATTACH_POINT_FOLLOW, "rfoot" );
				m_apStatusEffects[i] = pParticle;
			}
			else if ((!(m_pStats->m_iStatusEffects&(1<<i)) || !bShouldDisplayEffects) && m_apStatusEffects[i])
			{
				m_apStatusEffects[i]->StopEmission();
				m_apStatusEffects[i] = NULL;
			}
		}
	}

	if ( IsKnockedOut() && !m_pKnockoutEffect )
	{
		m_pKnockoutEffect = ParticleProp()->Create( "kod", PATTACH_ABSORIGIN_FOLLOW );
	}
	else if ( !IsKnockedOut() && m_pKnockoutEffect )
	{
		ParticleProp()->StopEmission( m_pKnockoutEffect );
		m_pKnockoutEffect = NULL;
	}

	// Hand magic effects!
	if (m_eLHEffectElementsOld != m_eLHEffectElements)
	{
		ShowHandMagic(this, m_apLHComboEffects, m_eLHEffectElements, "lmagic");
		m_eLHEffectElementsOld = m_eLHEffectElements;
	}

	if (m_eRHEffectElementsOld != m_eRHEffectElements)
	{
		ShowHandMagic(this, m_apRHComboEffects, m_eRHEffectElements, "rmagic");
		m_eRHEffectElementsOld = m_eRHEffectElements;
	}

	if (GetMagicWeapon())
	{
		if ( GetMagicWeapon()->IsCharging() && GetMagicWeapon()->m_iChargeAttack == 0 && !m_pLHChargeEffect )
		{
			m_pLHChargeEffect = ParticleProp()->Create( "hand_charge", PATTACH_POINT_FOLLOW, "lmagic" );
		}
		else if ( !GetMagicWeapon()->IsCharging() && m_pLHChargeEffect )
		{
			ParticleProp()->StopEmission( m_pLHChargeEffect );
			m_pLHChargeEffect = NULL;
		}

		if ( GetMagicWeapon()->IsCharging() && GetMagicWeapon()->m_iChargeAttack == 1 && !m_pRHChargeEffect )
		{
			m_pRHChargeEffect = ParticleProp()->Create( "hand_charge", PATTACH_POINT_FOLLOW, "rmagic" );
		}
		else if ( !GetMagicWeapon()->IsCharging() && m_pRHChargeEffect )
		{
			ParticleProp()->StopEmission( m_pRHChargeEffect );
			m_pRHChargeEffect = NULL;
		}
	}
}

const QAngle& C_CFPlayer::EyeAngles()
{
	if ( IsLocalPlayer() && !g_nKillCamMode )
	{
		return BaseClass::EyeAngles();
	}
	else
	{
		return m_angEyeAngles;
	}
}

void C_CFPlayer::CalcView( Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov )
{
	if (gpGlobals->curtime < m_flCameraCinematicUntil)
	{
		C_BaseEntity* pEnt = m_hCameraCinematic;

		if (pEnt)
		{
			C_BaseAnimating* pCamera = pEnt->GetBaseAnimating();
			if (pCamera)
			{
				Vector vecCamera;
				QAngle angCamera;
				pCamera->GetBonePosition(pCamera->LookupBone("cin_Camera.camera"), vecCamera, angCamera);
				eyeOrigin = vecCamera;
				eyeAngles = angCamera;

				if (pCamera->LookupBone("cin_Camera.fov") >= 0)
				{
					Vector vecFOV;
					QAngle angFOV;
					pCamera->GetBonePosition(pCamera->LookupBone("cin_Camera.fov"), vecFOV, angFOV);
					Vector vecDistance = vecFOV - vecCamera;

					fov = RemapValClamped(vecDistance.Length(), 5, 100, 5, 100);
				}

				return;
			}
		}
	}

	BaseClass::CalcView( eyeOrigin, eyeAngles, zNear, zFar, fov );

	zNear = 3;
}

void C_CFPlayer::CalcInEyeCamView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov)
{
	C_CFPlayer *pTarget = ToCFPlayer(GetObserverTarget());

	if ( !pTarget ) 
	{
		// just copy a save in-map position
		VectorCopy( EyePosition(), eyeOrigin );
		VectorCopy( EyeAngles(), eyeAngles );
		return;
	};

	if ( pTarget->ShouldForceThirdPerson() )
	{
		CalcChaseCamView( eyeOrigin, eyeAngles, fov );
		return;
	}

	fov = GetFOV();	// TODO use tragets FOV

	m_flObserverChaseDistance = 0.0;

	eyeAngles = pTarget->EyeAngles();
	eyeOrigin = pTarget->EyePosition();

	// Apply punch angle
	VectorAdd( eyeAngles, GetPunchAngle(), eyeAngles );

	engine->SetViewAngles( eyeAngles );
}

void C_CFPlayer::CalcChaseCamView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov)
{
	C_CFPlayer *pTarget = ToCFPlayer(GetObserverTarget());

	if ( !pTarget ) 
	{
		// just copy a save in-map position
		VectorCopy( EyePosition(), eyeOrigin );
		VectorCopy( EyeAngles(), eyeAngles );
		return;
	};

	// If our target isn't visible, we're at a camera point of some kind.
	// Instead of letting the player rotate around an invisible point, treat
	// the point as a fixed camera.
	if ( !pTarget->GetBaseAnimating() && !pTarget->GetModel() )
	{
		CalcRoamingView( eyeOrigin, eyeAngles, fov );
		return;
	}

	// The following code was ripped from CCFInput::CAM_ThirdPersonNormalThink.
	// Should probably delete it and move that code to C_CFPlayer so that it can be used here too.

	eyeAngles = pTarget->EyeAngles();

	Vector vecEngineForward, vecEngineRight, vecEngineUp;
	AngleVectors(eyeAngles, &vecEngineForward, &vecEngineRight, &vecEngineUp);

	Vector vecEngineForward2D;
	AngleVectors(QAngle(0, eyeAngles[YAW], 0), &vecEngineForward2D);

	// Don't use EyePosition() because that changes depending on the animation.
	Vector vecCameraCenter = pTarget->GetAbsOrigin() + VEC_VIEW;

	m_bThirdPositionMelee = true;
	if (pTarget->GetPrimaryWeapon() && !pTarget->GetPrimaryWeapon()->IsMeleeWeapon())
		m_bThirdPositionMelee = false;
	if (pTarget->GetSecondaryWeapon() && !pTarget->GetSecondaryWeapon()->IsMeleeWeapon())
		m_bThirdPositionMelee = false;

	Vector vecRearRightPosition = vecCameraCenter + vecEngineRight * cvar->FindVar("cam_right")->GetFloat() - vecEngineForward * cvar->FindVar("cam_back")->GetFloat();
	Vector vecRearMeleePosition = vecCameraCenter + vecEngineRight * cvar->FindVar("cam_right_melee")->GetFloat() - vecEngineForward * cvar->FindVar("cam_back_melee")->GetFloat();

	m_flThirdPositionMeleeWeight = Approach(m_bThirdPositionMelee?0:1, m_flThirdPositionMeleeWeight, gpGlobals->frametime * (1/cvar->FindVar("cam_switchtime")->GetFloat()));

	float flWeight = Gain(m_flThirdPositionMeleeWeight, 0.8f);

	eyeOrigin = vecRearMeleePosition * (1-flWeight) + vecRearRightPosition * flWeight;

	if ( pTarget )
	{
		trace_t trace;

		// Trace back to see if the camera is in a wall.
		CTraceFilterNoNPCsOrPlayer traceFilter( pTarget, COLLISION_GROUP_NONE );
		UTIL_TraceHull( vecCameraCenter, eyeOrigin,
			Vector(-9,-9,-9), Vector(9,9,9),
			MASK_SOLID, &traceFilter, &trace );

		if( trace.fraction < 1.0 )
			eyeOrigin = trace.endpos;
	}

	fov = GetFOV();
}

void C_CFPlayer::ThirdPersonSwitch( bool bThirdperson )
{
	BaseClass::ThirdPersonSwitch(bThirdperson);

	Instructor_LessonLearned(HINT_X_THIRDPERSON);
}

bool C_CFPlayer::ShouldForceThirdPerson()
{
	if (IsKnockedOut())
		return true;
	
	if (m_hReviving != NULL)
		return true;

	if (HasObjective())
		return true;

	return false;
}

C_CFPlayer* C_CFPlayer::GetDirectTarget()
{
	Assert(!m_hDirectTarget.Get() || m_hDirectTarget.Get()->IsPlayer());

	return (C_CFPlayer*)(m_hDirectTarget.Get());
}

C_CFPlayer* C_CFPlayer::GetRecursedTarget()
{
	Assert(!m_hRecursedTarget.Get() || m_hRecursedTarget.Get()->IsPlayer());

	return (C_CFPlayer*)(m_hRecursedTarget.Get());
}

C_CFPlayer* C_CFPlayer::GetLocalCFPlayer()
{
	return ToCFPlayer( C_BasePlayer::GetLocalPlayer() );
}


const QAngle& C_CFPlayer::GetRenderAngles()
{
	if ( IsRagdoll() )
	{
		return vec3_angle;
	}
	else
	{
		return m_PlayerAnimState->GetRenderAngles();
	}
}


void C_CFPlayer::UpdateClientSideAnimation()
{
	// Update the animation data. It does the local check here so this works when using
	// a third-person camera (and we don't have valid player angles).
	if ( this == C_CFPlayer::GetLocalCFPlayer() )
	{
		if (m_hReviving != NULL || m_hReviver != NULL)
		{
			C_CFPlayer* pTarget = m_hReviving;
			if (!pTarget)
				pTarget = m_hReviver;

			// Snap the animation to face the model during fatalities.
			Vector vecToTarget = pTarget->GetAbsOrigin() - GetAbsOrigin();
			QAngle angToTarget;
			VectorAngles(vecToTarget, angToTarget);

			m_PlayerAnimState->Update( angToTarget[YAW], angToTarget[PITCH] );
		}
		else
			m_PlayerAnimState->Update( EyeAngles()[YAW], EyeAngles()[PITCH] );
	}
	else
		m_PlayerAnimState->Update( m_angEyeAngles[YAW], m_angEyeAngles[PITCH] );

	BaseClass::UpdateClientSideAnimation();
}

bool C_CFPlayer::CreateMove( float flInputSampleTime, CUserCmd *pCmd )
{
	return BaseClass::CreateMove(flInputSampleTime, pCmd);
}

void C_CFPlayer::PostDataUpdate( DataUpdateType_t updateType )
{
	// C_BaseEntity assumes we're networking the entity's angles, so pretend that it
	// networked the same value we already have.
	SetNetworkAngles( GetLocalAngles() );
	
	BaseClass::PostDataUpdate( updateType );
}

void C_CFPlayer::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}

	UpdateVisibility();

	if ( m_bOverdrive && !m_pOverdriveEffect )
	{
		m_pOverdriveEffect = ParticleProp()->Create( "overdrive", PATTACH_ABSORIGIN_FOLLOW );
	}
	else if ( !m_bOverdrive && m_pOverdriveEffect )
	{
		ParticleProp()->StopEmission( m_pOverdriveEffect );
		m_pOverdriveEffect = NULL;
	}
}


void C_CFPlayer::DoAnimationEvent( PlayerAnimEvent_t event, int mData, bool bSecondary )
{
	if ( IsLocalPlayer() )
	{
		if ( prediction->InPrediction() && !prediction->IsFirstTimePredicted() )
			return;
	}

	MDLCACHE_CRITICAL_SECTION();
	m_PlayerAnimState->DoAnimationEvent( event, mData, bSecondary );
}

bool C_CFPlayer::GetIntervalMovement( float flIntervalUsed, bool &bMoveSeqFinished, Vector &newPosition, QAngle &newAngles )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );
	if (! pstudiohdr || !pstudiohdr->SequencesAvailable())
		return false;

	float flComputedCycleRate = GetSequenceCycleRate( pstudiohdr, GetSequence() );
	
	float flNextCycle = GetCycle() + flIntervalUsed * flComputedCycleRate * m_flPlaybackRate;

	if ((!SequenceLoops()) && flNextCycle > 1.0)
	{
		flIntervalUsed = GetCycle() / (flComputedCycleRate * m_flPlaybackRate);
		flNextCycle = 1.0;
		bMoveSeqFinished = true;
	}
	else
	{
		bMoveSeqFinished = false;
	}

	Vector deltaPos;
	QAngle deltaAngles;

	float poseParameters[MAXSTUDIOPOSEPARAM];
	GetPoseParameters(pstudiohdr, poseParameters);

	if (Studio_SeqMovement( pstudiohdr, GetSequence(), GetCycle(), flNextCycle, poseParameters, deltaPos, deltaAngles ))
	{
		VectorYawRotate( deltaPos, GetLocalAngles().y, deltaPos );
		newPosition = GetLocalOrigin() + deltaPos;
		newAngles.Init();
		newAngles.y = GetLocalAngles().y + deltaAngles.y;
		return true;
	}
	else
	{
		newPosition = GetLocalOrigin();
		newAngles = GetLocalAngles();
		return false;
	}
}

bool C_CFPlayer::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	if ( collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT )
		return false;

	return BaseClass::ShouldCollide( collisionGroup, contentsMask );
}

void C_CFPlayer::Touch( CBaseEntity *pOther )
{
	SharedTouch(pOther);
}

bool C_CFPlayer::ShouldDraw( void )
{
	// If we're dead, our ragdoll will be drawn for us instead.
	if ( !IsAlive() && !IsKnockedOut() )
		return false;

	if( GetTeamNumber() == TEAM_SPECTATOR )
		return false;

	if( IsLocalPlayer() && IsRagdoll() )
		return true;

	if (m_hCameraCinematic != NULL)
		return false;

	// Skip C_BasePlayer::ShouldDraw() because it has a bunch of logic we don't care for.
	return C_BaseCombatCharacter::ShouldDraw();
}

int C_CFPlayer::DrawModel( int flags )
{
	// if local player is spectating this player in first person mode, don't draw it
	C_CFPlayer * pPlayer = GetLocalCFPlayer();

	if (m_bIsDecapitated)
		SetBodygroup(1, 1);
	else if ((!input->CAM_IsThirdPerson() && pPlayer == this)
		|| pPlayer->IsFirstPersonSpectating(this))
		SetBodygroup(1, 1);
	else
		SetBodygroup(1, 0);

	if (IsFuse())
	{
		if ((!input->CAM_IsThirdPerson() && pPlayer == this)
			|| pPlayer->IsFirstPersonSpectating(this))
			SetBodygroup(2, 0);
		else
			SetBodygroup(2, 1);
	}

	// Skip C_BasePlayer::DrawModel() because it has a bunch of logic we don't care for.
	int iResult = C_BaseCombatCharacter::DrawModel( flags );

	if (C_CFPlayer::GetLocalCFPlayer() == this || pPlayer->IsObserver() && pPlayer->GetObserverTarget() && ToCFPlayer(pPlayer->GetObserverTarget()) == this)
		DrawTargets();

	// Put submodels back where they are supposed to be so that shadows and such render them properly.
	SetBodygroup(1, m_bIsDecapitated);
	if (IsFuse())
		SetBodygroup(2, 1);

	if (gpGlobals->curtime - m_flShieldTime < 0.5f)
	{
		Vector vecDmgDirection = m_vecShieldDmgOrigin - GetCentroid();
		QAngle angShield;
		VectorAngles(vecDmgDirection, angShield);
		m_pBarrier->SetAbsAngles(angShield);
		m_pBarrier->SetAbsOrigin(GetAbsOrigin());

		float flAlpha = 0;
		if ((gpGlobals->curtime - m_flShieldTime) < 0.2f)
			flAlpha = RemapVal(gpGlobals->curtime - m_flShieldTime, 0.0f, 0.2f, 0, 255);
		else
			flAlpha = RemapVal(gpGlobals->curtime - m_flShieldTime, 0.2f, 0.5f, 255, 0);

		flAlpha *= m_flShieldStrength;

		if (C_CFPlayer::GetLocalCFPlayer() == this)
			flAlpha /= 2;

		if (flAlpha)
		{
			m_pBarrier->SetBodygroup(0, m_bShieldPhysical);
			m_pBarrier->SetRenderColorA(flAlpha);
			m_pBarrier->DrawModel(flags);
		}
	}

	return iResult;
}

void C_CFPlayer::DrawTargets()
{
	if (m_hDirectTarget != m_hDrawingDirectTarget)
	{
		m_hDrawingDirectTarget = m_hDirectTarget;
		m_flReceivedDirectTarget = gpGlobals->curtime;
	}

	if (m_hRecursedTarget != m_hDrawingRecursedTarget)
	{
		m_hDrawingRecursedTarget = m_hRecursedTarget;
		m_flReceivedRecursedTarget = gpGlobals->curtime;
	}

	for (int i = 1; i < gpGlobals->maxClients; i++)
	{
		C_BasePlayer* pPlayer = UTIL_PlayerByIndex(i);

		if (!pPlayer)
			continue;

		C_CFPlayer* pCFPlayer = ToCFPlayer(pPlayer);

		if ( this == pCFPlayer )
			continue;

		if (!pPlayer->IsAlive())
			continue;

		Vector vecTargetPosition = pCFPlayer->GetAbsOrigin() + Vector(0, 0, 10);

		bool bTeammate = CFGameRules() && CFGameRules()->PlayerRelationship(this, pCFPlayer) == GR_TEAMMATE;

		float flWooshTime = 0;
		if (m_hDirectTarget == pCFPlayer)
			flWooshTime = gpGlobals->curtime - m_flReceivedDirectTarget;
		else
			flWooshTime = gpGlobals->curtime - m_flReceivedRecursedTarget;

		if ( (m_hDirectTarget == pCFPlayer || m_hRecursedTarget == pCFPlayer) )
		{
			DrawTarget(bTeammate?"effects/target_friend":"effects/target_enemy", vecTargetPosition, 0, flWooshTime);
		}

		if (m_hDirectTarget == pCFPlayer && m_hRecursedTarget != pCFPlayer && bTeammate)
		{
			QAngle angTarget;
			VectorAngles((m_hRecursedTarget->GetAbsOrigin() - pCFPlayer->GetAbsOrigin()), angTarget);
			DrawTarget(bTeammate?"effects/targeting_friend":"effects/targeting_enemy", vecTargetPosition, angTarget.y, flWooshTime);
		}

		if (pCFPlayer->m_hDirectTarget == this || pCFPlayer->m_hRecursedTarget == this)
		{
			QAngle angTarget;
			VectorAngles((GetAbsOrigin() - pCFPlayer->GetAbsOrigin()), angTarget);
			DrawTarget(bTeammate?"effects/targeting_friend":"effects/targeting_enemy", vecTargetPosition, angTarget.y, flWooshTime);
		}
	}
}

void C_CFPlayer::DrawTarget(const char* pszMaterial, Vector vecOrigin, float flYaw, float flElapsed, CViewSetup* pPushView)
{
	CMatRenderContextPtr pRenderContext( materials );

	Frustum drawFrustum;

	if (pPushView)
		render->Push3DView( *pPushView, 0, NULL, drawFrustum );

	IMaterial* pMaterial = materials->FindMaterial( pszMaterial, TEXTURE_GROUP_CLIENT_EFFECTS );

	pRenderContext->Bind( pMaterial );

	IMesh* pMesh = pRenderContext->GetDynamicMesh();

	float flWooshTime = 0.2f;

	unsigned char color[4];
	color[0] = 255;
	color[1] = 255;
	color[2] = 255;
	color[3] = RemapValClamped(flElapsed, 0, flWooshTime, 0, 255);

	Vector vec1;
	Vector vec2;
	Vector vec3;
	Vector vec4;
	Vector vecForward, vecRight, vecUp;

	AngleVectors(QAngle(0, flYaw, 0), &vecForward, &vecRight, &vecUp);

	float flSize = RemapValClamped(flElapsed, 0, flWooshTime, 96, 32);

	vecOrigin.z += RemapValClamped(flElapsed, 0, flWooshTime, 20, 0);

	vec1 = vecOrigin - vecForward * flSize - vecRight * flSize;
	vec2 = vecOrigin + vecForward * flSize - vecRight * flSize;
	vec3 = vecOrigin + vecForward * flSize + vecRight * flSize;
	vec4 = vecOrigin - vecForward * flSize + vecRight * flSize;

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Color4ubv( color );
	meshBuilder.TexCoord2f( 0, 0, 1 );
	meshBuilder.Position3fv( vec1.Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv( color );
	meshBuilder.TexCoord2f( 0, 0, 0 );
	meshBuilder.Position3fv( vec2.Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv( color );
	meshBuilder.TexCoord2f( 0, 1, 0 );
	meshBuilder.Position3fv( vec3.Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv( color );
	meshBuilder.TexCoord2f( 0, 1, 1 );
	meshBuilder.Position3fv( vec4.Base() );
	meshBuilder.AdvanceVertex();
	
	meshBuilder.End();
	pMesh->Draw();

	if (pPushView)
		render->PopView( drawFrustum );
}

bool C_CFPlayer::OnInternalDrawModel( ClientModelRenderInfo_t *pInfo )
{
	// if local player is spectating this player in first person mode, don't draw it
	C_CFPlayer * pPlayer = C_CFPlayer::GetLocalCFPlayer();

	if (!cl_drawlocalplayer.GetBool() && pPlayer == this)
		return false;

	if (m_hCameraCinematic != NULL)
		return false;

	CMatRenderContextPtr pRenderContext( materials );

	if ((!input->CAM_IsThirdPerson() && pPlayer == this)
		|| pPlayer->IsFirstPersonSpectating(this))
		pRenderContext->DepthRange(0.0f, 0.01f);

	return BaseClass::OnInternalDrawModel(pInfo);
}

bool C_CFPlayer::OnPostInternalDrawModel( ClientModelRenderInfo_t *pInfo )
{
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->DepthRange(0.0f, 1.0f);

	return BaseClass::OnPostInternalDrawModel(pInfo);
}

void C_CFPlayer::ShowHandMagic(C_BaseEntity* pEnt, CUtlVector<CNewParticleEffect*>& aHandComboEffects, element_t eElements, const char* pszAttachment)
{
	MDLCACHE_CRITICAL_SECTION();
	int i;
	for (i = 0; i < aHandComboEffects.Count(); i++)
		pEnt->ParticleProp()->StopEmission(aHandComboEffects[i]);

	aHandComboEffects.RemoveAll();

	for (i = 0; i < TOTAL_ELEMENTS; i++)
	{
		if (!(eElements&(1<<i)))
			continue;

		aHandComboEffects.AddToTail(pEnt->ParticleProp()->Create( VarArgs("hand_%s", ElementToString((element_t)(1<<i))), PATTACH_POINT_FOLLOW, pszAttachment ));
	}
}

void C_CFPlayer::ValidateModelIndex( void )
{
	if (!C_BasePlayer::GetLocalPlayer())
		return;

	C_CFPlayer* pLocal = C_CFPlayer::GetLocalCFPlayer();

	if ( pLocal && pLocal->IsPariah() && !IsFuse() && !IsPariah() )
		m_nModelIndex = modelinfo->GetModelIndex( PLAYER_MODEL_SHADE );
	else
		m_nModelIndex = modelinfo->GetModelIndex( GetCFModelName() );

	BaseClass::ValidateModelIndex();
}

KeyValues *C_CFPlayer::GetSequenceKeyValues( int iSequence )
{
	const char *szText = Studio_GetKeyValueText( GetModelPtr(), iSequence );

	if (szText)
	{
		KeyValues *seqKeyValues = new KeyValues("");
		if ( seqKeyValues->LoadFromBuffer( modelinfo->GetModelName( GetModel() ), szText ) )
		{
			return seqKeyValues;
		}
		seqKeyValues->deleteThis();
	}
	return NULL;
}

bool C_CFPlayer::IsUseableEntity( CBaseEntity *pEntity, unsigned int requiredCaps )
{
	if ( pEntity )
	{
		int caps = pEntity->ObjectCaps();
		if ( caps & (FCAP_IMPULSE_USE|FCAP_CONTINUOUS_USE|FCAP_ONOFF_USE|FCAP_DIRECTIONAL_USE) )
		{
			if ( (caps & requiredCaps) == requiredCaps )
			{
				return true;
			}
		}
	}

	return false;
}

CWeaponCFBase* C_CFPlayer::GetActiveCFWeapon() const
{
	return dynamic_cast< CWeaponCFBase* >( GetActiveWeapon() );
}

ShadowType_t C_CFPlayer::ShadowCastType( void )
{
	if ( !IsVisible() )
		return SHADOWS_NONE;
	else
		return SHADOWS_RENDER_TO_TEXTURE_DYNAMIC;
}

void C_CFPlayer::GetShadowRenderBounds( Vector &mins, Vector &maxs, ShadowType_t shadowType )
{
	GetRenderBounds( mins, maxs );

	// We do this because the normal bbox calculations don't take pose params into account, and 
	// the rotation of the guy's upper torso can place his gun a ways out of his bbox, and 
	// the shadow will get cut off as he rotates.
	//
	// Thus, we give it some padding here.
	mins -= Vector( 4, 4, 0 );
	maxs += Vector( 4, 4, 0 );
}

void C_CFPlayer::SetRecoilAmount( float flPitchRecoil, float flYawRecoil )
{
	//Slam the values, abandon previous recoils
	m_flPitchRecoilAccumulator = flPitchRecoil;

	flYawRecoil = flYawRecoil * random->RandomFloat( 0.8, 1.1 );

	if( random->RandomInt( 0,1 ) <= 0 )
		m_flYawRecoilAccumulator = flYawRecoil;
	else
		m_flYawRecoilAccumulator = -flYawRecoil;

	m_flRecoilTimeRemaining = RECOIL_DURATION;
}

//Get the amount of recoil we should do this frame
void C_CFPlayer::GetRecoilToAddThisFrame( float &flPitchRecoil, float &flYawRecoil )
{
	if( m_flRecoilTimeRemaining <= 0 )
	{
		flPitchRecoil = 0.0;
		flYawRecoil = 0.0;
		return;
	}

	float flRemaining = min( m_flRecoilTimeRemaining, gpGlobals->frametime );

	float flRecoilProportion = ( flRemaining / RECOIL_DURATION );

	flPitchRecoil = m_flPitchRecoilAccumulator * flRecoilProportion;
	flYawRecoil = m_flYawRecoilAccumulator * flRecoilProportion;

	m_flRecoilTimeRemaining -= gpGlobals->frametime;
}

bool C_CFPlayer::AutoFollowMode()
{
	return cl_autofollowmode.GetBool();
}


bool C_CFPlayer::IsFirstPersonSpectating( C_BaseEntity* pTarget )
{
	if (!IsObserver())
		return false;

	if (GetObserverMode() != OBS_MODE_IN_EYE)
		return false;

	if (!GetObserverTarget())
		return false;

	if (GetObserverTarget() != pTarget)
		return false;

	if (ToCFPlayer(pTarget) && ToCFPlayer(pTarget)->ShouldForceThirdPerson())
		return false;

	return true;
}

void RecvProxy_Statistics( const RecvProp *pProp, void **pOut, void *pData, int objectID )
{
	CStatistics *pStats = ((C_BasePlayer*)pData)->m_pStats;
	Assert( pStats );
	*pOut = pStats;
}
