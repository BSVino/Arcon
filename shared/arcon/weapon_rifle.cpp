//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "weapon_rifle.h"
#include "weapon_magic.h"
#include "in_buttons.h"
#include "tempentity.h"
#include "takedamageinfo.h"
#include "shot_manipulator.h"
#include "weapon_draingrenade.h"

#ifdef CLIENT_DLL
#include "prediction.h"
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponRifle, DT_WeaponRifle )

BEGIN_NETWORK_TABLE( CWeaponRifle, DT_WeaponRifle )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponRifle )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_rifle, CWeaponRifle );
PRECACHE_WEAPON_REGISTER( weapon_rifle );

#define SECONDARY_FIRE_RATE 1.5f

acttable_t	CWeaponRifle::s_CommonActions[] = 
{
	{ ACT_CF_EXECUTE_ANYTHING,			ACT_CF_EXECUTE_UNARMED_UNARMED,			false },
	{ ACT_CF_EXECUTED_ANYTHING,			ACT_CF_EXECUTED_UNARMED_UNARMED,		false },
	{ ACT_CF_LATCH_JUMP_START,			ACT_CF_LATCH_JUMP_START_RIFLE,			false },
	{ ACT_CF_LATCH_LEFT,				ACT_CF_LATCH_LEFT_RIFLE,				false },
	{ ACT_CF_LATCH_LEFT_RELOAD,			ACT_CF_LATCH_LEFT_RELOAD_RIFLE,			false },
	{ ACT_CF_LATCH_LEFT_RANGE_ATTACK,	ACT_CF_LATCH_LEFT_RANGE_ATTACK_RIFLE,	false },
	{ ACT_CF_LATCH_RIGHT,				ACT_CF_LATCH_RIGHT_RIFLE,				false },
	{ ACT_CF_LATCH_RIGHT_RELOAD,		ACT_CF_LATCH_RIGHT_RELOAD_RIFLE,		false },
	{ ACT_CF_LATCH_RIGHT_RANGE_ATTACK,	ACT_CF_LATCH_RIGHT_RANGE_ATTACK_RIFLE,	false },
	{ ACT_CF_LATCH_BACK,				ACT_CF_LATCH_BACK_RIFLE,				false },
	{ ACT_CF_LATCH_BACK_RELOAD,			ACT_CF_LATCH_BACK_RELOAD_RIFLE,			false },
	{ ACT_CF_LATCH_BACK_RANGE_ATTACK,	ACT_CF_LATCH_BACK_RANGE_ATTACK_RIFLE,	false },
};

