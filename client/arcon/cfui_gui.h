#ifndef CFGUI_H
#define CFGUI_H

#include "hud.h"
#include <vgui_controls/Frame.h>
#include <game/client/iviewport.h>
#include "utlvector.h"
#include "keyvalues.h"
#include <igameevents.h>
#include "armament.h"

// Not my favorite hack.
#define EVENT_CALLBACK(type, pfn) \
	void pfn##Callback(KeyValues* pParms); \
	static void pfn(IEventListener* obj, KeyValues* pParms) \
	{ \
		((type*)obj)->pfn##Callback(pParms); \
	}

namespace cfgui
{
	class CRect
	{
	public:
		CRect(int x, int y, int w, int h) { this->x = x; this->y = y; this->w = w; this->h = h; };
		CRect() { CRect(0, 0, 0, 0); };

		int Size() { return w*h; }

		int x;
		int y;
		int w;
		int h;
	};

	// Things to do with the automatic screen resizing.
	int CFScreenWidth();
	int CFScreenHeight();

	float CFScreenWidthScale();
	float CFScreenHeightScale();

	class CCFHudTexture : public CHudTexture
	{
	public:
		void DrawSelf( int x, int y, Color& clr ) const;
		void DrawSelf( int x, int y, int w, int h, Color& clr ) const;
		void DrawSelfCropped( int x, int y, int cropx, int cropy, int cropw, int croph, int finalWidth, int finalHeight, Color& clr ) const;
	};

	inline CCFHudTexture* ToCFTex(CHudTexture* rhs)
	{
		return (CCFHudTexture*)rhs;	// It's that easy.
	}

	CCFHudTexture* GetHudTexture(const char* pszName);

	class IPopup
	{
	public:
		virtual void		Open()=0;
		virtual void		Close()=0;
	
		virtual void		GetAbsDimensions(int &x, int &y, int &w, int &h)=0;	// Screen space

		virtual void		SetVisible(bool bVisible)=0;
	};

	class IControl
	{
	public:
		virtual IControl*	GetParent()=0;
		virtual void		SetParent(IControl* pParent)=0;

		virtual void		LevelShutdown()=0;
		virtual void		LoadTextures()=0;

		virtual void		SetSize(int w, int h)=0;
		virtual void		SetPos(int x, int y)=0;
		virtual void		GetSize(int &w, int &h)=0;
		virtual void		GetPos(int &x, int &y)=0;
		virtual void		GetAbsPos(int &x, int &y)=0;	// Screen space
		virtual void		GetAbsDimensions(int &x, int &y, int &w, int &h)=0;	// Screen space
		virtual void		GetBR(int &x, int &y)=0;
		virtual int			GetWidth()=0;
		virtual int			GetHeight()=0;

		virtual bool		IsVisible()=0;
		virtual void		SetVisible(bool bVisible)=0;
		virtual void		Paint()=0;
		virtual void		Paint(int x, int y)=0;
		virtual void		Paint(int x, int y, int w, int h)=0;
		virtual void		Layout()=0;
		virtual void		Think()=0;

		virtual void		PostRenderVGui()=0;

		virtual bool		KeyPressed(vgui::KeyCode code)=0;
		virtual bool		KeyReleased(vgui::KeyCode code)=0;
		virtual bool		MousePressed(vgui::MouseCode code)=0;
		virtual bool		MouseReleased(vgui::MouseCode code)=0;
		virtual bool		IsCursorListener()=0;
		virtual void		CursorMoved(int x, int y)=0;
		virtual void		CursorIn()=0;
		virtual void		CursorOut()=0;

		virtual void		Destructor()=0;
		virtual void		Delete()=0;
	};

	class IDroppable;

	// An object that can be grabbed and dragged around the screen.
	class IDraggable
	{
	public:
		typedef enum
		{
			DC_UNSPECIFIED	= 0,
			DC_WEAPONICON,
			DC_RUNEICON,
			DC_RUNECOMBO,
		} DragClass_t;		// Where the hookers go to learn their trade.
		virtual void			Destructor()=0;
		virtual void			Delete()=0;

		virtual void			SetHoldingRect(const CRect)=0;
		virtual CRect			GetHoldingRect()=0;

		virtual IDroppable*		GetDroppable()=0;
		virtual void			SetDroppable(IDroppable* pDroppable)=0;

