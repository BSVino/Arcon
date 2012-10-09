#include "cbase.h"
#include "cfui_menu.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CArmorDescription::CArmorDescription(bool bChanges)
	: CLabel(0, 0, PANEL_WIDTH, PANEL_HEIGHT, ""),
	IHighlightListener<ArmamentID>()
{
	m_bChanges = bChanges;
	m_ePaintedArmament = CArmamentData::InvalidArmament();
	SetAlign(TA_TOPLEFT);
}

void CArmorDescription::Paint(int x, int y, int w, int h)
{
	CLabel::Paint(x, y, w, h);

	int iTextureWidth = 150;

	if (m_ePaintedArmament != CArmamentData::InvalidArmament())
		CTextureHandler::GetTexture(m_ePaintedArmament)->DrawSelf(x + GetWidth() - iTextureWidth, y, iTextureWidth, iTextureWidth, Color(255, 255, 255, 255));
}

void CArmorDescription::Highlighted(ArmamentID eArmor)
{
	SetActiveArmor(eArmor);
}

void CArmorDescription::CreateDeltaString(char* pszString, int iLength, int iCurrentValue, int iNewValue)
{
	Q_strncpy(pszString, VarArgs("%s%d", ((iNewValue >= iCurrentValue)?"+":""), iNewValue - iCurrentValue), iLength);
}

void CArmorDescription::SetActiveArmor(ArmamentID eArmor)
{
	m_ePaintedArmament = eArmor;
	
	if (m_ePaintedArmament == CArmamentData::InvalidArmament())
	{
		SetText("");
		return;
	}

	CArmamentData* pData = (CArmamentData::GetData(eArmor));

	// Find the player's chosen armor.
	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	CArmamentData* pCurData = pArm->GetArmamentData(3);

	if (pCurData && m_bChanges)
	{
		char szCost[64];
		if (pData->m_iCost < pCurData->m_iCost)
			Q_strncpy(szCost, VarArgs("Reclaim: %d space", pCurData->m_iCost - pData->m_iCost), sizeof(szCost));
		else
			Q_strncpy(szCost, VarArgs("Cost: %d additional space", pData->m_iCost - pCurData->m_iCost), sizeof(szCost));

		char szHP[8], szHPR[8], szFP[8], szFPR[8], szSP[8], szSPR[8], szD[8], szR[8], szC[8];

		CreateDeltaString(szHP,		sizeof(szHP),	pCurData->m_iMaxHealth, pData->m_iMaxHealth);
		CreateDeltaString(szHPR,	sizeof(szHPR),	pCurData->m_iHealthRegen, pData->m_iHealthRegen);
		CreateDeltaString(szFP,		sizeof(szFP),	pCurData->m_iMaxFocus, pData->m_iMaxFocus);
		CreateDeltaString(szFPR,	sizeof(szFPR),	pCurData->m_iFocusRegen, pData->m_iFocusRegen);
		CreateDeltaString(szSP,		sizeof(szSP),	pCurData->m_iMaxStamina, pData->m_iMaxStamina);
		CreateDeltaString(szSPR,	sizeof(szSPR),	pCurData->m_iStaminaRegen, pData->m_iStaminaRegen);
		CreateDeltaString(szD,		sizeof(szD),	pCurData->m_iDefense, pData->m_iDefense);
		CreateDeltaString(szR,		sizeof(szR),	pCurData->m_iResistance, pData->m_iResistance);
		CreateDeltaString(szC,		sizeof(szC),	pCurData->m_iCritical, pData->m_iCritical);

		SetText(VarArgs(
			"%s\n"
			" \n"
			"%s\n"
			" \n"
			"Health bonus change: %s\n"
			"Health regen bonus change: %s\n"
			"Focus bonus change: %s\n"
			"Focus regen bonus change: %s\n"
			"Stamina bonus change: %s\n"
			"Stamina regen bonus change: %s\n"
			" \n"
			"Defense bonus change: %s\n"
			"Resistance bonus change: %s\n"
			"Critical hit bonus change: %s\n",

			pData->m_szPrintName,
			szCost,
			szHP, szHPR, szFP, szFPR, szSP, szSPR,
			szD, szR, szC
		));
	}
	else
	{
		SetText(VarArgs(
			"%s\n"
			" \n"
			"Cost: %d\n"
			" \n"
			"Health bonus: %d\n"
			"Health regen bonus: %d\n"
			"Atma bonus: %d\n"
			"Atma regen bonus: %d\n"
			"Stamina bonus: %d\n"
			"Stamina regen bonus: %d\n"
			" \n"
			"Defense bonus: %d\n"
			"Resistance bonus: %d\n"
			"Critical hit bonus: %d\n",

			pData->m_szPrintName,
			pData->m_iCost,
			pData->m_iMaxHealth,
			pData->m_iHealthRegen,
			pData->m_iMaxFocus,
			pData->m_iFocusRegen,
			pData->m_iMaxStamina,
			pData->m_iStaminaRegen,
			pData->m_iDefense,
			pData->m_iResistance,
			pData->m_iCritical
		));
	}

	SetAlign(TA_TOPLEFT);
}