acttable_t	CWeaponRifle::s_RifleActions[] = 
{
	{ ACT_MP_RUN,						ACT_CF_RUN_RIFLE,						false },
	{ ACT_MP_STAND_IDLE,				ACT_CF_IDLE_RIFLE,						false },
	{ ACT_MP_CROUCH_IDLE,				ACT_CF_IDLE_CROUCH_RIFLE,				false },
	{ ACT_MP_CROUCHWALK,				ACT_CF_WALK_CROUCH_RIFLE,				false },
	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_CF_GESTURE_RANGE_ATTACK_RIFLE,		false },
	{ ACT_MP_RELOAD_STAND,				ACT_CF_GESTURE_RELOAD_RIFLE,			false },
	{ ACT_MP_JUMP_START,				ACT_CF_JUMP_START_RIFLE,				false },
	{ ACT_MP_JUMP_FLOAT,				ACT_CF_JUMP_FLOAT_RIFLE,				false },
	{ ACT_MP_JUMP_LAND,					ACT_CF_JUMP_LAND_RIFLE,					false },
	{ ACT_CF_POWERJUMP_FORWARD,			ACT_CF_POWERJUMP_FORWARD_RIFLE,			false },
	{ ACT_CF_POWERJUMP_UP,				ACT_CF_POWERJUMP_UP_RIFLE,				false },
	{ ACT_CF_POWERJUMP_LEFT,			ACT_CF_POWERJUMP_LEFT_RIFLE,			false },
	{ ACT_CF_POWERJUMP_RIGHT,			ACT_CF_POWERJUMP_RIGHT_RIFLE,			false },
	{ ACT_CF_POWERJUMP_BACK,			ACT_CF_POWERJUMP_BACK_RIFLE,			false },

	{ ACT_CF_IDLE_NUMEN_ACTIVE,					ACT_CF_IDLE_RIFLE_NUMEN,				false },
	{ ACT_CF_RUN_NUMEN_ACTIVE,					ACT_CF_RUN_RIFLE_NUMEN,					false },
	{ ACT_CF_CHARGEUP_NUMEN_ACTIVE,				ACT_CF_CHARGEUP_RIFLE_NUMEN,			false },
	{ ACT_CF_CAST_NUMEN_ACTIVE,					ACT_CF_CAST_RIFLE_NUMEN,				false },
	{ ACT_CF_IDLE_NUMEN_ACTIVE,					ACT_CF_IDLE_RIFLE_NUMEN,				false },
	{ ACT_CF_RUN_NUMEN_ACTIVE,					ACT_CF_RUN_RIFLE_NUMEN,					false },
	{ ACT_CF_JUMP_START_NUMEN_ACTIVE,			ACT_CF_JUMP_START_RIFLE_NUMEN,			false },
	{ ACT_CF_JUMP_FLOAT_NUMEN_ACTIVE,			ACT_CF_JUMP_FLOAT_RIFLE_NUMEN,			false },
	{ ACT_CF_JUMP_LAND_NUMEN_ACTIVE,			ACT_CF_JUMP_LAND_RIFLE_NUMEN,			false },
	{ ACT_CF_POWERJUMP_FORWARD_NUMEN_ACTIVE,	ACT_CF_POWERJUMP_FORWARD_RIFLE_NUMEN,	false },
	{ ACT_CF_POWERJUMP_UP_NUMEN_ACTIVE,			ACT_CF_POWERJUMP_UP_RIFLE_NUMEN,		false },
	{ ACT_CF_POWERJUMP_BACK_NUMEN_ACTIVE,		ACT_CF_POWERJUMP_BACK_RIFLE_NUMEN,		false },
	{ ACT_CF_POWERJUMP_LEFT_NUMEN_ACTIVE,		ACT_CF_POWERJUMP_LEFT_RIFLE_NUMEN,		false },
	{ ACT_CF_POWERJUMP_RIGHT_NUMEN_ACTIVE,		ACT_CF_POWERJUMP_RIGHT_RIFLE_NUMEN,		false },
	{ ACT_CF_JUMP_START_NUMEN_ACTIVE,			ACT_CF_JUMP_START_RIFLE_NUMEN,			false },
	{ ACT_CF_LATCH_LEFT_NUMEN_ACTIVE,			ACT_CF_LATCH_LEFT_RIFLE_NUMEN,			false },
	{ ACT_CF_LATCH_LEFT_CHARGEUP_NUMEN_ACTIVE,	ACT_CF_LATCH_LEFT_CHARGEUP_RIFLE_NUMEN,	false },
	{ ACT_CF_LATCH_LEFT_CAST_NUMEN_ACTIVE,		ACT_CF_LATCH_LEFT_CAST_RIFLE_NUMEN,		false },
	{ ACT_CF_LATCH_RIGHT_NUMEN_ACTIVE,			ACT_CF_LATCH_RIGHT_RIFLE_NUMEN,			false },
	{ ACT_CF_LATCH_RIGHT_CHARGEUP_NUMEN_ACTIVE,	ACT_CF_LATCH_RIGHT_CHARGEUP_RIFLE_NUMEN,false },
	{ ACT_CF_LATCH_RIGHT_CAST_NUMEN_ACTIVE,		ACT_CF_LATCH_RIGHT_CAST_RIFLE_NUMEN,	false },
	{ ACT_CF_LATCH_BACK_NUMEN_ACTIVE,			ACT_CF_LATCH_BACK_RIFLE_NUMEN,			false },
	{ ACT_CF_LATCH_BACK_CHARGEUP_NUMEN_ACTIVE,	ACT_CF_LATCH_BACK_CHARGEUP_RIFLE_NUMEN,	false },
	{ ACT_CF_LATCH_BACK_CAST_NUMEN_ACTIVE,		ACT_CF_LATCH_BACK_CAST_RIFLE_NUMEN,		false },
};

