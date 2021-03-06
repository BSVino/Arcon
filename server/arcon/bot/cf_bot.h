//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

//
// Author: Michael S. Booth (mike@turtlerockstudios.com), 2003
//
// NOTE: The CF Bot code uses Doxygen-style comments. If you run Doxygen over this code, it will 
// auto-generate documentation.  Visit www.doxygen.org to download the system for free.
//

#ifndef _CF_BOT_H_
#define _CF_BOT_H_

#include "bot/bot.h"
#include "bot/cf_bot_manager.h"
#include "bot/cf_bot_chatter.h"
#include "cf_gamestate.h"
#include "cf_player.h"
#include "weapon_cfbase.h"
#include "nav_pathfind.h"

class CBaseDoor;
class CBasePropDoor;
class CCFBot;
class CPushAwayEnumerator;

//--------------------------------------------------------------------------------------------------------------
/**
 * For use with player->m_rgpPlayerItems[]
 */
enum InventorySlotType
{
	PRIMARY_WEAPON_SLOT = 1,
	PISTOL_SLOT,
	KNIFE_SLOT,
	GRENADE_SLOT,
	C4_SLOT
};


//--------------------------------------------------------------------------------------------------------------
/**
 * The definition of a bot's behavior state.  One or more finite state machines 
 * using these states implement a bot's behaviors.
 */
class BotState
{
public:
	virtual void OnEnter( CCFBot *bot ) { }				///< when state is entered
	virtual void OnUpdate( CCFBot *bot ) { }			///< state behavior
	virtual void OnExit( CCFBot *bot ) { }				///< when state exited
	virtual const char *GetName( void ) const = 0;		///< return state name
};


//--------------------------------------------------------------------------------------------------------------
/**
 * The state is invoked when a bot has nothing to do, or has finished what it was doing.
 * A bot never stays in this state - it is the main action selection mechanism.
 */
class IdleState : public BotState
{
public:
	virtual void OnEnter( CCFBot *bot );
	virtual void OnUpdate( CCFBot *bot );
	virtual const char *GetName( void ) const		{ return "Idle"; }
};


//--------------------------------------------------------------------------------------------------------------
/**
 * When a bot is actively searching for an enemy.
 */
class HuntState : public BotState
{
public:
	virtual void OnEnter( CCFBot *bot );
	virtual void OnUpdate( CCFBot *bot );
	virtual void OnExit( CCFBot *bot );
	virtual const char *GetName( void ) const		{ return "Hunt"; }

	void ClearHuntArea( void )						{ m_huntArea = NULL; }

private:
	CNavArea *m_huntArea;										///< "far away" area we are moving to
};


//--------------------------------------------------------------------------------------------------------------
/**
 * When a bot has an enemy and is attempting to kill it
 */
class AttackState : public BotState
{
public:
	virtual void OnEnter( CCFBot *bot );
	virtual void OnUpdate( CCFBot *bot );
	virtual void OnExit( CCFBot *bot );
	virtual const char *GetName( void ) const		{ return "Attack"; }
	
protected:
	enum DodgeStateType
	{
		STEADY_ON,
		SLIDE_LEFT,
		SLIDE_RIGHT,
		JUMP,

		NUM_ATTACK_STATES
	};
	DodgeStateType m_dodgeState;
	float m_nextDodgeStateTimestamp;

	CountdownTimer m_repathTimer;
	float m_scopeTimestamp;

	bool m_haveSeenEnemy;										///< false if we haven't yet seen the enemy since we started this attack (told by a friend, etc)
	bool m_isEnemyHidden;										///< true we if we have lost line-of-sight to our enemy
	float m_reacquireTimestamp;									///< time when we can fire again, after losing enemy behind cover
	float m_shieldToggleTimestamp;								///< time to toggle shield deploy state
	bool m_shieldForceOpen;										///< if true, open up and shoot even if in danger

	float m_pinnedDownTimestamp;								///< time when we'll consider ourselves "pinned down" by the enemy

	bool m_didAmbushCheck;
	bool m_shouldDodge;
	bool m_firstDodge;

	bool m_isCoward;											///< if true, we'll retreat if outnumbered during this fight
	CountdownTimer m_retreatTimer;

	void StopAttacking( CCFBot *bot );
	void Dodge( CCFBot *bot );									///< do dodge behavior
};


//--------------------------------------------------------------------------------------------------------------
/**
 * When a bot has heard an enemy noise and is moving to find out what it was.
 */
class InvestigateNoiseState : public BotState
{
public:
	virtual void OnEnter( CCFBot *bot );
	virtual void OnUpdate( CCFBot *bot );
	virtual void OnExit( CCFBot *bot );
	virtual const char *GetName( void ) const		{ return "InvestigateNoise"; }

private:
	void AttendCurrentNoise( CCFBot *bot );						///< move towards currently heard noise
	Vector m_checkNoisePosition;								///< the position of the noise we're investigating
	CountdownTimer m_minTimer;									///< minimum time we will investigate our current noise
};


//--------------------------------------------------------------------------------------------------------------
/**
 * When a bot is buying equipment at the start of a round.
 */
class BuyState : public BotState
{
public:
	virtual void OnEnter( CCFBot *bot );
	virtual void OnUpdate( CCFBot *bot );
	virtual void OnExit( CCFBot *bot );
	virtual const char *GetName( void ) const		{ return "Buy"; }

private:
	bool m_isInitialDelay;
	int m_prefRetries;											///< for retrying buying preferred weapon at current index
	int m_prefIndex;											///< where are we in our list of preferred weapons

	int m_retries;
	bool m_doneBuying;
	bool m_buyPistol;
};


//--------------------------------------------------------------------------------------------------------------
/**
 * When a bot is moving to a potentially far away position in the world.
 */
class MoveToState : public BotState
{
public:
	virtual void OnEnter( CCFBot *bot );
	virtual void OnUpdate( CCFBot *bot );
	virtual void OnExit( CCFBot *bot );
	virtual const char *GetName( void ) const		{ return "MoveTo"; }
	void SetGoalPosition( const Vector &pos )		{ m_goalPosition = pos; }
	void SetRouteType( RouteType route )			{ m_routeType = route; }

private:
	Vector m_goalPosition;										///< goal position of move
	RouteType m_routeType;										///< the kind of route to build
	bool m_askedForCover;
};


//--------------------------------------------------------------------------------------------------------------
/**
 * When a Terrorist bot is moving to pick up a dropped bomb.
 */
class FetchBombState : public BotState
{
public:
	virtual void OnEnter( CCFBot *bot );
	virtual void OnUpdate( CCFBot *bot );
	virtual const char *GetName( void ) const	{ return "FetchBomb"; }
};


//--------------------------------------------------------------------------------------------------------------
/**
 * When a Terrorist bot is actually planting the bomb.
 */
class PlantBombState : public BotState
{
public:
	virtual void OnEnter( CCFBot *bot );
	virtual void OnUpdate( CCFBot *bot );
	virtual void OnExit( CCFBot *bot );
	virtual const char *GetName( void ) const	{ return "PlantBomb"; }
};


//--------------------------------------------------------------------------------------------------------------
/**
 * When a CT bot is actually defusing a live bomb.
 */
class DefuseBombState : public BotState
{
public:
	virtual void OnEnter( CCFBot *bot );
	virtual void OnUpdate( CCFBot *bot );
	virtual void OnExit( CCFBot *bot );
	virtual const char *GetName( void ) const	{ return "DefuseBomb"; }
};


//--------------------------------------------------------------------------------------------------------------
/**
 * When a bot is hiding in a corner.
 * NOTE: This state also includes MOVING TO that hiding spot, which may be all the way
 * across the map!
 */
class HideState : public BotState
{
public:
	virtual void OnEnter( CCFBot *bot );
	virtual void OnUpdate( CCFBot *bot );
	virtual void OnExit( CCFBot *bot );
	virtual const char *GetName( void ) const	{ return "Hide"; }

	void SetHidingSpot( const Vector &pos )		{ m_hidingSpot = pos; }
	const Vector &GetHidingSpot( void ) const	{ return m_hidingSpot; }

	void SetSearchArea( CNavArea *area )		{ m_searchFromArea = area; }
	void SetSearchRange( float range )			{ m_range = range; }
	void SetDuration( float time )				{ m_duration = time; }
	void SetHoldPosition( bool hold )			{ m_isHoldingPosition = hold; }

	bool IsAtSpot( void ) const					{ return m_isAtSpot; }

	float GetHideTime( void ) const
	{
		if (IsAtSpot())
		{
			return m_duration - m_hideTimer.GetRemainingTime();
		}

		return 0.0f;
	}

private:
	CNavArea *m_searchFromArea;
	float m_range;

	Vector m_hidingSpot;
	bool m_isLookingOutward;
	bool m_isAtSpot;
	float m_duration;
	CountdownTimer m_hideTimer;								///< how long to hide

	bool m_isHoldingPosition;
	float m_holdPositionTime;								///< how long to hold our position after we hear nearby enemy noise

	bool m_heardEnemy;										///< set to true when we first hear an enemy
	float m_firstHeardEnemyTime;							///< when we first heard the enemy

	int m_retry;											///< counter for retrying hiding spot

	Vector m_leaderAnchorPos;								///< the position of our follow leader when we decided to hide

