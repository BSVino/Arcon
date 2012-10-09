#ifndef CFMENU_H
#define CFMENU_H

#include "cfui_gui.h"
#include "cfgui_shared.h"
#include "utlvector.h"
#include "hud_indicators.h"
#include "cfui_draggables.h"
#include "runecombo.h"
#include "cfui_config.h"
#include "cfui_renderable.h"

#pragma warning( disable : 4201 ) // warning C4201: nonstandard extension used : nameless struct/union

using namespace cfgui;

// Space between screen edge
#define MENU_SPACE		20
#define MENU_SPACE_LOWER 100
#define MENU_WIDTH		(CFScreenWidth()-MENU_SPACE*2)
#define MENU_HEIGHT		(CFScreenHeight()-MENU_SPACE-MENU_SPACE_LOWER)

// Space between buttons
#define BTN_BORDER		5
#define BTN_WIDTH		85
#define BTN_HEIGHT		28
#define TAB_WIDTH		65
#define TAB_HEIGHT		20

// Space between top row of runes and the expanded runes below.
#define RUNE_BUFFER		25

#define PANEL_LEFT		(MENU_SPACE)
#define PANEL_TOP		(MENU_SPACE+BTN_HEIGHT+BTN_BORDER)
#define PANEL_WIDTH		(MENU_WIDTH - 150)
#define PANEL_HEIGHT	(MENU_HEIGHT-BTN_HEIGHT*2-BTN_BORDER*2)

class CCFMOTD : public CPanel, public IEventListener
{
public:
							CCFMOTD();
	virtual void			Destructor();
	virtual void			Delete() { delete this; };

	virtual void			LoadMOTD();

	virtual void			SetVisible(bool bVisible);

	virtual bool			KeyPressed(vgui::KeyCode code);

	static void				Open();
	static void				Close();
	static bool				IsOpen() { return s_pMOTD && s_pMOTD->IsVisible(); };

	EVENT_CALLBACK(CCFMOTD, Finish);

private:
	static CCFMOTD*			s_pMOTD;

	CPanel*					m_pTextPanel;
	CLabel*					m_pText;
	CLabel*					m_pHostname;
	CButton*				m_pFinish;

	CInputManager			m_Input;
};

class CCFGameInfo : public CPanel, public IEventListener
{
public:
							CCFGameInfo();
	virtual void			Destructor();
	virtual void			Delete() { delete this; };

	virtual void			Paint(int x, int y, int w, int h);

	virtual void			SetVisible(bool bVisible);

	virtual bool			KeyPressed(vgui::KeyCode code);

	static void				Open();
	static void				Close();
	static bool				IsOpen() { return s_pGameInfo && s_pGameInfo->IsVisible(); };

	EVENT_CALLBACK(CCFGameInfo, OK);

	static CCFHudTexture*		s_pCTF;
	static CCFHudTexture*		s_pPariah;

private:
	static CCFGameInfo*		s_pGameInfo;

	CButton*				m_pOK;

	CInputManager			m_Input;
};

class CCFMenu : public CPanel, public IEventListener
{
public:
							CCFMenu();
	virtual void			Destructor();
	virtual void			Delete() { delete this; };

	virtual void			Layout();
	virtual void			Paint(int x, int y, int w, int h);

	virtual void			SetVisible(bool bVis);

	virtual void			SetPanel(CFPanel ePanel);
	virtual CFPanel			GetPanel() { return m_eCurrPanel; };

	virtual bool			KeyPressed(vgui::KeyCode code);
	virtual bool			KeyReleased(vgui::KeyCode code);

	typedef enum
	{
		PO_TL,
		PO_L,
		PO_BL,
		PO_TR,
		PO_R,
		PO_BR,
	} PanelOrientation;

	static void				OpenMenu(CFPanel ePanel, bool bCloseAfter);
	static bool				IsOpen();
	static void				Close();

	static void				KillOnClose();

	EVENT_CALLBACK(CCFMenu, Finish);
	EVENT_CALLBACK(CCFMenu, Panel);
	EVENT_CALLBACK(CCFMenu, SaveConfig);

	static CCFMenu*			s_pMenu;

protected:
	bool					m_bShowingScoreboard;

	CButton*				m_pFinish;