acttable_t	CWeaponRifle::s_RiflePistolActions[] = 
{
	{ ACT_MP_RUN,						ACT_CF_RUN_RIFLE_PISTOL,						false },
	{ ACT_MP_STAND_IDLE,				ACT_CF_IDLE_RIFLE_PISTOL,						false },
	{ ACT_MP_CROUCH_IDLE,				ACT_CF_IDLE_CROUCH_RIFLE_PISTOL,				false },
	{ ACT_MP_CROUCHWALK,				ACT_CF_WALK_CROUCH_RIFLE_PISTOL,				false },
	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_CF_GESTURE_RANGE_ATTACK_RIFLE_PISTOL,		false },
	{ ACT_MP_RELOAD_STAND,				ACT_CF_GESTURE_RELOAD_RIFLE_PISTOL,				false },
	{ ACT_CF_DRAW_SECONDARY,			ACT_CF_DRAW_SECONDARY_RIFLE_PISTOL,				false },
	{ ACT_CF_S_RANGE_ATTACK,			ACT_CF_S_RANGE_ATTACK_RIFLE_PISTOL,				false },
	{ ACT_CF_S_RELOAD,					ACT_CF_S_RELOAD_RIFLE_PISTOL,					false },
	{ ACT_MP_JUMP_START,				ACT_CF_JUMP_START_RIFLE_PISTOL,					false },
	{ ACT_MP_JUMP_FLOAT,				ACT_CF_JUMP_FLOAT_RIFLE_PISTOL,					false },
	{ ACT_MP_JUMP_LAND,					ACT_CF_JUMP_LAND_RIFLE_PISTOL,					false },
	{ ACT_CF_POWERJUMP_FORWARD,			ACT_CF_POWERJUMP_FORWARD_RIFLE_PISTOL,			false },
	{ ACT_CF_POWERJUMP_UP,				ACT_CF_POWERJUMP_UP_RIFLE_PISTOL,				false },
	{ ACT_CF_POWERJUMP_LEFT,			ACT_CF_POWERJUMP_LEFT_RIFLE_PISTOL,				false },
	{ ACT_CF_POWERJUMP_RIGHT,			ACT_CF_POWERJUMP_RIGHT_RIFLE_PISTOL,			false },
	{ ACT_CF_POWERJUMP_BACK,			ACT_CF_POWERJUMP_BACK_RIFLE_PISTOL,				false },
};

acttable_t *CWeaponRifle::ActivityList( CFWeaponType eSecondary, bool bCommon )
{
	return ActivityListStatic(eSecondary, bCommon);
}

int CWeaponRifle::ActivityListCount( CFWeaponType eSecondary, bool bCommon )
{
	return ActivityListCountStatic(eSecondary, bCommon);
}

acttable_t *CWeaponRifle::ActivityListStatic( CFWeaponType eSecondary, bool bCommon )
{
	if (bCommon)
		return s_CommonActions;

	if (eSecondary == WT_PISTOL)
		return s_RiflePistolActions;

	return s_RifleActions;
}

int CWeaponRifle::ActivityListCountStatic( CFWeaponType eSecondary, bool bCommon )
{
	if (bCommon)
		return ARRAYSIZE(s_CommonActions);

	if (eSecondary == WT_PISTOL)
		return ARRAYSIZE(s_RiflePistolActions);

	return ARRAYSIZE(s_RifleActions);
}

CWeaponRifle::CWeaponRifle()
{
}

void CWeaponRifle::Precache( )
{
	PrecacheScriptSound("Weapon_Rifle.Grenade");
	BaseClass::Precache();
}

bool CWeaponRifle::Deploy( )
{
	CCFPlayer *pPlayer = GetPlayerOwner();
	pPlayer->m_iShotsFired = 0;

	return BaseClass::Deploy();
}

