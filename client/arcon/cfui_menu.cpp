#include "cbase.h"
#include "cfui_menu.h"
#include "cfgui_shared.h"
#include "hud_macros.h"
#include "input.h"
#include "weapon_cfbase.h"
#include "cf_gamerules.h"
#include <igameresources.h>
#include "networkstringtable_clientdll.h"
#include "armament.h"
#include "cfui_scoreboard.h"
#include "hud_objectives.h"
#include "hud_death.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CCFMOTD* CCFMOTD::s_pMOTD = NULL;
CCFGameInfo* CCFGameInfo::s_pGameInfo = NULL;
CCFMenu* CCFMenu::s_pMenu = NULL;

CCFHudTexture* CCFGameInfo::s_pCTF = NULL;
CCFHudTexture* CCFGameInfo::s_pPariah = NULL;

CCFHudTexture* CDeleteButton::s_pX = NULL;
CCFHudTexture* CDeleteButton::s_pXH = NULL;
CCFHudTexture* CDeleteButton::s_pXD = NULL;

CCFHudTexture* CArrowButton::s_pUp = NULL;
CCFHudTexture* CArrowButton::s_pUpH = NULL;
CCFHudTexture* CArrowButton::s_pUpD = NULL;
CCFHudTexture* CArrowButton::s_pDown = NULL;
CCFHudTexture* CArrowButton::s_pDownH = NULL;
CCFHudTexture* CArrowButton::s_pDownD = NULL;

CCFHudTexture* CCheckmark::s_pCheckmark = NULL;
CCFHudTexture* CCheckmark::s_pCheckmarkD = NULL;

#ifdef _DEBUG
// The purpose of this method is to test the memory allocation/deallocation
// functions to see if they are working properly or will cause problems.
// Testing by brute force, I like. Fuck unit tests, who needs them? Anyways
// if it works it might also be useful to test code changes to the menu after
// Edit and Continue by forcing everything to rebuild itself.
static void KillHudMenu_f( void )
{
	CCFMenu* pMenu = CCFMenu::s_pMenu;
	if (pMenu)
	{
		pMenu->Close();
		pMenu->Destructor();
		pMenu->Delete();
	}

	// Destructor() sets s_pMenu to NULL, so the next time OpenMenu is called it should be recreated.
}

static ConCommand hud_killmenu( "hud_killmenu", KillHudMenu_f, "Destroy the hud menu and allow it to recreate itself.", FCVAR_NONE );
#endif

static void __MsgFunc_CFPanel( bf_read &msg )
{
	CFPanel ePanel = (CFPanel)msg.ReadByte();
	bool bShow = !!msg.ReadByte();
	bool bCloseAfter = !!msg.ReadByte();

	if (ePanel == CF_SCOREBOARD)
	{
		CScoreboard::Get()->OpenScoreboard(bShow);
		return;
	}

	if (ePanel == CF_ROUND_OVER)
	{
		CRoundVictoryPanel::Open();
		return;
	}

	if (ePanel == CF_KILLERINFO)
	{
		CKillerInfo::Open();
		return;
	}

	if (bShow)
		CCFMenu::OpenMenu(ePanel, bCloseAfter);
	else
		CCFMenu::Close();
}

extern void __MsgFunc_Damage( bf_read &msg );
extern void __MsgFunc_CFMessage( bf_read &msg );
extern void __MsgFunc_PlayerInfo( bf_read &msg );

void CRootPanel::Init()
{
	HOOK_MESSAGE( CFPanel );
	HOOK_MESSAGE( Damage );
	HOOK_MESSAGE( CFMessage );
	HOOK_MESSAGE( PlayerInfo );
}

CCFMOTD::CCFMOTD()
	: CPanel(MENU_SPACE, MENU_SPACE, MENU_WIDTH, MENU_HEIGHT)
{
	Assert(!s_pMOTD);

	s_pMOTD = this;

	SetBorder(BT_NONE);

	m_pHostname = new CLabel(0, 0, GetWidth(), BTN_HEIGHT, "");
	m_pHostname->SetText(CRootPanel::GetRoot()->GetHostname());
	AddControl(m_pHostname);

	m_pTextPanel = new CPanel(0, BTN_HEIGHT, GetWidth(), GetHeight()-BTN_HEIGHT*2-BTN_BORDER);
	AddControl(m_pTextPanel);

	m_pText = new CLabel(BTN_BORDER, BTN_BORDER,
		m_pTextPanel->GetWidth()-BTN_BORDER/2,
		m_pTextPanel->GetHeight()-BTN_BORDER/2,
		"");
	m_pText->SetAlign(CLabel::TA_TOPLEFT);
	m_pTextPanel->AddControl(m_pText);

	m_pFinish = new CButton(GetWidth() - BTN_WIDTH, GetHeight() - BTN_HEIGHT, BTN_WIDTH, BTN_HEIGHT, "#OK");
	AddControl(m_pFinish);
	m_pFinish->SetClickedListener(this, &CCFMOTD::Finish);

	LoadMOTD();

	CRootPanel::GetRoot()->AddControl(this);
}

