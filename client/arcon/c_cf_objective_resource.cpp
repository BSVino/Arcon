//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates objective data
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "clientmode_cf.h"
#include "c_cf_objective_resource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


IMPLEMENT_CLIENTCLASS_DT( C_CFObjectiveResource, DT_CFObjectiveResource, CCFObjectiveResource)
		RecvPropArray	( RecvPropVector( RECVINFO( m_avecCapturePoints[0] ) ), m_avecCapturePoints ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_CFObjectiveResource::C_CFObjectiveResource()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_CFObjectiveResource::~C_CFObjectiveResource()
{
}

void C_CFObjectiveResource::ClientThink()
{
	// This won't catch and remove all old objectives, so be prepared for GetObjective() to return NULL.
	for (int i = 0; i < m_ahObjectives.Count(); i++)
	{
		if (m_ahObjectives[i] == NULL)
			m_ahObjectives.Remove(i);
	}

	BaseClass::ClientThink();
}

void C_CFObjectiveResource::AddObjective(C_InfoObjective* pObjective)
{
	m_ahObjectives.AddToTail(pObjective);
}

C_InfoObjective* C_CFObjectiveResource::GetObjective(int i)
{
	return m_ahObjectives[i];
}

int C_CFObjectiveResource::GetObjectives()
{
	return m_ahObjectives.Count();
}

Vector& C_CFObjectiveResource::GetCapturePoint(int iTeam)
{
	Assert(iTeam > LAST_SHARED_TEAM && iTeam < CF_TEAM_COUNT);
	if (iTeam <= LAST_SHARED_TEAM)
		return Vector(0,0,0);
	else if (iTeam >= CF_TEAM_COUNT)
		return Vector(0,0,0);

	return m_avecCapturePoints[iTeam];
}
