//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "objectives.h"
#include "player.h"
#include "team.h"
#include "cf_gamerules.h"
#include "statistics.h"
#include "cf_team.h"
#include "weapon_flag.h"
#include "mp_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( info_game, CInfoGame );

BEGIN_DATADESC( CInfoGame )
	DEFINE_KEYFIELD( m_iszTeamNames[TEAM_NUMENI],	FIELD_STRING,	"team1name" ),
	DEFINE_KEYFIELD( m_iszTeamNames[TEAM_MACHINDO],	FIELD_STRING,	"team2name" ),
	//DEFINE_KEYFIELD( m_iszTeamNames[TEAM_SECT],		FIELD_STRING,	"team3name" ),
	//DEFINE_KEYFIELD( m_iszTeamNames[TEAM_FACTION],	FIELD_STRING,	"team4name" ),
END_DATADESC()

extern ConVar cf_ctf_tugofwar;

void CInfoGame::LevelInitPostEntity()
{
	// Lets see, what do we have here?
	CTeam* pTeam;

	for (int i = 0; i <= GetNumberOfTeams(); i++)
	{
		pTeam = GetGlobalTeam(i);
		if (!pTeam)
			continue;

		if (STRING(m_iszTeamNames[i])[0])
			pTeam->SetName(m_iszTeamNames[i]);
	}
}

bool CInfoGame::SupportsGameMode(CFGameType eGameMode)
{
	return !!(GetSpawnFlags() & CCFGameRules::GameModeSpawnFlag(eGameMode));
}

LINK_ENTITY_TO_CLASS( info_objective, CInfoObjective );

IMPLEMENT_SERVERCLASS_ST( CInfoObjective, DT_InfoObjective )
	SendPropEHandle( SENDINFO( m_hPlayer ) ),
	SendPropBool( SENDINFO( m_bAtSpawn ) ),
END_SEND_TABLE()

BEGIN_DATADESC( CInfoObjective )
	DEFINE_FUNCTION( SpawnThink ),
	DEFINE_FUNCTION( DroppedThink ),
	DEFINE_FUNCTION( NeutralizeThink ),
	DEFINE_FUNCTION( ObjectiveTouch ),

	DEFINE_FIELD( m_vecOriginalPosition, FIELD_VECTOR ),
	DEFINE_FIELD( m_vecReturnPosition, FIELD_VECTOR ),

	DEFINE_KEYFIELD( m_bPowerjumping,	FIELD_BOOLEAN,	"powerjumping" ),

	DEFINE_OUTPUT( m_OnPickup, "OnPickup"),
	DEFINE_OUTPUT( m_OnDrop, "OnDrop"),
	DEFINE_OUTPUT( m_OnCapture, "OnCapture"),
	DEFINE_OUTPUT( m_OnReturn, "OnReturn"),

	DEFINE_INPUTFUNC( FIELD_VOID, "Respawn", InputRespawn ),

	DEFINE_FIELD( m_hPlayer, FIELD_EHANDLE ),
END_DATADESC()

CInfoObjective::CInfoObjective ( void )
{
	m_TeamData.SetSize( GetNumberOfTeams() );
}

void CInfoObjective::Precache ( void )
{
	for ( int i = 0; i < m_TeamData.Count(); i++ )
	{
		// Skip over spectator
		if ( i == TEAM_SPECTATOR )
			continue;

		if ( m_TeamData[i].iszModel != NULL_STRING )
		{
			PrecacheModel( STRING(m_TeamData[i].iszModel) );
		}
	}

	PrecacheScriptSound( "CaptureFlag.MachindoOurFlagStolen" );
	PrecacheScriptSound( "CaptureFlag.MachindoOurFlagDropped" );
	PrecacheScriptSound( "CaptureFlag.MachindoOurFlagRecovered" );
	PrecacheScriptSound( "CaptureFlag.MachindoOurFlagCaptured" );
	PrecacheScriptSound( "CaptureFlag.MachindoEnemyFlagStolen" );
	PrecacheScriptSound( "CaptureFlag.MachindoEnemyFlagDropped" );
	PrecacheScriptSound( "CaptureFlag.MachindoEnemyFlagRecovered" );
	PrecacheScriptSound( "CaptureFlag.MachindoEnemyFlagCaptured" );
	PrecacheScriptSound( "CaptureFlag.NumeniOurFlagStolen" );
	PrecacheScriptSound( "CaptureFlag.NumeniOurFlagDropped" );
	PrecacheScriptSound( "CaptureFlag.NumeniOurFlagRecovered" );
	PrecacheScriptSound( "CaptureFlag.NumeniOurFlagCaptured" );
	PrecacheScriptSound( "CaptureFlag.NumeniEnemyFlagStolen" );
	PrecacheScriptSound( "CaptureFlag.NumeniEnemyFlagDropped" );
	PrecacheScriptSound( "CaptureFlag.NumeniEnemyFlagRecovered" );
	PrecacheScriptSound( "CaptureFlag.NumeniEnemyFlagCaptured" );
}