void CCFMOTD::LoadMOTD()
{
	const char *pszText = NULL;
	int iLength = 0;
	int iIndex = g_pStringTableInfoPanel->FindStringIndex( "motd" );

	if ( iIndex != ::INVALID_STRING_INDEX )
		pszText = (const char *)g_pStringTableInfoPanel->GetStringUserData( iIndex, &iLength );

	if ( !pszText || !pszText[0] )
		return;

	m_pText->SetText(pszText);
}

void CCFMOTD::Destructor()
{
	Assert(s_pMOTD);

	s_pMOTD = NULL;

	CPanel::Destructor();
}

void CCFMOTD::Open()
{
	if (engine->IsPlayingDemo())
		return;

	if (!s_pMOTD)
		new CCFMOTD();	// deleted in CPanel::Destructor()

	s_pMOTD->SetVisible(true);

	CCFMenu::Close();
}

void CCFMOTD::Close()
{
	if (!s_pMOTD)
		return;

	s_pMOTD->SetVisible(false);
}

void CCFMOTD::FinishCallback(KeyValues* pParms)
{
	s_pMOTD->SetVisible(false);
	CCFGameInfo::Open();
}

void CCFMOTD::SetVisible(bool bVisible)
{
	CPanel::SetVisible( bVisible );
	m_Input.Activate( bVisible );
	CRootPanel::GetRoot()->Activate( bVisible );
	CRootPanel::GetRoot()->SetBackground( bVisible );
}

bool CCFMOTD::KeyPressed( vgui::KeyCode code )
{
	// These keys cause the panel to shutdown
	if ( code == KEY_ENTER || code == KEY_SPACE )
	{
		FinishCallback(NULL);
		return true;
	}

	return CPanel::KeyPressed(code);
}

CCFGameInfo::CCFGameInfo()
	: CPanel(MENU_SPACE, MENU_SPACE, MENU_WIDTH, MENU_HEIGHT)
{
	Assert(!s_pGameInfo);

	s_pGameInfo = this;

	SetBorder(BT_NONE);

	m_pOK = new CButton(GetWidth()/2 + (GetHeight()-BTN_HEIGHT*2-BTN_BORDER)/2 - BTN_WIDTH, GetHeight() - BTN_HEIGHT, BTN_WIDTH, BTN_HEIGHT, "#OK");
	AddControl(m_pOK);
	m_pOK->SetClickedListener(this, &CCFGameInfo::OK);

	CRootPanel::GetRoot()->AddControl(this);
}

void CCFGameInfo::Destructor()
{
	Assert(s_pGameInfo);

	s_pGameInfo = NULL;

	CPanel::Destructor();
}

void CCFGameInfo::Paint(int x, int y, int w, int h)
{
	if (!IsVisible())
		return;

	int dw = h-BTN_HEIGHT*2-BTN_BORDER;
	int dh = h-BTN_HEIGHT*2-BTN_BORDER;
	int dx = x + w/2 - dw/2;
	int dy = y + BTN_HEIGHT;

	switch (CFGameRules()->GetGameMode())
	{
	case CF_GAME_CTF:
	default:
		if (s_pCTF)
			s_pCTF->DrawSelf(dx, dy, dw, dh, Color(255, 255, 255, 255));
		break;

	case CF_GAME_PARIAH:
		if (s_pPariah)
			s_pPariah->DrawSelf(dx, dy, dw, dh, Color(255, 255, 255, 255));
		break;
	}

	CPanel::Paint(x, y, w, h);
}

void CCFGameInfo::Open()
{
	if (engine->IsPlayingDemo())
		return;

	if (CFGameRules()->GetGameMode() == CF_GAME_TDM)
	{
		if (s_pGameInfo)
			s_pGameInfo->SetVisible(false);
		CCFMenu::OpenMenu(CF_TEAM_PANEL, false);
		return;
	}

	if (!s_pGameInfo)
		new CCFGameInfo();	// deleted in CPanel::Destructor()

	s_pGameInfo->SetVisible(true);
}

void CCFGameInfo::Close()
{
	if (!s_pGameInfo)
		return;

	s_pGameInfo->SetVisible(false);
}

