#include "cbase.h"
#include "cf_choreo.h"
#include "sceneentity.h"
#include "cf_shareddefs.h"
#include "cf_player.h"
#include "utlbuffer.h"
#include "choreoactor.h"
#include <string>
#include "filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( cf_actor_weapon, CCFActorWeapon );

void CCFActorWeapon::Spawn()
{
	SetModel( STRING( GetModelName() ) );

	m_ePosition = WP_WJAT;
}

IMPLEMENT_SERVERCLASS_ST(CCFActor, DT_CFActor)
	SendPropFloat(SENDINFO(m_flEffectMagnitude), 12, 0, 0, 1),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( cf_actor, CCFActor );

PRECACHE_REGISTER(cf_actor);

void CCFActor::Precache()
{
	PrecacheModel( "models/dev/camera.mdl" );
}

void CCFActor::Spawn()
{
	BaseClass::Spawn();

	SetNextThink( gpGlobals->curtime );

	SetModel( STRING( GetModelName() ) );

	CapabilitiesAdd( bits_CAP_ANIMATEDFACE );

	SetAbsOrigin(m_vecLockOrigin);
	SetAbsAngles(m_angLockAngles);

	NPCInit();

	m_flEffectMagnitude = 0;
}

int CCFActor::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

void CCFActor::NPCThink( void )
{
	BaseClass::NPCThink();

	// Lock us in place so that the animations from XSI do all of the moving and rotating.
	SetAbsOrigin(m_vecLockOrigin);
	SetAbsAngles(m_angLockAngles);
}

void CCFActor::GiveWeapon(CCinematicWeapon* pWeapon)
{
	CCFActorWeapon* pCFWeapon = (CCFActorWeapon*)CreateEntityByName("cf_actor_weapon");

	pCFWeapon->SetModelName(MAKE_STRING(pWeapon->m_szModelName));

	DispatchSpawn(pCFWeapon);

	pCFWeapon->m_ePosition = pWeapon->m_ePosition;
	pCFWeapon->FollowEntity(this);

	m_hActorWeapons.AddToTail(pCFWeapon);
}

CCinematicActor::CCinematicActor()
{
	m_vecLockOrigin = Vector(0,0,0);
	m_angLockAngles = QAngle(0,0,0);

	m_szActorName[0] = 0;
	m_nSkin = 0;
}

CCinematicEvent::CCinematicEvent()
{
	m_eEventClass = EC_UNDEFINED;
	m_bStarted = false;
	m_bExecuted = false;
	m_flStartTime = 0;
	m_flEndTime = 0;

	// Parameters
	m_flTimeScale = 0;
	m_szName[0] = 0;
	m_eParticleDispatchType = PDT_UNDEFINED;
	m_eParticleAttachmentType = (ParticleAttachment_t)0;
	m_szActor[0] = 0;
	m_szAttachment[0] = 0;
	m_vecOrigin = Vector(0,0,0);
	m_angAngles = QAngle(0,0,0);
	m_bVisible = true;
	m_flValue = 0;
}

CCinematicEvent::CCinematicEvent(const CCinematicEvent &o)
{
	m_eEventClass = o.m_eEventClass;
	m_bStarted = o.m_bStarted;
	m_bExecuted = o.m_bExecuted;
	m_flStartTime = o.m_flStartTime;
	m_flEndTime = o.m_flEndTime;

	// Parameters
	m_flTimeScale = o.m_flTimeScale;
	strcpy(m_szName, o.m_szName);
	m_eParticleDispatchType = o.m_eParticleDispatchType;
	m_eParticleAttachmentType = o.m_eParticleAttachmentType;
	strcpy(m_szActor, o.m_szActor);
	strcpy(m_szAttachment, o.m_szAttachment);
	m_vecOrigin = o.m_vecOrigin;
	m_angAngles = o.m_angAngles;
	m_bVisible = o.m_bVisible;
	m_flValue = o.m_flValue;

	for (int i = 0; i < o.m_apControlPoints.Count(); i++)
	{
		CCinematicCP* pCP = new CCinematicCP();
		m_apControlPoints.AddToTail(pCP);
		*pCP = *o.m_apControlPoints[i];
	}
}

CCinematic::CCinematic(const char* pszName, CCinematicManager* pManager)
{
	Q_strncpy(m_szCinematicName, pszName, CINEMATIC_NAME_LENGTH);

	m_pManager = pManager;
	m_pScene = NULL;
	m_flFramerate = 0;
}

void CCinematic::Think(float flFrametime)
{
	if ( !m_pScene )
		return;

	float flCurrentTime = m_pScene->GetTime() + flFrametime;

	m_pScene->Think( flCurrentTime );

	for (int i = 0; i < m_aEvents.Count(); i++ )
	{
		CCinematicEvent* pEvent = &m_aEvents[i];

		if (flCurrentTime >= pEvent->m_flStartTime && !pEvent->m_bStarted)
			PlayEvent(i);
		else if (pEvent->m_flEndTime && pEvent->m_bStarted && !pEvent->m_bExecuted && flCurrentTime >= pEvent->m_flEndTime)
			FinishEvent(i);
	}

	m_flTimeScaleCurrent = UTIL_Approach(m_flTimeScaleGoal, m_flTimeScaleCurrent, flFrametime*10);

	float flEngineTimescale = cvar->FindVar("host_timescale")->GetFloat();
	if (m_flTimeScaleCurrent != flEngineTimescale)
		cvar->FindVar("host_timescale")->SetValue(m_flTimeScaleCurrent);

	if ( m_pScene->SimulationFinished() )
	{
		cvar->FindVar("host_timescale")->SetValue(1);
		UnloadScene();
	}
}

