//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "prediction.h"
#include "c_baseplayer.h"
#include "c_cf_player.h"
#include "igamemovement.h"


static CMoveData g_MoveData;
CMoveData *g_pMoveData = &g_MoveData;


class CCFPrediction : public CPrediction
{
DECLARE_CLASS( CCFPrediction, CPrediction );

public:
	virtual void	RunCommand( C_BasePlayer *player, CUserCmd *ucmd, IMoveHelper *moveHelper );
	virtual void	SetupMove( C_BasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move );
	virtual void	FinishMove( C_BasePlayer *player, CUserCmd *ucmd, CMoveData *move );
};

void CCFPrediction::RunCommand( C_BasePlayer *player, CUserCmd *ucmd, IMoveHelper *moveHelper )
{
	BaseClass::RunCommand( player, ucmd, moveHelper );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCFPrediction::SetupMove( C_BasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, 
	CMoveData *move )
{
	// Call the default SetupMove code.
	BaseClass::SetupMove( player, ucmd, pHelper, move );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCFPrediction::FinishMove( C_BasePlayer *player, CUserCmd *ucmd, CMoveData *move )
{
	// Call the default FinishMove code.
	BaseClass::FinishMove( player, ucmd, move );
}


// Expose interface to engine
// Expose interface to engine
static CCFPrediction g_Prediction;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CCFPrediction, IPrediction, VCLIENT_PREDICTION_INTERFACE_VERSION, g_Prediction );

CPrediction *prediction = &g_Prediction;

