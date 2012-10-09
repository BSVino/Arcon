//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Basic BOT handling.
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "player.h"
#include "cf_player.h"
#include "in_buttons.h"
#include "movehelper_server.h"
#include "gameinterface.h"
#include "team.h"
#include "armament.h"
#include "runes.h"

class CCFBot;
void Bot_Think( CCFBot *pBot );


ConVar bot_forcefireweapon( "bot_forcefireweapon", "", 0, "Force bots with the specified weapon to fire." );
ConVar bot_forceattack2( "bot_forceattack2", "0", 0, "When firing, use attack2." );
ConVar bot_forceattackon( "bot_forceattackon", "0", 0, "When firing, don't tap fire, hold it down." );
ConVar bot_flipout( "bot_flipout", "0", 0, "When on, all bots fire their guns." );
ConVar bot_changeclass( "bot_changeclass", "0", 0, "Force all bots to change to the specified class." );
ConVar bot_mimic( "bot_mimic", "0", 0, "Bot uses usercmd of player by index." );
static ConVar bot_mimic_yaw_offset( "bot_mimic_yaw_offset", "0", 0, "Offsets the bot yaw." );

ConVar bot_sendcmd( "bot_sendcmd", "", 0, "Forces bots to send the specified command." );

ConVar bot_crouch( "bot_crouch", "0", 0, "Bot crouches" );

static int g_CurBotNumber = 1;


// This is our bot class.
class CCFBot : public CCFPlayer
{
public:
	DECLARE_CLASS( CCFBot, CCFPlayer );

	bool			m_bBackwards;

	float			m_flNextTurnTime;
	bool			m_bLastTurnToRight;

	float			m_flNextStrafeTime;
	float			m_flSideMove;

	QAngle			m_ForwardAngle;
	QAngle			m_LastAngles;

	virtual void			SetModel( const char *szModelName );
};

LINK_ENTITY_TO_CLASS( cf_bot, CCFBot );

class CBotManager
{
public:
	static CBasePlayer* ClientPutInServerOverride_Bot( edict_t *pEdict, const char *playername )
	{
		// This tells it which edict to use rather than creating a new one.
		CBasePlayer::s_PlayerEdict = pEdict;

		CCFBot *pPlayer = static_cast<CCFBot *>( CreateEntityByName( "cf_bot" ) );
		if ( pPlayer )
		{
			char trimmedName[MAX_PLAYER_NAME_LENGTH];
			Q_strncpy( trimmedName, playername, sizeof( trimmedName ) );
			pPlayer->PlayerData()->netname = AllocPooledString( trimmedName );
		}

		return pPlayer;
	}
};


//-----------------------------------------------------------------------------
// Purpose: Create a new Bot and put it in the game.
// Output : Pointer to the new Bot, or NULL if there's no free clients.
//-----------------------------------------------------------------------------
CBasePlayer *BotPutInServer( bool bFrozen )
{
	char botname[ 64 ];
	Q_snprintf( botname, sizeof( botname ), "Bot%02i", g_CurBotNumber );

	
	// This trick lets us create a CCFBot for this client instead of the CCFPlayer
	// that we would normally get when ClientPutInServer is called.
	ClientPutInServerOverride( &CBotManager::ClientPutInServerOverride_Bot );
	edict_t *pEdict = engine->CreateFakeClient( botname );
	ClientPutInServerOverride( NULL );

	if (!pEdict)
	{
		Msg( "Failed to create Bot.\n");
		return NULL;
	}

	// Allocate a player entity for the bot, and call spawn
	CCFBot *pPlayer = ((CCFBot*)CBaseEntity::Instance( pEdict ));

	pPlayer->ClearFlags();
	pPlayer->AddFlag( FL_CLIENT | FL_FAKECLIENT );

	if ( bFrozen )
		pPlayer->AddEFlags( EFL_BOT_FROZEN );

	int iLowestTeam = FIRST_GAME_TEAM;
	int iLowestMembers = GetGlobalTeam( iLowestTeam )->m_aPlayers.Count();
	for (int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++)
	{
		if (GetGlobalTeam( i )->m_aPlayers.Count() < iLowestMembers)
		{
			iLowestTeam = i;
			iLowestMembers = GetGlobalTeam( iLowestTeam )->m_aPlayers.Count();
		}
	}
	Msg("Bot joining the %s team.\n", GetGlobalTeam( iLowestTeam )->GetName());
	pPlayer->ChangeTeam( iLowestTeam );
	pPlayer->RemoveAllItems( true );
	pPlayer->Spawn();

	g_CurBotNumber++;

	return pPlayer;
}

