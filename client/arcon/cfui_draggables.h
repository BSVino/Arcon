#ifndef DRAGGABLES_H
#define DRAGGABLES_H

#include <vgui/Mousecode.h>
#include "input.h"

// Draggable template classes.

using namespace cfgui;

template <class T>
class IHighlightListener
{
public:
	virtual void			Highlighted(T eID)=0;
};

template <class T> class CDraggableIcon;

template <class T, class U = CDraggableIcon<T> >
class CDroppableChoice : public CDroppablePanel
{
public:
							CDroppableChoice<T, U>(T eDraggable, IDraggable::DragClass_t eDragClass);
							CDroppableChoice<T, U>(char iWeapon, char iRune);
	virtual void			Destructor();
	virtual void			Delete() { delete this; };

	virtual void			Paint(int x, int y, int w, int h);

	virtual bool			IsCursorListener() {return true;};
	virtual void			CursorIn();
	virtual void			CursorOut();

	virtual IDraggable*		GetCurrentDraggable();
	virtual const CRect		GetHoldingRect();

	virtual bool			CanDropHere(IDraggable* pDraggable) { return false; };

	virtual bool			IsInfinite() { return true; };

	virtual void			SetHighlightListener(IHighlightListener<T>* pListener) { m_pHighlightListener = pListener; };

	T						m_eDraggableID;

	int						m_iDraggableWidth;
	IDraggable::DragClass_t	m_eDragClass;

	IHighlightListener<T>*	m_pHighlightListener;
};

template <class T, class U>
CDroppableChoice<T, U>::CDroppableChoice(T eDraggable, IDraggable::DragClass_t eDragClass)
	: CDroppablePanel(0, 0, 100, 100)
{
	SetBorder(BT_NONE);

	m_eDraggableID = eDraggable;
	m_eDragClass = eDragClass;

	m_iDraggableWidth = 64;

	m_pHighlightListener = NULL;

	Layout();

	// Automatically create a draggable in this slot.
	SetDraggable(new U(this, eDraggable, eDragClass));
}

template <class T, class U>
CDroppableChoice<T, U>::CDroppableChoice(char iWeapon, char iRune)
	: CDroppablePanel(0, 0, 100, 100)
{
	SetBorder(BT_NONE);

	m_eDragClass = IDraggable::DC_RUNECOMBO;

	m_iDraggableWidth = 64;

	m_pHighlightListener = NULL;

	Layout();

	// Automatically create a draggable in this slot.
	SetDraggable(new U(this, iWeapon, iRune));
}

template <class T, class U>
void CDroppableChoice<T, U>::Destructor()
{
	CDroppablePanel::Destructor();
}

template <class T, class U>
void CDroppableChoice<T, U>::Paint(int x, int y, int w, int h)
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
				pHighlight->DrawSelf(Icon.x, Icon.y, Icon.w, Icon.h, Color(255, 255, 255, 255));
		}
	}

	CDroppablePanel::Paint(x, y, w, h);
}

template <class T, class U>
IDraggable* CDroppableChoice<T, U>::GetCurrentDraggable()
{
	return m_apDraggables[0];
}

template <class T, class U>
const CRect CDroppableChoice<T, U>::GetHoldingRect()
{
	int x = 0, y = 0;
	GetAbsPos(x, y);

	return CRect(x, y, m_iW, m_iH);
}

template <class T, class U>
void CDroppableChoice<T, U>::CursorIn()
{
	CDroppablePanel::CursorIn();

	if (m_pHighlightListener)
		m_pHighlightListener->Highlighted(m_eDraggableID);

	SetHighlighted(true);
}

template <class T, class U>
void CDroppableChoice<T, U>::CursorOut()
{
	CDroppablePanel::CursorOut();

	SetHighlighted(false);
}

template <class T>
class CDraggableIcon : public CBaseControl, public IDraggable
{
public:
						CDraggableIcon<T>(IDroppable* pParent, T eDraggable, DragClass_t eDragClass);
	virtual void		Destructor() {};
	virtual void		Delete() { delete this; };

	virtual bool		IsVisible() { return m_bVisible; };

	virtual IControl*	GetParent() { Assert(false); return NULL; };
	virtual void		SetParent(IControl* pParent) { Assert(false); };

	virtual IDroppable*	GetDroppable() { return m_pDroppable; };
	virtual void		SetDroppable(IDroppable* pDroppable) { m_pDroppable = pDroppable; };

