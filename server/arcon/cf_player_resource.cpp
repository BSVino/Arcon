//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: CF's custom CPlayerResource
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "cf_player.h"
#include "player_resource.h"
#include "cf_player_resource.h"
#include "cf_gamerules.h"
#include <coordsize.h>

// Datatable
IMPLEMENT_SERVERCLASS_ST( CCFPlayerResource, DT_CFPlayerResource )
	SendPropArray3( SENDINFO_ARRAY3( m_iTotalScore ), SendPropInt( SENDINFO_ARRAY( m_iTotalScore ), 12, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iMaxHealth ), SendPropInt( SENDINFO_ARRAY( m_iMaxHealth ), 11, SPROP_UNSIGNED ) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( cf_player_manager, CCFPlayerResource );

CCFPlayerResource::CCFPlayerResource( void )
{
	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCFPlayerResource::UpdatePlayerData( void )
{
	int i;

	BaseClass::UpdatePlayerData();

	for ( i = 1 ; i <= gpGlobals->maxClients; i++ )
	{
		CCFPlayer *pPlayer = (CCFPlayer*)UTIL_PlayerByIndex( i );
		
		if ( pPlayer && pPlayer->IsConnected() )
		{			
			m_iMaxHealth.Set( i, pPlayer->GetMaxHealth() );
			m_iTotalScore.Set( i, pPlayer->GetEloScore() );
		}
	}
}

void CCFPlayerResource::Spawn( void )
{
	int i;

	for ( i = 0; i < MAX_PLAYERS + 1; i++ )
	{
		m_iTotalScore.Set( i, 0 );
		m_iMaxHealth.Set( i, 1 );
	}

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Gets a value from an array member
//-----------------------------------------------------------------------------
int CCFPlayerResource::GetTotalScore( int iIndex )
{
	CCFPlayer *pPlayer = (CCFPlayer*)UTIL_PlayerByIndex( iIndex );

	if ( pPlayer && pPlayer->IsConnected() )
	{	
		return m_iTotalScore[iIndex];
	}

	return 0;
}