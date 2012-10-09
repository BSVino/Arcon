//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "weapon_cfbase.h"
#include "gamerules.h"
#include "npcevent.h"
#include "engine/IEngineSound.h"
#include "weapon_basecfgrenade.h"
#include "in_buttons.h"	


#ifdef CLIENT_DLL

	#include "c_cf_player.h"

#else

	#include "cf_player.h"
	#include "items.h"

#endif


#define GRENADE_TIMER	1.5f //Seconds


IMPLEMENT_NETWORKCLASS_ALIASED( BaseCFGrenade, DT_BaseCFGrenade )

BEGIN_NETWORK_TABLE(CBaseCFGrenade, DT_BaseCFGrenade)

#ifndef CLIENT_DLL
	SendPropBool( SENDINFO(m_bRedraw) ),
	SendPropBool( SENDINFO(m_bPinPulled) ),
	SendPropFloat( SENDINFO(m_fThrowTime), 0, SPROP_NOSCALE ),
#else
	RecvPropBool( RECVINFO(m_bRedraw) ),
	RecvPropBool( RECVINFO(m_bPinPulled) ),
	RecvPropFloat( RECVINFO(m_fThrowTime) ),
#endif

END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CBaseCFGrenade )
	DEFINE_PRED_FIELD( m_bRedraw, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bRedraw, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_basecfgrenade, CBaseCFGrenade );