	bool m_isPaused;										///< if true, we have paused in our retreat for a moment
	CountdownTimer m_pauseTimer;							///< for stoppping and starting our pauses while we retreat
};


//--------------------------------------------------------------------------------------------------------------
/**
 * When a bot is attempting to flee from a bomb that is about to explode.
 */
class EscapeFromBombState : public BotState
{
public:
	virtual void OnEnter( CCFBot *bot );
	virtual void OnUpdate( CCFBot *bot );
	virtual void OnExit( CCFBot *bot );
	virtual const char *GetName( void ) const		{ return "EscapeFromBomb"; }
};


//--------------------------------------------------------------------------------------------------------------
/**
 * When a bot is following another player.
 */
class FollowState : public BotState
{
public:
	virtual void OnEnter( CCFBot *bot );
	virtual void OnUpdate( CCFBot *bot );
	virtual void OnExit( CCFBot *bot );
	virtual const char *GetName( void ) const		{ return "Follow"; }

	void SetLeader( CCFPlayer *player )				{ m_leader = player; }

private:
	CHandle< CCFPlayer > m_leader;								///< the player we are following
	Vector m_lastLeaderPos;										///< where the leader was when we computed our follow path
	bool m_isStopped;
	float m_stoppedTimestamp;

	enum LeaderMotionStateType
	{
		INVALID,
		STOPPED,
		WALKING,
		RUNNING
	};
	LeaderMotionStateType m_leaderMotionState;
	IntervalTimer m_leaderMotionStateTime;

	bool m_isSneaking;
	float m_lastSawLeaderTime;
	CountdownTimer m_repathInterval;

	IntervalTimer m_walkTime;
	bool m_isAtWalkSpeed;

	float m_waitTime;
	CountdownTimer m_idleTimer;

	void ComputeLeaderMotionState( float leaderSpeed );
};


//--------------------------------------------------------------------------------------------------------------
/**
 * When a bot is actually using another entity (ie: facing towards it and pressing the use key)
 */
class UseEntityState : public BotState
{
public:
	virtual void OnEnter( CCFBot *bot );
	virtual void OnUpdate( CCFBot *bot );
	virtual void OnExit( CCFBot *bot );
	virtual const char *GetName( void ) const			{ return "UseEntity"; }

	void SetEntity( CBaseEntity *entity )				{ m_entity = entity; }

private:
	EHANDLE m_entity;											///< the entity we will use
};


//--------------------------------------------------------------------------------------------------------------
/**
 * When a bot is opening a door
 */
class OpenDoorState : public BotState
{
public:
	virtual void OnEnter( CCFBot *bot );
	virtual void OnUpdate( CCFBot *bot );
	virtual void OnExit( CCFBot *bot );
	virtual const char *GetName( void ) const		{ return "OpenDoor"; }

	void SetDoor( CBaseEntity *door );

	bool IsDone( void ) const						{ return m_isDone; }	///< return true if behavior is done

private:
	CHandle< CBaseDoor > m_funcDoor;									///< the func_door we are opening
	CHandle< CBasePropDoor > m_propDoor;								///< the prop_door we are opening
	bool m_isDone;
	CountdownTimer m_timeout;
};


//--------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------
/**
 * The Counter-strike Bot
 */
class CCFBot : public CBot< CCFPlayer >
{
public:
	DECLARE_CLASS( CCFBot, CBot< CCFPlayer > );
	DECLARE_DATADESC();

	CCFBot( void );												///< constructor initializes all values to zero
	virtual ~CCFBot();
	virtual bool Initialize( const BotProfile *profile, int team );		///< (EXTEND) prepare bot for action

	virtual void Spawn( void );									///< (EXTEND) spawn the bot into the game
	virtual void Touch( CBaseEntity *other );					///< (EXTEND) when touched by another entity

	virtual void Upkeep( void );								///< lightweight maintenance, invoked frequently
	virtual void Update( void );								///< heavyweight algorithms, invoked less often
	virtual void BuildUserCmd( CUserCmd& cmd, const QAngle& viewangles, float forwardmove, float sidemove, float upmove, int buttons, byte impulse );
	virtual float GetMoveSpeed( void );							///< returns current movement speed (for walk/run)

	virtual bool ForceFireWeapon( CUserCmd& cmd );

	virtual void Walk( void );
	virtual bool Jump( bool mustJump = false );					///< returns true if jump was started

	virtual bool IsReadyToPlay( void );
	virtual bool IsReadyToSpawn( void );

	//- behavior properties ------------------------------------------------------------------------------------------
	float GetCombatRange( void ) const;
	bool IsRogue( void ) const;									///< return true if we dont listen to teammates or pursue scenario goals
	void SetRogue( bool rogue );
	bool IsHurrying( void ) const;								///< return true if we are in a hurry 
	void Hurry( float duration );								///< force bot to hurry
	bool IsSafe( void ) const;									///< return true if we are in a safe region
	bool IsWellPastSafe( void ) const;							///< return true if it is well past the early, "safe", part of the round
	bool IsEndOfSafeTime( void ) const;							///< return true if we were in the safe time last update, but not now
	float GetSafeTimeRemaining( void ) const;					///< return the amount of "safe time" we have left
	float GetSafeTime( void ) const;							///< return what we think the total "safe time" for this map is
	virtual void Blind( float holdTime, float fadeTime, float startingAlpha = 255 );	// player blinded by a flashbang
	bool IsUnhealthy( void ) const;								///< returns true if bot is low on health
	
	bool IsAlert( void ) const;									///< return true if bot is in heightened "alert" mode
	void BecomeAlert( void );									///< bot becomes "alert" for immediately nearby enemies

	bool IsSneaking( void ) const;								///< return true if bot is sneaking
	void Sneak( float duration );								///< sneak for given duration

	//- behaviors ---------------------------------------------------------------------------------------------------
	void Idle( void );

	void Hide( CNavArea *searchFromArea = NULL, float duration = -1.0f, float hideRange = 750.0f, bool holdPosition = false );	///< DEPRECATED: Use TryToHide() instead
	#define USE_NEAREST true
	bool TryToHide( CNavArea *searchFromArea = NULL, float duration = -1.0f, float hideRange = 750.0f, bool holdPosition = false, bool useNearest = false );	///< try to hide nearby, return false if cannot
	void Hide( const Vector &hidingSpot, float duration = -1.0f, bool holdPosition = false );	///< move to the given hiding place
	bool IsHiding( void ) const;								///< returns true if bot is currently hiding
	bool IsAtHidingSpot( void ) const;							///< return true if we are hiding and at our hiding spot
	float GetHidingTime( void ) const;							///< return number of seconds we have been at our current hiding spot

	bool MoveToInitialEncounter( void );						///< move to a hiding spot and wait for initial encounter with enemy team (return false if no spots are available)

	bool TryToRetreat( float maxRange = 1000.0f, float duration = -1.0f );	///< retreat to a nearby hiding spot, away from enemies

	void Hunt( void );
	bool IsHunting( void ) const;								///< returns true if bot is currently hunting

	void Attack( CCFPlayer *victim );
	void FireWeaponAtEnemy( void );								///< fire our active weapon towards our current enemy
	void StopAttacking( void );
	bool IsAttacking( void ) const;								///< returns true if bot is currently engaging a target

	void MoveTo( const Vector &pos, RouteType route = SAFEST_ROUTE );	///< move to potentially distant position
	bool IsMovingTo( void ) const;								///< return true if we are in the MoveTo state

	void PlantBomb( void );

	void FetchBomb( void );										///< bomb has been dropped - go get it
	bool NoticeLooseBomb( void ) const;							///< return true if we noticed the bomb on the ground or on radar
	bool CanSeeLooseBomb( void ) const;							///< return true if we directly see the loose bomb

	void DefuseBomb( void );
	bool IsDefusingBomb( void ) const;							///< returns true if bot is currently defusing the bomb
	bool CanSeePlantedBomb( void ) const;						///< return true if we directly see the planted bomb

	void EscapeFromBomb( void );
	bool IsEscapingFromBomb( void ) const;						///< return true if we are escaping from the bomb

	void UseEntity( CBaseEntity *entity );						///< use the entity

	void OpenDoor( CBaseEntity *door );							///< open the door (assumes we are right in front of it)
	bool IsOpeningDoor( void ) const;							///< return true if we are in the process of opening a door

	void Buy( void );											///< enter the buy state
	bool IsBuying( void ) const;

	void Panic( void );											///< look around in panic
	bool IsPanicking( void ) const;								///< return true if bot is panicked
	void StopPanicking( void );									///< end our panic
	void UpdatePanicLookAround( void );							///< do panic behavior

	void TryToJoinTeam( int team );								///< try to join the given team

	void Follow( CCFPlayer *player );							///< begin following given Player
	void ContinueFollowing( void );								///< continue following our leader after finishing what we were doing
	void StopFollowing( void );									///< stop following
	bool IsFollowing( void ) const;								///< return true if we are following someone (not necessarily in the follow state)
	CCFPlayer *GetFollowLeader( void ) const;					///< return the leader we are following
	float GetFollowDuration( void ) const;						///< return how long we've been following our leader
	bool CanAutoFollow( void ) const;							///< return true if we can auto-follow

	bool IsNotMoving( float minDuration = 0.0f ) const;			///< return true if we are currently standing still and have been for minDuration

