//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

// Author: Michael S. Booth (mike@turtlerockstudios.com), 2003

#include "cbase.h"
#include "cf_gamerules.h"
#include "cf_bot.h"
#include "armament.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//--------------------------------------------------------------------------------------------------------------
ConVar bot_loadout( "bot_loadout", "", FCVAR_CHEAT, "bots are given these items at round start" );
ConVar bot_randombuy( "bot_randombuy", "0", FCVAR_CHEAT, "should bots ignore their prefered weapons and just buy weapons at random?" );

//--------------------------------------------------------------------------------------------------------------
/**
 *  Debug command to give a named weapon
 */
void CCFBot::GiveWeapon( const char *weaponAlias )
{
	const char *translatedAlias = GetTranslatedWeaponAlias( weaponAlias );

	char wpnName[128];
	Q_snprintf( wpnName, sizeof( wpnName ), "weapon_%s", translatedAlias );
	WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( wpnName );
	if ( hWpnInfo == GetInvalidWeaponInfoHandle() )
	{
		return;
	}

	CCFWeaponInfo *pWeaponInfo = dynamic_cast< CCFWeaponInfo* >( GetFileWeaponInfoFromHandle( hWpnInfo ) );
	if ( !pWeaponInfo )
	{
		return;
	}

	GiveNamedItem( wpnName );
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Buy weapons, armor, etc.
 */
void BuyState::OnEnter( CCFBot *me )
{
	m_retries = 0;
	m_prefRetries = 0;
	m_prefIndex = 0;

	m_doneBuying = false;

	m_isInitialDelay = true;

	// this will force us to stop holding live grenade
	me->EquipBestWeapon( MUST_EQUIP );

	m_buyPistol = false;

	me->m_pArmament->Reset();
}


enum WeaponType
{
	PISTOL,
	SHOTGUN,
	SUB_MACHINE_GUN,
	RIFLE,
	MACHINE_GUN,
	SNIPER_RIFLE,
	GRENADE,
	LONGSWORD,
	BUSTER,

	NUM_WEAPON_TYPES
};

struct BuyInfo
{
	WeaponType type;
	char *buyAlias;
	CFWeaponID id;
};

#define PRIMARY_WEAPON_BUY_COUNT 5

static BuyInfo primaryWeaponBuyInfo[ PRIMARY_WEAPON_BUY_COUNT ] =
{
	{ SHOTGUN,			"shotgun",		WEAPON_SHOTGUN },
	{ RIFLE,			"rifle",		WEAPON_RIFLE },
	{ PISTOL,			"pistol",		WEAPON_PISTOL },
	{ LONGSWORD,		"rivenblade",	WEAPON_RIVENBLADE },
	{ BUSTER,			"valangard",	WEAPON_VALANGARD },
};

/**
 * Given a weapon alias, return the kind of weapon it is
 */
inline WeaponType GetWeaponType( const char *alias )
{
	int i;
	
	for( i=0; i<PRIMARY_WEAPON_BUY_COUNT; ++i )
	{
		if (!stricmp( alias, primaryWeaponBuyInfo[i].buyAlias ))
			return primaryWeaponBuyInfo[i].type;
	}

	return NUM_WEAPON_TYPES;
}




//--------------------------------------------------------------------------------------------------------------
void BuyState::OnUpdate( CCFBot *me )
{
	char cmdBuffer[256];

	// wait for a Navigation Mesh
	if (!TheNavMesh->IsLoaded())
		return;

	// apparently we cant buy things in the first few seconds, so wait a bit
	if (m_isInitialDelay)
	{
		const float waitToBuyTime = 0.25f;
		if (gpGlobals->curtime - me->GetStateTimestamp() < waitToBuyTime)
			return;

		m_isInitialDelay = false;
	}

	// if we're done buying and still in the freeze period, wait
	if (m_doneBuying)
		return;

	// If we're supposed to buy a specific weapon for debugging, do so and then bail
	const char *cheatWeaponString = bot_loadout.GetString();
	if ( cheatWeaponString && *cheatWeaponString )
	{
		CUtlVector<char*, CUtlMemory<char*> > loadout;
		Q_SplitString( cheatWeaponString, " ", loadout );
		for ( int i=0; i<loadout.Count(); ++i )
		{
			const char *item = loadout[i];
			for (int j = 0; j < PRIMARY_WEAPON_BUY_COUNT; j++)
			{
				if (FStrEq(primaryWeaponBuyInfo[j].buyAlias, item))
				{
					me->m_pArmament->BuyWeapon(primaryWeaponBuyInfo[j].id);
					break;
				}
			}
		}
		m_doneBuying = true;
		return;
	}


	// try to buy some weapons
	const float buyInterval = 0.02f;
	if (gpGlobals->curtime - me->GetStateTimestamp() > buyInterval)
	{
		me->m_stateTimestamp = gpGlobals->curtime;

		bool isPreferredAllDisallowed = true;

		// try to buy our preferred weapons first
		if (m_prefIndex < me->GetProfile()->GetWeaponPreferenceCount() && bot_randombuy.GetBool() == false )
		{
			// need to retry because sometimes first buy fails??
			const int maxPrefRetries = 2;
			if (m_prefRetries >= maxPrefRetries)
			{
				// try to buy next preferred weapon
				++m_prefIndex;
				m_prefRetries = 0;
				return;
			}

			CFWeaponID weaponPreference = (CFWeaponID)me->GetProfile()->GetWeaponPreference( m_prefIndex );

			if( me->m_pArmament->CanBuyWeapon(weaponPreference) < 0 )
			{
				// done with buying preferred weapon
				m_prefIndex = 9999;
				return;
			}

			const char *buyAlias = NULL;
			buyAlias = WeaponIDToAlias( weaponPreference );
			WeaponType type = GetWeaponType( buyAlias );
			switch( type )
			{
				case PISTOL:
					if (!TheCFBots()->AllowPistols())
						buyAlias = NULL;
					break;

				case SHOTGUN:
					if (!TheCFBots()->AllowLongGuns())
						buyAlias = NULL;
					break;

				case RIFLE:
					if (!TheCFBots()->AllowLongGuns())
						buyAlias = NULL;
					break;

				case LONGSWORD:
					if (!TheCFBots()->AllowLongSwords())
						buyAlias = NULL;
					break;

				case BUSTER:
					if (!TheCFBots()->AllowBusters())
						buyAlias = NULL;
					break;
			}

			if (buyAlias)
			{
				me->m_pArmament->BuyWeapon(weaponPreference);

				me->PrintIfWatched( "Tried to buy preferred weapon %s.\n", buyAlias );
				isPreferredAllDisallowed = false;
			}

			++m_prefRetries;

			// bail out so we dont waste money on other equipment
			// unless everything we prefer has been disallowed, then buy at random
			if (isPreferredAllDisallowed == false)
				return;
		}

		// if we have no preferred primary weapon (or everything we want is disallowed), buy at random
		if ((isPreferredAllDisallowed || !me->GetProfile()->HasPrimaryPreference()))
		{
			// build list of allowable weapons to buy
			BuyInfo *masterPrimary = primaryWeaponBuyInfo;
			BuyInfo *stockPrimary[ PRIMARY_WEAPON_BUY_COUNT ];
			int stockPrimaryCount = 0;

			for( int i=0; i<PRIMARY_WEAPON_BUY_COUNT; ++i )
			{
				if ((masterPrimary[i].type == SHOTGUN && TheCFBots()->AllowLongGuns()) ||
					(masterPrimary[i].type == RIFLE && TheCFBots()->AllowLongGuns()) ||
					(masterPrimary[i].type == PISTOL && TheCFBots()->AllowPistols()) ||
					(masterPrimary[i].type == LONGSWORD && TheCFBots()->AllowLongSwords()) ||
					(masterPrimary[i].type == BUSTER && TheCFBots()->AllowBusters()))
				{
					stockPrimary[ stockPrimaryCount++ ] = &masterPrimary[i];
				}
			}
 
			if (stockPrimaryCount)
			{
				// buy primary weapon if we don't have one
				int which = RandomInt( 0, stockPrimaryCount-1 );

				me->m_pArmament->BuyWeapon(stockPrimary[which]->id);

				me->PrintIfWatched( "Tried to buy %s.\n", stockPrimary[ which ]->buyAlias );
			}
		}


		//
		// If we now have a weapon, or have tried for too long, we're done
		//
		if (me->GetPrimaryWeapon() || m_retries++ > 5)
		{
			CCommand args;

			// pistols - if we have no preferred pistol, buy at random
			if (TheCFBots()->AllowPistols() && !me->GetProfile()->HasPistolPreference())
			{
				if (m_buyPistol)
				{
					//int which = RandomInt( 0, SECONDARY_WEAPON_BUY_COUNT-1 );
					
					const char *what = NULL;

					//what = secondaryWeaponBuyInfo[ which ].buyAlias;

					Q_snprintf( cmdBuffer, 256, "buy %s\n", what );
					args.Tokenize( cmdBuffer );
					me->ClientCommand( args );


					// only buy one pistol
					m_buyPistol = false;
				}

				// make sure we have enough pistol ammo
				args.Tokenize( "buy secammo" );
				me->ClientCommand( args );
			}

			m_doneBuying = true;
		}
	}
}

//--------------------------------------------------------------------------------------------------------------
void BuyState::OnExit( CCFBot *me )
{
	me->ResetStuckMonitor();
	me->EquipBestWeapon();
}

