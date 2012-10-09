//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "armament.h"
#include <KeyValues.h>

#ifdef CLIENT_DLL
#include "hud_macros.h"
#include "cfui_gui.h"
#include "cfui_menu.h"
#include "hud_indicators.h"
#else
#include "cfgui_shared.h"
#endif

#include "UtlSortVector.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define ConPrint(x) Msg(x)
#else
#define ConPrint(x) ClientPrint( pPlayer, HUD_PRINTCONSOLE, x)
//	ClientPrint( pPlayer, HUD_PRINTCONSOLE, "Armament report:\n" );
#endif

static ConVar mp_inventory("mp_inventory", "1000", FCVAR_CHEAT|FCVAR_NOTIFY|FCVAR_REPLICATED, "Inventory space available to the player.", true, 0, true, 100000);
static ConVar cf_numen_status_effect_magnitude( "cf_numen_status_effect_magnitude", "0.2", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );

static const CBuyCommand g_Commands[] = {
	{ "a",	"armor",	1, &BuyArmorCommand,	BA_ARMOR,	BA_NOTHING,	BA_NOTHING,	BA_NOTHING,	BA_NOTHING },
	{ "b",	"bind",		3, &BuyBindCommand,		BA_ATTACK,	BA_POSITION,BA_SLOT,	BA_NOTHING,	BA_NOTHING },
	{ "r",	"rune",		5, &BuyRuneCommand,		BA_RUNE,	BA_POSITION,BA_SLOT,	BA_MOD,		BA_NOTHING },
	{ "w",	"weapon",	2, &BuyWeaponCommand,	BA_WEAPON,	BA_POSITION,BA_NOTHING,	BA_NOTHING,	BA_NOTHING },
	{ "m",	"move",		2, &MoveWeaponCommand,	BA_POSITION,BA_POSITION,BA_NOTHING,	BA_NOTHING,	BA_NOTHING },
	{ "d",	"describe",	0, &DescribeCommand,	BA_NOTHING,	BA_NOTHING,	BA_NOTHING,	BA_NOTHING,	BA_NOTHING },
	{ "h",	"help",		0, &HelpCommand,		BA_NOTHING,	BA_NOTHING,	BA_NOTHING,	BA_NOTHING,	BA_NOTHING },
	{ "?",	"help",		0, &HelpCommand,		BA_NOTHING,	BA_NOTHING,	BA_NOTHING,	BA_NOTHING,	BA_NOTHING },
	{ "-",	"-",		0, &ResetCommand,		BA_NOTHING,	BA_NOTHING,	BA_NOTHING,	BA_NOTHING,	BA_NOTHING },
	{ "-a",	"-armor",	0, &RemoveArmorCommand,	BA_POSITION,BA_NOTHING,	BA_NOTHING,	BA_NOTHING,	BA_NOTHING },
	{ "-b",	"-bind",	1, &RemoveBindCommand,	BA_ATTACK,	BA_NOTHING,	BA_NOTHING,	BA_NOTHING,	BA_NOTHING },
	{ "-r",	"-rune",	3, &RemoveRuneCommand,	BA_POSITION,BA_SLOT,	BA_MOD,		BA_NOTHING,	BA_NOTHING },
	{ "-w",	"-weapon",	1, &RemoveWeaponCommand,BA_POSITION,BA_NOTHING,	BA_NOTHING,	BA_NOTHING,	BA_NOTHING },
};

#ifdef CLIENT_DLL
// Client stubs
CCFPlayer* GetCommandPlayer()
{
	return C_CFPlayer::GetLocalCFPlayer();
}

void ShowPanel(CFPanel ePanel)
{
	CCFMenu::OpenMenu(ePanel, true);
}
#else
// Server stubs
CCFPlayer* GetCommandPlayer()
{
	return ToCFPlayer( UTIL_GetCommandClient() );
}

// What an awesome hack.
#define ShowPanel(ePanel) \
	pPlayer->ShowCFPanel( ePanel, true );
#endif

void CC_Buy_f(const CCommand &args)
{
	CCFPlayer *pPlayer = ToCFPlayer(GetCommandPlayer());
	if ( !pPlayer )
		return;

#ifdef CLIENT_DLL
	if (FStrEq(args[0], "c") && CArmament::s_eWriteTo != WRITETO_ARMAMENT)
	{
		AssertMsg(false, "Config buy command issued while not writing to armament.\n");
		return;
	}
#endif

	pPlayer->Instructor_LessonLearned(HINT_BUYMENU);

	if (args.ArgC() == 1)
	{
		ShowPanel( CF_CHAR_PANEL );
		return;
	}

	char* szCommand = (char*)args[1];

	for (int i = 0; i < sizeof(g_Commands)/sizeof(g_Commands[0]); i++)
	{
		if (strncmp(szCommand, g_Commands[i].m_pszAbbr, strlen(szCommand)) == 0
			|| strcmp(szCommand, g_Commands[i].m_pszFull) == 0)
		{
			g_Commands[i].m_pfnCommand(pPlayer, args);
			break;
		}
	}

	// If the "buy" command is used, a player is typing it into their console,
	// so update the client as to what the player has purchased. Otherwise, the
	// client has already added this item to its version of the armament, so all
	// is well.
	if (strlen(args[0]) == 3)
		CArmament::GetWriteArmament(pPlayer)->UpdateClient(pPlayer);
}

static int BuyCompletion( const char *pszPartial, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] )
{
	const char *pszBuy = "buy";

	const char *pszSubCommands[6] =
	{
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
	};

	char szCommand[1024];
	char szReturnedCommand[1024];
	strcpy(szCommand, pszPartial);
	strcpy(szReturnedCommand, pszBuy);

	char* pszTok = strtok(szCommand, " ");
	pszTok = strtok(NULL, " ");	// get rid of "buy"

	int iParameter = 0;
	while (pszTok)
	{
		pszSubCommands[iParameter] = pszTok;
		pszTok = strtok(NULL, " ");
		if (pszTok || pszPartial[strlen(pszPartial)-1] == ' ')
			Q_snprintf(szReturnedCommand + strlen(szReturnedCommand), 1024 - strlen(szReturnedCommand), " %s", pszSubCommands[iParameter]);
		if (++iParameter >= 6)
			pszTok = NULL;
	}

	if (pszPartial[strlen(pszPartial)-1] != ' ')
		iParameter--;

	int iCompletions = 0;

	for ( int i = 0; i < sizeof(g_Commands)/sizeof(g_Commands[0]); i++ )
	{
		const CBuyCommand *pCommand = &g_Commands[ i ];

		if (iParameter == 0)
		{
			if ( pszSubCommands[0] && Q_strncasecmp( pCommand->m_pszFull, pszSubCommands[0], strlen( pszSubCommands[0] ) ) )
				continue;

			// If the command is already in the buffer don't list it twice.
			bool bContinue = false;
			for (int j = 0; j < COMMAND_COMPLETION_MAXITEMS; j++)
				if (Q_strcasecmp( commands[j], VarArgs("%s %s", szReturnedCommand, pCommand->m_pszFull) ) == 0)
					bContinue = true;

			if (bContinue)
				continue;

			Q_snprintf( commands[ iCompletions ], sizeof( commands[ iCompletions ] ), "%s %s", szReturnedCommand, pCommand->m_pszFull );
			iCompletions++;
		}
		else
		{
			if (!FStrEq(pCommand->m_pszFull, pszSubCommands[0]))
				continue;

			int j = 0;
			const char *pszVal = NULL;
			CRuneData *pRune = NULL;
			CArmamentData *pArm = NULL;

			switch (pCommand->m_eArguments[iParameter-1])
			{
			default:
			case BA_NOTHING:
				break;

			case BA_WEAPON:
				j = 2;	// Skip none and magic.
				while ((pszVal = WeaponIDToAlias((CFWeaponID)j++)) != NULL)
				{
					if ( pszSubCommands[iParameter] && Q_strncasecmp( pszVal, pszSubCommands[iParameter], strlen( pszSubCommands[iParameter] ) ) )
						continue;

					Q_snprintf( commands[ iCompletions ], sizeof( commands[ iCompletions ] ), "%s %s", szReturnedCommand, pszVal );
					iCompletions++;
				}
				break;

			case BA_POSITION:
				for (j = 0; j < 5; j++)
				{
					Q_snprintf( commands[ iCompletions ], sizeof( commands[ iCompletions ] ), "%s %d", szReturnedCommand, j );
					iCompletions++;
				}
				break;

			case BA_RUNE:
				while ((pRune = CRuneData::GetData((RuneID)j++)) != NULL)
				{
					if (!pRune->m_bBuyable)
						continue;

					if (pRune->m_eType == RUNETYPE_INVALID)
						continue;

					if (pRune->m_eEffect == RE_DEFAULTSPELL)
						continue;

					if ( pszSubCommands[iParameter] && Q_strncasecmp( pRune->m_szName, pszSubCommands[iParameter], strlen( pszSubCommands[iParameter] ) ) )
						continue;

					Q_snprintf( commands[ iCompletions ], sizeof( commands[ iCompletions ] ), "%s %s", szReturnedCommand, pRune->m_szName );
					iCompletions++;
				}
				break;

			case BA_SLOT:
				for (j = 0; j < MAX_RUNES; j++)
				{
					Q_snprintf( commands[ iCompletions ], sizeof( commands[ iCompletions ] ), "%s %d", szReturnedCommand, j );
					iCompletions++;
				}
				break;

			case BA_MOD:
				for (j = 0; j < MAX_RUNE_MODS; j++)
				{
					Q_snprintf( commands[ iCompletions ], sizeof( commands[ iCompletions ] ), "%s %d", szReturnedCommand, j );
					iCompletions++;
				}
				break;

			case BA_ARMOR:
				while ((pArm = CArmamentData::GetData((ArmamentID)j++)) != NULL)
				{
					if (!pArm->m_bBuyable)
						continue;

					if (pArm->m_eType == ARMAMENTTYPE_INVALID)
						continue;

					if ( pszSubCommands[iParameter] && Q_strncasecmp( pArm->m_szName, pszSubCommands[iParameter], strlen( pszSubCommands[iParameter] ) ) )
						continue;

					Q_snprintf( commands[ iCompletions ], sizeof( commands[ iCompletions ] ), "%s %s", szReturnedCommand, pArm->m_szName );
					iCompletions++;
				}
				break;

			case BA_ATTACK:
				for (j = 0; j < ATTACK_BINDS; j++)
				{
					Q_snprintf( commands[ iCompletions ], sizeof( commands[ iCompletions ] ), "%s %d", szReturnedCommand, j );
					iCompletions++;
				}
				break;
			}
			break;
		}
	}

	return iCompletions;
}

#ifdef CLIENT_DLL
static ConCommand buy_config("c", CC_Buy_f, "", FCVAR_CLIENTCMD_CAN_EXECUTE);
#else
static ConCommand buy("buy", CC_Buy_f, "Buy something.", 0, BuyCompletion);
static ConCommand buy_client("b", CC_Buy_f, "");
#endif

DEFINE_BUYCOMMAND(BuyArmorCommand)
{
	// buy armor mythril
	// b a 3
	// buys some mythril armor

	if (args.ArgC() < 3)
	{
		ShowPanel( CF_ARMR_PANEL );
		return;
	}

	CArmament::GetWriteArmament(pPlayer)->BuyArmament(CArmamentData::AliasToArmamentID(args[2]));
}

DEFINE_BUYCOMMAND(BuyBindCommand)
{
	// buy bind 0 1 2
	// b b 0 1 2
	// binds attack1 to the third rune on the second weapon

	if (args.ArgC() < 5)
	{
		ShowPanel( CF_BIND_PANEL );
		return;
	}

	bool bRebind = false;
	if (args.ArgC() >= 6)
		bRebind = !!atoi(args[5]);

	CArmament* pWriteArmament;
	if (bRebind)
		pWriteArmament = pPlayer->GetActiveArmament();
	else
		pWriteArmament = CArmament::GetWriteArmament(pPlayer);

	pWriteArmament->Bind(CRuneData::AliasToRuneID(args[2]),
		atoi(args[3]),
		atoi(args[4]));
}

DEFINE_BUYCOMMAND(BuyRuneCommand)
{
	// buy rune fire 2 1 0
	// b r 0 2 1 0
	// buys a fire rune and places it on the first slot of the backup secondary weapon

	if (args.ArgC() < 6)
	{
		ShowPanel( CF_RUNE_PANEL );
		return;
	}

	CArmament::GetWriteArmament(pPlayer)->BuyRune(CRuneData::AliasToRuneID(args[2]),
		atoi(args[3]),
		atoi(args[4]),
		atoi(args[5]));
}

