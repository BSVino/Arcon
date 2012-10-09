//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Hud element that indicates the direction of damage taken by the player
//
//=============================================================================//

#include "cbase.h"
#include "view.h"
#include "hud_damage.h"
#include "hud_indicators.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

float CDamageIndicator::m_iMaximumDamage;
float CDamageIndicator::m_flMinimumTime = 1;
float CDamageIndicator::m_flMaximumTime = 2;
float CDamageIndicator::m_flTravelTime = 0.1f;
float CDamageIndicator::m_flFadeOutPercentage = 0.7f;
float CDamageIndicator::m_flNoise = 0.1f;
CUtlVector<CDamageIndicator::damage_t> CDamageIndicator::m_aDamages;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CDamageIndicator::CDamageIndicator()
{
	if (C_BasePlayer::GetLocalPlayer())
		m_iMaximumDamage = C_BasePlayer::GetLocalPlayer()->m_iMaxHealth/3;

	m_aIndicators.AddMultipleToHead(8);
	// Arrange our points in a circle.
	m_aIndicators[0] = CIndicator(-0.707106781, -0.707106781);
	m_aIndicators[1] = CIndicator(-1,            0);
	m_aIndicators[2] = CIndicator(-0.707106781,  0.707106781);
	m_aIndicators[3] = CIndicator( 0,            1);
	m_aIndicators[4] = CIndicator( 0.707106781,  0.707106781);
	m_aIndicators[5] = CIndicator( 1,            0);
	m_aIndicators[6] = CIndicator( 0.707106781, -0.707106781);
	m_aIndicators[7] = CIndicator( 0,           -1);
}

void CDamageIndicator::LoadTextures()
{
	m_aIndicators[0].m_pTexture = GetHudTexture("dmg-nw");
	m_aIndicators[1].m_pTexture = GetHudTexture("dmg-w");
	m_aIndicators[2].m_pTexture = GetHudTexture("dmg-sw");
	m_aIndicators[3].m_pTexture = GetHudTexture("dmg-s");
	m_aIndicators[4].m_pTexture = GetHudTexture("dmg-se");
	m_aIndicators[5].m_pTexture = GetHudTexture("dmg-e");
	m_aIndicators[6].m_pTexture = GetHudTexture("dmg-ne");
	m_aIndicators[7].m_pTexture = GetHudTexture("dmg-n");
}

//-----------------------------------------------------------------------------
// Purpose: Convert a damage position in world units to the screen's units
//-----------------------------------------------------------------------------
void CDamageIndicator::GetDamagePosition( const Vector &vecDelta, float *xpos, float *ypos, float *flRotation )
{
	// Player Data
	Vector playerPosition = MainViewOrigin();
	QAngle playerAngles = MainViewAngles();

	Vector forward, right, up(0,0,1);
	AngleVectors (playerAngles, &forward, NULL, NULL );
	forward.z = 0;
	VectorNormalize(forward);
	CrossProduct( up, forward, right );
	*xpos = -DotProduct(vecDelta, forward);
	*ypos = -DotProduct(vecDelta, right);

	// Get the rotation (yaw)
	*flRotation = atan2(*ypos,*xpos) + M_PI;
	*flRotation *= 180 / M_PI;

	float yawRadians = -(*flRotation) * M_PI / 180.0f;
	float ca = -cos( yawRadians );
	float sa = sin( yawRadians );

	// Rotate it around the circle
	*xpos = sa;
	*ypos = ca;
}

