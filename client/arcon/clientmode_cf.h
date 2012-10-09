//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef CF_CLIENTMODE_H
#define CF_CLIENTMODE_H
#ifdef _WIN32
#pragma once
#endif

#include "clientmode_shared.h"
#include "cfviewport.h"

class ClientModeCFNormal : public ClientModeShared 
{
DECLARE_CLASS( ClientModeCFNormal, ClientModeShared );

private:

// IClientMode overrides.
public:

					ClientModeCFNormal();
	virtual			~ClientModeCFNormal();

	virtual void	OverrideView( CViewSetup *pSetup );

	virtual void	Init();
	virtual void	InitViewport();

	virtual void	FireGameEvent( IGameEvent *event );

	virtual float	GetViewModelFOV( void );

	int				GetDeathMessageStartHeight( void );

	virtual void	PostRenderVGui();

	virtual int		HandleSpectatorKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

private:
	
	//	void	UpdateSpectatorMode( void );

};


extern IClientMode *GetClientModeNormal();
extern ClientModeCFNormal* GetClientModeCFNormal();


#endif // CF_CLIENTMODE_H
