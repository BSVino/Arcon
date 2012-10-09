//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "view.h"
#include "c_objectives.h"
#include "c_cf_objective_resource.h"
#include "dlight.h"
#include "iefx.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#undef CInfoObjective
IMPLEMENT_CLIENTCLASS_DT( C_InfoObjective, DT_InfoObjective, CInfoObjective )
	RecvPropBool		( RECVINFO( m_bAtSpawn ) ),
	RecvPropEHandle		( RECVINFO( m_hPlayer ) ),
END_RECV_TABLE()

void C_InfoObjective::Precache()
{
	PrecacheScriptSound("CaptureFlag.FlagHum");
}

void C_InfoObjective::Spawn()
{
	m_bSoundPlaying = false;

	C_CFObjectiveResource* pOR = CFObjectiveResource();
	Assert(pOR);
	if (pOR)
		pOR->AddObjective(this);
}

void C_InfoObjective::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged( updateType );

	SetNextClientThink(CLIENT_THINK_ALWAYS);
}

void C_InfoObjective::ClientThink()
{
	if (!m_bSoundPlaying)
	{
		EmitSound("CaptureFlag.FlagHum");
		m_bSoundPlaying = true;
	}

	dlight_t* dl = effects->CL_AllocDlight ( index );
	dl->origin = GetAbsOrigin();
	dl->origin[2] += 16;

	Color c = GameResources()->GetTeamColor(GetTeamNumber());
	dl->color.r = c.r();
	dl->color.g = c.g();
	dl->color.b = c.b();

	dl->radius = random->RandomFloat(200,250);
	dl->die = gpGlobals->curtime + 0.001;
}

CStudioHdr *C_InfoObjective::OnNewModel()
{
	// When the flag gets re-created, stop its firey particle systems from emitting too. Otherwise it will continue even after it's changed colors.
	// Why doesn't this happen elsewhere?
	ParticleProp()->StopEmission();

	return BaseClass::OnNewModel();
}

const Vector& C_InfoObjective::GetAbsOrigin( void ) const
{
	if (m_hPlayer != NULL)
	{
		return const_cast<C_InfoObjective*>(this)->GetRenderOrigin();
	}
	else
	{
		return BaseClass::GetAbsOrigin();	
	}
}

const Vector& C_InfoObjective::GetRenderOrigin( void )
{
	if (m_hPlayer != NULL)
	{
		if (!IsBoneAccessAllowed())
			return m_vecRenderOrigin;

		m_vecRenderOrigin = GetHoldingPosition(m_hPlayer);
		return m_vecRenderOrigin;
	}
	else
	{
		return BaseClass::GetRenderOrigin();	
	}
}

int C_InfoObjective::DrawModel( int flags )
{
	Vector vecOrigin;
	if (m_hPlayer != NULL)
		vecOrigin = m_vecRenderOrigin;
	else
		vecOrigin = GetAbsOrigin();

	Vector vecDirection = CurrentViewOrigin() - vecOrigin;
	QAngle angDrection;
	
	VectorAngles(vecDirection, angDrection);

	// Always orient towards the camera!
	SetAbsAngles( angDrection );

	return BaseClass::DrawModel( flags );
}
