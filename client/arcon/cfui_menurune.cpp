#include "cbase.h"
#include "cfui_menu.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define BTN_BIND_SPACE 6
#define BTN_BIND_WIDTH 20

CRunePanel* CRunePanel::s_pRunePanel = NULL;
int CRunePanel::s_iActiveArmament = 0;
int CRunePanel::s_iActiveCombo = 0;

CCFHudTexture* CRuneLayoutPanel::s_pMouseButtons = NULL;
CCFHudTexture* CRuneLayoutPanel::s_pMouseButtonsLMB = NULL;
CCFHudTexture* CRuneLayoutPanel::s_pMouseButtonsRMB = NULL;

CSelectableRuneCombo::CSelectableRuneCombo(int iWeapon, int iRune)
	: CGUIRuneCombo(iWeapon, iRune, true)
{
	m_iWeapon = iWeapon;
	m_iRune = iRune;

	m_bActive = false;
	m_bHovering = false;
	m_flGoalSize = m_flCurrSize = 1;
	m_flDeleteGoalSize = m_flDeleteCurrSize = 0.5f;

	m_pRuneSlotTexture = CTextureHandler::GetRuneSlotTexture();

	m_Dim = CRect(0, 0, 100, 100);

	m_pHasCursor = NULL;

	m_pDelete = new CDeleteButton();
	m_pDelete->SetParent(this);
	m_pDelete->SetSize(32, 32);
	m_pDelete->SetClickedListener(this, &CSelectableRuneCombo::Remove);

	CRootPanel::GetRoot()->AddDroppable(this);
}

void CSelectableRuneCombo::Destructor()
{
	if (CRootPanel::GetRoot())
		CRootPanel::GetRoot()->RemoveDroppable(this);

	CGUIRuneCombo::Destructor();

	m_pDelete->SetParent(NULL);
	m_pDelete->Destructor();
	m_pDelete->Delete();
}

void CSelectableRuneCombo::Think()
{
	if (!C_BasePlayer::GetLocalPlayer())
		return;

	CGUIRuneCombo::Think();
	m_flCurrSize = Approach(m_flGoalSize, m_flCurrSize, gpGlobals->frametime*5);
	m_flDeleteCurrSize = Approach(m_flDeleteGoalSize, m_flDeleteCurrSize, gpGlobals->frametime*5/2);

	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	m_pDelete->SetPos(BTN_BORDER, BTN_BORDER);
	m_pDelete->SetVisible(IsVisible() && pArm->GetRunesInSlot(m_iWeapon, m_iRune));
}

void CSelectableRuneCombo::Paint(int x, int y)
{
	Paint(x, y, m_iW*m_flCurrSize, m_iH*m_flCurrSize);
}

void CSelectableRuneCombo::Paint(int x, int y, int w, int h)
{
	if (!IsVisible())
		return;

	if (m_bActive)
		// WEAPON_MAGIC is the highlight.
		CTextureHandler::GetTexture(WEAPON_MAGIC)->DrawSelf(x, y, w, h, Color(255, 255, 255, 255));

	m_pRuneSlotTexture->DrawSelf(
		x,
		y,
		w,
		h, Color(255, 255, 255, 255));

	CGUIRuneCombo::Paint(x, y, w, h);

	int dx, dy;
	m_pDelete->GetAbsPos(dx, dy);
	m_pDelete->Paint(dx, dy, m_pDelete->GetWidth()*m_flDeleteCurrSize, m_pDelete->GetHeight()*m_flDeleteCurrSize);
}

CRect CSelectableRuneCombo::GetAbsCenter()
{
	int x = 0, y = 0, w = 0, h = 0;
	GetAbsDimensions(x, y, w, h);
	return CRect(x+w/2, y+h/2, w/2, h/2);
}

void CSelectableRuneCombo::CursorIn()
{
	CGUIRuneCombo::CursorIn();
	m_flGoalSize = 2;
	m_flDeleteGoalSize = 1;
	SetHovering(true);
}

void CSelectableRuneCombo::CursorOut()
{
	CGUIRuneCombo::CursorOut();
	m_flGoalSize = 1;
	m_flDeleteGoalSize = 0.5f;
	SetHovering(false);

	if (m_pHasCursor)
	{
		m_pHasCursor->CursorOut();
		m_pHasCursor = NULL;
	}
}

bool CSelectableRuneCombo::MousePressed(vgui::MouseCode code)
{
	int mx, my;
	CRootPanel::GetFullscreenMousePos(mx, my);

	if (m_pDelete->IsVisible())
	{
		int x = 0, y = 0, w = 0, h = 0;
		m_pDelete->GetAbsDimensions(x, y, w, h);
		if (mx >= x &&
			my >= y &&
			mx < x + w &&
			my < y + h)
		{
			if (m_pDelete->MousePressed(code))
				return true;
		}
	}

	// If I could just get those TPS reports 
	GetMaster()->RequestFocus(this);
	return true;
}

bool CSelectableRuneCombo::MouseReleased(vgui::MouseCode code)
{
	int mx, my;
	CRootPanel::GetFullscreenMousePos(mx, my);

	if (m_pDelete->IsVisible())
	{
		int x = 0, y = 0, w = 0, h = 0;
		m_pDelete->GetAbsDimensions(x, y, w, h);
		if (mx >= x &&
			my >= y &&
			mx < x + w &&
			my < y + h)
		{
			if (m_pDelete->MouseReleased(code))
				return true;
		}
	}

	return false;
}

void CSelectableRuneCombo::CursorMoved(int mx, int my)
{
	if (m_pDelete->IsVisible() && m_pDelete->IsCursorListener())
	{
		int x, y, w, h;
		m_pDelete->GetAbsDimensions(x, y, w, h);
		if (mx >= x &&
			my >= y &&
			mx < x + w &&
			my < y + h)
		{
			if (m_pHasCursor != m_pDelete)
			{
				if (m_pHasCursor)
				{
					m_pHasCursor->CursorOut();
				}
				m_pHasCursor = m_pDelete;
				m_pHasCursor->CursorIn();
			}

			m_pDelete->CursorMoved(mx, my);
		}
	}
}

const CRect CSelectableRuneCombo::GetHoldingRect()
{
	int x, y, w, h;
	GetAbsDimensions(x, y, w, h);
	m_Dim.x = x;
	m_Dim.y = y;
	m_Dim.w = w;
	m_Dim.h = h;

	return m_Dim;
}

bool CSelectableRuneCombo::CanDropHere(IDraggable* pDraggable)
{
	if (!GetMaster()->GetRuneDrop(0))
		return false;

	for (int i = 0; i < MAX_RUNE_MODS; i++)
	{
		if (pDraggable->GetClass() != IDraggable::DC_RUNEICON)
			continue;

		CDraggableIcon<RuneID>* pIcon = (CDraggableIcon<RuneID>*)pDraggable;

		CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

		if (!pArm->CanBuyRune(pIcon->GetWeapon(), m_iWeapon, m_iRune, i))
			continue;

		return true;
	}
	return false;
}

void CSelectableRuneCombo::AddDraggable(IDraggable* pDraggable)
{
	GetMaster()->RequestFocus(this);

	// Just pass it along to the first child that can eat it.
	for (int i = 0; i < MAX_RUNE_MODS; i++)
	{
		if (GetMaster()->GetRuneDrop(i) && GetMaster()->GetRuneDrop(i)->CanDropHere(pDraggable))
		{
			GetMaster()->GetRuneDrop(i)->SetDraggable(pDraggable);
			return;
		}
	}
}

void CSelectableRuneCombo::SetDraggable(IDraggable* pDraggable, bool bDelete)
{
	AddDraggable(pDraggable);
}

void CSelectableRuneCombo::RemoveCallback(KeyValues* pParms)
{
	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;
	pArm->RemoveCombo(m_iWeapon, m_iRune);

	CCFMenu::KillOnClose();

	CRootPanel::GetRoot()->Layout();
}

CRuneCombosPanel::CRuneCombosPanel(CRuneLayoutPanel* pRunePanel, int iWeapon)
	: CPanel(0, BTN_HEIGHT + BTN_BORDER, PANEL_WIDTH, PANEL_HEIGHT)
{
	m_pRunePanel = pRunePanel;
	m_iWeapon = iWeapon;
	m_iActiveCombo = -1;

	SetBorder(CPanel::BT_NONE);

	m_pYourCombos = new CLabel(0, 0, 16, 16, "#Your_Combos");
	AddControl(m_pYourCombos);

	m_pYourCombos->SetWrap(false);

	for (int i = 0; i < 6; i++)
	{
		m_apRuneComboPanels.AddToTail(new CSelectableRuneCombo(iWeapon, i));
		AddControl(m_apRuneComboPanels[i]);
		m_apRuneComboPanels[i]->SetMaster(this);

		m_apXLabels.AddToTail(new CLabel(0, 0, 16, 16, ""));
		AddControl(m_apXLabels[i]);
	}
}

