#include "cbase.h"
#include "input.h"
#include "clientmode_cf.h"
#include "c_cf_player.h"
#include "prediction.h"
#include "ivieweffects.h"
#include "view_shared.h"
#include "cf_in_main.h"
#include "tier0/vprof.h"
#include "engine/ivdebugoverlay.h"
#include "cf_gamerules.h"

static ConVar cam_fm_back( "cam_fm_back", "68", 0 );
static ConVar cam_fm_up( "cam_fm_up", "-20", 0 );
static ConVar cam_fm_right( "cam_fm_right", "-36", 0 );

static ConVar cam_back( "cam_back", "40", 0 );
static ConVar cam_right( "cam_right", "-30", 0 );

static ConVar cam_back_melee( "cam_back_melee", "80", 0 );
static ConVar cam_right_melee( "cam_right_melee", "-20", 0 );

static ConVar cam_goal_time( "cam_goal_time", "0.4", 0 );

static ConVar cam_show_positions( "cam_show_positions", "0", 0 );

#define CAM_HULL_OFFSET		9.0    // the size of the bounding hull used for collision checking
static Vector g_vecCamHullMin(-CAM_HULL_OFFSET,-CAM_HULL_OFFSET,-CAM_HULL_OFFSET);
static Vector g_vecCamHullMax( CAM_HULL_OFFSET, CAM_HULL_OFFSET, CAM_HULL_OFFSET);

static ConVar cam_switchtime( "cam_switchtime", ".2", FCVAR_USERINFO );

CCFInput::CCFInput( )
{
	m_bWasInFollowMode = false;
	m_flCameraWeight = 0;
	m_bThirdPositionRight = true;
	m_flThirdPositionMeleeWeight = 0;
	m_flThirdPositionRightWeight = 0;
}

void CCFInput::CAM_SetUpCamera( Vector& vecOffset, QAngle& angCamera )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	if (pPlayer->IsObserver())
		return;

	VectorCopy( m_vecCamera, vecOffset );
	VectorCopy( m_angCamera, angCamera );
}

void ClientModeCFNormal::OverrideView( CViewSetup *pSetup )
{
	QAngle camAngles;

	// Let the player override the view.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if(!pPlayer)
		return;

	pPlayer->OverrideView( pSetup );

	if( CFInput()->CAM_IsThirdPerson() )
	{
		CFInput()->CAM_SetUpCamera( pSetup->origin, pSetup->angles );
	}
	else if (::input->CAM_IsOrthographic())
	{
		pSetup->m_bOrtho = true;
		float w, h;
		::input->CAM_OrthographicSize( w, h );
		w *= 0.5f;
		h *= 0.5f;
		pSetup->m_OrthoLeft   = -w;
		pSetup->m_OrthoTop    = -h;
		pSetup->m_OrthoRight  = w;
		pSetup->m_OrthoBottom = h;
	}
}

