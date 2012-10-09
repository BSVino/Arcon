#include "cbase.h"
#include "runecombo.h"
#include "hud_indicators.h"
#include <vgui/ISurface.h>
#include "particle_parse.h"
#include "particles_new.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CRuneCombo::CRuneCombo(long iNumen, int iWeapon, int iDiff1, int iDiff2, bool bIcons)
{
	m_iNumen = iNumen;
	m_iArmament = iWeapon;
	m_bDrawIcons = bIcons;
}

CRuneCombo::CRuneCombo(int iWeapon, int iRune, bool bIcons)
{
	CArmament* pArm = C_CFPlayer::GetLocalCFPlayer()->m_pArmament;
	m_iNumen = pArm->SerializeCombo(iWeapon, iRune);
	m_iArmament = iWeapon;
	m_bDrawIcons = bIcons;
}

CRuneCombo::CRuneCombo(CArmament* pArm, int iWeapon, int iRune, bool bIcons)
{
	m_iNumen = pArm->SerializeCombo(iWeapon, iRune);
	m_iArmament = iWeapon;
	m_bDrawIcons = bIcons;
}

void CRuneCombo::Paint()
{
	int x = 0, y = 0;
	GetAbsPos(x, y);

	Paint(x, y, GetWidth(), GetHeight(), 255);
}

void CRuneCombo::Paint(int x, int y, int w, int h)
{
	Paint(x, y, w, h, 255);
}

void CRuneCombo::Paint(int x, int y, int w, int h, int a)
{
	if (m_bDrawIcons)
		PaintIcons(x, y, w, h, a);
	else
		PaintRunes(x, y, w, h, a);
}

void CRuneCombo::PaintRunes(int x, int y, int w, int h, int a)
{
	int i;
	CRuneData* pRune;
	int iBaseRunes = 0;
	int iForceRunes = 0;
	int iModRunes = 0;

	// Count up the base runes.
	for (i = 0; i < CRuneData::TotalRunes(); i++)
	{
		if ((m_iNumen & (1<<i)) == 0)
			continue;

		pRune = CRuneData::GetData(i);

		Assert(pRune);
		if (!pRune)
			continue;

		if (pRune->m_eType == RUNETYPE_BASE)
			iBaseRunes++;

		if (pRune->m_eType == RUNETYPE_SUPPORT)
			iModRunes++;

		if (pRune->m_eType == RUNETYPE_FORCE)
			iForceRunes++;
	}

	// How many runes processed so far?
	int iProcessed = 0;

	float flOneThird = ((float)w + (float)h)/6;
	float flTwoThirds = ((float)w + (float)h)*2/6;

	for (i = 0; i < CRuneData::TotalRunes(); i++)
	{
		if ((m_iNumen & (1<<i)) == 0)
			continue;

		pRune = CRuneData::GetData(i);

		if (!pRune)
			continue;

		if (pRune->m_eType != RUNETYPE_BASE)
			continue;

		int iX = x;
		int iY = y;
		int iWidth = w;
		int iHeight = h;

		if (iForceRunes)
		{
			iX += flOneThird;
			iWidth = flTwoThirds;
		}
		
		if (iModRunes)
		{
			iY += flOneThird;
			iHeight = flTwoThirds;
		}

		int iXOffset = 0, iYOffset = 0;

		if (iBaseRunes == 2)
		{
			if (iProcessed == 0)
			{
				// Make a large one on the left.
				iWidth /= 2;
			}
			else
			{
				// Make a large one on the right.
				iXOffset = iWidth/2;
				iWidth /= 2;
			}
		}
		else if (iBaseRunes == 3)
		{
			if (iProcessed == 0)
			{
				// Make a large one on the left.
				iWidth /= 2;
			}
			else if (iProcessed == 1)
			{
				// Make a large one on the top right.
				iXOffset = iWidth/2;
				iWidth /= 2;
				iHeight /= 2;
			}
			else
			{
				// Make a large one on the bottom right.
				iXOffset = iWidth/2;
				iYOffset = iHeight/2;
				iWidth /= 2;
				iHeight /= 2;
			}
		}
		else if (iBaseRunes >= 4)
		{
			if (iProcessed == 0)
			{
				// Make a large one on the top left.
				iWidth /= 2;
				iHeight /= 2;
			}
			else if (iProcessed == 1)
			{
				// Make a large one on the bottom left.
				iYOffset = iWidth/2;
				iWidth /= 2;
				iHeight /= 2;
			}
			else if (iProcessed == 2)
			{
				// Make a large one on the top right.
				iXOffset = iWidth/2;
				iWidth /= 2;
				iHeight /= 2;
			}
			else if (iProcessed == 3)
			{
				// Make a large one on the bottom right.
				iXOffset = iWidth/2;
				iYOffset = iHeight/2;
				iWidth /= 2;
				iHeight /= 2;
			}
			else
				// Base runes past the fourth are simply not drawn. What else would you do with them?
				continue;
		}

		PaintRune(i, iX + iXOffset, iY + iYOffset, iWidth, iHeight, a);

		iProcessed++;
	}

	for (i = 0; i < CRuneData::TotalRunes(); i++)
	{
		if ((m_iNumen & (1<<i)) == 0)
			continue;

		pRune = CRuneData::GetData(i);

		if (!pRune)
			continue;

		if (pRune->m_eType == RUNETYPE_BASE)
			continue;

		if (pRune->m_eType == RUNETYPE_SUPPORT && iForceRunes)
			PaintRune(i, x + flOneThird, y, flTwoThirds, h, a);
		else
			PaintRune(i, x, y, w, h, a);
	}
}

