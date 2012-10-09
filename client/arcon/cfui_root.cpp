#include "cbase.h"
#include "hud.h"
#include "iclientmode.h"
#include <vgui/ISurface.h>
#include <vgui/IPanel.h>
#include "input.h"
#include "viewport_panel_names.h"
#include "engine/IEngineSound.h"

#include "c_cf_player.h"
#include "cfui_gui.h"
#include "cfui_scoreboard.h"
#include "cfui_menu.h"
#include "armament.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar r_letterboxing("r_letterboxing", "2", 0, "Letterboxing mode: 0 - Off, 1 - 16:10, 2 - 16:9, 3 - 12:5.");

using namespace cfgui;

CRootPanel*	CRootPanel::s_pRootPanel = NULL;

vgui::HFont CRootPanel::s_hDefaultFont = NULL;

CRootPanel::CRootPanel( IViewPort *pViewPort ) :
	BaseClass( NULL, PANEL_CFGUI ), CPanel(0, 0, CFScreenWidth(), CFScreenHeight())
{
	Assert(!s_pRootPanel);

	s_pRootPanel = this;

	vgui::Panel *pParent = g_pClientMode->GetViewport();
	vgui::Panel::SetParent( pParent );

	Init();

	CPanel::SetBorder(BT_NONE);

	SetTitleBarVisible( false );
	SetMoveable( false );
	SetSizeable( false );
	SetMouseInputEnabled( false );
	SetKeyBoardInputEnabled( false );
	SetPaintBackgroundEnabled( false );
	SetPaintBorderEnabled( false );

	LoadControlSettings("Resource/UI/CFGUI.res");

	m_pArm = NULL;
	m_bBackground = false;
	m_pButtonDown = NULL;
	m_pszHostname = NULL;

	CreateHUDIndicators();

	m_pDragging = NULL;
	m_bCloseAfter = false;

	m_pPopup = NULL;

	m_bFollowMode = false;
	m_flLetterboxGoal = m_flLetterboxCurr = 0;

	gameeventmanager->AddListener(this, "server_spawn", false);
	gameeventmanager->AddListener(this, "player_hintmessage", false);
	gameeventmanager->AddListener(this, "teamplay_game_over", false);

	m_flMenuMusicVolume = m_flMenuMusicGoal = 0;
	m_bMenuMusicPlaying = false;
}

CRootPanel::~CRootPanel( )
{
	Destructor();
}

void CRootPanel::Destructor( )
{
	gameeventmanager->RemoveListener(this);

	if (m_pszHostname)
		free(m_pszHostname);

	m_bDestructing = true;

	int iCount = m_apDroppables.Count();
	for (int i = 0; i < iCount; i++)
	{
		// Christ.
		m_apDroppables[i]->Destructor();
		m_apDroppables[i]->Delete();
	}
	m_apDroppables.Purge();

	m_bDestructing = false;

	CPanel::Destructor();

	s_pRootPanel = NULL;
}

void CRootPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	CButton::s_pButtonL = GetHudTexture("buttonl");
	CButton::s_pButtonR = GetHudTexture("buttonr");
	CButton::s_pButtonT = GetHudTexture("buttont");
	CButton::s_pButtonB = GetHudTexture("buttonb");
	CButton::s_pButtonTL = GetHudTexture("buttontl");
	CButton::s_pButtonTR = GetHudTexture("buttontr");
	CButton::s_pButtonBL = GetHudTexture("buttonbl");
	CButton::s_pButtonBR = GetHudTexture("buttonbr");
	CButton::s_pButtonC = GetHudTexture("buttonc");

	CButton::s_pButtonHL = GetHudTexture("buttonhl");
	CButton::s_pButtonHR = GetHudTexture("buttonhr");
	CButton::s_pButtonHT = GetHudTexture("buttonht");
	CButton::s_pButtonHB = GetHudTexture("buttonhb");
	CButton::s_pButtonHTL = GetHudTexture("buttonhtl");
	CButton::s_pButtonHTR = GetHudTexture("buttonhtr");
	CButton::s_pButtonHBL = GetHudTexture("buttonhbl");
	CButton::s_pButtonHBR = GetHudTexture("buttonhbr");
	CButton::s_pButtonHC = GetHudTexture("buttonhc");

	CButton::s_pButtonDL = GetHudTexture("buttondl");
	CButton::s_pButtonDR = GetHudTexture("buttondr");
	CButton::s_pButtonDT = GetHudTexture("buttondt");
	CButton::s_pButtonDB = GetHudTexture("buttondb");
	CButton::s_pButtonDTL = GetHudTexture("buttondtl");
	CButton::s_pButtonDTR = GetHudTexture("buttondtr");
	CButton::s_pButtonDBL = GetHudTexture("buttondbl");
	CButton::s_pButtonDBR = GetHudTexture("buttondbr");
	CButton::s_pButtonDC = GetHudTexture("buttondc");

	CButton::s_pButtonIL = GetHudTexture("buttonil");
	CButton::s_pButtonIR = GetHudTexture("buttonir");
	CButton::s_pButtonIT = GetHudTexture("buttonit");
	CButton::s_pButtonIB = GetHudTexture("buttonib");
	CButton::s_pButtonITL = GetHudTexture("buttonitl");
	CButton::s_pButtonITR = GetHudTexture("buttonitr");
	CButton::s_pButtonIBL = GetHudTexture("buttonibl");
	CButton::s_pButtonIBR = GetHudTexture("buttonibr");
	CButton::s_pButtonIC = GetHudTexture("buttonic");

	CPanel::s_pPanelL = GetHudTexture("panell");
	CPanel::s_pPanelR = GetHudTexture("panelr");
	CPanel::s_pPanelT = GetHudTexture("panelt");
	CPanel::s_pPanelB = GetHudTexture("panelb");
	CPanel::s_pPanelTL = GetHudTexture("paneltl");
	CPanel::s_pPanelTR = GetHudTexture("paneltr");
	CPanel::s_pPanelBL = GetHudTexture("panelbl");
	CPanel::s_pPanelBR = GetHudTexture("panelbr");
	CPanel::s_pPanelC = GetHudTexture("panelc");

	CPanel::s_pPanelHL = GetHudTexture("panelhl");
	CPanel::s_pPanelHR = GetHudTexture("panelhr");
	CPanel::s_pPanelHT = GetHudTexture("panelht");
	CPanel::s_pPanelHB = GetHudTexture("panelhb");
	CPanel::s_pPanelHTL = GetHudTexture("panelhtl");
	CPanel::s_pPanelHTR = GetHudTexture("panelhtr");
	CPanel::s_pPanelHBL = GetHudTexture("panelhbl");
	CPanel::s_pPanelHBR = GetHudTexture("panelhbr");
	CPanel::s_pPanelHC = GetHudTexture("panelhc");

	CSlidingPanel::s_pArrowExpanded = GetHudTexture("sliderexpanded");
	CSlidingPanel::s_pArrowCollapsed = GetHudTexture("slidercollapsed");

	CDeleteButton::s_pX = GetHudTexture("x");
	CDeleteButton::s_pXH = GetHudTexture("xh");
	CDeleteButton::s_pXD = GetHudTexture("xd");

	CArrowButton::s_pUp = GetHudTexture("up");
	CArrowButton::s_pUpH = GetHudTexture("uph");
	CArrowButton::s_pUpD = GetHudTexture("upd");
	CArrowButton::s_pDown = GetHudTexture("down");
	CArrowButton::s_pDownH = GetHudTexture("downh");
	CArrowButton::s_pDownD = GetHudTexture("downd");

	CCheckmark::s_pCheckmark = GetHudTexture("checkmark");
	CCheckmark::s_pCheckmarkD = GetHudTexture("checkmarkd");

	CWeaponGrid::s_pGridBox = GetHudTexture("gridbox");
	CWeaponDescription::s_pRadialGraph = GetHudTexture("radialgraph");

	CTeamChoice::s_pNumeni = GetHudTexture("numeni");
	CTeamChoice::s_pMachindo = GetHudTexture("machindo");

	CCFGameInfo::s_pCTF = GetHudTexture("info_ctf");
	CCFGameInfo::s_pPariah = GetHudTexture("info_pariah");

	CRuneLayoutPanel::s_pMouseButtons = GetHudTexture("mousebuttons");
	CRuneLayoutPanel::s_pMouseButtonsLMB = GetHudTexture("mousebuttons_lmb");
	CRuneLayoutPanel::s_pMouseButtonsRMB = GetHudTexture("mousebuttons_rmb");

	CCharPanel::s_pNYellow = GetHudTexture("cn_yellow");
	CCharPanel::s_pNBeige = GetHudTexture("cn_beige");
	CCharPanel::s_pNRed = GetHudTexture("cn_red");
	CCharPanel::s_pNOrange = GetHudTexture("cn_orange");

	CCharPanel::s_pMBlue = GetHudTexture("cm_blue");
	CCharPanel::s_pMTeal = GetHudTexture("cm_teal");
	CCharPanel::s_pMGreen = GetHudTexture("cm_green");
	CCharPanel::s_pMPurple = GetHudTexture("cm_purple");

	int iCount = m_apControls.Count();
	for (int i = 0; i < iCount; i++)
	{
		m_apControls[i]->LoadTextures();
	}

	s_hDefaultFont = pScheme->GetFont( "Default", true );

	vgui::Panel::SetSize(ScreenWidth(), ScreenHeight());

	SetPaintBackgroundEnabled( false );

	Activate(false);
}

