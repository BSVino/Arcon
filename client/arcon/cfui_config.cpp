#include "cbase.h"
#include <vgui/ISurface.h>
#include "cfui_menu.h"
#include "cfui_config.h"
#include "client_thinklist.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CConfigsPanel* CConfigsPanel::s_pConfigsPanel = NULL;
CConfigMgr* CConfigMgr::s_pMgr = NULL;

CConfigMgr::CConfigMgr(char const *pszName)
	: CAutoGameSystemPerFrame( pszName )
{
	m_pWaitingOn = NULL;
}

CConfigMgr* CConfigMgr::Get()
{
	if (!s_pMgr)
		s_pMgr = new CConfigMgr("ConfigMgr");

	return s_pMgr;
}

void CConfigMgr::Update( float flFrametime )
{
	if (m_pWaitingOn && CArmament::GetWriteTo() == WRITETO_SERVER)
	{
		const char* pszNext = filesystem->FindNext( m_hFindHandle );

		if (!pszNext)
		{
			filesystem->FindClose( m_hFindHandle );
			m_pWaitingOn = NULL;
		}
		else
		{
			// Process one new CArmament this frame.
			SetupArmament(pszNext);

			CRootPanel::GetRoot()->Layout();
		}
	}
}

void CConfigMgr::LevelShutdownPreEntity()
{
	ClearArmaments();
}

void CConfigMgr::LoadArmaments()
{
	if (m_apPresets.Count() != 0 || m_apCustoms.Count() != 0)
		return;

	const char* pszNext = filesystem->FindFirst( CFG_PATH "/*.cfg", &m_hFindHandle );

	if (pszNext)
	{
		SetupArmament(pszNext);
	}
}

void CConfigMgr::SetupArmament(const char* pszName)
{
	Assert(CArmament::GetWriteTo() == WRITETO_SERVER);
	CArmament* pArm = new CArmament();

	if (Q_strncmp(pszName, "default", 7) != 0 &&
		Q_strncmp(pszName, "user", 4) != 0)
	{
		// This isn't a special config, just run it through.
		engine->ClientCmd( VarArgs("exec " CFG_DIR "/%s", pszName) );
		return;
	}

	// Isolate the file name by chopping off the extension.
	char pszFilepath[256];
	Q_strncpy(pszFilepath, pszName, 256);

	char *pszFilename = pszFilepath;
	pszFilename[strlen(pszFilename)-4] = '\0';

	pArm->LoadConfig(pszFilename);

	if (Q_strncmp(pszFilename, "default", 7) == 0)
		m_apPresets.AddToTail(pArm);
	else
		m_apCustoms.AddToTail(pArm);

	// We need to wait a frame for the LoadConfig to complete.
	// Otherwise, we will load a crapton of config files into
	// the command buffer, all of which will be processed at
	// the end of the frame, spilling them all into one CArmament.
	m_pWaitingOn = pArm;
}

CUtlVector<CArmament*>* CConfigMgr::GetArmamentList(ConfigPanelType eType)
{
	if (eType == CConfigMgr::CFG_ALL)
	{
		m_apAll.RemoveAll();
		m_apAll.AddVectorToTail(m_apPresets);
		m_apAll.AddVectorToTail(m_apCustoms);
		return &m_apAll;
	}

	if (eType == CConfigMgr::CFG_PRESETS)
	{
		return &m_apPresets;
	}

	int iLevel;
	switch (eType)
	{
	case CConfigMgr::CFG_BASIC:
		iLevel = 0;
		break;

	case CConfigMgr::CFG_MEDIUM:
		iLevel = 1;
		break;

	case CConfigMgr::CFG_ADVANCED:
		iLevel = 2;
		break;

	case CConfigMgr::CFG_CUSTOM:
	case CConfigMgr::CFG_SAVE:
	default:
		return &m_apCustoms;
	}

	m_apSubset.RemoveAll();
	for (int i = 0; i < m_apPresets.Count(); i++)
	{
		if (m_apPresets[i]->m_iLevel == iLevel)
			m_apSubset.AddToTail(m_apPresets[i]);
	}

	return &m_apSubset;
}

