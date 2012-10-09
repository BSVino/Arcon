//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: DOD's objective resource, transmits all objective states to players
//
// $NoKeywords: $
//=============================================================================//

#ifndef CF_OBJECTIVE_RESOURCE_H
#define CF_OBJECTIVE_RESOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "cf_shareddefs.h"
#include "team_objectiveresource.h"
#include "objectives.h"

class CCFObjectiveResource : public CBaseTeamObjectiveResource
{
	DECLARE_CLASS( CCFObjectiveResource, CBaseTeamObjectiveResource );
public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	virtual void Spawn( void );

	virtual void SetCapturePoint(int iTeam, Vector vecObjective);

	CNetworkArray(Vector, m_avecCapturePoints, CF_TEAM_COUNT);
};

inline CCFObjectiveResource* CFObjectiveResource()
{
	return (CCFObjectiveResource*)g_pObjectiveResource;
}

#endif	// CF_OBJECTIVE_RESOURCE_H

