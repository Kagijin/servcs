
#include "stdafx.h"
#include "desc.h"
#include "utils.h"
#include "config.h"
#include "char.h"
#include "char_manager.h"
#include "affect.h"
#include "start_position.h"
#include "p2p.h"
#include "db.h"
#include "dungeon.h"
#include <string>
#include <boost/algorithm/string/replace.hpp>
#include "desc_manager.h"
#include "buffer_manager.h"
#include "dev_log.h"
#include <fstream>
#include <algorithm>
#include <iostream>
#include "constants.h"
#include "questmanager.h"
#include "questlua.h"
#include "desc_client.h"
#include "sectree_manager.h"
#include "regen.h"
#include "item.h"
#include <boost/format.hpp>
#include "item_manager.h"
#include "tournament.h"

#define insert_winners(fmt, ...) RegisterWinners(__FUNCTION__, __LINE__, fmt, __VA_ARGS__)
#define FILENAME_LOG_WINNER		"tournament_logs_winners.txt"
#define FILENAME_BLOCK_ITEMS	"tournament_forbidden_items.txt"

#ifdef ENABLE_PLAYER2_ACCOUNT2__
#define MYSQL_DATABASE_RANKING	"player2.tournament_ranking"
#else
#define MYSQL_DATABASE_RANKING	"player.tournament_ranking"
#endif

#ifdef TOURNAMENT_PVP_SYSTEM
std::map<DWORD, DWORD> m_mapParticipants;
int TOURNAMENT_MAX_PLAYERS = 0;

static const std::map<std::string, std::string> LC_TRANSLATE_MAP = {
	{"TOURNAMENT_CATEGORY_LOW", "低级场"},
	{"TOURNAMENT_CATEGORY_MEDIUM", "中级场"},
	{"TOURNAMENT_CATEGORY_HARD", "高级场"},
	{"TOURNAMENT_TEAM_MEMBER_RED", "红色"},
	{"TOURNAMENT_TEAM_MEMBER_BLUE", "蓝色"},
	{"TOURNAMENT_ANNOUNCEMENT_BEFORE_START", "<红蓝对抗赛> %s PVP竞技赛将在 %d 分钟后开始,已报名的玩家请进入竞技场等待"},
	{"TOURNAMENT_ANNOUNCEMENT_START_LINE_1", "<红蓝对抗赛> %s PVP竞技赛已经开始,兄弟们干吧!"},
	{"TOURNAMENT_ANNOUNCEMENT_START_LINE_2", "<红蓝对抗赛> 比赛持续时间为 %d 分钟,赢得比赛请留在原地等待倒计时结束,否则无法获得奖励"},
	{"TOURNAMENT_ANNOUNCEMENT_START_LINE_3", "<红蓝对抗赛> 观战玩家如果想离开,请点击屏幕左侧的任务"},
	{"TOURNAMENT_ANNOUNCEMENT_NOT_STARTED_LINE_1", "<红蓝对抗赛> [%s] 因玩家数量不足,本次比赛自动取消,下期再来吧."},
	{"TOURNAMENT_ANNOUNCEMENT_NOT_STARTED_LINE_2", "<红蓝对抗赛> 下期开始比赛时间为:%s，[%s] 请多留意开放时间."},
	{"TOURNAMENT_ANNOUNCEMENT_FINISHED_LINE_1", "<红蓝对抗赛> [%s] 竞技赛已结束了."},
	{"TOURNAMENT_ANNOUNCEMENT_FINISHED_LINE_2", "<红蓝对抗赛> 下期开始比赛时间为:%s，[%s] 请多留意时间."},
	{"TOURNAMENT_ANNOUNCEMENT_WINNER_SAME", "<红蓝对抗赛> 战斗以平局结束，因两队存活下来的人员数量相同."},
	{"TOURNAMENT_ANNOUNCEMENT_WINNER_NOT_EXIST", "<红蓝对抗赛> 这场比赛没有任何赢家,因没有队员等待活动结束.."},
	{"TOURNAMENT_ANNOUNCEMENT_WINNER_SUCCES", "<红蓝对抗赛> 祝贺[%s]的队伍,获得胜利,队伍存活剩余 %d 人."},
	{"TOURNAMENT_MEMBER_REMAINING_LIFE", "<红蓝对抗赛> 剩余生命次数: %d."},
	{"TOURNAMENT_MEMBER_FINISHED_LIFE_LINE_1", "<红蓝对抗赛> 你的生命次数已耗尽."},
	{"TOURNAMENT_MEMBER_FINISHED_LIFE_LINE_2", "<红蓝对抗赛> 你已从比赛中被淘汰，请再接再厉！"},
	{"TOURNAMENT_MEMBER_DIVIDED", "<红蓝对抗赛> 您被系统分配到了 [%s] 的队伍."},
	{"TOURNAMENT_MEMBER_BLOCK_DUEL", "<红蓝对抗赛> 不能使用对决功能"},
	{"TOURNAMENT_MEMBER_BLOCK_PARTY", "<红蓝对抗赛> 不能使用组队功能."},
	{"TOURNAMENT_MEMBER_BLOCK_RING_MARRIAGE", "<红蓝对抗赛> 不能使用结婚戒指."},
	{"TOURNAMENT_MEMBER_BLOCK_POLY", "<红蓝对抗赛> 不能使用幻化物品."},
	{"TOURNAMENT_MEMBER_BLOCK_CHANGE_PKMODE", "<红蓝对抗赛> 不能使用PVP模式."},
	{"TOURNAMENT_MEMBER_BLOCK_MOUNT", "<红蓝对抗赛> 不能使用坐骑物品."},
	{"TOURNAMENT_MEMBER_BLOCK_HORSE", "<红蓝对抗赛> 不能使用骑乘马匹."},
	{"TOURNAMENT_MEMBER_BLOCK_EXIT_OBSERVER_MODE_LINE_1", "<红蓝对抗赛> 结束观战模式不可用."},
	{"TOURNAMENT_MEMBER_BLOCK_EXIT_OBSERVER_MODE_LINE_2", "<红蓝对抗赛> 如果想退出观战模式,请点击屏幕左侧的任务离开."},
	{"TOURNAMENT_MEMBER_OPEN_REWARD_WRONG_SIZE", "<红蓝对抗赛> 物品栏没有足够的空间,无法打开奖品盒,背包请腾出%d个位置."},
	{"TOURNAMENT_MEMBER_CANNOT_USE_ITEM", "<红蓝对抗赛> 该地图限制使用物品:%s."},
	{"TOURNAMENT_INSERT_LOG_WINNERS", "角色: %s | 等级: %d | IP: %s | 剩余生命: %d | 队员: %s"},
#ifdef ENABLE_EXTRA_LIVES_TOURNAMENT
	{"TOURNAMENT_MEMBER_ADD_EXTRA_LIVES_ERROR_LINE_1", "<红蓝对抗赛> 该物品只能在竞技场以外的地图提前使用."},
	{"TOURNAMENT_MEMBER_ADD_EXTRA_LIVES_ERROR_LINE_2", "<红蓝对抗赛> 暂时不能使用,需要消耗所有的生命."},
	{"TOURNAMENT_MEMBER_ADD_EXTRA_LIVES_ERROR_LINE_3", "<红蓝对抗赛> 剩余生命：%d 最大生命 %d"},
	{"TOURNAMENT_MEMBER_ADD_EXTRA_LIVES_SUCCES", "<红蓝对抗赛> 恭喜获得+%d次复活机会,最大复活%d次."},
#endif
};

