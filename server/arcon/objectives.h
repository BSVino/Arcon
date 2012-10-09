//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef OBJECTIVES_H
#define OBJECTIVES_H
#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"
#include "baseentity.h"
#include "baseflex.h"
#include "cf_player.h"
#include "trigger_area_capture.h"
#include "team_control_point.h"
#include "team_control_point_master.h"

class CInfoGame : public CPointEntity
{
	DECLARE_CLASS( CInfoGame, CPointEntity );
public:
	DECLARE_DATADESC();

	void	LevelInitPostEntity();

	bool	SupportsGameMode(CFGameType eGameMode);

	string_t	m_iszTeamNames[5];
};

class CInfoObjective : public CBaseFlex
{
	DECLARE_CLASS( CInfoObjective, CBaseFlex );
	DECLARE_SERVERCLASS();
public:
	DECLARE_DATADESC();

			CInfoObjective();

	void	Spawn( void );
	bool	KeyValue( const char *szKeyName, const char *szValue );
	void	Precache( void );
	void	SpawnThink ( void );
	void	DroppedThink ( void );
	void	NeutralizeThink ( void );
	void	ObjectiveTouch ( CBaseEntity *pOther );

	void	Drop ( bool bThrow = false );
	void	Captured ( void );
	void	Return ( bool bCaptured );

	Vector	GetHoldingPosition(CCFPlayer* pHolder);

	bool	IsAtSpawn() { return m_bAtSpawn; }

	void	SetReturnPosition(Vector vecReturn) { m_vecReturnPosition = vecReturn; };

	virtual int	UpdateTransmitState();

	// Input handlers
	virtual void InputRespawn( inputdata_t &inputdata );

	COutputEvent m_OnPickup;
	COutputEvent m_OnDrop;
	COutputEvent m_OnCapture;
	COutputEvent m_OnReturn;

	Vector	m_vecOriginalPosition;
	Vector	m_vecReturnPosition;

	bool	m_bPowerjumping;
	CNetworkVar( bool,	m_bAtSpawn );

	CNetworkHandle(CCFPlayer, m_hPlayer);

	struct perteamdata_t
	{
		perteamdata_t()
		{
			iszModel = NULL_STRING;
		}

		string_t	iszModel;
	};
	CUtlVector<perteamdata_t>	m_TeamData;
};

class CInfoFlag : public CInfoObjective
{
	DECLARE_CLASS( CInfoFlag, CInfoObjective );
public:

	DECLARE_DATADESC();
};

class CTriggerObjective : public CBaseTrigger
{
	DECLARE_CLASS( CTriggerObjective, CBaseTrigger );
public:
	DECLARE_DATADESC();

	void Spawn( void );
	void ObjectiveTouch ( CBaseEntity *pOther );
	void MultiWaitOver( void );
	void ActivateMultiTrigger(CBaseEntity *pActivator);

	// Outputs
	COutputEvent m_OnObjective;
};

class CInfoScore : public CPointEntity
{
	DECLARE_CLASS( CInfoScore, CPointEntity );
public:

	DECLARE_DATADESC();

	bool	m_bRespawnPlayers;

	void	RespawnThink ( void );

	// Input handlers
	virtual void InputScore( inputdata_t &inputdata );
};

class CCFAreaCapture : public CTriggerAreaCapture
{
	DECLARE_CLASS( CCFAreaCapture, CTriggerAreaCapture );
public:
	DECLARE_DATADESC();

	virtual void		Spawn( void );
};

class CCFControlPoint : public CTeamControlPoint
{
	DECLARE_CLASS( CCFControlPoint, CTeamControlPoint );
public:
	DECLARE_DATADESC();

	virtual void PlayerCapped( CBaseMultiplayerPlayer *pPlayer );
	virtual void PlayerBlocked( CBaseMultiplayerPlayer *pPlayer );
};

class CCFControlPointMaster : public CTeamControlPointMaster
{
	DECLARE_CLASS( CCFControlPointMaster, CTeamControlPointMaster );
public:
	DECLARE_DATADESC();

	virtual const char *GetTriggerAreaCaptureName( void ) { return "cf_trigger_capture_area"; }
	virtual const char *GetControlPointName( void ) { return "cf_team_control_point"; }
	virtual const char *GetControlPointRoundName( void ) { return "cf_team_control_point_round"; }
};

#endif // OBJECTIVES_H
