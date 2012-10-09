//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: The TF Game rules object
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef CF_GAMERULES_H
#define CF_GAMERULES_H

#ifdef _WIN32
#pragma once
#endif


#include "teamplayroundbased_gamerules.h"
#include "convar.h"
#include "gamevars_shared.h"
#include "cf_shareddefs.h"
#include "weapon_pariah.h"

#ifdef CLIENT_DLL
	#include "c_cf_player.h"
	#include "c_baseplayer.h"
#else
	#include "cf_player.h"
	#include "player.h"
	#include "objectives.h"
#endif


enum
{
	SF_SUPPORTS_TDM				= 0x100000,
	SF_SUPPORTS_CTF				= 0x200000,
	SF_SUPPORTS_PARIAH			= 0x400000,
};

#define SF_SUPPORTS_ANY (SF_SUPPORTS_TDM|SF_SUPPORTS_CTF|SF_SUPPORTS_PARIAH)

#define	WINNER_NONE		0
#define WINNER_DRAW		1
#define WINNER_MACHINDO	TEAM_MACHINDO
#define WINNER_NUMENI	TEAM_NUMENI

#ifndef CLIENT_DLL
extern ConVar mp_autoteambalance;
#endif // !CLIENT_DLL

#ifdef CLIENT_DLL
	#define CCFGameRules C_CFGameRules
	#define CCFGameRulesProxy C_CFGameRulesProxy
#endif

class CCFGameRulesProxy : public CTeamplayRoundBasedRulesProxy
{
public:
	DECLARE_CLASS( CCFGameRulesProxy, CTeamplayRoundBasedRulesProxy );
	DECLARE_NETWORKCLASS();
};


class CCFGameRules : public CTeamplayRoundBasedRules
{
public:
	DECLARE_CLASS( CCFGameRules, CTeamplayRoundBasedRules );

	virtual bool	ShouldCollide( int collisionGroup0, int collisionGroup1 );

	virtual int		PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );

	virtual CFGameType GetGameMode() { return m_eGameMode; };

#ifdef CLIENT_DLL

	DECLARE_CLIENTCLASS_NOBASE(); // This makes datatables able to access our private vars.

	virtual bool		ShouldShowFuse(CCFPlayer* pPlayer);
	virtual Vector		GetFuseLocation();

	virtual bool		ShouldShowPariah(CCFPlayer* pPlayer);
	virtual Vector		GetPariahLocation();

