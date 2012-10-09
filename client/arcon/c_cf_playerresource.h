//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: TF's custom C_PlayerResource
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_CF_PLAYERRESOURCE_H
#define C_CF_PLAYERRESOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "cf_shareddefs.h"
#include "c_playerresource.h"

class C_CF_PlayerResource : public C_PlayerResource
{
	DECLARE_CLASS( C_CF_PlayerResource, C_PlayerResource );
public:
	DECLARE_CLIENTCLASS();

	C_CF_PlayerResource();
	virtual ~C_CF_PlayerResource();

	int	GetTotalScore( int iIndex ) { return GetArrayValue( iIndex, m_iTotalScore, 0 ); }
	int GetMaxHealth( int iIndex )   { return GetArrayValue( iIndex, m_iMaxHealth, 1 ); }

protected:
	int GetArrayValue( int iIndex, int *pArray, int defaultVal );

	int		m_iTotalScore[MAX_PLAYERS+1];
	int		m_iMaxHealth[MAX_PLAYERS+1];
};


#endif // C_CF_PLAYERRESOURCE_H