	void AimAtEnemy( void );									///< point our weapon towards our enemy
	void StopAiming( void );									///< stop aiming at enemy
	bool IsAimingAtEnemy( void ) const;							///< returns true if we are trying to aim at an enemy

	float GetStateTimestamp( void ) const;						///< get time current state was entered

	bool IsDoingScenario( void ) const;							///< return true if we will do scenario-related tasks

	//- scenario / gamestate -----------------------------------------------------------------------------------------
	CFGameState *GetGameState( void );							///< return an interface to this bot's gamestate
	const CFGameState *GetGameState( void ) const;				///< return an interface to this bot's gamestate

	bool IsAtBombsite( void );									///< return true if we are in a bomb planting zone
	bool GuardRandomZone( float range = 500.0f );				///< pick a random zone and hide near it

	bool IsBusy( void ) const;									///< return true if we are busy doing something important

	//- high-level tasks ---------------------------------------------------------------------------------------------
	enum TaskType
	{
		SEEK_AND_DESTROY,
		PLANT_BOMB,
		FIND_TICKING_BOMB,
		DEFUSE_BOMB,
		GUARD_TICKING_BOMB,
		GUARD_BOMB_DEFUSER,
		GUARD_LOOSE_BOMB,
		GUARD_BOMB_ZONE,
		GUARD_INITIAL_ENCOUNTER,
		ESCAPE_FROM_BOMB,
		HOLD_POSITION,
		FOLLOW,
		VIP_ESCAPE,
		GUARD_VIP_ESCAPE_ZONE,
		MOVE_TO_LAST_KNOWN_ENEMY_POSITION,
		MOVE_TO_SNIPER_SPOT,
		SNIPING,

		NUM_TASKS
	};
	void SetTask( TaskType task, CBaseEntity *entity = NULL );	///< set our current "task"
	TaskType GetTask( void ) const;
	CBaseEntity *GetTaskEntity( void );
	const char *GetTaskName( void ) const;						///< return string describing current task

	//- behavior modifiers ------------------------------------------------------------------------------------------
	enum DispositionType
	{
		ENGAGE_AND_INVESTIGATE,								///< engage enemies on sight and investigate enemy noises
		OPPORTUNITY_FIRE,									///< engage enemies on sight, but only look towards enemy noises, dont investigate
		SELF_DEFENSE,										///< only engage if fired on, or very close to enemy
		IGNORE_ENEMIES,										///< ignore all enemies - useful for ducking around corners, running away, etc

		NUM_DISPOSITIONS
	};
	void SetDisposition( DispositionType disposition );		///< define how we react to enemies
	DispositionType GetDisposition( void ) const;
	const char *GetDispositionName( void ) const;			///< return string describing current disposition

	void IgnoreEnemies( float duration );					///< ignore enemies for a short duration

	enum MoraleType
	{
		TERRIBLE = -3,
		BAD = -2,
		NEGATIVE = -1,
		NEUTRAL = 0,
		POSITIVE = 1,
		GOOD = 2,
		EXCELLENT = 3,
	};
	MoraleType GetMorale( void ) const;
	const char *GetMoraleName( void ) const;				///< return string describing current morale
	void IncreaseMorale( void );
	void DecreaseMorale( void );

	void Surprise( float duration );						///< become "surprised" - can't attack
	bool IsSurprised( void ) const;							///< return true if we are "surprised"


	//- listening for noises ----------------------------------------------------------------------------------------
	bool IsNoiseHeard( void ) const;							///< return true if we have heard a noise
	bool HeardInterestingNoise( void );							///< return true if we heard an enemy noise worth checking in to
	void InvestigateNoise( void );								///< investigate recent enemy noise
	bool IsInvestigatingNoise( void ) const;					///< return true if we are investigating a noise
	const Vector *GetNoisePosition( void ) const;				///< return position of last heard noise, or NULL if none heard
	CNavArea *GetNoiseArea( void ) const;						///< return area where noise was heard
	void ForgetNoise( void );									///< clear the last heard noise
	bool CanSeeNoisePosition( void ) const;						///< return true if we directly see where we think the noise came from
	float GetNoiseRange( void ) const;							///< return approximate distance to last noise heard

	bool CanHearNearbyEnemyGunfire( float range = -1.0f ) const;///< return true if we hear nearby threatening enemy gunfire within given range (-1 == infinite)
	PriorityType GetNoisePriority( void ) const;				///< return priority of last heard noise

	//- radio and chatter--------------------------------------------------------------------------------------------
	void SpeakAudio( const char *voiceFilename, float duration, int pitch );	///< send voice chatter
	BotChatterInterface *GetChatter( void );					///< return an interface to this bot's chatter system
	bool RespondToHelpRequest( CCFPlayer *player, Place place, float maxRange = -1.0f );	///< decide if we should move to help the player, return true if we will
	bool IsUsingVoice() const;									///< new-style "voice" chatter gets voice feedback


	//- enemies ------------------------------------------------------------------------------------------------------
	// BOTPORT: GetEnemy() collides with GetEnemy() in CBaseEntity - need to use different nomenclature
	void SetBotEnemy( CCFPlayer *enemy );						///< set given player as our current enemy
	CCFPlayer *GetBotEnemy( void ) const;
	int GetNearbyEnemyCount( void ) const;						///< return max number of nearby enemies we've seen recently
	unsigned int GetEnemyPlace( void ) const;					///< return location where we see the majority of our enemies
	bool CanSeeBomber( void ) const;							///< return true if we can see the bomb carrier
	CCFPlayer *GetBomber( void ) const;

	int GetNearbyFriendCount( void ) const;						///< return number of nearby teammates
	CCFPlayer *GetClosestVisibleFriend( void ) const;			///< return the closest friend that we can see
	CCFPlayer *GetClosestVisibleHumanFriend( void ) const;		///< return the closest human friend that we can see
	CCFPlayer *CCFBot::GetClosestVisibleKO( void ) const;

	bool IsOutnumbered( void ) const;							///< return true if we are outnumbered by enemies
	int OutnumberedCount( void ) const;							///< return number of enemies we are outnumbered by

	#define ONLY_VISIBLE_ENEMIES true
	CCFPlayer *GetImportantEnemy( bool checkVisibility = false ) const;	///< return the closest "important" enemy for the given scenario (bomb carrier, VIP, hostage escorter)

	void UpdateReactionQueue( void );							///< update our reaction time queue
	CCFPlayer *GetRecognizedEnemy( void );						///< return the most dangerous threat we are "conscious" of
	bool IsRecognizedEnemyReloading( void );					///< return true if the enemy we are "conscious" of is reloading
	bool IsRecognizedEnemyProtectedByShield( void );			///< return true if the enemy we are "conscious" of is hiding behind a shield
	float GetRangeToNearestRecognizedEnemy( void );				///< return distance to closest enemy we are "conscious" of

	CCFPlayer *GetAttacker( void ) const;						///< return last enemy that hurt us
	float GetTimeSinceAttacked( void ) const;					///< return duration since we were last injured by an attacker
	float GetFirstSawEnemyTimestamp( void ) const;				///< time since we saw any enemies
	float GetLastSawEnemyTimestamp( void ) const;
	float GetTimeSinceLastSawEnemy( void ) const;
	float GetTimeSinceAcquiredCurrentEnemy( void ) const;
	bool HasNotSeenEnemyForLongTime( void ) const;				///< return true if we haven't seen an enemy for "a long time"
	const Vector &GetLastKnownEnemyPosition( void ) const;
	bool IsEnemyVisible( void ) const;							///< is our current enemy visible
	float GetEnemyDeathTimestamp( void ) const;
	bool IsFriendInLineOfFire( void );							///< return true if a friend is in our weapon's way
	bool IsAwareOfEnemyDeath( void ) const;						///< return true if we *noticed* that our enemy died
	int GetLastVictimID( void ) const;							///< return the ID (entindex) of the last victim we killed, or zero

	bool CanSeeSniper( void ) const;							///< return true if we can see an enemy sniper
	bool HasSeenSniperRecently( void ) const;					///< return true if we have seen a sniper recently

	float GetTravelDistanceToPlayer( CCFPlayer *player ) const;	///< return shortest path travel distance to this player	
	bool DidPlayerJustFireWeapon( const CCFPlayer *player ) const;	///< return true if the given player just fired their weapon

	//- navigation --------------------------------------------------------------------------------------------------
	bool HasPath( void ) const;
	void DestroyPath( void );

	float GetFeetZ( void ) const;								///< return Z of bottom of feet

	enum PathResult
	{
		PROGRESSING,		///< we are moving along the path
		END_OF_PATH,		///< we reached the end of the path
		PATH_FAILURE		///< we failed to reach the end of the path
	};
	#define NO_SPEED_CHANGE false
	PathResult UpdatePathMovement( bool allowSpeedChange = true );	///< move along our computed path - if allowSpeedChange is true, bot will walk when near goal to ensure accuracy

