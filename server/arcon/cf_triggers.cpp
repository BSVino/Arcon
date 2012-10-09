#include "cbase.h"
#include "triggers.h"
#include "cf_player.h"

class CCFTriggerKill : public CBaseTrigger
{
public:
	CCFTriggerKill()
	{
	}

	DECLARE_CLASS( CCFTriggerKill, CBaseTrigger );

	void Spawn( void );
	void Touch( CBaseEntity *pOther );

	DECLARE_DATADESC();

	float	m_flDamage;			// Damage per second.
	int		m_bitsDamageInflict;	// DMG_ damage type that the door or tigger does

	// Outputs
	COutputEvent m_OnHurt;
	COutputEvent m_OnHurtPlayer;
};

BEGIN_DATADESC( CCFTriggerKill )

	// Fields
	DEFINE_KEYFIELD( m_flDamage, FIELD_FLOAT, "damage" ),
	DEFINE_KEYFIELD( m_bitsDamageInflict, FIELD_INTEGER, "damagetype" ),

	// Inputs
	DEFINE_INPUT( m_flDamage, FIELD_FLOAT, "SetDamage" ),

	// Outputs
	DEFINE_OUTPUT( m_OnHurt, "OnHurt" ),
	DEFINE_OUTPUT( m_OnHurtPlayer, "OnHurtPlayer" ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( trigger_kill, CCFTriggerKill );


//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CCFTriggerKill::Spawn( void )
{
	BaseClass::Spawn();

	InitTrigger();

	SetNextThink( TICK_NEVER_THINK );
	SetThink( NULL );
}

void CCFTriggerKill::Touch( CBaseEntity *pOther )
{
	if (pOther->IsPlayer())
	{
		CTakeDamageInfo info( this, this, 10000, m_bitsDamageInflict );
		pOther->TakeDamage( info );
		ToCFPlayer(pOther)->PlayerDeathThink();
		ToCFPlayer(pOther)->FadeRagdoll();
	}
	else
	{
		UTIL_Remove(pOther);
	}
}
