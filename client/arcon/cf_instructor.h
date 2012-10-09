#ifndef CF_INSTRUCTOR_H
#define CF_INSTRUCTOR_H

typedef bool (*pfnConditionsMet)( C_CFPlayer *pPlayer, class CCFGameLesson *pLesson );

bool WhoCaresConditions( C_CFPlayer *pPlayer, class CCFGameLesson *pLesson );

class CCFGameLesson
{
public:
	typedef enum
	{
		LEARN_DISPLAYING,
		LEARN_PERFORMING,
	} learn_t;

	typedef enum
	{
		LESSON_BUTTON,		// Tell the player about a button. Lower center display. Icon to display is interpreted as a button.
		LESSON_INFO,		// Tell the player some information. Lower center display. Icon is a regular texture.
		LESSON_ENVIRONMENT,	// Point to the player something in his environment. Custom painting must be done in CHUDHints::Paint()
	} lesson_t;

	CCFGameLesson(int iPriority, int iHint, char* pszCommand, bool bPreferNoEnemies, learn_t iLearningMethod, int iTimestoLearn, lesson_t iLessonType, pfnConditionsMet pfnConditions = &WhoCaresConditions);

	void				TapButton(bool bTap) { m_bTapButton = bTap; };

	int					m_iPriority;
	int					m_iHint;
	char*				m_pszCommand;
	CUtlVector<int>		m_aiPrerequisites;

	float				m_flLastLesson;

	learn_t				m_iLearningMethod;
	int					m_iTimesLearned;
	int					m_iTimesToLearn;
	float				m_flLastTimeLearned;

	lesson_t			m_iLessonType;

	bool				m_bPreferNoEnemies;
	pfnConditionsMet	m_pfnConditions;

	bool				m_bTapButton;
};

class LessonPriorityLess
{
public:
	typedef CCFGameLesson *LessonPointer;
	bool Less( const LessonPointer& lhs, const LessonPointer& rhs, void *pCtx );
};

#endif