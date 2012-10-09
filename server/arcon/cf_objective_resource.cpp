//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates general data needed by clients for every player.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "cf_objective_resource.h"
#include "shareddefs.h"
#include <coordsize.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Datatable
IMPLEMENT_SERVERCLASS_ST( CCFObjectiveResource, DT_CFObjectiveResource )
	SendPropArray( SendPropVector( SENDINFO_ARRAY( m_avecCapturePoints ), -1, SPROP_COORD ), m_avecCapturePoints ),
END_SEND_TABLE()


BEGIN_DATADESC( CCFObjectiveResource )
END_DATADESC()


LINK_ENTITY_TO_CLASS( cf_objective_resource, CCFObjectiveResource );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCFObjectiveResource::Spawn( void )
{
	BaseClass::Spawn();
}

void CCFObjectiveResource::SetCapturePoint(int iTeam, Vector vecObjective)
{
	m_avecCapturePoints.Set(iTeam, vecObjective);
}
