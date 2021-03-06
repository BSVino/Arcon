//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

// Author: Michael S. Booth (mike@turtlerockstudios.com), 2003

#include "cbase.h"
#include "cf_bot.h"
#include "cf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//--------------------------------------------------------------------------------------------------------------
/**
 * Move to a potentially far away position.
 */
void MoveToState::OnEnter( CCFBot *me )
{
	// if we need to find the bomb, get there as quick as we can
	RouteType route;
	switch (me->GetTask())
	{
		case CCFBot::FIND_TICKING_BOMB:
		case CCFBot::DEFUSE_BOMB:
		case CCFBot::MOVE_TO_LAST_KNOWN_ENEMY_POSITION:
			route = FASTEST_ROUTE;
			break;

		default:
			route = SAFEST_ROUTE;
			break;
	}
		
	// build path to, or nearly to, goal position
	me->ComputePath( m_goalPosition, route );

	m_askedForCover = false;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Move to a potentially far away position.
 */
void MoveToState::OnUpdate( CCFBot *me )
{
	Vector myOrigin = GetCentroid( me );

	// assume that we are paying attention and close enough to know our enemy died
	if (me->GetTask() == CCFBot::MOVE_TO_LAST_KNOWN_ENEMY_POSITION)
	{
		/// @todo Account for reaction time so we take some time to realized the enemy is dead
		CBasePlayer *victim = static_cast<CBasePlayer *>( me->GetTaskEntity() );
		if (victim == NULL || !victim->IsAlive())
		{
			me->PrintIfWatched( "The enemy I was chasing was killed - giving up.\n" );
			me->Idle();
			return;
		}
	}

	// look around
	me->UpdateLookAround();

	//
	// Scenario logic
	//
	switch (TheCFBots()->GetScenario())
	{
		case CCFBotManager::SCENARIO_DEFUSE_BOMB:
		{
			// if the bomb has been planted, find it
			// NOTE: This task is used by both CT and T's to find the bomb
			if (me->GetTask() == CCFBot::FIND_TICKING_BOMB)
			{
				if (!me->GetGameState()->IsBombPlanted())
				{
					// the bomb is not planted - give up this task
					me->Idle();
					return;
				}

				if (me->GetGameState()->GetPlantedBombsite() != CFGameState::UNKNOWN)
				{
					// we know where the bomb is planted, stop searching
					me->Idle();
					return;
				}

				// check off bombsites that we explore or happen to stumble into
				for( int z=0; z<TheCFBots()->GetZoneCount(); ++z )
				{
					// don't re-check zones
					if (me->GetGameState()->IsBombsiteClear( z ))
						continue;

					if (TheCFBots()->GetZone(z)->m_extent.Contains( myOrigin ))
					{
						// note this bombsite is clear
						me->GetGameState()->ClearBombsite( z );

						// find another zone to check
						me->Idle();

						return;
					}
				}

				// move to a bombsite
				break;
			}
		}
	}


	if (me->UpdatePathMovement() != CCFBot::PROGRESSING)
	{
		// reached destination
		switch( me->GetTask() )
		{
			case CCFBot::MOVE_TO_LAST_KNOWN_ENEMY_POSITION:
			{
				CBasePlayer *victim = static_cast<CBasePlayer *>( me->GetTaskEntity() );
				if (victim && victim->IsAlive())
				{
					// if we got here and haven't re-acquired the enemy, we lost him
					BotStatement *say = new BotStatement( me->GetChatter(), REPORT_ENEMY_LOST, 8.0f );

					say->AppendPhrase( TheBotPhrases->GetPhrase( "LostEnemy" ) );
					say->SetStartTime( gpGlobals->curtime + RandomFloat( 3.0f, 5.0f ) );

					me->GetChatter()->AddStatement( say );
				}
				break;
			}
		}

		// default behavior when destination is reached
		me->Idle();
		return;
	}
}

//--------------------------------------------------------------------------------------------------------------
void MoveToState::OnExit( CCFBot *me )
{
	// reset to run in case we were walking near our goal position
	me->Run();
	me->SetDisposition( CCFBot::ENGAGE_AND_INVESTIGATE );
	//me->StopAiming();
}
