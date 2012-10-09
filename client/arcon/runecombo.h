#ifndef RUNECOMBO_H
#define RUNECOMBO_H

#include "cfui_gui.h"
#include "cfgui_shared.h"
#include "hud_indicators.h"

class CRuneCombo
{
public:
						CRuneCombo(long iNumen, int iWeapon, int iDiff1, int iDiff2, bool bIcons = false);	// iDiff1/2 is just to differentiate it from the other constructors, it's otherwise unnecessary.
						CRuneCombo(int iWeapon, int iRune, bool bIcons = false);
						CRuneCombo(CArmament* pArm, int iWeapon, int iRune, bool bIcons = false);

	virtual void		Paint();
	virtual void		Paint(int x, int y, int w, int h);
	virtual void		Paint(int x, int y, int w, int h, int a);

	virtual void		PaintRunes(int x, int y, int w, int h, int a);
	virtual void		PaintIcons(int x, int y, int w, int h, int a);
	virtual void		PaintStatusIcons(statuseffect_t iStatus, bool bInfused, bool bForce, int x, int y, int w, int h, int a);

	virtual void		PaintRune(RuneID eRune, int iX, int iY, int iW, int iH, int iA)=0;
	virtual void		PaintForceIcon(forceeffect_t eForce, int iX, int iY, int iW, int iH, Color c)=0;
	virtual void		PaintStatusIcon(statuseffect_t eStatus, int iX, int iY, int iW, int iH, int iA)=0;
	virtual void		PaintInfuseIcon(int iArmament, int iX, int iY, int iW, int iH, int iA)=0;

	virtual void		GetAbsPos(int &x, int &y)=0;

	virtual void		SetNumen(long iNumen) { m_iNumen = iNumen; };

	virtual int			GetWidth()=0;
	virtual int			GetHeight()=0;

	virtual bool		IsEmpty();

protected:
	long				m_iNumen;
	int					m_iArmament;
	bool				m_bDrawIcons;
};

class CRuneComboIcon : public CRuneCombo
{
public:
					CRuneComboIcon()
						: CRuneCombo(0, 0, 0, 0) {}

	static void		Paint(long iNumen, int iArmament, int x, int y, int w, int h, int a=255);
	static void		PaintInfuseIcons(statuseffect_t iStatus, int iArmament, int x, int y, int w, int h, int a=255);
	virtual void	Paint(int x, int y, int w, int h);
	virtual void	Paint(int x, int y, int w, int h, int a);

	virtual void	PaintRune(RuneID eRune, int iX, int iY, int iW, int iH, int iA);
	virtual void	PaintForceIcon(forceeffect_t eForce, int iX, int iY, int iW, int iH, Color c);
	virtual void	PaintStatusIcon(statuseffect_t eStatus, int iX, int iY, int iW, int iH, int iA);
	virtual void	PaintInfuseIcon(int iArmament, int iX, int iY, int iW, int iH, int iA);

	void			GetAbsPos(int &x, int &y) { x=y=0; };

	virtual int			GetWidth() { return CFScreenWidth(); };
	virtual int			GetHeight() { return CFScreenHeight(); };
};

class CGUIRuneCombo : public CRuneCombo, public cfgui::CBaseControl
{
public:
						CGUIRuneCombo(CArmament* pArm, int iWeapon, int iRune, bool bIcons = false)
							: CRuneCombo(pArm, iWeapon, iRune, bIcons),
							cfgui::CBaseControl(0, 0, 100, 100) {};

						CGUIRuneCombo(int iWeapon, int iRune, bool bIcons = false)
							: CRuneCombo(iWeapon, iRune, bIcons),
							cfgui::CBaseControl(0, 0, 100, 100) {};

	virtual void		Delete() { delete this; };

	virtual void		Paint() { int x = 0, y = 0; GetAbsPos(x, y); Paint(x, y); };
	virtual void		Paint(int x, int y) { Paint(x, y, m_iW, m_iH); };
	virtual void		Paint(int x, int y, int w, int h) { CRuneCombo::Paint(x, y, w, h); };
	virtual void		Paint(int x, int y, int w, int h, int a) { CRuneCombo::Paint(x, y, w, h, a); };
	virtual void		Layout() {};
	virtual void		Think() {};

	virtual void		PaintRune(RuneID eRune, int iX, int iY, int iW, int iH, int iA);
	virtual void		PaintForceIcon(forceeffect_t eForce, int iX, int iY, int iW, int iH, Color c);
	virtual void		PaintStatusIcon(statuseffect_t eStatus, int iX, int iY, int iW, int iH, int iA);
	virtual void		PaintInfuseIcon(int iArmament, int iX, int iY, int iW, int iH, int iA);

	virtual void		GetAbsPos(int &x, int &y) { CBaseControl::GetAbsPos(x, y); };

	virtual int			GetWidth() { return CBaseControl::GetWidth(); };
	virtual int			GetHeight() { return CBaseControl::GetHeight(); };
};

class CRuneComboDraggable : public IDraggable, public CGUIRuneCombo
{
public:
							CRuneComboDraggable(IDroppable* pParent, int iWeapon, int iRune);

	virtual void			Destructor() {};
	virtual void			Delete() { delete this; };

	virtual void			GetAbsPos(int &x, int &y);
	virtual void			SetHoldingRect(const CRect);
	virtual CRect			GetHoldingRect();

	virtual IControl*		GetParent() { Assert(false); return NULL; };
	virtual void			SetParent(IControl* pParent) { Assert(false); };

	virtual IDroppable*		GetDroppable() { return m_pDroppable; };
	virtual void			SetDroppable(IDroppable* pDroppable) { m_pDroppable = pDroppable; };

	virtual void			Paint();
	virtual void			Paint(int x, int y);
	virtual void			Paint(int x, int y, int w, int h);
	virtual void			Paint(int x, int y, int w, int h, int a);
	virtual void			Paint(int x, int y, int w, int h, bool bFloating);

	virtual int				GetWeapon() { return m_iWeapon; };
	virtual int				GetRune() { return m_iRune; };

	virtual DragClass_t		GetClass() { return DC_RUNECOMBO; };
	virtual IDraggable&		MakeCopy();

protected:
	IDroppable*				m_pDroppable;

	int						m_iWeapon;
	int						m_iRune;
};

#endif
