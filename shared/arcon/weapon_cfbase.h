//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef WEAPON_CFBASE_H
#define WEAPON_CFBASE_H
#ifdef _WIN32
#pragma once
#endif

#include "cf_playeranimstate.h"
#include "cf_weapon_parse.h"

#if defined( CLIENT_DLL )
	#define CWeaponCFBase C_WeaponCFBase
#endif

class CCFPlayer;

// These are the names of the ammo types that the weapon script files reference.
#define AMMO_BULLETS			"AMMO_BULLETS"
#define AMMO_ROCKETS			"AMMO_ROCKETS"
#define AMMO_GRENADE			"AMMO_GRENADE"
#define AMMO_BUCKSHOT			"AMMO_BUCKSHOT"

//--------------------------------------------------------------------------------------------------------
//
// Weapon IDs for all CF Game weapons
//
typedef enum
{
	WEAPON_NONE = 0,

	WEAPON_MAGIC,
	WEAPON_PISTOL,
	WEAPON_RIFLE,
	WEAPON_RIVENBLADE,
	WEAPON_SHOTGUN,
	WEAPON_VALANGARD,
	WEAPON_PARIAH,
	WEAPON_FLAG,
	
	WEAPON_MAX,		// number of weapons weapon index
} CFWeaponID;

typedef enum
{
	Primary_Mode = 0,
	Secondary_Mode,
} CFWeaponMode;

typedef enum
{
	WT_NONE = 0,
	WT_PISTOL,
	WT_LONGSWORD,
	WT_LONGGUN,
	WT_BUSTER,
} CFWeaponType;

typedef CHandle<CWeaponCFBase> CWeaponCFBaseHandle;

const char *WeaponIDToAlias( CFWeaponID id );
extern CFWeaponType WeaponClassFromWeaponID( CFWeaponID weaponID );
CFWeaponID AliasToWeaponID( const char *alias );
extern bool	IsPrimaryWeapon( int id );
extern bool IsSecondaryWeapon( int id );
inline const char *GetTranslatedWeaponAlias(const char *alias) { return alias; };

class CWeaponCFBase : public CBaseCombatWeapon
{
public:
	DECLARE_CLASS( CWeaponCFBase, CBaseCombatWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CWeaponCFBase();

	#ifdef GAME_DLL
		DECLARE_DATADESC();
	#endif

	// All predicted weapons need to implement and return true
	virtual bool			IsPredicted() const;
	virtual CFWeaponID		GetWeaponID( void ) const { return WEAPON_NONE; }
	virtual CFWeaponType	GetWeaponType( void ) const;
	static CFWeaponType		GetWeaponType( CFWeaponID eWeapon );
	virtual bool			IsFullAuto() { return true; };
	virtual bool			IsMeleeWeapon() const;

	virtual void			Equip( CBaseCombatCharacter *pOwner );
	virtual bool			Deploy( void );
	virtual void			SetWeaponVisible( bool visible );
	virtual bool			IsWeaponVisible( void );

	virtual void			ItemPreFrame( void );
	virtual void			ItemPostFrame( void );

	virtual void 			DefaultTouch( CBaseEntity *pOther );	// default weapon touch
	virtual void			SetPickupTouch( void );

	virtual void			PrimaryAttack( void );
	virtual void			SecondaryAttack( void );
	virtual bool			DefaultReload( int iClipSize1, int iClipSize2, int iActivity );

	virtual void			DisableForSeconds(float flTime, bool bFlag = false);
	virtual void			DisableUntil(float flTime, bool bFlag = false);
	virtual bool			IsDisableFlagged();

	// Melee stuff
	virtual void			StopMeleeAttack(bool bDualAware = false, bool bInterruptChain = true) {};
	virtual float			GetChargeStartTime() { return 0; };

	virtual void			SetHand( bool bRight ) { m_bRightHand = bRight; };
	virtual bool			IsRightHanded() { return m_bRightHand; };

	virtual void			SetPosition( char iPosition ) { m_iPosition = iPosition; };
	virtual char			GetPosition() { return m_iPosition; };

	virtual bool			IsPrimary();
	virtual bool			IsActive();

	// Melee stubs
	virtual bool			CanBlockBullets() { return false; };
	virtual void			BulletBlocked(int iDamageType) {};
	virtual void			ShowBlock() {};
	virtual bool			IsStrongAttack() { return false; };
	virtual void			EndRush() {};

	virtual	int				UpdateClientData( CBasePlayer *pPlayer );
#ifdef CLIENT_DLL
	virtual	void			NotifyShouldTransmit( ShouldTransmitState_t state );
#endif

	virtual char*			GetBoneMergeName(int iBone);

#ifdef CLIENT_DLL
	virtual int				DrawModel( int flags );
	virtual bool			OnInternalDrawModel( ClientModelRenderInfo_t *pInfo );
	virtual bool			OnPostInternalDrawModel( ClientModelRenderInfo_t *pInfo );

	virtual ShadowType_t	ShadowCastType();
#endif

	virtual unsigned int	FiringButtons();
	virtual unsigned int	PrimaryButtons();
	virtual unsigned int	SecondaryButtons();
	virtual unsigned int	AttackButtons();

	virtual Activity		ActivityOverride( Activity baseAct, bool *pRequired );
	static Activity			ActivityOverride( CFWeaponID ePrimary, CFWeaponType eSecondary, Activity baseAct, bool *pRequired );
	virtual	acttable_t*		ActivityList( void ) { AssertMsg(false, "Old ActivityList() called."); return NULL; }
	virtual	int				ActivityListCount( void ) { AssertMsg(false, "Old ActivityListCount() called."); return 0; }
	virtual	acttable_t*		ActivityList( CFWeaponType eSecondary, bool bCommon ) { return NULL; }
	virtual	int				ActivityListCount( CFWeaponType eSecondary, bool bCommon ) { return 0; }
	static	acttable_t*		ActivityList( CFWeaponID ePrimary, CFWeaponType eSecondary, bool bCommon );
	static	int				ActivityListCount( CFWeaponID ePrimary, CFWeaponType eSecondary, bool bCommon );

	virtual float			GetFireRate( void );

	// Get CF weapon specific weapon data.
	CCFWeaponInfo const	&GetCFWpnData() const;

	// Get a pointer to the player that owns this weapon
	CCFPlayer* GetPlayerOwner() const;

	// override to play custom empty sounds
	virtual bool PlayEmptySound();

#ifdef GAME_DLL
	virtual void SendReloadEvents();
#endif

	class CGroupedSound
	{
	public:
		string_t m_SoundName;
		Vector m_vPos;
	};

	static CUtlVector<CGroupedSound> s_GroupedSounds;

	static void ShotgunImpactSoundGroup( const char *pSoundName, const Vector &vEndPos );

protected:
	CNetworkVar( char, m_iPosition );
	CNetworkVar( bool, m_bRightHand );
	CNetworkVar( float, m_flDisabledUntil );

	bool m_bWeaponVisible;

private:
	CWeaponCFBase( const CWeaponCFBase & );
};


#endif // WEAPON_CFBASE_H