void CCinematic::Destruct()
{
	for (int i = 0; i < m_aEvents.Count(); i++)
		m_aEvents[i].m_apControlPoints.PurgeAndDeleteElements();

	UnloadScene();

	m_apActors.PurgeAndDeleteElements();
}

void CCinematic::LoadFile()
{
	if (!filesystem->FileExists(VarArgs("cinematics/%s.txt", m_szCinematicName)))
		return;

	Destruct();

	byte* pBytes = UTIL_LoadFileForMe(VarArgs("cinematics/%s.txt", m_szCinematicName), NULL);
	const char* pFile = (char*)pBytes;

	const int iTokenSize = 1024;
	char szToken[iTokenSize];

	while ( pFile )
	{
		pFile = engine->ParseFile(pFile, szToken, iTokenSize);
		if (strlen(szToken) <= 0)
			break;

		if ( !Q_stricmp( szToken, "scene" ) )
		{
			pFile = engine->ParseFile(pFile, szToken, iTokenSize);
			m_pScene = LoadScene(szToken, this);
		}
		else if (!Q_stricmp(szToken, "actor"))
		{
			pFile = engine->ParseFile(pFile, szToken, iTokenSize);
			int iActor = m_apActors.AddToTail(new CCinematicActor());

			CCinematicActor* pCinematicActor = m_apActors[iActor];

			Q_strncpy(pCinematicActor->m_szActorName, szToken, ACTOR_NAME_LENGTH);

			// {
			pFile = engine->ParseFile(pFile, szToken, iTokenSize);

			while ( pFile )
			{
				pFile = engine->ParseFile(pFile, szToken, iTokenSize);
				if ( !Q_stricmp( szToken, "}" ) )
					break;

				if ( !Q_stricmp( szToken, "skin" ) )
				{
					pFile = engine->ParseFile(pFile, szToken, iTokenSize);
					pCinematicActor->m_nSkin = atoi(szToken);
				}
				else if ( !Q_stricmp( szToken, "weapon" ) )
				{
					int iWeapon = pCinematicActor->m_aWeapons.AddToTail(CCinematicWeapon());
					CCinematicWeapon* pWeapon = &pCinematicActor->m_aWeapons[iWeapon];

					pFile = engine->ParseFile(pFile, szToken, iTokenSize);

					if (FStrEq(szToken, "righthand"))
						pWeapon->m_ePosition = WP_RIGHTHAND;
					else if (FStrEq(szToken, "lefthand"))
						pWeapon->m_ePosition = WP_LEFTHAND;
					else if (FStrEq(szToken, "rightleg"))
						pWeapon->m_ePosition = WP_RIGHTLEG;
					else if (FStrEq(szToken, "leftleg"))
						pWeapon->m_ePosition = WP_LEFTLEG;
					else if (FStrEq(szToken, "rightback"))
						pWeapon->m_ePosition = WP_RIGHTBACK;
					else if (FStrEq(szToken, "leftback"))
						pWeapon->m_ePosition = WP_LEFTBACK;
					else
						pWeapon->m_ePosition = WP_WJAT;

					pFile = engine->ParseFile(pFile, szToken, iTokenSize);
					Q_strncpy(pWeapon->m_szModelName, szToken, WEAPON_NAME_LENGTH);
				}
				else if ( !Q_stricmp( szToken, "origin" ) )
				{
					Vector vecLockOrigin;
					pFile = engine->ParseFile(pFile, szToken, iTokenSize);
					vecLockOrigin.x = atof(szToken);
					pFile = engine->ParseFile(pFile, szToken, iTokenSize);
					vecLockOrigin.y = atof(szToken);
					pFile = engine->ParseFile(pFile, szToken, iTokenSize);
					vecLockOrigin.z = atof(szToken);
					pCinematicActor->m_vecLockOrigin = vecLockOrigin;
				}
				else if ( !Q_stricmp( szToken, "angles" ) )
				{
					QAngle angLockAngles;
					pFile = engine->ParseFile(pFile, szToken, iTokenSize);
					angLockAngles.x = atof(szToken);
					pFile = engine->ParseFile(pFile, szToken, iTokenSize);
					angLockAngles.y = atof(szToken);
					pFile = engine->ParseFile(pFile, szToken, iTokenSize);
					angLockAngles.z = atof(szToken);
					pCinematicActor->m_angLockAngles = angLockAngles;
				}
				else if ( !Q_stricmp( szToken, "xsiorigin" ) )
				{
					Vector vecLockOrigin;
					pFile = engine->ParseFile(pFile, szToken, iTokenSize);
					vecLockOrigin.x = atof(szToken);
					pFile = engine->ParseFile(pFile, szToken, iTokenSize);
					vecLockOrigin.z = atof(szToken);
					pFile = engine->ParseFile(pFile, szToken, iTokenSize);
					vecLockOrigin.y = -atof(szToken);
					pCinematicActor->m_vecLockOrigin = vecLockOrigin;
				}
				else if ( !Q_stricmp( szToken, "xsiangles" ) )
				{
					QAngle angLockAngles;
					pFile = engine->ParseFile(pFile, szToken, iTokenSize);
					angLockAngles.x = atof(szToken);
					pFile = engine->ParseFile(pFile, szToken, iTokenSize);
					angLockAngles.y = atof(szToken) - 90;
					pFile = engine->ParseFile(pFile, szToken, iTokenSize);
					angLockAngles.z = atof(szToken);
					pCinematicActor->m_angLockAngles = angLockAngles;
				}
			}
		}
		else if (!Q_stricmp(szToken, "framerate"))
		{
			pFile = engine->ParseFile(pFile, szToken, iTokenSize);
			m_flFramerate = atof(szToken);
		}
		else if (!Q_stricmp(szToken, "events"))
		{
			// {
			pFile = engine->ParseFile(pFile, szToken, iTokenSize);

			while ( pFile )
			{
				pFile = engine->ParseFile(pFile, szToken, iTokenSize);
				if ( !Q_stricmp( szToken, "}" ) )
					break;

				int iEvent = m_aEvents.AddToTail(CCinematicEvent());
				CCinematicEvent* pEvent = &m_aEvents[iEvent];

				if ( !Q_stricmp( szToken, "timescale" ) )
				{
					pEvent->m_eEventClass = EC_TIMESCALE;

					pFile = engine->ParseFile(pFile, szToken, iTokenSize);
					pEvent->m_flStartTime = FrameToTime(atof(szToken));

					pFile = engine->ParseFile(pFile, szToken, iTokenSize);
					pEvent->m_flTimeScale = FrameToTime(atof(szToken));
					if (pEvent->m_flTimeScale <= 0)
						pEvent->m_flTimeScale = 1/60;
				}
				else if ( !Q_stricmp( szToken, "particles" ) )
				{
					pEvent->m_eEventClass = EC_PARTICLES;

					pFile = engine->ParseFile(pFile, szToken, iTokenSize);
					strcpy(pEvent->m_szName, szToken);

					// {
					pFile = engine->ParseFile(pFile, szToken, iTokenSize);

					while ( pFile )
					{
						pFile = engine->ParseFile(pFile, szToken, iTokenSize);
						if ( !Q_stricmp( szToken, "}" ) )
							break;

						if (FStrEq(szToken, "start"))
						{
							pFile = engine->ParseFile(pFile, szToken, iTokenSize);
							pEvent->m_flStartTime = FrameToTime(atof(szToken));
						}
						else if (FStrEq(szToken, "end"))
						{
							pFile = engine->ParseFile(pFile, szToken, iTokenSize);
							pEvent->m_flEndTime = FrameToTime(atof(szToken));
						}
						else if (FStrEq(szToken, "attachment"))
						{
							pEvent->m_eParticleDispatchType = PDT_ATTACHMENT;

							pFile = engine->ParseFile(pFile, szToken, iTokenSize);
							if (FStrEq(szToken, "absorigin"))
								pEvent->m_eParticleAttachmentType = PATTACH_ABSORIGIN;
							else if (FStrEq(szToken, "absorigin_follow"))
								pEvent->m_eParticleAttachmentType = PATTACH_ABSORIGIN_FOLLOW;
							else if (FStrEq(szToken, "customorigin"))
								pEvent->m_eParticleAttachmentType = PATTACH_CUSTOMORIGIN;
							else if (FStrEq(szToken, "point"))
								pEvent->m_eParticleAttachmentType = PATTACH_POINT;
							else if (FStrEq(szToken, "point_follow"))
								pEvent->m_eParticleAttachmentType = PATTACH_POINT_FOLLOW;
							else if (FStrEq(szToken, "worldorigin"))
								pEvent->m_eParticleAttachmentType = PATTACH_WORLDORIGIN;

							pFile = engine->ParseFile(pFile, szToken, iTokenSize);
							strcpy(pEvent->m_szActor, szToken);

							pFile = engine->ParseFile(pFile, szToken, iTokenSize);
							strcpy(pEvent->m_szAttachment, szToken);
						}
						else if (FStrEq(szToken, "origin"))
						{
							pEvent->m_eParticleDispatchType = PDT_ORIGIN;

							Vector vecOrigin;
							pFile = engine->ParseFile(pFile, szToken, iTokenSize);
							vecOrigin.x = atof(szToken);
							pFile = engine->ParseFile(pFile, szToken, iTokenSize);
							vecOrigin.y = atof(szToken);
							pFile = engine->ParseFile(pFile, szToken, iTokenSize);
							vecOrigin.z = atof(szToken);
							pEvent->m_vecOrigin = vecOrigin;

							QAngle angAngles;
							pFile = engine->ParseFile(pFile, szToken, iTokenSize);
							angAngles.x = atof(szToken);
							pFile = engine->ParseFile(pFile, szToken, iTokenSize);
							angAngles.y = atof(szToken);
							pFile = engine->ParseFile(pFile, szToken, iTokenSize);
							angAngles.z = atof(szToken);
							pEvent->m_angAngles = angAngles;
						}
						else if (FStrEq(szToken, "controlpoint"))
						{
							CCinematicCP* pCP = new CCinematicCP();
							pEvent->m_apControlPoints.AddToTail(pCP);

							pFile = engine->ParseFile(pFile, szToken, iTokenSize);
							pCP->m_iCP = atoi(szToken);

							pFile = engine->ParseFile(pFile, szToken, iTokenSize);
							if (FStrEq(szToken, "absorigin"))
								pCP->m_eParticleAttachmentType = PATTACH_ABSORIGIN;
							else if (FStrEq(szToken, "absorigin_follow"))
								pCP->m_eParticleAttachmentType = PATTACH_ABSORIGIN_FOLLOW;
							else if (FStrEq(szToken, "customorigin"))
								pCP->m_eParticleAttachmentType = PATTACH_CUSTOMORIGIN;
							else if (FStrEq(szToken, "point"))
								pCP->m_eParticleAttachmentType = PATTACH_POINT;
							else if (FStrEq(szToken, "point_follow"))
								pCP->m_eParticleAttachmentType = PATTACH_POINT_FOLLOW;
							else if (FStrEq(szToken, "worldorigin"))
								pCP->m_eParticleAttachmentType = PATTACH_WORLDORIGIN;

							switch (pCP->m_eParticleAttachmentType)
							{
							case PATTACH_CUSTOMORIGIN:
								pFile = engine->ParseFile(pFile, szToken, iTokenSize);
								pCP->m_vecOrigin.x = atof(szToken);
								pFile = engine->ParseFile(pFile, szToken, iTokenSize);
								pCP->m_vecOrigin.y = atof(szToken);
								pFile = engine->ParseFile(pFile, szToken, iTokenSize);
								pCP->m_vecOrigin.z = atof(szToken);
								break;

							case PATTACH_POINT_FOLLOW:
								pFile = engine->ParseFile(pFile, szToken, iTokenSize);
								strcpy(pCP->m_szTargetActor, szToken);

								pFile = engine->ParseFile(pFile, szToken, iTokenSize);
								strcpy(pCP->m_szAttachment, szToken);
								break;
							}
						}
					}
				}
				else if ( !Q_stricmp( szToken, "visibility" ) )
				{
					pEvent->m_eEventClass = EC_VISIBILITY;

					pFile = engine->ParseFile(pFile, szToken, iTokenSize);
					strcpy(pEvent->m_szActor, szToken);

					pFile = engine->ParseFile(pFile, szToken, iTokenSize);
					pEvent->m_flStartTime = FrameToTime(atof(szToken));

					pFile = engine->ParseFile(pFile, szToken, iTokenSize);
					pEvent->m_bVisible = !!atoi(szToken);
				}
				else if ( !Q_stricmp( szToken, "materialproxy" ) )
				{
					pEvent->m_eEventClass = EC_MATERIALPROXY;

					pFile = engine->ParseFile(pFile, szToken, iTokenSize);
					strcpy(pEvent->m_szActor, szToken);

					pFile = engine->ParseFile(pFile, szToken, iTokenSize);
					pEvent->m_flStartTime = FrameToTime(atof(szToken));

					pFile = engine->ParseFile(pFile, szToken, iTokenSize);
					pEvent->m_flValue = atof(szToken);
				}
				else
					m_aEvents.Remove(iEvent);
			}
		}
	}

	UTIL_FreeFile( pBytes );
}

