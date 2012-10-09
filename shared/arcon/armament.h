//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ARMAMENT_H
#define ARMAMENT_H
#ifdef _WIN32
#pragma once
#endif

#ifdef GAME_DLL
#include "cf_player.h"
#else
#include "c_cf_player.h"
#endif

#include "filesystem.h"

#include "weapon_cfbase.h"
#include "runes.h"

#define MAX_WEIGHT 4
#define MIN_PRIMARY_WEIGHT 2
#define MIN_SECONDARY_WEIGHT 1
#define MAX_ARMAMENT_STRING	80
#define ARMAMENT_PRINTNAME_MISSING "!!! Missing printname on armament"
#define ATTACK_BINDS 2

typedef unsigned char ArmamentID;

typedef enum armamenttype_e
{
	ARMAMENTTYPE_INVALID = 0,
	ARMAMENTTYPE_ARMOR = 3,
	ARMAMENTTYPE_ITEM = 4,
	MAX_ARMAMENTTYPES
} armamenttype_t;

typedef enum writeto_e
{
	WRITETO_SERVER = 0,
	WRITETO_FILE = 1,
	WRITETO_ARMAMENT = 2,
	MAX_WRITETO
} writeto_t;

#define CFG_DIR "configs"
#define CFG_PATH "cfg/" CFG_DIR

class CArmamentData
{
public:
							CArmamentData(const char* pszName);
							~CArmamentData() { };

	char					m_szName[MAX_ARMAMENT_STRING];
	char					m_szPrintName[MAX_ARMAMENT_STRING];
	bool					m_bBuyable;
	unsigned int			m_iCost;
	armamenttype_t			m_eType;

	int						m_iMaxHealth;
	int						m_iMaxFocus;
	int						m_iMaxStamina;
	int						m_iHealthRegen;
	int						m_iFocusRegen;
	int						m_iStaminaRegen;
	int						m_iAttack;
	int						m_iEnergy;
	int						m_iResistance;
	int						m_iDefense;
	int						m_iSpeed;
	int						m_iCritical;

	bool					m_bPowerjump;
	bool					m_bGrip;

	int						m_iRuneSlots[MAX_RUNES];

	element_t				m_eElementNull;
	element_t				m_eElementHalf;

	static void				PrecacheDatabase( IFileSystem *filesystem, const unsigned char *pICEKey );
	static void				ResetDatabase( void );
	static CArmamentData*	GetData( ArmamentID handle );
	static ArmamentID		AliasToArmamentID( const char *pszAlias );
	static ArmamentID		InvalidArmament() { return s_pArmamentInfoDatabase->InvalidIndex(); };
	static unsigned int		TotalArmaments() { if (!s_pArmamentInfoDatabase) return 0; return s_pArmamentInfoDatabase->Count(); };

private:
	static KeyValues*		ReadEncryptedKVFile( IFileSystem *filesystem, const char *szFilenameWithoutExtension, const unsigned char *pICEKey );
	static void				Parse( KeyValues *pKey );
	static CArmamentData*	FindData( const char *name );

	void					ParseGeneral( KeyValues *pKey );

	static CUtlDict< CArmamentData*, ArmamentID >* s_pArmamentInfoDatabase;
};

class CBuyWeapon
{
public:
	CBuyWeapon() {};
	~CBuyWeapon() {};

	union {
		CFWeaponID	m_iWeapon;
		ArmamentID	m_iArmament;
	};

	RuneID			m_aRunes[MAX_RUNES][MAX_RUNE_MODS];

	// Anything after this won't be serialized!
	char			m_iSerializeEnd;

	// Don't serialize pointers.
	union {
		CCFWeaponInfo*		m_pWeaponData;
		CArmamentData*		m_pArmamentData;
	};
	CRuneData*		m_apRuneData[MAX_RUNES][MAX_RUNE_MODS];
};

// CRunePosition is serialized and networked so keep it as small as possible.
// Everything that's not necessary for binds put in CRuneInfo
class CRunePosition
{
public:
			CRunePosition()
				: m_iWeapon(0), m_iRune(0), m_iMod(0) {};

			CRunePosition(char iWeapon, char iRune, char iMod)
				: m_iWeapon(iWeapon), m_iRune(iRune), m_iMod(iMod) {};

	char	m_iWeapon;
	char	m_iRune;
	char	m_iMod;
};

class CRuneInfo : public CRunePosition
{
public:
	RuneID		m_iRuneID;
};

class CRuneComboPreset
{
public:
					CRuneComboPreset();

	void			Reset();

	void			CalculateData();
	long			Serialize();

	char			m_szName[256];

	unsigned char	m_iSlots;
	unsigned int	m_iCredits;
	bool			m_bHasCritical;
	bool			m_bHasPowerjump;
	bool			m_bHasLatch;

	RuneID			m_aRunes[MAX_RUNE_MODS];
};

class CArmament
{
public:
			CArmament();
			~CArmament() { };

	void	Reset();