void CInfoObjective::Spawn( void )
{
	Precache();
	SetSolid( SOLID_NONE );

	SetModel( STRING(m_TeamData[GetTeamNumber()].iszModel) );

	SetThink (&CInfoObjective::SpawnThink);
	SetNextThink( gpGlobals->curtime + 0.5f );

	m_vecOriginalPosition = GetAbsOrigin();
	SetReturnPosition(m_vecOriginalPosition);

	m_bAtSpawn = true;
}

bool CInfoObjective::KeyValue( const char *szKeyName, const char *szValue )
{	
	if ( !Q_strncmp( szKeyName, "team_model_", 11 ) )
	{
		int iTeam = atoi(szKeyName+11);
		Assert( iTeam >= 0 && iTeam < m_TeamData.Count() );

		m_TeamData[iTeam].iszModel = AllocPooledString(szValue);
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}

void CInfoObjective::SpawnThink ( void )
{
	SetModel( STRING(m_TeamData[GetTeamNumber()].iszModel) );

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_TRIGGER|FSOLID_NOT_SOLID );
	SetMoveType( MOVETYPE_NONE );
	UTIL_SetSize ( this, Vector ( -16, -16, -16 ), Vector ( 16, 16, 16 ) );

	// Go up look down to see where the ground is, resting a bit on top of it.
	trace_t tr;
	UTIL_TraceHull( m_vecReturnPosition + Vector(0, 0, 100), m_vecReturnPosition - Vector(0, 0, 200), Vector(-16, -16, -42), Vector(16, 16, 16), MASK_PLAYERSOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr);

	SetAbsOrigin(tr.endpos);
	SetAbsVelocity(vec3_origin);

	m_bAtSpawn = true;

	SetThink ( NULL );
	SetTouch ( &CInfoObjective::ObjectiveTouch );

	CFGameRules()->AddFlagToWell(this);

	if (cf_ctf_tugofwar.GetBool())
	{
		// Determine who can pick up the flag now.
		int iPickupTeams = SF_MACHINDO|SF_NUMENI;
		int iOwningTeam = 0;

		if (GetTeamNumber() == TEAM_NUMENI)
			iOwningTeam = SF_NUMENI;
		else if (GetTeamNumber() == TEAM_MACHINDO)
			iOwningTeam = SF_MACHINDO;

		iPickupTeams &= ~iOwningTeam;

		RemoveSpawnFlags(iOwningTeam);
		AddSpawnFlags(iPickupTeams);
	}
}