void CCinematic::StartEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( event );

	if ( !Q_stricmp( event->GetName(), "NULL" ) )
 	{
 		Scene_Printf( "CCinematic: %8.2f:  ignored %s\n", currenttime, event->GetDescription() );
 		return;
 	}

	CBaseEntity *pActor = NULL;
	CChoreoActor *actor = event->GetActor();
	if ( actor )
	{
		pActor = FindNamedActor( actor->GetName() );
		if (pActor == NULL)
		{
			Warning( "CCinematic: Unable to find actor named \"%s\"\n", actor->GetName() );
			return;
		}
	}

	if (pActor && pActor->MyNPCPointer())
		pActor->MyNPCPointer()->AddSceneEvent( scene, event );
}

void CCinematic::EndEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( event );

	if ( !Q_stricmp( event->GetName(), "NULL" ) )
 	{
 		return;
 	}

	CBaseEntity *pActor = NULL;
	CChoreoActor *actor = event->GetActor();
	if ( actor )
		pActor = FindNamedActor( actor->GetName() );

	if (pActor && pActor->MyNPCPointer())
		pActor->MyNPCPointer()->RemoveSceneEvent( scene, event, false );
}

void CCinematic::ProcessEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
}

bool CCinematic::CheckEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	return true;
}

CChoreoScene *CCinematic::LoadScene( const char *filename, IChoreoEventCallback *pCallback )
{
	DevMsg( 2, "Blocking load of scene from '%s'\n", filename );

	char* pBuffer = (char*)UTIL_LoadFileForMe(filename, NULL);

	// Read the text versions of the scene files, so that we can get at extra data like model names.
	g_TokenProcessor.SetBuffer( pBuffer );
	return ChoreoLoadScene(filename, pCallback, &g_TokenProcessor, Scene_Printf );
}

