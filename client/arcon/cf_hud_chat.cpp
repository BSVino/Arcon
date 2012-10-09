//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "cf_hud_chat.h"
//#include "c_cf_playerresource.h"
#include "hud_macros.h"
#include "text_message.h"
#include "vguicenterprint.h"
#include "vgui/ILocalize.h"
#include "c_team.h"
#include "c_playerresource.h"
#include "c_cf_player.h"
#include "cf_gamerules.h"

DECLARE_HUDELEMENT( CHudChat );

DECLARE_HUD_MESSAGE( CHudChat, SayText );
DECLARE_HUD_MESSAGE( CHudChat, SayText2 );
DECLARE_HUD_MESSAGE( CHudChat, TextMsg );

//=====================
//CHudChatLine
//=====================

CHudChatLine::CHudChatLine( vgui::Panel *parent, const char *panelName ) : CBaseHudChatLine( parent, panelName )
{
	m_text = NULL;
}

CHudChatLine::~CHudChatLine()
{
	if ( m_text )
	{
		delete[] m_text;
		m_text = NULL;
	}
}

void CHudChatLine::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: Parses a line of text for color markup and inserts it via Colorize()
//-----------------------------------------------------------------------------
void CHudChatLine::InsertAndColorizeText( wchar_t *buf, int clientIndex )
{
	if ( m_text )
	{
		delete[] m_text;
		m_text = NULL;
	}
	m_textRanges.RemoveAll();

	m_text = CloneWString( buf );
	CBaseHudChat *pChat = dynamic_cast<CBaseHudChat*>(GetParent() );

	if ( pChat == NULL )
		return;

	wchar_t *txt = m_text;
	int lineLen = wcslen( m_text );
	if ( m_text[0] == COLOR_PLAYERNAME || m_text[0] == COLOR_LOCATION || m_text[0] == COLOR_NORMAL )
	{
		while ( txt && *txt )
		{
			TextRange range;

			switch ( *txt )
			{
			case COLOR_PLAYERNAME:
			case COLOR_LOCATION:
			case COLOR_NORMAL:
				{
					// save ths start
					range.start = (txt-m_text) + 1;
					range.color = pChat->GetTextColorForClient( (TextColor)(*txt), clientIndex );
					range.end = lineLen;

					int count = m_textRanges.Count();
					if ( count )
					{
						m_textRanges[count-1].end = range.start - 1;
					}

					m_textRanges.AddToTail( range );
				}
				++txt;
				break;

			default:
				++txt;
			}
		}
	}

	if ( !m_textRanges.Count() && m_iNameLength > 0 && m_text[0] == COLOR_USEOLDCOLORS )
	{
		TextRange range;
		range.start = 0;
		range.end = m_iNameStart;
		range.color = pChat->GetTextColorForClient( COLOR_NORMAL, clientIndex );
		m_textRanges.AddToTail( range );

		range.start = m_iNameStart;
		range.end = m_iNameStart + m_iNameLength;
		range.color = pChat->GetTextColorForClient( COLOR_PLAYERNAME, clientIndex );
		m_textRanges.AddToTail( range );

		range.start = range.end;
		range.end = wcslen( m_text );
		range.color = pChat->GetTextColorForClient( COLOR_NORMAL, clientIndex );
		m_textRanges.AddToTail( range );
	}

	if ( !m_textRanges.Count() )
	{
		TextRange range;
		range.start = 0;
		range.end = wcslen( m_text );
		range.color = pChat->GetTextColorForClient( COLOR_NORMAL, clientIndex );
		m_textRanges.AddToTail( range );
	}

	for ( int i=0; i<m_textRanges.Count(); ++i )
	{
		wchar_t * start = m_text + m_textRanges[i].start;
		if ( *start > 0 && *start < COLOR_MAX )
		{
			m_textRanges[i].start += 1;
		}
	}

	Colorize();
}

//-----------------------------------------------------------------------------
// Purpose: Inserts colored text into the RichText control at the given alpha
//-----------------------------------------------------------------------------
void CHudChatLine::Colorize( int alpha )
{
	// clear out text
	SetText( "" );

	wchar_t wText[4096];
	Color color;
	for ( int i=0; i<m_textRanges.Count(); ++i )
	{
		wchar_t * start = m_text + m_textRanges[i].start;
		int len = m_textRanges[i].end - m_textRanges[i].start + 1;
		if ( len > 1 )
		{
			wcsncpy( wText, start, len );
			wText[len-1] = 0;
			color = m_textRanges[i].color;
			color[3] = alpha;
			InsertColorChange( color );
			InsertString( wText );
		}
	}

	InvalidateLayout( true );
}

void CHudChatLine::PerformFadeout( void )
{
	// Flash + Extra bright when new
	float curtime = gpGlobals->curtime;

	//CSPort chat only fades out, no blinking.
	if ( curtime <= m_flExpireTime && curtime > m_flExpireTime - CHATLINE_FADE_TIME )
	{
		float frac = ( m_flExpireTime - curtime ) / CHATLINE_FADE_TIME;

		int alpha = frac * 255;
		alpha = clamp( alpha, 0, 255 );

		Colorize( alpha );
	}
	
	OnThink();
}



//=====================
//CHudChatInputLine
//=====================

void CHudChatInputLine::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
}

//=====================
//CHudChat
//=====================

CHudChat::CHudChat( const char *pElementName ) : BaseClass( pElementName )
{
	
}

void CHudChat::CreateChatInputLine( void )
{
	m_pChatInput = new CHudChatInputLine( this, "ChatInputLine" );
	m_pChatInput->SetVisible( false );
}

void CHudChat::CreateChatLines( void )
{
	m_ChatLine = new CHudChatLine( this, "ChatLine1" );
	m_ChatLine->SetVisible( false );	
}

void CHudChat::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
}


void CHudChat::Init( void )
{
	BaseClass::Init();

	HOOK_HUD_MESSAGE( CHudChat, SayText );
	HOOK_HUD_MESSAGE( CHudChat, SayText2 );
	HOOK_HUD_MESSAGE( CHudChat, TextMsg );
}

//-----------------------------------------------------------------------------
// Purpose: Overrides base reset to not cancel chat at round restart
//-----------------------------------------------------------------------------
void CHudChat::Reset( void )
{
}

int CHudChat::GetChatInputOffset( void )
{
	if ( m_pChatInput->IsVisible() )
	{
		return m_iFontHeight;
	}
	else
		return 0;
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Color CHudChat::GetTextColorForClient( TextColor colorNum, int clientIndex )
{
	Color c;
	switch ( colorNum )
	{
	case COLOR_PLAYERNAME:
		c = GetClientColor( clientIndex );
		break;

	case COLOR_LOCATION:
		c = g_ColorDarkGreen;
		break;

	default:
		c = g_ColorYellow;
	}

	return Color( c[0], c[1], c[2], 255 );
}


Color CHudChat::GetClientColor( int clientIndex )
{
	if ( clientIndex == 0 ) // console msg
	{
		return g_ColorGreen;
	}
	else if( g_PR )
	{
		return g_ColorGrey;
	}

	return g_ColorYellow;
}