void CRootPanel::OnThink()
{
	CPanel::Think();

	if (!C_BasePlayer::GetLocalPlayer())
		return;

	C_CFPlayer* pPlayer = ToCFPlayer(C_BasePlayer::GetLocalPlayer());
	if (m_bFollowMode != pPlayer->IsInFollowMode())
	{
		m_bFollowMode = pPlayer->IsInFollowMode();
		m_flLetterboxGoal = m_bFollowMode?1:0;
	}
	m_flLetterboxCurr = Approach(m_flLetterboxGoal, m_flLetterboxCurr, gpGlobals->frametime*4);

	if (CCFMOTD::IsOpen() || CCFGameInfo::IsOpen() || CCFMenu::IsOpen())
		m_flMenuMusicGoal = 1;
	else
		m_flMenuMusicGoal = 0;

	float flOldVolume = m_flMenuMusicVolume;

	m_flMenuMusicVolume = Approach(m_flMenuMusicGoal, m_flMenuMusicVolume, gpGlobals->frametime);

	if (m_flMenuMusicVolume != flOldVolume)
	{
		EmitSound_t params;
		params.m_pSoundName = "CFMusic.Menu";
		params.m_flSoundTime = 0;
		params.m_pOrigin = NULL;
		params.m_pflSoundDuration = NULL;
		params.m_bWarnOnDirectWaveReference = true;
		params.m_flVolume = m_flMenuMusicVolume;

		if (m_bMenuMusicPlaying)
			params.m_nFlags = SND_CHANGE_VOL;

		C_BaseEntity::EmitSound( CLocalPlayerFilter(), SOUND_FROM_WORLD, params );

		m_bMenuMusicPlaying = true;
	}
}

void CRootPanel::Paint()
{
	C_CFPlayer *pPlayer = ToCFPlayer(C_BasePlayer::GetLocalPlayer());

	if ( !pPlayer )
		return;

	if (pPlayer->m_hCameraCinematic != NULL)
		return;

	C_BaseCombatWeapon *pWeapon = pPlayer->GetPrimaryWeapon();

	// Letterboxing.
	if (r_letterboxing.GetInt() && m_flLetterboxCurr > 0)
	{
		float flWidth = CFScreenWidth();
		float flHeight = CFScreenHeight();

		float flLetterboxedHeight;
		switch (r_letterboxing.GetInt())
		{
		default:
			flLetterboxedHeight = flWidth;
			break;

		case 1:
			flLetterboxedHeight = flWidth/16*10;
			break;

		case 2:
			flLetterboxedHeight = flWidth/16*9;
			break;

		case 3:
			flLetterboxedHeight = flWidth/12*5;
			break;
		}

		float flBorderHeight = (flHeight - flLetterboxedHeight)/2 * m_flLetterboxCurr;

		if (flBorderHeight > 0)
		{
			DrawRect(0, 0, flWidth, flBorderHeight, Color(0, 0, 0, m_flLetterboxCurr*255));
			DrawRect(0, flHeight-flBorderHeight, flWidth, flHeight, Color(0, 0, 0, m_flLetterboxCurr*255));
		}
	}

	if ( pWeapon )
		pWeapon->Redraw();

	if (m_bBackground)
		DrawRect(0, 0, CFScreenWidth(), CFScreenHeight(), Color(0, 0, 0, 200));

	// This call comes from VGUI. Channel it through to CFGUI.
	CPanel::Paint();

	if (m_pDragging)
	{
		int mx, my;
		CRootPanel::GetFullscreenMousePos(mx, my);

		int iWidth = CFScreenHeight()/12;
		m_pDragging->GetCurrentDraggable()->Paint(mx-iWidth/2, my-iWidth/2, iWidth, iWidth, true);
	}
}

