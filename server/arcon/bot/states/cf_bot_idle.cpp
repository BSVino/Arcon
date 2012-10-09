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

// range for snipers to select a hiding spot
const float sniperHideRange = 2000.0f;

//--------------------------------------------------------------------------------------------------------------
/**
 * The Idle state.
 * We never stay in the Idle state - it is a "home base" for the state machine that
 * does various checks to determine what we should do next.
 */
void IdleState::OnEnter( CCFBot *me )
{
	me->DestroyPath();
	me->SetBotEnemy( NULL );

	//
	// Since Idle assigns tasks, we assume that coming back to Idle means our task is complete
	//
	me->SetTask( CCFBot::SEEK_AND_DESTROY );
	me->SetDisposition( CCFBot::ENGAGE_AND_INVESTIGATE );
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Determine what we should do next
 */
void IdleState::OnUpdate( CCFBot *me )
{
	// all other states assume GetLastKnownArea() is valid, ensure that it is
	if (me->GetLastKnownArea() == NULL && me->StayOnNavMesh() == false)
		return;

	// zombies never leave the Idle state
	if (cv_bot_zombie.GetBool())
	{
		me->ResetStuckMonitor();
		return;
	}

	if (me->GetClosestVisibleKO())
	{
		bool bCloseToKO = (me->GetCentroid() - me->GetClosestVisibleKO()->GetCentroid()).IsLengthLessThan(PLAYER_USE_RADIUS - 10);
		if (bCloseToKO)
			me->SetLookAt("Looking at KO", me->GetClosestVisibleKO()->GetCentroid(), PRIORITY_HIGH, 1);

		if (me->IsLookingAtPosition(me->GetClosestVisibleKO()->GetCentroid()) && bCloseToKO)
		{
			me->UseEntity(me->GetGoalEntity());
			me->PrintIfWatched( "Fatality/Revival\n" );
			return;
		}

		// I feel pretty safe, let's do some KO's.
		if (!me->HasPath())
		{
			if (me->ComputePath( me->GetClosestVisibleKO()->GetCentroid(), FASTEST_ROUTE ))
				me->SetGoalEntity(me->GetClosestVisibleKO());
		}

		// move along our path
		if (me->UpdatePathMovement( NO_SPEED_CHANGE ) != CCFBot::PROGRESSING)
		{
			me->DestroyPath();
			me->SetGoalEntity(NULL);
		}

		return;
	}

	// if round is over, hunt
	if (me->GetGameState()->IsRoundOver())
	{
		me->Hunt();
		return;
	}

	const float offenseSniperCampChance = 10.0f;

	// if we were following someone, continue following them
	if (me->IsFollowing())
	{
		me->ContinueFollowing();
		return;
	}

	//
	// Scenario logic
	//
	switch (TheCFBots()->GetScenario())
	{
		case 0:
		default:	// deathmatch
		{
			// sniping check
			if (me->GetFriendsRemaining() && me->IsSniper() && RandomFloat( 0, 100.0f ) < offenseSniperCampChance)
			{
				me->SetTask( CCFBot::MOVE_TO_SNIPER_SPOT );
				me->Hide( me->GetLastKnownArea(), RandomFloat( 10.0f, 30.0f ), sniperHideRange );
				me->SetDisposition( CCFBot::OPPORTUNITY_FIRE );
				me->PrintIfWatched( "Sniping!\n" );
				return;
			}
			break;
		}
	}

	// if we have nothing special to do, go hunting for enemies
	me->Hunt();
}

