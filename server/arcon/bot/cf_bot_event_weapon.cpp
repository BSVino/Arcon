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
void CCFBot::OnWeaponFire( IGameEvent *event )
{
	if ( !IsAlive() )
		return;

	// don't react to our own events
	CBasePlayer *player = UTIL_PlayerByUserId( event->GetInt( "userid" ) );
	if ( player == this )
		return;

	const float ShortRange = 1000.0f;
	const float NormalRange = 2000.0f;

	float range;

	/// @todo Secondary weapon?
	CWeaponCFBase *weapon = (CWeaponCFBase *)((player)?ToCFPlayer(player)->GetPrimaryWeapon():NULL);

	if (weapon == NULL)
		return;

	switch( weapon->GetWeaponID() )
	{
		// quiet
		case WEAPON_MAGIC:
		case WEAPON_RIVENBLADE:
		case WEAPON_VALANGARD:
		case WEAPON_PARIAH:
		case WEAPON_FLAG:
			range = ShortRange;
			break;

		// normal
		default:
			range = NormalRange;
			break;
	}

	OnAudibleEvent( event, player, range, PRIORITY_HIGH, true ); // weapon_fire
}


//--------------------------------------------------------------------------------------------------------------
void CCFBot::OnWeaponFireOnEmpty( IGameEvent *event )
{
	if ( !IsAlive() )
		return;

	// don't react to our own events
	CBasePlayer *player = UTIL_PlayerByUserId( event->GetInt( "userid" ) );
	if ( player == this )
		return;

	// for melee fighting - if our victim is attacking or reloading, rush him
	/// @todo Propagate events into active state
	if (GetEnemy() == player && IsUsingPrimaryMelee())
	{
		ForceRun( 5.0f );
	}

	OnAudibleEvent( event, player, 1100.0f, PRIORITY_LOW, false ); // weapon_fire_on_empty
}


//--------------------------------------------------------------------------------------------------------------
void CCFBot::OnWeaponReload( IGameEvent *event )
{
	if ( !IsAlive() )
		return;

	// don't react to our own events
	CBasePlayer *player = UTIL_PlayerByUserId( event->GetInt( "userid" ) );
	if ( player == this )
		return;

	// for melee fighting - if our victim is attacking or reloading, rush him
	/// @todo Propagate events into active state
	if (GetEnemy() == player && IsUsingPrimaryMelee())
	{
		ForceRun( 5.0f );
	}

	OnAudibleEvent( event, player, 1100.0f, PRIORITY_LOW, false ); // weapon_reload
}


//--------------------------------------------------------------------------------------------------------------
void CCFBot::OnWeaponZoom( IGameEvent *event )
{
	if ( !IsAlive() )
		return;

	// don't react to our own events
	CBasePlayer *player = UTIL_PlayerByUserId( event->GetInt( "userid" ) );
	if ( player == this )
		return;

	OnAudibleEvent( event, player, 1100.0f, PRIORITY_LOW, false ); // weapon_zoom
}