void CCFGameInfo::OKCallback(KeyValues* pParms)
{
	s_pGameInfo->SetVisible(false);
	CCFMenu::OpenMenu(CF_TEAM_PANEL, false);
}

void CCFGameInfo::SetVisible(bool bVisible)
{
	CPanel::SetVisible( bVisible );
	m_Input.Activate( bVisible );
	CRootPanel::GetRoot()->Activate( bVisible );
	CRootPanel::GetRoot()->SetBackground( bVisible );
}

bool CCFGameInfo::KeyPressed( vgui::KeyCode code )
{
	// These keys cause the panel to shutdown
	if ( code == KEY_ENTER || code == KEY_SPACE )
	{
		OKCallback(NULL);
		return true;
	}

	return CPanel::KeyPressed(code);
}

CCFMenu::CCFMenu()
	: CPanel(MENU_SPACE, MENU_SPACE, MENU_WIDTH, MENU_HEIGHT)
{
	Assert(!s_pMenu);

	m_bShowingScoreboard = false;

	SetBorder(BT_NONE);

	m_pFinish	= new CButton(GetWidth() - BTN_BORDER*0 - BTN_WIDTH*1, 0, BTN_WIDTH, BTN_HEIGHT, "#Close");
	m_pFinish->SetClickedListener(this, &CCFMenu::Finish);
	AddControl(m_pFinish);

	m_pConfigs = new CButton(0, 0, BTN_WIDTH, BTN_HEIGHT, "#Save_Config");
	AddControl(m_pConfigs);

	m_pConfigs->SetClickedListener(this, &CCFMenu::SaveConfig);

	char* aszNames[] =
	{
		"#Team",
		"#Character",
		"#Weapon",
		"#Armor",
		"#Numen",
		"#Binding"
	};

	CButton *pButton;

	int i;
	for (i = 0; i < 6; i++)
	{
		pButton = new CButton(BTN_BORDER*i + BTN_WIDTH*i, 0, BTN_WIDTH, BTN_HEIGHT, aszNames[i], true);
		pButton->SetClickedListener(this, &CCFMenu::Panel, new KeyValues("choosepanel", "panel", i));
		pButton->SetUnclickedListener(this, &CCFMenu::Panel, new KeyValues("choosepanel", "panel", i));
		m_apTabs.AddToTail(pButton);
		AddControl(pButton);
	}

	// FIXME: Temporarily disable the Binds tab until we figure out what to do with it.
	m_apTabs[5]->SetVisible(false);

	// Deallocated in ~CPanel
	m_pTeamPanel = new CTeamPanel();
	m_pCharPanel = new CCharPanel();
	m_pWeapPanel = new CWeapPanel();
	m_pArmrPanel = new CArmrPanel();
	m_pRunePanel = new CRunePanel();
	m_pBindPanel = new CBindPanel();

	AddControl(m_pTeamPanel);
	AddControl(m_pCharPanel);
	AddControl(m_pWeapPanel);
	AddControl(m_pArmrPanel);
	AddControl(m_pRunePanel);
	AddControl(m_pBindPanel);

	int iCreditsWidth = BTN_BORDER*6 + BTN_WIDTH*6;
	m_pCredits = new CLabel(iCreditsWidth, 0, GetWidth()-iCreditsWidth, BTN_HEIGHT, "#Credits");
	AddControl(m_pCredits);

	m_pTips = new CLabel(0, 0, BTN_WIDTH, BTN_HEIGHT, "#Respawn_Tip");
	AddControl(m_pTips);

	Layout();
}

void CCFMenu::Destructor()
{
	s_pMenu = NULL;

	CPanel::Destructor();
}

