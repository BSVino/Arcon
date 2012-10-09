//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Draws the death notices
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "hud_objectives.h"
#include "cfui_menu.h"
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include "cf_gamerules.h"
#include "c_cf_objective_resource.h"
#include "cfui_scoreboard.h"
#include "c_cf_team.h"
#include "view.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CCFHudTexture* CObjectives::s_pNumeniFlag = NULL;
CCFHudTexture* CObjectives::s_pMachindoFlag = NULL;
CCFHudTexture* CObjectives::s_pFlagPole = NULL;
CCFHudTexture* CObjectives::s_pMShard = NULL;
CCFHudTexture* CObjectives::s_pNShard = NULL;
CCFHudTexture* CObjectives::s_pNeutralShard = NULL;
CCFHudTexture* CObjectives::s_pShardDropped = NULL;
CCFHudTexture* CObjectives::s_pShardCarriedMachindo = NULL;
CCFHudTexture* CObjectives::s_pShardCarriedNumeni = NULL;

CRoundVictoryPanel*	CRoundVictoryPanel::s_pVictoryPanel = NULL;

extern ConVar cf_ctf_tugofwar;

CRoundStatusPanel::CRoundStatusPanel()
	: CPanel(0, 0, 100, 100)
{
	SetBorder(BT_NONE);

	m_pTimeleft = new CLabel(0, 0, BTN_WIDTH, BTN_HEIGHT, "");
	AddControl(m_pTimeleft);

	m_pStatus = new CLabel(0, 0, BTN_WIDTH, BTN_HEIGHT, "");
	AddControl(m_pStatus);
}

void CRoundStatusPanel::Layout()
{
	SetSize(CFScreenWidth()/8, BTN_HEIGHT*2);
	SetPos(CFScreenWidth()/2 - GetWidth()/2, 0);

	m_pTimeleft->SetPos(0, 0);
	m_pTimeleft->SetSize(GetWidth(), GetHeight()/2);
	m_pTimeleft->SetAlign(CLabel::TA_CENTER);
	m_pStatus->SetPos(0, GetHeight()/2);
	m_pStatus->SetSize(GetWidth(), GetHeight()/2);
	m_pStatus->SetAlign(CLabel::TA_CENTER);

	CPanel::Layout();
}

void CRoundStatusPanel::Think()
{
	if (!CFGameRules())
		return;

	if (CFGameRules()->IsInWaitingForPlayers())
		m_pStatus->SetText("#Waiting_for_players");
	else if (CFGameRules()->InRoundRestart())
		m_pStatus->SetText("#Get_ready");
	else if (CFGameRules()->InOvertime())
		m_pStatus->SetText("#Overtime");
	else
		m_pStatus->SetText("");

	if (!ObjectiveResource())
		return;

	CTeamRoundTimer *pTimer = dynamic_cast< CTeamRoundTimer* >( ClientEntityList().GetEnt( ObjectiveResource()->GetTimerToShowInHUD() ) );

	if (pTimer && !pTimer->IsDormant())
	{
		float flTimeRemaining = pTimer->GetTimeRemaining();
		int iMinutes, iSeconds;
		iMinutes = flTimeRemaining / 60;
		iSeconds = fmod(flTimeRemaining, 60);
		m_pTimeleft->SetText(VarArgs("%d:%02d", iMinutes, iSeconds));
	}
	else
		m_pTimeleft->SetText("");
}

void CRoundStatusPanel::Paint(int x, int y, int w, int h)
{
	CRootPanel::DrawRect(x, y, x+w, y+h, Color(0, 0, 0, 200));

	CPanel::Paint(x, y, w, h);
}

CRoundVictoryPanel::CRoundVictoryPanel()
	: CPanel(0, 0, 100, 100)
{
	CRootPanel::GetRoot()->AddControl(this, true);

	Assert(!s_pVictoryPanel);

	s_pVictoryPanel = this;

	m_pWhoWon = new CLabel(0, 0, BTN_WIDTH, BTN_HEIGHT, "");
	AddControl(m_pWhoWon);

	m_pHowWon = new CLabel(0, 0, BTN_WIDTH, BTN_HEIGHT, "");
	AddControl(m_pHowWon);
}

