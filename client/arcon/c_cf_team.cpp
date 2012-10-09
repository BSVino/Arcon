//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Client side C_CFTeam class
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "engine/IEngineSound.h"
#include "hud.h"
#include "recvproxy.h"
#include "c_cf_team.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


IMPLEMENT_CLIENTCLASS_DT(C_CFTeam, DT_CFTeam, CCFTeam)
	RecvPropInt( RECVINFO( m_nFlagCaptures ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_CFTeam::C_CFTeam()
{
	m_nFlagCaptures = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_CFTeam::~C_CFTeam()
{
}

C_CFTeam *GetGlobalCFTeam( int iIndex )
{
	return (C_CFTeam*)GetGlobalTeam( iIndex );
}
