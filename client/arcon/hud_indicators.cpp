#include "cbase.h"
#include "hud_indicators.h"
#include "hud_macros.h"
#include "input.h"
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <igameresources.h>
#include "statistics.h"
#include "cfui_menu.h"
#include "c_cf_player.h"
#include "hud_death.h"
#include "hud_damage.h"
#include "hud_objectives.h"
#include "weapon_cfbase.h"
#include "weapon_cfbasemelee.h"
#include "in_buttons.h"
#include "datacache/imdlcache.h"
#include "cf_gamerules.h"
#include "c_cf_playerresource.h"
#include "view_scene.h"
#include "runes.h"
#include "view.h"
#include "weapon_magic.h"

#include <cctype>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace cfgui;

extern ConVar	mp_hpbonustime;
extern ConVar	mp_hpbonusamnt;
extern ConVar	mp_respawntimer;

ConVar	hint_time("hint_time", "8");
ConVar	hud_fastswitch( "hud_fastswitch", "0", FCVAR_ARCHIVE | FCVAR_ARCHIVE_XBOX );

void CRootPanel::CreateHUDIndicators()
{
	// These add themselves to CRootPanel, and so are deallocated in ~CPanel
	new CHudHealth();
	new CHudMagic();
	new CHudWeapons();
	new CHudStatusEffects();
	new CHudTarget();
	new CHudCFCrosshair();
	new CDeathNotifier();
	new CObituaries();
	new CObjectives();
	new CDamageIndicator();
	new CHUDMessages();
	new CHUDHints();
	new CRank();
}

CHudHealth* CHudHealth::s_pHudHealth = NULL;
CCFHudTexture* CHudHealth::s_pHealth = NULL;
CCFHudTexture* CHudHealth::s_pFocus = NULL;
CCFHudTexture* CHudHealth::s_pStamina = NULL;
CCFHudTexture* CHudHealth::s_pBarsL = NULL;
CCFHudTexture* CHudHealth::s_pBarsR = NULL;
CCFHudTexture* CHudHealth::s_pBarsC = NULL;
CCFHudTexture* CHudHealth::s_pDamage = NULL;
CCFHudTexture* CHudHealth::s_pHealthIcon = NULL;
CCFHudTexture* CHudHealth::s_pFocusIcon = NULL;
CCFHudTexture* CHudHealth::s_pStaminaIcon = NULL;

CHudHealth::CHudHealth()
{
	Assert(!s_pHudHealth);
	s_pHudHealth = this;

	m_flAlpha = 1;
	m_flAlphaGoal = 1;
	m_flLastPing = 0;
	m_iMaxHealth = 1000;
	m_iMaxFocus = 100;
	m_iMaxStamina = 100;
	m_flHealthFlash = 0;
	m_flFocusFlash = 0;
	m_flStaminaFlash = 0;

	m_flBonusStart = 0;
	m_flBonusHealth = 0;
}

void CHudHealth::Destructor()
{
	s_pHudHealth = NULL;
}

void CHudHealth::Think()
{
	float flHealth, flFocus, flStamina;
	int iMaxHealth, iMaxFocus, iMaxStamina;
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if ( pPlayer )
	{
		CCFPlayer* pCFPlayer = C_CFPlayer::GetLocalCFPlayer();
		CArmament* pArm = pCFPlayer->m_pArmament;

		if (pCFPlayer->IsObserver() && pCFPlayer->GetObserverTarget())
		{
			pCFPlayer = ToCFPlayer(pCFPlayer->GetObserverTarget());
			pArm = pCFPlayer->m_pArmament;
		}

		if (CCFMenu::IsOpen() && pArm)
		{
			flHealth = iMaxHealth = pArm->GetMaxHealth();
			flFocus = iMaxFocus = pArm->GetMaxFocus();
			flStamina = iMaxStamina = pArm->GetMaxStamina();
		}
		else
		{
			if (C_CFPlayer::GetLocalCFPlayer() == pCFPlayer)
				flHealth = pCFPlayer->m_pStats->GetHealth();
			else
				flHealth = pCFPlayer->GetHealth();
			iMaxHealth = pCFPlayer->m_iMaxHealth;
			flFocus = pCFPlayer->m_pStats->GetFocus();
			iMaxFocus = pCFPlayer->m_pStats->m_iMaxFocus;
			flStamina = pCFPlayer->m_pStats->GetStamina();
			iMaxStamina = pCFPlayer->m_pStats->m_iMaxStamina;
		}

		if (CCFMenu::IsOpen())
		{
			m_flHealth = m_iMaxHealth = flHealth;
		}
		else if (pCFPlayer->m_lifeState <= LIFE_DYING)
		{
			if (m_flHealth - flHealth > 0)
				Ping();
			else if (flHealth - m_flHealth > 0)
				Ping(0.5f);
			else if (pCFPlayer->m_lifeState == LIFE_DYING)
				Ping();

			// Never -below zero
			m_flHealth = max( flHealth, 0 );
			m_iMaxHealth = max( iMaxHealth, 1 );
		}
		else if (pCFPlayer->m_lifeState < LIFE_RESPAWNABLE)
		{
			m_flHealth = 0;
			Ping();
		}
		else
		{
			m_flHealth = m_iMaxHealth;
			Ping();
		}

		if (m_flFocus - flFocus > 1)
			Ping();
		else if (flFocus - m_flFocus > 1)
			Ping(0.5f);

		// Never below zero
		m_flFocus = max( flFocus, 0 );
		m_iMaxFocus = max( iMaxFocus, 1 );

		if (m_flStamina - flStamina > 1)
			Ping();
		else if (flStamina - m_flStamina > 1)
			Ping(0.5f);

		m_flStamina = max( flStamina, 0 );
		m_iMaxStamina = max( iMaxStamina, 1 );
	}

	if (m_flHealth < m_iMaxHealth/3)
	{
		if (m_flHealthFlash == 0)
			m_flHealthFlash = gpGlobals->curtime;
	}
	else
	{
		if (m_flHealthFlash != 0)
			m_flHealthFlash = 0;
	}

	if (m_flFocus < m_iMaxFocus/3)
	{
		if (m_flFocusFlash == 0)
			m_flFocusFlash = gpGlobals->curtime;
	}
	else
	{
		if (m_flFocusFlash != 0)
			m_flFocusFlash = 0;
	}

	if (m_flStamina < m_iMaxStamina/3)
	{
		if (m_flStaminaFlash == 0)
			m_flStaminaFlash = gpGlobals->curtime;
	}
	else
	{
		if (m_flStaminaFlash != 0)
			m_flStaminaFlash = 0;
	}

	if (m_flLastHealth > m_iMaxHealth)
		m_flLastHealth = m_iMaxHealth;
	else if (m_flLastHealth > m_flHealth)
		m_flLastHealth -= 10.0f;
	else
		m_flLastHealth = min(m_flHealth, m_iMaxHealth);

	if (m_flLastFocus > m_iMaxFocus)
		m_flLastFocus = m_iMaxFocus;
	else if (m_flLastFocus > m_flFocus)
		m_flLastFocus -= 1.0f;
	else
		m_flLastFocus = m_flFocus;

	if (m_flLastStamina > m_iMaxStamina)
		m_flLastStamina = m_iMaxStamina;
	else if (m_flLastStamina > m_flStamina)
		m_flLastStamina -= 1.0f;
	else
		m_flLastStamina = m_flStamina;

	// Make this larger than STATISTICS_FRAMETIME so that it doesn't keep disappearing and reappearing.
	if (gpGlobals->curtime - m_flLastPing > 6)
	{
		m_flAlphaGoal = 0.5f;
		m_flLastPing = gpGlobals->curtime;
	}

	if (CCFMenu::IsOpen())
		m_flAlpha = 1;
	else if (fabs(m_flAlphaGoal - m_flAlpha) < .1)
		m_flAlpha = m_flAlphaGoal;
	else
		m_flAlpha = Approach(m_flAlphaGoal, m_flAlpha, 0.1f);

	if (gpGlobals->curtime > m_flBonusStart + 2)
		m_flBonusStart = 0;
}

float CHudHealth::GetAlphaMultiplier(float flFlash)
{
	if (flFlash)
	{
		float flRemap = fmod(gpGlobals->curtime - flFlash, 1);
		if (flRemap < 0.5f)
			return RemapVal(flRemap, 0, 0.5f, 1, 0);
		else
			return RemapVal(flRemap, 0.5f, 1, 0, 1);
	}
	return 1;
}

void CHudHealth::Paint(int x, int y, int w, int h)
{
	C_CFPlayer *pPlayer = C_CFPlayer::GetLocalCFPlayer();

	if (pPlayer->IsObserver() && pPlayer->GetObserverTarget())
		pPlayer = ToCFPlayer(pPlayer->GetObserverTarget());

	if (!pPlayer->IsAlive() && !pPlayer->IsKnockedOut() && !CCFMenu::IsOpen())
		return;

	int iScreenWidth = CFScreenWidth();
	int iScreenHeight = CFScreenHeight();

	int iTotalWidth = (iScreenWidth-600)/2;
	int iX = iScreenWidth/2-iTotalWidth/2;

	int iBarHeight = 14;
	int iBarSpace = 2 + iBarHeight;
	int iTopY = iScreenHeight - 30 - iBarSpace*3;

	PaintBar(0, m_flHealth / m_iMaxHealth, iX, iTopY, iTotalWidth, iBarHeight, GetAlpha(), m_flHealthFlash);

	if (m_flBonusStart)
	{
		int iBonusWidth = RemapVal(m_flBonusHealth, 0, 1000, 0, iTotalWidth);

		CRootPanel::DrawRect(
			RemapValClamped(gpGlobals->curtime - m_flBonusStart, 0, 1, m_flBonusX, iX + iTotalWidth),
			RemapValClamped(gpGlobals->curtime - m_flBonusStart, 0, 1, m_flBonusY, iTopY),
			RemapValClamped(gpGlobals->curtime - m_flBonusStart, 0, 1, m_flBonusX+m_flBonusW, iX + iTotalWidth + iBonusWidth),
			RemapValClamped(gpGlobals->curtime - m_flBonusStart, 0, 1, m_flBonusY+m_flBonusH, iTopY + iBarHeight),
			Color(0, 255, 0, RemapValClamped(gpGlobals->curtime - m_flBonusStart, 1, 2, 200, 0)));
	}

	iTopY += iBarSpace;
	PaintBar(1, m_flFocus / m_iMaxFocus, iX, iTopY, iTotalWidth, iBarHeight, GetAlpha(), m_flFocusFlash);

	iTopY += iBarSpace;
	PaintBar(2, m_flStamina / m_iMaxStamina, iX, iTopY, iTotalWidth, iBarHeight, GetAlpha(), m_flStaminaFlash);
}

void CHudHealth::PaintBar(int iStyle, float flFilled, int x, int y, int w, int h, int a, float flFlash)
{
	CCFHudTexture* pBars = NULL;
	CCFHudTexture* pIcon = NULL;

	switch (iStyle)
	{
	case 0:
	default:
		pBars = s_pHealth;
		pIcon = s_pHealthIcon;
		break;

	case 1:
		pBars = s_pFocus;
		pIcon = s_pFocusIcon;
		break;

	case 2:
		pBars = s_pStamina;
		pIcon = s_pStaminaIcon;
		break;
	}

	int iUsed = flFilled * w;

	float flAlpha = GetAlphaMultiplier(flFlash);

	//m_pDamage->DrawSelf(x + iUsed, y, iDamage, h, Color(255, 255, 255, a*flAlpha));
	pBars->DrawSelf(x, y, iUsed, h, Color(255, 255, 255, a*flAlpha));
	s_pBarsL->DrawSelf(x-1, y, 1, h, Color(255, 255, 255, a));
	s_pBarsC->DrawSelf(x, y, w, h, Color(255, 255, 255, a));
	s_pBarsR->DrawSelf(x + w, y, 1, h, Color(255, 255, 255, a));
	pIcon->DrawSelf(x, y, h*23/18, h, Color(255, 255, 255, a));
}

void CHudHealth::Ping(float flAlpha)
{
	if (flAlpha >= m_flAlphaGoal)
	{
		m_flAlphaGoal = flAlpha;
		m_flLastPing = gpGlobals->curtime;
	}
}

void CHudHealth::SetBonusDimensions(int x, int y, int w, int h)
{
	m_flBonusX = x;
	m_flBonusY = y;
	m_flBonusW = w;
	m_flBonusH = h;
}

void CHudHealth::AnimateBonus(int iBonusHealth)
{
	m_flBonusHealth = iBonusHealth;
	m_flBonusStart = gpGlobals->curtime;
}

void CHudHealth::LevelShutdown()
{
	m_flHealth = m_iMaxHealth = m_flLastHealth = 1000;

	m_flFocus = m_iMaxFocus = m_flLastFocus = 100;

	m_flStamina = m_iMaxStamina = m_flLastStamina = 100;

	m_flAlpha = m_flAlphaGoal = m_flLastPing = 0;

	m_flBonusStart = 0;
}

void CHudHealth::LoadTextures()
{
	s_pHealth = GetHudTexture("hp");
	s_pFocus = GetHudTexture("ap");
	s_pStamina = GetHudTexture("sp");
	s_pBarsL = GetHudTexture("barsl");
	s_pBarsR = GetHudTexture("barsr");
	s_pBarsC = GetHudTexture("barsc");
	s_pDamage = GetHudTexture("bardamage");
	s_pHealthIcon = GetHudTexture("hpicon");
	s_pFocusIcon = GetHudTexture("apicon");
	s_pStaminaIcon = GetHudTexture("spicon");
}

