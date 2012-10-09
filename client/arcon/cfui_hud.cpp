#include "cbase.h"
#include "cfui_hud.h"
#include "cfui_menu.h"

using namespace cfgui;

ICFHUD::ICFHUD()
	: CPanel(0, 0, CFScreenWidth(), CFScreenHeight())
{
	// Always add HUD controls to the tail so they are drawn last (on top)
	CRootPanel::GetRoot()->AddControl(this, true);
	SetBorder(BT_NONE);
}

void ICFHUD::SetParent(IControl* pParent)
{
	if (pParent != CRootPanel::GetRoot())
		Assert(false);
}

bool ICFHUD::IsVisible()
{
	if (CCFMOTD::IsOpen())
		return false;

	if (CCFGameInfo::IsOpen())
		return false;

	if (CCFMenu::IsOpen())
		return false;

	return true;
}