		virtual void			Paint()=0;
		virtual void			Paint(int x, int y)=0;
		virtual void			Paint(int x, int y, int w, int h)=0;
		virtual void			Paint(int x, int y, int w, int h, bool bFloating)=0;

		virtual DragClass_t		GetClass()=0;
		virtual IDraggable&		MakeCopy()=0;
	};

	// A place where an IDraggable is allowed to be dropped.
	class IDroppable
	{
	public:
		virtual void			Destructor()=0;
		virtual void			Delete()=0;

		virtual IControl*		GetParent()=0;
		virtual void			SetParent(IControl* pParent)=0;

		// Get the place where a droppable object should be.
		virtual const CRect		GetHoldingRect()=0;

		virtual void			AddDraggable(IDraggable*)=0;
		virtual void			SetDraggable(IDraggable*, bool bDelete = true)=0;
		virtual void			ClearDraggables(bool bDelete = true)=0;
		virtual IDraggable*		GetDraggable(int i)=0;
		virtual IDraggable*		GetCurrentDraggable()=0;

		// I already know.
		virtual void			SetGrabbale(bool bGrabbable)=0;
		virtual bool			IsGrabbale()=0;

		virtual bool			CanDropHere(IDraggable*)=0;

		// Is this droppable a bottomless pit of draggables?
		virtual bool			IsInfinite()=0;

		virtual bool			IsVisible()=0;
	};

	class IEventListener
	{
	public:
		typedef void (*Callback)(IEventListener*, KeyValues*);
	};

	class CBaseControl : public IControl
	{
	public:
						CBaseControl(int x, int y, int w, int h);
						CBaseControl(const CRect& Rect);

		virtual void	Destructor();
		virtual void	Delete() { delete this; };

		virtual IControl*	GetParent() { return m_pParent; };
		virtual void	SetParent(IControl* pParent) { m_pParent = pParent; };

		virtual void	LoadTextures() {};

		virtual void	Paint() {};
		virtual void	Paint(int x, int y) {};
		virtual void	Paint(int x, int y, int w, int h) {};
		virtual void	Layout() {};
		virtual void	Think() {};

		virtual void	PostRenderVGui() {};

		virtual void	SetSize(int w, int h) { m_iW = w; m_iH = h; };
		virtual void	SetPos(int x, int y) { m_iX = x; m_iY = y; };
		virtual void	GetSize(int &w, int &h) { w = m_iW; h = m_iH; };
		virtual void	GetPos(int &x, int &y) { x = m_iX; y = m_iY; };
		virtual void	GetAbsPos(int &x, int &y);
		virtual void	GetAbsDimensions(int &x, int &y, int &w, int &h);
		virtual int		GetWidth() { return m_iW; };
		virtual int		GetHeight() { return m_iH; };
		virtual void	SetDimensions(int x, int y, int w, int h) { m_iX = x; m_iY = y; m_iW = w; m_iH = h; };	// Local space
		virtual void	SetDimensions(const CRect& Dims) { SetDimensions(Dims.x, Dims.y, Dims.w, Dims.h); };	// Local space
		virtual void	GetBR(int &x, int &y) { x = m_iX + m_iW; y = m_iY + m_iH; };

		virtual void	SetVisible(bool bVis) { m_bVisible = bVis; };
		virtual bool	IsVisible();

		virtual void	LevelShutdown( void ) { return; };
		virtual bool	KeyPressed(vgui::KeyCode code) { return false; };
		virtual bool	KeyReleased(vgui::KeyCode code) { return false; };
		virtual bool	MousePressed(vgui::MouseCode code) { return false; };
		virtual bool	MouseReleased(vgui::MouseCode code) { return false; };
		virtual bool	IsCursorListener() { return false; };
		virtual void	CursorMoved(int x, int y) {};
		virtual void	CursorIn() {};
		virtual void	CursorOut() {};

#ifdef _DEBUG
		virtual void	PaintDebugRect(int x, int y, int w, int h);
#endif

	protected:
		IControl*		m_pParent;

		int				m_iX;
		int				m_iY;
		int				m_iW;
		int				m_iH;

		bool			m_bVisible;
	};

