//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Encapsulation of the current scenario/game state. Allows each bot imperfect knowledge.
//
// $NoKeywords: $
//=============================================================================//

// Author: Michael S. Booth (mike@turtlerockstudios.com), 2003

#include "cbase.h"
#include "KeyValues.h"

#include "cf_bot.h"
#include "cf_gamestate.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//--------------------------------------------------------------------------------------------------------------
CFGameState::CFGameState( CCFBot *owner )
{
	m_owner = owner;
	m_isRoundOver = false;

	m_bombState = MOVING;
	m_lastSawBomber.Invalidate();
	m_lastSawLooseBomb.Invalidate();
	m_isPlantedBombPosKnown = false;
	m_plantedBombsite = UNKNOWN;

	m_bombsiteCount = 0;
	m_bombsiteSearchIndex = 0;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Reset at round start
 */
void CFGameState::Reset( void )
{
	m_isRoundOver = false;

	// bomb -----------------------------------------------------------------------
	m_bombState = MOVING;
	m_lastSawBomber.Invalidate();
	m_lastSawLooseBomb.Invalidate();
	m_isPlantedBombPosKnown = false;
	m_plantedBombsite = UNKNOWN;

	m_bombsiteCount = TheCFBots()->GetZoneCount();

	int i;
	for( i=0; i<m_bombsiteCount; ++i )
	{
		m_isBombsiteClear[i] = false;
		m_bombsiteSearchOrder[i] = i;
	}

	// shuffle the bombsite search order
	// allows T's to plant at random site, and TEAM_CT's to search in a random order
	// NOTE: VS6 std::random_shuffle() doesn't work well with an array of two elements (most maps)
	for( i=0; i < m_bombsiteCount; ++i )
	{
		int swap = m_bombsiteSearchOrder[i];
		int rnd = RandomInt( i, m_bombsiteCount-1 );
		m_bombsiteSearchOrder[i] = m_bombsiteSearchOrder[ rnd ];
		m_bombsiteSearchOrder[ rnd ] = swap;
	}

	m_bombsiteSearchIndex = 0;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Update game state based on events we have received
 */
void CFGameState::OnRoundEnd( IGameEvent *event )
{
	m_isRoundOver = true;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Update game state based on events we have received
 */
void CFGameState::OnRoundStart( IGameEvent *event )
{
	Reset();
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Update game state based on events we have received
 */
void CFGameState::OnBombPlanted( IGameEvent *event )
{
	// change state - the event is announced to everyone
	SetBombState( PLANTED );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Update game state based on events we have received
 */
void CFGameState::OnBombDefused( IGameEvent *event )
{
	// change state - the event is announced to everyone
	SetBombState( DEFUSED );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Update game state based on events we have received
 */
void CFGameState::OnBombExploded( IGameEvent *event )
{
	// change state - the event is announced to everyone
	SetBombState( EXPLODED );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * True if round has been won or lost (but not yet reset)
 */
bool CFGameState::IsRoundOver( void ) const
{
	return m_isRoundOver;
}

//--------------------------------------------------------------------------------------------------------------
void CFGameState::SetBombState( BombState state )
{
	// if state changed, reset "last seen" timestamps
	if (m_bombState != state)
	{
		m_bombState = state;
	}
}

//--------------------------------------------------------------------------------------------------------------
void CFGameState::UpdateLooseBomb( const Vector &pos )
{
	m_looseBombPos = pos;
	m_lastSawLooseBomb.Reset();

	// we saw the loose bomb, update our state
	SetBombState( LOOSE );
}

//--------------------------------------------------------------------------------------------------------------
float CFGameState::TimeSinceLastSawLooseBomb( void ) const
{
	return m_lastSawLooseBomb.GetElapsedTime();
}

//--------------------------------------------------------------------------------------------------------------
bool CFGameState::IsLooseBombLocationKnown( void ) const
{
	if (m_bombState != LOOSE)
		return false;

	return (m_lastSawLooseBomb.HasStarted()) ? true : false;
}

//--------------------------------------------------------------------------------------------------------------
void CFGameState::UpdateBomber( const Vector &pos )
{
	m_bomberPos = pos;
	m_lastSawBomber.Reset();

	// we saw the bomber, update our state
	SetBombState( MOVING );
}

//--------------------------------------------------------------------------------------------------------------
float CFGameState::TimeSinceLastSawBomber( void ) const
{
	return m_lastSawBomber.GetElapsedTime();
}

//--------------------------------------------------------------------------------------------------------------
bool CFGameState::IsPlantedBombLocationKnown( void ) const
{
	if (m_bombState != PLANTED)
		return false;

	return m_isPlantedBombPosKnown;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Return the zone index of the planted bombsite, or UNKNOWN
 */
int CFGameState::GetPlantedBombsite( void ) const
{
	if (m_bombState != PLANTED)
		return UNKNOWN;

	return m_plantedBombsite;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Return true if we are currently in the bombsite where the bomb is planted
 */
bool CFGameState::IsAtPlantedBombsite( void ) const
{
	if (m_bombState != PLANTED)
		return false;

	Vector myOrigin = GetCentroid( m_owner );
	const CCFBotManager::Zone *zone = TheCFBots()->GetClosestZone( myOrigin );

	if (zone)
	{
		return (m_plantedBombsite == zone->m_index);
	}

	return false;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Return the zone index of the next bombsite to search
 */
int CFGameState::GetNextBombsiteToSearch( void )
{
	if (m_bombsiteCount <= 0)
		return 0;

	int i;

	// return next non-cleared bombsite index
	for( i=m_bombsiteSearchIndex; i<m_bombsiteCount; ++i )
	{
		int z = m_bombsiteSearchOrder[i];
		if (!m_isBombsiteClear[z])
		{
			m_bombsiteSearchIndex = i;
			return z;
		}
	}

	// all the bombsites are clear, someone must have been mistaken - start search over
	for( i=0; i<m_bombsiteCount; ++i )
		m_isBombsiteClear[i] = false;
	m_bombsiteSearchIndex = 0;

	return GetNextBombsiteToSearch();
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Returns position of bomb in its various states (moving, loose, planted),
 * or NULL if we don't know where the bomb is
 */
const Vector *CFGameState::GetBombPosition( void ) const
{
	switch( m_bombState )
	{
		case MOVING:
		{
			if (!m_lastSawBomber.HasStarted())
				return NULL;

			return &m_bomberPos;
		}

		case LOOSE:
		{
			if (IsLooseBombLocationKnown())
				return &m_looseBombPos;

			return NULL;
		}

		case PLANTED:
		{
			if (IsPlantedBombLocationKnown())
				return &m_plantedBombPos;

			return NULL;
		}
	}

	return NULL;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * We see the planted bomb at 'pos'
 */
void CFGameState::UpdatePlantedBomb( const Vector &pos )
{
	const CCFBotManager::Zone *zone = TheCFBots()->GetClosestZone( pos );

	if (zone == NULL)
	{
		CONSOLE_ECHO( "ERROR: Bomb planted outside of a zone!\n" );
		m_plantedBombsite = UNKNOWN;
	}
	else
	{
		m_plantedBombsite = zone->m_index;
	}

	m_plantedBombPos = pos;
	m_isPlantedBombPosKnown = true;
	SetBombState( PLANTED );
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Someone told us where the bomb is planted
 */
void CFGameState::MarkBombsiteAsPlanted( int zoneIndex )
{
	m_plantedBombsite = zoneIndex;
	SetBombState( PLANTED );
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Someone told us a bombsite is clear
 */
void CFGameState::ClearBombsite( int zoneIndex )
{
	if (zoneIndex >= 0 && zoneIndex < m_bombsiteCount)
		m_isBombsiteClear[ zoneIndex ] = true;	
}

//--------------------------------------------------------------------------------------------------------------
bool CFGameState::IsBombsiteClear( int zoneIndex ) const
{
	if (zoneIndex >= 0 && zoneIndex < m_bombsiteCount)
		return m_isBombsiteClear[ zoneIndex ];

	return false;
}
