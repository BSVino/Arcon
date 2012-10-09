#include "cbase.h"
#include "cfui_menu.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CCFHudTexture* CWeaponGrid::s_pGridBox = NULL;
CCFHudTexture* CWeaponDescription::s_pRadialGraph = NULL;

CFWeaponID CWeapPanel::s_aeWeaponSlots[3][6] =
{
	{
		WEAPON_VALANGARD,
		WEAPON_RIFLE,
		WEAPON_SHOTGUN,
		WEAPON_RIVENBLADE,
		WEAPON_PISTOL,
		WEAPON_NONE,
	},
	{
		WEAPON_NONE,
		WEAPON_NONE,
		WEAPON_NONE,
		WEAPON_NONE,
		WEAPON_NONE,
		WEAPON_NONE,
	},
	{
		WEAPON_NONE,
		WEAPON_NONE,
		WEAPON_NONE,
		WEAPON_NONE,
		WEAPON_NONE,
		WEAPON_NONE,
	},
};

CWeaponIcon::CWeaponIcon(IDroppable* pParent, CFWeaponID eWeapon, DragClass_t eDragClass)
	: CDraggableIcon<CFWeaponID>(pParent, eWeapon, eDragClass)
{
}

void CWeaponIcon::Paint(int x, int y, int w, int h)
{
	CDraggableIcon<CFWeaponID>::Paint(x, y, w, h);
}

CWeaponDrop::CWeaponDrop(int iSlot, const CRect &HoldingRect)
	: CDroppableIcon<CWeaponIcon>(IDraggable::DC_WEAPONICON, HoldingRect)
{
	m_iSlot = iSlot;

	m_pDelete = new CDeleteButton();
	AddControl(m_pDelete);
	m_pDelete->SetClickedListener(this, &CRuneDrop::Remove);
}

void CWeaponDrop::Layout()
{
	int iSize = 32;
	m_pDelete->SetPos(m_iW-iSize-BTN_BORDER, m_iH-iSize-BTN_BORDER);
	m_pDelete->SetSize(iSize, iSize);
	m_pDelete->SetVisible(GetDraggableIcon() != NULL);

	CDroppableIcon<CWeaponIcon>::Layout();
}

void CWeaponDrop::Paint(int x, int y, int w, int h)
{
	if (!IsVisible())
		return;

	CDroppableIcon<CWeaponIcon>::Paint(x, y, w, h);
}

void CWeaponDrop::DraggableChanged(CWeaponIcon* pDragged)
{
	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	if (pDragged)
		pArm->BuyWeapon(pDragged->GetWeapon(), m_iSlot);
	else
		pArm->RemoveWeapon(m_iSlot);

	CCFMenu::KillOnClose();

	CRootPanel::UpdateArmament(pArm);

	CRootPanel::GetRoot()->Layout();
}

bool CWeaponDrop::CanDropHere(IDraggable* pDraggable)
{
	// Check the superclass to make sure we have the correct drag class before we execute the cast below.
	if (!CDroppableIcon<CWeaponIcon>::CanDropHere(pDraggable))
		return false;

	if (!pDraggable)
		return true;

	CWeaponIcon* pWeapon = (CWeaponIcon*)pDraggable;

	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	if (!pArm->CanBuyWeapon(pWeapon->GetWeapon(), m_iSlot, 1))
		return false;

	return true;
}

void CWeaponDrop::RemoveCallback(KeyValues* pParms)
{
	SetDraggable(NULL);
}

CWeapDropPanel::CWeapDropPanel(int iSlot)
	: CPanel(0, 0, PANEL_WIDTH, PANEL_HEIGHT)
{
	char* pszLabel = NULL;
	switch(iSlot)
	{
	case 0:
	default:
		pszLabel = "#Primary_Weapon";
		break;

	case 1:
		pszLabel = "#Secondary_Weapon";
		break;

	case 2:
		pszLabel = "#Secondary_Weapon";
		break;
	}

	m_pLabel = new CLabel(0, 0, BTN_WIDTH, BTN_HEIGHT, pszLabel);
	AddControl(m_pLabel);

	m_pDrop = new CWeaponDrop(iSlot, CRect(0, 0, 0, 0));
	AddControl(m_pDrop);
}

