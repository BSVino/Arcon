#ifndef CFCONFIG_H
#define CFCONFIG_H

class CConfigMgr : public CAutoGameSystemPerFrame
{
private:
							CConfigMgr(char const *pszName);

public:
	typedef enum {
		CFG_ALL,
		CFG_PRESETS,
		CFG_BASIC,
		CFG_MEDIUM,
		CFG_ADVANCED,
		CFG_CUSTOM,
		CFG_SAVE,
	} ConfigPanelType;

	virtual void			Update( float flFrametime );
	virtual void			LevelShutdownPreEntity();

	virtual void			LoadArmaments();
	virtual void			SetupArmament(const char* pszName);

	virtual void			ClearArmaments();

	CUtlVector<CArmament*>*	GetArmamentList(ConfigPanelType eType = CFG_ALL);
	CUtlVector<CRuneComboPreset*>*	GetRuneList(int iSlots);

	virtual void			AddCustom(CArmament* pArm);

	virtual void			AddRuneCombo(CRuneComboPreset* pCombo);

	static CConfigMgr*		Get();

protected:
	CUtlVector<CArmament*>	m_apPresets;
	CUtlVector<CArmament*>	m_apCustoms;
	CUtlVector<CArmament*>	m_apAll;
	CUtlVector<CArmament*>	m_apSubset;

	CUtlVector<CRuneComboPreset*>	m_apRunePresets;
	CUtlVector<CRuneComboPreset*>	m_apRunePresetSlots;

	FileFindHandle_t		m_hFindHandle;
	CArmament*				m_pWaitingOn;

private:
	static CConfigMgr*		s_pMgr;
};

class CConfigsMenu;

class CConfigsPopup : public CPanel, public IPopup, public IEventListener
{
public:
							CConfigsPopup(CConfigsMenu* pButton);

	virtual void			Delete() { delete this; };

	virtual void			Layout();
	virtual void			Paint(int x, int y, int w, int h);

	virtual void			GetAbsDimensions(int &x, int &y, int &w, int &h) { CPanel::GetAbsDimensions(x, y, w, h); }
	virtual void			SetVisible(bool bVisible) { CPanel::SetVisible(bVisible); }

	virtual void			Open();
	virtual void			Close();

	EVENT_CALLBACK(CConfigsPopup, OpenPanel);

protected:
	CConfigsMenu*			m_pButton;
	CUtlVector<CButton*>	m_apButtons;
};

class CConfigChoice : public CButton
{
	friend class CConfigsPanel;

public:
							CConfigChoice(CArmament* pArm);
	virtual void			Destructor();

	virtual void			Layout();
	virtual void			Paint(int x, int y, int w, int h);
	virtual void			PaintArmament(int iArmament, int x, int y, int w, int h);

	virtual void			SetArmament(CArmament* pArm);

	virtual void			SetImage(char* pszImage);

	virtual void			Load();

	virtual void			CursorIn();
	virtual void			CursorOut();

	virtual void			SetHighlightListener(IHighlightListener<CArmament*>* pListener) { m_pHighlightListener = pListener; };

protected:
	CArmament*				m_pArm;

	CCFHudTexture*			m_pImage;

	IHighlightListener<CArmament*>*	m_pHighlightListener;
};

class CConfigsPanel : public CPanel, public IEventListener
{
public:

							CConfigsPanel();
	virtual void			Destructor();

	virtual void			Delete() { delete this; };

	virtual void			Layout();
	virtual void			Paint(int x, int y, int w, int h);

	EVENT_CALLBACK(CConfigsPanel, Close);
	EVENT_CALLBACK(CConfigsPanel, Save);
	EVENT_CALLBACK(CConfigsPanel, ChooseArmament);

	static void				Open(CConfigMgr::ConfigPanelType eType);
	static void				Close();
	static bool				IsOpen() { return s_pConfigsPanel && s_pConfigsPanel->IsVisible(); };

protected:
	CConfigMgr::ConfigPanelType m_eType;

	CButton*				m_pNewSave;
	CButton*				m_pClose;
	CUtlVector<CConfigChoice*> m_apButtons;

	static CConfigsPanel*	s_pConfigsPanel;
};

#endif