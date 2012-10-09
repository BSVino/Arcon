//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
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
 * Escape from the bomb.
 */
void EscapeFromBombState::OnEnter( CCFBot *me )
{
	me->StandUp();
	me->Run();
	me->DestroyPath();
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Escape from the bomb.
 */
void EscapeFromBombState::OnUpdate( CCFBot *me )
{
	const Vector *bombPos = me->GetGameState()->GetBombPosition();

	// if we don't know where the bomb is, we shouldn't be in this state
	if (bombPos == NULL)
	{
		me->Idle();
		return;
	}

	// look around
	me->UpdateLookAround();

	if (me->UpdatePathMovement() != CCFBot::PROGRESSING)
	{
		// we have no path, or reached the end of one - create a new path far away from the bomb
		FarAwayFromPositionFunctor func( *bombPos );
		CNavArea *goalArea = FindMinimumCostArea( me->GetLastKnownArea(), func );

		// if this fails, we'll try again next time
		me->ComputePath( goalArea->GetCenter(), FASTEST_ROUTE );
	}
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Escape from the bomb.
 */
void EscapeFromBombState::OnExit( CCFBot *me )
{
	me->EquipBestWeapon();
}