void CRootPanel::Layout()
{
	// Don't layout if 
	if (m_pDragging)
		return;

	CPanel::Layout();
}

void CRootPanel::PostRenderVGui()
{
	CPanel::PostRenderVGui();
}

void CRootPanel::LevelShutdown()
{
	CPanel::LevelShutdown();

	CScoreboard::SetLocked(false);
	CScoreboard::OpenScoreboard(false);

	EnableMouse( false );
	EnableKeyboard( false );

	// This will become invalid over map changes.
	m_pArm = NULL;

	m_flMenuMusicVolume = m_flMenuMusicGoal = 0;

	C_BaseEntity::StopSound(SOUND_FROM_WORLD, "CFMusic.Menu");
	m_bMenuMusicPlaying = false;
}

void CRootPanel::SetArmament(CArmament* pArm)
{
	CRootPanel* pRoot = GetRoot();
	Assert(pRoot);

	pRoot->m_pArm = pArm;

	UpdateArmament(pRoot->m_pArm);
}

void CRootPanel::UpdateArmament(CArmament* pArm)
{
	CRootPanel* pRoot = GetRoot();
	Assert(pRoot);

	if (pArm != pRoot->m_pArm)
		return;

	// Update stuff like the "Credits: 123/234" label and other stuff.
	pRoot->Layout();
}

void CRootPanel::Activate( bool bEnabled )
{
	C_CFPlayer* pPlayer = NULL;

	if (C_BasePlayer::GetLocalPlayer())
		pPlayer = C_CFPlayer::GetLocalCFPlayer();

	vgui::Panel::SetVisible(true);
	vgui::Panel::SetEnabled(true);

	if (pPlayer)
	{
		if (bEnabled)
		{
			SetArmament(pPlayer->m_pArmament);
		}
		else
		{
			if (!pPlayer->IsAlive() || pPlayer->GetTeamNumber() == TEAM_SPECTATOR || pPlayer->GetTeamNumber() == TEAM_UNASSIGNED)
				SetArmament(pPlayer->m_pArmament);
			else
				SetArmament(pPlayer->m_pCurrentArmament);
		}
	}

	EnableMouse( CInputManager::ShouldHaveInput() );
	EnableKeyboard( CInputManager::ShouldHaveInput() );
}

void CRootPanel::EnableMouse( bool bEnabled )
{
	SetMouseInputEnabled( bEnabled );
}

void CRootPanel::EnableKeyboard( bool bEnabled )
{
	SetKeyBoardInputEnabled( bEnabled );
}

void CRootPanel::SetButtonDown(CButton* pButton)
{
	m_pButtonDown = pButton;
}

CButton* CRootPanel::GetButtonDown()
{
	return m_pButtonDown;
}

void CRootPanel::OnKeyCodePressed( vgui::KeyCode code )
{
	if (!CPanel::KeyPressed(code))
		vgui::Frame::OnKeyCodePressed(code);
}

void CRootPanel::OnKeyCodeReleased( vgui::KeyCode code )
{
	if (!CPanel::KeyReleased(code))
		vgui::Frame::OnKeyCodeReleased(code);
}

void CRootPanel::OnMousePressed(vgui::MouseCode code)
{
	Assert(!m_pDragging);

	if (m_pPopup)
	{
		int mx, my;
		CRootPanel::GetFullscreenMousePos(mx, my);

		int x = 0, y = 0, w = 0, h = 0;
		m_pPopup->GetAbsDimensions(x, y, w, h);
		if (!(mx >= x &&
			my >= y &&
			mx < x + w &&
			my < y + h))
		{
			m_pPopup->Close();
			m_pPopup = NULL;
		}
	}

	CPanel::MousePressed(code);
}

void CRootPanel::OnMouseReleased(vgui::MouseCode code)
{
	if (m_pDragging)
	{
		if (DropDraggable())
			return;
	}

	bool bUsed = CPanel::MouseReleased(code);

	if (!bUsed)
	{
		// Nothing caught the mouse release, so lets try to pop some buttons.
		if (m_pButtonDown)
			m_pButtonDown->Pop(false, true);
	}
}

void CRootPanel::OnCursorMoved(int x, int y)
{
	if (!m_pDragging)
	{
		CPanel::CursorMoved(x/CFScreenWidthScale(), y/CFScreenHeightScale());
	}
}

