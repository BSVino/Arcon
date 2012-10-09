#include "cbase.h"
#include "tempent.h"
#include "c_te_effect_dispatch.h"
#include "ClientEffectPrecacheSystem.h"
#include "runecombo.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectNumen )
CLIENTEFFECT_MATERIAL( "runes/addedeffect_sprite" )
CLIENTEFFECT_MATERIAL( "runes/areaeffect_sprite" )
CLIENTEFFECT_MATERIAL( "runes/bullet_sprite" )
CLIENTEFFECT_MATERIAL( "runes/element_sprite" )
CLIENTEFFECT_MATERIAL( "runes/fire_sprite" )
CLIENTEFFECT_MATERIAL( "runes/lightning_sprite" )
CLIENTEFFECT_REGISTER_END()

class CParticleRuneCombo : public CRuneCombo
{
public:
						CParticleRuneCombo(long iNumen, ClientEntityHandle_t hEntity, Vector vecPos, QAngle angDir);

	virtual void		Layout() {};
	virtual void		Think() {};

	virtual void		PaintRune(RuneID eRune, int iX, int iY, int iW, int iH, int iA);
	virtual void		PaintForceIcon(forceeffect_t eForce, int iX, int iY, int iW, int iH, Color c) {};
	virtual void		PaintStatusIcon(statuseffect_t eStatus, int iX, int iY, int iW, int iH, int iA) {};
	virtual void		PaintInfuseIcon(int iArmament, int iX, int iY, int iW, int iH, int iA) {};

	virtual void		GetAbsPos(int &x, int &y) { x = y = 0; };	// Uses m_vecPos instead.

	virtual int			GetWidth() { return 8; };		// How many units wide the rune will appear ingame.
	virtual int			GetHeight() { return 8; };

protected:
	ClientEntityHandle_t m_hEntity;
	Vector				m_vecPos;
	QAngle				m_angDir;
};

CParticleRuneCombo::CParticleRuneCombo(long iNumen, ClientEntityHandle_t hEntity, Vector vecPos, QAngle angDir)
	: CRuneCombo(iNumen, -1, 0, 0)
{
	m_hEntity = hEntity;
	m_vecPos = vecPos;
	m_angDir = angDir;
}

void CParticleRuneCombo::PaintRune(RuneID eRune, int iX, int iY, int iW, int iH, int iA)
{
	const char *pszName = VarArgs("numen_%s", CRuneData::GetData(eRune)->m_szName);

	C_BaseEntity *pEnt = C_BaseEntity::Instance( m_hEntity );
	if ( pEnt && !pEnt->IsDormant() )
	{
		CSmartPtr<CNewParticleEffect> pEffect = pEnt->ParticleProp()->Create( pszName, PATTACH_ABSORIGIN_FOLLOW, 0 );
		AssertMsg2( pEffect.IsValid() && pEffect->IsValid(), "%s could not create numen cast particle effect %s",
			pEnt->GetDebugName(), pszName );
		if ( pEffect.IsValid() && pEffect->IsValid() )
		{
			Vector vecForward, vecRight, vecUp;
			AngleVectors( m_angDir, &vecForward, &vecRight, &vecUp );
			pEffect->SetSortOrigin( m_vecPos );
			pEffect->SetControlPoint( 1, m_vecPos - vecRight*(GetWidth()/2 - iX)  + vecUp*(GetHeight()/2 - iY) );
			pEffect->SetControlPoint( 2, m_vecPos - vecRight*(GetWidth()/2-iX-iW) + vecUp*(GetHeight()/2-iY-iH) );
			pEffect->SetControlPointOrientation( 1, vecForward, vecRight, vecUp );
		}
	}
}

void NumenCastEffectCallback( const CEffectData &data )
{
	if ( SuppressingParticleEffects() )
		return;

	CParticleRuneCombo NumenEffect(data.m_nHitBox, data.m_hEntity, data.m_vStart, data.m_vAngles);

	NumenEffect.Paint();
}

DECLARE_CLIENT_EFFECT( "NumenCastEffect", NumenCastEffectCallback );

