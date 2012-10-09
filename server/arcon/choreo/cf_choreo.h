#include "cbase.h"
#include "ai_baseactor.h"
#include "choreoscene.h"
#include "ichoreoeventcallback.h"
#include "scenefilecache/ISceneFileCache.h"
#include "choreo/cf_choreo_shared.h"
#include "particle_parse.h"

#define CINEMATIC_NAME_LENGTH 64
#define ACTOR_NAME_LENGTH 64
#define WEAPON_NAME_LENGTH 64
#define ATTACHMENT_NAME_LENGTH 64

class CCFActorWeapon : public CBaseAnimating
{
public:
	DECLARE_CLASS( CCFActorWeapon, CBaseAnimating );
	DECLARE_SERVERCLASS();

	virtual void		Spawn();

	virtual char*		GetBoneMergeName(int iBone);

	CNetworkVar(weaponposition_t, m_ePosition);
};

class CCFActor : public CAI_BaseActor
{
public:
	DECLARE_CLASS( CCFActor, CAI_BaseActor );
	DECLARE_SERVERCLASS();

	virtual void		Precache();
	virtual void		Spawn();

	virtual int			UpdateTransmitState( void );

	virtual void		NPCThink( void );

	virtual void		GiveWeapon(class CCinematicWeapon* pWeapon);

	Vector				m_vecLockOrigin;
	QAngle				m_angLockAngles;

	CUtlVector< CHandle<CCFActorWeapon> >	m_hActorWeapons;

	CNetworkVar(float, m_flEffectMagnitude);
};

class CCinematicWeapon
{
public:
	char				m_szModelName[WEAPON_NAME_LENGTH];
	weaponposition_t	m_ePosition;
};

class CCinematicActor
{
public:
						CCinematicActor();

	Vector				m_vecLockOrigin;
	QAngle				m_angLockAngles;

	char				m_szActorName[ACTOR_NAME_LENGTH];
	int					m_nSkin;

	CUtlVector<CCinematicWeapon>	m_aWeapons;
};

typedef enum
{
	EC_UNDEFINED = 0,
	EC_TIMESCALE,
	EC_PARTICLES,
	EC_VISIBILITY,
	EC_MATERIALPROXY,
} eventclass_t;

class CCinematicCP
{
public:
	int						m_iCP;

	ParticleAttachment_t	m_eParticleAttachmentType;

	Vector					m_vecOrigin;

	char					m_szTargetActor[ACTOR_NAME_LENGTH];
	char					m_szAttachment[ATTACHMENT_NAME_LENGTH];
};

class CCinematicEvent
{
public:
						CCinematicEvent();
						CCinematicEvent(const CCinematicEvent &);

	eventclass_t		m_eEventClass;
	float				m_flStartTime;
	float				m_flEndTime;
	bool				m_bStarted;
	bool				m_bExecuted;

	// Parameters
	float					m_flTimeScale;
	char					m_szName[ACTOR_NAME_LENGTH];
	particledispatch_t		m_eParticleDispatchType;
	ParticleAttachment_t	m_eParticleAttachmentType;
	char					m_szActor[ACTOR_NAME_LENGTH];
	char					m_szAttachment[ATTACHMENT_NAME_LENGTH];
	Vector					m_vecOrigin;
	QAngle					m_angAngles;
	CUtlVector<CCinematicCP*>	m_apControlPoints;
	bool					m_bVisible;
	float					m_flValue;
};

class CCinematic : public IChoreoEventCallback
{
public:
							CCinematic(const char* pszName, class CCinematicManager* pManager);

	virtual void			Think(float flFrametime);

	virtual void			Destruct();
	virtual void			LoadFile();

	virtual CChoreoScene*	LoadScene( const char* pszSceneName, IChoreoEventCallback *pCallback );
	virtual void			UnloadScene();

	virtual float			Play();
	virtual void			PlayEvent(int i);
	virtual void			FinishEvent(int i);
	virtual bool			IsFinished();

	virtual float			FrameToTime(float flFrame);

	// From IChoreoEventCallback
	virtual void			StartEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual void			EndEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual void			ProcessEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual bool			CheckEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );

	virtual void			ClearSchedules( CChoreoScene* pScene );
	virtual void			ClearSceneEvents( CChoreoScene* pScene, bool canceled );

	virtual CBaseEntity*	FindNamedActor( const char* pszActorName );
	virtual CBaseEntity*	FindNamedEntity( const char *name );

	virtual CCinematicActor*	FindCinematicActor( const char* pszActorName );

	char					m_szCinematicName[CINEMATIC_NAME_LENGTH];
	class CCinematicManager*	m_pManager;
	CChoreoScene*			m_pScene;

	float					m_flFramerate;

	float					m_flTimeScaleCurrent;
	float					m_flTimeScaleGoal;

	CUtlVector<CCinematicActor*>	m_apActors;
	CUtlVector<CCinematicEvent>		m_aEvents;
};

class CCinematicManager : public CAutoGameSystemPerFrame
{
public:
							CCinematicManager();

	virtual void			LevelInitPreEntity();
	virtual void			FrameUpdatePostEntityThink();

	virtual void			LoadCinematic( const char* pszCinematicName );
	virtual void			UnloadCinematic();

	virtual float			PlayCinematic( const char* pszCinematicName );

	CCinematic*				m_pCinematic;

	static CCinematicManager	s_oCinematicManager;
};

CCinematicManager* CinematicManager();
