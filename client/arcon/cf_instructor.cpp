#include "cbase.h"
#include "c_cf_player.h"
#include "cfui_menu.h"
#include "cf_gamerules.h"
#include "prediction.h"

static ConVar lesson_nexttime("lesson_nexttime", "20");	// Time between hints
static ConVar lesson_downtime("lesson_downtime", "70");	// Time for three hints other in the intervening time.
static ConVar lesson_learntime("lesson_learntime", "15");	// Time before a lesson can be learned again.
static ConVar lesson_debug("lesson_debug", "0");

bool WhoCaresConditions( C_CFPlayer *pPlayer, class CCFGameLesson *pLesson )
{
	return true;
}

bool ValidPlayerConditions( C_CFPlayer *pPlayer, class CCFGameLesson *pLesson )
{
	if (pPlayer->GetTeamNumber() != TEAM_SPECTATOR && pPlayer->GetTeamNumber() != TEAM_UNASSIGNED && !CCFMenu::IsOpen())
		return true;

	return false;
}

bool ValidPlayerAliveConditions( C_CFPlayer *pPlayer, class CCFGameLesson *pLesson )
{
	if (!ValidPlayerConditions(pPlayer, pLesson))
		return false;

	if (!pPlayer->IsAlive())
		return false;

	if (pPlayer->m_flLastRespawn == 0 && gpGlobals->curtime < pPlayer->m_flLastRespawn + 2)
		return false;

	return true;
}

bool ValidPlayerDeadConditions( C_CFPlayer *pPlayer, class CCFGameLesson *pLesson )
{
	if (!ValidPlayerConditions(pPlayer, pLesson))
		return false;

	if (pPlayer->IsAlive())
		return false;

	return true;
}

bool HasNoObjectiveConditions( C_CFPlayer *pPlayer, class CCFGameLesson *pLesson )
{
	if (pPlayer->HasObjective())
		return false;

	return true;
}

bool ThirdPersonConditions( C_CFPlayer *pPlayer, class CCFGameLesson *pLesson )
{
	if (!ValidPlayerConditions(pPlayer, pLesson))
		return false;

	if (!HasNoObjectiveConditions(pPlayer, pLesson))
		return false;

	return true;
}

bool PriNumenConditions( C_CFPlayer *pPlayer, class CCFGameLesson *pLesson )
{
	if (!ValidPlayerAliveConditions(pPlayer, pLesson))
		return false;

	if (!HasNoObjectiveConditions(pPlayer, pLesson))
		return false;

	if (pPlayer->GetActiveArmament()->m_aAttackBinds[0].m_iWeapon == -1 && pPlayer->GetActiveArmament()->m_aAttackBinds[1].m_iWeapon == -1)
		return false;

	// Only display if we have a primary weapon to switch back and forth between since this is for the f attackmode key.
	if (!pPlayer->GetPrimaryWeapon())
		return false;

	return true;
}

bool PrimaryMeleeConditions( C_CFPlayer *pPlayer, class CCFGameLesson *pLesson )
{
	if (!ValidPlayerAliveConditions(pPlayer, pLesson))
		return false;

	if (!HasNoObjectiveConditions(pPlayer, pLesson))
		return false;

	return pPlayer->GetPrimaryWeapon() && pPlayer->GetPrimaryWeapon()->IsMeleeWeapon();
}

bool HasSecondaryConditions( C_CFPlayer *pPlayer, class CCFGameLesson *pLesson )
{
	if (!ValidPlayerAliveConditions(pPlayer, pLesson))
		return false;

	if (!HasNoObjectiveConditions(pPlayer, pLesson))
		return false;

	return pPlayer->GetSecondaryWeapon();
}

bool HasNoSecondaryConditions( C_CFPlayer *pPlayer, class CCFGameLesson *pLesson )
{
	if (!ValidPlayerAliveConditions(pPlayer, pLesson))
		return false;

	return !pPlayer->GetSecondaryWeapon();
}

bool PrimaryFirearmConditions( C_CFPlayer *pPlayer, class CCFGameLesson *pLesson )
{
	if (!HasNoSecondaryConditions(pPlayer, pLesson))
		return false;

	if (!HasNoObjectiveConditions(pPlayer, pLesson))
		return false;

	if (!pPlayer->GetPrimaryWeapon())
		return false;

	if (pPlayer->GetPrimaryWeapon()->IsMeleeWeapon())
		return false;

	return true;
}

