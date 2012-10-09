//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Draws the death notices
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "cfui_menu.h"
#include "cfui_scoreboard.h"
#include <vgui_controls/ImageList.h>
#include <vgui/ISurface.h>
#include <igameresources.h>
#include "vgui_avatarimage.h"
#include "c_cf_playerresource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CScoreboard* CScoreboard::s_pScoreboard = NULL;
bool CScoreboard::s_bLocked = false;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CScoreboard::CScoreboard()
	: CPanel(0, 0, 100, 100)
{
	m_iPlayerIndexSymbol = KeyValuesSystem()->GetSymbolForString("playerIndex");
	
	CRootPanel::GetRoot()->AddControl(this);

	SetVisible(false);

	m_pHostname = new CLabel(0, 0, 100, 100, "");
	m_pHostname->SetAlign(CLabel::TA_CENTER);
	AddControl(m_pHostname);

	m_pPlayerList = new CPlayerList();
	AddControl(m_pPlayerList);

	m_pImageList = new vgui::ImageList( false );

	m_mapAvatarsToImageList.SetLessFunc( &CScoreboard::AvatarIndexLessFunc );
	m_mapAvatarsToImageList.RemoveAll();
	memset( &m_iImageAvatars, 0, sizeof(int) * (MAX_PLAYERS+1) );

	m_flLastUpdate = 0;
}

void CScoreboard::Destructor()
{
	if (s_pScoreboard)
		s_pScoreboard = NULL;

	if ( NULL != m_pImageList )
	{
		delete m_pImageList;
		m_pImageList = NULL;
	}
}

void CScoreboard::LevelShutdown()
{
	m_flLastUpdate = 0;
}

void CScoreboard::Think()
{
	if (gpGlobals->curtime > m_flLastUpdate + 1)
	{
		UpdatePlayerInfo();

		m_flLastUpdate = gpGlobals->curtime;
	}

	CPanel::Think();
}

void CScoreboard::Layout()
{
	SetPos(CFScreenWidth()/2-260, 42);
	SetSize(520, CFScreenHeight()-120);

	m_pHostname->SetSize(GetWidth(), BTN_HEIGHT);
	m_pHostname->SetText(CRootPanel::GetRoot()->GetHostname());

	m_iHighestScore = -1;
	m_iLowestScore = -1;

	UpdatePlayerInfo();

	m_pPlayerList->SetDimensions(3, BTN_HEIGHT, GetWidth()-6, GetHeight()-BTN_HEIGHT-3);

	CPanel::Layout();
}

void CScoreboard::UpdatePlayerInfo()
{
	int iSelectedRow = -1;

	// walk all the players and make sure they're in the scoreboard
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		IGameResources *gr = GameResources();

		if ( gr && gr->IsConnected( i ) )
		{
			// add the player to the list
			KeyValues *playerData = new KeyValues("data");
			GetPlayerScoreInfo( i, playerData );
			UpdatePlayerAvatar( i, playerData );

			const char *oldName = playerData->GetString("name","");
			int bufsize = strlen(oldName) * 2 + 1;
			char *newName = (char *)_alloca( bufsize );

			UTIL_MakeSafeName( oldName, newName, bufsize );

			playerData->SetString("name", newName);

			int iScore = playerData->GetInt("score");
			if (iScore > m_iHighestScore || m_iHighestScore == -1)
				m_iHighestScore = iScore;
			if (iScore < m_iLowestScore || m_iLowestScore == -1)
				m_iLowestScore = iScore;

			int iItemID = FindItemIDForPlayerIndex( i );
			int iTeam = gr->GetTeam( i );

			if ( gr->IsLocalPlayer( i ) )
			{
				iSelectedRow = iItemID;
			}
			
			if (iItemID == -1)
			{
				// add a new row
				iItemID = m_pPlayerList->AddItem( playerData );
			}
			else
			{
				// modify the current row
				m_pPlayerList->ModifyItem( iItemID, playerData );
			}

			// set the row color based on the players team
			m_pPlayerList->SetItemTeam( iItemID, iTeam );

			if (UTIL_PlayerByIndex(i) && ToCFPlayer(UTIL_PlayerByIndex(i))->IsPariah())
				m_pPlayerList->SetItemColor(iItemID, Color(255, 0, 0, 255));

			playerData->deleteThis();
		}
		else
		{
			// remove the player
			int itemID = FindItemIDForPlayerIndex( i );
			if (itemID != -1)
			{
				m_pPlayerList->RemoveItem(itemID);
			}
		}
	}

	if ( iSelectedRow != -1 )
	{
		m_pPlayerList->SetSelectedItem(iSelectedRow);
	}
}