	// A panel is a container for other controls. It is for organization
	// purposes only; it does not currently keep its children from drawing
	// outside of it.
	class CPanel : public CBaseControl
	{

#ifdef _DEBUG
		// Just so CBaseControl can get at CPanel's textures for the purpose of debug paint methods.
		friend class CBaseControl;
#endif

	public:
								CPanel(int x, int y, int w, int h);
		virtual void			Destructor();
		virtual void			Delete() { delete this; };

		virtual void			Paint();
		virtual void			Paint(int x, int y);
		virtual void			Paint(int x, int y, int w, int h);
		virtual void			Layout();
		virtual void			Think();

		virtual void			PostRenderVGui();

		virtual void			LevelShutdown( void );

		virtual bool			KeyPressed(vgui::KeyCode code);
		virtual bool			KeyReleased(vgui::KeyCode code);
		virtual bool			MousePressed(vgui::MouseCode code);
		virtual bool			MouseReleased(vgui::MouseCode code);
		virtual bool			IsCursorListener() {return true;};
		virtual void			CursorMoved(int mx, int my);
		virtual void			CursorOut();

		virtual void			AddControl(IControl* pControl, bool bToTail = false);
		virtual void			RemoveControl(IControl* pControl);

		virtual void			SetHighlighted(bool bHighlight) { m_bHighlight = bHighlight; };
		virtual bool			IsHighlighted() { return m_bHighlight; };

		typedef enum
		{
			BT_NONE	= 0,
			BT_SOME = 1
		} Border;

		void					SetBorder(Border b) { m_eBorder = b; };

	protected:
		virtual void			PaintBorder(int x, int y, int w, int h);

		static CCFHudTexture*	s_pPanelL;
		static CCFHudTexture*	s_pPanelR;
		static CCFHudTexture*	s_pPanelT;
		static CCFHudTexture*	s_pPanelB;
		static CCFHudTexture*	s_pPanelTL;
		static CCFHudTexture*	s_pPanelTR;
		static CCFHudTexture*	s_pPanelBL;
		static CCFHudTexture*	s_pPanelBR;
		static CCFHudTexture*	s_pPanelC;

		static CCFHudTexture*	s_pPanelHL;
		static CCFHudTexture*	s_pPanelHR;
		static CCFHudTexture*	s_pPanelHT;
		static CCFHudTexture*	s_pPanelHB;
		static CCFHudTexture*	s_pPanelHTL;
		static CCFHudTexture*	s_pPanelHTR;
		static CCFHudTexture*	s_pPanelHBL;
		static CCFHudTexture*	s_pPanelHBR;
		static CCFHudTexture*	s_pPanelHC;

		CUtlVector<IControl*>	m_apControls;

		// If two controls in the same panel are never layered, a single
		// pointer should suffice. Otherwise a list must be created.
		IControl*				m_pHasCursor;

		Border					m_eBorder;

		bool					m_bHighlight;
		bool					m_bDestructing;
	};

	class CDroppablePanel : public CPanel, public IDroppable
	{
	public:
							CDroppablePanel(int x, int y, int w, int h);
		virtual void		Destructor();
		virtual void		Delete() { delete this; };

		// It's already in CBaseControl, but we need this again for IDroppable.
		virtual IControl*	GetParent() { return CPanel::GetParent(); };
		virtual void		SetParent(IControl* pParent) { return CPanel::SetParent(pParent); };
		virtual bool		IsVisible() { return CPanel::IsVisible(); };

		virtual void		Paint(int x, int y, int w, int h);

		virtual void		SetSize(int w, int h);
		virtual void		SetPos(int x, int y);

		virtual bool		MousePressed(vgui::MouseCode code);

		virtual void		AddDraggable(IDraggable* pDragged);
		virtual void		SetDraggable(IDraggable* pDragged, bool bDelete = true);
		virtual void		ClearDraggables(bool bDelete = true);
		virtual IDraggable*	GetDraggable(int i);

		virtual void		SetGrabbale(bool bGrabbable) { m_bGrabbable = bGrabbable; };
		virtual bool		IsGrabbale() { return m_bGrabbable; };

	protected:
		bool				m_bGrabbable;

		CUtlVector<IDraggable*>	m_apDraggables;
	};

	class CRootPanel : public CPanel, public vgui::Frame, public IViewPortPanel, public IGameEventListener2
	{
		DECLARE_CLASS_SIMPLE( CRootPanel, vgui::Frame );
	public:
									CRootPanel( IViewPort *pViewPort );
									~CRootPanel( );
		virtual void				Destructor( );
		virtual void				Delete() { delete this; };