void CCFInput::CAM_Think( void )
{
	VPROF("CCFInput::CAM_Think");

	if(!C_BasePlayer::GetLocalPlayer())
		return;

	C_CFPlayer* pPlayer = C_CFPlayer::GetLocalCFPlayer();

	if (!pPlayer)
		return;

	if ( !CAM_IsThirdPerson() )
		return;

	Vector vecFollowCamera;
	QAngle angFollowCamera;
	Vector vecThirdCamera;
	QAngle angThirdCamera;

	// Always give this thing a chance to do logic.
	Vector vecTarget = pPlayer->CalcFollowModeCameraTargets();

	m_flCameraWeight = Approach(pPlayer->IsInFollowMode()?1:0, m_flCameraWeight, gpGlobals->frametime * (1/cam_switchtime.GetFloat()));

	if (m_flCameraWeight > 0.0f)
	{
		//if (pPlayer->IsInFollowMode() && !m_bWasInFollowMode)
		//	CAM_ResetFollowModeVars();

		m_bWasInFollowMode = pPlayer->IsInFollowMode();

		CAM_FollowModeThink(vecFollowCamera, angFollowCamera);
	}

	if (m_flCameraWeight < 1.0f)
	{
		CAM_ThirdPersonThink(vecThirdCamera, angThirdCamera);
	}

	float flWeight = Gain(m_flCameraWeight, 0.8f);

	m_vecCamera = vecFollowCamera * flWeight + vecThirdCamera * (1-flWeight);

	float flAngleDiff;

	// This complicated math is used to avoid a situation where (-170 + 170) = 0 causing camera freakouts.
	flAngleDiff = AngleDiff(angThirdCamera.x, angFollowCamera.x);
	m_angCamera.x = angFollowCamera.x * flWeight + (angFollowCamera.x + flAngleDiff) * (1-flWeight);
	flAngleDiff = AngleDiff(angThirdCamera.y, angFollowCamera.y);
	m_angCamera.y = angFollowCamera.y * flWeight + (angFollowCamera.y + flAngleDiff) * (1-flWeight);

	if ( cvar->FindVar("cam_showangles")->GetInt() )
	{
		engine->Con_NPrintf( 6, "X: %6.1f Y: %6.1f Z: %6.1f %38s", m_vecCamera.x, m_vecCamera.y, m_vecCamera.z, "Combined camera offset" );
		engine->Con_NPrintf( 8, "   Pitch: %6.1f Yaw: %6.1f %38s", m_angCamera.x, m_angCamera.y, "Combined camera angles" );

		engine->Con_NPrintf( 12, "X: %6.1f Y: %6.1f Z: %6.1f %38s", vecThirdCamera.x, vecThirdCamera.y, vecThirdCamera.z, "Normal camera offset" );
		engine->Con_NPrintf( 14, "   Pitch: %6.1f Yaw: %6.1f %38s", angThirdCamera.x, angThirdCamera.y, "Normal camera angles" );
		engine->Con_NPrintf( 16, "X: %6.1f Y: %6.1f Z: %6.1f %38s", vecFollowCamera.x, vecFollowCamera.y, vecFollowCamera.z, "Follow camera offset" );
		engine->Con_NPrintf( 18, "   Pitch: %6.1f Yaw: %6.1f %38s", angFollowCamera.x, angFollowCamera.y, "Follow camera angles" );
	}
}

void CCFInput::CAM_ThirdPersonThink( Vector& vecCamera, QAngle& angCamera )
{
	C_CFPlayer* pPlayer = C_CFPlayer::GetLocalCFPlayer();

	Vector vecNormalCamera;
	QAngle angNormalCamera;
	Vector vecGripCamera;
	QAngle angGripCamera;

	m_flCameraGripWeight = Approach((pPlayer->m_bLatched && pPlayer->GetAbsVelocity().LengthSqr() < 0.1f)?1:0, m_flCameraGripWeight, gpGlobals->frametime * (1/cam_switchtime.GetFloat()));

	if (m_flCameraGripWeight > 0.0f)
		CAM_ThirdPersonGripThink(vecGripCamera, angGripCamera);

	if (m_flCameraGripWeight < 1.0f)
		CAM_ThirdPersonNormalThink(vecNormalCamera, angNormalCamera);

	float flWeight = Gain(m_flCameraGripWeight, 0.8f);

	vecCamera = vecGripCamera * flWeight + vecNormalCamera * (1-flWeight);
	angCamera = angGripCamera * flWeight + angNormalCamera * (1-flWeight);

	if ( cam_show_positions.GetInt() )
	{
		// blue is me
		debugoverlay->AddBoxOverlay( pPlayer->GetAbsOrigin(), Vector(-2,-2,-2), Vector(2,2,2), QAngle( 0, 0, 0), fmod(gpGlobals->curtime, 1)*255,0,255,127, 20 );
		// green is the camera
		debugoverlay->AddBoxOverlay( vecCamera, Vector(-2,-2,-2), Vector(2,2,2), QAngle( 0, 0, 0), 0,255,fmod(gpGlobals->curtime, 1)*255,127, 20 );

		static float flLastTime = 0;
		static Vector vecLastCamera = Vector(0, 0, 0);
		DevMsg("%f - %f %f %f - %f\n", gpGlobals->curtime, vecCamera.x, vecCamera.y, vecCamera.z, (vecCamera-vecLastCamera).Length()/(gpGlobals->curtime-flLastTime));
		flLastTime = gpGlobals->curtime;
		vecLastCamera = vecCamera;
	}
}

