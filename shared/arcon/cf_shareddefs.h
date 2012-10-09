//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef CF_SHAREDDEFS_H
#define CF_SHAREDDEFS_H
#ifdef _WIN32
#pragma once
#endif

#include "eventlist.h"

#define CF_PLAYER_VIEW_OFFSET	Vector( 0, 0, 53.5 )

#define FATALITY_TIME 3
#define LATCH_BREAK_TIME 0.2f

enum eteams_list
{
    TEAM_NUMENI		= LAST_SHARED_TEAM+1,	// These values are in the FGD
    TEAM_MACHINDO,
    CF_TEAM_COUNT
};

#define PLAYER_MODEL_MACHINDO "models/player/machindomale.mdl"
#define PLAYER_MODEL_NUMENI "models/player/numenimale.mdl"
#define PLAYER_MODEL_PARIAH "models/player/pariah.mdl"
#define PLAYER_MODEL_SHADE "models/player/shade.mdl"

typedef enum
{
	MESSAGE_CRITICAL,
	MESSAGE_MULTIPLIER,
	MESSAGE_BONUSHEALTH,
	MESSAGE_FLAGCAPTURED,
} cfmessage_t;

typedef enum
{
	OBJECTIVE_HOME,
	OBJECTIVE_DROPPED,
	OBJECTIVE_STOLEN,
} objectivestatus_t;

//-----------------------------------------------------------------------------
// CF Hints
//-----------------------------------------------------------------------------
enum
{
	HINT_ENEMY_KILLED = 0,				// #Hint_killing_enemies_is_good

	HINT_BUYMENU,						// #Hint_BuyMenu
	HINT_R_RELOAD,						// #Hint_R_Reload
	HINT_SPACE_LATCH,					// #Hint_Space_Latch
	HINT_Q_SWITCH_WEAPONS,				// #Hint_Q_Switch_Weapons
	HINT_RMB_FOCUS_BLAST,				// #Hint_RMB_Focus_Blast
	HINT_X_THIRDPERSON,					// #Hint_X_Thirdperson
	HINT_F_WEAPONMODE,					// #Hint_F_Weaponmode
	HINT_SHIFT_MAGIC,					// #Hint_Shift_Magic
	HINT_RMB_TARGET,					// #Hint_RMB_Target
	HINT_RMB_FOLLOW,					// #Hint_RMB_Followmode
	HINT_SHIFT_AIMIN,					// #Hint_Shift_AimIn
	HINT_S_BLOCK,						// #Hint_S_Block
	HINT_HOLD_CHARGE,					// #Hint_Hold_Charge
	HINT_NUMBERS_COMBOS,				// #Hint_Numbers_Combos
	HINT_E_FATALITY,					// #Hint_E_Fatality
	HINT_E_REVIVE,						// #Hint_E_Revive
	HINT_DOUBLETAP_POWERJUMP,			// #Hint_Doubletap_Powerjump
	HINT_RMB_STRONG_ATTACK,				// #Hint_RMB_Strong_Attack
	HINT_COLLECT_ALL_5,					// #Hint_Collect_All_5

	HINT_LOW_STAMINA,					// #Hint_Low_Stamina
	HINT_OUT_OF_FOCUS,					// #Hint_Out_Of_Focus

	HINT_YOU_ARE_PARIAH,				// #Hint_You_Are_Pariah
	HINT_YOU_ARE_FUSE,					// #Hint_You_Are_Fuse

	// Menu hints
	HINT_ONE_FORCE_BASE,				// #Hint_One_Force_Base

	NUM_HINTS
};
extern const char *g_pszHintMessages[];

// Game mode types
typedef enum
{
	CF_GAME_TDM = 0,	// The default game mode if there is an error should have a zero value.
	CF_GAME_CTF,
	CF_GAME_PARIAH,
	NUM_GAMEMODES
} CFGameType;
extern const char *g_pszGameModes[];

extern const char *g_aTeamNames[CF_TEAM_COUNT];
extern Color g_aTeamColors[CF_TEAM_COUNT];