#else

	DECLARE_SERVERCLASS_NOBASE(); // This makes datatables able to access our private vars.
	
						CCFGameRules();
	virtual				~CCFGameRules();

	virtual bool		ClientCommand( CBaseEntity *pEdict, const CCommand &args );
	virtual void		ClientDisconnected( edict_t *pClient );
	virtual void		RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore );
	virtual void		Think();
	virtual void		EndGameFrame( void );

	virtual void		FireGameEvent( IGameEvent *event );
	virtual const char*	GetGameDescription( void );

	virtual int			DefaultFOV( void ) { return 75; }

	virtual void		CreateStandardEntities();

	virtual bool		RoundCleanupShouldIgnore( CBaseEntity *pEnt );
	virtual bool		ShouldCreateEntity( const char *pszClassName );
	virtual void		CleanUpMap( void );

	virtual void		GoToIntermission();
	virtual void		SetInWaitingForPlayers( bool bWaitingForPlayers );
	virtual bool		CanGoToStalemate( void );
	virtual bool		CheckCapsPerRound();
	virtual void		RoundRespawn( void );
	virtual void		SetupOnRoundStart( void );
	virtual void		SetupOnRoundRunning( void );
	void				RecalculateControlPointState( void );
	virtual void		InternalHandleTeamWin( int iWinningTeam );
	virtual void		SendWinPanelInfo( void );

	virtual void		OnOvertimeStart();

	virtual bool		ShouldScorePerRound( void ){ return false; }

	virtual float		GetCaptureValueForPlayer( CBasePlayer *pPlayer );
	int					GetFarthestOwnedControlPoint( int iTeam, bool bWithSpawnpoints );
	virtual bool		PlayerMayCapturePoint( CBasePlayer *pPlayer, int iPointIndex, char *pszReason = NULL, int iMaxReasonLength = 0 );
	virtual bool		PlayerMayBlockPoint( CBasePlayer *pPlayer, int iPointIndex, char *pszReason = NULL, int iMaxReasonLength = 0 );

	void				AddFlagToWell(CInfoObjective* pFlag);
	void				RemoveFlagFromWell(CInfoObjective* pFlag);
	void				ResetWell(int iTeam);
	void				EmptyWells();

	virtual void		PlayWinSong( int team );
	virtual void		BroadcastSound( const char *sound );
	virtual void		SpeakConceptFromTeamLeaders( int iConcept, const char* pszModifiers=NULL );

	// Pariah mode.
	virtual void		PickRandomFuse();
	virtual void		PickRandomPariah();
	virtual void		MakePariahBlade( Vector vecOrigin );
	virtual void		MakeFuse(CCFPlayer* pPlayer);
	virtual bool		MakePariah(CCFPlayer* pPlayer, class CWeaponPariahBlade* pBlade);
	virtual void		RemoveFuse(CCFPlayer* pPlayer = NULL);
	virtual void		RemovePariah(CCFPlayer* pPlayer = NULL);
	virtual bool		ShouldShowDamage(CCFPlayer* pVictim, CCFPlayer* pAttacker);

	virtual bool		CanGoOverdrive(CCFPlayer* pPlayer);
	virtual bool		CanGiveOverdrive(CCFPlayer* pPlayer);

	virtual CBaseEntity*GetPlayerSpawnSpot( CBasePlayer *pPlayer );
	virtual bool		IsSpawnPointValid( CBaseEntity *pSpot, CBasePlayer *pPlayer );
	virtual bool		CanHavePlayerItem( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon );

	virtual const char*	GetChatPrefix( bool bTeamOnly, CBasePlayer *pPlayer );

	virtual void		LevelInitPostEntity();
	virtual void		LevelInitGameMode();

	virtual float		GetAverageElo(CCFPlayer *pExclude = NULL, int iTeam = TEAM_UNASSIGNED);

	virtual bool		FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker, CTakeDamageInfo& info );
	virtual bool		CanReceiveCritical(CCFPlayer* pVictim, CCFPlayer* pAttacker);
	virtual bool		CanFreezeVictim(CCFPlayer* pVictim, CCFPlayer* pAttacker);
	virtual int			IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled );
	virtual void		PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &inputInfo );
	virtual void		DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info );
	virtual const char*	GetDamageCustomString( const CTakeDamageInfo &info );

	virtual bool		CanBePromoted(CCFPlayer *pPlayer);

	static CFGameType	GameModeFromString(const char* pszString);
	static int			GameModeSpawnFlag(CFGameType eGameMode);

	virtual int			SelectBestTeam( bool ignoreBots = false );

#if defined(ARCON_MOD) || defined(ARCON_DEMO)
	virtual bool		ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );
	virtual void		ServerValidated() { m_bServerValidated = true; };
	bool				m_bServerValidated;
#endif

private:

	void				RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore, bool bIgnoreWorld );

	CUtlVector<Vector>		m_avecFlagWells;
	CUtlVector<CHandle<CInfoObjective> >	m_ahFlagWells [MAX_TEAMS];

	bool				m_bPlayedRoundEndingVOs;

#endif


	CNetworkVar(CFGameType, m_eGameMode);

	CNetworkHandle( CCFPlayer, m_hFuse );
	CNetworkHandle( CCFPlayer, m_hPariah );
	CNetworkHandle( CWeaponPariahBlade, m_hPariahBlade);

	CUtlVector<Vector>		m_avecPariahStartingPoints;

	// Stole these from TF2
	bool					m_bControlSpawnsPerTeam[ MAX_TEAMS ][ MAX_CONTROL_POINTS ];
	int						m_iNumCaps[CF_TEAM_COUNT];				// # of captures ever by each team during a round
};

//-----------------------------------------------------------------------------
// Gets us at the team fortress game rules
//-----------------------------------------------------------------------------

inline CCFGameRules* CFGameRules()
{
	return static_cast<CCFGameRules*>(g_pGameRules);
}

#endif // CF_GAMERULES_H
