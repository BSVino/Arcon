#include "cbase.h"
#include "hud.h"
#include "iclientmode.h"
#include <vgui/ISurface.h>
#include <vgui/IPanel.h>
#include <vgui/ILocalize.h>
#include "input.h"
#include "viewport_panel_names.h"

#include "cfui_gui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace cfgui;

CCFHudTexture* CButton::s_pButtonL = NULL;
CCFHudTexture* CButton::s_pButtonR = NULL;
CCFHudTexture* CButton::s_pButtonT = NULL;
CCFHudTexture* CButton::s_pButtonB = NULL;
CCFHudTexture* CButton::s_pButtonTL = NULL;
CCFHudTexture* CButton::s_pButtonTR = NULL;
CCFHudTexture* CButton::s_pButtonBL = NULL;
CCFHudTexture* CButton::s_pButtonBR = NULL;
CCFHudTexture* CButton::s_pButtonC = NULL;

CCFHudTexture* CButton::s_pButtonHL = NULL;
CCFHudTexture* CButton::s_pButtonHR = NULL;
CCFHudTexture* CButton::s_pButtonHT = NULL;
CCFHudTexture* CButton::s_pButtonHB = NULL;
CCFHudTexture* CButton::s_pButtonHTL = NULL;
CCFHudTexture* CButton::s_pButtonHTR = NULL;
CCFHudTexture* CButton::s_pButtonHBL = NULL;
CCFHudTexture* CButton::s_pButtonHBR = NULL;
CCFHudTexture* CButton::s_pButtonHC = NULL;

CCFHudTexture* CButton::s_pButtonDL = NULL;
CCFHudTexture* CButton::s_pButtonDR = NULL;
CCFHudTexture* CButton::s_pButtonDT = NULL;
CCFHudTexture* CButton::s_pButtonDB = NULL;
CCFHudTexture* CButton::s_pButtonDTL = NULL;
CCFHudTexture* CButton::s_pButtonDTR = NULL;
CCFHudTexture* CButton::s_pButtonDBL = NULL;
CCFHudTexture* CButton::s_pButtonDBR = NULL;
CCFHudTexture* CButton::s_pButtonDC = NULL;

CCFHudTexture* CButton::s_pButtonIL = NULL;
CCFHudTexture* CButton::s_pButtonIR = NULL;
CCFHudTexture* CButton::s_pButtonIT = NULL;
CCFHudTexture* CButton::s_pButtonIB = NULL;
CCFHudTexture* CButton::s_pButtonITL = NULL;
CCFHudTexture* CButton::s_pButtonITR = NULL;
CCFHudTexture* CButton::s_pButtonIBL = NULL;
CCFHudTexture* CButton::s_pButtonIBR = NULL;
CCFHudTexture* CButton::s_pButtonIC = NULL;

CCFHudTexture* CPanel::s_pPanelL = NULL;
CCFHudTexture* CPanel::s_pPanelR = NULL;
CCFHudTexture* CPanel::s_pPanelT = NULL;
CCFHudTexture* CPanel::s_pPanelB = NULL;
CCFHudTexture* CPanel::s_pPanelTL = NULL;
CCFHudTexture* CPanel::s_pPanelTR = NULL;
CCFHudTexture* CPanel::s_pPanelBL = NULL;
CCFHudTexture* CPanel::s_pPanelBR = NULL;
CCFHudTexture* CPanel::s_pPanelC = NULL;

CCFHudTexture* CPanel::s_pPanelHL = NULL;
CCFHudTexture* CPanel::s_pPanelHR = NULL;
CCFHudTexture* CPanel::s_pPanelHT = NULL;
CCFHudTexture* CPanel::s_pPanelHB = NULL;
CCFHudTexture* CPanel::s_pPanelHTL = NULL;
CCFHudTexture* CPanel::s_pPanelHTR = NULL;
CCFHudTexture* CPanel::s_pPanelHBL = NULL;
CCFHudTexture* CPanel::s_pPanelHBR = NULL;
CCFHudTexture* CPanel::s_pPanelHC = NULL;

CCFHudTexture* CSlidingPanel::s_pArrowExpanded = NULL;
CCFHudTexture* CSlidingPanel::s_pArrowCollapsed = NULL;

int CInputManager::s_iActivated = 0;

#if _DEBUG
#define CFGUI_SHOWUNLOCALIZED "1"
#else
#define CFGUI_SHOWUNLOCALIZED "0"
#endif

static ConVar cfgui_showunlocalized("cfgui_showunlocalized", CFGUI_SHOWUNLOCALIZED);

// Hard coded width and height, pretending the screen is this wide.
// The values are scaled just before drawing.
int cfgui::CFScreenWidth()
{
	return 1280;
}

int cfgui::CFScreenHeight()
{
	return 800;
}

float cfgui::CFScreenWidthScale()
{
	return (float)ScreenWidth() / (float)CFScreenWidth();
}

float cfgui::CFScreenHeightScale()
{
	return (float)ScreenHeight() / (float)CFScreenHeight();
}

void CCFHudTexture::DrawSelf( int x, int y, Color& clr ) const
{
	float flXScale = CFScreenWidthScale();
	float flYScale = CFScreenHeightScale();
	CHudTexture::DrawSelf( x * flXScale, y * flYScale, clr );
}

void CCFHudTexture::DrawSelf( int x, int y, int w, int h, Color& clr ) const
{
	float flXScale = CFScreenWidthScale();
	float flYScale = CFScreenHeightScale();
	CHudTexture::DrawSelf( x * flXScale, y * flYScale, w * flXScale, h * flYScale, clr );
}