#define CFHUD_MAGIC_COMMAND(x) void __CmdFunc_##x( void ) \
{ \
	CHudMagic::Get()->UserCmd_##x( ); \
}

CFHUD_MAGIC_COMMAND( Slot1 );
CFHUD_MAGIC_COMMAND( Slot2 );
CFHUD_MAGIC_COMMAND( Slot3 );
CFHUD_MAGIC_COMMAND( Slot4 );
CFHUD_MAGIC_COMMAND( Slot5 );
CFHUD_MAGIC_COMMAND( Slot6 );
CFHUD_MAGIC_COMMAND( Slot7 );
CFHUD_MAGIC_COMMAND( Slot8 );
CFHUD_MAGIC_COMMAND( Slot9 );
CFHUD_MAGIC_COMMAND( Slot0 );
CFHUD_MAGIC_COMMAND( Close );

HOOK_COMMAND( slot1, Slot1 );
HOOK_COMMAND( slot2, Slot2 );
HOOK_COMMAND( slot3, Slot3 );
HOOK_COMMAND( slot4, Slot4 );
HOOK_COMMAND( slot5, Slot5 );
HOOK_COMMAND( slot6, Slot6 );
HOOK_COMMAND( slot7, Slot7 );
HOOK_COMMAND( slot8, Slot8 );
HOOK_COMMAND( slot9, Slot9 );
HOOK_COMMAND( slot0, Slot0 );
HOOK_COMMAND( cancelselect, Close );

CHudMagic* CHudMagic::s_pHudMagic = NULL;

CHudMagic::CHudMagic()
{
	Assert(!s_pHudMagic);
	s_pHudMagic = this;

	m_flLastModeChange = 0;

	m_pCombos = new CComboPanel();
	AddControl(m_pCombos);

	m_pNumbers = new CLabel(0, 0, 10, 10, "");
	AddControl(m_pNumbers);
}

void CHudMagic::Destructor()
{
	s_pHudMagic = NULL;

	ICFHUD::Destructor();
}

void CHudMagic::LoadTextures()
{
	m_pCombos->LoadTextures();
}

void CHudMagic::LevelShutdown()
{
	m_flLastModeChange = 0;
}

void CHudMagic::Layout()
{
	if (!C_BasePlayer::GetLocalPlayer())
		return;

	C_CFPlayer *pPlayer = ToCFPlayer(C_BasePlayer::GetLocalPlayer());

	CArmament* pArm = pPlayer->GetActiveArmament();

	char szNumbers[100];
	szNumbers[0] = '\0';
	for (int i = 0; i < pArm->GetRuneCombos(); i++)
		Q_strncat(szNumbers, VarArgs("%s%d", (i!=0)?" ":"", i+1), sizeof(szNumbers));

	m_pNumbers->SetText(szNumbers);
	m_pNumbers->SetSize(0, 0);
	m_pNumbers->EnsureTextFits();
	m_pNumbers->SetPos(50, CFScreenHeight()-m_pNumbers->GetHeight());

	CPanel::Layout();
}

void CHudMagic::Think()
{
	if (!C_BasePlayer::GetLocalPlayer())
		return;

	C_CFPlayer *pPlayer = ToCFPlayer(C_BasePlayer::GetLocalPlayer());

	bool bMode = pPlayer->IsPhysicalMode();

	if (m_bMode != bMode)
	{
		m_flLastModeChange = gpGlobals->curtime;
		m_bMode = bMode;
	}
}

void CHudMagic::Paint(int x, int y, int w, int h)
{
	if (!C_BasePlayer::GetLocalPlayer())
		return;

	C_CFPlayer *pPlayer = ToCFPlayer(C_BasePlayer::GetLocalPlayer());

	if (!pPlayer->IsAlive() && !pPlayer->IsKnockedOut())
		return;

	CArmament* pArm = pPlayer->GetActiveArmament();

	if (!pArm->GetRuneCombos())
		return;

	CRootPanel::DrawRect(50, CFScreenHeight()-m_pNumbers->GetHeight(), 50+m_pNumbers->GetWidth(), CFScreenHeight(), Color(0, 0, 0, 200));

	CPanel::Paint(x, y, w, h);
}

void CHudMagic::OpenMenu( void )
{
	m_pCombos->OpenMenu();
}

void CHudMagic::CloseMenu( void )
{
	m_pCombos->CloseMenu();
}

void CHudMagic::SetActiveCombo( int iCombo )
{
	OpenMenu();
	m_pCombos->SetActiveCombo(iCombo);
}

bool CHudMagic::IsMenuOpen()
{
	return m_pCombos->IsVisible();
}

void CHudMagic::UserCmd_Slot1( void )
{
	SetActiveCombo(0);
}

void CHudMagic::UserCmd_Slot2( void )
{
	SetActiveCombo(1);
}

void CHudMagic::UserCmd_Slot3( void )
{
	SetActiveCombo(2);
}

void CHudMagic::UserCmd_Slot4( void )
{
	SetActiveCombo(3);
}

void CHudMagic::UserCmd_Slot5( void )
{
	SetActiveCombo(4);
}

void CHudMagic::UserCmd_Slot6( void )
{
	SetActiveCombo(5);
}

void CHudMagic::UserCmd_Slot7( void )
{
	SetActiveCombo(6);
}

void CHudMagic::UserCmd_Slot8( void )
{
	SetActiveCombo(7);
}

void CHudMagic::UserCmd_Slot9( void )
{
	SetActiveCombo(8);
}

void CHudMagic::UserCmd_Slot0( void )
{
	SetActiveCombo(9);
}

void CHudMagic::UserCmd_Close( void )
{
	CloseMenu();
}

CHudMagic::CComboPanel::CComboPanel()
	: CPanel(0, 0, 100, 100)
{
	SetBorder(BT_SOME);
	SetVisible(false);
	m_iComboHeight = 64;
	m_iActiveCombo = 0;
}

void CHudMagic::CComboPanel::LoadTextures()
{
	m_pMouseLeft = GetHudTexture("mousebuttons_lmb");
	m_pMouseRight = GetHudTexture("mousebuttons_rmb");
}

void CHudMagic::CComboPanel::Layout()
{
	if (!C_BasePlayer::GetLocalPlayer())
		return;

	C_CFPlayer *pPlayer = ToCFPlayer(C_BasePlayer::GetLocalPlayer());

	CArmament* pArm = pPlayer->GetActiveArmament();

	int iCombos = pArm->GetRuneCombos();

	int iHeight = m_iComboHeight*iCombos + BTN_BORDER*(iCombos+1);

	int iX = 50;
	int iY = CFScreenHeight() - BTN_HEIGHT - iHeight;

	SetPos(iX, iY);
	SetSize(64+BTN_BORDER*2, iHeight);
}

void CHudMagic::CComboPanel::Paint(int x, int y, int w, int h)
{
	if (!C_BasePlayer::GetLocalPlayer())
		return;

	C_CFPlayer *pPlayer = ToCFPlayer(C_BasePlayer::GetLocalPlayer());

	if (!pPlayer->IsAlive() && !pPlayer->IsKnockedOut())
		return;

	CArmament* pArm = pPlayer->GetActiveArmament();

	CRootPanel::DrawRect(x, y, x+w, y+h, Color(0, 0, 0, 200));

	for (int i = 0; i < pArm->GetRuneCombos(); i++)
	{
		int iCX, iCY;
		iCX = x+BTN_BORDER;
		iCY = y + BTN_BORDER*(i+1) + m_iComboHeight*i;

		if (i == m_iActiveCombo)
		{
			CRootPanel::DrawRect(iCX-2, iCY-2, iCX+m_iComboHeight+2, iCY+m_iComboHeight+2, Color(255, 255, 255, 100));
			CRootPanel::DrawRect(iCX, iCY, iCX+m_iComboHeight, iCY+m_iComboHeight, Color(0, 0, 0, 200));
		
			CCFHudTexture* pAttackTex;  
			const char *key; 

			//draw textures for attack-bound keys on hud beside magic combo box.
			if (fmod(gpGlobals->curtime, 1) < 0.5f)  //alternate every half second.
			{	
				key = engine->Key_LookupBinding("+attack");  //capture attack1 binding as const char *
				pAttackTex = CTextureHandler::GetTextureFromKey(key);  //load appropriate texture
			} else {
				key = engine->Key_LookupBinding("+attack2");  //capture attack2 bindings as const char *
				pAttackTex = CTextureHandler::GetTextureFromKey(key);  //load appropriate texture
			}
			
			if (pAttackTex)  //if we have a texture to draw, display it properly.
			{
				int iMouseHeight = m_iComboHeight*3/5;
				int iBRX, iBRY;
				GetBR(iBRX, iBRY);
				pAttackTex->DrawSelf(iBRX + BTN_BORDER, iCY + iMouseHeight/2, iMouseHeight, iMouseHeight, Color(255, 255, 255, 255));
			}
		}

		CRunePosition* pCombo = pArm->GetRuneCombo(i);
		long iCombo = pArm->SerializeCombo(pCombo->m_iWeapon, pCombo->m_iRune);
		CRuneComboIcon::Paint(iCombo, pCombo->m_iWeapon, iCX, iCY, m_iComboHeight, m_iComboHeight, 255);
	}

	CPanel::Paint(x, y, w, h);
}

void CHudMagic::CComboPanel::OpenMenu()
{
	if (!C_CFPlayer::GetLocalCFPlayer())
		return;

	if (IsVisible())
		return;

	if (!C_CFPlayer::GetLocalCFPlayer()->IsAlive() && !C_CFPlayer::GetLocalCFPlayer()->IsKnockedOut())
		return;

	Layout();
	SetVisible(true);
	SetActiveCombo(0);

	if (C_CFPlayer::GetLocalCFPlayer())
		C_CFPlayer::GetLocalCFPlayer()->Instructor_LessonLearned(HINT_NUMBERS_COMBOS);
}

void CHudMagic::CComboPanel::CloseMenu()
{
	SetVisible(false);
}

void CHudMagic::CComboPanel::SetActiveCombo( int iCombo )
{
	if (!C_BasePlayer::GetLocalPlayer())
		return;

	C_CFPlayer *pPlayer = ToCFPlayer(C_BasePlayer::GetLocalPlayer());

	CArmament* pArm = pPlayer->GetActiveArmament();

	if (iCombo >= pArm->GetRuneCombos())
		return;

	if (iCombo < 0)
		return;

	m_iActiveCombo = iCombo;
}

bool CHudMagic::CComboPanel::KeyPressed( vgui::KeyCode code )
{
	if (!C_BasePlayer::GetLocalPlayer())
		return false;

	if (!IsVisible())
		return false;

	C_CFPlayer *pPlayer = ToCFPlayer(C_BasePlayer::GetLocalPlayer());

	CArmament* pArm = pPlayer->GetActiveArmament();

	if ( code == MOUSE_LEFT || code == MOUSE_RIGHT )
	{
		CHudMagic::Get()->CloseMenu();
		CRunePosition* pCombo = pArm->GetRuneCombo(m_iActiveCombo);
		if (pCombo)
			pArm->Bind((code == MOUSE_LEFT)?0:1, pCombo->m_iWeapon, pCombo->m_iRune, true);
		return true;
	}

	return CPanel::KeyPressed(code);
}

void CHudWeapons::CHudWeaponIcon::Think()
{
	if (x < 0 || y < 0 || w < 0 || h < 0 || a < 0)
	{
		x = gx;
		y = gy;
		w = gw;
		h = gh;
		a = ga;
	}
	else
	{
		x = Approach(gx, x, gpGlobals->frametime * 500);
		y = Approach(gy, y, gpGlobals->frametime * 500);
		w = Approach(gw, w, gpGlobals->frametime * 500);
		h = Approach(gh, h, gpGlobals->frametime * 500);
		a = Approach(ga, a, gpGlobals->frametime * 500);
	}
}

void CHudWeapons::CHudWeaponIcon::SetGoal(int x, int y, int w, int h, int a)
{
	gx = x;
	gy = y;
	gw = w;
	gh = h;
	ga = a;
}

CHudWeapons* CHudWeapons::s_pHudWeapons = NULL;

CHudWeapons::CHudWeapons()
{
	for (int i = 0; i < 3; i++)
		RemoveWeaponSlot(i);

	for (int i = 0; i < ATTACK_BINDS; i++)
		RemoveMagicSlot(i);

	Assert(!s_pHudWeapons);
	s_pHudWeapons = this;
}

void CHudWeapons::Destructor()
{
	s_pHudWeapons = NULL;

	ICFHUD::Destructor();
}

void CHudWeapons::LevelShutdown()
{
	m_flNextThink = 0;

	m_aWeaponIcons[0].m_eWeaponID = WEAPON_NONE;
	m_aWeaponIcons[1].m_eWeaponID = WEAPON_NONE;
	m_aWeaponIcons[2].m_eWeaponID = WEAPON_NONE;

	m_aMagicIcons[0].m_iSerializedCombo = 0;
	m_aMagicIcons[1].m_iSerializedCombo = 0;
}

void CHudWeapons::SetWeaponSlot(int iSlot, CFWeaponID eWeapon)
{
	CHudWeaponIcon* pIcon = &m_aWeaponIcons[iSlot];

	if (eWeapon == pIcon->m_eWeaponID)
		return;

	pIcon->m_eWeaponID = eWeapon;

	pIcon->x = pIcon->y = pIcon->w = pIcon->h = pIcon->a = -1;
	pIcon->m_flFlash = 0;
}

