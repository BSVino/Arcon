//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Draws the death notices
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_playerresource.h"
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <keyvalues.h>
#include "hud_death.h"
#include "runecombo.h"
#include "c_cf_player.h"
#include "cfui_menu.h"
#include "hud_objectives.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CObituaries::CObituaries()
{
	gameeventmanager->AddListener(this, "player_death", false);
	memset( m_aDeathNoticeList, 0, sizeof(m_aDeathNoticeList) );
}

void CObituaries::LoadTextures()
{
	m_pSkullIcon = GetHudTexture("skull");
}

static int DEATHNOTICE_DISPLAY_TIME = 6;

#define DEATHNOTICE_TOP		YRES( 140 )

static ConVar hud_deathnotice_time( "hud_deathnotice_time", "6", 0 );

void CObituaries::Paint(int x, int y, int w, int h)
{
	vgui::surface()->DrawSetTextFont(CRootPanel::s_hDefaultFont);
	vgui::surface()->DrawSetTextColor(Color(255, 255, 255, 255));

	int lw = 0, lh = 0;
	vgui::surface()->GetTextSize(CRootPanel::s_hDefaultFont, L"!", lw, lh);

	float flXScale = CFScreenWidthScale();
	float flYScale = CFScreenHeightScale();

	for ( int i = 0; i < MAX_DEATHNOTICES; i++ )
	{
		// we've gone through them all
		if ( m_aDeathNoticeList[i].pIconDeath == NULL )
			break;  

		// display time has expired
		// remove the current item from the list
		if ( m_aDeathNoticeList[i].flDisplayTime < gpGlobals->curtime )
		{ 
			Q_memmove( &m_aDeathNoticeList[i], &m_aDeathNoticeList[i+1], sizeof(DeathNoticeItem) * (MAX_DEATHNOTICES - i) );
			// continue on the next item;  stop the counter getting incremented
			i--;  
			continue;
		}

		m_aDeathNoticeList[i].flDisplayTime = min( m_aDeathNoticeList[i].flDisplayTime, gpGlobals->curtime + DEATHNOTICE_DISPLAY_TIME );

		// Draw the death notice
		y = DEATHNOTICE_TOP + (20 * i) + 100;  //!!!

		CCFHudTexture *pIcon = m_aDeathNoticeList[i].pIconDeath;
		long iNumen = m_aDeathNoticeList[i].iNumen;
		if ( !pIcon && !iNumen )
			continue;

		wchar_t victim[ 256 ];
		wchar_t killer[ 256 ];

		g_pVGuiLocalize->ConvertANSIToUnicode( m_aDeathNoticeList[i].szVictim, victim, sizeof( victim ) );
		g_pVGuiLocalize->ConvertANSIToUnicode( m_aDeathNoticeList[i].szKiller, killer, sizeof( killer ) );

		int len = UTIL_ComputeStringWidth( CRootPanel::s_hDefaultFont, victim );
		int iWeaponWidth = CFScreenWidth()/20;

		// x is already converted to screen resolution.
		x = (CFScreenWidth() - iWeaponWidth - 5) * flXScale - len;

		if ( !m_aDeathNoticeList[i].iSuicide )
		{
			int lenkiller = UTIL_ComputeStringWidth( CRootPanel::s_hDefaultFont, killer );

			x -= (5 + lenkiller );

			// Draw killer's name
			vgui::surface()->DrawSetTextPos( x, y * flYScale );
			vgui::surface()->DrawUnicodeString( killer );
			vgui::surface()->DrawGetTextPos( x, y );

			y /= flYScale;

			x += 5;
		}

		Color iconColor( 255, 255, 255, 255 );

		if ( m_aDeathNoticeList[i].iTeamKill )
		{
			// display it in sickly green
			iconColor = Color( 10, 240, 10, 255 );
		}

		// Draw death weapon
		if (iNumen)
			CRuneComboIcon::Paint(iNumen, -1, x / flXScale, y + lh/2 - iWeaponWidth/2, 64, iWeaponWidth);
		else
			pIcon->DrawSelf( x / flXScale, y + lh/2 - iWeaponWidth/2, iWeaponWidth, iWeaponWidth, iconColor );

		x += iWeaponWidth * flXScale;

		// Draw victims name
		vgui::surface()->DrawSetTextPos( x, y * flYScale );
		vgui::surface()->DrawUnicodeString( victim );
	}
}