void CRuneCombosPanel::Destructor()
{
	CPanel::Destructor();
}

void CRuneCombosPanel::Layout()
{
	if (!IsVisible())
		return;

	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	int iHeight = GetHeight();
	if (GetParent())
		iHeight = GetParent()->GetHeight();

	int iTotalRunes = pArm->GetRuneCombos(m_iWeapon);

	float flMinWidth = ((float)iHeight - BTN_BORDER*(iTotalRunes+1))/(iTotalRunes+2);

	m_flSlotWidth = min(flMinWidth, 80);

	float flTotalHeight = m_flSlotWidth*iTotalRunes + (iTotalRunes-1)*BTN_BORDER;
	float flStartHeight = (iHeight - flTotalHeight)/2;

	SetPos(0, flStartHeight);
	SetSize(m_flSlotWidth, flTotalHeight);

	int iCount = m_apRuneComboPanels.Count();
	for (int i = 0; i < iCount; i++)
	{
		CSelectableRuneCombo* pCombo = m_apRuneComboPanels[i];
		pCombo->SetPos(0, i*m_flSlotWidth + i*BTN_BORDER);
		pCombo->SetSize(m_flSlotWidth, m_flSlotWidth);
		pCombo->SetVisible(pArm->GetRunesMaximum(m_iWeapon, i) > 0);
		pCombo->SetNumen(pArm->SerializeCombo(m_iWeapon, i));

		int iX, iY;
		pCombo->GetBR(iX, iY);
		m_apXLabels[i]->SetPos(iX, iY);
		m_apXLabels[i]->SetVisible(pCombo->IsVisible());
		m_apXLabels[i]->SetText(VarArgs("#x%d", pArm->GetRunesMaximum(m_iWeapon, i)));
		m_apXLabels[i]->SetWrap(false);
	}

	if (CRunePanel::GetActiveArmament() == m_iWeapon)
		RequestFocus(m_apRuneComboPanels[CRunePanel::GetActiveCombo()]);
	if (m_iActiveCombo != -1)
		RequestFocus(m_apRuneComboPanels[m_iActiveCombo]);
	else
		// If none are active make the first active.
		RequestFocus(m_apRuneComboPanels[0]);

	CPanel::Layout();
}

void CRuneCombosPanel::Paint(int x, int y, int w, int h)
{
	if (!IsVisible())
		return;

	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;
	int iTotalRunes = pArm->GetRuneCombos(m_iWeapon);

	int mx, my, i;
	CRootPanel::GetFullscreenMousePos(mx, my);

	float flTotalHeight = (iTotalRunes-1)*BTN_BORDER;

	int iCount = m_apRuneComboPanels.Count();
	for (i = 0; i < iCount; i++)
	{
		if (m_apRuneComboPanels[i]->IsVisible())
			flTotalHeight += m_apRuneComboPanels[i]->GetCurrSize();
	}

	float flStartHeight = (GetHeight() - flTotalHeight)/2;
	float flNextY = flStartHeight;

	int iLabelHeight = 10;

	m_pYourCombos->Paint(x, y + flNextY - BTN_HEIGHT, BTN_WIDTH, BTN_HEIGHT);

	for (i = 0; i < iCount; i++)
	{
		m_apRuneComboPanels[i]->Paint(x, y + flNextY, m_apRuneComboPanels[i]->GetCurrSize(), m_apRuneComboPanels[i]->GetCurrSize());
		m_apXLabels[i]->Paint(x + m_apRuneComboPanels[i]->GetCurrSize() - iLabelHeight, y + flNextY + m_apRuneComboPanels[i]->GetCurrSize() - iLabelHeight, iLabelHeight, iLabelHeight);
		flNextY += m_apRuneComboPanels[i]->GetCurrSize() + BTN_BORDER;
	}
}

void CRuneCombosPanel::RequestFocus(CSelectableRuneCombo* pRequesting)
{
	if (pRequesting->IsActive())
		return;

	for (int i = 0; i < m_apRuneComboPanels.Count(); i++)
		m_apRuneComboPanels[i]->SetActive(false);

	if (!pRequesting)
	{
		CRunePanel::SetActive(-1, -1);
		m_iActiveCombo = -1;
		return;
	}

	int iIndex = m_apRuneComboPanels.Find(pRequesting);
	if (!m_apRuneComboPanels.IsValidIndex(iIndex))
		return;

	m_iActiveCombo = iIndex;
	CRunePanel::SetActive(m_iWeapon, iIndex);
	m_apRuneComboPanels[CRunePanel::GetActiveCombo()]->SetActive(true);

	int x = 0, y = 0;
	GetAbsPos(x, y);

	float flCurrSize = m_apRuneComboPanels[CRunePanel::GetActiveCombo()]->GetCurrSize();
	m_pRunePanel->SetRune(CRunePanel::GetActiveCombo(), true, CRect(x, m_apRuneComboPanels[CRunePanel::GetActiveCombo()]->GetAbsCenter().y-flCurrSize/2, flCurrSize, flCurrSize));
}

CRuneDrop* CRuneCombosPanel::GetRuneDrop(int iMod)
{
	return m_pRunePanel->GetRuneDropPanel(iMod)->GetDrop();
}

CRuneIcon::CRuneIcon(IDroppable* pParent, RuneID eDraggable, DragClass_t eDragClass)
	: CDraggableIcon<RuneID>(pParent, eDraggable, eDragClass)
{
	m_bDrawSmaller = false;
	m_bDrawSolid = false;
}

void CRuneIcon::Paint(int x, int y, int w, int h)
{
	if (m_bDrawSmaller && CRuneData::GetData(m_eDraggableID)->m_eType == RUNETYPE_BASE)
	{
		float flTwo3rds = ((float)m_iW + (float)m_iH)/3;

		CDraggableIcon<RuneID>::Paint(x + (m_iW-flTwo3rds)/2, y + (m_iH-flTwo3rds)/2, flTwo3rds, flTwo3rds);
	}
	else
	{
		CDraggableIcon<RuneID>::Paint(x, y, w, h);
	}
}

IDraggable& CRuneIcon::MakeCopy()
{
	CRuneIcon* pIcon = new CRuneIcon(*this);
	return *pIcon;
}

int CRuneIcon::GetPaintAlpha()
{
	if (m_bDrawSolid)
		return 255;

	return CDraggableIcon<RuneID>::GetPaintAlpha();
}

CRuneDrop::CRuneDrop(int iWeapon, int iRune, int iMod)
	: CDroppableIcon<CRuneIcon>(IDraggable::DC_RUNEICON, CRect(0, 0, 100, 100))
{
	m_iWeapon = iWeapon;
	m_iRune = iRune;
	m_iMod = iMod;
}

void CRuneDrop::SetDraggable(IDraggable* pDragged, bool bDelete)
{
	// CRuneIcons should be allowed to draw themselves slightly smaller
	// (as base runes are wont to do) when placed in a CRuneDrop.
	// pDragged should only ever be a CRuneIcon. If not that's a problem.
	for (int i = 0; i < m_apDraggables.Count(); i++)
		((CRuneIcon*)m_apDraggables[i])->DrawSmaller(false);

	CDroppableIcon<CRuneIcon>::SetDraggable(pDragged, bDelete);

	if (pDragged)
		((CRuneIcon*)pDragged)->DrawSmaller(true);
}

void CRuneDrop::DraggableChanged(CRuneIcon* pDragged)
{
	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	CRunePanel::SetActive(m_iWeapon, m_iRune);

	if (pDragged)
		pDragged->DrawSolid(true);

	CRunePosition DPos(m_iWeapon, m_iRune, m_iMod);

	bool bBindableBefore = pArm->IsBindableCombo(&DPos);

	if (pDragged)
		pArm->BuyRune(pDragged->GetWeapon(), m_iWeapon, m_iRune, m_iMod);
	else
		pArm->RemoveRune(m_iWeapon, m_iRune, m_iMod);

	CCFMenu::KillOnClose();

	// HE CAN BE TAUGHT!!!
	if (pArm->IsBindableCombo(&DPos) && !bBindableBefore)
		C_CFPlayer::GetLocalCFPlayer()->Instructor_LessonLearned(HINT_ONE_FORCE_BASE);

	// If there is a free attack, bind the attack to it.
	if (pArm->GetAttackBind(m_iWeapon, m_iRune) == -1)
	{
		for (int i = 0; i < ATTACK_BINDS; i++)
		{
			if (pArm->m_aAttackBinds[i].m_iWeapon == -1 && pArm->CanBind(i, m_iWeapon, m_iRune))
			{
				pArm->Bind(i, m_iWeapon, m_iRune);
				break;
			}
		}
	}

	CRootPanel::UpdateArmament(pArm);

	if (GetParent())
		GetParent()->Layout();
}