void CRoundVictoryPanel::Destructor()
{
	Assert(s_pVictoryPanel);

	s_pVictoryPanel = NULL;

	CPanel::Destructor();
}

void CRoundVictoryPanel::Layout()
{
	SetSize(CFScreenWidth()/3, CFScreenHeight()/4);
	SetPos(CFScreenWidth()/2 - GetWidth()/2, CFScreenHeight()/4*2.6f);
	SetBorder(BT_SOME);

	m_pWhoWon->SetSize(GetWidth(), BTN_HEIGHT);
	m_pHowWon->SetSize(GetWidth(), BTN_HEIGHT);

	m_pWhoWon->SetPos(0, BTN_BORDER);
	m_pHowWon->SetPos(0, BTN_BORDER*2 + BTN_HEIGHT);

	if (CFGameRules()->GetGameMode() == CF_GAME_CTF)
		m_pHowWon->SetText("#CF_Victory_Condition_CTF");
	else if (CFGameRules()->GetGameMode() == CF_GAME_PARIAH)
		m_pHowWon->SetText("#CF_Victory_Condition_Pariah");
	else
		m_pHowWon->SetText("");

	if (CFGameRules()->GetWinningTeam() == TEAM_MACHINDO)
		m_pWhoWon->SetText("#CF_Victory_Machindo");
	else if (CFGameRules()->GetWinningTeam() == TEAM_NUMENI)
		m_pWhoWon->SetText("#CF_Victory_Numeni");
	else
	{
		m_pWhoWon->SetText("#CF_Victory_Nobody");
		m_pHowWon->SetText("#CF_Victory_Condition_Nobody");
	}
}

void CRoundVictoryPanel::Think()
{
	if (CFGameRules()->State_Get() != GR_STATE_TEAM_WIN)
		Close();
}

void CRoundVictoryPanel::Paint(int x, int y, int w, int h)
{
	// Don't draw until we know that the game rules have updated the client with the winning team and all.
	if (!CFGameRules()->RoundHasBeenWon())
		return;

	// When that happens make sure we do layout again so it displays the proper victor. Kindof a hack but watev.
	Layout();

	CRootPanel::DrawRect(x, y, x+w, y+h, Color(0, 0, 0, 200));

	CPanel::Paint(x, y, w, h);
}

void CRoundVictoryPanel::Open()
{
	if (engine->IsPlayingDemo())
		return;

	if (!s_pVictoryPanel)
		new CRoundVictoryPanel();	// deleted in CPanel::Destructor()

	s_pVictoryPanel->Layout();
	s_pVictoryPanel->SetVisible(true);
}

void CRoundVictoryPanel::Close()
{
	if (!s_pVictoryPanel)
		return;

	s_pVictoryPanel->SetVisible(false);
}

CObjectives::CObjectives()
{
	m_pRoundStatus = new CRoundStatusPanel();
	AddControl(m_pRoundStatus);

	m_pMFlags = new CLabel(0, 0, BTN_WIDTH, BTN_HEIGHT, "");
	m_pNFlags = new CLabel(0, 0, BTN_WIDTH, BTN_HEIGHT, "");
	AddControl(m_pMFlags);
	AddControl(m_pNFlags);
}

void CObjectives::LoadTextures()
{
	CObjectives::s_pNumeniFlag = GetHudTexture("flag_numeni");
	CObjectives::s_pMachindoFlag = GetHudTexture("flag_machindo");
	CObjectives::s_pFlagPole = GetHudTexture("flag_pole");
	CObjectives::s_pMShard = GetHudTexture("shard_machindo");
	CObjectives::s_pNShard = GetHudTexture("shard_numeni");
	CObjectives::s_pNeutralShard = GetHudTexture("shard_neutral");
	CObjectives::s_pShardDropped = GetHudTexture("shard_dropped");
	CObjectives::s_pShardCarriedMachindo = GetHudTexture("shard_carried_machindo");
	CObjectives::s_pShardCarriedNumeni = GetHudTexture("shard_carried_numeni");
}