	//bool AStarSearch( CNavArea *startArea, CNavArea *goalArea );	///< find shortest path from startArea to goalArea - don't actually buid the path
	bool ComputePath( const Vector &goal, RouteType route = SAFEST_ROUTE );	///< compute path to goal position
	bool StayOnNavMesh( void );
	CNavArea *GetLastKnownArea( void ) const;						///< return the last area we know we were inside of
	const Vector &GetPathEndpoint( void ) const;					///< return final position of our current path
	float GetPathDistanceRemaining( void ) const;					///< return estimated distance left to travel along path
	void ResetStuckMonitor( void );
	bool IsAreaVisible( const CNavArea *area ) const;				///< is any portion of the area visible to this bot
	const Vector &GetPathPosition( int index ) const;
	bool GetSimpleGroundHeightWithFloor( const Vector &pos, float *height, Vector *normal = NULL );	///< find "simple" ground height, treating current nav area as part of the floor
	void BreakablesCheck( void );
	void DoorCheck( void );											///< Check for any doors along our path that need opening

	virtual void PushawayTouch( CBaseEntity *pOther );

	Place GetPlace( void ) const;									///< get our current radio chatter place

	bool IsUsingLadder( void ) const;								///< returns true if we are in the process of negotiating a ladder
	void GetOffLadder( void );										///< immediately jump off of our ladder, if we're on one

	void SetGoalEntity( CBaseEntity *entity );
	CBaseEntity *GetGoalEntity( void );

	bool IsNearJump( void ) const;									///< return true if nearing a jump in the path
	float GetApproximateFallDamage( float height ) const;			///< return how much damage will will take from the given fall height

	void ForceRun( float duration );								///< force the bot to run if it moves for the given duration
	virtual bool IsRunning( void ) const;

	void Wait( float duration );									///< wait where we are for the given duration
	bool IsWaiting( void ) const;									///< return true if we are waiting
	void StopWaiting( void );										///< stop waiting

	void Wiggle( void );											///< random movement, for getting un-stuck

	bool IsFriendInTheWay( const Vector &goalPos );					///< return true if a friend is between us and the given position
	void FeelerReflexAdjustment( Vector *goalPosition );			///< do reflex avoidance movements if our "feelers" are touched

	bool HasVisitedEnemySpawn( void ) const;						///< return true if we have visited enemy spawn at least once
	bool IsAtEnemySpawn( void ) const;								///< return true if we are at the/an enemy spawn right now

	//- looking around ----------------------------------------------------------------------------------------------

	// BOTPORT: EVIL VILE HACK - why is EyePosition() not const?!?!?
	const Vector &EyePositionConst( void ) const;
	
	void SetLookAngles( float yaw, float pitch );					///< set our desired look angles
	void UpdateLookAngles( void );									///< move actual view angles towards desired ones
	void UpdateLookAround( bool updateNow = false );				///< update "looking around" mechanism
	void InhibitLookAround( float duration );						///< block all "look at" and "looking around" behavior for given duration - just look ahead

	/// @todo Clean up notion of "forward angle" and "look ahead angle"
	void SetForwardAngle( float angle );							///< define our forward facing
	void SetLookAheadAngle( float angle );							///< define default look ahead angle

	/// look at the given point in space for the given duration (-1 means forever)
	void SetLookAt( const char *desc, const Vector &pos, PriorityType pri, float duration = -1.0f, bool clearIfClose = false, float angleTolerance = 5.0f, bool attack = false );
	void ClearLookAt( void );										///< stop looking at a point in space and just look ahead
	bool IsLookingAtSpot( PriorityType pri = PRIORITY_LOW ) const;	///< return true if we are looking at spot with equal or higher priority
	bool IsViewMoving( float angleVelThreshold = 1.0f ) const;		///< returns true if bot's view angles are rotating (not still)
	bool HasViewBeenSteady( float duration ) const;					///< how long has our view been "steady" (ie: not moving) for given duration

	bool HasLookAtTarget( void ) const;								///< return true if we are in the process of looking at a target

	bool IsNoticable( const CCFPlayer *player, unsigned char visibleParts ) const;	///< return true if we "notice" given player 

	bool IsEnemyPartVisible( VisiblePartType part ) const;			///< if enemy is visible, return the part we see for our current enemy

	float ComputeWeaponSightRange( void );							///< return line-of-sight distance to obstacle along weapon fire ray

	bool IsAnyVisibleEnemyLookingAtMe( bool testFOV = false ) const;///< return true if any enemy I have LOS to is looking directly at me

	bool IsSignificantlyCloser( const CCFPlayer *testPlayer, const CCFPlayer *referencePlayer ) const;	///< return true if testPlayer is significantly closer than referencePlayer

	//- approach points ---------------------------------------------------------------------------------------------
	void ComputeApproachPoints( void );								///< determine the set of "approach points" representing where the enemy can enter this region
	void UpdateApproachPoints( void );								///< recompute the approach point set if we have moved far enough to invalidate the current ones
	void ClearApproachPoints( void );
	void DrawApproachPoints( void ) const;							///< for debugging
	float GetHidingSpotCheckTimestamp( HidingSpot *spot ) const;	///< return time when given spot was last checked
	void SetHidingSpotCheckTimestamp( HidingSpot *spot );			///< set the timestamp of the given spot to now

	const CNavArea *GetInitialEncounterArea( void ) const;			///< return area where we think we will first meet the enemy
	void SetInitialEncounterArea( const CNavArea *area );

	//- weapon query and equip --------------------------------------------------------------------------------------
	#define MUST_EQUIP true
	void EquipBestWeapon( bool mustEquip = false );					///< equip the best weapon we are carrying that has ammo
	void EquipSecondary( void );									///< equip our pistol

	#define DONT_USE_SMOKE_GRENADE true
	bool EquipGrenade( bool noSmoke = false );						///< equip a grenade, return false if we cant

	bool IsUsingPrimaryMelee( void ) const;							///< returns true if we have knife equipped
	bool IsUsingGrenade( void ) const;								///< returns true if we have grenade equipped
	bool IsUsingSniperRifle( void ) const;							///< returns true if using a "sniper" rifle
	bool IsUsing( CFWeaponID weapon ) const;						///< returns true if using the specific weapon
	bool IsSniper( void ) const;									///< return true if we have a sniper rifle in our inventory
	bool IsSniping( void ) const;									///< return true if we are actively sniping (moving to sniper spot or settled in)
	bool IsUsingShotgun( void ) const;								///< returns true if using a shotgun
	bool IsUsingMachinegun( void ) const;							///< returns true if using the big 'ol machinegun
	bool HasBackupFirearm( void ) const;
	void ThrowGrenade( const Vector &target );						///< begin the process of throwing the grenade
	bool IsThrowingGrenade( void ) const;							///< return true if we are in the process of throwing a grenade
	bool HasGrenade( void ) const;									///< return true if we have a grenade in our inventory
	void AvoidEnemyGrenades( void );								///< react to enemy grenades we see
	bool IsAvoidingGrenade( void ) const;							///< return true if we are in the act of avoiding a grenade
	CWeaponCFBase *GetActiveCFWeapon( void ) const;					///< get our current Counter-Strike weapon

	void GiveWeapon( const char *weaponAlias );						///< Debug command to give a named weapon

	virtual void PrimaryAttack( void );								///< presses the fire button, unless we're holding a pistol that can't fire yet (so we can just always call PrimaryAttack())

	enum ZoomType { NO_ZOOM, LOW_ZOOM, HIGH_ZOOM };
	ZoomType GetZoomLevel( void );									///< return the current zoom level of our weapon

	bool AdjustZoom( float range );									///< change our zoom level to be appropriate for the given range
	bool IsWaitingForZoom( void ) const;							///< return true if we are reacquiring after our zoom

	bool IsPrimaryWeaponEmpty( void ) const;						///< return true if primary weapon doesn't exist or is totally out of ammo
	bool IsSecondaryWeaponEmpty( void ) const;						///< return true if secondary weapon doesn't exist or is totally out of ammo

	//------------------------------------------------------------------------------------
	// Event hooks
	//

	/// invoked when injured by something (EXTEND) - returns the amount of damage inflicted
	virtual int OnTakeDamage( const CTakeDamageInfo &info );

	/// invoked when killed (EXTEND)
	virtual void Event_Killed( const CTakeDamageInfo &info );

	virtual bool BumpWeapon( CBaseCombatWeapon *pWeapon );		///< invoked when in contact with a CWeaponBox


	/// invoked when event occurs in the game (some events have NULL entity)
	void OnPlayerFootstep( IGameEvent *event );
	void OnPlayerRadio( IGameEvent *event );
	void OnPlayerDeath( IGameEvent *event );
	void OnPlayerFallDamage( IGameEvent *event );

	void OnBombPickedUp( IGameEvent *event );
	void OnBombPlanted( IGameEvent *event );
	void OnBombBeep( IGameEvent *event );
	void OnBombDefuseBegin( IGameEvent *event );
	void OnBombDefused( IGameEvent *event );
	void OnBombDefuseAbort( IGameEvent *event );
	void OnBombExploded( IGameEvent *event );

	void OnRoundEnd( IGameEvent *event );
	void OnRoundStart( IGameEvent *event );

	void OnDoorMoving( IGameEvent *event );

	void OnBreakProp( IGameEvent *event );
	void OnBreakBreakable( IGameEvent *event );

	void OnWeaponFire( IGameEvent *event );
	void OnWeaponFireOnEmpty( IGameEvent *event );
	void OnWeaponReload( IGameEvent *event );
	void OnWeaponZoom( IGameEvent *event );

	void OnBulletImpact( IGameEvent *event );

