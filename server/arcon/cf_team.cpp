//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Team management class. Contains all the details for a specific team
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "cf_team.h"
#include "entitylist.h"
#include "cf_shareddefs.h"
#include "cf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


// Datatable
IMPLEMENT_SERVERCLASS_ST(CCFTeam, DT_CFTeam)
	SendPropInt( SENDINFO( m_nFlagCaptures ) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( cf_team_manager, CCFTeam );

//-----------------------------------------------------------------------------
// Purpose: Get a pointer to the specified TF team manager
//-----------------------------------------------------------------------------
CCFTeam *GetGlobalCFTeam( int iIndex )
{
	return (CCFTeam*)GetGlobalTeam( iIndex );
}


CCFTeam::CCFTeam()
{
	m_nFlagCaptures = 0;
}

CCFTeam::~CCFTeam()
{
}

//-----------------------------------------------------------------------------
// Purpose: Needed because this is an entity, but should never be used
//-----------------------------------------------------------------------------
void CCFTeam::Init( const char *pName, int iNumber )
{
	BaseClass::Init( pName, iNumber );

	// Only detect changes every half-second.
	NetworkProp()->SetUpdateInterval( 0.75f );
}

// Static functions.

void CCFTeam::CreateTeams()
{
	g_Teams.Purge();

	// Create the team list.
	for ( int iTeam = 0; iTeam < CF_TEAM_COUNT; ++iTeam )
	{
		int index = CreateTeam( g_aTeamNames[iTeam] );
		Assert( index == iTeam );
		if ( index != iTeam )
			return;
	}
}

int CCFTeam::CreateTeam( const char *pName )
{
	CTeam *pTeam = static_cast<CTeam*>( CreateEntityByName( "cf_team_manager" ) );
	if ( pTeam )
	{
		// Add the team to the global list of teams.
		int iTeam = g_Teams.AddToTail( pTeam );

		// Initialize the team.
		pTeam->Init( pName, iTeam );

		return iTeam;
	}

	// Error.
	return -1;
}

int CCFTeam::GetScore( void )
{
	int iScore = 0;

	if (CFGameRules()->GetGameMode() == CF_GAME_CTF)
	{
		CInfoObjective *pFlag = NULL;
		while ( (pFlag = dynamic_cast<CInfoObjective*>( gEntList.FindEntityByClassname( pFlag, "info_objective*" ) )) != NULL )
		{
			if (pFlag->GetTeamNumber() == GetTeamNumber())
				iScore++;
		}
	}

	return iScore;
}

CCFPlayer* CCFTeam::GetHighestScoringPlayer(CCFPlayer* pBelow)
{
	int iHighestScore = 0;
	CCFPlayer* pHighestScoringPlayer = NULL;

	for (int i = 0; i < GetNumPlayers(); i++)
	{
		CCFPlayer* pPlayer = ToCFPlayer(m_aPlayers[i]);

		if (!pPlayer)
			continue;

		int iScore = pPlayer->GetEloScore();

		if (pPlayer == pBelow)
			continue;

		if (pBelow && iScore >= (int)pBelow->GetEloScore())
			continue;

		if (iScore > iHighestScore)
		{
			pHighestScoringPlayer = pPlayer;
			iHighestScore = iScore;
		}
	}

	return pHighestScoringPlayer;
}

void CCFTeam::PromoteToCaptain(CCFPlayer *pPlayer)
{
	if (pPlayer->IsCaptain())
		return;

	if (m_hCaptain != NULL)
		m_hCaptain->DemoteRank();

	pPlayer->PromoteToCaptain();
	m_hCaptain = pPlayer;
	if (GetSergeant() == GetCaptain())
		m_hSergeant = NULL;
}

void CCFTeam::PromoteToSergeant(CCFPlayer *pPlayer)
{
	if (pPlayer->IsCaptain() || pPlayer->IsSergeant())
		return;

	if (m_hSergeant != NULL)
		m_hSergeant->DemoteRank();

	pPlayer->PromoteToSergeant();
	m_hSergeant = pPlayer;
}

void CCFTeam::DemotePlayer(CCFPlayer *pPlayer)
{
	if (!pPlayer->IsCaptain() && !pPlayer->IsSergeant())
		return;

	if (GetCaptain() == pPlayer)
		m_hCaptain = NULL;

	if (GetSergeant() == pPlayer)
		m_hSergeant = NULL;

	pPlayer->DemoteRank();
}

void CCFTeam::SpeakConceptFromLeaders(int iConcept, const char* pszModifiers)
{
	CCFPlayer::MarkAllPlayers(false);

	int i;
	CCFPlayer* pTalker = NULL;
	if (GetCaptain())
		pTalker = GetCaptain();
	else if (GetSergeant())
		pTalker = GetSergeant();
	else
		pTalker = GetHighestScoringPlayer();

	while (pTalker)
	{
		pTalker->SpeakConceptIfAllowed( iConcept, pszModifiers );
		pTalker->Mark();

		//NDebugOverlay::Box( pTalker->GetCentroid(), Vector(-2,-2,-2), Vector(2,2,2), 0,0,255,127, 10 );

		// Figure out who can hear it.
		for (i = 0; i < GetNumPlayers(); i++)
		{
			CCFPlayer* pPlayer = ToCFPlayer(m_aPlayers[i]);

			if (!pPlayer)
				continue;

			if (pPlayer->IsMarked())
				continue;

			if ((pTalker->GetAbsOrigin() - pPlayer->GetAbsOrigin()).LengthSqr() > 700*700)
				continue;

			if (!pPlayer->IsVisible(pTalker))
				continue;

			// If we heard it, we don't have to say it ourselves.
			pPlayer->Mark();
		}

		// If we have a sergeant that didn't hear the Captain, use him.
		if (GetSergeant() && !GetSergeant()->IsMarked())
		{
			pTalker = GetSergeant();
			continue;
		}

		// Find the next highest scoring player who hasn't already heard it and use him as the next talker.
		int iHighestScore = 0;
		CCFPlayer* pHighestScoringPlayer = NULL;

		for (i = 0; i < GetNumPlayers(); i++)
		{
			CCFPlayer* pPlayer = ToCFPlayer(m_aPlayers[i]);

			if (!pPlayer)
				continue;

			if (pPlayer->GetTeamNumber() != GetTeamNumber())
				continue;

			if (pPlayer->IsMarked())
				continue;

			int iScore = pPlayer->GetEloScore();

			if (iScore > iHighestScore)
			{
				pHighestScoringPlayer = pPlayer;
				iHighestScore = iScore;
			}
		}

		pTalker = pHighestScoringPlayer;
	}
}