void CObjectives::FireGameEvent( IGameEvent* event)
{
}

void CObjectives::Paint(int x, int y, int w, int h)
{
	C_CFObjectiveResource* pOR = CFObjectiveResource();
	C_CFPlayer* pPlayer = C_CFPlayer::GetLocalCFPlayer();

	if (!pOR || !pPlayer)
		return;

	if (CFGameRules()->GetGameMode() == CF_GAME_PARIAH)
	{
		for (int i = 0; i < pOR->GetNumControlPoints(); i++)
		{
			if (pOR->GetCappingTeam(i) > 0 || (CScoreboard::Get() && CScoreboard::Get()->IsVisible()))
				PaintControlPoint(i, x, y, w, h);
		}

		if (pPlayer->IsFuse())
		{
			for (int i = 0; i < pOR->GetNumControlPoints(); i++)
			{
				if (pOR->GetOwningTeam(i) == pPlayer->GetTeamNumber() && pOR->GetCPCapPercentage(i))
					PaintObjective("#Objective_Defend", pOR->GetCPPosition(i), pOR->GetOwningTeam(i));
				else if (pOR->GetOwningTeam(i) != pPlayer->GetTeamNumber())
					PaintObjective("#Objective_Capture", pOR->GetCPPosition(i), pOR->GetOwningTeam(i));
			}
		}
		else if (pPlayer->IsPariah())
		{
		}
		else
		{
			if (CFGameRules()->ShouldShowFuse(pPlayer))
				PaintObjective("#Objective_Assist", CFGameRules()->GetFuseLocation());

			if (CFGameRules()->ShouldShowPariah(pPlayer))
				PaintObjective("#Objective_Pariah", CFGameRules()->GetPariahLocation());

			for (int i = 0; i < pOR->GetNumControlPoints(); i++)
			{
				if (pOR->GetCPCapPercentage(i) && pOR->GetCappingTeam(i) != pPlayer->GetTeamNumber())
					PaintObjective("#Objective_Defend", pOR->GetCPPosition(i), pOR->GetOwningTeam(i));
			}
		}
	}
	else if (CFGameRules()->GetGameMode() == CF_GAME_CTF)
	{
		int iShardWidth = 80;
		if (cf_ctf_tugofwar.GetBool())
		{
			int iTotalFlags = pOR->GetObjectives();
			int i;

			CUtlVector<C_InfoObjective*> apMachindoFlags;
			CUtlVector<C_InfoObjective*> apNumeniFlags;
			CUtlVector<C_InfoObjective*> apNeutralFlags;

			for (i = 0; i < iTotalFlags; i++)
			{
				C_InfoObjective* pFlag = pOR->GetObjective(i);

				if (!pFlag)
					continue;

				if (pFlag->GetTeamNumber() == TEAM_NUMENI)
					apNumeniFlags.AddToTail(pFlag);
				else if (pFlag->GetTeamNumber() == TEAM_MACHINDO)
					apMachindoFlags.AddToTail(pFlag);
				else
					apNeutralFlags.AddToTail(pFlag);
			}

			int iDrawn = 0;

			int iShardY = BTN_HEIGHT*2;	// Same height as the bottom of the timer.

			int iIconWidth = s_pShardDropped->Width()/2;
			int iIconHeight = s_pShardDropped->Height()/2;
			int iIconY = iShardY + iShardWidth - iIconHeight;

			float flFlagAlpha = 200;
			float flBlinkAlpha;
			if (fmod(gpGlobals->curtime, 1) < 0.5f)
				flBlinkAlpha = RemapValClamped(fmod(gpGlobals->curtime, 1), 0, 0.5f, 0, flFlagAlpha);
			else
				flBlinkAlpha = RemapValClamped(fmod(gpGlobals->curtime, 1), 0.5f, 1.0f, flFlagAlpha, 0);

			for (i = 0; i < apMachindoFlags.Count(); i++)
			{
				int iX = CFScreenWidth()/2 - (iShardWidth*iTotalFlags)/2 + iShardWidth*(iDrawn++);
				C_InfoObjective* pFlag = apMachindoFlags[i];
				bool bCarried = (pFlag->m_hPlayer != NULL);

				s_pMShard->DrawSelf(iX, iShardY, iShardWidth, iShardWidth, Color(255,255,255,bCarried?flBlinkAlpha:flFlagAlpha));

				int iIconX = iX + iShardWidth/2 - iIconWidth/2;
				if (pFlag->m_hPlayer != NULL)
				{
					if (pFlag->m_hPlayer->GetTeamNumber() == TEAM_MACHINDO)
						s_pShardCarriedMachindo->DrawSelf(iIconX, iIconY, iIconWidth, iIconHeight, Color(255, 255, 255, 255));
					else
						s_pShardCarriedNumeni->DrawSelf(iIconX, iIconY, iIconWidth, iIconHeight, Color(255, 255, 255, 255));
				}
				else if (!pFlag->m_bAtSpawn)
					s_pShardDropped->DrawSelf(iIconX, iIconY, iIconWidth, iIconHeight, Color(255, 255, 255, 255));
			}

			for (i = 0; i < apNeutralFlags.Count(); i++)
			{
				int iX = CFScreenWidth()/2 - (iShardWidth*iTotalFlags)/2 + iShardWidth*(iDrawn++);
				C_InfoObjective* pFlag = apNeutralFlags[i];
				bool bCarried = (pFlag->m_hPlayer != NULL);

				s_pNeutralShard->DrawSelf(iX, iShardY, iShardWidth, iShardWidth, Color(255,255,255,bCarried?flBlinkAlpha:flFlagAlpha));

				int iIconX = iX + iShardWidth/2 - iIconWidth/2;
				if (pFlag->m_hPlayer != NULL)
				{
					if (pFlag->m_hPlayer->GetTeamNumber() == TEAM_MACHINDO)
						s_pShardCarriedMachindo->DrawSelf(iIconX, iIconY, iIconWidth, iIconHeight, Color(255, 255, 255, 255));
					else
						s_pShardCarriedNumeni->DrawSelf(iIconX, iIconY, iIconWidth, iIconHeight, Color(255, 255, 255, 255));
				}
				else if (!pFlag->m_bAtSpawn)
					s_pShardDropped->DrawSelf(iIconX, iIconY, iIconWidth, iIconHeight, Color(255, 255, 255, 255));
			}

			for (i = 0; i < apNumeniFlags.Count(); i++)
			{
				int iX = CFScreenWidth()/2 - (iShardWidth*iTotalFlags)/2 + iShardWidth*(iDrawn++);
				C_InfoObjective* pFlag = apNumeniFlags[i];
				bool bCarried = (pFlag->m_hPlayer != NULL);

				s_pNShard->DrawSelf(iX, iShardY, iShardWidth, iShardWidth, Color(255,255,255,bCarried?flBlinkAlpha:flFlagAlpha));

				int iIconX = iX + iShardWidth/2 - iIconWidth/2;
				if (pFlag->m_hPlayer != NULL)
				{
					if (pFlag->m_hPlayer->GetTeamNumber() == TEAM_MACHINDO)
						s_pShardCarriedMachindo->DrawSelf(iIconX, iIconY, iIconWidth, iIconHeight, Color(255, 255, 255, 255));
					else
						s_pShardCarriedNumeni->DrawSelf(iIconX, iIconY, iIconWidth, iIconHeight, Color(255, 255, 255, 255));
				}
				else if (!pFlag->m_bAtSpawn)
					s_pShardDropped->DrawSelf(iIconX, iIconY, iIconWidth, iIconHeight, Color(255, 255, 255, 255));
			}
		}
		else
		{
			s_pMShard->DrawSelf(GetWidth()/2 - iShardWidth*2, 0, iShardWidth, iShardWidth, Color(255,255,255,100));
			s_pNShard->DrawSelf(GetWidth()/2 + iShardWidth, 0, iShardWidth, iShardWidth, Color(255,255,255,100));

			m_pMFlags->SetText(VarArgs("%d/%d", GetGlobalCFTeam(TEAM_MACHINDO)->GetFlagCaptures(), cvar->FindVar("cf_flag_caps_per_round")->GetInt()));
			m_pMFlags->SetDimensions(GetWidth()/2 - iShardWidth*2, 0, iShardWidth, iShardWidth);

			m_pNFlags->SetText(VarArgs("%d/%d", GetGlobalCFTeam(TEAM_NUMENI)->GetFlagCaptures(), cvar->FindVar("cf_flag_caps_per_round")->GetInt()));
			m_pNFlags->SetDimensions(GetWidth()/2 + iShardWidth, 0, iShardWidth, iShardWidth);
		}

		for (int i = 0; i < pOR->GetObjectives(); i++)
		{
			C_InfoObjective* pObj = pOR->GetObjective(i);
			if (!pObj)
				continue;

			if (pObj->GetPlayer() == pPlayer)
				continue;

			if (CFGameRules()->PlayerRelationship(pPlayer, pObj) == GR_NOTTEAMMATE)
			{
				if (pObj->GetPlayer())
				{
					if (CFGameRules()->PlayerRelationship(pPlayer, pObj->GetPlayer()) == GR_TEAMMATE)
						PaintObjective("#Objective_Assist", pObj->WorldSpaceCenter(), pObj->GetTeamNumber());
					else
						PaintObjective("#Objective_Intercept", pObj->WorldSpaceCenter(), pObj->GetTeamNumber());
				}
				else
					PaintObjective("#Objective_Capture", pObj->WorldSpaceCenter(), pObj->GetTeamNumber());
			}
			else
			{
				if (!pObj->IsAtSpawn())
					PaintObjective("#Objective_Recover", pObj->WorldSpaceCenter(), pObj->GetTeamNumber());
				else
					PaintObjective("#Objective_Defend", pObj->WorldSpaceCenter(), pObj->GetTeamNumber());
			}
		}

		if (pPlayer->HasObjective())
		{
			m_GoalArrow.SetModelName("models/other/goalarrow.mdl");

			QAngle angDir;
			Vector vecDir;
			Vector vecCapture = pOR->GetCapturePoint(pPlayer->GetTeamNumber());

			Vector3DMultiplyPosition( CurrentWorldToViewMatrix(), vecCapture, vecDir );

			// Got to convert to ingame handedness.
			VectorAngles(Vector(-vecDir.z, -vecDir.x, vecDir.y), angDir);

			int iArrowWidth = 300;
			int iArrowHeight = 300;
			m_GoalArrow.Render(CFScreenWidth()/2 - iArrowWidth/2, 100, iArrowWidth, iArrowHeight, Vector(20, 0, 0), angDir);
		}
	}

	ICFHUD::Paint(x, y, w, h);
}