	void OnHEGrenadeDetonate( IGameEvent *event );
	void OnFlashbangDetonate( IGameEvent *event );
	void OnSmokeGrenadeDetonate( IGameEvent *event );
	void OnGrenadeBounce( IGameEvent *event );

	void OnNavBlocked( IGameEvent *event );

	void OnEnteredNavArea( CNavArea *newArea );						///< invoked when bot enters a nav area

private:
	#define IS_FOOTSTEP true
	void OnAudibleEvent( IGameEvent *event, CBasePlayer *player, float range, PriorityType priority, bool isHostile, bool isFootstep = false, const Vector *actualOrigin = NULL );	///< Checks if the bot can hear the event

private:
	friend class CCFBotManager;

	/// @todo Get rid of these
	friend class AttackState;
	friend class BuyState;

	// BOTPORT: Remove this vile hack
	Vector m_eyePosition;

	void ResetValues( void );										///< reset internal data to initial state
	void BotDeathThink( void );

	char m_name[64];												///< copied from STRING(pev->netname) for debugging
	void DebugDisplay( void ) const;								///< render bot debug info

	//- behavior properties ------------------------------------------------------------------------------------------
	float m_combatRange;											///< desired distance between us and them during gunplay
	mutable bool m_isRogue;											///< if true, the bot is a "rogue" and listens to no-one
	mutable CountdownTimer m_rogueTimer;
	MoraleType m_morale;											///< our current morale, based on our win/loss history
	bool m_diedLastRound;											///< true if we died last round
	float m_safeTime;												///< duration at the beginning of the round where we feel "safe"
	bool m_wasSafe;													///< true if we were in the safe time last update
	void AdjustSafeTime( void );									///< called when enemy seen to adjust safe time for this round
	NavRelativeDirType m_blindMoveDir;								///< which way to move when we're blind
	bool m_blindFire;												///< if true, fire weapon while blinded
	CountdownTimer m_surpriseTimer;									///< when we were surprised

	bool m_isFollowing;												///< true if we are following someone
	CHandle< CCFPlayer > m_leader;									///< the ID of who we are following
	float m_followTimestamp;										///< when we started following
	float m_allowAutoFollowTime;									///< time when we can auto follow

	CountdownTimer m_hurryTimer;									///< if valid, bot is in a hurry
	CountdownTimer m_alertTimer;									///< if valid, bot is alert
	CountdownTimer m_sneakTimer;									///< if valid, bot is sneaking
	CountdownTimer m_panicTimer;									///< if valid, bot is panicking


	// instances of each possible behavior state, to avoid dynamic memory allocation during runtime
	IdleState				m_idleState;
	HuntState				m_huntState;
	AttackState				m_attackState;
	InvestigateNoiseState	m_investigateNoiseState;
	BuyState				m_buyState;
	MoveToState				m_moveToState;
	FetchBombState			m_fetchBombState;
	PlantBombState			m_plantBombState;
	DefuseBombState			m_defuseBombState;
	HideState				m_hideState;
	EscapeFromBombState		m_escapeFromBombState;
	FollowState				m_followState;
	UseEntityState			m_useEntityState;
	OpenDoorState			m_openDoorState;

	/// @todo Allow multiple simultaneous state machines (look around, etc)	
	void SetState( BotState *state );								///< set the current behavior state
	BotState *m_state;												///< current behavior state
	float m_stateTimestamp;											///< time state was entered
	bool m_isAttacking;												///< if true, special Attack state is overriding the state machine
	bool m_isOpeningDoor;											///< if true, special OpenDoor state is overriding the state machine

	TaskType m_task;												///< our current task
	EHANDLE m_taskEntity;											///< an entity used for our task

	//- navigation ---------------------------------------------------------------------------------------------------
	Vector m_goalPosition;
	EHANDLE m_goalEntity;
	void MoveTowardsPosition( const Vector &pos );					///< move towards position, independant of view angle
	void MoveAwayFromPosition( const Vector &pos );					///< move away from position, independant of view angle
	void StrafeAwayFromPosition( const Vector &pos );				///< strafe (sidestep) away from position, independant of view angle
	void StuckCheck( void );										///< check if we have become stuck
	CNavArea *m_currentArea;										///< the nav area we are standing on
	CNavArea *m_lastKnownArea;										///< the last area we were in
	EHANDLE m_avoid;												///< higher priority player we need to make way for
	float m_avoidTimestamp;
	bool m_isStopping;												///< true if we're trying to stop because we entered a 'stop' nav area
	bool m_hasVisitedEnemySpawn;									///< true if we have been at the enemy spawn
	IntervalTimer m_stillTimer;										///< how long we have been not moving

	//- path navigation data ----------------------------------------------------------------------------------------
	enum { MAX_PATH_LENGTH = 256 };
	struct ConnectInfo
	{
		CNavArea *area;												///< the area along the path
		NavTraverseType how;										///< how to enter this area from the previous one
		Vector pos;													///< our movement goal position at this point in the path
		const CNavLadder *ladder;									///< if "how" refers to a ladder, this is it
	}
	m_path[ MAX_PATH_LENGTH ];
	int m_pathLength;
	int m_pathIndex;												///< index of next area on path
	float m_areaEnteredTimestamp;
	void BuildTrivialPath( const Vector &goal );					///< build trivial path to goal, assuming we are already in the same area

	CountdownTimer m_repathTimer;									///< must have elapsed before bot can pathfind again

	bool ComputePathPositions( void );								///< determine actual path positions bot will move between along the path
	void SetupLadderMovement( void );
	void SetPathIndex( int index );									///< set the current index along the path
	void DrawPath( void );
	int FindOurPositionOnPath( Vector *close, bool local = false ) const;	///< compute the closest point to our current position on our path
	int FindPathPoint( float aheadRange, Vector *point, int *prevIndex = NULL );	///< compute a point a fixed distance ahead along our path.
	bool FindClosestPointOnPath( const Vector &pos, int startIndex, int endIndex, Vector *close ) const;	///< compute closest point on path to given point
	bool IsStraightLinePathWalkable( const Vector &goal ) const;	///< test for un-jumpable height change, or unrecoverable fall
	void ComputeLadderAngles( float *yaw, float *pitch );			///< computes ideal yaw/pitch for traversing the current ladder on our path

	mutable CountdownTimer m_avoidFriendTimer;						///< used to throttle how often we check for friends in our path
	mutable bool m_isFriendInTheWay;								///< true if a friend is blocking our path
	CountdownTimer m_politeTimer;									///< we'll wait for friend to move until this runs out
	bool m_isWaitingBehindFriend;									///< true if we are waiting for a friend to move

	#define ONLY_JUMP_DOWN true
	bool DiscontinuityJump( float ground, bool onlyJumpDown = false, bool mustJump = false ); ///< check if we need to jump due to height change

	enum LadderNavState
	{
		APPROACH_ASCENDING_LADDER,									///< prepare to scale a ladder
		APPROACH_DESCENDING_LADDER,									///< prepare to go down ladder 
		FACE_ASCENDING_LADDER,
		FACE_DESCENDING_LADDER,
		MOUNT_ASCENDING_LADDER,										///< move toward ladder until "on" it
		MOUNT_DESCENDING_LADDER,									///< move toward ladder until "on" it
		ASCEND_LADDER,												///< go up the ladder
		DESCEND_LADDER,												///< go down the ladder
		DISMOUNT_ASCENDING_LADDER,									///< get off of the ladder
		DISMOUNT_DESCENDING_LADDER,									///< get off of the ladder
		MOVE_TO_DESTINATION,										///< dismount ladder and move to destination area
	}
	m_pathLadderState;
	bool m_pathLadderFaceIn;										///< if true, face towards ladder, otherwise face away
	const CNavLadder *m_pathLadder;									///< the ladder we need to use to reach the next area
	bool UpdateLadderMovement( void );								///< called by UpdatePathMovement()
	NavRelativeDirType m_pathLadderDismountDir;						///< which way to dismount
	float m_pathLadderDismountTimestamp;							///< time when dismount started
	float m_pathLadderEnd;											///< if ascending, z of top, if descending z of bottom
	void ComputeLadderEndpoint( bool ascending );
	float m_pathLadderTimestamp;									///< time when we started using ladder - for timeout check

	CountdownTimer m_mustRunTimer;									///< if nonzero, bot cannot walk
	CountdownTimer m_waitTimer;										///< if nonzero, we are waiting where we are

	void UpdateTravelDistanceToAllPlayers( void );					///< periodically compute shortest path distance to each player
	CountdownTimer m_updateTravelDistanceTimer;						///< for throttling travel distance computations
	float m_playerTravelDistance[ MAX_PLAYERS ];					///< current distance from this bot to each player
	unsigned char m_travelDistancePhase;							///< a counter for optimizing when to compute travel distance

	//- game scenario mechanisms -------------------------------------------------------------------------------------
	CFGameState m_gameState;										///< our current knowledge about the state of the scenario

	int m_desiredTeam;												///< the team we want to be on
	bool m_hasJoined;												///< true if bot has actually joined the game

	//- listening mechanism ------------------------------------------------------------------------------------------
	Vector m_noisePosition;											///< position we last heard non-friendly noise
	float m_noiseTravelDistance;									///< the travel distance to the noise
	float m_noiseTimestamp;											///< when we heard it (can get zeroed)
	CNavArea *m_noiseArea;											///< the nav area containing the noise
	PriorityType m_noisePriority;									///< priority of currently heard noise
	bool UpdateLookAtNoise( void );									///< return true if we decided to look towards the most recent noise source
	CountdownTimer m_noiseBendTimer;								///< for throttling how often we bend our line of sight to the noise location
	Vector m_bentNoisePosition;										///< the last computed bent line of sight
	bool m_bendNoisePositionValid;

