//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "cf_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const char *g_pszHintMessages[] =
{
	"#Hint_killing_enemies_is_good",
	"#Hint_BuyMenu",
	"#Hint_R_Reload",
	"#Hint_Space_Latch",
	"#Hint_Q_Switch_Weapons",
	"#Hint_RMB_Focus_Blast",
	"#Hint_X_Thirdperson",
	"#Hint_F_Weaponmode",
	"#Hint_Shift_Magic",
	"#Hint_RMB_Target",
	"#Hint_RMB_Followmode",
	"#Hint_Shift_AimIn",
	"#Hint_S_Block",
	"#Hint_Hold_Charge",
	"#Hint_Numbers_Combos",
	"#Hint_E_Fatality",
	"#Hint_E_Revive",
	"#Hint_Doubletap_Powerjump",
	"#Hint_RMB_Strong_Attack",
	"#Hint_Collect_All_5",

	"#Hint_Low_Stamina",
	"#Hint_Out_Of_Focus",

	"#Hint_You_Are_Pariah",
	"#Hint_You_Are_Fuse",

	"#Hint_One_Force_Base",
};

const char *g_pszGameModes[] =
{
	"tdm",
	"ctf",
	"pariah",
};

//-----------------------------------------------------------------------------
// Teams.
//-----------------------------------------------------------------------------
const char *g_aTeamNames[CF_TEAM_COUNT] =
{
	"Unassigned",
	"Spectator",
	"Numeni",
	"Machindo"
};

Color g_aTeamColors[CF_TEAM_COUNT] = 
{
	Color( 0, 0, 0, 0 ),
	Color( 0, 0, 0, 0 ),
	Color( 255, 178, 0, 255 ),
	Color( 153, 204, 255, 255 )
};

char* g_szWeaponTypes[] =
{
	"ranged",
	"magic",
	"melee",
};

char* g_szElementTypes[] =
{
	"typeless",
	"fire",
	"ice",
	"lightning",
	"earth",
	"light",
	"dark",
	"gravity",
};

Color g_aElementColors[] =
{
	Color(255, 255, 255, 255),
	Color(245, 144,  51, 255),
	Color( 51, 108, 245, 255),
	Color(248, 249,  16, 255),
	Color(177, 109,  15, 255),
	Color(255, 255, 255, 255),
	Color(  0,   0,   0, 255),
	Color( 98,  61, 138, 255),
};

element_t StringToElement( const char* szString )
{
	if (!szString)
		return ELEMENT_TYPELESS;

	for (int i = 0; i < TOTAL_ELEMENTS; i++)
	{
		if (Q_strcmp(szString, g_szElementTypes[i]) == 0)
			return (element_t)(1<<(i-1));
	}
	return ELEMENT_TYPELESS;
}

const char* ElementToString( element_t iElement )
{
	if (iElement == ELEMENT_TYPELESS)
		return g_szElementTypes[0];

	static char szElements[256];
	bool bFirst = true;

	szElements[0] = '\0';

	for (int i = 0; i < TOTAL_ELEMENTS; i++)
	{
		if (iElement & (1<<i))
		{
			Q_snprintf(szElements, 256, "%s%s%s", szElements, (bFirst?"":" "), g_szElementTypes[i+1]);
			bFirst = false;
		}
	}

	return szElements;
}

const Color ElementToColor( element_t iElement )
{
	if (iElement == ELEMENT_TYPELESS)
		return g_aElementColors[0];

	for (int i = 0; i < TOTAL_ELEMENTS; i++)
	{
		if (iElement & (1<<i))
		{
			// Until better behavior is found, just return the first one.
			return g_aElementColors[i+1];
		}
	}

	return g_aElementColors[0];
}

char* g_szStatusEffectTypes[] =
{
	"none",
	"dot",
	"slowness",
	"weakness",
	"disorient",
	"blindlight",
	"blinddark",
	"gravity",
	"atrophy",
	"silence",
	"regen",
	"poison",
	"haste",
	"shield",
	"barrier",
	"reflect",
	"stealth",
};

statuseffect_t StringToStatusEffect( const char* szString )
{
	if (!szString)
		return STATUSEFFECT_NONE;

	for (int i = 0; i < TOTAL_STATUSEFFECTS; i++)
	{
		if (Q_strcmp(szString, g_szStatusEffectTypes[i]) == 0)
			return (statuseffect_t)(1<<(i-1));
	}
	return STATUSEFFECT_NONE;
}

const char* StatusEffectToString( statuseffect_t iEffect )
{
	if (iEffect == STATUSEFFECT_NONE)
		return g_szStatusEffectTypes[0];

	static char szEffects[256];
	bool bFirst = true;

	szEffects[0] = '\0';

	for (int i = 0; i < TOTAL_STATUSEFFECTS; i++)
	{
		if (iEffect & (1<<i))
		{
			Q_snprintf(szEffects, 256, "%s%s%s", szEffects, (bFirst?"":" "), g_szStatusEffectTypes[i+1]);
			bFirst = false;
		}
	}

	return szEffects;
}

bool StatusEffectDesireable( statuseffect_t iElement )
{
	switch (iElement)
	{
	case STATUSEFFECT_DOT:
	case STATUSEFFECT_SLOWNESS:
	case STATUSEFFECT_WEAKNESS:
	case STATUSEFFECT_DISORIENT:
	case STATUSEFFECT_BLINDNESS:
	case STATUSEFFECT_ATROPHY:
	case STATUSEFFECT_SILENCE:
	case STATUSEFFECT_POISON:
		return false;

	case STATUSEFFECT_REGEN:
	case STATUSEFFECT_HASTE:
	case STATUSEFFECT_SHIELD:
	case STATUSEFFECT_BARRIER:
	case STATUSEFFECT_REFLECT:
	case STATUSEFFECT_STEALTH:
	default:	// If it's not bad, it must be good.
		return true;
	}
}

statuseffect_t StatusEffectForElement( element_t eElements )
{
	statuseffect_t eStatuses = STATUSEFFECT_NONE;

	for (int i = 0; i < TOTAL_ELEMENTS; i++)
	{
		switch (eElements&(1<<i))
		{
		case ELEMENT_TYPELESS:
			break;

		case ELEMENT_FIRE:
			eStatuses |= STATUSEFFECT_DOT;
			break;

		case ELEMENT_ICE:
			eStatuses |= STATUSEFFECT_SLOWNESS;
			break;

		case ELEMENT_LIGHTNING:
			eStatuses |= STATUSEFFECT_WEAKNESS;
			break;

		case ELEMENT_DARK:
			eStatuses |= STATUSEFFECT_BLINDNESS;
			break;
		}
	}

	return eStatuses;
}

void CF_Precache()
{
	REGISTER_SHARED_ANIMEVENT( AE_CF_ENDAFFLICT, AE_TYPE_CLIENT|AE_TYPE_SERVER );
	REGISTER_SHARED_ANIMEVENT( AE_CF_ENDFATALITY, AE_TYPE_SERVER );
	REGISTER_SHARED_ANIMEVENT( AE_CF_DECAPITATE, AE_TYPE_SERVER );
	REGISTER_SHARED_ANIMEVENT( AE_CF_STEP_LEFT, AE_TYPE_CLIENT|AE_TYPE_SERVER );
	REGISTER_SHARED_ANIMEVENT( AE_CF_STEP_RIGHT, AE_TYPE_CLIENT|AE_TYPE_SERVER );

	CBaseEntity::PrecacheModel("models/other/goalarrow.mdl");
}