void CCFHudTexture::DrawSelfCropped( int x, int y, int cropx, int cropy, int cropw, int croph, int finalWidth, int finalHeight, Color& clr ) const
{
	float flXScale = CFScreenWidthScale();
	float flYScale = CFScreenHeightScale();
	CHudTexture::DrawSelfCropped( x * flXScale, y * flYScale, cropx, cropy, cropw, croph, finalWidth * flXScale, finalHeight * flYScale, clr );
}

CCFHudTexture* cfgui::GetHudTexture(const char* pszName)
{
	return ToCFTex(gHUD.GetIcon(pszName));
}

CBaseControl::CBaseControl(int x, int y, int w, int h)
{
	SetParent(NULL);
	m_iX = x;
	m_iY = y;
	m_iW = w;
	m_iH = h;
	m_bVisible = true;
}

CBaseControl::CBaseControl(const CRect& Rect)
{
	CBaseControl(Rect.x, Rect.y, Rect.w, Rect.h);
}

void CBaseControl::Destructor()
{
	if (GetParent())
	{
		CPanel *pPanel = dynamic_cast<CPanel*>(GetParent());
		if (pPanel)
			pPanel->RemoveControl(this);
	}

	// Parent is IControl, which is virtual.
}

void CBaseControl::GetAbsPos(int &x, int &y)
{
	int px = 0;
	int py = 0;
	if (GetParent())
		GetParent()->GetAbsPos(px, py);
	x = m_iX + px;
	y = m_iY + py;
}

void CBaseControl::GetAbsDimensions(int &x, int &y, int &w, int &h)
{
	GetAbsPos(x, y);
	w = m_iW;
	h = m_iH;
}

bool CBaseControl::IsVisible()
{
	if (GetParent() && !GetParent()->IsVisible())
		return false;
	
	return m_bVisible;
}

#ifdef _DEBUG
void CBaseControl::PaintDebugRect(int x, int y, int w, int h)
{
	CPanel::s_pPanelHTL->DrawSelf	(x,			y,			3,		3,		Color(255, 255, 255, 255));
	CPanel::s_pPanelHT->DrawSelf	(x+3,		y,			w-6,	3,		Color(255, 255, 255, 255));
	CPanel::s_pPanelHTR->DrawSelf	(x+w-3,		y,			3,		3,		Color(255, 255, 255, 255));
	CPanel::s_pPanelHL->DrawSelf	(x,			y+3,		3,		h-6,	Color(255, 255, 255, 255));
	CPanel::s_pPanelHC->DrawSelf	(x+3,		y+3,		w-6,	h-6,	Color(255, 255, 255, 255));
	CPanel::s_pPanelHR->DrawSelf	(x+w-3,		y+3,		3,		h-6,	Color(255, 255, 255, 255));
	CPanel::s_pPanelHBL->DrawSelf	(x,			y+h-3,		3,		3,		Color(255, 255, 255, 255));
	CPanel::s_pPanelHB->DrawSelf	(x+3,		y+h-3,		w-6,	3,		Color(255, 255, 255, 255));
	CPanel::s_pPanelHBR->DrawSelf	(x+w-3,		y+h-3,		3,		3,		Color(255, 255, 255, 255));
}
#endif

CPanel::CPanel(int x, int y, int w, int h)
	: CBaseControl(x, y, w, h)
{
	SetBorder(BT_SOME);
	m_pHasCursor = NULL;
	m_bHighlight = false;
	m_bDestructing = false;
}

void CPanel::Destructor()
{
	// Protect m_apControls from accesses elsewhere.
	m_bDestructing = true;

	int iCount = m_apControls.Count();
	int i;
	for (i = 0; i < iCount; i++)
	{
		// Christ.
		IControl* pControl = m_apControls[i];
		pControl->Destructor();
		pControl->Delete();
	}
	m_apControls.Purge();

	m_bDestructing = false;

	CBaseControl::Destructor();
}

void CPanel::LevelShutdown( void )
{
	int iCount = m_apControls.Count();
	for (int i = 0; i < iCount; i++)
	{
		IControl* pControl = m_apControls[i];
		pControl->LevelShutdown();
	}
}

bool CPanel::KeyPressed(vgui::KeyCode code)
{
	int iCount = m_apControls.Count();

	// Start at the end of the list so that items drawn last are tested for keyboard events first.
	for (int i = iCount-1; i >= 0; i--)
	{
		IControl* pControl = m_apControls[i];

		if (!pControl->IsVisible())
			continue;

		if (pControl->KeyPressed(code))
			return true;
	}
	return false;
}

bool CPanel::KeyReleased(vgui::KeyCode code)
{
	int iCount = m_apControls.Count();

	// Start at the end of the list so that items drawn last are tested for keyboard events first.
	for (int i = iCount-1; i >= 0; i--)
	{
		IControl* pControl = m_apControls[i];

		if (!pControl->IsVisible())
			continue;

		if (pControl->KeyReleased(code))
			return true;
	}
	return false;
}

bool CPanel::MousePressed(vgui::MouseCode code)
{
	int mx, my;
	CRootPanel::GetFullscreenMousePos(mx, my);

	int iCount = m_apControls.Count();
	// Start at the end of the list so that items drawn last are tested for mouse events first.
	for (int i = iCount-1; i >= 0; i--)
	{
		IControl* pControl = m_apControls[i];

		if (!pControl->IsVisible())
			continue;

		int x = 0, y = 0, w = 0, h = 0;
		pControl->GetAbsDimensions(x, y, w, h);
		if (mx >= x &&
			my >= y &&
			mx < x + w &&
			my < y + h)
		{
			if (pControl->MousePressed(code))
				return true;
		}
	}
	return false;
}