CUtlVector<CRuneComboPreset*>* CConfigMgr::GetRuneList(int iSlots)
{
	m_apRunePresetSlots.RemoveAll();

	for (int i = 0; i < m_apRunePresets.Count(); i++)
	{
		if (m_apRunePresets[i]->m_iSlots == iSlots)
			m_apRunePresetSlots.AddToTail(m_apRunePresets[i]);
	}

	return &m_apRunePresetSlots;
}

void CConfigMgr::AddCustom(CArmament* pArm)
{
	m_apCustoms.AddToTail(pArm);
}

void CConfigMgr::AddRuneCombo(CRuneComboPreset* pCombo)
{
	m_apRunePresets.AddToTail(pCombo);
}

void CConfigMgr::ClearArmaments()
{
	m_apPresets.PurgeAndDeleteElements();
	m_apCustoms.PurgeAndDeleteElements();
	m_apRunePresets.PurgeAndDeleteElements();

	m_pWaitingOn = NULL;
}


#ifdef CLIENT_DLL
void CC_CfgCombo(const CCommand &args)
{
	CConfigMgr* pCfg = CConfigMgr::Get();

	Assert(pCfg);

	if (!pCfg)
		return;

	CRuneComboPreset* pNew = new CRuneComboPreset();

	int iSlot = 0;

	Q_strncpy(pNew->m_szName, args[1], sizeof(pNew->m_szName));

	// For each armament
	for (int i = 2; i < args.ArgC(); i++)
		pNew->m_aRunes[iSlot++] = CRuneData::AliasToRuneID(args[i]);

	pNew->CalculateData();

	pCfg->AddRuneCombo(pNew);
}

static ConCommand cfg_combo("cfg_combo", CC_CfgCombo, "", FCVAR_CLIENTCMD_CAN_EXECUTE);
#endif

CConfigsMenu::CConfigsMenu()
	: CButton(0, 0, BTN_WIDTH, BTN_HEIGHT, "#Configs", true)
{
	m_pPopup = new CConfigsPopup(this);
	CRootPanel::GetRoot()->AddControl(m_pPopup, true);

	SetClickedListener(this, OpenMenu);
	SetUnclickedListener(this, CloseMenu);
}

void CConfigsMenu::Layout()
{
	int x, y, x2, y2;
	GetAbsPos(x, y);
	GetBR(x2, y2);

	m_pPopup->SetPos(x, y+y2+BTN_BORDER);
	m_pPopup->Layout();
}

void CConfigsMenu::OpenMenuCallback(KeyValues* pParms)
{
	m_pPopup->Open();
}

void CConfigsMenu::CloseMenuCallback(KeyValues* pParms)
{
	m_pPopup->Close();
}

CConfigsPopup::CConfigsPopup(CConfigsMenu* pButton)
	: CPanel(0, 0, 100, 100)
{
	m_pButton = pButton;

	SetVisible(false);

	for (int i = 0; i < 3; i++)
	{
		m_apButtons.AddToTail(new CButton(BTN_BORDER, BTN_BORDER*(i+1) + BTN_HEIGHT*i, BTN_WIDTH*1.5f, BTN_HEIGHT, ""));
		AddControl(m_apButtons[i]);
	}

	m_apButtons[0]->SetText("#Browse_Presets");
	m_apButtons[1]->SetText("#Load_Custom");
	m_apButtons[2]->SetText("#Save_This_Config");

	m_apButtons[0]->SetClickedListener(this, OpenPanel, new KeyValues("open", "window", "presets"));
	m_apButtons[1]->SetClickedListener(this, OpenPanel, new KeyValues("open", "window", "custom"));
	m_apButtons[2]->SetClickedListener(this, OpenPanel, new KeyValues("open", "window", "save"));
}