bool FindTargetConditions( C_CFPlayer *pPlayer, class CCFGameLesson *pLesson )
{
	if (!ValidPlayerAliveConditions(pPlayer, pLesson))
		return false;

	if (!HasNoObjectiveConditions(pPlayer, pLesson))
		return false;

	if (!pPlayer->GetPrimaryWeapon() || !pPlayer->GetPrimaryWeapon()->IsMeleeWeapon())
		return false;

	if (pPlayer->GetSecondaryWeapon() && !pPlayer->GetSecondaryWeapon()->IsMeleeWeapon())
		return false;

	if (pPlayer->GetRecursedTarget())
		return false;

	if (pPlayer->IsInFollowMode())
		return false;

	return true;
}

bool FollowModeConditions( C_CFPlayer *pPlayer, class CCFGameLesson *pLesson )
{
	if (!HasNoSecondaryConditions(pPlayer, pLesson))
		return false;

	if (!HasNoObjectiveConditions(pPlayer, pLesson))
		return false;

	if (!pPlayer->GetPrimaryWeapon() || !pPlayer->GetPrimaryWeapon()->IsMeleeWeapon())
		return false;

	if (!pPlayer->GetRecursedTarget())
		return false;

	if (CFGameRules()->PlayerRelationship(pPlayer, pPlayer->GetRecursedTarget()) == GR_TEAMMATE)
		return false;

	if (pPlayer->IsInFollowMode())
		return false;

	return true;
}

bool LowStaminaConditions( C_CFPlayer *pPlayer, class CCFGameLesson *pLesson )
{
	if (!ValidPlayerAliveConditions(pPlayer, pLesson))
		return false;

	if (!HasNoObjectiveConditions(pPlayer, pLesson))
		return false;

	return pPlayer->m_pStats->GetStamina() < pPlayer->m_pStats->m_iMaxStamina/3;
}

bool LowAmmoConditions( C_CFPlayer *pPlayer, class CCFGameLesson *pLesson )
{
	if (!ValidPlayerAliveConditions(pPlayer, pLesson))
		return false;

	if (!HasNoObjectiveConditions(pPlayer, pLesson))
		return false;

	CWeaponCFBase* pWeapon = pPlayer->GetPrimaryWeapon();
	if (!pWeapon)
		return false;

	if (pWeapon->IsMeleeWeapon())
		return false;

	return pWeapon->Clip1() < pWeapon->GetMaxClip1();
}

bool OutOfFocusConditions( C_CFPlayer *pPlayer, class CCFGameLesson *pLesson )
{
	if (!ValidPlayerAliveConditions(pPlayer, pLesson))
		return false;

	if (!pPlayer->GetActiveArmament()->GetRuneCombos())
		return false;

	return pPlayer->m_pStats->GetFocus() < 0;
}

bool PariahConditions( C_CFPlayer *pPlayer, class CCFGameLesson *pLesson )
{
	if (!ValidPlayerAliveConditions(pPlayer, pLesson))
		return false;

	if (pPlayer->IsPariah())
		return true;

	return false;
}

bool FuseConditions( C_CFPlayer *pPlayer, class CCFGameLesson *pLesson )
{
	if (!ValidPlayerAliveConditions(pPlayer, pLesson))
		return false;

	if (pPlayer->IsFuse())
		return true;

	return false;
}

bool MagicCombosConditions( C_CFPlayer *pPlayer, class CCFGameLesson *pLesson )
{
	if (!ValidPlayerAliveConditions(pPlayer, pLesson))
		return false;

	// Only show for a couple seconds.
	if (gpGlobals->curtime > pPlayer->m_flSpawnTime + 5)
		return false;

	if (!pPlayer->GetActiveArmament()->GetRuneCombos())
		return false;

	// Don't show it unless we can actually learn it.
	if (gpGlobals->curtime < pLesson->m_flLastTimeLearned + lesson_learntime.GetFloat())
		return false;

	return true;
}

bool MagicModeConditions( C_CFPlayer *pPlayer, class CCFGameLesson *pLesson )
{
	if (!ValidPlayerAliveConditions(pPlayer, pLesson))
		return false;

	if (!pPlayer->IsMagicMode())
		return false;

	// Don't show it unless we can actually learn it.
	if (gpGlobals->curtime < pLesson->m_flLastTimeLearned + lesson_learntime.GetFloat())
		return false;

	return true;
}

