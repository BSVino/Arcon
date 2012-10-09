//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Spawn Point.
//
//=============================================================================//
#ifndef CF_SPAWNPOINT_H
#define CF_SPAWNPOINT_H

#ifdef _WIN32
#pragma once
#endif

class CTeamControlPoint;
class CTeamControlPointRound;

//=============================================================================
//
// CF team spawning entity.
//

class CCFTeamSpawn : public CPointEntity
{
public:
	DECLARE_CLASS( CCFTeamSpawn, CPointEntity );

	CCFTeamSpawn();

	void Activate( void );

	bool IsDisabled( void ) { return m_bDisabled; }
	void SetDisabled( bool bDisabled ) { m_bDisabled = bDisabled; }

	// Inputs/Outputs.
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputRoundSpawn( inputdata_t &inputdata );

	int DrawDebugTextOverlays(void);

	CHandle<CTeamControlPoint> GetControlPoint( void ) { return m_hControlPoint; }
	CHandle<CTeamControlPointRound> GetRoundBlueSpawn( void ) { return m_hRoundBlueSpawn; }
	CHandle<CTeamControlPointRound> GetRoundRedSpawn( void ) { return m_hRoundRedSpawn; }

private:
	bool	m_bDisabled;		// Enabled/Disabled?

	string_t						m_iszControlPointName;
	string_t						m_iszRoundBlueSpawn;
	string_t						m_iszRoundRedSpawn;

	CHandle<CTeamControlPoint>		m_hControlPoint;
	CHandle<CTeamControlPointRound>	m_hRoundBlueSpawn;
	CHandle<CTeamControlPointRound>	m_hRoundRedSpawn;

	DECLARE_DATADESC();
};

#endif // CF_SPAWNPOINT_H