//-----------------------------------------------------------------------------
// Purpose: Draw a single damage indicator
//-----------------------------------------------------------------------------
void CDamageIndicator::DrawDamageIndicator(CDamageIndicator::CIndicator* pIndicator)
{
	if (pIndicator->m_flAlpha <= 0)
		return;

	if (pIndicator->m_flAlpha > 255)
		pIndicator->m_flAlpha = 255;

	int iX = 0, iY = 0, iW = CFScreenWidth()/2, iH = CFScreenHeight()/2;

	if (pIndicator->m_vecPos.x > 0.1f)	// a little epsilon here
		iX = CFScreenWidth()/2;
	if (pIndicator->m_vecPos.y > 0.1f)
		iY = CFScreenHeight()/2;

	if (fabs(pIndicator->m_vecPos.x) < 0.1f)	// a little epsilon here
		iW = CFScreenWidth();
	if (fabs(pIndicator->m_vecPos.y) < 0.1f)
		iH = CFScreenHeight();

	pIndicator->m_pTexture->DrawSelf(iX, iY, iW, iH, Color(255, 255, 255, pIndicator->m_flAlpha));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDamageIndicator::Paint(int x, int y, int w, int h)
{
	int i, iSize;

	if (C_BasePlayer::GetLocalPlayer())
	{
		if (!C_BasePlayer::GetLocalPlayer()->IsAlive())
		{
			m_aDamages.RemoveAll();
			return;
		}

		m_iMaximumDamage = C_BasePlayer::GetLocalPlayer()->m_iMaxHealth/3;
	}

	ResetIndicators();

	// Iterate backwards, because we might remove them as we go
	iSize = m_aDamages.Count();
	for (i = iSize-1; i >= 0; i--)
	{
		if (m_aDamages[i].iPlayer != C_BasePlayer::GetLocalPlayer()->entindex())
			continue;

		// Scale size to the damage
		// Find the place to draw it
		float xpos, ypos;
		float flRotation;
		float flTimeSinceStart = ( gpGlobals->curtime - m_aDamages[i].flStartTime );
		GetDamagePosition( m_aDamages[i].vecDelta, &xpos, &ypos, &flRotation );

		CDamageIndicator::CIndicator* pIndicators[2];

		GetIndicators(xpos, ypos, pIndicators[0], pIndicators[1]);

		float flAlpha = RemapVal(clamp( m_aDamages[i].iScale, 0, m_iMaximumDamage ), 0, m_iMaximumDamage, 100, 255);

		// Life is to the left.
		float flLifeLeft = ( m_aDamages[i].flLifeTime - gpGlobals->curtime );
		if ( flLifeLeft > 0 )
		{
			float flPercent = flTimeSinceStart / (m_aDamages[i].flLifeTime - m_aDamages[i].flStartTime);
			if ( pIndicators[0] || pIndicators[1] )
			{
				for (int j = 0; j < 2; j++)
				{
					if (!pIndicators[j])
						continue;

					if ( flPercent <= m_flFadeOutPercentage )
						pIndicators[j]->m_flAlpha += flAlpha * pIndicators[j]->m_flScale;
					else
						pIndicators[j]->m_flAlpha += (flAlpha - RemapVal( flPercent, m_flFadeOutPercentage, 1.0, 0.0, 255.0 )) * pIndicators[j]->m_flScale;
				}
			}
			else
			{
				// Can't find any particular place for this damage, so stick it on everyone.
				for (int j = 0; j < m_aIndicators.Count(); j++)
				{
					if ( flPercent <= m_flFadeOutPercentage )
						m_aIndicators[j].m_flAlpha += flAlpha;
					else
						m_aIndicators[j].m_flAlpha += flAlpha - RemapVal( flPercent, m_flFadeOutPercentage, 1.0, 0.0, 255.0 );
				}
			}
		}
		else
		{
			m_aDamages.Remove(i);
		}
	}

	for (i = 0; i < m_aIndicators.Count(); i++)
	{
		CDamageIndicator::CIndicator* pIndicator = &m_aIndicators[i];
		DrawDamageIndicator(pIndicator);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Message handler for Damage message
//-----------------------------------------------------------------------------
void __MsgFunc_Damage( bf_read &msg )
{
	if (!C_BasePlayer::GetLocalPlayer())
		return;

	CDamageIndicator::damage_t damage;

	damage.iPlayer = msg.ReadByte();

	damage.iScale = msg.ReadByte();
	if ( damage.iScale > CDamageIndicator::m_iMaximumDamage )
	{
		damage.iScale = CDamageIndicator::m_iMaximumDamage;
	}

	bool bHealing = msg.ReadByte();
	damage.iScale = bHealing?-damage.iScale:damage.iScale;

	damage.iElement = (element_t)msg.ReadShort();

	Vector vecDirection;
	msg.ReadBitVec3Coord( vecDirection );

	msg.ReadBitVec3Coord( damage.vecOrigin );

	if (bHealing)
	{
		C_BasePlayer* pHealed = UTIL_PlayerByIndex(damage.iPlayer);
		CSmartPtr<CNewParticleEffect> pEffect = pHealed->ParticleProp()->Create("healed", PATTACH_ABSORIGIN_FOLLOW);
		if (pEffect.IsValid() && pEffect->IsValid())
		{
			pEffect->SetSortOrigin( pHealed->GetAbsOrigin() );
			pEffect->SetControlPoint( 0, pHealed->GetAbsOrigin() );
			pEffect->SetControllerValue( 0, damage.iScale );
		}
		return;
	}

	// Don't accept new damage messages for this player if the player is dead.
	// Still accept for other players so that we see their damage even though we are dead.
	if (!C_BasePlayer::GetLocalPlayer()->IsAlive() && (damage.iPlayer == C_BasePlayer::GetLocalPlayer()->entindex()))
		return;

	// Currently we don't have a hud graphic for healing the local player other than the health bar rising.
	if ((damage.iPlayer == C_BasePlayer::GetLocalPlayer()->entindex()) && damage.iScale < 0)
		return;

	damage.flStartTime = gpGlobals->curtime;

	if (damage.iPlayer == C_BasePlayer::GetLocalPlayer()->entindex())
		damage.flLifeTime = gpGlobals->curtime + RemapVal(damage.iScale, 0, CDamageIndicator::m_iMaximumDamage, CDamageIndicator::m_flMinimumTime, CDamageIndicator::m_flMaximumTime);
	else
		damage.flLifeTime = gpGlobals->curtime + CDamageIndicator::m_flMaximumTime;

	if ( vecDirection == vec3_origin )
	{
		vecDirection = MainViewOrigin();
	}

	damage.vecDelta = (vecDirection - MainViewOrigin());
	VectorNormalize( damage.vecDelta );

	// Add some noise
	damage.vecDelta[0] += random->RandomFloat( -CDamageIndicator::m_flNoise, CDamageIndicator::m_flNoise );
	damage.vecDelta[1] += random->RandomFloat( -CDamageIndicator::m_flNoise, CDamageIndicator::m_flNoise );
	damage.vecDelta[2] += random->RandomFloat( -CDamageIndicator::m_flNoise, CDamageIndicator::m_flNoise );
	VectorNormalize( damage.vecDelta );

	CDamageIndicator::m_aDamages.AddToTail( damage );
}

void CDamageIndicator::GetIndicators(float flX, float flY, CDamageIndicator::CIndicator*& pFirst, CDamageIndicator::CIndicator*& pSecond)
{
	Vector2D vecPos(flX, flY);

	pFirst = pSecond = NULL;
	for (int i = 0; i < m_aIndicators.Count(); i++)
	{
		CDamageIndicator::CIndicator* pIndicator = &m_aIndicators[i];
		float flDistance = (pIndicator->m_vecPos - vecPos).Length();
		if (flDistance <= 0.5f)
		{
			if (flDistance <= 0.25f)
				pIndicator->m_flScale = 1;
			else
				pIndicator->m_flScale = RemapVal(flDistance-0.25f, 0, 0.25f, 1, 0);

			if (pFirst)
			{
				pSecond = pIndicator;
				return;
			}
			else
				pFirst = pIndicator;
		}
	}
}

void CDamageIndicator::ResetIndicators()
{
	for (int i = 0; i < m_aIndicators.Count(); i++)
	{
		CDamageIndicator::CIndicator* pIndicator = &m_aIndicators[i];
		pIndicator->m_flAlpha = 0;
	}
}

CDamageIndicator::CIndicator::CIndicator()
{
	m_flAlpha = 0;
	m_vecPos = Vector2D(0, 0);
}

CDamageIndicator::CIndicator::CIndicator(float flX, float flY)
{
	m_flAlpha = 0;
	m_vecPos = Vector2D(flX, flY);
}