bool CPanel::MouseReleased(vgui::MouseCode code)
{
	int mx, my;
	CRootPanel::GetFullscreenMousePos(mx, my);

	int iCount = m_apControls.Count();
	// Start at the end of the list so that items drawn last are tested for mouse events first.
	for (int i = iCount-1; i >= 0; i--)
	{
		IControl* pControl = m_apControls[i];

		if (!pControl->IsVisible())
			continue;

		int x, y, w, h;
		pControl->GetAbsDimensions(x, y, w, h);
		if (mx >= x &&
			my >= y &&
			mx < x + w &&
			my < y + h)
		{
			if (pControl->MouseReleased(code))
				return true;
		}
	}
	return false;
}

void CPanel::CursorMoved(int mx, int my)
{
	bool bFoundControlWithCursor = false;

	int iCount = m_apControls.Count();
	// Start at the end of the list so that items drawn last are tested for mouse events first.
	for (int i = iCount-1; i >= 0; i--)
	{
		IControl* pControl = m_apControls[i];

		if (!pControl->IsVisible() || !pControl->IsCursorListener())
			continue;

		int x, y, w, h;
		pControl->GetAbsDimensions(x, y, w, h);
		if (mx >= x &&
			my >= y &&
			mx < x + w &&
			my < y + h)
		{
			if (m_pHasCursor != pControl)
			{
				if (m_pHasCursor)
				{
					m_pHasCursor->CursorOut();
				}
				m_pHasCursor = pControl;
				m_pHasCursor->CursorIn();
			}

			pControl->CursorMoved(mx, my);

			bFoundControlWithCursor = true;
			break;
		}
	}

	if (!bFoundControlWithCursor && m_pHasCursor)
	{
		m_pHasCursor->CursorOut();
		m_pHasCursor = NULL;
	}
}

void CPanel::CursorOut()
{
	if (m_pHasCursor)
	{
		m_pHasCursor->CursorOut();
		m_pHasCursor = NULL;
	}
}

void CPanel::AddControl(IControl* pControl, bool bToTail)
{
	if (!pControl)
		return;

#ifdef _DEBUG
	for (int i = 0; i < m_apControls.Count(); i++)
		Assert(m_apControls[i] != pControl);	// You're adding a control to the panel twice! Quit it!
#endif

	pControl->SetParent(this);

	if (bToTail)
		m_apControls.AddToTail(pControl);
	else
		m_apControls.AddToHead(pControl);
}

void CPanel::RemoveControl(IControl* pControl)
{
	// If we are destructing then this RemoveControl is being called from this CPanel's
	// destructor's m_apControls[i]->Destructor() so we should not delete this element
	// because it will be m_apControls.Purge()'d later.
	if (!m_bDestructing)
		m_apControls.FindAndRemove(pControl);

	if (m_pHasCursor == pControl)
		m_pHasCursor = NULL;
}

void CPanel::Layout( void )
{
	int iCount = m_apControls.Count();
	for (int i = 0; i < iCount; i++)
	{
		m_apControls[i]->Layout();
	}
}

void CPanel::Paint()
{
	int x = 0, y = 0;
	GetAbsPos(x, y);
	Paint(x, y);
}

void CPanel::Paint(int x, int y)
{
	Paint(x, y, m_iW, m_iH);
}

void CPanel::Paint(int x, int y, int w, int h)
{
	if (!IsVisible())
		return;

	if (m_eBorder == BT_SOME)
		PaintBorder(x, y, w, h);

	int iCount = m_apControls.Count();
	for (int i = 0; i < iCount; i++)
	{
		if (!m_apControls[i]->IsVisible())
			continue;

		// Translate this location to the child's local space.
		int cx, cy, ax, ay;
		m_apControls[i]->GetAbsPos(cx, cy);
		GetAbsPos(ax, ay);
		m_apControls[i]->Paint(cx+x-ax, cy+y-ay);
	}
}

void CPanel::PaintBorder(int x, int y, int w, int h)
{
	if (m_bHighlight)
	{
		CPanel::s_pPanelHTL->DrawSelf	(x,			y,			3,		3,		Color(255, 255, 255, 255));
		CPanel::s_pPanelHT->DrawSelf	(x+3,		y,			w-6,	3,		Color(255, 255, 255, 255));
		CPanel::s_pPanelHTR->DrawSelf	(x+w-3,		y,			3,		3,		Color(255, 255, 255, 255));
		CPanel::s_pPanelHL->DrawSelf	(x,			y+3,		3,		h-6,	Color(255, 255, 255, 255));
		CPanel::s_pPanelHC->DrawSelf	(x+3,		y+3,		w-6,	h-6,	Color(255, 255, 255, 255));
		CPanel::s_pPanelHR->DrawSelf	(x+w-3,		y+3,		3,		h-6,	Color(255, 255, 255, 255));
		CPanel::s_pPanelHBL->DrawSelf	(x,			y+h-3,		3,		3,		Color(255, 255, 255, 255));
		CPanel::s_pPanelHB->DrawSelf	(x+3,		y+h-3,		w-6,	3,		Color(255, 255, 255, 255));
		CPanel::s_pPanelHBR->DrawSelf	(x+w-3,		y+h-3,		3,		3,		Color(255, 255, 255, 255));
	}
	else
	{
		CPanel::s_pPanelTL->DrawSelf(x,			y,			3,		3,		Color(255, 255, 255, 255));
		CPanel::s_pPanelT->DrawSelf	(x+3,		y,			w-6,	3,		Color(255, 255, 255, 255));
		CPanel::s_pPanelTR->DrawSelf(x+w-3,		y,			3,		3,		Color(255, 255, 255, 255));
		CPanel::s_pPanelL->DrawSelf	(x,			y+3,		3,		h-6,	Color(255, 255, 255, 255));
		CPanel::s_pPanelC->DrawSelf	(x+3,		y+3,		w-6,	h-6,	Color(255, 255, 255, 255));
		CPanel::s_pPanelR->DrawSelf	(x+w-3,		y+3,		3,		h-6,	Color(255, 255, 255, 255));
		CPanel::s_pPanelBL->DrawSelf(x,			y+h-3,		3,		3,		Color(255, 255, 255, 255));
		CPanel::s_pPanelB->DrawSelf	(x+3,		y+h-3,		w-6,	3,		Color(255, 255, 255, 255));
		CPanel::s_pPanelBR->DrawSelf(x+w-3,		y+h-3,		3,		3,		Color(255, 255, 255, 255));
	}
}

