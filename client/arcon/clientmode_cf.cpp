//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//
#include "cbase.h"
#include "hud.h"
#include "clientmode_cf.h"
#include "cdll_client_int.h"
#include "iinput.h"
#include "vgui/isurface.h"
#include "vgui/ipanel.h"
#include <vgui_controls/AnimationController.h>
#include "ivmodemanager.h"
#include "filesystem.h"
#include "vgui/ivgui.h"
#include "hud_chat.h"
#include "view_shared.h"
#include "view.h"
#include "ivrenderview.h"
#include "model_types.h"
#include "iefx.h"
#include "dlight.h"
#include <imapoverview.h>
#include "c_playerresource.h"
#include <keyvalues.h>
#include "text_message.h"
#include "panelmetaclassmgr.h"
#include "hud_macros.h"
#include "engine/IEngineSound.h"
#include "cfui_menu.h"
#include "cfui_gui.h"

ConVar default_fov( "default_fov", "75", FCVAR_CHEAT );

ConVar cl_hud_minmode( "cl_hud_minmode", "0", FCVAR_ARCHIVE|FCVAR_DEVELOPMENTONLY, "Set to 1 to turn on the advanced minimalist HUD mode." );

IClientMode *g_pClientMode = NULL;


// --------------------------------------------------------------------------------- //
// CCFModeManager.
// --------------------------------------------------------------------------------- //

class CCFModeManager : public IVModeManager
{
public:
	virtual void	Init();
	virtual void	SwitchMode( bool commander, bool force ) {}
	virtual void	LevelInit( const char *newmap );
	virtual void	LevelShutdown( void );
	virtual void	ActivateMouse( bool isactive ) {}
};

static CCFModeManager g_ModeManager;
IVModeManager *modemanager = ( IVModeManager * )&g_ModeManager;

// --------------------------------------------------------------------------------- //
// CCFModeManager implementation.
// --------------------------------------------------------------------------------- //

#define SCREEN_FILE		"scripts/vgui_screens.txt"

void __MsgFunc_Respawn(bf_read &msg);
void __MsgFunc_LessonLearned(bf_read &msg);
void __MsgFunc_CinematicEvent(bf_read &msg);

void CCFModeManager::Init()
{
	g_pClientMode = GetClientModeNormal();
	
	PanelMetaClassMgr()->LoadMetaClassDefinitionFile( SCREEN_FILE );

	HOOK_MESSAGE( Respawn );
	HOOK_MESSAGE( LessonLearned );
	HOOK_MESSAGE( CinematicEvent );
}

void CCFModeManager::LevelInit( const char *newmap )
{
	g_pClientMode->LevelInit( newmap );

	CCFMOTD::Open();
}

void CCFModeManager::LevelShutdown( void )
{
	g_pClientMode->LevelShutdown();

	cfgui::CRootPanel::GetRoot()->LevelShutdown();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ClientModeCFNormal::ClientModeCFNormal()
{
}

//-----------------------------------------------------------------------------
// Purpose: If you don't know what a destructor is by now, you are probably going to get fired
//-----------------------------------------------------------------------------
ClientModeCFNormal::~ClientModeCFNormal()
{
}

void ClientModeCFNormal::Init()
{
	BaseClass::Init();

	ListenForGameEvent( "cf_broadcast_audio" );
}

void ClientModeCFNormal::InitViewport()
{
	m_pViewport = new CFViewport();
	m_pViewport->Start( gameuifuncs, gameeventmanager );
}

ClientModeCFNormal g_ClientModeNormal;

IClientMode *GetClientModeNormal()
{
	return &g_ClientModeNormal;
}


ClientModeCFNormal* GetClientModeCFNormal()
{
	Assert( dynamic_cast< ClientModeCFNormal* >( GetClientModeNormal() ) );

	return static_cast< ClientModeCFNormal* >( GetClientModeNormal() );
}

float ClientModeCFNormal::GetViewModelFOV( void )
{
	return 74.0f;
}

int ClientModeCFNormal::GetDeathMessageStartHeight( void )
{
	return m_pViewport->GetDeathMessageStartHeight();
}

void ClientModeCFNormal::PostRenderVGui()
{
	if (cfgui::CRootPanel::GetRoot())
		cfgui::CRootPanel::GetRoot()->PostRenderVGui();
}

void __MsgFunc_Respawn(bf_read &msg)
{
	if (C_BasePlayer::GetLocalPlayer())
		C_BasePlayer::GetLocalPlayer()->Spawn();
}

void ClientModeCFNormal::FireGameEvent( IGameEvent * event)
{
	const char *eventname = event->GetName();

	if ( !eventname || !eventname[0] )
		return;

	if ( Q_strcmp( "cf_broadcast_audio", eventname ) == 0 )
	{
		CLocalPlayerFilter filter;
		const char *pszSoundName = event->GetString("sound");

		C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, pszSoundName );
	}
	else
		BaseClass::FireGameEvent( event );
}

//-----------------------------------------------------------------------------
// Purpose: See if spectator input occurred. Return 0 if the key is swallowed.
//-----------------------------------------------------------------------------
int ClientModeCFNormal::HandleSpectatorKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if ( down && pszCurrentBinding && Q_strcmp( pszCurrentBinding, "+forward" ) == 0 )
	{
		if( pPlayer && pPlayer->GetObserverMode() != OBS_MODE_ROAMING )
		{
			engine->ClientCmd( "spec_next" );
			return 0;
		}
	}
	else if ( down && pszCurrentBinding && Q_strcmp( pszCurrentBinding, "+back" ) == 0 )
	{
		if( pPlayer && pPlayer->GetObserverMode() != OBS_MODE_ROAMING )
		{
			engine->ClientCmd( "spec_prev" );
			return 0;
		}
	}
	else if ( down && pszCurrentBinding && Q_strcmp( pszCurrentBinding, "+jump" ) == 0 )
	{
		engine->ClientCmd( "spec_mode" );
		return 0;
	}
	// These keys control spawning, don't let spectator mode interfere with them!
	else if ( down && pszCurrentBinding && Q_strcmp( pszCurrentBinding, "+attack" ) == 0 )
		return 1;
	else if ( down && pszCurrentBinding && Q_strcmp( pszCurrentBinding, "+attack2" ) == 0 )
		return 1;

	return BaseClass::HandleSpectatorKeyInput(down, keynum, pszCurrentBinding);
}
