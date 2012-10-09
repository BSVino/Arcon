//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include "in_buttons.h"
#include "tempentity.h"
#include "takedamageinfo.h"
#include "shot_manipulator.h"

#if defined( CLIENT_DLL )

	#include "c_cf_player.h"
	#include "cf_in_main.h"

#else

	#include "cf_player.h"
	#include "baseentity.h"
	#include "soundent.h"
	#include "te_effect_dispatch.h"

#endif

#include "particle_parse.h"
#include "weapon_draingrenade.h"
#include "cf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define GRENADE_DRAIN_FOCUS 50.0f
#define GRENADE_DRAIN_RADIUS 450.0f
ConVar cf_minfocusdrain("cf_minfocusdrain", "-15", FCVAR_CHEAT|FCVAR_DEVELOPMENTONLY);

#define GRENADE_DRAIN_MODEL "models/weapons/draingrenade.mdl"

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponDrainGrenade, DT_WeaponDrainGrenade )

LINK_ENTITY_TO_CLASS( drain_grenade, CWeaponDrainGrenade );
PRECACHE_REGISTER( drain_grenade );

#ifdef GAME_DLL
	BEGIN_DATADESC( CWeaponDrainGrenade )
		DEFINE_ENTITYFUNC(DetonateTouch),
	END_DATADESC()
#endif

BEGIN_NETWORK_TABLE( CWeaponDrainGrenade, DT_WeaponDrainGrenade )
	#ifdef CLIENT_DLL
		RecvPropFloat( RECVINFO( m_flRealDamage ) ),
	#else
		SendPropFloat( SENDINFO( m_flRealDamage ), 10, SPROP_ROUNDDOWN, -2048.0, 2048.0 ),
	#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponDrainGrenade )
END_PREDICTION_DATA()


#ifdef GAME_DLL
CWeaponDrainGrenade* CWeaponDrainGrenade::Create( const Vector &position, const QAngle &angles, 
													const Vector &velocity, CBaseCombatCharacter *pOwner)
{
	CWeaponDrainGrenade *pGrenade = (CWeaponDrainGrenade*)CBaseEntity::Create( "drain_grenade", position, angles, pOwner );
	
	// Set the timer for 1 second less than requested. We're going to issue a SOUND_DANGER
	// one second before detonation.

	pGrenade->SetDetonateTimerLength( 1.5 );
	pGrenade->SetAbsVelocity( velocity );
	pGrenade->SetLocalAngularVelocity( QAngle( random->RandomFloat ( -100, -500 ), 0, 0 ) );
	pGrenade->SetupInitialTransmittedGrenadeVelocity( velocity );
	pGrenade->SetThrower( pOwner ); 

	pGrenade->SetGravity( BaseClass::GetGrenadeGravity() );
	pGrenade->SetFriction( BaseClass::GetGrenadeFriction() );
	pGrenade->SetElasticity( BaseClass::GetGrenadeElasticity() );

	//TODO: Define constants.
	pGrenade->SetDamage(100.0f);
	pGrenade->SetFocusDrain( GRENADE_DRAIN_FOCUS );
	pGrenade->SetDamageRadius( GRENADE_DRAIN_RADIUS );
	//pGrenade->SetDamageRadius(pGrenade->GetDamage() * 3.5f);
	pGrenade->ChangeTeam( pOwner->GetTeamNumber() );

	// make NPCs afaid of it while in the air
	pGrenade->SetThink( &CBaseGrenadeProjectile::DangerSoundThink );
	pGrenade->SetNextThink( gpGlobals->curtime );

	pGrenade->SetTouch( &CWeaponDrainGrenade::DetonateTouch );

	return pGrenade;
}

void CWeaponDrainGrenade::Spawn()
{
	SetModel(GRENADE_DRAIN_MODEL);

	BaseClass::Spawn();
}

void CWeaponDrainGrenade::Precache()
{
	//TODO: Rename this particle.
	PrecacheParticleSystem( "grenade_particle" );

	//TODO: Tweak grenade_explosion particle.
	PrecacheParticleSystem( "grenade_explosion" );

	//TODO: Implement a explosion sound.
	PrecacheScriptSound( "Numen.FireAoE" );

	PrecacheModel( GRENADE_DRAIN_MODEL );

	BaseClass::Precache();
}

