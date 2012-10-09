//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Game-specific impact effect hooks
//
//=============================================================================//
#include "cbase.h"
#include "fx_impact.h"
#include "tempent.h"
#include "c_te_effect_dispatch.h"
#include "c_te_legacytempents.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Handle weapon effect callbacks
//-----------------------------------------------------------------------------
void CF_EjectBrass( int shell, const CEffectData &data )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if( !pPlayer )
		return;

	tempents->CSEjectBrass( data.m_vOrigin, data.m_vAngles, data.m_fFlags, shell, pPlayer );
}

/*
void CF_FX_EjectBrass_9mm_Callback( const CEffectData &data )
{
	CF_EjectBrass( CS_SHELL_9MM, data );
}

void CF_FX_EjectBrass_12Gauge_Callback( const CEffectData &data )
{
	CF_EjectBrass( CS_SHELL_12GAUGE, data );
}



DECLARE_CLIENT_EFFECT( "EjectBrass_9mm", CF_FX_EjectBrass_9mm_Callback );
DECLARE_CLIENT_EFFECT( "EjectBrass_12Gauge",CF_FX_EjectBrass_12Gauge_Callback );
*/