CArmamentChoice::CArmamentChoice(ArmamentID eArm)
	: CButton(0, 0, 100, 100, "", true)
{
	m_eArm = eArm;

	m_pHighlightListener = NULL;

	// Kind of a hack, but we use the fact that this is a button to display the no cash thing as its label.
	SetText("#No_Cash");
	SetFGColor(Color(255, 0, 0, 255));
}

void CArmamentChoice::Paint(int x, int y, int w, int h)
{
	int iWidth = w<h?w:h;

	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;
	CArmamentData* pArmament = CArmamentData::GetData(m_eArm);
	CArmamentData* pCurrentArmament = pArm->GetArmamentData(3);

	int iCurrentCost = pCurrentArmament?pCurrentArmament->m_iCost:0;

	bool bTooExpensive = pArmament->m_iCost > pArm->m_iCredits + iCurrentCost && !GetState();

	if (!bTooExpensive && GetState())
		CTextureHandler::GetTexture(m_eArm, CTextureHandler::TT_HIGHLIGHT)->DrawSelf(x, y, iWidth, iWidth, Color(255, 255, 255, 255));

	if (m_eArm != CArmamentData::InvalidArmament())
		CTextureHandler::GetTexture(m_eArm)->DrawSelf(x, y, iWidth, iWidth, Color(255, 255, 255, bTooExpensive?100:255));

	// If we're too expensive, our label already says #No_Cash
	if (bTooExpensive)
		CLabel::Paint(x, y, w, h);
}

void CArmamentChoice::CursorIn()
{
	CButton::CursorIn();

	if (m_pHighlightListener)
		m_pHighlightListener->Highlighted(m_eArm);
}

void CArmamentChoice::CursorOut()
{
	CButton::CursorOut();

	if (m_pHighlightListener)
		m_pHighlightListener->Highlighted(CArmamentData::InvalidArmament());
}