		virtual void				LevelShutdown( void );

		virtual void				OnThink();
		virtual void				Init();
		virtual void				CreateHUDIndicators();
		virtual void				Paint();
		virtual void				Layout();

		virtual void				PostRenderVGui();

		virtual void				SetBackground( bool bOn ) { m_bBackground = bOn; };
		virtual bool				IsBackgroundOn( ) { return m_bBackground; };

		virtual const char*			GetName( void ) { return PANEL_CFGUI; }
		virtual void				SetData(KeyValues *data) {};
		virtual void				Reset() {};
		virtual void				Update() {};
		virtual bool				NeedsUpdate( void ) { return false; }
		virtual bool				HasInputElements( void ) { return true; }
		virtual void				ShowPanel( bool bShow ) {};

		// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
		vgui::VPANEL				GetVPanel( void ) { return BaseClass::GetVPanel(); }
		virtual bool				IsVisible() { return BaseClass::IsVisible() && CPanel::IsVisible(); }
	  	virtual void				SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }

		virtual void				EnableKeyboard(bool bEnabled);
		virtual void				OnKeyCodePressed(vgui::KeyCode code);
		virtual void				OnKeyCodeReleased(vgui::KeyCode code);
		virtual void				EnableMouse(bool bEnabled);
		virtual void				OnMousePressed(vgui::MouseCode code);
		virtual void				OnMouseReleased(vgui::MouseCode code);
		virtual void				OnCursorMoved(int mx, int my);
		virtual void				OnKeyCodeTyped(vgui::KeyCode code);

		// Dragon Drop stuff is in this class, because this is always the
		// top-level panel so all the messages go through it first.
		virtual void				DragonDrop(IDroppable* pDroppable);
		virtual void				AddDroppable(IDroppable* pDroppable);
		virtual void				RemoveDroppable(IDroppable* pDroppable);
		virtual bool				DropDraggable();
		virtual IDraggable*			GetCurrentDraggable() { return m_pDragging?m_pDragging->GetCurrentDraggable():NULL; };
		virtual IDroppable*			GetCurrentDroppable() { return m_pDragging; };

		virtual void				Popup(IPopup* pControl);

		virtual void				Activate( bool bEnabled );

		virtual void				FireGameEvent( IGameEvent *event );

		void						SetButtonDown(class CButton* pButton);
		class CButton*				GetButtonDown();

		virtual const char*			GetHostname() { return m_pszHostname; };

		inline static CRootPanel*	GetRoot() { return s_pRootPanel; };
		static vgui::HFont			s_hDefaultFont;

		static void					SetArmament(CArmament* pArm);
		static void					UpdateArmament(CArmament* pArm);

		// Should the window close after the selections have been made? (ie team choosing?)
		// If not, it should advance to the next window.
		static void					SetCloseAfter(bool bCloseAfter);
		static bool					GetCloseAfter();

		static void					GetFullscreenMousePos(int& mx, int& my);
		static void					DrawRect(int x, int y, int x2, int y2, Color clr);

	private:
		virtual void				ApplySchemeSettings( vgui::IScheme *scheme );

		static CRootPanel*			s_pRootPanel;

		CArmament*					m_pArm;

		CUtlVector<IDroppable*>		m_apDroppables;
		IDroppable*					m_pDragging;

		IPopup*						m_pPopup;

		bool						m_bBackground;
		bool						m_bCloseAfter;

		// If the mouse is released over nothing, then try popping this button.
		CButton*					m_pButtonDown;

		bool						m_bFollowMode;
		float						m_flLetterboxGoal;	// A multiple
		float						m_flLetterboxCurr;	// A multiple

		char*						m_pszHostname;