void CRuneCombo::PaintIcons(int x, int y, int w, int h, int a)
{
	Color c(255, 255, 255, a);
	int iStatus = 0;
	int iStatuses = 0;
	long i;
	forceeffect_t eForce = CArmament::GetDominantForce(m_iNumen);
	bool bInfused = false;

	for (i = 0; i < CRuneData::TotalRunes(); i++)
	{
		if (!(m_iNumen & (1<<i)))
			continue;

		RuneID eRune = (RuneID)i;
		CRuneData* pData = CRuneData::GetData(eRune);
		if (!pData)
			continue;

		if (pData->m_eEffect == RE_RESTORE)
			c = Color(0, 255, 0, a);

		if (pData->m_eEffect == RE_SPELL && pData->m_eStatusEffect)
		{
			iStatus |= pData->m_eStatusEffect;
			iStatuses++;
		}

		if (pData->m_eEffect == RE_ELEMENT)
			bInfused = true;
	}

	// No force and no infusion? Display it grayed out to show that it doesn't do anything.
	if (!bInfused && eForce == FE_INVALID)
		a /= 2;

	if (iStatuses)
		PaintStatusIcons((statuseffect_t)iStatus, bInfused, eForce != FE_INVALID, x, y, w, h, a);

	if (eForce != FE_INVALID)
		PaintForceIcon(eForce, x, y, w, h, c);
}

void CRuneCombo::PaintStatusIcons(statuseffect_t iStatus, bool bInfused, bool bForce, int x, int y, int w, int h, int a)
{
	int i, iStatuses = 0;
	for (i = 0; i < TOTAL_STATUSEFFECTS; i++)
	{
		if (!(iStatus & (1<<i)))
			continue;

		iStatuses++;
	}

	int iMaxStatuses = min(iStatuses, 4);
	int iStatusesDrawn = 0;

	for (i = 0; i < TOTAL_STATUSEFFECTS; i++)
	{
		if (!(iStatus & (1<<i)))
			continue;

		if (iStatuses == 1)
		{
			int iY = y+h/2;
			if (!bForce)
				iY = y+h/4;	// Center vertically.
			PaintStatusIcon((statuseffect_t)(1<<i), x+w/4, iY, w/2, h/2, a);

			// Don't do it if there is no force, because we do a larger one in that case.
			if (bInfused && m_iArmament >= 0 && bForce)
				PaintInfuseIcon(m_iArmament, x+w/4, iY, w/2, h/2, a);
		}
		else
		{
			int iWidth = w/iMaxStatuses;
			int iHeight = h/iMaxStatuses;
			int iY = y+h-iHeight;
			if (!bForce)
				iY = y+h/2-iHeight/2;	// Center vertically.
			PaintStatusIcon((statuseffect_t)(1<<i), x+iWidth*iStatusesDrawn, iY, iWidth, iHeight, a);

			// Don't do it if there is no force, because we do a larger one in that case.
			if (bInfused && m_iArmament >= 0 && bForce)
				PaintInfuseIcon(m_iArmament, x+iWidth*iStatusesDrawn, iY, iWidth, iHeight, a);
		}

		if (++iStatusesDrawn >= iMaxStatuses)
			break;
	}

	if (!bForce && bInfused && m_iArmament >= 0)
		PaintInfuseIcon(m_iArmament, x, y, w, h, a);
}

bool CRuneCombo::IsEmpty()
{
	return m_iNumen == 0;
}

static CRuneComboIcon g_IconSingleton;

void CRuneComboIcon::Paint(long iNumen, int iArmament, int x, int y, int w, int h, int a)
{
	g_IconSingleton.m_iNumen = iNumen;
	g_IconSingleton.m_iArmament = iArmament;
	g_IconSingleton.m_bDrawIcons = true;
	g_IconSingleton.Paint(x, y, w, h, a);
}

void CRuneComboIcon::PaintInfuseIcons(statuseffect_t iStatus, int iArmament, int x, int y, int w, int h, int a)
{
	g_IconSingleton.m_iArmament = iArmament;
	g_IconSingleton.PaintStatusIcons(iStatus, true, false, x, y, w, h, a);
}

void CRuneComboIcon::Paint(int x, int y, int w, int h)
{
	CRuneCombo::Paint(x, y, w, h);
}

