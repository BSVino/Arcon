//========= Copyright � 1996-2004, Valve LLC, All rights reserved. ============
//
// Purpose: Simple HUD element
//
//=============================================================================

#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "view.h"

using namespace vgui;

#include <vgui_controls/Panel.h>
#include <vgui_controls/Frame.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>

class CHudGameMessage : public CHudElement, public vgui::Panel
{
private:
	DECLARE_CLASS_SIMPLE( CHudGameMessage, vgui::Panel );

public:

	CHudGameMessage( const char *pElementName ) : CHudElement( pElementName ), vgui::Panel( NULL, "HudGameMessage" ) 
	{
		// Set our parent window
		SetParent( g_pClientMode->GetViewport() );
		
		m_pIcon = NULL;

		// Never hide
		SetHiddenBits( 0 );
	};

	void	Init( void );
	void	VidInit( void );
	void	Paint( void );
	
	// Callback function for the "GameMessage" user message
	void	MsgFunc_GameMessage( bf_read &msg );

private:
	CHudTexture *m_pIcon;		// Icon texture reference
	wchar_t		m_pText[256];	// Unicode text buffer

	float		m_flStartTime;	// When the message was recevied
	float		m_flDuration;	// Duration of the message
};

DECLARE_HUDELEMENT( CHudGameMessage );
DECLARE_HUD_MESSAGE( CHudGameMessage, GameMessage );

void CHudGameMessage::VidInit( void )
{
	// Store off a reference to our icon
	// *** You need to put a defined texture here, otherwise no icon will appear
	m_pIcon = gHUD.GetIcon( "dmg_cold" );
	m_pText[0] = '\0';
 
	// *** Fade all out immediately
	m_flStartTime=gpGlobals->curtime-100.f;
}

void CHudGameMessage::Init( void )
{
	HOOK_HUD_MESSAGE( CHudGameMessage, GameMessage );
}

void CHudGameMessage::MsgFunc_GameMessage( bf_read &msg )
{
	// Read in our string
	char szString[256];
	msg.ReadString( szString, sizeof(szString) );

	// Convert it to localize friendly unicode
	g_pVGuiLocalize->ConvertANSIToUnicode( szString, m_pText, sizeof(m_pText) );

	// Setup our time trackers
	m_flStartTime = gpGlobals->curtime;
	m_flDuration = 5.0f;
}

void CHudGameMessage::Paint( void )
{
	// Find our fade based on our time shown
	float dt = ( m_flStartTime - gpGlobals->curtime );
	float flAlpha = SimpleSplineRemapVal( dt, 0.0f, m_flDuration, 255, 0 );
	flAlpha = clamp( flAlpha, 0.0f, 255.0f );
 
	// *** Find the faded color of the current background, by leaving rgb alone and just manipulate a
	Color c=GetBgColor();
	c[3]=flAlpha/2;
	SetBgColor(c);
 
	// *** Draw our icon only if it's available
	if ( m_pIcon )
		m_pIcon->DrawSelf( 0, 0, 32, 32, Color(255,255,255,flAlpha) );
 
	// Get our scheme and font information
	vgui::HScheme scheme = vgui::scheme()->GetScheme( "ClientScheme" );
	vgui::HFont hFont = vgui::scheme()->GetIScheme(scheme)->GetFont( "Default" );

	// Draw our text
	surface()->DrawSetTextFont( hFont ); // set the font	
	surface()->DrawSetTextColor( 255, 255, 255, flAlpha ); // white
	surface()->DrawSetTextPos( 32, 8 ); // x,y position
	surface()->DrawPrintText( m_pText, wcslen(m_pText) ); // print text
}

