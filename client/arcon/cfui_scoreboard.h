#ifndef CFSCOREBOARD_H
#define CFSCOREBOARD_H

#include "cbase.h"
#include "hud_indicators.h"

#define PING_WIDTH 100

class CPlayerButton : public CButton
{
public:
								CPlayerButton();
	virtual void				Delete() { delete this; };

	virtual void				SetBGAlpha(int iAlpha) { m_iBGAlpha = iAlpha; };

	virtual void				Paint(int x, int y, int w, int h);

protected:
	int							m_iBGAlpha;
};

class CPlayerItem : public CPanel
{
public:
								CPlayerItem(const KeyValues *pKV);
	virtual void				Destructor();
	virtual void				Delete() { delete this; };

	virtual void				SetData(const KeyValues *pKV);
	virtual KeyValues*			GetData() const;

	virtual void				SetTeam(int iTeam);
	virtual int					GetTeam() { return m_iTeam; };

	virtual void				SetColor(Color c);

	virtual void				Think();
	virtual void				Paint(int x, int y, int w, int h);

	virtual void				SetClickedListener(IEventListener* pListener, IEventListener::Callback pfnCallback, KeyValues *pParms = NULL);
	// Toggle buttons only
	virtual void				SetUnclickedListener(IEventListener* pListener, IEventListener::Callback pfnCallback, KeyValues *pParms = NULL);

protected:
	KeyValues*					m_pKV;
	int							m_iTeam;

	CPlayerButton*				m_pButton;

	CLabel*						m_pPing;
};

class CPlayerList : public CPanel, public IEventListener
{
public:
								CPlayerList();
	virtual void				Delete() { delete this; };

	virtual void				Layout();

	virtual int					AddItem(const KeyValues *data);
	virtual void				ModifyItem(int iItemID, const KeyValues *data);
	virtual void				RemoveItem(int iItemID);
	virtual void				AddTargetListener(int i);

	virtual void				SetItemTeam(int iItemID, int iTeam);
	virtual void				SetSelectedItem(int iItemID);

	virtual void				SetItemColor(int iItemID, Color c);

	virtual int					GetHighestItemID();
	virtual bool				IsItemIDValid(int iItemID);
	virtual KeyValues*			GetItemData(int iItemID);

	EVENT_CALLBACK(CPlayerList, TargetPlayer);

	typedef CPlayerItem*		ScoreSortType;
	static int					SortByScore(const ScoreSortType* pItemLeft, const ScoreSortType* pItemRight);

protected:
	int							m_iSelectedItem;
	CUtlVector<CPlayerItem*>	m_apPlayerList;

	CLabel*						m_pName;
	CLabel*						m_pPing;
};

class CScoreboard : public CPanel
{
	friend CPlayerList;
public:
								CScoreboard();
	virtual void				Destructor();
	virtual void				Delete() { delete this; };

	virtual void				LevelShutdown();

	virtual void				Think();
	virtual void				Layout();
	virtual void				Paint(int x, int y, int w, int h);

	static CScoreboard*			Get();
	static void					OpenScoreboard(bool bVis);
	static void					SetLocked(bool bLock) { s_bLocked = bLock; };
	static bool					IsLocked() { return s_bLocked; };

	virtual void				UpdatePlayerInfo();
	virtual bool				GetPlayerScoreInfo(int playerIndex, KeyValues *kv);

	virtual void				UpdatePlayerAvatar( int playerIndex, KeyValues *kv );
	static bool					AvatarIndexLessFunc( const int &lhs, const int &rhs );

	virtual int					FindItemIDForPlayerIndex(int playerIndex);

	virtual int					GetHighestScore() { return m_iHighestScore; };
	virtual int					GetLowestScore() { return m_iLowestScore; };

	static bool					ClaimMouse();

protected:
	CLabel*						m_pHostname;

	vgui::ImageList				*m_pImageList;
	int							m_iImageAvatars[MAX_PLAYERS+1];
	CUtlMap<int,int>			m_mapAvatarsToImageList;

	int							m_iHighestScore;
	int							m_iLowestScore;

	CPlayerList*				m_pPlayerList;

	CInputManager				m_Input;

private:
	int							m_iPlayerIndexSymbol;

	float						m_flLastUpdate;

	static CScoreboard*			s_pScoreboard;

	static bool					s_bLocked;
};

#endif