void CRuneComboIcon::Paint(int x, int y, int w, int h, int a)
{
	CRuneCombo::Paint(x, y, w, h, a);
}

void CRuneComboIcon::PaintRune(RuneID eRune, int iX, int iY, int iW, int iH, int iA)
{
	CCFHudTexture* pTexture = CTextureHandler::GetTexture(eRune);

	if (pTexture)
		pTexture->DrawSelf(iX, iY, iW, iH, Color(255, 255, 255, iA));
}

void CRuneComboIcon::PaintForceIcon(forceeffect_t eForce, int iX, int iY, int iW, int iH, Color c)
{
	CCFHudTexture* pTexture = CTextureHandler::GetTexture(eForce);

	if (pTexture)
		pTexture->DrawSelf(iX, iY, iW, iH, c);
}

void CRuneComboIcon::PaintStatusIcon(statuseffect_t eStatus, int iX, int iY, int iW, int iH, int iA)
{
	CCFHudTexture* pTexture = CTextureHandler::GetTexture(eStatus);

	if (pTexture)
		pTexture->DrawSelf(iX, iY, iW, iH, Color(255, 255, 255, iA));
}

void CRuneComboIcon::PaintInfuseIcon(int iArmament, int iX, int iY, int iW, int iH, int iA)
{
	CCFHudTexture* pTexture = GetHudTexture((iArmament<=2)?"force_attack":"force_defend");

	if (pTexture)
		pTexture->DrawSelf(iX, iY, iW, iH, Color(255, 255, 255, iA));
}

void CGUIRuneCombo::PaintRune(RuneID eRune, int iX, int iY, int iW, int iH, int iA)
{
	CCFHudTexture* pTexture = CTextureHandler::GetTexture(eRune);

	if (pTexture)
		pTexture->DrawSelf(iX, iY, iW, iH, Color(255, 255, 255, iA));
}

void CGUIRuneCombo::PaintForceIcon(forceeffect_t eForce, int iX, int iY, int iW, int iH, Color c)
{
	CCFHudTexture* pTexture = CTextureHandler::GetTexture(eForce);

	if (pTexture)
		pTexture->DrawSelf(iX, iY, iW, iH, c);
}

void CGUIRuneCombo::PaintStatusIcon(statuseffect_t eStatus, int iX, int iY, int iW, int iH, int iA)
{
	CCFHudTexture* pTexture = CTextureHandler::GetTexture(eStatus);

	if (pTexture)
		pTexture->DrawSelf(iX, iY, iW, iH, Color(255, 255, 255, iA));
}

void CGUIRuneCombo::PaintInfuseIcon(int iArmament, int iX, int iY, int iW, int iH, int iA)
{
	CCFHudTexture* pTexture = GetHudTexture((iArmament<=2)?"force_attack":"force_defend");

	if (pTexture)
		pTexture->DrawSelf(iX, iY, iW, iH, Color(255, 255, 255, iA));
}

CRuneComboDraggable::CRuneComboDraggable(IDroppable* pDroppable, int iWeapon, int iRune)
	: CGUIRuneCombo(iWeapon, iRune)
{
	m_pDroppable = pDroppable;
	m_iWeapon = iWeapon;
	m_iRune = iRune;
}

IDraggable& CRuneComboDraggable::MakeCopy()
{
	CRuneComboDraggable* pIcon = new CRuneComboDraggable(*this);
	return *pIcon;
}

void CRuneComboDraggable::Paint()
{
	CGUIRuneCombo::Paint();
}

void CRuneComboDraggable::Paint(int x, int y)
{
	CGUIRuneCombo::Paint(x, y);
}

void CRuneComboDraggable::Paint(int x, int y, int w, int h)
{
	CGUIRuneCombo::Paint(x, y, w, h);
}

void CRuneComboDraggable::Paint(int x, int y, int w, int h, int a)
{
	CGUIRuneCombo::Paint(x, y, w, h, a);
}

void CRuneComboDraggable::Paint(int x, int y, int w, int h, bool bFloating)
{
	CGUIRuneCombo::Paint(x, y, w, h);
}

void CRuneComboDraggable::SetHoldingRect(const CRect HoldingRect)
{
	int x = 0, y = 0;
	GetAbsPos(x, y);

	CRect NewRect(
		HoldingRect.x - x,
		HoldingRect.y - y,
		HoldingRect.w,
		HoldingRect.h
	);

	SetDimensions(NewRect);
}

CRect CRuneComboDraggable::GetHoldingRect()
{
	CRect r;
	GetAbsDimensions(r.x, r.y, r.w, r.h);
	return r;
}

void CRuneComboDraggable::GetAbsPos(int &x, int &y)
{
	if (GetDroppable())
	{
		const CRect ParentRect = GetDroppable()->GetHoldingRect();
		x = ParentRect.x;
		y = ParentRect.y;
		return;
	}
	Assert(false);
}