void CPanel::Think()
{
	int iCount = m_apControls.Count();
	for (int i = 0; i < iCount; i++)
	{
		m_apControls[i]->Think();
	}
}

void CPanel::PostRenderVGui()
{
	if (!IsVisible())
		return;

	int iCount = m_apControls.Count();
	for (int i = 0; i < iCount; i++)
	{
		m_apControls[i]->PostRenderVGui();
	}
}

CDroppablePanel::CDroppablePanel(int x, int y, int w, int h)
	: CPanel(x, y, w, h)
{
	m_bGrabbable = true;

	CRootPanel::GetRoot()->AddDroppable(this);
};

void CDroppablePanel::Destructor()
{
	if (m_apDraggables.Count())
	{
		for (int i = 0; i < m_apDraggables.Count(); i++)
		{
			m_apDraggables[i]->Destructor();
			m_apDraggables[i]->Delete();
		}
	}

	if (CRootPanel::GetRoot())
		CRootPanel::GetRoot()->RemoveDroppable(this);

	CPanel::Destructor();
}

void CDroppablePanel::Paint(int x, int y, int w, int h)
{
	if (!IsVisible())
		return;

	for (int i = 0; i < m_apDraggables.Count(); i++)
	{
		// Translate this location to the child's local space.
		int ax, ay;
		CRect c = m_apDraggables[i]->GetHoldingRect();
		GetAbsPos(ax, ay);
		m_apDraggables[i]->Paint(c.x+x-ax, c.y+y-ay);
	}

	CPanel::Paint(x, y, w, h);
}

void CDroppablePanel::SetSize(int w, int h)
{
	CPanel::SetSize(w, h);
	for (int i = 0; i < m_apDraggables.Count(); i++)
		m_apDraggables[i]->SetHoldingRect(GetHoldingRect());
}

void CDroppablePanel::SetPos(int x, int y)
{
	CPanel::SetPos(x, y);
	for (int i = 0; i < m_apDraggables.Count(); i++)
		m_apDraggables[i]->SetHoldingRect(GetHoldingRect());
}

bool CDroppablePanel::MousePressed(vgui::MouseCode code)
{
	if (!IsVisible())
		return false;

	if (m_bGrabbable && m_apDraggables.IsValidIndex(0))
	{
		int mx, my;
		CRootPanel::GetFullscreenMousePos(mx, my);

		CRect r = GetHoldingRect();
		if (code == MOUSE_LEFT &&
			mx >= r.x &&
			my >= r.y &&
			mx < r.x + r.w &&
			my < r.y + r.h)
		{
			CRootPanel::GetRoot()->DragonDrop(this);
			return true;
		}
	}

	return CPanel::MousePressed(code);
}

void CDroppablePanel::SetDraggable(IDraggable* pDragged, bool bDelete)
{
	ClearDraggables(bDelete);

	AddDraggable(pDragged);
}

void CDroppablePanel::AddDraggable(IDraggable* pDragged)
{
	if (pDragged)
	{
		m_apDraggables.AddToTail(pDragged);
		pDragged->SetHoldingRect(GetHoldingRect());
		pDragged->SetDroppable(this);
	}
}

void CDroppablePanel::ClearDraggables(bool bDelete)
{
	if (bDelete)
	{
		for (int i = 0; i < m_apDraggables.Count(); i++)
		{
			m_apDraggables[i]->Destructor();
			m_apDraggables[i]->Delete();
		}
	}

	m_apDraggables.RemoveAll();
}

IDraggable* CDroppablePanel::GetDraggable(int i)
{
	return m_apDraggables[i];
}

CLabel::CLabel(int x, int y, int w, int h, char* pszText)
	: CBaseControl(x, y, w, h)
{
	m_bEnabled = true;
	m_bWrap = true;
	m_pszText = NULL;
	m_iTotalLines = 0;
	m_eAlign = TA_CENTER;
	m_FGColor = Color(255, 255, 255, 255);

	SetText(pszText);
}

void CLabel::Destructor()
{
	if (m_pszText)
		free(m_pszText);

	CBaseControl::Destructor();
}

