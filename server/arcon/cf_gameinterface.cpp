//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "gameinterface.h"
#include "mapentities.h"
#include "vstdlib/jobthread.h"
#include "cf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// -------------------------------------------------------------------------------------------- //
// Mod-specific CServerGameClients implementation.
// -------------------------------------------------------------------------------------------- //

void CServerGameClients::GetPlayerLimits( int& minplayers, int& maxplayers, int &defaultMaxPlayers ) const
{
	minplayers = 2;  // Force multiplayer.
	maxplayers = MAX_PLAYERS;
	defaultMaxPlayers = 32;
}

// -------------------------------------------------------------------------------------------- //
// Mod-specific CServerGameDLL implementation.
// -------------------------------------------------------------------------------------------- //

void CServerGameDLL::LevelInit_ParseAllEntities( const char *pMapEntities )
{
}

static CUtlVector<const char*> g_aWhitelistedSteamIDs;

CON_COMMAND( cf_whitelist, "Add a STEAM ID to the whitelist." )
{
	if ( args.ArgC() < 2 )
	{
		Msg("Usage: cf_whitelist \"STEAM_0:0:1234\"");
	}
	else
	{
		g_aWhitelistedSteamIDs.AddToTail(strdup(args[1]));
	}
}

int WhiteListCount()
{
	return g_aWhitelistedSteamIDs.Count();
}

bool NetworkIDInWhiteList(const char* pszNetworkID)
{
	// Bots aren't real people!
	if (FStrEq("BOT", pszNetworkID))
		return true;

	for (int i = 0; i < g_aWhitelistedSteamIDs.Count(); i++)
	{
		if (FStrEq(g_aWhitelistedSteamIDs[i], pszNetworkID))
		{
			return true;
		}
	}
	return false;
}

void DisconnectNotInWhiteList(CBasePlayer* pPlayer)
{
	engine->ClientCommand(pPlayer->edict(), "disconnect");
	engine->ServerCommand(VarArgs("kickid %d\n", pPlayer->GetPlayerInfo()->GetUserID()));
}

//-----------------------------------------------------------------------------
// Purpose: A user has had their network id setup and validated 
//-----------------------------------------------------------------------------
void CServerGameClients::NetworkIDValidated( const char *pszUserName, const char *pszNetworkID )
{
	if (g_aWhitelistedSteamIDs.Count())
	{
		for (int i = 0; i < gpGlobals->maxClients; i++)
		{
			CBasePlayer* pPlayer = UTIL_PlayerByIndex( i );

			if (!pPlayer)
				continue;

			if (FStrEq(pPlayer->GetPlayerName(), pszUserName))
			{
				if (!NetworkIDInWhiteList(pszNetworkID))
					DisconnectNotInWhiteList(pPlayer);
				break;
			}
		}
	}
}

#if !defined(__linux__)
#if defined(ARCON_MOD) || defined(ARCON_DEMO)
CJob *g_pValidateServer = NULL;

bool g_bNotifyGameRulesValidated = false;

extern bool CFServerValidation();
static void ProcessValidation( CFunctor **pData, unsigned int nCount )
{
#if !defined(_DEBUG) && !defined(ARCON_VALVE)
	if (CFServerValidation())
#endif
		g_bNotifyGameRulesValidated = true;
}

class CValidationManager : CAutoGameSystem
{
public:

	CValidationManager( char const *name ) : CAutoGameSystem( name )
	{
	}

	virtual bool Init()
	{
		g_pValidateServer = ThreadExecute( &ProcessValidation, (CFunctor**)NULL, 0 );
		return true;
	}

	virtual void LevelInitPostEntity()
	{
		// If the thread is executing, then wait for it to finish
		if ( g_pValidateServer )
		{
			g_pValidateServer->WaitForFinishAndRelease();
			g_pValidateServer = NULL;
		}

		if (g_bNotifyGameRulesValidated)
		{
			CFGameRules()->ServerValidated();
		}
	}
};

CValidationManager g_ValidationManager( "CValidationManager" );
#endif
#endif
