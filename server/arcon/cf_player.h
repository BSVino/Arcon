//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Player for CF Game
//
// $NoKeywords: $
//=============================================================================//

#ifndef CF_PLAYER_H
#define CF_PLAYER_H
#pragma once


#include "basemultiplayerplayer.h"
#include "server_class.h"
#include "cf_playeranimstate.h"
#include "cf_shareddefs.h"
#include "weapon_cfbase.h"
#include "weapon_cfbasemelee.h"
#include "cfgui_shared.h"
#include "cf_team.h"
#include "objectives.h"

extern ConVar bot_mimic;

//=============================================================================
// >> CF Game player
//=============================================================================
class CCFPlayer : public CBaseMultiplayerPlayer
{
public:
	DECLARE_CLASS( CCFPlayer, CBaseMultiplayerPlayer );
	DECLARE_SERVERCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_DATADESC();

	friend class CCFGameRules;
	friend class CCFTeam;

	typedef enum
	{
		VIS_NONE		= 0x00,
		VIS_GUT			= 0x01,
		VIS_HEAD		= 0x02,
		VIS_LEFT_SIDE	= 0x04,			///< the left side of the object from our point of view (not their left side)
		VIS_RIGHT_SIDE	= 0x08,			///< the right side of the object from our point of view (not their right side)
		VIS_FEET		= 0x10
	} VisiblePartType;

	struct PartInfo
	{
		Vector m_headPos;											///< current head position
		Vector m_gutPos;											///< current gut position
		Vector m_feetPos;											///< current feet position
		Vector m_leftSidePos;										///< current left side position
		Vector m_rightSidePos;										///< current right side position
		int m_validFrame;											///< frame of last computation (for lazy evaluation)
	};

	CCFPlayer();
	~CCFPlayer();

	static CCFPlayer *CreatePlayer( const char *className, edict_t *ed );
	static CCFPlayer* Instance( int iEnt );

	virtual void ChangeTeam( int iTeamNum ) { ChangeTeam(iTeamNum,false, false); }
	virtual void ChangeTeam(int iTeamNum, bool bAutoTeam, bool bSilent);

	// This passes the event to the client's and server's CPlayerAnimState.
	virtual void DoAnimationEvent( PlayerAnimEvent_t event, int mData = 0, bool bSecondary = false );
	virtual void SetAnimation( PLAYER_ANIM playerAnim );
	virtual bool IsInFullBodyAnimation();
	virtual bool ShouldUseForwardFacingAnimationStyle();

	virtual void HandleAnimEvent( animevent_t *pEvent );

	virtual void Rush();
	virtual void Rush(CCFBaseMeleeWeapon* pWeapon, float flDistance, int iAttack);
	virtual void EndRush();

	virtual void FlashlightTurnOn( void );
	virtual void FlashlightTurnOff( void );
	virtual int FlashlightIsOn( void );

	virtual void PreThink();
	virtual void PostThink();
	virtual void Spawn();
	virtual void SharedSpawn();
	virtual void InitialSpawn();
	virtual CBaseEntity* EntSelectSpawnPoint( void );
	virtual void Precache();
	virtual void ItemPreFrame( void );
	virtual void ItemPostFrame( void );
	virtual bool ClientCommand( const CCommand &args );

	virtual bool IsReadyToPlay( void );
	virtual bool IsReadyToSpawn( void );
	virtual bool ShouldGainInstantSpawn( void );

	virtual bool ShouldCollide( int collisionGroup, int contentsMask ) const;
	virtual void Touch( CBaseEntity *pOther );
	virtual void SharedTouch( CBaseEntity *pOther );

	virtual void StopPowerjump();
	virtual void StartLatch(const trace_t &tr, CBaseEntity* pOther);
	virtual void StopLatch();
	virtual void LatchThink();
	virtual bool IsLatchable() { return false; };

	virtual void ReviveUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual void StartRevive( CCFPlayer* pPlayer );
	virtual void StopRevive( );
	virtual void StartFatality( CCFPlayer* pPlayer );
	virtual void StopFatality(bool bKill = false);
	virtual void Decapitate();

	virtual void SetPhysicalMode( bool bPhysical = true ) { SetWeaponMode(bPhysical); };
	virtual void SetMagicMode( bool bMagic = true ) { SetWeaponMode(!bMagic); };
	virtual void SetWeaponMode( bool bPhysical );
	virtual bool IsPhysicalMode();
	virtual bool IsMagicMode() { return !IsPhysicalMode(); };
	virtual bool IsReloading( void ) const;

