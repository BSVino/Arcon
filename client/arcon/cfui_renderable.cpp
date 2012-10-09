#include "cbase.h"
#include "cfui_renderable.h"
#include "c_cf_player.h"
#include "cfui_gui.h"

// For rendering the player model
#include "iefx.h"
#include "dlight.h"
#include "view_shared.h"
#include "view.h"
#include "model_types.h"
#include "datacache/imdlcache.h"
#include "viewrender.h"

using namespace cfgui;

CCFRenderable::CCFRenderable()
{
	m_bDirty = true;
	m_pszModelName = "";
	m_iActivity = ACT_INVALID;
	m_iSkin = 0;
	m_hCopy = NULL;
}

void CCFRenderable::SetModelName(const char *pszName)
{
	if (ShouldRecreatePreviewModel(pszName))
		m_bDirty = true;

	m_pszModelName = pszName;
}

void CCFRenderable::SetActivity(Activity iActivity)
{
	m_iActivity = iActivity;
}

void CCFRenderable::SetCopyTarget(C_BaseAnimatingOverlay* pCopy)
{
	// Recreate model when we get a different copy target so that it doesn't blend animations from the old target to the new one.
	if (pCopy != m_hCopy)
		m_bDirty = true;

	m_hCopy = pCopy;
}

void CCFRenderable::SetSkin(int iSkin)
{
	m_iSkin = iSkin;
}

void CCFRenderable::Render(int x, int y, int w, int h, Vector vecOffset, QAngle angDirection)
{
	MDLCACHE_CRITICAL_SECTION();

	if (IsDirty())
		RecreatePreviewModel();

	if (GetModel() == NULL)
		return;

	if (m_hCopy != NULL)
	{
		// Copy all values from the given entity! Make us look exactly like him! I mean it!
		GetModel()->m_nSkin = m_hCopy->GetSkin();
		GetModel()->m_nBody = m_hCopy->GetBody();

		GetModel()->SetSequence(m_hCopy->GetSequence());
		GetModel()->SetCycle(m_hCopy->GetCycle());

		GetModel()->SetNumAnimOverlays(m_hCopy->m_AnimOverlay.Count());

		int i;
		for (i = 0; i < m_hCopy->m_AnimOverlay.Count(); i++)
		{
			CAnimationLayer *pDstLayer = GetModel()->GetAnimOverlay(i);
			CAnimationLayer *pSrcLayer = m_hCopy->GetAnimOverlay(i);

			pDstLayer->m_flWeight = pSrcLayer->m_flWeight;
			pDstLayer->m_nSequence = pSrcLayer->m_nSequence;
			pDstLayer->m_flPlaybackRate = pSrcLayer->m_flPlaybackRate;
			pDstLayer->m_flCycle = pSrcLayer->m_flCycle;
			pDstLayer->m_nOrder = pSrcLayer->m_nOrder;
			pDstLayer->m_flPrevCycle = pSrcLayer->m_flPrevCycle;
			pDstLayer->m_flLayerAnimtime = pSrcLayer->m_flLayerAnimtime;
			pDstLayer->m_flLayerFadeOuttime = pSrcLayer->m_flLayerFadeOuttime;
		}

		for (i = 0; i < m_hCopy->GetModelPtr()->GetNumPoseParameters(); i++)
		{
			float flMin, flMax;
			m_hCopy->GetPoseParameterRange(i, flMin, flMax);
			GetModel()->SetPoseParameter( i, RemapValClamped(m_hCopy->GetPoseParameter(i), 0, 1, flMin, flMax) );
		}

		for (i = 0; i < m_hCopy->GetNumBodyGroups(); i++)
			GetModel()->SetBodygroup(i, m_hCopy->GetBodygroup(i));
	}
	else
	{
		int iSequence = (m_iActivity>=0)?GetModel()->SelectWeightedSequence( m_iActivity ):0;

		GetModel()->m_nSkin = m_iSkin;
		GetModel()->SetSequence(iSequence);
	}

	// move player model in front of our view
	GetModel()->SetAbsOrigin( vecOffset );
	GetModel()->SetAbsAngles( angDirection );

	if (m_hCopy != NULL)
		GetModel()->FrameAdvance( gpGlobals->frametime );

	// Now draw it.
	CViewSetup view;

	// setup the views location, size and fov (amongst others)
	float flXScale = CFScreenWidthScale();
	float flYScale = CFScreenHeightScale();
	view.x = x*flXScale;
	view.y = y*flYScale;
	view.width = w*flXScale;
	view.height = h*flYScale;

	view.m_bOrtho = false;
	view.fov = 20;

	view.origin = Vector(0, 0, 0);

	// make sure that we see all of the player model
	Vector vMins, vMaxs;
	GetModel()->C_BaseAnimating::GetRenderBounds( vMins, vMaxs );
	view.origin.z += ( vMins.z + vMaxs.z )/2;

	view.angles.Init();
	view.zNear = VIEW_NEARZ;
	view.zFar = 1000;

	CMatRenderContextPtr pRenderContext( materials );

	if ( g_pMaterialSystemHardwareConfig->GetHDREnabled() )
	{
		pRenderContext->BindLocalCubemap( m_DefaultHDREnvCubemap );
	}
	else
	{
		pRenderContext->BindLocalCubemap( m_DefaultEnvCubemap );
	}

	pRenderContext->SetLightingOrigin( vec3_origin );
	pRenderContext->SetAmbientLight( 0.4, 0.4, 0.4 );

	static Vector white[6] = 
	{
		Vector( 0.4, 0.4, 0.4 ),
		Vector( 0.4, 0.4, 0.4 ),
		Vector( 0.4, 0.4, 0.4 ),
		Vector( 0.4, 0.4, 0.4 ),
		Vector( 0.4, 0.4, 0.4 ),
		Vector( 0.4, 0.4, 0.4 ),
	};

	g_pStudioRender->SetAmbientLightColors( white );
	g_pStudioRender->SetLocalLights( 0, NULL );

	// render it out to the new CViewSetup area
	// it's possible that ViewSetup3D will be replaced in future code releases
	Frustum menuFrustum;

	// New Function instead of ViewSetup3D...
	render->Push3DView( view, 0, NULL, menuFrustum );

	modelrender->SuppressEngineLighting( true );

	float color[3] = { 1.0f, 1.0f, 1.0f };
	render->SetColorModulation( color );
	render->SetBlend( 1.0f );

	GetModel()->DrawModel( STUDIO_RENDER );

	DrawOtherModels();

	modelrender->SuppressEngineLighting( false );

	render->PopView( menuFrustum );

	pRenderContext->BindLocalCubemap( NULL );
}