	//- "looking around" mechanism -----------------------------------------------------------------------------------
	float m_lookAroundStateTimestamp;								///< time of next state change
	float m_lookAheadAngle;											///< our desired forward look angle
	float m_forwardAngle;											///< our current forward facing direction
	float m_inhibitLookAroundTimestamp;								///< time when we can look around again

	enum LookAtSpotState
	{
		NOT_LOOKING_AT_SPOT,			///< not currently looking at a point in space
		LOOK_TOWARDS_SPOT,				///< in the process of aiming at m_lookAtSpot
		LOOK_AT_SPOT,					///< looking at m_lookAtSpot
		NUM_LOOK_AT_SPOT_STATES
	}
	m_lookAtSpotState;
	Vector m_lookAtSpot;											///< the spot we're currently looking at
	PriorityType m_lookAtSpotPriority;
	float m_lookAtSpotDuration;										///< how long we need to look at the spot
	float m_lookAtSpotTimestamp;									///< when we actually began looking at the spot
	float m_lookAtSpotAngleTolerance;								///< how exactly we must look at the spot
	bool m_lookAtSpotClearIfClose;									///< if true, the look at spot is cleared if it gets close to us
	bool m_lookAtSpotAttack;										///< if true, the look at spot should be attacked
	const char *m_lookAtDesc;										///< for debugging
	void UpdateLookAt( void );
	void UpdatePeripheralVision();									///< update enounter spot timestamps, etc
	float m_peripheralTimestamp;

	enum { MAX_APPROACH_POINTS = 16 };
	struct ApproachPoint
	{
		Vector m_pos;
		CNavArea *m_area;
	};

	ApproachPoint m_approachPoint[ MAX_APPROACH_POINTS ];
	unsigned char m_approachPointCount;
	Vector m_approachPointViewPosition;								///< the position used when computing current approachPoint set

	CBaseEntity * FindEntitiesOnPath( float distance, CPushAwayEnumerator *enumerator, bool checkStuck );

	IntervalTimer m_viewSteadyTimer;								///< how long has our view been "steady" (ie: not moving)

	bool BendLineOfSight( const Vector &eye, const Vector &target, Vector *bend, float angleLimit = 135.0f ) const;		///< "bend" our line of sight until we can see the target point. Return bend point, false if cant bend.
	bool FindApproachPointNearestPath( Vector *pos );				///< find the approach point that is nearest to our current path, ahead of us
	enum GrenadeTossState
	{
		NOT_THROWING,				///< not yet throwing
		START_THROW,				///< lining up throw
		THROW_LINED_UP,				///< pause for a moment when on-line
		FINISH_THROW,				///< throwing
	};
	GrenadeTossState m_grenadeTossState;
	CountdownTimer m_tossGrenadeTimer;								///< timeout timer for grenade tossing
	const CNavArea *m_initialEncounterArea;							///< area where we think we will initially encounter the enemy
	void LookForGrenadeTargets( void );								///< look for grenade throw targets and throw our grenade at them
	void UpdateGrenadeThrow( void );								///< process grenade throwing
	CountdownTimer m_isAvoidingGrenade;								///< if nonzero we are in the act of avoiding a grenade


	SpotEncounter *m_spotEncounter;									///< the spots we will encounter as we move thru our current area
	float m_spotCheckTimestamp;										///< when to check next encounter spot

	/// @todo Add timestamp for each possible client to hiding spots
	enum { MAX_CHECKED_SPOTS = 64 };
	struct HidingSpotCheckInfo
	{
		HidingSpot *spot;
		float timestamp;
	}
	m_checkedHidingSpot[ MAX_CHECKED_SPOTS ];
	int m_checkedHidingSpotCount;

	//- view angle mechanism -----------------------------------------------------------------------------------------
	float m_lookPitch;												///< our desired look pitch angle
	float m_lookPitchVel;
	float m_lookYaw;												///< our desired look yaw angle
	float m_lookYawVel;

	//- aim angle mechanism -----------------------------------------------------------------------------------------
	Vector m_aimOffset;												///< current error added to victim's position to get actual aim spot
	Vector m_aimOffsetGoal;											///< desired aim offset
	float m_aimOffsetTimestamp;										///< time of next offset adjustment
	float m_aimSpreadTimestamp;										///< time used to determine max spread as it begins to tighten up
	void SetAimOffset( float accuracy );							///< set the current aim offset
	void UpdateAimOffset( void );									///< wiggle aim error based on m_accuracy
	Vector m_aimSpot;												///< the spot we are currently aiming to fire at

	//- attack state data --------------------------------------------------------------------------------------------
	DispositionType m_disposition;									///< how we will react to enemies
	CountdownTimer m_ignoreEnemiesTimer;							///< how long will we ignore enemies
	mutable CHandle< CCFPlayer > m_enemy;							///< our current enemy
	bool m_isEnemyVisible;											///< result of last visibility test on enemy
	unsigned char m_visibleEnemyParts;								///< which parts of the visible enemy do we see
	Vector m_lastEnemyPosition;										///< last place we saw the enemy
	float m_lastSawEnemyTimestamp;
	float m_firstSawEnemyTimestamp;
	float m_currentEnemyAcquireTimestamp;
	float m_enemyDeathTimestamp;									///< if m_enemy is dead, this is when he died
	float m_friendDeathTimestamp;									///< time since we saw a friend die
	bool m_isLastEnemyDead;											///< true if we killed or saw our last enemy die
	int m_nearbyEnemyCount;											///< max number of enemies we've seen recently
	unsigned int m_enemyPlace;										///< the location where we saw most of our enemies

	struct WatchInfo
	{
		float timestamp;											///< time we last saw this player, zero if never seen
		bool isEnemy;
	}
	m_watchInfo[ MAX_PLAYERS ];
	mutable CHandle< CCFPlayer > m_bomber;							///< points to bomber if we can see him

	int m_nearbyFriendCount;										///< number of nearby teammates
	mutable CHandle< CCFPlayer > m_closestVisibleFriend;			///< the closest friend we can see
	mutable CHandle< CCFPlayer > m_closestVisibleHumanFriend;		///< the closest human friend we can see
	mutable CHandle< CCFPlayer > m_closestVisibleKO;				///< the closest KO'd player we can see

	IntervalTimer m_attentionInterval;								///< time between attention checks

	CCFPlayer *m_attacker;											///< last enemy that hurt us (may not be same as m_enemy)
	float m_attackedTimestamp;										///< when we were hurt by the m_attacker

	int m_lastVictimID;												///< the entindex of the last victim we killed, or zero
	bool m_isAimingAtEnemy;											///< if true, we are trying to aim at our enemy
	bool m_isRapidFiring;											///< if true, RunUpkeep() will toggle our primary attack as fast as it can
	IntervalTimer m_equipTimer;										///< how long have we had our current weapon equipped
	CountdownTimer m_zoomTimer;										///< for delaying firing immediately after zoom
	bool DoEquip( CWeaponCFBase *gun );								///< equip the given item

	void ReloadCheck( void );										///< reload our weapon if we must
	void SilencerCheck( void );										///< use silencer

	float m_fireWeaponTimestamp;

	bool m_isEnemySniperVisible;									///< do we see an enemy sniper right now
	CountdownTimer m_sawEnemySniperTimer;							///< tracking time since saw enemy sniper
	
	//- reaction time system -----------------------------------------------------------------------------------------
	enum { MAX_ENEMY_QUEUE = 20 };
	struct ReactionState
	{
		// NOTE: player position & orientation is not currently stored separately
		CHandle<CCFPlayer> player;
		bool isReloading;
		bool isProtectedByShield;
	}
	m_enemyQueue[ MAX_ENEMY_QUEUE ];								///< round-robin queue for simulating reaction times
	byte m_enemyQueueIndex;
	byte m_enemyQueueCount;
	byte m_enemyQueueAttendIndex;									///< index of the timeframe we are "conscious" of

	CCFPlayer *FindMostDangerousThreat( void );						///< return most dangerous threat in my field of view (feeds into reaction time queue)


	//- stuck detection ---------------------------------------------------------------------------------------------
	bool m_isStuck;
	float m_stuckTimestamp;											///< time when we got stuck
	Vector m_stuckSpot;												///< the location where we became stuck
	NavRelativeDirType m_wiggleDirection;
	CountdownTimer m_wiggleTimer;
	CountdownTimer m_stuckJumpTimer;								///< time for next jump when stuck

	enum { MAX_VEL_SAMPLES = 10 };	
	float m_avgVel[ MAX_VEL_SAMPLES ];
	int m_avgVelIndex;
	int m_avgVelCount;
	Vector m_lastOrigin;

	/// new-style "voice" chatter gets voice feedback
	float m_voiceEndTimestamp;

	BotChatterInterface m_chatter;									///< chatter mechanism
};


//
// Inlines
//

inline float CCFBot::GetFeetZ( void ) const
{
	return GetAbsOrigin().z;
}

