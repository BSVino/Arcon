//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Client side CTFTeam class
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_CF_TEAM_H
#define C_CF_TEAM_H
#ifdef _WIN32
#pragma once
#endif

#include "c_team.h"
#include "shareddefs.h"
#include "c_cf_player.h"

class C_BaseEntity;
class C_BaseObject;
class CBaseTechnology;

//-----------------------------------------------------------------------------
// Purpose: TF's Team manager
//-----------------------------------------------------------------------------
class C_CFTeam : public C_Team
{
	DECLARE_CLASS( C_CFTeam, C_Team );
	DECLARE_CLIENTCLASS();

public:

					C_CFTeam();
	virtual			~C_CFTeam();

	int				GetFlagCaptures( void ) { return m_nFlagCaptures; }

private:
	int		m_nFlagCaptures;

	CNetworkHandle( CCFPlayer, m_hCaptain );
	CNetworkHandle( CCFPlayer, m_hSergeant );
};

extern C_CFTeam *GetGlobalCFTeam( int iIndex );

#endif // C_CF_TEAM_H