bool CRuneDrop::CanDropHere(IDraggable* pDraggable)
{
	// Check the superclass to make sure we have the correct drag class before we execute the cast below.
	if (!CDroppableIcon<CRuneIcon>::CanDropHere(pDraggable))
		return false;

	if (!pDraggable)
		return true;

	CRuneIcon* pWeapon = (CRuneIcon*)pDraggable;

	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	if (!pArm->CanBuyRune(pWeapon->GetWeapon(), m_iWeapon, m_iRune, m_iMod))
		return false;

	return true;
}

void CRuneDrop::SetRune(int iRune)
{
	m_iRune = iRune;

	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	RuneID NewRune = pArm->GetRuneID(m_iWeapon, m_iRune, m_iMod);
	if (NewRune == CRuneData::InvalidRune())
		SetDraggableIcon(NULL);
	else
	{
		CRuneIcon* pIcon = new CRuneIcon(this, NewRune, IDraggable::DC_RUNEICON);
		pIcon->DrawSmaller(true);
		pIcon->DrawSolid(true);
		SetDraggableIcon(pIcon);
	}
}

void CRuneDrop::RemoveCallback(KeyValues* pParms)
{
	SetDraggable(NULL);
}

CRuneDropPanel::CRuneDropPanel(int iWeapon, int iRune, int iMod)
	: CPanel(0, 0, 100, 100)
{
	m_iWeapon = iWeapon;
	m_iRune = iRune;
	m_iMod = iMod;

	SetBorder(CPanel::BT_NONE);

	// Delete button first so that it receives mouse messages ahead of CRuneDrop.
	m_pDelete = new CDeleteButton();
	AddControl(m_pDelete);

	m_pDrop = new CRuneDrop(iWeapon, iRune, iMod);
	AddControl(m_pDrop);

	m_pDropWhat = new CLabel(0, 0, 16, 16, "#Drop_Numen_Here");
	AddControl(m_pDropWhat);

	m_pDelete->SetClickedListener(m_pDrop, &CRuneDrop::Remove);

	m_pRuneSlotTexture = CTextureHandler::GetRuneSlotTexture();

	Layout();
}

void CRuneDropPanel::Layout()
{
	Assert(m_pDrop);

	CRect HoldingRect(0, 0, 0, 0);
	GetAbsDimensions(HoldingRect.x, HoldingRect.y, HoldingRect.w, HoldingRect.h);
	HoldingRect.h = HoldingRect.w;	// Kind of a hack... this lops off the area for the buttons on the bottom.
	m_pDrop->SetHoldingRect(HoldingRect);
	m_pDrop->SetSize(HoldingRect.w, HoldingRect.h);

	float flHeight = ((float)m_iW)/4;

	m_pDropWhat->SetPos(0, 0);
	m_pDropWhat->SetSize(HoldingRect.w, HoldingRect.h);
	m_pDropWhat->SetText("#Drop_Numen_Here");
	m_pDropWhat->SetVisible(!m_pDrop->GetDraggableIcon());

	m_pDelete->SetPos(flHeight*3, m_iH-flHeight);
	m_pDelete->SetSize(flHeight, flHeight);
	m_pDelete->SetVisible(m_pDrop->GetDraggableIcon() != NULL);

	CPanel::Layout();
}

void CRuneDropPanel::Paint(int x, int y, int w, int h)
{
	if (!IsVisible())
		return;

	IDraggable* pDragging = CRootPanel::GetRoot()->GetCurrentDraggable();

	if (pDragging && m_pDrop->CanDropHere(pDragging))
		// WEAPON_MAGIC is the highlight.
		CTextureHandler::GetTexture(WEAPON_MAGIC)->DrawSelf(x, y, w, w, Color(255, 255, 255, 255));

	m_pRuneSlotTexture->DrawSelf(
		x,
		y,
		w,
		w, Color(255, 255, 255, 255));

	CPanel::Paint(x, y, w, h);
}

bool CRuneDropPanel::MousePressed(vgui::MouseCode code)
{
	if (m_pDrop->MousePressed(code))
		return true;
	
	return CPanel::MousePressed(code);
}

void CRuneDropPanel::SetRune(int iRune)
{
	m_iRune = iRune;
	m_pDrop->SetRune(iRune);
}

const float CRuneLayoutPanel::s_flShiftTime = 0.2f;
const float CRuneLayoutPanel::s_flSwirlTime = 0.25f;

CRuneLayoutPanel::CRuneLayoutPanel(CRuneAdvancedPanel* pRunePanel, int iWeapon)
	: CPanel(0, BTN_HEIGHT + BTN_BORDER, PANEL_WIDTH, PANEL_HEIGHT)
{
	m_pRunePanel = pRunePanel;
	m_iWeapon = iWeapon;
	m_iRune = 0;
	m_flComboSourceTime = 0;

	m_pRuneCombo = new CGUIRuneCombo(m_iWeapon, m_iRune, true);
	AddControl(m_pRuneCombo);

	SetBorder(CPanel::BT_NONE);

	int i;

	for (i = 0; i < 6; i++)
	{
		m_apRuneDropPanels.AddToTail(new CRuneDropPanel(iWeapon, 0, i));
		AddControl(m_apRuneDropPanels[i]);
	}

	m_pAttackButton = new CLabel(0, 0, BTN_BIND_SPACE*3 + BTN_BIND_WIDTH*4, 20, "");
	AddControl(m_pAttackButton);

	m_pBasePresent = new CLabel(0, 0, BTN_WIDTH, BTN_HEIGHT, "");
	AddControl(m_pBasePresent);

	m_pForcePresent = new CLabel(0, 0, BTN_WIDTH, BTN_HEIGHT, "");
	AddControl(m_pForcePresent);

	m_pBaseCheckmark = new CCheckmark();
	AddControl(m_pBaseCheckmark);

	m_pForceCheckmark = new CCheckmark();
	AddControl(m_pForceCheckmark);

	for (i = 0; i < ATTACK_BINDS; i++)
	{
		CButton* pButton = new CButton(BTN_BIND_SPACE + BTN_BORDER, BTN_BIND_SPACE + BTN_BORDER, BTN_BIND_WIDTH, BTN_BIND_WIDTH, "", true);
		m_apAttackBinds.AddToTail(pButton);
		AddControl(pButton);
		pButton->SetUnclickedListener(this, &CRuneLayoutPanel::Unbind, new KeyValues("unbind", "attack", i));
	}

	m_pInfo = new CLabel(0, 0, BTN_WIDTH, BTN_HEIGHT, "");
	AddControl(m_pInfo);

	SetRune(0);
}