inline const Vector *CCFBot::GetNoisePosition( void ) const
{
	if (m_noiseTimestamp > 0.0f)
		return &m_noisePosition;

	return NULL;
}

inline bool CCFBot::IsAwareOfEnemyDeath( void ) const
{
	if (GetEnemyDeathTimestamp() == 0.0f)
		return false;

	if (m_enemy == NULL)
		return true;

	if (!m_enemy->IsAlive() && gpGlobals->curtime - GetEnemyDeathTimestamp() > (1.0f - 0.8f * GetProfile()->GetSkill()))
		return true;

	return false;
}

inline void CCFBot::Panic( void )
{
	// we are stunned for a moment
	Surprise( RandomFloat( 0.2f, 0.3f ) );

	const float panicTime = 3.0f;
	m_panicTimer.Start( panicTime );

	const float panicRetreatRange = 300.0f;
	TryToRetreat( panicRetreatRange, 0.0f );

	PrintIfWatched( "*** PANIC ***\n" );
}

inline bool CCFBot::IsPanicking( void ) const
{
	return !m_panicTimer.IsElapsed();
}

inline void CCFBot::StopPanicking( void )
{
	m_panicTimer.Invalidate();
}

inline bool CCFBot::IsNotMoving( float minDuration ) const
{
	return (m_stillTimer.HasStarted() && m_stillTimer.GetElapsedTime() >= minDuration);
}

inline CWeaponCFBase *CCFBot::GetActiveCFWeapon( void ) const
{
	return reinterpret_cast<CWeaponCFBase *>( GetActiveWeapon() );
}


inline float CCFBot::GetCombatRange( void ) const
{ 
	return m_combatRange; 
}

inline void CCFBot::SetRogue( bool rogue )
{ 
	m_isRogue = rogue;
}

inline void CCFBot::Hurry( float duration )
{ 
	m_hurryTimer.Start( duration ); 
}

inline float CCFBot::GetSafeTime( void ) const
{
	return m_safeTime;
}

inline bool CCFBot::IsUnhealthy( void ) const
{
	return (GetHealth() <= 40);
}

inline bool CCFBot::IsAlert( void ) const
{
	return !m_alertTimer.IsElapsed();
}

inline void CCFBot::BecomeAlert( void )
{
	const float alertCooldownTime = 10.0f;
	m_alertTimer.Start( alertCooldownTime );
}

inline bool CCFBot::IsSneaking( void ) const
{
	return !m_sneakTimer.IsElapsed();
}

inline void CCFBot::Sneak( float duration )
{
	m_sneakTimer.Start( duration );
}

inline bool CCFBot::IsFollowing( void ) const
{ 
	return m_isFollowing;
}

inline CCFPlayer *CCFBot::GetFollowLeader( void ) const
{ 
	return m_leader;
}

inline float CCFBot::GetFollowDuration( void ) const
{
	return gpGlobals->curtime - m_followTimestamp;
}

inline bool CCFBot::CanAutoFollow( void ) const
{ 
	return (gpGlobals->curtime > m_allowAutoFollowTime);
}

inline void CCFBot::AimAtEnemy( void )
{ 
	m_isAimingAtEnemy = true;
}

inline void CCFBot::StopAiming( void )
{ 
	m_isAimingAtEnemy = false;
}

inline bool CCFBot::IsAimingAtEnemy( void ) const
{
	return m_isAimingAtEnemy;
}

inline float CCFBot::GetStateTimestamp( void ) const
{
	return m_stateTimestamp;
}

inline CFGameState *CCFBot::GetGameState( void )
{
	return &m_gameState;
}

inline const CFGameState *CCFBot::GetGameState( void ) const
{
	return &m_gameState;
}

inline void CCFBot::SetTask( TaskType task, CBaseEntity *entity )
{
	m_task = task;
	m_taskEntity = entity;
}

inline CCFBot::TaskType CCFBot::GetTask( void ) const
{
	return m_task;
}

inline CBaseEntity *CCFBot::GetTaskEntity( void )
{
	return static_cast<CBaseEntity *>( m_taskEntity );
}

inline CCFBot::MoraleType CCFBot::GetMorale( void ) const
{
	return m_morale;
}

inline void CCFBot::Surprise( float duration )
{
	m_surpriseTimer.Start( duration );
}

inline bool CCFBot::IsSurprised( void ) const
{
	return !m_surpriseTimer.IsElapsed();
}

inline CNavArea *CCFBot::GetNoiseArea( void ) const
{
	return m_noiseArea;
}

inline void CCFBot::ForgetNoise( void )
{
	m_noiseTimestamp = 0.0f;
}

inline float CCFBot::GetNoiseRange( void ) const
{
	if (IsNoiseHeard())
		return m_noiseTravelDistance;

	return 999999999.9f;
}

inline PriorityType CCFBot::GetNoisePriority( void ) const
{ 
	return m_noisePriority;
}

inline BotChatterInterface *CCFBot::GetChatter( void )
{
	return &m_chatter;
}

inline CCFPlayer *CCFBot::GetBotEnemy( void ) const
{
	return m_enemy;
}

inline int CCFBot::GetNearbyEnemyCount( void ) const
{ 
	return min( GetEnemiesRemaining(), m_nearbyEnemyCount );
}

inline unsigned int CCFBot::GetEnemyPlace( void ) const
{
	return m_enemyPlace;
}

inline bool CCFBot::CanSeeBomber( void ) const
{
	return (m_bomber == NULL) ? false : true;
}

inline CCFPlayer *CCFBot::GetBomber( void ) const
{
	return m_bomber;
}

inline int CCFBot::GetNearbyFriendCount( void ) const
{
	return min( GetFriendsRemaining(), m_nearbyFriendCount );
}

inline CCFPlayer *CCFBot::GetClosestVisibleFriend( void ) const
{
	return m_closestVisibleFriend;
}

inline CCFPlayer *CCFBot::GetClosestVisibleHumanFriend( void ) const
{
	return m_closestVisibleHumanFriend;
}

inline CCFPlayer *CCFBot::GetClosestVisibleKO( void ) const
{
	return m_closestVisibleKO;
}

inline float CCFBot::GetTimeSinceAttacked( void ) const
{
	return gpGlobals->curtime - m_attackedTimestamp;
}

inline float CCFBot::GetFirstSawEnemyTimestamp( void ) const
{
	return m_firstSawEnemyTimestamp;
}

inline float CCFBot::GetLastSawEnemyTimestamp( void ) const
{
	return m_lastSawEnemyTimestamp;
}

inline float CCFBot::GetTimeSinceLastSawEnemy( void ) const
{
	return gpGlobals->curtime - m_lastSawEnemyTimestamp;
}

inline float CCFBot::GetTimeSinceAcquiredCurrentEnemy( void ) const
{
	return gpGlobals->curtime - m_currentEnemyAcquireTimestamp;
}

inline const Vector &CCFBot::GetLastKnownEnemyPosition( void ) const
{
	return m_lastEnemyPosition;
}

inline bool CCFBot::IsEnemyVisible( void ) const			
{
	return m_isEnemyVisible;
}

inline float CCFBot::GetEnemyDeathTimestamp( void ) const	
{
	return m_enemyDeathTimestamp;
}

inline int CCFBot::GetLastVictimID( void ) const
{
	return m_lastVictimID;
}

inline bool CCFBot::CanSeeSniper( void ) const
{
	return m_isEnemySniperVisible;
}

inline bool CCFBot::HasSeenSniperRecently( void ) const
{
	return !m_sawEnemySniperTimer.IsElapsed();
}

inline float CCFBot::GetTravelDistanceToPlayer( CCFPlayer *player ) const
{
	if (player == NULL)
		return -1.0f;

	if (!player->IsAlive())
		return -1.0f;

	return m_playerTravelDistance[ player->entindex() % MAX_PLAYERS ];
}

inline bool CCFBot::HasPath( void ) const
{
	return (m_pathLength) ? true : false;
}

inline void CCFBot::DestroyPath( void )		
{
	m_isStopping = false;
	m_pathLength = 0;
	m_pathLadder = NULL;
}

inline CNavArea *CCFBot::GetLastKnownArea( void ) const		
{
	return m_lastKnownArea;
}

inline const Vector &CCFBot::GetPathEndpoint( void ) const		
{
	return m_path[ m_pathLength-1 ].pos;
}

inline const Vector &CCFBot::GetPathPosition( int index ) const
{
	return m_path[ index ].pos;
}

inline bool CCFBot::IsUsingLadder( void ) const	
{
	return (m_pathLadder) ? true : false;
}

inline void CCFBot::SetGoalEntity( CBaseEntity *entity )	
{
	m_goalEntity = entity;
}

inline CBaseEntity *CCFBot::GetGoalEntity( void )
{
	return m_goalEntity;
}

inline void CCFBot::ForceRun( float duration )
{
	Run();
	m_mustRunTimer.Start( duration );
}

inline void CCFBot::Wait( float duration )			
{
	m_waitTimer.Start( duration );
}

inline bool CCFBot::IsWaiting( void ) const		
{
	return !m_waitTimer.IsElapsed();
}

inline void CCFBot::StopWaiting( void )			
{
	m_waitTimer.Invalidate();
}

inline bool CCFBot::HasVisitedEnemySpawn( void ) const		
{
	return m_hasVisitedEnemySpawn;
}

inline const Vector &CCFBot::EyePositionConst( void ) const		
{
	return m_eyePosition;
}
	