void CRootPanel::OnKeyCodeTyped(vgui::KeyCode code)
{
	if (code == KEY_ESCAPE)
		CCFMenu::Close();
}

void CRootPanel::DragonDrop(IDroppable* pDroppable)
{
	if (!pDroppable->IsVisible())
		return;

	Assert(pDroppable);

	m_pDragging = pDroppable;

	C_BaseEntity::EmitSound( CLocalPlayerFilter(), SOUND_FROM_LOCAL_PLAYER, "CFHUD.Pickup" );
}

void CRootPanel::AddDroppable(IDroppable* pDroppable)
{
	Assert(pDroppable);
	m_apDroppables.AddToTail(pDroppable);
}

void CRootPanel::RemoveDroppable(IDroppable* pDroppable)
{
	Assert(pDroppable);
	if (!m_bDestructing)
		m_apDroppables.FindAndRemove(pDroppable);
}

bool CRootPanel::DropDraggable()
{
	Assert(m_pDragging);

	int mx, my;
	CRootPanel::GetFullscreenMousePos(mx, my);

	// Drop that shit like a bad habit.

	int iCount = m_apDroppables.Count();
	for (int i = 0; i < iCount; i++)
	{
		IDroppable* pDroppable = m_apDroppables[i];

		AssertMsg(pDroppable, "NULL entry in m_apDroppables.\n");

		if (!pDroppable)
			continue;

		if (!pDroppable->IsVisible())
			continue;

		if (!pDroppable->CanDropHere(m_pDragging->GetCurrentDraggable()))
			continue;

		CRect r = pDroppable->GetHoldingRect();
		if (mx >= r.x &&
			my >= r.y &&
			mx < r.x + r.w &&
			my < r.y + r.h)
		{
			pDroppable->SetDraggable(m_pDragging->GetCurrentDraggable());

			m_pDragging = NULL;

			// Layouts during dragging are blocked. Do a Layout() here to do any updates that need doing since the thing was dropped.
			Layout();

			C_BaseEntity::EmitSound( CLocalPlayerFilter(), SOUND_FROM_LOCAL_PLAYER, "CFHUD.Drop" );
		
			return true;
		}
	}

	// Layouts during dragging are blocked. Do a Layout() here to do any updates that need doing since the thing was dropped.
	Layout();

	// Couldn't find any places to drop? Whatever nobody cares about that anyways.
	m_pDragging = NULL;

	C_BaseEntity::EmitSound( CLocalPlayerFilter(), SOUND_FROM_LOCAL_PLAYER, "CFHUD.Err" );

	return false;
}

void CRootPanel::Popup(IPopup* pPopup)
{
	m_pPopup = pPopup;
}

void CRootPanel::FireGameEvent( IGameEvent *event )
{
	const char *pszType = event->GetName();

	if ( Q_strcmp(pszType, "server_spawn") == 0 )
	{
		m_pszHostname = strdup(event->GetString( "hostname" ));
	}

	if ( Q_strcmp(pszType, "teamplay_game_over") == 0)
	{
		CCFMenu::Close();
		CScoreboard::OpenScoreboard(true);
		CScoreboard::SetLocked(true);
	}
}

void CRootPanel::SetCloseAfter(bool bCloseAfter)
{
	CRootPanel* pRoot = GetRoot();
	Assert(pRoot);

	pRoot->m_bCloseAfter = bCloseAfter;
}

bool CRootPanel::GetCloseAfter()
{
	CRootPanel* pRoot = GetRoot();
	Assert(pRoot);

	return pRoot->m_bCloseAfter;
}

void CRootPanel::GetFullscreenMousePos(int& mx, int& my)
{
	float flXScale = CFScreenWidthScale();
	float flYScale = CFScreenHeightScale();

	input->GetFullscreenMousePos(&mx, &my, NULL, NULL);

	mx /= flXScale;
	my /= flYScale;
}

void CRootPanel::DrawRect(int x, int y, int x2, int y2, Color clr)
{
	float flXScale = CFScreenWidthScale();
	float flYScale = CFScreenHeightScale();

	vgui::surface()->DrawSetColor(clr.r(), clr.g(), clr.b(), clr.a());
	vgui::surface()->DrawFilledRect(x*flXScale, y*flYScale, x2*flXScale, y2*flYScale);
}
