//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "../EventLog.h"
#include "KeyValues.h"

class CCFEventLog : public CEventLog
{
private:
	typedef CEventLog BaseClass;

public:
	virtual ~CCFEventLog() {};

public:
	bool PrintEvent( IGameEvent * event )	// override virtual function
	{
		if ( BaseClass::PrintEvent( event ) )
		{
			return true;
		}
	
		if ( Q_strcmp(event->GetName(), "cf_") == 0 )
		{
			return PrintCFEvent( event );
		}

		return false;
	}

protected:

	bool PrintCFEvent( IGameEvent * event )	// print Mod specific logs
	{
		//const char * name = event->GetName() + Q_strlen("cf_"); // remove prefix
		return false;
	}

};

CCFEventLog g_CFEventLog;

//-----------------------------------------------------------------------------
// Singleton access
//-----------------------------------------------------------------------------
IGameSystem* GameLogSystem()
{
	return &g_CFEventLog;
}

