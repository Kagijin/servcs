#include "stdafx.h"
#undef sys_err
#ifndef __WIN32__
	#define sys_err(fmt, args...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, ##args)
#else
	#define sys_err(fmt, ...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, __VA_ARGS__)
#endif

// #ifdef __NEW_EVENTS__
// #include "new_events.h"
#include "char.h"
#include "questlua.h"
#include "questmanager.h"
#include "tournament.h"

namespace quest
{
	int tournament_is_map(lua_State* L)
	{
		LPCHARACTER pkChar = CQuestManager::instance().GetCurrentCharacterPtr();
		if (pkChar && pkChar->GetMapIndex() == TOURNAMENT_MAP_INDEX)
			lua_pushboolean(L, true);
		else
			lua_pushboolean(L, false);
		return 1;
	}
	
	int tournament_register(lua_State* L)
	{
		LPCHARACTER pkChar = CQuestManager::instance().GetCurrentCharacterPtr();
		
		DWORD teamIdx = (DWORD)lua_tonumber(L, 1);
		CTournamentPvP::instance().Register(pkChar, teamIdx);
		return 1;
	}
	
	int tournament_warp(lua_State* L)
	{
		LPCHARACTER pkChar = CQuestManager::instance().GetCurrentCharacterPtr();
		if (pkChar)
			CTournamentPvP::instance().Warp(pkChar);
		return 1;
	}
	
	int tournament_delete_register(lua_State* L)
	{
		LPCHARACTER pkChar = CQuestManager::instance().GetCurrentCharacterPtr();
		if (pkChar)
		{
			if (CTournamentPvP::instance().IsRegister(pkChar, m_map_category_low))
				m_map_category_low.erase(pkChar->GetPlayerID());
			
			else if (CTournamentPvP::instance().IsRegister(pkChar, m_map_category_medium))
				m_map_category_medium.erase(pkChar->GetPlayerID());			
			
			else if (CTournamentPvP::instance().IsRegister(pkChar, m_map_category_hard))
				m_map_category_hard.erase(pkChar->GetPlayerID());			
		}
		return 1;
	}
	
	int tournament_info_timer(lua_State* L)
	{
		int typeInfo = (int)lua_tonumber(L, 1);
		int typeCat = (int)lua_tonumber(L, 2);
		
		lua_pushstring(L, CTournamentPvP::instance().ConvertTimeToString(typeInfo, typeCat).c_str());
		return 1;
	}
	
	int tournament_is_running(lua_State* L)
	{
		if (CTournamentPvP::instance().GetStatus() == TOURNAMENT_STARTED)
		{
			lua_pushboolean(L, true);
		}
		else {
			lua_pushboolean(L, false);
		}
		return 1;
	}	
	
	int tournament_observer(lua_State* L)
	{
		LPCHARACTER pkChar = CQuestManager::instance().GetCurrentCharacterPtr();
		pkChar->SetQuestFlag(FLAG_OBSERVER, 1);
		CTournamentPvP::instance().Warp(pkChar);
		return 1;
	}	
	
	int tournament_get_participants(lua_State* L)
	{
		int categoryIndex = (int)lua_tonumber(L, 1);
		
		int m_counter[TOURNAMENT_MAX_CATEGORY] =
		{
			CTournamentPvP::instance().GetParticipantsLow(),
			CTournamentPvP::instance().GetParticipantsMedium(),
			CTournamentPvP::instance().GetParticipantsHard()
		};
		
		lua_pushnumber(L, m_counter[categoryIndex - 1]);
		return 1;
	}
	
	int tournament_get_is_register(lua_State* L)
	{
		LPCHARACTER pkChar = CQuestManager::instance().GetCurrentCharacterPtr();
		
		if (CTournamentPvP::instance().IsRegister(pkChar, m_map_category_low) || CTournamentPvP::instance().IsRegister(pkChar, m_map_category_medium) || CTournamentPvP::instance().IsRegister(pkChar, m_map_category_hard)) {
			lua_pushboolean(L, true);
		}
		else {
			lua_pushboolean(L, false);
		}
		return 1;
	}
	
	int tournament_info_current_timer(lua_State* L)
	{
		time_t currentTime;
		struct tm *localTime;
		time(&currentTime);
		localTime = localtime(&currentTime);
		lua_pushstring(L, asctime(localTime));
		return 1;
	}

	void RegisterTournamentPvPFunctionTable()
	{
		luaL_reg tournament_functions[] =
		{
			{	"get_is_register",	tournament_get_is_register	},
			{	"participants",		tournament_get_participants	},
			{	"is_map",			tournament_is_map			},
			{	"register",			tournament_register			},
			{	"warp",				tournament_warp				},
			{	"is_running",		tournament_is_running		},
			{	"observer",			tournament_observer			},
			{	"info_timer",		tournament_info_timer		},
			{	"info_current_timer", tournament_info_current_timer },
			{	"delete_register",	tournament_delete_register	},
			{	NULL,		NULL					}
		};
		
		CQuestManager::instance().AddLuaFunctionTable("tournament", tournament_functions);
	}
}