void CCinematic::UnloadScene()
{
	if ( m_pScene )
	{
		ClearSchedules( m_pScene );
		ClearSceneEvents( m_pScene, false );

		for ( int i = 0 ; i < m_pScene->GetNumActors(); i++ )
		{
			CChoreoActor* pChoreoActor = m_pScene->GetActor(i);
			if (!pChoreoActor)
				continue;

			CBaseEntity *pTestActor = FindNamedActor( pChoreoActor->GetName() );

			if ( !pTestActor )
				continue;

			if (pTestActor->MyNPCPointer())
				pTestActor->MyNPCPointer()->RemoveChoreoScene( m_pScene );

			UTIL_Remove(pTestActor);
		}

		delete m_pScene;
	}
	m_pScene = NULL;
}

float CCinematic::Play()
{
	if (!m_pScene)
		LoadFile();

	if (!m_pScene)
		return -1;

	int i;

	for ( i = 0; i < m_pScene->GetNumActors(); i++ )
	{
		CChoreoActor* pChoreoActor = m_pScene->GetActor(i);

		if (!pChoreoActor)
			continue;

		CCinematicActor* pCinematicActor = FindCinematicActor(pChoreoActor->GetName());

		EHANDLE pent;

		pent = CreateEntityByName("cf_actor");

		if (pent == NULL)
			continue;

		CBaseFlex* pActor = pent->MyCombatCharacterPointer();
		CCFActor* pCFActor = (CCFActor*)pActor;

		if (pCinematicActor)
		{
			pCFActor->m_vecLockOrigin = pCinematicActor->m_vecLockOrigin;
			pCFActor->m_angLockAngles = pCinematicActor->m_angLockAngles;

			pent->SetLocalOrigin( pCinematicActor->m_vecLockOrigin );
		}

		pent->SetName(MAKE_STRING(pChoreoActor->GetName()));

		char szModelName[ACTOR_NAME_LENGTH];
		const char* pszModelName = pChoreoActor->GetFacePoserModelName();
		pszModelName = strstr(pszModelName, "models");

		if (pszModelName)
		{
			// Replace backslashes with forward slashes.
			std::string sModelName(pszModelName);
			size_t iSlash;
			while ((iSlash = sModelName.find('\\')) != std::string::npos)
				sModelName.replace(iSlash, 1, "/");

			Q_strncpy(szModelName, sModelName.c_str(), ACTOR_NAME_LENGTH);
			pent->SetModelName(MAKE_STRING(szModelName));
		}
		else
		{
			pent->SetModelName(MAKE_STRING("models/dev/camera.mdl"));
		}

		DispatchSpawn( pent );

		pCFActor->AddEFlags(EFL_FORCE_CHECK_TRANSMIT);

		pCFActor->StartChoreoScene( m_pScene );

		if (pCinematicActor)
		{
			pCFActor->m_nSkin = pCinematicActor->m_nSkin;
			for (int j = 0; j < pCinematicActor->m_aWeapons.Count(); j++)
				pCFActor->GiveWeapon(&pCinematicActor->m_aWeapons[j]);
		}
	}

	m_flTimeScaleCurrent = m_flTimeScaleGoal = 1;

	m_pScene->ResetSimulation();

	return m_pScene->FindStopTime();
}