void CHudWeapons::SetMagicSlot(int iSlot, long iCombo)
{
	CHudWeaponIcon* pIcon = &m_aMagicIcons[iSlot];

	if (iCombo == pIcon->m_iSerializedCombo)
		return;

	pIcon->m_iSerializedCombo = iCombo;

	pIcon->x = pIcon->y = pIcon->w = pIcon->h = pIcon->a = -1;
}

void CHudWeapons::RemoveWeaponSlot(int iSlot)
{
	m_aWeaponIcons[iSlot].m_eWeaponID = WEAPON_NONE;

	CHudWeaponIcon* pIcon = &m_aWeaponIcons[iSlot];
	pIcon->x = pIcon->y = pIcon->w = pIcon->h = pIcon->a = -1;
	pIcon->m_flFlash = 0;
}

void CHudWeapons::RemoveMagicSlot(int iSlot)
{
	m_aMagicIcons[iSlot].m_iSerializedCombo = 0;

	CHudWeaponIcon* pIcon = &m_aMagicIcons[iSlot];
	pIcon->x = pIcon->y = pIcon->w = pIcon->h = pIcon->a = -1;
}

void CHudWeapons::LoadTextures()
{
	m_pWeaponPanel = GetHudTexture("weapons");
	m_pRifleBullets = GetHudTexture("riflebullets");
	m_pPistolBullets = GetHudTexture("pistolbullets");
	m_pStrongAttack = GetHudTexture("meleestrong");
	m_pFirearmsAlternate = GetHudTexture("firearmalt");
	Assert(m_pWeaponPanel);
}

void CHudWeapons::PaintWeapon(CHudWeaponIcon* pIcon, C_WeaponCFBase* pWeapon)
{
	if (!C_BasePlayer::GetLocalPlayer())
		return;

	if (pWeapon)
	{
		if (pWeapon->UsesPrimaryAmmo())
		{
			float flRemaining = (float)pWeapon->Clip1() / (float)pWeapon->GetMaxClip1();

			if (flRemaining)
			{
				Color cRemaining(255, 255, 255, pIcon->a/2);
				if (flRemaining < 0.3f)
				{
					if (!pIcon->m_flFlash)
						pIcon->m_flFlash = gpGlobals->curtime;

					float flRemap = fmod(gpGlobals->curtime - pIcon->m_flFlash, 1);
					if (flRemap < 0.5f)
						cRemaining = Color(255, 0, 0, RemapVal(flRemap, 0, 0.5f, 1, 0)*255);
					else
						cRemaining = Color(255, 0, 0, RemapVal(flRemap, 0.5f, 1, 0, 1)*255);
				}
				else
					pIcon->m_flFlash = 0;

				if (pWeapon->GetWeaponType() == WT_PISTOL)
					m_pPistolBullets->DrawSelfCropped(pIcon->x, pIcon->y+7*pIcon->h/8, 0, 0, 3*pWeapon->Clip1(), 8, flRemaining*pIcon->w, pIcon->h/8, cRemaining);
				else
					m_pRifleBullets->DrawSelfCropped(pIcon->x, pIcon->y+7*pIcon->h/8, 0, 0, 4*pWeapon->Clip1(), 15, flRemaining*pIcon->w, pIcon->h/8, cRemaining);
			}
		}
		else if (pWeapon->IsMeleeWeapon() && ((CCFBaseMeleeWeapon*)pWeapon)->IsCharging(false))
		{
			float flCharge = (gpGlobals->curtime - pWeapon->GetChargeStartTime()) / cvar->FindVar("mp_chargetime")->GetFloat();

			if (flCharge)
			{
				if (flCharge >= 1)
				{
					flCharge = 1;
					if (!pIcon->m_flFlash)
						pIcon->m_flFlash = gpGlobals->curtime;
				}
				else
					pIcon->m_flFlash = 0;

				float flAlpha;
				if (flCharge == 1)
				{
					float flRemap = fmod(gpGlobals->curtime - pIcon->m_flFlash, 1);
					if (flRemap < 0.5f)
						flAlpha = RemapVal(flRemap, 0, 0.5f, pIcon->a/2, 0);
					else
						flAlpha = RemapVal(flRemap, 0.5f, 1, 0, pIcon->a/2);
				}
				else
					flAlpha = pIcon->a/2;

				CRootPanel::DrawRect(pIcon->x, pIcon->y+7*pIcon->h/8, pIcon->x+flCharge*pIcon->w, pIcon->y+pIcon->h, Color(255, 255, 255, flAlpha));
			}
		}

		CCFHudTexture *pWeaponTex = CTextureHandler::GetTexture(pIcon->m_eWeaponID);
		if (pWeaponTex)
			pWeaponTex->DrawSelf(pIcon->x, pIcon->y, pIcon->w, pIcon->h, Color(255, 255, 255, pIcon->a));
	}
}

void CHudWeapons::PaintAltAbility(CHudWeaponIcon* pIcon, C_WeaponCFBase* pWeapon)
{
	if (!C_BasePlayer::GetLocalPlayer())
		return;

	if (pWeapon)
	{
		if (pWeapon->IsMeleeWeapon())
		{
			CCFHudTexture *pWeaponTex = CTextureHandler::GetTexture(pWeapon->GetWeaponID());
			if (pWeaponTex)
				pWeaponTex->DrawSelf(pIcon->x, pIcon->y, pIcon->w, pIcon->h, Color(255, 255, 255, pIcon->a));

			m_pStrongAttack->DrawSelf(pIcon->x, pIcon->y, pIcon->w, pIcon->h, Color(255, 255, 255, pIcon->a));
		}
		else if (pWeapon->UsesPrimaryAmmo())
		{
			m_pFirearmsAlternate->DrawSelf(pIcon->x, pIcon->y, pIcon->w, pIcon->h, Color(255, 255, 255, pIcon->a));
		}
	}
}

void CHudWeapons::PaintMagic(CHudWeaponIcon* pIcon)
{
	if (pIcon->m_iSerializedCombo == 0)
	{
		return;
	}

	CRuneComboIcon::Paint(pIcon->m_iSerializedCombo, -1, pIcon->x, pIcon->y, pIcon->w, pIcon->h, pIcon->a);
}

void CHudWeapons::Paint(int x, int y, int w, int h)
{
	if (!C_BasePlayer::GetLocalPlayer())
		return;

	C_CFPlayer* pPlayer = C_CFPlayer::GetLocalCFPlayer();

	if (!pPlayer->IsAlive())
		return;

	m_pWeaponPanel->DrawSelf(CFScreenWidth()-301, CFScreenHeight()-99, 301, 99, Color(255, 255, 255, GetAlpha()));

	if (pPlayer->IsPhysicalMode())
	{
		PaintMagic(&m_aMagicIcons[0]);
		PaintMagic(&m_aMagicIcons[1]);

		if (pPlayer->GetSecondaryWeapon() && !pPlayer->HasDualMelee())
		{
			if (pPlayer->GetPrimaryWeapon()->IsRightHanded())
			{
				// Draw the secondary on the left mouse button.
				PaintWeapon(&m_aWeaponIcons[0], pPlayer->GetSecondaryWeapon());
				PaintWeapon(&m_aWeaponIcons[1], pPlayer->GetPrimaryWeapon());
			}
			else
			{
				PaintWeapon(&m_aWeaponIcons[0], pPlayer->GetPrimaryWeapon());
				PaintWeapon(&m_aWeaponIcons[1], pPlayer->GetSecondaryWeapon());
			}
		}
		else
		{
			if (pPlayer->GetPrimaryWeapon())
			{
				PaintWeapon(&m_aWeaponIcons[0], pPlayer->GetPrimaryWeapon());
				PaintAltAbility(&m_aWeaponIcons[1], pPlayer->GetPrimaryWeapon());
			}
		}
	}
	else
	{
		PaintWeapon(&m_aWeaponIcons[0], pPlayer->GetPrimaryWeapon());
		PaintWeapon(&m_aWeaponIcons[1], pPlayer->GetSecondaryWeapon());
		PaintMagic(&m_aMagicIcons[0]);
		PaintMagic(&m_aMagicIcons[1]);
	}
}

void CHudWeapons::Think()
{
	if (gpGlobals->curtime < m_flNextThink)
		return;

	m_flNextThink += 0.1f;

	if (!C_BasePlayer::GetLocalPlayer())
		return;

	CCFPlayer* pPlayer = C_CFPlayer::GetLocalCFPlayer();
	CArmament* pArm = pPlayer->GetActiveArmament();

	if (!pArm)
		return;

	// Update weapons
	if (pPlayer->GetPrimaryWeapon())
	{
		if (pPlayer->GetSecondaryWeapon())
		{
			if (pPlayer->GetPrimaryWeapon()->IsRightHanded())
			{
				SetWeaponSlot(0, pPlayer->GetSecondaryWeapon()->GetWeaponID());
				SetWeaponSlot(1, pPlayer->GetPrimaryWeapon()->GetWeaponID());
			}
			else
			{
				SetWeaponSlot(0, pPlayer->GetPrimaryWeapon()->GetWeaponID());
				SetWeaponSlot(1, pPlayer->GetSecondaryWeapon()->GetWeaponID());
			}
		}
		else
		{
			SetWeaponSlot(0, pPlayer->GetPrimaryWeapon()->GetWeaponID());
			RemoveWeaponSlot(1);
		}
	}

	int i;

	// Update magic
	for (i = 0; i < ATTACK_BINDS; i++)
	{
		CRunePosition* pPos = &pArm->m_aAttackBinds[i];
		if (pPos->m_iWeapon >= 0)
			SetMagicSlot(i, pArm->SerializeCombo(pPos->m_iWeapon, pPos->m_iRune));
		else
			RemoveMagicSlot(i);
	}

	if (pPlayer->IsPhysicalMode())
	{
		m_aWeaponIcons[0].SetGoal(CFScreenWidth()-290, CFScreenHeight()-130, 128, 128, 255);
		m_aWeaponIcons[1].SetGoal(CFScreenWidth()-130, CFScreenHeight()-130, 128, 128, 255);
		m_aWeaponIcons[2].SetGoal(CFScreenWidth()-70,  CFScreenHeight()-120, 48,  48,  70);
		m_aMagicIcons[0].SetGoal(CFScreenWidth()-210, CFScreenHeight()-50,  48, 48, 70);
		m_aMagicIcons[1].SetGoal(CFScreenWidth()-60,  CFScreenHeight()-50,  48, 48, 70);
	}
	else
	{
		m_aWeaponIcons[0].SetGoal(CFScreenWidth()-210, CFScreenHeight()-50, 48, 48, 70);
		m_aWeaponIcons[1].SetGoal(CFScreenWidth()-70,  CFScreenHeight()-38, 32, 32, 70);
		m_aWeaponIcons[2].SetGoal(CFScreenWidth()-46,  CFScreenHeight()-70, 24, 24, 70);
		m_aMagicIcons[0].SetGoal(CFScreenWidth()-290, CFScreenHeight()-130, 128, 128, 255);
		m_aMagicIcons[1].SetGoal(CFScreenWidth()-140, CFScreenHeight()-120, 96,  96,  255);
	}

	for (i = 0; i < 3; i++)
	{
		m_aWeaponIcons[i].Think();
	}

	for (i = 0; i < ATTACK_BINDS; i++)
	{
		m_aMagicIcons[i].Think();
	}
}

bool CHudWeapons::MousePressed(vgui::MouseCode code)
{
	return ICFHUD::MousePressed(code);
}

CHudStatusEffects::CHudStatusEffects()
{
	Init();
}

void CHudStatusEffects::Init()
{
	for (int i = 0; i < TOTAL_STATUSEFFECTS; i++)
	{
		m_iStatusEffects = 0;
		m_flStatusEffectStart[i] = 0;
	}
}

void CHudStatusEffects::LevelShutdown()
{
	Init();
}

void CHudStatusEffects::Think()
{
	if (!C_BasePlayer::GetLocalPlayer())
		return;

	C_CFPlayer* pPlayer = C_CFPlayer::GetLocalCFPlayer();

	if (pPlayer->IsFirstPersonSpectating(pPlayer->GetObserverTarget()))
		pPlayer = ToCFPlayer(pPlayer->GetObserverTarget());

	for (int i = 0; i < TOTAL_STATUSEFFECTS; i++)
	{
		int iStatus = (1<<i);
		if ((pPlayer->m_pStats->m_iStatusEffects&iStatus))
		{
			if (!(m_iStatusEffects&iStatus) && (pPlayer->m_pStats->m_iStatusEffects&iStatus))
			{
				m_flStatusEffectStart[i] = gpGlobals->curtime;
			}
			m_iStatusEffects |= iStatus;
		}
		else
			m_iStatusEffects &= ~iStatus;
	}
}

