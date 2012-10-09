#include "cbase.h"
#include "cfui_menu.h"
#include "weapon_magic.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CCFHudTexture* CCharPanel::s_pNYellow = NULL;
CCFHudTexture* CCharPanel::s_pNBeige = NULL;
CCFHudTexture* CCharPanel::s_pNRed = NULL;
CCFHudTexture* CCharPanel::s_pNOrange = NULL;

CCFHudTexture* CCharPanel::s_pMBlue = NULL;
CCFHudTexture* CCharPanel::s_pMTeal = NULL;
CCFHudTexture* CCharPanel::s_pMGreen = NULL;
CCFHudTexture* CCharPanel::s_pMPurple = NULL;

CCharPanel::CCharPanel()
	: CPanel(0, BTN_HEIGHT + BTN_BORDER, MENU_WIDTH, PANEL_HEIGHT)
{
	m_pChoose = new CLabel(3, 3, PANEL_WIDTH - 6, BTN_HEIGHT, "#Choose_Config");

	AddControl(m_pChoose);

	int iConfigsHeight = PANEL_HEIGHT - BTN_HEIGHT - BTN_BORDER*3;
	int iConfigsWidth = BTN_WIDTH*3 + BTN_BORDER*2;
	int iInfoHeight = PANEL_HEIGHT - BTN_HEIGHT - BTN_BORDER*3;
	int iInfoWidth = PANEL_WIDTH - iConfigsWidth - BTN_BORDER*3;

	m_pConfigs	= new CPanel(BTN_BORDER, BTN_HEIGHT + BTN_BORDER*2, iConfigsWidth, iConfigsHeight);
	m_pInfo		= new CPanel(BTN_BORDER + iConfigsWidth + BTN_BORDER, BTN_HEIGHT + BTN_BORDER*2, iInfoWidth, iInfoHeight);
	AddControl(m_pConfigs);
	AddControl(m_pInfo);

	m_pDescPanel = new CPanel(BTN_BORDER, BTN_BORDER, 100, 100);
	m_pInfo->AddControl(m_pDescPanel);

	m_pDesc = new CConfigDescription();
	m_pDescPanel->AddControl(m_pDesc);

	m_pAbilitiesPanel = new CAbilitiesPanel();
	AddControl(m_pAbilitiesPanel);

	m_pStatsPanel = new CPanel(BTN_BORDER, BTN_BORDER, 100, 100);
	AddControl(m_pStatsPanel);

	m_pColors = new CPanel(0,0,100,100);
	AddControl(m_pColors);

	m_apDifficultyButtons.AddToTail(new CButton(0,0,BTN_WIDTH,BTN_HEIGHT, "#Basic", true));
	m_apDifficultyButtons.AddToTail(new CButton(0,0,BTN_WIDTH,BTN_HEIGHT, "#Medium", true));
	m_apDifficultyButtons.AddToTail(new CButton(0,0,BTN_WIDTH,BTN_HEIGHT, "#Advanced", true));
	m_apDifficultyButtons.AddToTail(new CButton(0,0,BTN_WIDTH,BTN_HEIGHT, "#Custom", true));

	for (int i = 0; i < m_apDifficultyButtons.Count(); i++)
		AddControl(m_apDifficultyButtons[i]);

	m_apNumeniColors.AddToTail(s_pNBeige);
	m_apNumeniColors.AddToTail(s_pNRed);
	m_apNumeniColors.AddToTail(s_pNYellow);
	m_apNumeniColors.AddToTail(s_pNOrange);

	m_apMachindoColors.AddToTail(s_pMBlue);
	m_apMachindoColors.AddToTail(s_pMGreen);
	m_apMachindoColors.AddToTail(s_pMTeal);
	m_apMachindoColors.AddToTail(s_pMPurple);

	// Pick a random color for now... store it later on.
	m_iColor = random->RandomInt(0, min(m_apNumeniColors.Count()-1, m_apMachindoColors.Count()-1));
	m_iCategory = 0;

	engine->ClientCmd(VarArgs("choosecolor %i", m_iColor));

	CConfigMgr::Get()->LoadArmaments();

	m_pPreviewArmament = NULL;
}