	virtual void		GetAbsPos(int &x, int &y);
	virtual void		SetHoldingRect(const CRect HoldingRect);	// Screen space
	virtual CRect		GetHoldingRect();	// Screen space

	virtual void		SetWeapon(T eDraggable) { m_eDraggableID = eDraggable; };
	virtual T			GetWeapon() { return m_eDraggableID; };

	virtual void		Paint();
	virtual void		Paint(int x, int y);
	virtual void		Paint(int x, int y, int w, int h);
	virtual void		Paint(int x, int y, int w, int h, bool bFloating);

	virtual int			GetPaintAlpha();

	IDraggable::DragClass_t	GetClass() { return m_eDragClass; };

	void				Layout() {};
	void				Think() {};
	virtual bool		MousePressed(vgui::MouseCode code) {return false;};
	virtual bool		MouseReleased(vgui::MouseCode code) {return false;};
	virtual bool		IsCursorListener() {return false;};

	virtual IDraggable&	MakeCopy();

protected:
	T					m_eDraggableID;
	IDroppable*			m_pDroppable;
	DragClass_t			m_eDragClass;
};

template <class T>
CDraggableIcon<T>::CDraggableIcon(IDroppable* pDroppable, T eDraggable, DragClass_t eDragClass)
   // These values will be corrected later.
 : CBaseControl(0, 0, 100, 100)
{
	// We need to be sitting in a parent when we spawn.
	Assert(pDroppable);

	m_pDroppable = pDroppable;
	m_eDraggableID = eDraggable;
	m_eDragClass = eDragClass;
}

template <class T>
void CDraggableIcon<T>::GetAbsPos(int &x, int &y)
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

template <class T>
void CDraggableIcon<T>::SetHoldingRect(const CRect HoldingRect)
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

template <class T>
CRect CDraggableIcon<T>::GetHoldingRect()
{
	CRect r;
	GetAbsDimensions(r.x, r.y, r.w, r.h);
	return r;
}

template <class T>
void CDraggableIcon<T>::Paint()
{
	int x = 0, y = 0;
	GetAbsPos(x, y);

	Paint(x, y);
}

template <class T>
void CDraggableIcon<T>::Paint(int x, int y)
{
	Paint(x, y, m_iW, m_iH);
}

template <class T>
void CDraggableIcon<T>::Paint(int x, int y, int w, int h)
{
	if (!IsVisible())
		return;

	Paint(x, y, w, h, false);
}

template <class T>
void CDraggableIcon<T>::Paint(int x, int y, int w, int h, bool bFloating)
{
	if (!IsVisible())
		return;

	CCFHudTexture* pTexture;
	if (bFloating)
		pTexture = CTextureHandler::GetTexture(m_eDraggableID, CTextureHandler::TT_SQUARE);
	else
		pTexture = CTextureHandler::GetTexture(m_eDraggableID, CTextureHandler::TT_WIDE);

	if (pTexture)
		pTexture->DrawSelf(x, y, w, h, Color(255, 255, 255, GetPaintAlpha()));
}

template <class T>
int CDraggableIcon<T>::GetPaintAlpha()
{
	if (GetDroppable() && !GetDroppable()->IsGrabbale())
		return 100;

	return 255;
}

template <class T>
IDraggable& CDraggableIcon<T>::MakeCopy()
{
	CDraggableIcon<T>* pIcon = new CDraggableIcon<T>(*this);
	return *pIcon;
}

template <class T>
class CDroppableIcon : public CDroppablePanel
{
public:
							CDroppableIcon<T>(IDraggable::DragClass_t eDragClass, const CRect &HoldingRect);
	virtual void			Destructor();
	virtual void			Delete() { delete this; };

	virtual void			Paint(int x, int y, int w, int h);

	virtual bool			IsCursorListener() {return true;};

	virtual const CRect		GetHoldingRect();	// Screen space
	virtual void			SetHoldingRect(const CRect &HoldingRect);	// Screen space

	virtual void			SetDraggable(IDraggable*, bool bDelete = true);
	virtual void			SetDraggableIcon(IDraggable*, bool bDelete = true);
	virtual void			DraggableChanged(T*) {};	// Subclasses can override as a triggered event.
	virtual IDraggable*		GetDraggable(int i);
	virtual IDraggable*		GetCurrentDraggable();
	virtual T*				GetDraggableIcon();

	virtual bool			CanDropHere(IDraggable*);

	virtual bool			IsInfinite() { return false; };

	virtual bool			IsVisible();
	virtual void			SetVisible(bool bVisible) { m_bVisible = bVisible; };