void CHudStatusEffects::Paint(int x, int y, int w, int h)
{
	C_CFPlayer* pPlayer = C_CFPlayer::GetLocalCFPlayer();

	if (pPlayer->IsFirstPersonSpectating(pPlayer->GetObserverTarget()))
		pPlayer = ToCFPlayer(pPlayer->GetObserverTarget());

	int iDrawn = 0;
	int iSmallWidth = 32;
	int iLargeWidth = CFScreenWidth()/3;

	for (int i = 0; i < TOTAL_STATUSEFFECTS; i++)
	{
		if ((pPlayer->m_pStats->m_iStatusEffects&(1<<i)))
		{
			CCFHudTexture* pTex = CTextureHandler::GetTexture((statuseffect_t)(1<<i));
			Assert(pTex);
			if (!pTex)
				continue;

			float flTime = gpGlobals->curtime - m_flStatusEffectStart[i];
			float flFreezeTime = 0.4f;
			float flTransitionTime = 0.4f;
			float flX, flY, flS, flA;
			if (flTime < flFreezeTime)
			{
				flX = 40;
				flY = ScreenHeight()/2 - iLargeWidth/2;
				flS = iLargeWidth;
				flA = 100;
			}
			else
			{
				flTime -= flFreezeTime;
				flX = RemapValClamped(flTime, 0, flTransitionTime, 40, 20);
				flY = RemapValClamped(flTime, 0, flTransitionTime, CFScreenHeight()/2 - iLargeWidth/2, CFScreenHeight()/2-100 + iSmallWidth*iDrawn);
				flS = RemapValClamped(flTime, 0, flTransitionTime, iLargeWidth, iSmallWidth);
				flA = RemapValClamped(flTime, 0, flTransitionTime, 100, 255);
			}
			pTex->DrawSelf(flX, flY, flS, flS, Color(255,255,255,flA));
			iDrawn++;
		}
	}
}

CHudTarget* CHudTarget::s_pTarget = NULL;

CHudTarget::CHudTarget()
{
	Assert(!s_pTarget);

	s_pTarget = this;

	m_hTarget = NULL;
	m_flLastTargetCheck = 0;
	m_flLastTarget = 0;
}

void CHudTarget::Destructor()
{
	Assert(s_pTarget);

	s_pTarget = NULL;

	ICFHUD::Destructor();
}

void CHudTarget::LoadTextures()
{
	m_pTargetFriend = GetHudTexture("target_friend");
	m_pTargetEnemy = GetHudTexture("target_enemy");
	m_pTargetingFriend = GetHudTexture("targeting_friend");
}

void CHudTarget::LevelShutdown()
{
	m_hTarget = NULL;
	m_flLastTargetCheck = m_flLastTarget = 0;
}

void CHudTarget::Think()
{
	if (!C_BasePlayer::GetLocalPlayer())
		return;

	C_CFPlayer *pPlayer = ToCFPlayer(C_BasePlayer::GetLocalPlayer());

	m_vecTargetSize = Vector(100, 100, 0);
	m_vecDTL = Vector(0, 0, 0);
	m_vecDCenter = m_vecDTL + m_vecTargetSize/2;
	m_vecRTL = Vector(50, 100, 0);
	m_vecRCenter = m_vecRTL + m_vecTargetSize/2;

	if ( gpGlobals->curtime >= m_flLastTarget+0.1f )
	{
		m_hTarget = NULL;
		m_flLastTargetCheck = 0;	// Force a recheck just in case there's another target we might pick up now.
	}

	// Check more often if we don't have a target.
	if ( gpGlobals->curtime >= m_flLastTargetCheck+0.1f )
	{
		m_flLastTargetCheck = gpGlobals->curtime;

		trace_t trace;
		Vector vecForward;

		pPlayer->EyeVectors(&vecForward);

		C_BaseEntity* pIgnore = pPlayer;
		if (pPlayer->GetObserverTarget())
			pIgnore = pPlayer->GetObserverTarget();

		UTIL_TraceLine(pPlayer->EyePosition(), pPlayer->EyePosition() + vecForward*1024, MASK_SHOT_HULL, pIgnore, COLLISION_GROUP_NONE, &trace);

		if ( trace.fraction < 1.0f && trace.m_pEnt->IsPlayer() )
		{
			m_flLastTarget = gpGlobals->curtime;
			m_hTarget = ToCFPlayer(trace.m_pEnt);
		}
	}

	// If the player has a restore 
	if (pPlayer->GetActiveArmament()->HasRestore())
	{
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			C_BasePlayer *pTarget = UTIL_PlayerByIndex( i );

			if (!pTarget)
				continue;

			C_CFPlayer *pCFTarget = ToCFPlayer(pTarget);

			if (pCFTarget == C_CFPlayer::GetLocalCFPlayer())
				continue;

			trace_t result;
			CTraceFilterNoNPCsOrPlayer traceFilter( NULL, COLLISION_GROUP_NONE );
			UTIL_TraceLine( pPlayer->EyePosition(), pCFTarget->WorldSpaceCenter(), MASK_VISIBLE_AND_NPCS, &traceFilter, &result );
			if (result.fraction != 1.0f)
			//if (!pPlayer->IsVisible(pCFTarget))	// This is unfortunately a server-only function, though I'd love to use it here.
				continue;
		}
	}
}

void CHudTarget::Paint(int x, int y, int w, int h)
{
	MDLCACHE_CRITICAL_SECTION();

	C_CFPlayer *pPlayer = ToCFPlayer(C_BasePlayer::GetLocalPlayer());

	if (pPlayer->IsObserver() && pPlayer->GetObserverTarget())
		pPlayer = ToCFPlayer(pPlayer->GetObserverTarget());

	Color EnemyColor(255, 0, 0, 128);	// The enima of my enemy is my friend.
	Color FriendColor(0, 0, 255, 128);

	vgui::surface()->DrawSetTextFont(CRootPanel::s_hDefaultFont);

	if (m_hTarget && !pPlayer->IsPariah())
	{
		C_CFPlayer* pTarget = ToCFPlayer(m_hTarget);
		wchar_t wszTargetName[ 256 ];
		char szTargetName[ 256 ];

		Q_snprintf(szTargetName, sizeof(szTargetName), "%s", pTarget->GetPlayerName());

		g_pVGuiLocalize->ConvertANSIToUnicode( szTargetName, wszTargetName, sizeof( szTargetName ) );

		int lw = 0, lh = 0;
		vgui::surface()->GetTextSize(CRootPanel::s_hDefaultFont, wszTargetName, lw, lh);

		vgui::surface()->DrawSetTextColor(GameResources()->GetTeamColor(pTarget->GetTeamNumber()));
		vgui::surface()->DrawSetTextPos(ScreenWidth()/2 - lw/2, ScreenHeight()/2 + lh*2);
		vgui::surface()->DrawPrintText(wszTargetName, wcslen(wszTargetName));
	}

	bool bTeammate;
	float flWooshTime;
	C_CF_PlayerResource* pGR = dynamic_cast<C_CF_PlayerResource*>(GameResources());

	if (pPlayer->GetDirectTarget())
	{
		C_CFPlayer* pTarget = pPlayer->GetDirectTarget();

		bTeammate = CFGameRules() && CFGameRules()->PlayerRelationship(pPlayer, pPlayer->GetDirectTarget()) == GR_TEAMMATE;
		flWooshTime = gpGlobals->curtime - pPlayer->m_flReceivedDirectTarget;

		RenderTargetIndicator(bTeammate?m_pTargetFriend:m_pTargetEnemy, m_vecDTL.x, m_vecDTL.y, m_vecTargetSize.x, m_vecTargetSize.y, flWooshTime);

		int iMaxHealth = 1000;
		if (pGR)
			iMaxHealth = pGR->GetMaxHealth(pTarget->entindex());

		int iBarX = m_vecDTL.x + m_vecTargetSize.x + BTN_BORDER;
		int iBarY = m_vecDTL.y + BTN_BORDER;
		int iBarH = 8;

		CHudHealth::PaintBar(0, (float)pTarget->GetHealth()/iMaxHealth, iBarX, iBarY, 120, 8, 255);

		iBarY += iBarH;
		CHudHealth::PaintBar(1, pTarget->m_pStats->m_flFocus/pTarget->m_pStats->m_iMaxFocus, iBarX, iBarY, 120, 8, 255);

		iBarY += iBarH;
		CHudHealth::PaintBar(2, pTarget->m_pStats->m_flStamina/pTarget->m_pStats->m_iMaxStamina, iBarX, iBarY, 120, 8, 255);

		iBarY += iBarH + BTN_BORDER;

		wchar_t wszTargetName[ 256 ];
		char szTargetName[ 256 ];
		Q_snprintf(szTargetName, sizeof(szTargetName), "%s", pTarget->GetPlayerName());
		g_pVGuiLocalize->ConvertANSIToUnicode( szTargetName, wszTargetName, sizeof( szTargetName ) );

		vgui::surface()->DrawSetTextColor(GameResources()->GetTeamColor(pTarget->GetTeamNumber()));
		vgui::surface()->DrawSetTextPos(CFScreenWidthScale() * (iBarX), CFScreenHeightScale() * (iBarY));
		vgui::surface()->DrawPrintText(wszTargetName, wcslen(wszTargetName));
	}

	if (pPlayer->GetRecursedTarget() && pPlayer->GetRecursedTarget() != pPlayer->GetDirectTarget())
	{
		C_CFPlayer* pTarget = pPlayer->GetRecursedTarget();

		bTeammate = CFGameRules() && CFGameRules()->PlayerRelationship(pPlayer, pPlayer->GetRecursedTarget()) == GR_TEAMMATE;
		flWooshTime = gpGlobals->curtime - pPlayer->m_flReceivedRecursedTarget;

		RenderTargetIndicator(bTeammate?m_pTargetFriend:m_pTargetEnemy, m_vecRTL.x, m_vecRTL.y, m_vecTargetSize.x, m_vecTargetSize.y, flWooshTime);

		QAngle angTarget;
		VectorAngles((m_vecRCenter - m_vecDCenter), angTarget);
		RenderTargetIndicator(m_pTargetingFriend, m_vecDTL.x, m_vecDTL.y, m_vecTargetSize.x, m_vecTargetSize.y, flWooshTime);

		int iMaxHealth = 1000;
		if (pGR)
			iMaxHealth = pGR->GetMaxHealth(pTarget->entindex());

		int iBarX = m_vecRTL.x + m_vecTargetSize.x + BTN_BORDER;
		int iBarY = m_vecRTL.y + BTN_BORDER;
		int iBarH = 8;

		CHudHealth::PaintBar(0, (float)pTarget->GetHealth()/iMaxHealth, iBarX, iBarY, 120, 8, 255);

		iBarY += iBarH;
		CHudHealth::PaintBar(1, pTarget->m_pStats->m_flFocus/pTarget->m_pStats->m_iMaxFocus, iBarX, iBarY, 120, 8, 255);

		iBarY += iBarH;
		CHudHealth::PaintBar(2, pTarget->m_pStats->m_flStamina/pTarget->m_pStats->m_iMaxStamina, iBarX, iBarY, 120, 8, 255);

		iBarY += iBarH + BTN_BORDER;

		wchar_t wszTargetName[ 256 ];
		char szTargetName[ 256 ];
		Q_snprintf(szTargetName, sizeof(szTargetName), "%s", pTarget->GetPlayerName());
		g_pVGuiLocalize->ConvertANSIToUnicode( szTargetName, wszTargetName, sizeof( szTargetName ) );

		vgui::surface()->DrawSetTextColor(GameResources()->GetTeamColor(pTarget->GetTeamNumber()));
		vgui::surface()->DrawSetTextPos(CFScreenWidthScale() * (iBarX), CFScreenHeightScale() * (iBarY));
		vgui::surface()->DrawPrintText(wszTargetName, wcslen(wszTargetName));
	}
}

void CHudTarget::PostRenderVGui()
{
	if (!IsVisible())
		return;

	C_CFPlayer *pPlayer = ToCFPlayer(C_BasePlayer::GetLocalPlayer());

	if (pPlayer->IsObserver() && pPlayer->GetObserverTarget())
		pPlayer = ToCFPlayer(pPlayer->GetObserverTarget());

	if (pPlayer->GetDirectTarget())
	{
		C_CFPlayer *pTarget = NULL;
		if (m_DirectTarget.GetCopyTarget())
			pTarget = ToCFPlayer(m_DirectTarget.GetCopyTarget());

		// Hand magic effects!
		if (pTarget && pTarget->m_eLHEffectElements != m_eDLHEffectElements)
		{
			m_DirectTarget.RemoveParticles(m_apDLHComboEffects);
			C_CFPlayer::ShowHandMagic(m_DirectTarget.GetModel(), m_apDLHComboEffects, pTarget->m_eLHEffectElements, "lmagic");
			m_DirectTarget.AddParticles(m_apDLHComboEffects);
			m_eDLHEffectElements = pTarget->m_eLHEffectElements;
		}

		if (pTarget && pTarget->m_eRHEffectElements != m_eDRHEffectElements)
		{
			m_DirectTarget.RemoveParticles(m_apDRHComboEffects);
			C_CFPlayer::ShowHandMagic(m_DirectTarget.GetModel(), m_apDRHComboEffects, pTarget->m_eRHEffectElements, "rmagic");
			m_DirectTarget.AddParticles(m_apDRHComboEffects);
			m_eDRHEffectElements = pTarget->m_eRHEffectElements;
		}

		RenderTarget(&m_DirectTarget, pPlayer->GetDirectTarget(), m_vecDTL.x, m_vecDTL.y, m_vecTargetSize.x, m_vecTargetSize.y);
	}

	if (pPlayer->GetRecursedTarget() && pPlayer->GetRecursedTarget() != pPlayer->GetDirectTarget())
	{
		C_CFPlayer *pTarget = NULL;
		if (m_RecursedTarget.GetCopyTarget())
			pTarget = ToCFPlayer(m_RecursedTarget.GetCopyTarget());

		// Hand magic effects!
		if (pTarget && pTarget->m_eLHEffectElements != m_eRLHEffectElements)
		{
			m_RecursedTarget.RemoveParticles(m_apRLHComboEffects);
			C_CFPlayer::ShowHandMagic(m_RecursedTarget.GetModel(), m_apRLHComboEffects, pTarget->m_eLHEffectElements, "lmagic");
			m_RecursedTarget.AddParticles(m_apRLHComboEffects);
			m_eRLHEffectElements = pTarget->m_eLHEffectElements;
		}

		if (pTarget && pTarget->m_eRHEffectElements != m_eRRHEffectElements)
		{
			m_RecursedTarget.RemoveParticles(m_apRRHComboEffects);
			C_CFPlayer::ShowHandMagic(m_RecursedTarget.GetModel(), m_apRRHComboEffects, pTarget->m_eRHEffectElements, "rmagic");
			m_RecursedTarget.AddParticles(m_apRRHComboEffects);
			m_eRRHEffectElements = pTarget->m_eRHEffectElements;
		}

		RenderTarget(&m_RecursedTarget, pPlayer->GetRecursedTarget(), m_vecRTL.x, m_vecRTL.y, m_vecTargetSize.x, m_vecTargetSize.y);
	}
}