void CWeapDropPanel::Destructor()
{
}

void CWeapDropPanel::Layout()
{
	m_pLabel->SetSize(GetWidth(), BTN_HEIGHT);

	Assert(m_pDrop);

	CRect Dims(0, 0, 0, 0);
	GetAbsDimensions(Dims.x, Dims.y, Dims.w, Dims.h);
	Dims.x += BTN_BORDER;
	Dims.y += BTN_BORDER;
	Dims.w -= BTN_BORDER*2;
	Dims.h -= BTN_BORDER*2;
	m_pDrop->SetHoldingRect(Dims);
	m_pDrop->SetSize(GetWidth(), GetHeight());

	CPanel::Layout();
}

void CWeapDropPanel::Paint(int x, int y, int w, int h)
{
	if (!IsVisible())
		return;

	IDraggable* pDragging = CRootPanel::GetRoot()->GetCurrentDraggable();

	if (pDragging && m_pDrop->CanDropHere(pDragging))
	{
		// WEAPON_MAGIC is the highlight.
		CTextureHandler::GetTexture(WEAPON_MAGIC)->DrawSelf(x, y, w, h, Color(255, 255, 255, 255));
		SetHighlighted(true);
	}
	else
		SetHighlighted(false);

	if (m_pDrop->GetDraggableIcon())
		m_pDrop->GetDraggableIcon()->Paint();

	CPanel::Paint(x, y, w, h);
}

bool CWeapDropPanel::MousePressed(vgui::MouseCode code)
{
	return m_pDrop->MousePressed(code);
}

CWeaponDescription::CWeaponDescription()
	: CLabel(0, 0, PANEL_WIDTH, PANEL_HEIGHT, ""),
	IHighlightListener<CFWeaponID>()
{
	m_ePaintedWeapon = WEAPON_NONE;
	SetAlign(TA_TOPLEFT);
}

void CWeaponDescription::Paint(int x, int y, int w, int h)
{
	CLabel::Paint(x, y, w, h);

	int iTextureWidth = 150;
	/*
	if (m_ePaintedWeapon != WEAPON_NONE)
		s_pRadialGraph->DrawSelf(x + GetWidth() - (3*iTextureWidth), y, iTextureWidth, iTextureWidth, Color(255, 255, 255, 255));

	if (m_ePaintedWeapon != WEAPON_NONE)
		CTextureHandler::GetTexture(m_ePaintedWeapon, CTextureHandler::TT_RADIAL)->DrawSelf(x + GetWidth() - (3*iTextureWidth), y, iTextureWidth, iTextureWidth, Color(255, 255, 255, 255));
	*/
	if (m_ePaintedWeapon != WEAPON_NONE)
		CTextureHandler::GetTexture(m_ePaintedWeapon)->DrawSelf(x + GetWidth() - iTextureWidth, y, iTextureWidth, iTextureWidth, Color(255, 255, 255, 255));
}

void CWeaponDescription::Highlighted(CFWeaponID eWeapon)
{
	SetActiveWeapon(eWeapon);
}