void CCFRenderable::DrawOtherModels()
{
	for (int i = 0; i < m_apParticles.Count(); i++)
	{
		m_apParticles[i]->SetVisible(true);
		m_apParticles[i]->DrawModel(STUDIO_TRANSPARENCY);
		m_apParticles[i]->SetVisible(false);
	}
}

// Called to see if we should be creating or recreating the model instances
bool CCFRenderable::ShouldRecreatePreviewModel( const char *pNewModelName )
{
	if ( !pNewModelName || !pNewModelName[0] )
		return true;

	if ( !GetModel() )
		return true;

	const model_t *pModel = GetModel()->GetModel();	// Ha.

	if ( !pModel )
		return true;

	const char *pName = modelinfo->GetModelName( pModel );

	if ( !pName )
		return true;

	// reload only if names are different
	return( Q_stricmp( pName, pNewModelName ) != 0 );
}

void CCFRenderable::RecreatePreviewModel()
{
	ITexture *pCubemapTexture;

	// Deal with the default cubemap
	if ( g_pMaterialSystemHardwareConfig->GetHDREnabled() )
	{
		pCubemapTexture = materials->FindTexture( "editor/cubemap.hdr", NULL, true );
		m_DefaultHDREnvCubemap.Init( pCubemapTexture );
	}
	else
	{
		pCubemapTexture = materials->FindTexture( "editor/cubemap", NULL, true );
		m_DefaultEnvCubemap.Init( pCubemapTexture );
	}

	// if the pointer already exists, remove it as we create a new one.
	if ( GetModel() )
	{
		GetModel()->Remove();
		SetModel(NULL);
	}

	if (!m_pszModelName || !m_pszModelName[0])
		return;

	// create a new instance
	C_BaseAnimatingOverlay *pModel = CreateModel();
	pModel->InitializeAsClientEntity( m_pszModelName, RENDER_GROUP_OPAQUE_ENTITY );
	pModel->UseClientSideAnimation();
	pModel->AddEffects( EF_NODRAW ); // don't let the renderer draw the model normally

	// have the player stand idle
	if (m_iActivity >= 0)
		pModel->SetSequence( pModel->SelectWeightedSequence( m_iActivity ) );

	m_apParticles.RemoveAll();

	SetModel(pModel);

	m_bDirty = false;
}