	CFPanel					m_eCurrPanel;

	class CTeamPanel*		m_pTeamPanel;
	class CCharPanel*		m_pCharPanel;
	class CWeapPanel*		m_pWeapPanel;
	class CArmrPanel*		m_pArmrPanel;
	class CRunePanel*		m_pRunePanel;
	class CBindPanel*		m_pBindPanel;

	CButton*				m_pConfigs;

	// Tabs
	CUtlVector<CButton*>	m_apTabs;

	CLabel*					m_pCredits;
	CLabel*					m_pTips;

	CInputManager			m_Input;

	bool					m_bKillOnClose;
};

class CDeleteButton : public CButton
{
public:
					CDeleteButton();
	virtual void	Delete() { delete this; };

	virtual void	Paint() { CButton::Paint(); };
	virtual void	Paint(int x, int y, int w, int h);

	static CCFHudTexture*	s_pX;
	static CCFHudTexture*	s_pXH;
	static CCFHudTexture*	s_pXD;
};

class CArrowButton : public CButton
{
public:
					CArrowButton(bool bUp);
	virtual void	Delete() { delete this; };

	virtual void	Paint(int x, int y, int w, int h);

	static CCFHudTexture*	s_pUp;
	static CCFHudTexture*	s_pUpH;
	static CCFHudTexture*	s_pUpD;
	static CCFHudTexture*	s_pDown;
	static CCFHudTexture*	s_pDownH;
	static CCFHudTexture*	s_pDownD;

protected:
	bool			m_bUp;
};

class CCheckmark : public CBaseControl
{
public:
					CCheckmark();
	virtual void	Delete() { delete this; };

	virtual void	Paint() { int x = 0, y = 0; GetAbsPos(x, y); Paint(x, y); };
	virtual void	Paint(int x, int y) { Paint(x, y, m_iW, m_iH); };
	virtual void	Paint(int x, int y, int w, int h);

	virtual void	Mark(bool bMarked) { m_bMarked = bMarked; };

	static CCFHudTexture*	s_pCheckmark;
	static CCFHudTexture*	s_pCheckmarkD;

protected:
	bool			m_bMarked;
};

class CConfigsMenu : public CButton, public IEventListener
{
public:
							CConfigsMenu();

	virtual void			Delete() { delete this; };

	virtual void			Layout();

	EVENT_CALLBACK(CConfigsMenu, OpenMenu);
	EVENT_CALLBACK(CConfigsMenu, CloseMenu);

protected:
	class CConfigsPopup*	m_pPopup;
};

// Smaller subpanel in the team-selection screen, that holds team information.
class CTeamChoice : public CButton
{
	friend class CTeamPanel;
public:
							CTeamChoice(CCFMenu::PanelOrientation eOrient, enum eteams_list eTeam);

	virtual void			Delete() { delete this; };

	virtual void			Layout();
	virtual void			Paint(int x, int y, int w, int h);

	static CCFHudTexture*	s_pNumeni;
	static CCFHudTexture*	s_pMachindo;

protected:
	CCFMenu::PanelOrientation m_eOrient;
	enum eteams_list		m_eTeam;
};

class CTeamPanel : public CPanel, public IEventListener
{
public:
							CTeamPanel();

	virtual void			Delete() { delete this; };

	virtual void			Layout();

	virtual void			UpdateTeams();

	EVENT_CALLBACK(CTeamPanel, ChooseTeam);
	EVENT_CALLBACK(CTeamPanel, Auto);
	EVENT_CALLBACK(CTeamPanel, Spec);

protected:
	CLabel*					m_pChoose;

	CButton*				m_pAuto;
	CButton*				m_pSpec;

	// Please make sure that m_apTeams[TEAM_WHATEVER-1] will always get the correct team.
	CUtlVector<CTeamChoice*> m_apTeams;
};

class CConfigDescription : public CLabel
{
public:
							CConfigDescription();
	virtual void			Delete() { delete this; };

	virtual void			SetActiveArmament(CArmament* pArm);

	virtual void			LevelShutdown();

protected:
	CArmament*				m_pArm;
};

class CColorChoice : public CButton
{
public:
							CColorChoice(CCFHudTexture* pTexture);
	virtual void			Delete() { delete this; };