void CCFInput::CAM_ThirdPersonNormalThink( Vector& vecCamera, QAngle& angCamera )
{
	C_CFPlayer* pPlayer = C_CFPlayer::GetLocalCFPlayer();

	QAngle angEngine;
	engine->GetViewAngles( angEngine );

	Vector vecEngineForward, vecEngineRight, vecEngineUp;
	AngleVectors(angEngine, &vecEngineForward, &vecEngineRight, &vecEngineUp);

	Vector vecEngineForward2D;
	AngleVectors(QAngle(0, angEngine[YAW], 0), &vecEngineForward2D);

	// Don't use EyePosition() because that changes depending on the animation.
	Vector vecCameraCenter = pPlayer->GetAbsOrigin() + VEC_VIEW;

	m_bThirdPositionMelee = true;
	if (pPlayer->GetPrimaryWeapon() && !pPlayer->GetPrimaryWeapon()->IsMeleeWeapon())
		m_bThirdPositionMelee = false;
	if (pPlayer->GetSecondaryWeapon() && !pPlayer->GetSecondaryWeapon()->IsMeleeWeapon())
		m_bThirdPositionMelee = false;

	Vector vecRearRightPosition = vecCameraCenter + vecEngineRight * cam_right.GetFloat() - vecEngineForward * cam_back.GetFloat();
	Vector vecRearMeleePosition = vecCameraCenter + vecEngineRight * cam_right_melee.GetFloat() - vecEngineForward * cam_back_melee.GetFloat();

	m_flThirdPositionMeleeWeight = Approach(m_bThirdPositionMelee?0:1, m_flThirdPositionMeleeWeight, gpGlobals->frametime * (1/cam_switchtime.GetFloat()));

	// If we're in normal view here, force right view for grip so it doesn't go over to the left we we start gripping again.
	if (!m_bThirdPositionMelee)
		m_bThirdPositionRight = true;

	float flWeight = Gain(m_flThirdPositionMeleeWeight, 0.8f);

	vecCamera = vecRearMeleePosition * (1-flWeight) + vecRearRightPosition * flWeight;

	if ( pPlayer )
	{
		trace_t trace;

		// Trace back to see if the camera is in a wall.
		CTraceFilterNoNPCsOrPlayer traceFilter( pPlayer, COLLISION_GROUP_NONE );
		UTIL_TraceHull( vecCameraCenter, vecCamera,
			g_vecCamHullMin, g_vecCamHullMax,
			MASK_SOLID, &traceFilter, &trace );

		if( trace.fraction < 1.0 )
			vecCamera = trace.endpos;
	}

	angCamera = angEngine;
}

