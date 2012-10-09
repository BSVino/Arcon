//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: The CF Game rules 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "cf_gamerules.h"
#include "ammodef.h"
#include "KeyValues.h"
#include "weapon_cfbase.h"
#include "takedamageinfo.h"

#ifdef CLIENT_DLL

	#include "c_cf_player.h"

#else
	
	#include "cf_player.h"
	#include "voice_gamemgr.h"
	#include "team.h"
	#include "objectives.h"
	#include "cf_player_resource.h"
	#include "cf_objective_resource.h"
	#include "cf_team.h"
	#include "weapon_pariah.h"
	#include "team_control_point_master.h"
	#include "cf_spawnpoint.h"
	#include "mp_shareddefs.h"
	#include "bot.h"

#endif


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef GAME_DLL
// CF overrides the default value of this convar

#ifdef _DEBUG
#define WAITING_FOR_PLAYERS_FLAGS	0
#else
#define WAITING_FOR_PLAYERS_FLAGS	FCVAR_DEVELOPMENTONLY
#endif

ConVar mp_waitingforplayers_time( "mp_waitingforplayers_time", (IsX360()?"15":"30"), FCVAR_GAMEDLL | WAITING_FOR_PLAYERS_FLAGS, "WaitingForPlayers time length in seconds" );

void ValidateCapturesPerRound( IConVar *pConVar, const char *oldValue, float flOldValue )
{
	ConVarRef var( pConVar );

	if ( var.GetInt() <= 0 )
	{
		// reset the flag captures being played in the current round
		int nTeamCount = GetNumberOfTeams();
		for ( int iTeam = FIRST_GAME_TEAM; iTeam < nTeamCount; ++iTeam )
		{
			CCFTeam *pTeam = GetGlobalCFTeam( iTeam );
			if ( !pTeam )
				continue;

			pTeam->SetFlagCaptures( 0 );
		}
	}
}
#endif	

ConVar cf_fuse_health_save( "cf_fuse_health_save", ".2", FCVAR_DEVELOPMENTONLY, "At what percent of his max health does the Fuse stop taking damage from normal people?");
ConVar cf_ctf_tugofwar( "cf_ctf_tugofwar", "1", FCVAR_REPLICATED|FCVAR_DEVELOPMENTONLY, "Makes CTF like tug of war, where there are only so many flags and one team must collect them all to win.");
ConVar cf_flag_caps_per_round( "cf_flag_caps_per_round", "3", FCVAR_REPLICATED, "Number of flag captures per round on CTF maps. Set to 0 to disable.", true, 0, true, 9
#ifdef GAME_DLL
							  , ValidateCapturesPerRound
#endif
							  );

#ifndef CLIENT_DLL
LINK_ENTITY_TO_CLASS(info_player_terrorist, CPointEntity);
LINK_ENTITY_TO_CLASS(info_player_counterterrorist,CPointEntity);
#endif

REGISTER_GAMERULES_CLASS( CCFGameRules );


BEGIN_NETWORK_TABLE_NOBASE( CCFGameRules, DT_CFGameRules )
#ifdef GAME_DLL
	SendPropInt( SENDINFO( m_eGameMode ),		6,	SPROP_UNSIGNED ),
	SendPropEHandle		( SENDINFO( m_hFuse ) ),
	SendPropEHandle		( SENDINFO( m_hPariah ) ),
	SendPropEHandle		( SENDINFO( m_hPariahBlade ) ),
#else
	RecvPropInt( RECVINFO( m_eGameMode ) ),
	RecvPropEHandle		( RECVINFO( m_hFuse ) ),
	RecvPropEHandle		( RECVINFO( m_hPariah ) ),
	RecvPropEHandle		( RECVINFO( m_hPariahBlade ) ),
#endif
END_NETWORK_TABLE()