void CConfigsPopup::Layout()
{
	SetSize(BTN_WIDTH*1.5f + BTN_BORDER*2, BTN_HEIGHT*3 + BTN_BORDER*4);
}

void CConfigsPopup::Paint(int x, int y, int w, int h)
{
	if (!IsVisible())
		return;

	CRootPanel::DrawRect(x, y, x+w, y+h, Color(0, 0, 0, 255));

	CPanel::Paint(x, y, w, h);
}

void CConfigsPopup::Open()
{
	CRootPanel::GetRoot()->Popup(this);
	m_pButton->SetState(true, false);
	SetVisible(true);
}

void CConfigsPopup::Close()
{
	CRootPanel::GetRoot()->Popup(NULL);
	m_pButton->SetState(false, false);
	SetVisible(false);
}

void CConfigsPopup::OpenPanelCallback(KeyValues* pKV)
{
	Close();

	const char* pszPanel = pKV->GetFirstValue()->GetString();
	if (FStrEq(pszPanel, "presets"))
		CConfigsPanel::Open(CConfigMgr::CFG_PRESETS);
	else if (FStrEq(pszPanel, "custom"))
		CConfigsPanel::Open(CConfigMgr::CFG_CUSTOM);
	else if (FStrEq(pszPanel, "save"))
		CConfigsPanel::Open(CConfigMgr::CFG_SAVE);
	else
		AssertMsg(false, "What panel was that again?");
}

CConfigChoice::CConfigChoice(CArmament *pArm)
	: CButton(0, 0, 100, 100, "")
{
	m_pArm = pArm;
	m_pHighlightListener = NULL;
	m_pImage = NULL;
}

void CConfigChoice::Destructor()
{
	CButton::Destructor();
}

void CConfigChoice::Layout()
{
	SetText(m_pArm->m_szName);
}