	virtual bool HasObjective() const { return m_hObjective.Get() != NULL; };
	virtual class CInfoObjective* GetObjective() { return m_hObjective.Get(); };
	virtual void SetObjective(class CInfoObjective* pObjective);

	// System for crawling players organically. Mark a player after he's been touched by an algorithm so he's not processed recursively or infinitely.
	static void MarkAllPlayers(bool bMarked);
	virtual void Mark() { m_bMark = true; };
	virtual bool IsMarked() { return m_bMark; };

	virtual bool GetInVehicle( IServerVehicle *pVehicle, int nRole );

	virtual void RemoveAllItems( bool removeSuit );

	virtual void PlayerDeathThink(void);
	virtual void Event_Revived( );
	virtual void Event_Killed( const CTakeDamageInfo &info );
	virtual void Event_Knockout( const CTakeDamageInfo &info ) { Event_Killed(info, true); };
	virtual void Event_Killed( const CTakeDamageInfo &info, bool bKnockout );
	virtual void Event_Dying( );
	virtual bool IsKnockedOut();
	virtual void CommitSuicide( bool bExplode = false, bool bForce = false );
	virtual bool BecomeRagdoll( const CTakeDamageInfo &info, const Vector &forceVector ) { return false; };	// We don't use this version, and it interferes with fatalities.
	virtual void FadeRagdoll();

	virtual bool IsFuse() const { return m_bIsFuse; };
	virtual bool IsBecomingFuse() const { return m_bBecomingFuse; };
	virtual bool IsPariah() const { return m_bIsPariah; };
	virtual bool IsBecomingPariah() const { return m_bBecomingPariah; };
	virtual void BecomeNormal();

	virtual bool IsCaptain() const { return m_bIsCaptain; };
	virtual bool IsSergeant() const { return m_bIsSergeant; };

private:
	virtual void BecomeFuse();
	virtual void DemoteFuse();
	virtual void BecomePariah(class CWeaponPariahBlade* pBlade);
	virtual void DemotePariah();

	virtual void PromoteToCaptain();
	virtual void PromoteToSergeant();
	virtual void DemoteRank();

public:
	virtual float GetEloProbability( CCFPlayer* pOpponent );
	virtual float GetEloProbability( int iTeam );
	virtual void AwardEloPoints( int iScoreType, CCFPlayer* pOpponent = NULL );
	virtual float GetEloScore() { return m_flEloScore; };