void CInfoObjective::ObjectiveTouch ( CBaseEntity *pOther )
{
	if ( !pOther->IsPlayer() )
		return;

	CCFPlayer *pPlayer = ToCFPlayer(pOther);

	if (!pPlayer->IsAlive())
		return;

	if (!cf_ctf_tugofwar.GetBool())
	{
		if (!m_bAtSpawn && CFGameRules()->PlayerRelationship(pPlayer, this) == GR_TEAMMATE)
		{
			pPlayer->AwardEloPoints(SCORE_RETURN_FLAG);	// Award points.
			Return(false);	// Return the flag.
			return;			// Return the function.
		}
	}

	// Points, flags, whatever.
	if (!CFGameRules()->PointsMayBeCaptured())
		return;

	if (!(GetSpawnFlags() & SF_NUMENI) && pPlayer->GetTeamNumber() == TEAM_NUMENI)
		return;

	if (!(GetSpawnFlags() & SF_MACHINDO) && pPlayer->GetTeamNumber() == TEAM_MACHINDO)
		return;

//	if (!(GetSpawnFlags() & SF_SECT) && pPlayer->GetTeamNumber() == TEAM_SECT)
//		return;

//	if (!(GetSpawnFlags() & SF_FACTION) && pPlayer->GetTeamNumber() == TEAM_FACTION)
//		return;

	if (pPlayer->HasObjective())
		return;

	SetSolid( SOLID_NONE );
	SetTouch ( NULL );

	FollowEntity( pOther );
	pPlayer->SetObjective(this);

	m_bAtSpawn = false;

	m_OnPickup.FireOutput(pPlayer, this);

	if (cf_ctf_tugofwar.GetBool())
	{
		CFGameRules()->RemoveFlagFromWell(this);
		AddSpawnFlags(SF_MACHINDO|SF_NUMENI);

		SetThink(&CInfoObjective::NeutralizeThink);
		SetNextThink(gpGlobals->curtime + 3.0f);
	}

	char szModifiers[256];
	Q_strcpy(szModifiers, VarArgs("flagteam:%s,carrierteam:%s", GetTeam()->GetName(), pPlayer->GetTeam()->GetName()));
	CFGameRules()->SpeakConceptFromTeamLeaders(MP_CONCEPT_FLAGPICKUP, szModifiers);
}

void CInfoObjective::NeutralizeThink ( void )
{
	ChangeTeam(TEAM_UNASSIGNED);	// Once a flag is grabbed, it belongs to nobody, it must be carried back to base
	SetModel( STRING(m_TeamData[GetTeamNumber()].iszModel) );
	SetReturnPosition(m_vecOriginalPosition);
}

void CInfoObjective::DroppedThink ( void )
{
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_TRIGGER|FSOLID_NOT_SOLID );
	SetNextThink( gpGlobals->curtime + 60.0f );
	SetThink ( &CInfoObjective::SpawnThink );
	SetTouch ( &CInfoObjective::ObjectiveTouch );
}

void CInfoObjective::Drop ( bool bThrow )
{
	if (m_hPlayer != NULL)
	{
		char szModifiers[256];
		Q_strcpy(szModifiers, VarArgs("carrierteam:%s", m_hPlayer->GetTeam()->GetName()));
		CFGameRules()->SpeakConceptFromTeamLeaders(MP_CONCEPT_FLAGDROPPED, szModifiers);
	}

	Vector vecPos = GetHoldingPosition(m_hPlayer);

	// Go up look down to see where the ground is, resting a bit on top of it.
	trace_t tr;
	UTIL_TraceHull( vecPos + Vector(0, 0, 100), vecPos - Vector(0, 0, 200), Vector(-16, -16, -42), Vector(16, 16, 16), MASK_PLAYERSOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr);

	if (enginetrace->GetPointContents(tr.endpos) & CONTENTS_SOLID)
		SetAbsOrigin(m_hPlayer->GetCentroid());
	else
		SetAbsOrigin(tr.endpos);

	// Why? I don't know. But it's happened before so I put in a guard.
	if (m_hPlayer.Get())
	{
		m_OnDrop.FireOutput(m_hPlayer, this);
		m_hPlayer->SetObjective(NULL);
	}

	RemoveSolidFlags( FSOLID_TRIGGER );
	FollowEntity( NULL );

	SetTouch(NULL);
	SetThink(&CInfoObjective::DroppedThink);
	SetNextThink( gpGlobals->curtime + 2.0f );
}