void CCFInput::CAM_ThirdPersonGripThink( Vector& vecCamera, QAngle& angCamera )
{
	C_CFPlayer* pPlayer = C_CFPlayer::GetLocalCFPlayer();

	QAngle angEngine;
	engine->GetViewAngles( angEngine );

	Vector vecEngineForward, vecEngineRight, vecEngineUp;
	AngleVectors(angEngine, &vecEngineForward, &vecEngineRight, &vecEngineUp);

	Vector vecEngineForward2D;
	AngleVectors(QAngle(0, angEngine[YAW], 0), &vecEngineForward2D);

	// Don't use EyePosition() because that changes depending on the animation.
	Vector vecCameraCenter = pPlayer->GetAbsOrigin() + VEC_VIEW;

	Vector vecRearRightPosition = vecCameraCenter + vecEngineRight * cam_right.GetFloat() - vecEngineForward * cam_back.GetFloat();
	Vector vecRearLeftPosition = vecCameraCenter + vecEngineRight * -cam_right.GetFloat() - vecEngineForward * cam_back.GetFloat();

	bool bUseRightCamera = m_bThirdPositionRight;
	
	// If we're looking to the right while latching, move the camera off to the left so we can get a better view.
	if (DotProduct(vecEngineForward, pPlayer->m_vecLatchPlaneNormal) > -0.4 && DotProduct(vecEngineRight, pPlayer->m_vecLatchPlaneNormal) < 0)
		bUseRightCamera = false;

	// If we're on one side and our side is blocked but the other side is clear, scoot on over there.
	if (m_bThirdPositionRight && enginetrace->GetPointContents( vecRearLeftPosition ) != CONTENTS_SOLID)
	{
		if (enginetrace->GetPointContents( vecRearRightPosition ) == CONTENTS_SOLID)
			bUseRightCamera = false;
		else
		{
			trace_t trace;

			// Trace back to see if the camera is in a wall.
			CTraceFilterNoNPCsOrPlayer traceFilter( pPlayer, COLLISION_GROUP_NONE );
			UTIL_TraceHull( vecCameraCenter, vecRearRightPosition,
				g_vecCamHullMin, g_vecCamHullMax,
				MASK_SOLID, &traceFilter, &trace );

			if( trace.fraction < 1.0 )
				bUseRightCamera = false;
		}
	}

	if (!m_bThirdPositionRight && enginetrace->GetPointContents( vecRearRightPosition ) != CONTENTS_SOLID)
	{
		if (enginetrace->GetPointContents( vecRearLeftPosition ) == CONTENTS_SOLID)
			bUseRightCamera = true;
		else
		{
			trace_t trace;

			// Trace back to see if the camera is in a wall.
			CTraceFilterNoNPCsOrPlayer traceFilter( pPlayer, COLLISION_GROUP_NONE );
			UTIL_TraceHull( vecCameraCenter, vecRearLeftPosition,
				g_vecCamHullMin, g_vecCamHullMax,
				MASK_SOLID, &traceFilter, &trace );

			if( trace.fraction < 1.0 )
				bUseRightCamera = true;
		}
	}

	m_bThirdPositionRight = bUseRightCamera;

	m_flThirdPositionRightWeight = Approach(bUseRightCamera?0:1, m_flThirdPositionRightWeight, gpGlobals->frametime * (1/cam_switchtime.GetFloat()));

	float flWeight = Gain(m_flThirdPositionRightWeight, 0.8f);

	vecCamera = vecRearRightPosition * (1-flWeight) + vecRearLeftPosition * flWeight;

	if ( pPlayer )
	{
		trace_t trace;

		// Trace back to see if the camera is in a wall.
		CTraceFilterNoNPCsOrPlayer traceFilter( pPlayer, COLLISION_GROUP_NONE );
		UTIL_TraceHull( vecCameraCenter, vecCamera,
			g_vecCamHullMin, g_vecCamHullMax,
			MASK_SOLID, &traceFilter, &trace );

		if( trace.fraction < 1.0 )
			vecCamera = trace.endpos;
	}

	angCamera = angEngine;
}