// Handler for the "bot" command.
void BotAdd_f()
{
	if (engine->IsDedicatedServer())
	{
		Msg("Bots are not allowed in dedicated servers.\n");
		return;
	}

	BotPutInServer( false );
}
ConCommand cc_Bot( "bot_add", BotAdd_f, "Add a bot." );


//-----------------------------------------------------------------------------
// Purpose: Run through all the Bots in the game and let them think.
//-----------------------------------------------------------------------------
void Bot_RunAll( void )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CCFPlayer *pPlayer = ToCFPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer && (pPlayer->GetFlags() & FL_FAKECLIENT) )
		{
			CCFBot *pBot = dynamic_cast< CCFBot* >( pPlayer );
			if ( pBot )
				Bot_Think( pBot );
		}
	}
}

bool Bot_RunMimicCommand( CUserCmd& cmd )
{
	if ( bot_mimic.GetInt() <= 0 )
		return false;

	if ( bot_mimic.GetInt() > gpGlobals->maxClients )
		return false;

	CBasePlayer *pPlayer = UTIL_PlayerByIndex( bot_mimic.GetInt()  );
	if ( !pPlayer )
		return false;

	if ( !pPlayer->GetLastUserCommand() )
		return false;

	cmd = *pPlayer->GetLastUserCommand();
	cmd.viewangles[YAW] += bot_mimic_yaw_offset.GetFloat();

	if( bot_crouch.GetInt() )
		cmd.buttons |= IN_DUCK;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Simulates a single frame of movement for a player
// Input  : *fakeclient - 
//			*viewangles - 
//			forwardmove - 
//			m_flSideMove - 
//			upmove - 
//			buttons - 
//			impulse - 
//			msec - 
// Output : 	virtual void
//-----------------------------------------------------------------------------
static void RunPlayerMove( CCFPlayer *fakeclient, CUserCmd &cmd, float frametime )
{
	if ( !fakeclient )
		return;

	// Store off the globals.. they're gonna get whacked
	float flOldFrametime = gpGlobals->frametime;
	float flOldCurtime = gpGlobals->curtime;

	float flTimeBase = gpGlobals->curtime + gpGlobals->frametime - frametime;
	fakeclient->SetTimeBase( flTimeBase );

	MoveHelperServer()->SetHost( fakeclient );
	fakeclient->PlayerRunCommand( &cmd, MoveHelperServer() );

	// save off the last good usercmd
	fakeclient->SetLastUserCommand( cmd );

	// Clear out any fixangle that has been set
	fakeclient->pl.fixangle = FIXANGLE_NONE;

	// Restore the globals..
	gpGlobals->frametime = flOldFrametime;
	gpGlobals->curtime = flOldCurtime;
}



void Bot_UpdateStrafing( CCFBot *pBot, CUserCmd &cmd )
{
	if ( gpGlobals->curtime >= pBot->m_flNextStrafeTime )
	{
		pBot->m_flNextStrafeTime = gpGlobals->curtime + 1.0f;

		if ( random->RandomInt( 0, 5 ) == 0 )
		{
			pBot->m_flSideMove = -600.0f + 1200.0f * random->RandomFloat( 0, 2 );
		}
		else
		{
			pBot->m_flSideMove = 0;
		}
		cmd.sidemove = pBot->m_flSideMove;

		if ( random->RandomInt( 0, 20 ) == 0 )
		{
			pBot->m_bBackwards = true;
		}
		else
		{
			pBot->m_bBackwards = false;
		}
	}
}


void Bot_UpdateDirection( CCFBot *pBot )
{
	float angledelta = 15.0;
	QAngle angle;

	int maxtries = (int)360.0/angledelta;

	if ( pBot->m_bLastTurnToRight )
	{
		angledelta = -angledelta;
	}

	angle = pBot->GetLocalAngles();

	trace_t trace;
	Vector vecSrc, vecEnd, forward;
	while ( --maxtries >= 0 )
	{
		AngleVectors( angle, &forward );

		vecSrc = pBot->GetLocalOrigin() + Vector( 0, 0, 36 );

		vecEnd = vecSrc + forward * 10;

		UTIL_TraceHull( vecSrc, vecEnd, VEC_HULL_MIN, VEC_HULL_MAX, 
			MASK_PLAYERSOLID, pBot, COLLISION_GROUP_NONE, &trace );

		if ( trace.fraction == 1.0 )
		{
			if ( gpGlobals->curtime < pBot->m_flNextTurnTime )
			{
				break;
			}
		}

		angle.y += angledelta;

		if ( angle.y > 180 )
			angle.y -= 360;
		else if ( angle.y < -180 )
			angle.y += 360;

		pBot->m_flNextTurnTime = gpGlobals->curtime + 2.0;
		pBot->m_bLastTurnToRight = random->RandomInt( 0, 1 ) == 0 ? true : false;

		pBot->m_ForwardAngle = angle;
		pBot->m_LastAngles = angle;
	}
	
	pBot->SetLocalAngles( angle );
}


void Bot_FlipOut( CCFBot *pBot, CUserCmd &cmd )
{
	if ( bot_flipout.GetInt() > 0 && pBot->IsAlive() )
	{
		if ( bot_forceattackon.GetBool() || (RandomFloat(0.0,1.0) > 0.5) )
		{
			cmd.buttons |= bot_forceattack2.GetBool() ? IN_ATTACK2 : IN_ATTACK;
		}

		if ( bot_flipout.GetInt() >= 2 )
		{
			QAngle angOffset = RandomAngle( -1, 1 );

			pBot->m_LastAngles += angOffset;

			for ( int i = 0 ; i < 2; i++ )
			{
				if ( fabs( pBot->m_LastAngles[ i ] - pBot->m_ForwardAngle[ i ] ) > 15.0f )
				{
					if ( pBot->m_LastAngles[ i ] > pBot->m_ForwardAngle[ i ] )
					{
						pBot->m_LastAngles[ i ] = pBot->m_ForwardAngle[ i ] + 15;
					}
					else
					{
						pBot->m_LastAngles[ i ] = pBot->m_ForwardAngle[ i ] - 15;
					}
				}
			}

			pBot->m_LastAngles[ 2 ] = 0;

			pBot->SetLocalAngles( pBot->m_LastAngles );
		}
	}
}


void Bot_HandleSendCmd( CCFBot *pBot )
{
	if ( strlen( bot_sendcmd.GetString() ) > 0 )
	{
//tony; this needs to be fixed
		//send the cmd from this bot
//		pBot->ClientCommand( bot_sendcmd.GetString() );

		bot_sendcmd.SetValue("");
	}
}


// If bots are being forced to fire a weapon, see if I have it
void Bot_ForceFireWeapon( CCFBot *pBot, CUserCmd &cmd )
{
	if ( bot_forcefireweapon.GetString() )
	{
		CBaseCombatWeapon *pWeapon = pBot->Weapon_OwnsThisType( bot_forcefireweapon.GetString() );
		if ( pWeapon )
		{
			// Switch to it if we don't have it out
			CBaseCombatWeapon *pPrimaryWeapon = pBot->GetPrimaryWeapon();
			CBaseCombatWeapon *pSecondaryWeapon = pBot->GetSecondaryWeapon();

			// Switch?
			if ( pPrimaryWeapon != pWeapon && pSecondaryWeapon != pWeapon )
			{
				pBot->Weapon_Switch( pWeapon );
			}
			else
			{
				// Start firing
				// Some weapons require releases, so randomise firing
				if ( bot_forceattackon.GetBool() || (RandomFloat(0.0,1.0) > 0.5) )
				{
					cmd.buttons |= bot_forceattack2.GetBool() ? IN_ATTACK2 : IN_ATTACK;
				}
			}
		}
	}
}


void Bot_SetForwardMovement( CCFBot *pBot, CUserCmd &cmd )
{
	if ( !pBot->IsEFlagSet(EFL_BOT_FROZEN) )
	{
		cmd.forwardmove = 0;
	}
}

extern void respawn(CBaseEntity *pEdict, bool fCopyCorpse);

void Bot_HandleRespawn( CCFBot *pBot, CUserCmd &cmd )
{
	// Wait for Reinforcement wave
	if ( (pBot->IsKnockedOut() && pBot->m_iHealth < -450) || pBot->m_lifeState >= LIFE_DEAD )
	{
		pBot->m_pArmament->Reset();
		pBot->m_pArmament->BuyWeapon(WEAPON_RIVENBLADE, 0);
		pBot->m_pArmament->BuyArmament(CArmamentData::AliasToArmamentID("Cloak"));
		pBot->m_pArmament->BuyRune(CRuneData::AliasToRuneID("fire"), 0, 0, 0);
		pBot->m_pArmament->BuyRune(CRuneData::AliasToRuneID("bullet"), 0, 0, 1);
		pBot->m_pArmament->BuyRune(CRuneData::AliasToRuneID("element"), 0, 0, 2);
		pBot->m_pArmament->BuyRune(CRuneData::AliasToRuneID("element"), 0, 0, 3);
		pBot->m_pArmament->Bind(0, 0, 0);
//		pBot->m_pArmament->BuyWeapon(WEAPON_PISTOL, 1);
		respawn( pBot, false );
	}
}


//-----------------------------------------------------------------------------
// Run this Bot's AI for one frame.
//-----------------------------------------------------------------------------
void Bot_Think( CCFBot *pBot )
{
	// Make sure we stay being a bot
	pBot->AddFlag( FL_FAKECLIENT );


	CUserCmd cmd;
	Q_memset( &cmd, 0, sizeof( cmd ) );
	
	
	// Finally, override all this stuff if the bot is being forced to mimic a player.
	if ( !Bot_RunMimicCommand( cmd ) )
	{
		cmd.sidemove = pBot->m_flSideMove;

		if ( pBot->IsAlive() && (pBot->GetSolid() == SOLID_BBOX) )
		{
			Bot_SetForwardMovement( pBot, cmd );

			// Handle console settings.
			Bot_ForceFireWeapon( pBot, cmd );
			Bot_HandleSendCmd( pBot );
		}
		else
		{
			Bot_HandleRespawn( pBot, cmd );
		}

		Bot_FlipOut( pBot, cmd );

		// Fix up the m_fEffects flags
		pBot->PostClientMessagesSent();

		
		
		cmd.viewangles = pBot->GetLocalAngles();
		cmd.upmove = 0;
		cmd.impulse = 0;
	}
	else
	{
		Bot_HandleRespawn( pBot, cmd );
	}

	float frametime = gpGlobals->frametime;
	RunPlayerMove( pBot, cmd, frametime );
}

void CCFBot::SetModel( const char *szModelName )
{
	BaseClass::SetModel(szModelName);

	CStudioHdr* pStudioHdr = GetModelPtr();
	if (pStudioHdr)
		m_iDesiredSkin = random->RandomInt(0, pStudioHdr->numskinfamilies()-1);
	else
		m_iDesiredSkin = 0;
}