void CConfigChoice::Paint(int x, int y, int w, int h)
{
	if (!m_pArm)
	{
		CButton::Paint(x, y, w, h);
		return;
	}

	if (m_bDown)
	{
		CButton::s_pButtonDTL->DrawSelf	(x,			y,			3,		3,		Color(255, 255, 255, 255));
		CButton::s_pButtonDT->DrawSelf	(x+3,		y,			w-6,	3,		Color(255, 255, 255, 255));
		CButton::s_pButtonDTR->DrawSelf	(x+w-3,		y,			3,		3,		Color(255, 255, 255, 255));
		CButton::s_pButtonDL->DrawSelf	(x,			y+3,		3,		h-6,	Color(255, 255, 255, 255));
		CButton::s_pButtonDC->DrawSelf	(x+3,		y+3,		w-6,	h-6,	Color(255, 255, 255, 255));
		CButton::s_pButtonDR->DrawSelf	(x+w-3,		y+3,		3,		h-6,	Color(255, 255, 255, 255));
		CButton::s_pButtonDBL->DrawSelf	(x,			y+h-3,		3,		3,		Color(255, 255, 255, 255));
		CButton::s_pButtonDB->DrawSelf	(x+3,		y+h-3,		w-6,	3,		Color(255, 255, 255, 255));
		CButton::s_pButtonDBR->DrawSelf	(x+w-3,		y+h-3,		3,		3,		Color(255, 255, 255, 255));
	}
	else if (m_bHighlight && CRootPanel::GetRoot()->GetButtonDown() == NULL)
	{
		CButton::s_pButtonHTL->DrawSelf	(x,			y,			3,		3,		Color(255, 255, 255, 255));
		CButton::s_pButtonHT->DrawSelf	(x+3,		y,			w-6,	3,		Color(255, 255, 255, 255));
		CButton::s_pButtonHTR->DrawSelf	(x+w-3,		y,			3,		3,		Color(255, 255, 255, 255));
		CButton::s_pButtonHL->DrawSelf	(x,			y+3,		3,		h-6,	Color(255, 255, 255, 255));
		CButton::s_pButtonHC->DrawSelf	(x+3,		y+3,		w-6,	h-6,	Color(255, 255, 255, 255));
		CButton::s_pButtonHR->DrawSelf	(x+w-3,		y+3,		3,		h-6,	Color(255, 255, 255, 255));
		CButton::s_pButtonHBL->DrawSelf	(x,			y+h-3,		3,		3,		Color(255, 255, 255, 255));
		CButton::s_pButtonHB->DrawSelf	(x+3,		y+h-3,		w-6,	3,		Color(255, 255, 255, 255));
		CButton::s_pButtonHBR->DrawSelf	(x+w-3,		y+h-3,		3,		3,		Color(255, 255, 255, 255));
	}
	else
	{
		CButton::s_pButtonTL->DrawSelf	(x,			y,			3,		3,		Color(255, 255, 255, 255));
		CButton::s_pButtonT->DrawSelf	(x+3,		y,			w-6,	3,		Color(255, 255, 255, 255));
		CButton::s_pButtonTR->DrawSelf	(x+w-3,		y,			3,		3,		Color(255, 255, 255, 255));
		CButton::s_pButtonL->DrawSelf	(x,			y+3,		3,		h-6,	Color(255, 255, 255, 255));
		CButton::s_pButtonC->DrawSelf	(x+3,		y+3,		w-6,	h-6,	Color(255, 255, 255, 255));
		CButton::s_pButtonR->DrawSelf	(x+w-3,		y+3,		3,		h-6,	Color(255, 255, 255, 255));
		CButton::s_pButtonBL->DrawSelf	(x,			y+h-3,		3,		3,		Color(255, 255, 255, 255));
		CButton::s_pButtonB->DrawSelf	(x+3,		y+h-3,		w-6,	3,		Color(255, 255, 255, 255));
		CButton::s_pButtonBR->DrawSelf	(x+w-3,		y+h-3,		3,		3,		Color(255, 255, 255, 255));
	}

	if (m_pImage)
	{
		m_pImage->DrawSelf(x, y, w, h, Color(255,255,255,255));

		// Skip button, already did that.
		CLabel::Paint(x, y, w, h);
		return;
	}

	int i;
	int iArms = 0;
	int iCurrent = -1;
	int iDrawn = 0;

	// How many arms do we have?
	for (i = 0; i < 5; i++)
	{
		if (m_pArm->m_aWeapons[i].m_pWeaponData || m_pArm->m_aWeapons[i].m_pArmamentData)
		{
			if (iCurrent < 0)
				iCurrent = i;
			iArms++;
		}
	}

	int iWidth = w/3 - 6;
	int iFullHeight = h - 6;
	int iHalfHeight = h/2 - 6;

	while (true)
	{
		switch (iDrawn++)
		{
		case 0:
			PaintArmament(iCurrent, x + 3, y + 3, iWidth, iArms==5?iHalfHeight:iFullHeight);
			break;

		case 1:
			PaintArmament(iCurrent, x + iWidth + 3, y + 3, iWidth, iArms>4?iHalfHeight:iFullHeight);
			break;

		case 2:
			PaintArmament(iCurrent, x + iWidth*2 + 3, y + 3, iWidth, iArms>3?iHalfHeight:iFullHeight);
			break;

		case 3:
			PaintArmament(iCurrent, x + iWidth*2 + 3, y + iHalfHeight + 3, iWidth, iHalfHeight);
			break;

		case 4:
			PaintArmament(iCurrent, x + iWidth + 3, y + iHalfHeight + 3, iWidth, iHalfHeight);
			break;

		default:
			Assert(!"Invalid armament number");
		}

		// Find the next one.
		for (i = iCurrent+1; i < 5; i++)
		{
			if (m_pArm->m_aWeapons[i].m_pWeaponData || m_pArm->m_aWeapons[i].m_pArmamentData)
			{
				iCurrent = i;
				break;
			}
		}

		if (i == 5)
			break;
	}

	// Skip button, already did that.
	CLabel::Paint(x, y, w, h);
}

