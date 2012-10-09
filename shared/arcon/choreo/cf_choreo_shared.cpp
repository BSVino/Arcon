#include "cbase.h"

#ifdef CLIENT_DLL
#include "choreo/c_cf_choreo.h"
#else
#include "choreo/cf_choreo.h"
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( CFActorWeapon, DT_CFActorWeapon )

BEGIN_NETWORK_TABLE( CCFActorWeapon, DT_CFActorWeapon )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO(m_ePosition)),
#else
	SendPropInt( SENDINFO(m_ePosition), 8, SPROP_UNSIGNED ),
#endif
END_NETWORK_TABLE()

char* CCFActorWeapon::GetBoneMergeName(int iBone)
{
	switch (m_ePosition)
	{
	default:
		return BaseClass::GetBoneMergeName(iBone);
	case WP_RIGHTHAND:
		return "ValveBiped.Weapon_R_Hand";
	case WP_LEFTHAND:
		return "ValveBiped.Weapon_L_Hand";
	case WP_RIGHTLEG:
		return "ValveBiped.Weapon_R_Leg";
	case WP_LEFTLEG:
		return "ValveBiped.Weapon_L_Leg";
	case WP_RIGHTBACK:
		return "ValveBiped.Weapon_R_Backside";
	case WP_LEFTBACK:
		return "ValveBiped.Weapon_L_Backside";
	}
}
