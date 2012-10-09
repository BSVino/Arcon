#ifndef CFDEATH_H
#define CFDEATH_H

#include "cbase.h"
#include "hud_indicators.h"

struct DeathNoticeItem {
	char			szKiller[MAX_PLAYER_NAME_LENGTH];
	char			szVictim[MAX_PLAYER_NAME_LENGTH];
	CCFHudTexture*	pIconDeath;	// the index number of the associated sprite
	long			iNumen;
	int				iSuicide;
	int				iTeamKill;
	float			flDisplayTime;
};

#define MAX_DEATHNOTICES	4

class CObituaries : public ICFHUD, public IGameEventListener2
{
public:
						CObituaries();
	virtual void		Delete() { delete this; };

	virtual void		LoadTextures();

	virtual void		Paint(int x, int y, int w, int h);

	virtual void		FireGameEvent(IGameEvent* event);

private:
	DeathNoticeItem		m_aDeathNoticeList[ MAX_DEATHNOTICES + 1 ];

	CCFHudTexture*		m_pSkullIcon;
};

class CKillerInfo : public CPanel
{
public:
						CKillerInfo();
	virtual void		Delete() { delete this; };
	virtual void		Destructor();

	static void			Open();
	static void			Close();
	static bool			IsOpen() { return s_pKillerInfo && s_pKillerInfo->IsVisible(); };
	static void			SetPlayerInfo( int iPlayer, int iInfo, int iStrongElements, int iWeakElements );

	virtual void		Layout();
	virtual void		Think();
	virtual void		Paint(int x, int y, int w, int h);

	virtual bool		IsVisible();

protected:
	CLabel*				m_pKilledBy;
	CPanel*				m_pStrength;
	CLabel*				m_pStrengthLabel;
	CPanel*				m_pWeakness;
	CLabel*				m_pWeaknessLabel;

	CHandle<C_CFPlayer>	m_hKiller;

	bool				m_bWeakToMelee;
	bool				m_bWeakToFirearms;
	bool				m_bWeakToMagic;
	bool				m_bWeakToPhyDmg;
	bool				m_bWeakToMagDmg;
	bool				m_bStrongToMelee;
	bool				m_bStrongToFirearms;
	bool				m_bStrongToMagic;
	bool				m_bStrongToPhyDmg;
	bool				m_bStrongToMagDmg;

	element_t			m_eStrongElements;
	element_t			m_eWeakElements;

	static CKillerInfo*	s_pKillerInfo;
};

#endif