void CHudTarget::RenderTarget(CCFRenderablePlayer* pRenderable, C_CFPlayer* pTarget, int x, int y, int w, int h)
{
	int iTeam = pTarget->GetTeamNumber();
	char* pszModelName = NULL;

	switch (iTeam)
	{
	case TEAM_NUMENI:
		pszModelName = PLAYER_MODEL_NUMENI;
		break;

	case TEAM_MACHINDO:
		pszModelName = PLAYER_MODEL_MACHINDO;
		break;
	}

	pRenderable->SetModelName(pszModelName);

	CFWeaponID ePrimary = pTarget->GetPrimaryWeapon()?pTarget->GetPrimaryWeapon()->GetWeaponID():WEAPON_NONE;
	CFWeaponID eSecondary = eSecondary = pTarget->GetSecondaryWeapon()?pTarget->GetSecondaryWeapon()->GetWeaponID():WEAPON_NONE;

	pRenderable->SetPrimaryWeapon(ePrimary);
	pRenderable->SetSecondaryWeapon(eSecondary);

	pRenderable->SetCopyTarget(pTarget);

	pRenderable->Render(x, y, w, h, Vector(200, 0, 0));
}

ConVar cl_rtifov("cl_rtifov", "20", FCVAR_DEVELOPMENTONLY);
ConVar cl_rtih("cl_rtih", "130", FCVAR_DEVELOPMENTONLY);

void CHudTarget::RenderTargetIndicator(CCFHudTexture* pMaterial, int x, int y, int w, int h, float flElapsed)
{
	float flWooshTime = 0.2f;

	float flEnlarge = RemapValClamped(flElapsed, 0, flWooshTime, 160, 20);

	x -= flEnlarge;
	y -= flEnlarge;
	w += flEnlarge*2;
	h += flEnlarge*2;

	pMaterial->DrawSelf(x, y, w, h, Color(255,255,255, RemapValClamped(flElapsed, 0, flWooshTime, 0, 255)));
}

CHudCFCrosshair* CHudCFCrosshair::s_pCrosshair = NULL;

CHudCFCrosshair::CHudCFCrosshair()
{
	Assert(!s_pCrosshair);

	s_pCrosshair = this;

	m_flFlash = 0;
}

void CHudCFCrosshair::Destructor()
{
	Assert(s_pCrosshair);

	s_pCrosshair = NULL;

	ICFHUD::Destructor();
}

void CHudCFCrosshair::LoadTextures()
{
	m_pCrosshairIcon		= GetHudTexture("crosshair_default");
	m_pCrosshairDisabled	= GetHudTexture("crosshair_disabled");
	m_pTargetRing			= GetHudTexture("target_ring");
	m_pDFAUp				= GetHudTexture("dfa_up");
	m_pDFADown				= GetHudTexture("dfa_down");
}

void CHudCFCrosshair::Think()
{
	if (!C_BasePlayer::GetLocalPlayer())
		return;
}

void CHudCFCrosshair::Paint(int x, int y, int w, int h)
{
	MDLCACHE_CRITICAL_SECTION();

	C_CFPlayer *pPlayer = ToCFPlayer(C_BasePlayer::GetLocalPlayer());

	if (pPlayer->IsFirstPersonSpectating(pPlayer->GetObserverTarget()))
		pPlayer = ToCFPlayer(pPlayer->GetObserverTarget());

	C_WeaponCFBase* pWeapon = pPlayer->GetPrimaryWeapon();

	if (!pPlayer)
		return;

	if (!pPlayer->IsAlive())
		return;

	Color clrMachindo = Color(108, 208, 255, 255);
	Color clrNumeni = Color(255, 208, 64, 255);
	Color clrCrosshair = (pPlayer->GetTeamNumber() == TEAM_NUMENI)?clrNumeni:clrMachindo;

	if (CHudTarget::Get() && CHudTarget::Get()->GetTarget() && CFGameRules()->PlayerRelationship(pPlayer, CHudTarget::Get()->GetTarget()) == GR_NOTTEAMMATE)
		clrCrosshair = Color(255, 0, 0, 255);

	// Grabbed some code from the wiki.
	Vector vecStart, vecStop, vecDirection, vecCrossPos;
	trace_t tr;

	AngleVectors(pPlayer->EyeAngles(), &vecDirection);

	vecStart= pPlayer->EyePosition();
	vecStop = vecStart + vecDirection * MAX_TRACE_LENGTH;

	UTIL_TraceLine( vecStart, vecStop, MASK_ALL, pPlayer, COLLISION_GROUP_NONE, &tr );

	if (input->CAM_IsThirdPerson() && !pPlayer->IsInFollowMode())
		ScreenTransform(tr.endpos, vecCrossPos);
	else
		vecCrossPos = Vector(0, 0, 0);

	int iX, iY, iDX, iDY, iCX, iCY;
	int iW, iH, iDW, iDH, iCW, iCH;

	// Crosshair size
	iW = iH = 24;
	iX = CFScreenWidth()/2+CFScreenWidth()/2*vecCrossPos[0]-iW/2;
	iY = CFScreenHeight()/2+(CFScreenHeight()/2*-vecCrossPos[1])-iH/2;

	// Disabled icon size
	iDW = iDH = 48;
	iDX = CFScreenWidth()/2+CFScreenWidth()/2*vecCrossPos[0]-iDW/2;
	iDY = CFScreenHeight()/2+(CFScreenHeight()/2*-vecCrossPos[1])-iDH/2;

	// Charge icon size
	iCW = 8;
	iCH = 64;
	iCX = CFScreenWidth()/2 + CFScreenWidth()/2*vecCrossPos[0] + iCH/2;
	iCY = CFScreenHeight()/2 + (CFScreenHeight()/2*-vecCrossPos[1]) - iCH/2;

	C_WeaponMagic* pMagic = pPlayer->GetMagicWeapon();
	if (pMagic && pMagic->IsCharging(false))
	{
		float flCharge = (gpGlobals->curtime - pMagic->GetChargeStartTime()) / cvar->FindVar("mp_magicchargetime")->GetFloat();

		int iMCX;
		if (pMagic->m_iChargeAttack == 0)
			iMCX = CFScreenWidth()/2 + CFScreenWidth()/2*vecCrossPos[0] - iCH/2 - iCW;
		else
			iMCX = CFScreenWidth()/2 + CFScreenWidth()/2*vecCrossPos[0] + iCH/2;

		if (flCharge)
		{
			if (flCharge >= 1)
			{
				flCharge = 1;
				if (!m_flFlash)
					m_flFlash = gpGlobals->curtime;
			}
			else
				m_flFlash = 0;

			float flAlpha;
			if (flCharge == 1)
			{
				float flRemap = fmod(gpGlobals->curtime - m_flFlash, 1);
				if (flRemap < 0.5f)
					flAlpha = RemapVal(flRemap, 0, 0.5f, 255/2, 0);
				else
					flAlpha = RemapVal(flRemap, 0.5f, 1, 0, 255/2);
			}
			else
				flAlpha = 255/2;

			CRootPanel::DrawRect(iMCX-2, iCY-2, iMCX+iCW+2, iCY+iCH+2, Color(0, 0, 0, 255));
			CRootPanel::DrawRect(iMCX, iCY + (1-flCharge)*iCH, iMCX+iCW, iCY+iCH, Color(255, 255, 255, flAlpha));
		}
	}

	if (pWeapon && pWeapon->IsMeleeWeapon())
	{
		CCFHudTexture* pAttackTexture = m_pCrosshairIcon;
		pAttackTexture->DrawSelf(iX, iY, iW, iH, clrCrosshair);

		if (pPlayer->GetPrimaryWeapon() && pPlayer->GetPrimaryWeapon()->IsDisableFlagged())
			m_pCrosshairDisabled->DrawSelf(iDX, iDY, iDW, iDH, Color(255, 0, 0, 255));

		bool bCanDownStrike = true;
		if (pPlayer->GetRecursedTarget())
		{
			Vector vecForward, vecTarget;
			vecTarget = pPlayer->GetRecursedTarget()->GetAbsOrigin() - pPlayer->GetAbsOrigin();
			vecTarget.z = 0;
			vecTarget.NormalizeInPlace();
			pPlayer->GetVectors(&vecForward, NULL, NULL);
			vecForward.z = 0;
			vecForward.NormalizeInPlace();
			bCanDownStrike = (DotProduct(vecForward, vecTarget) > 0.9f);
		}

		if (pPlayer->CanDownStrike(bCanDownStrike))
		{
			CCFHudTexture* pDFATexture;
			if (fmod(gpGlobals->curtime, 0.6f) < 0.3f)
				pDFATexture = m_pDFAUp;
			else
				pDFATexture = m_pDFADown;

			int iSize = 200;
			int iDropAmount = 200;
			int iDrop = Gain(RemapVal(fmod(gpGlobals->curtime, 0.6f), 0, 0.6f, 0, 1), 0.8f) * iDropAmount;
			pDFATexture->DrawSelf(CFScreenWidth()/2 + iSize/2, CFScreenHeight()/2 - iSize/2 - iDropAmount/2 + iDrop, iSize, iSize, clrCrosshair);
		}

		if (((CCFBaseMeleeWeapon*)pWeapon)->IsCharging())
		{
			float flCharge = (gpGlobals->curtime - pWeapon->GetChargeStartTime()) / cvar->FindVar("mp_chargetime")->GetFloat();

			if (flCharge)
			{
				if (flCharge >= 1)
				{
					flCharge = 1;
					if (!m_flFlash)
						m_flFlash = gpGlobals->curtime;
				}
				else
					m_flFlash = 0;

				float flAlpha;
				if (flCharge == 1)
				{
					float flRemap = fmod(gpGlobals->curtime - m_flFlash, 1);
					if (flRemap < 0.5f)
						flAlpha = RemapVal(flRemap, 0, 0.5f, 255/2, 0);
					else
						flAlpha = RemapVal(flRemap, 0.5f, 1, 0, 255/2);
				}
				else
					flAlpha = 255/2;

				CRootPanel::DrawRect(iCX-2, iCY-2, iCX+iCW+2, iCY+iCH+2, Color(0, 0, 0, 255));
				CRootPanel::DrawRect(iCX, iCY + (1-flCharge)*iCH, iCX+iCW, iCY+iCH, Color(255, 255, 255, flAlpha));
			}
		}

		if (pPlayer->GetEnemyFrozenUntil() > gpGlobals->curtime)
		{
			float flEnemyDisabled = (pPlayer->GetEnemyFrozenUntil() - gpGlobals->curtime) / cvar->FindVar("mp_chargetime")->GetFloat();
			int flRadius = flEnemyDisabled * CFScreenHeight()/4;
			Vector vecCirclePos;

			if (pPlayer->GetEnemyFrozen())
				ScreenTransform(pPlayer->GetEnemyFrozen()->WorldSpaceCenter(), vecCirclePos);
			else
				vecCirclePos = Vector(0, 0, 0);

			m_pTargetRing->DrawSelf(
				CFScreenWidth()/2 + CFScreenWidth()*vecCirclePos.x/2 - flRadius,
				CFScreenHeight()/2 - CFScreenHeight()*vecCirclePos.y/2 - flRadius,
				flRadius*2,
				flRadius*2,
				Color(255,
					RemapValClamped(flEnemyDisabled, 0.5f, 1, 0, 255),
					RemapValClamped(flEnemyDisabled, 0.5f, 1, 0, 255),
					RemapValClamped(flEnemyDisabled, 0.5f, 1, 120, 255))
			);
		}
	}
	else
	{
		m_pCrosshairIcon->DrawSelf( iX, iY, iW, iH, clrCrosshair );
	}
}

CDeathNotifier::CDeathNotifier()
	: CPanel(0, 0, 100, 100)
{
	CRootPanel::GetRoot()->AddControl(this);

	m_pText = new CLabel(0, 0, 100, 100, "");
	m_pText->SetAlign(CLabel::TA_CENTER);
	AddControl(m_pText);

	m_pRespawn = new CLabel(0, 0, 100, 100, "");
	m_pRespawn->SetAlign(CLabel::TA_CENTER);
	AddControl(m_pRespawn);

	Layout();
}