void CCharPanel::Layout()
{
	int iConfigsWidth = GetWidth();
	int iConfigsHeight = GetWidth()/6;

	int iColorButtonWidth = 25;
	int iColorsWidth = iColorButtonWidth*4 + BTN_BORDER*5;
	m_pColors->SetPos(m_pInfo->GetWidth()*2/3 - iColorsWidth + 15, GetHeight()*2/3 - BTN_HEIGHT*2 - BTN_HEIGHT);
	m_pColors->SetSize(iColorsWidth, iColorButtonWidth + BTN_BORDER*2);
	m_pColors->SetBorder(BT_NONE);

	m_pConfigs->SetPos(0, GetHeight()*2/3 - BTN_HEIGHT);
	m_pConfigs->SetSize(iConfigsWidth, iConfigsHeight);
	m_pConfigs->SetBorder(BT_NONE);
	m_pInfo->SetPos(BTN_BORDER, BTN_BORDER);
	m_pInfo->SetSize(GetWidth()-BTN_BORDER*2, GetHeight()-BTN_BORDER*2 - BTN_HEIGHT);
	m_pInfo->SetBorder(BT_NONE);

	m_pAbilitiesPanel->Layout();

	m_pStatsPanel->SetSize(GetWidth()/6, GetHeight()/8);
	m_pStatsPanel->SetPos(GetWidth()/8, GetHeight()/3);
	m_pStatsPanel->SetBorder(BT_SOME);

	int iDescPanelBorder = 20;

	m_pDescPanel->SetPos(m_pInfo->GetWidth()*2/3 + iDescPanelBorder, iDescPanelBorder);
	m_pDescPanel->SetSize(GetWidth()/3 - iDescPanelBorder*2, m_pInfo->GetHeight()*2/3 - iDescPanelBorder*2);
	m_pDesc->SetPos(BTN_BORDER, BTN_BORDER);
	m_pDesc->SetSize(m_pDescPanel->GetWidth() - BTN_BORDER*2, m_pDescPanel->GetHeight() - BTN_BORDER*2 - BTN_HEIGHT);
	m_pDesc->SetAlign(CLabel::TA_TOPLEFT);

	CCFPlayer* pPlayer = C_CFPlayer::GetLocalCFPlayer();

	while (m_apColorButtons.Count())
	{
		m_apColorButtons[0]->Destructor();
		m_apColorButtons[0]->Delete();
		m_apColorButtons.Remove(0);
	}

	for (int i = 0; i < 4; i++)
	{
		if (!pPlayer)
			continue;

		CColorChoice* pColorButton;
		switch (pPlayer->GetTeamNumber())
		{
		case TEAM_NUMENI:
			pColorButton = new CColorChoice(m_apNumeniColors[i]);
			break;

		case TEAM_MACHINDO:
			pColorButton = new CColorChoice(m_apMachindoColors[i]);
			break;

		default:
			continue;
		}

		m_apColorButtons.AddToTail(pColorButton);
		m_pColors->AddControl(pColorButton);
		pColorButton->SetSize(iColorButtonWidth, iColorButtonWidth);
		pColorButton->SetPos(BTN_BORDER*(i+1) + iColorButtonWidth*i, BTN_BORDER);
		pColorButton->SetClickedListener(this, ChooseColor, new KeyValues("choose", "color", i));
		pColorButton->SetUnclickedListener(this, ChooseColor, new KeyValues("choose", "color", i));
		pColorButton->SetState(m_iColor == i, false);
	}

	while (m_apButtons.Count())
	{
		m_apButtons[0]->Destructor();
		m_apButtons[0]->Delete();
		m_apButtons.Remove(0);
	}

	while (m_apEditButtons.Count())
	{
		m_apEditButtons[0]->Destructor();
		m_apEditButtons[0]->Delete();
		m_apEditButtons.Remove(0);
	}

	CConfigMgr::Get()->LoadArmaments();

	CUtlVector<CArmament*>* pArmaments;
	
	switch (m_iCategory)
	{
	case 0:
		pArmaments = CConfigMgr::Get()->GetArmamentList(CConfigMgr::CFG_BASIC);
		break;
	case 1:
		pArmaments = CConfigMgr::Get()->GetArmamentList(CConfigMgr::CFG_MEDIUM);
		break;
	case 2:
		pArmaments = CConfigMgr::Get()->GetArmamentList(CConfigMgr::CFG_ADVANCED);
		break;
	case 3:
	default:
		pArmaments = CConfigMgr::Get()->GetArmamentList(CConfigMgr::CFG_CUSTOM);
		break;
	}

	int iWidth = (m_pConfigs->GetWidth()-BTN_BORDER*6)/6;
	int iHeight = iWidth;

	int iCount = min(pArmaments->Count(), 6);
	for (int i = 0; i < iCount; i++)
	{
		CConfigChoice* pButton = new CConfigChoice(pArmaments->Element(i));
		pButton->SetImage(pArmaments->Element(i)->m_szImage);

		m_apButtons.AddToTail(pButton);
		m_pConfigs->AddControl(pButton, true);
		pButton->SetDimensions(BTN_BORDER + (iWidth*i + BTN_BORDER*i), 0, iWidth, iHeight);
		pButton->SetClickedListener(this, ChooseArmament, new KeyValues("choose", "armament", i));
		pButton->SetHighlightListener(this);
		pButton->Layout();

		CButton* pEditButton = new CButton(0,0,BTN_WIDTH,BTN_HEIGHT,"#Edit");
		m_apEditButtons.AddToTail(pEditButton);
		m_pConfigs->AddControl(pEditButton, true);
		pEditButton->SetSize(pEditButton->GetTextWidth()+4, pEditButton->GetTextHeight()+4);
		pEditButton->SetPos(BTN_BORDER + (iWidth*i + BTN_BORDER*i) + pButton->GetWidth() - pEditButton->GetWidth() - BTN_BORDER, pButton->GetHeight() - pEditButton->GetHeight() - BTN_BORDER);
		pEditButton->SetClickedListener(this, EditArmament, new KeyValues("edit", "armament", i));
		pEditButton->SetWrap(false);
	}

	int iDifficultyWidth = BTN_WIDTH*4 + BTN_BORDER*3;
	for (int i = 0; i < m_apDifficultyButtons.Count(); i++)
	{
		CButton* pButton = m_apDifficultyButtons[i];
		pButton->SetPos(GetWidth()/2 - iDifficultyWidth/2 + BTN_WIDTH*i + BTN_BORDER*i, GetHeight() - BTN_HEIGHT);
		pButton->SetState(i == m_iCategory, false);
		pButton->SetClickedListener(this, ChooseCategory, new KeyValues("choose", "category", i));
		pButton->SetUnclickedListener(this, ChooseCategory, new KeyValues("choose", "category", i));
	}
}