	virtual void			Paint(int x, int y, int w, int h);

protected:
	CCFHudTexture*			m_pTexture;
};

class CAbilitiesPanel : public CPanel
{
public:
							CAbilitiesPanel();

	virtual void			Delete() { delete this; };

	virtual void			Layout();
	virtual void			Paint(int x, int y, int w, int h);

	virtual void			SetArmament(CArmament* pArm);

	virtual void			LevelShutdown();

protected:
	CArmament*				m_pArm;
	CLabel*					m_pText;
	int						m_iLineHeight;
};

class CCharPanel : public CPanel, public IEventListener, public IHighlightListener<CArmament*>
{
public:
							CCharPanel();

	virtual void			Delete() { delete this; };

	virtual void			Layout();
	virtual void			Paint(int x, int y, int w, int h);

	virtual void			Highlighted(CArmament* pArm);

	virtual void			SetPreviewArmament(CArmament* pArmament);

	virtual void			LevelShutdown();

	virtual void			PostRenderVGui();
	virtual void			RenderPlayerModel();

	static CCFHudTexture*	s_pNYellow;
	static CCFHudTexture*	s_pNBeige;
	static CCFHudTexture*	s_pNRed;
	static CCFHudTexture*	s_pNOrange;

	static CCFHudTexture*	s_pMBlue;
	static CCFHudTexture*	s_pMTeal;
	static CCFHudTexture*	s_pMGreen;
	static CCFHudTexture*	s_pMPurple;

protected:
	CPanel*					m_pConfigs;
	CPanel*					m_pInfo;
	CPanel*					m_pDescPanel;
	CAbilitiesPanel*		m_pAbilitiesPanel;
	CPanel*					m_pStatsPanel;

	CConfigDescription*		m_pDesc;

	CLabel*					m_pChoose;

	CPanel*					m_pColors;

	CUtlVector<CConfigChoice*>	m_apButtons;
	CUtlVector<CButton*>		m_apEditButtons;
	CUtlVector<CColorChoice*>	m_apColorButtons;

	EVENT_CALLBACK(CCharPanel, ChooseArmament);
	EVENT_CALLBACK(CCharPanel, EditArmament);
	EVENT_CALLBACK(CCharPanel, ChooseColor);
	EVENT_CALLBACK(CCharPanel, ChooseCategory);

	CUtlVector<CCFHudTexture*>	m_apNumeniColors;
	CUtlVector<CCFHudTexture*>	m_apMachindoColors;

	CUtlVector<CButton*>	m_apDifficultyButtons;

	CArmament*				m_pPreviewArmament;
	CCFRenderablePlayer		m_pPreviewPlayer;

	CUtlVector<CNewParticleEffect*>	m_apLHComboEffects;
	CUtlVector<CNewParticleEffect*>	m_apRHComboEffects;
	element_t				m_eLHEffectElements;
	element_t				m_eRHEffectElements;

	int						m_iColor;
	int						m_iCategory;
};

class CWeaponIcon : public CDraggableIcon<CFWeaponID>
{
public:
							CWeaponIcon(IDroppable* pParent, CFWeaponID eWeapon, DragClass_t eDragClass);
	virtual void			Delete() { delete this; };

	virtual void			Paint() { CDraggableIcon<CFWeaponID>::Paint(); };
	virtual void			Paint(int x, int y, int w, int h);
};

class CWeaponDrop : public CDroppableIcon<CWeaponIcon>, public IEventListener
{
public:
							CWeaponDrop(int iSlot, const CRect &HoldingRect);
	virtual void			Delete() { delete this; };

	virtual void			Layout();
	virtual void			Paint(int x, int y, int w, int h);

	virtual void			DraggableChanged(CWeaponIcon*);

	virtual bool			CanDropHere(IDraggable*);

	EVENT_CALLBACK(CWeaponDrop, Remove);

protected:
	int						m_iSlot;

	CDeleteButton*			m_pDelete;
};

class CWeapDropPanel : public CPanel
{
public:
					CWeapDropPanel(int iSlot);
	virtual void	Destructor();
	virtual void	Delete() { delete this; };

	virtual void	Layout();
	virtual void	Paint(int x, int y, int w, int h);