static const char* LC_TRANSLATE(const char* key) 
{
	auto it = LC_TRANSLATE_MAP.find(key);
	if (it != LC_TRANSLATE_MAP.end())
		return it->second.c_str();
	return key; // 未找到时返回原键，避免空指针
}

long g_position_tournament[2][2] = 
{
	{ 102800,	436700 },	// TEAM_MEMBERS_A
	{ 98700,	432500 },	// TEAM_MEMBERS_B
};

long g_observers_position_tournament[3][2] = 
{
	{ 95100,	437600 },	// Random 1
	{ 95000,	433300 },	// Random 2
	{ 106700,	437500 }	// Random 3
};

bool CTournamentPvP::file_is_empty(std::ifstream& file)
{
	if (!file || !file.is_open()) 
		return true;

	return file.peek() == std::ifstream::traits_type::eof();
}

int CTournamentPvP::GetStatus()
{
	return quest::CQuestManager::instance().GetEventFlag("tournament_event");
}

void CTournamentPvP::SetStatus(int iValue)
{
	quest::CQuestManager::instance().RequestSetEventFlag("tournament_event", iValue);	
}

void RegisterWinners(const char *func, int line, const char *format, ...)
{
	va_list kwargs;
	time_t vKey = time(0);  
	char *time_s = asctime(localtime(&vKey));

	FILE* file = NULL;
	char szBuf[1024 + 2];
	int length;
	
	char szFileName[256];
	snprintf(szFileName, sizeof(szFileName), "%s/%s", LocaleService_GetBasePath().c_str(), FILENAME_LOG_WINNER);
	file = fopen(szFileName, "a+"); 

	if (!file)
	{
		sys_err("Error %s", szFileName);
		return;
	}

	time_s[strlen(time_s) - 1] = '\0';
	length = snprintf(szBuf, 1024, "Date: %-15.15s | ", time_s + 4);
	szBuf[1025] = '\0';

	if (length < 1024)
	{
		va_start(kwargs, format);
		vsnprintf(szBuf + length, 1024 - length, format, kwargs);
		va_end(kwargs);
	}

	strcat(szBuf, "\n");

	fputs(szBuf, file);
	fflush(file);

	fputs(szBuf, stdout);
	fflush(stdout);
	fclose(file);
}

void CTournamentPvP::ReadFileItems()
{
	char szFileName[256];
	snprintf(szFileName, sizeof(szFileName), "%s/%s", LocaleService_GetBasePath().c_str(), FILENAME_BLOCK_ITEMS);
	std::ifstream file(szFileName);

	if (!file.is_open())
	{
		sys_err("Error %s", szFileName);
		return;
	}

	std::string line;
	while (getline(file, line))
	{
		if (line.empty())
			continue;
		
		DWORD iVnum = atoi(line.c_str());
		m_listForbidden.push_back(iVnum);
	}
}

