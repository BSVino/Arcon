#if !defined( CF_IN_MAIN_H )
#define CF_IN_MAIN_H

#include "input.h"

class CCFInput : public CInput
{
public:
						CCFInput( void );

	virtual void		MouseMove ( CUserCmd *cmd );
	virtual int			KeyEvent( int down, ButtonCode_t code, const char *pszCurrentBinding );

	virtual void		CAM_Think();
	virtual void		CAM_ThirdPersonThink(Vector& vecCamera, QAngle& angCamera);
	virtual void		CAM_ThirdPersonNormalThink(Vector& vecCamera, QAngle& angCamera);
	virtual void		CAM_ThirdPersonGripThink(Vector& vecCamera, QAngle& angCamera);
	virtual void		CAM_FollowModeThink(Vector& vecCamera, QAngle& angCamera);
	virtual void		CAM_ToThirdPerson();
	virtual void		CAM_ToFirstPerson();
	virtual void		CAM_SetUpCamera( Vector& vecOffset, QAngle& angCamera );
	virtual void		CAM_ResetFollowModeVars();
	virtual	int			CAM_IsThirdPerson( void );

	static CCFInput		s_pInput;

protected:
	QAngle				m_angCamera;
	QAngle				m_angTargetCurr;
	Vector				m_vecCamera;
	Vector				m_vecTargetGoal;
	Vector				m_vecTarget;

	bool				m_bWasInFollowMode;
	float				m_flCameraWeight;

	float				m_flCameraGripWeight;

	bool				m_bThirdPositionMelee;
	float				m_flThirdPositionMeleeWeight;
	bool				m_bThirdPositionRight;
	float				m_flThirdPositionRightWeight;
};

inline CCFInput* CFInput()
{
	return &CCFInput::s_pInput;
}

#endif