//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
/*

===== tf_client.cpp ========================================================

  HL2 client/server game specific stuff
*/

#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "entitylist.h"
#include "physics.h"
#include "game.h"
#include "ai_network.h"
#include "ai_node.h"
#include "ai_hull.h"
#include "shake.h"
#include "player_resource.h"
#include "engine/IEngineSound.h"
#include "cf_player.h"
#include "cf_gamerules.h"
#include "tier0/vprof.h"
#include "cf_bot_temp.h"
#include "cfgui_shared.h"
#include "team.h"
#include "runes.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


extern CBaseEntity *FindPickerEntity( CBasePlayer *pPlayer );

extern bool			g_fGameOver;

extern ConVar	mp_respawntimer;

void FinishClientPutInServer( CCFPlayer *pPlayer )
{
	pPlayer->InitialSpawn();
	pPlayer->Spawn();

	char sName[128];
	Q_strncpy( sName, pPlayer->GetPlayerName(), sizeof( sName ) );
	
	// First parse the name and remove any %'s
	for ( char *pApersand = sName; pApersand != NULL && *pApersand != 0; pApersand++ )
	{
		// Replace it with a space
		if ( *pApersand == '%' )
				*pApersand = ' ';
	}

	// notify other clients of player joining the game
	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Game_connected", sName[0] != 0 ? sName : "<unconnected>" );
}

extern int WhiteListCount();
extern bool NetworkIDInWhiteList(const char* pszNetworkID);
extern void DisconnectNotInWhiteList(CBasePlayer* pPlayer);

/*
===========
ClientPutInServer

called each time a player is spawned into the game
============
*/
void ClientPutInServer( edict_t *pEdict, const char *playername )
{
	// Allocate a CBaseTFPlayer for pev, and call spawn
	CCFPlayer *pPlayer = CCFPlayer::CreatePlayer( "player", pEdict );
	pPlayer->SetPlayerName( playername );

	if (WhiteListCount())
	{
		if (!NetworkIDInWhiteList(pPlayer->GetNetworkIDString()))
			DisconnectNotInWhiteList(pPlayer);
	}
}


void ClientActive( edict_t *pEdict, bool bLoadGame )
{
	// Can't load games in CS!
	Assert( !bLoadGame );

	CCFPlayer *pPlayer = ToCFPlayer( CBaseEntity::Instance( pEdict ) );
	FinishClientPutInServer( pPlayer );
}


/*
===============
const char *GetGameDescription()

Returns the descriptive name of this .dll.  E.g., Half-Life, or Team Fortress 2
===============
*/
const char *GetGameDescription()
{
	if ( g_pGameRules ) // this function may be called before the world has spawned, and the game rules initialized
		return g_pGameRules->GetGameDescription();
	else
		return "Arcon";
}


//-----------------------------------------------------------------------------
// Purpose: Precache game-specific models & sounds
//-----------------------------------------------------------------------------
void ClientGamePrecache( void )
{
	CBaseEntity::PrecacheScriptSound( "Player.Powerjump" );
	CBaseEntity::PrecacheScriptSound( "Player.PowerjumpFailed" );
	CBaseEntity::PrecacheScriptSound( "Player.Latch" );
	CBaseEntity::PrecacheScriptSound( "Player.ReviveBreath" );
	CBaseEntity::PrecacheScriptSound( "Player.Fatality" );
	CBaseEntity::PrecacheScriptSound( "Player.Overdrive" );
	CBaseEntity::PrecacheScriptSound( "Player.BecomeFuse" );
	CBaseEntity::PrecacheScriptSound( "Player.BecomePariah" );
	CBaseEntity::PrecacheScriptSound( "Player.ChargeYell" );
	CBaseEntity::PrecacheScriptSound( "Player.LowStamina" );
	CBaseEntity::PrecacheScriptSound( "HUD.DirectTargetAcquired" );
	CBaseEntity::PrecacheScriptSound( "HUD.RecursedTargetAcquired" );

	CBaseEntity::PrecacheScriptSound( "CFMusic.Menu" );
	CBaseEntity::PrecacheScriptSound( "CFMusic.Pariah" );
	CBaseEntity::PrecacheScriptSound( "CFMusic.VictoryMachindo" );
	CBaseEntity::PrecacheScriptSound( "CFMusic.VictoryNumeni" );
	CBaseEntity::PrecacheScriptSound( "CFMusic.FuseDeath" );
	CBaseEntity::PrecacheScriptSound( "CFMusic.Promoted" );

	int i;
	for (i = 0; i < TOTAL_STATUSEFFECTS; i++)
	{
		if (CRuneData::IsStatusEffectPossible((statuseffect_t)(1<<i)))
		{
			CBaseEntity::PrecacheScriptSound( VarArgs("Player.Status_%s", StatusEffectToString((statuseffect_t)(1<<i))) );
		}
	}

	// Materials used by the client effects
	CBaseEntity::PrecacheModel( "sprites/white.vmt" );
	CBaseEntity::PrecacheModel( "sprites/physbeam.vmt" );
}


