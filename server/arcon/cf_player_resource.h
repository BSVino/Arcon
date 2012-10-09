//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: CF's custom CPlayerResource
//
// $NoKeywords: $
//=============================================================================//

#ifndef CF_PLAYER_RESOURCE_H
#define CF_PLAYER_RESOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "player_resource.h"

class CCFPlayerResource : public CPlayerResource
{
	DECLARE_CLASS( CCFPlayerResource, CPlayerResource );
	
public:
	DECLARE_SERVERCLASS();

	CCFPlayerResource();

	virtual void UpdatePlayerData( void );
	virtual void Spawn( void );

	int	GetTotalScore( int iIndex );

protected:
	CNetworkArray( int,	m_iTotalScore, MAX_PLAYERS+1 );
	CNetworkArray( int, m_iMaxHealth, MAX_PLAYERS+1 );
};

#endif // CF_PLAYER_RESOURCE_H
