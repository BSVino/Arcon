#ifndef HUD_INDICATORS_H
#define HUD_INDICATORS_H

#include "cfui_hud.h"
#include "weapon_cfbase.h"
#include <utlvector.h>
#include "armament.h"
#include "cfui_draggables.h"
#include "cfui_renderable.h"

#pragma warning( disable : 4201 ) // warning C4201: nonstandard extension used : nameless struct/union

using namespace cfgui;

class CHudHealth : public ICFHUD
{
public:
					CHudHealth();
	virtual void	Destructor();
	virtual void	Delete() { delete this; };

	virtual void	LevelShutdown();
	void			LoadTextures();

	void			Paint(int x, int y, int w, int h);
	void			Think();

	static void		PaintBar(int iStyle, float flFilled, int x, int y, int w, int h, int a, float flFlash = 0);
	static float	GetAlphaMultiplier(float flFlash);

	int				GetAlpha() { return m_flAlpha*255; };
	void			Ping(float flAlpha = 1);

	void			SetBonusDimensions(int x, int y, int w, int h);
	void			AnimateBonus(int iBonusHealth);

	static CHudHealth*	Get() {return s_pHudHealth;};

private:
	float			m_flHealth;
	int				m_iMaxHealth;
	float			m_flLastHealth;
	float			m_flHealthFlash;

	float			m_flFocus;
	int				m_iMaxFocus;
	float			m_flLastFocus;
	float			m_flFocusFlash;

	float			m_flStamina;
	int				m_iMaxStamina;
	float			m_flLastStamina;
	float			m_flStaminaFlash;

	float			m_flAlpha;
	float			m_flAlphaGoal;
	float			m_flLastPing;

	float			m_flBonusX;
	float			m_flBonusY;
	float			m_flBonusW;
	float			m_flBonusH;
	float			m_flBonusHealth;
	float			m_flBonusStart;

	static CCFHudTexture*	s_pHealth;
	static CCFHudTexture*	s_pFocus;
	static CCFHudTexture*	s_pStamina;
	static CCFHudTexture*	s_pBarsL;
	static CCFHudTexture*	s_pBarsR;
	static CCFHudTexture*	s_pBarsC;
	static CCFHudTexture*	s_pDamage;
	static CCFHudTexture*	s_pHealthIcon;
	static CCFHudTexture*	s_pFocusIcon;
	static CCFHudTexture*	s_pStaminaIcon;

	static CHudHealth*	s_pHudHealth;
};

class CHudMagic : public ICFHUD
{
public:
						CHudMagic();
	virtual void		Destructor();
	virtual void		Delete() { delete this; };

	virtual void		LevelShutdown();
	
	void _cdecl			UserCmd_Slot1( void );
	void _cdecl			UserCmd_Slot2( void );
	void _cdecl			UserCmd_Slot3( void );
	void _cdecl			UserCmd_Slot4( void );
	void _cdecl			UserCmd_Slot5( void );
	void _cdecl			UserCmd_Slot6( void );
	void _cdecl			UserCmd_Slot7( void );
	void _cdecl			UserCmd_Slot8( void );
	void _cdecl			UserCmd_Slot9( void );
	void _cdecl			UserCmd_Slot0( void );
	void _cdecl			UserCmd_Close( void );

	void				LoadTextures();
	void				Layout();
	void				Think();
	void				Paint(int x, int y, int w, int h);

	void				OpenMenu();
	void				CloseMenu();
	void				SetActiveCombo(int iCombo);
	bool				IsMenuOpen();

	static CHudMagic*	Get() {return s_pHudMagic;};

private:
	float				m_flLastModeChange;
	bool				m_bMode;

	CLabel*				m_pNumbers;

	static CHudMagic*	s_pHudMagic;

	class CComboPanel : public CPanel
	{
	public:
							CComboPanel();
		virtual void		Delete() { delete this; };

		void				LoadTextures();
		void				Layout();

		void				Paint(int x, int y, int w, int h);
		void				Paint() { CPanel::Paint(); };

		void				OpenMenu();
		void				CloseMenu();

		virtual bool		KeyPressed( vgui::KeyCode code );

		void				SetActiveCombo(int iCombo);

	private:
		CCFHudTexture*		m_pMouseLeft;
		CCFHudTexture*		m_pMouseRight;

		int					m_iActiveCombo;

		int					m_iComboHeight;
	};

	CComboPanel*		m_pCombos;

};

class CWeaponDrop;
class CHudWeapons : public ICFHUD
{
public:
	class CHudWeaponIcon
	{
	public:
		void			Think();

		void			SetGoal(int x, int y, int w, int h, int a);

		CFWeaponID		m_eWeaponID;
		long			m_iSerializedCombo;

		int				x, y, w, h, a;
		int				gx, gy, gw, gh, ga;

		float			m_flFlash;
	};

						CHudWeapons();
	virtual void		Destructor();
	virtual void		Delete() { delete this; };

	virtual void		LevelShutdown();