	virtual void LeaveVehicle( const Vector &vecExitPoint, const QAngle &vecExitAngles );
	virtual void UpdateOnRemove( void );
	virtual int  ShouldTransmit( const CCheckTransmitInfo *pInfo );
	virtual void SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways );
	virtual int  ObjectCaps( void );

	virtual void FollowMode();
	virtual void FindTarget(bool bAvoidTeammates = false, bool bNotBehind = false, bool bBackwards = false);
	virtual CCFPlayer* FindClosestEnemy();
	virtual void SetDirectTarget(CCFPlayer* pTarget);
	virtual void SetRecursedTarget(CCFPlayer* pTarget, bool bSignal = false);
	virtual void SetRecursedTargetInternal(CCFPlayer* pTarget, bool bSignal = false);
	virtual void SuggestRecursedTarget(CCFPlayer* pTarget);
	virtual void SuggestRecursedTargetInternal(CCFPlayer* pTarget);
	virtual void RemoveTargeters(bool bKeepSameTeam = false);
	virtual bool TargetersCanSee(CCFPlayer* pTarget);
	virtual bool TargetersCanSeeInternal(CCFPlayer* pTarget);
	virtual CCFPlayer* GetDirectTarget() { return m_hDirectTarget; };
	virtual CCFPlayer* GetRecursedTarget() { return m_hRecursedTarget; };
	virtual bool CanBeTargeted();
	virtual bool IsInFollowMode();
	virtual bool IsAimingIn();
	virtual bool ShouldLockFollowModeView();
	virtual bool CanFollowMode();
	virtual bool ToggleFollowMode();
	virtual bool AutoFollowMode();
	virtual void StartAutoFollowMode();

	virtual bool CanDownStrike(bool bAllowNoFollowMode = false);

	virtual bool FVisible(CBaseEntity* pEntity, int iTraceMask = MASK_OPAQUE, CBaseEntity** ppBlocker = NULL);
	virtual bool IsVisible(const Vector &pos, bool testFOV = false, const CBaseEntity *ignore = NULL) const;	///< return true if we can see the point
	virtual bool IsVisible(CCFPlayer* pPlayer, bool testFOV = false, unsigned char* visParts = NULL) const;
	virtual Vector GetPartPosition(CCFPlayer* player, VisiblePartType part) const;	///< return world space position of given part on player
	virtual void ComputePartPositions(CCFPlayer *player);					///< compute part positions from bone location
	virtual Vector GetCentroid() const;
	virtual CCFPlayer* FindClosestFriend(float flMaxDistance, bool bFOV = true);

	virtual void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );
	virtual int		CFTakeHealth( float flHealth, int bitsDamageType, bool bNofity = true );
	virtual int		TakeHealth( float flHealth, int bitsDamageType );

	virtual void	ShowDefense( CTakeDamageInfo* pDmg, float flScale );

	virtual void	SetLastAttacker( CCFPlayer* hLastAttacker );
	virtual CCFPlayer*	GetLastAttacker( void ) { return m_hLastAttacker; };

	virtual void	SetLastDOTAttacker( CCFPlayer* hLastDOTAttacker );
	virtual CCFPlayer*	GetLastDOTAttacker( void ) { return m_hLastDOTAttacker; };

	virtual int		OnTakeDamage_Dying( const CTakeDamageInfo &info );

	// CF doesn't want the explosion ringing sound
	virtual void	OnDamagedByExplosion( const CTakeDamageInfo &info ) { return; }

	virtual CWeaponCFBase* GetActiveCFWeapon() const;
	virtual CBaseViewModel* GetViewModel( int index = 0 );
	virtual void	CreateViewModel( int viewmodelindex = 0 );
	virtual void	DestroyViewModels( void ) {};

	virtual class CArmament* GetActiveArmament();
	virtual CBaseCombatWeapon*	GetActiveWeapon() const;
	virtual CWeaponCFBase*	GetWeapon(int i) const;
	virtual void	SetWeapon(int i, CWeaponCFBase* pWeapon);
	virtual CWeaponCFBase*	GetPrimaryWeapon(bool bFallback = true) const;
	virtual CWeaponCFBase*	GetSecondaryWeapon(bool bFallback = true) const;
	virtual CWeaponCFBase*	GetInactiveSecondaryWeapon(int* pIndex = NULL) const;
	virtual int		GetActiveSecondary() { return m_iActiveSecondary; };
	virtual bool	ShouldPromoteSecondary() const;
	virtual int		SecondaryToPromote(bool bOther = false) const;
	virtual bool	ShouldUseAlternateWeaponSet() const;
	virtual void	SetNextAttack(float flNextAttack);

	virtual bool	HasDualMelee() const;
	virtual bool	HasAFirearm() const;
	virtual bool	HasAMelee() const;
	virtual bool	HasAMagicCombo() const;

	virtual bool	IsCharging();
	virtual void	StopMeleeAttack();
	virtual bool	IsBlocking();
	virtual CWeaponCFBase*	GetBlockingWeapon();
	virtual CWeaponCFBase*	GetDefendingWeapon();

	virtual bool	CanBlockBulletsWithWeapon(CWeaponCFBase* pWeapon);

	virtual bool	BumpWeapon( CBaseCombatWeapon *pWeapon );
	virtual void	Weapon_Equip( CBaseCombatWeapon *pWeapon );
	virtual void	Weapon_Equip( CWeaponCFBase *pWeapon, int iSlot );
	virtual void	Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget = NULL, const Vector *pVelocity = NULL );
	virtual	bool	Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex = 0 );
	virtual	void	Weapon_SwitchSecondaries();
	virtual void	Weapon_CalcHands();

	virtual Vector	EyePosition( );
	virtual void	CalcPlayerView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );

	virtual void	CheatImpulseCommands( int iImpulse );

	virtual bool	StartObserverMode(int mode); // true, if successful

	virtual void	PackDeadPlayerItems( void );

	virtual char*	GetCFModelName();

	virtual class CCFTeam*	GetCFTeam();

	virtual void	FreezePlayer(float flAmount = 0, float flTime = -1);
	virtual bool	PlayerFrozen(float flPadFrozenTime = 0);
	virtual int		AdjustFrozenButtons(int iButtons);
	virtual void	FreezeRotation(float flAmount = 0, float flTime = -1);
	virtual void	EnemyFrozen(CCFPlayer* pPlayer, float flTime);
	virtual CCFPlayer*	GetEnemyFrozen();
	virtual float	GetEnemyFrozenUntil();
	virtual void	CalculateMovementSpeed();

	virtual void	CalculateHandMagic();

	virtual void	SuspendGravity(float flTime, float flAt = 0);
	virtual bool	IsGravitySuspended() const;
	virtual float	GetGravity( void ) const;

	virtual void 	ModifyOrAppendPlayerCriteria( AI_CriteriaSet& set );

	class CCFAreaCapture*	GetControlPointStandingOn( void );

	virtual void	StartGroundContact( CBaseEntity *ground );
	virtual void	EndGroundContact( CBaseEntity *ground );

	virtual void	UpdateStepSound( surfacedata_t *psurface, const Vector &vecOrigin, const Vector &vecVelocity  );
	virtual void	PlayStepSound( Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force );
	virtual void	DeathSound( const CTakeDamageInfo &info );

	virtual void SetMenuOpen( bool bIsOpen );
	virtual bool IsMenuOpen( void );
	virtual void	ShowCFPanel( CFPanel_e ePanel, bool bShow, bool bCloseAfter = false );

	virtual void	SendPlayerInfo( CCFPlayer* pPlayer );

	virtual bool	RunMimicCommand( CUserCmd& cmd );

	virtual void	Instructor_LessonLearned(int iLesson, bool bForceSend = false);

	virtual void	CameraCinematic(float flSceneLength);

	int				m_iDesiredSkin;

	CNetworkHandle( CBaseEntity, m_hCameraCinematic );
	CNetworkVar( float,	m_flCameraCinematicUntil);

	CNetworkVar( float, m_flFreezeUntil );
	CNetworkVar( float, m_flFreezeAmount );

	CNetworkVar( float, m_flFreezeRotation );
	CNetworkVar( float, m_flFreezeRAmount );
	CNetworkQAngle( m_angFreeze );

	CNetworkVar( float, m_flEnemyFrozenUntil );
	CNetworkHandle( CCFPlayer, m_hEnemyFrozen );

	CNetworkVar( float, m_flSuspendGravityAt );
	CNetworkVar( float, m_flSuspendGravityUntil );

	CNetworkVar( float,	m_flEloScore);

	CNetworkVar( bool,	m_bOverdrive);

	CNetworkHandle( CCFPlayer, m_hReviver ); // Player is currently being revived (or fatality-d) by this player.
	CNetworkHandle( CCFPlayer, m_hReviving ); // Player is currently reviving (or fatality-ing) this player.
	CNetworkVar( bool,	m_bReviving); // True if reviving, false if fatality-ing. (if neither, m_hReviving is NULL.)
	CNetworkVar( float,	m_flNextRespawn );
	float				m_flRespawnBonus;
	CNetworkVar( float,	m_flFatalityStart);
	float				m_flNextRevive;
	float				m_flSpawnTime;
	CNetworkVar( bool,	m_bIsDecapitated);

	CNetworkHandle( CCFPlayer, m_hLastAttacker );
	CNetworkVar( float,		m_flLastAttackedTime );
	CNetworkHandle( CCFPlayer, m_hLastDOTAttacker );

	CNetworkVar( float,	m_flStrongAttackJumpTime );
	CNetworkVar( bool,	m_bStrongAttackJump );

	CNetworkVar( bool,	m_bBecomingFuse);
	CNetworkVar( bool,	m_bIsFuse);
	CNetworkVar( float,	m_flFuseStartTime );
	CNetworkVar( bool,	m_bBecomingPariah);
	CNetworkVar( bool,	m_bIsPariah);
	CNetworkVar( float,	m_flPariahStartTime );

	CNetworkVar( bool,	m_bIsCaptain);
	CNetworkVar( bool,	m_bIsSergeant);

	CNetworkVar( float,	m_flLastDashTime );
	CNetworkVar( bool,	m_bWantVelocityMatched );

	CNetworkVar( int,	m_iAirMeleeAttacks );
	CNetworkVar( int,	m_iMeleeChain );

	CNetworkVar( float,	m_flRushDistance );
	CNetworkHandle( CCFBaseMeleeWeapon, m_hRushingWeapon );
	CNetworkVar(bool,	m_bDownStrike);

	CNetworkVar( bool,	m_bLatched );
	CNetworkVar( int,	m_iLatchTriggerCount );
	CNetworkVar( float,	m_flLastLatch );
	CNetworkVector(		m_vecLatchPlaneNormal );
	EHANDLE				m_hLatchEntity;
	bool				m_bLatchSoundPlaying;

	CNetworkVar( bool, m_bCanPowerjump );
	CNetworkVar( bool, m_bPowerjump );
	CNetworkVar( bool, m_bChargejump );

	CNetworkVar( int,	m_iLastMovementButton );
	CNetworkVar( float,	m_flLastMovementButtonTime );

	CNetworkVar( element_t, m_eLHEffectElements );
	CNetworkVar( element_t, m_eRHEffectElements );
	CNetworkVar( int,	m_iLastCombo );

	bool				m_bIsMenuOpen;

	CNetworkVar( bool, m_bInFollowMode );
	bool				m_bOldInFollowMode;
	CUtlVector< CHandle<CCFPlayer> > m_hTargeters;
	float				m_flFollowModeStarted;
	float				m_flAutoFollowModeEnds;
	CHandle<CCFPlayer>	m_hFollowModeTarget;

	// Target that the player selected himself.
	CNetworkHandle( CCFPlayer, m_hDirectTarget );
	// Target's target's target's target's target.
	CNetworkHandle( CCFPlayer, m_hRecursedTarget );

	// For preventing infinite recursion in targeting.
	bool				m_bMark;
	bool				m_bAutoTarget;
	float				m_flLastSeenDirectTarget;
	float				m_flLastSeenRecursedTarget;

	CNetworkHandle(CInfoObjective, m_hObjective);

	CNetworkVar( bool, m_bPhysicalMode );

	CNetworkQAngle( m_angEyeAngles );	// Copied from EyeAngles() so we can send it to the client.
	CNetworkVar( int, m_iDirection );	// The current lateral kicking direction; 1 = right,  0 = left
	CNetworkVar( int, m_iShotsFired );	// number of shots fired recently

	float				m_flLastLandDustSpawn;

	// Tracks our ragdoll entity.
	CNetworkHandle( CBaseEntity, m_hRagdoll );	// networked entity handle 

	CHintSystem		m_Hints;

	CNetworkArray( CWeaponCFBaseHandle, m_hWeapons, 3 );
	CNetworkArray( CWeaponCFBaseHandle, m_hAlternateWeapons, 3 );

	CNetworkVar( int, m_iActiveSecondary );

	class CArmament*		m_pArmament;
	class CArmament*		m_pCurrentArmament;

	CUniformRandomStream	m_Randomness;	// What comes out of female mouths.