void CWeaponDescription::SetActiveWeapon(CFWeaponID eWeapon)
{
	m_ePaintedWeapon = eWeapon;
	
	CCFWeaponInfo* pInfo = CArmament::WeaponIDToInfo(eWeapon);

	SetText(VarArgs(
		"%s\n"
		" \n"
		"Slots required: %d\n"
		"%s\n"
		" \n"
		"Damage: %d\n"
		"Fire rate: %d rpm\n"
		"Cost: %d\n"
		" \n"
		"Attack bonus: +%d\n"
		"Energy bonus: +%d\n"
		" \n"
		"Accuracy: %.2f\n"
		"Atma regeneration: %d\n"
		"Stamina regeneration: %d\n",

		pInfo->szPrintName,
		pInfo->iWeight,
		pInfo->iWeight<2?"Secondary only":(pInfo->iWeight>2?"Primary only":"Primary/Secondary"),
		pInfo->m_iDamage,
		(int)(60/pInfo->m_flCycleTime),
		pInfo->m_iCost,
		(int)(pInfo->m_flAttack),
		(int)(pInfo->m_flEnergy),
		pInfo->m_flAccuracy,
		(int)(pInfo->m_flFocusRegen*10),
		(int)(pInfo->m_flStaminaRegen*10)
	));

	SetAlign(TA_TOPLEFT);
}

void CWeaponDescription::SetActiveCategory(int iCat)
{
	m_ePaintedWeapon = WEAPON_NONE;

	if (iCat == 0)
		SetText("Ranged weapons allow you to shoot people, but are weak in Numen.\n");
	else if (iCat == 1)
		SetText("Melee weapons are strong, and are moderately strong in Numen.\n");
	else
		SetText("Numen-enhancing weapons are physically weak but magnify the wielder's Numen.\n");

	SetAlign(TA_CENTER);
}

CWeaponGrid::CWeaponGrid()
	: CDroppablePanel(0, 0, 100, 100)
{
	for (int i = 0; i < 4; i++)
	{
		CDeleteButton* pDelete = new CDeleteButton();
		AddControl(pDelete);
		m_apDeleteButtons.AddToTail(pDelete);
	}
}

void CWeaponGrid::Layout()
{
	int i;
	int iSlot = 0;
	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	for (i = 0; i < 3; i++)
	{
		if (pArm->m_aWeapons[i].m_iWeapon != WEAPON_NONE)
		{
			for (int j = 0; j < pArm->GetWeaponData(i)->iWeight; j++)
			{
				m_apDeleteButtons[iSlot]->SetVisible(j == 0);
				m_apDeleteButtons[iSlot]->SetPos(BTN_BORDER+m_iW*iSlot/4, BTN_BORDER);
				m_apDeleteButtons[iSlot]->SetSize(32, 32);
				m_apDeleteButtons[iSlot]->SetClickedListener(this, &CWeaponGrid::Remove, new KeyValues("removeweapon", "weapon", i));
				iSlot++;
			}
		}
	}

	for (i = iSlot; i < 4; i++)
	{
		m_apDeleteButtons[i]->SetVisible(false);
	}
}

void CWeaponGrid::Paint(int x, int y, int w, int h)
{
	if (s_pGridBox)
	{
		s_pGridBox->DrawSelf(x, y, w/4, h, Color(255, 255, 255, 255));
		s_pGridBox->DrawSelf(x+w/4, y, w/4, h, Color(255, 255, 255, 255));
		s_pGridBox->DrawSelf(x+w*2/4, y, w/4, h, Color(255, 255, 255, 255));
		s_pGridBox->DrawSelf(x+w*3/4, y, w/4, h, Color(255, 255, 255, 255));
	}

	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	int i;
	int iSlot = 0;

	for (i = 0; i < 3; i++)
	{
		if (pArm->m_aWeapons[i].m_iWeapon != WEAPON_NONE)
		{
			CCFHudTexture* pTex = CTextureHandler::GetTexture(pArm->m_aWeapons[i].m_iWeapon, CTextureHandler::TT_WIDE);
			if (pTex)
				pTex->DrawSelf(x+w*iSlot/4, y, w*pArm->GetWeaponData(i)->iWeight/4, h, Color(255, 255, 255, 255));

			iSlot += pArm->GetWeaponData(i)->iWeight;
		}
	}

	for (i = 0; i < 4; i++)
		m_apDeleteButtons[i]->Paint();
}

const CRect CWeaponGrid::GetHoldingRect()
{
	CRect r;
	GetAbsDimensions(r.x, r.y, r.w, r.h);
	return r;
}