CArmrPanel::CArmrPanel()
	: CPanel(0, BTN_HEIGHT + BTN_BORDER, PANEL_WIDTH, PANEL_HEIGHT)
{
	m_pChoose = new CLabel(3, 3, PANEL_WIDTH - 6, BTN_HEIGHT, "#Select_Armor");
	AddControl(m_pChoose);

	m_pYourArmor = new CLabel(0, 0, 200, BTN_HEIGHT, "#Your_Current_Armor");
	AddControl(m_pYourArmor);

	m_pSelectedArmor = new CLabel(0, 0, 200, BTN_HEIGHT, "#Comparison");
	AddControl(m_pSelectedArmor);

	m_pArmorLabel = new CLabel(0, 0, 200, BTN_HEIGHT, "#Armor");
	AddControl(m_pArmorLabel);

	m_pAccessoriesLabel = new CLabel(0, 0, 200, BTN_HEIGHT, "#Accessory");
	AddControl(m_pAccessoriesLabel);

	int iArmorHeight = PANEL_HEIGHT - BTN_HEIGHT - BTN_BORDER*3;
	int iArmorWidth = BTN_WIDTH*3 + BTN_BORDER*2;
	int iInfoHeight = (PANEL_HEIGHT - BTN_HEIGHT - BTN_BORDER*4)/2;
	int iInfoWidth = PANEL_WIDTH - iArmorWidth - BTN_BORDER*3;

	m_pArmor	= new CPanel(BTN_BORDER, BTN_HEIGHT + BTN_BORDER*2, iArmorWidth, iArmorHeight);
	m_pAccessories = new CPanel(BTN_BORDER, BTN_HEIGHT + BTN_BORDER*2, iArmorWidth, iArmorHeight);
	m_pCurInfo	= new CPanel(BTN_BORDER + iArmorWidth + BTN_BORDER, BTN_HEIGHT + BTN_BORDER*2, iInfoWidth, iInfoHeight);
	m_pSelInfo	= new CPanel(BTN_BORDER + iArmorWidth + BTN_BORDER, BTN_HEIGHT + BTN_BORDER*3 + iInfoHeight, iInfoWidth, iInfoHeight);

	AddControl(m_pArmor);
	AddControl(m_pAccessories);
	AddControl(m_pCurInfo);
	AddControl(m_pSelInfo);

	m_pCurDesc = new CArmorDescription();
	m_pCurInfo->AddControl(m_pCurDesc);

	m_pSelDesc = new CArmorDescription(true);
	m_pSelInfo->AddControl(m_pSelDesc);

	CArmamentChoice* pArmor;
	for (unsigned int i = 0; i < CArmamentData::TotalArmaments(); i++)
	{
		pArmor = new CArmamentChoice(i);
		pArmor->SetHighlightListener(m_pSelDesc);
		pArmor->SetClickedListener(this, &CArmrPanel::SelectArmor, new KeyValues("select", "id", i));
		pArmor->SetUnclickedListener(this, &CArmrPanel::DeselectArmor, new KeyValues("type", "type", CArmamentData::GetData(pArmor->GetArmamentID())->m_eType));
		m_apArmor.AddToTail(pArmor);

		if (CArmamentData::GetData(pArmor->GetArmamentID())->m_eType == ARMAMENTTYPE_ARMOR)
			m_pArmor->AddControl(pArmor);
		else
			m_pAccessories->AddControl(pArmor);
	}

	Layout();
}