void CCFInput::CAM_FollowModeThink( Vector& vecCamera, QAngle& angCamera )
{
	C_CFPlayer* pPlayer = C_CFPlayer::GetLocalCFPlayer();

	Vector vecView = pPlayer->GetAbsOrigin() + VEC_VIEW;
	Vector vecTarget;

	if (pPlayer->ShouldLockFollowModeView())
	{
		vecTarget = pPlayer->CalcFollowModeCameraTargets();
		VectorAngles(vecTarget - vecView, angCamera);
	}
	else
	{
		engine->GetViewAngles(angCamera);
		if (pPlayer->GetRecursedTarget() && pPlayer->ShouldLockFollowModeView())
			vecTarget = pPlayer->GetRecursedTarget()->WorldSpaceCenter() + pPlayer->GetRecursedTarget()->GetTargetLockingOffset();
		else
			vecTarget = pPlayer->m_vecLastTargetPosition;
	}

	Vector vecCameraForward, vecCameraRight;
	AngleVectors(angCamera, &vecCameraForward, &vecCameraRight, NULL);

	// Don't use EyePosition() because that changes depending on the animation.
	vecCamera = vecView + Vector(0, 0, cam_fm_up.GetFloat()) - vecCameraForward*cam_fm_back.GetFloat() + vecCameraRight*cam_fm_right.GetFloat();

	bool bUseAngleOffset = false;
	if (pPlayer->ShouldLockFollowModeView())
		bUseAngleOffset = true;
	// Maybe we just lost our target.
	else if (pPlayer->m_flLastCameraTargetTime && gpGlobals->curtime < pPlayer->m_flLastCameraTargetTime + cam_goal_time.GetFloat())
		bUseAngleOffset = true;
	// Maybe we just left follow mode.
	else if (m_flCameraWeight < 1 && m_flCameraWeight > 0)
		bUseAngleOffset = true;

	if (bUseAngleOffset)
	{
		QAngle angToTarget;
		VectorAngles(vecTarget - vecView, angToTarget);

		Vector vecTargetForward, vecTargetRight;
		AngleVectors(angToTarget, &vecTargetForward, &vecTargetRight, NULL);

		// If we do this while lerping out of being locked, the player may be moving the camera around.
		// *Sigh* So where WAS the camera when we started lerping?
		Vector vecTargetCamera = vecView + Vector(0, 0, cam_fm_up.GetFloat()) - vecTargetForward*cam_fm_back.GetFloat() + vecTargetRight*cam_fm_right.GetFloat();

		Vector vecCameraToTarget = vecTarget - vecTargetCamera;
		QAngle angCameraToTarget;
		VectorAngles(vecCameraToTarget, angCameraToTarget);

		QAngle angTargetOffset = angCameraToTarget - angToTarget;

		NormalizeAngles(angTargetOffset);

		angTargetOffset *= m_flCameraWeight;

		float flCameraTargetMultiplier = 1;
		if (!pPlayer->m_bCameraTargetSwitchBetweenTwoPlayers && pPlayer->m_flLastCameraTargetTime &&
			gpGlobals->curtime < pPlayer->m_flLastCameraTargetTime + cam_goal_time.GetFloat())
		{
			flCameraTargetMultiplier = (gpGlobals->curtime - pPlayer->m_flLastCameraTargetTime) / cam_goal_time.GetFloat();

			// If we lost targets or are exiting follow mode, reverse it.
			if (!pPlayer->GetRecursedTarget() || (pPlayer->GetRecursedTarget() && !pPlayer->ShouldLockFollowModeView()) || !pPlayer->IsInFollowMode())
				flCameraTargetMultiplier = 1-flCameraTargetMultiplier;
		}

		angTargetOffset *= flCameraTargetMultiplier;

		angCamera += angTargetOffset;
	}

	NormalizeAngles(angCamera);

	if ( pPlayer )
	{
		trace_t trace;

		// Trace back to see if the camera is in a wall.
		CTraceFilterNoNPCsOrPlayer traceFilter( pPlayer, COLLISION_GROUP_NONE );
		UTIL_TraceHull( pPlayer->EyePosition(), vecCamera,
			g_vecCamHullMin, g_vecCamHullMax,
			MASK_SOLID, &traceFilter, &trace );

		if( trace.fraction < 1.0 )
			vecCamera = trace.endpos;
	}

	if ( cam_show_positions.GetInt() )
	{
		// blue is me
		debugoverlay->AddBoxOverlay( pPlayer->GetAbsOrigin(), Vector(-2,-2,-2), Vector(2,2,2), QAngle( 0, 0, 0), fmod(gpGlobals->curtime, 1)*255,0,255,127, 20 );
		// green is the camera
		debugoverlay->AddBoxOverlay( vecCamera, Vector(-2,-2,-2), Vector(2,2,2), QAngle( 0, 0, 0), 0,255,fmod(gpGlobals->curtime, 1)*255,127, 20 );

		static float flLastTime = 0;
		static Vector vecLastCamera = Vector(0, 0, 0);
		DevMsg("%f - %f %f %f - %f\n", gpGlobals->curtime, vecCamera.x, vecCamera.y, vecCamera.z, (vecCamera-vecLastCamera).Length()/(gpGlobals->curtime-flLastTime));
		flLastTime = gpGlobals->curtime;
		vecLastCamera = vecCamera;
	}
}

void CCFInput::CAM_ToThirdPerson(void)
{ 
	if (!C_BasePlayer::GetLocalPlayer())
		return;

	C_CFPlayer* pPlayer = C_CFPlayer::GetLocalCFPlayer();

	if( !m_fCameraInThirdPerson && pPlayer && (pPlayer->IsAlive() || pPlayer->IsKnockedOut()))
	{
		m_fCameraInThirdPerson = true;

		CAM_ResetFollowModeVars();
	}
}

void CCFInput::CAM_ResetFollowModeVars(void)
{
	C_CFPlayer* pPlayer = C_CFPlayer::GetLocalCFPlayer();

	QAngle viewangles;
	engine->GetViewAngles( viewangles );

	m_angCamera = viewangles;
	m_angCamera.z = 0;
	m_angTargetCurr = viewangles;

	if (pPlayer->GetRecursedTarget())
		m_vecTargetGoal = m_vecTarget = pPlayer->GetRecursedTarget()->GetAbsOrigin();
}