	void				SetWeaponSlot(int iSlot, CFWeaponID eWeapon);
	void				RemoveWeaponSlot(int iSlot);
	void				SetMagicSlot(int iSlot, long iCombo);
	void				RemoveMagicSlot(int iSlot);

	void				LoadTextures();
	void				PaintWeapon(CHudWeaponIcon* pIcon, C_WeaponCFBase* pWeapon);
	void				PaintAltAbility(CHudWeaponIcon* pIcon, C_WeaponCFBase* pWeapon);
	void				PaintMagic(CHudWeaponIcon* pIcon);
	void				Paint(int x, int y, int w, int h);
	void				Think();

	virtual bool		MousePressed(vgui::MouseCode code);

	int					GetAlpha() { return 255; };

	static CHudWeapons*	Get() {return s_pHudWeapons;};

private:
	CCFHudTexture*		m_pWeaponPanel;
	CCFHudTexture*		m_pRifleBullets;
	CCFHudTexture*		m_pPistolBullets;
	CCFHudTexture*		m_pStrongAttack;
	CCFHudTexture*		m_pFirearmsAlternate;

	float				m_flNextThink;

	CHudWeaponIcon		m_aWeaponIcons[3];
	CHudWeaponIcon		m_aMagicIcons[ATTACK_BINDS];

	static CHudWeapons*	s_pHudWeapons;
};

class CHudStatusEffects : public ICFHUD
{
public:
					CHudStatusEffects();
	virtual void	Delete() { delete this; };

	virtual void	Init();
	virtual void	LevelShutdown();

	void			Think();
	void			Paint(int x, int y, int w, int h);

private:
	unsigned int	m_iStatusEffects;
	float			m_flStatusEffectStart[TOTAL_STATUSEFFECTS];
};

class CHudTarget : public ICFHUD
{
public:
						CHudTarget();
	virtual void		Destructor();
	virtual void		Delete() { delete this; };

	virtual void		LevelShutdown();
	void				LoadTextures();
	void				Think();
	void				Paint(int x, int y, int w, int h);

	void				PostRenderVGui();
	void				RenderTarget(CCFRenderablePlayer* pRenderable, C_CFPlayer* pTarget, int x, int y, int w, int h);
	void				RenderTargetIndicator(CCFHudTexture* pMaterial, int x, int y, int w, int h, float flElapsed);

	static CHudTarget*	Get() {return s_pTarget;};

	C_BaseEntity*		GetTarget() { return m_hTarget; };

private:
	class CHealthBar
	{
		friend class CHudTarget;
		EHANDLE			m_hPlayer;
		float			m_flAlpha;
	};

	static CHudTarget*	s_pTarget;

	EHANDLE				m_hTarget;
	float				m_flLastTargetCheck;
	float				m_flLastTarget;

	CCFRenderablePlayer	m_DirectTarget;
	CUtlVector<CNewParticleEffect*>	m_apDLHComboEffects;
	CUtlVector<CNewParticleEffect*>	m_apDRHComboEffects;
	element_t			m_eDLHEffectElements;
	element_t			m_eDRHEffectElements;

	CCFRenderablePlayer	m_RecursedTarget;
	CUtlVector<CNewParticleEffect*>	m_apRLHComboEffects;
	CUtlVector<CNewParticleEffect*>	m_apRRHComboEffects;
	element_t			m_eRLHEffectElements;
	element_t			m_eRRHEffectElements;

	CCFHudTexture*		m_pTargetFriend;
	CCFHudTexture*		m_pTargetEnemy;
	CCFHudTexture*		m_pTargetingFriend;

	Vector				m_vecTargetSize;
	Vector				m_vecDTL;
	Vector				m_vecDCenter;
	Vector				m_vecRTL;
	Vector				m_vecRCenter;
};

class CHudCFCrosshair : public ICFHUD
{
public:
							CHudCFCrosshair();
	virtual void			Destructor();
	virtual void			Delete() { delete this; };

	void					LoadTextures();
	void					Think();
	void					Paint(int x, int y, int w, int h);

	static CHudCFCrosshair*	Get() {return s_pCrosshair;};

private:
	CCFHudTexture*			m_pCrosshairIcon;

	CCFHudTexture*			m_pDFAUp;
	CCFHudTexture*			m_pDFADown;

	float					m_flFlash;

	CCFHudTexture*			m_pCrosshairDisabled;

	CCFHudTexture*			m_pTargetRing;

	static CHudCFCrosshair*	s_pCrosshair;
};

class CDeathNotifier : public CPanel
{
public:
						CDeathNotifier();
	virtual void		Delete() { delete this; };

	void				Layout();
	void				Think();
	void				Paint(int x, int y, int w, int h);

protected:
	CLabel*				m_pText;
	CLabel*				m_pRespawn;
};

class CHUDMessages : public CPanel
{
public:
						CHUDMessages();
	virtual void		Destructor();
	virtual void		Delete() { delete this; };

	virtual void		LevelShutdown();
	void				LoadTextures();

