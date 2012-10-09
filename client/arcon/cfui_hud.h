#ifndef CFHUD_H
#define CFHUD_H

#include "cfui_gui.h"

namespace cfgui
{
	class ICFHUD : public CPanel
	{
	public:
						ICFHUD();

		virtual void	SetParent(IControl* pParent);

		virtual void	LevelShutdown() {};

		// For the purposes of these functions, HUD indicators always take up
		// the whole screen.
		virtual void	SetSize(int w, int h) {};
		virtual void	SetPos(int x, int y) {};
		virtual void	GetSize(int &w, int &h) { w=CFScreenWidth(); h=CFScreenHeight(); };
		virtual void	GetPos(int &x, int &y) { x=y=0; };

		virtual void	GetAbsPos(int &x, int &y) {x=y=0;};
		virtual void	GetAbsDimensions(int &x, int &y, int &w, int &h) { x=y=0; w=CFScreenWidth(); h=CFScreenHeight(); };
		virtual void	GetBR(int &x, int &y) { x=CFScreenWidth(); y=CFScreenHeight(); };
		virtual int		GetWidth() { return CFScreenWidth(); };
		virtual int		GetHeight() { return CFScreenHeight(); };

		virtual bool	IsVisible();
		virtual void	SetVisible(bool bVisible) {};

		// HUD elements never interact with the mouse.
		virtual bool	MousePressed(vgui::MouseCode code) {return false;};
		virtual bool	MouseReleased(vgui::MouseCode code) {return false;};
		virtual bool	IsCursorListener() {return false;};
	};

};

#endif