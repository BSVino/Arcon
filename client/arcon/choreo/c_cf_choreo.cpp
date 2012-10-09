#include "cbase.h"
#include "c_cf_choreo.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#undef CCFActor

IMPLEMENT_CLIENTCLASS_DT(C_CFActor, DT_CFActor, CCFActor)
	RecvPropFloat(RECVINFO(m_flEffectMagnitude)),
END_RECV_TABLE()

class CCinParticleEffect
{
public:
	CCinParticleEffect(C_BaseEntity* pEntity, CNewParticleEffect* pEffect)
	{
		m_hEntity = pEntity;
		m_pEffect = pEffect;
		strcpy(m_pszName, pEffect->GetName());
	}

	C_BaseEntity*		GetEntity() { return m_hEntity; }
	CNewParticleEffect*	GetEffect() { return m_pEffect; }
	const char*			GetName() { return m_pszName; }

protected:
	EHANDLE				m_hEntity;
	CNewParticleEffect*	m_pEffect;
	char				m_pszName[256];
};

CUtlVector<CCinParticleEffect*> g_apParticleEffects;

void __MsgFunc_CinematicEvent( bf_read &msg )
{
	CBaseEntity* pEntity = C_BasePlayer::GetLocalPlayer();

	const int iStrLen = 1024;
	char szName[iStrLen];

	msg.ReadString(szName, iStrLen);

	char iParticleDispatchType = msg.ReadByte();

	if (iParticleDispatchType == PDT_STOPEMISSION)
	{
		int iTarget = msg.ReadShort();
		C_BaseEntity* pTarget = C_BaseEntity::Instance( iTarget );

		for (int i = g_apParticleEffects.Count()-1; i >= 0; i--)
		{
			if (strlen(szName) && !FStrEq(szName, g_apParticleEffects[i]->GetName()))
				continue;

			if (iTarget && pTarget != g_apParticleEffects[i]->GetEntity())
				continue;

			C_BaseEntity* pEntity = g_apParticleEffects[i]->GetEntity();
			if (!pEntity)
				pEntity = C_BasePlayer::GetLocalPlayer();

			pEntity->ParticleProp()->StopEmission(g_apParticleEffects[i]->GetEffect());
			delete g_apParticleEffects[i];
			g_apParticleEffects.Remove(i);
		}

		return;
	}

	CNewParticleEffect* pParticle = NULL;

	Vector vecOrigin;
	QAngle angAngles;

	switch (iParticleDispatchType)
	{
	case PATTACH_CUSTOMORIGIN:
	{
		pParticle = pEntity->ParticleProp()->Create( szName, PATTACH_CUSTOMORIGIN );

		msg.ReadBitVec3Coord( vecOrigin );
		pParticle->SetControlPoint(0, vecOrigin);

		msg.ReadBitVec3Coord( vecOrigin );
		vecOrigin.CopyToArray(angAngles.Base());
		Vector f, r, u;
		AngleVectors(angAngles, &f, &r, &u);
		pParticle->SetControlPointOrientation(0, f, r, u);
		break;
	}

	case PDT_ATTACHMENT:
	{
		pEntity = C_BaseEntity::Instance( msg.ReadShort() );

		if (!pEntity)
			pEntity = C_BasePlayer::GetLocalPlayer();

		ParticleAttachment_t iAttachmentType = (ParticleAttachment_t)msg.ReadByte();
		short iAttachment = msg.ReadShort();

		pParticle = pEntity->ParticleProp()->Create( szName, iAttachmentType, iAttachment );
		break;
	}

	case PDT_UNDEFINED:
		pParticle = pEntity->ParticleProp()->Create( szName, PATTACH_CUSTOMORIGIN );
		break;
	}

	if (!pParticle)
		return;

	int iNumCP = msg.ReadByte();
	for (int i = 0; i < iNumCP; i++)
	{
		int iCP = msg.ReadByte();
		ParticleAttachment_t iAttachmentType = (ParticleAttachment_t)msg.ReadByte();

		switch (iAttachmentType)
		{
		case PATTACH_CUSTOMORIGIN:
			pEntity->ParticleProp()->AddControlPoint(pParticle, iCP, NULL, PATTACH_CUSTOMORIGIN);

			msg.ReadBitVec3Coord( vecOrigin );
			pParticle->SetControlPoint(iCP, vecOrigin);
			break;

		case PATTACH_POINT_FOLLOW:
		{
			C_BaseEntity* pTarget = C_BaseEntity::Instance( msg.ReadShort() );
			char szAttachment[iStrLen];
			msg.ReadString(szAttachment, iStrLen);

			if (pTarget)
				pEntity->ParticleProp()->AddControlPoint(pParticle, iCP, pTarget, PATTACH_POINT_FOLLOW, szAttachment);
			break;
		}
		}
	}

	g_apParticleEffects.AddToTail(new CCinParticleEffect(pEntity, pParticle));
}
