#ifndef C_CF_CHOREO
#define C_CF_CHOREO

#include "c_baseanimating.h"
#include "c_ai_basenpc.h"
#include "choreo/cf_choreo_shared.h"

class C_CFActorWeapon : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_CFActorWeapon, C_BaseAnimating );
	DECLARE_CLIENTCLASS();

	virtual char*		GetBoneMergeName(int iBone);

	weaponposition_t	m_ePosition;
};

#define CCFActorWeapon C_CFActorWeapon

class C_CFActor : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS( C_CFActor, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

	float				m_flEffectMagnitude;
};

#define CCFActor C_CFActor

#endif
