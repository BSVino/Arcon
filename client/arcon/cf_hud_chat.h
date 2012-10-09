//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CF_HUD_CHAT_H
#define CF_HUD_CHAT_H
#ifdef _WIN32
#pragma once
#endif

#include <hud_basechat.h>

//--------------------------------------------------------------------------------------------------------
/**
* Simple utility function to allocate memory and duplicate a wide string
*/
#ifdef _WIN32
inline wchar_t *CloneWString( const wchar_t *str )
{
	wchar_t *cloneStr = new wchar_t [ wcslen(str)+1 ];
	wcscpy( cloneStr, str );
	return cloneStr;
}
#endif

class CHudChatLine : public CBaseHudChatLine
{
	DECLARE_CLASS_SIMPLE( CHudChatLine, CBaseHudChatLine );

public:
	CHudChatLine( vgui::Panel *parent, const char *panelName );
	~CHudChatLine();

	virtual void	ApplySchemeSettings(vgui::IScheme *pScheme);

	void			PerformFadeout( void );

	void			MsgFunc_SayText(bf_read &msg);

	void			SetNameStart( int iStart ) { m_iNameStart = iStart;	}

	void InsertAndColorizeText( wchar_t *buf, int clientIndex );			///< Parses the color markup and calls Colorize()

private:
	CHudChatLine( const CHudChatLine & ); // not defined, not accessible

	void Colorize( int alpha = 255 );										///< Re-inserts the text in the appropriate colors at the given alpha

	CUtlVector< TextRange > m_textRanges;
	wchar_t					*m_text;

	int				m_iNameStart;
};

//-----------------------------------------------------------------------------
// Purpose: The prompt and text entry area for chat messages
//-----------------------------------------------------------------------------
class CHudChatInputLine : public CBaseHudChatInputLine
{
	DECLARE_CLASS_SIMPLE( CHudChatInputLine, CBaseHudChatInputLine );
	
public:
	CHudChatInputLine( CBaseHudChat *parent, char const *panelName ) : CBaseHudChatInputLine( parent, panelName ) {}

	virtual void	ApplySchemeSettings(vgui::IScheme *pScheme);
};

class CHudChat : public CBaseHudChat
{
	DECLARE_CLASS_SIMPLE( CHudChat, CBaseHudChat );

public:
	CHudChat( const char *pElementName );

	virtual void	CreateChatInputLine( void );
	virtual void	CreateChatLines( void );

	virtual void	Init( void );
	virtual void	Reset( void );
	virtual void	ApplySchemeSettings(vgui::IScheme *pScheme);

	int				GetChatInputOffset( void );
	virtual Color	GetClientColor( int clientIndex );
	virtual Color	GetTextColorForClient( TextColor colorNum, int clientIndex );

};

#endif	//CF_HUD_CHAT_H