void CWeaponDrainGrenade::DetonateTouch( CBaseEntity *pOther )
{
	if ( pOther == GetThrower() )
		return;

	// Verify a correct "other."
	if ( !pOther->IsSolid() || pOther->IsSolidFlagSet( FSOLID_VOLUME_CONTENTS ) )
		return;

	// Handle hitting skybox (disappear).
	trace_t pTrace;
	Vector velDir = GetAbsVelocity();
	VectorNormalize( velDir );
	Vector vecSpot = GetAbsOrigin() - velDir * 32;
	UTIL_TraceLine( vecSpot, vecSpot + velDir * 64, MASK_SOLID, this, COLLISION_GROUP_NONE, &pTrace );

	if ( pTrace.fraction < 1.0 && pTrace.surface.flags & SURF_SKY )
	{
		UTIL_Remove( this );
		return;
	}

	Detonate();
}

void CWeaponDrainGrenade::BounceSound( void )
{
	//TODO: Implement a bounce sound.
	//EmitSound( "Numen.GrenadeBounce" );
}

void CWeaponDrainGrenade::RemoveMe()
{
	StopParticleEffects( this );
	SetModelName( NULL_STRING );//invisible
	AddSolidFlags( FSOLID_NOT_SOLID );

	m_takedamage = DAMAGE_NO;

	SetThink( &CBaseGrenade::SUB_Remove );
	SetTouch( NULL );
	SetSolid( SOLID_NONE );
	
	AddEffects( EF_NODRAW );
	SetAbsVelocity( vec3_origin );

	SetNextThink( gpGlobals->curtime + 0.1 );
}

void CWeaponDrainGrenade::Detonate()
{
	trace_t		tr;
	Vector		vecSpot;// trace starts here!

	SetThink( NULL );

	vecSpot = GetAbsOrigin() + Vector ( 0 , 0 , 8 );
	UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -32 ), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, & tr);

	if( tr.startsolid )
	{
		// Since we blindly moved the explosion origin vertically, we may have inadvertently moved the explosion into a solid,
		// in which case nothing is going to be harmed by the grenade's explosion because all subsequent traces will startsolid.
		// If this is the case, we do the downward trace again from the actual origin of the grenade. (sjb) 3/8/2007  (for ep2_outland_09)
		UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + Vector( 0, 0, -32), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr );
	}

#if !defined( CLIENT_DLL )

	Explode( GetThrower(), 
			this, 
			NULL, 
			tr.endpos, 
			GetDamage(), 
			GetDamageRadius(),
			GetFocusDrain());

	//CSoundEnt::InsertSound ( SOUND_COMBAT, GetAbsOrigin(), BASEGRENADE_EXPLOSION_VOLUME, 3.0 );

#endif //!defined( CLIENT_DLL )

	RemoveMe();
} 

int CWeaponDrainGrenade::UpdateTransmitState( void)
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