DEFINE_BUYCOMMAND(BuyWeaponCommand)
{
	// buy weapon rivenblade 0
	// b w 3 0
	// buys a rivenblade and places it in the primary weapon slot

	if (args.ArgC() < 4)
	{
		ShowPanel( CF_WEAP_PANEL );
		return;
	}

	CFWeaponID eWeapon = AliasToWeaponID(args[2]);

	// Can't buy the Pariah blade through the menu or command line.
	if (eWeapon == WEAPON_PARIAH)
		return;

	CArmament::GetWriteArmament(pPlayer)->BuyWeapon(eWeapon, atoi(args[3]));
}

DEFINE_BUYCOMMAND(MoveWeaponCommand)
{
	// buy move 0 1
	// b m 0 1
	// move something in the primary weapon slot to the secondary

	if (args.ArgC() < 4)
	{
		return;
	}

	CArmament::GetWriteArmament(pPlayer)->MoveWeapon(atoi(args[2]), atoi(args[3]));
}

DEFINE_BUYCOMMAND(DescribeCommand)
{
	CArmament::GetWriteArmament(pPlayer)->Describe(pPlayer);
}

DEFINE_BUYCOMMAND(HelpCommand)
{
	if (args.ArgC() > 2)
	{
		if (FStrEq(args[2], "weapon"))
		{
			ClientPrint( pPlayer, HUD_PRINTCONSOLE, "buy weapon [name] [slot:0-2]\n" );
		}
		else if (FStrEq(args[2], "rune"))
		{
			ClientPrint( pPlayer, HUD_PRINTCONSOLE, "buy rune [name] [weapon:0-4] [slot:0-5] [mod:0-5]\n" );
		}
		else if (FStrEq(args[2], "bind"))
		{
			ClientPrint( pPlayer, HUD_PRINTCONSOLE, "buy bind [attack:0-1] [weapon:0-4] [slot:0-5]\n" );
		}
		else if (FStrEq(args[2], "armor"))
		{
			ClientPrint( pPlayer, HUD_PRINTCONSOLE, "buy armor [name]\n" );
		}
		else
		{
			ClientPrint( pPlayer, HUD_PRINTCONSOLE, "Help with what, now?\n" );
		}
	}
	else
	{
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "buy [-][weapon|rune|bind|armor|describe]\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "buy weapon [name] [slot:0-2]\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "buy rune [name] [weapon:0-4] [slot:0-5] [mod:0-5]\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "buy bind [attack:0-1] [weapon:0-4] [slot:0-5]\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "buy armor [name]\n" );
	}
}

DEFINE_BUYCOMMAND(RemoveArmorCommand)
{
	if (args.ArgC() < 3)
	{
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "buy -armor [3:armor|4:accessory]\n" );
		return;
	}

	CArmament::GetWriteArmament(pPlayer)->RemoveArmament((armamenttype_e)atoi(args[2]));
}

DEFINE_BUYCOMMAND(RemoveBindCommand)
{
	if (args.ArgC() < 3)
	{
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "buy -bind [attack:0-1]\n" );
		return;
	}

	CArmament::GetWriteArmament(pPlayer)->RemoveBind(atoi(args[2]));
}

DEFINE_BUYCOMMAND(RemoveRuneCommand)
{
	if (args.ArgC() < 5)
	{
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "buy -rune [weapon:0-4] [slot:0-5] [mod:0-5]\n" );
		return;
	}

	CArmament::GetWriteArmament(pPlayer)->RemoveRune(atoi(args[2]), atoi(args[3]), atoi(args[4]));
}

DEFINE_BUYCOMMAND(RemoveWeaponCommand)
{
	if (args.ArgC() < 3)
	{
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "buy -weapon [weapon:0-2]\n" );
		return;
	}

	CArmament::GetWriteArmament(pPlayer)->RemoveWeapon(atoi(args[2]));
}

DEFINE_BUYCOMMAND(ResetCommand)
{
	CArmament::GetWriteArmament(pPlayer)->Reset();
}

#ifdef CLIENT_DLL
CON_COMMAND_F( cfg_save, "Save the current configuration to a file.", FCVAR_CLIENTCMD_CAN_EXECUTE )
{
	CCFPlayer *pPlayer = C_CFPlayer::GetLocalCFPlayer(); 
	if ( !pPlayer )
		return;

	if (args.ArgC() == 1)
	{
		CCFMenu::OpenMenu(CF_CHAR_PANEL, true);
		return;
	}

	CArmament::GetWriteArmament(pPlayer)->SaveConfig(atoi(args[1]));
}

CON_COMMAND_F( cfg_load, "Load a configuration file from the disk.", FCVAR_CLIENTCMD_CAN_EXECUTE )
{
	CCFPlayer *pPlayer = C_CFPlayer::GetLocalCFPlayer(); 
	if ( !pPlayer )
		return;

	if (args.ArgC() == 1)
	{
		CCFMenu::OpenMenu(CF_CHAR_PANEL, true);
		return;
	}

	CArmament::GetWriteArmament(pPlayer)->LoadConfig(args[1]);
}

CON_COMMAND_F( cfg_done, "Always end a config file with this.", FCVAR_CLIENTCMD_CAN_EXECUTE )
{
	CArmament::FinishConfig();
}

CON_COMMAND_F( cfg_name, "Name this configuration.", FCVAR_CLIENTCMD_CAN_EXECUTE )
{
	CCFPlayer *pPlayer = C_CFPlayer::GetLocalCFPlayer(); 
	if ( !pPlayer )
		return;

	if (args.ArgC() == 1)
		return;

	Q_memcpy(CArmament::GetWriteArmament(pPlayer)->m_szName, args[1], sizeof(CArmament::GetWriteArmament(pPlayer)->m_szName));
}

CON_COMMAND_F( cfg_img, "Graphic for this config.", FCVAR_CLIENTCMD_CAN_EXECUTE )
{
	CCFPlayer *pPlayer = C_CFPlayer::GetLocalCFPlayer(); 
	if ( !pPlayer )
		return;

	if (args.ArgC() == 1)
		return;

	Q_memcpy(CArmament::GetWriteArmament(pPlayer)->m_szImage, args[1], sizeof(CArmament::GetWriteArmament(pPlayer)->m_szImage));
}

CON_COMMAND_F( cfg_level, "Difficulty level for this config.", FCVAR_CLIENTCMD_CAN_EXECUTE )
{
	CCFPlayer *pPlayer = C_CFPlayer::GetLocalCFPlayer(); 
	if ( !pPlayer )
		return;

	if (args.ArgC() == 1)
		return;

	CArmament::GetWriteArmament(pPlayer)->m_iLevel = atoi(args[1]);
}
#endif

writeto_t CArmament::s_eWriteTo = WRITETO_SERVER;
FileHandle_t CArmament::s_hWriteToFile = NULL;
CArmament* CArmament::s_pWriteToArmament = NULL;
CArmament CArmament::s_PariahArmament;

CArmament::CArmament()
{
	m_iNoNetwork = 0;
	Q_strncpy(m_szName, "", sizeof(m_szName));
	Q_strncpy(m_szFile, "", sizeof(m_szFile));
	Reset();
}

void CArmament::Reset()
{
	m_iSlots = MAX_WEIGHT;
	m_iCredits = mp_inventory.GetInt();
	m_bPowerjump = false;
	m_bGrip = false;
	m_aRuneCombos.Purge();

	int i;
	for (i = 0; i < 3; i++)
	{
		m_aWeapons[i].m_iWeapon = WEAPON_NONE;
		m_aWeapons[i].m_pWeaponData = NULL;
	}

	for (i = 3; i < 5; i++)
	{
		m_aWeapons[i].m_iArmament = CArmamentData::InvalidArmament();
		m_aWeapons[i].m_pArmamentData = NULL;
	}

	for (i = 0; i < 5; i++)
	{
		for (int j = 0; j < MAX_RUNES; j++)
		{
			for (int k = 0; k < MAX_RUNE_MODS; k++)
			{
				m_aWeapons[i].m_aRunes[j][k] = CRuneData::InvalidRune();
				m_aWeapons[i].m_apRuneData[j][k] = NULL;
			}
		}
	}

	for (i = 0; i < ATTACK_BINDS; i++)
	{
		m_aAttackBinds[i].m_iWeapon = -1;
		m_aAttackBinds[i].m_iRune = -1;
	}
}

void CArmament::InitializePariahArmament()
{
	WriteToArmament(&s_PariahArmament);
	s_PariahArmament.BuyWeapon(WEAPON_PARIAH, 0);
	s_PariahArmament.BuyRune(CRuneData::AliasToRuneID("powerjump"), 0, 0, 0);
	s_PariahArmament.BuyArmament(CArmamentData::AliasToArmamentID("Chestplate"));
	WriteToServer();
}

#if defined(ARCON_MOD) || defined(ARCON_DEMO)
bool CArmament::CanBuyWeaponInDemo(CFWeaponID iWeapon)
{
	if (iWeapon == WEAPON_PISTOL || iWeapon == WEAPON_RIVENBLADE || iWeapon == WEAPON_RIFLE)
		return true;

#ifdef ARCON_MOD
	// These are available in the mod build (locked for playtesting) but not available in the demo.
	return true;
#endif

	return false;
}

bool CArmament::CanBuyRuneInDemo(RuneID iRune)
{
	CRuneData* pData = CRuneData::GetData(iRune);

	if (!pData)
		return false;

	if (pData->m_eEffect == RE_SPELL || pData->m_eEffect == RE_BULLET || pData->m_eEffect == RE_AOE ||
		pData->m_eEffect == RE_ELEMENT || pData->m_eEffect == RE_POWERJUMP || pData->m_eEffect == RE_LATCH ||
		pData->m_eEffect == RE_RESTORE)
		return true;

#ifdef ARCON_MOD
	// These are available in the mod build (locked for playtesting) but not available in the demo.
	return true;
#endif

	return false;
}
#endif

bool CArmament::CanBuyRune(RuneID iRune, int iWeapon, int iSlot, int iMod)
{
#if defined(ARCON_MOD) || defined(ARCON_DEMO)
	// The mod demo only allows the player to buy default spells and some force runes.
	if (!CanBuyRuneInDemo(iRune))
		return false;
#endif

	if (iWeapon < 0 || iWeapon > 4)
		return false;

	if (iSlot < 0 || iSlot > MAX_RUNES)
		return false;

	if (iMod < 0 || iMod > MAX_RUNE_MODS)
		return false;

	if (!GetWeaponData(iWeapon) && !GetArmamentData(iWeapon))
		return false;

	if (CRuneData::GetData(m_aWeapons[iWeapon].m_aRunes[iSlot][iMod]) != NULL)
		return false;

	if (GetWeaponData(iWeapon) && !GetWeaponData(iWeapon)->m_iRuneSlots[iSlot])
		return false;

	if (GetArmamentData(iWeapon) && !GetArmamentData(iWeapon)->m_iRuneSlots[iSlot])
		return false;

	CRuneData* pData = CRuneData::GetData(iRune);

	if (!pData)
		return false;

	if (!pData->m_bBuyable)
		return false;

	if (pData->m_iCost > m_iCredits)
		return false;

	switch (pData->m_eType)
	{
	case RUNETYPE_FORCE:
	case RUNETYPE_SUPPORT:
	case RUNETYPE_BASE:
		if (GetWeaponData(iWeapon) && iMod >= GetWeaponData(iWeapon)->m_iRuneSlots[iSlot])
			return false;

		if (GetArmamentData(iWeapon) && iMod >= GetArmamentData(iWeapon)->m_iRuneSlots[iSlot])
			return false;

		break;

	default:
		return false;
	}

	return true;
}

bool CArmament::BuyRune(RuneID iRune, int iWeapon, int iSlot, int iMod)
{
	if (!CanBuyRune(iRune, iWeapon, iSlot, iMod))
		return false;

	CRuneData* pData = CRuneData::GetData(iRune);

	m_iCredits -= pData->m_iCost;
	m_aWeapons[iWeapon].m_aRunes[iSlot][iMod] = iRune;
	m_aWeapons[iWeapon].m_apRuneData[iSlot][iMod] = pData;

	// If this is a bindable combo and there is not already a combo for this slot, create one.
	CRunePosition ComboPosition(iWeapon, iSlot, 0);
	if (IsBindableCombo(&ComboPosition) && GetRuneCombo(iWeapon, iSlot) == -1)
        m_aRuneCombos.AddToTail(ComboPosition);

	WriteRune(iWeapon, iSlot, iMod);

	return true;
}