bool PowerJumpConditions( C_CFPlayer* pPlayer, class CCFGameLesson* pLesson )
{
	if (!ValidPlayerAliveConditions(pPlayer, pLesson))
		return false;

	if (!HasNoObjectiveConditions(pPlayer, pLesson))
		return false;

	if (!cvar->FindVar("cl_doubletapdodges")->GetBool())
		return false;

	return pPlayer->GetActiveArmament()->GetPowerjumpDistance();
}

bool CTFConditions( C_CFPlayer *pPlayer, class CCFGameLesson *pLesson )
{
	if (!ValidPlayerConditions(pPlayer, pLesson))
		return false;

	if (CFGameRules()->GetGameMode() != CF_GAME_CTF)
		return false;

	// Only show for a couple seconds.
	if (gpGlobals->curtime > pPlayer->m_flSpawnTime + 10)
		return false;

	// Don't do it if the player already has it because then he needs to pay attention to the arrow instead.
	if (pPlayer->HasObjective())
		return false;

	return true;
}

static bool Int_LessFunc( int const &a, int const &b )
{
	return a < b;
}

void C_CFPlayer::Instructor_Initialize()
{
	m_flLastLesson = 0;

	m_apLessons.Purge();
	m_apLessons.RemoveAll();

	m_apLessons.SetLessFunc(&Int_LessFunc);

	m_apLessons.Insert(HINT_YOU_ARE_PARIAH,		new CCFGameLesson(1, HINT_YOU_ARE_PARIAH,	"",				false,	CCFGameLesson::LEARN_DISPLAYING, 5, CCFGameLesson::LESSON_INFO, &PariahConditions));
	m_apLessons.Insert(HINT_YOU_ARE_FUSE,		new CCFGameLesson(1, HINT_YOU_ARE_FUSE,		"",				false,	CCFGameLesson::LEARN_DISPLAYING, 5, CCFGameLesson::LESSON_INFO, &FuseConditions));
	m_apLessons.Insert(HINT_LOW_STAMINA,		new CCFGameLesson(1, HINT_LOW_STAMINA,		"",				false,	CCFGameLesson::LEARN_DISPLAYING, 5, CCFGameLesson::LESSON_INFO, &LowStaminaConditions));
	m_apLessons.Insert(HINT_OUT_OF_FOCUS,		new CCFGameLesson(1, HINT_OUT_OF_FOCUS,		"",				false,	CCFGameLesson::LEARN_DISPLAYING, 5, CCFGameLesson::LESSON_INFO, &OutOfFocusConditions));
	m_apLessons.Insert(HINT_BUYMENU,			new CCFGameLesson(1, HINT_BUYMENU,			"buy",			true,	CCFGameLesson::LEARN_PERFORMING, 2, CCFGameLesson::LESSON_BUTTON, &ValidPlayerConditions));
	m_apLessons.Insert(HINT_R_RELOAD,			new CCFGameLesson(2, HINT_R_RELOAD,			"+reload",		true,	CCFGameLesson::LEARN_PERFORMING, 2, CCFGameLesson::LESSON_BUTTON, &LowAmmoConditions));
	m_apLessons.Insert(HINT_Q_SWITCH_WEAPONS,	new CCFGameLesson(2, HINT_Q_SWITCH_WEAPONS,	"lastinv",		true,	CCFGameLesson::LEARN_PERFORMING, 2, CCFGameLesson::LESSON_BUTTON, &HasSecondaryConditions));
	m_apLessons.Insert(HINT_RMB_FOCUS_BLAST,	new CCFGameLesson(2, HINT_RMB_FOCUS_BLAST,	"+attack2",		true,	CCFGameLesson::LEARN_PERFORMING, 2, CCFGameLesson::LESSON_BUTTON, &PrimaryFirearmConditions));
	m_apLessons.Insert(HINT_RMB_TARGET,			new CCFGameLesson(2, HINT_RMB_TARGET,		"targetnext",	false,	CCFGameLesson::LEARN_PERFORMING, 3, CCFGameLesson::LESSON_BUTTON, &FindTargetConditions));
	m_apLessons.Insert(HINT_DOUBLETAP_POWERJUMP,new CCFGameLesson(2, HINT_DOUBLETAP_POWERJUMP,"+forward",	false,	CCFGameLesson::LEARN_PERFORMING, 2, CCFGameLesson::LESSON_BUTTON, &PowerJumpConditions));
	m_apLessons.Insert(HINT_RMB_STRONG_ATTACK,	new CCFGameLesson(2, HINT_RMB_STRONG_ATTACK,"+attack2",		false,	CCFGameLesson::LEARN_PERFORMING, 3, CCFGameLesson::LESSON_BUTTON, &PrimaryMeleeConditions));
	m_apLessons.Insert(HINT_SPACE_LATCH,		new CCFGameLesson(3, HINT_SPACE_LATCH,		"+jump",		true,	CCFGameLesson::LEARN_PERFORMING, 3, CCFGameLesson::LESSON_BUTTON, &ValidPlayerAliveConditions));
	m_apLessons.Insert(HINT_RMB_FOLLOW,			new CCFGameLesson(3, HINT_RMB_FOLLOW,		"+alt1",		false,	CCFGameLesson::LEARN_PERFORMING, 3, CCFGameLesson::LESSON_BUTTON, &FollowModeConditions));
	m_apLessons.Insert(HINT_SHIFT_AIMIN,		new CCFGameLesson(3, HINT_SHIFT_AIMIN,		"+alt1",		false,	CCFGameLesson::LEARN_PERFORMING, 3, CCFGameLesson::LESSON_BUTTON, &PrimaryFirearmConditions));
	m_apLessons.Insert(HINT_SHIFT_MAGIC,		new CCFGameLesson(3, HINT_SHIFT_MAGIC,		"+alt2",		true,	CCFGameLesson::LEARN_PERFORMING, 3, CCFGameLesson::LESSON_BUTTON, &PriNumenConditions));
	m_apLessons.Insert(HINT_S_BLOCK,			new CCFGameLesson(3, HINT_S_BLOCK,			"+back",		false,	CCFGameLesson::LEARN_PERFORMING, 3, CCFGameLesson::LESSON_BUTTON, &PrimaryMeleeConditions));
	m_apLessons.Insert(HINT_HOLD_CHARGE,		new CCFGameLesson(4, HINT_HOLD_CHARGE,		"+attack",		true,	CCFGameLesson::LEARN_PERFORMING, 3, CCFGameLesson::LESSON_BUTTON, &MagicModeConditions));
	m_apLessons.Insert(HINT_X_THIRDPERSON,		new CCFGameLesson(5, HINT_X_THIRDPERSON,	"thirdperson",	true,	CCFGameLesson::LEARN_PERFORMING, 3, CCFGameLesson::LESSON_BUTTON, &ThirdPersonConditions));
	m_apLessons.Insert(HINT_NUMBERS_COMBOS,		new CCFGameLesson(5, HINT_NUMBERS_COMBOS,	"",				false,	CCFGameLesson::LEARN_PERFORMING, 3, CCFGameLesson::LESSON_ENVIRONMENT, &MagicCombosConditions));
	m_apLessons.Insert(HINT_E_FATALITY,			new CCFGameLesson(5, HINT_E_FATALITY,		"+use",			false,	CCFGameLesson::LEARN_PERFORMING, 5, CCFGameLesson::LESSON_ENVIRONMENT, &ValidPlayerAliveConditions));
	m_apLessons.Insert(HINT_E_REVIVE,			new CCFGameLesson(5, HINT_E_REVIVE,			"+use",			false,	CCFGameLesson::LEARN_PERFORMING, 3, CCFGameLesson::LESSON_ENVIRONMENT, &ValidPlayerAliveConditions));
	m_apLessons.Insert(HINT_ONE_FORCE_BASE,		new CCFGameLesson(5, HINT_ONE_FORCE_BASE,	"",				false,	CCFGameLesson::LEARN_PERFORMING, 3, CCFGameLesson::LESSON_ENVIRONMENT, &WhoCaresConditions));
	m_apLessons.Insert(HINT_COLLECT_ALL_5,		new CCFGameLesson(5, HINT_COLLECT_ALL_5,	"",				true,	CCFGameLesson::LEARN_PERFORMING, 2, CCFGameLesson::LESSON_ENVIRONMENT, &CTFConditions));

	m_apLessons[m_apLessons.Find(HINT_SPACE_LATCH)]->TapButton(true);
	m_apLessons[m_apLessons.Find(HINT_DOUBLETAP_POWERJUMP)]->TapButton(true);

	m_apLessonPriorities.RemoveAll();

	Instructor_InvalidateLessons();
}