void CRuneLayoutPanel::Layout()
{
	if (!IsVisible())
		return;

	int iWidth = GetWidth();
	int iHeight = GetHeight();

	float flMinWidth = ((float)iWidth - BTN_BORDER*5)/6;
	float flMinHeight = ((float)iHeight - RUNE_BUFFER)/2;

	m_flSlotWidth = min(flMinWidth, flMinHeight);

	SetPos(100, 0);
	if (GetParent())
		SetSize(GetParent()->GetWidth()-100, GetParent()->GetHeight());

	m_iAttackBindsX = GetWidth() - 4*(BTN_BIND_SPACE + BTN_BIND_WIDTH) - BTN_BORDER*3;
	m_iAttackBindsY = GetHeight() - BTN_BIND_WIDTH - BTN_HEIGHT - BTN_BORDER*3;

	m_pRuneCombo->SetDimensions(m_iAttackBindsX/2-m_flSlotWidth/2, GetHeight()/2-m_flSlotWidth/2, m_flSlotWidth, m_flSlotWidth);

	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	int iMaxRunes = pArm->GetRunesMaximum(m_iWeapon, m_iRune);
	float flCenterX, flCenterY, flHypotenuse;
	flCenterX = m_iAttackBindsX/2;
	flCenterY = GetHeight()/2;
	flHypotenuse = flCenterY - m_flSlotWidth/2;		// Radius is the distance to the center minus half the width of one rune so that runes lie on the outside without overlapping.

	unsigned int iCount = (unsigned int) m_apRuneDropPanels.Count();
	unsigned int i;
	for (i = 0; i < iCount; i++)
	{
		float flDegrees = 2*M_PI*i/iMaxRunes - 90*M_PI/180;		// Rotate left so the first one is on top.
		m_apRuneDropPanels[i]->SetPos(flCenterX + cos(flDegrees) * flHypotenuse - m_flSlotWidth/2, flCenterY + sin(flDegrees) * flHypotenuse - m_flSlotWidth/2);
		m_apRuneDropPanels[i]->SetSize(m_flSlotWidth, m_flSlotWidth);	// A little extra on the bottom for the buttons.
		m_apRuneDropPanels[i]->SetVisible(pArm->GetRunesMaximum(m_iWeapon, m_iRune) > i);
	}

	int iBindButtonsWidth = BTN_BIND_WIDTH*2 + BTN_BIND_SPACE;
	for (i = 0; i < ATTACK_BINDS; i++)
	{
		m_apAttackBinds[i]->SetPos((m_iAttackBindsX + GetWidth())/2 - iBindButtonsWidth/2 + i*(BTN_BIND_SPACE + BTN_BIND_WIDTH), m_iAttackBindsY + BTN_BIND_SPACE);
		m_apAttackBinds[i]->SetSize(BTN_BIND_WIDTH, BTN_BIND_WIDTH);
		m_apAttackBinds[i]->SetText(VarArgs("#Bind_%d", i+1));
		m_apAttackBinds[i]->SetEnabled(pArm->CanBind(i, m_iWeapon, m_iRune));
		m_apAttackBinds[i]->SetToggleState(pArm->m_aAttackBinds[i].m_iWeapon == m_iWeapon && pArm->m_aAttackBinds[i].m_iRune == m_iRune);
	}
	m_pAttackButton->SetPos(m_iAttackBindsX, m_iAttackBindsY + BTN_BIND_SPACE + BTN_BIND_WIDTH);
	m_pAttackButton->SetSize(GetWidth()-m_iAttackBindsX, BTN_BIND_WIDTH);
	m_pAttackButton->SetText("#Attack_Binding");
	m_pAttackButton->SetAlign(CLabel::TA_CENTER);

	m_pInfo->SetPos(GetWidth()-128, 128);
	m_pInfo->SetSize(128, GetHeight()-(m_iAttackBindsY-128));
	m_pInfo->SetText("#Rune_info");
	m_pInfo->SetAlign(CLabel::TA_TOPLEFT);

	int iCheckmarkWidth = 32;
	int iAboveAttackBinds = BTN_BIND_WIDTH*2 + BTN_HEIGHT + BTN_BORDER*4;
	// Start with the point on the left of the attack binds,
	// add in half of the width of attack binds, and remove half of the width of the checkmark boxes
	// to arrive at the point the checkmarks should be centered relative to the boxes.
	int iCheckmarksX = m_iAttackBindsX + (3*BTN_BIND_SPACE + 4*BTN_BIND_WIDTH)/2 - iCheckmarkWidth;

	m_pBasePresent->SetPos(iCheckmarksX, GetHeight() - iAboveAttackBinds - BTN_BORDER);
	m_pBasePresent->SetSize(BTN_HEIGHT, iCheckmarkWidth);
	m_pBasePresent->SetText("#Base");
	m_pBasePresent->SetWrap(false);

	m_pForcePresent->SetPos(iCheckmarksX + iCheckmarkWidth + BTN_BORDER, GetHeight() - iAboveAttackBinds - BTN_BORDER);
	m_pForcePresent->SetSize(BTN_HEIGHT, iCheckmarkWidth);
	m_pForcePresent->SetText("#Force");
	m_pForcePresent->SetWrap(false);

	m_pBaseCheckmark->SetPos(iCheckmarksX, GetHeight() - iAboveAttackBinds - BTN_BORDER - BTN_HEIGHT);
	m_pBaseCheckmark->Mark(pArm->HasType(RUNETYPE_BASE, m_iWeapon, m_iRune));

	m_pForceCheckmark->SetPos(iCheckmarksX + iCheckmarkWidth + BTN_BORDER, GetHeight() - iAboveAttackBinds - BTN_BORDER - BTN_HEIGHT);
	m_pForceCheckmark->Mark(pArm->HasType(RUNETYPE_FORCE, m_iWeapon, m_iRune));

	SetRune(m_iRune, false);

	CPanel::Layout();
}

void CRuneLayoutPanel::Paint(int x, int y, int w, int h)
{
	if (!IsVisible())
		return;

	int iCount, i;

	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	CCFHudTexture* pWeaponIcon;
	if (m_iWeapon < 3)
		pWeaponIcon = CTextureHandler::GetTexture(pArm->m_aWeapons[m_iWeapon].m_iWeapon);
	else
		pWeaponIcon = CTextureHandler::GetTexture(pArm->m_aWeapons[m_iWeapon].m_iArmament);
	if (pWeaponIcon)
		pWeaponIcon->DrawSelf( x + w - 128, y, 128, 128, Color(255, 255, 255, 255));

	CRuneInfo RuneInfo;
	RuneInfo.m_iWeapon = m_iWeapon;
	RuneInfo.m_iRune = m_iRune;

	statuseffect_t eEffects;
	if (m_iWeapon < 3)
		eEffects = pArm->GetPhysicalAttackStatusEffect(m_iWeapon);
	else
	{
		int eElements, iDefense;
		pArm->GetElementDefense(&RuneInfo, eElements, iDefense);
		eEffects = StatusEffectForElement((element_t)eElements);
	}

	CRuneComboIcon::PaintInfuseIcons(eEffects, (m_iWeapon==3)?3:-1, x + w - 128, y, 128, 128);

	m_pAttackButton->Paint();

	iCount = m_apAttackBinds.Count();
	for (i = 0; i < iCount; i++)
	{
		((CBaseControl*)m_apAttackBinds[i])->Paint();
	}

	CCFHudTexture* pMouseIcon = NULL;
	if (m_apAttackBinds[0]->IsHighlighted())
		pMouseIcon = s_pMouseButtonsLMB;
	else if (m_apAttackBinds[1]->IsHighlighted())
		pMouseIcon = s_pMouseButtonsRMB;
	else
		pMouseIcon = s_pMouseButtons;

	if (pMouseIcon)
		pMouseIcon->DrawSelf(x + m_iAttackBindsX - 42, y + m_iAttackBindsY, 40, 40, Color(255,255,255,255));

	m_pBasePresent->Paint();
	m_pForcePresent->Paint();
	m_pBaseCheckmark->Paint();
	m_pForceCheckmark->Paint();

	m_pInfo->Paint();

	if (!m_pRuneCombo->IsEmpty() && gpGlobals->curtime - m_flComboSourceTime < s_flShiftTime)
	{
		float flPerc = RemapVal(gpGlobals->curtime - m_flComboSourceTime, 0.0f, s_flShiftTime, 0, 1);

		int cx, cy, cw, ch;
		m_pRuneCombo->GetPos(cx, cy);
		m_pRuneCombo->GetSize(cw, ch);
		m_pRuneCombo->Paint(RemapVal(flPerc, 0, 1, m_ComboSource.x, x+cx), RemapVal(flPerc, 0, 1, m_ComboSource.y, y+cy), cw, ch);
	}
	else
	{
		m_pRuneCombo->Paint();

		float flSwirlTime = gpGlobals->curtime - m_flComboSourceTime - (m_pRuneCombo->IsEmpty()?0:s_flShiftTime);

		if (flSwirlTime < s_flSwirlTime)
		{
			float flPerc = RemapVal(flSwirlTime, 0.0f, s_flSwirlTime, 0, 1);

			int iMaxRunes = pArm->GetRunesMaximum(m_iWeapon, m_iRune);
			float flCenterX, flCenterY, flHypotenuse;
			flCenterX = m_iAttackBindsX/2;
			flCenterY = GetHeight()/2;
			flHypotenuse = (flCenterY - m_flSlotWidth/2)*flPerc;		// Radius is the distance to the center minus half the width of one rune so that runes lie on the outside without overlapping.


			if (m_pRuneCombo->IsEmpty())
			{
				flCenterX = Lerp<float>(flPerc, m_ComboSource.x + m_flSlotWidth/2, x + flCenterX);
				flCenterY = Lerp<float>(flPerc, m_ComboSource.y + m_flSlotWidth/2, y + flCenterY);
			}
			else
			{
				flCenterX += x;
				flCenterY += y;
			}

			iCount = m_apRuneDropPanels.Count();
			for (i = 0; i < iCount; i++)
			{
				float flDegrees = 2*M_PI*i/iMaxRunes - 180*M_PI/180;		// Over-rotate left so the first one is on the side.
				flDegrees += 90*M_PI/180*flPerc;							// Rotate it back up to the top as we go.
				m_apRuneDropPanels[i]->Paint(flCenterX + cos(flDegrees) * flHypotenuse - m_flSlotWidth/2, flCenterY + sin(flDegrees) * flHypotenuse - m_flSlotWidth/2);
			}
		}
		else
		{
			iCount = m_apRuneDropPanels.Count();
			for (i = 0; i < iCount; i++)
			{
				m_apRuneDropPanels[i]->Paint();
			}
		}
	}
}

