#include "cbase.h"
#include "cfui_menu.h"
#include "cf_gamerules.h"
#include <igameresources.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CCFHudTexture* CTeamChoice::s_pNumeni = NULL;
CCFHudTexture* CTeamChoice::s_pMachindo = NULL;

CTeamChoice::CTeamChoice(CCFMenu::PanelOrientation eOrient, enum eteams_list eTeam)
	: CButton(PANEL_LEFT, PANEL_TOP, MENU_WIDTH, PANEL_HEIGHT, "")
{
	m_eOrient = eOrient;
	m_eTeam = eTeam;
}

void CTeamChoice::Layout()
{
	int iFullHeight = PANEL_HEIGHT - BTN_HEIGHT - BTN_BORDER*3;
	int iHalfHeight = (PANEL_HEIGHT - BTN_HEIGHT - BTN_BORDER*4)/2;
	int iWidth = (MENU_WIDTH - BTN_BORDER*3)/2;

	switch (m_eOrient)
	{
	case CCFMenu::PO_TL:
		SetPos(BTN_BORDER, BTN_HEIGHT + BTN_BORDER*2);
		SetSize(iWidth, iHalfHeight);
		break;

	case CCFMenu::PO_L:
		SetPos(BTN_BORDER, BTN_HEIGHT + BTN_BORDER*2);
		SetSize(iWidth, iFullHeight);
		break;

	case CCFMenu::PO_BL:
		SetPos(BTN_BORDER, BTN_HEIGHT + BTN_BORDER*3 + iHalfHeight);
		SetSize(iWidth, iHalfHeight);
		break;

	case CCFMenu::PO_TR:
		SetPos(BTN_BORDER + iWidth + BTN_BORDER, BTN_HEIGHT + BTN_BORDER*2);
		SetSize(iWidth, iHalfHeight);
		break;

	case CCFMenu::PO_R:
		SetPos(BTN_BORDER + iWidth + BTN_BORDER, BTN_HEIGHT + BTN_BORDER*2);
		SetSize(iWidth, iFullHeight);
		break;

	case CCFMenu::PO_BR:
		SetPos(BTN_BORDER + iWidth + BTN_BORDER, BTN_HEIGHT + BTN_BORDER*3 + iHalfHeight);
		SetSize(iWidth, iHalfHeight);
		break;

	default:
		Assert(!"CTeamChoice::CTeamChoice() Bad orientation\n");
		break;
	}

	CButton::Layout();
}

void CTeamChoice::Paint(int x, int y, int w, int h)
{
	int iWidth = min(w, h)-6;
	
	bool bAlignedRight = false;
	switch (m_eOrient)
	{
	case CCFMenu::PO_TR:
	case CCFMenu::PO_R:
	case CCFMenu::PO_BR:
		bAlignedRight = true;
		break;
	default:
		break;
	}

	int iPadding = 0;
	if (bAlignedRight)
		iPadding = w - 6 - iWidth;

	if (m_eTeam == TEAM_MACHINDO)
		s_pMachindo->DrawSelf(x+3+iPadding, y+3, iWidth, iWidth, Color(255, 255, 255, 255));
	else if (m_eTeam == TEAM_NUMENI)
		s_pNumeni->DrawSelf(x+3+iPadding, y+3, iWidth, iWidth, Color(255, 255, 255, 255));

	CButton::Paint(x, y, w, h);
}

CTeamPanel::CTeamPanel()
	: CPanel(0, BTN_HEIGHT + BTN_BORDER, MENU_WIDTH, PANEL_HEIGHT)
{
	m_pChoose = new CLabel(3, 3, MENU_WIDTH - 6, BTN_HEIGHT, "#Choose_a_team");

	m_pAuto = new CButton(GetWidth() - BTN_BORDER*2 - BTN_WIDTH*2, BTN_BORDER, BTN_WIDTH, BTN_HEIGHT, "#Auto_assign");
	m_pSpec = new CButton(GetWidth() - BTN_BORDER*1 - BTN_WIDTH*1, BTN_BORDER, BTN_WIDTH, BTN_HEIGHT, "#Spectate");

	m_apTeams.AddToTail(new CTeamChoice(CCFMenu::PO_R, TEAM_NUMENI));
	m_apTeams.AddToTail(new CTeamChoice(CCFMenu::PO_L, TEAM_MACHINDO));

	AddControl(m_pChoose);
	AddControl(m_pAuto);
	AddControl(m_pSpec);

	for (int i = 0; i < m_apTeams.Count(); i++)
	{
		AddControl(m_apTeams[i]);
		m_apTeams[i]->SetClickedListener(this, &CTeamPanel::ChooseTeam, new KeyValues("chooseteam", "team", m_apTeams[i]->m_eTeam));
	}

	m_pAuto->SetClickedListener(this, &CTeamPanel::ChooseTeam, new KeyValues("chooseteam", "team", TEAM_UNASSIGNED));
	m_pSpec->SetClickedListener(this, &CTeamPanel::ChooseTeam, new KeyValues("chooseteam", "team", TEAM_SPECTATOR));

	Layout();
}

void CTeamPanel::Layout()
{
	SetSize(MENU_WIDTH, PANEL_HEIGHT);

	m_pChoose->SetSize(MENU_WIDTH - 6, BTN_HEIGHT);
	m_pAuto->SetPos(GetWidth() - BTN_BORDER*2 - BTN_WIDTH*2, BTN_BORDER);
	m_pSpec->SetPos(GetWidth() - BTN_BORDER*1 - BTN_WIDTH*1, BTN_BORDER);

	UpdateTeams();

	CPanel::Layout();
}

void CTeamPanel::UpdateTeams()
{
	int i;

	if (!GameResources())
		return;

	for (i = 0; i < 2; i++)
	{
		const char* pszTeamName = GameResources()->GetTeamName( m_apTeams[i]->m_eTeam );

		// Give us a good, strong name that would make our grandparents proud.
		m_apTeams[i]->SetText(VarArgs("#%s", pszTeamName));
	}
}

void CTeamPanel::ChooseTeamCallback(KeyValues* pParms)
{
	char cmd[64];

	int iTeam = pParms->GetFirstValue()->GetInt();

	if (iTeam >= TEAM_UNASSIGNED && iTeam < CF_TEAM_COUNT)
	{
		Q_snprintf( cmd, sizeof( cmd ), "jointeam %i", iTeam );
		engine->ClientCmd(cmd);
	}

	// Assume for now that the deed is done, and have it be overwritten if it is wrong.
	C_BasePlayer::GetLocalPlayer()->ChangeTeam(iTeam);

	if (CRootPanel::GetCloseAfter() || iTeam == TEAM_SPECTATOR)
		CCFMenu::Close();
	else
	{
		CCFMenu::s_pMenu->SetPanel((CFPanel)(CCFMenu::s_pMenu->GetPanel()+1));
		CCFMenu::s_pMenu->Layout();
	}
}