void CObjectives::PaintObjective(char* pszObjective, Vector vecPosition, int iTeam)
{
	C_CFPlayer* pPlayer = C_CFPlayer::GetLocalCFPlayer();
	int iLead = 10;
	int x, y, a;

	a = RemapValClamped((vecPosition - pPlayer->EyePosition()).LengthSqr(), 100, 4000000, 255, 50);
	bool bOnScreen = GetVectorInScreenSpace(vecPosition, x, y);

	if (!bOnScreen)
		return;

	if (!pszObjective || pszObjective[0] == '\0')
		return;

	if (!pPlayer->HasObjective() && pPlayer->GetTeamNumber() != TEAM_SPECTATOR)
	{
		wchar_t* pszLocalized = NULL;

		if (pszObjective[0] == '#')
		{
			pszLocalized = g_pVGuiLocalize->Find( pszObjective );
			if (!pszLocalized)
				pszLocalized = L"";
		}

		bool bFree = false;
		if (!pszLocalized)
		{
			int iSize = (strlen(pszObjective) + 1) * sizeof(wchar_t);
			pszLocalized = (wchar_t*)malloc(iSize);
			g_pVGuiLocalize->ConvertANSIToUnicode( pszObjective, pszLocalized, iSize );
			bFree = true;
		}

		int w, h;
		vgui::surface()->GetTextSize(CRootPanel::s_hDefaultFont, pszLocalized, w, h);

		// Not using CRootPanel::DrawRect() because we're already in properly-scaled coordinates.
		vgui::surface()->DrawSetColor(0, 0, 0, a);
		vgui::surface()->DrawFilledRect(x-2, y-2, x + w+iLead + 2, y + h + 2);
		vgui::surface()->DrawSetColor(255, 255, 255, a);
		vgui::surface()->DrawFilledRect(x, y, x + w+iLead, y + 2);

		vgui::surface()->DrawSetTextFont(CRootPanel::s_hDefaultFont);
		vgui::surface()->DrawSetTextColor(Color(255, 255, 255, a));
		vgui::surface()->DrawSetTextPos(x+iLead, y);
		vgui::surface()->DrawPrintText(pszLocalized, wcslen(pszLocalized));

		if (bFree)
			free(pszLocalized);
	}

	if (iTeam >= 0)
	{
		CCFHudTexture* pIcon;
		if (iTeam == TEAM_NUMENI)
			pIcon = s_pNShard;
		else if (iTeam == TEAM_MACHINDO)
			pIcon = s_pMShard;
		else
			pIcon = s_pNeutralShard;

		float flScaleX = CFScreenWidthScale();
		float flScaleY = CFScreenHeightScale();

		int iSize = 30;

		float flAlpha = a;
		if ((vecPosition - pPlayer->EyePosition()).LengthSqr() < 400000)
			flAlpha = RemapValClamped((vecPosition - pPlayer->EyePosition()).LengthSqr(), 400000, 200000, a, 00);
		if (flAlpha > 1)
			pIcon->DrawSelf(x/flScaleX - iSize/2, y/flScaleY - iSize/2, iSize, iSize, Color(255, 255, 255, flAlpha));
	}
}