void CConfigChoice::PaintArmament(int iArmament, int x, int y, int w, int h)
{
	if (iArmament <= 2)
	{
		FileWeaponInfo_t* pArmData = m_pArm->GetWeaponData(iArmament);

		if (!pArmData)
			return;

		CFWeaponID eWeapon = m_pArm->m_aWeapons[iArmament].m_iWeapon;
		if (CTextureHandler::GetTexture(eWeapon))
			CTextureHandler::GetTexture(eWeapon)->DrawSelf(x, y, h, h, Color(255, 255, 255, 255));
	}
	else
	{
		CArmamentData* pArmData = m_pArm->GetArmamentData(iArmament);

		if (!pArmData)
			return;

		ArmamentID eArmament = m_pArm->m_aWeapons[iArmament].m_iArmament;
		if (CTextureHandler::GetTexture(eArmament))
			CTextureHandler::GetTexture(eArmament)->DrawSelf(x, y, h, h, Color(255, 255, 255, 255));
	}

	int iRunesDrawn = 0;
	int iRow = 0;
	int iRuneWidth = h/2;

	for (int i = 0; i < MAX_RUNES; i++)
	{
		CGUIRuneCombo Combo = CGUIRuneCombo(m_pArm, iArmament, i, true);

		if (Combo.IsEmpty())
			continue;
		
		Combo.Paint(x + iRunesDrawn*iRuneWidth, y + iRow*iRuneWidth, iRuneWidth, iRuneWidth);

		iRunesDrawn++;

		if (iRunesDrawn*iRuneWidth + iRuneWidth > h)
		{
			iRunesDrawn = 0;
			Assert(iRow < 2);	// Only have space for two rows at the moment. More is a problem!
			iRow++;
		}
	}
}

void CConfigChoice::SetArmament(CArmament* pArm)
{
	m_pArm = pArm;
}

void CConfigChoice::SetImage(char* pszImage)
{
	m_pImage = GetHudTexture(pszImage);
}

void CConfigChoice::Load()
{
	CCFPlayer* pPlayer = C_CFPlayer::GetLocalCFPlayer();

	Assert(pPlayer);
	if (!pPlayer)
		return;

	*pPlayer->m_pArmament = *m_pArm;
	pPlayer->m_pArmament->WriteAll();
	CConfigsPanel::Close();
	cfgui::CRootPanel::UpdateArmament(pPlayer->m_pArmament);
}

void CConfigChoice::CursorIn()
{
	CButton::CursorIn();

	if (m_pHighlightListener)
		m_pHighlightListener->Highlighted(m_pArm);
}

void CConfigChoice::CursorOut()
{
	CButton::CursorOut();

	if (m_pHighlightListener)
		m_pHighlightListener->Highlighted(NULL);
}

CConfigsPanel::CConfigsPanel()
	: CPanel(CFScreenWidth()/2-300, 50, 600, CFScreenHeight()-200)
{
	Assert(!s_pConfigsPanel);
	CRootPanel::GetRoot()->AddControl(this, true);

	m_pClose = new CButton(GetWidth()-BTN_BORDER-BTN_WIDTH, BTN_BORDER, BTN_WIDTH, BTN_HEIGHT, "#Close");
	AddControl(m_pClose);
	m_pClose->SetClickedListener(this, Close);

	m_pNewSave = new CButton(GetWidth()-BTN_BORDER*2-BTN_WIDTH*2, BTN_BORDER, BTN_WIDTH, BTN_HEIGHT, "#New_Save");
	AddControl(m_pNewSave);
	m_pNewSave->SetClickedListener(this, Save, new KeyValues("save", "armament", -1));

	CConfigMgr::Get()->LoadArmaments();
}

void CConfigsPanel::Destructor()
{
	Assert(s_pConfigsPanel);
	s_pConfigsPanel = NULL;

	CPanel::Destructor();
}