void CDeathNotifier::Layout()
{
	SetPos(100, CFScreenHeight()/2-120);
	SetSize(300, 180);

	m_pText->SetSize(300, BTN_HEIGHT*3);

	m_pRespawn->SetSize(300, BTN_HEIGHT);
	m_pRespawn->SetPos(0, 180-BTN_HEIGHT-BTN_BORDER);
	m_pRespawn->SetText("#Hint_Respawn");
}

void CDeathNotifier::Think()
{
	if (!C_BasePlayer::GetLocalPlayer())
		return;

	bool bVisible = false;
	C_CFPlayer *pPlayer = ToCFPlayer(C_BasePlayer::GetLocalPlayer());

	if (pPlayer->m_lifeState == LIFE_DYING)
		bVisible = true;

	// Show while doing fatalities.
	if (pPlayer->m_hReviving != NULL)
		bVisible = true;

	// Show while there is a respawn timer.
	if (gpGlobals->curtime < pPlayer->m_flNextRespawn)
		bVisible = true;

	if (CCFMenu::IsOpen())
		bVisible = false;

	SetVisible(bVisible);

	if (!bVisible)
		return;

	if (pPlayer->m_hReviving != NULL && !pPlayer->m_bReviving)
		m_pText->SetText(
				"#Finish_The_Job"
			);
	else if (pPlayer->m_hReviving != NULL && pPlayer->m_bReviving)
		m_pText->SetText(
				"#Reviving_Your_Teammate"
			);
	else if (pPlayer->m_hReviver != NULL && !pPlayer->m_bReviving)
		m_pText->SetText(
				"#Sucks_To_Be_You"
			);
	else if (pPlayer->m_hReviver != NULL && pPlayer->m_bReviving)
		m_pText->SetText(
				"#Help_Is_On_The_Way"
			);
	else if (gpGlobals->curtime < pPlayer->m_flNextRespawn)
	{
		m_pText->SetText("#Next_respawn");
		m_pText->AppendText(VarArgs(
				"%ds",
				(int)(pPlayer->m_flNextRespawn - gpGlobals->curtime)
			));
	}
	else
		m_pText->SetText(
				"#Incapacitated"
			);

	if (pPlayer->IsKnockedOut() && pPlayer->m_hReviver == NULL && gpGlobals->curtime > pPlayer->GetDeathTime() + mp_respawntimer.GetFloat())
		m_pRespawn->SetVisible(true);
	else
		m_pRespawn->SetVisible(false);
}

void CDeathNotifier::Paint(int x, int y, int w, int h)
{
	if (!IsVisible())
		return;

	C_CFPlayer *pPlayer = C_CFPlayer::GetLocalCFPlayer();
	
	CRootPanel::DrawRect(x, y, x+w, y+h, Color(0, 0, 0, 200));

	CPanel::Paint(x, y, w, h);

	if (pPlayer->m_hReviving != NULL)
	{
		CRootPanel::DrawRect(x+50, y+h/2+20, x+50+(gpGlobals->curtime-pPlayer->m_flFatalityStart)/FATALITY_TIME*200, y+h/2+30, Color(255, 255, 255, 200));
	}

	if (pPlayer->IsKnockedOut() && pPlayer->m_hReviver == NULL)
	{
		float flBonus = RemapValClamped(gpGlobals->curtime, pPlayer->GetDeathTime() + mp_respawntimer.GetFloat(), pPlayer->GetDeathTime() + mp_respawntimer.GetFloat() + mp_hpbonustime.GetFloat(), 0, 200);
		CRootPanel::DrawRect(x+50, y+45+24, x+50+flBonus, y+45+30, Color(0, 255, 0, 200));
		if (CHudHealth::Get())
			CHudHealth::Get()->SetBonusDimensions(x+50, y+h/2+20, flBonus, 10);
	}

	if (pPlayer->IsKnockedOut() && pPlayer->m_hReviver == NULL && gpGlobals->curtime > pPlayer->GetDeathTime() + mp_respawntimer.GetFloat())
	{
		const char* pKey = engine->Key_LookupBinding("+attack");
		CCFHudTexture* pRespawnKey = CTextureHandler::GetTextureFromKey(pKey);
		if (pRespawnKey)
			pRespawnKey->DrawSelf(x + 120, y + h - BTN_HEIGHT - 64, 64, 64, Color(255,255,255,255));
	}
}

void __MsgFunc_CFMessage( bf_read &msg )
{
	cfmessage_t cfmsg = (cfmessage_t)msg.ReadByte();

	int iParameter = 0;
	if (cfmsg == MESSAGE_MULTIPLIER)
	{
		iParameter = msg.ReadShort();
		CHUDMessages::Get()->Multiplier(iParameter);
	}
	else if (cfmsg == MESSAGE_CRITICAL)
	{
		CHUDMessages::Get()->Critical();
	}
	else if (cfmsg == MESSAGE_BONUSHEALTH)
	{
		iParameter = msg.ReadShort();
		CHudHealth::Get()->AnimateBonus(iParameter);
	}
	else if (cfmsg == MESSAGE_FLAGCAPTURED)
	{
		CHUDMessages::Get()->FlagCaptured();
	}
}

CHUDMessages* CHUDMessages::s_pHUDMessages = NULL;

CHUDMessages::CHUDMessages()
	: CPanel(0, 0, 100, 100)
{
	CRootPanel::GetRoot()->AddControl(this);

	Layout();

	Assert(!s_pHUDMessages);
	s_pHUDMessages = this;
}

void CHUDMessages::Destructor()
{
	Assert(s_pHUDMessages);
	s_pHUDMessages = NULL;
}

void CHUDMessages::LevelShutdown()
{
	m_flLastCritical = m_flLastMultiplier = 0;
	m_flLastFlagCap = 0;
}

void CHUDMessages::LoadTextures()
{
	m_pCritical		= GetHudTexture("critical");
	m_pOverdrive	= GetHudTexture("overdrive");
	m_pHit			= GetHudTexture("hit");
	m_pHitSpace		= GetHudTexture("hitspace");
	m_pHitX			= GetHudTexture("hitx");
	for (int i = 0; i < 10; i++)
		m_pHits[i]		= GetHudTexture(VarArgs("hit%d", i));
	m_pHitBang		= GetHudTexture("hitbang");
}

void CHUDMessages::Paint(int x, int y, int w, int h)
{
	if (!IsVisible())
		return;

	C_CFPlayer *pPlayer = C_CFPlayer::GetLocalCFPlayer();

	if (pPlayer->IsFirstPersonSpectating(pPlayer->GetObserverTarget()))
		pPlayer = ToCFPlayer(pPlayer->GetObserverTarget());

	if (m_pOverdrive && pPlayer && pPlayer->m_pStats->IsInOverdrive())
	{
		Color OverdriveColors[] =
		{
			Color(255,   0, 0,   255),
			Color(0,   255, 0,   255),
			Color(0,     0, 255, 255),
			Color(255, 255, 0,   255),
			Color(0,   255, 255, 255),
		};

		m_pOverdrive->DrawSelf(BTN_WIDTH, CFScreenHeight()/2 - m_pCritical->Height()/2 - m_pOverdrive->Height(), OverdriveColors[(int)(gpGlobals->curtime*5)%5]);
	}

	if (m_pCritical && gpGlobals->curtime < m_flLastCritical + 1)
	{
		m_pCritical->DrawSelf(BTN_WIDTH, CFScreenHeight()/2 - m_pCritical->Height()/2, Color(255, 255, 255, (m_flLastCritical - gpGlobals->curtime)*255));
	}

	if (m_pHit && gpGlobals->curtime < m_flLastMultiplier + 1)
	{
		int iY = CFScreenHeight()/2 + m_pCritical->Height()/2;
		int iX = BTN_WIDTH;
		int iAlpha = (m_flLastMultiplier - gpGlobals->curtime)*255;
		m_pHit->DrawSelf(iX, iY, Color(255, 255, 255, iAlpha));
		iX += m_pHit->Width();

		if (m_iLastMultiplier > 1)
		{
			m_pHitSpace->DrawSelf(iX, iY, Color(255, 255, 255, iAlpha));
			iX += m_pHitSpace->Width();

			m_pHitX->DrawSelf(iX, iY, Color(255, 255, 255, iAlpha));
			iX += m_pHitX->Width();

			int iDigits = 0;
			int iMult = m_iLastMultiplier;

			while(iMult > 0)
			{
				iDigits++;
				iMult/=10;
			}

			iMult = m_iLastMultiplier;
			while (iDigits > 0)
			{
				int iDigit = iMult/pow(10.0f, iDigits-1);

				m_pHits[iDigit]->DrawSelf(iX, iY, Color(255, 255, 255, iAlpha));
				iX += m_pHits[iDigit]->Width();

				iMult %= (int)pow(10.0f, iDigits-1);
				iDigits--;
			}
		}

		m_pHitBang->DrawSelf(iX, iY, Color(255, 255, 255, iAlpha));
		iX += m_pHitBang->Width();
	}
}

CHUDHints* CHUDHints::s_pHUDHints = NULL;

CHUDHints::CHUDHints()
	: CPanel(0, 0, 100, 100)
{
	CRootPanel::GetRoot()->AddControl(this);

	m_pText = new CLabel(0, 0, 100, 100, "");
	m_pText->SetAlign(CLabel::TA_CENTER);
	AddControl(m_pText);

	m_pFatality = new CLabel(0, 0, 100, 100, "");
	m_pFatality->SetAlign(CLabel::TA_CENTER);
	m_pFatality->SetWrap(false);
	AddControl(m_pFatality);

	m_pNumbers = new CLabel(0, 0, 100, 100, "");
	m_pNumbers->SetAlign(CLabel::TA_CENTER);
	m_pNumbers->SetWrap(false);
	AddControl(m_pNumbers);

	m_pInstructorText = new CLabel(0, 0, 100, 100, "");
	m_pInstructorText->SetAlign(CLabel::TA_CENTER);
	m_pInstructorText->SetWrap(false);
	AddControl(m_pInstructorText);

	SetVisible(true);

	Layout();

	m_flLastMessage = -1;
	m_flLastLesson = -1;

	m_pLesson = NULL;

	m_bShowingMagicMenuLesson = false;
	m_bShowingCollectAllLesson = false;

	Assert(!s_pHUDHints);
	s_pHUDHints = this;
}

void CHUDHints::Destructor()
{
	Assert(s_pHUDHints);
	s_pHUDHints = NULL;
}

void CHUDHints::LoadTextures()
{
	m_pInfoTexture	= GetHudTexture("hint_info");
}

void CHUDHints::LevelShutdown()
{
	m_flLastMessage = -1;
	m_flLastLesson = -1;
}

void CHUDHints::Layout()
{
	SetSize(m_pText->GetTextWidth()+BTN_BORDER*4, m_pText->GetTextHeight()+BTN_BORDER*4);
	SetPos(CFScreenWidth()-100-GetWidth(), CFScreenHeight()/2-GetHeight()/2);

	m_pText->SetSize(GetWidth(), GetHeight());
	m_pText->SetPos(0, 0);

	CPanel::Layout();
}

void CHUDHints::Think()
{
}

void CHUDHints::Paint(int x, int y, int w, int h)
{
	if (CCFMenu::IsOpen())
		return;

	if (CRoundVictoryPanel::IsOpen())
		return;

	PaintCenterLesson();
	PaintFatalityLesson();
	PaintMagicMenuLesson();
	PaintCaptureAllLesson();
}

void CHUDHints::PaintFatalityLesson()
{
	CCFPlayer* pLocalCFPlayer = CCFPlayer::GetLocalCFPlayer();

	bool bLearnFatalities = pLocalCFPlayer->Instructor_IsLessonValid(HINT_E_FATALITY);
	bool bLearnRevivals = pLocalCFPlayer->Instructor_IsLessonValid(HINT_E_REVIVE);

	if (!bLearnFatalities && !bLearnRevivals)
		return;

	for (int i = 1; i < gpGlobals->maxClients; i++)
	{
		CBasePlayer* pPlayer = UTIL_PlayerByIndex( i );

		if (!pPlayer)
			continue;

		CCFPlayer* pCFPlayer = ToCFPlayer(pPlayer);

		if (pCFPlayer == pLocalCFPlayer)
			continue;

		if (!pCFPlayer->IsKnockedOut())
			continue;

		if (pCFPlayer->m_hReviver != NULL)
			continue;

		bool bTeammate = (CFGameRules()->PlayerRelationship(pLocalCFPlayer, pCFPlayer) == GR_TEAMMATE);
		if (bTeammate && !bLearnRevivals || !bTeammate && !bLearnFatalities)
			continue;

		Vector vecCrossPos;
		ScreenTransform(pCFPlayer->WorldSpaceCenter(), vecCrossPos);

		int iX, iY, iW, iH;

		// Icon size
		iW = iH = 32;
		iX = CFScreenWidth()/2+CFScreenWidth()/2*vecCrossPos[0]-iW/2;
		iY = CFScreenHeight()/2+(CFScreenHeight()/2*-vecCrossPos[1])-iH/2;

		trace_t result;
		CTraceFilterNoNPCsOrPlayer traceFilter( NULL, COLLISION_GROUP_NONE );
		UTIL_TraceLine( pLocalCFPlayer->EyePosition(), pCFPlayer->WorldSpaceCenter(), MASK_VISIBLE_AND_NPCS, &traceFilter, &result );
		if (result.fraction != 1.0f)
		//if (!pPlayer->IsVisible(pCFTarget))	// This is unfortunately a server-only function, though I'd love to use it here.
			continue;

		int iAlpha = RemapValClamped((pLocalCFPlayer->EyePosition()-pCFPlayer->WorldSpaceCenter()).Length(), 100, 500, 255, 100);
		
		const char *useKey; 
		useKey = engine->Key_LookupBinding("+use");  
		CCFHudTexture* pUseTex = CTextureHandler::GetTextureFromKey(useKey);  

		if (pUseTex)
			pUseTex->DrawSelf(iX, iY, iW, iH, Color(255, 255, 255, iAlpha));

		if (bTeammate)
			m_pFatality->SetText("#Hint_Revive");
		else
			m_pFatality->SetText("#Hint_Fatality");

		m_pFatality->SetVisible(true);
		m_pFatality->SetFGColor(Color(255, 255, 255, iAlpha));
		m_pFatality->Paint(iX, iY + iH, iW, BTN_HEIGHT);
		m_pFatality->SetVisible(false);
	}
}