//-----------------------------------------------------------------------------
// Purpose: This message handler may be better off elsewhere
//-----------------------------------------------------------------------------
void CObituaries::FireGameEvent( IGameEvent* event)
{
	// Got message during connection
	if ( !g_PR )
		return;

	int killer = engine->GetPlayerForUserID( event->GetInt("attacker") ); 
	int victim = engine->GetPlayerForUserID( event->GetInt("userid") );

	char killedwith[32];
	Q_snprintf( killedwith, sizeof( killedwith ), "%s", event->GetString("weapon") );

	int iNumen = event->GetInt("numen");

	if (FStrEq(killedwith, "world"))
		killedwith[0] = '\0';

	int i;
	for ( i = 0; i < MAX_DEATHNOTICES; i++ )
	{
		if ( m_aDeathNoticeList[i].pIconDeath == NULL )
			break;
	}
	if ( i == MAX_DEATHNOTICES )
	{ // move the rest of the list forward to make room for this item
		Q_memmove( m_aDeathNoticeList, m_aDeathNoticeList+1, sizeof(DeathNoticeItem) * MAX_DEATHNOTICES );
		i = MAX_DEATHNOTICES - 1;
	}

	// Get the names of the players
	const char *killer_name = NULL;
	const char *victim_name = g_PR->GetPlayerName( victim );

	if (killer)
		killer_name = g_PR->GetPlayerName( killer );

	if ( !killer_name )
		killer_name = "";
	if ( !victim_name )
		victim_name = "";

	Q_strncpy( m_aDeathNoticeList[i].szKiller, killer_name, MAX_PLAYER_NAME_LENGTH );
	Q_strncpy( m_aDeathNoticeList[i].szVictim, victim_name, MAX_PLAYER_NAME_LENGTH );

	if ( killer == victim || killer == 0 )
		m_aDeathNoticeList[i].iSuicide = true;

	if ( !strcmp( killedwith, "teammate" ) )
		m_aDeathNoticeList[i].iTeamKill = true;

	CFWeaponID iWeapon = AliasToWeaponID(killedwith);

	// try and find the death identifier in the icon list
	if (FStrEq(killedwith, "fatality"))
        m_aDeathNoticeList[i].pIconDeath = GetHudTexture( killedwith );
	else if (iWeapon == WEAPON_NONE)
        m_aDeathNoticeList[i].pIconDeath = GetHudTexture( "skull" );
	else if (iWeapon == WEAPON_MAGIC)
	{
        m_aDeathNoticeList[i].iNumen = iNumen;
		m_aDeathNoticeList[i].pIconDeath = GetHudTexture( "weaponicon1" );
	}
	else
        m_aDeathNoticeList[i].pIconDeath = GetHudTexture( VarArgs("weaponicon%d", iWeapon) );

	if ( !m_aDeathNoticeList[i].pIconDeath )
	{
		// can't find it, so use the default skull & crossbones icon
		m_aDeathNoticeList[i].pIconDeath = m_pSkullIcon;
	}

	DEATHNOTICE_DISPLAY_TIME = hud_deathnotice_time.GetFloat();

	m_aDeathNoticeList[i].flDisplayTime = gpGlobals->curtime + DEATHNOTICE_DISPLAY_TIME;

	// record the death notice in the console
	if ( m_aDeathNoticeList[i].iSuicide )
	{
		Msg( "%s", m_aDeathNoticeList[i].szVictim );

		if ( !strcmp( killedwith, "world" ) )
		{
			Msg( " died" );
		}
		else
		{
			Msg( " killed self" );
		}
	}
	else if ( m_aDeathNoticeList[i].iTeamKill )
	{
		Msg( "%s", m_aDeathNoticeList[i].szKiller );
		Msg( " killed his teammate " );
		Msg( "%s", m_aDeathNoticeList[i].szVictim );
	}
	else
	{
		Msg( "%s", m_aDeathNoticeList[i].szKiller );
		Msg( " killed " );
		Msg( "%s", m_aDeathNoticeList[i].szVictim );
	}

	if ( killedwith && *killedwith && (*killedwith > 13 ) && strcmp( killedwith, "world" ) && !m_aDeathNoticeList[i].iTeamKill )
	{
		Msg( " with " );

		Msg( killedwith );
	}

	Msg( "\n" );
}

void __MsgFunc_PlayerInfo( bf_read &msg )
{
	int iPlayer = msg.ReadByte();
	int iInfo = msg.ReadShort();
	int iStrongElements = msg.ReadByte();
	int iWeakElements = msg.ReadByte();

	CKillerInfo::SetPlayerInfo(iPlayer, iInfo, iStrongElements, iWeakElements);
}

CKillerInfo* CKillerInfo::s_pKillerInfo = NULL;

