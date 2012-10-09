//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "kbutton.h"
#include "input.h"
#include "iviewrender.h"
#include "iclientmode.h"
#include "vgui/isurface.h"
#include "vgui_controls/controls.h"
#include "c_cf_player.h"
#include "cf_in_main.h"
#include "cfui_scoreboard.h"
#include "hud_indicators.h"

CCFInput CCFInput::s_pInput;

// Expose this interface
IInput *input = ( IInput * )CFInput();

void CCFInput::MouseMove( CUserCmd *cmd )
{
	float	mouse_x, mouse_y;
	float	mx, my;
	QAngle	viewangles;
	bool	bSetAngles = true;

	CCFPlayer* pCFPlayer = CCFPlayer::GetLocalCFPlayer();
	if (pCFPlayer && pCFPlayer->ShouldLockFollowModeView())
		bSetAngles = false;

	if (pCFPlayer->m_flFreezeRotation != 0 && gpGlobals->curtime < pCFPlayer->m_flFreezeRotation)
		bSetAngles = false;

	// Get view angles from engine
	engine->GetViewAngles( viewangles );

	// Validate mouse speed/acceleration settings
	CheckMouseAcclerationVars();

	// Don't drift pitch at all while mouselooking.
	view->StopPitchDrift ();

	//jjb - this disables normal mouse control if the user is trying to 
	//      move the camera, or if the mouse cursor is visible 
	if ( !m_fCameraInterceptingMouse && 
		 !vgui::surface()->IsCursorVisible() )
	{
		// Sample mouse one more time
		AccumulateMouse();

		// Latch accumulated mouse movements and reset accumulators
		GetAccumulatedMouseDeltasAndResetAccumulators( &mx, &my );

		// Filter, etc. the delta values and place into mouse_x and mouse_y
		GetMouseDelta( mx, my, &mouse_x, &mouse_y );

		// Apply scaling factor
		ScaleMouse( &mouse_x, &mouse_y );

		// Let the client mode at the mouse input before it's used
		g_pClientMode->OverrideMouseInput( &mouse_x, &mouse_y );

		// Add mouse X/Y movement to cmd
		ApplyMouse( viewangles, cmd, mouse_x, mouse_y );

		// Re-center the mouse.
		ResetMouse();
	}

	if (bSetAngles)
	{
		// Store out the new viewangles.
		engine->SetViewAngles( viewangles );
	}
}

int CCFInput::KeyEvent( int down, ButtonCode_t code, const char *pszCurrentBinding )
{
	// If the mouse is clicked pop up the scoreboard.
	if (code == MOUSE_LEFT && down)
	{
		if (CScoreboard::ClaimMouse())
			return 0;
	}

	if ((code == MOUSE_LEFT || code == MOUSE_RIGHT) && down)
	{
		if (CHudMagic::Get() && CHudMagic::Get()->IsMenuOpen())
		{
			CHudMagic::Get()->KeyPressed(code);
			return 0;
		}
	}

	return CInput::KeyEvent(down, code, pszCurrentBinding);
}