void CArmament::WriteRune(int iWeapon, int iSlot, int iMod, bool bComments)
{
	if (bComments)
	{
		CRuneData* pRune = m_aWeapons[iWeapon].m_apRuneData[iSlot][iMod];
		WriteString( VarArgs("// Rune: %s", pRune->m_szName) );
	}

	// Expand the name of the rune if comments is on to prevent changes to runes.txt
	// from altering the rune IDs and thus screwing up config files.
	char szName[256];
	if (bComments)
		strcpy(szName, VarArgs("%s", m_aWeapons[iWeapon].m_apRuneData[iSlot][iMod]->m_szName));
	else
		strcpy(szName, VarArgs("%d", m_aWeapons[iWeapon].m_aRunes[iSlot][iMod]));

	WriteString( VarArgs("%s r %s %d %d %d",
		GetBuyCommand(),
		szName, iWeapon, iSlot, iMod) );
}

bool CArmament::CanBuyCombo(CRuneComboPreset* pCombo, int iWeapon, int iSlot)
{
	int iCurrentComboCost = 0;
	int i;
	for (i = 0; i < MAX_RUNE_MODS; i++)
	{
		if (m_aWeapons[iWeapon].m_aRunes[iSlot][i] != CRuneData::InvalidRune())
		{
			CRuneData* pData = CRuneData::GetData(m_aWeapons[iWeapon].m_aRunes[iSlot][i]);
			iCurrentComboCost += pData->m_iCost;
		}
	}

	if ((int)m_iCredits < (int)pCombo->m_iCredits - iCurrentComboCost)
		return false;

	return true;
}

bool CArmament::BuyCombo(CRuneComboPreset* pCombo, int iWeapon, int iSlot)
{
	if (!CanBuyCombo(pCombo, iWeapon, iSlot))
		return false;

	RemoveCombo(iWeapon, iSlot);

	int i;

	// Purchase the new combo
	for (i = 0; i < MAX_RUNE_MODS; i++)
		BuyRune(pCombo->m_aRunes[i], iWeapon, iSlot, i);

	return true;
}

void CArmament::RemoveCombo(int iWeapon, int iSlot)
{
	for (int i = 0; i < MAX_RUNE_MODS; i++)
		RemoveRune(iWeapon, iSlot, i);
}

// iOptions = 1 pretends that the weapon currently in the desired slot has already been removed.
// iOptions = 2 pretends that the weapon currently in the desired slot has been moved to another slot.
bool CArmament::CanBuyWeapon(CFWeaponID iWeapon, int iSlot, int iOptions, int* pMoveTo)
{
	// Makes no sense to remove first if there's nothing there in the first place!
	if (m_aWeapons[iSlot].m_iWeapon == WEAPON_NONE)
		iOptions = 0;

#if defined(ARCON_MOD) || defined(ARCON_DEMO)
	// The mod demo only allows the player to buy one of these two weapons.
	if (!CanBuyWeaponInDemo(iWeapon))
		return false;
#endif

	if (iWeapon <= WEAPON_NONE || iWeapon >= WEAPON_MAX)
		return false;

	if (iWeapon == WEAPON_MAGIC)
		return false;

	if (iSlot < 0 || iSlot > 2)
		return false;

	if (m_aWeapons[iSlot].m_iWeapon && iOptions == 0)
		return false;

	// If we are checking based on the assumption that the current weapon will be moved,
	// check if there are available slots. If we want a primary slot, check if the secondary
	// slots are open and that the weapon currently in the primary can be moved to secondary.
	if (iOptions == 2 && iSlot == 0)
	{
		if (m_aWeapons[iSlot].m_pWeaponData && m_aWeapons[0].m_pWeaponData->iWeight > 2)
			return false;

		if (m_aWeapons[1].m_iWeapon != WEAPON_NONE)
		{
			if (pMoveTo != NULL)
				*pMoveTo = 1;
		}
		else if (m_aWeapons[2].m_iWeapon != WEAPON_NONE)
		{
			if (pMoveTo != NULL)
				*pMoveTo = 2;
		}
		else
			return false;
	}

	// If we are checking based on the assumption that the current weapon will be moved,
	// check if there are available slots. If we want a secondary slot, check if the primary
	// slot is open and that the weapon currently in the secondary can be moved to primary.
	if (iOptions == 2 && (iSlot == 1 || iSlot == 2))
	{
		if (m_aWeapons[iSlot].m_pWeaponData && m_aWeapons[iSlot].m_pWeaponData->iWeight < 2)
			return false;

		if (m_aWeapons[0].m_iWeapon)
			return false;

		if (pMoveTo != NULL)
			*pMoveTo = 0;
	}

	int iSlots = m_iSlots;
	unsigned int iCredits = m_iCredits;

	if (iOptions == 1)
	{
		iSlots += m_aWeapons[iSlot].m_pWeaponData->iWeight;
		iCredits += m_aWeapons[iSlot].m_pWeaponData->m_iCost;
	}

	CCFWeaponInfo* pWpnData = WeaponIDToInfo(iWeapon);

	if (pWpnData->iWeight > iSlots)
		return false;

	if (iSlot == 0 && (pWpnData->iWeight < MIN_PRIMARY_WEIGHT || pWpnData->iWeight > MAX_WEIGHT))
		return false;

	if (iSlot != 0 && (pWpnData->iWeight < MIN_SECONDARY_WEIGHT || pWpnData->iWeight > MIN_PRIMARY_WEIGHT))
		return false;

	if (pWpnData->m_iCost > iCredits)
		return false;

	// OK! This is a good purchase.
	return true;
}

bool CArmament::BuyWeapon(CFWeaponID iWeapon, int iSlot)
{
	if (!CanBuyWeapon(iWeapon, iSlot))
		return false;

	CCFWeaponInfo* pWpnData = WeaponIDToInfo(iWeapon);

	m_iSlots -= pWpnData->iWeight;
	m_iCredits -= pWpnData->m_iCost;
	m_aWeapons[iSlot].m_iWeapon = iWeapon;
	m_aWeapons[iSlot].m_pWeaponData = pWpnData;

	WriteWeapon(iSlot);

	return true;
}

int CArmament::CanBuyWeapon(CFWeaponID iWeapon, int* pMoveTo)
{
	int i = 0;

	// Try to find a free space first
	for (i = 0; i < 3; i++)
		if (CanBuyWeapon(iWeapon, i))
			return i;

	// Settle for moving another weapon
	for (i = 0; i < 3; i++)
		if (CanBuyWeapon(iWeapon, i, 2, pMoveTo))
			return i;

	return -1;
}

bool CArmament::BuyWeapon(CFWeaponID iWeapon)
{
	int iMoveTo = -1;
	int iSlot = CanBuyWeapon(iWeapon, &iMoveTo);

	if (iSlot == -1)
		return false;

	if (iMoveTo >= 0 && m_aWeapons[iSlot].m_iWeapon)
		MoveWeapon(iSlot, iMoveTo);

	return BuyWeapon(iWeapon, iSlot);
}

bool CArmament::MoveWeapon(int iSlot, int iNewSlot)
{
	int i;

	if (iSlot < 0 || iNewSlot < 0 || iSlot >= 3 || iNewSlot >=3 || iSlot == iNewSlot)
		return false;

	if (m_aWeapons[iSlot].m_iWeapon == WEAPON_NONE)
		return false;

	if (m_aWeapons[iNewSlot].m_iWeapon != WEAPON_NONE)
		return false;

	// Copy this whole weapon over to the new slot.
	m_aWeapons[iNewSlot] = m_aWeapons[iSlot];	// Copy constructor?!

	m_aWeapons[iSlot].m_iWeapon = WEAPON_NONE;
	m_aWeapons[iSlot].m_pWeaponData = NULL;

	for (i = 0; i < MAX_RUNES; i++)
	{
		for (int j = 0; j < MAX_RUNES; j++)
		{
			m_aWeapons[iSlot].m_aRunes[i][j] = CRuneData::InvalidRune();
			m_aWeapons[iSlot].m_apRuneData[i][j] = NULL;
		}
	}

	for (i = 0; i < m_aRuneCombos.Count(); i++)
		if (m_aRuneCombos[i].m_iWeapon == iSlot)
			m_aRuneCombos[i].m_iWeapon = iNewSlot;

	for (i = 0; i < ATTACK_BINDS; i++)
		if (m_aAttackBinds[i].m_iWeapon == iSlot)
			m_aAttackBinds[i].m_iWeapon = iNewSlot;

	WriteString( VarArgs("%s m %d %d",
		GetBuyCommand(),
		iSlot, iNewSlot) );

	return true;
}

void CArmament::WriteWeapon(int iSlot, bool bComments)
{
	if (bComments)
		WriteString( VarArgs("// Weapon: %s", WeaponIDToAlias(m_aWeapons[iSlot].m_iWeapon)) );

	WriteString( VarArgs("%s w %d %d",
		GetBuyCommand(),
		m_aWeapons[iSlot].m_iWeapon, iSlot) );
}

bool CArmament::CanBind(int iAttack, int iWeapon, int iSlot)
{
	if (iAttack < 0 || iAttack > 1)
		return false;

	if (iWeapon < 0 || iWeapon > 4)
		return false;

	if (iSlot < 0 || iSlot > MAX_RUNES)
		return false;

	if (!GetWeaponData(iWeapon) && !GetArmamentData(iWeapon))
		return false;

	int iCombo = GetRuneCombo(iWeapon, iSlot);

	if (iCombo == -1)
		return false;

	CRunePosition* pRune = GetRuneCombo(iCombo);

	if (!IsValidCombo(pRune))
		return false;

	return true;
}

bool CArmament::Bind(int iAttack, int iWeapon, int iSlot, bool bRebind)
{
	if (!CanBind(iAttack, iWeapon, iSlot))
		return false;

	m_aAttackBinds[iAttack].m_iWeapon = iWeapon;
	m_aAttackBinds[iAttack].m_iRune = iSlot;

	WriteBind( iAttack, false, bRebind );

	return true;
}

void CArmament::WriteBind(int iAttack, bool bComments, bool bRebind)
{
	if (bComments)
	{
		CRuneData* pRune = m_aWeapons[m_aAttackBinds[iAttack].m_iWeapon].m_apRuneData[m_aAttackBinds[iAttack].m_iRune][0];
		WriteString( VarArgs("// Bind: attack%d to %s", iAttack, pRune->m_szName) );
	}

	WriteString( VarArgs("%s b %d %d %d %d",
		GetBuyCommand(),
		iAttack, m_aAttackBinds[iAttack].m_iWeapon, m_aAttackBinds[iAttack].m_iRune,
		bRebind) );
}

bool CArmament::CanBuyArmament(ArmamentID iArm)
{
	CArmamentData* pData = CArmamentData::GetData(iArm);

	if (!pData)
		return false;

	CArmamentData* pCurrent = CArmamentData::GetData(m_aWeapons[pData->m_eType].m_iArmament);

	int iCurrentCost = 0;
	if (pCurrent)
		iCurrentCost = pCurrent->m_iCost;

	if (!pData)
		return false;

	if (!pData->m_bBuyable)
		return false;

	if (pData->m_iCost > m_iCredits + iCurrentCost)
		return false;

	if (pData->m_eType == ARMAMENTTYPE_INVALID)
		return false;

	// OK! This is a good purchase.
	return true;
}

bool CArmament::BuyArmament(ArmamentID iArm)
{
	if (!CanBuyArmament(iArm))
		return false;

	CArmamentData* pData = CArmamentData::GetData(iArm);

	CArmamentData* pCurrent = CArmamentData::GetData(m_aWeapons[pData->m_eType].m_iArmament);

	if (pCurrent)
		m_iCredits += pCurrent->m_iCost;

	m_iCredits -= pData->m_iCost;

	m_aWeapons[pData->m_eType].m_iArmament = iArm;
	m_aWeapons[pData->m_eType].m_pArmamentData = pData;

	Assert(m_iNoNetwork >= 0);

	// Remove runes that won't fit on this armor.
	m_iNoNetwork++;
	for (int i = 0; i < MAX_RUNES; i++)
	{
		for (int j = 0; j < MAX_RUNE_MODS; j++)
		{
			if (j >= pData->m_iRuneSlots[i])
				RemoveRune(pData->m_eType, i, j);
		}
	}
	m_iNoNetwork--;

	Assert(m_iNoNetwork >= 0);

	WriteArmament(pData->m_eType);

	CalculateAbilities();

	return true;
}

void CArmament::WriteArmament(armamenttype_t iArm, bool bComments)
{
	if (bComments)
		WriteString( VarArgs("// Armament: %s", GetArmamentData(iArm)->m_szName) );

	WriteString( VarArgs("%s a %d", 
		GetBuyCommand(),
		m_aWeapons[iArm].m_iArmament) );
}

