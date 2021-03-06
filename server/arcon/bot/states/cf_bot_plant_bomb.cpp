//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

// Author: Michael S. Booth (mike@turtlerockstudios.com), 2003

#include "cbase.h"
#include "cf_bot.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//--------------------------------------------------------------------------------------------------------------
/**
 * Plant the bomb.
 */
void PlantBombState::OnEnter( CCFBot *me )
{
	me->Crouch();
	me->SetDisposition( CCFBot::SELF_DEFENSE );

	// look at the floor
//	Vector down( myOrigin.x, myOrigin.y, -1000.0f );

	float yaw = me->EyeAngles().y;
	Vector2D dir( BotCOS(yaw), BotSIN(yaw) );
	Vector myOrigin = GetCentroid( me );

	Vector down( myOrigin.x + 10.0f * dir.x, myOrigin.y + 10.0f * dir.y, me->GetFeetZ() );
	me->SetLookAt( "Plant bomb on floor", down, PRIORITY_HIGH );
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Plant the bomb.
 */
void PlantBombState::OnUpdate( CCFBot *me )
{
	CBaseCombatWeapon *gun = me->GetActiveWeapon();
	bool holdingC4 = false;
	if (gun)
	{
		if (FStrEq( gun->GetClassname(), "weapon_c4" ))
			holdingC4 = true;
	}

	// if we aren't holding the C4, grab it, otherwise plant it
	if (holdingC4)
		me->PrimaryAttack();
	else
		me->SelectItem( "weapon_c4" );

	// if we time out, it's because we slipped into a non-plantable area
	const float timeout = 5.0f;
	if (gpGlobals->curtime - me->GetStateTimestamp() > timeout)
		me->Idle();
}

//--------------------------------------------------------------------------------------------------------------
void PlantBombState::OnExit( CCFBot *me )
{
	// equip our rifle (in case we were interrupted while holding C4)
	me->EquipBestWeapon();
	me->StandUp();
	me->ResetStuckMonitor();
	me->SetDisposition( CCFBot::ENGAGE_AND_INVESTIGATE );
	me->ClearLookAt();
}