void CInfoObjective::Captured ( void )
{
	m_OnCapture.FireOutput(m_hPlayer, this);

	char szModifiers[256];
	Q_strcpy(szModifiers, VarArgs("carrierteam:%s", m_hPlayer->GetTeam()->GetName()));
	CFGameRules()->SpeakConceptFromTeamLeaders(MP_CONCEPT_FLAGCAPTURED, szModifiers);

	m_hPlayer->AwardEloPoints(SCORE_CAPTURE_FLAG);

	CSingleUserRecipientFilter user( m_hPlayer );
	user.MakeReliable();
	UserMessageBegin( user, "CFMessage" );
		WRITE_BYTE( MESSAGE_FLAGCAPTURED );
	MessageEnd();

	m_hPlayer->Instructor_LessonLearned(HINT_COLLECT_ALL_5, true);

	if (cf_ctf_tugofwar.GetBool())
	{
		ChangeTeam(m_hPlayer->GetTeamNumber());
		SetModel( STRING(m_TeamData[GetTeamNumber()].iszModel) );
		CFGameRules()->AddFlagToWell(this);
	}

	Return(true);
}

void CInfoObjective::Return(bool bCaptured)
{
	RemoveSolidFlags( FSOLID_TRIGGER );

	if (!bCaptured && !cf_ctf_tugofwar.GetBool())
	{
		// VO's used to exist here, but they were replaced with the ai speaker system.
		// Should we ever go back to non tug of war ctf, then this is where they would go back.
	}

	m_OnReturn.FireOutput(m_hPlayer, this);

	if (cf_ctf_tugofwar.GetBool())
		CFGameRules()->AddFlagToWell(this);

	FollowEntity( NULL );
	if (m_hPlayer.Get())
		m_hPlayer->SetObjective(NULL);

	SetThink (&CInfoObjective::SpawnThink);
	SetNextThink( gpGlobals->curtime + 1.0f );
}

int CInfoObjective::UpdateTransmitState()
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}

void CInfoObjective::InputRespawn( inputdata_t &inputdata )
{
	// Abandon the player who was carrying us and return to the spawn.
	Return(false);
}

LINK_ENTITY_TO_CLASS( info_objective_flag, CInfoFlag );

BEGIN_DATADESC( CInfoFlag )
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_objective, CTriggerObjective );

BEGIN_DATADESC( CTriggerObjective )
	DEFINE_FUNCTION( ObjectiveTouch ),
	DEFINE_FUNCTION( MultiWaitOver ),

	DEFINE_OUTPUT(m_OnObjective, "OnObjective")
END_DATADESC()

void CTriggerObjective::Spawn( void )
{
	BaseClass::Spawn();

	InitTrigger();

	if (m_flWait == 0)
	{
		m_flWait = 0.2;
	}

	// It will throw out non-players anyway.
	AddSpawnFlags(SF_TRIGGER_ALLOW_ALL);

	ASSERTSZ(m_iHealth == 0, "trigger_objective with health");
	SetTouch( &CTriggerObjective::ObjectiveTouch );
}

void CTriggerObjective::ObjectiveTouch ( CBaseEntity *pOther )
{
	if (PassesTriggerFilters(pOther))
	{
		ActivateMultiTrigger( pOther );
	}
}

void CTriggerObjective::ActivateMultiTrigger(CBaseEntity *pOther)
{
	if (GetNextThink() > gpGlobals->curtime)
		return;         // still waiting for reset time

	if ( !pOther->IsPlayer() )
		return;

	CCFPlayer *pPlayer = ToCFPlayer(pOther);

	if ((GetSpawnFlags() & SF_REQUIRE_FLAG) && !pPlayer->HasObjective())
		return;

	m_OnObjective.FireOutput(pOther, this);

	if (m_flWait > 0)
	{
		SetThink( &CTriggerObjective::MultiWaitOver );
		SetNextThink( gpGlobals->curtime + m_flWait );
	}
	else
	{
		// we can't just remove (self) here, because this is a touch function
		// called while C code is looping through area links...
		SetTouch( NULL );
		SetNextThink( gpGlobals->curtime + 0.1f );
		SetThink( &CTriggerObjective::SUB_Remove );
	}

	if (pPlayer->HasObjective())
		pPlayer->GetObjective()->Captured();
}

void CTriggerObjective::MultiWaitOver( void )
{
	SetThink( NULL );
}

LINK_ENTITY_TO_CLASS( info_score, CInfoScore );

