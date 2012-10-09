//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Team management class. Contains all the details for a specific team
//
// $NoKeywords: $
//=============================================================================//

#ifndef CF_TEAM_H
#define CF_TEAM_H

#ifdef _WIN32
#pragma once
#endif


#include "utlvector.h"
#include "team.h"
#include "cf_player.h"

//-----------------------------------------------------------------------------
// Purpose: Team Manager
//-----------------------------------------------------------------------------
class CCFTeam : public CTeam
{
	DECLARE_CLASS( CCFTeam, CTeam );
	DECLARE_SERVERCLASS();

public:

					CCFTeam();
					~CCFTeam();

	// Initialization
	virtual void Init( const char *pName, int iNumber );

	// Flag Captures
	int				GetFlagCaptures( void ) { return m_nFlagCaptures; }
	void			SetFlagCaptures( int nCaptures ) { m_nFlagCaptures = nCaptures; }
	void			IncrementFlagCaptures( void ) { m_nFlagCaptures++; }

	virtual void	AddScore( int iScore ) { Assert(false); };
	virtual void	SetScore( int iScore ) { Assert(false); };
	virtual int		GetScore( void );
	virtual void	ResetScores( void ) {};

	CCFPlayer*		GetHighestScoringPlayer(CCFPlayer* pBelow = NULL);
	void			PromoteToCaptain(CCFPlayer *pPlayer);
	void			PromoteToSergeant(CCFPlayer *pPlayer);
	void			DemotePlayer(CCFPlayer *pPlayer);
	CCFPlayer*		GetCaptain() { return m_hCaptain; };
	CCFPlayer*		GetSergeant() { return m_hSergeant; };

	void			SpeakConceptFromLeaders(int iConcept, const char* pszModifiers = NULL);

	static void CreateTeams();
	static int CreateTeam( const char *pName );

private:
	CNetworkVar( int, m_nFlagCaptures );

	CNetworkHandle( CCFPlayer, m_hCaptain );
	CNetworkHandle( CCFPlayer, m_hSergeant );
};


extern CCFTeam *GetGlobalCFTeam( int iIndex );


#endif // CF_TEAM_H