void CArmament::Buy(CCFPlayer* pPlayer, bool bWeaponsAndHealth)
{
	pPlayer->m_pStats->InitStats();

	CalculateAbilities();

#ifdef GAME_DLL
	if (bWeaponsAndHealth)
	{
		pPlayer->RemoveAllItems(false);

		for (int i = 0; i < 3; i++)
		{
			if (m_aWeapons[i].m_iWeapon)
			{
				// Give the player his weapon
				EHANDLE pent = CreateEntityByName(VarArgs("weapon_%s", WeaponIDToAlias(m_aWeapons[i].m_iWeapon)));
				AssertMsg(pent, "Purchased weapon doesn't exist.");

				pent->SetLocalOrigin( pPlayer->GetLocalOrigin() );
				pent->AddSpawnFlags( SF_NORESPAWN );

				DispatchSpawn( pent );

				pPlayer->Weapon_Equip( dynamic_cast<CWeaponCFBase*>((CBaseEntity*)pent), i );
			}
		}
	}

	pPlayer->m_iMaxHealth				= 1000;
	pPlayer->m_pStats->m_iMaxFocus		= 100;
	pPlayer->m_pStats->m_iMaxStamina	= 100;

	pPlayer->m_pStats->AddArmamentModifiers();

	if (bWeaponsAndHealth)
	{
		pPlayer->m_iHealth				= pPlayer->m_iMaxHealth;
		pPlayer->m_pStats->m_flFocus	= pPlayer->m_pStats->m_iMaxFocus;
		pPlayer->m_pStats->m_flStamina	= pPlayer->m_pStats->m_iMaxStamina;

		pPlayer->m_iHealth += pPlayer->m_flRespawnBonus;

		if (pPlayer->m_flRespawnBonus > 1)
		{
			CSingleUserRecipientFilter user( pPlayer );
			user.MakeReliable();
			UserMessageBegin( user, "CFMessage" );
				WRITE_BYTE( MESSAGE_BONUSHEALTH );
				WRITE_SHORT( pPlayer->m_flRespawnBonus );
			MessageEnd();
		}
	}
#endif
}

void CArmament::CalculateAbilities()
{
	m_bPowerjump = false;
	m_bGrip = false;

	for (int i = 3; i < 5; i++)
	{
		if (m_aWeapons[i].m_iArmament)
		{
			CArmamentData* pData = CArmamentData::GetData(m_aWeapons[i].m_iArmament);

			if (!pData)
				continue;

			if (pData->m_bPowerjump)
				m_bPowerjump = true;
			if (pData->m_bGrip)
				m_bGrip = true;
		}
	}
}

void CArmament::RemoveWeapon(CFWeaponID iWeapon)
{
	if (iWeapon <= WEAPON_NONE || iWeapon >= WEAPON_MAX)
		return;

	for (int i = 0; i < 3; i++)
	{
		if (m_aWeapons[i].m_iWeapon == iWeapon)
			RemoveWeapon(m_aWeapons[i].m_iWeapon);
	}
}

void CArmament::RemoveWeapon(int iSlot)
{
	if (iSlot < 0 || iSlot > 2)
		return;

	CFWeaponID iWeapon = m_aWeapons[iSlot].m_iWeapon;

	if (iWeapon <= WEAPON_NONE || iWeapon >= WEAPON_MAX)
		return;

	CCFWeaponInfo* pWpnData = WeaponIDToInfo(iWeapon);

	m_iSlots += pWpnData->iWeight;
	m_iCredits += pWpnData->m_iCost;
	m_aWeapons[iSlot].m_iWeapon = WEAPON_NONE;
	m_aWeapons[iSlot].m_pWeaponData = NULL;

	Assert(m_iNoNetwork >= 0);

	// Remove all runes on this weapon.
	m_iNoNetwork++;
	for (int i = 0; i < MAX_RUNES; i++)
	{
		for (int j = 0; j < MAX_RUNE_MODS; j++)
		{
			RemoveRune(iSlot, i, j);
		}
	}
	m_iNoNetwork--;

	Assert(m_iNoNetwork >= 0);

	WriteString( VarArgs("%s -w %d",
		GetBuyCommand(),
		iSlot) );
}

void CArmament::RemoveRune(int iWeapon, int iSlot, int iMod)
{
	if (iWeapon < 0 || iWeapon > 4)
		return;

	if (iSlot < 0 || iSlot >= MAX_RUNES)
		return;

	if (iMod < 0 || iMod >= MAX_RUNE_MODS)
		return;

	CRuneData* pData = CRuneData::GetData(m_aWeapons[iWeapon].m_aRunes[iSlot][iMod]);

	if (!pData)
		return;

	m_iCredits += pData->m_iCost;
	m_aWeapons[iWeapon].m_aRunes[iSlot][iMod] = CRuneData::InvalidRune();
	m_aWeapons[iWeapon].m_apRuneData[iSlot][iMod] = NULL;

	Assert(m_iNoNetwork >= 0);
	
	// If this combination is no longer bindable, remove all of its binds.
	m_iNoNetwork++;
	CRunePosition RunePos(iWeapon, iSlot, 0);
	if (!IsBindableCombo(&RunePos))
	{
		int i = GetRuneCombo(iWeapon, iSlot);
		if (i >= 0)
			m_aRuneCombos.Remove(i);

		while (GetAttackBind(iWeapon, iSlot) >= 0)
			RemoveBind(GetAttackBind(iWeapon, iSlot));
	}
	m_iNoNetwork--;

	Assert(m_iNoNetwork >= 0);

	WriteString( VarArgs("%s -r %d %d %d",
		GetBuyCommand(),
		iWeapon, iSlot, iMod) );
}

void CArmament::RemoveBind(int iAttack)
{
	if (iAttack < 0 || iAttack > ATTACK_BINDS)
		return;

	m_aAttackBinds[iAttack].m_iWeapon = -1;
	m_aAttackBinds[iAttack].m_iRune = -1;

	WriteString( VarArgs("%s -b %d",
		GetBuyCommand(),
		iAttack) );
}

void CArmament::RemoveArmament(armamenttype_t iArm)
{
	if (iArm != ARMAMENTTYPE_ARMOR && iArm != ARMAMENTTYPE_ITEM)
		return;

	CArmamentData* pData = CArmamentData::GetData(m_aWeapons[iArm].m_iArmament);

	if (!pData)
		return;

	m_aWeapons[iArm].m_iArmament = CArmamentData::InvalidArmament();
	m_aWeapons[iArm].m_pArmamentData = NULL;

	m_iCredits += pData->m_iCost;

	Assert(m_iNoNetwork >= 0);

	// Remove all runes on this weapon.
	m_iNoNetwork++;
	for (int i = 0; i < MAX_RUNES; i++)
	{
		for (int j = 0; j < MAX_RUNE_MODS; j++)
		{
			RemoveRune(iArm, i, j);
		}
	}
	m_iNoNetwork--;

	Assert(m_iNoNetwork >= 0);

	WriteString( VarArgs("%s -a %d",
		GetBuyCommand(),
		iArm) );
}

void CArmament::Describe(CCFPlayer* pPlayer)
{
	ConPrint( "Armament report:\n" );

	if (m_aWeapons[0].m_iWeapon)
	{
		ConPrint( " Primary Weapon:\n" );
		DescribeWeapon(pPlayer, 0);
	}

	if (m_aWeapons[1].m_iWeapon)
	{
		ConPrint( " Active Secondary Weapon:\n" );
		DescribeWeapon(pPlayer, 1);
	}

	if (m_aWeapons[2].m_iWeapon)
	{
		ConPrint( " Inactive Secondary Weapon:\n" );
		DescribeWeapon(pPlayer, 2);
	}

	if (GetArmamentData(3))
	{
		ConPrint( " Armor:\n" );
		DescribeWeapon(pPlayer, 3);
	}

	if (GetArmamentData(4))
	{
		ConPrint( " Accessory:\n" );
		DescribeWeapon(pPlayer, 4);
	}

	for (int i = 0; i < ATTACK_BINDS; i++)
	{
		if (m_aAttackBinds[i].m_iWeapon == -1)
			continue;

		CRuneData* pRune = m_aWeapons[m_aAttackBinds[i].m_iWeapon].m_apRuneData[m_aAttackBinds[i].m_iRune][0];
		Assert(pRune);

		ConPrint( VarArgs(" Bind: attack%d to %s\n", i, pRune->m_szName) );
	}

	ConPrint( VarArgs("Credits remaining: %d.\n", m_iCredits) );
	ConPrint( "End of report.\n" );
}

void CArmament::DescribeWeapon(CCFPlayer* pPlayer, int iSlot)
{
	if (GetWeaponData(iSlot))
	{
		ConPrint( VarArgs("  Name: %s\n", WeaponIDToAlias(m_aWeapons[iSlot].m_iWeapon)) );
		ConPrint( VarArgs("  Slots: %d/%d %d/%d %d/%d %d/%d %d/%d %d/%d\n",
			GetRunesInSlot(iSlot, 0), GetWeaponData(iSlot)->m_iRuneSlots[0],
			GetRunesInSlot(iSlot, 1), GetWeaponData(iSlot)->m_iRuneSlots[1],
			GetRunesInSlot(iSlot, 2), GetWeaponData(iSlot)->m_iRuneSlots[2],
			GetRunesInSlot(iSlot, 3), GetWeaponData(iSlot)->m_iRuneSlots[3],
			GetRunesInSlot(iSlot, 4), GetWeaponData(iSlot)->m_iRuneSlots[4],
			GetRunesInSlot(iSlot, 5), GetWeaponData(iSlot)->m_iRuneSlots[5]
			) );
	}
	else if (GetArmamentData(iSlot))
	{
		ConPrint( VarArgs("  Name: %s\n", GetArmamentData(iSlot)->m_szName) );
		ConPrint( VarArgs("  Slots: %d/%d %d/%d %d/%d %d/%d %d/%d %d/%d\n",
			GetRunesInSlot(iSlot, 0), GetArmamentData(iSlot)->m_iRuneSlots[0],
			GetRunesInSlot(iSlot, 1), GetArmamentData(iSlot)->m_iRuneSlots[1],
			GetRunesInSlot(iSlot, 2), GetArmamentData(iSlot)->m_iRuneSlots[2],
			GetRunesInSlot(iSlot, 3), GetArmamentData(iSlot)->m_iRuneSlots[3],
			GetRunesInSlot(iSlot, 4), GetArmamentData(iSlot)->m_iRuneSlots[4],
			GetRunesInSlot(iSlot, 5), GetArmamentData(iSlot)->m_iRuneSlots[5]
			) );
	}

	for (int i = 0; i < MAX_RUNES; i++)
		DescribeRune(pPlayer, iSlot, i);
}

void CArmament::DescribeRune(CCFPlayer* pPlayer, int iSlot, int iRune)
{
	CRuneData* pRune;

	if (GetRunesInSlot(iSlot, iRune) == 0)
		return;

	ConPrint( VarArgs("   Rune:\n") );

	int iRuneSlots = 0;
	if (GetWeaponData(iSlot))
		iRuneSlots = GetWeaponData(iSlot)->m_iRuneSlots[iRune];
	else if (GetArmamentData(iSlot))
		iRuneSlots = GetArmamentData(iSlot)->m_iRuneSlots[iRune];

	for (int j = 0; j < iRuneSlots; j++)
	{
		pRune = m_aWeapons[iSlot].m_apRuneData[iRune][j];

		if (!pRune)
			continue;

		if (pRune->m_eType == RUNETYPE_BASE)
			ConPrint( VarArgs("    Base: %s\n", pRune->m_szName) );
	}

	for (int j = 0; j < iRuneSlots; j++)
	{
		pRune = m_aWeapons[iSlot].m_apRuneData[iRune][j];

		if (!pRune)
			continue;

		if (pRune->m_eType == RUNETYPE_FORCE)
			ConPrint( VarArgs("    Force: %s\n", pRune->m_szName) );
	}

	for (int j = 0; j < iRuneSlots; j++)
	{
		pRune = m_aWeapons[iSlot].m_apRuneData[iRune][j];

		if (!pRune)
			continue;

		if (pRune->m_eType == RUNETYPE_SUPPORT)
			ConPrint( VarArgs("    Mod: %s\n", pRune->m_szName) );
	}
}

#ifdef CLIENT_DLL
void CArmament::SaveConfig(int iIndex)
{
	char szGameDir[256];
	FileHandle_t hNewConfig;

	Q_snprintf(szGameDir, 256, "%s/" CFG_PATH "/user%d.cfg", engine->GetGameDirectory(), iIndex);
	hNewConfig = filesystem->Open(szGameDir, "wt");

	WriteToFile(hNewConfig);
	WriteString("// This is a Arcon armament configuration file.");
	WriteString("// Please don't modify it unless you know what you are doing.");
	WriteString("// Always start the file with \"c -\" and end it with \"cfg_done\".");
	WriteString("// http://www.calamityfuse.com/");
	WriteAll(true);
	WriteString("cfg_done");
	WriteToServer();

	filesystem->Close(hNewConfig);

	Q_snprintf(m_szFile, sizeof(m_szFile), "user%d", iIndex);
}