bool CWeaponGrid::CanDropHere(IDraggable* pDraggable)
{
	if (!pDraggable)
		return true;

	if (pDraggable->GetClass() != IDraggable::DC_WEAPONICON)
		return false;

	CWeaponIcon* pWeapon = (CWeaponIcon*)pDraggable;
	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	return pArm->CanBuyWeapon(pWeapon->GetWeapon()) >= 0;
}

void CWeaponGrid::AddDraggable(IDraggable* pDraggable)
{
	if (!CanDropHere(pDraggable))
		return;

	CWeaponIcon* pWeapon = (CWeaponIcon*)pDraggable;
	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	if (!pArm->BuyWeapon(pWeapon->GetWeapon()))
		return;

	CCFMenu::KillOnClose();

	CRootPanel::GetRoot()->Layout();
}

void CWeaponGrid::RemoveCallback(KeyValues* pParms)
{
	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	pArm->RemoveWeapon(pParms->GetFirstValue()->GetInt());

	CCFMenu::KillOnClose();

	CRootPanel::GetRoot()->Layout();
}

CWeaponChoice::CWeaponChoice(CFWeaponID eWeapon)
	: CDroppableChoice<CFWeaponID, CWeaponIcon>(eWeapon, IDraggable::DC_WEAPONICON)
{
	m_pNoCash = new CLabel(0, 0, 10, 10, "");
	AddControl(m_pNoCash);
	m_pNoCash->SetPos(0, 0);
	m_pNoCash->SetText("#No_Cash");
	m_pNoCash->SetFGColor(Color(255, 0, 0, 255));

#if defined(ARCON_MOD) || defined(ARCON_DEMO)
	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;
	if (!pArm->CanBuyWeaponInDemo(eWeapon))
		m_pNoCash->SetText("This weapon is not available in the demo version!");
#endif
}

void CWeaponChoice::Layout()
{
	CDroppableChoice<CFWeaponID, CWeaponIcon>::Layout();

	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;
	CCFWeaponInfo* pInfo = CArmament::WeaponIDToInfo(m_eDraggableID);

	SetGrabbale(pInfo && pInfo->m_iCost <= pArm->m_iCredits);

	m_pNoCash->SetSize(GetWidth(), GetHeight());
	m_pNoCash->SetVisible(!IsGrabbale());

#if defined(ARCON_MOD) || defined(ARCON_DEMO)
	if (!pArm->CanBuyWeaponInDemo(m_eDraggableID))
	{
		SetGrabbale(false);
		m_pNoCash->SetVisible(true);
	}
#endif
}

void CWeaponChoice::Paint(int x, int y, int w, int h)
{
	if (!IsVisible())
		return;

	CCFWeaponInfo* pInfo = CArmament::WeaponIDToInfo(m_eDraggableID);

	Assert(pInfo);

	if (pInfo && CWeaponGrid::s_pGridBox)
	{
		for (int i = 0; i < pInfo->iWeight; i++)
			CWeaponGrid::s_pGridBox->DrawSelf(x + i*h, y, h, h, Color(255, 255, 255, 255));
	}

	CDroppableChoice<CFWeaponID, CWeaponIcon>::Paint(x, y, w, h);
}