	virtual bool	MousePressed(vgui::MouseCode code);

	CLabel*			m_pLabel;
	CDroppableIcon<CWeaponIcon>*	m_pDrop;
};

class CWeaponDescription : public CLabel, public IHighlightListener<CFWeaponID>
{
public:
							CWeaponDescription();
	virtual void			Delete() { delete this; };

	virtual void			Paint(int x, int y, int w, int h);

	virtual void			Highlighted(CFWeaponID eWeapon);

	virtual void			SetActiveWeapon(CFWeaponID eWeapon);
	virtual void			SetActiveCategory(int iCat);

	static CCFHudTexture*	s_pRadialGraph;

protected:
	
	CFWeaponID				m_ePaintedWeapon;
};

class CWeaponGrid : public CDroppablePanel, public IEventListener
{
public:
							CWeaponGrid();
	virtual void			Delete() { delete this; };

	virtual void			Layout();
	virtual void			Paint(int x, int y, int w, int h);

	virtual const CRect		GetHoldingRect();	// Screen space

	virtual IDraggable*		GetCurrentDraggable() { return NULL; };	// Can't drag stuff out
	virtual bool			IsInfinite() { return false; };
	virtual bool			CanDropHere(IDraggable* pDraggable);

	virtual void			AddDraggable(IDraggable*);

	EVENT_CALLBACK(CWeaponGrid, Remove);

	static CCFHudTexture*	s_pGridBox;

protected:
	CUtlVector<CDeleteButton*>	m_apDeleteButtons;
};

class CWeaponChoice : public CDroppableChoice<CFWeaponID, CWeaponIcon>
{
public:
							CWeaponChoice(CFWeaponID eWeapon);
	virtual void			Delete() { delete this; };

	virtual void			Layout();
	virtual void			Paint(int x, int y, int w, int h);

	CLabel*					m_pNoCash;
};

class CWeapPanel : public CPanel, public IEventListener
{
public:
							CWeapPanel();

	virtual void			Delete() { delete this; };

	virtual void			Layout();
	virtual void			LayoutChoice(CWeaponChoice* pChoice, int i);

	virtual void			Paint(int x, int y, int w, int h);

	void					SetPanel(int i);

	EVENT_CALLBACK(CWeapPanel, Panel);

protected:
	CLabel*					m_pChoose;

	CPanel*					m_pWeapons;
	CPanel*					m_pInfo;

	CWeaponDescription*		m_pDesc;

	CUtlVector<CButton*>	m_apCats;
	int						m_iCurrentCat;

	static CFWeaponID		s_aeWeaponSlots[3][6];

	CUtlVector<CWeaponChoice*>	m_apRangedWeapons;
	CUtlVector<CWeaponChoice*>	m_apMeleeWeapons;
	CUtlVector<CWeaponChoice*>	m_apMagicWeapons;

	CWeaponGrid*			m_pGrid;
};

class CArmorDescription : public CLabel, public IHighlightListener<ArmamentID>
{
public:
							CArmorDescription(bool bChanges = false);
	virtual void			Delete() { delete this; };

	virtual void			Paint(int x, int y, int w, int h);

	virtual void			Highlighted(ArmamentID eArmor);

	virtual void			SetActiveArmor(ArmamentID eArmor);

	virtual void			CreateDeltaString(char* pszString, int iLength, int iCurrentValue, int iNewValue);

protected:
	ArmamentID				m_ePaintedArmament;
	bool					m_bChanges;
};

class CArmamentChoice : public CButton
{
public:
							CArmamentChoice(ArmamentID eArm);

	virtual void			Delete() { delete this; };

	virtual void			Paint(int x, int y, int w, int h);

	virtual void			SetHighlightListener(IHighlightListener<ArmamentID>* pListener) { m_pHighlightListener = pListener; };

	virtual void			CursorIn();
	virtual void			CursorOut();

	virtual ArmamentID		GetArmamentID() { return m_eArm; };

protected:
	ArmamentID				m_eArm;

	IHighlightListener<ArmamentID>*	m_pHighlightListener;
};

class CArmrPanel : public CPanel, public IEventListener
{
public:
							CArmrPanel();

	virtual void			Delete() { delete this; };