CKillerInfo::CKillerInfo()
	: CPanel(0, 0, 100, 100)
{
	CRootPanel::GetRoot()->AddControl(this);

	Assert(!s_pKillerInfo);
	s_pKillerInfo = this;

	m_pKilledBy			= new CLabel(0, 0, BTN_WIDTH, BTN_HEIGHT, "");
	m_pStrength			= new CPanel(0, 0, 100, 100);
	m_pStrengthLabel	= new CLabel(0, 0, BTN_WIDTH, BTN_HEIGHT, "");
	m_pWeakness			= new CPanel(0, 0, 100, 100);
	m_pWeaknessLabel	= new CLabel(0, 0, BTN_WIDTH, BTN_HEIGHT, "");

	AddControl(m_pKilledBy);
	AddControl(m_pStrength);
	m_pStrength->AddControl(m_pStrengthLabel);
	AddControl(m_pWeakness);
	m_pWeakness->AddControl(m_pWeaknessLabel);
}

void CKillerInfo::Destructor()
{
	Assert(s_pKillerInfo);
	s_pKillerInfo = NULL;

	CPanel::Destructor();
}

void CKillerInfo::Open()
{
	if (engine->IsPlayingDemo())
		return;

	if (!s_pKillerInfo)
		new CKillerInfo();	// deleted in CPanel::Destructor()

	s_pKillerInfo->Layout();
	s_pKillerInfo->SetVisible(true);
}

void CKillerInfo::Close()
{
	if (!s_pKillerInfo)
		return;

	s_pKillerInfo->SetVisible(false);
}


void CKillerInfo::SetPlayerInfo(int iPlayer, int iInfo, int iStrongElements, int iWeakElements)
{
	if (!s_pKillerInfo)
		new CKillerInfo();	// deleted in CPanel::Destructor()

	if (iPlayer == 0)
		s_pKillerInfo->m_hKiller = NULL;
	else if (UTIL_PlayerByIndex(iPlayer))
		s_pKillerInfo->m_hKiller = ToCFPlayer(UTIL_PlayerByIndex(iPlayer));
	else
		s_pKillerInfo->m_hKiller = NULL;

	s_pKillerInfo->m_bWeakToMelee		= !!(iInfo&(1<<9));
	s_pKillerInfo->m_bWeakToFirearms	= !!(iInfo&(1<<8));
	s_pKillerInfo->m_bWeakToMagic		= !!(iInfo&(1<<7));
	s_pKillerInfo->m_bWeakToPhyDmg		= !!(iInfo&(1<<6));
	s_pKillerInfo->m_bWeakToMagDmg		= !!(iInfo&(1<<5));
	s_pKillerInfo->m_bStrongToMelee		= !!(iInfo&(1<<4));
	s_pKillerInfo->m_bStrongToFirearms	= !!(iInfo&(1<<3));
	s_pKillerInfo->m_bStrongToMagic		= !!(iInfo&(1<<2));
	s_pKillerInfo->m_bStrongToPhyDmg	= !!(iInfo&(1<<1));
	s_pKillerInfo->m_bStrongToMagDmg	= !!(iInfo&(1<<0));

	s_pKillerInfo->m_eStrongElements	= (element_t)iStrongElements;
	s_pKillerInfo->m_eWeakElements		= (element_t)iWeakElements;

	s_pKillerInfo->Layout();
}