struct FRefreshWindow
{
	void operator() (LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = static_cast<LPCHARACTER>(ent);
			if (!ch || !ch->GetDesc())
				return;

			if (!ch->IsObserverMode() || ch->IsGM())
			{
				TPacketGCTournamentAdd p;
				p.header = HEADER_GC_TOURNAMENT_ADD;
				p.membersOnline_A = CTournamentPvP::instance().GetMembersTeamA();
				p.membersOnline_B = CTournamentPvP::instance().GetMembersTeamB();
				p.membersDead_A = (CTournamentPvP::instance().GetMembersTeamA() == TOURNAMENT_MAX_PLAYERS / 2) ? 0 : TOURNAMENT_MAX_PLAYERS / 2 - CTournamentPvP::instance().GetMembersTeamA();
				p.membersDead_B = (CTournamentPvP::instance().GetMembersTeamB() == TOURNAMENT_MAX_PLAYERS / 2) ? 0 : TOURNAMENT_MAX_PLAYERS / 2 - CTournamentPvP::instance().GetMembersTeamB();
				p.memberLives = CTournamentPvP::instance().GetMyLives(ch->GetPlayerID());
				p.dwTimeRemained = TOURNAMENT_DURATING_FIGHT;
				ch->GetDesc()->Packet(&p, sizeof(TPacketGCTournamentAdd));
			}
		}
	}
};

struct FWarpToHome
{
	void operator() (LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = static_cast<LPCHARACTER>(ent);
			if (ch->IsGM())
			{
				ch->ChatPacket(CHAT_TYPE_COMMAND, "TournamentPvP_RefreshBoard"); 
			}
			
			
			if (ch && ch->IsPC() && !ch->IsGM())
				ch->GoHome();
		}
	}
};

bool CTournamentPvP::IsTournamentMap(LPCHARACTER ch, int key)
{
	if (ch->GetMapIndex() == TOURNAMENT_MAP_INDEX)
	{
		switch(key)
		{
			case TOURNAMENT_BLOCK_DUEL:
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TRANSLATE("TOURNAMENT_MEMBER_BLOCK_DUEL")); 
				return true;

			case TOURNAMENT_BLOCK_PARTY:
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TRANSLATE("TOURNAMENT_MEMBER_BLOCK_PARTY"));
				return true;

			case TOURNAMENT_BLOCK_RING_MARRIAGE:
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TRANSLATE("TOURNAMENT_MEMBER_BLOCK_RING_MARRIAGE"));
				return true;

			case TOURNAMENT_BLOCK_POLY:
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TRANSLATE("TOURNAMENT_MEMBER_BLOCK_POLY")); 
				return true;

			case TOURNAMENT_BLOCK_CHANGE_PKMODE:
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TRANSLATE("TOURNAMENT_MEMBER_BLOCK_CHANGE_PKMODE"));
				return true;

			case TOURNAMENT_BLOCK_MOUNT:
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TRANSLATE("TOURNAMENT_MEMBER_BLOCK_MOUNT"));
				return true;			
			
			case TOURNAMENT_BLOCK_HORSE:
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TRANSLATE("TOURNAMENT_MEMBER_BLOCK_HORSE"));
				return true;

			case TOURNAMENT_BLOCK_EXIT_OBSERVER_MODE:
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TRANSLATE("TOURNAMENT_MEMBER_BLOCK_EXIT_OBSERVER_MODE_LINE_1"));		
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TRANSLATE("TOURNAMENT_MEMBER_BLOCK_EXIT_OBSERVER_MODE_LINE_2"));
				return true;

			default:
				return false;
		}
	}
	// 新增默认返回值：非竞技场地图时返回false
	return false;
}

std::string CTournamentPvP::ConvertTeamToString(DWORD idxTeam)
{
	switch (idxTeam)
	{
		case TEAM_MEMBERS_A:
			return LC_TRANSLATE("TOURNAMENT_TEAM_MEMBER_RED");
		case TEAM_MEMBERS_B:
			return LC_TRANSLATE("TOURNAMENT_TEAM_MEMBER_BLUE");
		default:
			return "Unknown Team";	//覆盖所有分支
	}
}

int CTournamentPvP::GetAttackMode(int indexTeam)
{
	switch (indexTeam)
	{
		case TEAM_MEMBERS_A:
			return PK_MODE_TEAM_A;
		case TEAM_MEMBERS_B:
			return PK_MODE_TEAM_B;
		default:
			return PK_MODE_PEACE;	// 默认和平模式
	}
}

void RefreshWindow()
{
	if (CTournamentPvP::instance().GetMembersTeamA() == TOURNAMENT_NO_MEMBERS && CTournamentPvP::instance().GetMembersTeamB() == TOURNAMENT_NO_MEMBERS)
		return;
	
	LPSECTREE_MAP pSectreeMap = SECTREE_MANAGER::instance().GetMap(TOURNAMENT_MAP_INDEX);
	if (pSectreeMap)
	{
		FRefreshWindow f;
		pSectreeMap->for_each(f);
	}
}