	void				FlagCaptured() { m_flLastFlagCap = gpGlobals->curtime; };
	void				Critical() { m_flLastCritical = gpGlobals->curtime; };
	void				Multiplier(int iMultiplier) { m_flLastMultiplier = gpGlobals->curtime; m_iLastMultiplier = iMultiplier; };

	void				Paint(int x, int y, int w, int h);

	float				m_flLastCritical;
	float				m_flLastFlagCap;
	float				m_flLastMultiplier;
	int					m_iLastMultiplier;

	static CHUDMessages*	Get() {return s_pHUDMessages;};

protected:
	CCFHudTexture*		m_pCritical;
	CCFHudTexture*		m_pOverdrive;
	CCFHudTexture*		m_pHit;
	CCFHudTexture*		m_pHitSpace;
	CCFHudTexture*		m_pHitX;
	CCFHudTexture*		m_pHits[10];
	CCFHudTexture*		m_pHitBang;

	static CHUDMessages*	s_pHUDMessages;
};

class CHUDHints : public CPanel
{
public:
						CHUDHints();
	virtual void		Destructor();
	virtual void		Delete() { delete this; };

	void				LoadTextures();
	virtual void		LevelShutdown();

	void				ProcessHint(CLabel* pLabel, const char* pszText);
	void				Lesson(CCFGameLesson* pLesson);

	void				Layout();
	void				Think();
	void				Paint(int x, int y, int w, int h);
	void				PaintCenterLesson();
	void				PaintFatalityLesson();
	void				PaintMagicMenuLesson();
	void				PaintCaptureAllLesson();

	float				m_flLastMessage;
	float				m_flLastLesson;

	static CHUDHints*	Get() {return s_pHUDHints;};

protected:
	CLabel*				m_pText;
	CLabel*				m_pFatality;
	CLabel*				m_pNumbers;
	CLabel*				m_pInstructorText;

	CCFHudTexture*		m_pInfoTexture;

	CCFGameLesson*		m_pLesson;

	bool				m_bShowingMagicMenuLesson;
	float				m_flStartedShowingMagicMenuLesson;

	bool				m_bShowingCollectAllLesson;
	float				m_flStartedShowingCollectAllLesson;

	static CHUDHints*	s_pHUDHints;
};

class CRank : public ICFHUD
{
public:
						CRank();
	virtual void		Delete() { delete this; };

	void				LoadTextures();

	void				Layout();
	void				Think();
	void				Paint(int x, int y, int w, int h);

protected:
	CLabel*				m_pYourRank;
	CLabel*				m_pRankName;

	CCFHudTexture*		m_pCaptain;
	CCFHudTexture*		m_pSergeant;
	CCFHudTexture*		m_pPromoted;

	int					m_iLastRank;
	float				m_flLastRankChange;
};

extern class CTextureHandler g_TextureHandler;

// A single base rune may have four versions.
class CRuneTexture
{
public:
						CRuneTexture() {};
	runetype_t			m_eRuneType;

	CCFHudTexture*	m_pTexture;
};

// Handles textures for weapons armor and runes.
class CTextureHandler
{
public:
	static void				LoadTextures();

	static CCFHudTexture*	GetRuneSlotTexture() { return Get()->m_pRuneSlotTexture; };

	typedef enum {
		TT_SQUARE,
		TT_WIDE,
		TT_HIGHLIGHT,
		TT_RADIAL,
	} TextureType;

	static CCFHudTexture*	GetTexture(CFWeaponID eWeapon, TextureType eType = TT_SQUARE);
	static CCFHudTexture*	GetTexture(ArmamentID eArmor, TextureType eType = TT_SQUARE);
	static CCFHudTexture*	GetTexture(RuneID eRune, TextureType eType = TT_SQUARE);
	static CCFHudTexture*	GetTexture(statuseffect_t eStatus);
	static CCFHudTexture*	GetTexture(forceeffect_t eStatus);
	static CCFHudTexture*	GetTexture(ButtonCode_t eButton);
	static CCFHudTexture*	GetTextureFromKey(const char * eKey);

	static CTextureHandler*	Get() {return &g_TextureHandler;};

private:
	CCFHudTexture*				m_pRuneSlotTexture;

	CUtlVector<CCFHudTexture*>	m_apWeaponTextures;
	CUtlVector<CCFHudTexture*>   m_apWeaponTexturesRadial;
	CUtlVector<CCFHudTexture*>	m_apWeaponTexturesWide;
	CUtlVector<CCFHudTexture*>	m_apWeaponTexturesHighlight;
	CUtlVector<CCFHudTexture*>	m_apArmorTextures;
	CUtlVector<CRuneTexture*>	m_apRuneTextures;

	CCFHudTexture*				m_pInfoTexture;
	CUtlMap<ButtonCode_t, CCFHudTexture*>	m_apKeyTextures;

	CCFHudTexture*				m_apStatusIcons[TOTAL_STATUSEFFECTS];

	CCFHudTexture*				m_apForceIcons[MAX_FORCEEFFECTS];
};

#endif