		float						m_flMenuMusicVolume;
		float						m_flMenuMusicGoal;
		bool						m_bMenuMusicPlaying;
	};

	class CLabel : public CBaseControl
	{
		friend CRootPanel;

	public:
						CLabel(int x, int y, int w, int h, char* pszText);
		virtual void	Destructor();
		virtual void	Delete() { delete this; };

		typedef enum
		{
			TA_TOPLEFT		= 0,
			TA_LEFTCENTER	= 1,
			TA_CENTER		= 2,
			TA_RIGHTCENTER	= 3,
		} TextAlign;

		virtual void	Paint() { int x = 0, y = 0; GetAbsPos(x, y); Paint(x, y); };
		virtual void	Paint(int x, int y) { Paint(x, y, m_iW, m_iH); };
		virtual void	Paint(int x, int y, int w, int h);
		virtual void	DrawLine(wchar_t* pszText, unsigned iLength, int x, int y, int w, int h);
		virtual void	Layout() {};
		virtual void	Think() {};

		virtual void	SetSize(int w, int h);

		virtual bool	MousePressed(vgui::MouseCode code) {return false;};
		virtual bool	MouseReleased(vgui::MouseCode code) {return false;};
		virtual bool	IsCursorListener() {return false;};

		virtual bool	IsEnabled() {return m_bEnabled;};
		virtual void	SetEnabled(bool bEnabled) {m_bEnabled = bEnabled;};

		virtual TextAlign	GetAlign() { return m_eAlign; };
		virtual void	SetAlign(TextAlign eAlign) { m_eAlign = eAlign; };

		virtual bool	GetWrap() { return m_bWrap; };
		virtual void	SetWrap(bool bWrap) { m_bWrap = bWrap; };

		virtual void	SetText(const wchar_t* pszText);
		virtual void	SetText(const char* pszText);
		virtual void	AppendText(const char* pszText);
		virtual void	AppendText(const wchar_t* pszText);
		virtual const wchar_t*	GetText();

		virtual int		GetTextWidth();
		virtual int		GetTextHeight();
		virtual void	ComputeLines(int w = -1, int h = -1);
		virtual void	EnsureTextFits();
		static int		GetTextWidth( vgui::HFont& font, const wchar_t *str, int iLength );

		virtual Color	GetFGColor();
		virtual void	SetFGColor(Color FGColor);

	protected:
		bool			m_bEnabled;
		bool			m_bWrap;
		wchar_t*		m_pszText;
		Color			m_FGColor;

		TextAlign		m_eAlign;

		int				m_iTotalLines;
		int				m_iLine;
	};

	class CButton : public CLabel
	{
		friend CRootPanel;
		friend class CSlidingPanel;

	public:
						CButton(int x, int y, int w, int h, char* szText, bool bToggle = false);
		virtual void	Destructor();
		virtual void	Delete() { delete this; };

		virtual void	Paint() { CLabel::Paint(); };
		virtual void	Paint(int x, int y, int w, int h);
		virtual void	PaintButton(int x, int y, int w, int h);

		virtual bool	MousePressed(vgui::MouseCode code);
		virtual bool	MouseReleased(vgui::MouseCode code);
		virtual bool	IsCursorListener() {return true;};
		virtual void	CursorIn();
		virtual void	CursorOut();

		virtual bool	IsToggleButton() {return m_bToggle;};
		virtual void	SetToggleState(bool bState);
		virtual bool	GetToggleState() {return m_bToggleOn;};

		virtual bool	Push();
		virtual bool	Pop(bool bRegister = true, bool bRevert = false);
		virtual void	SetState(bool bDown, bool bRegister = true);
		virtual bool	GetState() {return m_bDown;};

		virtual void	SetClickedListener(IEventListener* pListener, IEventListener::Callback pfnCallback, KeyValues *pParms = NULL);
		// Toggle buttons only
		virtual void	SetUnclickedListener(IEventListener* pListener, IEventListener::Callback pfnCallback, KeyValues *pParms = NULL);

		virtual bool	IsHighlighted() {return m_bHighlight;};

	protected:
		bool			m_bToggle;
		bool			m_bToggleOn;
		bool			m_bDown;
		bool			m_bHighlight;

		// Need multiple event listeners? Too bad! Make a list.
		IEventListener::Callback m_pfnClickCallback;
		IEventListener*	m_pClickListener;
		KeyValues*		m_pClickParms;

		IEventListener::Callback m_pfnUnclickCallback;
		IEventListener*	m_pUnclickListener;
		KeyValues*		m_pUnclickParms;

		// Regular Ol' Button
		static CCFHudTexture* s_pButtonL;
		static CCFHudTexture* s_pButtonR;
		static CCFHudTexture* s_pButtonT;
		static CCFHudTexture* s_pButtonB;
		static CCFHudTexture* s_pButtonTL;
		static CCFHudTexture* s_pButtonTR;
		static CCFHudTexture* s_pButtonBL;
		static CCFHudTexture* s_pButtonBR;
		static CCFHudTexture* s_pButtonC;

		// Highlighted Button
		static CCFHudTexture* s_pButtonHL;
		static CCFHudTexture* s_pButtonHR;
		static CCFHudTexture* s_pButtonHT;
		static CCFHudTexture* s_pButtonHB;
		static CCFHudTexture* s_pButtonHTL;
		static CCFHudTexture* s_pButtonHTR;
		static CCFHudTexture* s_pButtonHBL;
		static CCFHudTexture* s_pButtonHBR;
		static CCFHudTexture* s_pButtonHC;

		// Depressed Button (Needs ice cream)
		static CCFHudTexture* s_pButtonDL;
		static CCFHudTexture* s_pButtonDR;
		static CCFHudTexture* s_pButtonDT;
		static CCFHudTexture* s_pButtonDB;
		static CCFHudTexture* s_pButtonDTL;
		static CCFHudTexture* s_pButtonDTR;
		static CCFHudTexture* s_pButtonDBL;
		static CCFHudTexture* s_pButtonDBR;
		static CCFHudTexture* s_pButtonDC;

		// Inactive Button
		static CCFHudTexture* s_pButtonIL;
		static CCFHudTexture* s_pButtonIR;
		static CCFHudTexture* s_pButtonIT;
		static CCFHudTexture* s_pButtonIB;
		static CCFHudTexture* s_pButtonITL;
		static CCFHudTexture* s_pButtonITR;
		static CCFHudTexture* s_pButtonIBL;
		static CCFHudTexture* s_pButtonIBR;
		static CCFHudTexture* s_pButtonIC;
	};

	class CSlidingContainer;
	class CSlidingPanel : public CPanel
	{
	public:
		friend CRootPanel;

		class CInnerPanel : public CPanel
		{
		public:
									CInnerPanel(CSlidingContainer* pMaster);
			
			virtual void			Delete() { delete this; };

			virtual bool			IsVisible();

			CSlidingContainer*		m_pMaster;
		};

									CSlidingPanel(CSlidingContainer* pParent, char* pszTitle);

		virtual void				Delete() { delete this; };

		virtual void				Layout();
		virtual void				Paint(int x, int y, int w, int h);

		virtual void				AddControl(IControl* pControl, bool bToTail = false);

		virtual bool				MousePressed(vgui::MouseCode code);

		virtual void				SetCurrent(bool bCurrent);

		virtual void				SetTitle(char* pszNew) { m_pTitle->SetText(pszNew); };
		virtual void				SetTitle(wchar_t* pszNew) { m_pTitle->SetText(pszNew); };
		virtual void				AppendTitle(char* pszNew) { m_pTitle->AppendText(pszNew); };
		virtual void				AppendTitle(wchar_t* pszNew) { m_pTitle->AppendText(pszNew); };

		static const int			SLIDER_COLLAPSED_HEIGHT = 30;

	protected:
		bool						m_bCurrent;

		CLabel*						m_pTitle;

		CPanel*						m_pInnerPanel;

		static CCFHudTexture*		s_pArrowExpanded;
		static CCFHudTexture*		s_pArrowCollapsed;
	};

	class CSlidingContainer : public CPanel
	{
	public:
									CSlidingContainer(int x, int y, int w, int h);

		virtual void				Delete() { delete this; };

		virtual void				Layout();

		virtual void				AddControl(IControl* pControl, bool bToTail = false);

		virtual bool				IsCurrent(int iPanel);
		virtual void				SetCurrent(int iPanel);
		virtual bool				IsCurrent(CSlidingPanel* pPanel);
		virtual void				SetCurrent(CSlidingPanel* pPanel);

		virtual bool				IsCurrentValid();

		virtual int					VisiblePanels();

	protected:
		int							m_iCurrent;
	};

	class CInputManager
	{
	public:
									CInputManager();
									~CInputManager();

		void						Activate(bool bOn);

		static bool					ShouldHaveInput() { return s_iActivated; };

	private:
		bool						m_bActivated;
		static int					s_iActivated;
	};
};

#endif