bool CWeaponRifle::Reload( )
{
	CCFPlayer *pPlayer = GetPlayerOwner();

	int iResult = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_IDLE );
	if ( !iResult )
		return false;

	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD, 0, pPlayer->GetSecondaryWeapon() == this );

#ifdef GAME_DLL
	SendReloadEvents();
#endif

#ifndef CLIENT_DLL
	if ((iResult) && (pPlayer->GetFOV() != pPlayer->GetDefaultFOV()))
	{
		pPlayer->SetFOV( pPlayer, pPlayer->GetDefaultFOV() );
	}
#endif

	pPlayer->m_iShotsFired = 0;

	return true;
}

void CWeaponRifle::WeaponIdle()
{
	if (m_flTimeWeaponIdle > gpGlobals->curtime)
		return;

	// only idle if the slid isn't back
	if ( m_iClip1 != 0 )
	{
		SetWeaponIdleTime( gpGlobals->curtime + 5.0f );
	}
}

void CWeaponRifle::PrimaryAttack()
{
	BaseClass::PrimaryAttack();
}

void CWeaponRifle::SecondaryAttack()
{
	const CCFWeaponInfo &pWeaponInfo = GetCFWpnData();
	CCFPlayer *pPlayer = GetPlayerOwner();
	
	pPlayer->m_iShotsFired++;

	DispatchParticleEffect("muzzle_flash", PATTACH_POINT_FOLLOW, this, "muzzle");

	// player "shoot" animation
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY, 0, pPlayer->GetSecondaryWeapon() == this );

	FireGrenade();

#ifdef CLIENT_DLL
	if (prediction->IsFirstTimePredicted())
#endif
	{
		CPASAttenuationFilter filter( GetOwner() );
		filter.UsePredictionRules();
		GetOwner()->EmitSound( filter, GetOwner()->entindex(), "Weapon_Rifle.Grenade" );
	}

	pPlayer->Recoil( pWeaponInfo.m_flPermRecoil*2, pWeaponInfo.m_flTempRecoil*2 );

	m_flNextSecondaryAttack = gpGlobals->curtime + SECONDARY_FIRE_RATE;
	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5;

	//start idle animation in 5 seconds
	//SetWeaponIdleTime( gpGlobals->curtime + 5.0 );
}

void CWeaponRifle::FireGrenade()
{
#ifdef GAME_DLL
	CCFPlayer *pOwner = ToCFPlayer( GetOwner() );
	if (!pOwner)
		return;

	QAngle angThrow = pOwner->LocalEyeAngles();

	Vector vForward, vRight, vUp;

	if (angThrow.x < 90 )
		angThrow.x = -10 + angThrow.x * ((90 + 10) / 90.0);
	else
	{
		angThrow.x = 360.0f - angThrow.x;
		angThrow.x = -10 + angThrow.x * -((90 - 10) / 90.0);
	}

	AngleVectors( angThrow, &vForward, &vRight, &vUp );

	Vector vecVelocity = ( vForward * 900 ) + ( vUp * 100.0f ) + ( random->RandomFloat( -10.0f, 10.0f ) * vRight ) +		
		( random->RandomFloat( -10.0f, 10.0f ) * vUp );

	// Don't autoaim on grenade tosses
	Vector vecSrc = pOwner->Weapon_ShootPosition();
	Vector	vecThrow;
	AngleVectors( pOwner->EyeAngles() + pOwner->GetPunchAngle(), &vecThrow );
	VectorScale( vecThrow, 1000.0f, vecThrow );

	CWeaponDrainGrenade* pGrenade = EmitGrenade( vecSrc, vec3_angle, vecVelocity );
	pGrenade->SetAbsVelocity( vecThrow );

	DispatchParticleEffect( "grenade_particle", vecSrc, vec3_angle, pGrenade);
#endif

}

#ifdef GAME_DLL
CWeaponDrainGrenade* CWeaponRifle::EmitGrenade(Vector vecSrc, QAngle vecAngles, Vector vecVel )
{
	CCFPlayer *pOwner = ToCFPlayer( GetOwner() );
	if (!pOwner)
		return NULL;

	return CWeaponDrainGrenade::Create( vecSrc, vecAngles, vecVel, pOwner );
}
#endif