CCFGameLesson::CCFGameLesson(int iPriority, int iHint, char* pszCommand, bool bPreferNoEnemies, learn_t iLearningMethod, int iTimestoLearn, lesson_t iLessonType, pfnConditionsMet pfnConditions)
{
	m_iPriority = iPriority;
	m_iHint = iHint;
	m_pszCommand = pszCommand;

	m_flLastLesson = 0;

	m_iLearningMethod = iLearningMethod;
	m_iTimesLearned = 0;
	m_iTimesToLearn = iTimestoLearn;
	m_flLastTimeLearned = 0;

	m_iLessonType = iLessonType;

	m_bPreferNoEnemies = bPreferNoEnemies;
	m_pfnConditions = pfnConditions;

	m_bTapButton = false;
}

void C_CFPlayer::Instructor_Respawn()
{
	if (!Instructor_Initialized())
		Instructor_Initialize();

	int i = m_apLessons.FirstInorder();

	while (m_apLessons.IsValidIndex(i))
	{
		m_apLessons[i]->m_flLastTimeLearned = 0;

		i = m_apLessons.NextInorder(i);
	}

	m_flLastLesson = 0;

	Instructor_InvalidateLessons();
}

void C_CFPlayer::Instructor_Think()
{
	if (!Instructor_Initialized())
		Instructor_Initialize();

	if (gpGlobals->curtime > m_flNextLessonSearchTime)
	{
		m_apLessonPriorities.RemoveAll();

		// Find the most important lessons to learn right now.
		int i = m_apLessons.FirstInorder();

		while (m_apLessons.IsValidIndex(i))
		{
			int iLesson = i;
			i = m_apLessons.NextInorder(iLesson);

			CCFGameLesson* pLesson = m_apLessons[iLesson];
			if (pLesson->m_iLessonType == CCFGameLesson::LESSON_ENVIRONMENT)
				continue;

			if (!Instructor_IsLessonValid(m_apLessons.Key(iLesson)))
				continue;

			m_apLessonPriorities.InsertNoSort(pLesson);
		}

		m_apLessonPriorities.RedoSort();

		// TODO: Go and find all the events that would invalidate this list and
		// have them call Instructor_InvalidateLessons() and then increase this value.
		// Example, switching secondaries, running low on stamina, becoming pariah, etc.
		m_flNextLessonSearchTime = gpGlobals->curtime + 1;
	}

	if (!m_flLastLesson || gpGlobals->curtime > m_flLastLesson + lesson_nexttime.GetFloat())
	{
		if (lesson_debug.GetBool() && m_apLessonPriorities.Count())
		{
			DevMsg("Instructor: Lesson priorities:\n");
			for (int j = 0; j < m_apLessonPriorities.Count(); j++)
				DevMsg(" %d - %s - %d\n", j+1, g_pszHintMessages[m_apLessonPriorities[j]->m_iHint], m_apLessonPriorities[j]->m_iPriority);
		}

		CCFGameLesson* pLesson = Instructor_GetBestLessonOfType((1<<CCFGameLesson::LESSON_BUTTON) | (1<<CCFGameLesson::LESSON_INFO));

		if (pLesson)
		{
			if (lesson_debug.GetBool())
				DevMsg("Instructor: New lesson: %s Priority: %d\n", g_pszHintMessages[pLesson->m_iHint], pLesson->m_iPriority);

			m_flLastLesson = gpGlobals->curtime;
			pLesson->m_flLastLesson = gpGlobals->curtime;

			CHUDHints::Get()->Lesson(pLesson);
		}
	}
}