void NumenEffectCallback( const CEffectData &data )
{
	if ( SuppressingParticleEffects() )
		return; // this needs to be before using data.m_nHitBox, since that may be a serialized value that's past the end of the current particle system string table

	const char *pszName = GetParticleSystemNameFromIndex( data.m_nHitBox );

	if ( data.m_fFlags & PARTICLE_DISPATCH_FROM_ENTITY )
	{
		if ( data.m_hEntity.Get() )
		{
			C_BaseEntity *pEnt = C_BaseEntity::Instance( data.m_hEntity );
			if ( pEnt && !pEnt->IsDormant() )
			{
				if ( data.m_fFlags & PARTICLE_DISPATCH_RESET_PARTICLES )
				{
					pEnt->ParticleProp()->StopEmission();
				}

				CSmartPtr<CNewParticleEffect> pEffect = pEnt->ParticleProp()->Create( pszName, (ParticleAttachment_t)data.m_nDamageType, data.m_nAttachmentIndex );
				AssertMsg2( pEffect.IsValid() && pEffect->IsValid(), "%s could not create numen particle effect %s",
					C_BaseEntity::Instance( data.m_hEntity )->GetDebugName(), pszName );
				if ( pEffect.IsValid() && pEffect->IsValid() )
				{
					if ( (ParticleAttachment_t)data.m_nDamageType == PATTACH_CUSTOMORIGIN )
					{
						pEffect->SetSortOrigin( data.m_vStart );
						pEffect->SetControlPoint( 0, data.m_vStart );
						pEffect->SetControlPoint( 1, data.m_vStart );
						pEffect->SetControlPoint( 2, data.m_vOrigin );
						pEffect->SetControllerValue( 0, data.m_flScale );
						pEffect->SetControllerValue( 1, data.m_flMagnitude );
						pEffect->SetControllerValue( 2, data.m_flRadius );
						Vector vecForward, vecRight, vecUp;
						AngleVectors( data.m_vAngles, &vecForward, &vecRight, &vecUp );
						pEffect->SetControlPointOrientation( 0, vecForward, vecRight, vecUp );
						pEffect->SetControlPointOrientation( 1, vecForward, vecRight, vecUp );
						pEffect->SetControlPointOrientation( 2, vecForward, vecRight, vecUp );
					}
				}
			}
		}
	}	
	else
	{
		CSmartPtr<CNewParticleEffect> pEffect = CNewParticleEffect::Create( NULL, pszName );
		if ( pEffect->IsValid() )
		{
			pEffect->SetSortOrigin( data.m_vOrigin );
			pEffect->SetControlPoint( 0, data.m_vOrigin );
			pEffect->SetControlPoint( 1, data.m_vStart );
			pEffect->SetControllerValue( 0, data.m_flScale );
			pEffect->SetControllerValue( 1, data.m_flMagnitude );
			pEffect->SetControllerValue( 2, data.m_flRadius );
			Vector vecForward, vecRight, vecUp;
			AngleVectors( data.m_vAngles, &vecForward, &vecRight, &vecUp );
			pEffect->SetControlPointOrientation( 0, vecForward, vecRight, vecUp );
			pEffect->SetControlPointOrientation( 1, vecForward, vecRight, vecUp );
			pEffect->SetControlPointOrientation( 2, vecForward, vecRight, vecUp );
		}
	}
}

DECLARE_CLIENT_EFFECT( "NumenEffect", NumenEffectCallback );

void NumenEffectStopCallback( const CEffectData &data )
{
	if ( data.m_hEntity.Get() )
	{
		C_BaseEntity *pEnt = C_BaseEntity::Instance( data.m_hEntity );
		if ( pEnt )
		{
			pEnt->ParticleProp()->StopEmission();
		}
	}
}

DECLARE_CLIENT_EFFECT( "NumenEffectStop", NumenEffectStopCallback );

void C_CFPlayer::ShowDefense(bool bPhysical, element_t eElements, Vector vecDamageOrigin, float flScale)
{
	m_bShieldPhysical = bPhysical;
	m_flShieldTime = gpGlobals->curtime;
	m_vecShieldDmgOrigin = vecDamageOrigin;

	// 30% armor defense (the strongest armor) shows up at full visibility.
	// .01% armor defense shows up at a low but still visible level.
	m_flShieldStrength = RemapValClamped(flScale, 0.7f, 1.0f, 1.0f, 0.5f);
}

void ShowDefenseCallback( const CEffectData &data )
{
	if ( !data.m_hEntity.Get() )
		return;

	C_CFPlayer* pPlayer = ToCFPlayer(C_BaseEntity::Instance( data.m_hEntity ));

	pPlayer->ShowDefense(data.m_nMaterial, (element_t)data.m_nDamageType, data.m_vOrigin, data.m_flMagnitude);
}

DECLARE_CLIENT_EFFECT( "ShowDefense", ShowDefenseCallback );