bool CTournamentPvP::Initialize()
{
	ReadFileItems();
	ClearRegisters();
	ClearSTDMap();
	SetStatus(TOURNAMENT_STATE_FINISHED);
	return true;
}

void CTournamentPvP::Destroy()
{
	ClearSTDMap();
	ClearRegisters();
	SetStatus(TOURNAMENT_STATE_FINISHED);
}

void CTournamentPvP::ClearSTDMap()
{
	if (m_pkTournamentUpdateEvent)
	{
		event_cancel(&m_pkTournamentUpdateEvent);
		m_pkTournamentUpdateEvent = NULL;
	}
	
	TOURNAMENT_MAX_PLAYERS = 0;
	m_map_team_a.clear();
	m_map_team_b.clear();
	m_map_lives.clear();
}

void CTournamentPvP::ClearRegisters()
{
	m_mapParticipants.clear();
}

bool CTournamentPvP::CanUseItem(LPCHARACTER ch, LPITEM item)
{
	if (!ch || !item)
		return false;

	switch (item->GetVnum())
	{
#ifdef ENABLE_EXTRA_LIVES_TOURNAMENT	
		case TOURNAMENT_ITEM_EXTRA_LIVES:
		{
			if (ch->GetMapIndex() == TOURNAMENT_MAP_INDEX)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TRANSLATE("TOURNAMENT_MEMBER_ADD_EXTRA_LIVES_ERROR_LINE_1"));
				return false;
			}

			if (CTournamentPvP::instance().GetExistExtraLives(ch))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TRANSLATE("TOURNAMENT_MEMBER_ADD_EXTRA_LIVES_ERROR_LINE_2"));
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TRANSLATE("TOURNAMENT_MEMBER_ADD_EXTRA_LIVES_ERROR_LINE_3"), (TOURNAMENT_CAN_USED_MAX_EXTRA_LIVES - CTournamentPvP::instance().GetUsedCountExtraLives(ch)), TOURNAMENT_CAN_USED_MAX_EXTRA_LIVES);
				return false;
			}

			ch->ChatPacket(CHAT_TYPE_INFO, LC_TRANSLATE("TOURNAMENT_MEMBER_ADD_EXTRA_LIVES_SUCCES"), (TOURNAMENT_EXTRA_LIVES - TOURNAMENT_MAX_LIVES), TOURNAMENT_CAN_USED_MAX_EXTRA_LIVES);
			ch->SetQuestFlag(FLAG_EXTRA_LIVES, 1);
			ch->RemoveSpecifyItem(item->GetVnum(), 1);

			// 关键修复：添加显式返回值（道具使用成功，返回true）
			return true;
		}
		break;
#endif

		default:
			return true;
	}
	
	// 兜底返回（防止极端情况编译器警告，实际逻辑不会走到这里）
	// return false;
}

void CTournamentPvP::GiveReward(LPCHARACTER ch)
{
	ch->AutoGiveItem(TOURNAMENT_ITEM_BOX, 1);//默认设置
}

void CTournamentPvP::SendNoticeLine(const char * format, ...)
{
	if (!format)
		return;

	char chatbuf[CHAT_MAX_LEN + 1];
	va_list args;

	va_start(args, format);
	vsnprintf(chatbuf, sizeof(chatbuf), format, args);
	va_end(args);
	BroadcastNotice(chatbuf);
}

#ifdef ENABLE_KILL_COUNTS_FOR_EACH_PLAYER	
void CTournamentPvP::InsertPlayerKillLogs(LPCHARACTER ch)
{
	if (!ch)
		return;

	int iPoints = ch->GetQuestFlag(FLAG_KILL_COUNT);
	std::unique_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT pid FROM %s WHERE pid = '%d'", MYSQL_DATABASE_RANKING, ch->GetPlayerID()));

	if (pMsg && pMsg->Get() && pMsg->Get()->uiNumRows)
		DBManager::instance().DirectQuery("UPDATE %s SET total_points = '%d' WHERE pid = '%d'", MYSQL_DATABASE_RANKING, iPoints, ch->GetPlayerID());
	else if (pMsg) 
		DBManager::instance().DirectQuery("INSERT INTO %s (pid, name, total_points) VALUES('%d', '%s', '%d')", MYSQL_DATABASE_RANKING, ch->GetPlayerID(), ch->GetName(), iPoints);	
}
#endif