void CRuneLayoutPanel::SetRune(int iRune, bool bLayout, CRect& Source)
{
	m_iRune = iRune;

	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	unsigned int iCount = (unsigned int) m_apRuneDropPanels.Count();
	unsigned int i;
	for (i = 0; i < iCount; i++)
	{
		m_apRuneDropPanels[i]->SetVisible(pArm->GetRunesMaximum(m_iWeapon, m_iRune) > i);
		m_apRuneDropPanels[i]->SetRune(m_iRune);
	}

	for (i = 0; i < 2; i++)
	{
		KeyValues* pPK = new KeyValues("bind");
		pPK->SetInt("attack", i);
		pPK->SetInt("weapon", m_iWeapon);
		pPK->SetInt("slot", m_iRune);

		m_apAttackBinds[i]->SetClickedListener(this, &CRuneLayoutPanel::Bind, pPK);
	}

	m_pRuneCombo->SetNumen(pArm->SerializeCombo(m_iWeapon, m_iRune));

	if (Source.Size() != 0)
	{
		m_pRuneCombo->SetDimensions(GetWidth()/2-Source.w/2, GetHeight()/2-Source.h/2, Source.w, Source.h);

		m_flComboSourceTime = gpGlobals->curtime;
		m_ComboSource = Source;
	}

	// Prevent an infinite recursion when called from Layout()
	if (bLayout)
		Layout();

	SetInfo();
}

void CRuneLayoutPanel::SetInfo()
{
	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	CRuneInfo RuneInfo;
	RuneInfo.m_iWeapon = m_iWeapon;
	RuneInfo.m_iRune = m_iRune;

	m_pInfo->SetText(L"");

	bool bUseful = false;

	if (pArm->IsBindableCombo(&RuneInfo))
	{
		m_pInfo->AppendText("#Attack_Type");
		m_pInfo->AppendText(L"\n");
		m_pInfo->AppendText("#ForceC");
		m_pInfo->AppendText(VarArgs("#%s_Force", CRuneData::ForceEffectToString(pArm->GetDominantForce(m_iWeapon, m_iRune))));
		m_pInfo->AppendText(L"\n");
		m_pInfo->AppendText("#ElementsC");

		element_t eElements = pArm->GetMagicalAttackElement(&RuneInfo);
		bool bFirst = true;
		for (int i = 0; i < TOTAL_ELEMENTS; i++)
		{
			if (eElements & (1<<i))
			{
				if (!bFirst)
					m_pInfo->AppendText(L" ");
				m_pInfo->AppendText(VarArgs("#%s_Element", ElementToString((element_t)(eElements & (1<<i)))));
				bFirst = false;
			}
		}

		m_pInfo->AppendText(L"\n \n");
		bUseful = true;
	}

	if (pArm->GetPhysicalAttackElement(m_iWeapon))
	{
		m_pInfo->AppendText("#Weapon_Amplified");

		element_t eElements = pArm->GetMagicalAttackElement(&RuneInfo);
		bool bFirst = true;
		for (int i = 0; i < TOTAL_ELEMENTS; i++)
		{
			if (eElements & (1<<i))
			{
				if (!bFirst)
					m_pInfo->AppendText(L" ");
				m_pInfo->AppendText(VarArgs("#%s_Element", ElementToString((element_t)(eElements & (1<<i)))));
				bFirst = false;
			}
		}

		m_pInfo->AppendText(L"\n \n");
		bUseful = true;
	}

	if (pArm->GetPhysicalAttackStatusEffect(m_iWeapon))
	{
		m_pInfo->AppendText("#Status_Inflicted");

		statuseffect_t eEffects = pArm->GetMagicalAttackStatusEffect(&RuneInfo);
		bool bFirst = true;
		for (int i = 0; i < TOTAL_STATUSEFFECTS; i++)
		{
			if (eEffects & (1<<i))
			{
				if (!bFirst)
					m_pInfo->AppendText(L" ");
				m_pInfo->AppendText(VarArgs("#%s_SE", StatusEffectToString((statuseffect_t)(eEffects & (1<<i)))));
				bFirst = false;
			}
		}

		m_pInfo->AppendText(L"\n \n");
		bUseful = true;
	}

	int eElements, iDefense;
	pArm->GetElementDefense(&RuneInfo, eElements, iDefense);
	if (eElements != ELEMENT_TYPELESS)
	{
		if (iDefense < 0)
			m_pInfo->AppendText("#Armor_Absorbs");
		else
			m_pInfo->AppendText("#Armor_Resists");

		bool bFirst = true;
		for (int i = 0; i < TOTAL_ELEMENTS; i++)
		{
			if (eElements & (1<<i))
			{
				if (!bFirst)
					m_pInfo->AppendText(L" ");
				m_pInfo->AppendText(VarArgs("#%s_Element", ElementToString((element_t)(eElements & (1<<i)))));
				bFirst = false;
			}
		}

		m_pInfo->AppendText(L"\n \n");
		bUseful = true;
	}

	if (!bUseful)
		m_pInfo->AppendText("#No_Effect");
}

void CRuneLayoutPanel::BindCallback(KeyValues* pParms)
{
	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	int iAttack = pParms->GetFirstValue()->GetInt();
	int iWeapon = pParms->GetFirstValue()->GetNextValue()->GetInt();
	int iSlot = pParms->GetFirstValue()->GetNextValue()->GetNextValue()->GetInt();

	pArm->Bind(iAttack, iWeapon, iSlot);
}

void CRuneLayoutPanel::UnbindCallback(KeyValues* pParms)
{
	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	int iAttack = pParms->GetFirstValue()->GetInt();

	pArm->RemoveBind(iAttack);
}

CRuneChoice::CRuneChoice(RuneID eDraggable)
	: CDroppableChoice<RuneID, CRuneIcon>(eDraggable, IDraggable::DC_RUNEICON)
{
	m_pInfo = new CLabel(GetHeight(), 0, GetWidth()-GetHeight(), GetHeight(), "");
	AddControl(m_pInfo);

	m_pPrice = new CLabel(GetHeight(), 0, GetWidth()-GetHeight(), GetHeight(), "");
	AddControl(m_pPrice);
}

void CRuneChoice::Layout()
{
	CRuneData* pData = CRuneData::GetData( m_eDraggableID );

	m_pInfo->SetPos(GetHeight(), 0);
	m_pPrice->SetPos(GetHeight(), 0);
	if (GetParent())
	{
		m_pInfo->SetSize(GetParent()->GetWidth()-GetHeight()-BTN_BORDER*4, GetHeight());
		m_pPrice->SetSize(GetParent()->GetWidth()-GetHeight()-BTN_BORDER*4, GetHeight());
	}

	m_pInfo->SetAlign(CLabel::TA_LEFTCENTER);
	m_pInfo->SetWrap(false);
	m_pInfo->SetText(pData->m_szPrintName);

	m_pPrice->SetAlign(CLabel::TA_RIGHTCENTER);
	m_pPrice->SetText(VarArgs("%d", pData->m_iCost));

	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;
	if (pData->m_iCost > pArm->m_iCredits)
	{
		m_pInfo->SetFGColor(Color(255, 0, 0, 255));
		m_pPrice->SetFGColor(Color(255, 0, 0, 255));
		SetGrabbale(false);
	}
	else
	{
		m_pInfo->SetFGColor(Color(255, 255, 255, 255));
		m_pPrice->SetFGColor(Color(255, 255, 255, 255));
		SetGrabbale(true);
	}
}

// Override so that we can have the grabbable area the whole box, but the icon only show on the left.
void CRuneChoice::Paint(int x, int y, int w, int h)
{
	if (!IsVisible())
		return;

	IDraggable* pDragging = CRootPanel::GetRoot()->GetCurrentDraggable();

	if (IsGrabbale() && !pDragging && m_bHighlight)
	{
		int mx, my;
		CRootPanel::GetFullscreenMousePos(mx, my);

		CRect Icon = GetHoldingRect();
		if (mx >= Icon.x &&
			my >= Icon.y &&
			mx < Icon.x + Icon.w &&
			my < Icon.y + Icon.h)
		{
			CCFHudTexture* pHighlight = CTextureHandler::GetTexture(m_eDraggableID, CTextureHandler::TT_HIGHLIGHT);
			if (pHighlight)
				pHighlight->DrawSelf(Icon.x, Icon.y, Icon.h, Icon.h, Color(255, 255, 255, 255));
		}
	}

	for (int i = 0; i < m_apDraggables.Count(); i++)
	{
		m_apDraggables[i]->Paint(x, y, h, h);
	}

	// Disregard CDroppablePanel, it sucks cocks.
	CPanel::Paint(x, y, w, h);
}

COneForceBaseHintPopup::COneForceBaseHintPopup()
	: CPanel(0, 0, 100, 100)
{
	m_pOK = new CButton(0, 0, BTN_WIDTH, BTN_HEIGHT, "#OK");
	m_pOK->SetClickedListener(this, &COneForceBaseHintPopup::Close);
	AddControl(m_pOK);

	m_pHint = new CLabel(BTN_BORDER, BTN_BORDER, GetWidth(), GetHeight(), "#Hint_One_Force_Base");
	AddControl(m_pHint);

	SetVisible(false);
}