typedef enum weapontype_e
{
	INVALID_WEAPONTYPE = -1,
	WEAPONTYPE_RANGED = 0,
	WEAPONTYPE_MAGIC,
	WEAPONTYPE_MELEE,
	// If you add one here, add to g_szWeaponTypes
	NUMBER_WEAPONTYPES
} weapontype_t;

extern char* g_szWeaponTypes[];

enum {
	SCORE_NOTHING = 0,
	SCORE_KO,
	SCORE_FATALITY,
	SCORE_REVIVAL,
	SCORE_DAMAGE,
	SCORE_HEAL,
	SCORE_INFLICT_STATUS,
	SCORE_HEAL_STATUS,
	SCORE_CAPTURE_FLAG,
	SCORE_KILL_ENEMY_FLAG_CARRIER,
	SCORE_RETURN_FLAG,
	SCORE_CAPTURE_POINT,
	SCORE_BLOCK_CAPTURE,
};

typedef enum element_e
{
    ELEMENT_TYPELESS	= 0,
    ELEMENT_FIRE		= (1<<0),
	ELEMENT_ICE			= (1<<1),
	ELEMENT_LIGHTNING	= (1<<2),
	ELEMENT_UNUSED1		= (1<<3),
	ELEMENT_LIGHT		= (1<<4),
	ELEMENT_DARK		= (1<<5),
} element_t;

#define TOTAL_ELEMENTS		7
#define ELEMENTS_ALL (ELEMENT_FIRE|ELEMENT_ICE|ELEMENT_LIGHTNING)

inline const element_t& operator|=( element_t& obj, const element_t& value ) 
{
	obj = (element_t)((int)obj | (int)value);
	return obj;
}

extern char* g_szElementTypes[];

element_t StringToElement( const char* szString );
const char* ElementToString( element_t iElement );
const Color ElementToColor( element_t iElement );

typedef enum statuseffect_e
{
	STATUSEFFECT_NONE		= 0,
	STATUSEFFECT_DOT		= (1<<0),
	STATUSEFFECT_SLOWNESS	= (1<<1),
	STATUSEFFECT_WEAKNESS	= (1<<2),
	STATUSEFFECT_DISORIENT	= (1<<3),
	STATUSEFFECT_BLINDNESS	= (1<<4),
	STATUSEFFECT_UNUSED1	= (1<<5),
	STATUSEFFECT_UNUSED2	= (1<<6),
	STATUSEFFECT_ATROPHY	= (1<<7),
	STATUSEFFECT_SILENCE	= (1<<8),
	STATUSEFFECT_REGEN		= (1<<9),
	STATUSEFFECT_POISON		= (1<<10),
	STATUSEFFECT_HASTE		= (1<<11),
	STATUSEFFECT_SHIELD		= (1<<12),
	STATUSEFFECT_BARRIER	= (1<<13),
	STATUSEFFECT_REFLECT	= (1<<14),
	STATUSEFFECT_STEALTH	= (1<<15),
} statuseffect_t;
#define TOTAL_STATUSEFFECTS		16
#define STATUSEFFECTS_NEGATIVE (STATUSEFFECT_DOT|STATUSEFFECT_SLOWNESS|STATUSEFFECT_WEAKNESS)	// None of the others are in yet
// If you increase this, be mindful of m_iStatusEffects's data type

#define TARGETLOSS_STATUSEFFECT STATUSEFFECT_WEAKNESS

inline const statuseffect_t& operator|=( statuseffect_t& obj, const statuseffect_t& value ) 
{
	obj = (statuseffect_t)((int)obj | (int)value);
	return obj;
}

extern char* g_szStatusEffectTypes[];

statuseffect_t StringToStatusEffect( const char* szString );
const char* StatusEffectToString( statuseffect_t iElement );
bool StatusEffectDesireable( statuseffect_t iElement );
statuseffect_t StatusEffectForElement( element_t eElements );

inline const Color HealColor() { return Color(11, 235, 5, 255); };

enum
{
	AE_CF_ENDAFFLICT = LAST_SHARED_ANIMEVENT,
	AE_CF_ENDFATALITY,
	AE_CF_DECAPITATE,
	AE_CF_STEP_LEFT,
	AE_CF_STEP_RIGHT,
};

extern void CF_Precache();

#endif // CF_SHAREDDEFS_H