void C_CFPlayer::Instructor_InvalidateLessons()
{
	m_flNextLessonSearchTime = 0;
}

void C_CFPlayer::Instructor_LessonLearned(int iLesson)
{
	if (!Instructor_Initialized())
		Instructor_Initialize();

	int iLessonIndex = m_apLessons.Find(iLesson);
	Assert (iLessonIndex != m_apLessons.InvalidIndex());
	if (iLessonIndex == m_apLessons.InvalidIndex())
		return;

	CCFGameLesson* pLesson = m_apLessons[iLessonIndex];

	Assert(pLesson);
	if (!pLesson)
		return;

	// Can only learn a lesson once in a while, to ensure that it is truly learned.
	// The idea is that the player spends a couple seconds toying around with the
	// new feature, but won't spend all of the lessons in that time.
	if (gpGlobals->curtime < pLesson->m_flLastTimeLearned + lesson_learntime.GetFloat())
		return;

	pLesson->m_flLastTimeLearned = gpGlobals->curtime;
	pLesson->m_iTimesLearned++;

	if (lesson_debug.GetBool())
	{
		if (pLesson->m_iTimesLearned < pLesson->m_iTimesToLearn)
			DevMsg("Instructor: Trained lesson %s - %d/%d\n", g_pszHintMessages[pLesson->m_iHint], pLesson->m_iTimesLearned, pLesson->m_iTimesToLearn);
		else if (pLesson->m_iTimesLearned == pLesson->m_iTimesToLearn)
			DevMsg("Instructor: Learned lesson %s\n", g_pszHintMessages[pLesson->m_iHint]);
	}
}