// called by ClientKill and DeadThink
void respawn( CBaseEntity *pEdict, bool fCopyCorpse )
{
	if (pEdict->m_lifeState < LIFE_DYING)
		return;

	CCFPlayer *pPlayer = ToCFPlayer(pEdict);

	if (pPlayer->m_hReviver != NULL)
		return;

	if (gpGlobals->curtime < (pPlayer->GetDeathTime() + mp_respawntimer.GetFloat()))
		return;

	if (gpGlobals->curtime < pPlayer->m_flNextRespawn)
		return;

	if (WhiteListCount())
	{
		if (!NetworkIDInWhiteList(pPlayer->GetNetworkIDString()))
		{
			DisconnectNotInWhiteList(pPlayer);
			return;
		}
	}

	if (gpGlobals->coop || gpGlobals->deathmatch)
	{
		if ( fCopyCorpse )
		{
			// make a copy of the dead body for appearances sake
			dynamic_cast< CBasePlayer* >( pEdict )->CreateCorpse();
		}

		// respawn player
		pEdict->Spawn();
	}
	else
	{       // restart the entire server
		engine->ServerCommand("reload\n");
	}
}

void GameStartFrame( void )
{
	VPROF( "GameStartFrame" );

	if ( g_pGameRules )
		g_pGameRules->Think();

	if ( g_fGameOver )
		return;

	gpGlobals->teamplay = teamplay.GetInt() ? true : false;
}

//=========================================================
// instantiate the proper game rules object
//=========================================================
void InstallGameRules()
{
	CreateGameRulesObject( "CCFGameRules" );
}

void CC_Lastinv(const CCommand &args)
{
	CCFPlayer *pPlayer = ToCFPlayer(UTIL_GetCommandClient());
	if (!pPlayer)
		return;

	pPlayer->Weapon_SwitchSecondaries();
}

static ConCommand lastinv("lastinv", CC_Lastinv, "Switches weapons.");

void CC_Attackmode_f(const CCommand &args)
{
	CCFPlayer *pPlayer = ToCFPlayer(UTIL_GetCommandClient());
	if (!pPlayer)
		return;

	bool bPhysical = pPlayer->IsPhysicalMode();

	if (args.ArgC() > 1)
		bPhysical = !!atoi( args[1] );
	else
		bPhysical = !bPhysical;

	pPlayer->SetPhysicalMode(bPhysical);
}

static ConCommand attackmode("attackmode", CC_Attackmode_f, "Switches between physical and magical attack modes." );

void CC_Target_f(const CCommand &args)
{
	CCFPlayer *pPlayer = ToCFPlayer(UTIL_GetCommandClient());
	if (!pPlayer)
		return;

	if (args.ArgC() > 1)
	{
		CCFPlayer* pTargetPlayer = ToCFPlayer(UTIL_PlayerByIndex(atoi(args[1])));

		if (!pTargetPlayer)
			return;

		// Only allow players to target team members with this command.
		if (CFGameRules()->PlayerRelationship(pPlayer, pTargetPlayer) == GR_NOTTEAMMATE)
			return;

		pPlayer->SetDirectTarget(pTargetPlayer);
	}
	else
		pPlayer->FindTarget();
}

static ConCommand cc_target("target", CC_Target_f, "Target a player." );