void CCharPanel::Paint(int x, int y, int w, int h)
{
	int iComboX = x + GetWidth()/3 + 64;
	int iComboY = y + GetHeight()*2/3 - BTN_BORDER - BTN_HEIGHT;

	if (m_pPreviewArmament)
	{
		int i;
		for (i = m_pPreviewArmament->GetRuneCombos()-1; i >= 0; i--)
		{
			CRunePosition* pCombo = m_pPreviewArmament->GetRuneCombo(i);
			CRuneComboIcon::Paint(m_pPreviewArmament->SerializeCombo(pCombo->m_iWeapon, pCombo->m_iRune), -1,
				iComboX - 64,
				iComboY - 64*(i+1),
				64, 64);
		}

		int iDrawn = 0;
		statuseffect_t eEffects = StatusEffectForElement(m_pPreviewArmament->GetElementsDefended());
		if (eEffects)
		{
			CCFHudTexture* pTexture = GetHudTexture("force_defend");
			if (pTexture)
				pTexture->DrawSelf(
					iComboX - 128,
					iComboY - 64*(iDrawn+1),
					64, 64, Color(255,255,255,255));
			CRuneComboIcon::PaintInfuseIcons(eEffects, 3,
				iComboX - 128,
				iComboY - 64*(iDrawn+1),
				64, 64);
			iDrawn++;
		}

		for (i = 4; i >= 0; i--)
		{
			eEffects = m_pPreviewArmament->GetPhysicalAttackStatusEffect(i);
			if (eEffects)
			{
				CCFHudTexture* pWeapon = CTextureHandler::GetTexture(m_pPreviewArmament->m_aWeapons[i].m_iWeapon);
				if (pWeapon)
					pWeapon->DrawSelf(
						iComboX - 128,
						iComboY - 64*(iDrawn+1),
						64, 64, Color(255,255,255,255));
				CRuneComboIcon::PaintInfuseIcons(eEffects, -1,
					iComboX - 128,
					iComboY - 64*(iDrawn+1),
					64, 64);
				iDrawn++;
			}
		}
	}

	CPanel::Paint(x, y, w, h);
}