void CLabel::Paint(int x, int y, int w, int h)
{
	if (!IsVisible())
		return;

	float flXScale = CFScreenWidthScale();
	float flYScale = CFScreenHeightScale();

	x *= flXScale;
	y *= flYScale;
	w *= flXScale;
	h *= flYScale;

	Color FGColor = m_FGColor;
	if (!m_bEnabled)
		FGColor.SetColor(m_FGColor.r()/2, m_FGColor.g()/2, m_FGColor.b()/2, 255);

	wchar_t* pszSeps = L"\n";
	wchar_t* pszText = wcsdup(m_pszText);
	wchar_t* pszTok = wcstok(pszText, pszSeps);
	m_iLine = 0;

	while (pszTok)
	{
		vgui::surface()->DrawSetTextFont(CRootPanel::s_hDefaultFont);
		vgui::surface()->DrawSetTextColor(FGColor);

		int tw, th;
		vgui::surface()->GetTextSize(CRootPanel::s_hDefaultFont, pszTok, tw, th);
		int t = vgui::surface()->GetFontTall(CRootPanel::s_hDefaultFont);

		if (!m_bWrap || tw < w || w == 0 || (m_iLine+1)*t > h)
		{
			DrawLine(pszTok, wcslen(pszTok), x, y, w, h);

			m_iLine++;
		}
		else
		{
			tw = 0;
			unsigned int iSource = 0;
			int iLastSpace = 0, iLastBreak = 0, iLength = 0;
			while (iSource < wcslen(pszTok))
			{
				int cw = vgui::surface()->GetCharacterWidth(CRootPanel::s_hDefaultFont, pszTok[iSource]);
				if (tw + cw < w || (tw == 0 && w < cw) || (m_iLine+1)*t > h)
				{
					iLength++;
					if (pszTok[iSource] == L' ')
						iLastSpace = iSource;
					tw += cw;
				}
				else
				{
					int iBackup = iSource - iLastSpace;
					if (iLastSpace == iLastBreak)
						iBackup = 0;

					iSource -= iBackup;
					iLength -= iBackup;

					DrawLine(pszTok + iLastBreak, iLength, x, y, w, h);

					iLength = tw = 0;
					while (iSource < wcslen(pszTok) && pszTok[iSource] == L' ')
						iSource++;
					iLastBreak = iLastSpace = iSource--;	// Skip over any following spaces, but leave iSource at the space 'cause it's incremented again below.
					m_iLine++;
				}

				iSource++;
			}

			DrawLine(pszTok + iLastBreak, iLength, x, y, w, h);
			m_iLine++;
		}

		pszTok = wcstok(NULL, pszSeps);
	}

	free(pszText);
}

void CLabel::DrawLine(wchar_t* pszText, unsigned iLength, int x, int y, int w, int h)
{
	int lw = GetTextWidth(CRootPanel::s_hDefaultFont, pszText, iLength);
	int t = vgui::surface()->GetFontTall(CRootPanel::s_hDefaultFont);
	int th = GetTextHeight();

	if (m_eAlign == TA_CENTER)
		vgui::surface()->DrawSetTextPos(x + w/2 - lw/2, y + h/2 - th/2 + m_iLine*t);
	else if (m_eAlign == TA_LEFTCENTER)
		vgui::surface()->DrawSetTextPos(x, y + h/2 - th/2 + m_iLine*t);
	else if (m_eAlign == TA_RIGHTCENTER)
		vgui::surface()->DrawSetTextPos(x + w - lw, y + h/2 - th/2 + m_iLine*t);
	else	// TA_TOPLEFT
		vgui::surface()->DrawSetTextPos(x, y + m_iLine*t);
	vgui::surface()->DrawPrintText(pszText, iLength);
}

void CLabel::SetSize(int w, int h)
{
	CBaseControl::SetSize(w, h);
	ComputeLines();
}

void CLabel::SetText(const wchar_t* pszText)
{
	if (m_pszText)
		free(m_pszText);
	m_pszText = NULL;

	if (!pszText)
		m_pszText = wcsdup(L"");
	else
		m_pszText = wcsdup(pszText);

	ComputeLines();
}

void CLabel::SetText(const char* pszText)
{
	if (m_pszText)
		free(m_pszText);
	m_pszText = NULL;

	if (!pszText)
		SetText(L"");
	else
	{
		if (pszText[0] == '#')
		{
			wchar_t *pszLocalized = g_pVGuiLocalize->Find( pszText );
			if (pszLocalized)
			{
				SetText(pszLocalized);
				return;
			}
		}
		
		int iSize = (strlen(pszText) + 1) * sizeof(wchar_t);
		wchar_t* pszBuf = (wchar_t*)malloc(iSize);
		g_pVGuiLocalize->ConvertANSIToUnicode( pszText, pszBuf, iSize );

		if (cfgui_showunlocalized.GetBool())
		{
			// So that we know which strings aren't localized.
			if (wcslen(pszBuf) > 0 && pszBuf[0] != L'#')
				pszBuf[0] = L'!';
		}

		SetText(pszBuf);
		free(pszBuf);
	}
}

void CLabel::AppendText(const wchar_t* pszText)
{
	if (!pszText)
		return;

	const wchar_t* pszCurr = GetText();

	int iLength = wcslen(pszText) + wcslen(pszCurr) + 1;

	int iSize = iLength * sizeof(wchar_t);
	wchar_t* pszBuf = (wchar_t*)malloc(iSize);

	wcscpy(pszBuf, pszCurr);
	wcscat(pszBuf, pszText);

	SetText(pszBuf);
	free(pszBuf);
}

