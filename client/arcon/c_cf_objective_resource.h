//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates general data needed by clients for every player.
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_CF_OBJECTIVE_RESOURCE_H
#define C_CF_OBJECTIVE_RESOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "cf_shareddefs.h"
#include "const.h"
#include "c_baseentity.h"
#include <igameresources.h>
#include "c_team_objectiveresource.h"
#include "c_objectives.h"
#include "cf_shareddefs.h"

class C_CFObjectiveResource : public C_BaseTeamObjectiveResource
{
	DECLARE_CLASS( C_CFObjectiveResource, C_BaseTeamObjectiveResource );
public:
	DECLARE_CLIENTCLASS();

					C_CFObjectiveResource();
	virtual			~C_CFObjectiveResource();

	virtual void	ClientThink();

	virtual void				AddObjective(C_InfoObjective* pObjective);
	virtual C_InfoObjective*	GetObjective(int i);
	virtual int					GetObjectives();

	virtual Vector&				GetCapturePoint(int iTeam);

protected:
	CUtlVector<CHandle<C_InfoObjective>>	m_ahObjectives;
	Vector						m_avecCapturePoints[CF_TEAM_COUNT];
};

inline C_CFObjectiveResource *CFObjectiveResource()
{
	return static_cast<C_CFObjectiveResource*>(g_pObjectiveResource);
}

#endif // C_CF_OBJECTIVE_RESOURCE_H