void CObjectives::PaintControlPoint(int iPoint, int x, int y, int w, int h)
{
	C_CFObjectiveResource* pOR = CFObjectiveResource();

	int iPoleHeight = (ScreenHeight()-100)/7 - 20;
	int iPoleWidth = 3;
	int iX = 10;
	int iY = 100 + (iPoleHeight+20)*iPoint;

	s_pFlagPole->DrawSelf(iX, iY, iPoleWidth, iPoleHeight, Color(255,255,255,255));

	int iFlagHeight = iPoleHeight*2/3;
	int iFlagWidth = iFlagHeight * s_pNumeniFlag->Width() / s_pNumeniFlag->Height();
	int iFlagRealHeight = iFlagHeight/2;	// The flag is this high in the texture, not including the trails.
	iX += iPoleWidth;

	CCFHudTexture* pFlag = NULL;
	switch (pOR->GetOwningTeam(iPoint))
	{
	case TEAM_NUMENI:
		pFlag = s_pNumeniFlag;
		break;
	case TEAM_MACHINDO:
		pFlag = s_pMachindoFlag;
		break;
	}

	if (pFlag)
		pFlag->DrawSelf(iX, iY, iFlagWidth, iFlagHeight, Color(255,255,255,255));

	pFlag = NULL;
	switch (pOR->GetCappingTeam(iPoint))
	{
	case TEAM_NUMENI:
		pFlag = s_pNumeniFlag;
		break;
	case TEAM_MACHINDO:
		pFlag = s_pMachindoFlag;
		break;
	}

	if (pFlag)
	{
		pFlag->DrawSelf(iX,
			iY + (iPoleHeight-iFlagRealHeight) * (1-pOR->GetCPCapPercentage(iPoint)),
			iFlagWidth, iFlagHeight,
			Color(255,255,255,255));
	}
}

void CObjectives::PostRenderVGui()
{
}