#ifdef _DEBUG
void CC_ForceTarget_f(const CCommand &args)
{
	if (args.ArgC() > 2)
	{
		CCFPlayer* pPlayer = ToCFPlayer(UTIL_PlayerByIndex(atoi(args[1])));
		CCFPlayer* pTarget = ToCFPlayer(UTIL_PlayerByIndex(atoi(args[2])));

		if (!pPlayer)
			return;

		pPlayer->SetDirectTarget(pTarget);
	}
}

static ConCommand cc_forcetarget("forcetarget", CC_ForceTarget_f, "Force one player to target another player." );
#endif

void CC_Chooseteam_f(const CCommand &args)
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() ); 
	if ( !pPlayer )
		return;

	ToCFPlayer(pPlayer)->ShowCFPanel( CF_TEAM_PANEL, true, true );
}

static ConCommand chooseteam("chooseteam", CC_Chooseteam_f, "Choose a team to play on." );

void CC_Jointeam_f(const CCommand &args)
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() ); 
	if ( !pPlayer )
		return;

	if (args.ArgC() < 2)
	{
		DevMsg("Usage: jointeam #\n 0 for random, 5 for spectator\n");
		return;
	}

	for (unsigned int ui = 0; ui < strlen(args[1]); ui++)
	{
		char cChar = args[1][ui];
		if (cChar < '0' || cChar > '9')
		{
			DevMsg("Usage: jointeam #\n 0 for random, 5 for spectator\n");
			return;
		}
	}

	int iTeam = atoi(args[1]);

	if (iTeam < TEAM_UNASSIGNED || iTeam >= CF_TEAM_COUNT)
		return;

	if (iTeam == TEAM_UNASSIGNED)
	{
		iTeam = FIRST_GAME_TEAM;
		int iLowestMembers = GetGlobalTeam( iTeam )->m_aPlayers.Count();
		for (int i = FIRST_GAME_TEAM; i < CF_TEAM_COUNT; i++)
		{
			if (GetGlobalTeam( i )->m_aPlayers.Count() < iLowestMembers)
			{
				iTeam = i;
				iLowestMembers = GetGlobalTeam( iTeam )->m_aPlayers.Count();
			}
		}
	}

	// Stop wasting my time, dammit!
	if (pPlayer->GetTeamNumber() == iTeam)
		return;

	pPlayer->CommitSuicide();
	pPlayer->ChangeTeam(iTeam);
}
static ConCommand jointeam("jointeam", CC_Jointeam_f, "Switch to the specified team." );

void CC_ChooseColor_f(const CCommand &args)
{
	CCFPlayer *pPlayer = ToCFPlayer( UTIL_GetCommandClient() ); 
	if ( !pPlayer )
		return;

	int iSkin = atoi(args[1]);
	if (iSkin >= 0 && iSkin < 4)
		pPlayer->m_iDesiredSkin = iSkin;
	else
		pPlayer->m_iDesiredSkin = 0;
}

static ConCommand choosecolor("choosecolor", CC_ChooseColor_f, "Choose a color player model." );

void CC_NextTarget_f(const CCommand &args)
{
	CCFPlayer *pPlayer = ToCFPlayer( UTIL_GetCommandClient() ); 
	if ( !pPlayer )
		return;

	CCFPlayer* pOriginal = pPlayer->GetDirectTarget();

	// This is a manual target change. We don't care about that guy anymore.
	pPlayer->m_hFollowModeTarget = NULL;

	pPlayer->FindTarget(false, true, false);

	if (pPlayer->GetDirectTarget() != pOriginal)
		pPlayer->Instructor_LessonLearned(HINT_RMB_TARGET, true);
}

static ConCommand targetnext("targetnext", CC_NextTarget_f, "Target the next player in your view." );

void CC_PrevTarget_f(const CCommand &args)
{
	CCFPlayer *pPlayer = ToCFPlayer( UTIL_GetCommandClient() ); 
	if ( !pPlayer )
		return;

	CCFPlayer* pOriginal = pPlayer->GetDirectTarget();

	// This is a manual target change. We don't care about that guy anymore.
	pPlayer->m_hFollowModeTarget = NULL;

	pPlayer->FindTarget(false, true, true);

	if (pPlayer->GetDirectTarget() != pOriginal)
		pPlayer->Instructor_LessonLearned(HINT_RMB_TARGET, true);
}

static ConCommand targetprev("targetprev", CC_PrevTarget_f, "Target the previous player in your view." );