void CTournamentPvP::GetTeamWinner()
{
	if (CTournamentPvP::instance().GetMembersTeamA() == CTournamentPvP::instance().GetMembersTeamB())
	{
		SendNoticeLine(LC_TRANSLATE("TOURNAMENT_ANNOUNCEMENT_WINNER_SAME"));	
		DestroyAll();
		return;
	}
	
	if (CTournamentPvP::instance().GetMembersTeamA() == TOURNAMENT_NO_MEMBERS && CTournamentPvP::instance().GetMembersTeamB() == TOURNAMENT_NO_MEMBERS)
	{
		SendNoticeLine(LC_TRANSLATE("TOURNAMENT_ANNOUNCEMENT_WINNER_NOT_EXIST"));	
		DestroyAll();
		return;
	}	

	int idxTeamWinner = (CTournamentPvP::instance().GetMembersTeamA() > CTournamentPvP::instance().GetMembersTeamB()) ? TEAM_MEMBERS_A : TEAM_MEMBERS_B;
	int idxSize = (CTournamentPvP::instance().GetMembersTeamA() > CTournamentPvP::instance().GetMembersTeamB()) ? CTournamentPvP::instance().GetMembersTeamA() : CTournamentPvP::instance().GetMembersTeamB();
	
	switch (idxTeamWinner)
	{
		case TEAM_MEMBERS_A:
		{
			itertype(m_map_team_a) it = m_map_team_a.begin();
			LPCHARACTER ch = NULL;
			for (; it != m_map_team_a.end(); ++it)
			{
				ch = CHARACTER_MANAGER::instance().FindByPID(it->second);
				if (ch)
				{
					insert_winners(LC_TRANSLATE("TOURNAMENT_INSERT_LOG_WINNERS"), ch->GetName(), ch->GetLevel(), ch->GetDesc()->GetHostName(), CTournamentPvP::instance().GetMyLives(ch->GetPlayerID()), CTournamentPvP::instance().ConvertTeamToString(idxTeamWinner).c_str());
					GiveReward(ch);
				}
			}
		}
		break;
		
		case TEAM_MEMBERS_B:
		{
			itertype(m_map_team_b) it = m_map_team_b.begin();
			LPCHARACTER ch = NULL;
			for (; it != m_map_team_b.end(); ++it)
			{
				ch = CHARACTER_MANAGER::instance().FindByPID(it->second);
				if (ch)
				{
					insert_winners(LC_TRANSLATE("TOURNAMENT_INSERT_LOG_WINNERS"), ch->GetName(), ch->GetLevel(), ch->GetDesc()->GetHostName(), CTournamentPvP::instance().GetMyLives(ch->GetPlayerID()), CTournamentPvP::instance().ConvertTeamToString(idxTeamWinner).c_str());
					GiveReward(ch);
				}
			}
		}
		break;
	}

	SendNoticeLine(LC_TRANSLATE("TOURNAMENT_ANNOUNCEMENT_WINNER_SUCCES"), CTournamentPvP::instance().ConvertTeamToString(idxTeamWinner).c_str(), idxSize);
	SetStatus(TOURNAMENT_STATE_FINISHED);
	DestroyAll();
}

bool CTournamentPvP::IsLimitedItem(LPCHARACTER ch, DWORD dwVnum)
{
	if (m_listForbidden.empty())
		return false;
	
	if (ch->GetMapIndex() != TOURNAMENT_MAP_INDEX)
		return false;
	
	if (std::find(m_listForbidden.begin(), m_listForbidden.end(), dwVnum) != m_listForbidden.end())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TRANSLATE("TOURNAMENT_MEMBER_CANNOT_USE_ITEM"), ITEM_MANAGER::instance().GetTable(dwVnum)->szLocaleName);
		return true;
	}
	
	return false;
}

int CTournamentPvP::GetIndexTeam(DWORD dwPID)
{
	itertype(m_map_team_a) it = m_map_team_a.find(dwPID);
	if (it != m_map_team_a.end())
		return TEAM_MEMBERS_A;
		
	itertype(m_map_team_b) it2 = m_map_team_b.find(dwPID);
	if (it2 != m_map_team_b.end())
		return TEAM_MEMBERS_B;
		
	return 0;
}

void CTournamentPvP::Register(DWORD dwPID)
{
	m_mapParticipants.insert(std::make_pair(dwPID, dwPID));
}

void CTournamentPvP::Respawn(LPCHARACTER ch)
{
	if (!ch)
		return;

	int teamIndex = CTournamentPvP::instance().GetIndexTeam(ch->GetPlayerID());
	if (teamIndex)
	{
		ch->Show(TOURNAMENT_MAP_INDEX, g_position_tournament[teamIndex - 1][0], g_position_tournament[teamIndex - 1][1]);
		ch->Stop();
	}
}

#ifdef ENABLE_EXTRA_LIVES_TOURNAMENT
bool CTournamentPvP::GetExistExtraLives(LPCHARACTER ch)
{
	return (ch->GetQuestFlag(FLAG_EXTRA_LIVES) > 0);
}

int CTournamentPvP::GetUsedCountExtraLives(LPCHARACTER ch)
{
	return ch->GetQuestFlag(FLAG_USED_COUNT_EXTRA_LIVES);
}

void CTournamentPvP::SetUsedCountExtraLives(LPCHARACTER ch, int val)
{
	ch->SetQuestFlag(FLAG_USED_COUNT_EXTRA_LIVES, val);
}
#endif