void CHUDHints::PaintMagicMenuLesson()
{
	CCFPlayer* pLocalCFPlayer = CCFPlayer::GetLocalCFPlayer();

	if (!pLocalCFPlayer->Instructor_IsLessonValid(HINT_NUMBERS_COMBOS))
	{
		m_bShowingMagicMenuLesson = false;
		return;
	}

	if (!m_bShowingMagicMenuLesson)
		m_flStartedShowingMagicMenuLesson = gpGlobals->curtime;

	m_bShowingMagicMenuLesson = true;

	int iInfoWidth = CFScreenWidth()/20;
	int iInfoStartX = CFScreenWidth()/2 - 50;
	int iInfoStartY = CFScreenHeight()/2;
	int iInfoX = 50;
	int iInfoY = CFScreenHeight() - BTN_HEIGHT*2 - iInfoWidth;

	float flTimeShowing = gpGlobals->curtime - m_flStartedShowingMagicMenuLesson;
	if (flTimeShowing < 1)
	{
		iInfoX = RemapValClamped(Gain(flTimeShowing, 0.8f), 0, 1.0f, iInfoStartX, iInfoX);
		iInfoY = RemapValClamped(Gain(flTimeShowing, 0.8f), 0, 1.0f, iInfoStartY, iInfoY);
	}

	if (CTextureHandler::GetTexture(KEY_NONE))
		CTextureHandler::GetTexture(KEY_NONE)->DrawSelf(iInfoX, iInfoY, iInfoWidth, iInfoWidth, Color(255, 255, 255, 255));

	m_pNumbers->SetText("#Hint_Numbers_Combos");

	int iTextWidth = m_pNumbers->GetTextWidth() / CFScreenWidthScale();

	CRootPanel::DrawRect(
		iInfoX + iInfoWidth/2 - iTextWidth/2 - 2,
		iInfoY + iInfoWidth,
		iInfoX + iInfoWidth/2 + iTextWidth/2 + 2,
		iInfoY + iInfoWidth + BTN_HEIGHT,
		Color(0, 0, 0, 200));

	m_pNumbers->SetVisible(true);
	m_pNumbers->SetFGColor(Color(255, 255, 255, 255));
	m_pNumbers->Paint(iInfoX, iInfoY + iInfoWidth, iInfoWidth, BTN_HEIGHT);
	m_pNumbers->SetVisible(false);
}

void CHUDHints::PaintCaptureAllLesson()
{
	CCFPlayer* pLocalCFPlayer = CCFPlayer::GetLocalCFPlayer();

	if (!pLocalCFPlayer->Instructor_IsLessonValid(HINT_COLLECT_ALL_5))
	{
		m_bShowingCollectAllLesson = false;
		return;
	}

	if (!m_bShowingCollectAllLesson)
		m_flStartedShowingCollectAllLesson = gpGlobals->curtime;

	m_bShowingCollectAllLesson = true;

	int iInfoWidth = CFScreenWidth()/20;
	int iInfoStartX = CFScreenWidth()/2 - iInfoWidth/2;
	int iInfoStartY = CFScreenHeight()/2;
	int iInfoX = CFScreenWidth()/2 - iInfoWidth/2;
	int iInfoY = 180;

	float flTimeShowing = gpGlobals->curtime - m_flStartedShowingCollectAllLesson;
	if (flTimeShowing < 1)
	{
		iInfoX = RemapValClamped(Gain(flTimeShowing, 0.8f), 0, 1.0f, iInfoStartX, iInfoX);
		iInfoY = RemapValClamped(Gain(flTimeShowing, 0.8f), 0, 1.0f, iInfoStartY, iInfoY);
	}

	if (CTextureHandler::GetTexture(KEY_NONE))
		CTextureHandler::GetTexture(KEY_NONE)->DrawSelf(iInfoX, iInfoY, iInfoWidth, iInfoWidth, Color(255, 255, 255, 255));

	m_pNumbers->SetText("#Hint_Collect_All_5");

	int iTextWidth = m_pNumbers->GetTextWidth() / CFScreenWidthScale();

	CRootPanel::DrawRect(
		iInfoX + iInfoWidth/2 - iTextWidth/2 - 2,
		iInfoY + iInfoWidth,
		iInfoX + iInfoWidth/2 + iTextWidth/2 + 2,
		iInfoY + iInfoWidth + BTN_HEIGHT,
		Color(0, 0, 0, 200));

	m_pNumbers->SetVisible(true);
	m_pNumbers->SetFGColor(Color(255, 255, 255, 255));
	m_pNumbers->Paint(iInfoX, iInfoY + iInfoWidth, iInfoWidth, BTN_HEIGHT);
	m_pNumbers->SetVisible(false);
}

void CHUDHints::PaintCenterLesson()
{
	// Draw the instructor's next lesson.
	if (!m_pLesson)
		return;

	if (m_flLastLesson < 0 || gpGlobals->curtime > m_flLastLesson + hint_time.GetFloat())
		return;

	CCFHudTexture* pInfoTexture = NULL;
	char* pszCommand = m_pLesson->m_pszCommand;
	if (pszCommand[0] == '!')
		pInfoTexture = (GetHudTexture(&pszCommand[1]) == NULL) ? m_pInfoTexture : GetHudTexture(&pszCommand[1]);
	else if (m_pLesson->m_iLessonType == CCFGameLesson::LESSON_INFO)
		pInfoTexture = m_pInfoTexture;
	else
	{
		const char *key = engine->Key_LookupBinding( *pszCommand == '+' ? pszCommand + 1 : pszCommand );
		pInfoTexture = CTextureHandler::GetTextureFromKey(key);  
	}

	int iMoveOverX = 0;
	if (CKillerInfo::IsOpen())
		iMoveOverX = CFScreenHeight()/2;

	int iInfoWidth = CFScreenWidth()/20;
	int iInfoX = CFScreenWidth()/2 - iInfoWidth/2 + iMoveOverX;
	int iInfoY = CFScreenHeight()*3/4 - iInfoWidth/2;

	int iTapY = 0;
	if (m_pLesson->m_bTapButton)
		iTapY += (fmod(gpGlobals->curtime, 0.5f) < 0.15)?5:0;

	float flTimeShown = RemapValClamped(gpGlobals->curtime - m_flLastLesson, 0, 0.5f, 0, 1);
	iInfoY = RemapValClamped(Bias(flTimeShown, 0.2f), 0, 1, CFScreenHeight(), iInfoY);

	pInfoTexture->DrawSelf(iInfoX, iInfoY+iTapY, iInfoWidth, iInfoWidth, Color(255,255,255,255));

	int iTextWidth = m_pInstructorText->GetTextWidth() / CFScreenWidthScale();
	int iTextHeight = m_pInstructorText->GetTextHeight() / CFScreenHeightScale();

	CRootPanel::DrawRect(
		CFScreenWidth()/2 - iTextWidth/2 - 2 + iMoveOverX,
		iInfoY + iInfoWidth + BTN_HEIGHT/2 - iTextHeight/2 - 2,
		CFScreenWidth()/2 + iTextWidth/2 + 2 + iMoveOverX,
		iInfoY + iInfoWidth + BTN_HEIGHT/2 + iTextHeight/2 + 2,
		Color(0, 0, 0, 200));

	m_pInstructorText->SetVisible(true);
	m_pInstructorText->Paint(iInfoX, iInfoY + iInfoWidth, iInfoWidth, BTN_HEIGHT);
	m_pInstructorText->SetVisible(false);
}

void CHUDHints::ProcessHint(CLabel* pLabel, const char* pszText)
{
	if (pszText[0] != '#')
	{
		pLabel->SetText(pszText);
	}
	else
	{
		wchar_t *pszLocalized = g_pVGuiLocalize->Find( pszText );

		if (!pszLocalized)
			pLabel->SetText(pszText);
		else
		{
			pLabel->SetText("");
			wchar_t *p = pszLocalized;

			while ( p )
			{
				wchar_t *line = p;
				wchar_t *end = wcschr( p, L'\n' );
				if ( end )
					p = end+1;
				else
					p = NULL;

				// copy to a new buf if there are vars
				wchar_t buf[512];
				buf[0] = '\0';
				int pos = 0;

				wchar_t *ws = line;
				while( ws != end && *ws != 0 )
				{
					// check for variables
					if ( *ws == '%' )
					{
						++ws;

						wchar_t *end = wcschr( ws, '%' );
						if ( end )
						{
							wchar_t token[64];
							wcsncpy( token, ws, end - ws );
							token[end - ws] = 0;

							ws += end - ws;

							// lookup key names
							char binding[64];
							g_pVGuiLocalize->ConvertUnicodeToANSI( token, binding, sizeof(binding) );

							const char *key = engine->Key_LookupBinding( *binding == '+' ? binding + 1 : binding );
							if ( !key )
							{
								key = "<unbound>";
							}

							//!! change some key names into better names
							char friendlyName[64];
							Q_snprintf( friendlyName, sizeof(friendlyName), "#%s", key );
							Q_strupr( friendlyName );

							wchar_t* pButton = g_pVGuiLocalize->Find( friendlyName );

							if (!pButton)
							{
								g_pVGuiLocalize->ConvertANSIToUnicode( friendlyName+1, token, sizeof(token) );
								pButton = token;
							}

							buf[pos] = '\0';
							wcscat( buf, pButton );
							pos += wcslen(pButton);
						}
						else
						{
							buf[pos] = *ws;
							++pos;
						}
					}
					else
					{
						buf[pos] = *ws;
						++pos;
					}

					++ws;
				}

				buf[pos] = '\0';

				pLabel->AppendText(buf);
				pLabel->AppendText(L"\n");
			}
		}
	}
}

void CHUDHints::Lesson(CCFGameLesson* pLesson)
{
	// If the menu is open and the lesson is being covered, we shouldn't count it as learned.
	if (CCFMenu::IsOpen())
		return;

	if (gpGlobals->curtime >= m_flLastLesson + hint_time.GetFloat())
	{
		if (!pLesson)
			return;

		m_pLesson = pLesson;
		m_flLastLesson = gpGlobals->curtime;

		ProcessHint(m_pInstructorText, g_pszHintMessages[pLesson->m_iHint]);

		C_CFPlayer *pLocalPlayer = C_CFPlayer::GetLocalCFPlayer();
		if (pLesson->m_iLearningMethod == CCFGameLesson::LEARN_DISPLAYING)
			pLocalPlayer->Instructor_LessonLearned(pLesson->m_iHint);

		Layout();
	}
}

CRank::CRank()
	: ICFHUD()
{
	m_pYourRank = new CLabel(0, 0, 100, 100, "#Your_Rank");
	m_pYourRank->SetAlign(CLabel::TA_CENTER);
	AddControl(m_pYourRank);

	m_pRankName = new CLabel(0, 0, 100, 100, "#Rank_Enlisted");
	m_pRankName->SetAlign(CLabel::TA_CENTER);
	AddControl(m_pRankName);

	m_iLastRank = 0;
	m_flLastRankChange = 0;
}

void CRank::LoadTextures()
{
	m_pCaptain = GetHudTexture("rankcaptain");
	m_pSergeant = GetHudTexture("ranksergeant");
	m_pPromoted = GetHudTexture("promoted");
}

void CRank::Layout()
{
	m_pYourRank->SetSize(BTN_WIDTH, BTN_HEIGHT);
	m_pYourRank->SetPos(GetWidth()-BTN_WIDTH, 0);

	m_pRankName->SetSize(BTN_WIDTH, BTN_HEIGHT);
	m_pRankName->SetPos(GetWidth()-BTN_WIDTH, BTN_HEIGHT+BTN_WIDTH);

	ICFHUD::Layout();
}

void CRank::Think()
{
	if (!C_BasePlayer::GetLocalPlayer())
		return;

	CCFPlayer* pLocalCFPlayer = CCFPlayer::GetLocalCFPlayer();

	if (!pLocalCFPlayer)
		return;

	if (pLocalCFPlayer->IsFirstPersonSpectating(pLocalCFPlayer->GetObserverTarget()))
		pLocalCFPlayer = ToCFPlayer(pLocalCFPlayer->GetObserverTarget());

	int iRank = (pLocalCFPlayer->IsCaptain()<<0) | (pLocalCFPlayer->IsSergeant()<<1);

	if (iRank != m_iLastRank)
	{
		m_iLastRank = iRank;
		m_flLastRankChange = gpGlobals->curtime;
		if (pLocalCFPlayer->IsCaptain())
			m_pRankName->SetText("#Rank_Captain");
		else if (pLocalCFPlayer->IsSergeant())
			m_pRankName->SetText("#Rank_Sergeant");
		else
			m_pRankName->SetText("#Rank_Enlisted");
	}

	ICFHUD::Think();
}