void CWeaponDrainGrenade::RadiusDamage( const CTakeDamageInfo &inputInfo, const Vector &vecSrcIn, float flRadius )
{
	CTakeDamageInfo info = inputInfo;

	CBaseEntity *pEntity = NULL;
	trace_t		tr;
	float		flAdjustedDamage, falloff;
	Vector		vecSpot;
	Vector		vecToTarget;
	Vector		vecEndPos;

	Vector vecSrc = vecSrcIn;

	if ( flRadius )
		falloff = info.GetDamage() / flRadius;
	else
		falloff = 1.0;

	int bInWater = (UTIL_PointContents ( vecSrc ) & MASK_WATER) ? true : false;
	
	vecSrc.z += 1;// in case grenade is lying on the ground

	// iterate on all entities in the vicinity.
	for ( CEntitySphereQuery sphere( vecSrc, flRadius ); ( pEntity = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
	{
		if (!pEntity || !ToCFPlayer(pEntity))
			continue;

		// Don't drain teammates.
		if (CFGameRules()->PlayerRelationship(GetThrower(), pEntity) == GR_TEAMMATE && ToCFPlayer(pEntity) != ToCFPlayer(GetThrower()))
			continue;

		if ( !pEntity->IsAlive() )
			continue;

		if ( pEntity->m_takedamage != DAMAGE_NO )
		{

			// blast's don't tavel into or out of water
			if (bInWater && pEntity->GetWaterLevel() == 0)
				continue;
			if (!bInWater && pEntity->GetWaterLevel() == 3)
				continue;

			// radius damage can only be blocked by the world
			vecSpot = pEntity->BodyTarget( vecSrc );

			bool bHit = false;

			UTIL_TraceLine( vecSrc, vecSpot, MASK_SOLID_BRUSHONLY, info.GetInflictor(), COLLISION_GROUP_NONE, &tr );

			if (tr.startsolid)
			{
				// if we're stuck inside them, fixup the position and distance
				tr.endpos = vecSrc;
				tr.fraction = 0.0;
			}

			vecEndPos = tr.endpos;

			if( tr.fraction == 1.0 || tr.m_pEnt == pEntity )
			{
				bHit = true;
			}

			if ( bHit )
			{
				// the explosion can 'see' this entity, so hurt them!
				vecToTarget = ( vecEndPos - vecSrc );

				// decrease damage for an ent that's farther from the blast's center.
				flAdjustedDamage = vecToTarget.Length() * falloff;
				flAdjustedDamage = info.GetDrainFocus() - flAdjustedDamage;

				if ( flAdjustedDamage > 0 )
				{
					CTakeDamageInfo adjustedInfo = info;
					adjustedInfo.SetDrainFocus( flAdjustedDamage );

					Vector dir = vecToTarget;
					VectorNormalize( dir );

					// If we don't have a damage force, manufacture one
					if ( adjustedInfo.GetDamagePosition() == vec3_origin || adjustedInfo.GetDamageForce() == vec3_origin )
					{
						CalculateExplosiveDamageForce( &adjustedInfo, dir, vecSrc, 1.5	/* explosion scale! */ );
					}
					else
					{
						// Assume the force passed in is the maximum force. Decay it based on falloff.
						float flForce = adjustedInfo.GetDamageForce().Length() * falloff;
						adjustedInfo.SetDamageForce( dir * flForce );
						adjustedInfo.SetDamagePosition( vecSrc );
					}

					float flFocusDrained = adjustedInfo.GetDrainFocus();

					float& flFocus = ToCFPlayer(pEntity)->m_pStats->m_flFocus.GetForModify();
					if (flFocus > cf_minfocusdrain.GetFloat())
						flFocus -= flFocusDrained;
					if (flFocus < cf_minfocusdrain.GetFloat())
						flFocus = cf_minfocusdrain.GetFloat();

					CEffectData	data;

					data.m_nHitBox = GetParticleSystemIndex( "grenade_drained" );
					data.m_vOrigin = ToCFPlayer(pEntity)->GetCentroid();
					data.m_vStart = vecSrc;
					data.m_vAngles = QAngle(0,0,0);

					data.m_nEntIndex = pEntity->entindex();
					data.m_fFlags |= PARTICLE_DISPATCH_FROM_ENTITY;
					data.m_nDamageType = PATTACH_CUSTOMORIGIN;

					DispatchEffect( "ParticleEffect", data );

					// Now hit all triggers along the way that respond to damage... 
					pEntity->TraceAttackToTriggers( adjustedInfo, vecSrc, vecEndPos, dir );
				}
			}
		}
	}
}
#endif //GAME_DLL

#ifdef CLIENT_DLL
void CWeaponDrainGrenade::OnDataChanged( DataUpdateType_t updateType )
{
	if (updateType == DATA_UPDATE_CREATED)
	{
		m_apEffects.AddToTail(ParticleProp()->Create( "grenade_particle", PATTACH_ABSORIGIN_FOLLOW ));
	}
}
#endif //CLIENT_DLL

void CWeaponDrainGrenade::Explode( CBaseCombatCharacter* pAttacker,
						   CBaseEntity* pInflictor,
						   CBaseEntity* pIgnore,
						   Vector vecPosition,
						   float flDamage,
						   float flRadius,
						   float flDrainFocus )
{

#if !defined( CLIENT_DLL )
	//CSoundEnt::InsertSound ( SOUND_COMBAT, vecPosition, 1024, 3.0 );
	
	CTakeDamageInfo info;
	info.CFSet( pInflictor, pAttacker, vec3_origin, vecPosition, flDamage, DMG_BLAST, WEAPON_NONE, &vecPosition, false );
	info.SetDrainFocus( flDrainFocus );

	DispatchParticleEffect("grenade_explosion", vecPosition, vec3_angle, pIgnore);

	//TODO: Figure out sound.
	CPASAttenuationFilter filter( this );
	filter.UsePredictionRules();
	EmitSound( filter, entindex(), "Numen.FireAoE" );
	//CSoundEnt::InsertSound ( SOUND_COMBAT, GetAbsOrigin(), BASEGRENADE_EXPLOSION_VOLUME, 3.0 );
	
	RadiusDamage( info, vecPosition, flRadius );

#endif //!defined( CLIENT_DLL )
}