	virtual void			Layout();
	virtual void			LayoutChoice(CArmamentChoice* pChoice, int i);

	EVENT_CALLBACK(CArmrPanel, SelectArmor);
	EVENT_CALLBACK(CArmrPanel, DeselectArmor);

protected:
	CLabel*					m_pArmorLabel;
	CLabel*					m_pAccessoriesLabel;
	CLabel*					m_pChoose;
	CLabel*					m_pYourArmor;
	CLabel*					m_pSelectedArmor;

	CPanel*					m_pArmor;
	CPanel*					m_pAccessories;

	CPanel*					m_pCurInfo;
	CPanel*					m_pSelInfo;

	CArmorDescription*		m_pCurDesc;
	CArmorDescription*		m_pSelDesc;

	CUtlVector<CArmamentChoice*>	m_apArmor;
};

class CSelectableRuneCombo;
class CRuneDrop;

class IComboMaster
{
public:
	virtual void		RequestFocus(CSelectableRuneCombo* pRequesting)=0;
	virtual CRuneDrop*	GetRuneDrop(int iMod)=0;
};

class CSelectableRuneCombo : public CGUIRuneCombo, public IDroppable, public IEventListener
{
public:
						CSelectableRuneCombo(int iWeapon, int iRune);
	virtual void		Destructor();
	virtual void		Delete() { delete this; };

	virtual void		Think();
	virtual void		Paint(int x, int y);
	virtual void		Paint(int x, int y, int w, int h);

	virtual void		SetSize(int w, int h) { CGUIRuneCombo::SetSize(w, h); };
	virtual void		SetDimensions(int x, int y, int w, int h) { CGUIRuneCombo::SetDimensions(x, y, w, h); };	// Local space

	virtual void		SetMaster(IComboMaster* pMaster) { m_pMaster = pMaster; };
	IComboMaster*		GetMaster() { return m_pMaster; };

	virtual IControl*	GetParent() { return CGUIRuneCombo::GetParent(); };
	virtual void		SetParent(IControl* pParent) { CGUIRuneCombo::SetParent(pParent); };
	virtual bool		IsVisible() { return CGUIRuneCombo::IsVisible(); };

	virtual CRect		GetAbsCenter();

	virtual bool		IsActive() { return m_bActive; };
	virtual void		SetActive(bool bActive) { m_bActive = bActive; };
	virtual void		SetHovering(bool bHovering) { m_bHovering = bHovering; };
	virtual float		GetCurrSize() { return m_iW*m_flCurrSize; };

	virtual bool		IsCursorListener() { return true; };
	virtual void		CursorIn();
	virtual void		CursorOut();
	virtual bool		MousePressed(vgui::MouseCode code);
	virtual bool		MouseReleased(vgui::MouseCode code);
	virtual void		CursorMoved(int mx, int my);

	// Get the place where a droppable object should be.
	virtual const CRect	GetHoldingRect();

	virtual void		AddDraggable(IDraggable* pDragged);
	virtual void		SetDraggable(IDraggable*, bool bDelete = true);
	virtual void		ClearDraggables(bool bDelete = true) {};
	virtual IDraggable*	GetDraggable(int i = 0) { return NULL; };
	virtual IDraggable*	GetCurrentDraggable() { return NULL; };

	virtual void		SetGrabbale(bool bGrabbable) {};
	virtual bool		IsGrabbale() { return true; };

	virtual bool		CanDropHere(IDraggable*);

	// Is this droppable a bottomless pit of draggables?
	virtual bool		IsInfinite() { return false; };

	virtual int			GetWeapon() { return m_iWeapon; };
	virtual int			GetRune() { return m_iRune; };

	EVENT_CALLBACK(CSelectableRuneCombo, Remove);

protected:
	IControl*			m_pHasCursor;

	CDeleteButton*		m_pDelete;

	CCFHudTexture*		m_pRuneSlotTexture;

	IComboMaster*		m_pMaster;

	int					m_iWeapon;
	int					m_iRune;

	bool				m_bActive;
	bool				m_bHovering;
	float				m_flGoalSize;	// A multiple
	float				m_flCurrSize;	// A multiple
	float				m_flDeleteGoalSize;	// A multiple
	float				m_flDeleteCurrSize;	// A multiple