void CKillerInfo::Layout()
{
	if (!C_BasePlayer::GetLocalPlayer())
		return;

	C_CFPlayer* pLocalPlayer = C_CFPlayer::GetLocalCFPlayer();

	SetSize(CFScreenWidth()/3, CFScreenHeight()/4);
	SetPos(CFScreenWidth()/2 - GetWidth()/2, CFScreenHeight()/4*2.6f);
	SetBorder(BT_SOME);

	m_pKilledBy->SetPos(0, 0);
	m_pKilledBy->SetSize(GetWidth(), BTN_HEIGHT);
	m_pKilledBy->SetWrap(false);

	m_pStrength->SetPos(0, BTN_HEIGHT);
	m_pStrength->SetSize(GetWidth()/2, GetHeight()-BTN_HEIGHT);
	m_pStrengthLabel->SetPos(0, 0);
	m_pStrengthLabel->SetSize(m_pStrength->GetWidth(), m_pStrength->GetHeight());

	m_pWeakness->SetPos(GetWidth()/2, BTN_HEIGHT);
	m_pWeakness->SetSize(GetWidth()/2, GetHeight()-BTN_HEIGHT);
	m_pWeaknessLabel->SetPos(0, 0);
	m_pWeaknessLabel->SetSize(m_pWeakness->GetWidth(), m_pWeakness->GetHeight());

	if (m_hKiller != NULL)
	{
		if (pLocalPlayer == m_hKiller)
			m_pKilledBy->SetText("#You_killed_yourself");
		else
		{
			m_pKilledBy->SetText("#You_were_killed_by");
			m_pKilledBy->AppendText(VarArgs(" %s.", m_hKiller->GetPlayerName()));
		}
	}
	else
		m_pKilledBy->SetText("#You_fell_to_your_death");	// We assume this because there's not any other way to die at the moment.

	m_pStrengthLabel->SetText("#Strengths");
	m_pStrengthLabel->AppendText(L"\n \n");
	m_pWeaknessLabel->SetText("#Weaknesses");
	m_pWeaknessLabel->AppendText(L"\n \n");

	if (pLocalPlayer == m_hKiller)
	{
		m_pWeaknessLabel->AppendText("#Killed_self_weakness");
		m_pStrengthLabel->AppendText("#Killed_self_strengths");
	}
	else if (m_hKiller == NULL)
	{
		m_pWeaknessLabel->AppendText("#Killed_falling_weakness");
		m_pStrengthLabel->AppendText("#Killed_falling_strengths");
	}
	else
	{
		if (m_bWeakToMelee)
		{
			m_pWeaknessLabel->AppendText("#Melee");
			m_pWeaknessLabel->AppendText(L"\n");
		}

		if (m_bWeakToFirearms)
		{
			m_pWeaknessLabel->AppendText("#Firearms");
			m_pWeaknessLabel->AppendText(L"\n");
		}

		if (m_bWeakToMagic)
		{
			m_pWeaknessLabel->AppendText("#Magic");
			m_pWeaknessLabel->AppendText(L"\n");
		}

		if (m_bWeakToPhyDmg)
		{
			m_pWeaknessLabel->AppendText("#Physical_attacks");
			m_pWeaknessLabel->AppendText(L"\n");
		}

		if (m_bWeakToMagDmg)
		{
			m_pWeaknessLabel->AppendText("#Magical_attacks");
			m_pWeaknessLabel->AppendText(L"\n");
		}

		const char* pszElements;

		if (m_eWeakElements)
		{
			pszElements = ElementToString(m_eWeakElements);
			m_pWeaknessLabel->AppendText(pszElements);
		}

		if (m_bStrongToMelee)
		{
			m_pStrengthLabel->AppendText("#Melee");
			m_pStrengthLabel->AppendText(L"\n");
		}

		if (m_bStrongToFirearms)
		{
			m_pStrengthLabel->AppendText("#Firearms");
			m_pStrengthLabel->AppendText(L"\n");
		}

		if (m_bStrongToMagic)
		{
			m_pStrengthLabel->AppendText("#Magic");
			m_pStrengthLabel->AppendText(L"\n");
		}

		if (m_bStrongToPhyDmg)
		{
			m_pStrengthLabel->AppendText("#Physical_attacks");
			m_pStrengthLabel->AppendText(L"\n");
		}

		if (m_bStrongToMagDmg)
		{
			m_pStrengthLabel->AppendText("#Magical_attacks");
			m_pStrengthLabel->AppendText(L"\n");
		}

		if (m_eStrongElements)
		{
			pszElements = ElementToString(m_eStrongElements);
			m_pStrengthLabel->AppendText(pszElements);
		}
	}
}

void CKillerInfo::Think()
{
	if (!C_BasePlayer::GetLocalPlayer())
		return;

	C_CFPlayer* pLocalPlayer = C_CFPlayer::GetLocalCFPlayer();

	if (pLocalPlayer->IsAlive())
		Close();

	if (CRoundVictoryPanel::IsOpen())
		Close();

	if (CCFMOTD::IsOpen() || CCFGameInfo::IsOpen() || CCFMenu::IsOpen())
		Close();
}

void CKillerInfo::Paint(int x, int y, int w, int h)
{
	CRootPanel::DrawRect(x, y, x+w, y+h, Color(0, 0, 0, 200));
	CPanel::Paint(x, y, w, h);
}

bool CKillerInfo::IsVisible()
{
	if (!C_BasePlayer::GetLocalPlayer())
		return false;

	C_CFPlayer* pLocalPlayer = C_CFPlayer::GetLocalCFPlayer();

	if (pLocalPlayer->GetTeamNumber() == TEAM_SPECTATOR)
		return false;

	return CPanel::IsVisible();
}