CWeapPanel::CWeapPanel()
	: CPanel(0, BTN_HEIGHT + BTN_BORDER, PANEL_WIDTH, PANEL_HEIGHT)
{
	int tlX = 0, tlY = 0, brX = 0, brY = 0;
	GetBR(brX, brY);
	tlX = m_iX;
	tlY = m_iY;

	char* aszNames[] =
	{
		"Ranged",
		"Melee",
		"TBD",
	};

	int iWeaponsHeight = PANEL_HEIGHT - BTN_HEIGHT - BTN_BORDER*3;
	int iWeaponsWidth = BTN_WIDTH*3 + BTN_BORDER*2;
	int iInfoHeight = PANEL_HEIGHT - BTN_HEIGHT - BTN_BORDER*3;
	int iInfoWidth = PANEL_WIDTH - iWeaponsWidth - BTN_BORDER*3;

	CButton* pButton;
	int i;
	for (i = 0; i < 3; i++)
	{
		pButton = new CButton(
			BTN_BORDER + BTN_BORDER*i + BTN_WIDTH*i,
			BTN_BORDER,
			BTN_WIDTH, BTN_HEIGHT, aszNames[i], true);
		pButton->SetClickedListener(this, &CWeapPanel::Panel, new KeyValues("choosepanel", "panel", i));
		pButton->SetUnclickedListener(this, &CWeapPanel::Panel, new KeyValues("choosepanel", "panel", i));
		m_apCats.AddToTail(pButton);
		AddControl(pButton);
	}

	m_pChoose = new CLabel(iWeaponsWidth + BTN_BORDER*2, BTN_BORDER, iInfoWidth, BTN_HEIGHT, "#Choose_Weapon");
	m_pChoose->SetAlign(CLabel::TA_LEFTCENTER);

	AddControl(m_pChoose);

	m_pWeapons	= new CPanel(BTN_BORDER, BTN_HEIGHT + BTN_BORDER*2, iWeaponsWidth, iWeaponsHeight);
	m_pInfo		= new CPanel(BTN_BORDER + iWeaponsWidth + BTN_BORDER, BTN_HEIGHT + BTN_BORDER*2, iInfoWidth, iInfoHeight);

	AddControl(m_pWeapons);
	AddControl(m_pInfo);

	m_pDesc = new CWeaponDescription();
	m_pInfo->AddControl(m_pDesc);

	CWeaponChoice* pWeap;
	for (i = 0; i < 6; i++)
	{
		if (s_aeWeaponSlots[0][i] != WEAPON_NONE)
		{
			pWeap = new CWeaponChoice(s_aeWeaponSlots[0][i]);
			pWeap->SetHighlightListener(m_pDesc);
			m_apRangedWeapons.AddToTail(pWeap);
			m_pWeapons->AddControl(pWeap);
		}

		if (s_aeWeaponSlots[0][i] != WEAPON_NONE)
		{
			pWeap = new CWeaponChoice(s_aeWeaponSlots[1][i]);
			pWeap->SetHighlightListener(m_pDesc);
			m_apMeleeWeapons.AddToTail(pWeap);
			m_pWeapons->AddControl(pWeap);
		}

		if (s_aeWeaponSlots[0][i] != WEAPON_NONE)
		{
			pWeap = new CWeaponChoice(s_aeWeaponSlots[2][i]);
			pWeap->SetHighlightListener(m_pDesc);
			m_apMagicWeapons.AddToTail(pWeap);
			m_pWeapons->AddControl(pWeap);
		}
	}

	m_pGrid = new CWeaponGrid();
	m_pInfo->AddControl(m_pGrid);

	m_iCurrentCat = 0;
	SetPanel(m_iCurrentCat);

	Layout();
}