void CRank::Paint(int x, int y, int w, int h)
{
	if (!C_BasePlayer::GetLocalPlayer())
		return;

	CCFPlayer* pLocalCFPlayer = CCFPlayer::GetLocalCFPlayer();
	if (!pLocalCFPlayer)
		return;

	if (pLocalCFPlayer->IsFirstPersonSpectating(pLocalCFPlayer->GetObserverTarget()))
		pLocalCFPlayer = ToCFPlayer(pLocalCFPlayer->GetObserverTarget());

	// We don't care if we're not an officer.
	if (!pLocalCFPlayer->IsCaptain() && !pLocalCFPlayer->IsSergeant())
		return;

	ICFHUD::Paint(x, y, w, h);

	CCFHudTexture* pTexture;
	if (pLocalCFPlayer->IsCaptain())
		pTexture = m_pCaptain;
	else
		pTexture = m_pSergeant;

	if (!pTexture)
		return;

	int iLargeWidth = CFScreenWidth()/4;
	int iSmallWidth = BTN_WIDTH;
	int iLargeX = GetWidth()-iLargeWidth-iSmallWidth;
	int iLargeY = BTN_HEIGHT;
	int iSmallX = GetWidth()-iSmallWidth;
	int iSmallY = BTN_HEIGHT;
	float flTime = gpGlobals->curtime - m_flLastRankChange;
	float flFreezeTime = 1.0f;
	float flTransitionTime = 0.4f;

	if (flTime < flFreezeTime)
	{
		float flAlpha = RemapValClamped(flTime, 0, flFreezeTime, 0, 255);

		pTexture->DrawSelf(iLargeX, iLargeY, iLargeWidth, iLargeWidth, Color(255,255,255,flAlpha));
	}
	else if (flTime < flFreezeTime + flTransitionTime)
	{
		flTime -= flFreezeTime;
		float flX = RemapVal(flTime, 0, flTransitionTime, iLargeX, iSmallX);
		float flY = RemapVal(flTime, 0, flTransitionTime, iLargeY, iSmallY);
		float flW = RemapVal(flTime, 0, flTransitionTime, iLargeWidth, iSmallWidth);
		pTexture->DrawSelf(flX, flY, flW, flW, Color(255,255,255,255));
	}
	else
	{
		pTexture->DrawSelf(iSmallX, iSmallY, iSmallWidth, iSmallWidth, Color(255,255,255,255));
	}
}

CTextureHandler g_TextureHandler;

// A tiny helper function.
void tolower(const char* s, char* b)
{
	int l = strlen(s);
	for (int i = 0; i < l; i++)
		b[i] = (s[i]>='A' && s[i]<='Z') ? s[i]-('A'-'a') : s[i];
	b[l] = '\0';
}

static bool ButtonCode_LessFunc( ButtonCode_t const &a, ButtonCode_t const &b )
{
	return a < b;
}

void CTextureHandler::LoadTextures()
{
	int iIndex = 0;
	unsigned int iUIndex = 0;

	if (!Get()->m_apRuneTextures.Count())
	{
		Get()->m_pRuneSlotTexture = GetHudTexture("runeslot");

		char szLower[256];
		CRuneData *pRune = NULL;
		int i = -1;
		while ((pRune = CRuneData::GetData((RuneID)++i)) != NULL)
		{
			CRuneTexture* pTexture = new CRuneTexture();
			pTexture->m_eRuneType = pRune->m_eType;

			tolower(pRune->m_szName, szLower);
			pTexture->m_pTexture = GetHudTexture(VarArgs("r%s", szLower));

			iIndex = Get()->m_apRuneTextures.AddToTail(pTexture);
			Assert(iIndex == i);
		}
	}

	if (!Get()->m_apWeaponTextures.Count())
	{
		// One fake entry for WEAPON_NONE.
		Get()->m_apWeaponTextures.AddToTail(NULL);

		// WEAPON_MAGIC is the highlight.
		for (int i = 1; i < WEAPON_MAX; i++)
		{
			iIndex = Get()->m_apWeaponTextures.AddToTail(GetHudTexture(VarArgs("weaponicon%d", i)));
			Assert(iIndex == i);
		}
	}
	if (!Get()->m_apWeaponTexturesRadial.Count())
	{
		// One fake entry for WEAPON_NONE.
		Get()->m_apWeaponTexturesRadial.AddToTail(NULL);

		// WEAPON_MAGIC is the highlight.
		for (int i = 1; i < WEAPON_MAX; i++)
		{
			iIndex = Get()->m_apWeaponTexturesRadial.AddToTail(GetHudTexture(VarArgs("weaponicon%d_r", i)));
			Assert(iIndex == i);
		}
	}

	if (!Get()->m_apWeaponTexturesWide.Count())
	{
		// One fake entry for WEAPON_NONE.
		Get()->m_apWeaponTexturesWide.AddToTail(NULL);

		// WEAPON_MAGIC is the highlight.
		for (int i = 1; i < WEAPON_MAX; i++)
		{
			iIndex = Get()->m_apWeaponTexturesWide.AddToTail(GetHudTexture(VarArgs("weaponicon%d_w", i)));
			Assert(iIndex == i);
		}
	}

	if (!Get()->m_apWeaponTexturesHighlight.Count())
	{
		// One fake entry for WEAPON_NONE.
		Get()->m_apWeaponTexturesHighlight.AddToTail(NULL);

		// WEAPON_MAGIC is the highlight.
		for (int i = 1; i < WEAPON_MAX; i++)
		{
			iIndex = Get()->m_apWeaponTexturesHighlight.AddToTail(GetHudTexture(VarArgs("weaponicon%d_h", i)));
			Assert(iIndex == i);
		}
	}

	if (!Get()->m_apArmorTextures.Count() && CArmamentData::TotalArmaments())
	{
		for (unsigned int i = 0; i < CArmamentData::TotalArmaments(); i++)
		{
			iUIndex = Get()->m_apArmorTextures.AddToTail(GetHudTexture(VarArgs("armoricon%d", i+1)));
			Assert(iUIndex == i);
		}
	}

	// m_apStatusIcons is a C vector, not a CUtlVector, so we don't need to worry about clobbering it.
	for (int i = 0; i < TOTAL_STATUSEFFECTS; i++)
	{
		if (CRuneData::IsStatusEffectPossible((statuseffect_t)(1<<i)))
			Get()->m_apStatusIcons[i] = GetHudTexture(VarArgs("status_%s", StatusEffectToString((statuseffect_t)(1<<i))));
		else
			Get()->m_apStatusIcons[i] = NULL;
	}

	for (int i = 0; i < MAX_FORCEEFFECTS; i++)
	{
		if (CRuneData::IsForceEffectPossible((forceeffect_t)i))
			Get()->m_apForceIcons[i] = GetHudTexture(VarArgs("force_%s", CRuneData::ForceEffectToString((forceeffect_t)i)));
		else
			Get()->m_apForceIcons[i] = NULL;
	}

	if (!Get()->m_apKeyTextures.Count())
	{
		Get()->m_apKeyTextures.SetLessFunc(&ButtonCode_LessFunc);

		Get()->m_pInfoTexture = GetHudTexture("hint_info");

		Get()->m_apKeyTextures.Insert(MOUSE_LEFT, GetHudTexture("mousebuttons_lmb"));
		Get()->m_apKeyTextures.Insert(MOUSE_RIGHT, GetHudTexture("mousebuttons_rmb"));
		Get()->m_apKeyTextures.Insert(MOUSE_4, GetHudTexture("mousebuttons_shoulder"));
		Get()->m_apKeyTextures.Insert(MOUSE_5, GetHudTexture("mousebuttons_shoulder"));
		Get()->m_apKeyTextures.Insert(MOUSE_WHEEL_UP, GetHudTexture("mousebuttons_scroll"));
		Get()->m_apKeyTextures.Insert(MOUSE_WHEEL_DOWN, GetHudTexture("mousebuttons_scroll"));

		Get()->m_apKeyTextures.Insert(KEY_SPACE, GetHudTexture("keyboard_spacebar"));
		Get()->m_apKeyTextures.Insert(KEY_LSHIFT, GetHudTexture("keyboard_shift"));

		for (char c = 'a'; c <= 'z'; c++)
		{
			char* pszKey = VarArgs("keyboard_%c", c);
			if (GetHudTexture(pszKey))
				Get()->m_apKeyTextures.Insert((ButtonCode_t)(KEY_A+c-'a'), GetHudTexture(pszKey));
		}
	}
}

CCFHudTexture* CTextureHandler::GetTexture(CFWeaponID eWeapon, TextureType eType)
{
	if (eWeapon == WEAPON_NONE)
		return NULL;

	switch(eType)
	{
	case TT_SQUARE:
	default:
		Assert(Get() && Get()->m_apWeaponTextures.IsValidIndex(eWeapon));
		Assert(Get()->m_apWeaponTextures[eWeapon]);
		return Get()->m_apWeaponTextures[eWeapon];

	case TT_WIDE:
		Assert(Get() && Get()->m_apWeaponTexturesWide.IsValidIndex(eWeapon));
		Assert(Get()->m_apWeaponTexturesWide[eWeapon]);
		return Get()->m_apWeaponTexturesWide[eWeapon];
	
	case TT_RADIAL:
		Assert(Get() && Get()->m_apWeaponTexturesRadial.IsValidIndex(eWeapon));		
		Assert(Get()->m_apWeaponTexturesRadial[eWeapon]);
		return Get()->m_apWeaponTexturesRadial[eWeapon];

	case TT_HIGHLIGHT:
		Assert(Get() && Get()->m_apWeaponTexturesHighlight.IsValidIndex(eWeapon));
		Assert(Get()->m_apWeaponTexturesHighlight[eWeapon]);
		return Get()->m_apWeaponTexturesHighlight[eWeapon];
	}
}

CCFHudTexture* CTextureHandler::GetTexture(ArmamentID eArmor, TextureType eType)
{
	// Maybe replace this when armaments and runes have their own highlights.
	if (eType == TT_HIGHLIGHT)
		// WEAPON_MAGIC is the highlight.
		return GetTexture(WEAPON_MAGIC);

	Assert(Get() && Get()->m_apArmorTextures.IsValidIndex(eArmor));
	Assert(Get()->m_apArmorTextures[eArmor]);
	return Get()->m_apArmorTextures[eArmor];
}

CCFHudTexture* CTextureHandler::GetTexture(RuneID eWeapon, TextureType eType)
{
	// Maybe replace this when armaments and runes have their own highlights.
	if (eType == TT_HIGHLIGHT)
		// WEAPON_MAGIC is the highlight.
		return GetTexture(WEAPON_MAGIC);

	Assert(Get() && Get()->m_apRuneTextures.IsValidIndex(eWeapon));
	Assert(Get()->m_apRuneTextures[eWeapon]->m_pTexture);
	return Get()->m_apRuneTextures[eWeapon]->m_pTexture;
}

CCFHudTexture* CTextureHandler::GetTexture(statuseffect_t eStatus)
{
	Assert(!(eStatus & (eStatus-1)));	// Only one bit please.

	int iStatus = 0;

	// I hate this but I can't figure out the real way of doing it.
	for( int i = 0; i < sizeof(eStatus); i++)
	{
		if (eStatus>>i == 1)
		{
			iStatus = i;
			break;
		}
	}

	Assert(Get() && Get()->m_apStatusIcons[iStatus]);
	return Get()->m_apStatusIcons[iStatus];
}

CCFHudTexture* CTextureHandler::GetTexture(forceeffect_t eForce)
{
	Assert(Get() && Get()->m_apForceIcons[eForce]);
	return Get()->m_apForceIcons[eForce];
}

CCFHudTexture* CTextureHandler::GetTexture(ButtonCode_t eButton)
{
	Assert(Get());

	if (!Get()->m_apKeyTextures.IsValidIndex(Get()->m_apKeyTextures.Find(eButton)))
		return Get()->m_pInfoTexture;

	return Get()->m_apKeyTextures[Get()->m_apKeyTextures.Find(eButton)];
}

CCFHudTexture* CTextureHandler::GetTextureFromKey(const char * eKey)
{
	CCFHudTexture* pInfoTexture;
	Assert(Get());

	if (!eKey)
		return Get()->m_pInfoTexture;

	else
	{
		ButtonCode_t iButton;
		if (Q_strcmp(eKey, "MWHEELUP") == 0)
			iButton = MOUSE_WHEEL_UP;
		else if (Q_strcmp(eKey, "MWHEELDOWN") == 0)
			iButton = MOUSE_WHEEL_DOWN;
		else if (Q_strncmp(eKey, "MOUSE", 5) == 0)
			iButton = (ButtonCode_t)(MOUSE_LEFT + eKey[5] - '1');
		else if (Q_strcmp(eKey, "SPACE") == 0)
			iButton = KEY_SPACE;
		else if (Q_strcmp(eKey, "SHIFT") == 0)
			iButton = KEY_LSHIFT;
		else if (isalpha(eKey[0]))
			iButton = (ButtonCode_t)(KEY_A + tolower(eKey[0]) - 'a');
		else
			iButton = (ButtonCode_t)(KEY_1 + eKey[0] - '0');

		return pInfoTexture = CTextureHandler::GetTexture(iButton);
	}
}