void CCFInput::CAM_ToFirstPerson(void)
{
	if (C_BasePlayer::GetLocalPlayer())
	{
		C_CFPlayer* pPlayer = C_CFPlayer::GetLocalCFPlayer();
		if (pPlayer->IsKnockedOut())
			return;
	}

	CInput::CAM_ToFirstPerson();
}

// Lots of code here to make sure all transitions are smooth. Lots of cases:
// * Being in follow mode with no target.
// * Being in follow mode with an enemy as your target.
// * Starting follow mode while pointing at an enemy.
// * Switching from one enemy to another.
// * Stopping follow mode while pointing at an enemy.
// * Losing a lock on an enemy.
// * Gaining a lock on an enemy.
// * Being in follow mode with a friend as your target.
// * Switching from a friend to an enemy.
// * Switching from an enemy to a friend.
// Make sure they all work!

Vector C_CFPlayer::CalcFollowModeCameraTargets()
{
	if (m_hLastCameraTarget != GetRecursedTarget())
	{
		m_bCameraTargetSwitchBetweenTwoPlayers = GetRecursedTarget() && m_hLastCameraTarget != NULL &&
			CFGameRules()->PlayerRelationship(GetRecursedTarget(), this) == GR_NOTTEAMMATE &&
			CFGameRules()->PlayerRelationship(m_hLastCameraTarget, this) == GR_NOTTEAMMATE;
		m_vecLastCameraTarget = m_vecLastTargetPosition;
		m_hLastCameraTarget = GetRecursedTarget();
		m_flLastCameraTargetTime = gpGlobals->curtime;
	}

	if (gpGlobals->curtime > m_flLastCameraTargetTime + cam_goal_time.GetFloat())
		m_flLastCameraTargetTime = 0;

	Vector vecTarget;
	if (GetRecursedTarget() && ShouldLockFollowModeView()
		&& m_flLastCameraTargetTime != 0 && gpGlobals->curtime - m_flLastCameraTargetTime < cam_goal_time.GetFloat())
	{
		Vector vecNextTarget = GetRecursedTarget()->WorldSpaceCenter() + GetRecursedTarget()->GetTargetLockingOffset();
		Vector vecLastTarget = m_vecLastCameraTarget;

		float flWeight = Gain((gpGlobals->curtime - m_flLastCameraTargetTime) / cam_goal_time.GetFloat(), 0.8);
		vecTarget = vecNextTarget * flWeight + vecLastTarget * (1-flWeight);
	}
	else if (GetRecursedTarget() && ShouldLockFollowModeView())
		vecTarget = GetRecursedTarget()->WorldSpaceCenter() + GetRecursedTarget()->GetTargetLockingOffset();
	else
		vecTarget = m_vecLastCameraTarget;

	m_vecLastTargetPosition = vecTarget;

	return vecTarget;
}

void C_CFPlayer::CalcFollowModeView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov )
{
	Vector vecTarget = CalcFollowModeCameraTargets();

	VectorAngles(vecTarget - EyePosition(), eyeAngles);
	VectorCopy( EyePosition(), eyeOrigin );

	// Save the major angle changes.
	SetLocalViewAngles( eyeAngles );
	engine->SetViewAngles( eyeAngles );

	if ( !prediction->InPrediction() )
	{
		SmoothViewOnStairs( eyeOrigin );
	}

	// Snack off the origin before bob + water offset are applied
	Vector vecBaseEyePosition = eyeOrigin;

	CalcViewRoll( eyeAngles );

	// Apply punch angle
	VectorAdd( eyeAngles, m_Local.m_vecPunchAngle, eyeAngles );

	if ( !prediction->InPrediction() )
	{
		// Shake it up baby!
		vieweffects->CalcShake();
		vieweffects->ApplyShake( eyeOrigin, eyeAngles, 1.0 );
	}

	// Apply a smoothing offset to smooth out prediction errors.
	Vector vSmoothOffset;
	GetPredictionErrorSmoothingVector( vSmoothOffset );
	eyeOrigin += vSmoothOffset;
	m_flObserverChaseDistance = 0.0;

	// calc current FOV
	fov = GetFOV();
}

int CCFInput::CAM_IsThirdPerson()
{
	C_CFPlayer* pPlayer = C_CFPlayer::GetLocalCFPlayer();

	if (pPlayer->ShouldForceThirdPerson())
		return true;

	if (pPlayer->m_hCameraCinematic != NULL)
		return false;

	return m_fCameraInThirdPerson;
}