BEGIN_DATADESC( CInfoScore )
	DEFINE_KEYFIELD( m_bRespawnPlayers, FIELD_BOOLEAN, "respawnplayers" ),

	DEFINE_FUNCTION( RespawnThink ),

	DEFINE_INPUTFUNC( FIELD_INTEGER, "Score", InputScore ),
END_DATADESC()

void CInfoScore::InputScore( inputdata_t &inputdata )
{
	int flTeam = GetTeamNumber();
	if (flTeam == TEAM_UNASSIGNED)
		flTeam = inputdata.pActivator->GetTeamNumber();

	if ( CFGameRules()->GetGameMode() == CF_GAME_CTF )
	{
		CCFTeam* pTeam = GetGlobalCFTeam( flTeam );
		// Reward the team
		if ( pTeam )
			pTeam->IncrementFlagCaptures();
	}

	if (m_bRespawnPlayers)
	{
		SetNextThink( gpGlobals->curtime + 3.0f );
		SetThink ( &CInfoScore::RespawnThink );
	}
}

extern void respawn(CBaseEntity *pEdict, bool fCopyCorpse);

void CInfoScore::RespawnThink()
{
	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if (pPlayer)
			respawn(pPlayer, false);
	}
}

void CCFPlayer::SetObjective(CInfoObjective* pObjective)
{
	if (pObjective)
	{
		// Put away all existing weapons, don't delete them though.
		for (int i = 0; i < m_hWeapons.Count(); i++)
			if (GetWeapon(i))
				GetWeapon(i)->Holster();
	}
	else
	{
		for (int i = 0; i < 3; i++)
		{
			CWeaponCFBase* pCFWeapon = GetWeapon(i);

			if (!pCFWeapon)
				continue;

			CWeaponFlag* pWeapon = dynamic_cast<CWeaponFlag*>(pCFWeapon);

			if (!pWeapon)
				continue;

			Weapon_Drop(pWeapon);
			UTIL_Remove(pWeapon);
		}
	}

	if (m_hObjective.Get())
		m_hObjective->m_hPlayer = NULL;

	m_hObjective = pObjective;
	if (m_hObjective.Get())
	{
		m_hObjective->m_hPlayer = this;
		if (!m_hObjective->m_bPowerjumping)
			m_bCanPowerjump = false;
	}
	else
	{
		m_bCanPowerjump = true;
	}

	m_pStats->ResetOverdrive();

	if (pObjective)
	{
		CWeaponFlag* pFlagWeapon = static_cast<CWeaponFlag*>(CreateEntityByName( "weapon_flag" ));
		pFlagWeapon->Spawn();
		pFlagWeapon->Activate();
		Weapon_Equip(pFlagWeapon, 0);
	}
	else
	{
		for (int i = 0; i < m_hWeapons.Count(); i++)
			if (GetWeapon(i))
				GetWeapon(i)->Deploy();
	}
}

LINK_ENTITY_TO_CLASS( cf_trigger_capture_area, CCFAreaCapture );

BEGIN_DATADESC( CCFAreaCapture )
END_DATADESC()

void CCFAreaCapture::Spawn( void )
{
	BaseClass::Spawn();
}

LINK_ENTITY_TO_CLASS( cf_team_control_point, CCFControlPoint );

BEGIN_DATADESC( CCFControlPoint )
END_DATADESC()

void CCFControlPoint::PlayerCapped( CBaseMultiplayerPlayer *pPlayer )
{
	BaseClass::PlayerCapped(pPlayer);

	ToCFPlayer(pPlayer)->AwardEloPoints(SCORE_CAPTURE_POINT);
}

void CCFControlPoint::PlayerBlocked( CBaseMultiplayerPlayer *pPlayer )
{
	BaseClass::PlayerBlocked(pPlayer);

	ToCFPlayer(pPlayer)->AwardEloPoints(SCORE_BLOCK_CAPTURE);
}

LINK_ENTITY_TO_CLASS( cf_team_control_point_master, CCFControlPointMaster );

BEGIN_DATADESC( CCFControlPointMaster )
END_DATADESC()