LINK_ENTITY_TO_CLASS( cf_gamerules, CCFGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( CFGameRulesProxy, DT_CFGameRulesProxy )


#ifdef CLIENT_DLL
	void RecvProxy_CFGameRules( const RecvProp *pProp, void **pOut, void *pData, int objectID )
	{
		CCFGameRules *pRules = CFGameRules();
		Assert( pRules );
		*pOut = pRules;
	}

	BEGIN_RECV_TABLE( CCFGameRulesProxy, DT_CFGameRulesProxy )
		RecvPropDataTable( "cf_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE( DT_CFGameRules ), RecvProxy_CFGameRules )
	END_RECV_TABLE()
#else
	void *SendProxy_CFGameRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
	{
		CCFGameRules *pRules = CFGameRules();
		Assert( pRules );
		pRecipients->SetAllRecipients();
		return pRules;
	}

	BEGIN_SEND_TABLE( CCFGameRulesProxy, DT_CFGameRulesProxy )
		SendPropDataTable( "cf_gamerules_data", 0, &REFERENCE_SEND_TABLE( DT_CFGameRules ), SendProxy_CFGameRules )
	END_SEND_TABLE()
#endif

ConVar cf_gamemode( "cf_gamemode", "ctf", FCVAR_REPLICATED );

#ifdef CLIENT_DLL

	bool CCFGameRules::ShouldShowFuse(CCFPlayer* pPlayer)
	{
		if (GetGameMode() == CF_GAME_PARIAH)
		{
			if (PlayerRelationship(pPlayer, m_hFuse) == GR_NOTTEAMMATE)
				return false;

			// This might happen sometimes!
			if (m_hFuse == NULL)
				return false;

			return true;
		}
		else
			return false;
	}

	Vector CCFGameRules::GetFuseLocation()
	{
		if (m_hFuse != NULL)
			return m_hFuse->WorldSpaceCenter();

		AssertMsg(false, "Couldn't find a Fuse location.");
		return Vector(0,0,0);
	}

	bool CCFGameRules::ShouldShowPariah(CCFPlayer* pPlayer)
	{
		if (GetGameMode() == CF_GAME_PARIAH)
		{
			// Show the Pariah's location to everybody but the Fuse.
			if (pPlayer->IsFuse())
				return false;

			if (m_hPariah != NULL || m_hPariahBlade != NULL)
				return true;

			return false;
		}
		else
			return false;
	}

	Vector CCFGameRules::GetPariahLocation()
	{
		if (m_hPariah != NULL)
			return m_hPariah->WorldSpaceCenter();
		else if (m_hPariahBlade != NULL)
			return m_hPariahBlade->WorldSpaceCenter();

		AssertMsg(false, "Couldn't find a Pariah location.");
		return Vector(0,0,0);
	}

#else

	// --------------------------------------------------------------------------------------------------- //
	// Voice helper
	// --------------------------------------------------------------------------------------------------- //

	class CVoiceGameMgrHelper : public IVoiceGameMgrHelper
	{
	public:
		virtual bool		CanPlayerHearPlayer( CBasePlayer *pListener, CBasePlayer *pTalker, bool &bProximity )
		{
			// Dead players can only be heard by other dead team mates
			if ( pTalker->IsAlive() == false )
			{
				if ( pListener->IsAlive() == false )
					return ( pListener->InSameTeam( pTalker ) );

				return false;
			}

			return ( pListener->InSameTeam( pTalker ) );
		}
	};
	CVoiceGameMgrHelper g_VoiceGameMgrHelper;
	IVoiceGameMgrHelper *g_pVoiceGameMgrHelper = &g_VoiceGameMgrHelper;



	// --------------------------------------------------------------------------------------------------- //
	// Globals.
	// --------------------------------------------------------------------------------------------------- //
/*
	// NOTE: the indices here must match TEAM_TERRORIST, TEAM_CT, TEAM_SPECTATOR, etc.
	char *sTeamNames[] =
	{
		"Unassigned",
		"Spectator",
		"Numeni",
		"Machindo",
		"Numeni Sect",
		"Machindo Faction",
	};
*/
	// --------------------------------------------------------------------------------------------------- //
	// Global helper functions.
	// --------------------------------------------------------------------------------------------------- //
	
	// World.cpp calls this but we don't use it in CF.
	void InitBodyQue()
	{
	}

	Vector DropToGround( 
		CBaseEntity *pMainEnt, 
		const Vector &vPos, 
		const Vector &vMins, 
		const Vector &vMaxs )
	{
		trace_t trace;
		UTIL_TraceHull( vPos, vPos + Vector( 0, 0, -500 ), vMins, vMaxs, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &trace );
		return trace.endpos;
	}

	// --------------------------------------------------------------------------------------------------- //
	// CCFGameRules implementation.
	// --------------------------------------------------------------------------------------------------- //

	CCFGameRules::CCFGameRules()
	{
		CreateEntityByName( "cf_gamerules" );

		CCFTeam::CreateTeams();

#if defined(ARCON_MOD) || defined(ARCON_DEMO)
		m_bServerValidated = false;
#endif
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	CCFGameRules::~CCFGameRules()
	{
		// Note, don't delete each team since they are in the gEntList and will 
		// automatically be deleted from there, instead.
		g_Teams.Purge();
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	// Input  :
	// Output :
	//-----------------------------------------------------------------------------
	bool CCFGameRules::ClientCommand( CBaseEntity *pEdict, const CCommand &args )
	{
		if (BaseClass::ClientCommand( pEdict, args ))
			return true;

		CCFPlayer *pPlayer = ToCFPlayer(pEdict);

		return pPlayer->ClientCommand( args );
	}

	void CCFGameRules::ClientDisconnected( edict_t *pClient )
	{
		CCFPlayer *pPlayer = ( CCFPlayer * )CBaseEntity::Instance( pClient );

		// Do this first, the base ClientDisconnected removes all weapons.
		pPlayer->BecomeNormal();

		BaseClass::ClientDisconnected( pClient );

		if (pPlayer->HasObjective())
			pPlayer->GetObjective()->Drop();

		pPlayer->RemoveTargeters();
		pPlayer->SetDirectTarget(NULL);
	}

	void CCFGameRules::FireGameEvent( IGameEvent *event )
	{
		const char *eventName = event->GetName();

	#ifdef GAME_DLL
		if ( !Q_strcmp( eventName, "teamplay_point_captured" ) )
		{
			RecalculateControlPointState();

			// keep track of how many times each team caps
			int iTeam = event->GetInt( "team" );
			Assert( iTeam >= FIRST_GAME_TEAM && iTeam < CF_TEAM_COUNT );
			m_iNumCaps[iTeam]++;
		}
	#endif

	}

	const char *CCFGameRules::GetGameDescription( void )
	{
		switch (GetGameMode())
		{
		default:
			return "Arcon";

		case CF_GAME_CTF:
			return "Arcon CTF";

		case CF_GAME_PARIAH:
			return "Arcon Pariah";
		}
	}

	void CCFGameRules::LevelInitPostEntity()
	{
		m_eGameMode = GameModeFromString(cf_gamemode.GetString());

		CInfoGame* pGame = dynamic_cast<CInfoGame*>(gEntList.FindEntityByClassname(NULL, "info_game"));

		if (!pGame)
			pGame = static_cast<CInfoGame*>(CreateEntityByName( "info_game" ));

		Assert(pGame);

		pGame->LevelInitPostEntity();

		if (!pGame->SupportsGameMode(m_eGameMode))
			m_eGameMode = CF_GAME_TDM;

#ifdef ARCON_DEMO
		if (m_eGameMode == CF_GAME_PARIAH)
			m_eGameMode = CF_GAME_TDM;
#endif

		LevelInitGameMode();
	}

	void CCFGameRules::LevelInitGameMode()
	{
		CBaseEntity *pEnt = NULL;

		CUtlVector<CBaseEntity*> aObjectives;
		CUtlVector<CCFPlayer*> aPlayers;

		while ( (pEnt = gEntList.NextEnt( pEnt )) != NULL )
		{
			if (!!(pEnt->GetSpawnFlags() & SF_SUPPORTS_ANY) && !(pEnt->GetSpawnFlags() & GameModeSpawnFlag(m_eGameMode)))
				UTIL_Remove(pEnt);
			else
			{
				if (m_eGameMode == CF_GAME_PARIAH)
				{
					if (FStrEq(pEnt->GetClassname(), "info_objective") || FStrEq(pEnt->GetClassname(), "info_objective_flag"))
						aObjectives.AddToTail(pEnt);
					else if (FStrEq(pEnt->GetClassname(), "player") || FStrEq(pEnt->GetClassname(), "cf_bot"))
						aPlayers.AddToTail(dynamic_cast<CCFPlayer*>(pEnt));
				}
			}
		}

		if (m_eGameMode == CF_GAME_PARIAH)
		{
			Assert(aObjectives.Count());
			int i;

			m_avecPariahStartingPoints.RemoveAll();

			for (i = 0; i < aObjectives.Count(); i++)
			{
				CInfoObjective* pObjective = dynamic_cast<CInfoObjective*>(aObjectives[i]);
				if (pObjective)
					m_avecPariahStartingPoints.AddToTail(pObjective->WorldSpaceCenter());

				// It'll get re-generated next round, don't need it anymore.
				UTIL_Remove(aObjectives[i]);
			}

			PickRandomFuse();
			PickRandomPariah();
		}
	}

	void CCFGameRules::PickRandomFuse()
	{
		int i;
		CUtlVector<CCFPlayer*> aPlayers;

		// Find all players.
		for (i = 1; i < gpGlobals->maxClients; i++)
		{
			CBasePlayer* pPlayer = UTIL_PlayerByIndex(i);
			if (pPlayer && pPlayer->IsAlive())
				aPlayers.AddToTail(ToCFPlayer(pPlayer));
		}

		for (i = 0; i < aPlayers.Count(); i++)
		{
			Assert(aPlayers[i] != NULL);
			if (aPlayers[i] == NULL)
				continue;

			RemoveFuse(aPlayers[i]);
		}

		if (aPlayers.Count())
		{
			// Pick a random one.
			int iFuse = random->RandomInt( 0, aPlayers.Count()-1 );
			MakeFuse(aPlayers[iFuse]);
		}
	}

	void CCFGameRules::PickRandomPariah()
	{
		int iPariah = random->RandomInt( 0, m_avecPariahStartingPoints.Count()-1 );
		MakePariahBlade(m_avecPariahStartingPoints[iPariah]);
	}

	void CCFGameRules::MakePariahBlade( Vector vecOrigin )
	{
		Assert(m_hPariahBlade == NULL);

		CWeaponPariahBlade* pBlade = static_cast<CWeaponPariahBlade*>(CreateEntityByName( "weapon_pariah" ));
		pBlade->SetAbsOrigin(vecOrigin);
		pBlade->SetAbsAngles( QAngle(0, 0, 1) );

		pBlade->Spawn();
		pBlade->Activate();

		m_hPariahBlade = pBlade;
	}

	void CCFGameRules::MakeFuse(CCFPlayer* pPlayer)
	{
		if (pPlayer->IsFuse())
			return;

		if (!pPlayer->IsAlive())
			return;

		if (pPlayer->IsCaptain() || pPlayer->IsSergeant())
			pPlayer->GetCFTeam()->DemotePlayer(pPlayer);

		RemovePariah(pPlayer);

		if (m_hFuse != NULL)
			RemoveFuse();

		pPlayer->BecomeFuse();
		m_hFuse = pPlayer;
	}

	bool CCFGameRules::MakePariah(CCFPlayer* pPlayer, class CWeaponPariahBlade* pBlade)
	{
		if (pPlayer->IsFuse())
			return false;

		if (pPlayer->IsPariah())
			return false;

		if (!pPlayer->IsAlive())
			return false;

		if (!pBlade)
			return false;

		if (pPlayer->IsCaptain() || pPlayer->IsSergeant())
			pPlayer->GetCFTeam()->DemotePlayer(pPlayer);

		if (m_hPariah != NULL)
			RemovePariah();

		RemoveFuse(pPlayer);

		pPlayer->BecomePariah(pBlade);
		m_hPariah = pPlayer;

		return true;
	}

	void CCFGameRules::RemoveFuse(CCFPlayer* pPlayer)
	{
		if (m_hFuse == NULL)
			return;

		if (pPlayer && m_hFuse.Get() != pPlayer)
			return;

		m_hFuse->DemoteFuse();
		m_hFuse = NULL;
	}

	void CCFGameRules::RemovePariah(CCFPlayer* pPlayer)
	{
		if (m_hPariah == NULL)
			return;

		if (pPlayer && m_hPariah.Get() != pPlayer)
			return;

		m_hPariah->DemotePariah();
		m_hPariah = NULL;
	}

	bool CCFGameRules::ShouldShowDamage(CCFPlayer* pVictim, CCFPlayer* pAttacker)
	{
		if (GetGameMode() == CF_GAME_PARIAH)
		{
			// The Pariah can attack anybody.
			if (pAttacker && pAttacker->IsPariah())
				return true;

			// Otherwise, the Fuse does not show damage.
			if (pVictim && pVictim->IsFuse())
				return false;
		}

		return true;
	}

	bool CCFGameRules::CanGoOverdrive(CCFPlayer* pPlayer)
	{
		return !pPlayer->IsFuse() && !pPlayer->IsPariah() && !pPlayer->HasObjective();
	}

	bool CCFGameRules::CanGiveOverdrive(CCFPlayer* pPlayer)
	{
		return !pPlayer->IsFuse();
	}

	void CCFGameRules::RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore )
	{
		RadiusDamage( info, vecSrcIn, flRadius, iClassIgnore, false );
	}

	// Add the ability to ignore the world trace
	void CCFGameRules::RadiusDamage( const CTakeDamageInfo &inputInfo, const Vector &vecSrcIn, float flRadius, int iClassIgnore, bool bIgnoreWorld )
	{
		CTakeDamageInfo info = inputInfo;

		CBaseEntity *pEntity = NULL;
		trace_t		tr;
		float		flAdjustedDamage, falloff;
		Vector		vecSpot;
		Vector		vecToTarget;
		Vector		vecEndPos;

		Vector vecSrc = vecSrcIn;

		if ( flRadius )
			falloff = info.GetDamage() / flRadius;
		else
			falloff = 1.0;

		int bInWater = (UTIL_PointContents ( vecSrc ) & MASK_WATER) ? true : false;
		
		vecSrc.z += 1;// in case grenade is lying on the ground

		// iterate on all entities in the vicinity.
		for ( CEntitySphereQuery sphere( vecSrc, flRadius ); ( pEntity = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
		{
			if ( pEntity->m_takedamage != DAMAGE_NO )
			{
				// UNDONE: this should check a damage mask, not an ignore
				if ( iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore )
				{// houndeyes don't hurt other houndeyes with their attack
					continue;
				}

				// blast's don't tavel into or out of water
				if (bInWater && pEntity->GetWaterLevel() == 0)
					continue;
				if (!bInWater && pEntity->GetWaterLevel() == 3)
					continue;

				// radius damage can only be blocked by the world
				vecSpot = pEntity->BodyTarget( vecSrc );



				bool bHit = false;

				if( bIgnoreWorld )
				{
					vecEndPos = vecSpot;
					bHit = true;
				}
				else
				{
					UTIL_TraceLine( vecSrc, vecSpot, MASK_SOLID_BRUSHONLY, info.GetInflictor(), COLLISION_GROUP_NONE, &tr );

					if (tr.startsolid)
					{
						// if we're stuck inside them, fixup the position and distance
						tr.endpos = vecSrc;
						tr.fraction = 0.0;
					}

					vecEndPos = tr.endpos;

					if( tr.fraction == 1.0 || tr.m_pEnt == pEntity )
					{
						bHit = true;
					}
				}

				if ( bHit )
				{
					// the explosion can 'see' this entity, so hurt them!
					//vecToTarget = ( vecSrc - vecEndPos );
					vecToTarget = ( vecEndPos - vecSrc );

					// decrease damage for an ent that's farther from the bomb.
					flAdjustedDamage = vecToTarget.Length() * falloff;
					flAdjustedDamage = info.GetDamage() - flAdjustedDamage;
				
					if ( flAdjustedDamage > 0 )
					{
						CTakeDamageInfo adjustedInfo = info;
						adjustedInfo.SetDamage( flAdjustedDamage );

						Vector dir = vecToTarget;
						VectorNormalize( dir );

						// If we don't have a damage force, manufacture one
						if ( adjustedInfo.GetDamagePosition() == vec3_origin || adjustedInfo.GetDamageForce() == vec3_origin )
						{
							CalculateExplosiveDamageForce( &adjustedInfo, dir, vecSrc, 1.5	/* explosion scale! */ );
						}
						else
						{
							// Assume the force passed in is the maximum force. Decay it based on falloff.
							float flForce = adjustedInfo.GetDamageForce().Length() * falloff;
							adjustedInfo.SetDamageForce( dir * flForce );
							adjustedInfo.SetDamagePosition( vecSrc );
						}

						pEntity->TakeDamage( adjustedInfo );
			
						// Now hit all triggers along the way that respond to damage... 
						pEntity->TraceAttackToTriggers( adjustedInfo, vecSrc, vecEndPos, dir );
					}
				}
			}
		}
	}

	void CCFGameRules::CreateStandardEntities()
	{
		g_pPlayerResource = (CCFPlayerResource*)CBaseEntity::Create( "cf_player_manager", vec3_origin, vec3_angle );
		g_pPlayerResource->AddEFlags( EFL_KEEP_ON_RECREATE_ENTITIES );

		g_pObjectiveResource = (CCFObjectiveResource *)CBaseEntity::Create( "cf_objective_resource", vec3_origin, vec3_angle );
		Assert( g_pObjectiveResource );
	}

	static const char *s_CFPreserveEnts[] =
	{
		"cf_gamerules",
		"cf_gamerules_data",
		"cf_team_manager",
		"cf_player_manager",
		"cf_objective_resource",
		"info_game",
		"keyframe_rope",
		"move_rope",
		"info_game",
		"info_player_start",
		"info_player_teamspawn",
		"info_player_deathmatch",
		"weapon_magic",
		"", // END Marker
	};

	bool CCFGameRules::RoundCleanupShouldIgnore( CBaseEntity *pEnt )
	{
		if ( FindInList( s_CFPreserveEnts, pEnt->GetClassname() ) )
			return true;

		return BaseClass::RoundCleanupShouldIgnore( pEnt );
	}

	bool CCFGameRules::ShouldCreateEntity( const char *pszClassName )
	{
		if ( FindInList( s_CFPreserveEnts, pszClassName ) )
			return false;

		return BaseClass::ShouldCreateEntity( pszClassName );
	}

	void CCFGameRules::CleanUpMap( void )
	{
		BaseClass::CleanUpMap();
	}

	void CCFGameRules::GoToIntermission()
	{
		BaseClass::GoToIntermission();

		for ( int i = 1; i <= MAX_PLAYERS; i++ )
		{
			CCFPlayer *pPlayer = ToCFPlayer(UTIL_PlayerByIndex( i ));
			if (!pPlayer)
				continue;

			ToCFPlayer(pPlayer)->ShowCFPanel( CF_SCOREBOARD, true );
		}
	}

	void CCFGameRules::SetInWaitingForPlayers( bool bWaitingForPlayers  )
	{
		if ( GetGameMode() == CF_GAME_TDM )
		{
			m_bInWaitingForPlayers = false;
			return;
		}

		BaseClass::SetInWaitingForPlayers( bWaitingForPlayers );
	}

	bool CCFGameRules::CanGoToStalemate( void )
	{
		// In CTF, don't go to stalemate if one of the flags isn't at home
		if ( m_eGameMode == CF_GAME_CTF )
		{
			CInfoObjective *pFlag = dynamic_cast<CInfoObjective*> ( gEntList.FindEntityByClassname( NULL, "info_objective*" ) );
			while( pFlag )
			{
				if ( !pFlag->IsAtSpawn() )
					return false;

				pFlag = dynamic_cast<CInfoObjective*> ( gEntList.FindEntityByClassname( pFlag, "info_objective*" ) );
			}

			// check that one team hasn't won by capping
			if ( CheckCapsPerRound() )
				return false;
		}

		return BaseClass::CanGoToStalemate();
	}

	bool CCFGameRules::CheckCapsPerRound()
	{
		if (GetGameMode() != CF_GAME_CTF)
			return false;

		if ( !cf_ctf_tugofwar.GetBool() && cf_flag_caps_per_round.GetInt() > 0 )
		{
			int iMaxCaps = -1;
			CCFTeam *pMaxTeam = NULL;

			// check to see if any team has won a "round"
			int nTeamCount = GetNumberOfTeams();
			for ( int iTeam = FIRST_GAME_TEAM; iTeam < nTeamCount; ++iTeam )
			{
				CCFTeam *pTeam = GetGlobalCFTeam( iTeam );
				if ( !pTeam )
					continue;

				// we might have more than one team over the caps limit (if the server op lowered the limit)
				// so loop through to see who has the most among teams over the limit
				if ( pTeam->GetFlagCaptures() >= cf_flag_caps_per_round.GetInt() )
				{
					if ( pTeam->GetFlagCaptures() > iMaxCaps )
					{
						iMaxCaps = pTeam->GetFlagCaptures();
						pMaxTeam = pTeam;
					}
				}
			}

			if ( iMaxCaps != -1 && pMaxTeam != NULL )
			{
				SetWinningTeam( pMaxTeam->GetTeamNumber(), WINREASON_FLAG_CAPTURE_LIMIT );
				return true;
			}
		}

		if ( cf_ctf_tugofwar.GetBool() )
		{
			// Count up how many flags we have real quick.
			int iFlags = 0;
			CInfoObjective* pObjective = NULL;
			while( (pObjective = dynamic_cast<CInfoObjective*>(gEntList.FindEntityByClassname( pObjective, "info_objective*" ))) != NULL )
				iFlags++;

			// If one team has all of the flags, they win!
			for ( int iTeam = FIRST_GAME_TEAM; iTeam < GetNumberOfTeams(); ++iTeam )
			{
				CCFTeam *pTeam = GetGlobalCFTeam( iTeam );
				if ( !pTeam )
					continue;

				if (m_ahFlagWells[iTeam].Count() >= iFlags)
				{
					SetWinningTeam( iTeam, WINREASON_FLAG_CAPTURE_LIMIT );
				}
			}
		}

		return false;
	}

	void CCFGameRules::RoundRespawn( void )
	{
		// Don't bother if there's only one person in the server.
		if (m_bPrevRoundWasWaitingForPlayers && CountActivePlayers() == 1)
		{
			// Avoid an assert.
			if (m_hWaitingForPlayersTimer != NULL && m_hWaitingForPlayersTimer->NetworkProp()->IsMarkedForDeletion())
				m_hWaitingForPlayersTimer = NULL;
			return;
		}

		if (GetGameMode() == CF_GAME_PARIAH)
		{
			RemovePariah();
		}

		BaseClass::RoundRespawn();

		// This recreates every entity and then deletes the ones that we don't want for this gamemode.
		// This might be really slow, so if there are problems with it, we should figure out a way to
		// block recreation of the ones we don't want later.
		LevelInitGameMode();

		// Finding well locations should be done after entities are recreated so that it picks up the proper positions.
		if (GetGameMode() == CF_GAME_CTF)
		{
			// reset the flag captures
			int nTeamCount = GetNumberOfTeams();
			for ( int iTeam = FIRST_GAME_TEAM; iTeam < nTeamCount; ++iTeam )
			{
				CCFTeam *pTeam = GetGlobalCFTeam( iTeam );
				if ( !pTeam )
					continue;

				pTeam->SetFlagCaptures( 0 );
			}

			if (cf_ctf_tugofwar.GetBool())
			{
				EmptyWells();

				m_avecFlagWells.RemoveAll();
				m_avecFlagWells.SetSize(GetNumberOfTeams());

				for ( int iTeam = FIRST_GAME_TEAM; iTeam < nTeamCount; ++iTeam )
				{
					CCFTeam *pTeam = GetGlobalCFTeam( iTeam );
					if ( !pTeam )
						continue;

					CInfoObjective* pObjective = NULL;
					while( (pObjective = dynamic_cast<CInfoObjective*>(gEntList.FindEntityByClassname( pObjective, "info_objective*" ))) != NULL )
					{
						if (pObjective->GetTeamNumber() != iTeam)
							continue;

						m_avecFlagWells[iTeam] = pObjective->GetAbsOrigin();

						CFObjectiveResource()->SetCapturePoint(iTeam, m_avecFlagWells[iTeam]);
						break;
					}
				}
			}
		}

		for (int i = 0; i < gpGlobals->maxClients; i++)
		{
			CBasePlayer* pPlayer = UTIL_PlayerByIndex(i);

			if (!pPlayer)
				continue;

			ToCFPlayer(pPlayer)->StopFatality();
		}
	}

	void CCFGameRules::SetupOnRoundStart( void )
	{
		for ( int i = 0; i < MAX_TEAMS; i++ )
		{
			ObjectiveResource()->SetBaseCP( -1, i );
		}

		for ( int i = 0; i < CF_TEAM_COUNT; i++ )
		{
			m_iNumCaps[i] = 0;
		}

		// Let all entities know that a new round is starting
		CBaseEntity *pEnt = gEntList.FirstEnt();
		while( pEnt )
		{
			variant_t emptyVariant;
			pEnt->AcceptInput( "RoundSpawn", NULL, NULL, emptyVariant, 0 );

			pEnt = gEntList.NextEnt( pEnt );
		}

		// All entities have been spawned, now activate them
		pEnt = gEntList.FirstEnt();
		while( pEnt )
		{
			variant_t emptyVariant;
			pEnt->AcceptInput( "RoundActivate", NULL, NULL, emptyVariant, 0 );

			pEnt = gEntList.NextEnt( pEnt );
		}

		if ( g_pObjectiveResource && !g_pObjectiveResource->PlayingMiniRounds() )
		{
			// Find all the control points with associated spawnpoints
			memset( m_bControlSpawnsPerTeam, 0, sizeof(bool) * MAX_TEAMS * MAX_CONTROL_POINTS );
			CBaseEntity *pSpot = gEntList.FindEntityByClassname( NULL, "info_player_teamspawn" );
			while( pSpot )
			{
				CCFTeamSpawn *pCFSpawn = assert_cast<CCFTeamSpawn*>(pSpot);
				if ( pCFSpawn->GetControlPoint() )
				{
					m_bControlSpawnsPerTeam[ pCFSpawn->GetTeamNumber() ][ pCFSpawn->GetControlPoint()->GetPointIndex() ] = true;
					pCFSpawn->SetDisabled( true );
				}

				pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_teamspawn" );
			}

			RecalculateControlPointState();

			SetRoundOverlayDetails();
		}
	}

	void CCFGameRules::SetupOnRoundRunning( void )
	{
		// Reset player speeds after preround lock
		CCFPlayer *pPlayer;
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			pPlayer = ToCFPlayer( UTIL_PlayerByIndex( i ) );

			if ( !pPlayer )
				continue;

			pPlayer->CalculateMovementSpeed();
		}

		SpeakConceptFromTeamLeaders(MP_CONCEPT_ROUND_START);

		m_bPlayedRoundEndingVOs = false;
	}

	void CCFGameRules::RecalculateControlPointState( void )
	{
		Assert( ObjectiveResource() );

		if ( !g_hControlPointMasters.Count() )
			return;

		if ( g_pObjectiveResource && g_pObjectiveResource->PlayingMiniRounds() )
			return;

		for ( int iTeam = LAST_SHARED_TEAM+1; iTeam < GetNumberOfTeams(); iTeam++ )
		{
			int iFarthestPoint = GetFarthestOwnedControlPoint( iTeam, true );
			if ( iFarthestPoint == -1 )
				continue;

			// Now enable all spawn points for that spawn point
			CBaseEntity *pSpot = gEntList.FindEntityByClassname( NULL, "info_player_teamspawn" );
			while( pSpot )
			{
				CCFTeamSpawn *pCFSpawn = assert_cast<CCFTeamSpawn*>(pSpot);
				if ( pCFSpawn->GetControlPoint() )
				{
					if ( pCFSpawn->GetTeamNumber() == iTeam )
					{
						if ( pCFSpawn->GetControlPoint()->GetPointIndex() == iFarthestPoint )
						{
							pCFSpawn->SetDisabled( false );
						}
						else
						{
							pCFSpawn->SetDisabled( true );
						}
					}
				}

				pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_teamspawn" );
			}
		}
	}

	void CCFGameRules::InternalHandleTeamWin( int iWinningTeam )
	{
		char szModifiers[256];
		Q_strcpy(szModifiers, VarArgs("winningteam:%s", GetGlobalCFTeam(iWinningTeam)->GetName()));
		SpeakConceptFromTeamLeaders(MP_CONCEPT_ROUND_ENDED, szModifiers);

		BaseClass::InternalHandleTeamWin(iWinningTeam);
	}

	void CCFGameRules::SendWinPanelInfo( void )
	{
		for (int i = 1; i < gpGlobals->maxClients; i++)
		{
			if (!UTIL_PlayerByIndex(i))
				continue;

			ToCFPlayer(UTIL_PlayerByIndex(i))->ShowCFPanel(CF_ROUND_OVER, true);
		}
	}

	void CCFGameRules::OnOvertimeStart()
	{
		SpeakConceptFromTeamLeaders(MP_CONCEPT_SUDDENDEATH_START);	// We borrow the sudden death event for overtime, since we don't have sudden death and there is no overtime entry.

		BaseClass::OnOvertimeStart();
	}

	float CCFGameRules::GetCaptureValueForPlayer( CBasePlayer *pPlayer )
	{
		CCFPlayer *pCFPlayer = ToCFPlayer( pPlayer );
		if ( GetGameMode() == CF_GAME_PARIAH)
		{
			if ( !pCFPlayer->IsFuse() )
			{
				// Regular players are slow as ass in Pariah mode.
				return 0.1f;
			}
		}

		return BaseClass::GetCaptureValueForPlayer( pPlayer );
	}

	int CCFGameRules::GetFarthestOwnedControlPoint( int iTeam, bool bWithSpawnpoints )
	{
		int iOwnedEnd = ObjectiveResource()->GetBaseControlPointForTeam( iTeam );
		if ( iOwnedEnd == -1 )
			return -1;

		int iNumControlPoints = ObjectiveResource()->GetNumControlPoints();
		int iWalk = 1;
		int iEnemyEnd = iNumControlPoints-1;
		if ( iOwnedEnd != 0 )
		{
			iWalk = -1;
			iEnemyEnd = 0;
		}

		// Walk towards the other side, and find the farthest owned point that has spawn points
		int iFarthestPoint = iOwnedEnd;
		for ( int iPoint = iOwnedEnd; iPoint != iEnemyEnd; iPoint += iWalk )
		{
			// If we've hit a point we don't own, we're done
			if ( ObjectiveResource()->GetOwningTeam( iPoint ) != iTeam )
				break;

			if ( bWithSpawnpoints && !m_bControlSpawnsPerTeam[iTeam][iPoint] )
				continue;

			iFarthestPoint = iPoint;
		}

		return iFarthestPoint;
	}

	bool CCFGameRules::PlayerMayCapturePoint( CBasePlayer *pPlayer, int iPointIndex, char *pszReason, int iMaxReasonLength )
	{
		if (GetGameMode() == CF_GAME_PARIAH)
		{
			if (ToCFPlayer(pPlayer)->IsPariah())
			{
				if (pszReason)
					Q_strncpy(pszReason, "", iMaxReasonLength);
				return false;
			}
		}

		return BaseClass::PlayerMayCapturePoint( pPlayer, iPointIndex, pszReason, iMaxReasonLength );
	}

	bool CCFGameRules::PlayerMayBlockPoint( CBasePlayer *pPlayer, int iPointIndex, char *pszReason, int iMaxReasonLength )
	{
		if (GetGameMode() == CF_GAME_PARIAH)
		{
			if (m_hFuse != NULL && PlayerRelationship(pPlayer, m_hFuse))
				return true;
		}

		return BaseClass::PlayerMayCapturePoint( pPlayer, iPointIndex, pszReason, iMaxReasonLength );
	}

	void CCFGameRules::AddFlagToWell(CInfoObjective* pFlag)
	{
		if (!cf_ctf_tugofwar.GetBool())
			return;

		int iTeam = pFlag->GetTeamNumber();

		// Only actual teams have wells.
		if (iTeam < FIRST_GAME_TEAM)
			return;

		for (int i = 0; i < m_ahFlagWells[iTeam].Count(); i++)
		{
			if (m_ahFlagWells[iTeam][i] == pFlag)
				return;
		}

		m_ahFlagWells[iTeam].AddToTail(pFlag);
		ResetWell(iTeam);
	}

	void CCFGameRules::RemoveFlagFromWell(CInfoObjective* pFlag)
	{
		if (!cf_ctf_tugofwar.GetBool())
			return;

		int iTeam = pFlag->GetTeamNumber();

		// Only actual teams have wells.
		if (iTeam < FIRST_GAME_TEAM)
			return;

		for (int i = 0; i < m_ahFlagWells[iTeam].Count(); i++)
		{
			if (m_ahFlagWells[iTeam][i] == pFlag)
			{
				m_ahFlagWells[iTeam].Remove(i);
				break;
			}
		}

		ResetWell(iTeam);
	}

	void CCFGameRules::ResetWell(int iTeam)
	{
		if (!cf_ctf_tugofwar.GetBool())
			return;

		// Only actual teams have wells.
		if (iTeam < FIRST_GAME_TEAM)
			return;

		// Round hasn't initialized yet. When it does the flags will respawn and ResetWell will get called again, so no worries!
		if (!m_avecFlagWells.Count())
			return;

		int iFlagsInWell = m_ahFlagWells[iTeam].Count();
		Vector vecWellPosition = m_avecFlagWells[iTeam];

		if (iFlagsInWell == 1)
		{
			CInfoObjective* pFlag = m_ahFlagWells[iTeam][0];
			Assert(pFlag);
			if (pFlag)
			{
				pFlag->SetReturnPosition(vecWellPosition);
				pFlag->Return(false);
			}
		}
		else
		{
			for (int i = 0; i < iFlagsInWell; i++)
			{
				CInfoObjective* pFlag = m_ahFlagWells[iTeam][i];

				Assert(pFlag);
				if (!pFlag)
					continue;

				float flAngle = 360*i/iFlagsInWell;
				QAngle angFlagAngle(0, flAngle, 0);
				Vector vecFlagAngle;
				AngleVectors(angFlagAngle, &vecFlagAngle);

				pFlag->SetReturnPosition(vecWellPosition + vecFlagAngle*30);
				pFlag->Return(false);
			}
		}
	}

	void CCFGameRules::EmptyWells()
	{
		if (!cf_ctf_tugofwar.GetBool())
			return;

		for (int i = 0; i < MAX_TEAMS; i++)
			m_ahFlagWells[i].RemoveAll();
	}

	void CCFGameRules::PlayWinSong( int team )
	{
		switch(team)
		{
		case TEAM_MACHINDO:
			BroadcastSound( "CFMusic.VictoryMachindo" );
			break;
		case TEAM_NUMENI:
			BroadcastSound( "CFMusic.VictoryNumeni" );
			break;
		default:
			break;
		}
	}

	void CCFGameRules::BroadcastSound( const char *sound )
	{
		//send it to everyone
		IGameEvent *event = gameeventmanager->CreateEvent( "cf_broadcast_audio" );
		if ( event )
		{
			event->SetString( "sound", sound );
			gameeventmanager->FireEvent( event );
		}
	}

	void CCFGameRules::SpeakConceptFromTeamLeaders( int iConcept, const char* pszModifiers )
	{
		GetGlobalCFTeam(TEAM_MACHINDO)->SpeakConceptFromLeaders(iConcept, pszModifiers);
		GetGlobalCFTeam(TEAM_NUMENI)->SpeakConceptFromLeaders(iConcept, pszModifiers);
	}

	CBaseEntity *CCFGameRules::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
	{
		// get valid spawn point
		CBaseEntity *pSpawnSpot = pPlayer->EntSelectSpawnPoint();

		// drop down to ground
		Vector GroundPos = DropToGround( pPlayer, pSpawnSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX );

		// Move the player to the place it said.
		pPlayer->SetLocalOrigin( GroundPos + Vector(0,0,1) );
		pPlayer->SetAbsVelocity( vec3_origin );
		pPlayer->SetLocalAngles( pSpawnSpot->GetLocalAngles() );
		pPlayer->m_Local.m_vecPunchAngle = vec3_angle;
		pPlayer->m_Local.m_vecPunchAngleVel = vec3_angle;
		pPlayer->SnapEyeAngles( pSpawnSpot->GetLocalAngles() );

		return pSpawnSpot;
	}

	// checks if the spot is clear of players
	bool CCFGameRules::IsSpawnPointValid( CBaseEntity *pSpot, CBasePlayer *pPlayer  )
	{
		// Players use any spawn point in Pariah mode.
		if (GetGameMode() != CF_GAME_PARIAH)
		{
			if ( pSpot->GetTeamNumber() != TEAM_UNASSIGNED && pSpot->GetTeamNumber() != pPlayer->GetTeamNumber() )
				return false;
		}

		if ( !pSpot->IsTriggered( pPlayer ) )
			return false;

		CCFTeamSpawn *pCFSpawn = dynamic_cast<CCFTeamSpawn*>( pSpot );
		if ( pCFSpawn )
		{
			if ( pCFSpawn->IsDisabled() )
				return false;
		}

		for (int i = 1; i < gpGlobals->maxClients; i++)
		{
			if (!UTIL_PlayerByIndex(i))
				continue;

			CCFPlayer* pCFPlayer = ToCFPlayer(UTIL_PlayerByIndex(i));

			if (PlayerRelationship(pCFPlayer, pPlayer) == GR_TEAMMATE)
				continue;

			if (pCFPlayer->IsVisible(pSpot->WorldSpaceCenter()))
				return false;
		}

		Vector mins = GetViewVectors()->m_vHullMin;
		Vector maxs = GetViewVectors()->m_vHullMax;

		Vector vTestMins = pSpot->GetAbsOrigin() + mins;
		Vector vTestMaxs = pSpot->GetAbsOrigin() + maxs;
		return UTIL_IsSpaceEmpty( pPlayer, vTestMins, vTestMaxs );
	}

	bool CCFGameRules::CanHavePlayerItem( CBasePlayer *pPlayer, CBaseCombatWeapon *pItem )
	{
		CWeaponCFBase* pWeapon = dynamic_cast<CWeaponCFBase*>(pItem);

		Assert(pWeapon);
		if (!pWeapon)
			return BaseClass::CanHavePlayerItem( pPlayer, pItem );

		// Really this is actually CanPickUpPlayerItem(), and unless it's a Pariah blade, the answer is no.
		if (pWeapon->GetWeaponID() == WEAPON_PARIAH)
			return true;
		else
			return false;	// NO MEANS NO!
	}

	bool CCFGameRules::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker, CTakeDamageInfo& info )
	{
		if ( pAttacker && PlayerRelationship( pPlayer, pAttacker ) == GR_TEAMMATE )
		{
			// my teammate hit me.
			if ( (friendlyfire.GetInt() == 0) && (pAttacker != pPlayer) )
			{
				// friendly fire is off, and this hit came from someone other than myself,  then don't get hurt
				return false;
			}
		}

		if (GetGameMode() == CF_GAME_PARIAH)
		{
			if ( ToCFPlayer(pPlayer)->IsBecomingFuse() || ToCFPlayer(pPlayer)->IsBecomingPariah() )
				return false;

			if ( pPlayer && ToCFPlayer(pPlayer)->IsFuse() )
			{
				// Pariah can always hit the Fuse.
				if (pAttacker && ToCFPlayer(pAttacker) && ToCFPlayer(pAttacker)->IsPariah())
					return true;

				// Everybody else can only get his health down to a certain level.
				if (((float)pPlayer->GetHealth()) / ((float)pPlayer->GetMaxHealth()) > cf_fuse_health_save.GetFloat())
				{
					float flMinFuseHealth = cf_fuse_health_save.GetFloat() * (float)pPlayer->GetMaxHealth();

					// Make sure we are not doing more damage than the health save.
					if (pPlayer->GetHealth() - info.GetDamage() < flMinFuseHealth)
						info.SetDamage(pPlayer->GetHealth() - flMinFuseHealth);

					if (info.GetDamage() < 1)
						return false;

					return true;
				}

				return false;
			}
		}

		// Skip teamplay, we don't want its team stuff
		return CMultiplayRules::FPlayerCanTakeDamage( pPlayer, pAttacker );
	}

	bool CCFGameRules::CanReceiveCritical(CCFPlayer* pVictim, CCFPlayer* pAttacker)
	{
		if (CFGameRules()->PlayerRelationship(pVictim, pAttacker) == GR_TEAMMATE)
			return false;

		// Down strikes can never critical, they already do extra damage as it is.
		if (pAttacker->m_bDownStrike)
			return false;

		if (GetGameMode() == CF_GAME_PARIAH)
		{
			if ( pVictim && (pVictim->IsBecomingFuse() || pVictim->IsBecomingPariah()) )
				return false;

			if ( pVictim && ToCFPlayer(pVictim)->IsFuse() )
			{
				// Pariah can always hit the Fuse.
				if (pAttacker && pAttacker->IsPariah())
					return true;

				return false;
			}
		}



		// Skip teamplay, we don't want its team stuff
		return true;
	}

	bool CCFGameRules::CanFreezeVictim(CCFPlayer* pVictim, CCFPlayer* pAttacker)
	{
		if (GetGameMode() == CF_GAME_PARIAH)
		{
			if (pVictim->IsBecomingFuse() || pVictim->IsBecomingPariah())
				return false;
		}

		return CFGameRules()->PlayerRelationship(pAttacker, pVictim) == GR_NOTTEAMMATE;
	}

	int CCFGameRules::IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
	{
		if ( !pKilled )
			return 0;

		if ( !pAttacker )
			return 1;

		if ( pAttacker != pKilled && PlayerRelationship( pAttacker, pKilled ) == GR_TEAMMATE )
			return -1;

		return 1;
	}

	void CCFGameRules::PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &inputInfo )
	{
		CTakeDamageInfo info = inputInfo;

		if (GetGameMode() == CF_GAME_PARIAH)
		{
			if (ToCFPlayer(pVictim)->IsFuse())
			{
				if (info.GetAttacker() && info.GetAttacker()->IsPlayer() && ToCFPlayer(info.GetAttacker())->IsPariah())
					MakeFuse(ToCFPlayer(info.GetAttacker()));
				else
					PickRandomFuse();

				BroadcastSound( "CFMusic.FuseDeath" );
			}

			ToCFPlayer(pVictim)->BecomeNormal();
		}

		// Find the killer & the scorer
		CBaseEntity *pInflictor = info.GetInflictor();
		CBaseEntity *pKiller = info.GetAttacker();
		CCFPlayer *pCFScorer = ToCFPlayer(GetDeathScorer( pKiller, pInflictor, pVictim ));
		CCFPlayer *pCFVictim = ToCFPlayer(pVictim);

		if (info.GetAttacker() && info.GetAttacker()->IsPlayer())
		{
			char szModifiers[256];
			Q_strcpy(szModifiers, VarArgs("victimhadobjective:%d", pCFVictim->HasObjective()));
			ToCFPlayer(info.GetAttacker())->SpeakConceptIfAllowed(MP_CONCEPT_KILLED_PLAYER, szModifiers);
		}

		if (pCFVictim->IsCaptain() || pCFVictim->IsSergeant())
			pCFVictim->GetCFTeam()->DemotePlayer(pCFVictim);

		// If there is no apparent player killer in CDamageInfo, award a kill to the last attacker.
		if ((!pCFScorer || pCFScorer == NULL) && (pCFVictim->GetLastAttacker() != NULL))
		{
			pCFScorer = ToCFPlayer(pCFVictim->GetLastAttacker());
			info.SetAttacker(pCFScorer);
		}

		DeathNotice( pVictim, info );

		if( pCFScorer && pCFScorer->IsPlayer() && pCFScorer != pCFVictim )
			pCFScorer->HintMessage( HINT_ENEMY_KILLED );

		// dvsents2: uncomment when removing all FireTargets
		// variant_t value;
		// g_EventQueue.AddEvent( "game_playerdie", "Use", value, 0, pVictim, pVictim );
		FireTargets( "game_playerdie", pVictim, pVictim, USE_TOGGLE, 0 );

		//if (pCFVictim->m_hLastAttacker != NULL)
			//DevMsg("Yes, there is a lastAttacker.");

		// Only increment death/frag count if there is a valid scorer and this is not a suicide.
		// These death/frag counts are used for overdrive and we don't want people abusing that by
		// killing themselves.
		if ( pCFScorer && pCFScorer != pVictim )  
		{
			pVictim->IncrementDeathCount( 1 );

			// if a player dies in a deathmatch game and the killer is a client, award the killer some points
			pCFScorer->IncrementFragCount( IPointsForKill( pCFScorer, pVictim ) );
			
			// Allow the scorer to immediately paint a decal
			pCFScorer->AllowImmediateDecalPainting();

			// dvsents2: uncomment when removing all FireTargets
			//variant_t value;
			//g_EventQueue.AddEvent( "game_playerkill", "Use", value, 0, pScorer, pScorer );
			FireTargets( "game_playerkill", pCFScorer, pCFScorer, USE_TOGGLE, 0 );

			pCFScorer->AwardEloPoints(SCORE_KO, pCFVictim);
		}

		//find the area the player is in and see if his death causes a block
		CBaseMultiplayerPlayer *pScorer = ToBaseMultiplayerPlayer(pCFScorer);
		CTriggerAreaCapture *pArea;
		while( (pArea = dynamic_cast<CTriggerAreaCapture *>(gEntList.FindEntityByClassname( NULL, "trigger_capture_area" ) )) != NULL )
		{
			CBaseMultiplayerPlayer *pMultiplayerPlayer = ToBaseMultiplayerPlayer(pVictim);

			if ( pArea->IsTouching( pMultiplayerPlayer ) )
			{
				if ( pArea->CheckIfDeathCausesBlock( pMultiplayerPlayer, pScorer ) )
					break;
			}	
		}

		if (pCFVictim->HasObjective())
		{
			if (pCFScorer)
				pCFScorer->AwardEloPoints(SCORE_KILL_ENEMY_FLAG_CARRIER);
			pCFVictim->GetObjective()->Drop();
		}
	}

	void CCFGameRules::DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &inputInfo )
	{
		CTakeDamageInfo info = inputInfo;

		// Work out what killed the player, and send a message to all clients about it
		const char *killer_weapon_name = "The Archon";		// by default, the player is killed by the world
		int killer_ID = 0;

		// Find the killer & the scorer
		CBaseEntity *pInflictor = info.GetInflictor();
		CBaseEntity *pKiller = info.GetAttacker();
		CBasePlayer *pScorer = GetDeathScorer( pKiller, pInflictor, pVictim );
		long iNumen = 0;

		// Custom damage type?
		if ( info.GetDamageCustom() )
		{
			killer_weapon_name = GetDamageCustomString( info );
			if ( pScorer )
			{
				killer_ID = pScorer->GetUserID();
			}
		}
		else
		{
			// Is the killer a client?
			if ( pScorer )
			{
				killer_ID = pScorer->GetUserID();
				
				if ( pInflictor )
				{
					if ( pInflictor != pScorer )
					{
						killer_weapon_name = STRING( pInflictor->m_iClassname );  // it's just that easy
					}
					else
					{
						AssertMsg(false, "Killer's weapon was not set properly.");
					}
				}
			}
			else
			{
				killer_weapon_name = STRING( pInflictor->m_iClassname );
			}

			// strip the NPC_* or weapon_* from the inflictor's classname
			if ( strncmp( killer_weapon_name, "weapon_", 7 ) == 0 )
			{
				killer_weapon_name += 7;
			}
			else if ( strncmp( killer_weapon_name, "NPC_", 8 ) == 0 )
			{
				killer_weapon_name += 8;
			}
			else if ( strncmp( killer_weapon_name, "func_", 5 ) == 0 )
			{
				killer_weapon_name += 5;
			}
		}

		if ( FStrEq( killer_weapon_name, "magic" ) )
			iNumen = info.GetNumen();

		IGameEvent * event = gameeventmanager->CreateEvent( "player_death" );
		if ( event )
		{
			event->SetInt("userid", pVictim->GetUserID() );
			event->SetInt("attacker", killer_ID );
			event->SetString("weapon", killer_weapon_name );
			event->SetInt("priority", 7 );	// HLTV event priority, not transmitted
			event->SetInt("numen", iNumen );

			gameeventmanager->FireEvent( event );
		}

	}

	float CCFGameRules::GetAverageElo(CCFPlayer *pExclude, int iTeam)
	{
		float flAggregator = 0;
		int iPlayers = 0;
		for (int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CCFPlayer *pPlayer = ToCFPlayer(UTIL_PlayerByIndex( i ));

			if (!pPlayer)
				continue;

			if (pExclude && pPlayer == pExclude)
				continue;

			if (iTeam != TEAM_UNASSIGNED && pPlayer->GetTeamNumber() != iTeam)
				continue;

			flAggregator += pPlayer->m_flEloScore;
			iPlayers++;
		}

		if (iPlayers)
			return flAggregator/iPlayers;
		else
			return 1000;
	}

	const char *CCFGameRules::GetDamageCustomString( const CTakeDamageInfo &info )
	{
		if (info.GetDamageCustom() == -1)
			return "suicide";

		return WeaponIDToAlias((CFWeaponID)info.GetDamageCustom());
	}

	void CCFGameRules::Think()
	{
		if ( !g_fGameOver )
		{
			if ( gpGlobals->curtime > m_flNextPeriodicThink )
			{
				if ( State_Get() != GR_STATE_TEAM_WIN && State_Get() != GR_STATE_GAME_OVER && IsInWaitingForPlayers() == false )
				{
					if ( CheckCapsPerRound() )
						return;
				}
			}
		}

		if (GetGameMode() == CF_GAME_PARIAH)
		{
			// Just in case everybody dies at once for some reason!
			if (m_hFuse == NULL)
				PickRandomFuse();

			if (m_hPariahBlade == NULL)
				PickRandomPariah();
		}

#if 0
		for (int i = 0; i < GetNumberOfTeams(); i++)
		{
			CCFTeam* pTeam = dynamic_cast<CCFTeam*>(GetGlobalTeam(i));
			if (!pTeam)
				continue;

			if (pTeam->GetNumPlayers() >= 4 && !pTeam->GetCaptain())
			{
				CCFPlayer* pNewCaptain = pTeam->GetHighestScoringPlayer();
				while (pNewCaptain && !CanBePromoted(pNewCaptain))
					pNewCaptain = pTeam->GetHighestScoringPlayer(pNewCaptain);

				if (pNewCaptain)
					pTeam->PromoteToCaptain(pNewCaptain);
			}

			if (pTeam->GetNumPlayers() >= 3 && !pTeam->GetSergeant())
			{
				CCFPlayer* pNewSergeant = pTeam->GetHighestScoringPlayer();
				while (pNewSergeant && !CanBePromoted(pNewSergeant))
					pNewSergeant = pTeam->GetHighestScoringPlayer(pNewSergeant);

				if (pNewSergeant)
					pTeam->PromoteToSergeant(pNewSergeant);
			}
		}
#endif

		if (!m_bPlayedRoundEndingVOs && GetTimeLeft() < 30 && !IsInWaitingForPlayers() && !InRoundRestart())
		{
			m_bPlayedRoundEndingVOs = true;
			SpeakConceptFromTeamLeaders(MP_CONCEPT_ROUND_ENDING);
		}

		BaseClass::Think();
	}

	// The bots do their processing after physics simulation etc so their visibility checks don't recompute
	// bone positions multiple times a frame.
	void CCFGameRules::EndGameFrame( void )
	{
		TheBots->StartFrame();

		BaseClass::EndGameFrame();
	}

	int CCFGameRules::SelectBestTeam( bool ignoreBots /*= false*/ )
	{
		if ( ignoreBots && ( FStrEq( cv_bot_join_team.GetString(), "n" ) || FStrEq( cv_bot_join_team.GetString(), "m" ) ) )
		{
			ignoreBots = false;	// don't ignore bots when they can't switch teams
		}

		if ( ignoreBots && !mp_autoteambalance.GetBool() )
		{
			ignoreBots = false;	// don't ignore bots when they can't switch teams
		}

		int team = TEAM_UNASSIGNED;
		int numNumeni = GetGlobalCFTeam(TEAM_NUMENI)->GetNumPlayers();
		int numMachindo = GetGlobalCFTeam(TEAM_MACHINDO)->GetNumPlayers();
		if ( ignoreBots )
		{
			numNumeni = UTIL_HumansOnTeam( TEAM_NUMENI );
			numMachindo = UTIL_HumansOnTeam( TEAM_MACHINDO );
		}

		// Choose the team that's lacking players
		if ( numNumeni < numMachindo )
		{
			team = TEAM_NUMENI;
		}
		else if ( numNumeni > numMachindo )
		{
			team = TEAM_MACHINDO;
		}
		// Choose the team that's losing
		else if ( GetGlobalCFTeam(TEAM_NUMENI)->GetScore() < GetGlobalCFTeam(TEAM_MACHINDO)->GetScore() )
		{
			team = TEAM_NUMENI;
		}
		else if ( GetGlobalCFTeam(TEAM_MACHINDO)->GetScore() < GetGlobalCFTeam(TEAM_NUMENI)->GetScore() )
		{
			team = TEAM_MACHINDO;
		}
		else
		{
			// Teams and scores are equal, pick a random team
			if ( random->RandomInt( 0, 1 ) == 0 )
			{
				team = TEAM_NUMENI;
			}
			else
			{
				team = TEAM_MACHINDO;
			}
		}

		return team;
	}

	bool CCFGameRules::CanBePromoted(CCFPlayer *pPlayer)
	{
		if (pPlayer->IsFuse())
			return false;

		if (pPlayer->IsPariah())
			return false;

		if (pPlayer->IsCaptain())
			return false;

		if (!pPlayer->IsAlive())
			return false;

		return true;
	}

	CFGameType CCFGameRules::GameModeFromString(const char *pszString)
	{
		for (int i = 0; i < NUM_GAMEMODES; i++)
			if (Q_strcmp(g_pszGameModes[i], cf_gamemode.GetString()) == 0)
				return (CFGameType)i;

		char* pszEnd;
		int iIndex = strtol(pszString, &pszEnd, 10);

		if (pszString == pszEnd)
		{
			Assert(!"Couldn't find a valid game type.");
			Warning("Couldn't find game type '%s'\n", pszString);
		}

		return (CFGameType)iIndex;	// If strtol returns 0 because of an invalid string, we'll get TDM, which is OK.
	}

	int CCFGameRules::GameModeSpawnFlag(CFGameType eGameMode)
	{
		Assert(eGameMode >= 0 && eGameMode < NUM_GAMEMODES);

		switch (eGameMode)
		{
		case CF_GAME_TDM:
		default:
			return SF_SUPPORTS_TDM;

		case CF_GAME_CTF:
			return SF_SUPPORTS_CTF;

		case CF_GAME_PARIAH:
			return SF_SUPPORTS_PARIAH;
		}
	}

#if defined(ARCON_MOD) || defined(ARCON_DEMO)
	char g_pszPassword[64];

	CON_COMMAND( cf_password, "Password required to run a CF mod-based server." )
	{
		if ( args.ArgC() < 2 )
			Msg("Usage: cf_password \"password\"");
		else
			Q_snprintf(g_pszPassword, sizeof(g_pszPassword), args[1]);
	}

	bool CCFGameRules::ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
	{
		// We don't include the real password here, because it can be searched for in a binary.
		// Instead use a version with the values one smaller, and increment them by one quickly.
		char pszPassword[64] = "g/kxb3k3l0sx";
		for (unsigned int i = 0; i < strlen(pszPassword); i++)
			pszPassword[i]++;

		if (!m_bServerValidated && !FStrEq(g_pszPassword, pszPassword))
		{
			Q_snprintf( reject, maxrejectlen, "Error 42" );
			return false;
		}
		else
			return BaseClass::ClientConnected( pEntity, pszName, pszAddress, reject, maxrejectlen );
	}
#endif

#endif


bool CCFGameRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	if ( collisionGroup0 > collisionGroup1 )
	{
		// swap so that lowest is always first
		swap(collisionGroup0,collisionGroup1);
	}
	
	//Don't stand on COLLISION_GROUP_WEAPON
	if( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT &&
		collisionGroup1 == COLLISION_GROUP_WEAPON )
	{
		return false;
	}
	
	return BaseClass::ShouldCollide( collisionGroup0, collisionGroup1 ); 
}