void CLabel::AppendText(const char* pszText)
{
	if (!pszText)
		return;

	if (pszText[0] == '#')
	{
		wchar_t *pszLocalized = g_pVGuiLocalize->Find( pszText );
		if (pszLocalized)
		{
			AppendText(pszLocalized);
			return;
		}
	}

	int iSize = (strlen(pszText) + 1) * sizeof(wchar_t);
	wchar_t* pszBuf = (wchar_t*)malloc(iSize);
	g_pVGuiLocalize->ConvertANSIToUnicode( pszText, pszBuf, iSize );
#ifdef _DEBUG
	// So that we know which strings aren't localized.
	if (wcslen(pszBuf) > 0 && pszBuf[0] != L'#')
		pszBuf[0] = L'!';
#endif
	AppendText(pszBuf);
	free(pszBuf);
}

int CLabel::GetTextWidth()
{
	int w = 0, h = 0;
	vgui::surface()->GetTextSize(CRootPanel::s_hDefaultFont, m_pszText, w, h);

	return w;
}

int CLabel::GetTextHeight()
{
	int t = vgui::surface()->GetFontTall(CRootPanel::s_hDefaultFont);

	return t * m_iTotalLines;
}

void CLabel::ComputeLines(int w, int h)
{
	if (w == -1)
		w = m_iW;

	if (h == -1)
		h = m_iH;

	wchar_t* pszSeps = L"\n";
	wchar_t* pszText = wcsdup(m_pszText);

	// Cut off any ending line returns so that labels don't have hanging space below.
	if (pszText[wcslen(pszText)-1] == L'\n')
		pszText[wcslen(pszText)-1] = L'\0';

	// FIXME: All this code is technically duplicated from Paint(),
	// but I can't think of a good way to reconcile them. Some kind
	// of lineating method is required or something...? We need to
	// add up all the lines as if they were being truncated during
	// printing to get the real height of all the text.
	wchar_t* pszTok = wcstok(pszText, pszSeps);

	m_iTotalLines = 0;

	while (pszTok)
	{
		int tw, th;
		vgui::surface()->GetTextSize(CRootPanel::s_hDefaultFont, pszText, tw, th);
		int t = vgui::surface()->GetFontTall(CRootPanel::s_hDefaultFont);

		if (!m_bWrap || tw < w || w == 0 || (m_iTotalLines+1)*t > h)
		{
			m_iTotalLines++;
		}
		else
		{
			tw = 0;
			unsigned int iSource = 0;
			int iLastSpace = 0, iLastBreak = 0, iLength = 0;
			while (iSource < wcslen(pszTok))
			{
				int cw = vgui::surface()->GetCharacterWidth(CRootPanel::s_hDefaultFont, pszTok[iSource]);
				if (tw + cw < w || (tw == 0 && w < cw) || (m_iTotalLines+1)*t > h)
				{
					iLength++;
					if (pszTok[iSource] == L' ')
						iLastSpace = iSource;
					tw += cw;
				}
				else
				{
					int iBackup = iSource - iLastSpace;
					if (iLastSpace == iLastBreak)
						iBackup = 0;

					iSource -= iBackup;
					iLength -= iBackup;

					iLength = tw = 0;
					while (iSource < wcslen(pszTok) && pszTok[iSource] == L' ')
						iSource++;
					iLastBreak = iLastSpace = iSource--;	// Skip over any following spaces, but leave iSource at the space 'cause it's incremented again below.
					m_iTotalLines++;
				}

				iSource++;
			}

			m_iTotalLines++;
		}

		pszTok = wcstok(NULL, pszSeps);
	}

	free(pszText);
}

// Make the label tall enough for one line of text to fit inside.
void CLabel::EnsureTextFits()
{
	int w = GetTextWidth();
	int h = GetTextHeight();

	if (m_iH < h+4)
		SetSize(m_iW, h+4);

	if (m_iW < w+4)
		SetSize(w+4, m_iH);
}

int CLabel::GetTextWidth( vgui::HFont& font, const wchar_t *str, int iLength )
{
	int pixels = 0;
	wchar_t *p = (wchar_t *)str;
	while ( *p && p-str < iLength )
	{
		pixels += vgui::surface()->GetCharacterWidth( font, *p++ );
	}
	return pixels;
}

const wchar_t* CLabel::GetText()
{
	if (!m_pszText)
		return L"";
	else
		return m_pszText;
}

Color CLabel::GetFGColor()
{
	return m_FGColor;
}

void CLabel::SetFGColor(Color FGColor)
{
	m_FGColor = FGColor;
}

CButton::CButton(int x, int y, int w, int h, char* pszText, bool bToggle)
	: CLabel(x, y, w, h, pszText)
{
	m_bToggle = bToggle;
	m_bToggleOn = false;
	m_bDown = false;
	m_bHighlight = false;
	m_pClickListener = NULL;
	m_pfnClickCallback = NULL;
	m_pClickParms = NULL;
	m_pUnclickListener = NULL;
	m_pfnUnclickCallback = NULL;
	m_pUnclickParms = NULL;
}

void CButton::Destructor()
{
	if (m_pClickParms)
		m_pClickParms->deleteThis();

	CLabel::Destructor();
}

void CButton::SetToggleState(bool bState)
{
	if (m_bDown == bState)
		return;

	m_bToggleOn = m_bDown = bState;
}

bool CButton::Push()
{
	if (!m_bEnabled)
		return false;

	if (m_bDown && !m_bToggle)
		return false;

	m_bDown = true;

	if (m_bToggle)
		m_bToggleOn = !m_bToggleOn;

	return true;
}

bool CButton::Pop(bool bRegister, bool bReverting)
{
	if (!m_bDown)
		return false;

	if (m_bToggle)
	{
		if (bReverting)
			m_bToggleOn = !m_bToggleOn;

		if (m_bToggleOn)
			SetState(true, bRegister);
		else
			SetState(false, bRegister);
	}
	else
		SetState(false, bRegister);

	return true;
}