void CArmament::LoadConfig(const char* pszConfig)
{
	Reset();
	WriteToArmament(this);
	engine->ClientCmd( VarArgs("exec " CFG_DIR "/%s.cfg", pszConfig) );

	Q_strncpy(m_szFile, pszConfig, sizeof(m_szFile));
}

void CArmament::FinishConfig()
{
	CArmament* pArmament = GetWriteArmament(C_CFPlayer::GetLocalCFPlayer());

	WriteToServer();

	if (strlen(pArmament->m_szName) == 0)
		Q_strncpy(pArmament->m_szName, pArmament->m_szFile, sizeof(pArmament->m_szName));

	// If we're loading it into the player's config, write it out to the server.
	if (pArmament == GetWriteArmament(C_CFPlayer::GetLocalCFPlayer()))
	{
		pArmament->WriteAll();
		cfgui::CRootPanel::UpdateArmament(pArmament);
	}
}
#endif

class CIntLookupLess
{
public:
	bool Less( const int& lhs, const int& rhs, void *pContext )
	{
		return lhs < rhs;
	}
};

int CArmament::GetUnusedUserConfigIndex()
{
	FileFindHandle_t hFindHandle;

	const char* pszName = filesystem->FindFirst( CFG_PATH "/user*.cfg", &hFindHandle );

	CUtlSortVector<int, CIntLookupLess> aIndices(0, 0);

	while (pszName)
	{
		// Isolate the file name by chopping off the extension.
		char pszFilepath[256];
		Q_strncpy(pszFilepath, pszName, 256);

		char *pszFilename = pszFilepath;
		pszFilename[strlen(pszFilename)-4] = '\0';

		pszFilename+=4;

		aIndices.Insert(atoi(pszFilename));

		pszName = filesystem->FindNext( hFindHandle );
	}
	
	filesystem->FindClose( hFindHandle );

	// If there's nothing, return 1.
	if (aIndices.Count() == 0)
		return 1;

	// If the first number is not 1, then we'll just use 1. Easy as pie.
	if (aIndices[0] > 1)
		return 1;

	int iLast = -1;
	for (int i = 0; i < aIndices.Count(); i++)
	{
		// If the current one is more than 1 larger than the previous, there must be a space, so use the previous+1.
		if (iLast != -1 && aIndices[i] > iLast+1)
			return iLast+1;

		iLast = aIndices[i];
	}

	// No spaces on the inside. Use the one after the last one in the list.
	return aIndices[aIndices.Count()-1]+1;
}

void CArmament::WriteToServer()
{
#ifdef CLIENT_DLL
	s_eWriteTo = WRITETO_SERVER;
	s_hWriteToFile = NULL;
	s_pWriteToArmament = NULL;
#endif
}

void CArmament::WriteToFile(FileHandle_t hFile)
{
#ifdef CLIENT_DLL
	s_eWriteTo = WRITETO_FILE;
	s_hWriteToFile = hFile;
	s_pWriteToArmament = NULL;
#endif
}

void CArmament::WriteToArmament(CArmament* pArmament)
{
#ifdef CLIENT_DLL
	AssertMsg(s_eWriteTo == WRITETO_SERVER, "Started writing another armament before finishing a previous.");
	s_eWriteTo = WRITETO_ARMAMENT;
	s_hWriteToFile = NULL;
	s_pWriteToArmament = pArmament;
#endif
}

void CArmament::WriteString(char* pszOut)
{
	switch (s_eWriteTo)
	{
	default:
		AssertMsg(false, "Invalid s_eWriteTo.");

		// Use WT_SERVER as default.
	case WRITETO_SERVER:
#ifdef CLIENT_DLL
		if (m_iNoNetwork == 0)
			engine->ClientCmd( pszOut );
#endif
		break;

	case WRITETO_FILE:
		filesystem->FPrintf(s_hWriteToFile, "%s\n", pszOut);
		break;

	case WRITETO_ARMAMENT:
		// No need, all the work was already done in the buying function.
		break;
	}
}

CArmament* CArmament::GetWriteArmament(CCFPlayer* pPlayer)
{
	switch (s_eWriteTo)
	{
	default:
		AssertMsg(false, "Invalid s_eWriteTo.");

		// Use WT_SERVER as default.
	case WRITETO_SERVER:
	case WRITETO_FILE:
		return pPlayer->m_pArmament;

	case WRITETO_ARMAMENT:
		return s_pWriteToArmament;
	}
}

char* CArmament::GetBuyCommand()
{
	switch (s_eWriteTo)
	{
	default:
		AssertMsg(false, "Invalid s_eWriteTo.");

		// Use WT_SERVER as default.
	case WRITETO_SERVER:
	case WRITETO_ARMAMENT:
		return "b";

	case WRITETO_FILE:
		return "c";
	}
}

void CArmament::WriteAll(bool bComments)
{
	int i;

	WriteString( VarArgs("%s -", GetBuyCommand()) );

	char szName[128];
	memcpy(szName, m_szName, sizeof(szName));

	WriteString( VarArgs("cfg_name \"%s\"", szName) );

	// Order is important in this function
	for (i = 0; i < 3; i++)
	{
		if (m_aWeapons[i].m_iWeapon)
		{
			WriteWeapon(i, bComments);
			WriteRunes(i, bComments);
		}
	}

	if (GetArmamentData(ARMAMENTTYPE_ARMOR))
	{
		WriteArmament(ARMAMENTTYPE_ARMOR, bComments);
		WriteRunes(ARMAMENTTYPE_ARMOR, bComments);
	}

	if (GetArmamentData(ARMAMENTTYPE_ITEM))
	{
		WriteArmament(ARMAMENTTYPE_ITEM, bComments);
		WriteRunes(ARMAMENTTYPE_ITEM, bComments);
	}

	// Now we bind our numen to attack keys
	for (i = 0; i < ATTACK_BINDS; i++)
	{
		if (m_aAttackBinds[i].m_iWeapon >= 0)
			WriteBind(i, bComments);
	}
}

void CArmament::WriteRunes(int iWeapon, bool bComments)
{
	for (int j = 0; j < MAX_RUNES; j++)
	{
		if (GetRunesInSlot(iWeapon, j) == 0)
			continue;

		int iRuneSlots = 0;
		if (GetWeaponData(iWeapon))
			iRuneSlots = GetWeaponData(iWeapon)->m_iRuneSlots[j];
		else if (GetArmamentData(iWeapon))
			iRuneSlots = GetArmamentData(iWeapon)->m_iRuneSlots[j];

		for (int k = 0; k < iRuneSlots; k++)
		{
			if (!m_aWeapons[iWeapon].m_apRuneData[j][k])
				continue;

			WriteRune(iWeapon, j, k, bComments);
		}
	}
}

#ifdef CLIENT_DLL
static void __MsgFunc_Armament( bf_read &msg )
{
	C_CFPlayer* pPlayer = C_CFPlayer::GetLocalCFPlayer();
	Assert(pPlayer);


	static unsigned char pData[sizeof(CArmament)];

	bool bNew = msg.ReadByte();
	int iBytes = msg.ReadByte();

	static long k = -1;

	Assert(bNew || k != -1);

	k = (bNew?0:k);

	Assert(iBytes + k <= sizeof(CArmament));

	for (int i = 0; i < iBytes; i++)
		pData[k++] = msg.ReadByte();

	CArmament::GetWriteArmament(pPlayer)->Unserialize(pData);

	cfgui::CRootPanel::UpdateArmament(CArmament::GetWriteArmament(pPlayer));
}
#endif

void CArmament::Init()
{
#ifdef CLIENT_DLL
	HOOK_MESSAGE( Armament );
#endif

	InitializePariahArmament();
}

void CArmament::UpdateClient(CCFPlayer* pPlayer)
{
#ifdef GAME_DLL
	unsigned char* pData;

	CSingleUserRecipientFilter filter( pPlayer );
	filter.MakeReliable();

	pData = Serialize();

	unsigned int j = 0;
	unsigned int i = SerializedLength();
	do
	{
		UserMessageBegin( filter, "Armament" );
			WRITE_BYTE( (j == 0) ? 1 : 0 );
			WRITE_BYTE( (i <= 253) ? i : 253 );
			for (int k = 0; k < 253 && i - k > 0; k++)
				WRITE_BYTE( pData[j++] );
		MessageEnd();
		if (i <= 253)
			break;
		i -= 253;
	}
	while (true);
#endif
}

long CArmament::SerializeCombo(int iWeapon, int iSlot)
{
	int iSerialized = 0;
	for ( int i = 0; i < MAX_RUNE_MODS; ++i )
	{
		RuneID iMod = m_aWeapons[iWeapon].m_aRunes[iSlot][i];
		if (iMod != CRuneData::InvalidRune())
			iSerialized |= (1<<iMod);
	}

	return iSerialized;
}

unsigned long CArmament::SerializedWeaponLength()
{
	return (unsigned long)&m_aWeapons[0].m_iSerializeEnd - (unsigned long)&m_aWeapons[0];
}

unsigned long CArmament::SerializedMainLength()
{
	return (unsigned long)&m_iSerializeEnd - (unsigned long)this;
}

unsigned long CArmament::SerializedLength()
{
	return SerializedWeaponLength() * 5 + SerializedMainLength();
}

// Not thread-safe!
unsigned char* CArmament::Serialize()
{
	static unsigned char Data[800];
	unsigned char* pData = Data;

	memcpy(pData, this, SerializedMainLength());
	pData += SerializedMainLength();

	for (int i = 0; i < 5; i++)
	{
		memcpy(pData, &m_aWeapons[i], SerializedWeaponLength());
		pData += SerializedWeaponLength();
	}

	// The trick to doing this right now is to keep pointers at the end of the structure and
	// pretend the rest is serialized info.
	return Data;
}

void CArmament::Unserialize(const unsigned char* pData)
{
	memcpy(this, pData, SerializedMainLength());
	pData += SerializedMainLength();

	for (int i = 0; i < 5; i++)
	{
		memcpy(&m_aWeapons[i], pData, SerializedWeaponLength());
		pData += SerializedWeaponLength();

		// Fill in the pointers.
		if (i < 3)
			m_aWeapons[i].m_pWeaponData = CArmament::WeaponIDToInfo(m_aWeapons[i].m_iWeapon);
		else
			m_aWeapons[i].m_pArmamentData = CArmamentData::GetData(m_aWeapons[i].m_iArmament);

		for (int j = 0; j < MAX_RUNES; j++)
			for (int k = 0; k < MAX_RUNE_MODS; k++)
				m_aWeapons[i].m_apRuneData[j][k] = CRuneData::GetData(m_aWeapons[i].m_aRunes[j][k]);
	}
}

CCFWeaponInfo* CArmament::WeaponIDToInfo(CFWeaponID iWeapon)
{
	return dynamic_cast< CCFWeaponInfo* >(GetFileWeaponInfoFromHandle(LookupWeaponInfoSlot(VarArgs("weapon_%s", WeaponIDToAlias(iWeapon)))));
}

// What is the rune in this slot?
RuneID CArmament::GetRuneID(int iWeapon, int iSlot, int iMod)
{
	if (iWeapon < 0 || iWeapon > 4)
		return CRuneData::InvalidRune();

	if (iSlot < 0 || iSlot >= MAX_RUNES)
		return CRuneData::InvalidRune();

	if (iMod < 0 || iMod >= MAX_RUNE_MODS)
		return CRuneData::InvalidRune();

	if (!m_aWeapons[iWeapon].m_apRuneData[iSlot][iMod])
		return CRuneData::InvalidRune();

	return m_aWeapons[iWeapon].m_aRunes[iSlot][iMod];
}

CRuneLevelData* CArmament::GetLevelData(int iWeapon, int iSlot, int iMod)
{
	if (iWeapon < 0 || iWeapon > 4)
		return NULL;

	if (iSlot < 0 || iSlot >= MAX_RUNES)
		return NULL;

	if (iMod < 0 || iMod >= MAX_RUNE_MODS)
		return NULL;

	return m_aWeapons[iWeapon].m_apRuneData[iSlot][iMod]->GetLevel(GetLevel(iWeapon, iSlot, iMod));
}

// What is the level of this particular rune?
unsigned int CArmament::GetLevel(int iWeapon, int iSlot, int iMod)
{
	if (iWeapon < 0 || iWeapon > 4)
		return ((unsigned int)~0);

	if (iSlot < 0 || iSlot >= MAX_RUNES)
		return ((unsigned int)~0);

	if (iMod < 0 || iMod >= MAX_RUNE_MODS)
		return ((unsigned int)~0);

	if (m_aWeapons[iWeapon].m_apRuneData[iSlot][iMod] && m_aWeapons[iWeapon].m_apRuneData[iSlot][iMod]->m_eType == RUNETYPE_BASE)
		return GetBaseLevel(iWeapon, iSlot);

	return GetRuneAmount(m_aWeapons[iWeapon].m_aRunes[iSlot][iMod], iWeapon, iSlot)-1;
}