	bool	CanBuyWeapon(CFWeaponID iWeapon, int iSlot, int iOptions = 0, int* pMoveTo = NULL);
	bool	BuyWeapon(CFWeaponID iWeapon, int iSlot);
	int		CanBuyWeapon(CFWeaponID iWeapon, int* pMoveTo = NULL);
	bool	BuyWeapon(CFWeaponID iWeapon);
	bool	MoveWeapon(int iSlot, int iNewSlot);
	void	WriteWeapon(int iSlot, bool bComments = false);
	void	RemoveWeapon(CFWeaponID iWeapon);
	void	RemoveWeapon(int iSlot);

	bool	CanBuyRune(RuneID iRune, int iWeapon, int iSlot, int iMod);
	bool	BuyRune(RuneID iRune, int iWeapon, int iSlot, int iMod);
	void	WriteRune(int iWeapon, int iSlot, int iMod, bool bComments = false);
	void	RemoveRune(int iWeapon, int iSlot, int iMod);

	bool	CanBuyCombo(CRuneComboPreset* pCombo, int iWeapon, int iSlot);
	bool	BuyCombo(CRuneComboPreset* pCombo, int iWeapon, int iSlot);
	void	RemoveCombo(int iWeapon, int iSlot);

	bool	CanBind(int iAttack, int iWeapon, int iSlot);
	bool	Bind(int iAttack, int iWeapon, int iSlot, bool bRebind = false);
	void	WriteBind(int iAttack, bool bComments = false, bool bRebind = false);
	void	RemoveBind(int iAttack);

	bool	CanBuyArmament(ArmamentID iArm);
	bool	BuyArmament(ArmamentID iArm);
	void	WriteArmament(armamenttype_t iArm, bool bComments = false);
	void	RemoveArmament(armamenttype_t iArm);

#if defined(ARCON_MOD) || defined(ARCON_DEMO)
	bool	CanBuyWeaponInDemo(CFWeaponID iWeapon);
	bool	CanBuyRuneInDemo(RuneID iRune);
#endif

#ifdef CLIENT_DLL
	void	SaveConfig(int iIndex = -1);
	void	LoadConfig(const char* pszConfig);
	static void	FinishConfig();

#endif
	static int	GetUnusedUserConfigIndex();

	long	SerializeCombo(int iWeapon, int iSlot);

	void	Buy(CCFPlayer* pPlayer, bool bRemoveCurrentWeapons = true);

	void	CalculateAbilities();

	void	UpdateClient(CCFPlayer* pPlayer);

	unsigned char*	Serialize();
	unsigned long	SerializedLength();
	unsigned long	SerializedMainLength();
	unsigned long	SerializedWeaponLength();
	void			Unserialize(const unsigned char* pData);

	void	Describe(CCFPlayer* pPlayer);
	void	DescribeWeapon(CCFPlayer* pPlayer, int iSlot);
	void	DescribeRune(CCFPlayer* pPlayer, int iSlot, int iRune);

	static void	Init();

	RuneID				GetRuneID(int iWeapon, int iSlot, int iMod);
	CRuneLevelData*		GetLevelData(int iWeapon, int iSlot, int iMod);
	unsigned int		GetLevel(int iWeapon, int iSlot, int iMod);

	unsigned int		GetBaseLevel(int iWeapon, int iSlot);
	unsigned int		GetBaseNumber(int iWeapon, int iSlot);

	int					GetRuneIndex(RuneID eRune, int iWeapon, int iCombo, int iStart = 0);
	bool				IsFirstOfID(RuneID eRune, int iWeapon, int iCombo, int iRune);
	bool				IsFirstOfType(runetype_t eType, int iWeapon, int iCombo, int iRune);
	int					GetRuneAmount(RuneID eRune, int iWeapon, int iCombo);
	int					GetRuneTypeAmount(runetype_t eType, int iWeapon, int iCombo);

	CCFWeaponInfo*		GetWeaponData(int iWeapon);
	CArmamentData*		GetArmamentData(int iArmament);

	unsigned int		GetRunesMaximum(int iWeapon, int iSlot);
	unsigned int		GetRunesInSlot(int iWeapon, int iSlot);
	unsigned int		GetRuneCombos(int iWeapon);

	// A way to iterate through all rune combos on every armament.
	int					GetRuneCombos();
	CRunePosition*		GetRuneCombo(int i);
	int					GetRuneCombo(int iWeapon, int iRune);

	// Return true if this combo follows the rules for a valid rune combo
	bool				IsValidCombo(CRunePosition* pRune);

	// Return true if this combo is bindable to a key -- ie has one force and one base rune.
	bool				IsBindableCombo(CRunePosition* pRune);
	bool				HasBindableCombo();		// Are there any bindable combos for this armament?

	bool				HasType(runetype_t eType, int iWeapon, int iSlot);
	bool				HasEffect(runeeffect_t eEffect, int iWeapon, int iSlot);

	bool				HasRestore();