void CCFMenu::Layout()
{
	if (!C_BasePlayer::GetLocalPlayer())
		return;

	SetSize(MENU_WIDTH, MENU_HEIGHT);

	// Move children around.
	m_pFinish->SetPos(GetWidth() - BTN_BORDER*0 - BTN_WIDTH, 0);
	m_pConfigs->SetPos(GetWidth() - BTN_BORDER*1 - BTN_WIDTH*2, 0);

	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	m_pCredits->SetPos(-MENU_SPACE, GetHeight() - 60);
	m_pCredits->SetSize(50+MENU_SPACE, BTN_HEIGHT*3);
	m_pCredits->SetText("#Credits");
	m_pCredits->AppendText(VarArgs(" \n%d/%d", pArm->m_iCredits, cvar->FindVar("mp_inventory")->GetInt()));
	m_pCredits->SetWrap(false);
	m_pCredits->SetVisible(m_eCurrPanel > CF_CHAR_PANEL);

	// Be red when health is low.
	if (pArm->m_iCredits < (unsigned int)cvar->FindVar("mp_inventory")->GetInt()/3)
		m_pCredits->SetFGColor(Color(255, 0, 0, 255));
	else
		m_pCredits->SetFGColor(Color(255, 255, 255, 255));

	m_pTips->SetPos(0, MENU_HEIGHT);
	m_pTips->SetSize(MENU_WIDTH, BTN_HEIGHT);

	// Disable navigation buttons if the player is a spectator, force him to pick a team.
	bool bHasTeam = false;
	if (C_BasePlayer::GetLocalPlayer()->GetTeamNumber() != TEAM_SPECTATOR)
		bHasTeam = true;

	// Skip the Team tab, that's the first one.
	for (int i = 1; i < m_apTabs.Count(); i++)
		m_apTabs[i]->SetEnabled(bHasTeam);

	m_pFinish->SetEnabled(bHasTeam);
	m_pConfigs->SetEnabled(bHasTeam);

	CPanel::Layout();
}

void CCFMenu::Paint(int x, int y, int w, int h)
{
	if (!m_bShowingScoreboard)
		CPanel::Paint(x, y, w, h);

	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	if (m_eCurrPanel > CF_CHAR_PANEL)
	{
		float flPercentEmpty = 1 - (pArm->m_iCredits/(float)cvar->FindVar("mp_inventory")->GetInt());
		int iY = y + BTN_HEIGHT + BTN_BORDER*2;
		int iHeight = h - (BTN_HEIGHT*3) - BTN_BORDER;
		Color cBar(255, 255, 255, 155);
		if (flPercentEmpty > 0.66f)
			cBar = Color(255, 0, 0, 155);
		CRootPanel::DrawRect(x, iY + iHeight*flPercentEmpty, x+30, iY+iHeight, cBar);
	}
}

void CCFMenu::OpenMenu(CFPanel ePanel, bool bCloseAfter)
{
	if (engine->IsPlayingDemo())
		return;

	CCFMOTD::Close();
	CCFGameInfo::Close();

	if (!s_pMenu)
	{
		s_pMenu = new CCFMenu();
		CRootPanel::GetRoot()->AddControl(s_pMenu);
	}
	else
	{
		s_pMenu->Layout();
	}

	s_pMenu->SetPanel(ePanel);
	s_pMenu->SetVisible(true);

	CRootPanel::SetCloseAfter(bCloseAfter);

	s_pMenu->m_bKillOnClose = false;
}

bool CCFMenu::IsOpen()
{
	if (!s_pMenu)
		return false;

	return s_pMenu->IsVisible();
}

static ConVar cl_killonclose("cl_killonclose", "1", FCVAR_ARCHIVE);
void CCFMenu::Close()
{
	if (s_pMenu)
	{
		s_pMenu->SetVisible(false);

		if (s_pMenu->m_bKillOnClose && cl_killonclose.GetBool())
			engine->ClientCmd("kill");
	}
}

void CCFMenu::KillOnClose()
{
	if (s_pMenu)
		s_pMenu->m_bKillOnClose = true;
}

void CCFMenu::SetPanel(CFPanel ePanel)
{
	if (C_BasePlayer::GetLocalPlayer() && C_BasePlayer::GetLocalPlayer()->GetTeamNumber() == TEAM_SPECTATOR)
		ePanel = CF_TEAM_PANEL;

	m_pTeamPanel->SetVisible(false);
	m_pCharPanel->SetVisible(false);
	m_pWeapPanel->SetVisible(false);
	m_pArmrPanel->SetVisible(false);
	m_pRunePanel->SetVisible(false);
	m_pBindPanel->SetVisible(false);

	for (int i = 0; i < m_apTabs.Count(); i++)
		m_apTabs[i]->SetToggleState(false);

	m_apTabs[ePanel]->SetToggleState(true);

	switch (ePanel)
	{
	case CF_TEAM_PANEL:
		m_pTeamPanel->SetVisible(true);
		break;

	case CF_CHAR_PANEL:
		m_pCharPanel->SetVisible(true);
		break;

	case CF_WEAP_PANEL:
		m_pWeapPanel->SetVisible(true);
		break;

	case CF_ARMR_PANEL:
		m_pArmrPanel->SetVisible(true);
		break;

	case CF_RUNE_PANEL:
		m_pRunePanel->SetVisible(true);
		break;

	case CF_BIND_PANEL:
		m_pBindPanel->SetVisible(true);
		break;

	default:
		Assert(!"CCFMenu::SetPanel(): Illegal panel\n");
		return;
	}

	m_eCurrPanel = ePanel;

	Layout();
}