// What is the highest level of all base runes combined?
// fire					= 0
// fire + ice			= 1
// fire + fire + ice	= 2
// etc
// If this returns 0 there are no force runes and the player should not be able to cast base spells in this rune slot.
unsigned int CArmament::GetBaseLevel(int iWeapon, int iSlot)
{
	unsigned int iLevel = 0;

	for (int i = 0; i < MAX_RUNE_MODS; i++)
	{
		CRuneData* pData = m_aWeapons[iWeapon].m_apRuneData[iSlot][i];
		if (!pData)
			continue;

		if (m_aWeapons[iWeapon].m_apRuneData[iSlot][i]->m_eType == RUNETYPE_BASE)
			iLevel++;
	}

	return iLevel;
}

// What is the total number of all unique base runes?
// fire				= 1
// fire L2			= 1
// fire + ice		= 2
// fire L2 + ice	= 2
// etc
unsigned int CArmament::GetBaseNumber(int iWeapon, int iSlot)
{
	unsigned int iTotal = 0;

	for (int i = 0; i < MAX_RUNE_MODS; i++)
	{
		CRuneData* pData = m_aWeapons[iWeapon].m_apRuneData[iSlot][i];
		if (!pData)
			continue;

		if (m_aWeapons[iWeapon].m_apRuneData[iSlot][i]->m_eType == RUNETYPE_BASE)
			iTotal++;
	}

	return iTotal;
}

// What is the index of the specified rune?
int CArmament::GetRuneIndex(RuneID eRune, int iWeapon, int iCombo, int iStart)
{
	for (int i = iStart; i < MAX_RUNE_MODS; i++)
	{
		CRuneData* pData = m_aWeapons[iWeapon].m_apRuneData[iCombo][i];
		if (!pData)
			continue;

		if (m_aWeapons[iWeapon].m_aRunes[iCombo][i] == eRune)
			return i;
	}

	return -1;
}

// Used typically to help only apply the rune's effects at the first time it is met in an iteration.
bool CArmament::IsFirstOfID(RuneID eRune, int iWeapon, int iCombo, int iRune)
{
	Assert(m_aWeapons[iWeapon].m_aRunes[iCombo][iRune] == eRune);

	return GetRuneIndex(m_aWeapons[iWeapon].m_aRunes[iCombo][iRune], iWeapon, iCombo) == iRune;
}

bool CArmament::IsFirstOfType(runetype_t eType, int iWeapon, int iCombo, int iRune)
{
	Assert(m_aWeapons[iWeapon].m_apRuneData[iCombo][iRune]->m_eType == eType);

	for (int i = 0; i < MAX_RUNE_MODS; i++)
	{
		CRuneData* pData = m_aWeapons[iWeapon].m_apRuneData[iCombo][i];
		if (!pData)
			continue;

		if (pData->m_eType == eType)
			return i == iRune;
	}

	return false;
}

// How many of the specified runes are there in this combo?
int CArmament::GetRuneAmount(RuneID eRune, int iWeapon, int iCombo)
{
	int iAmount = 0;
	for (int i = 0; i < MAX_RUNE_MODS; i++)
	{
		CRuneData* pData = m_aWeapons[iWeapon].m_apRuneData[iCombo][i];
		if (!pData)
			continue;

		if (m_aWeapons[iWeapon].m_aRunes[iCombo][i] == eRune)
			iAmount++;
	}

	return iAmount;
}

int CArmament::GetRuneTypeAmount(runetype_t eType, int iWeapon, int iCombo)
{
	int iAmount = 0;
	for (int i = 0; i < MAX_RUNE_MODS; i++)
	{
		CRuneData* pData = m_aWeapons[iWeapon].m_apRuneData[iCombo][i];
		if (!pData)
			continue;

		if (pData->m_eType == eType)
			iAmount++;
	}

	return iAmount;
}

CCFWeaponInfo* CArmament::GetWeaponData(int iWeapon)
{
	if (iWeapon < 0 || iWeapon > 2)
		return NULL;

	return m_aWeapons[iWeapon].m_pWeaponData;
}

CArmamentData* CArmament::GetArmamentData(int iWeapon)
{
	if (iWeapon < 3 || iWeapon > 4)
		return NULL;

	return m_aWeapons[iWeapon].m_pArmamentData;
}

// Get the maximum number of runes in this rune combination.
unsigned int CArmament::GetRunesMaximum(int iWeapon, int iSlot)
{
	if (GetWeaponData(iWeapon))
		return GetWeaponData(iWeapon)->m_iRuneSlots[iSlot];

	if (GetArmamentData(iWeapon))
		return GetArmamentData(iWeapon)->m_iRuneSlots[iSlot];

	return 0;
}

// Get the current number of runes in this rune combination.
unsigned int CArmament::GetRunesInSlot(int iWeapon, int iSlot)
{
	int iMods = 0;
	for (int i = 0; i < MAX_RUNE_MODS; i++)
		if (m_aWeapons[iWeapon].m_apRuneData[iSlot][i] != NULL)
			iMods++;

	return iMods;
}

// Get the number of rune combos available in this weapon.
unsigned int CArmament::GetRuneCombos(int iWeapon)
{
	if (!GetWeaponData(iWeapon) && !GetArmamentData(iWeapon))
		return 0;

	for (int i = 0; i < MAX_RUNE_MODS; i++)
	{
		if (GetWeaponData(iWeapon) && GetWeaponData(iWeapon)->m_iRuneSlots[i] == 0)
			return i;

		if (GetArmamentData(iWeapon) && GetArmamentData(iWeapon)->m_iRuneSlots[i] == 0)
			return i;
	}

	return 6;
}

int CArmament::GetRuneCombos()
{
	return m_aRuneCombos.Count();
}

CRunePosition* CArmament::GetRuneCombo(int i)
{
	Assert(m_aRuneCombos.IsValidIndex(i));
	if (!m_aRuneCombos.IsValidIndex(i))
		return NULL;
	return &m_aRuneCombos[i];
}

int CArmament::GetRuneCombo(int iWeapon, int iRune)
{
	for (int i = 0; i < m_aRuneCombos.Count(); i++)
	{
		CRunePosition* pRune = &m_aRuneCombos[i];
		if (pRune->m_iWeapon == iWeapon &&
			pRune->m_iRune == iRune)
		{
			return i;
		}
	}

	return -1;
}

bool CArmament::IsValidCombo(CRunePosition* pRune)
{
	int iBase = 0;
	int iForce = 0;

	for (int i = 0; i < MAX_RUNE_MODS; i++)
	{
		CRuneData *pData = m_aWeapons[pRune->m_iWeapon].m_apRuneData[pRune->m_iRune][i];
		if (pData)
		{
			switch (pData->m_eType)
			{
			case RUNETYPE_BASE:
				iBase++;
				break;

			case RUNETYPE_FORCE:
				iForce++;
				break;

			case RUNETYPE_SUPPORT:
				break;

			default:
				Assert(false); // A rune type that apparently doesn't exist.
			}
		}
	}

	if (iBase >= 1 && iForce >= 1)
		return true;

	return false;
}

bool CArmament::IsBindableCombo(CRunePosition* pRune)
{
	return HasType(RUNETYPE_BASE, pRune->m_iWeapon, pRune->m_iRune) && HasType(RUNETYPE_FORCE, pRune->m_iWeapon, pRune->m_iRune);
}

bool CArmament::HasBindableCombo()
{
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < MAX_RUNES; j++)
		{
			CRunePosition RunePos(i, j, 0);
			if (IsBindableCombo(&RunePos))
				return true;
		}
	}

	return false;
}

bool CArmament::HasType(runetype_t eType, int iWeapon, int iSlot)
{
	for (int i = 0; i < MAX_RUNE_MODS; i++)
	{
		CRuneData *pData = m_aWeapons[iWeapon].m_apRuneData[iSlot][i];
		if (pData && pData->m_eType == eType)
			return true;
	}

	return false;
}

bool CArmament::HasEffect(runeeffect_t eEffect, int iWeapon, int iSlot)
{
	for (int i = 0; i < MAX_RUNE_MODS; i++)
	{
		CRuneData *pData = m_aWeapons[iWeapon].m_apRuneData[iSlot][i];
		if (pData && pData->m_eEffect == eEffect)
			return true;
	}

	return false;
}

bool CArmament::HasRestore()
{
	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			for (int k = 0; k < MAX_RUNES; k++)
			{
				if (HasEffect(RE_RESTORE, j, k))
					return true;
			}
		}
	}

	return false;
}

forceeffect_t CArmament::GetDominantForce(int iWeapon, int iSlot)
{
	unsigned int iEffects = 0;

	for (int i = 0; i < MAX_RUNE_MODS; i++)
	{
		CRuneData* pData = m_aWeapons[iWeapon].m_apRuneData[iSlot][i];
		if (!pData)
			continue;

		if (pData->m_eType == RUNETYPE_FORCE)
			iEffects |= (1<<pData->m_eEffect);
	}

	return GetDominantForceFromEffects(iEffects);
}

forceeffect_t CArmament::GetDominantForce(long iRunes)
{
	unsigned int iEffects = 0;

	for (int i = 0; i < CRuneData::TotalRunes(); i++)
	{
		if (!(iRunes & (1<<i)))
			continue;

		RuneID eRune = (RuneID)i;
		CRuneData* pData = CRuneData::GetData(eRune);
		if (!pData)
			continue;

		if (pData->m_eType == RUNETYPE_FORCE)
			iEffects |= (1<<pData->m_eEffect);
	}

	return GetDominantForceFromEffects(iEffects);
}

// Kind of the reverse of CRuneData::IsForceEffectPossible()
forceeffect_t CArmament::GetDominantForceFromEffects(unsigned int iEffects)
{
	forceeffect_t DominantForce = FE_INVALID;

	if ( FBitSet(iEffects, (1<<RE_BULLET)) )
	{
		if ( FBitSet(iEffects, (1<<RE_EXPLODE)) )
		{
			if ( FBitSet(iEffects, (1<<RE_PROJECTILE)) )
				DominantForce = FE_RPG;
			else
				DominantForce = FE_EXPLBULLET;
		}
		else if ( FBitSet(iEffects, (1<<RE_ROF)) )
			DominantForce = FE_AUTOBULLET;
		else if ( FBitSet(iEffects, (1<<RE_AOE)) )
			DominantForce = FE_BLAST;
		else
			DominantForce = FE_BULLET;
	}
	else if ( FBitSet(iEffects, (1<<RE_EXPLODE)) )
	{
		if ( FBitSet(iEffects, (1<<RE_ROF)) )
			DominantForce = FE_MULTEXPLODE;
		else if ( FBitSet(iEffects, (1<<RE_PROJECTILE)) )
			DominantForce = FE_GRENADE;
		else
			DominantForce = FE_EXPLODE;
	}
	else if ( FBitSet(iEffects, (1<<RE_ROF)) )
	{
		DominantForce = FE_ROF;
	}
	else if ( FBitSet(iEffects, (1<<RE_AOE)) )
	{
		if ( FBitSet(iEffects, (1<<RE_PROJECTILE)) )
			DominantForce = FE_SOFTNADE;
		else
			DominantForce = FE_AOE;
	}
	else if ( FBitSet(iEffects, (1<<RE_PROJECTILE)) )
	{
		DominantForce = FE_PROJECTILE;
	}

	return DominantForce;
}

int CArmament::GetMaxHealth()
{
	int iMaxHealth = 1000;

	for (int i = 0; i < 5; i++)
	{
		CArmamentData* pArmamentData = GetArmamentData(i);
		if (pArmamentData)
			iMaxHealth *= Perc(pArmamentData->m_iMaxHealth) + 1;

		for (int j = 0; j < MAX_RUNES; j++)
		{
			for (int k = 0; k < MAX_RUNE_MODS; k++)
			{
				CRuneData *pRune = m_aWeapons[i].m_apRuneData[j][k];
				if (!pRune)
					continue;

				if (pRune->m_eEffect == RE_SPELL || pRune->m_eEffect == RE_RESTORE)
					iMaxHealth *= Perc(pRune->m_iMaxHealth) + 1;
			}
		}
	}

	if (iMaxHealth < 1)
		iMaxHealth = 1;

	return iMaxHealth;
}