void CCinematic::PlayEvent(int i)
{
	CCinematicEvent* pEvent = &m_aEvents[i];

	pEvent->m_bStarted = true;

	switch (pEvent->m_eEventClass)
	{
	case EC_TIMESCALE:
		m_flTimeScaleGoal = pEvent->m_flTimeScale;
		break;

	case EC_PARTICLES:
	{
		CReliableBroadcastRecipientFilter filter;
		UserMessageBegin( filter, "CinematicEvent" );
			WRITE_STRING( pEvent->m_szName );
			WRITE_BYTE( pEvent->m_eParticleDispatchType );

			switch (pEvent->m_eParticleDispatchType)
			{
			case PATTACH_CUSTOMORIGIN:
				WRITE_VEC3COORD( pEvent->m_vecOrigin );
				WRITE_VEC3COORD( Vector(pEvent->m_angAngles.x, pEvent->m_angAngles.y, pEvent->m_angAngles.z) );
				break;

			case PDT_ATTACHMENT:
			{
				CBaseEntity* pEntity = FindNamedActor(pEvent->m_szActor);
				if (pEntity)
					WRITE_SHORT( pEntity->entindex() );
				else
					WRITE_SHORT( 0 );

				WRITE_BYTE( pEvent->m_eParticleAttachmentType );
				WRITE_SHORT( (pEntity&&pEntity->GetBaseAnimating())?pEntity->GetBaseAnimating()->LookupAttachment(pEvent->m_szAttachment):0 );
				break;
			}
			}

			WRITE_BYTE( pEvent->m_apControlPoints.Count() );
			for (int i = 0; i < pEvent->m_apControlPoints.Count(); i++)
			{
				CCinematicCP* pCP = pEvent->m_apControlPoints[i];
				WRITE_BYTE( pCP->m_iCP );
				WRITE_BYTE( pCP->m_eParticleAttachmentType );

				switch (pCP->m_eParticleAttachmentType)
				{
				case PATTACH_CUSTOMORIGIN:
					WRITE_VEC3COORD( pCP->m_vecOrigin );
					break;

				case PATTACH_POINT_FOLLOW:
				{
					CBaseEntity* pTargetActor = FindNamedActor(pCP->m_szTargetActor);
					WRITE_SHORT( pTargetActor?pTargetActor->entindex():0 );
					WRITE_STRING( pCP->m_szAttachment );
					break;
				}
				}
			}

		MessageEnd();
		break;
	}
	case EC_VISIBILITY:
	{
		CBaseEntity* pTargetActor = FindNamedActor(pEvent->m_szActor);
		if (pTargetActor)
		{
			if (pEvent->m_bVisible)
				pTargetActor->RemoveEffects( EF_NODRAW );
			else
				pTargetActor->AddEffects( EF_NODRAW );
		}
		break;
	}
	case EC_MATERIALPROXY:
	{
		CBaseEntity* pTargetActor = FindNamedActor(pEvent->m_szActor);
		CCFActor* pCFActor = dynamic_cast<CCFActor*>(pTargetActor);
		if (pCFActor)
			pCFActor->m_flEffectMagnitude = pEvent->m_flValue;
		break;
	}
	}

	if (!pEvent->m_flEndTime)
		pEvent->m_bExecuted = true;
}

