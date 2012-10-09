//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef CF_PLAYERANIMSTATE_H
#define CF_PLAYERANIMSTATE_H
#ifdef _WIN32
#pragma once
#endif


#include "convar.h"
#include "iplayeranimstate.h"
#include "multiplayer_animstate.h"

#ifdef CLIENT_DLL
	class C_BaseAnimatingOverlay;
	class C_WeaponCFBase;
	class C_CFPlayer;
	#define CBaseAnimatingOverlay C_BaseAnimatingOverlay
	#define CWeaponCFBase C_WeaponCFBase
	#define CCFPlayer C_CFPlayer
#else
	class CBaseAnimatingOverlay;
	class CWeaponCFBase; 
	class CCFPlayer;
#endif

typedef enum
{
	LD_LEFT,
	LD_RIGHT,
	LD_BACK,
} latchdir_t;

// When moving this fast, he plays run anim.
#define ARBITRARY_RUN_SPEED		175.0f

inline bool IsServerSendableAnimEvent( PlayerAnimEvent_t iEvent, int iData )
{
	return ( iEvent == PLAYERANIMEVENT_ATTACK && iData == ACT_INVALID) || ( iEvent == PLAYERANIMEVENT_FLINCH_CHEST )
		 || ( iEvent == PLAYERANIMEVENT_DIE ) || ( iEvent == PLAYERANIMEVENT_EXECUTE ) || ( iEvent == PLAYERANIMEVENT_EXECUTED )
		 || ( iEvent == PLAYERANIMEVENT_BLOCK ) || ( iEvent == PLAYERANIMEVENT_BLOCKED );
}

struct CFPlayerPoseData_t
{
	int			m_iHeadYaw;
	int			m_iHeadPitch;

	float		m_flHeadYaw;
	float		m_flHeadPitch;

	void Init()
	{
		m_iHeadYaw = 0;
		m_iHeadPitch = 0;
		m_flHeadYaw = 0;
		m_flHeadPitch = 0;
	}
};

// ------------------------------------------------------------------------------------------------ //
// CCFPlayerAnimState declaration.
// ------------------------------------------------------------------------------------------------ //

class CCFPlayerAnimState : public CMultiPlayerAnimState
{
public:

	DECLARE_CLASS( CCFPlayerAnimState, CMultiPlayerAnimState );

	CCFPlayerAnimState();
	CCFPlayerAnimState( CBasePlayer *pPlayer, MultiPlayerMovementData_t &movementData );


	void InitCF( CCFPlayer* pPlayer );
	CCFPlayer *GetCFPlayer( void )							{ return m_pCFPlayer; }

	virtual void		ClearAnimationState();
	virtual Activity	TranslateActivity( Activity actDesired );
	virtual Activity	CalcMainActivity();	
	virtual void		RestartMainSequence();
	virtual int			SelectWeightedSequence( Activity activity );
	virtual void		ComputeSequences( CStudioHdr *pStudioHdr );

	virtual void		DoAnimationEvent( PlayerAnimEvent_t event, int mData = 0, bool bSecondary = false );

	virtual void		FullBodyAnimation( Activity iFullBody, bool bResetGestures = true, bool bAvoidRepeat = false );
	virtual bool		IsInFullBodyAnimation() { return m_iFullBodyAnimation != ACT_INVALID; };

	virtual bool		HandleKnockout( Activity &idealActivity );
	virtual bool		HandleFullBodyAnimation( Activity &idealActivity );
	virtual bool		HandleRushing( Activity &idealActivity );
	virtual bool		HandleDFA( Activity &idealActivity );
	virtual bool		HandleJumping( Activity &idealActivity );
	virtual bool		HandleSwimming( Activity &idealActivity );
	virtual bool		HandleLatching( Activity &idealActivity );

	virtual latchdir_t	FindInitialLatchingDirection();
	virtual latchdir_t	FindLatchingDirection();

	virtual bool		SetupPoseParameters( CStudioHdr *pStudioHdr );
	virtual void		ComputePoseParam_MoveYaw( CStudioHdr *pStudioHdr );
	virtual void		ComputePoseParam_AimPitch( CStudioHdr *pStudioHdr );
	virtual void		ComputePoseParam_AimYaw( CStudioHdr *pStudioHdr );
	virtual void		ComputePoseParam_HeadPitch( CStudioHdr *pStudioHdr );
	virtual void		ComputePoseParam_HeadYaw( CStudioHdr *pStudioHdr );
	virtual bool		ShouldLookAtTarget( void );
	virtual void		EstimateYaw( void );
	virtual void		ConvergeYawAngles( float flGoalYaw, float flYawRate, float flDeltaTime, float &flCurrentYaw );
	virtual void		OverrideEyeYaw( float flYaw ) { m_flEyeYaw = flYaw; }
	virtual float		GetEyeYaw() const { return m_flEyeYaw; }

	GestureSlot_t*		GetGestureSlot(int i) { return &m_aGestureSlots[i]; };
	void				RestartGesture( int iGestureSlot, Activity iGestureActivity, bool bAutoKill = true );

protected:
	bool				m_bFacingForward;

	// Latching.
	bool				m_bLatching;
	bool				m_bLatchJumping;
	latchdir_t			m_eLatchDirection;

	bool				m_bRushing;
	int					m_iRush;

	Activity			m_iFullBodyAnimation;
	int					m_iPredictedFullBodySequence;
	float				m_flFullBodyYaw;

	Vector				m_vecLastAirVelocity;

private:
	CCFPlayer*			m_pCFPlayer;
	CFPlayerPoseData_t	m_CFParameterData;
};


CCFPlayerAnimState* CreatePlayerAnimState( CCFPlayer *pEntity );

// If this is set, then the game code needs to make sure to send player animation events
// to the local player if he's the one being watched.
extern ConVar cl_showanimstate;


#endif // CF_PLAYERANIMSTATE_H
