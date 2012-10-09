//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef C_CF_PLAYER_H
#define C_CF_PLAYER_H
#ifdef _WIN32
#pragma once
#endif


#include "cf_playeranimstate.h"
#include "c_baseplayer.h"
#include "cf_shareddefs.h"
#include "baseparticleentity.h"
#include "weapon_cfbase.h"
#include "weapon_cfbasemelee.h"
#include "cf_instructor.h"
#include "c_objectives.h"

#define RECOIL_DURATION 0.1

class C_CFPlayer : public C_BasePlayer
{
public:
	DECLARE_CLASS( C_CFPlayer, C_BasePlayer );
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();
	DECLARE_DATADESC();

	C_CFPlayer();
	~C_CFPlayer();

	virtual void		PreThink( void );
	virtual void		PostThink( void );
	virtual void		ClientThink( void );

	virtual void		ItemPreFrame( void );
	virtual void		ItemPostFrame( void );

	virtual void		Spawn();
	virtual void		SharedSpawn();

	virtual bool		ShouldCollide( int collisionGroup, int contentsMask ) const;
	virtual void		Touch( CBaseEntity *pOther );
	virtual void		SharedTouch( CBaseEntity *pOther );

	virtual void		FollowMode();
	virtual void		CalcFollowModeView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );
	C_CFPlayer*			GetDirectTarget();
	C_CFPlayer*			GetRecursedTarget();
	virtual bool		IsInFollowMode();
	virtual bool		IsAimingIn();
	virtual bool		ShouldLockFollowModeView();
	virtual bool		AutoFollowMode();
	virtual Vector		CalcFollowModeCameraTargets();

	virtual bool		CanDownStrike(bool bAllowNoFollowMode = false);

	virtual void		StopPowerjump();
	virtual void		StartLatch(const trace_t &tr, CBaseEntity* pOther);
	virtual void		StopLatch();
	virtual void		LatchThink();
	virtual bool		IsLatchable() { return false; };

	static C_CFPlayer*	GetLocalCFPlayer();

	virtual const QAngle& GetRenderAngles();
	virtual void UpdateClientSideAnimation();
	virtual void PostDataUpdate( DataUpdateType_t updateType );
	virtual void OnDataChanged( DataUpdateType_t updateType );

	ShadowType_t	ShadowCastType( void );
	void			GetShadowRenderBounds( Vector &mins, Vector &maxs, ShadowType_t shadowType );

	virtual bool	CreateMove( float flInputSampleTime, CUserCmd *pCmd );

	virtual const QAngle& EyeAngles();
	virtual Vector	EyePosition( );
	virtual void	CalcView( Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov );
	virtual void	CalcPlayerView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );
	virtual void	CalcInEyeCamView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov);
	virtual void	CalcChaseCamView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );

	virtual void	ThirdPersonSwitch( bool bThirdperson );
	virtual bool	ShouldForceThirdPerson();