void CTournamentPvP::AppendLives(LPCHARACTER ch)
{
#ifdef ENABLE_EXTRA_LIVES_TOURNAMENT
	if (CTournamentPvP::instance().GetExistExtraLives(ch))
	{
		m_map_lives.insert(std::make_pair(ch->GetPlayerID(), TOURNAMENT_EXTRA_LIVES));
	}
	else
	{
		m_map_lives.insert(std::make_pair(ch->GetPlayerID(), TOURNAMENT_MAX_LIVES));
	}
#else
	m_map_lives.insert(std::make_pair(ch->GetPlayerID(), TOURNAMENT_MAX_LIVES));
#endif
}

void CTournamentPvP::DestroyAll()
{
	LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(TOURNAMENT_MAP_INDEX);
	if (pkSectreeMap)
	{
		struct FWarpToHome f;
		pkSectreeMap->for_each(f);
	}
	
	ClearSTDMap();
}

void CTournamentPvP::OnDisconnect(LPCHARACTER ch)
{
	if (!ch)
		return;
	
	if (ch->GetMapIndex() != TOURNAMENT_MAP_INDEX)
		return;

	DWORD dwPID = ch->GetPlayerID();
	m_mapParticipants.erase(dwPID);
	m_map_lives.erase(dwPID);
	m_map_team_a.erase(dwPID);
	m_map_team_b.erase(dwPID);
	ch->GoHome();
	RefreshWindow();
}

void CTournamentPvP::OnLogin(LPCHARACTER ch)
{
	if (!ch)
		return;
	
	if (ch->GetMapIndex() != TOURNAMENT_MAP_INDEX)
		return;
	
	if (GetStatus() == TOURNAMENT_STATE_OPEN)
	{
		ch->SetObserverMode(true);
		CTournamentPvP::instance().Register(ch->GetPlayerID());
	}
}

bool CTournamentPvP::RemoveLives(LPCHARACTER pkDead)	
{
	if (!pkDead)
		return false;

    itertype(m_map_lives) it = m_map_lives.find(pkDead->GetPlayerID());

	if (it == m_map_lives.end())
		return false;
	
	if (it->second == TOURNAMENT_LAST_LIFE)
	{
		pkDead->ChatPacket(CHAT_TYPE_INFO, LC_TRANSLATE("TOURNAMENT_MEMBER_FINISHED_LIFE_LINE_1"));
		pkDead->ChatPacket(CHAT_TYPE_INFO, LC_TRANSLATE("TOURNAMENT_MEMBER_FINISHED_LIFE_LINE_2"));	
		OnDisconnect(pkDead);
		return false;
	}

	it->second -= 1;
	m_map_lives[pkDead->GetPlayerID()] = it->second;
	RefreshWindow();
	return true;
}

void CTournamentPvP::OnKill(LPCHARACTER pkKiller, LPCHARACTER pkDead)
{
	if (!pkKiller || !pkDead)
		return;

	if (!pkKiller->IsPC())
		return;
	
	if (!pkDead->IsPC())
		return;
		
	if (pkKiller->GetMapIndex() != TOURNAMENT_MAP_INDEX)
		return;
	
	if (pkKiller->GetPKMode() == pkDead->GetPKMode())
		return;

#ifdef ENABLE_KILL_COUNTS_FOR_EACH_PLAYER	
	int KillCount = pkKiller->GetQuestFlag(FLAG_KILL_COUNT);
	KillCount += 1;
	pkKiller->SetQuestFlag(FLAG_KILL_COUNT, KillCount);

	CTournamentPvP::instance().InsertPlayerKillLogs(pkKiller);
#endif

	if (CTournamentPvP::instance().RemoveLives(pkDead))
	{
		pkDead->ChatPacket(CHAT_TYPE_INFO, LC_TRANSLATE("TOURNAMENT_MEMBER_REMAINING_LIFE"), m_map_lives[pkDead->GetPlayerID()]);
		CTournamentPvP::instance().Respawn(pkDead);
	}
}

void CTournamentPvP::PushBack(std::vector<DWORD>* m_vecCacheParticipants)
{
	itertype(m_mapParticipants) iter = m_mapParticipants.begin();
	for (; iter != m_mapParticipants.end(); ++iter)
		m_vecCacheParticipants->push_back(iter->second);
}

bool CTournamentPvP::CheckingStart()
{
	return false;
}