void COneForceBaseHintPopup::Layout()
{
	SetDimensions(CFScreenWidth()/2-150, CFScreenHeight()/2-75, 300, 150);

	m_pOK->SetPos(GetWidth()-BTN_BORDER-BTN_WIDTH, GetHeight()-BTN_BORDER-BTN_HEIGHT);
	m_pHint->SetSize(GetWidth()-BTN_BORDER*2, GetHeight()-BTN_BORDER*2);
}

void COneForceBaseHintPopup::Paint(int x, int y, int w, int h)
{
	if (!IsVisible())
		return;

	CRootPanel::DrawRect(x, y, x+w, y+h, Color(0, 0, 0, 255));

	CPanel::Paint(x, y, w, h);
}

void COneForceBaseHintPopup::Open()
{
	CRootPanel::GetRoot()->Popup(this);
	Layout();
	SetVisible(true);
}

void COneForceBaseHintPopup::Close()
{
	CRootPanel::GetRoot()->Popup(NULL);
	SetVisible(false);
}

void COneForceBaseHintPopup::CloseCallback(KeyValues* pParms)
{
	Close();
}

CRuneAdvancedPanel::CRuneAdvancedPanel()
	: CPanel(BTN_BORDER, BTN_HEIGHT + BTN_BORDER*2, PANEL_WIDTH - BTN_BORDER*2, PANEL_HEIGHT - BTN_HEIGHT - BTN_BORDER*3)
{
	m_pChoose = new CLabel(3, 3, PANEL_WIDTH - 6, BTN_HEIGHT, "#Select_Numen");
	m_pGetAWeaponStupid = new CLabel(3, 15, PANEL_WIDTH - 6, BTN_HEIGHT, "#Get_A_Weapon_Stupid");

	m_pGetAWeaponStupid->SetVisible(false);

	AddControl(m_pChoose);
	AddControl(m_pGetAWeaponStupid);

	m_pSlidingContainer = new CSlidingContainer(BTN_BORDER, BTN_HEIGHT*2 + BTN_BORDER*3, 400, 400);

	AddControl(m_pSlidingContainer);

	m_apSliders[0] = new CSlidingPanel(m_pSlidingContainer, "#Primary_Weapon");
	m_apSliders[1] = new CSlidingPanel(m_pSlidingContainer, "#Secondary_Weapon");
	m_apSliders[2] = new CSlidingPanel(m_pSlidingContainer, "#Secondary_Weapon");
	m_apSliders[3] = new CSlidingPanel(m_pSlidingContainer, "#Armor");
	//m_apSliders[4] = new CSlidingPanel(m_pSlidingContainer, "#Accessory");

	int i;
	for (i = 0; i < ARRAYSIZE(m_apSliders); i++)
	{
		m_apLayouts[i] = new CRuneLayoutPanel(this, i);
		m_apSliders[i]->AddControl(m_apLayouts[i]);
		m_apCombos[i] = new CRuneCombosPanel(m_apLayouts[i], i);
		m_apSliders[i]->AddControl(m_apCombos[i]);
	}

	char* aszNames[] =
	{
		"#Base",
		"#Force",
		"#Augment",
	};

	CLabel* pLabel;
	for (i = 0; i < 3; i++)
	{
		pLabel = new CLabel(0, 0, TAB_WIDTH, TAB_HEIGHT, aszNames[i]);
		m_apCats.AddToTail(pLabel);
		AddControl(pLabel);
	}

	m_pRunes = new CPanel(BTN_BORDER, BTN_HEIGHT*2 + BTN_BORDER*3, 100, 100);
	AddControl(m_pRunes);

	CRuneChoice* pRune;
	CRuneData *pRuneData = NULL;
	i = -1;
	while ((pRuneData = CRuneData::GetData((RuneID)++i)) != NULL)
	{
		if (!pRuneData->m_bBuyable)
			continue;

		switch (pRuneData->m_eType)
		{
		default:
			AssertMsg(false, "Invalid rune type.");
			// Fall through, use a default case.

		case RUNETYPE_BASE:
			pRune = new CRuneChoice(i);
			m_apBaseRunes.AddToTail(pRune);
			m_pRunes->AddControl(pRune);
			break;

		case RUNETYPE_FORCE:
			pRune = new CRuneChoice(i);
			m_apForceRunes.AddToTail(pRune);
			m_pRunes->AddControl(pRune);
			break;

		case RUNETYPE_SUPPORT:
			pRune = new CRuneChoice(i);
			m_apSupportRunes.AddToTail(pRune);
			m_pRunes->AddControl(pRune);
			break;
		}
	}

	Layout();

	m_pPopup = new COneForceBaseHintPopup();
	CRootPanel::GetRoot()->AddControl(m_pPopup, true);
}

void CRuneAdvancedPanel::SetVisible(bool bVis)
{
	if (bVis)
	{
		bool bLearnOneBaseOneForce = C_CFPlayer::GetLocalCFPlayer()->Instructor_IsLessonValid(HINT_ONE_FORCE_BASE);
		if (bLearnOneBaseOneForce)
			m_pPopup->Open();
	}

	CPanel::SetVisible(bVis);
}

void CRuneAdvancedPanel::Layout()
{
	SetBorder(BT_NONE);

	int iRunesHeight = GetHeight() - TAB_HEIGHT - BTN_BORDER;
	int iRunesWidth = TAB_WIDTH*3 + BTN_BORDER*2;

	m_pRunes->SetPos(0, TAB_HEIGHT + BTN_BORDER*1);
	m_pRunes->SetSize(iRunesWidth, iRunesHeight);

	m_pChoose->SetDimensions(iRunesWidth, 0, GetWidth()-iRunesWidth, TAB_HEIGHT);

	int iSliderHeight = GetHeight() - TAB_HEIGHT - BTN_BORDER;
	int iSliderWidth = GetWidth() - iRunesWidth - BTN_BORDER*1;

	m_pSlidingContainer->SetPos(iRunesWidth + BTN_BORDER, TAB_HEIGHT + BTN_BORDER*1);
	m_pSlidingContainer->SetSize(iSliderWidth, iSliderHeight);

	int i;

	int iFirstVisible = -1;
	for (i = 0; i < ARRAYSIZE(m_apSliders); i++)
	{
		bool bExists = SlotExists(i);
		m_apSliders[i]->SetVisible(bExists);
		if (bExists && iFirstVisible < 0)
			iFirstVisible = i;
	}

	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	for (i = 0; i < ARRAYSIZE(m_apSliders); i++)
	{
		if (pArm->m_aWeapons[i].m_pWeaponData)
		{
			m_apSliders[i]->SetTitle(VarArgs("#Armament%d", i+1));
			m_apSliders[i]->AppendTitle(L" - ");
			m_apSliders[i]->AppendTitle((i<3)?pArm->m_aWeapons[i].m_pWeaponData->szPrintName:pArm->m_aWeapons[i].m_pArmamentData->m_szPrintName);
		}
	}

	if (iFirstVisible == -1)
	{
		m_pGetAWeaponStupid->SetPos(BTN_BORDER + iRunesWidth + BTN_BORDER, BTN_HEIGHT + BTN_BORDER*2);
		m_pGetAWeaponStupid->SetSize(iSliderWidth, BTN_HEIGHT);
		m_pGetAWeaponStupid->SetVisible(true);
	}
	else
	{
		m_pGetAWeaponStupid->SetVisible(false);
		m_pSlidingContainer->SetCurrent(CRunePanel::GetActiveArmament());
		if (!m_pSlidingContainer->IsCurrentValid())
			m_pSlidingContainer->SetCurrent(iFirstVisible);
	}

	int iDone = 0;

	iDone++;

	m_apCats[0]->SetPos(BTN_BORDER, BTN_BORDER + BTN_HEIGHT*iDone);
	m_apCats[0]->SetSize(TAB_WIDTH, TAB_HEIGHT);

	int iCount;
	iCount = m_apBaseRunes.Count();
	for (i = 0; i < iCount; i++)
	{
		LayoutChoice(m_apBaseRunes[i], iDone++);
	}

	iDone++;
	iDone++;

	m_apCats[1]->SetPos(BTN_BORDER, BTN_BORDER + BTN_HEIGHT*iDone);
	m_apCats[1]->SetSize(TAB_WIDTH, TAB_HEIGHT);

	iCount = m_apForceRunes.Count();
	for (i = 0; i < iCount; i++)
	{
		LayoutChoice(m_apForceRunes[i], iDone++);
	}

	iDone++;
	iDone++;

	m_apCats[2]->SetPos(BTN_BORDER, BTN_BORDER + BTN_HEIGHT*iDone);
	m_apCats[2]->SetSize(TAB_WIDTH, TAB_HEIGHT);

	iCount = m_apSupportRunes.Count();
	for (i = 0; i < iCount; i++)
	{
		LayoutChoice(m_apSupportRunes[i], iDone++);
	}

	CPanel::Layout();
}