void CButton::SetState(bool bDown, bool bRegister)
{
	m_bDown = bDown;

	if (m_bToggle)
		m_bToggleOn = bDown;

	if (m_bToggle && !m_bToggleOn)
	{
		if (bRegister && m_pUnclickListener && m_pfnUnclickCallback)
			m_pfnUnclickCallback(m_pUnclickListener, m_pUnclickParms);
	}
	else
	{
		if (bRegister && m_pClickListener && m_pfnClickCallback)
			m_pfnClickCallback(m_pClickListener, m_pClickParms);
	}
}

void CButton::SetClickedListener(IEventListener* pListener, IEventListener::Callback pfnCallback, KeyValues* pParms)
{
	Assert(pListener && pfnCallback || !pListener && !pfnCallback && !pParms);
	m_pClickListener = pListener;
	m_pfnClickCallback = pfnCallback;

	if (m_pClickParms)
		m_pClickParms->deleteThis();

	m_pClickParms = pParms;
}

void CButton::SetUnclickedListener(IEventListener* pListener, IEventListener::Callback pfnCallback, KeyValues* pParms)
{
	Assert(pListener && pfnCallback || !pListener && !pfnCallback && !pParms);
	m_pUnclickListener = pListener;
	m_pfnUnclickCallback = pfnCallback;

	if (m_pUnclickParms)
		m_pUnclickParms->deleteThis();

	m_pUnclickParms = pParms;
}

bool CButton::MousePressed(vgui::MouseCode code)
{
	if (!IsVisible())
		return CLabel::MousePressed(code);

	bool bUsed = false;
	if (code == MOUSE_LEFT)
	{
		bUsed = Push();
		CRootPanel::GetRoot()->SetButtonDown(this);
	}
	return bUsed;
}

bool CButton::MouseReleased(vgui::MouseCode code)
{
	if (!IsVisible())
		return CLabel::MouseReleased(code);

	if (CRootPanel::GetRoot()->GetButtonDown() != this)
		return false;

	bool bUsed = false;
	if (code == MOUSE_LEFT)
	{
		bUsed = Pop();
		CRootPanel::GetRoot()->SetButtonDown(NULL);
	}
	return bUsed;
}

void CButton::CursorIn()
{
	CLabel::CursorIn();

	m_bHighlight = true;
}

void CButton::CursorOut()
{
	CLabel::CursorOut();

	m_bHighlight = false;
}

void CButton::Paint(int x, int y, int w, int h)
{
	if (!IsVisible())
		return;

	PaintButton(x, y, w, h);

	// Now paint the text which appears on the button.
	CLabel::Paint(x, y, w, h);
}

void CButton::PaintButton(int x, int y, int w, int h)
{
	if (!m_bEnabled)
	{
		CButton::s_pButtonITL->DrawSelf	(x,			y,			3,		3,		Color(255, 255, 255, 255));
		CButton::s_pButtonIT->DrawSelf	(x+3,		y,			w-6,	3,		Color(255, 255, 255, 255));
		CButton::s_pButtonITR->DrawSelf	(x+w-3,		y,			3,		3,		Color(255, 255, 255, 255));
		CButton::s_pButtonIL->DrawSelf	(x,			y+3,		3,		h-6,	Color(255, 255, 255, 255));
		CButton::s_pButtonIC->DrawSelf	(x+3,		y+3,		w-6,	h-6,	Color(255, 255, 255, 255));
		CButton::s_pButtonIR->DrawSelf	(x+w-3,		y+3,		3,		h-6,	Color(255, 255, 255, 255));
		CButton::s_pButtonIBL->DrawSelf	(x,			y+h-3,		3,		3,		Color(255, 255, 255, 255));
		CButton::s_pButtonIB->DrawSelf	(x+3,		y+h-3,		w-6,	3,		Color(255, 255, 255, 255));
		CButton::s_pButtonIBR->DrawSelf	(x+w-3,		y+h-3,		3,		3,		Color(255, 255, 255, 255));
	}
	else if (m_bDown)
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
}

CSlidingPanel::CInnerPanel::CInnerPanel(CSlidingContainer* pMaster)
	: CPanel(0, 0, 100, SLIDER_COLLAPSED_HEIGHT)
{
	m_pMaster = pMaster;
}

bool CSlidingPanel::CInnerPanel::IsVisible()
{
	if (!m_pMaster->IsCurrent(dynamic_cast<CSlidingPanel*>(m_pParent)))
		return false;

	return CPanel::IsVisible();
}

CSlidingPanel::CSlidingPanel(CSlidingContainer* pParent, char* pszTitle)
	: CPanel(0, 0, 100, 5)
{
	Assert(pParent);

	m_bCurrent = false;

	m_pTitle = new CLabel(0, 0, 100, SLIDER_COLLAPSED_HEIGHT, pszTitle);
	AddControl(m_pTitle);

	m_pInnerPanel = new CInnerPanel(pParent);
	m_pInnerPanel->SetBorder(CPanel::BT_NONE);
	AddControl(m_pInnerPanel);

	// Add to tail so that panels appear in the order they are added.
	pParent->AddControl(this, true);
}

void CSlidingPanel::Layout()
{
	m_pTitle->SetSize(m_pParent->GetWidth(), SLIDER_COLLAPSED_HEIGHT);

	m_pInnerPanel->SetPos(5, SLIDER_COLLAPSED_HEIGHT);
	m_pInnerPanel->SetSize(GetWidth() - 10, GetHeight() - 5 - SLIDER_COLLAPSED_HEIGHT);

	CPanel::Layout();
}

