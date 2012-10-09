//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: CF's custom C_PlayerResource
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_cf_playerresource.h"
#include <shareddefs.h>
#include <cf_shareddefs.h>
#include "hud.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT( C_CF_PlayerResource, DT_CFPlayerResource, CCFPlayerResource )
	RecvPropArray3( RECVINFO_ARRAY( m_iTotalScore ), RecvPropInt( RECVINFO( m_iTotalScore[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iMaxHealth ), RecvPropInt( RECVINFO( m_iMaxHealth[0] ) ) ),
END_RECV_TABLE()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_CF_PlayerResource::C_CF_PlayerResource()
{
	m_Colors[TEAM_NUMENI] = g_aTeamColors[TEAM_NUMENI];
	m_Colors[TEAM_MACHINDO] = g_aTeamColors[TEAM_MACHINDO];
	m_Colors[TEAM_SPECTATOR] = COLOR_GREY;
	m_Colors[TEAM_UNASSIGNED] = COLOR_GREY;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_CF_PlayerResource::~C_CF_PlayerResource()
{
}

//-----------------------------------------------------------------------------
// Purpose: Gets a value from an array member
//-----------------------------------------------------------------------------
int C_CF_PlayerResource::GetArrayValue( int iIndex, int *pArray, int iDefaultVal )
{
	if ( !IsConnected( iIndex ) )
	{
		return iDefaultVal;
	}
	return pArray[iIndex];
}