void CRuneAdvancedPanel::LayoutChoice(CRuneChoice* pChoice, int i)
{
	// Six rows.
	int iHeight = BTN_HEIGHT;
	// One column.
	int iWidth = (m_pRunes->GetWidth()-BTN_BORDER*2);

	pChoice->m_iDraggableWidth = min(iWidth, iHeight);

	pChoice->SetPos(BTN_BORDER, BTN_BORDER + iHeight*i);
	// Align to the left.
	pChoice->SetSize(iWidth, pChoice->m_iDraggableWidth);
}

bool CRuneAdvancedPanel::SlotExists(int iSlot)
{
	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	if (iSlot >= 0 && iSlot <= 2 && pArm->GetWeaponData(iSlot))
		return true;

	if (iSlot >= 3 && iSlot <= 4 && pArm->GetArmamentData(iSlot))
		return true;

	return false;
}

int CRuneArmamentCombos::s_iSlotWidth = 0;

CRuneArmamentCombos::CRuneArmamentCombos(IComboMaster* pMaster, int iArmament)
	: CPanel(0, 0, 400, 400)
{
	m_iArmament = iArmament;

	for (int i = 0; i < MAX_RUNE_MODS; i++)
	{
		m_apCombos.AddToTail(new CSelectableRuneCombo(m_iArmament, i));
		AddControl(m_apCombos[i]);
		m_apCombos[i]->SetMaster(pMaster);

		m_apXLabels.AddToTail(new CLabel(0, 0, 16, 16, ""));
		AddControl(m_apXLabels[i]);
	}
}

void CRuneArmamentCombos::Layout()
{
	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	SetVisible(pArm->GetRuneCombos(m_iArmament) > 0);
	if (!IsVisible())
		return;

	int i;

	if (!s_iSlotWidth)
	{
		int iHeight = GetHeight();
		if (GetParent())
			iHeight = GetParent()->GetHeight();

		int iTotalRunes = 0;
		for (i = 0; i < 5; i++)
			iTotalRunes += pArm->GetRuneCombos(i);

		float flMinWidth = (iHeight/iTotalRunes);

		s_iSlotWidth = min(flMinWidth, 80);
	}

	SetSize(s_iSlotWidth*2, s_iSlotWidth*pArm->GetRuneCombos(m_iArmament));

	int iCount = m_apCombos.Count();
	for (i = 0; i < iCount; i++)
	{
		CSelectableRuneCombo* pCombo = m_apCombos[i];
		pCombo->SetPos(GetWidth()-s_iSlotWidth, i*s_iSlotWidth);
		pCombo->SetSize(s_iSlotWidth, s_iSlotWidth);
		pCombo->SetVisible(pArm->GetRunesMaximum(pCombo->GetWeapon(), pCombo->GetRune()) > 0);
		pCombo->SetNumen(pArm->SerializeCombo(pCombo->GetWeapon(), pCombo->GetRune()));

		int iX, iY;
		pCombo->GetBR(iX, iY);
		m_apXLabels[i]->SetPos(iX, iY);
		m_apXLabels[i]->SetVisible(pCombo->IsVisible());
		m_apXLabels[i]->SetText(VarArgs("#x%d", pArm->GetRunesMaximum(m_iArmament, i)));
		m_apXLabels[i]->SetWrap(false);
	}
}

// Override CPanel::Paint() to only draw some borders.
void CRuneArmamentCombos::Paint(int x, int y, int w, int h)
{
	if (!IsVisible())
		return;

	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	CCFHudTexture* pArmamentIcon;
	if (m_iArmament < 3)
		pArmamentIcon = CTextureHandler::GetTexture(pArm->m_aWeapons[m_iArmament].m_iWeapon);
	else
		pArmamentIcon = CTextureHandler::GetTexture(pArm->m_aWeapons[m_iArmament].m_iArmament);
	pArmamentIcon->DrawSelf( x, y, s_iSlotWidth, s_iSlotWidth, Color(255, 255, 255, 255));

	CPanel::s_pPanelTL->DrawSelf(x,			y,			3,		3,		Color(255, 255, 255, 255));
	CPanel::s_pPanelT->DrawSelf	(x+3,		y,			w-6,	3,		Color(255, 255, 255, 255));
	CPanel::s_pPanelTR->DrawSelf(x+w-3,		y,			3,		3,		Color(255, 255, 255, 255));
	CPanel::s_pPanelL->DrawSelf	(x,			y+3,		3,		h-6,	Color(255, 255, 255, 255));
	CPanel::s_pPanelC->DrawSelf	(x+3,		y+3,		w-6,	h-6,	Color(255, 255, 255, 255));
	CPanel::s_pPanelBL->DrawSelf(x,			y+h-3,		3,		3,		Color(255, 255, 255, 255));

	int iLabelHeight = 10;

	for (int i = 0; i < m_apCombos.Count(); i++)
	{
		int cx, cy, ax, ay;
		m_apCombos[i]->GetAbsPos(cx, cy);
		GetAbsPos(ax, ay);

		m_apCombos[i]->Paint(cx+x-ax, cy+y-ay, m_apCombos[i]->GetCurrSize(), m_apCombos[i]->GetCurrSize());
		m_apXLabels[i]->Paint(cx+x-ax + m_apCombos[i]->GetCurrSize() - iLabelHeight, cy+y-ay + m_apCombos[i]->GetCurrSize() - iLabelHeight, iLabelHeight, iLabelHeight);
	}
}

CRunePresetButton::CRunePresetButton()
	: CButton(0, 0, 400, 400, "")
{
	m_pPreset = NULL;
	m_pHighlightListener = NULL;
}

void CRunePresetButton::Paint(int x, int y, int w, int h)
{
	if (!IsVisible())
		return;

	CButton::PaintButton(x, y, w, h);

	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	int iAlpha = 255;
	if (!pArm->CanBuyCombo(m_pPreset, CRunePanel::GetActiveArmament(), CRunePanel::GetActiveCombo()))
		iAlpha = 100;

	if (m_pPreset)
		CRuneComboIcon::Paint(m_pPreset->Serialize(), CRunePanel::GetActiveArmament(), x, y, h, h, iAlpha);

	// Skip CButton cause we already did it,
	// instead draw the label text on the right hand side.
	CLabel::Paint(x + h, y, w - h, h);
}

void CRunePresetButton::SetPreset(CRuneComboPreset* pPreset)
{
	m_pPreset = pPreset;

	SetText(pPreset->m_szName);
}

void CRunePresetButton::CursorIn()
{
	CButton::CursorIn();

	if (m_pHighlightListener)
		m_pHighlightListener->Highlighted(m_pPreset);
}

void CRunePresetButton::CursorOut()
{
	CButton::CursorOut();

	if (m_pHighlightListener)
		m_pHighlightListener->Highlighted(NULL);
}

CRuneSimplePresets::CRuneSimplePresets()
	: CPanel(0, 0, 400, 400)
{
	for (int i = 0; i < 8; i++)
	{
		m_apPresets.AddToTail(new CRunePresetButton());
		AddControl(m_apPresets[i]);
		m_apPresets[i]->SetClickedListener(this, ChooseCombo, new KeyValues("choose", "combo", i));
		m_apPresets[i]->SetHighlightListener(this);
	}

	m_pCombosLabel = new CLabel(0, 0, 0, 0, "");
	AddControl(m_pCombosLabel);

	m_pDescription = new CLabel(0, 0, 0, 0, "");
	AddControl(m_pDescription);
}

