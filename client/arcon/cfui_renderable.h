#ifndef CF_RENDERABLE_H
#define CF_RENDERABLE_H

#include "weapon_cfbase.h"

class CCFRenderable
{
public:
					CCFRenderable();

	virtual void	SetModelName(const char* pszName);
	virtual void	SetActivity(Activity iActivity);
	virtual void	SetSkin(int iSkin);

	virtual void	SetCopyTarget(C_BaseAnimatingOverlay* pCopy);
	virtual C_BaseAnimatingOverlay*	GetCopyTarget() { return m_hCopy; };

	virtual void	Render(int x, int y, int w, int h, Vector vecOffset = Vector(0, 0, 0), QAngle angDirection = QAngle(0, -160, 0));
	virtual void	DrawOtherModels();

	virtual bool	ShouldRecreatePreviewModel( const char *pNewModelName );
	virtual void	RecreatePreviewModel();
	virtual C_BaseAnimatingOverlay*	CreateModel() { return new C_BaseAnimatingOverlay(); };

	virtual C_BaseAnimatingOverlay*	GetModel() { return m_hModel; };
	virtual void	SetModel(C_BaseAnimatingOverlay* pModel) { m_hModel = pModel; };

	virtual void	AddParticles(CUtlVector<CNewParticleEffect*>& apNewParticles);
	virtual void	RemoveParticles(CUtlVector<CNewParticleEffect*>& apOldParticles);

	virtual bool	IsDirty() { return m_bDirty; };

protected:
	const char*		m_pszModelName;
	Activity		m_iActivity;
	int				m_iSkin;
	bool			m_bDirty;

	CHandle<C_BaseAnimatingOverlay>	m_hCopy;

	CTextureReference		m_DefaultEnvCubemap;
	CTextureReference		m_DefaultHDREnvCubemap;

	CUtlVector<CNewParticleEffect*>	m_apParticles;

private:
	CHandle<C_BaseAnimatingOverlay>	m_hModel;
};

class CPreviewWeapon : public C_BaseAnimatingOverlay
{
public:
	void			SetHand(bool bRight) { m_bRightHand = bRight; };

	virtual char*	GetBoneMergeName(int iBone);

protected:
	bool			m_bRightHand;
};

class CCFRenderableWeapon : public CCFRenderable
{
public:
					CCFRenderableWeapon();

	virtual void	SetID(CFWeaponID eWeapon);

	void			SetHand(bool bIsPrimary, bool bHasPrimary);

	virtual C_BaseAnimatingOverlay*	CreateModel() { return new CPreviewWeapon(); };
	virtual C_BaseAnimatingOverlay*	GetModel() { return m_hWeaponModel; };
	virtual void	SetModel(C_BaseAnimatingOverlay* pModel);

protected:
	CFWeaponID		m_eWeapon;
	bool			m_bIsMelee;

private:
	CHandle<CPreviewWeapon>	m_hWeaponModel;
};

class CCFRenderablePlayer : public CCFRenderable
{
public:
	virtual void			SetPrimaryWeapon(CFWeaponID eWeapon);
	virtual void			SetSecondaryWeapon(CFWeaponID eWeapon);

	virtual void			DrawOtherModels();

	virtual void			RecreatePreviewModel();

	virtual bool			IsDirty();

protected:
	CCFRenderableWeapon		m_PrimaryWeapon;
	CCFRenderableWeapon		m_SecondaryWeapon;
};

#endif