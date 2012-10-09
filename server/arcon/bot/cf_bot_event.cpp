//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

// Author: Michael S. Booth (mike@turtlerockstudios.com), 2003

#include "cbase.h"
#include "cf_gamerules.h"
#include "KeyValues.h"

#include "cf_bot.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//--------------------------------------------------------------------------------------------------------------
/**
 * Checks if the bot can hear the event
 */
void CCFBot::OnAudibleEvent( IGameEvent *event, CBasePlayer *player, float range, PriorityType priority, bool isHostile, bool isFootstep, const Vector *actualOrigin )
{
	/// @todo Listen to non-player sounds
	if (player == NULL)
		return;

	// don't pay attention to noise that friends make
	if (!IsEnemy( player ))
		return;

	Vector playerOrigin = ToCFPlayer(player)->GetCentroid();
	Vector myOrigin = GetCentroid();

	// If the event occurs far from the triggering player, it may override the origin
	if ( actualOrigin )
	{
		playerOrigin = *actualOrigin;
	}

	// check if noise is close enough for us to hear
	const Vector *newNoisePosition = &playerOrigin;
	float newNoiseDist = (myOrigin - *newNoisePosition).Length();
	if (newNoiseDist < range)
	{
		// we heard the sound
		if ((IsLocalPlayerWatchingMe() && cv_bot_debug.GetInt() == 3) || cv_bot_debug.GetInt() == 4)
		{
			PrintIfWatched( "Heard noise (%s from %s, pri %s, time %3.1f)\n", 
											(FStrEq( "weapon_fire", event->GetName() )) ? "Weapon fire " : "",
											(player) ? player->GetPlayerName() : "NULL",
											(priority == PRIORITY_HIGH) ? "HIGH" : ((priority == PRIORITY_MEDIUM) ? "MEDIUM" : "LOW"),
											gpGlobals->curtime );
		}

		// should we pay attention to it
		// if noise timestamp is zero, there is no prior noise
		if (m_noiseTimestamp > 0.0f)
		{
			// only overwrite recent sound if we are louder (closer), or more important - if old noise was long ago, its faded
			const float shortTermMemoryTime = 3.0f;
			if (gpGlobals->curtime - m_noiseTimestamp < shortTermMemoryTime)
			{
				// prior noise is more important - ignore new one
				if (priority < m_noisePriority)
					return;

				float oldNoiseDist = (myOrigin - m_noisePosition).Length();
				if (newNoiseDist >= oldNoiseDist)
					return;
			}
		}

		// find the area in which the noise occured
		/// @todo Better handle when noise occurs off the nav mesh
		/// @todo Make sure noise area is not through a wall or ceiling from source of noise
		/// @todo Change GetNavTravelTime to better deal with NULL destination areas
		CNavArea *noiseArea = TheNavMesh->GetNearestNavArea( *newNoisePosition );
		if (noiseArea == NULL)
		{
			PrintIfWatched( "  *** Noise occurred off the nav mesh - ignoring!\n" );
			return;
		}

		m_noiseArea = noiseArea;

		// remember noise priority
		m_noisePriority = priority;

		// randomize noise position in the area a bit - hearing isn't very accurate
		// the closer the noise is, the more accurate our placement
		/// @todo Make sure not to pick a position on the opposite side of ourselves.
		const float maxErrorRadius = 400.0f;
		const float maxHearingRange = 2000.0f;
		float errorRadius = maxErrorRadius * newNoiseDist/maxHearingRange;

		m_noisePosition.x = newNoisePosition->x + RandomFloat( -errorRadius, errorRadius );
		m_noisePosition.y = newNoisePosition->y + RandomFloat( -errorRadius, errorRadius );

		// note the *travel distance* to the noise
		m_noiseTravelDistance = GetTravelDistanceToPlayer( (CCFPlayer *)player );

		// make sure noise position remains in the same area
		m_noiseArea->GetClosestPointOnArea( m_noisePosition, &m_noisePosition );

		// note when we heard the noise
		m_noiseTimestamp = gpGlobals->curtime;

		// if we hear a nearby enemy, become alert
		const float nearbyNoiseRange = 1000.0f;
		if (m_noiseTravelDistance < nearbyNoiseRange && m_noiseTravelDistance > 0.0f)
		{
			BecomeAlert();
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
void CCFBot::OnHEGrenadeDetonate( IGameEvent *event )
{
	if ( !IsAlive() )
		return;

	// don't react to our own events
	CBasePlayer *player = UTIL_PlayerByUserId( event->GetInt( "userid" ) );
	if ( player == this )
		return;

	OnAudibleEvent( event, player, 99999.0f, PRIORITY_HIGH, true ); // hegrenade_detonate
}


//--------------------------------------------------------------------------------------------------------------
void CCFBot::OnFlashbangDetonate( IGameEvent *event )
{
	if ( !IsAlive() )
		return;

	// don't react to our own events
	CBasePlayer *player = UTIL_PlayerByUserId( event->GetInt( "userid" ) );
	if ( player == this )
		return;

	OnAudibleEvent( event, player, 1000.0f, PRIORITY_LOW, true ); // flashbang_detonate
}


//--------------------------------------------------------------------------------------------------------------
void CCFBot::OnSmokeGrenadeDetonate( IGameEvent *event )
{
	if ( !IsAlive() )
		return;

	// don't react to our own events
	CBasePlayer *player = UTIL_PlayerByUserId( event->GetInt( "userid" ) );
	if ( player == this )
		return;

	OnAudibleEvent( event, player, 1000.0f, PRIORITY_LOW, true ); // smokegrenade_detonate
}


//--------------------------------------------------------------------------------------------------------------
void CCFBot::OnGrenadeBounce( IGameEvent *event )
{
	if ( !IsAlive() )
		return;

	// don't react to our own events
	CBasePlayer *player = UTIL_PlayerByUserId( event->GetInt( "userid" ) );
	if ( player == this )
		return;

	OnAudibleEvent( event, player, 500.0f, PRIORITY_LOW, true ); // grenade_bounce
}


//--------------------------------------------------------------------------------------------------------------
void CCFBot::OnBulletImpact( IGameEvent *event )
{
	if ( !IsAlive() )
		return;

	// don't react to our own events
	CBasePlayer *player = UTIL_PlayerByUserId( event->GetInt( "userid" ) );
	if ( player == this )
		return;

	// Construct an origin for the sound, since it can be far from the originating player
	Vector actualOrigin;
	actualOrigin.x = event->GetFloat( "x", 0.0f );
	actualOrigin.y = event->GetFloat( "y", 0.0f );
	actualOrigin.z = event->GetFloat( "z", 0.0f );

	/// @todo Ignoring bullet impact events for now - we dont want bots to look directly at them!
	//OnAudibleEvent( event, player, 1100.0f, PRIORITY_MEDIUM, true, false, &actualOrigin ); // bullet_impact
}


//--------------------------------------------------------------------------------------------------------------
void CCFBot::OnBreakProp( IGameEvent *event )
{
	if ( !IsAlive() )
		return;

	// don't react to our own events
	CBasePlayer *player = UTIL_PlayerByUserId( event->GetInt( "userid" ) );
	if ( player == this )
		return;

	OnAudibleEvent( event, player, 1100.0f, PRIORITY_MEDIUM, true ); // break_prop
}


//--------------------------------------------------------------------------------------------------------------
void CCFBot::OnBreakBreakable( IGameEvent *event )
{
	if ( !IsAlive() )
		return;

	// don't react to our own events
	CBasePlayer *player = UTIL_PlayerByUserId( event->GetInt( "userid" ) );
	if ( player == this )
		return;

	OnAudibleEvent( event, player, 1100.0f, PRIORITY_MEDIUM, true ); // break_glass
}


//--------------------------------------------------------------------------------------------------------------
void CCFBot::OnDoorMoving( IGameEvent *event )
{
	if ( !IsAlive() )
		return;

	// don't react to our own events
	CBasePlayer *player = UTIL_PlayerByUserId( event->GetInt( "userid" ) );
	if ( player == this )
		return;

	OnAudibleEvent( event, player, 1100.0f, PRIORITY_MEDIUM, false ); // door_moving
}


//--------------------------------------------------------------------------------------------------------------
void CCFBot::OnRoundEnd( IGameEvent *event )
{
	// Morale adjustments happen even for dead players
	int winner = event->GetInt( "winner" );
	switch ( winner )
	{
	case WINNER_NUMENI:
		if (GetTeamNumber() == TEAM_NUMENI)
		{
			DecreaseMorale();
		}
		else
		{
			IncreaseMorale();
		}
		break;

	case WINNER_MACHINDO:
		if (GetTeamNumber() == TEAM_MACHINDO)
		{
			IncreaseMorale();
		}
		else
		{
			DecreaseMorale();
		}
		break;

	default:
		break;
	}

	m_gameState.OnRoundEnd( event );

	if ( !IsAlive() )
		return;

	if ( event->GetInt( "winner" ) == WINNER_MACHINDO )
	{
		if (GetTeamNumber() == TEAM_MACHINDO)
			GetChatter()->CelebrateWin();
	}
	else if ( event->GetInt( "winner" ) == WINNER_NUMENI )
	{
		if (GetTeamNumber() == TEAM_NUMENI)
			GetChatter()->CelebrateWin();
	}
}


//--------------------------------------------------------------------------------------------------------------
void CCFBot::OnRoundStart( IGameEvent *event )
{
	m_gameState.OnRoundStart( event );
}


//--------------------------------------------------------------------------------------------------------------
void CCFBot::OnNavBlocked( IGameEvent *event )
{
	if ( event->GetBool( "blocked" ) )
	{
		unsigned int areaID = event->GetInt( "area" );
		if ( areaID )
		{
			// An area was blocked off.  Reset our path if it has this area on it.
			for( int i=0; i<m_pathLength; ++i )
			{
				const ConnectInfo *info = &m_path[ i ];
				if ( info->area && info->area->GetID() == areaID )
				{
					DestroyPath();
					return;
				}
			}
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Invoked when bot enters a nav area
 */
void CCFBot::OnEnteredNavArea( CNavArea *newArea )
{
	// assume that we "clear" an area of enemies when we enter it
	newArea->SetClearedTimestamp( GetTeamNumber()-1 );

	// if we just entered a 'stop' area, set the flag
	if ( newArea->GetAttributes() & NAV_MESH_STOP )
	{
		m_isStopping = true;
	}

	/// @todo Flag these areas as spawn areas during load
	if (IsAtEnemySpawn())
	{
		m_hasVisitedEnemySpawn = true;
	}
}