void CSlidingPanel::Paint(int x, int y, int w, int h)
{
	if (!IsVisible())
		return;

	CButton::s_pButtonC->DrawSelf(x+3, y+3, w-6, SLIDER_COLLAPSED_HEIGHT-3, Color(255, 255, 255, 255));

	if (m_bCurrent)
		s_pArrowExpanded->DrawSelf(x, y, SLIDER_COLLAPSED_HEIGHT, SLIDER_COLLAPSED_HEIGHT, Color(255, 255, 255, 255));
	else
		s_pArrowCollapsed->DrawSelf(x, y, SLIDER_COLLAPSED_HEIGHT, SLIDER_COLLAPSED_HEIGHT, Color(255, 255, 255, 255));

	CPanel::Paint(x, y, w, h);
}

bool CSlidingPanel::MousePressed(vgui::MouseCode code)
{
	CSlidingContainer* pParent = dynamic_cast<CSlidingContainer*>(m_pParent);

	if (pParent->IsCurrent(this))
		return CPanel::MousePressed(code);
	else
	{
		pParent->SetCurrent(this);
		return true;
	}
}

void CSlidingPanel::AddControl(IControl* pControl, bool bToTail)
{
	// The title and inner panel should be added to this panel.
	// All other controls should be added to the inner panel.
	// This way the inner panel can be set not visible in order
	// to set all children not visible at once.

	if (pControl != m_pTitle && pControl != m_pInnerPanel)
	{
		m_pInnerPanel->AddControl(pControl, bToTail);
		return;
	}

	CPanel::AddControl(pControl, bToTail);
}

void CSlidingPanel::SetCurrent(bool bCurrent)
{
	m_bCurrent = bCurrent;

	m_pInnerPanel->SetVisible(bCurrent);
}

CSlidingContainer::CSlidingContainer(int x, int y, int w, int h)
	: CPanel(x, y, w, h)
{
	SetBorder(BT_NONE);
	SetCurrent(0);
}

void CSlidingContainer::Layout()
{
	int iY = 0;
	int iCount = m_apControls.Count();
	int iCurrentHeight = GetHeight() - CSlidingPanel::SLIDER_COLLAPSED_HEIGHT * (VisiblePanels()-1);

	for (int i = 0; i < iCount; i++)
	{
		if (!m_apControls[i]->IsVisible())
			continue;

		m_apControls[i]->SetPos(0, iY);
		m_apControls[i]->SetSize(GetWidth(), (i == m_iCurrent)?iCurrentHeight:CSlidingPanel::SLIDER_COLLAPSED_HEIGHT);

		iY += (i == m_iCurrent)?iCurrentHeight:CSlidingPanel::SLIDER_COLLAPSED_HEIGHT;
	}

	CPanel::Layout();
}

void CSlidingContainer::AddControl(IControl* pControl, bool bToTail)
{
	if (!pControl)
		return;

	AssertMsg(dynamic_cast<CSlidingPanel*>(pControl), "CSlidingContainer can only take 'CSlidingPanel' as children.");

	CPanel::AddControl(pControl, bToTail);

	// Re-layout now that we've added some. Maybe this one is the current one!
	SetCurrent(m_iCurrent);
}

bool CSlidingContainer::IsCurrent(int iPanel)
{
	return iPanel == m_iCurrent;
}

void CSlidingContainer::SetCurrent(int iPanel)
{
	if (m_apControls.IsValidIndex(m_iCurrent))
		dynamic_cast<CSlidingPanel*>(m_apControls[m_iCurrent])->SetCurrent(false);

	m_iCurrent = iPanel;

	// iPanel may be invalid, for example if the container is empty and being initialized to 0.
	if (m_apControls.IsValidIndex(m_iCurrent))
	{
		dynamic_cast<CSlidingPanel*>(m_apControls[m_iCurrent])->SetCurrent(true);

		Layout();
	}
}

bool CSlidingContainer::IsCurrent(CSlidingPanel* pPanel)
{
	return IsCurrent(m_apControls.Find(pPanel));
}

void CSlidingContainer::SetCurrent(CSlidingPanel* pPanel)
{
	SetCurrent(m_apControls.Find(pPanel));
}

bool CSlidingContainer::IsCurrentValid()
{
	if (!m_apControls.IsValidIndex(m_iCurrent))
		return false;

	if (!m_apControls[m_iCurrent]->IsVisible())
		return false;

	return true;
}

int CSlidingContainer::VisiblePanels()
{
	int iResult = 0;
	int iCount = m_apControls.Count();
	for (int i = 0; i < iCount; i++)
	{
		if (m_apControls[i]->IsVisible())
			iResult++;
	}
	return iResult;
}

CInputManager::CInputManager()
{
	m_bActivated = false;
}

CInputManager::~CInputManager()
{
	Activate(false);
}

void CInputManager::Activate(bool bOn)
{
	if (bOn && !m_bActivated)
	{
		m_bActivated = true;
		if (s_iActivated == 0)
		{
			CRootPanel::GetRoot()->EnableMouse(true);
			CRootPanel::GetRoot()->EnableKeyboard(true);
		}
		s_iActivated++;
	}

	if (!bOn && m_bActivated)
	{
		m_bActivated = false;
		s_iActivated--;
		if (s_iActivated == 0)
		{
			CRootPanel::GetRoot()->EnableMouse(false);
			CRootPanel::GetRoot()->EnableKeyboard(false);
		}
	}
}