	CRect				m_Dim;
};

class CRuneLayoutPanel;

// A class that lays rune combos out so that the player see what kinds of runes they have made.
class CRuneCombosPanel : public CPanel, public IComboMaster
{
public:
							CRuneCombosPanel(CRuneLayoutPanel* pRunePanel, int iWeapon);
	virtual void			Destructor();
	virtual void			Delete() { delete this; };

	virtual void			Layout();
	virtual void			Paint(int x, int y, int w, int h);

	//IComboMaster
	virtual void			RequestFocus(CSelectableRuneCombo* pRequesting);
	virtual CRuneDrop*		GetRuneDrop(int iMod);

protected:
	int						m_iWeapon;

	// This is a floating point so that it retains sub-integer values better.
	// Otherwise there is extra space at the end of the slots.
	float					m_flSlotWidth;

	CLabel*					m_pYourCombos;

	CUtlVector<CSelectableRuneCombo*>	m_apRuneComboPanels;
	CUtlVector<CLabel*>		m_apXLabels;
	CRuneLayoutPanel*		m_pRunePanel;

	int						m_iActiveCombo;
};

class CRuneIcon : public CDraggableIcon<RuneID>
{
public:
							CRuneIcon(IDroppable* pParent, RuneID eDraggable, DragClass_t eDragClass);

	virtual void			Delete() { delete this; };

	virtual void			Paint(int x, int y, int w, int h);
	virtual int				GetPaintAlpha();

	virtual IDraggable&		MakeCopy();

	virtual void			DrawSmaller(bool bDrawSmaller) { m_bDrawSmaller = bDrawSmaller; };
	virtual void			DrawSolid(bool bDrawSolid) { m_bDrawSolid = bDrawSolid; };

protected:
	bool					m_bDrawSmaller;
	bool					m_bDrawSolid;
};

class CRuneDrop : public CDroppableIcon<CRuneIcon>, public IEventListener
{
public:
							CRuneDrop(int iWeapon, int iRune, int iMod);

	virtual void			Delete() { delete this; };

	virtual void			SetDraggable(IDraggable* pDragged, bool bDelete = true);

	virtual void			DraggableChanged(CRuneIcon*);

	virtual bool			CanDropHere(IDraggable*);

	virtual void			SetRune(int iRune);

	EVENT_CALLBACK(CRuneDrop, Remove);

protected:
	int						m_iWeapon;
	int						m_iRune;
	int						m_iMod;
};

class CRuneDropPanel : public CPanel
{
public:
					CRuneDropPanel(int iWeapon, int iRune, int iMod);
	virtual void	Destructor() {};
	virtual void	Delete() { delete this; };

	virtual void	Layout();
	virtual void	Paint() { CPanel::Paint(); };
	virtual void	Paint(int x, int y) { CPanel::Paint(x, y); };
	virtual void	Paint(int x, int y, int w, int h);

	virtual bool	MousePressed(vgui::MouseCode code);

	virtual void	SetRune(int iRune);
	virtual CRuneDrop*	GetDrop() { return m_pDrop; };

protected:
	int				m_iWeapon;
	int				m_iRune;
	int				m_iMod;

	CCFHudTexture*	m_pRuneSlotTexture;

	CRuneDrop*		m_pDrop;

	CLabel*			m_pDropWhat;

	CDeleteButton*	m_pDelete;
};

class CRuneAdvancedPanel;

// A class that lays rune slots out so that the player can drag runes into it to buy them.
class CRuneLayoutPanel : public CPanel, public IEventListener
{
public:
							CRuneLayoutPanel(CRuneAdvancedPanel* pRunePanel, int iWeapon);
	virtual void			Delete() { delete this; };

	virtual void			Layout();
	virtual void			Paint(int x, int y, int w, int h);

	virtual void			SetRune(int iSlot, bool bLayout = true, CRect& Source = CRect(0,0,0,0));
	virtual CRuneDropPanel*	GetRuneDropPanel(int iMod) { return m_apRuneDropPanels[iMod]; };

	virtual void			SetInfo();

	EVENT_CALLBACK(CRuneLayoutPanel, Bind);
	EVENT_CALLBACK(CRuneLayoutPanel, Unbind);