void CCinematic::FinishEvent(int i)
{
	CCinematicEvent* pEvent = &m_aEvents[i];

	pEvent->m_bExecuted = true;

	switch (pEvent->m_eEventClass)
	{
	case EC_PARTICLES:
	{
		CReliableBroadcastRecipientFilter filter;
		UserMessageBegin( filter, "CinematicEvent" );
			WRITE_STRING( pEvent->m_szName );
			WRITE_BYTE( PDT_STOPEMISSION );

			CBaseEntity* pTargetActor = FindNamedActor( pEvent->m_szActor );
			WRITE_SHORT( pTargetActor?pTargetActor->entindex():0 );
		MessageEnd();
		break;
	}
	}
}

bool CCinematic::IsFinished()
{
	if (!m_pScene)
		return true;

	return m_pScene->SimulationFinished();
}

float CCinematic::FrameToTime(float flFrame)
{
	if (m_flFramerate)
		return flFrame/m_flFramerate;

	return flFrame;
}

void CCinematic::ClearSceneEvents( CChoreoScene* pScene, bool canceled )
{
	if (!pScene)
		return;

	int i;
	for ( i = 0 ; i < pScene->GetNumActors(); i++ )
	{
		CChoreoActor* pChoreoActor = pScene->GetActor(i);

		if (!pChoreoActor)
			continue;

		CBaseEntity *pActor = FindNamedActor( pChoreoActor->GetName() );
		if ( !pActor )
			continue;

		// Clear any existing expressions
		if (pActor->MyNPCPointer())
			pActor->MyNPCPointer()->ClearSceneEvents( pScene, canceled );
	}

	// Iterate events and precache necessary resources
	for ( i = 0; i < pScene->GetNumEvents(); i++ )
	{
		CChoreoEvent *event = pScene->GetEvent( i );
		if ( !event )
			continue;

		// load any necessary data
		switch (event->GetType() )
		{
		default:
			break;
		case CChoreoEvent::SUBSCENE:
			{
				// Only allow a single level of subscenes for now
				if ( !pScene->IsSubScene() )
				{
					CChoreoScene *pSubscene = event->GetSubScene();
					if ( pSubscene )
						ClearSceneEvents( pSubscene, canceled );
				}
			}
			break;
		}
	}

	CReliableBroadcastRecipientFilter filter;
	UserMessageBegin( filter, "CinematicEvent" );
		WRITE_STRING( "" );
		WRITE_BYTE( PDT_STOPEMISSION );
		WRITE_SHORT( 0 );
	MessageEnd();
}