bool CScoreboard::GetPlayerScoreInfo(int playerIndex, KeyValues *kv)
{
	C_CF_PlayerResource* pGR = dynamic_cast<C_CF_PlayerResource*>(GameResources());

	if (!pGR )
		return false;

	kv->SetInt("deaths", pGR->GetDeaths( playerIndex ) );
	kv->SetInt("score", pGR->GetTotalScore( playerIndex) );
	kv->SetInt("ping", pGR->GetPing( playerIndex ) ) ;
	kv->SetString("name", pGR->GetPlayerName( playerIndex ) );
	kv->SetInt("playerIndex", playerIndex);

	return true;
}

void CScoreboard::UpdatePlayerAvatar( int playerIndex, KeyValues *kv )
{
	// Update their avatar
	if ( kv && steamapicontext->SteamFriends() && steamapicontext->SteamUtils() )
	{
		player_info_t pi;
		if ( engine->GetPlayerInfo( playerIndex, &pi ) )
		{
			if ( pi.friendsID )
			{
				CSteamID steamIDForPlayer( pi.friendsID, 1, steamapicontext->SteamUtils()->GetConnectedUniverse(), k_EAccountTypeIndividual );

				// See if the avatar's changed
				int iAvatar = steamapicontext->SteamFriends()->GetFriendAvatar( steamIDForPlayer );
				if ( m_iImageAvatars[playerIndex] != iAvatar )
				{
					m_iImageAvatars[playerIndex] = iAvatar;

					// Now see if we already have that avatar in our list
					int iIndex = m_mapAvatarsToImageList.Find( iAvatar );
					if ( iIndex == m_mapAvatarsToImageList.InvalidIndex() )
					{
						CAvatarImage *pImage = new CAvatarImage();
						pImage->SetAvatarSteamID( steamIDForPlayer );
						pImage->SetSize( 32, 32 );	// Deliberately non scaling
						int iImageIndex = m_pImageList->AddImage( pImage );

						m_mapAvatarsToImageList.Insert( iAvatar, iImageIndex );
					}
				}

				int iIndex = m_mapAvatarsToImageList.Find( iAvatar );

				if ( iIndex != m_mapAvatarsToImageList.InvalidIndex() )
				{
					kv->SetInt( "avatar", m_mapAvatarsToImageList[iIndex] );
				}
			}
		}
	}
}

int CScoreboard::FindItemIDForPlayerIndex(int playerIndex)
{
	for (int i = 0; i <= m_pPlayerList->GetHighestItemID(); i++)
	{
		if (m_pPlayerList->IsItemIDValid(i))
		{
			KeyValues *kv = m_pPlayerList->GetItemData(i);
			kv = kv->FindKey(m_iPlayerIndexSymbol);
			if (kv && kv->GetInt() == playerIndex)
				return i;
		}
	}
	return -1;
}

bool CScoreboard::AvatarIndexLessFunc( const int &lhs, const int &rhs )	
{ 
	return lhs < rhs; 
}

void CScoreboard::Paint(int x, int y, int w, int h)
{
	if (!IsVisible())
		return;

	CRootPanel::DrawRect(x, y, x+w, y+h, Color(0, 0, 0, 200));

	CPanel::Paint(x, y, w, h);
}