CBaseCFGrenade::CBaseCFGrenade()
{
	m_bRedraw = false;
	m_bPinPulled = false;
	m_fThrowTime = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseCFGrenade::Precache()
{
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseCFGrenade::Deploy()
{
	m_bRedraw = false;
	m_bPinPulled = false;
	m_fThrowTime = 0;

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseCFGrenade::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_bRedraw = false;
	m_bPinPulled = false; // when this is holstered make sure the pin isn�t pulled.
	m_fThrowTime = 0;

#ifndef CLIENT_DLL
	// If they attempt to switch weapons before the throw animation is done, 
	// allow it, but kill the weapon if we have to.
	CCFPlayer *pPlayer = GetPlayerOwner();

	if( pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0 )
	{
		CBaseCombatCharacter *pOwner = (CBaseCombatCharacter *)pPlayer;
		pOwner->Weapon_Drop( this );
		UTIL_Remove(this);
	}
#endif

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseCFGrenade::PrimaryAttack()
{
	if ( m_bRedraw || m_bPinPulled )
		return;

	CCFPlayer *pPlayer = GetPlayerOwner();
	if ( !pPlayer || pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
		return;

	// The pull pin animation has to finish, then we wait until they aren't holding the primary
	// attack button, then throw the grenade.
	SendWeaponAnim( ACT_VM_PULLPIN );
	m_bPinPulled = true;
	
	// Don't let weapon idle interfere in the middle of a throw!
	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );

	DisableForSeconds(SequenceDuration());
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseCFGrenade::SecondaryAttack()
{
	if ( m_bRedraw )
		return;

	CCFPlayer *pPlayer = GetPlayerOwner();
	
	if ( pPlayer == NULL )
		return;

	//See if we're ducking
	if ( pPlayer->GetFlags() & FL_DUCKING )
	{
		//Send the weapon animation
		SendWeaponAnim( ACT_VM_SECONDARYATTACK );
	}
	else
	{
		//Send the weapon animation
		SendWeaponAnim( ACT_VM_HAULBACK );
	}

	// Don't let weapon idle interfere in the middle of a throw!
	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );

	DisableForSeconds(SequenceDuration());
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseCFGrenade::Reload()
{
	if ( ( m_bRedraw ) && ( m_flNextPrimaryAttack <= gpGlobals->curtime ) && ( m_flNextSecondaryAttack <= gpGlobals->curtime ) )
	{
		//Redraw the weapon
		SendWeaponAnim( ACT_VM_DRAW );

		//Update our times
		DisableForSeconds(SequenceDuration());

		SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseCFGrenade::ItemPostFrame()
{
	CCFPlayer *pPlayer = GetPlayerOwner();
	if ( !pPlayer )
		return;

	CBaseViewModel *vm = pPlayer->GetViewModel( m_nViewModelIndex );
	if ( !vm )
		return;

	// If they let go of the fire button, they want to throw the grenade.
	if ( m_bPinPulled && !(pPlayer->m_nButtons & IN_ATTACK) ) 
	{
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_THROW_GRENADE );

		StartGrenadeThrow();
		
		DecrementAmmo( pPlayer );
	
		m_bPinPulled = false;
		SendWeaponAnim( ACT_VM_THROW );	
		SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );
	}
	else if ((m_fThrowTime > 0) && (m_fThrowTime < gpGlobals->curtime))
	{
		ThrowGrenade();
	}
	else if( m_bRedraw )
	{
		// Has the throw animation finished playing
		if( m_flTimeWeaponIdle < gpGlobals->curtime )
		{
#ifdef GAME_DLL
			// if we're officially out of grenades, ditch this weapon
			if( pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0 )
			{
				pPlayer->Weapon_Drop( this, NULL, NULL );
				UTIL_Remove(this);
			}
			else
			{
				pPlayer->SwitchToNextBestWeapon( this );
			}
#endif
			return;	//don't animate this grenade any more!
		}	
	}
	else if( !m_bRedraw )
	{
		BaseClass::ItemPostFrame();
	}
}



#ifdef CLIENT_DLL

	void CBaseCFGrenade::DecrementAmmo( CBaseCombatCharacter *pOwner )
	{
	}

	void CBaseCFGrenade::DropGrenade()
	{
		m_bRedraw = true;
		m_fThrowTime = 0.0f;
	}

	void CBaseCFGrenade::ThrowGrenade()
	{
		m_bRedraw = true;
		m_fThrowTime = 0.0f;
	}

	void CBaseCFGrenade::StartGrenadeThrow()
	{
		m_fThrowTime = gpGlobals->curtime + 0.1f;
	}

#else

	BEGIN_DATADESC( CBaseCFGrenade )
		DEFINE_FIELD( m_bRedraw, FIELD_BOOLEAN ),
	END_DATADESC()

	int CBaseCFGrenade::CapabilitiesGet()
	{
		return bits_CAP_WEAPON_RANGE_ATTACK1; 
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	// Input  : *pOwner - 
	//-----------------------------------------------------------------------------
	void CBaseCFGrenade::DecrementAmmo( CBaseCombatCharacter *pOwner )
	{
		pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );
	}

	void CBaseCFGrenade::StartGrenadeThrow()
	{
		m_fThrowTime = gpGlobals->curtime + 0.1f;
	}

	void CBaseCFGrenade::ThrowGrenade()
	{
		CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
		if ( !pPlayer )
		{
			Assert( false );
			return;
		}

		QAngle angThrow = pPlayer->LocalEyeAngles();

		Vector vForward, vRight, vUp;

		if (angThrow.x < 90 )
			angThrow.x = -10 + angThrow.x * ((90 + 10) / 90.0);
		else
		{
			angThrow.x = 360.0f - angThrow.x;
			angThrow.x = -10 + angThrow.x * -((90 - 10) / 90.0);
		}

		float flVel = (90 - angThrow.x) * 6;

		if (flVel > 750)
			flVel = 750;

		AngleVectors( angThrow, &vForward, &vRight, &vUp );

		Vector vecSrc = pPlayer->GetAbsOrigin() + pPlayer->GetViewOffset();

		vecSrc += vForward * 16;
	
		Vector vecThrow = vForward * flVel + pPlayer->GetAbsVelocity();

		EmitGrenade( vecSrc, vec3_angle, vecThrow, AngularImpulse(600,random->RandomInt(-1200,1200),0), pPlayer );

		m_bRedraw = true;
		m_fThrowTime = 0.0f;
	}

	void CBaseCFGrenade::DropGrenade()
	{
		CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
		if ( !pPlayer )
		{
			Assert( false );
			return;
		}

		Vector vForward;
		pPlayer->EyeVectors( &vForward );
		Vector vecSrc = pPlayer->GetAbsOrigin() + pPlayer->GetViewOffset() + vForward * 16; 

		Vector vecVel = pPlayer->GetAbsVelocity();

		EmitGrenade( vecSrc, vec3_angle, vecVel, AngularImpulse(600,random->RandomInt(-1200,1200),0), pPlayer );

		m_bRedraw = true;
		m_fThrowTime = 0.0f;
	}

	void CBaseCFGrenade::EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel, AngularImpulse angImpulse, CBasePlayer *pPlayer )
	{
		Assert( 0 && "CBaseCFGrenade::EmitGrenade should not be called. Make sure to implement this in your subclass!\n" );
	}

	bool CBaseCFGrenade::AllowsAutoSwitchFrom( void ) const
	{
		return !m_bPinPulled;
	}

#endif