void CCharPanel::Highlighted(CArmament* pArm)
{
	m_pDesc->SetActiveArmament(pArm);
	SetPreviewArmament(pArm);
}

void CCharPanel::SetPreviewArmament(CArmament* pArmament)
{
	if (pArmament)
	{
		m_pPreviewArmament = pArmament;
		m_pAbilitiesPanel->SetArmament(pArmament);
	}
}

void CCharPanel::LevelShutdown()
{
	m_pPreviewArmament = NULL;

	for (int i = 0; i < m_apButtons.Count(); i++)
		m_apButtons[i]->SetArmament(NULL);

	CPanel::LevelShutdown();
}

void CCharPanel::PostRenderVGui()
{
	if (!IsVisible())
		return;

	// A bit of a hack, but whatever. The config save menu covers this area.
	if (CConfigsPanel::IsOpen())
		return;

	RenderPlayerModel();
}

void CCharPanel::RenderPlayerModel()
{
	CCFPlayer* pLocalPlayer = C_CFPlayer::GetLocalCFPlayer();

	int iTeam = pLocalPlayer->GetTeamNumber();
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

	m_pPreviewPlayer.SetModelName(pszModelName);

	CFWeaponID ePrimary = WEAPON_NONE;
	CFWeaponID eSecondary = WEAPON_NONE;
	CRunePosition* pPrimaryCombo = NULL;
	CRunePosition* pSecondaryCombo = NULL;

	if (m_pPreviewArmament)
	{
		ePrimary = m_pPreviewArmament->m_aWeapons[0].m_iWeapon;
		eSecondary = m_pPreviewArmament->m_aWeapons[1].m_iWeapon;
		pPrimaryCombo = &m_pPreviewArmament->m_aAttackBinds[0];
		pSecondaryCombo = &m_pPreviewArmament->m_aAttackBinds[1];
	}

	Activity eTranslated = ACT_MP_STAND_IDLE;
	if (ePrimary == WEAPON_NONE && eSecondary == WEAPON_NONE && (pPrimaryCombo && pPrimaryCombo->m_iWeapon >= 0 || pSecondaryCombo && pSecondaryCombo->m_iWeapon >= 0))
		eTranslated = CWeaponMagic::ActivityOverride(eTranslated, false);
	else if (ePrimary != WEAPON_NONE)
		eTranslated = CWeaponCFBase::ActivityOverride(ePrimary, CWeaponCFBase::GetWeaponType(eSecondary), ACT_MP_STAND_IDLE, NULL);
	else if (eSecondary != WEAPON_NONE)
		eTranslated = CWeaponCFBase::ActivityOverride(eSecondary, CWeaponCFBase::GetWeaponType(WEAPON_NONE), ACT_MP_STAND_IDLE, NULL);

	m_pPreviewPlayer.SetActivity(eTranslated);

	m_pPreviewPlayer.SetPrimaryWeapon(ePrimary);
	m_pPreviewPlayer.SetSecondaryWeapon(eSecondary);

	m_pPreviewPlayer.SetSkin(m_iColor);

	int iInfoX, iInfoY;
	m_pInfo->GetAbsPos(iInfoX, iInfoY);

	int iConfigsX, iConfigsY;
	m_pConfigs->GetAbsPos(iConfigsX, iConfigsY);

	int iPreviewWidth = CFScreenWidth()/3;
	int iPreviewHeight = iConfigsY - iInfoY - BTN_BORDER;

	element_t eLHEffectElements = ELEMENT_TYPELESS;
	element_t eRHEffectElements = ELEMENT_TYPELESS;
	if (m_pPreviewArmament && ePrimary == WEAPON_NONE && eSecondary == WEAPON_NONE)
	{
		if (pPrimaryCombo->m_iWeapon >= 0)
			eLHEffectElements = m_pPreviewArmament->GetMagicalAttackElement(pPrimaryCombo);

		if (pSecondaryCombo->m_iWeapon >= 0)
			eRHEffectElements = m_pPreviewArmament->GetMagicalAttackElement(pSecondaryCombo);
	}

	// Hand magic effects!
	if (m_pPreviewPlayer.GetModel() && eLHEffectElements != m_eLHEffectElements)
	{
		m_pPreviewPlayer.RemoveParticles(m_apLHComboEffects);
		C_CFPlayer::ShowHandMagic(m_pPreviewPlayer.GetModel(), m_apLHComboEffects, eLHEffectElements, "lmagic");
		m_pPreviewPlayer.AddParticles(m_apLHComboEffects);
		m_eLHEffectElements = eLHEffectElements;
	}

	if (m_pPreviewPlayer.GetModel() && eRHEffectElements != m_eRHEffectElements)
	{
		m_pPreviewPlayer.RemoveParticles(m_apRHComboEffects);
		C_CFPlayer::ShowHandMagic(m_pPreviewPlayer.GetModel(), m_apRHComboEffects, eRHEffectElements, "rmagic");
		m_pPreviewPlayer.AddParticles(m_apRHComboEffects);
		m_eRHEffectElements = eRHEffectElements;
	}

	m_pPreviewPlayer.Render(CFScreenWidth()/2 - iPreviewWidth/2, iInfoY + BTN_BORDER, iPreviewWidth, iPreviewHeight, Vector(200, 0, -6), QAngle(0, -170, 0));
}