void CRuneSimplePresets::Layout()
{
	SetBorder(BT_NONE);

	if (!CConfigMgr::Get())
		return;

	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;
	CConfigMgr* pCfg = CConfigMgr::Get();
	int iWidth = CRuneArmamentCombos::s_iSlotWidth*2;

	bool bSomethingSelected = (CRunePanel::GetActiveArmament() != -1);
	int iRunesInCombo = pArm->GetRunesMaximum(CRunePanel::GetActiveArmament(), CRunePanel::GetActiveCombo());

	CUtlVector<CRuneComboPreset*>* apRunePresets = pCfg->GetRuneList(iRunesInCombo);

	int iHeight = 500;
	if (GetParent())
		iHeight = GetParent()->GetHeight();

	int iButtonHeight = (iHeight - BTN_HEIGHT - BTN_BORDER*m_apPresets.Count())/m_apPresets.Count();

	m_pCombosLabel->SetPos(0, 0);
	m_pCombosLabel->SetSize(iWidth, BTN_HEIGHT);

	if (iRunesInCombo == 0)
		m_pCombosLabel->SetText("");
	else if (!bSomethingSelected)
		m_pCombosLabel->SetText("#Select_a_combo");
	else
		m_pCombosLabel->SetText(VarArgs("#Combos_%d_slots", iRunesInCombo));

	for (int i = 0; i < m_apPresets.Count(); i++)
	{
		CRunePresetButton* pButton = m_apPresets[i];
		pButton->SetPos(0, BTN_HEIGHT + iButtonHeight*i + BTN_BORDER*(i+1));
		pButton->SetSize(iWidth, iButtonHeight);
		pButton->SetVisible(bSomethingSelected && apRunePresets->IsValidIndex(i));
		if (pButton->IsVisible())
		{
			CRuneComboPreset* pPreset = (*apRunePresets)[i];
			pButton->SetPreset(pPreset);
			if (pArm->CanBuyCombo(pPreset, CRunePanel::GetActiveArmament(), CRunePanel::GetActiveCombo()))
			{
				pButton->SetEnabled(true);
				pButton->SetFGColor(Color(255, 255, 255, 255));
			}
			else
			{
				pButton->SetEnabled(false);
				pButton->SetText("#No_Cash");
				pButton->SetFGColor(Color(255, 0, 0, 255));
			}
		}
		pButton->ComputeLines(pButton->GetWidth()-iButtonHeight);	// Special compute lines, because it is drawn at a special width.
	}

	m_pDescription->SetPos(iWidth + BTN_BORDER, 0);
	m_pDescription->SetSize(GetWidth()-iWidth-BTN_BORDER, GetHeight());
}

void CRuneSimplePresets::SetRune()
{
	Layout();
}

void CRuneSimplePresets::ChooseComboCallback(KeyValues* pKV)
{
	CRuneComboPreset* pPreset = m_apPresets[pKV->GetFirstValue()->GetInt()]->GetPreset();

	if (!pPreset)
		return;

	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	pArm->BuyCombo(pPreset, CRunePanel::GetActiveArmament(), CRunePanel::GetActiveCombo());

	CCFMenu::KillOnClose();

	for (int i = 0; i < ATTACK_BINDS; i++)
	{
		if (pArm->CanBind(i, CRunePanel::GetActiveArmament(), CRunePanel::GetActiveCombo()) && pArm->m_aAttackBinds[i].m_iWeapon == -1)
		{
			pArm->Bind(i, CRunePanel::GetActiveArmament(), CRunePanel::GetActiveCombo());
			break;
		}
	}

	CRootPanel::GetRoot()->Layout();
}

void CRuneSimplePresets::Highlighted(CRuneComboPreset* pPreset)
{
	if (!pPreset)
		m_pDescription->SetText("");
	else if (pPreset->m_szName[0] == '#')
		m_pDescription->SetText(VarArgs("%s_Description", pPreset->m_szName));
	else
		m_pDescription->SetText(pPreset->m_szName);
}

CRuneSimplePanel::CRuneSimplePanel()
	: CPanel(BTN_BORDER, BTN_HEIGHT + BTN_BORDER*2, PANEL_WIDTH - BTN_BORDER*2, PANEL_HEIGHT - BTN_HEIGHT - BTN_BORDER*3)
{
	for (int i = 0; i < 5; i++)
	{
		m_apCombos.AddToTail(new CRuneArmamentCombos(this, i));
		AddControl(m_apCombos[i]);
	}

	m_pPresets = new CRuneSimplePresets();
	AddControl(m_pPresets);

	m_pGetAWeaponStupid = new CLabel(3, 15, PANEL_WIDTH - 6, BTN_HEIGHT, "#Get_A_Weapon_Stupid");
	m_pGetAWeaponStupid->SetVisible(false);
	AddControl(m_pGetAWeaponStupid);
}

void CRuneSimplePanel::Layout()
{
	SetBorder(BT_NONE);

	// Do children first so that they figure out how tall they want to be.
	CPanel::Layout();

	int iCurrent = 0;

	for (int i = 0; i < 5; i++)
	{
		if (!m_apCombos[i]->IsVisible())
			continue;

		m_apCombos[i]->SetPos(0, iCurrent);

		iCurrent += m_apCombos[i]->GetHeight() + BTN_BORDER;
	}

	m_pPresets->SetPos(CRuneArmamentCombos::s_iSlotWidth*2 + BTN_BORDER, 0);
	m_pPresets->SetSize(GetWidth() - CRuneArmamentCombos::s_iSlotWidth*2 - BTN_BORDER, GetHeight());
	m_pPresets->Layout();	// Layout again now that we know what size we are, so we can resize the buttons properly.

	if (CRunePanel::GetActiveArmament() == -1 && m_apCombos.IsValidIndex(0))
	{
		if (m_apCombos[0]->m_apCombos.IsValidIndex(0))
			RequestFocus(m_apCombos[0]->m_apCombos[0]);
	}
	else if (CRunePanel::GetActiveArmament() != -1 && CRunePanel::GetActiveCombo() != -1)
		RequestFocus(m_apCombos[CRunePanel::GetActiveArmament()]->m_apCombos[CRunePanel::GetActiveCombo()]);

	if (iCurrent == 0)
	{
		m_pGetAWeaponStupid->SetPos(0, BTN_HEIGHT + BTN_BORDER);
		m_pGetAWeaponStupid->SetSize(GetWidth(), BTN_HEIGHT);
		m_pGetAWeaponStupid->SetVisible(true);
	}
	else
	{
		m_pGetAWeaponStupid->SetVisible(false);
	}
}

void CRuneSimplePanel::RequestFocus(CSelectableRuneCombo* pRequesting)
{
	if (pRequesting->IsActive())
		return;

	for (int i = 0; i < m_apCombos.Count(); i++)
	{
		for (int j = 0; j < m_apCombos[i]->m_apCombos.Count(); j++)
			m_apCombos[i]->GetCombo(j)->SetActive(false);
	}

	if (!pRequesting)
	{
		CRunePanel::SetActive(-1, -1);
	}
	else
	{
		CRunePanel::SetActive(pRequesting->GetWeapon(), pRequesting->GetRune());

		m_apCombos[CRunePanel::GetActiveArmament()]->GetCombo(CRunePanel::GetActiveCombo())->SetActive(true);
	}

	m_pPresets->SetRune();
}

CRuneDrop* CRuneSimplePanel::GetRuneDrop(int iMod)
{
	// Nobody will ever be dragging anything here.
	return NULL;
}

CRunePanel::CRunePanel()
	: CPanel(0, BTN_HEIGHT + BTN_BORDER, PANEL_WIDTH, PANEL_HEIGHT)
{
	Assert(!s_pRunePanel);
	s_pRunePanel = this;

	m_pAdvancedButton = new CButton(BTN_BORDER, BTN_BORDER, PANEL_WIDTH - BTN_BORDER*2, BTN_HEIGHT, "#Advanced");
	AddControl(m_pAdvancedButton);
	m_pAdvancedButton->SetClickedListener(this, &CRunePanel::Advanced);

	m_pAdvancedPanel = new CRuneAdvancedPanel();
	AddControl(m_pAdvancedPanel);
	m_pSimplePanel = new CRuneSimplePanel();
	AddControl(m_pSimplePanel);

	SetAdvanced(false);
}

void CRunePanel::Destructor()
{
	Assert(s_pRunePanel);
	s_pRunePanel = NULL;
}

void CRunePanel::Layout()
{
	SetSize(PANEL_WIDTH, PANEL_HEIGHT);
	SetPos(50, m_iY);

	m_pAdvancedButton->SetDimensions(BTN_BORDER, BTN_BORDER, PANEL_WIDTH - BTN_BORDER*2, BTN_HEIGHT);

	CPanel::Layout();
}

void CRunePanel::AdvancedCallback(KeyValues* pParms)
{
	SetAdvanced(!m_bAdvanced);
}

void CRunePanel::SetAdvanced(bool bAdvanced)
{
	m_bAdvanced = bAdvanced;

	m_pAdvancedPanel->SetVisible(bAdvanced);
	m_pSimplePanel->SetVisible(!bAdvanced);

	m_pAdvancedButton->SetText(bAdvanced?"#Simple":"#Advanced");

	Layout();
}

void CRunePanel::SetActive(int iArmament, int iCombo)
{
	if (s_iActiveArmament == iArmament && s_iActiveCombo == iCombo)
		return;

	s_iActiveArmament = iArmament;
	s_iActiveCombo = iCombo;

	if (s_pRunePanel)
		s_pRunePanel->Layout();
}

void CRunePanel::GetActive(int &iArmament, int &iCombo)
{
	iArmament = s_iActiveArmament;
	iCombo = s_iActiveCombo;
}