void CConfigsPanel::Open(CConfigMgr::ConfigPanelType eType)
{
	if (!s_pConfigsPanel)
		s_pConfigsPanel = new CConfigsPanel();

	s_pConfigsPanel->m_eType = eType;
	s_pConfigsPanel->SetVisible(true);
	s_pConfigsPanel->Layout();
}

void CConfigsPanel::Close()
{
	if (s_pConfigsPanel)
		s_pConfigsPanel->SetVisible(false);
}

void CConfigsPanel::Layout()
{
	SetDimensions(CFScreenWidth()/2-300, 50, 600, CFScreenHeight()-200);

	// Must remove the old buttons, because its CArmament pointer may become invalid between map changes.
	while (m_apButtons.Count())
	{
		m_apButtons[0]->Destructor();
		m_apButtons[0]->Delete();
		m_apButtons.Remove(0);
	}

	CConfigMgr::Get()->LoadArmaments();

	CUtlVector<CArmament*>* pArmaments = CConfigMgr::Get()->GetArmamentList(m_eType);

	m_pClose->SetPos(GetWidth()-BTN_BORDER-BTN_WIDTH, BTN_BORDER);
	m_pNewSave->SetPos(GetWidth()-BTN_BORDER*2-BTN_WIDTH*2, BTN_BORDER);

	m_pNewSave->SetVisible(m_eType == CConfigMgr::CFG_SAVE);

	int iCount = pArmaments->Count();
	for (int i = 0; i < iCount; i++)
	{
		CConfigChoice* pButton = new CConfigChoice(pArmaments->Element(i));
		m_apButtons.AddToTail(pButton);
		AddControl(pButton, true);
		pButton->SetDimensions(BTN_BORDER, BTN_HEIGHT + BTN_BORDER*2 + (100*i + BTN_BORDER*i), 600-BTN_BORDER*2, 100);
		if (m_eType == CConfigMgr::CFG_SAVE)
			pButton->SetClickedListener(this, Save, new KeyValues("save", "armament", i));
		else
			pButton->SetClickedListener(this, ChooseArmament, new KeyValues("choose", "armament", i));
		pButton->Layout();
	}
}

void CConfigsPanel::Paint(int x, int y, int w, int h)
{
	if (!IsVisible())
		return;

	CRootPanel::DrawRect(x, y, x+w, y+h, Color(0, 0, 0, 255));

	CPanel::Paint(x, y, w, h);
}

void CConfigsPanel::CloseCallback(KeyValues* pKV)
{
	SetVisible(false);
}

void CConfigsPanel::SaveCallback(KeyValues* pKV)
{
	CCFPlayer* pPlayer = C_CFPlayer::GetLocalCFPlayer();
	CArmament* pArm = NULL;
	int iIndex;

	if (pKV->GetFirstValue()->GetInt() >= 0)
	{
		iIndex = pKV->GetFirstValue()->GetInt();
		pArm = m_apButtons[iIndex]->m_pArm;

		// Use the index of the user file to save the armament.
		if (Q_strncmp(pArm->m_szFile, "user", 4) == 0)
			iIndex = atoi(pArm->m_szFile+4);
		else
			iIndex = CArmament::GetUnusedUserConfigIndex();	// If for whatever reason...
	}
	else
	{
		pArm = new CArmament();
		CConfigMgr::Get()->AddCustom(pArm);

		iIndex = CArmament::GetUnusedUserConfigIndex();
	}

	*pArm = *pPlayer->m_pArmament;
	Q_snprintf(pArm->m_szName, sizeof(pArm->m_szName), "#Custom_Config", iIndex);
	pArm->SaveConfig(iIndex);

	CConfigsPanel::Close();

	CRootPanel::GetRoot()->Layout();
}

void CConfigsPanel::ChooseArmamentCallback(KeyValues* pKV)
{
	m_apButtons[pKV->GetFirstValue()->GetInt()]->Load();
}