int CArmament::GetMaxFocus()
{
	int iMaxFocus = 100;

	for (int i = 0; i < 5; i++)
	{
		CArmamentData* pArmamentData = GetArmamentData(i);
		if (pArmamentData)
			iMaxFocus *= Perc(pArmamentData->m_iMaxFocus) + 1;

		for (int j = 0; j < MAX_RUNES; j++)
		{
			for (int k = 0; k < MAX_RUNE_MODS; k++)
			{
				CRuneData *pRune = m_aWeapons[i].m_apRuneData[j][k];
				if (!pRune)
					continue;

				if (pRune->m_eEffect == RE_SPELL || pRune->m_eEffect == RE_RESTORE)
					iMaxFocus *= Perc(pRune->m_iMaxFocus) + 1;
			}
		}
	}

	if (iMaxFocus < 1)
		iMaxFocus = 1;

	return iMaxFocus;
}

int CArmament::GetMaxStamina()
{
	int iMaxStamina = 100;

	for (int i = 0; i < 5; i++)
	{
		CArmamentData* pArmamentData = GetArmamentData(i);
		if (pArmamentData)
			iMaxStamina *= Perc(pArmamentData->m_iMaxStamina) + 1;

		for (int j = 0; j < MAX_RUNES; j++)
		{
			for (int k = 0; k < MAX_RUNE_MODS; k++)
			{
				CRuneData *pRune = m_aWeapons[i].m_apRuneData[j][k];
				if (!pRune)
					continue;

				if (pRune->m_eEffect == RE_SPELL || pRune->m_eEffect == RE_RESTORE)
					iMaxStamina *= Perc(pRune->m_iMaxStamina) + 1;
			}
		}
	}

	if (iMaxStamina < 1)
		iMaxStamina = 1;

	return iMaxStamina;
}

int CArmament::GetPhysicalAttackDamage(int iWeapon, int iWeaponDamage)
{
	if (iWeapon < 0 || iWeapon > 2)
		return iWeaponDamage;

	for (int i = 0; i < MAX_RUNES; i++)
	{
		for (int j = 0; j < MAX_RUNE_MODS; j++)
		{
			CRuneData *pMod = m_aWeapons[iWeapon].m_apRuneData[i][j];
			if (pMod && pMod->m_eEffect == RE_ELEMENT)
			{
				if (IsFirstOfID(m_aWeapons[iWeapon].m_aRunes[i][j], iWeapon, i, j))
					iWeaponDamage *= Perc(GetLevelData(iWeapon, i, j)->m_iWeapon) + 1;
			}
		}
	}

	return iWeaponDamage;
}

element_t CArmament::GetPhysicalAttackElement(int iWeapon)
{
	element_t eElement = ELEMENT_TYPELESS;

	if (iWeapon < 0 || iWeapon > 2)
		return eElement;

	for (int i = 0; i < MAX_RUNES; i++)
	{
		// If this thing has no force runes in it, then it gets a small weapon enhancement instead.
		if (GetDominantForce(iWeapon, i) == FE_INVALID)
		{
			CRunePosition Pos;
			Pos.m_iWeapon = iWeapon;
			Pos.m_iRune = i;
			eElement |= GetMagicalAttackElement(&Pos);
			// We already know this pRune's elemental will be applied, no point in looking any further.
			continue;
		}

		for (int j = 0; j < MAX_RUNE_MODS; j++)
		{
			CRuneData *pMod = m_aWeapons[iWeapon].m_apRuneData[i][j];
			if (pMod && pMod->m_eEffect == RE_ELEMENT)
			{
				CRunePosition Pos;
				Pos.m_iWeapon = iWeapon;
				Pos.m_iRune = i;
				eElement |= GetMagicalAttackElement(&Pos);
				// We already know this pRune's elemental will be applied, no point in looking any further.
				break;
			}
		}
	}

	return eElement;
}

statuseffect_t CArmament::GetPhysicalAttackStatusEffect(int iWeapon)
{
	statuseffect_t eStatusEffect = STATUSEFFECT_NONE;

	if (iWeapon < 0 || iWeapon > 2)
		return eStatusEffect;

	for (int i = 0; i < MAX_RUNES; i++)
	{
		// If this thing has no force runes in it, then it gets a small weapon enhancement instead.
		if (GetDominantForce(iWeapon, i) == FE_INVALID)
		{
			CRunePosition Pos;
			Pos.m_iWeapon = iWeapon;
			Pos.m_iRune = i;
			eStatusEffect |= GetMagicalAttackStatusEffect(&Pos);
			// We already know this pRune's elemental will be applied, no point in looking any further.
			continue;
		}

		for (int j = 0; j < MAX_RUNE_MODS; j++)
		{
			CRuneData *pMod = m_aWeapons[iWeapon].m_apRuneData[i][j];
			if (pMod && pMod->m_eEffect == RE_ELEMENT)
			{
				CRunePosition Pos;
				Pos.m_iWeapon = iWeapon;
				Pos.m_iRune = i;
				eStatusEffect |= GetMagicalAttackStatusEffect(&Pos);
				// We already know this pRune's status effect will be applied, no point in looking any further.
				break;
			}
		}
	}

	return eStatusEffect;
}

float CArmament::GetPhysicalAttackStatusEffectMagnitude(int iWeapon)
{
	if (iWeapon < 0 || iWeapon > 2)
		return 0;

	for (int i = 0; i < MAX_RUNES; i++)
	{
		// If this thing has no force runes in it, then it gets a small weapon enhancement instead.
		if (GetDominantForce(iWeapon, i) == FE_INVALID)
		{
			return 0.05f;
		}

		for (int j = 0; j < MAX_RUNE_MODS; j++)
		{
			CRuneData *pMod = m_aWeapons[iWeapon].m_apRuneData[i][j];
			if (pMod && pMod->m_eEffect == RE_ELEMENT)
			{
				if (IsFirstOfID(m_aWeapons[iWeapon].m_aRunes[i][j], iWeapon, i, j))
					return Perc(GetLevelData(iWeapon, i, j)->m_iWeapon);
			}
		}
	}

	return 0;
}

float CArmament::GetMagicalAttackCost(CRunePosition* pRune)
{
	float flFocus = 0;
	int i;

	// Add up the values from each base rune.
	for (i = 0; i < MAX_RUNE_MODS; i++)
	{
		CRuneData* pData = m_aWeapons[pRune->m_iWeapon].m_apRuneData[pRune->m_iRune][i];

		if (!pData)
			continue;

		if (pData->m_eType == RUNETYPE_BASE)
		{
			if (IsFirstOfType(RUNETYPE_BASE, pRune->m_iWeapon, pRune->m_iRune, i))
				flFocus += GetLevelData(pRune->m_iWeapon, pRune->m_iRune, i)->m_flFocus;
		}
	}

	// Now make another loop for force mods. This way force mod multipliers always catch all base runes.
	for (i = 0; i < MAX_RUNE_MODS; i++)
	{
		CRuneData* pData = m_aWeapons[pRune->m_iWeapon].m_apRuneData[pRune->m_iRune][i];

		if (!pData)
			continue;

		if (pData->m_eType == RUNETYPE_FORCE)
		{
			if (IsFirstOfID(m_aWeapons[pRune->m_iWeapon].m_aRunes[pRune->m_iRune][i], pRune->m_iWeapon, pRune->m_iRune, i))
				flFocus *= GetLevelData(pRune->m_iWeapon, pRune->m_iRune, i)->m_flFocus;
		}
	}

	Assert(flFocus != 0);

	return flFocus;
}

float CArmament::GetMagicalAttackDamage(CRunePosition* pRune)
{
	float flDamage = 0;
	int i;

	// Add up the values from each base rune.
	for (i = 0; i < MAX_RUNE_MODS; i++)
	{
		CRuneData *pData = m_aWeapons[pRune->m_iWeapon].m_apRuneData[pRune->m_iRune][i];

		if (!pData)
			continue;

		if (pData->m_eType == RUNETYPE_BASE)
		{
			if (IsFirstOfType(RUNETYPE_BASE, pRune->m_iWeapon, pRune->m_iRune, i))
			{
				float flLevelDamage = GetLevelData(pRune->m_iWeapon, pRune->m_iRune, i)->m_flDamage;

				// One restore makes every other base rune heal as well.
				if (HasEffect(RE_RESTORE, pRune->m_iWeapon, pRune->m_iRune) && pData->m_eEffect != RE_RESTORE)
					flDamage -= flLevelDamage;
				else
					flDamage += flLevelDamage;
			}
		}
	}

	// Now make another loop for force mods. This way force mod multipliers always catch all base runes.
	for (i = 0; i < MAX_RUNE_MODS; i++)
	{
		CRuneData *pData = m_aWeapons[pRune->m_iWeapon].m_apRuneData[pRune->m_iRune][i];

		if (!pData)
			continue;

		if (pData->m_eType == RUNETYPE_FORCE)
		{
			if (IsFirstOfID(m_aWeapons[pRune->m_iWeapon].m_aRunes[pRune->m_iRune][i], pRune->m_iWeapon, pRune->m_iRune, i))
				flDamage *= GetLevelData(pRune->m_iWeapon, pRune->m_iRune, i)->m_flDamage;
		}
	}

	Assert(flDamage != 0);

	return flDamage;
}

element_t CArmament::GetMagicalAttackElement(CRunePosition* pRune)
{
	element_t eElement = ELEMENT_TYPELESS;

	for (int i = 0; i < MAX_RUNE_MODS; i++)
	{
		CRuneData *pData = m_aWeapons[pRune->m_iWeapon].m_apRuneData[pRune->m_iRune][i];

		if (!pData)
			continue;

		if (pData->m_eType == RUNETYPE_BASE && pData->m_eEffect == RE_SPELL)
			eElement |= pData->m_eElement;
	}

	return eElement;
}

statuseffect_t CArmament::GetMagicalAttackStatusEffect(CRunePosition* pRune)
{
	statuseffect_t eStatusEffect = STATUSEFFECT_NONE;

	for (int i = 0; i < MAX_RUNE_MODS; i++)
	{
		CRuneData *pData = m_aWeapons[pRune->m_iWeapon].m_apRuneData[pRune->m_iRune][i];

		if (!pData)
			continue;

		if (pData->m_eType == RUNETYPE_BASE && pData->m_eEffect == RE_SPELL)
			eStatusEffect |= pData->m_eStatusEffect;
	}

	return eStatusEffect;
}

float CArmament::GetMagicalAttackStatusMagnitude(CRunePosition* pRune)
{
	return GetBaseLevel(pRune->m_iWeapon, pRune->m_iRune) * cf_numen_status_effect_magnitude.GetFloat();
}

float CArmament::GetMagicalAttackCastTime(CRunePosition* pRune)
{
	float flCastTime = 0;

	int i;
	// Add up the values from each base rune. Multiply each base rune's value by its current level so that it can be averaged out below.
	for (i = 0; i < MAX_RUNE_MODS; i++)
	{
		CRuneData* pData = m_aWeapons[pRune->m_iWeapon].m_apRuneData[pRune->m_iRune][i];

		if (!pData)
			continue;

		if (pData->m_eType == RUNETYPE_BASE)
		{
			if (IsFirstOfType(RUNETYPE_BASE, pRune->m_iWeapon, pRune->m_iRune, i))
				flCastTime += GetLevelData(pRune->m_iWeapon, pRune->m_iRune, i)->m_flCastTime;
		}
	}

	// Now make another loop for force mods. This way force mod multipliers always catch all base runes.
	for (i = 0; i < MAX_RUNE_MODS; i++)
	{
		CRuneData *pData = m_aWeapons[pRune->m_iWeapon].m_apRuneData[pRune->m_iRune][i];

		if (!pData)
			continue;

		if (pData->m_eType == RUNETYPE_FORCE)
		{
			if (IsFirstOfID(m_aWeapons[pRune->m_iWeapon].m_aRunes[pRune->m_iRune][i], pRune->m_iWeapon, pRune->m_iRune, i))
				flCastTime *= GetLevelData(pRune->m_iWeapon, pRune->m_iRune, i)->m_flCastTime;
		}
	}

	return flCastTime;
}

float CArmament::GetMagicalAttackReload(CRunePosition* pRune)
{
	float flReload = 0;

	int i;
	// Add up the values from each base rune. Multiply each base rune's value by its current level so that it can be averaged out below.
	for (i = 0; i < MAX_RUNE_MODS; i++)
	{
		CRuneData* pData = m_aWeapons[pRune->m_iWeapon].m_apRuneData[pRune->m_iRune][i];

		if (!pData)
			continue;

		if (pData->m_eType == RUNETYPE_BASE)
		{
			if (IsFirstOfType(RUNETYPE_BASE, pRune->m_iWeapon, pRune->m_iRune, i))
				flReload += GetLevelData(pRune->m_iWeapon, pRune->m_iRune, i)->m_flReload;
		}
	}

	// Now make another loop for force mods. This way force mod multipliers always catch all base runes.
	for (i = 0; i < MAX_RUNE_MODS; i++)
	{
		CRuneData *pData = m_aWeapons[pRune->m_iWeapon].m_apRuneData[pRune->m_iRune][i];

		if (!pData)
			continue;

		if (pData->m_eType == RUNETYPE_FORCE)
		{
			if (IsFirstOfID(m_aWeapons[pRune->m_iWeapon].m_aRunes[pRune->m_iRune][i], pRune->m_iWeapon, pRune->m_iRune, i))
				flReload *= GetLevelData(pRune->m_iWeapon, pRune->m_iRune, i)->m_flReload;
		}
	}

	return flReload;
}