inline void CCFBot::SetLookAngles( float yaw, float pitch )
{
	m_lookYaw = yaw;
	m_lookPitch = pitch;
}

inline void CCFBot::SetForwardAngle( float angle ) 
{
	m_forwardAngle = angle;
}

inline void CCFBot::SetLookAheadAngle( float angle ) 
{
	m_lookAheadAngle = angle;
}

inline void CCFBot::ClearLookAt( void )
{ 
	//PrintIfWatched( "ClearLookAt()\n" );
	m_lookAtSpotState = NOT_LOOKING_AT_SPOT; 
	m_lookAtDesc = NULL; 
}

inline bool CCFBot::IsLookingAtSpot( PriorityType pri ) const
{ 
	if (m_lookAtSpotState != NOT_LOOKING_AT_SPOT && m_lookAtSpotPriority >= pri)
		return true;

	return false;
}

inline bool CCFBot::IsViewMoving( float angleVelThreshold ) const
{
	if (m_lookYawVel < angleVelThreshold && m_lookYawVel > -angleVelThreshold &&
		m_lookPitchVel < angleVelThreshold && m_lookPitchVel > -angleVelThreshold)
	{
		return false;
	}

	return true;
}

inline bool CCFBot::HasViewBeenSteady( float duration ) const
{
	return (m_viewSteadyTimer.GetElapsedTime() > duration);
}

inline bool CCFBot::HasLookAtTarget( void ) const
{
	return (m_lookAtSpotState != NOT_LOOKING_AT_SPOT);
}

inline bool CCFBot::IsEnemyPartVisible( VisiblePartType part ) const
{ 
	VPROF_BUDGET( "CCFBot::IsEnemyPartVisible", VPROF_BUDGETGROUP_NPCS );

	if (!IsEnemyVisible())
		return false;

	return (m_visibleEnemyParts & part) ? true : false;
}

inline bool CCFBot::IsSignificantlyCloser( const CCFPlayer *testPlayer, const CCFPlayer *referencePlayer ) const
{
	if ( !referencePlayer )
		return true;

	if ( !testPlayer )
		return false;

	float testDist = ( GetAbsOrigin() - testPlayer->GetAbsOrigin() ).Length();
	float referenceDist = ( GetAbsOrigin() - referencePlayer->GetAbsOrigin() ).Length();

	const float significantRangeFraction = 0.7f;
	if ( testDist < referenceDist * significantRangeFraction )
		return true;

	return false;
}

inline void CCFBot::ClearApproachPoints( void )	
{
	m_approachPointCount = 0;
}

inline const CNavArea *CCFBot::GetInitialEncounterArea( void ) const
{
	return m_initialEncounterArea;
}

inline void CCFBot::SetInitialEncounterArea( const CNavArea *area )		
{
	m_initialEncounterArea = area;
}

inline bool CCFBot::IsThrowingGrenade( void ) const		
{
	return m_grenadeTossState != NOT_THROWING;
}

inline bool CCFBot::IsAvoidingGrenade( void ) const
{
	return !m_isAvoidingGrenade.IsElapsed();
}

inline void CCFBot::PrimaryAttack( void )
{
	BaseClass::PrimaryAttack();
}

inline CCFBot::ZoomType CCFBot::GetZoomLevel( void )
{
	if (GetFOV() > 60.0f)
		return NO_ZOOM;
	if (GetFOV() > 25.0f)
		return LOW_ZOOM;
	return HIGH_ZOOM;
}

inline bool CCFBot::IsWaitingForZoom( void ) const		
{
	return !m_zoomTimer.IsElapsed();
}

inline bool CCFBot::IsUsingVoice() const
{
	return m_voiceEndTimestamp > gpGlobals->curtime; 
}

inline bool CCFBot::IsOpeningDoor( void ) const
{
	return m_isOpeningDoor;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Return true if the given weapon is a sniper rifle
 */
inline bool IsSniperRifle( CWeaponCFBase *weapon )
{
	if (weapon == NULL)
		return false;

	switch( weapon->GetWeaponID() )
	{
	case WEAPON_NONE:
	default:
		return false;
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Functor used with NavAreaBuildPath()
 */
class PathCost
{
public:
	PathCost( CCFBot *bot, RouteType route = SAFEST_ROUTE )
	{
		m_bot = bot;
		m_route = route;
	}

	float operator() ( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder )
	{
		float baseDangerFactor = 100.0f;	// 100

		// respond to the danger modulated by our aggression (even super-aggressives pay SOME attention to danger)
		float dangerFactor = (1.0f - (0.95f * m_bot->GetProfile()->GetAggression())) * baseDangerFactor;

		if (fromArea == NULL)
		{
			if (m_route == FASTEST_ROUTE)
				return 0.0f;

			// first area in path, cost is just danger
			return dangerFactor * area->GetDanger( m_bot->GetTeamNumber() );
		}
		else if ((fromArea->GetAttributes() & NAV_MESH_JUMP) && (area->GetAttributes() & NAV_MESH_JUMP))
		{
			// cannot actually walk in jump areas - disallow moving from jump area to jump area
			return -1.0f;
		}
		else
		{
			// compute distance from previous area to this area
			float dist;
			if (ladder)
			{
				// ladders are slow to use
				const float ladderPenalty = 1.0f; // 3.0f;
				dist = ladderPenalty * ladder->m_length;

				// if we are currently escorting hostages, avoid ladders (hostages are confused by them)
				//if (m_bot->GetHostageEscortCount())
				//	dist *= 100.0f;
			}
			else
			{
				dist = (area->GetCenter() - fromArea->GetCenter()).Length();
			}

			// compute distance travelled along path so far
			float cost = dist + fromArea->GetCostSoFar();

			// zombies ignore all path penalties
			if (cv_bot_zombie.GetBool())
				return cost;

			// add cost of "jump down" pain unless we're jumping into water
			if (!area->IsUnderwater() && area->IsConnected( fromArea, NUM_DIRECTIONS ) == false)
			{
				// this is a "jump down" (one way drop) transition - estimate damage we will take to traverse it
				float fallDistance = -fromArea->ComputeHeightChange( area );

				// if it's a drop-down ladder, estimate height from the bottom of the ladder to the lower area
				if ( ladder && ladder->m_bottom.z < fromArea->GetCenter().z && ladder->m_bottom.z > area->GetCenter().z )
				{
					fallDistance = ladder->m_bottom.z - area->GetCenter().z;
				}

				float fallDamage = m_bot->GetApproximateFallDamage( fallDistance );

				if (fallDamage > 0.0f)
				{
					// if the fall would kill us, don't use it
					const float deathFallMargin = 10.0f;
					if (fallDamage + deathFallMargin >= m_bot->GetHealth())
						return -1.0f;

					// if we need to get there in a hurry, ignore minor pain
					const float painTolerance = 15.0f * m_bot->GetProfile()->GetAggression() + 10.0f;
					if (m_route != FASTEST_ROUTE || fallDamage > painTolerance)
					{
						// cost is proportional to how much it hurts when we fall
						// 10 points - not a big deal, 50 points - ouch!
						cost += 100.0f * fallDamage * fallDamage;
					}
				}
			}

			// if this is a "crouch" or "walk" area, add penalty
			if (area->GetAttributes() & (NAV_MESH_CROUCH | NAV_MESH_WALK))
			{
				// these areas are very slow to move through
				float penalty = (m_route == FASTEST_ROUTE) ? 20.0f : 5.0f;

				cost += penalty * dist;
			}

			// if this is a "jump" area, add penalty
			if (area->GetAttributes() & NAV_MESH_JUMP)
			{
				// jumping can slow you down
				//const float jumpPenalty = (m_route == FASTEST_ROUTE) ? 100.0f : 0.5f;
				const float jumpPenalty = 1.0f;
				cost += jumpPenalty * dist;
			}

			// if this is an area to avoid, add penalty
			if (area->GetAttributes() & NAV_MESH_AVOID)
			{
				const float avoidPenalty = 20.0f;
				cost += avoidPenalty * dist;
			}

			if (m_route == SAFEST_ROUTE)
			{
				// add in the danger of this path - danger is per unit length travelled
				cost += dist * dangerFactor * area->GetDanger( m_bot->GetTeamNumber() );
			}

			if (!m_bot->IsAttacking())
			{
				// add in cost of teammates in the way

				// approximate density of teammates based on area
				float size = (area->GetSizeX() + area->GetSizeY())/2.0f;

				// degenerate check
				if (size >= 1.0f)
				{
					// cost is proportional to the density of teammates in this area
					const float costPerFriendPerUnit = 50000.0f;
					cost += costPerFriendPerUnit * (float)area->GetPlayerCount( m_bot->GetTeamNumber() ) / size;
				}
			}

			return cost;
		}
	}

private:
	CCFBot *m_bot;
	RouteType m_route;
};


//--------------------------------------------------------------------------------------------------------------
//
// Prototypes
//
extern int GetBotFollowCount( CCFPlayer *leader );
extern const Vector *FindNearbyRetreatSpot( CCFBot *me, float maxRange = 250.0f );
extern const HidingSpot *FindInitialEncounterSpot( CBaseEntity *me, const Vector &searchOrigin, float enemyArriveTime, float maxRange, bool isSniper );


#endif	// _CF_BOT_H_