//-----------------------------------------------------------------------------
// Purpose: Init CS ammo definitions
//-----------------------------------------------------------------------------

// shared ammo definition
// JAY: Trying to make a more physical bullet response
#define BULLET_MASS_GRAINS_TO_LB(grains)	(0.002285*(grains)/16.0f)
#define BULLET_MASS_GRAINS_TO_KG(grains)	lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

// exaggerate all of the forces, but use real numbers to keep them consistent
#define BULLET_IMPULSE_EXAGGERATION			1	

// convert a velocity in ft/sec and a mass in grains to an impulse in kg in/s
#define BULLET_IMPULSE(grains, ftpersec)	((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)


ConVar	sk_plr_dmg_grenade		( "sk_plr_dmg_grenade","0", FCVAR_REPLICATED); //Tony; added so base compiles!
CAmmoDef* GetAmmoDef()
{
	static CAmmoDef def;
	static bool bInitted = false;

	if ( !bInitted )
	{
		bInitted = true;
		
		// def.AddAmmoType( BULLET_PLAYER_50AE,		DMG_BULLET, TRACER_LINE, 0, 0, "ammo_50AE_max",		2400, 0, 10, 14 );
		def.AddAmmoType( AMMO_GRENADE, DMG_BLAST, TRACER_LINE, 0, 0,	INFINITE_AMMO, 1, 0 );
		def.AddAmmoType( AMMO_BULLETS, DMG_BULLET, TRACER_LINE, 0, 0,	INFINITE_AMMO, 1, 0 );
		def.AddAmmoType( AMMO_BUCKSHOT, DMG_BUCKSHOT | DMG_BULLET, TRACER_LINE, 0, 0, INFINITE_AMMO, 1, 0 );
	}

	return &def;
}


#ifndef CLIENT_DLL

const char *CCFGameRules::GetChatPrefix( bool bTeamOnly, CBasePlayer *pPlayer )
{
	return NULL;
}

#endif

//-----------------------------------------------------------------------------
// Purpose: Find the relationship between players (teamplay vs. deathmatch)
//-----------------------------------------------------------------------------
int CCFGameRules::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
	// The Pariah is an outcast, and is never on anybody else's team.
	if (GetGameMode() == CF_GAME_PARIAH)
	{
		if ( pPlayer && ToCFPlayer(pPlayer)->IsPariah() )
			return GR_NOTTEAMMATE;

		if ( pTarget && ToCFPlayer(pTarget) && ToCFPlayer(pTarget)->IsPariah() )
			return GR_NOTTEAMMATE;
	}

	if ( !pPlayer || !pTarget )
		return GR_NOTTEAMMATE;

	// half life multiplay has a simple concept of Player Relationships.
	// you are either on another player's team, or you are not.
	if ( pPlayer->GetTeamNumber() == pTarget->GetTeamNumber() )
		return GR_TEAMMATE;

	return GR_NOTTEAMMATE;
}
