#ifndef CFOBJECTIVES_H
#define CFOBJECTIVES_H

#include "cbase.h"
#include "hud_indicators.h"
#include "GameEventListener.h"

class CRoundStatusPanel : public CPanel
{
public:
						CRoundStatusPanel();
	virtual void		Delete() { delete this; };

	virtual void		Think();
	virtual void		Layout();

	virtual void		Paint(int x, int y, int w, int h);

	CLabel*				m_pStatus;
	CLabel*				m_pTimeleft;
};

class CRoundVictoryPanel : public CPanel
{
public:
						CRoundVictoryPanel();
	virtual void		Destructor();
	virtual void		Delete() { delete this; };

	virtual void		Layout();
	virtual void		Think();
	virtual void		Paint(int x, int y, int w, int h);

	static void			Open();
	static void			Close();
	static bool			IsOpen() { return s_pVictoryPanel && s_pVictoryPanel->IsVisible(); };

private:
	static CRoundVictoryPanel*	s_pVictoryPanel;

	CLabel*				m_pWhoWon;
	CLabel*				m_pHowWon;
};

class CObjectives : public ICFHUD, public CGameEventListener
{
public:
						CObjectives();
	virtual void		Delete() { delete this; };

	virtual void		LoadTextures();

	virtual void		FireGameEvent(IGameEvent* event);

	virtual void		Paint(int x, int y, int w, int h);
	virtual void		PaintControlPoint(int iPoint, int x, int y, int w, int h);
	virtual void		PaintObjective(char* pszObjective, Vector vecPosition, int iTeam = -1);

	virtual void		PostRenderVGui();

	CRoundStatusPanel*	m_pRoundStatus;

	CLabel*				m_pMFlags;
	CLabel*				m_pNFlags;

protected:
	static CCFHudTexture*	s_pNumeniFlag;
	static CCFHudTexture*	s_pMachindoFlag;
	static CCFHudTexture*	s_pFlagPole;
	static CCFHudTexture*	s_pMShard;
	static CCFHudTexture*	s_pNShard;
	static CCFHudTexture*	s_pNeutralShard;
	static CCFHudTexture*	s_pShardDropped;
	static CCFHudTexture*	s_pShardCarriedMachindo;
	static CCFHudTexture*	s_pShardCarriedNumeni;

	CCFRenderable		m_GoalArrow;
};

#endif