bool CTournamentPvP::TransferByCategory()
{
	std::vector<DWORD> m_vec_character, m_vecCacheParticipants;
	DWORD dwPID;
	LPCHARACTER ch = NULL;
	
	m_map_team_a.clear();
	m_map_team_b.clear();
	m_map_lives.clear();

	CTournamentPvP::instance().PushBack(&m_vec_character);

	srand(time(0));
	while (CTournamentPvP::instance().GetMembersTeamA() < TOURNAMENT_MAX_PLAYERS / 2)
	{
		dwPID = m_vec_character[rand() % m_vec_character.size()];
		while (std::find(m_vecCacheParticipants.begin(), m_vecCacheParticipants.end(), dwPID) != m_vecCacheParticipants.end())
		{
			dwPID = m_vec_character[rand() % m_vec_character.size()];
		}
		
		ch = CHARACTER_MANAGER::instance().FindByPID(dwPID);
		if (ch)
		{
			m_map_team_a.insert(std::make_pair(dwPID, dwPID));
			m_vecCacheParticipants.push_back(dwPID);
			m_mapParticipants.erase(dwPID);
		}
	}
	
	srand(time(0));
	while (CTournamentPvP::instance().GetMembersTeamB() < TOURNAMENT_MAX_PLAYERS / 2)
	{
		dwPID = m_vec_character[rand() % m_vec_character.size()];
		while (std::find(m_vecCacheParticipants.begin(), m_vecCacheParticipants.end(), dwPID) != m_vecCacheParticipants.end())
		{
			dwPID = m_vec_character[rand() % m_vec_character.size()];
		}

		ch = CHARACTER_MANAGER::instance().FindByPID(dwPID);
		if (ch)
		{
			m_map_team_b.insert(std::make_pair(dwPID, dwPID));
			m_vecCacheParticipants.push_back(dwPID);
			m_mapParticipants.erase(dwPID);
		}
	}

	CTournamentPvP::instance().TeleportMembers(m_map_team_a, TEAM_MEMBERS_A);
	CTournamentPvP::instance().TeleportMembers(m_map_team_b, TEAM_MEMBERS_B);
	// 新增返回语句：表示队伍分配和传送成功
	return true;
}

void CTournamentPvP::TeleportMembers(std::map<DWORD, DWORD> m_map_teams, DWORD index)
{
	itertype(m_map_teams) it = m_map_teams.begin();
	LPCHARACTER ch = NULL;

	for (; it != m_map_teams.end(); ++it)
	{
		ch = CHARACTER_MANAGER::instance().FindByPID(it->second);
		if (ch != NULL)
		{
			if (ch->IsObserverMode())
				ch->SetObserverMode(false);
			
			CTournamentPvP::instance().AppendLives(ch);
			ch->SetPKMode(CTournamentPvP::instance().GetAttackMode(index));
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TRANSLATE("TOURNAMENT_MEMBER_DIVIDED"), CTournamentPvP::instance().ConvertTeamToString(index).c_str());

#ifdef ENABLE_EXTRA_LIVES_TOURNAMENT			
			int iUsedCount = CTournamentPvP::instance().GetUsedCountExtraLives(ch);
			if (iUsedCount == TOURNAMENT_CAN_USED_MAX_EXTRA_LIVES - 1)
			{
				CTournamentPvP::instance().SetUsedCountExtraLives(ch, 0);
				ch->SetQuestFlag(FLAG_EXTRA_LIVES, 0);
			}
			else
			{
				CTournamentPvP::instance().SetUsedCountExtraLives(ch, iUsedCount + 1);
			}
#endif

			ch->Show(TOURNAMENT_MAP_INDEX, g_position_tournament[index - 1][0], g_position_tournament[index - 1][1]);
			ch->Stop();
		}
	}
}

void CTournamentPvP::Warp(LPCHARACTER ch)
{
	if (!ch)
		return;

	int iRandomPos = number(0, 2);
	ch->WarpSet(g_observers_position_tournament[iRandomPos][0], g_observers_position_tournament[iRandomPos][1]);
}

int CTournamentPvP::GetParticipants()
{
	return m_mapParticipants.size();
}

int CTournamentPvP::NeedMinParticipants()
{
	return TOURNAMENT_MIN_MEMBERS;
}

EVENTINFO(TournamentPvPInfoData)
{
	DWORD bSeconds;
	TournamentPvPInfoData() : bSeconds(0) {}
};

EVENTFUNC(tournament_timer)
{
	if (!event || !event->info)
		return 0;

	TournamentPvPInfoData* info = dynamic_cast<TournamentPvPInfoData*>(event->info);
	if (!info)
		return 0;
	
	if (info->bSeconds > 0) 
	{
		if (CTournamentPvP::instance().CanFinish())
		{
			CTournamentPvP::instance().GetTeamWinner();
			CTournamentPvP::instance().SetStatus(TOURNAMENT_STATE_FINISHED);	
			CTournamentPvP::instance().ClearRegisters();
			CTournamentPvP::instance().DestroyAll();
			CTournamentPvP::instance().StopTournamentUpdateEvent();	
			return 0;
		}
		
		if (info->bSeconds <= 10)
		{
			char buf[128];
			snprintf(buf, sizeof(buf), "<红蓝对抗赛> 距离比赛结束时间剩余：%d秒。", info->bSeconds);
			BroadcastNotice(buf);
		}

		--info->bSeconds;
		return PASSES_PER_SEC(1);
	}

	CTournamentPvP::instance().SetStatus(TOURNAMENT_STATE_FINISHED);	
	CTournamentPvP::instance().ClearRegisters();
	CTournamentPvP::instance().DestroyAll();
	CTournamentPvP::instance().StopTournamentUpdateEvent();	
	return 0;
}

bool CTournamentPvP::CanFinish()
{
	int iTeamA = GetMembersTeamA(), iTeamB = GetMembersTeamB();
	// 修复：给右侧的 && 表达式添加括号，明确优先级
	return ((iTeamA && iTeamB == TOURNAMENT_NO_MEMBERS) || (iTeamB && iTeamA == TOURNAMENT_NO_MEMBERS));
}