void CCFRenderable::AddParticles(CUtlVector<CNewParticleEffect*>& apNewParticles)
{
	for (int i = 0; i < apNewParticles.Count(); i++)
	{
		// We are showing it in fake world? Don't show it in real world.
		apNewParticles[i]->SetVisible(false);
		m_apParticles.AddToTail(apNewParticles[i]);
	}
}

void CCFRenderable::RemoveParticles(CUtlVector<CNewParticleEffect*>& apOldParticles)
{
	for (int i = 0; i < apOldParticles.Count(); i++)
	{
		// If it was created for the purpose of the fake world we probably don't want it ending up in the real world either.
		// So don't turn it back on when it gets removed.
		//apOldParticles[i]->SetVisible(true);
		m_apParticles.FindAndRemove(apOldParticles[i]);
	}
}

void CCFRenderablePlayer::SetPrimaryWeapon(CFWeaponID eWeapon)
{
	m_PrimaryWeapon.SetID(eWeapon);
}

void CCFRenderablePlayer::SetSecondaryWeapon(CFWeaponID eWeapon)
{
	m_SecondaryWeapon.SetID(eWeapon);
}

void CCFRenderablePlayer::RecreatePreviewModel()
{
	if (m_bDirty)
	{
		CCFRenderable::RecreatePreviewModel();

		if (GetModel())
		{
			GetModel()->SetPoseParameter( 0, -20.0f ); // head_yaw
			GetModel()->SetPoseParameter( 1, -10.0f ); // head_pitch
			GetModel()->SetPoseParameter( 2, -20.0f ); // body_yaw
			GetModel()->SetPoseParameter( 3, -10.0f ); // body_pitch
			GetModel()->SetPoseParameter( 4, 0.0f ); // move_y
			GetModel()->SetPoseParameter( 5, 0.0f ); // move_x
		}
	}

	if (m_PrimaryWeapon.IsDirty())
		m_PrimaryWeapon.RecreatePreviewModel();
	if (m_SecondaryWeapon.IsDirty())
		m_SecondaryWeapon.RecreatePreviewModel();

	if (m_PrimaryWeapon.GetModel())
	{
		m_PrimaryWeapon.SetHand(true, true);
		m_PrimaryWeapon.GetModel()->FollowEntity( GetModel() ); // attach to player model
	}

	if (m_SecondaryWeapon.GetModel())
	{
		m_SecondaryWeapon.SetHand(false, m_PrimaryWeapon.GetModel());
		m_SecondaryWeapon.GetModel()->FollowEntity( GetModel() ); // attach to player model
	}
}

void CCFRenderablePlayer::DrawOtherModels()
{
	if ( m_PrimaryWeapon.GetModel() )
	{
		m_PrimaryWeapon.GetModel()->DrawModel( STUDIO_RENDER );
	}

	if ( m_SecondaryWeapon.GetModel() )
	{
		m_SecondaryWeapon.GetModel()->DrawModel( STUDIO_RENDER );
	}

	CCFRenderable::DrawOtherModels();
}

bool CCFRenderablePlayer::IsDirty()
{
	return m_bDirty || m_PrimaryWeapon.IsDirty() || m_SecondaryWeapon.IsDirty();
}

CCFRenderableWeapon::CCFRenderableWeapon()
{
	SetID(WEAPON_NONE);
}

void CCFRenderableWeapon::SetID(CFWeaponID eWeapon)
{
	m_eWeapon = eWeapon;

	CCFWeaponInfo* pPrimWpnData = CArmament::WeaponIDToInfo(eWeapon);

	if (!pPrimWpnData)
	{
		SetModelName("");
		return;
	}

	SetModelName(pPrimWpnData->szWorldModel);

	m_bIsMelee = (pPrimWpnData->m_eWeaponType == WEAPONTYPE_MELEE);
}

void CCFRenderableWeapon::SetHand(bool bIsPrimary, bool bHasPrimary)
{
	if (m_hWeaponModel != NULL)
	{
		if (bIsPrimary)
			m_hWeaponModel->SetHand(!m_bIsMelee);
		else
			m_hWeaponModel->SetHand(m_bIsMelee || !bHasPrimary);
	}
}

void CCFRenderableWeapon::SetModel(C_BaseAnimatingOverlay* pModel)
{
	// if pModel exist it needs to be a CPreviewWeapon
	Assert(!pModel || dynamic_cast<CPreviewWeapon*>(pModel));

	m_hWeaponModel = dynamic_cast<CPreviewWeapon*>(pModel);
}

char* CPreviewWeapon::GetBoneMergeName(int iBone)
{
	if (m_bRightHand)
		return "ValveBiped.Weapon_R_Hand";
	else
		return "ValveBiped.Weapon_L_Hand";
}