	static CCFHudTexture*	s_pMouseButtons;
	static CCFHudTexture*	s_pMouseButtonsLMB;
	static CCFHudTexture*	s_pMouseButtonsRMB;

protected:
	int						m_iWeapon;
	int						m_iRune;

	// This is a floating point so that it retains sub-integer values better.
	// Otherwise there is extra space at the end of the slots.
	float					m_flSlotWidth;

	CRuneAdvancedPanel*		m_pRunePanel;

	// A RuneDropPanel holds a RuneDrop so that it can be moved around like an IControl.
	CUtlVector<CRuneDropPanel*>	m_apRuneDropPanels;

	CLabel*					m_pAttackButton;
	CUtlVector<CButton*>	m_apAttackBinds;

	CLabel*					m_pBasePresent;
	CLabel*					m_pForcePresent;
	CCheckmark*				m_pBaseCheckmark;
	CCheckmark*				m_pForceCheckmark;

	CGUIRuneCombo*			m_pRuneCombo;

	CLabel*					m_pInfo;

	int						m_iAttackBindsX;
	int						m_iAttackBindsY;

	// Animation
	static const float		s_flShiftTime;	// How long does it take for the rune combo to shift over to the layout pane?
	static const float		s_flSwirlTime;	// How long does it take for the rune mod slots to swirl out?

	CRect					m_ComboSource;
	float					m_flComboSourceTime;
};

class CRuneChoice : public CDroppableChoice<RuneID, CRuneIcon>
{
public:
							CRuneChoice(RuneID eDraggable);
	virtual void			Delete() { delete this; };

	virtual void			Layout();

	virtual void			Paint(int x, int y, int w, int h);

protected:
	CLabel*					m_pInfo;
	CLabel*					m_pPrice;
};

class COneForceBaseHintPopup : public CPanel, public IPopup, public IEventListener
{
public:
							COneForceBaseHintPopup();

	virtual void			Delete() { delete this; };

	virtual void			Layout();
	virtual void			Paint(int x, int y, int w, int h);

	virtual void			GetAbsDimensions(int &x, int &y, int &w, int &h) { CPanel::GetAbsDimensions(x, y, w, h); }
	virtual void			SetVisible(bool bVisible) { CPanel::SetVisible(bVisible); }

	virtual void			Open();
	virtual void			Close();

	EVENT_CALLBACK(COneForceBaseHintPopup, Close);

protected:
	CLabel*					m_pHint;
	CButton*				m_pOK;
};

class CRuneAdvancedPanel : public CPanel, public IEventListener
{
public:
							CRuneAdvancedPanel();

	virtual void			Delete() { delete this; };

	virtual void			SetVisible(bool bVis);

	virtual void			Layout();
	virtual void			LayoutChoice(CRuneChoice* pChoice, int i);

	virtual bool			SlotExists(int iSlot);

protected:
	CLabel*					m_pChoose;
	CLabel*					m_pGetAWeaponStupid;

	CPanel*					m_pRunes;

	CUtlVector<CLabel*>		m_apCats;

	CUtlVector<CRuneChoice*>	m_apBaseRunes;
	CUtlVector<CRuneChoice*>	m_apForceRunes;
	CUtlVector<CRuneChoice*>	m_apSupportRunes;

	CSlidingContainer*		m_pSlidingContainer;
	CSlidingPanel*			m_apSliders[4];

	CRuneCombosPanel*		m_apCombos[5];
	CRuneLayoutPanel*		m_apLayouts[5];

	COneForceBaseHintPopup*	m_pPopup;
};

class CRuneArmamentCombos : public CPanel
{
public:
							CRuneArmamentCombos(IComboMaster* pMaster, int iArmament);

	virtual void			Delete() { delete this; };

	virtual void			Layout();

	virtual void			Paint(int x, int y, int w, int h);

	virtual CSelectableRuneCombo*	GetCombo(int i) { return m_apCombos[i]; };

	CUtlVector<CSelectableRuneCombo*>	m_apCombos;
	CUtlVector<CLabel*>		m_apXLabels;

	int						m_iArmament;
	static int				s_iSlotWidth;
};

class CRunePresetButton : public CButton
{
public:
							CRunePresetButton();