void CCinematic::ClearSchedules( CChoreoScene* pScene )
{
	if ( !pScene )
		return;

	int i;
	for ( i = 0 ; i < pScene->GetNumActors(); i++ )
	{
		CChoreoActor* pChoreoActor = pScene->GetActor(i);
		if (!pChoreoActor)
			continue;

		CBaseEntity *pActor = FindNamedActor( pChoreoActor->GetName() );
		if ( !pActor )
			continue;

		CAI_BaseNPC *pNPC = pActor->MyNPCPointer();

		if ( !pNPC )
		{
			if (pActor->GetBaseAnimating())
			{
				pActor->GetBaseAnimating()->ResetSequence( pActor->GetBaseAnimating()->SelectWeightedSequence( ACT_IDLE ) );
				pActor->GetBaseAnimating()->SetCycle( 0 );
			}
		}
	}

	// Iterate events and precache necessary resources
	for ( i = 0; i < pScene->GetNumEvents(); i++ )
	{
		CChoreoEvent *event = pScene->GetEvent( i );
		if ( !event )
			continue;

		// load any necessary data
		switch (event->GetType() )
		{
		default:
			break;
		case CChoreoEvent::SUBSCENE:
			{
				// Only allow a single level of subscenes for now
				if ( !pScene->IsSubScene() )
				{
					CChoreoScene *subscene = event->GetSubScene();
					if ( subscene )
						ClearSchedules( subscene );
				}
			}
			break;
		}
	}
}

CBaseEntity* CCinematic::FindNamedActor( const char* pszActorName )
{
	char szActorNameOnly[ACTOR_NAME_LENGTH];
	Q_strncpy(szActorNameOnly, pszActorName, ACTOR_NAME_LENGTH);

	// Find the dot.
	char* pszDot = strstr(szActorNameOnly, ".");
	if (pszDot)
	{
		// If there's a dot then we're looking at a weapon attachment.
		pszDot[0] = '\0';
		CBaseEntity* pActor = FindNamedActor(szActorNameOnly);
		CCFActor* pCFActor = dynamic_cast<CCFActor*>(pActor);

		if (pCFActor)
		{
			// The weapon begins right after the dot.
			char* pszWeapon = pszDot+1;
			for (int i = 0; i < pCFActor->m_hActorWeapons.Count(); i++)
			{
				CCFActorWeapon* pWeapon = pCFActor->m_hActorWeapons[i];

				if (!pWeapon)
					continue;

				if (pWeapon->m_ePosition == WP_RIGHTHAND && FStrEq(pszWeapon, "righthand"))
					return pWeapon;
				else if (pWeapon->m_ePosition == WP_LEFTHAND && FStrEq(pszWeapon, "lefthand"))
					return pWeapon;
				else if (pWeapon->m_ePosition == WP_RIGHTLEG && FStrEq(pszWeapon, "rightleg"))
					return pWeapon;
				else if (pWeapon->m_ePosition == WP_LEFTLEG && FStrEq(pszWeapon, "leftleg"))
					return pWeapon;
				else if (pWeapon->m_ePosition == WP_RIGHTBACK && FStrEq(pszWeapon, "rightback"))
					return pWeapon;
				else if (pWeapon->m_ePosition == WP_LEFTBACK && FStrEq(pszWeapon, "leftback"))
					return pWeapon;
			}
		}

		// If we couldn't find a weapon, return the actor instead.
		// (Or NULL if it couldn't even find the actor.)
		return pCFActor;
	}

	CBaseEntity *entity = FindNamedEntity( pszActorName );

	if ( !entity )
	{
		// Couldn't find actor!
		return NULL;
	}

	// Make sure it can actually do facial animation, etc.
	CBaseFlex *flexEntity = dynamic_cast< CBaseFlex * >( entity );
	if ( !flexEntity )
	{
		// That actor was not a CBaseFlex!
		return NULL;
	}

	return flexEntity;
}

#define FINDNAMEDENTITY_MAX_ENTITIES	32

CBaseEntity* CCinematic::FindNamedEntity( const char *name )
{
	CBaseEntity *entity = NULL;

	// search for up to 32 entities with the same name and choose one randomly
	CBaseEntity *entityList[ FINDNAMEDENTITY_MAX_ENTITIES ];
	int	iCount;

	entity = NULL;
	for( iCount = 0; iCount < FINDNAMEDENTITY_MAX_ENTITIES; iCount++ )
	{
		entity = gEntList.FindEntityByName( entity, name );
		if ( !entity )
			break;
		entityList[ iCount ] = entity;
	}

	if ( iCount > 0 )
		entity = entityList[ RandomInt( 0, iCount - 1 ) ];
	else
		entity = NULL;

	return entity;
}

CCinematicActor* CCinematic::FindCinematicActor( const char* pszActorName )
{
	for (int i = 0; i < m_apActors.Size(); i++)
		if (stricmp(m_apActors[i]->m_szActorName, pszActorName) == 0)
			return m_apActors[i];

	return NULL;
}

CCinematicManager CCinematicManager::s_oCinematicManager;

CCinematicManager::CCinematicManager()
	: CAutoGameSystemPerFrame("Cinematics")
{
	m_pCinematic = NULL;
}

ConVar cf_cinematic("cf_cinematic", "");

class CCinematicInfo
{
public:
	std::string					m_sName;
	CUtlVector<std::string>		m_asModelPrecaches;
	CUtlVector<std::string>		m_asParticlePrecaches;
};

CUtlVector<CCinematicInfo> g_aCinematics;