void CWeapPanel::Layout()
{
	SetSize(PANEL_WIDTH, PANEL_HEIGHT);
	SetPos(50, m_iY);

	int iWeaponsHeight = PANEL_HEIGHT - BTN_HEIGHT - BTN_BORDER*3;
	int iWeaponsWidth = BTN_WIDTH*3 + BTN_BORDER*2;
	int iInfoHeight = PANEL_HEIGHT - BTN_HEIGHT - BTN_BORDER*3;
	int iInfoWidth = PANEL_WIDTH - iWeaponsWidth - BTN_BORDER*3;

	m_pChoose->SetSize(iInfoWidth, BTN_HEIGHT);

	m_pWeapons->SetPos(BTN_BORDER, BTN_HEIGHT + BTN_BORDER*2);
	m_pInfo->SetPos(BTN_BORDER + iWeaponsWidth + BTN_BORDER, BTN_HEIGHT + BTN_BORDER*2);
	m_pWeapons->SetSize(iWeaponsWidth, iWeaponsHeight);
	m_pInfo->SetSize(iInfoWidth, iInfoHeight);

	// Remove me when we have more weapons.
	m_apCats[0]->SetVisible(false);
	m_apCats[1]->SetVisible(false);

	// Remove me when we come up with a third category.
	m_apCats[2]->SetVisible(false);

	for (int i = 0; i < 6; i++)
	{
		if (m_apRangedWeapons.IsValidIndex(i))
		{
			m_apRangedWeapons[i]->SetVisible(m_iCurrentCat==0?true:false);
			LayoutChoice(m_apRangedWeapons[i], i);
		}

		if (m_apMeleeWeapons.IsValidIndex(i))
		{
			m_apMeleeWeapons[i]->SetVisible(m_iCurrentCat==1?true:false);
			LayoutChoice(m_apMeleeWeapons[i], i);
		}

		if (m_apMagicWeapons.IsValidIndex(i))
		{
			m_apMagicWeapons[i]->SetVisible(m_iCurrentCat==2?true:false);
			LayoutChoice(m_apMagicWeapons[i], i);
		}
	}

	int iGridSize = (iInfoWidth - BTN_BORDER*5)/4;

	m_pGrid->SetPos(BTN_BORDER, BTN_BORDER);
	m_pGrid->SetSize(iInfoWidth - BTN_BORDER*2, iGridSize);

	m_pDesc->SetPos(BTN_BORDER, iGridSize + BTN_BORDER*2);
	m_pDesc->SetSize(iInfoWidth - BTN_BORDER*2, iGridSize - BTN_BORDER*2);

	CPanel::Layout();
}

void CWeapPanel::LayoutChoice(CWeaponChoice* pChoice, int i)
{
	if (pChoice->m_eDraggableID == WEAPON_NONE)
		return;	// Seeya.

	// Six rows.
	int iHeight = (m_pWeapons->GetHeight()-BTN_BORDER*2)/6;
	// One column.
	int iWidth = (m_pWeapons->GetWidth()-BTN_BORDER*2);

	int iUnit = iWidth/4;

	pChoice->m_iDraggableWidth = iUnit;

	CCFWeaponInfo* pInfo = CArmament::WeaponIDToInfo(pChoice->m_eDraggableID);

	Assert(pInfo);

	int iWeight = 1;
	if (pInfo)
		iWeight = pInfo->iWeight;

	pChoice->SetPos(BTN_BORDER, BTN_BORDER + iHeight*i);
	pChoice->SetSize(iUnit*iWeight, iUnit);
}

void CWeapPanel::Paint(int x, int y, int w, int h)
{
	if (!IsVisible())
		return;

	CPanel::Paint(x, y, w, h);
}

void CWeapPanel::SetPanel(int iPanel)
{
	if (iPanel < 0 || iPanel > 2)
		return;

	m_iCurrentCat = iPanel;

	int i;
	for (i = 0; i < m_apCats.Count(); i++)
		m_apCats[i]->SetToggleState(false);

	m_apCats[iPanel]->SetToggleState(true);

	for (i = 0; i < 6; i++)
	{
		if (m_apRangedWeapons.IsValidIndex(i))
			m_apRangedWeapons[i]->SetVisible(m_iCurrentCat==0?true:false);
	
		if (m_apMeleeWeapons.IsValidIndex(i))
			m_apMeleeWeapons[i]->SetVisible(m_iCurrentCat==1?true:false);

		if (m_apMagicWeapons.IsValidIndex(i))
			m_apMagicWeapons[i]->SetVisible(m_iCurrentCat==2?true:false);
	}

}

void CWeapPanel::PanelCallback(KeyValues* pParms)
{
	m_pDesc->SetActiveCategory(pParms->GetFirstValue()->GetInt());
	SetPanel(pParms->GetFirstValue()->GetInt());
}