void CScoreboard::OpenScoreboard(bool bVis)
{
	// Don't allow the player to close the scoreboard during intermission time.
	if (IsLocked())
		return;

	if (bVis)
		Get()->Layout();
	else
		Get()->m_Input.Activate(false);

	Get()->SetVisible(bVis);
}

bool CScoreboard::ClaimMouse()
{
	if (!Get()->IsVisible())
		return false;

	Get()->m_Input.Activate(true);
	return true;
}

CScoreboard* CScoreboard::Get()
{
	if (!s_pScoreboard)
		s_pScoreboard = new CScoreboard();

	return s_pScoreboard;
}

CPlayerList::CPlayerList()
	: CPanel(0, 0, 100, 100)
{
	m_pName = new CLabel(0, 0, 100, 100, "#Name");
	m_pPing = new CLabel(0, 0, 100, 100, "#Ping");
	AddControl(m_pName);
	AddControl(m_pPing);

	SetBorder(CPanel::BT_NONE);
}

void CPlayerList::Layout()
{
	int iHeight = GetHeight()/24;

	m_apPlayerList.Sort(SortByScore);

	m_pName->SetDimensions(0, 0, GetWidth()-PING_WIDTH, iHeight);
	m_pPing->SetDimensions(GetWidth()-PING_WIDTH, 0, PING_WIDTH, iHeight);

	for (int i = 0; i < m_apPlayerList.Count(); i++)
		m_apPlayerList[i]->SetDimensions(0, GetHeight()*(i+1)/24, GetWidth(), iHeight);
}

int CPlayerList::AddItem(const KeyValues *pData)
{
	CPlayerItem* pItem = new CPlayerItem(pData);
	AddControl(pItem);
	int iItemID = m_apPlayerList.AddToTail(pItem);
	AddTargetListener(iItemID);
	return iItemID;
}

void CPlayerList::ModifyItem(int iItemID, const KeyValues *pData)
{
	m_apPlayerList[iItemID]->SetData(pData);
	AddTargetListener(iItemID);
}

void CPlayerList::RemoveItem(int iItemID)
{
	RemoveControl(m_apPlayerList[iItemID]);
	m_apPlayerList[iItemID]->Destructor();
	m_apPlayerList[iItemID]->Delete();
	m_apPlayerList.Remove(iItemID);
}

void CPlayerList::AddTargetListener(int i)
{
	// Clear it out just in case.
	m_apPlayerList[i]->SetClickedListener(NULL, NULL);
	
	IGameResources *gr = GameResources();

	int iPlayer = m_apPlayerList[i]->GetData()->GetInt("playerIndex");
	if ( gr && gr->IsConnected( iPlayer ) && gr->GetTeam( iPlayer ) == C_BasePlayer::GetLocalPlayer()->GetTeamNumber() )
		m_apPlayerList[i]->SetClickedListener(this, &CPlayerList::TargetPlayer, new KeyValues("target", "id", iPlayer));
}

int CPlayerList::SortByScore(const ScoreSortType* pItemLeft, const ScoreSortType* pItemRight)
{
	KeyValues* pLeft = (*pItemLeft)->GetData();
	KeyValues* pRight = (*pItemRight)->GetData();

	// first compare score
	int v1 = pLeft->GetInt("score");
	int v2 = pRight->GetInt("score");
	if (v1 != v2)
		return v2 - v1;

	// next compare deaths
	v1 = pLeft->GetInt("deaths");
	v2 = pRight->GetInt("deaths");
	if (v1 != v2)
		return v2 - v1;

	// the same, so compare itemID's (as a sentinel value to get deterministic sorts)
	return pRight->GetInt("playerIndex") - pLeft->GetInt("playerIndex");
}

void CPlayerList::TargetPlayerCallback(KeyValues *data)
{
	engine->ClientCmd(VarArgs("target %d", data->GetInt("id")));
}

void CPlayerList::SetItemTeam(int iItemID, int iTeam)
{
	m_apPlayerList[iItemID]->SetTeam(iTeam);
}