void CCinematicManager::LevelInitPreEntity()
{
	if (filesystem->FileExists("cinematics/manifest.txt"))
	{
		byte* pBytes = UTIL_LoadFileForMe("cinematics/manifest.txt", NULL);
		const char* pFile = (char*)pBytes;

		const int iTokenSize = 1024;
		char szToken[iTokenSize];

		// cinematics
		pFile = engine->ParseFile(pFile, szToken, iTokenSize);
		// {
		pFile = engine->ParseFile(pFile, szToken, iTokenSize);

		pFile = engine->ParseFile(pFile, szToken, iTokenSize);
		while (!FStrEq(szToken, "}"))
		{
			g_aCinematics.AddToTail();
			CCinematicInfo* pInfo = &g_aCinematics[g_aCinematics.Count()-1];
			pInfo->m_sName = std::string(szToken);

			// {
			pFile = engine->ParseFile(pFile, szToken, iTokenSize);

			pFile = engine->ParseFile(pFile, szToken, iTokenSize);
			while (!FStrEq(szToken, "}"))
			{
				if (FStrEq(szToken, "precache"))
				{
					// {
					pFile = engine->ParseFile(pFile, szToken, iTokenSize);

					pFile = engine->ParseFile(pFile, szToken, iTokenSize);
					while (!FStrEq(szToken, "}"))
					{
						char szToken2[iTokenSize];
						pFile = engine->ParseFile(pFile, szToken2, iTokenSize);
						if (FStrEq(szToken, "model"))
							pInfo->m_asModelPrecaches.AddToTail(std::string(szToken2));
						else if (FStrEq(szToken, "particle"))
							pInfo->m_asParticlePrecaches.AddToTail(std::string(szToken2));

						pFile = engine->ParseFile(pFile, szToken, iTokenSize);
					}
				}
				pFile = engine->ParseFile(pFile, szToken, iTokenSize);
			}
			pFile = engine->ParseFile(pFile, szToken, iTokenSize);
		}

		UTIL_FreeFile( pBytes );
	}

	for (int i = 0; i < g_aCinematics.Count(); i++)
	{
		// If cf_cinematic is set then we want to preload models for that cinematic.
		if (FStrEq(g_aCinematics[i].m_sName.c_str(), cf_cinematic.GetString()))
		{
			int j;
			for (j = 0; j < g_aCinematics[i].m_asModelPrecaches.Count(); j++)
				CBaseEntity::PrecacheModel(g_aCinematics[i].m_asModelPrecaches[j].c_str());

			for (j = 0; j < g_aCinematics[i].m_asParticlePrecaches.Count(); j++)
				PrecacheParticleSystem(g_aCinematics[i].m_asParticlePrecaches[j].c_str());

			break;
		}
	}
}

void CCinematicManager::FrameUpdatePostEntityThink()
{
	if ( !m_pCinematic )
		return;

	float flFrametime = min( 0.1, gpGlobals->frametime );
	m_pCinematic->Think(flFrametime);

	if (m_pCinematic->IsFinished())
		UnloadCinematic();
}

void CCinematicManager::LoadCinematic( const char* pszCinematicName )
{
	UnloadCinematic();

	if (!filesystem->FileExists(VarArgs("cinematics/%s.txt", pszCinematicName)))
	{
		Warning("Can't find that cinematic.\n");
		return;
	}

	m_pCinematic = new CCinematic(pszCinematicName, this);
}

void CCinematicManager::UnloadCinematic()
{
	if (m_pCinematic)
	{
		m_pCinematic->Destruct();
		delete m_pCinematic;
		m_pCinematic = NULL;
	}
}

float CCinematicManager::PlayCinematic(const char* pszCinematicName)
{
	if (m_pCinematic)
		UnloadCinematic();

	LoadCinematic(pszCinematicName);

	if (!m_pCinematic)
		return -1;

	float flLength = m_pCinematic->Play();

	return flLength;
}

CCinematicManager* CinematicManager()
{
	return &CCinematicManager::s_oCinematicManager;
}

void CC_PlayCinematic(const CCommand &args)
{
	if (args.ArgC() <= 1)
	{
		DevMsg("What cinematic?\n");
		return;
	}

	float flSceneLength = CinematicManager()->PlayCinematic(args[1]);

	if (flSceneLength < 0)
	{
		DevMsg("Can't find that cinematic.\n");
		return;
	}

	if (FStrEq(args[0], "cinematic_record"))
	{
		for (int i = 1; i < gpGlobals->maxClients; i++)
		{
			if (!UTIL_PlayerByIndex(i))
				continue;

			CCFPlayer* pPlayer = ToCFPlayer(UTIL_PlayerByIndex(i));

			pPlayer->CameraCinematic(flSceneLength);
		}
	}
}

static ConCommand cinematic_play("cinematic_play", CC_PlayCinematic, "Play a scene." );
static ConCommand cinematic_record("cinematic_record", CC_PlayCinematic, "Play a scene, but view it from the point of view of the camera." );

void CCFPlayer::CameraCinematic(float flSceneLength)
{
	CBaseEntity* pEnt = gEntList.FindEntityByName( NULL, "camera" );

	if (pEnt)
	{
		m_hCameraCinematic = pEnt;
		m_flCameraCinematicUntil = gpGlobals->curtime + flSceneLength + 1;
	}
};