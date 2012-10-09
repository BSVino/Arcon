#include "cbase.h"
#include "cfui_menu.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CAttackBindDrop::CAttackBindDrop(int iBind)
	: CDroppableIcon<CRuneComboDraggable>(IDraggable::DC_RUNECOMBO, CRect(0, 0, 0, 0))
{
	m_iBind = iBind;

	m_pDelete = new CDeleteButton();
	AddControl(m_pDelete);
	m_pDelete->SetClickedListener(this, &CAttackBindDrop::Remove);
}

void CAttackBindDrop::Layout()
{
	m_pDelete->SetPos(m_iW-16-BTN_BORDER, m_iH-16-BTN_BORDER);
	m_pDelete->SetSize(16, 16);
	m_pDelete->SetVisible(GetDraggableIcon() != NULL);

	CDroppableIcon<CRuneComboDraggable>::Layout();
}

void CAttackBindDrop::DraggableChanged(CRuneComboDraggable* pDragged)
{
	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	if (pDragged)
		pArm->Bind(m_iBind, pDragged->GetWeapon(), pDragged->GetRune());
	else
		pArm->RemoveBind(m_iBind);

	CRootPanel::UpdateArmament(pArm);

	CRootPanel::GetRoot()->Layout();
}

bool CAttackBindDrop::CanDropHere(IDraggable* pDraggable)
{
	// Check the superclass to make sure we have the correct drag class before we execute the cast below.
	if (!CDroppableIcon<CRuneComboDraggable>::CanDropHere(pDraggable))
		return false;

	if (!pDraggable)
		return true;

	CRuneComboDraggable* pWeapon = (CRuneComboDraggable*)pDraggable;

	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	if (!pArm->CanBind(m_iBind, pWeapon->GetWeapon(), pWeapon->GetRune()))
		return false;

	return true;
}

void CAttackBindDrop::RemoveCallback(KeyValues* pParms)
{
	SetDraggable(NULL);
}

CAttkDropPanel::CAttkDropPanel(int iBind)
	: CPanel(0, 0, PANEL_WIDTH, PANEL_HEIGHT)
{
	m_iBind = iBind;

	m_pLabel = new CLabel(0, 0, BTN_WIDTH, BTN_HEIGHT, "");
	AddControl(m_pLabel);

	m_pDrop = new CAttackBindDrop(iBind);
	AddControl(m_pDrop);
}

void CAttkDropPanel::Destructor()
{
}

void CAttkDropPanel::Layout()
{
	m_pLabel->SetSize(GetWidth(), BTN_HEIGHT);
	m_pLabel->SetText(VarArgs("+attack%d", m_iBind+1));

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

void CAttkDropPanel::Paint(int x, int y, int w, int h)
{
	if (!IsVisible())
		return;

	IDraggable* pDragging = CRootPanel::GetRoot()->GetCurrentDraggable();

	if (pDragging && m_pDrop->CanDropHere(pDragging))
	{
		// WEAPON_MAGIC is the highlight.
		CTextureHandler::GetTexture(WEAPON_MAGIC)->DrawSelf(x, y, m_iW, m_iH, Color(255, 255, 255, 255));
		SetHighlighted(true);
	}
	else
		SetHighlighted(false);

	if (m_pDrop->GetDraggableIcon())
		m_pDrop->GetDraggableIcon()->Paint();

	CPanel::Paint(x, y, w, h);
}

bool CAttkDropPanel::MousePressed(vgui::MouseCode code)
{
	return m_pDrop->MousePressed(code);
}

CBindPanel::CBindPanel()
	: CPanel(0, BTN_HEIGHT + BTN_BORDER, PANEL_WIDTH, PANEL_HEIGHT)
{
	int tlX = 0, tlY = 0, brX = 0, brY = 0;
	GetBR(brX, brY);
	tlX = m_iX;
	tlY = m_iY;

	m_pChoose = new CLabel(3, 3, PANEL_WIDTH - 6, BTN_HEIGHT, "");
	AddControl(m_pChoose);

	m_pRunes = new CPanel(BTN_BORDER, BTN_HEIGHT*2 + BTN_BORDER*3, 100, 100);
	AddControl(m_pRunes);

	for (int i = 0; i < ATTACK_BINDS; i++)
	{
		m_apBinds[i] = new CAttkDropPanel(i);
		AddControl(m_apBinds[i]);
	}

	Layout();
}

void CBindPanel::Layout()
{
	int i;
	int iRunesHeight = PANEL_HEIGHT - BTN_HEIGHT - BTN_BORDER*3;
	int iRunesWidth = TAB_WIDTH*4 + BTN_BORDER*3;

	m_pRunes->SetPos(BTN_BORDER, BTN_HEIGHT + BTN_BORDER*2);
	m_pRunes->SetSize(iRunesWidth, iRunesHeight);

	for (i = 0; i < m_apBindRunes.Count(); i++)
	{
		CDroppableChoice<RuneID, CRuneComboDraggable>* pRune = m_apBindRunes[i];
		m_apBindRunes.Remove(i);
		m_pRunes->RemoveControl(pRune);
		pRune->Destructor();
		pRune->Delete();
	}

	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;

	unsigned int iCount = pArm->GetRuneCombos();
	for (i = 0; i < (int)iCount; i++)
	{
		// Six rows.
		int iHeight = (m_pRunes->GetHeight()-BTN_BORDER*2)/6;
		// Three columns.
		int iWidth = (m_pRunes->GetWidth()-BTN_BORDER*2)/3;

		CRunePosition* pBind = pArm->GetRuneCombo(i);

		CDroppableChoice<RuneID, CRuneComboDraggable>* pRune = new CDroppableChoice<RuneID, CRuneComboDraggable>(pBind->m_iWeapon, pBind->m_iRune);
		m_apBindRunes.AddToTail(pRune);
		m_pRunes->AddControl(pRune);

		pRune->m_iDraggableWidth = min(iWidth, iHeight);

		int iColumn = i%3;
		pRune->SetPos(BTN_BORDER + (iColumn*pRune->m_iDraggableWidth), BTN_BORDER + iHeight*((int)(((float)i)/3)));
		// Align to the left.
		pRune->SetSize(pRune->m_iDraggableWidth, pRune->m_iDraggableWidth);
	}

	int iBindsHeight = (iRunesHeight - BTN_BORDER*3)/4;

	for (int i = 0; i < ATTACK_BINDS; i++)
	{
		m_apBinds[i]->SetPos(BTN_BORDER*2 + iRunesWidth, BTN_HEIGHT + BTN_BORDER*2 + iBindsHeight*i + BTN_BORDER*i);
		m_apBinds[i]->SetSize(iBindsHeight, iBindsHeight);
	}

	CPanel::Layout();
}