bool C_CFPlayer::Instructor_IsLessonLearned(int iLesson)
{
	if (!Instructor_Initialized())
		Instructor_Initialize();

	if (!m_apLessons.IsValidIndex(m_apLessons.Find(iLesson)))
		return true;

	CCFGameLesson* pLesson = m_apLessons[m_apLessons.Find(iLesson)];

	Assert(pLesson);
	if (!pLesson)
		return true;

	return pLesson->m_iTimesLearned >= pLesson->m_iTimesToLearn;
}

// Can this lesson be displayed right now?
bool C_CFPlayer::Instructor_IsLessonValid(int iLesson)
{
	if (!Instructor_Initialized())
		Instructor_Initialize();

	if (!m_apLessons.IsValidIndex(m_apLessons.Find(iLesson)))
		return false;

	CCFGameLesson* pLesson = m_apLessons[m_apLessons.Find(iLesson)];

	Assert(pLesson);
	if (!pLesson)
		return false;

	if (Instructor_IsLessonLearned(iLesson))
		return false;

	if (pLesson->m_flLastLesson != 0 && gpGlobals->curtime < pLesson->m_flLastLesson + lesson_downtime.GetFloat())
		return false;

	for (int i = 0; i < pLesson->m_aiPrerequisites.Count(); i++)
	{
		if (!Instructor_IsLessonLearned(pLesson->m_aiPrerequisites[i]))
			return false;
	}

	return pLesson->m_pfnConditions(this, pLesson);
}

CCFGameLesson* C_CFPlayer::Instructor_GetBestLessonOfType(int iLessonMask)
{
	for (int i = 0; i < m_apLessonPriorities.Count(); i++)
	{
		if ((1<<m_apLessonPriorities[i]->m_iLessonType) & iLessonMask)
			return m_apLessonPriorities[i];
	}

	return NULL;
}

bool LessonPriorityLess::Less( const LessonPointer& lhs, const LessonPointer& rhs, void *pCtx )
{
	C_CFPlayer* pPlayer = C_CFPlayer::GetLocalCFPlayer();

	if (pPlayer->m_flLastEnemySeen && gpGlobals->curtime < pPlayer->m_flLastEnemySeen + 3 && lhs->m_bPreferNoEnemies != rhs->m_bPreferNoEnemies)
		return !lhs->m_bPreferNoEnemies;

	// If two lessons are the same priority, use the one that was taught the most amount of time ago.
	if (lhs->m_iPriority == rhs->m_iPriority)
		return lhs->m_flLastLesson < rhs->m_flLastLesson;

	return ( lhs->m_iPriority < rhs->m_iPriority );
}


void __MsgFunc_LessonLearned( bf_read &msg )
{
	int iLesson = msg.ReadByte();

	C_CFPlayer* pPlayer = C_CFPlayer::GetLocalCFPlayer();

	pPlayer->Instructor_LessonLearned(iLesson);
}

void CC_ResetLessons()
{
	C_CFPlayer* pPlayer = C_CFPlayer::GetLocalCFPlayer();
	pPlayer->Instructor_Initialize();
}

static ConCommand lesson_reset("lesson_reset", CC_ResetLessons, "Reset the game instructor lessons.");