bool CArmament::GetMagicalAttackPositive(CRunePosition* pRune)
{
	if (HasEffect(RE_RESTORE, pRune->m_iWeapon, pRune->m_iRune))
		return true;

	return false;
}

void CArmament::GetElementDefense(CRunePosition* pRune, int &iElements, int &iDefense)
{
	bool bHasElement = false;
	int i;

	iDefense = 100;
	iElements = ELEMENT_TYPELESS;

	if (m_aWeapons[pRune->m_iWeapon].m_iArmament == CArmamentData::InvalidArmament())
		return;

	if (pRune->m_iWeapon < ARMAMENTTYPE_ARMOR || pRune->m_iWeapon > ARMAMENTTYPE_ITEM)
		return;

	for (i = 0; i < MAX_RUNE_MODS; i++)
	{
		CRuneData *pMod = m_aWeapons[pRune->m_iWeapon].m_apRuneData[pRune->m_iRune][i];
		if (pMod && pMod->m_eEffect == RE_ELEMENT)
		{
			iDefense = GetLevelData(pRune->m_iWeapon, pRune->m_iRune, i)->m_iArmor;
			bHasElement = true;
			break;
		}
	}

	if (!bHasElement)
		iDefense = 90;

	// So we know there is an element in here somewhere. Let's find all of the elemental types and add them up.
	for (i = 0; i < MAX_RUNE_MODS; i++)
	{
		CRuneData *pData = m_aWeapons[pRune->m_iWeapon].m_apRuneData[pRune->m_iRune][i];

		if (!pData)
			continue;

		if (pData->m_eType == RUNETYPE_BASE && pData->m_eEffect == RE_SPELL)
			iElements |= pData->m_eElement;
	}

	if (!iElements)
		iDefense = 100;
}

float CArmament::GetElementDefenseScale(int iDamageElements)
{
	int iBestDefense = 100;

	CRunePosition Pos;

	for (int i = ARMAMENTTYPE_ARMOR; i <= ARMAMENTTYPE_ITEM; i++)
	{
		for (int j = 0; j < MAX_RUNES; j++)
		{
			Pos.m_iWeapon = i;
			Pos.m_iRune = j;

			int iDefense, iElements;
			GetElementDefense(&Pos, iElements, iDefense);

			if ((iElements & iDamageElements) && (iDefense < iBestDefense))
				iBestDefense = iDefense;
		}
	}

	return Perc(iBestDefense);
}

element_t CArmament::GetElementsDefended()
{
	element_t eElements = ELEMENT_TYPELESS;

	CRunePosition Pos;

	for (int i = ARMAMENTTYPE_ARMOR; i <= ARMAMENTTYPE_ITEM; i++)
	{
		for (int j = 0; j < MAX_RUNES; j++)
		{
			Pos.m_iWeapon = i;
			Pos.m_iRune = j;

			int iDefense, iElements;
			GetElementDefense(&Pos, iElements, iDefense);

			eElements = (element_t)(iElements|(int)eElements);
		}
	}

	return eElements;
}

float CArmament::GetRadiusMultiplier(CRunePosition* pRune)
{
	float flRadius = 1;
	for (int i = 0; i < MAX_RUNE_MODS; i++)
	{
		CRuneData *pData = m_aWeapons[pRune->m_iWeapon].m_apRuneData[pRune->m_iRune][i];
		if (pData && pData->m_eEffect == RE_AOE)
			flRadius *= GetLevelData(pRune->m_iWeapon, pRune->m_iRune, i)->m_flRadius;
	}
	return flRadius;
}

int CArmament::GetAttackBind(int iWeapon, int iRune)
{
	for (int i = 0; i < ATTACK_BINDS; i++)
	{
		if (m_aAttackBinds[i].m_iWeapon == iWeapon &&
			m_aAttackBinds[i].m_iRune == iRune)
			return i;
	}

	return -1;
}

int CArmament::GetPowerjumpDistance()
{
	return m_bPowerjump?400:0;
}

bool CArmament::CanFullLatch()
{
	return m_bGrip;
}

CUtlDict< CArmamentData*, ArmamentID >* CArmamentData::s_pArmamentInfoDatabase;

CArmamentData::CArmamentData(const char* pszName)
{
	strcpy(m_szName, pszName);
}

void CArmamentData::PrecacheDatabase( IFileSystem *filesystem, const unsigned char *pICEKey )
{
	if ( !s_pArmamentInfoDatabase )
		ResetDatabase();

	s_pArmamentInfoDatabase = new CUtlDict< CArmamentData*, ArmamentID >();

	char* pszFileName = "scripts/armament";
	KeyValues *pKV = ReadEncryptedKVFile( filesystem, pszFileName, pICEKey );
	if ( pKV )
	{
		Parse( pKV );

		pKV->deleteThis();

#ifdef CLIENT_DLL
		CTextureHandler::LoadTextures();
#endif
	}
}

void CArmamentData::Parse( KeyValues *pKeyValuesData )
{
	KeyValues *pArmKey = pKeyValuesData->GetFirstSubKey();
	while (pArmKey)
	{
		CArmamentData* pData = FindData(pArmKey->GetName());

		pData->ParseGeneral( pArmKey );

		pArmKey = pArmKey->GetNextKey();
	}
}

void CArmamentData::ParseGeneral( KeyValues *pKey )
{
	Q_strncpy( m_szPrintName, pKey->GetString( "printname", ARMAMENT_PRINTNAME_MISSING ), MAX_ARMAMENT_STRING );
	m_bBuyable = ( pKey->GetInt( "buyable", 1 ) != 0 ) ? true : false;
	m_iCost = pKey->GetInt( "cost", 0 );
	m_eType = FStrEq(pKey->GetString( "type", "armor" ), "armor")?ARMAMENTTYPE_ARMOR:ARMAMENTTYPE_ITEM;

	m_iMaxHealth = pKey->GetInt( "maxhealth", 0 );
	m_iMaxFocus = pKey->GetInt( "maxfocus", 0 );
	m_iMaxStamina = pKey->GetInt( "maxstamina", 0 );
	m_iHealthRegen = pKey->GetInt( "healthregen", 0 );
	m_iFocusRegen = pKey->GetInt( "focusregen", 0 );
	m_iStaminaRegen = pKey->GetInt( "staminaregen", 0 );
	m_iAttack = pKey->GetInt( "attack", 0 );
	m_iEnergy = pKey->GetInt( "energy", 0 );
	m_iResistance = pKey->GetInt( "resistance", 0 );
	m_iDefense = pKey->GetInt( "defense", 0 );
	m_iSpeed = pKey->GetInt( "speed", 0 );
	m_iCritical = pKey->GetInt( "critical", 0 );

	m_bPowerjump = (pKey->GetInt( "powerjump", 0 ))?true:false;
	m_bGrip = (pKey->GetInt( "grip", 0 ))?true:false;

	m_eElementNull = StringToElement(pKey->GetString( "elementnull", "" ));
	m_eElementHalf = StringToElement(pKey->GetString( "elementhalf", "" ));

	for (int i = 0; i < MAX_RUNES; i++)
		m_iRuneSlots[i] = pKey->GetInt( VarArgs("rune%d_slots", i+1), 0 );
}

CArmamentData* CArmamentData::FindData( const char *pszAlias )
{
	// Complain about duplicately defined metaclass names...
	ArmamentID iArmament = s_pArmamentInfoDatabase->Find( pszAlias );
	if ( iArmament != s_pArmamentInfoDatabase->InvalidIndex() )
		return GetData(iArmament);

	CArmamentData *pData = new CArmamentData(pszAlias);

	iArmament = s_pArmamentInfoDatabase->Insert( pszAlias, pData );
	Assert( iArmament != s_pArmamentInfoDatabase->InvalidIndex() );
	return pData;
}

CArmamentData* CArmamentData::GetData( ArmamentID handle )
{
	if ( handle < 0 || handle >= s_pArmamentInfoDatabase->Count() )
		return NULL;

	if ( handle == s_pArmamentInfoDatabase->InvalidIndex() )
		return NULL;

	return (*s_pArmamentInfoDatabase)[ handle ];
}

void CArmamentData::ResetDatabase( )
{
	if ( !s_pArmamentInfoDatabase )
		return;

	s_pArmamentInfoDatabase->PurgeAndDeleteElements(); 
	delete s_pArmamentInfoDatabase;
	s_pArmamentInfoDatabase = NULL;
}

ArmamentID CArmamentData::AliasToArmamentID( const char *pszAlias )
{
	if (pszAlias)
	{
		ArmamentID iArmament = s_pArmamentInfoDatabase->Find( pszAlias );
		if ( iArmament != s_pArmamentInfoDatabase->InvalidIndex() )
			return iArmament;

		if (pszAlias[0] >= '0' && pszAlias[0] <= '9')
		{
			iArmament = (ArmamentID)atoi(pszAlias);
			if (s_pArmamentInfoDatabase->IsValidIndex(iArmament))
				return iArmament;
		}
	}

	return s_pArmamentInfoDatabase->InvalidIndex();
}

CRuneComboPreset::CRuneComboPreset()
{
	Reset();
}

void CRuneComboPreset::Reset()
{
	m_szName[0] = '\0';

	for (int i = 0; i < MAX_RUNE_MODS; i++)
	{
		m_aRunes[i] = CRuneData::InvalidRune();
	}

	m_iSlots = 0;
	m_iCredits = 0;
	m_bHasCritical = false;
	m_bHasPowerjump = false;
	m_bHasLatch = false;
}

void CRuneComboPreset::CalculateData()
{
	m_iSlots = 0;
	m_iCredits = 0;
	for (int i = 0; i < MAX_RUNE_MODS; i++)
	{
		if (m_aRunes[i] != CRuneData::InvalidRune())
		{
			CRuneData* pData = CRuneData::GetData(m_aRunes[i]);
			m_iCredits += pData->m_iCost;
			m_iSlots++;
			if (pData->m_iCritical)
				m_bHasCritical = true;
			if (pData->m_eEffect == RE_POWERJUMP)
				m_bHasPowerjump = true;
			if (pData->m_eEffect == RE_LATCH)
				m_bHasLatch = true;
		}
	}
}

long CRuneComboPreset::Serialize()
{
	long iSerialized = 0;
	for (int i = 0; i < MAX_RUNE_MODS; i++)
	{
		if (m_aRunes[i] != CRuneData::InvalidRune())
			iSerialized |= (1<<m_aRunes[i]);
	}

	return iSerialized;
}

#ifdef CLIENT_DLL
void CC_DumpRunes(const CCommand &args)
{
	CCFPlayer* pPlayer = ToCFPlayer(GetCommandPlayer());
	if ( !pPlayer )
		return;

	CArmament* pArm = CArmament::GetWriteArmament(pPlayer);

	// For each armament
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < MAX_RUNES; j++)
		{
			if (pArm->GetRunesInSlot(i, j) == 0)
				continue;

			Msg( "//" );
			int iRuneSlots = 0;
			if (pArm->GetWeaponData(i))
				iRuneSlots = pArm->GetWeaponData(i)->m_iRuneSlots[j];
			else if (pArm->GetArmamentData(i))
				iRuneSlots = pArm->GetArmamentData(i)->m_iRuneSlots[j];

			for (int k = 0; k < iRuneSlots; k++)
			{
				CRuneData* pRune = pArm->m_aWeapons[i].m_apRuneData[j][k];

				if (!pRune)
					continue;

				Msg( VarArgs(" %s", pRune->m_szName) );
			}
			
			Msg( "\n" );

			Msg("cfg_combo \"#RCfg_CHANGEME\"");
			for (int k = 0; k < MAX_RUNE_MODS; k++)
			{
				if (pArm->m_aWeapons[i].m_aRunes[j][k] == CRuneData::InvalidRune())
					continue;

				Msg(VarArgs(" %d", (short)pArm->m_aWeapons[i].m_aRunes[j][k]));
			}

			Msg("\n\n");
		}
	}
}

static ConCommand dump_runes("dump_runes", CC_DumpRunes, "", FCVAR_CLIENTCMD_CAN_EXECUTE);
#endif