// Called by shared code.
public:
	
	virtual void	DoAnimationEvent( PlayerAnimEvent_t event, int mData = 0, bool bSecondary = false );
	virtual void	SetAnimation( PLAYER_ANIM playerAnim );
	virtual bool	IsInFullBodyAnimation();
	virtual bool	ShouldUseForwardFacingAnimationStyle();

	virtual void	FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options );

	virtual void	Rush();
	virtual void	Rush(CCFBaseMeleeWeapon* pWeapon, float flDistance, int iAttack);
	virtual void	EndRush();

	// Shit that C_BaseAnimating SHOULD have.
	virtual bool	GetIntervalMovement( float flIntervalUsed, bool &bMoveSeqFinished, Vector &newPosition, QAngle &newAngles );

	virtual bool	ShouldDraw();
	virtual int		DrawModel( int flags );
	virtual bool	OnInternalDrawModel( ClientModelRenderInfo_t *pInfo );
	virtual bool	OnPostInternalDrawModel( ClientModelRenderInfo_t *pInfo );
	virtual void	ValidateModelIndex( void );
	virtual char*	GetCFModelName();
	virtual void	DrawTargets();
	static void		DrawTarget(const char* pszMaterial, Vector vecOrigin, float flYaw, float flElapsed = 0, CViewSetup* pPushView = NULL);

	virtual void	ShowDefense(bool bPhysical, element_t eElements, Vector vecDamageOrigin, float flScale);

	virtual bool	IsUseableEntity( CBaseEntity *pEntity, unsigned int requiredCaps );
	virtual int		ObjectCaps( void );

	virtual Vector	GetCentroid() const;

	virtual void	CreateViewModel( int viewmodelindex = 0 );
	virtual CBaseViewModel* GetViewModel( int index = 0 );

	virtual class CArmament* GetActiveArmament();
	virtual CBaseCombatWeapon* GetActiveWeapon() const;
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

	bool			IsPhysicalMode();
	bool			IsMagicMode() { return !IsPhysicalMode(); };

	virtual bool	HasObjective() const { return m_hObjective.Get() != NULL; };
	virtual class C_InfoObjective* GetObjective() { return m_hObjective.Get(); };

	virtual	bool	Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex = 0 );
	virtual	void	Weapon_SwitchSecondaries();

	KeyValues*		GetSequenceKeyValues( int iSequence );

	virtual void	FreezePlayer(float flAmount = 0, float flTime = -1);
	virtual bool	PlayerFrozen(float flPadFrozenTime = 0);
	virtual int		AdjustFrozenButtons(int iButtons);
	virtual void	FreezeRotation(float flAmount = 0, float flTime = -1);
	virtual void	EnemyFrozen(CCFPlayer* pPlayer, float flTime);
	virtual float	GetEnemyFrozenUntil();
	virtual C_CFPlayer*	GetEnemyFrozen();
	virtual void	CalculateMovementSpeed();

	virtual void	CalculateHandMagic();
	static void		ShowHandMagic(C_BaseEntity* pEnt, CUtlVector<CNewParticleEffect*>& aHandComboEffects, element_t eElements, const char* pszAttachment);

	virtual void	SuspendGravity(float flTime, float flAt = 0);
	virtual bool	IsGravitySuspended() const;
	virtual float	GetGravity( void ) const;

	virtual void	StartGroundContact( CBaseEntity *ground );
	virtual void	EndGroundContact( CBaseEntity *ground );

	virtual void	UpdateStepSound( surfacedata_t *psurface, const Vector &vecOrigin, const Vector &vecVelocity  );
	virtual void	PlayStepSound( Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force );

	virtual bool	IsKnockedOut();

	virtual bool	IsFuse() const { return m_bIsFuse; };
	virtual bool	IsBecomingFuse() const { return m_bBecomingFuse; };
	virtual bool	IsPariah() const { return m_bIsPariah; };
	virtual bool	IsBecomingPariah() const { return m_bBecomingPariah; };

	virtual bool	IsCaptain() const { return m_bIsCaptain; };
	virtual bool	IsSergeant() const { return m_bIsSergeant; };

	virtual bool	IsFirstPersonSpectating( C_BaseEntity* pTarget );

	virtual void	Instructor_Initialize();
	virtual bool	Instructor_Initialized() { return m_apLessons.Count() > 0; };
	virtual void	Instructor_Think();
	virtual void	Instructor_Respawn();
	virtual void	Instructor_InvalidateLessons();

	virtual void	Instructor_LessonLearned(int iLesson);
	virtual bool	Instructor_IsLessonLearned(int iLesson);
	virtual bool	Instructor_IsLessonValid(int iLesson);
	virtual CCFGameLesson*	Instructor_GetBestLessonOfType(int iLessonMask);

	CUtlMap<int, CCFGameLesson*>	m_apLessons;
	CUtlSortVector<CCFGameLesson*, LessonPriorityLess>	m_apLessonPriorities;
	float			m_flNextLessonSearchTime;
	float			m_flLastLesson;

	float			m_flLastEnemySeen;

	CCFPlayerAnimState *m_PlayerAnimState;

	QAngle	m_angEyeAngles;
	CInterpolatedVar< QAngle >	m_iv_angEyeAngles;

	CNetworkVar( int, m_iDirection );	// The current lateral kicking direction; 1 = right,  0 = left
	CNetworkVar( int, m_iShotsFired );	// number of shots fired recently

	float			m_flLastLandDustSpawn;

	EHANDLE	m_hRagdoll;

	float			m_flLastRespawn;
	float			m_flLastAnimFrameTime;

	CHandle< CBaseEntity > m_hCameraCinematic;
	CNetworkVar( float, m_flCameraCinematicUntil );

	CNetworkVar( float, m_flFreezeUntil );
	CNetworkVar( float, m_flFreezeAmount );

	CNetworkVar( float, m_flFreezeRotation );
	CNetworkVar( float, m_flFreezeRAmount );
	QAngle			m_angFreeze;

	CNetworkVar( float, m_flEnemyFrozenUntil );
	CHandle< C_CFPlayer > m_hEnemyFrozen;

	CNetworkVar( float, m_flSuspendGravityAt );
	CNetworkVar( float, m_flSuspendGravityUntil );

	CNetworkVar( bool, m_bInFollowMode );
	bool			m_bOldInFollowMode;
	float			m_flFollowModeStarted;

	bool			m_bOverdrive;

	CHandle< C_CFPlayer >	m_hReviver; // Player is currently being revived (or fatality-d) by this player.
	CHandle< C_CFPlayer >	m_hReviving; // Player is currently reviving (or fatality-ing) this player.
	bool					m_bReviving; // True if reviving, false if fatality-ing. (if neither, m_hReviving is NULL.)
	float					m_flNextRespawn;
	CNetworkVar( float,		m_flFatalityStart);
	float					m_flSpawnTime;
	bool					m_bIsDecapitated;

	CHandle< C_CFPlayer >	m_hLastAttacker;
	CNetworkVar( float,		m_flLastAttackedTime);
	CHandle< C_CFPlayer >	m_hLastDOTAttacker;

	CNetworkVar( float,		m_flStrongAttackJumpTime);
	CNetworkVar( bool,		m_bStrongAttackJump);

	bool					m_bShieldPhysical;
	float					m_flShieldTime;
	float					m_flShieldStrength;
	Vector					m_vecShieldDmgOrigin;

	bool					m_bBecomingFuse;
	bool					m_bIsFuse;
	float					m_flFuseStartTime;
	bool					m_bBecomingPariah;
	bool					m_bIsPariah;
	float					m_flPariahStartTime;

	bool					m_bIsCaptain;
	bool					m_bIsSergeant;

	float					m_flLastDashTime;
	bool					m_bWantVelocityMatched;

	int						m_iAirMeleeAttacks;
	int						m_iMeleeChain;

	float					m_flRushDistance;
	CHandle< C_CFBaseMeleeWeapon >	m_hRushingWeapon;
	CNetworkVar(bool,		m_bDownStrike);

	bool					m_bLatched;
	int						m_iLatchTriggerCount;
	float					m_flLastLatch;
	CNetworkVector(			m_vecLatchPlaneNormal );
	EHANDLE					m_hLatchEntity;
	bool					m_bLatchSoundPlaying;

	bool					m_bCanPowerjump;
	bool					m_bPowerjump;
	bool					m_bChargejump;

	int						m_iLastMovementButton;
	float					m_flLastMovementButtonTime;

	C_BaseAnimatingOverlay* m_pBarrier;

	CNewParticleEffect*		m_pOverdriveEffect;
	CNewParticleEffect*		m_pKnockoutEffect;

	CNewParticleEffect*		m_apStatusEffects[TOTAL_STATUSEFFECTS];

	CUtlVector<CNewParticleEffect*> m_apLatchEffects;

	CUtlVector<CNewParticleEffect*>	m_apLHComboEffects;
	CUtlVector<CNewParticleEffect*>	m_apRHComboEffects;
	element_t				m_eLHEffectElements;
	element_t				m_eRHEffectElements;
	element_t				m_eLHEffectElementsOld;
	element_t				m_eRHEffectElementsOld;
	int						m_iLastCombo;

	CNewParticleEffect*		m_pLHChargeEffect;
	CNewParticleEffect*		m_pRHChargeEffect;

	CHandle< C_CFPlayer >	m_hDirectTarget;
	CHandle< C_CFPlayer >	m_hRecursedTarget;

	CHandle< C_CFPlayer >	m_hLastCameraTarget;
	Vector					m_vecLastCameraTarget;
	Vector					m_vecLastTargetPosition;
	float					m_flLastCameraTargetTime;
	bool					m_bCameraTargetSwitchBetweenTwoPlayers;

	CHandle< C_InfoObjective >	m_hObjective;

	CHandle< C_CFPlayer >	m_hDrawingDirectTarget;
	CHandle< C_CFPlayer >	m_hDrawingRecursedTarget;
	float					m_flReceivedDirectTarget;
	float					m_flReceivedRecursedTarget;

	CHintSystem		m_Hints;

	class CArmament*	m_pArmament;
	class CArmament*	m_pCurrentArmament;

	CHandle<C_WeaponCFBase>		m_hWeapons[3];
	CHandle<C_WeaponCFBase>		m_hAlternateWeapons[3];

	bool				m_bPhysicalMode;

	int					m_iActiveSecondary;

	float				m_flRecoilTimeRemaining;
	float				m_flPitchRecoilAccumulator;
	float				m_flYawRecoilAccumulator;

	// Only used by spectators at the moment.
	bool				m_bThirdPositionMelee;
	float				m_flThirdPositionMeleeWeight;

	CWeaponCFBase *GetActiveCFWeapon() const;

	C_BaseAnimating *BecomeRagdollOnClient();
	IRagdoll* C_CFPlayer::GetRepresentativeRagdoll() const;

	void FireBullets( 
		const Vector &vOrigin,
		const QAngle &vAngles,
		CFWeaponID iWeaponID,
		int iSeed,
		float flSpread );

	void FireBullet( 
		Vector vecSrc, 
		const QAngle &shootAngles, 
		CFWeaponID iWeaponID,
		float vecSpread, 
		int iDamage, 
		int iBulletType,
		element_t eElements,
		statuseffect_t eStatusEffects,
		float flStatusEffectsMagnitude,
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
	void SetRecoilAmount( float flPitchRecoil, float flYawRecoil );
	void GetRecoilToAddThisFrame( float &flPitchRecoil, float &flYawRecoil );

private:
	C_CFPlayer( const C_CFPlayer & );
};


inline C_CFPlayer* ToCFPlayer( CBaseEntity *pPlayer )
{
	Assert( dynamic_cast< C_CFPlayer* >( pPlayer ) != NULL );
	return static_cast< C_CFPlayer* >( pPlayer );
}


#endif // C_CF_PLAYER_H