// In shared code.
public:
	
	void FireBullets( 
		const Vector &vOrigin,
		const QAngle &vAngles,
		CFWeaponID	iWeapon,
		int iSeed,
		float flSpread );

	void FireBullet( 
		Vector vecSrc, 
		const QAngle &shootAngles, 
		CFWeaponID iWeapon,
		float vecSpread,
		int iDamage, 
		int iBulletType,
		element_t eElements,
		statuseffect_t eStatusEffects,
		float flMagnitude,
		CBaseEntity *pevAttacker,
		float x,
		float y );

	void FireDrainer( 
		const Vector &vOrigin,
		const QAngle &vAngles,
		CFWeaponID	iWeapon,
		int iSeed,
		float flSpread );

	void ImpactEffects(
		trace_t& tr,
		int iDamageType,
		Vector vecStart,
		Vector vecDir,
		CBaseEntity* pIgnore,
		float flRange
		);

	void Recoil(float flPermanentRecoil, float flTemporaryRecoil);

private:

	void CreateRagdollEntity();

	CCFPlayerAnimState *m_PlayerAnimState;

protected:
	static PartInfo m_partInfo[ MAX_PLAYERS ];						///< part positions for each player
};


inline CCFPlayer *ToCFPlayer( CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

#ifdef _DEBUG
	Assert( dynamic_cast<CCFPlayer*>( pEntity ) != 0 );
#endif
	return static_cast< CCFPlayer* >( pEntity );
}


#endif	// CF_PLAYER_H