void CCFMenu::SetVisible(bool bVisible)
{
	if (!bVisible)
		CConfigsPanel::Close();
	CPanel::SetVisible( bVisible );
	m_Input.Activate( bVisible );
	CRootPanel::GetRoot()->Activate( bVisible );
	CRootPanel::GetRoot()->SetBackground( bVisible );

	if (bVisible)
		engine->ServerCmd( "menuopen" );
	else
		engine->ServerCmd( "menuclosed" );
}

void CCFMenu::FinishCallback(KeyValues* pParms)
{
	Close();
}

void CCFMenu::PanelCallback(KeyValues* pParms)
{
	SetPanel((CFPanel)pParms->GetFirstValue()->GetInt());
}

void CCFMenu::SaveConfigCallback(KeyValues* pKV)
{
	CConfigsPanel::Open(CConfigMgr::CFG_SAVE);
}

bool CCFMenu::KeyPressed( vgui::KeyCode code )
{
	// These keys cause the panel to shutdown
	if ( code == KEY_B && C_BasePlayer::GetLocalPlayer()->GetTeamNumber() != TEAM_SPECTATOR )
	{
		Close();
		return true;
	}
	else if (code == KEY_TAB)
	{
		// The menu stops and stops showing the mouse so the user can't click,
		// but technically is still visible and receiving keyboard events so that
		// it can trap the KeyReleased(KEY_TAB) event.
		m_bShowingScoreboard = true;
		CScoreboard::OpenScoreboard(true);
		CRootPanel::GetRoot()->EnableMouse(false);
		return true;
	}
	else
	{
		return CPanel::KeyPressed( code );
	}
}

bool CCFMenu::KeyReleased( vgui::KeyCode code )
{
	if (code == KEY_TAB)
	{
		m_bShowingScoreboard = false;
		CScoreboard::OpenScoreboard(false);
		CRootPanel::GetRoot()->EnableMouse(true);
		return true;
	}
	else
	{
		return CPanel::KeyPressed( code );
	}
}

CDeleteButton::CDeleteButton()
: CButton(0, 0, 16, 16, "")
{
}

void CDeleteButton::Paint(int x, int y, int w, int h)
{
	if (!IsVisible())
		return;

	if (m_bDown)
		CDeleteButton::s_pXD->DrawSelf(x, y, w, h, Color(255, 255, 255, 255));
	else if (m_bHighlight && CRootPanel::GetRoot()->GetButtonDown() == NULL)
		CDeleteButton::s_pXH->DrawSelf(x, y, w, h, Color(255, 255, 255, 255));
	else
		CDeleteButton::s_pX->DrawSelf(x, y, w, h, Color(255, 255, 255, 255));
}

CArrowButton::CArrowButton(bool bUp)
: CButton(0, 0, 16, 16, "")
{
	m_bUp = bUp;
}

void CArrowButton::Paint(int x, int y, int w, int h)
{
	if (!IsVisible())
		return;

	if (m_bUp)
	{
		if (m_bDown)
			CArrowButton::s_pUpD->DrawSelf(x, y, m_iW, m_iH, Color(255, 255, 255, 255));
		else if (m_bHighlight && CRootPanel::GetRoot()->GetButtonDown() == NULL)
			CArrowButton::s_pUpH->DrawSelf(x, y, m_iW, m_iH, Color(255, 255, 255, 255));
		else
			CArrowButton::s_pUp->DrawSelf(x, y, m_iW, m_iH, Color(255, 255, 255, 255));
	}
	else
	{
		if (m_bDown)
			CArrowButton::s_pDownD->DrawSelf(x, y, m_iW, m_iH, Color(255, 255, 255, 255));
		else if (m_bHighlight && CRootPanel::GetRoot()->GetButtonDown() == NULL)
			CArrowButton::s_pDownH->DrawSelf(x, y, m_iW, m_iH, Color(255, 255, 255, 255));
		else
			CArrowButton::s_pDown->DrawSelf(x, y, m_iW, m_iH, Color(255, 255, 255, 255));
	}
}

CCheckmark::CCheckmark()
: CBaseControl(0, 0, 32, 32)
{
	m_bMarked = false;
}

void CCheckmark::Paint(int x, int y, int w, int h)
{
	if (!IsVisible())
		return;

	if (m_bMarked)
		CCheckmark::s_pCheckmarkD->DrawSelf(x, y, w, h, Color(255, 255, 255, 255));
	else
		CCheckmark::s_pCheckmark->DrawSelf(x, y, w, h, Color(255, 255, 255, 255));
}