void CCharPanel::ChooseArmamentCallback(KeyValues* pKV)
{
	m_apButtons[pKV->GetFirstValue()->GetInt()]->Load();

	CCFMenu::KillOnClose();

	CCFMenu::Close();
}

void CCharPanel::EditArmamentCallback(KeyValues* pKV)
{
	m_apButtons[pKV->GetFirstValue()->GetInt()]->Load();

	CCFMenu::KillOnClose();

	CCFMenu::s_pMenu->SetPanel((CFPanel)(CCFMenu::s_pMenu->GetPanel()+1));
}

void CCharPanel::ChooseColorCallback(KeyValues* pKV)
{
	CCFPlayer* pPlayer = C_CFPlayer::GetLocalCFPlayer();
	int iTotalColors = 0;
	switch (pPlayer->GetTeamNumber())
	{
	case TEAM_NUMENI:
		iTotalColors = m_apNumeniColors.Count();
		break;

	case TEAM_MACHINDO:
		iTotalColors = m_apMachindoColors.Count();
		break;
	}

	m_iColor = pKV->GetFirstValue()->GetInt();

	for (int i = 0; i < iTotalColors; i++)
		m_apColorButtons[i]->SetState(m_iColor == i, false);

	engine->ClientCmd(VarArgs("choosecolor %i", m_iColor));
}

void CCharPanel::ChooseCategoryCallback(KeyValues* pKV)
{
	m_iCategory = pKV->GetFirstValue()->GetInt();

	Layout();
}

CConfigDescription::CConfigDescription()
	: CLabel(0, 0, 10, 10, "")
{
	m_pArm = NULL;
}

void CConfigDescription::SetActiveArmament(CArmament* pArm)
{
	m_pArm = pArm;

	if (pArm)
	{
		SetText(VarArgs("%s_Description", pArm->m_szName));
	}
}

void CConfigDescription::LevelShutdown()
{
	m_pArm = NULL;
}

CColorChoice::CColorChoice(CCFHudTexture* pTexture)
	: CButton(0, 0, BTN_HEIGHT, BTN_HEIGHT, "", true)
{
	m_pTexture = pTexture;
}

void CColorChoice::Paint(int x, int y, int w, int h)
{
	CRootPanel::DrawRect(x, y, x+w, y+h, Color(0,0,0,255));
	m_pTexture->DrawSelf(x, y, w, h, Color(255,255,255,255));

	if (m_bDown)
	{
		CButton::s_pButtonDTL->DrawSelf	(x,			y,			2,		2,		Color(255, 255, 255, 255));
		CButton::s_pButtonDT->DrawSelf	(x+2,		y,			w-4,	2,		Color(255, 255, 255, 255));
		CButton::s_pButtonDTR->DrawSelf	(x+w-2,		y,			2,		2,		Color(255, 255, 255, 255));
		CButton::s_pButtonDL->DrawSelf	(x,			y+2,		2,		h-4,	Color(255, 255, 255, 255));
		CButton::s_pButtonDR->DrawSelf	(x+w-2,		y+2,		2,		h-4,	Color(255, 255, 255, 255));
		CButton::s_pButtonDBL->DrawSelf	(x,			y+h-2,		2,		2,		Color(255, 255, 255, 255));
		CButton::s_pButtonDB->DrawSelf	(x+2,		y+h-2,		w-4,	2,		Color(255, 255, 255, 255));
		CButton::s_pButtonDBR->DrawSelf	(x+w-2,		y+h-2,		2,		2,		Color(255, 255, 255, 255));
	}
}

