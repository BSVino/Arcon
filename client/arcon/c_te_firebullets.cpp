//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "c_cf_player.h"
#include "c_basetempentity.h"
#include <cliententitylist.h>


class C_TEFireBullets : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEFireBullets, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:
	int			m_iPlayer;
	Vector		m_vecOrigin;
	QAngle		m_vecAngles;
	CFWeaponID	m_iWeaponID;
	int			m_iMode;
	int			m_iSeed;
	float		m_flSpread;
};


void C_TEFireBullets::PostDataUpdate( DataUpdateType_t updateType )
{
	// Create the effect.
	
	m_vecAngles.z = 0;
	
	C_BaseEntity *pEnt = ClientEntityList().GetBaseEntity( m_iPlayer+1 );

	if (!pEnt)
		return;

	C_CFPlayer *pPlayer = ToCFPlayer( pEnt );

	if (m_iMode == Primary_Mode)
		pPlayer->FireBullets( 
			m_vecOrigin,
			m_vecAngles,
			m_iWeaponID,
			m_iSeed,
			m_flSpread );
	else
		pPlayer->FireDrainer( 
			m_vecOrigin,
			m_vecAngles,
			m_iWeaponID,
			m_iSeed,
			m_flSpread );
}


IMPLEMENT_CLIENTCLASS_EVENT( C_TEFireBullets, DT_TEFireBullets, CTEFireBullets );


BEGIN_RECV_TABLE_NOBASE(C_TEFireBullets, DT_TEFireBullets)
	RecvPropVector( RECVINFO( m_vecOrigin ) ),
	RecvPropFloat( RECVINFO( m_vecAngles[0] ) ),
	RecvPropFloat( RECVINFO( m_vecAngles[1] ) ),
	RecvPropInt( RECVINFO( m_iWeaponID ) ),
	RecvPropInt( RECVINFO( m_iMode ) ), 
	RecvPropInt( RECVINFO( m_iSeed ) ),
	RecvPropInt( RECVINFO( m_iPlayer ) ),
	RecvPropFloat( RECVINFO( m_flSpread ) ),
END_RECV_TABLE()