	virtual void			SetDroppable(bool bDroppable) { m_bDroppable = bDroppable; };

protected:
	IDraggable::DragClass_t	m_eDragClass;

	bool					m_bDroppable;

	CRect					m_HoldingRect;
};

template <class T>
CDroppableIcon<T>::CDroppableIcon(IDraggable::DragClass_t eDragClass, const CRect &HoldingRect)
 : CDroppablePanel(0, 0, 100, 100),
 m_HoldingRect(0, 0, 0, 0)
{
	m_eDragClass = eDragClass;
	m_HoldingRect = HoldingRect;
	m_pParent = NULL;

	SetBorder(CPanel::BT_NONE);

	SetGrabbale(false);

	m_bDroppable = true;
}

template <class T>
void CDroppableIcon<T>::Destructor()
{
	CDroppablePanel::Destructor();
}

template <class T>
void CDroppableIcon<T>::Paint(int x, int y, int w, int h)
{
	if (!IsVisible())
		return;

	IDraggable* pDragging = CRootPanel::GetRoot()->GetCurrentDraggable();

	if (GetDraggable(0) && !pDragging && m_bHighlight)
	{
		int mx, my;
		CRootPanel::GetFullscreenMousePos(mx, my);

		CRect Icon = GetHoldingRect();
		if (mx >= Icon.x &&
			my >= Icon.y &&
			mx < Icon.x + Icon.w &&
			my < Icon.y + Icon.h)
		{
			// WEAPON_MAGIC is the highlight.
			CTextureHandler::GetTexture(WEAPON_MAGIC)->DrawSelf(Icon.x, Icon.y, Icon.w, Icon.h, Color(255, 255, 255, 255));
		}
	}

	CDroppablePanel::Paint(x, y, w, h);
}

template <class T>
const CRect CDroppableIcon<T>::GetHoldingRect()
{
	return m_HoldingRect;
}

template <class T>
void CDroppableIcon<T>::SetHoldingRect(const CRect &HoldingRect)
{
	m_HoldingRect = HoldingRect;

	if (m_apDraggables.IsValidIndex(0))
	{
		m_apDraggables[0]->SetHoldingRect(GetHoldingRect());
	}
}

// Set the draggable's icon directly.
template <class T>
void CDroppableIcon<T>::SetDraggableIcon(IDraggable* pDragged, bool bDelete)
{
	CDroppablePanel::SetDraggable(pDragged, bDelete);
}

template <class T>
void CDroppableIcon<T>::SetDraggable(IDraggable* pDragged, bool bDelete)
{
	if (!CanDropHere(pDragged))
		return;

	SetDraggableIcon(NULL, bDelete);
	DraggableChanged(NULL);

	if (pDragged)
	{
		if (CRootPanel::GetRoot()->GetCurrentDroppable()->IsInfinite())
		{
			// if the draggable belongs to another panel, copy it.
			SetDraggableIcon(&pDragged->MakeCopy(), bDelete);
		}
		else
		{
			// otherwise take this one.
			((T*)pDragged)->GetDroppable()->SetDraggable(NULL, false);
			SetDraggableIcon((T*)pDragged, bDelete);
		}
		DraggableChanged(GetDraggableIcon());
	}
}

template <class T>
IDraggable* CDroppableIcon<T>::GetDraggable(int i)
{
	if (!m_apDraggables.IsValidIndex(i))
		return NULL;

	return m_apDraggables[i];
}

template <class T>
IDraggable* CDroppableIcon<T>::GetCurrentDraggable()
{
	return GetDraggable(0);
}

template <class T>
T* CDroppableIcon<T>::GetDraggableIcon()
{
	Assert(!m_apDraggables.IsValidIndex(0) || m_apDraggables[0]->GetClass() == m_eDragClass);
	return (T*)GetDraggable(0);
}

template <class T>
bool CDroppableIcon<T>::IsVisible()
{
	if (GetParent() && !GetParent()->IsVisible())
		return false;
	
	return m_bVisible;
}

template <class T>
bool CDroppableIcon<T>::CanDropHere(IDraggable* pDraggable)
{
	if (!m_bDroppable)
		return false;

	if (!pDraggable)
		return true;

	IDroppable* pDroppable = CRootPanel::GetRoot()->GetCurrentDroppable();

	if (pDroppable == this)
		return false;

	if (pDraggable->GetClass() != m_eDragClass)
		return false;

	return true;
}

#endif
