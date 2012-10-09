#ifndef CFDAMAGE_H
#define CFDAMAGE_H

#include "hud_indicators.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CDamageIndicator : public ICFHUD
{
	friend void __MsgFunc_Damage( bf_read &msg );

public:
						CDamageIndicator();
	virtual void		Delete() { delete this; };

	virtual void		LoadTextures();

	virtual void		Paint(int x, int y, int w, int h);

private:
	class CIndicator
	{
	public:
						CIndicator();
						CIndicator(float flX, float flY);

		float			m_flAlpha;
		float			m_flScale;
		Vector2D		m_vecPos;
		CCFHudTexture*	m_pTexture;
	};
	CUtlVector<CIndicator>	m_aIndicators;

	// List of damages we've taken
	struct damage_t
	{
		int		iPlayer;
		int		iScale;
		element_t	iElement;
		float	flLifeTime;
		float	flStartTime;
		Vector	vecDelta;	// Damage origin relative to the player
		Vector	vecOrigin;	// Damage origin absolute world space
	};

	virtual void		GetIndicators(float flX, float flY, CIndicator*& pFirst, CIndicator*& pSecond);
	virtual void		ResetIndicators();

	// Painting
	virtual void		GetDamagePosition( const Vector &vecDelta, float *xpos, float *ypos, float *flRotation );
	virtual void		DrawDamageIndicator( CIndicator* pIndicator );

private:
	// Indication times

	static float		m_iMaximumDamage;
	static float		m_flMinimumTime;
	static float		m_flMaximumTime;
	static float		m_flTravelTime;
	static float		m_flFadeOutPercentage;
	static float		m_flNoise;

	float				m_flAlphaNE;
	float				m_flAlphaE;
	float				m_flAlphaSE;
	float				m_flAlphaS;
	float				m_flAlphaSW;
	float				m_flAlphaW;
	float				m_flAlphaNW;

	static CUtlVector<damage_t>	m_aDamages;
};

#endif
