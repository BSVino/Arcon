#ifndef C_OBJECTIVES_H
#define C_OBJECTIVES_H

#include "c_cf_player.h"

#define CInfoObjective C_InfoObjective

class C_InfoObjective : public C_BaseFlex
{
	DECLARE_CLASS( C_InfoObjective, C_BaseFlex );
	DECLARE_CLIENTCLASS();
public:
	virtual void	Precache();
	virtual void	Spawn();
	virtual void	OnDataChanged(DataUpdateType_t updateType);
	virtual void	ClientThink();

	Vector	GetHoldingPosition(CCFPlayer* pHolder);

	virtual CStudioHdr* OnNewModel( void );

	virtual const Vector&	GetAbsOrigin( void ) const;
	virtual const Vector&	GetRenderOrigin( void );
	virtual int		DrawModel( int flags );

	virtual bool	IsAtSpawn() { return m_bAtSpawn; };
	virtual C_CFPlayer* GetPlayer() { return m_hPlayer; };

	bool			m_bAtSpawn;

	CHandle<C_CFPlayer>	m_hPlayer;

	Vector			m_vecRenderOrigin;

protected:
	bool			m_bSoundPlaying;
};

#endif