void CPlayerList::SetSelectedItem(int iItemID)
{
	m_iSelectedItem = iItemID;
}

void CPlayerList::SetItemColor(int iItemID, Color c)
{
	m_apPlayerList[iItemID]->SetColor(c);
}

int CPlayerList::GetHighestItemID()
{
	return m_apPlayerList.Count();
}

bool CPlayerList::IsItemIDValid(int iItemID)
{
	return m_apPlayerList.IsValidIndex(iItemID);
}

KeyValues* CPlayerList::GetItemData(int iItemID)
{
	return m_apPlayerList[iItemID]->GetData();
}

CPlayerItem::CPlayerItem(const KeyValues* pKV)
	: CPanel(0, 0, 100, 100)
{
	m_pButton = new CPlayerButton();
	AddControl(m_pButton);

	m_pPing = new CLabel(0, 0, PING_WIDTH, BTN_HEIGHT, "");
	AddControl(m_pPing);

	SetBorder(BT_NONE);
	m_pKV = NULL;
	SetData(pKV);
	SetTeam(TEAM_SPECTATOR);
}

void CPlayerItem::Destructor()
{
	if (m_pKV)
		m_pKV->deleteThis();

	CPanel::Destructor();
}

void CPlayerItem::SetData(const KeyValues* pKV)
{
	if (m_pKV)
		m_pKV->deleteThis();

	m_pKV = pKV->MakeCopy();

	m_pButton->SetText(m_pKV->GetString("name"));
}

KeyValues* CPlayerItem::GetData() const
{
	return m_pKV;
}

void CPlayerItem::SetTeam(int iTeam)
{
	m_iTeam = iTeam;

	IGameResources *gr = GameResources();

	if (gr)
		m_pButton->SetFGColor(gr->GetTeamColor( iTeam ));

	if (iTeam == TEAM_MACHINDO)
		m_pButton->SetAlign(CLabel::TA_LEFTCENTER);
	else if (iTeam == TEAM_NUMENI)
		m_pButton->SetAlign(CLabel::TA_RIGHTCENTER);
	else
		m_pButton->SetAlign(CLabel::TA_CENTER);
}

void CPlayerItem::SetColor(Color c)
{
	m_pButton->SetFGColor(c);
}

void CPlayerItem::Think()
{
	int iHighest = CScoreboard::Get()->GetHighestScore();
	int iLowest = CScoreboard::Get()->GetLowestScore();

	int iAlpha;
	if (iHighest == iLowest)
		iAlpha = 255/2;
	else
		iAlpha = (m_pKV->GetInt("score")-iLowest) * 255 / (iHighest - iLowest);

	m_pButton->SetBGAlpha(iAlpha);

	m_pButton->SetSize(GetWidth()-PING_WIDTH, GetHeight());

	m_pPing->SetPos(GetWidth()-PING_WIDTH, 0);
	m_pPing->SetText(VarArgs("%d", m_pKV->GetInt("ping")));
}

void CPlayerItem::Paint(int x, int y, int w, int h)
{
	CPanel::Paint(x, y, w, h);
}

void CPlayerItem::SetClickedListener(IEventListener* pListener, IEventListener::Callback pfnCallback, KeyValues *pParms)
{
	m_pButton->SetClickedListener(pListener, pfnCallback, pParms);
}

void CPlayerItem::SetUnclickedListener(IEventListener* pListener, IEventListener::Callback pfnCallback, KeyValues *pParms)
{
	m_pButton->SetUnclickedListener(pListener, pfnCallback, pParms);
}

CPlayerButton::CPlayerButton()
	: CButton(0, 0, 100, 100, "")
{
}

void CPlayerButton::Paint(int x, int y, int w, int h)
{
	CRootPanel::DrawRect(x, y, x+w, y+h, Color(255, 255, 255, m_iBGAlpha));

	// Just paint the text which appears on the button, but not the button decorations in CButton::Paint().
	CLabel::Paint(x, y, w, h);
}