	forceeffect_t		GetDominantForce(int iWeapon, int iSlot);
	static forceeffect_t	GetDominantForce(long iRunes);
	static forceeffect_t	GetDominantForceFromEffects(unsigned int iEffects);

	float				GetRadiusMultiplier(CRunePosition* pRune);

	int					GetAttackBind(int iWeapon, int iRune);

	int					GetPowerjumpDistance();
	bool				CanFullLatch();

	int					GetMaxHealth();
	int					GetMaxFocus();
	int					GetMaxStamina();

	int					GetPhysicalAttackDamage(int iWeapon, int iWeaponDamage);
	element_t			GetPhysicalAttackElement(int iWeapon);
	statuseffect_t		GetPhysicalAttackStatusEffect(int iWeapon);
	float				GetPhysicalAttackStatusEffectMagnitude(int iWeapon);

	float				GetMagicalAttackCost(CRunePosition* pRune);
	float				GetMagicalAttackDamage(CRunePosition* pRune);
	element_t			GetMagicalAttackElement(CRunePosition* pRune);
	statuseffect_t		GetMagicalAttackStatusEffect(CRunePosition* pRune);
	float				GetMagicalAttackStatusMagnitude(CRunePosition* pRune);
	float				GetMagicalAttackCastTime(CRunePosition* pRune);
	float				GetMagicalAttackReload(CRunePosition* pRune);
	bool				GetMagicalAttackPositive(CRunePosition* pRune);

	void				GetElementDefense(CRunePosition* pRune, int &iElements, int &iDefense);
	float				GetElementDefenseScale( int iDamageElements );
	element_t			GetElementsDefended();

	static CCFWeaponInfo* WeaponIDToInfo(CFWeaponID iWeapon);

	static void			WriteToServer();
	static void			WriteToFile(FileHandle_t hFile);
	static void			WriteToArmament(CArmament* pArmament);
	static writeto_t	GetWriteTo() { return s_eWriteTo; };
	void				WriteString(char* pszOut);
	static CArmament*	GetWriteArmament(CCFPlayer* pPlayer);
	void				WriteAll(bool bComments = false);
	void				WriteRunes(int iWeapon, bool bComments = false);

	static char*		GetBuyCommand();

	static CArmament*	GetPariahArmament() { return &s_PariahArmament; };
	static void			InitializePariahArmament();

	unsigned char	m_iSlots;
	unsigned int	m_iCredits;
	CRunePosition	m_aAttackBinds[ATTACK_BINDS];

	bool			m_bPowerjump;
	bool			m_bGrip;

	// Anything after this won't be serialized!
	char			m_iSerializeEnd;

	// These objects have their own serialization.
	CBuyWeapon		m_aWeapons[5];	// Three weapons, armor, accessory

	char			m_szName[32];
	char			m_szFile[32];
	char			m_szImage[32];

	int				m_iLevel;	// Difficulty level, 0 is beginner, 1 medium, 2 is advanced

	int				m_iNoNetwork;

	static writeto_t	s_eWriteTo;
	static FileHandle_t	s_hWriteToFile;
	static CArmament*	s_pWriteToArmament;

protected:
	// TODO: Move more of the stuff above down here into protected.
	CUtlVector<CRunePosition>	m_aRuneCombos;

	static CArmament	s_PariahArmament;
};

typedef enum {
	BA_NOTHING = 0,
	BA_WEAPON,		// Weapon name or ID
	BA_POSITION,	// 0 = primary, 1 = secondary active, 2 = secondary inactive, 3 = armor, 4 = accessory
	BA_RUNE,		// Rune name or ID
	BA_LEVEL,		// Level of a rune
	BA_SLOT,		// Rune slot on weapon
	BA_MOD,			// Mod slot on rune
	BA_ARMOR,		// Armor name or ID
	BA_ATTACK,		// +attack#, 0-3
} buyarg_e;

class CBuyCommand
{
public:
	char*		m_pszAbbr;
	char*		m_pszFull;
	int			m_iParameters;
	void		(*m_pfnCommand) (CCFPlayer *pPlayer, const CCommand &args);
	buyarg_e	m_eArguments[5];
};

#define DEFINE_BUYCOMMAND(name) \
void name(CCFPlayer *pPlayer, const CCommand &args)

DEFINE_BUYCOMMAND(BuyArmorCommand);
DEFINE_BUYCOMMAND(BuyBindCommand);
DEFINE_BUYCOMMAND(BuyRuneCommand);
DEFINE_BUYCOMMAND(BuyWeaponCommand);
DEFINE_BUYCOMMAND(MoveWeaponCommand);
DEFINE_BUYCOMMAND(DescribeCommand);
DEFINE_BUYCOMMAND(HelpCommand);
DEFINE_BUYCOMMAND(RemoveArmorCommand);
DEFINE_BUYCOMMAND(RemoveBindCommand);
DEFINE_BUYCOMMAND(RemoveRuneCommand);
DEFINE_BUYCOMMAND(RemoveWeaponCommand);
DEFINE_BUYCOMMAND(ResetCommand);

#endif // ARMAMENT_H