void CArmrPanel::Layout()
{
	SetSize(PANEL_WIDTH, PANEL_HEIGHT);
	SetPos(50, m_iY);

	int iArmorHeight = PANEL_HEIGHT - BTN_HEIGHT - BTN_BORDER*3;
	int iArmorWidth = BTN_WIDTH*3 + BTN_BORDER*2;
	int iInfoHeight = (PANEL_HEIGHT - BTN_HEIGHT*3 - BTN_BORDER*4)/2;
	int iInfoWidth = PANEL_WIDTH - iArmorWidth - BTN_BORDER*3;

	m_pChoose->SetSize(iInfoWidth, BTN_HEIGHT);
	m_pYourArmor->SetPos(BTN_BORDER + iArmorWidth + BTN_BORDER, BTN_HEIGHT + BTN_BORDER*2);
	m_pSelectedArmor->SetPos(BTN_BORDER + iArmorWidth + BTN_BORDER, BTN_HEIGHT*2 + BTN_BORDER*3 + iInfoHeight);
	m_pYourArmor->SetAlign(CLabel::TA_LEFTCENTER);
	m_pSelectedArmor->SetAlign(CLabel::TA_LEFTCENTER);

	m_pArmorLabel->SetPos(BTN_BORDER, BTN_BORDER);
	m_pArmorLabel->SetSize(iArmorWidth, BTN_HEIGHT);
	m_pArmorLabel->SetAlign(CLabel::TA_CENTER);
	m_pAccessoriesLabel->SetPos(BTN_BORDER, BTN_HEIGHT + BTN_BORDER*2 + iArmorHeight*2/3);
	m_pAccessoriesLabel->SetSize(iArmorWidth, BTN_HEIGHT);
	m_pAccessoriesLabel->SetAlign(CLabel::TA_CENTER);

	m_pArmor->SetPos(BTN_BORDER, BTN_HEIGHT + BTN_BORDER*2);
	m_pArmor->SetSize(iArmorWidth, iArmorHeight*2/3);
	m_pAccessories->SetPos(BTN_BORDER, BTN_HEIGHT + BTN_BORDER*2 + iArmorHeight*2/3 + BTN_HEIGHT);
	m_pAccessories->SetSize(iArmorWidth, iArmorHeight/3- BTN_HEIGHT);
	m_pCurInfo->SetPos(BTN_BORDER + iArmorWidth + BTN_BORDER, BTN_HEIGHT*2 + BTN_BORDER*2);
	m_pCurInfo->SetSize(iInfoWidth, iInfoHeight);
	m_pSelInfo->SetSize(iInfoWidth, iInfoHeight);
	m_pSelInfo->SetPos(BTN_BORDER + iArmorWidth + BTN_BORDER, BTN_HEIGHT*3 + BTN_BORDER*3 + iInfoHeight);

	unsigned int i;
	int iItem = 0;
	for (i = 0; i < 6; i++)
	{
		CArmamentData* pData = CArmamentData::GetData(i);
		if (!pData)
			break;

		if (!pData->m_bBuyable)
			continue;

		if (pData->m_eType != ARMAMENTTYPE_ARMOR)
			continue;

		LayoutChoice(m_apArmor[i], iItem++);
	}

	iItem = 0;
	for (i = 0; i < 6; i++)
	{
		CArmamentData* pData = CArmamentData::GetData(i);
		if (!pData)
			break;

		if (!pData->m_bBuyable)
			continue;

		if (pData->m_eType != ARMAMENTTYPE_ITEM)
			continue;

		LayoutChoice(m_apArmor[i], iItem++);
	}

	m_pCurDesc->SetPos(BTN_BORDER, BTN_BORDER);
	m_pCurDesc->SetSize(m_pCurInfo->GetWidth() - BTN_BORDER*2, m_pCurInfo->GetHeight() - BTN_BORDER*2);

	// Find the player's chosen armor.
	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	CArmamentData* pData = pArm->GetArmamentData(3);

	if (pData)
	{
		Assert(pData->m_eType == ARMAMENTTYPE_ARMOR);

		m_pCurDesc->SetActiveArmor(pArm->m_aWeapons[3].m_iArmament);
	}
	else
		m_pCurDesc->SetActiveArmor(CArmamentData::InvalidArmament());

	m_pSelDesc->SetPos(BTN_BORDER, BTN_BORDER);
	m_pSelDesc->SetSize(m_pSelInfo->GetWidth() - BTN_BORDER*2, m_pSelInfo->GetHeight() - BTN_BORDER*2);

	CPanel::Layout();
}

void CArmrPanel::LayoutChoice(CArmamentChoice* pChoice, int i)
{
	// Three rows.
	int iHeight = (pChoice->GetParent()->GetHeight()-BTN_BORDER*2)/3;
	// Two columns.
	int iWidth = (pChoice->GetParent()->GetWidth()-BTN_BORDER*2)/2;

	bool bLeft = (i%2==0);
	pChoice->SetPos(BTN_BORDER + (bLeft?0:iWidth), BTN_BORDER + iHeight*((int)(((float)i)/2)));
	pChoice->SetSize(iWidth, iWidth);

	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;
	pChoice->SetState(pArm->m_aWeapons[CArmamentData::GetData(pChoice->GetArmamentID())->m_eType].m_iArmament == pChoice->GetArmamentID(), false);
}

void CArmrPanel::SelectArmorCallback(KeyValues* pParms)
{
	unsigned int iArmament = pParms->GetFirstValue()->GetInt();
	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	if (!pArm->CanBuyArmament(iArmament))
	{
		m_apArmor[iArmament]->Pop(false, true);
		return;
	}

	pArm->BuyArmament(iArmament);

	CCFMenu::KillOnClose();

	CArmamentData* pData = CArmamentData::GetData(iArmament);

	// Pop all the other buttons.
	for (unsigned int i = 0; i < CArmamentData::TotalArmaments(); i++)
	{
		if (i == iArmament)
			continue;

		if (CArmamentData::GetData(m_apArmor[i]->GetArmamentID())->m_eType != pData->m_eType)
			continue;

		m_apArmor[i]->SetState(false, false);
	}

	CRootPanel::GetRoot()->Layout();
}

void CArmrPanel::DeselectArmorCallback(KeyValues* pParms)
{
	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	pArm->RemoveArmament((armamenttype_t)pParms->GetFirstValue()->GetInt());

	CCFMenu::KillOnClose();

	CRootPanel::GetRoot()->Layout();
}