	virtual void			Delete() { delete this; };

	virtual void			Paint(int x, int y, int w, int h);

	virtual void			CursorIn();
	virtual void			CursorOut();

	virtual void			SetPreset(CRuneComboPreset* pPreset);
	virtual CRuneComboPreset*	GetPreset() { return m_pPreset; };

	virtual void			SetHighlightListener(IHighlightListener<CRuneComboPreset*>* pListener) { m_pHighlightListener = pListener; };

protected:
	CRuneComboPreset*		m_pPreset;

	IHighlightListener<CRuneComboPreset*>*	m_pHighlightListener;
};

class CRuneSimplePresets : public CPanel, public IEventListener, public IHighlightListener<CRuneComboPreset*>
{
public:
							CRuneSimplePresets();

	virtual void			Delete() { delete this; };

	virtual void			Layout();

	virtual void			SetRune();

	virtual void			Highlighted(CRuneComboPreset* pPreset);

	EVENT_CALLBACK(CRuneSimplePresets, ChooseCombo);

	CUtlVector<CRunePresetButton*>	m_apPresets;
	CLabel*					m_pCombosLabel;

	CLabel*					m_pDescription;
};

class CRuneSimplePanel : public CPanel, public IComboMaster
{
public:
							CRuneSimplePanel();

	virtual void			Delete() { delete this; };

	virtual void			Layout();

	virtual void			RequestFocus(CSelectableRuneCombo* pRequesting);
	virtual CRuneDrop*		GetRuneDrop(int iMod);

	CLabel*					m_pGetAWeaponStupid;

	CUtlVector<CRuneArmamentCombos*>	m_apCombos;

	CRuneSimplePresets*		m_pPresets;
};

class CRunePanel : public CPanel, public IEventListener
{
public:
							CRunePanel();

	virtual void			Destructor();
	virtual void			Delete() { delete this; };

	virtual void			Layout();

	virtual void			SetAdvanced(bool bAdvanced);

	static void				SetActive(int iArmament, int iCombo);
	static void				GetActive(int &iArmament, int &iCombo);
	static int				GetActiveArmament() { return s_iActiveArmament; };
	static int				GetActiveCombo() { return s_iActiveCombo; };

	EVENT_CALLBACK(CRunePanel, Advanced);

	CButton*				m_pAdvancedButton;

	bool					m_bAdvanced;

	CRuneAdvancedPanel*		m_pAdvancedPanel;
	CRuneSimplePanel*		m_pSimplePanel;

private:
	static CRunePanel*		s_pRunePanel;

	static int				s_iActiveArmament;
	static int				s_iActiveCombo;
};

class CAttackBindDrop : public CDroppableIcon<CRuneComboDraggable>, public IEventListener
{
public:
							CAttackBindDrop(int iBind);
	virtual void			Delete() { delete this; };

	virtual void			Layout();

	virtual void			DraggableChanged(CRuneComboDraggable*);

	virtual bool			CanDropHere(IDraggable*);

	EVENT_CALLBACK(CAttackBindDrop, Remove);

protected:
	int						m_iBind;

	CDeleteButton*			m_pDelete;
};

class CAttkDropPanel : public CPanel
{
public:
					CAttkDropPanel(int iBind);
	virtual void	Destructor();
	virtual void	Delete() { delete this; };

	virtual void	Layout();
	virtual void	Paint(int x, int y, int w, int h);

	virtual bool	MousePressed(vgui::MouseCode code);

protected:
	int				m_iBind;

	CLabel*			m_pLabel;
	CDroppableIcon<CRuneComboDraggable>*	m_pDrop;
};

class CBindPanel : public CPanel
{
public:
							CBindPanel();

	virtual void			Layout();

	virtual void			Delete() { delete this; };

protected:
	CLabel*					m_pChoose;

	CPanel*					m_pRunes;

	CUtlVector<CRunePosition*>	m_apRuneBinds;

	CUtlVector<CRuneDropPanel*>	m_apRuneDropPanels;

	CUtlVector<CDroppableChoice<RuneID, CRuneComboDraggable>*>	m_apBindRunes;

	CAttkDropPanel*			m_apBinds[4];
};

#endif // CFMENU_H