void CTournamentPvP::StopTournamentUpdateEvent()
{
	if (m_pkTournamentUpdateEvent)
	{
		event_cancel(&m_pkTournamentUpdateEvent);
		m_pkTournamentUpdateEvent = NULL;
	}
}

void CTournamentPvP::StartTournamentUpdateEvent()
{
	CTournamentPvP::instance().StopTournamentUpdateEvent();

	TournamentPvPInfoData* info = AllocEventInfo<TournamentPvPInfoData>();
	info->bSeconds = TOURNAMENT_DURATING_FIGHT;
	m_pkTournamentUpdateEvent = event_create(tournament_timer, info, 1);
}

int CTournamentPvP::StartFight()
{
	if (CTournamentPvP::instance().GetStatus() == TOURNAMENT_STATE_FIGHT)
		return 0;
	
	if (CTournamentPvP::instance().GetParticipants() == TOURNAMENT_NO_MEMBERS)
		return 1;
	
	if (CTournamentPvP::instance().GetParticipants() < TOURNAMENT_MIN_MEMBERS)
		return 2;

	std::vector<std::pair<DWORD, DWORD> > m_vecActors(m_mapParticipants.begin(), m_mapParticipants.end());
	TOURNAMENT_MAX_PLAYERS = CTournamentPvP::instance().GetParticipants();
	
	if (TOURNAMENT_MAX_PLAYERS % 2 != 0)
	{
		m_vecActors.resize(CTournamentPvP::instance().GetParticipants() - 1);
		
		DWORD dwFindByPID = m_vecActors[CTournamentPvP::instance().GetParticipants() - 1].second;
		LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(dwFindByPID);
		if (ch)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "<红蓝对抗赛> 比赛无法开始,因为参加的玩家双方人数不平等.");
			ch->GoHome();
		}
		
		m_mapParticipants = std::map<DWORD, DWORD>(m_vecActors.begin(), m_vecActors.end());
		TOURNAMENT_MAX_PLAYERS = CTournamentPvP::instance().GetParticipants();
	}

	CTournamentPvP::instance().StartTournamentUpdateEvent();
	CTournamentPvP::instance().TransferByCategory();
	CTournamentPvP::instance().SetStatus(TOURNAMENT_STATE_FIGHT);
	RefreshWindow();
	return 99;
}

namespace quest
{
	int tournament_map_index(lua_State* L)
	{
		lua_pushnumber(L, TOURNAMENT_MAP_INDEX);
		return 1;
	}
	
	int tournament_channel(lua_State* L)
	{
		lua_pushnumber(L, TOURNAMENT_NEED_CHANNEL);
		return 1;
	}
	
	int tournament_warp(lua_State* L)
	{
		LPCHARACTER pkChar = CQuestManager::instance().GetCurrentCharacterPtr();
		if (pkChar)
			CTournamentPvP::instance().Warp(pkChar);
		return 1;
	}
	
	int tournament_end_event_force(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (ch && ch->IsGM())
		{
			CTournamentPvP::instance().SetStatus(TOURNAMENT_STATE_FINISHED);	
			CTournamentPvP::instance().ClearRegisters();
			CTournamentPvP::instance().DestroyAll();
		}
		return 1;
	}	
	
	int tournament_get_participants(lua_State* L)
	{
		lua_pushnumber(L, CTournamentPvP::instance().GetParticipants());
		return 1;
	}
	
	int tournament_start_fight(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (ch && ch->IsGM())
			lua_pushnumber(L, CTournamentPvP::instance().StartFight());
		return 1;
	}
	
	int tournament_close(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (ch && ch->IsGM())
			CTournamentPvP::instance().SetStatus(TOURNAMENT_STATE_CLOSE);
		return 1;
	}
	
	int tournament_open(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (ch && ch->IsGM())
			CTournamentPvP::instance().SetStatus(TOURNAMENT_STATE_OPEN);
		return 1;
	}
	
	int tournament_need_min_participants(lua_State* L)
	{
		lua_pushnumber(L, CTournamentPvP::instance().NeedMinParticipants());
		return 1;
	}
	
	int tournament_get_duration(lua_State* L)
	{
		lua_pushnumber(L, TOURNAMENT_DURATING_FIGHT);
		return 1;
	}

	void RegisterTournamentPvPFunctionTable()
	{
		luaL_reg tournament_functions[] =
		{
			{	"need_min_participants",	tournament_need_min_participants},
			{	"participants",		tournament_get_participants	},
			{	"map_index",		tournament_map_index		},
			{	"get_duration",		tournament_get_duration		},
			{	"channel",			tournament_channel			},
			{	"warp",				tournament_warp				},
			{	"close",			tournament_close			},
			{	"open",				tournament_open				},
			{	"start_fight",		tournament_start_fight		},
			{	"end_event_force",	tournament_end_event_force	},
			{	NULL,				NULL						}
		};
		
		CQuestManager::instance().AddLuaFunctionTable("tournament", tournament_functions);
	}
}
#endif