CAbilitiesPanel::CAbilitiesPanel()
	: CPanel(0,0,100,100)
{
	m_pArm = NULL;
	m_iLineHeight = 40;

	m_pText = new CLabel(0,0,BTN_WIDTH*2,BTN_HEIGHT,"");
	AddControl(m_pText);
}

void CAbilitiesPanel::SetArmament(CArmament* pArm)
{
	m_pArm = pArm;
	Layout();
}

void CAbilitiesPanel::Layout()
{
	SetSize(GetParent()->GetWidth()/6, (m_iLineHeight+BTN_BORDER)*4);
	SetPos(GetParent()->GetWidth()/8, BTN_BORDER + (GetParent()->GetHeight()/3 - GetHeight()));

	m_pText->SetAlign(CLabel::TA_LEFTCENTER);
}

void CAbilitiesPanel::Paint(int x, int y, int w, int h)
{
	if (!m_pArm)
		return;

	int iDrawn = 0;
	int iAbsX, iAbsY;
	GetAbsPos(iAbsX, iAbsY);
	int iBRX, iBRY;
	iBRX = iAbsX + GetWidth();
	iBRY = iAbsY + GetHeight();

	int iButtonX = iAbsX + BTN_BORDER;
	int iLineY = iBRY - m_iLineHeight;
	int iButtonW = 32;

	CCFHudTexture* pAttackTex = NULL;  
	const char *key; 

	if (m_pArm->HasBindableCombo())
	{
		if (m_pArm->m_aAttackBinds[0].m_iWeapon != -1)
		{
			key = engine->Key_LookupBinding("+alt2");
			pAttackTex = CTextureHandler::GetTextureFromKey(key);
		}

		if (pAttackTex)
		{
			pAttackTex->DrawSelf(iButtonX, iLineY - m_iLineHeight*iDrawn, iButtonW, iButtonW, Color(255, 255, 255, 255));
			m_pText->SetText("#Magic_Combo");
			m_pText->Paint(iButtonX + iButtonW + BTN_BORDER, iLineY - m_iLineHeight*iDrawn);
			iDrawn++;
			pAttackTex = NULL;
		}

		if (m_pArm->m_aAttackBinds[1].m_iWeapon != -1)
		{
			key = engine->Key_LookupBinding("+alt2");
			pAttackTex = CTextureHandler::GetTextureFromKey(key);
		}

		if (pAttackTex)
		{
			pAttackTex->DrawSelf(iButtonX, iLineY - m_iLineHeight*iDrawn, iButtonW, iButtonW, Color(255, 255, 255, 255));
			m_pText->SetText("#Magic_Combo");
			m_pText->Paint(iButtonX + iButtonW + BTN_BORDER, iLineY - m_iLineHeight*iDrawn);
			iDrawn++;
		}
	}

	if (m_pArm->CanFullLatch())
	{
		key = engine->Key_LookupBinding("+jump");
		pAttackTex = CTextureHandler::GetTextureFromKey(key);

		if (pAttackTex)
		{
			pAttackTex->DrawSelf(iButtonX, iLineY - m_iLineHeight*iDrawn, iButtonW, iButtonW, Color(255, 255, 255, 255));
			m_pText->SetText("#Grip");
			m_pText->Paint(iButtonX + iButtonW + BTN_BORDER, iLineY - m_iLineHeight*iDrawn);
			iDrawn++;
		}
	}

	if (m_pArm->GetPowerjumpDistance())
	{
		key = engine->Key_LookupBinding("+jump");
		pAttackTex = CTextureHandler::GetTextureFromKey(key);

		if (pAttackTex)
		{
			pAttackTex->DrawSelf(iButtonX, iLineY - m_iLineHeight*iDrawn, iButtonW, iButtonW, Color(255, 255, 255, 255));
			m_pText->SetText("#Doublejump");
			m_pText->Paint(iButtonX + iButtonW + BTN_BORDER, iLineY - m_iLineHeight*iDrawn);
			iDrawn++;
		}
	}

	m_pText->SetText("");
}

void CAbilitiesPanel::LevelShutdown()
{
	m_pArm = NULL;
}
