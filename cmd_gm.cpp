#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "item_manager.h"
#include "sectree_manager.h"
#include "mob_manager.h"
#include "packet.h"
#include "cmd.h"
#include "regen.h"
#include "guild.h"
#include "guild_manager.h"
#include "p2p.h"
#include "buffer_manager.h"
#include "fishing.h"
#include "mining.h"
#include "questmanager.h"
#include "vector.h"
#include "affect.h"
#include "db.h"
#include "priv_manager.h"
#include "building.h"
#include "battle.h"
#include "arena.h"
#include "start_position.h"
#include "party.h"
#include "BattleArena.h"

#include "unique_item.h"
#include "DragonSoul.h"
#include "../../common/Controls.h"
#ifdef ENABLE_OFFLINE_SHOP
#include "new_offlineshop.h"
#include "new_offlineshop_manager.h"
#endif
#ifdef ENABLE_HWID
#include "hwid_manager.h"
#endif
#ifdef ENABLE_NEW_PET_SYSTEM
#include "New_PetSystem.h"
#endif

// ADD_COMMAND_SLOW_STUN
enum
{
	COMMANDAFFECT_STUN,
	COMMANDAFFECT_SLOW,
};

void Command_ApplyAffect(LPCHARACTER ch, const char* argument, const char* affectName, int cmdAffect)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, "用法: %s <name>", affectName);
		return;
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);
	if (!tch)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, "%s 和你所在的地图不相同", arg1);
		return;
	}

	switch (cmdAffect)
	{
	case COMMANDAFFECT_STUN:
		SkillAttackAffect(tch, 1000, IMMUNE_STUN, AFFECT_STUN, POINT_NONE, 0, AFF_STUN, 30, "GM_STUN");
		break;
	case COMMANDAFFECT_SLOW:
		SkillAttackAffect(tch, 1000, IMMUNE_SLOW, AFFECT_SLOW, POINT_MOV_SPEED, -30, AFF_SLOW, 30, "GM_SLOW");
		break;
	}

	ch->ChatPacket(CHAT_TYPE_INFO, "%s %s", arg1, affectName);
}
// END_OF_ADD_COMMAND_SLOW_STUN

#ifdef TOURNAMENT_PVP_SYSTEM
ACMD(do_warp_tournament_map)
{
	if (!ch)
		return;

	ch->WarpSet(100400, 440000);
}
#endif

ACMD(do_stun)
{
	Command_ApplyAffect(ch, argument, "stun", COMMANDAFFECT_STUN);
}

ACMD(do_slow)
{
	Command_ApplyAffect(ch, argument, "slow", COMMANDAFFECT_SLOW);
}

ACMD(do_transfer)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "用法：/transfer <name>");
		return;
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);
	if (!tch)
	{
		CCI* pkCCI = P2P_MANAGER::instance().Find(arg1);

		if (pkCCI)
		{
			if (pkCCI->bChannel != g_bChannel)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "目标(%s) 在 %d个通道 (当前通道 %d)", arg1, pkCCI->bChannel, g_bChannel);
				return;
			}
			TPacketGGTransfer pgg;

			pgg.bHeader = HEADER_GG_TRANSFER;
			strlcpy(pgg.szName, arg1, sizeof(pgg.szName));
			pgg.lX = ch->GetX();
			pgg.lY = ch->GetY();

			P2P_MANAGER::instance().Send(&pgg, sizeof(TPacketGGTransfer));
			ch->ChatPacket(CHAT_TYPE_INFO, "已传送.");
		}
		else
			ch->ChatPacket(CHAT_TYPE_INFO, "(%s)不在线或没有该角色", arg1);

		return;
	}

	if (ch == tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "已传送到身边");
		return;
	}

	//tch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY(), ch->GetZ());
	tch->WarpSet(ch->GetX(), ch->GetY(), ch->GetMapIndex());
}

// LUA_ADD_GOTO_INFO
struct GotoInfo
{
	std::string 	st_name;

	BYTE 	empire;
	int 	mapIndex;
	DWORD 	x, y;

	GotoInfo()
	{
		st_name = "";
		empire = 0;
		mapIndex = 0;

		x = 0;
		y = 0;
	}
	GotoInfo(const GotoInfo& c_src)
	{
		__copy__(c_src);
	}
	void operator = (const GotoInfo& c_src)
	{
		__copy__(c_src);
	}
	void __copy__(const GotoInfo& c_src)
	{
		st_name = c_src.st_name;
		empire = c_src.empire;
		mapIndex = c_src.mapIndex;

		x = c_src.x;
		y = c_src.y;
	}
};

static std::vector<GotoInfo> gs_vec_gotoInfo;

void CHARACTER_AddGotoInfo(const std::string& c_st_name, BYTE empire, int mapIndex, DWORD x, DWORD y)
{
	GotoInfo newGotoInfo;
	newGotoInfo.st_name = c_st_name;
	newGotoInfo.empire = empire;
	newGotoInfo.mapIndex = mapIndex;
	newGotoInfo.x = x;
	newGotoInfo.y = y;
	gs_vec_gotoInfo.push_back(newGotoInfo);
}

bool FindInString(const char* c_pszFind, const char* c_pszIn)
{
	const char* c = c_pszIn;
	const char* p;

	p = strchr(c, '|');

	if (!p)
		return (0 == strncasecmp(c_pszFind, c_pszIn, strlen(c_pszFind)));
	else
	{
		char sz[64 + 1];

		do
		{
			strlcpy(sz, c, MIN(sizeof(sz), (p - c) + 1));

			if (!strncasecmp(c_pszFind, sz, strlen(c_pszFind)))
				return true;

			c = p + 1;
		} while ((p = strchr(c, '|')));

		strlcpy(sz, c, sizeof(sz));

		if (!strncasecmp(c_pszFind, sz, strlen(c_pszFind)))
			return true;
	}

	return false;
}

bool CHARACTER_GoToName(LPCHARACTER ch, BYTE empire, int mapIndex, const char* gotoName)
{
	std::vector<GotoInfo>::iterator i;
	for (i = gs_vec_gotoInfo.begin(); i != gs_vec_gotoInfo.end(); ++i)
	{
		const GotoInfo& c_eachGotoInfo = *i;

		if (mapIndex != 0)
		{
			if (mapIndex != c_eachGotoInfo.mapIndex)
				continue;
		}
		else if (!FindInString(gotoName, c_eachGotoInfo.st_name.c_str()))
			continue;

		if (c_eachGotoInfo.empire == 0 || c_eachGotoInfo.empire == empire)
		{
			int x = c_eachGotoInfo.x * 100;
			int y = c_eachGotoInfo.y * 100;

			ch->ChatPacket(CHAT_TYPE_INFO, "You warp to ( %d, %d )", x, y);
			ch->WarpSet(x, y);
			ch->Stop();
			return true;
		}
	}
	return false;
}

// END_OF_LUA_ADD_GOTO_INFO

ACMD(do_goto)
{
	char arg1[256], arg2[256];
	int x = 0, y = 0, z = 0;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 && !*arg2)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "用法: /goto <x 坐标> <y 坐标>");
		return;
	}

	if (isnhdigit(*arg1) && isnhdigit(*arg2))
	{
		str_to_number(x, arg1);
		str_to_number(y, arg2);

		PIXEL_POSITION p;

		if (SECTREE_MANAGER::instance().GetMapBasePosition(ch->GetX(), ch->GetY(), p))
		{
			x += p.x / 100;
			y += p.y / 100;
		}

		ch->ChatPacket(CHAT_TYPE_INFO, "已移动到坐标 ( %d, %d )", x, y);
	}
	else
	{
		int mapIndex = 0;
		BYTE empire = 0;

		if (*arg1 == '#')
			str_to_number(mapIndex, (arg1 + 1));

		if (*arg2 && isnhdigit(*arg2))
		{
			str_to_number(empire, arg2);
			empire = MINMAX(1, empire, 3);
		}
		else
			empire = ch->GetEmpire();

		if (CHARACTER_GoToName(ch, empire, mapIndex, arg1))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "找不到地图命令：/goto <地图名> [国家]国家代码1,2,3查询当前地图编码/state");
			return;
		}

		return;

		/*
		   int iMapIndex = 0;
		   for (int i = 0; aWarpInfo[i].c_pszName != NULL; ++i)
		   {
		   if (iMapIndex != 0)
		   {
		   if (iMapIndex != aWarpInfo[i].iMapIndex)
		   continue;
		   }
		   else if (!FindInString(arg1, aWarpInfo[i].c_pszName))
		   continue;

		   if (aWarpInfo[i].bEmpire == 0 || aWarpInfo[i].bEmpire == bEmpire)
		   {
		   x = aWarpInfo[i].x * 100;
		   y = aWarpInfo[i].y * 100;

		   ch->ChatPacket(CHAT_TYPE_INFO, "You warp to ( %d, %d )", x, y);
		   ch->WarpSet(x, y);
		   ch->Stop();
		   return;
		   }
		   }
		 */
	}

	x *= 100;
	y *= 100;

	ch->Show(ch->GetMapIndex(), x, y, z);
	ch->Stop();
}

ACMD(do_warp)
{
	char arg1[256], arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "用法：/warp <角色名称>或坐标 <x> <y>");
		return;
	}

	int x = 0, y = 0;
#ifdef ENABLE_CMD_WARP_IN_DUNGEON
	int mapIndex = 0;
#endif

	if (isnhdigit(*arg1) && isnhdigit(*arg2))
	{
		str_to_number(x, arg1);
		str_to_number(y, arg2);
	}
	else
	{
		LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);

		if (NULL == tch)
		{
			const CCI* pkCCI = P2P_MANAGER::instance().Find(arg1);

			if (NULL != pkCCI)
			{
#ifndef ENABLE_P2P_WARP
				if (pkCCI->bChannel != g_bChannel)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "目标(%s)在%d频道(我的频道%d)", arg1, pkCCI->bChannel, g_bChannel);
					return;
				}
#endif
				ch->WarpToPID(pkCCI->dwPID);
			}
			else
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "输入的名字(%s)不存在", arg1);
			}

			return;
		}
		else
		{
			x = tch->GetX() / 100;
			y = tch->GetY() / 100;
#ifdef ENABLE_CMD_WARP_IN_DUNGEON
			mapIndex = tch->GetMapIndex();
#endif
		}
	}

	x *= 100;
	y *= 100;

#ifdef ENABLE_CMD_WARP_IN_DUNGEON
	ch->ChatPacket(CHAT_TYPE_INFO, "已传送(%d, %d, %d )", x, y, mapIndex);
	ch->WarpSet(x, y, mapIndex);
#else
	ch->ChatPacket(CHAT_TYPE_INFO, "已传送(%d, %d )", x, y);
	ch->WarpSet(x, y);
#endif
	ch->Stop();
}

#ifdef ENABLE_NEWSTUFF
ACMD(do_rewarp)
{
	ch->ChatPacket(CHAT_TYPE_INFO, "已传送( %d, %d )", ch->GetX(), ch->GetY());
	ch->WarpSet(ch->GetX(), ch->GetY());
	ch->Stop();
}
#endif

ACMD(do_item)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, "例如：/i 19 或使用中文刷-例如：/i 短刀+9 ->请参照物品大全代码");
		return;
	}

	int iCount = 1;

	if (*arg2)
	{
		str_to_number(iCount, arg2);
#ifndef NEW_TYPE_DO_ITEM
		iCount = MINMAX(1, iCount, g_bItemCountLimit);
#else
		iCount = MAX(1, abs(iCount));
#endif
	}

	DWORD dwVnum;

	if (isnhdigit (*arg1))
	{
		str_to_number (dwVnum, arg1);
	}
	else
	{
		if (!ITEM_MANAGER::instance().GetVnum(arg1, dwVnum))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "%u 输入有误,(%s)或不存在.", dwVnum, arg1);
			return;
		}
	}

	

#ifndef NEW_TYPE_DO_ITEM
	LPITEM item = ITEM_MANAGER::instance().CreateItem(dwVnum, iCount, 0, true);
	if (item)
	{
		if (item->IsDragonSoul())
		{
			int iEmptyPos = ch->GetEmptyDragonSoulInventory(item);

			if (iEmptyPos != -1)
			{
				item->AddToCharacter(ch, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyPos));
			}
			else
			{
				M2_DESTROY_ITEM(item);
				if (!ch->DragonSoul_IsQualified())
				{
					ch->ChatPacket (CHAT_TYPE_INFO, "物品栏没有足够的位置.");
				}
				else
					ch->ChatPacket (CHAT_TYPE_INFO, "物品栏没有足够的位置.");
			}
		}
#ifdef ENABLE_SPECIAL_STORAGE
		else if (item->IsUpgradeItem())
		{
			int iEmptyPos = ch->GetEmptyUpgradeInventory(item);

			if (iEmptyPos != -1)
			{
				item->AddToCharacter(ch, TItemPos(UPGRADE_INVENTORY, iEmptyPos));
			}
			else
			{
				M2_DESTROY_ITEM(item);
				ch->ChatPacket(CHAT_TYPE_INFO, "物品栏没有足够的位置.");
			}
		}
		else if (item->IsBook())
		{
			int iEmptyPos = ch->GetEmptyBookInventory(item);

			if (iEmptyPos != -1)
			{
				item->AddToCharacter(ch, TItemPos(BOOK_INVENTORY, iEmptyPos));
			}
			else
			{
				M2_DESTROY_ITEM(item);
				ch->ChatPacket(CHAT_TYPE_INFO, "物品栏没有足够的位置.");
			}
		}
		else if (item->IsStone())
		{
			int iEmptyPos = ch->GetEmptyStoneInventory(item);

			if (iEmptyPos != -1)
			{
				item->AddToCharacter(ch, TItemPos(STONE_INVENTORY, iEmptyPos));
			}
			else
			{
				M2_DESTROY_ITEM(item);
				ch->ChatPacket(CHAT_TYPE_INFO, "物品栏没有足够的位置.");
			}
		}
		else if (item->IsChest())
		{
			int iEmptyPos = ch->GetEmptyChestInventory(item);

			if (iEmptyPos != -1)
			{
				item->AddToCharacter(ch, TItemPos(CHEST_INVENTORY, iEmptyPos));
			}
			else
			{
				M2_DESTROY_ITEM(item);
				ch->ChatPacket(CHAT_TYPE_INFO, "物品栏没有足够的位置.");
			}
		}
#endif
		else
		{
			int iEmptyPos = ch->GetEmptyInventory(item->GetSize());

			if (iEmptyPos != -1)
			{
				item->AddToCharacter(ch, TItemPos(INVENTORY, iEmptyPos));
			}
			else
			{
				M2_DESTROY_ITEM(item);
				ch->ChatPacket(CHAT_TYPE_INFO, "物品栏没有足够的位置.");
			}
		}
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "%u 输入有误或%s不存在.", dwVnum, arg1);
	}
#else
	while (iCount > 0)
	{
		LPITEM item = ITEM_MANAGER::instance().CreateItem(dwVnum, iCount, 0, true);
		if (!item) 
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "该物品不存在.");
			return;
		}

		int iEmptyPos = -1;
#ifdef ENABLE_SPECIAL_STORAGE
		if (item->IsUpgradeItem())
			iEmptyPos = ch->GetEmptyUpgradeInventory(item);
		else if (item->IsBook())
			iEmptyPos = ch->GetEmptyBookInventory(item);
		else if (item->IsStone())
			iEmptyPos = ch->GetEmptyStoneInventory(item);
		else if (item->IsChest())
			iEmptyPos = ch->GetEmptyChestInventory(item);
		else if (item->IsDragonSoul())
			iEmptyPos = ch->GetEmptyDragonSoulInventory(item);
		else
#endif
			iEmptyPos = ch->GetEmptyInventory(item->GetSize());
		

		if (iEmptyPos == -1) 
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "物品栏空间不足.");
			ITEM_MANAGER::instance().DestroyItem(item);
			return;
		}

		if (item->IsDragonSoul())
			item->AddToCharacter(ch, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyPos));
#ifdef ENABLE_SPECIAL_STORAGE
		else if (item->IsUpgradeItem())
			item->AddToCharacter(ch, TItemPos(UPGRADE_INVENTORY, iEmptyPos));
		else if (item->IsBook())
			item->AddToCharacter(ch, TItemPos(BOOK_INVENTORY, iEmptyPos));
		else if (item->IsStone())
			item->AddToCharacter(ch, TItemPos(STONE_INVENTORY, iEmptyPos));
		else if (item->IsChest())
			item->AddToCharacter(ch, TItemPos(CHEST_INVENTORY, iEmptyPos));
#endif
		else
			item->AddToCharacter(ch, TItemPos(INVENTORY, iEmptyPos));

		iCount -= item->IsStackable() ? SET_MAX_COUNT : 1;
	}
#endif
}

ACMD(do_group_random)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, "用法-例如：/grrandom 101 ->刷组怪物参照:group.txt");
		return;
	}

	DWORD dwVnum = 0;
	str_to_number(dwVnum, arg1);
	CHARACTER_MANAGER::instance().SpawnGroupGroup(dwVnum, ch->GetMapIndex(), ch->GetX() - 500, ch->GetY() - 500, ch->GetX() + 500, ch->GetY() + 500);
}

ACMD(do_group)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, "用法-例如：/group 101 ->刷组怪物参照:group.txt");
		return;
	}

	DWORD dwVnum = 0;
	str_to_number(dwVnum, arg1);
	CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, ch->GetMapIndex(), ch->GetX() - 500, ch->GetY() - 500, ch->GetX() + 500, ch->GetY() + 500);
}

ACMD(do_mob_coward)
{
	char	arg1[256], arg2[256];
	DWORD	vnum = 0;
	LPCHARACTER	tch;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, "用法：/mc 101 ->这样刷出的怪物会不断移动");
		return;
	}

	const CMob* pkMob;

	if (isdigit(*arg1))
	{
		str_to_number(vnum, arg1);

		if ((pkMob = CMobManager::instance().Get(vnum)) == NULL)
			vnum = 0;
	}
	else
	{
		pkMob = CMobManager::Instance().Get(arg1, true);

		if (pkMob)
			vnum = pkMob->m_table.dwVnum;
	}

	if (vnum == 0)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, "输入有误,不存在的怪物编码: %s", arg1);
		return;
	}

	int iCount = 1;
	if (*arg2)
	{
		str_to_number (iCount, arg2);
		iCount = MINMAX (1, iCount, 50);   // DevFix 121
	}

	if (test_server)
	{
		iCount = MIN (50, iCount);
	}
	else
	{
		iCount = MIN (40, iCount);
	}

	while (iCount--)
	{
		tch = CHARACTER_MANAGER::instance().SpawnMobRange (vnum, ch->GetMapIndex(), ch->GetX() - number (200, 750), ch->GetY() - number (200, 750), ch->GetX() + number (200, 750), ch->GetY() + number (200, 750), true, pkMob->m_table.bType == CHAR_TYPE_STONE);
		if (tch)
		{
			tch->SetCoward();
		}
	}
}

ACMD(do_mob_map)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "用法: /mm <vnum>");
		return;
	}

	DWORD vnum = 0;
	str_to_number(vnum, arg1);
	LPCHARACTER tch = CHARACTER_MANAGER::instance().SpawnMobRandomPosition(vnum, ch->GetMapIndex());

	if (tch)
		ch->ChatPacket(CHAT_TYPE_INFO, "%s在坐标%dx%d位置生成", tch->GetName(), tch->GetX(), tch->GetY());
	else
		ch->ChatPacket(CHAT_TYPE_INFO, "Spawn failed.");
}

ACMD(do_mob_aggresive)
{
	char	arg1[256], arg2[256];
	DWORD	vnum = 0;
	LPCHARACTER	tch;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, "用法：/mob 101 或中文/m 幼狼  ->请参照怪物大全代码");
		return;
	}

	const CMob* pkMob;

	if (isdigit(*arg1))
	{
		str_to_number(vnum, arg1);

		if ((pkMob = CMobManager::instance().Get(vnum)) == NULL)
			vnum = 0;
	}
	else
	{
		pkMob = CMobManager::Instance().Get(arg1, true);

		if (pkMob)
			vnum = pkMob->m_table.dwVnum;
	}

	if (vnum == 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "服务器不存在编码(%s)", arg1);
		return;
	}

	int iCount = 1;
	if (*arg2)
	{
		str_to_number (iCount, arg2);
		iCount = MINMAX (1, iCount, 50);   // DevFix 121
	}

	if (test_server)
	{
		iCount = MIN (50, iCount);
	}
	else
	{
		iCount = MIN (50, iCount);
	}

	while (iCount--)
	{
		tch = CHARACTER_MANAGER::instance().SpawnMobRange (vnum, ch->GetMapIndex(), ch->GetX() - number (200, 750), ch->GetY() - number (200, 750), ch->GetX() + number (200, 750), ch->GetY() + number (200, 750), true, pkMob->m_table.bType == CHAR_TYPE_STONE);
		if (tch)
		{
			tch->SetAggressive();
		}
	}
}

ACMD (do_mob)
{
	char arg1[256], arg2[256];
	DWORD vnum = 0;
	two_arguments (argument, arg1, sizeof (arg1), arg2, sizeof (arg2));
	if (!*arg1)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, "用法：/mob 101 或中文/m 幼狼  ->请参照怪物大全代码");
		return;
	}

	const CMob* pkMob = NULL;
	if (isnhdigit (*arg1))
	{
		str_to_number (vnum, arg1);
		if ((pkMob = CMobManager::instance().Get (vnum)) == NULL)
		{
			vnum = 0;
		}
	}
	else
	{
		pkMob = CMobManager::Instance().Get (arg1, true);
		if (pkMob)
		{
			vnum = pkMob->m_table.dwVnum;
		}
	}

	if (vnum == 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "服务器不存在怪物编码(%s)", arg1);
		return;
	}

	int iCount = 1;
	if (*arg2)
	{
		str_to_number (iCount, arg2);
		iCount = MINMAX (1, iCount, 50);   // DevFix 121
	}

	if (test_server)
	{
		iCount = MIN (50, iCount);
	}
	else
	{
		iCount = MIN (50, iCount);
	}

	while (iCount--)
	{
		CHARACTER_MANAGER::instance().SpawnMobRange (vnum, ch->GetMapIndex(), ch->GetX() - number (200, 750), ch->GetY() - number (200, 750), ch->GetX() + number (200, 750), ch->GetY() + number (200, 750), true, pkMob->m_table.bType == CHAR_TYPE_STONE);
	}
}

ACMD(do_mob_ld)
{
	char	arg1[256], arg2[256], arg3[256], arg4[256];
	DWORD	vnum = 0;

	two_arguments(two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3), arg4, sizeof(arg4));

	if (!*arg1)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, "例如：/mob 101 或/mob 幼狼 -->请参照怪物大全代码");
		return;
	}

	const CMob* pkMob = NULL;

	if (isnhdigit(*arg1))
	{
		str_to_number(vnum, arg1);

		if ((pkMob = CMobManager::instance().Get(vnum)) == NULL)
			vnum = 0;
	}
	else
	{
		pkMob = CMobManager::Instance().Get(arg1, true);

		if (pkMob)
			vnum = pkMob->m_table.dwVnum;
	}

	if (vnum == 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "服务器不存在: %s 怪物编码", arg1);
		return;
	}

	int dir = 1;
	long x = 0, y = 0;

	if (*arg2)
		str_to_number(x, arg2);
	if (*arg3)
		str_to_number(y, arg3);
	if (*arg4)
		str_to_number(dir, arg4);

	CHARACTER_MANAGER::instance().SpawnMob(vnum,
		ch->GetMapIndex(),
		x * 100,
		y * 100,
		ch->GetZ(),
		pkMob->m_table.bType == CHAR_TYPE_STONE,
		dir);
}

struct FuncPurge
{
	LPCHARACTER m_pkGM;
	bool	m_bAll;

	FuncPurge(LPCHARACTER ch) : m_pkGM(ch), m_bAll(false)
	{
	}

	void operator () (LPENTITY ent)
	{
		if (!ent->IsType(ENTITY_CHARACTER))
			return;

		LPCHARACTER pkChr = (LPCHARACTER)ent;

		int iDist = DISTANCE_APPROX(pkChr->GetX() - m_pkGM->GetX(), pkChr->GetY() - m_pkGM->GetY());

		if (!m_bAll && iDist >= 1000)
			return;
#ifdef ENABLE_BOT_PLAYER
		if (pkChr->IsBotCharacter())
			return;
#endif
		if (pkChr->IsNPC() && !pkChr->IsPet()
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
			&& !pkChr->IsMount()
#endif
#ifdef ENABLE_NEW_PET_SYSTEM
			&& !pkChr->IsNewPet()
#endif
			&& pkChr->GetRider() == NULL
			)
		{
			M2_DESTROY_CHARACTER(pkChr);
		}
	}
};

ACMD(do_purge)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	FuncPurge func(ch);

	if (*arg1 && !strcmp(arg1, "map"))
	{
		CHARACTER_MANAGER::instance().DestroyCharacterInMap(ch->GetMapIndex());
	}
	else
	{
		if (*arg1 && !strcmp(arg1, "all"))
			func.m_bAll = true;
		LPSECTREE sectree = ch->GetSectree();
		if (sectree) // #431
			sectree->ForEachAround(func);
		else
			sys_err("PURGE_ERROR.NULL_SECTREE(mapIndex=%d, pos=(%d, %d)", ch->GetMapIndex(), ch->GetX(), ch->GetY());
	}
}

#define ENABLE_CMD_IPURGE_EX
ACMD(do_item_purge)
{
#ifdef ENABLE_CMD_IPURGE_EX
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	if (!*arg1)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, "用法: /ipurge <GM清理身上物品>");
		ch->ChatPacket (CHAT_TYPE_INFO, "可用操作列表：");
		ch->ChatPacket (CHAT_TYPE_INFO, " 输入: /ip all   <是清空所有物品>");
		ch->ChatPacket (CHAT_TYPE_INFO, " 输入: /ip inv   <是清空背包中的物品>");
		ch->ChatPacket (CHAT_TYPE_INFO, " 输入: /ip equip <是清空身上装备物品>");
		ch->ChatPacket (CHAT_TYPE_INFO, " 输入: /ip ds    <是清空龙魂石库存>");
		// ch->ChatPacket (CHAT_TYPE_INFO, " 输入: /ip belt  <是清空腰带物品>");
#ifdef ENABLE_SPECIAL_STORAGE
		ch->ChatPacket (CHAT_TYPE_INFO, " 输入: /ip sp    <是清空分类储存物品>");
#endif
		return;
	}

	if (ch->GetWear(WEAR_NEW_PET))
	{
		ch->ChatPacket (CHAT_TYPE_INFO, " 背包中有新宠物或法师伴侣所以不能清理");
		ch->ChatPacket (CHAT_TYPE_INFO, " 将此物品移除后尝试.");
		return;
	}

	int         i;
	LPITEM      item;

	std::string strArg(arg1);
	if (!strArg.compare(0, 3, "all"))
	{
		for (i = 0; i < INVENTORY_AND_EQUIP_SLOT_MAX; ++i)
		{
			if ((item = ch->GetInventoryItem(i)))
			{
				ITEM_MANAGER::instance().RemoveItem(item, "PURGE");
				ch->SyncQuickslot(QUICKSLOT_TYPE_ITEM, i, UINT16_MAX);
			}
		}
		for (i = 0; i < DRAGON_SOUL_INVENTORY_MAX_NUM; ++i)
		{
			if ((item = ch->GetItem(TItemPos(DRAGON_SOUL_INVENTORY, i))))
			{
				ITEM_MANAGER::instance().RemoveItem(item, "PURGE");
			}
		}
#ifdef ENABLE_SPECIAL_STORAGE
		for (i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			if ((item = ch->GetItem(TItemPos(UPGRADE_INVENTORY, i))))
			{
				ITEM_MANAGER::instance().RemoveItem(item, "PURGE");
			}
		}
		for (i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			if ((item = ch->GetItem(TItemPos(BOOK_INVENTORY, i))))
			{
				ITEM_MANAGER::instance().RemoveItem(item, "PURGE");
			}
		}
		for (i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			if ((item = ch->GetItem(TItemPos(STONE_INVENTORY, i))))
			{
				ITEM_MANAGER::instance().RemoveItem(item, "PURGE");
			}
		}
		for (i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			if ((item = ch->GetItem(TItemPos(CHEST_INVENTORY, i))))
			{
				ITEM_MANAGER::instance().RemoveItem(item, "PURGE");
			}
		}
#endif
	}
	else if (!strArg.compare(0, 3, "inv"))
	{
		for (i = 0; i < INVENTORY_MAX_NUM; ++i)
		{
			if ((item = ch->GetInventoryItem(i)))
			{
				ITEM_MANAGER::instance().RemoveItem(item, "PURGE");
				ch->SyncQuickslot(QUICKSLOT_TYPE_ITEM, i, UINT16_MAX);
			}
		}
	}
#ifdef ENABLE_SPECIAL_STORAGE
	else if (!strArg.compare(0, 7, "special") || !strArg.compare(0, 2, "sp"))
	{
		for (i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			if ((item = ch->GetItem(TItemPos(UPGRADE_INVENTORY, i))))
			{
				ITEM_MANAGER::instance().RemoveItem(item, "PURGE");
			}
			if ((item = ch->GetItem(TItemPos(BOOK_INVENTORY, i))))
			{
				ITEM_MANAGER::instance().RemoveItem(item, "PURGE");
			}
			if ((item = ch->GetItem(TItemPos(STONE_INVENTORY, i))))
			{
				ITEM_MANAGER::instance().RemoveItem(item, "PURGE");
			}
			if ((item = ch->GetItem(TItemPos(CHEST_INVENTORY, i))))
			{
				ITEM_MANAGER::instance().RemoveItem(item, "PURGE");
			}
		}
	}
#endif
	else if (!strArg.compare(0, 5, "equip"))
	{
		for (i = 0; i < WEAR_MAX_NUM; ++i)
		{
			if ((item = ch->GetInventoryItem(INVENTORY_MAX_NUM + i)))
			{
				ITEM_MANAGER::instance().RemoveItem(item, "PURGE");
				ch->SyncQuickslot(QUICKSLOT_TYPE_ITEM, INVENTORY_MAX_NUM + i, UINT16_MAX);
			}
		}
	}
	else if (!strArg.compare(0, 6, "dragon") || !strArg.compare(0, 2, "ds"))
	{
		for (i = 0; i < DRAGON_SOUL_INVENTORY_MAX_NUM; ++i)
		{
			if ((item = ch->GetItem(TItemPos(DRAGON_SOUL_INVENTORY, i))))
			{
				ITEM_MANAGER::instance().RemoveItem(item, "PURGE");
			}
		}
	}
	else if (!strArg.compare(0, 4, "belt"))
	{
		for (i = 0; i < BELT_INVENTORY_SLOT_COUNT; ++i)
		{
			if ((item = ch->GetInventoryItem(BELT_INVENTORY_SLOT_START + i)))
			{
				ITEM_MANAGER::instance().RemoveItem(item, "PURGE");
				ch->SyncQuickslot(QUICKSLOT_TYPE_ITEM, BELT_INVENTORY_SLOT_START + i, UINT16_MAX);
			}
		}
	}
#else
	int         i;
	LPITEM      item;

	for (i = 0; i < INVENTORY_AND_EQUIP_SLOT_MAX; ++i)
	{
		if ((item = ch->GetInventoryItem(i)))
		{
			ITEM_MANAGER::instance().RemoveItem(item, "PURGE");
			ch->SyncQuickslot(QUICKSLOT_TYPE_ITEM, i, 255);
		}
	}
	for (i = 0; i < DRAGON_SOUL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = ch->GetItem(TItemPos(DRAGON_SOUL_INVENTORY, i))))
		{
			ITEM_MANAGER::instance().RemoveItem(item, "PURGE");
		}
	}
#endif
}

ACMD(do_state)
{
	char arg1[256];
	LPCHARACTER tch;

	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		if (arg1[0] == '#')
		{
			tch = CHARACTER_MANAGER::instance().Find(strtoul(arg1 + 1, NULL, 10));
		}
		else
		{
			LPDESC d = DESC_MANAGER::instance().FindByCharacterName(arg1);

			if (!d)
				tch = NULL;
			else
				tch = d->GetCharacter();
		}
	}
	else
		tch = ch;

	if (!tch)
		return;

	char buf[256];

	snprintf(buf, sizeof(buf), "%s's State: ", tch->GetName());

	if (tch->IsPosition(POS_FIGHTING))
		strlcat(buf, "战斗", sizeof(buf));
	else if (tch->IsPosition(POS_DEAD))
		strlcat(buf, "死亡", sizeof(buf));
	else
		strlcat(buf, "正常", sizeof(buf));

	if (ch->GetShop())
		strlcat(buf, ", 开店", sizeof(buf));

	if (ch->GetExchange())
		strlcat(buf, ", 交易", sizeof(buf));

#ifdef ENABLE_AURA_SYSTEM
	if (ch->isAuraOpened(true))
		strlcat(buf, ", 光环改良", sizeof(buf));

	if (ch->isAuraOpened(false))
		strlcat(buf, ", 光滑吸收", sizeof(buf));
#endif

	ch->ChatPacket(CHAT_TYPE_INFO, "%s", buf);

	int len;
	len = snprintf(buf, sizeof(buf), "坐标 %ldx%ld (%ldx%ld)",
		tch->GetX(), tch->GetY(), tch->GetX() / 100, tch->GetY() / 100);

	if (len < 0 || len >= (int)sizeof(buf))
		len = sizeof(buf) - 1;

	LPSECTREE pSec = SECTREE_MANAGER::instance().Get(tch->GetMapIndex(), tch->GetX(), tch->GetY());

	if (pSec)
	{
		TMapSetting& map_setting = SECTREE_MANAGER::instance().GetMap(tch->GetMapIndex())->m_setting;
		snprintf(buf + len, sizeof(buf) - len, "地图ID: %ld 属性 %08X 当前坐标: (%ld x %ld)",
			tch->GetMapIndex(), pSec->GetAttribute(tch->GetX(), tch->GetY()), (tch->GetX() - map_setting.iBaseX) / 100, (tch->GetY() - map_setting.iBaseY) / 100);
	}

	ch->ChatPacket(CHAT_TYPE_INFO, "%s", buf);
	ch->ChatPacket(CHAT_TYPE_INFO, "等级: %d", tch->GetLevel());
	ch->ChatPacket(CHAT_TYPE_INFO, "生命: %d/%d", tch->GetHP(), tch->GetMaxHP());
	ch->ChatPacket(CHAT_TYPE_INFO, "精力: %d/%d", tch->GetSP(), tch->GetMaxSP());
	ch->ChatPacket(CHAT_TYPE_INFO, "ATT %d MAGIC_ATT %d SPD %d CRIT %d%% PENE %d%% ATT_BONUS %d%%",
		tch->GetPoint(POINT_ATT_GRADE),
		tch->GetPoint(POINT_MAGIC_ATT_GRADE),
		tch->GetPoint(POINT_ATT_SPEED),
		tch->GetPoint(POINT_CRITICAL_PCT),
		tch->GetPoint(POINT_PENETRATE_PCT),
		tch->GetPoint(POINT_ATT_BONUS));
	ch->ChatPacket(CHAT_TYPE_INFO, "DEF %d MAGIC_DEF %d BLOCK %d%% DODGE %d%% DEF_BONUS %d%%",
		tch->GetPoint(POINT_DEF_GRADE),
		tch->GetPoint(POINT_MAGIC_DEF_GRADE),
		tch->GetPoint(POINT_BLOCK),
		tch->GetPoint(POINT_DODGE),
		tch->GetPoint(POINT_DEF_BONUS));
	ch->ChatPacket(CHAT_TYPE_INFO, "RESISTANCES:");
	ch->ChatPacket(CHAT_TYPE_INFO, "   WARR:%3d%% ASAS:%3d%% SURA:%3d%% SHAM:%3d%%"
#ifdef ENABLE_WOLFMAN_CHARACTER
		" WOLF:%3d%%"
#endif
		,
		tch->GetPoint(POINT_RESIST_WARRIOR),
		tch->GetPoint(POINT_RESIST_ASSASSIN),
		tch->GetPoint(POINT_RESIST_SURA),
		tch->GetPoint(POINT_RESIST_SHAMAN)
#ifdef ENABLE_WOLFMAN_CHARACTER
		, tch->GetPoint(POINT_RESIST_WOLFMAN)
#endif
	);
	ch->ChatPacket(CHAT_TYPE_INFO, "   SWORD:%3d%% THSWORD:%3d%% DAGGER:%3d%% BELL:%3d%% FAN:%3d%% BOW:%3d%%"
#ifdef ENABLE_WOLFMAN_CHARACTER
		" CLAW:%3d%%"
#endif
		,
		tch->GetPoint(POINT_RESIST_SWORD),
		tch->GetPoint(POINT_RESIST_TWOHAND),
		tch->GetPoint(POINT_RESIST_DAGGER),
		tch->GetPoint(POINT_RESIST_BELL),
		tch->GetPoint(POINT_RESIST_FAN),
		tch->GetPoint(POINT_RESIST_BOW)
#ifdef ENABLE_WOLFMAN_CHARACTER
		, tch->GetPoint(POINT_RESIST_CLAW)
#endif
	);
	ch->ChatPacket(CHAT_TYPE_INFO, "   FIRE:%3d%% ELEC:%3d%% MAGIC:%3d%% WIND:%3d%% CRIT:%3d%% PENE:%3d%%",
		tch->GetPoint(POINT_RESIST_FIRE),
		tch->GetPoint(POINT_RESIST_ELEC),
		tch->GetPoint(POINT_RESIST_MAGIC),
		tch->GetPoint(POINT_RESIST_WIND),
		tch->GetPoint(POINT_RESIST_CRITICAL),
		tch->GetPoint(POINT_RESIST_PENETRATE));
	ch->ChatPacket(CHAT_TYPE_INFO, "   ICE:%3d%% EARTH:%3d%% DARK:%3d%%",
		tch->GetPoint(POINT_RESIST_ICE),
		tch->GetPoint(POINT_RESIST_EARTH),
		tch->GetPoint(POINT_RESIST_DARK));

#ifdef ENABLE_MAGIC_REDUCTION_SYSTEM
	ch->ChatPacket(CHAT_TYPE_INFO, "   MAGICREDUCT:%3d%%", tch->GetPoint(POINT_RESIST_MAGIC_REDUCTION));
#endif

	ch->ChatPacket(CHAT_TYPE_INFO, "MALL:");
	ch->ChatPacket(CHAT_TYPE_INFO, "   ATT:%3d%% DEF:%3d%% EXP:%3d%% ITEMx%d GOLDx%d",
		tch->GetPoint(POINT_MALL_ATTBONUS),
		tch->GetPoint(POINT_MALL_DEFBONUS),
		tch->GetPoint(POINT_MALL_EXPBONUS),
		tch->GetPoint(POINT_MALL_ITEMBONUS) / 10,
		tch->GetPoint(POINT_MALL_GOLDBONUS) / 10);

	ch->ChatPacket(CHAT_TYPE_INFO, "BONUS:");
	ch->ChatPacket(CHAT_TYPE_INFO, "   SKILL:%3d%% NORMAL:%3d%% SKILL_DEF:%3d%% NORMAL_DEF:%3d%%",
		tch->GetPoint(POINT_SKILL_DAMAGE_BONUS),
		tch->GetPoint(POINT_NORMAL_HIT_DAMAGE_BONUS),
		tch->GetPoint(POINT_SKILL_DEFEND_BONUS),
		tch->GetPoint(POINT_NORMAL_HIT_DEFEND_BONUS));

	ch->ChatPacket(CHAT_TYPE_INFO, "   HUMAN:%3d%% ANIMAL:%3d%% ORC:%3d%% MILGYO:%3d%% UNDEAD:%3d%%",
		tch->GetPoint(POINT_ATTBONUS_HUMAN),
		tch->GetPoint(POINT_ATTBONUS_ANIMAL),
		tch->GetPoint(POINT_ATTBONUS_ORC),
		tch->GetPoint(POINT_ATTBONUS_MILGYO),
		tch->GetPoint(POINT_ATTBONUS_UNDEAD));

	ch->ChatPacket(CHAT_TYPE_INFO, "   DEVIL:%3d%% INSECT:%3d%% FIRE:%3d%% ICE:%3d%% DESERT:%3d%%",
		tch->GetPoint(POINT_ATTBONUS_DEVIL),
		tch->GetPoint(POINT_ATTBONUS_INSECT),
		tch->GetPoint(POINT_ATTBONUS_FIRE),
		tch->GetPoint(POINT_ATTBONUS_ICE),
		tch->GetPoint(POINT_ATTBONUS_DESERT));

	ch->ChatPacket(CHAT_TYPE_INFO, "   TREE:%3d%% MONSTER:%3d%%",
		tch->GetPoint(POINT_ATTBONUS_TREE),
		tch->GetPoint(POINT_ATTBONUS_MONSTER));

	ch->ChatPacket(CHAT_TYPE_INFO, "   WARR:%3d%% ASAS:%3d%% SURA:%3d%% SHAM:%3d%%"
#ifdef ENABLE_WOLFMAN_CHARACTER
		" WOLF:%3d%%"
#endif
		,
		tch->GetPoint(POINT_ATTBONUS_WARRIOR),
		tch->GetPoint(POINT_ATTBONUS_ASSASSIN),
		tch->GetPoint(POINT_ATTBONUS_SURA),
		tch->GetPoint(POINT_ATTBONUS_SHAMAN)
#ifdef ENABLE_WOLFMAN_CHARACTER
		, tch->GetPoint(POINT_ATTBONUS_WOLFMAN)
#endif
	);
	ch->ChatPacket(CHAT_TYPE_INFO, "IMMUNE:");
	ch->ChatPacket(CHAT_TYPE_INFO, "   STUN:%d SLOW:%d FALL:%d",
		tch->GetPoint(POINT_IMMUNE_STUN),
		tch->GetPoint(POINT_IMMUNE_SLOW),
		tch->GetPoint(POINT_IMMUNE_FALL));
}

struct notice_packet_func
{
	const char* m_str;
#ifdef ENABLE_FULL_NOTICE
	bool m_bBigFont;
	notice_packet_func(const char* str, bool bBigFont = false) : m_str(str), m_bBigFont(bBigFont)
#else
	notice_packet_func(const char* str) : m_str(str)
#endif
	{
	}

	void operator () (LPDESC d)
	{
		if (!d->GetCharacter())
			return;
#ifdef ENABLE_FULL_NOTICE
		d->GetCharacter()->ChatPacket((m_bBigFont) ? CHAT_TYPE_BIG_NOTICE : CHAT_TYPE_NOTICE, "%s", m_str);
#else
		d->GetCharacter()->ChatPacket(CHAT_TYPE_NOTICE, "%s", m_str);
#endif
	}
};

void SendNotice(const char* c_pszBuf, bool bBigFont)
{
	const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();

#ifdef ENABLE_FULL_NOTICE
	std::for_each (c_ref_set.begin(), c_ref_set.end(), notice_packet_func (c_pszBuf, bBigFont));
#else
	std::for_each (c_ref_set.begin(), c_ref_set.end(), notice_packet_func (c_pszBuf));
#endif
}

struct notice_map_packet_func
{
	const char* m_str;
	int m_mapIndex;
	bool m_bBigFont;

	notice_map_packet_func(const char* str, int idx, bool bBigFont) : m_str(str), m_mapIndex(idx), m_bBigFont(bBigFont)
	{
	}

	void operator() (LPDESC d)
	{
		if (d->GetCharacter() == NULL) return;
		if (d->GetCharacter()->GetMapIndex() != m_mapIndex) return;

		d->GetCharacter()->ChatPacket(m_bBigFont == true ? CHAT_TYPE_BIG_NOTICE : CHAT_TYPE_NOTICE, "%s", m_str);
	}
};

void SendNoticeMap(const char* c_pszBuf, int nMapIndex, bool bBigFont)
{
	const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();
	std::for_each(c_ref_set.begin(), c_ref_set.end(), notice_map_packet_func(c_pszBuf, nMapIndex, bBigFont));
}

struct log_packet_func
{
	const char* m_str;

	log_packet_func(const char* str) : m_str(str)
	{
	}

	void operator () (LPDESC d)
	{
		if (!d->GetCharacter())
			return;

		if (d->GetCharacter()->GetGMLevel() > GM_PLAYER)
			d->GetCharacter()->ChatPacket(CHAT_TYPE_NOTICE, "%s", m_str);
	}
};

void SendLog(const char* c_pszBuf)
{
	const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();
	std::for_each(c_ref_set.begin(), c_ref_set.end(), log_packet_func(c_pszBuf));
}

#ifdef ENABLE_FULL_NOTICE
void BroadcastNotice(const char* c_pszBuf, bool bBigFont)
#else
void BroadcastNotice(const char* c_pszBuf)
#endif
{
	TPacketGGNotice p;
#ifdef ENABLE_FULL_NOTICE
	p.bHeader = (bBigFont) ? HEADER_GG_BIG_NOTICE : HEADER_GG_NOTICE;
#else
	p.bHeader = HEADER_GG_NOTICE;
#endif
	p.lSize = strlen(c_pszBuf) + 1;

	TEMP_BUFFER buf;
	buf.write(&p, sizeof(p));
	buf.write(c_pszBuf, p.lSize);

	P2P_MANAGER::instance().Send(buf.read_peek(), buf.size()); // HEADER_GG_NOTICE

#ifdef ENABLE_FULL_NOTICE
	SendNotice(c_pszBuf, bBigFont);
#else
	SendNotice(c_pszBuf);
#endif
}

ACMD(do_notice)
{
	BroadcastNotice(argument);
}

ACMD(do_map_notice)
{
	SendNoticeMap(argument, ch->GetMapIndex(), false);
}


ACMD (do_big_notice)
{
	#ifdef ENABLE_FULL_NOTICE
	BroadcastNotice (argument, true);
	#else
	ch->ChatPacket (CHAT_TYPE_BIG_NOTICE, "%s", argument);
	#endif
}

#ifdef ENABLE_FULL_NOTICE
ACMD(do_map_big_notice)
{
	SendNoticeMap(argument, ch->GetMapIndex(), true);
}

ACMD(do_notice_test)
{
	ch->ChatPacket(CHAT_TYPE_NOTICE, "%s", argument);
}

ACMD(do_big_notice_test)
{
	ch->ChatPacket(CHAT_TYPE_BIG_NOTICE, "%s", argument);
}
#endif

ACMD(do_who)
{
	int iTotal;
	int* paiEmpireUserCount;
	int iLocal;

	DESC_MANAGER::instance().GetUserCount(iTotal, &paiEmpireUserCount, iLocal);

	ch->ChatPacket(CHAT_TYPE_INFO, "在线统计[%d] 红%d /黄%d /蓝%d (当前通道 %d)-其他",
		iTotal, paiEmpireUserCount[1], paiEmpireUserCount[2], paiEmpireUserCount[3], iLocal);
}

class user_func
{
public:
	LPCHARACTER	m_ch;
	static int count;
	static char str[128];
	static int str_len;

	user_func()
		: m_ch(NULL)
	{}

	void initialize(LPCHARACTER ch)
	{
		m_ch = ch;
		str_len = 0;
		count = 0;
		str[0] = '\0';
	}

	void operator () (LPDESC d)
	{
		if (!d->GetCharacter())
			return;

		int len = snprintf(str + str_len, sizeof(str) - str_len, "%-16s ", d->GetCharacter()->GetName());

		if (len < 0 || len >= (int)sizeof(str) - str_len)
			len = (sizeof(str) - str_len) - 1;

		str_len += len;
		++count;

		if (!(count % 4))
		{
			m_ch->ChatPacket(CHAT_TYPE_INFO, str);

			str[0] = '\0';
			str_len = 0;
		}
	}
};

int	user_func::count = 0;
char user_func::str[128] = { 0, };
int	user_func::str_len = 0;

ACMD(do_user)
{
	const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();
	user_func func;

	func.initialize(ch);
	std::for_each(c_ref_set.begin(), c_ref_set.end(), func);

	if (func.count % 4)
		ch->ChatPacket(CHAT_TYPE_INFO, func.str);

	ch->ChatPacket(CHAT_TYPE_INFO, "当前通道 %d", func.count);
}

ACMD(do_disconnect)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "踢人例如: /dc 张三 ->");
		return;
	}

	LPDESC d = DESC_MANAGER::instance().FindByCharacterName(arg1);
	LPCHARACTER	tch = d ? d->GetCharacter() : NULL;

	if (!tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "输入的角色不在线或没该角色");
		return;
	}

	if (tch == ch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "管理员不能踢自身");
		return;
	}

	DESC_MANAGER::instance().DestroyDesc(d);
}

ACMD(do_client_gb)//关闭玩家客户端
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1) {
		ch->ChatPacket(CHAT_TYPE_INFO, "踢人例如: /dc 张三 ->");
		return;
	}

	LPDESC d = DESC_MANAGER::instance().FindByCharacterName(arg1);
	LPCHARACTER	tch = d ? d->GetCharacter() : NULL;

	if (!tch) {
		ch->ChatPacket(CHAT_TYPE_INFO, "玩家 %s 目前不在线", arg1);
		return;
	}

	if (tch == ch) {
		ch->ChatPacket(CHAT_TYPE_INFO, "GM不能关闭自身");
		return;
	}
	tch->ChatPacket(CHAT_TYPE_COMMAND, "quit client");
}

ACMD(do_kill)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "用法：/kill 张三  <死亡>");
		return;
	}

	LPDESC	d = DESC_MANAGER::instance().FindByCharacterName(arg1);
	LPCHARACTER tch = d ? d->GetCharacter() : NULL;

	if (!tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "%s: 角色不存在或不在线", arg1);
		return;
	}

	tch->Dead();
}

#ifdef ENABLE_NEWSTUFF
ACMD(do_poison)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "ex) /poison <角色名字>");
		return;
	}

	LPDESC	d = DESC_MANAGER::instance().FindByCharacterName(arg1);
	LPCHARACTER tch = d ? d->GetCharacter() : NULL;

	if (!tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "%s 不存在或不在线", arg1);
		return;
	}

	tch->AttackedByPoison(NULL);
}
#endif
#ifdef ENABLE_WOLFMAN_CHARACTER
ACMD(do_bleeding)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "ex) /bleeding <角色名字>");
		return;
	}

	LPDESC	d = DESC_MANAGER::instance().FindByCharacterName(arg1);
	LPCHARACTER tch = d ? d->GetCharacter() : NULL;

	if (!tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "%s:没有该角色或不在线", arg1);
		return;
	}

	tch->AttackedByBleeding(NULL);
}
#endif

#define MISC    0
#define BINARY  1
#define NUMBER  2

namespace DoSetTypes {
	typedef enum do_set_types_s { GOLD, RACE, SEX, JOB, EXP, MAX_HP, MAX_SP, SKILL, ALIGNMENT, ALIGN, TEST,
	} do_set_types_t;
}

const struct set_struct
{
	const char* cmd;
	const char type;
	const char* help;
} set_fields[] = {
	{ "gold",		NUMBER,	NULL	},
#ifdef ENABLE_WOLFMAN_CHARACTER
	{ "race",		NUMBER,	"0. 猛将, 1. 刺客, 2. 修罗, 3. 法师, 4. 狼人"		},
#else
	{ "race",		NUMBER,	"0. 猛将, 1. 刺客, 2. 修罗, 3. 法师"		},
#endif
	{ "sex",		NUMBER,	"0. 男性, 1. 女性"	},
	{ "job",		NUMBER,	"0. 未加入, 1. 职业1, 2. 职业2"	},
	{ "exp",		NUMBER,	NULL	},
	{ "max_hp",		NUMBER,	NULL	},
	{ "max_sp",		NUMBER,	NULL	},
	{ "skill",		NUMBER,	NULL	},
	{ "alignment",	NUMBER,	NULL	},
	{ "align",		NUMBER,	NULL	},
	{ "test",		NUMBER,	NULL	},
#ifdef ENABLE_GEM_SYSTEM
	{ "gem",		NUMBER, NULL	},
#endif
	{ "\n",			MISC,	NULL	}
};


ACMD(do_set)
{
	char arg1[256], arg2[256], arg3[256];

	LPCHARACTER tch = NULL;

	int i, len;
	const char* line;

	line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	one_argument(line, arg3, sizeof(arg3));

	if (!*arg1 || !*arg2 || !*arg3)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "用法: /set <角色> <字段> <值>");
#ifdef ENABLE_NEWSTUFF
		ch->ChatPacket(CHAT_TYPE_INFO, "可用字段列表:");
		for (i = 0; *(set_fields[i].cmd) != '\n'; i++)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, " %d. %s", i + 1, set_fields[i].cmd);
			if (set_fields[i].help != NULL)
				ch->ChatPacket(CHAT_TYPE_INFO, "  帮助: %s", set_fields[i].help);
		}
#endif
		return;
	}

	tch = (!strcmp(arg1, "*")) ? CHARACTER_MANAGER::instance().FindPC(ch->GetName()) : CHARACTER_MANAGER::instance().FindPC(arg1);

	if (!tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "%s 不存在", arg1);
		return;
	}

	len = strlen(arg2);

	for (i = 0; *(set_fields[i].cmd) != '\n'; i++)
		if (!strncmp(arg2, set_fields[i].cmd, len))
			break;

	switch (i)
	{
#ifdef ENABLE_EXTENDED_YANG_LIMIT
	case DoSetTypes::GOLD:	// gold
	{
		int64_t gold = 0;
		str_to_number(gold, arg3);
		tch->PointChange(POINT_GOLD, gold, true);
		break;
	}
#else
	case DoSetTypes::GOLD:	// gold
	{
		int gold = 0;
		str_to_number(gold, arg3);
		tch->PointChange(POINT_GOLD, gold, true);
	}
	break;
#endif

	case DoSetTypes::RACE: // race
#ifdef ENABLE_NEWSTUFF
	{
		int amount = 0;
		str_to_number(amount, arg3);
		amount = MINMAX(0, amount, JOB_MAX_NUM);
		ESex mySex = GET_SEX(tch);
		DWORD dwRace = MAIN_RACE_WARRIOR_M;
		switch (amount)
		{
		case JOB_WARRIOR:
			dwRace = (mySex == SEX_MALE) ? MAIN_RACE_WARRIOR_M : MAIN_RACE_WARRIOR_W;
			break;
		case JOB_ASSASSIN:
			dwRace = (mySex == SEX_MALE) ? MAIN_RACE_ASSASSIN_M : MAIN_RACE_ASSASSIN_W;
			break;
		case JOB_SURA:
			dwRace = (mySex == SEX_MALE) ? MAIN_RACE_SURA_M : MAIN_RACE_SURA_W;
			break;
		case JOB_SHAMAN:
			dwRace = (mySex == SEX_MALE) ? MAIN_RACE_SHAMAN_M : MAIN_RACE_SHAMAN_W;
			break;
#ifdef ENABLE_WOLFMAN_CHARACTER
		case JOB_WOLFMAN:
			dwRace = (mySex == SEX_MALE) ? MAIN_RACE_WOLFMAN_M : MAIN_RACE_WOLFMAN_M;
			break;
#endif
		}
		if (dwRace != tch->GetRaceNum())
		{
			tch->SetRace(dwRace);
			tch->ClearSkill();
			tch->SetSkillGroup(0);
			// quick mesh change workaround begin
			tch->SetPolymorph(101);
			tch->SetPolymorph(0);
			// quick mesh change workaround end
		}
		break;
	}
#endif
	case DoSetTypes::SEX: // sex
#ifdef ENABLE_NEWSTUFF
	{
		int amount = 0;
		str_to_number(amount, arg3);
		amount = MINMAX(SEX_MALE, amount, SEX_FEMALE);
		if (amount != GET_SEX(tch))
		{
			tch->ChangeSex();
			// quick mesh change workaround begin
			tch->SetPolymorph(101);
			tch->SetPolymorph(0);
			// quick mesh change workaround end
		}
		break;
	}
#endif

	case DoSetTypes::JOB: // job
#ifdef ENABLE_NEWSTUFF
	{
		int amount = 0;
		str_to_number(amount, arg3);
		amount = MINMAX(0, amount, 2);
		if (amount != tch->GetSkillGroup())
		{
			tch->ClearSkill();
			tch->SetSkillGroup(amount);
		}
		break;
	}
#endif

	case DoSetTypes::EXP: // exp
	{
		int amount = 0;
		str_to_number(amount, arg3);
		tch->PointChange(POINT_EXP, amount, true);
		break;
	}

	case DoSetTypes::MAX_HP: // max_hp
	{
		int amount = 0;
		str_to_number(amount, arg3);
		tch->PointChange(POINT_MAX_HP, amount, true);
		break;
	}

	case DoSetTypes::MAX_SP: // max_sp
	{
		int amount = 0;
		str_to_number(amount, arg3);
		tch->PointChange(POINT_MAX_SP, amount, true);
	}
	break;

	case DoSetTypes::SKILL: // active skill point
	{
		int amount = 0;
		str_to_number(amount, arg3);
		tch->PointChange(POINT_SKILL, amount, true);
		break;
	}
	

	case DoSetTypes::ALIGN: // alignment
	case DoSetTypes::ALIGNMENT: // alignment
	{
		int	amount = 0;
		str_to_number(amount, arg3);
		// tch->UpdateAlignment(amount - ch->GetRealAlignment());
		tch->UpdateAlignment(amount - tch->GetRealAlignment());//@Lightwork#5
		break;
	}
	
	case DoSetTypes::TEST:
	{
		int	amount = 0;
		str_to_number(amount, arg3);
		ch->TestYapcam31(amount);
		break;
	}
	
	}//switch

	if (set_fields[i].type == NUMBER)
	{
#ifdef ENABLE_EXTENDED_YANG_LIMIT
		int64_t	amount = 0;
		str_to_number(amount, arg3);
		ch->ChatPacket(CHAT_TYPE_INFO, "%s %s 已调整为 [%lld]", tch->GetName(), set_fields[i].cmd, amount);
#else
		int	amount = 0;
		str_to_number(amount, arg3);
		ch->ChatPacket(CHAT_TYPE_INFO, "%s's %s set to [%d]", tch->GetName(), set_fields[i].cmd, amount);
#endif
	}
}

ACMD(do_reset)
{
	ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
	ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
	ch->Save();
}

ACMD(do_advance)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "调等级用法: /advance <角色> <等级>");
		return;
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (!tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "%s 不存在", arg1);
		return;
	}

	int level = 0;
	str_to_number(level, arg2);

	tch->ResetPoint(MINMAX(0, level, gPlayerMaxLevel));
}

ACMD(do_respawn)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1 && !strcasecmp(arg1, "all"))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "已重生所有");
		regen_reset(0, 0);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "已重生周围");
		regen_reset(ch->GetX(), ch->GetY());
	}
}

ACMD(do_safebox_size)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	int size = 0;

	if (*arg1)
		str_to_number(size, arg1);

	if (size > 3 || size < 0)
		size = 0;

	ch->ChatPacket(CHAT_TYPE_INFO, "仓库大小设置为 %d", size);
	ch->ChangeSafeboxSize(size);
}

ACMD(do_makeguild)
{
	if (ch->GetGuild())
		return;

	CGuildManager& gm = CGuildManager::instance();

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	TGuildCreateParameter cp;
	memset(&cp, 0, sizeof(cp));

	cp.master = ch;
	strlcpy(cp.name, arg1, sizeof(cp.name));

	if (!check_name(cp.name))
	{
		return;
	}

	gm.CreateGuild(cp);
}

ACMD(do_deleteguild)
{
	if (ch->GetGuild())
		ch->GetGuild()->RequestDisband(ch->GetPlayerID());
}

ACMD(do_greset)
{
	if (ch->GetGuild())
		ch->GetGuild()->Reset();
}

// REFINE_ROD_HACK_BUG_FIX
ACMD(do_refine_rod)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	CELL_UINT cell = 0;
	str_to_number(cell, arg1);
	LPITEM item = ch->GetInventoryItem(cell);
	if (item)
		fishing::RealRefineRod(ch, item);
}
// END_OF_REFINE_ROD_HACK_BUG_FIX

// REFINE_PICK
ACMD(do_refine_pick)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	CELL_UINT cell = 0;
	str_to_number(cell, arg1);
	LPITEM item = ch->GetInventoryItem(cell);
	if (item)
	{
		mining::CHEAT_MAX_PICK(ch, item);
		mining::RealRefinePick(ch, item);
	}
}

ACMD(do_max_pick)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	CELL_UINT cell = 0;
	str_to_number(cell, arg1);
	LPITEM item = ch->GetInventoryItem(cell);
	if (item)
	{
		mining::CHEAT_MAX_PICK(ch, item);
	}
}
// END_OF_REFINE_PICK

ACMD(do_invisibility)
{
	if (ch->IsAffectFlag(AFF_INVISIBILITY))
	{
		ch->RemoveAffect(AFFECT_INVISIBILITY);
	}
	else
	{
		ch->AddAffect(AFFECT_INVISIBILITY, POINT_NONE, 0, AFF_INVISIBILITY, INFINITE_AFFECT_DURATION, 0, true);
	}
}

ACMD(do_event_flag)
{
	char arg1[256];
	char arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!(*arg1) || !(*arg2))
		return;

	int value = 0;
	str_to_number(value, arg2);

	if (!strcmp(arg1, "mob_item") ||
		!strcmp(arg1, "mob_exp") ||
		!strcmp(arg1, "mob_gold") ||
		!strcmp(arg1, "mob_dam") ||
		!strcmp(arg1, "mob_gold_pct") ||
		!strcmp(arg1, "mob_item_buyer") ||
		!strcmp(arg1, "mob_exp_buyer") ||
		!strcmp(arg1, "mob_gold_buyer") ||
		!strcmp(arg1, "mob_gold_pct_buyer")
		)
		value = MINMAX(0, value, 1000);

	//quest::CQuestManager::instance().SetEventFlag(arg1, atoi(arg2));
	quest::CQuestManager::instance().RequestSetEventFlag(arg1, value);
	ch->ChatPacket(CHAT_TYPE_INFO, "RequestSetEventFlag %s %d", arg1, value);
}

ACMD(do_get_event_flag)
{
	quest::CQuestManager::instance().SendEventFlagList(ch);
}

ACMD(do_private)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "用法： /private <map index>");
		return;
	}

	long lMapIndex;
	long map_index = 0;
	str_to_number(map_index, arg1);
	if ((lMapIndex = SECTREE_MANAGER::instance().CreatePrivateMap(map_index)))
	{
		ch->SaveExitLocation();

		LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(lMapIndex);
		ch->WarpSet(pkSectreeMap->m_setting.posSpawn.x, pkSectreeMap->m_setting.posSpawn.y, lMapIndex);
	}
	else
		ch->ChatPacket(CHAT_TYPE_INFO, "无法通过索引 %d 找到地图", map_index);
}

ACMD(do_qf)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	quest::PC* pPC = quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID());
	std::string questname = pPC->GetCurrentQuestName();

	if (!questname.empty())
	{
		int value = quest::CQuestManager::Instance().GetQuestStateIndex(questname, arg1);

		pPC->SetFlag(questname + ".__status", value);
		pPC->ClearTimer();

		quest::PC::QuestInfoIterator it = pPC->quest_begin();
		unsigned int questindex = quest::CQuestManager::instance().GetQuestIndexByName(questname);

		while (it != pPC->quest_end())
		{
			if (it->first == questindex)
			{
				it->second.st = value;
				break;
			}

			++it;
		}

		ch->ChatPacket(CHAT_TYPE_INFO, "设置任务状态标志 %s %s %d", questname.c_str(), arg1, value);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "设置任务状态标志失败");
	}
}

LPCHARACTER chHori, chForge, chLib, chTemple, chTraining, chTree, chPortal, chBall;

ACMD(do_b1)
{
	chHori = CHARACTER_MANAGER::instance().SpawnMobRange(14017, ch->GetMapIndex(), 304222, 742858, 304222, 742858, true, false);
	chHori->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_BUILDING_CONSTRUCTION_SMALL, 65535, 0, true);
	chHori->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);

	for (int i = 0; i < 30; ++i)
	{
		int rot = number(0, 359);
		float fx, fy;
		GetDeltaByDegree(rot, 800, &fx, &fy);

		LPCHARACTER tch = CHARACTER_MANAGER::instance().SpawnMobRange(number(701, 706),
			ch->GetMapIndex(),
			304222 + (int)fx,
			742858 + (int)fy,
			304222 + (int)fx,
			742858 + (int)fy,
			true,
			false);
		tch->SetAggressive();
	}

	for (int i = 0; i < 5; ++i)
	{
		int rot = number(0, 359);
		float fx, fy;
		GetDeltaByDegree(rot, 800, &fx, &fy);

		LPCHARACTER tch = CHARACTER_MANAGER::instance().SpawnMobRange(8009,
			ch->GetMapIndex(),
			304222 + (int)fx,
			742858 + (int)fy,
			304222 + (int)fx,
			742858 + (int)fy,
			true,
			false);
		tch->SetAggressive();
	}
}

ACMD(do_b2)
{
	chHori->RemoveAffect(AFFECT_DUNGEON_UNIQUE);
}

ACMD(do_b3)
{
	chForge = CHARACTER_MANAGER::instance().SpawnMobRange(14003, ch->GetMapIndex(), 307500, 746300, 307500, 746300, true, false);
	chForge->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);

	chLib = CHARACTER_MANAGER::instance().SpawnMobRange(14007, ch->GetMapIndex(), 307900, 744500, 307900, 744500, true, false);
	chLib->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);

	chTemple = CHARACTER_MANAGER::instance().SpawnMobRange(14004, ch->GetMapIndex(), 307700, 741600, 307700, 741600, true, false);
	chTemple->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);

	chTraining = CHARACTER_MANAGER::instance().SpawnMobRange(14010, ch->GetMapIndex(), 307100, 739500, 307100, 739500, true, false);
	chTraining->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
	chTree = CHARACTER_MANAGER::instance().SpawnMobRange(14013, ch->GetMapIndex(), 300800, 741600, 300800, 741600, true, false);
	chTree->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
	chPortal = CHARACTER_MANAGER::instance().SpawnMobRange(14001, ch->GetMapIndex(), 300900, 744500, 300900, 744500, true, false);
	chPortal->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
	chBall = CHARACTER_MANAGER::instance().SpawnMobRange(14012, ch->GetMapIndex(), 302500, 746600, 302500, 746600, true, false);
	chBall->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
}

ACMD(do_b4)
{
	chLib->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_BUILDING_UPGRADE, 65535, 0, true);

	for (int i = 0; i < 30; ++i)
	{
		int rot = number(0, 359);
		float fx, fy;
		GetDeltaByDegree(rot, 1200, &fx, &fy);

		LPCHARACTER tch = CHARACTER_MANAGER::instance().SpawnMobRange(number(701, 706),
			ch->GetMapIndex(),
			307900 + (int)fx,
			744500 + (int)fy,
			307900 + (int)fx,
			744500 + (int)fy,
			true,
			false);
		tch->SetAggressive();
	}

	for (int i = 0; i < 5; ++i)
	{
		int rot = number(0, 359);
		float fx, fy;
		GetDeltaByDegree(rot, 1200, &fx, &fy);

		LPCHARACTER tch = CHARACTER_MANAGER::instance().SpawnMobRange(8009,
			ch->GetMapIndex(),
			307900 + (int)fx,
			744500 + (int)fy,
			307900 + (int)fx,
			744500 + (int)fy,
			true,
			false);
		tch->SetAggressive();
	}
}

ACMD(do_b5)
{
	M2_DESTROY_CHARACTER(chLib);
	//chHori->RemoveAffect(AFFECT_DUNGEON_UNIQUE);
	chLib = CHARACTER_MANAGER::instance().SpawnMobRange(14008, ch->GetMapIndex(), 307900, 744500, 307900, 744500, true, false);
	chLib->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
}

ACMD(do_b6)
{
	chLib->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_BUILDING_UPGRADE, 65535, 0, true);
}
ACMD(do_b7)
{
	M2_DESTROY_CHARACTER(chLib);
	//chHori->RemoveAffect(AFFECT_DUNGEON_UNIQUE);
	chLib = CHARACTER_MANAGER::instance().SpawnMobRange(14009, ch->GetMapIndex(), 307900, 744500, 307900, 744500, true, false);
	chLib->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
}

ACMD(do_book)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	CSkillProto* pkProto;

	if (isnhdigit(*arg1))
	{
		DWORD vnum = 0;
		str_to_number(vnum, arg1);
		pkProto = CSkillManager::instance().Get(vnum);
	}
	else
		pkProto = CSkillManager::instance().Get(arg1);

	if (!pkProto)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, "没有这样的技能.");
		return;
	}

	LPITEM item = ch->AutoGiveItem(50300);
	item->SetSocket(0, pkProto->dwVnum);
}

ACMD(do_setskillother)
{
	char arg1[256], arg2[256], arg3[256];
	argument = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	one_argument(argument, arg3, sizeof(arg3));

	if (!*arg1 || !*arg2 || !*arg3 || !isdigit(*arg3))
	{
		ch->ChatPacket (CHAT_TYPE_INFO, "用法: /setskillother <角色> <技能名称> <技能等级>");
		return;
	}

	LPCHARACTER tch;

	tch = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (!tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "没有这样的名字.");
		return;
	}

	CSkillProto* pk;

	if (isdigit(*arg2))
	{
		DWORD vnum = 0;
		str_to_number(vnum, arg2);
		pk = CSkillManager::instance().Get(vnum);
	}
	else
		pk = CSkillManager::instance().Get(arg2);

	if (!pk)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "没有这样的技能名称.");
		return;
	}

	BYTE level = 0;
	str_to_number(level, arg3);
	tch->SetSkillLevel(pk->dwVnum, level);
	tch->ComputePoints();
	tch->SkillLevelPacket();
}

ACMD(do_setskill)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2 || !isdigit(*arg2))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "用法: /setskill <技能代码> <等级1~40级 20=M1 30=G1 40=P");
		return;
	}

	CSkillProto* pk;

	if (isdigit(*arg1))
	{
		DWORD vnum = 0;
		str_to_number(vnum, arg1);
		pk = CSkillManager::instance().Get(vnum);
	}

	else
		pk = CSkillManager::instance().Get(arg1);

	if (!pk)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "没有这样的技能的名称.");
		return;
	}

	BYTE level = 0;
	str_to_number(level, arg2);
	ch->SetSkillLevel(pk->dwVnum, level);
	ch->ComputePoints();
	ch->SkillLevelPacket();
}

ACMD(do_set_skill_point)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	int skill_point = 0;
	if (*arg1)
		str_to_number(skill_point, arg1);

	ch->SetRealPoint(POINT_SKILL, skill_point);
	ch->SetPoint(POINT_SKILL, ch->GetRealPoint(POINT_SKILL));
	ch->PointChange(POINT_SKILL, 0);
}

ACMD(do_set_skill_group)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	int skill_group = 0;
	if (*arg1)
		str_to_number(skill_group, arg1);

	ch->SetSkillGroup(skill_group);

	ch->ClearSkill();
	ch->ChatPacket(CHAT_TYPE_INFO, "技能组设置为 %d.", skill_group);
}

ACMD(do_reload)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		switch (LOWER(*arg1))
		{
			case 'u':
				ch->ChatPacket(CHAT_TYPE_INFO, "在线用户数据已更新.");
				LoadStateUserCount();
				break;

			case 'p':
				ch->ChatPacket(CHAT_TYPE_INFO, "已成功重新加载物品表,怪物表,改良表,技能表,");
				db_clientdesc->DBPacket(HEADER_GD_RELOAD_PROTO, 0, NULL, 0);
				break;

			case 'q':
				ch->ChatPacket(CHAT_TYPE_INFO, "已重新载入任务.");
				quest::CQuestManager::instance().Reload();
				break;

			case 'f':
				fishing::Initialize();
				ch->ChatPacket(CHAT_TYPE_INFO, "已重载入钓鱼列表.");
				break;

				//RELOAD_ADMIN
			case 'a':
				ch->ChatPacket(CHAT_TYPE_INFO, "已重载入管理员列表.");
				db_clientdesc->DBPacket(HEADER_GD_RELOAD_ADMIN, 0, NULL, 0);
				sys_log(0, "Reloading admin infomation.");
				break;
				//END_RELOAD_ADMIN
			case 'c':	// cube
				Cube_init();
				break;
#ifdef ENABLE_ITEMSHOP
			case 'i':
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "已重载入物品商城列表.");
				BYTE subIndex = ITEMSHOP_RELOAD;
				db_clientdesc->DBPacketHeader(HEADER_GD_ITEMSHOP, 0, sizeof(BYTE));
				db_clientdesc->Packet(&subIndex, sizeof(BYTE));
				break;
			}
#endif
#ifdef ENABLE_RELOAD_REGEN
			case 'r':
			{
				SendNoticeMap("重新加载再生表", ch->GetMapIndex(), false);
				regen_free_map(ch->GetMapIndex());
				CHARACTER_MANAGER::instance().DestroyCharacterInMap(ch->GetMapIndex());
				regen_reload(ch->GetMapIndex());
				SendNoticeMap("重新加载再生表", ch->GetMapIndex(), false);
				break;
			}

			case 'd':
			{
				char szMOBDropItemFileName[256];
				char szSpecialItemGroupFileName[256];
				snprintf(szMOBDropItemFileName, sizeof(szMOBDropItemFileName),
					"%s/mob_drop_item.txt", LocaleService_GetBasePath().c_str());
				snprintf(szSpecialItemGroupFileName, sizeof(szSpecialItemGroupFileName),
					"%s/special_item_group.txt", LocaleService_GetBasePath().c_str());

				ch->ChatPacket(CHAT_TYPE_INFO, "重新加载: %s", szMOBDropItemFileName);//高级爆率表
				if (!ITEM_MANAGER::instance().ReadMonsterDropItemGroup(szMOBDropItemFileName, true))
					ch->ChatPacket(CHAT_TYPE_INFO, "无法加载: %s", szMOBDropItemFileName);
				else
					ch->ChatPacket(CHAT_TYPE_INFO, "重新加载已成功: %s", szMOBDropItemFileName);

				ch->ChatPacket(CHAT_TYPE_INFO, "重新加载: %s", szSpecialItemGroupFileName);//宝箱物品
				if (!ITEM_MANAGER::instance().ReadSpecialDropItemFile(szSpecialItemGroupFileName, true))
					ch->ChatPacket(CHAT_TYPE_INFO, "无法加载: %s", szSpecialItemGroupFileName);
				else
					ch->ChatPacket(CHAT_TYPE_INFO, "重新加载已成功: %s", szSpecialItemGroupFileName);

				break;
			}

#endif
		}
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "已重新加载当前在线状态");
		LoadStateUserCount();

		ch->ChatPacket(CHAT_TYPE_INFO, "重新加载物品表,怪物表成功,");
		db_clientdesc->DBPacket(HEADER_GD_RELOAD_PROTO, 0, NULL, 0);
	}
}

ACMD(do_cooltime)
{
	ch->DisableCooltime();
}

ACMD(do_level)
{
	char arg2[256];
	one_argument(argument, arg2, sizeof(arg2));

	if (!*arg2)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "GM自身调级: /level 99 ->99代表等级");
		return;
	}

	int	level = 0;
	str_to_number(level, arg2);

	ch->ResetPoint(MINMAX(1, level, gPlayerMaxLevel));

#ifndef ENABLE_NO_RESET_SKILL_WHEN_GM_LEVEL_UP
	ch->ClearSkill();
	ch->ClearSubSkill();
#endif
}

ACMD(do_gwlist)
{
	CGuildManager::instance().ShowGuildWarList(ch);
}

ACMD(do_stop_guild_war)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
		return;

	int id1 = 0, id2 = 0;

	str_to_number(id1, arg1);
	str_to_number(id2, arg2);

	if (!id1 || !id2)
		return;

	if (id1 > id2)
	{
		std::swap(id1, id2);
	}

	ch->ChatPacket(CHAT_TYPE_TALKING, "%d %d", id1, id2);
	CGuildManager::instance().RequestEndWar(id1, id2);
}

ACMD(do_cancel_guild_war)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	int id1 = 0, id2 = 0;
	str_to_number(id1, arg1);
	str_to_number(id2, arg2);

	if (id1 > id2)
		std::swap(id1, id2);

	CGuildManager::instance().RequestCancelWar(id1, id2);
}

ACMD(do_guild_state)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	CGuild* pGuild = CGuildManager::instance().FindGuildByName(arg1);
	if (pGuild != NULL)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "帮会ID: %d", pGuild->GetID());
		ch->ChatPacket(CHAT_TYPE_INFO, "帮会会长PID: %d", pGuild->GetMasterPID());
		ch->ChatPacket(CHAT_TYPE_INFO, "是否在战争: %d", pGuild->UnderAnyWar());
	}
}

struct FuncWeaken
{
	LPCHARACTER m_pkGM;
	bool	m_bAll;

	FuncWeaken(LPCHARACTER ch) : m_pkGM(ch), m_bAll(false)
	{
	}

	void operator () (LPENTITY ent)
	{
		if (!ent->IsType(ENTITY_CHARACTER))
			return;

		LPCHARACTER pkChr = (LPCHARACTER)ent;

		int iDist = DISTANCE_APPROX(pkChr->GetX() - m_pkGM->GetX(), pkChr->GetY() - m_pkGM->GetY());

		if (!m_bAll && iDist >= 1000)
			return;

		if (pkChr->IsNPC())
			pkChr->PointChange(POINT_HP, (10 - pkChr->GetHP()));
	}
};

ACMD(do_weaken)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	FuncWeaken func(ch);

	if (*arg1 && !strcmp(arg1, "all"))
		func.m_bAll = true;

	ch->GetSectree()->ForEachAround(func);
}

ACMD(do_getqf)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	LPCHARACTER tch;

	if (!*arg1)
		tch = ch;
	else
	{
		tch = CHARACTER_MANAGER::instance().FindPC(arg1);

		if (!tch)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "没有这样的角色");
			return;
		}
	}

	quest::PC* pPC = quest::CQuestManager::instance().GetPC(tch->GetPlayerID());

	if (pPC)
		pPC->SendFlagList(ch);
}

#define ENABLE_SET_STATE_WITH_TARGET
ACMD(do_set_state)
{
	char arg1[256];
	char arg2[256];

	argument = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
	{
		ch->ChatPacket(CHAT_TYPE_INFO,
			"语法：/set_state <任务名称> <状态名称>"
#ifdef ENABLE_SET_STATE_WITH_TARGET
			" [<character name>]"
#endif
		);
		return;
	}

#ifdef ENABLE_SET_STATE_WITH_TARGET
	LPCHARACTER tch = ch;
	char arg3[256];
	argument = one_argument(argument, arg3, sizeof(arg3));
	if (*arg3)
	{
		tch = CHARACTER_MANAGER::instance().FindPC(arg3);
		if (!tch)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "没有这样的角色.");
			return;
		}
	}
	quest::PC* pPC = quest::CQuestManager::instance().GetPCForce(tch->GetPlayerID());
#else
	quest::PC* pPC = quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID());
#endif
	std::string questname = arg1;
	std::string statename = arg2;

	if (!questname.empty())
	{
		int value = quest::CQuestManager::Instance().GetQuestStateIndex(questname, statename);

		pPC->SetFlag(questname + ".__status", value);
		pPC->ClearTimer();

		quest::PC::QuestInfoIterator it = pPC->quest_begin();
		unsigned int questindex = quest::CQuestManager::instance().GetQuestIndexByName(questname);

		while (it != pPC->quest_end())
		{
			if (it->first == questindex)
			{
				it->second.st = value;
				break;
			}

			++it;
		}

		ch->ChatPacket(CHAT_TYPE_INFO, "设置任务状态标志 %s %s %d", questname.c_str(), arg1, value);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "设置任务状态标志失败");
	}
}

ACMD(do_setqf)
{
	char arg1[256];
	char arg2[256];
	char arg3[256];

	one_argument(two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "用法: /setqf <标记> <值> [<角色名称>]");
		return;
	}

	LPCHARACTER tch = ch;

	if (*arg3)
		tch = CHARACTER_MANAGER::instance().FindPC(arg3);

	if (!tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "没有这样的角色.");
		return;
	}

	quest::PC* pPC = quest::CQuestManager::instance().GetPC(tch->GetPlayerID());

	if (pPC)
	{
		int value = 0;
		str_to_number(value, arg2);
		pPC->SetFlag(arg1, value);
		ch->ChatPacket(CHAT_TYPE_INFO, "Quest flag set: %s %d", arg1, value);
	}
}

ACMD(do_delqf)
{
	char arg1[256];
	char arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "用法: /delqf <标记> [<字符名>]");
		return;
	}

	LPCHARACTER tch = ch;

	if (*arg2)
		tch = CHARACTER_MANAGER::instance().FindPC(arg2);

	if (!tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "该角色不存在.");
		return;
	}

	quest::PC* pPC = quest::CQuestManager::instance().GetPC(tch->GetPlayerID());

	if (pPC)
	{
		if (pPC->DeleteFlag(arg1))
			ch->ChatPacket(CHAT_TYPE_INFO, "删除成功.");
		else
			ch->ChatPacket(CHAT_TYPE_INFO, "删除失败。任务状态不存在.");
	}
}

ACMD(do_forgetme)
{
	ch->ForgetMyAttacker();
}

ACMD(do_aggregate)
{
	ch->AggregateMonster();
}

ACMD(do_attract_ranger)
{
	ch->AttractRanger();
}

ACMD(do_pull_monster)
{
	ch->PullMonster();
}

ACMD(do_polymorph)
{
	char arg1[256], arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	if (*arg1)
	{
		DWORD dwVnum = 0;
		str_to_number(dwVnum, arg1);
		bool bMaintainStat = false;
		if (*arg2)
		{
			int value = 0;
			str_to_number(value, arg2);
			bMaintainStat = (value > 0);
		}

		ch->SetPolymorph(dwVnum, bMaintainStat);
	}
}

ACMD(do_polymorph_item)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		DWORD dwVnum = 0;
		str_to_number(dwVnum, arg1);

		LPITEM item = ITEM_MANAGER::instance().CreateItem(70104, 1, 0, true);
		if (item)
		{
			item->SetSocket(0, dwVnum);
			int iEmptyPos = ch->GetEmptyInventory(item->GetSize());

			if (iEmptyPos != -1)
			{
				item->AddToCharacter(ch, TItemPos(INVENTORY, iEmptyPos));
			}
			else
			{
				M2_DESTROY_ITEM(item);
				ch->ChatPacket(CHAT_TYPE_INFO, "身上没有足够的位置.");
			}
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "#%d 道具不存在 ", 70103);
		}
		//ch->SetPolymorph(dwVnum, bMaintainStat);
	}
}

ACMD(do_priv_empire)
{
	char arg1[256] = { 0 };
	char arg2[256] = { 0 };
	char arg3[256] = { 0 };
	char arg4[256] = { 0 };
	int empire = 0;
	int type = 0;
	int value = 0;
	int duration = 0;

	const char* line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
		goto USAGE;

	if (!line)
		goto USAGE;

	two_arguments(line, arg3, sizeof(arg3), arg4, sizeof(arg4));

	if (!*arg3 || !*arg4)
		goto USAGE;

	str_to_number(empire, arg1);
	str_to_number(type, arg2);
	str_to_number(value, arg3);
	value = MINMAX(0, value, 1000);
	str_to_number(duration, arg4);

	if (empire < 0 || 3 < empire)
		goto USAGE;

	if (type < 1 || 4 < type)
		goto USAGE;

	if (value < 0)
		goto USAGE;

	if (duration < 0)
		goto USAGE;

	duration = duration * (60 * 60);
	CPrivManager::instance().RequestGiveEmpirePriv(empire, type, value, duration);
	return;

USAGE:
	ch->ChatPacket(CHAT_TYPE_INFO, "用法：/priv_empire <帝国> <类型> <值> <持续时间>");
	ch->ChatPacket(CHAT_TYPE_INFO, "  <国家>  0 - 3 (0==所有国家)");
	ch->ChatPacket(CHAT_TYPE_INFO, "  <类型>  1:物品爆率, 2:金钱掉落, 3:金钱爆率, 4:经验值");
	ch->ChatPacket(CHAT_TYPE_INFO, "  <值>    百分比");
	ch->ChatPacket(CHAT_TYPE_INFO, "  <时间>  小时");
}

ACMD(do_priv_guild)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		CGuild* g = CGuildManager::instance().FindGuildByName(arg1);

		if (!g)
		{
			DWORD guild_id = 0;
			str_to_number(guild_id, arg1);
			g = CGuildManager::instance().FindGuild(guild_id);
		}

		else
		{
			char buf[1024 + 1];
			snprintf(buf, sizeof(buf), "%d", g->GetID()); // @fixme177

			using namespace quest;
			PC* pc = CQuestManager::instance().GetPC(ch->GetPlayerID());
			QuestState qs = CQuestManager::instance().OpenState("ADMIN_QUEST", QUEST_FISH_REFINE_STATE_INDEX);
			luaL_loadbuffer(qs.co, buf, strlen(buf), "ADMIN_QUEST");
			pc->SetQuest("ADMIN_QUEST", qs);

			QuestState& rqs = *pc->GetRunningQuestState();

			if (!CQuestManager::instance().RunState(rqs))
			{
				CQuestManager::instance().CloseState(rqs);
				pc->EndRunning();
				return;
			}
		}
	}
}

ACMD(do_mount_test)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		DWORD vnum = 0;
		str_to_number(vnum, arg1);
		ch->MountVnum(vnum);
	}
}

ACMD(do_observer)
{
	ch->SetObserverMode(!ch->IsObserverMode());
}

ACMD(do_socket_item)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (*arg1)
	{
		DWORD dwVnum = 0;
		str_to_number(dwVnum, arg1);

		int iSocketCount = 0;
		str_to_number(iSocketCount, arg2);

		if (!iSocketCount || iSocketCount >= ITEM_SOCKET_MAX_NUM)
			iSocketCount = 3;

		if (!dwVnum)
		{
			if (!ITEM_MANAGER::instance().GetVnum(arg1, dwVnum))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "#%d 物品不存在.", dwVnum);
				return;
			}
		}

		LPITEM item = ch->AutoGiveItem(dwVnum);

		if (item)
		{
			for (int i = 0; i < iSocketCount; ++i)
				item->SetSocket(i, 1);
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "#%d 无法创建物品", dwVnum);
		}
	}
}

// BLOCK_CHAT
ACMD(do_block_chat_list)
{
	if (!ch || (ch->GetGMLevel() < GM_HIGH_WIZARD && ch->GetQuestFlag("chat_privilege.block") <= 0))
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("弊繁 疙飞绢绰 绝嚼聪促"));
		return;
	}

	DBManager::instance().ReturnQuery(QID_BLOCK_CHAT_LIST, ch->GetPlayerID(), NULL,
		"SELECT p.name, a.lDuration FROM affect%s as a, player%s as p WHERE a.bType = %d AND a.dwPID = p.id",
		get_table_postfix(), get_table_postfix(), AFFECT_BLOCK_CHAT);
}

ACMD(do_vote_block_chat)
{
	return;

	char arg1[256];
	argument = one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "用法: vote_block_chat <角色名字>");
		return;
	}

	const char* name = arg1;
	long lBlockDuration = 10;

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(name);

	if (!tch)
	{
		CCI* pkCCI = P2P_MANAGER::instance().Find(name);

		if (pkCCI)
		{
			TPacketGGBlockChat p;

			p.bHeader = HEADER_GG_BLOCK_CHAT;
			strlcpy(p.szName, name, sizeof(p.szName));
			p.lBlockDuration = lBlockDuration;
			P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGBlockChat));
		}
		else
		{
			TPacketBlockChat p;

			strlcpy(p.szName, name, sizeof(p.szName));
			p.lDuration = lBlockDuration;
			db_clientdesc->DBPacket(HEADER_GD_BLOCK_CHAT, ch ? ch->GetDesc()->GetHandle() : 0, &p, sizeof(p));
		}

		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "禁言操作成功.");

		return;
	}

	if (tch && ch != tch)
		tch->AddAffect(AFFECT_BLOCK_CHAT, POINT_NONE, 0, AFF_NONE, lBlockDuration, 0, true);
}

ACMD(do_block_chat)
{
	if (ch && (ch->GetGMLevel() < GM_HIGH_WIZARD && ch->GetQuestFlag("chat_privilege.block") <= 0))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("弊繁 疙飞绢绰 绝嚼聪促"));
		return;
	}

	char arg1[256];
	argument = one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "用法：/block_chat <名称> <时间> (0 至关闭)");

		return;
	}

	const char* name = arg1;
	long lBlockDuration = parse_time_str(argument);

	if (lBlockDuration < 0)
	{
		return;
	}
	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(name);

	if (!tch)
	{
		CCI* pkCCI = P2P_MANAGER::instance().Find(name);

		if (pkCCI)
		{
			TPacketGGBlockChat p;

			p.bHeader = HEADER_GG_BLOCK_CHAT;
			strlcpy(p.szName, name, sizeof(p.szName));
			p.lBlockDuration = lBlockDuration;
			P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGBlockChat));
		}
		else
		{
			TPacketBlockChat p;

			strlcpy(p.szName, name, sizeof(p.szName));
			p.lDuration = lBlockDuration;
			db_clientdesc->DBPacket(HEADER_GD_BLOCK_CHAT, ch ? ch->GetDesc()->GetHandle() : 0, &p, sizeof(p));
		}

		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "禁言操作已成功.");

		return;
	}

	if (tch && ch != tch)
		tch->AddAffect(AFFECT_BLOCK_CHAT, POINT_NONE, 0, AFF_NONE, lBlockDuration, 0, true);
}
// END_OF_BLOCK_CHAT

// BUILD_BUILDING
ACMD(do_build)
{
	using namespace building;

	char arg1[256], arg2[256], arg3[256], arg4[256];
	const char* line = one_argument(argument, arg1, sizeof(arg1));
	BYTE GMLevel = ch->GetGMLevel();

	CLand* pkLand = CManager::instance().FindLand(ch->GetMapIndex(), ch->GetX(), ch->GetY());

	if (!pkLand)
	{
		sys_err("%s trying to build on not buildable area.", ch->GetName());
		return;
	}

	if (!*arg1)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, "输入命令无效：无此命令");
		return;
	}

	if (GMLevel == GM_PLAYER)
	{
		if ((!ch->GetGuild() || ch->GetGuild()->GetID() != pkLand->GetOwner()))
		{
			sys_err("%s trying to build on not owned land.", ch->GetName());
			return;
		}

		if (ch->GetGuild()->GetMasterPID() != ch->GetPlayerID())
		{
			sys_err("%s trying to build while not the guild master.", ch->GetName());
			return;
		}
	}

	switch (LOWER(*arg1))
	{
	case 'c':
	{
		// /build c vnum x y x_rot y_rot z_rot
		char arg5[256], arg6[256];
		line = one_argument(two_arguments(line, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3)); // vnum x y
		one_argument(two_arguments(line, arg4, sizeof(arg4), arg5, sizeof(arg5)), arg6, sizeof(arg6)); // x_rot y_rot z_rot

		if (!*arg1 || !*arg2 || !*arg3 || !*arg4 || !*arg5 || !*arg6)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, "无效的语法");
			return;
		}

		DWORD dwVnum = 0;
		str_to_number(dwVnum, arg1);

		using namespace building;

		const TObjectProto* t = CManager::instance().GetObjectProto(dwVnum);
		if (!t)
		{
			return;
		}

		const DWORD BUILDING_MAX_PRICE = 100000000;

		if (t->dwGroupVnum)
		{
			if (pkLand->FindObjectByGroup(t->dwGroupVnum))
			{
				return;
			}
		}

		if (t->dwDependOnGroupVnum)
		{
			{
				if (!pkLand->FindObjectByGroup(t->dwDependOnGroupVnum))
				{
					return;
				}
			}
		}

		if (test_server || GMLevel == GM_PLAYER)
		{
			if (t->dwPrice > BUILDING_MAX_PRICE)
			{
				return;
			}

			if (ch->GetGold() < (int)t->dwPrice)
			{
				return;
			}

			int i;
			for (i = 0; i < OBJECT_MATERIAL_MAX_NUM; ++i)
			{
				DWORD dwItemVnum = t->kMaterials[i].dwItemVnum;
				DWORD dwItemCount = t->kMaterials[i].dwCount;

				if (dwItemVnum == 0)
					break;

				if ((int)dwItemCount > ch->CountSpecifyItem(dwItemVnum))
				{
					return;
				}
			}
		}

		float x_rot = atof(arg4);
		float y_rot = atof(arg5);
		float z_rot = atof(arg6);

		long map_x = 0;
		str_to_number(map_x, arg2);
		long map_y = 0;
		str_to_number(map_y, arg3);

		bool isSuccess = pkLand->RequestCreateObject(dwVnum,
			ch->GetMapIndex(),
			map_x,
			map_y,
			x_rot,
			y_rot,
			z_rot, true);

		if (!isSuccess)
		{
			return;
		}

		if (test_server || GMLevel == GM_PLAYER)

		{
#ifdef ENABLE_EXTENDED_YANG_LIMIT
			ch->PointChange(POINT_GOLD, -static_cast<int64_t>(t->dwPrice));
#else
			ch->PointChange(POINT_GOLD, -t->dwPrice);
#endif

			{
				int i;
				for (i = 0; i < OBJECT_MATERIAL_MAX_NUM; ++i)
				{
					DWORD dwItemVnum = t->kMaterials[i].dwItemVnum;
					DWORD dwItemCount = t->kMaterials[i].dwCount;

					if (dwItemVnum == 0)
						break;
					ch->RemoveSpecifyItem(dwItemVnum, dwItemCount);
				}
			}
		}
	}
	break;

	case 'd':
		// build (d)elete ObjectID
	{
		one_argument(line, arg1, sizeof(arg1));

		if (!*arg1)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, "无效的语法");
			return;
		}

		DWORD vid = 0;
		str_to_number(vid, arg1);
		pkLand->RequestDeleteObjectByVID(vid);
	}
	break;

	// BUILD_WALL

	// build w n/e/w/s
	case 'w':
		if (GMLevel > GM_PLAYER)
		{
			int mapIndex = ch->GetMapIndex();

			one_argument(line, arg1, sizeof(arg1));

			switch (arg1[0])
			{
			case 's':
				pkLand->RequestCreateWall(mapIndex, 0.0f);
				break;
			case 'n':
				pkLand->RequestCreateWall(mapIndex, 180.0f);
				break;
			case 'e':
				pkLand->RequestCreateWall(mapIndex, 90.0f);
				break;
			case 'w':
				pkLand->RequestCreateWall(mapIndex, 270.0f);
				break;
			default:
				ch->ChatPacket(CHAT_TYPE_INFO, "guild.wall.build unknown_direction[%s]", arg1);
				sys_err("guild.wall.build unknown_direction[%s]", arg1);
				break;
			}
		}
		break;

	case 'e':
		if (GMLevel > GM_PLAYER)
		{
			pkLand->RequestDeleteWall();
		}
		break;

	case 'W':

		if (GMLevel > GM_PLAYER)
		{
			int setID = 0, wallSize = 0;
			char arg5[256], arg6[256];
			line = two_arguments(line, arg1, sizeof(arg1), arg2, sizeof(arg2));
			line = two_arguments(line, arg3, sizeof(arg3), arg4, sizeof(arg4));
			two_arguments(line, arg5, sizeof(arg5), arg6, sizeof(arg6));

			str_to_number(setID, arg1);
			str_to_number(wallSize, arg2);

			if (setID != 14105 && setID != 14115 && setID != 14125)
			{
				break;
			}
			else
			{
				bool door_east = false;
				str_to_number(door_east, arg3);
				bool door_west = false;
				str_to_number(door_west, arg4);
				bool door_south = false;
				str_to_number(door_south, arg5);
				bool door_north = false;
				str_to_number(door_north, arg6);
				pkLand->RequestCreateWallBlocks(setID, ch->GetMapIndex(), wallSize, door_east, door_west, door_south, door_north);
			}
		}
		break;

	case 'E':

		if (GMLevel > GM_PLAYER)
		{
			one_argument(line, arg1, sizeof(arg1));
			DWORD id = 0;
			str_to_number(id, arg1);
			pkLand->RequestDeleteWallBlocks(id);
		}
		break;

	default:
		ch->ChatPacket (CHAT_TYPE_INFO, "输入的命令无效 %s", arg1);
		break;
	}
}
// END_OF_BUILD_BUILDING

ACMD(do_clear_quest)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	quest::PC* pPC = quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID());
	pPC->ClearQuest(arg1);
}

ACMD(do_horse_state)
{
	ch->ChatPacket(CHAT_TYPE_INFO, "战马资料:");
	ch->ChatPacket(CHAT_TYPE_INFO, "等级 %d", ch->GetHorseLevel());
	ch->ChatPacket(CHAT_TYPE_INFO, "体力 %d/%d (%d%%", ch->GetHorseHealth(), ch->GetHorseMaxHealth(), ch->GetHorseHealth() * 100 / ch->GetHorseMaxHealth());
	ch->ChatPacket(CHAT_TYPE_INFO, "耐力 %d/%d (%d%%)", ch->GetHorseStamina(), ch->GetHorseMaxStamina(), ch->GetHorseStamina() * 100 / ch->GetHorseMaxStamina());
}

ACMD(do_horse_level)
{
	char arg1[256] = { 0 };
	char arg2[256] = { 0 };
	LPCHARACTER victim;
	int	level = 0;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, "用法 : /horse_level <角色> <马等级>");
		return;
	}

	victim = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (NULL == victim)
	{
		return;
	}

	str_to_number(level, arg2);
	level = MINMAX(0, level, HORSE_MAX_LEVEL);

	ch->ChatPacket (CHAT_TYPE_INFO, "马等级已设置为  (%s: %d)", victim->GetName(), level);

	victim->SetHorseLevel(level);
	victim->ComputePoints();
	victim->SkillLevelPacket();
	return;
}

ACMD(do_horse_ride)
{
	if (ch->IsHorseRiding())
		ch->StopRiding();
	else
		ch->StartRiding();
}

ACMD(do_horse_summon)
{
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	if (ch->IsRidingMount())
		return;
#endif
	ch->HorseSummon(true, true);
}

ACMD(do_horse_unsummon)
{
	ch->HorseSummon(false, true);
}

ACMD(do_horse_set_stat)
{
	char arg1[256], arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (*arg1 && *arg2)
	{
		HP_LL hp = 0;
		str_to_number(hp, arg1);
		int stam = 0;
		str_to_number(stam, arg2);
		ch->UpdateHorseHealth(hp - ch->GetHorseHealth());
		ch->UpdateHorseStamina(stam - ch->GetHorseStamina());
	}
	else
	{
		ch->ChatPacket (CHAT_TYPE_INFO, "用法 : /horse_set_stat <hp> <耐力>");
	}
}

ACMD(do_save_attribute_to_image) // command "/saveati" for alias
{
	char szFileName[256];
	char szMapIndex[256];

	two_arguments(argument, szMapIndex, sizeof(szMapIndex), szFileName, sizeof(szFileName));

	if (!*szMapIndex || !*szFileName)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "语法: /saveati <map_index> <文件名>");
		return;
	}

	long lMapIndex = 0;
	str_to_number(lMapIndex, szMapIndex);

	if (SECTREE_MANAGER::instance().SaveAttributeToImage(lMapIndex, szFileName))
		ch->ChatPacket(CHAT_TYPE_INFO, "保存完成.");
	else
		ch->ChatPacket(CHAT_TYPE_INFO, "保存失败.");
}

ACMD(do_affect_remove)
{
	char arg1[256];
	char arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "用法: /affect_remove <角色名字>");
		ch->ChatPacket(CHAT_TYPE_INFO, "用法: /affect_remove <类型> <点>");

		LPCHARACTER tch = ch;

		if (*arg1)
			if (!(tch = CHARACTER_MANAGER::instance().FindPC(arg1)))
				tch = ch;

		ch->ChatPacket(CHAT_TYPE_INFO, "--影响列表 %s -------------------------------", tch->GetName());
		ch->ChatPacket(CHAT_TYPE_INFO, "类型点修改持续时间标志");

		const std::list<CAffect*>& cont = tch->GetAffectContainer();

		itertype(cont) it = cont.begin();

		while (it != cont.end())
		{
			CAffect* pkAff = *it++;

			ch->ChatPacket(CHAT_TYPE_INFO, "%4d %5d %5d %8d %u",
				pkAff->dwType, pkAff->bApplyOn, pkAff->lApplyValue, pkAff->lDuration, pkAff->dwFlag);
		}
		return;
	}

	bool removed = false;

	CAffect* af;

	DWORD	type = 0;
	str_to_number(type, arg1);
	BYTE	point = 0;
	str_to_number(point, arg2);
	while ((af = ch->FindAffect(type, point)))
	{
		ch->RemoveAffect(af);
		removed = true;
	}

	if (removed)
		ch->ChatPacket(CHAT_TYPE_INFO, "效果已成功消除.");
	else
		ch->ChatPacket(CHAT_TYPE_INFO, "该效果无法删除.");
}

ACMD(do_change_attr)
{
	LPITEM weapon = ch->GetWear(WEAR_WEAPON);
	if (weapon)
		weapon->ChangeAttribute();
}

ACMD(do_add_attr)
{
	LPITEM weapon = ch->GetWear(WEAR_WEAPON);
	if (weapon)
		weapon->AddAttribute();
}

ACMD(do_add_socket)
{
	LPITEM weapon = ch->GetWear(WEAR_WEAPON);
	if (weapon)
		weapon->AddSocket();
}

#ifdef ENABLE_NEWSTUFF
ACMD(do_change_rare_attr)
{
	LPITEM weapon = ch->GetWear(WEAR_WEAPON);
	if (weapon)
		weapon->ChangeRareAttribute();
}

ACMD(do_add_rare_attr)
{
	LPITEM weapon = ch->GetWear(WEAR_WEAPON);
	if (weapon)
		weapon->AddRareAttribute();
}
#endif

ACMD(do_show_arena_list)
{
	CArenaManager::instance().SendArenaMapListTo(ch);
}

ACMD(do_end_all_duel)
{
	CArenaManager::instance().EndAllDuel();
}

ACMD(do_end_duel)
{
	char szName[256];

	one_argument(argument, szName, sizeof(szName));

	LPCHARACTER pChar = CHARACTER_MANAGER::instance().FindPC(szName);
	if (pChar == NULL)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("粮犁窍瘤 臼绰 某腐磐 涝聪促."));
		return;
	}

	if (CArenaManager::instance().EndDuel(pChar->GetPlayerID()) == false)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("措访 碍力 辆丰 角菩"));
	}
	else
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("措访 碍力 辆丰 己傍"));
	}
}

ACMD(do_duel)
{
	char szName1[256];
	char szName2[256];
	char szSet[256];
	char szMinute[256];
	int set = 0;
	int minute = 0;

	argument = two_arguments(argument, szName1, sizeof(szName1), szName2, sizeof(szName2));
	two_arguments(argument, szSet, sizeof(szSet), szMinute, sizeof(szMinute));

	str_to_number(set, szSet);

	if (set < 0) set = 1;
	if (set > 5) set = 5;

	if (!str_to_number(minute, szMinute))
		minute = 5;

	if (minute < 5)
		minute = 5;

	LPCHARACTER pChar1 = CHARACTER_MANAGER::instance().FindPC(szName1);
	LPCHARACTER pChar2 = CHARACTER_MANAGER::instance().FindPC(szName2);

	if (pChar1 != NULL && pChar2 != NULL)
	{
		pChar1->RemoveGoodAffect();
		pChar2->RemoveGoodAffect();

		pChar1->RemoveBadAffect();
		pChar2->RemoveBadAffect();

		LPPARTY pParty = pChar1->GetParty();
		if (pParty != NULL)
		{
			if (pParty->GetMemberCount() == 2)
			{
				CPartyManager::instance().DeleteParty(pParty);
			}
			else
			{
				pChar1->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<颇萍> 颇萍俊辑 唱啊继嚼聪促."));
				pParty->Quit(pChar1->GetPlayerID());
			}
		}

		pParty = pChar2->GetParty();
		if (pParty != NULL)
		{
			if (pParty->GetMemberCount() == 2)
			{
				CPartyManager::instance().DeleteParty(pParty);
			}
			else
			{
				pChar2->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<颇萍> 颇萍俊辑 唱啊继嚼聪促."));
				pParty->Quit(pChar2->GetPlayerID());
			}
		}

		if (CArenaManager::instance().StartDuel(pChar1, pChar2, set, minute) == true)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("措访捞 己傍利栏肺 矫累 登菌嚼聪促."));
		}
		else
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("措访 矫累俊 巩力啊 乐嚼聪促."));
		}
	}
}

#define ENABLE_STATPLUS_NOLIMIT
ACMD(do_stat_plus_amount)
{
	char szPoint[256];

	one_argument(argument, szPoint, sizeof(szPoint));

	if (*szPoint == '\0')
		return;

	if (ch->IsPolymorphed())
	{
		return;
	}

	int nRemainPoint = ch->GetPoint(POINT_STAT);

	if (nRemainPoint <= 0)
	{
		return;
	}

	int nPoint = 0;
	str_to_number(nPoint, szPoint);

	if (nRemainPoint < nPoint)
	{
		return;
	}

	if (nPoint < 0)
	{
		return;
	}

#ifndef ENABLE_STATPLUS_NOLIMIT
	switch (subcmd)
	{
	case POINT_HT:
		if (nPoint + ch->GetPoint(POINT_HT) > 90)
		{
			nPoint = 90 - ch->GetPoint(POINT_HT);
		}
		break;

	case POINT_IQ:
		if (nPoint + ch->GetPoint(POINT_IQ) > 90)
		{
			nPoint = 90 - ch->GetPoint(POINT_IQ);
		}
		break;

	case POINT_ST:
		if (nPoint + ch->GetPoint(POINT_ST) > 90)
		{
			nPoint = 90 - ch->GetPoint(POINT_ST);
		}
		break;

	case POINT_DX:
		if (nPoint + ch->GetPoint(POINT_DX) > 90)
		{
			nPoint = 90 - ch->GetPoint(POINT_DX);
		}
		break;

	default:
		return;
		break;
	}
#endif

	if (nPoint != 0)
	{
		ch->SetRealPoint(subcmd, ch->GetRealPoint(subcmd) + nPoint);
		ch->SetPoint(subcmd, ch->GetPoint(subcmd) + nPoint);
		ch->ComputePoints();
		ch->PointChange(subcmd, 0);

		ch->PointChange(POINT_STAT, -nPoint);
		ch->ComputePoints();
	}
}

struct tTwoPID
{
	int pid1;
	int pid2;
};

ACMD(do_break_marriage)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	tTwoPID pids = { 0, 0 };

	str_to_number(pids.pid1, arg1);
	str_to_number(pids.pid2, arg2);
	db_clientdesc->DBPacket(HEADER_GD_BREAK_MARRIAGE, 0, &pids, sizeof(pids));
}

ACMD(do_effect)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	int	effect_type = 0;
	str_to_number(effect_type, arg1);
	ch->EffectPacket(effect_type);
}

struct FCountInMap
{
	int m_Count[4];
	FCountInMap() { memset(m_Count, 0, sizeof(int) * 4); }
	void operator()(LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER)ent;
			if (ch && ch->IsPC())
				++m_Count[ch->GetEmpire()];
		}
	}
	int GetCount(BYTE bEmpire) { return m_Count[bEmpire]; }
};

ACMD(do_reset_subskill)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "用法: reset_subskill <角色>");
		return;
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (tch == NULL)
		return;

	tch->ClearSubSkill();
	ch->ChatPacket(CHAT_TYPE_INFO, "[%s] 的子技能已重置", tch->GetName());
}

ACMD(do_flush)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (0 == arg1[0])
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "用法 : /flush player_id");
		return;
	}

	DWORD pid = (DWORD)strtoul(arg1, NULL, 10);

	db_clientdesc->DBPacketHeader(HEADER_GD_FLUSH_CACHE, 0, sizeof(DWORD));
	db_clientdesc->Packet(&pid, sizeof(DWORD));
}

ACMD(do_weeklyevent)
{
	char arg1[256];
	int empire = 0;

	if (CBattleArena::instance().IsRunning() == false)
	{
		one_argument(argument, arg1, sizeof(arg1));

		empire = strtol(arg1, NULL, 10);

		if (empire == 1 || empire == 2 || empire == 3)
		{
			CBattleArena::instance().Start(empire);
		}
		else
		{
			CBattleArena::instance().Start(rand() % 3 + 1);
		}
		ch->ChatPacket(CHAT_TYPE_INFO, "每周活动开始");
	}
	else
	{
		CBattleArena::instance().ForceEnd();
		ch->ChatPacket(CHAT_TYPE_INFO, "每周活动结束");
	}
}

struct FMobCounter
{
	int nCount;

	void operator () (LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER pChar = static_cast<LPCHARACTER>(ent);

			if (pChar->IsMonster() == true || pChar->IsStone())
			{
				nCount++;
			}
		}
	}
};

ACMD(do_get_mob_count)
{
	LPSECTREE_MAP pSectree = SECTREE_MANAGER::instance().GetMap(ch->GetMapIndex());

	if (pSectree == NULL)
		return;

	FMobCounter f;
	f.nCount = 0;

	pSectree->for_each(f);

	ch->ChatPacket(CHAT_TYPE_INFO, "地图ID: %d 怪物数量 %d", ch->GetMapIndex(), f.nCount);
}

ACMD(do_clear_land)
{
	const building::CLand* pLand = building::CManager::instance().FindLand(ch->GetMapIndex(), ch->GetX(), ch->GetY());

	if (NULL == pLand)
	{
		return;
	}

	ch->ChatPacket(CHAT_TYPE_INFO, "帮会土地(%d)已清理", pLand->GetID());

	building::CManager::instance().ClearLand(pLand->GetID());
}

ACMD(do_special_item)
{
	ITEM_MANAGER::instance().ConvSpecialDropItemFile();
}

ACMD(do_set_stat)
{
	char szName[256];
	char szChangeAmount[256];

	two_arguments(argument, szName, sizeof(szName), szChangeAmount, sizeof(szChangeAmount));

	if (*szName == '\0' || *szChangeAmount == '\0')
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "无效参数.");
		return;
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(szName);

	if (!tch)
	{
		CCI* pkCCI = P2P_MANAGER::instance().Find(szName);

		if (pkCCI)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "找不到玩家(%s),%s不在您的游戏服务器中.", szName, szName);
			return;
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "找不到玩家(%s) 也许%s没有登录游戏.", szName, szName);
			return;
		}
	}
	else
	{
		if (tch->IsPolymorphed())
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("敌癌 吝俊绰 瓷仿阑 棵副 荐 绝嚼聪促."));
			return;
		}

		if (subcmd != POINT_HT && subcmd != POINT_IQ && subcmd != POINT_ST && subcmd != POINT_DX)
		{
			return;
		}
		int nRemainPoint = tch->GetPoint(POINT_STAT);
		int nCurPoint = tch->GetRealPoint(subcmd);
		int nChangeAmount = 0;
		str_to_number(nChangeAmount, szChangeAmount);
		int nPoint = nCurPoint + nChangeAmount;

		int n = -1;
		switch (subcmd)
		{
		case POINT_HT:
			if (nPoint < JobInitialPoints[tch->GetJob()].ht)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Cannot set stat under initial stat."));
				return;
			}
			n = 0;
			break;
		case POINT_IQ:
			if (nPoint < JobInitialPoints[tch->GetJob()].iq)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Cannot set stat under initial stat."));
				return;
			}
			n = 1;
			break;
		case POINT_ST:
			if (nPoint < JobInitialPoints[tch->GetJob()].st)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Cannot set stat under initial stat."));
				return;
			}
			n = 2;
			break;
		case POINT_DX:
			if (nPoint < JobInitialPoints[tch->GetJob()].dx)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Cannot set stat under initial stat."));
				return;
			}
			n = 3;
			break;
		}

		if (nPoint > 90)
		{
			nChangeAmount -= nPoint - 90;
			nPoint = 90;
		}

		if (nRemainPoint < nChangeAmount)
		{
			return;
		}

		tch->SetRealPoint(subcmd, nPoint);
		tch->SetPoint(subcmd, tch->GetPoint(subcmd) + nChangeAmount);
		tch->ComputePoints();
		tch->PointChange(subcmd, 0);

		tch->PointChange(POINT_STAT, -nChangeAmount);
		tch->ComputePoints();

		const char* stat_name[4] = { "con", "int", "str", "dex" };
		if (-1 == n)
			return;
		ch->ChatPacket(CHAT_TYPE_INFO, "%s's %s change %d to %d", szName, stat_name[n], nCurPoint, nPoint);
	}
}

ACMD(do_get_item_id_list)
{
	for (int i = 0; i < INVENTORY_AND_EQUIP_SLOT_MAX; i++)
	{
		LPITEM item = ch->GetInventoryItem(i);
		if (item != NULL)
			ch->ChatPacket(CHAT_TYPE_INFO, "单元格：%d，名称：%s，ID：%d", item->GetCell(), item->GetName(), item->GetID());
	}
}

ACMD(do_set_socket)
{
	char arg1[256];
	char arg2[256];
	char arg3[256];

	one_argument(two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3));

	int item_id, socket_num, value;
	if (!str_to_number(item_id, arg1) || !str_to_number(socket_num, arg2) || !str_to_number(value, arg3))
		return;

	LPITEM item = ITEM_MANAGER::instance().Find(item_id);
	if (item)
		item->SetSocket(socket_num, value);
}

ACMD(do_can_dead)
{
	if (subcmd)
		ch->SetArmada();
	else
		ch->ResetArmada();
}

ACMD(do_all_skill_master)
{
	ch->SetHorseLevel(SKILL_MAX_LEVEL);
	for (int i = 0; i < SKILL_MAX_NUM; i++)
	{
		if (true == ch->CanUseSkill(i))
		{
			switch (i)
			{
				// @fixme154 BEGIN
				// taking out the it->second->bMaxLevel from map_pkSkillProto (&& 1==40|SKILL_MAX_LEVEL) will be very resource-wasting, so we go full ugly so far
			case SKILL_COMBO:
				ch->SetSkillLevel(i, 2);
				break;
			case SKILL_LANGUAGE1:
			case SKILL_LANGUAGE2:
			case SKILL_LANGUAGE3:
				ch->SetSkillLevel(i, 20);
				break;
			case SKILL_HORSE_SUMMON:
				ch->SetSkillLevel(i, 10);
				break;
			case SKILL_HORSE:
				ch->SetSkillLevel(i, HORSE_MAX_LEVEL);
				break;
				// CanUseSkill will be true for skill_horse_skills if riding
			case SKILL_HORSE_WILDATTACK:
			case SKILL_HORSE_CHARGE:
			case SKILL_HORSE_ESCAPE:
			case SKILL_HORSE_WILDATTACK_RANGE:
				ch->SetSkillLevel(i, 20);
				break;
				// @fixme154 END
			default:
				ch->SetSkillLevel(i, SKILL_MAX_LEVEL - 10);
				break;
			}
		}
		else
		{
			switch (i)
			{
			case SKILL_HORSE_WILDATTACK:
			case SKILL_HORSE_CHARGE:
			case SKILL_HORSE_ESCAPE:
			case SKILL_HORSE_WILDATTACK_RANGE:
				ch->SetSkillLevel(i, 20); // @fixme154 40 -> 20
				break;
#ifdef ENABLE_PASSIVE_SKILLS
			case SKILL_PASSIVE_MONSTER:
			case SKILL_PASSIVE_STONE:
			case SKILL_PASSIVE_BERSERKER:
			case SKILL_PASSIVE_HUMAN:
			case SKILL_PASSIVE_SKILL_COOLDOWN:
			case SKILL_PASSIVE_DRAGON_HEARTH:
			case SKILL_PASSIVE_HERBOLOGY:
			case SKILL_PASSIVE_UPGRADING:
				break;
#endif
#ifdef ENABLE_BUFFI_SYSTEM
			case SKILL_BUFFI_1:
			case SKILL_BUFFI_2:
			case SKILL_BUFFI_3:
			case SKILL_BUFFI_4:
			case SKILL_BUFFI_5:
				ch->SetSkillLevel(i, SKILL_MAX_LEVEL - 10);
				break;
#endif
			}
		}
	}
	ch->ComputePoints();
	ch->SkillLevelPacket();
}

ACMD(do_item_full_set)
{
	BYTE job = ch->GetJob();
	LPITEM item;
	for (int i = 0; i < 6; i++)
	{
		item = ch->GetWear(i);
		if (item != NULL)
			ch->UnequipItem(item);
	}
	item = ch->GetWear(WEAR_SHIELD);
	if (item != NULL)
		ch->UnequipItem(item);

	switch (job)
	{
	case JOB_SURA:
	{
		item = ITEM_MANAGER::instance().CreateItem(11699);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(13049);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(15189);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(189);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(12529);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(14109);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(17209);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(16209);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
	}
	break;
	case JOB_WARRIOR:
	{
		item = ITEM_MANAGER::instance().CreateItem(11299);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(13049);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(15189);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(3159);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(12249);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(14109);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(17109);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(16109);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
	}
	break;
	case JOB_SHAMAN:
	{
		item = ITEM_MANAGER::instance().CreateItem(11899);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(13049);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(15189);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(7159);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(12669);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(14109);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(17209);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(16209);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
	}
	break;
	case JOB_ASSASSIN:
	{
		item = ITEM_MANAGER::instance().CreateItem(11499);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(13049);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(15189);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(1139);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(12389);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(14109);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(17189);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(16189);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
	}
	break;
#ifdef ENABLE_WOLFMAN_CHARACTER
	case JOB_WOLFMAN:
	{
		item = ITEM_MANAGER::instance().CreateItem(21049);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(13049);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(15189);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(6049);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(21559);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(14109);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(17209);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
		item = ITEM_MANAGER::instance().CreateItem(16209);
		if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
			M2_DESTROY_ITEM(item);
	}
	break;
#endif
	}
}

ACMD(do_attr_full_set)
{
	BYTE job = ch->GetJob();
	LPITEM item;

	switch (job)
	{
	case JOB_WARRIOR:
	case JOB_ASSASSIN:
	case JOB_SURA:
	case JOB_SHAMAN:
#ifdef ENABLE_WOLFMAN_CHARACTER
	case JOB_WOLFMAN:
#endif
	{
		item = ch->GetWear(WEAR_HEAD);
		if (item != NULL)
		{
			item->ClearAttribute();
			item->SetForceAttribute(0, APPLY_ATT_SPEED, 8);
			item->SetForceAttribute(1, APPLY_HP_REGEN, 30);
			item->SetForceAttribute(2, APPLY_SP_REGEN, 30);
			item->SetForceAttribute(3, APPLY_DODGE, 15);
			item->SetForceAttribute(4, APPLY_STEAL_SP, 10);
		}

		item = ch->GetWear(WEAR_WEAPON);
		if (item != NULL)
		{
			item->ClearAttribute();
			item->SetForceAttribute(0, APPLY_CAST_SPEED, 20);
			item->SetForceAttribute(1, APPLY_CRITICAL_PCT, 10);
			item->SetForceAttribute(2, APPLY_PENETRATE_PCT, 10);
			item->SetForceAttribute(3, APPLY_ATTBONUS_DEVIL, 20);
			item->SetForceAttribute(4, APPLY_STR, 12);
		}

		item = ch->GetWear(WEAR_SHIELD);
		if (item != NULL)
		{
			item->ClearAttribute();
			item->SetForceAttribute(0, APPLY_CON, 12);
			item->SetForceAttribute(1, APPLY_BLOCK, 15);
			item->SetForceAttribute(2, APPLY_REFLECT_MELEE, 10);
			item->SetForceAttribute(3, APPLY_IMMUNE_STUN, 1);
			item->SetForceAttribute(4, APPLY_IMMUNE_SLOW, 1);
		}

		item = ch->GetWear(WEAR_BODY);
		if (item != NULL)
		{
			item->ClearAttribute();
			item->SetForceAttribute(0, APPLY_MAX_HP, 2000);
			item->SetForceAttribute(1, APPLY_CAST_SPEED, 20);
			item->SetForceAttribute(2, APPLY_STEAL_HP, 10);
			item->SetForceAttribute(3, APPLY_REFLECT_MELEE, 10);
			item->SetForceAttribute(4, APPLY_ATT_GRADE_BONUS, 50);
		}

		item = ch->GetWear(WEAR_FOOTS);
		if (item != NULL)
		{
			item->ClearAttribute();
			item->SetForceAttribute(0, APPLY_MAX_HP, 2000);
			item->SetForceAttribute(1, APPLY_MAX_SP, 80);
			item->SetForceAttribute(2, APPLY_MOV_SPEED, 8);
			item->SetForceAttribute(3, APPLY_ATT_SPEED, 8);
			item->SetForceAttribute(4, APPLY_CRITICAL_PCT, 10);
		}

		item = ch->GetWear(WEAR_WRIST);
		if (item != NULL)
		{
			item->ClearAttribute();
			item->SetForceAttribute(0, APPLY_MAX_HP, 2000);
			item->SetForceAttribute(1, APPLY_MAX_SP, 80);
			item->SetForceAttribute(2, APPLY_PENETRATE_PCT, 10);
			item->SetForceAttribute(3, APPLY_STEAL_HP, 10);
			item->SetForceAttribute(4, APPLY_MANA_BURN_PCT, 10);
		}
		item = ch->GetWear(WEAR_NECK);
		if (item != NULL)
		{
			item->ClearAttribute();
			item->SetForceAttribute(0, APPLY_MAX_HP, 2000);
			item->SetForceAttribute(1, APPLY_MAX_SP, 80);
			item->SetForceAttribute(2, APPLY_CRITICAL_PCT, 10);
			item->SetForceAttribute(3, APPLY_PENETRATE_PCT, 10);
			item->SetForceAttribute(4, APPLY_STEAL_SP, 10);
		}
		item = ch->GetWear(WEAR_EAR);
		if (item != NULL)
		{
			item->ClearAttribute();
			item->SetForceAttribute(0, APPLY_MOV_SPEED, 20);
			item->SetForceAttribute(1, APPLY_MANA_BURN_PCT, 10);
			item->SetForceAttribute(2, APPLY_POISON_REDUCE, 5);
			item->SetForceAttribute(3, APPLY_ATTBONUS_DEVIL, 20);
			item->SetForceAttribute(4, APPLY_ATTBONUS_UNDEAD, 20);
		}
	}
	break;
	}
}

ACMD(do_full_set)
{
	do_all_skill_master(ch, NULL, 0, 0);
	do_item_full_set(ch, NULL, 0, 0);
	do_attr_full_set(ch, NULL, 0, 0);
}

ACMD(do_use_item)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	int cell = 0;
	str_to_number(cell, arg1);

	LPITEM item = ch->GetInventoryItem(cell);
	if (item)
	{
		ch->UseItem(TItemPos(INVENTORY, cell));
	}
	else
	{
		ch->ChatPacket (CHAT_TYPE_INFO, "酒捞袍捞 绝绢辑 馒侩且 荐 绝绢.");
	}
}

ACMD(do_clear_affect)
{
	ch->ClearAffect(true);
}

ACMD(do_dragon_soul)
{
	char arg1[512];
	const char* rest = one_argument(argument, arg1, sizeof(arg1));
	switch (arg1[0])
	{
	case 'a':
	{
		one_argument(rest, arg1, sizeof(arg1));
		int deck_idx;
		if (str_to_number(deck_idx, arg1) == false)
		{
			return;
		}
		ch->DragonSoul_ActivateDeck(deck_idx);
	}
	break;
	case 'd':
	{
		ch->DragonSoul_DeactivateAll();
	}
	break;
	}
}

ACMD(do_ds_list)
{
	for (int i = 0; i < DRAGON_SOUL_INVENTORY_MAX_NUM; i++)
	{
		TItemPos cell(DRAGON_SOUL_INVENTORY, i);

		LPITEM item = ch->GetItem(cell);
		if (item != NULL)
			ch->ChatPacket(CHAT_TYPE_INFO, "cell : %d, name : %s, id : %d", item->GetCell(), item->GetName(), item->GetID());
	}
}

#ifdef ENABLE_OFFLINE_SHOP
std::string GetNewShopName(const std::string& oldname, const std::string& newname)
{
	auto nameindex = oldname.find('@');
	if (nameindex == std::string::npos)
	{
		return newname;
	}
	else
	{
		auto playername = oldname.substr(0, nameindex);
		return playername + '@' + newname;
	}
}

ACMD(do_offshop_change_shop_name)
{
	char arg1[50]; char arg2[256];
	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));

	if (arg1[0] != 0 && isdigit(arg1[0]) && arg2[0] != 0)
	{
		DWORD id = 0;
		str_to_number(id, arg1);

		if (id == 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("syntax: offshop_change_shop_name <player-id> <new-name>"));
			return;
		}
		else
		{
			offlineshop::CShop* pkShop = offlineshop::GetManager().GetShopByOwnerID(id);
			if (!pkShop)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Cannot find shop by id %u"));
				return;
			}
			else
			{
				std::string oldname = pkShop->GetName();
				offlineshop::GetManager().SendShopChangeNameDBPacket(id, GetNewShopName(oldname, arg2).c_str());
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("shop's name changed."));
			}
		}
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("syntax: offshop_change_shop_name <player-id> <new-name>"));
		return;
	}
}

ACMD(do_offshop_force_close_shop)
{
	char arg1[50];
	argument = one_argument(argument, arg1, sizeof(arg1));
	if (arg1[0] != 0 && isdigit(arg1[0]))
	{
		DWORD id = 0;
		str_to_number(id, arg1);

		if (id == 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("syntax: offshop_force_close_shop <player-id>"));
			return;
		}
		else
		{
			offlineshop::CShop* pkShop = offlineshop::GetManager().GetShopByOwnerID(id);
			if (!pkShop)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Cannot find shop by id %u"), id);
				return;
			}
			else
			{
				offlineshop::GetManager().SendShopForceCloseDBPacket(id);
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("shop closed successfully."));
			}
		}
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("syntax: offshop_force_close_shop <player-id>"));
		return;
	}
}
#endif

#ifdef ENABLE_HWID
ACMD(do_hwidban)
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "输入的名称无效");
		return;
	}

	char szQuery[256];
#ifdef ENABLE_PLAYER2_ACCOUNT2__
	snprintf(szQuery, sizeof(szQuery), "SELECT account_id FROM player2.player WHERE name = '%s'", arg1);
#else
	snprintf(szQuery, sizeof(szQuery), "SELECT account_id FROM player.player WHERE name = '%s'", arg1);
#endif
	std::unique_ptr<SQLMsg> msg(DBManager::instance().DirectQuery(szQuery));
	if (!msg->Get() || msg->Get()->uiNumRows == 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "输入的名称无效");
		return;
	}

	MYSQL_ROW row;
	row = mysql_fetch_row(msg->Get()->pSQLResult);
	DWORD account_id = 0;
	str_to_number(account_id, row[0]);
#ifdef ENABLE_PLAYER2_ACCOUNT2__
	snprintf(szQuery, sizeof(szQuery), "SELECT hwidID, hwidID2 FROM account2.account WHERE id = %u", account_id);
#else
	snprintf(szQuery, sizeof(szQuery), "SELECT hwidID, hwidID2 FROM account.account WHERE id = %u", account_id);
#endif
	std::unique_ptr<SQLMsg> msg2(DBManager::instance().DirectQuery(szQuery));
	if (!msg2->Get() || msg2->Get()->uiNumRows == 0)
	{
		return;
	}
	row = mysql_fetch_row(msg2->Get()->pSQLResult);

	DWORD hwidID[2];
	memset(hwidID, 0, sizeof(hwidID));
	str_to_number(hwidID[0], row[0]);
	str_to_number(hwidID[1], row[1]);

	if (hwidID[1] == 0)
	{
#ifdef ENABLE_PLAYER2_ACCOUNT2__
		snprintf(szQuery, sizeof(szQuery), "SELECT hwid FROM account2.hwid_list WHERE id = %u", hwidID[0]);
#else
		snprintf(szQuery, sizeof(szQuery), "SELECT hwid FROM account.hwid_list WHERE id = %u", hwidID[0]);
#endif
		
		std::unique_ptr<SQLMsg> msg3(DBManager::instance().DirectQuery(szQuery));
		if (!msg3->Get() || msg3->Get()->uiNumRows == 0)
		{
			return;
		}
		row = mysql_fetch_row(msg3->Get()->pSQLResult);
#ifdef ENABLE_PLAYER2_ACCOUNT2__
		snprintf(szQuery, sizeof(szQuery), "INSERT INTO account2.hwid_ban_list (hwid) VALUES('%s')", row[0]);
#else
		snprintf(szQuery, sizeof(szQuery), "INSERT INTO account.hwid_ban_list (hwid) VALUES('%s')", row[0]);
#endif
		DBManager::instance().DirectQuery(szQuery);
		CHwidManager::Instance().SetHwidBanGD(row[0]);
	}
	else
	{
		for (int i = 0; i < 2; i++)
		{
#ifdef ENABLE_PLAYER2_ACCOUNT2__
			snprintf(szQuery, sizeof(szQuery), "SELECT hwid FROM account2.hwid_list WHERE id = %u", hwidID[i]);
#else
			snprintf(szQuery, sizeof(szQuery), "SELECT hwid FROM account.hwid_list WHERE id = %u", hwidID[i]);
#endif
			std::unique_ptr<SQLMsg> msg3(DBManager::instance().DirectQuery(szQuery));
			if (!msg3->Get() || msg3->Get()->uiNumRows == 0)
			{
				return;
			}
			row = mysql_fetch_row(msg3->Get()->pSQLResult);
#ifdef ENABLE_PLAYER2_ACCOUNT2__
			snprintf(szQuery, sizeof(szQuery), "INSERT INTO account2.hwid_ban_list (hwid) VALUES('%s')", row[0]);
#else
			snprintf(szQuery, sizeof(szQuery), "INSERT INTO account.hwid_ban_list (hwid) VALUES('%s')", row[0]);
#endif
			DBManager::instance().DirectQuery(szQuery);
			CHwidManager::Instance().SetHwidBanGD(row[0]);
		}
	}

	std::vector<DWORD> v_accountList;
	if (hwidID[1] == 0)
	{
#ifdef ENABLE_PLAYER2_ACCOUNT2__
		snprintf(szQuery, sizeof(szQuery),"UPDATE account2.account SET `status` = 'BLOCK' WHERE hwidID = %u OR hwidID2 = %u",hwidID[0], hwidID[0]);
#else
		snprintf(szQuery, sizeof(szQuery),"UPDATE account.account SET `status` = 'BLOCK' WHERE hwidID = %u OR hwidID2 = %u",hwidID[0], hwidID[0]);
#endif
		DBManager::instance().DirectQuery(szQuery);
#ifdef ENABLE_PLAYER2_ACCOUNT2__
		snprintf(szQuery, sizeof(szQuery),"SELECT id FROM account2.account WHERE hwidID = %u OR hwidID2 = %u",hwidID[0], hwidID[0]);
#else
		snprintf(szQuery, sizeof(szQuery),"SELECT id FROM account.account WHERE hwidID = %u OR hwidID2 = %u",hwidID[0], hwidID[0]);
#endif
		std::unique_ptr<SQLMsg> msg4(DBManager::instance().DirectQuery(szQuery));
		if (!msg4->Get() || msg4->Get()->uiNumRows == 0)
		{
			return;
		}

		while ((row = mysql_fetch_row(msg4->Get()->pSQLResult)))
		{
			str_to_number(account_id, row[0]);
			v_accountList.push_back(account_id);
		}
	}
	else
	{
#ifdef ENABLE_PLAYER2_ACCOUNT2__
		snprintf(szQuery, sizeof(szQuery),"UPDATE account2.account SET `status` = 'BLOCK' WHERE hwidID = %u OR hwidID2 = %u OR hwidID = %u OR hwidID2 = %u",hwidID[0], hwidID[0], hwidID[1], hwidID[1]);
#else
		snprintf(szQuery, sizeof(szQuery),"UPDATE account.account SET `status` = 'BLOCK' WHERE hwidID = %u OR hwidID2 = %u OR hwidID = %u OR hwidID2 = %u",hwidID[0], hwidID[0], hwidID[1], hwidID[1]);
#endif
		DBManager::instance().DirectQuery(szQuery);
#ifdef ENABLE_PLAYER2_ACCOUNT2__
		snprintf(szQuery, sizeof(szQuery),"SELECT id FROM account2.account WHERE hwidID = %u OR hwidID2 = %u, OR hwidID = %u OR hwidID2 = %u",hwidID[0], hwidID[0], hwidID[1], hwidID[1]);
#else
		snprintf(szQuery, sizeof(szQuery),"SELECT id FROM account.account WHERE hwidID = %u OR hwidID2 = %u, OR hwidID = %u OR hwidID2 = %u",hwidID[0], hwidID[0], hwidID[1], hwidID[1]);
#endif
		std::unique_ptr<SQLMsg> msg4(DBManager::instance().DirectQuery(szQuery));
		if (!msg4->Get() || msg4->Get()->uiNumRows == 0)
		{
			return;
		}

		while ((row = mysql_fetch_row(msg4->Get()->pSQLResult)))
		{
			str_to_number(account_id, row[0]);
			v_accountList.push_back(account_id);
		}
	}

	std::vector<DWORD> v_playerList;
	for (const DWORD& acclist : v_accountList)
	{
#ifdef ENABLE_PLAYER2_ACCOUNT2__
		snprintf(szQuery, sizeof(szQuery),"SELECT id FROM player2.player WHERE account_id = %u",acclist);
#else
		snprintf(szQuery, sizeof(szQuery),"SELECT id FROM player.player WHERE account_id = %u",acclist);
#endif
		std::unique_ptr<SQLMsg> msg5(DBManager::instance().DirectQuery(szQuery));
		if (!msg5->Get() || msg5->Get()->uiNumRows == 0)
		{
			return;
		}

		while ((row = mysql_fetch_row(msg5->Get()->pSQLResult)))
		{
			DWORD pid;
			if (pid == ch->GetPlayerID())
				continue;
			str_to_number(pid, row[0]);
			v_playerList.push_back(pid);
		}
	}

	for (const DWORD& plist : v_playerList)
	{
		offlineshop::CShop* pkShop = offlineshop::GetManager().GetShopByOwnerID(plist);
		if (pkShop)
			offlineshop::GetManager().SendShopForceCloseDBPacket(plist);

		LPCHARACTER tch = CHARACTER_MANAGER::instance().FindByPID(plist);

		if (!tch || !tch->GetDesc())
			continue;

		if (ch->GetDesc() == tch->GetDesc())
			continue;

		tch->GetDesc()->SetPhase(PHASE_CLOSE);
	}
	ch->ChatPacket(CHAT_TYPE_INFO, "被封人的所有账户均会被封锁.");
}

ACMD(do_ban)
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "输入的名称无效");
		return;
	}

	DWORD playerID = 0;
	char szQuery[256];
#ifdef ENABLE_PLAYER2_ACCOUNT2__
	snprintf(szQuery, sizeof(szQuery), "SELECT account_id, id FROM player2.player WHERE name = '%s'", arg1);
#else
	snprintf(szQuery, sizeof(szQuery), "SELECT account_id, id FROM player.player WHERE name = '%s'", arg1);
#endif
	std::unique_ptr<SQLMsg> msg(DBManager::instance().DirectQuery(szQuery));
	if (!msg->Get() || msg->Get()->uiNumRows == 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "输入的名称无效");
		return;
	}

	MYSQL_ROW row;
	row = mysql_fetch_row(msg->Get()->pSQLResult);
	DWORD account_id = 0;
	str_to_number(account_id, row[0]);
	str_to_number(playerID, row[1]);
#ifdef ENABLE_PLAYER2_ACCOUNT2__
	snprintf(szQuery, sizeof(szQuery),"UPDATE account2.account SET `status` = 'BLOCK' WHERE account_id = %u",account_id);
#else
	snprintf(szQuery, sizeof(szQuery),"UPDATE account.account SET `status` = 'BLOCK' WHERE account_id = %u",account_id);
#endif
	DBManager::instance().DirectQuery(szQuery);

	offlineshop::CShop* pkShop = offlineshop::GetManager().GetShopByOwnerID(playerID);
	if (pkShop)
		offlineshop::GetManager().SendShopForceCloseDBPacket(playerID);

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);
	if (tch && tch->GetDesc())
		tch->GetDesc()->SetPhase(PHASE_CLOSE);

	ch->ChatPacket(CHAT_TYPE_INFO, "帐户被封锁.");
}
#endif

#ifdef ENABLE_GET_TOTAL_ONLINE_COUNT
ACMD(do_get_total_online)
{
	const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();
	DESC_MANAGER::DESC_SET::const_iterator it = c_ref_set.begin();

	unsigned long ulLoginCount = 0;
	while (it != c_ref_set.end())
	{
		LPDESC d = *(it++);
		if (d->GetCharacter())
			++ulLoginCount;
	}

	ulLoginCount = ulLoginCount + (int)P2P_MANAGER::instance().GetPIDCount();

	ch->ChatPacket(CHAT_TYPE_INFO, "目前在线总数: %u", ulLoginCount);
}
#endif

#ifdef ENABLE_EXTENDED_BATTLE_PASS
#include "battlepass_manager.h"
ACMD(do_battlepass_get_info)
{
	if (CBattlePassManager::instance().GetNormalBattlePassID() == 0)
		ch->ChatPacket(CHAT_TYPE_INFO, "目前没有正常的战斗通行证有效");
	else
	{
		std::unique_ptr<SQLMsg> pMsgRegistred(DBManager::instance().DirectQuery("SELECT COUNT(*) FROM `battlepass_playerindex` WHERE battlepass_type = 1 and battlepass_id = %d", CBattlePassManager::instance().GetNormalBattlePassID()));
		std::unique_ptr<SQLMsg> pMsgCompledet(DBManager::instance().DirectQuery("SELECT COUNT(*) FROM `battlepass_playerindex` WHERE battlepass_type = 1 and battlepass_id = %d and battlepass_completed = 1", CBattlePassManager::instance().GetNormalBattlePassID()));
		if (!pMsgRegistred->uiSQLErrno && !pMsgCompledet->uiSQLErrno)
		{
			MYSQL_ROW row_registred = mysql_fetch_row(pMsgRegistred->Get()->pSQLResult);
			MYSQL_ROW row_compledet = mysql_fetch_row(pMsgCompledet->Get()->pSQLResult);

			ch->ChatPacket(CHAT_TYPE_INFO, "---------------------------------------------------------------");
			ch->ChatPacket(CHAT_TYPE_INFO, "实际普通任务通行证ID = %d", CBattlePassManager::instance().GetNormalBattlePassID());
			ch->ChatPacket(CHAT_TYPE_INFO, "注册普通任务通行证的玩家 = %d", std::atoi(row_registred[0]));
			ch->ChatPacket(CHAT_TYPE_INFO, "结束普通任务通行证 = %d / %d", std::atoi(row_compledet[0]), std::atoi(row_registred[0]));
			ch->ChatPacket(CHAT_TYPE_INFO, "---------------------------------------------------------------");
		}
	}

	if (CBattlePassManager::instance().GetPremiumBattlePassID() == 0)
		ch->ChatPacket(CHAT_TYPE_INFO, "目前尚未激活高级 Battlepass");
	else
	{
		std::unique_ptr<SQLMsg> pMsgRegistred(DBManager::instance().DirectQuery("SELECT COUNT(*) FROM `battlepass_playerindex` WHERE battlepass_type = 2 and battlepass_id = %d", CBattlePassManager::instance().GetPremiumBattlePassID()));
		std::unique_ptr<SQLMsg> pMsgCompledet(DBManager::instance().DirectQuery("SELECT COUNT(*) FROM `battlepass_playerindex` WHERE battlepass_type = 2 and battlepass_id = %d and battlepass_completed = 1", CBattlePassManager::instance().GetPremiumBattlePassID()));
		if (!pMsgRegistred->uiSQLErrno && !pMsgCompledet->uiSQLErrno)
		{
			MYSQL_ROW row_registred = mysql_fetch_row(pMsgRegistred->Get()->pSQLResult);
			MYSQL_ROW row_compledet = mysql_fetch_row(pMsgCompledet->Get()->pSQLResult);

			ch->ChatPacket(CHAT_TYPE_INFO, "---------------------------------------------------------------");
			ch->ChatPacket(CHAT_TYPE_INFO, "实际高级战斗通行证 ID = %d", CBattlePassManager::instance().GetPremiumBattlePassID());
			ch->ChatPacket(CHAT_TYPE_INFO, "注册高级任务战斗通行证 = %d", std::atoi(row_registred[0]));
			ch->ChatPacket(CHAT_TYPE_INFO, "结束高级任务通行证 = %d / %d", std::atoi(row_compledet[0]), std::atoi(row_registred[0]));
			ch->ChatPacket(CHAT_TYPE_INFO, "---------------------------------------------------------------");
		}
	}

	if (CBattlePassManager::instance().GetEventBattlePassID() == 0)
		ch->ChatPacket(CHAT_TYPE_INFO, "目前没有活动 Battlepass");
	else
	{
		std::unique_ptr<SQLMsg> pMsgRegistred(DBManager::instance().DirectQuery("SELECT COUNT(*) FROM `battlepass_playerindex` WHERE battlepass_type = 3 and battlepass_id = %d", CBattlePassManager::instance().GetEventBattlePassID()));
		std::unique_ptr<SQLMsg> pMsgCompledet(DBManager::instance().DirectQuery("SELECT COUNT(*) FROM `battlepass_playerindex` WHERE battlepass_type = 3 and battlepass_id = %d and battlepass_completed = 1", CBattlePassManager::instance().GetEventBattlePassID()));
		if (!pMsgRegistred->uiSQLErrno && !pMsgCompledet->uiSQLErrno)
		{
			MYSQL_ROW row_registred = mysql_fetch_row(pMsgRegistred->Get()->pSQLResult);
			MYSQL_ROW row_compledet = mysql_fetch_row(pMsgCompledet->Get()->pSQLResult);

			ch->ChatPacket(CHAT_TYPE_INFO, "---------------------------------------------------------------");
			ch->ChatPacket(CHAT_TYPE_INFO, "实际活动任务通行证 ID = %d", CBattlePassManager::instance().GetEventBattlePassID());
			ch->ChatPacket(CHAT_TYPE_INFO, "注册活动任务通行证 = %d", std::atoi(row_registred[0]));
			ch->ChatPacket(CHAT_TYPE_INFO, "结束活动任务通行证 = %d / %d", std::atoi(row_compledet[0]), std::atoi(row_registred[0]));
			ch->ChatPacket(CHAT_TYPE_INFO, "---------------------------------------------------------------");
		}
	}
}

ACMD(do_battlepass_set_mission)
{
	char arg1[256], arg2[256], arg3[256], arg4[256];
	four_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2), arg3, sizeof(arg3), arg4, sizeof(arg4));

	if (!*arg1 || !*arg2 || !*arg3)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "/battlepass_set_mission <battlepass_type> <mission_index> <value> (<playername>)");
		ch->ChatPacket(CHAT_TYPE_INFO, "battlepass_type: 1 = NORMAL | 2 = PREMIUM | 3 = EVENT");
		ch->ChatPacket(CHAT_TYPE_INFO, "mission_index：任务索引，表示从顶部开始数的任务编号，从 1 开始");
		ch->ChatPacket(CHAT_TYPE_INFO, "value: 输入要覆盖的值");
		return;
	}

	int battlepass_type = 0;
	int mission_index = 0;
	int value = 0;

	str_to_number(battlepass_type, arg1);
	str_to_number(mission_index, arg2);
	str_to_number(value, arg3);

	value = MAX(0, value);

	if (battlepass_type == 1 && CBattlePassManager::instance().GetNormalBattlePassID() == 0) {
		ch->ChatPacket(CHAT_TYPE_INFO, "目前没有正常的战斗通行证有效");
		return;
	}
	if (battlepass_type == 2 && CBattlePassManager::instance().GetPremiumBattlePassID() == 0) {
		ch->ChatPacket(CHAT_TYPE_INFO, "目前尚未激活高级 Battlepass");
		return;
	}
	if (battlepass_type == 3 && CBattlePassManager::instance().GetEventBattlePassID() == 0) {
		ch->ChatPacket(CHAT_TYPE_INFO, "目前没有活动 Battlepass");
		return;
	}

	LPCHARACTER tch;
	DWORD mission_type = 0;

	if (*arg4 && ch->GetName() != arg4)
		tch = CHARACTER_MANAGER::instance().FindPC(arg4);
	else
		tch = CHARACTER_MANAGER::instance().FindPC(ch->GetName());

	if (!tch) 
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "该玩家不在线或不存在.");
		return;
	}
	if (battlepass_type == 2 && CBattlePassManager::instance().GetPremiumBattlePassID() != tch->GetExtBattlePassPremiumID()) 
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "该玩家无权访问当前的高级任务通行证.");
		return;
	}

	mission_type = CBattlePassManager::instance().GetMissionTypeByIndex(battlepass_type, mission_index);

	if (mission_type == 0) 
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "任务通行证类型 %d 中没有任务索引 %d", mission_index, battlepass_type);
		return;
	}

	tch->SetExtBattlePassMissionProgress(battlepass_type, mission_index, mission_type, value);
}

ACMD(do_battlepass_premium_activate)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	int value;
	str_to_number(value, arg2);

	if (!*arg1 || !*arg2 || value > 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "语法：/battlepass_premium_activate <playername> <activate = 1 / deactivate = 0>");
		return;
	}

	if (CBattlePassManager::instance().GetPremiumBattlePassID() == 0) {
		ch->ChatPacket(CHAT_TYPE_INFO, "目前尚未激活高级 Battlepass");
		return;
	}

	if (ch->GetName() != arg1) {
		LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);
		if (!tch)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "该玩家不在线或不存在.");
			return;
		}

		if (value == 1)
		{
			tch->PointChange(POINT_BATTLE_PASS_PREMIUM_ID, CBattlePassManager::instance().GetPremiumBattlePassID());
			CBattlePassManager::instance().BattlePassRequestOpen(tch, false);
			tch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("BATTLEPASS_NOW_IS_ACTIVATED_PREMIUM_BATTLEPASS"));
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("BATTLEPASS_CMDGM_ACTIVATE_PREMIUM_TO_PLAYER"), tch->GetName());
		}
		if (value == 0)
		{
			tch->PointChange(POINT_BATTLE_PASS_PREMIUM_ID, 0);
			tch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("BATTLEPASS_CMDGM_DEACTIVATE_PREMIUM_PLAYER"));
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("BATTLEPASS_CMDGM_DEACTIVATE_PREMIUM_TO_PLAYER"), tch->GetName());
		}
	}
	else
	{
		if (value == 1)
		{
			ch->PointChange(POINT_BATTLE_PASS_PREMIUM_ID, CBattlePassManager::instance().GetPremiumBattlePassID());
			CBattlePassManager::instance().BattlePassRequestOpen(ch, false);
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("BATTLEPASS_NOW_IS_ACTIVATED_PREMIUM_BATTLEPASS_OWN"));
		}
		if (value == 0)
		{
			ch->PointChange(POINT_BATTLE_PASS_PREMIUM_ID, 0);
			CBattlePassManager::instance().BattlePassRequestOpen(ch, false);
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("BATTLEPASS_CMDGM_DEACTIVATE_PREMIUM_OWN"));
		}
	}
}
#endif

ACMD(do_new_full)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	bool farm = false;
	str_to_number(farm, arg1);

	LPITEM item = ch->AutoGiveItem(55701, 1);
	if (item)
	{
		item->SetSocket(0, 200);
		item->SetSocket(1, 200);
		item->SetSocket(2, 200);
		item->SetSocket(4, 6);
		item->SetSocket(5, 34047);

		item->SetForceAttribute(0, 3, 120);
		item->SetForceAttribute(1, 10, 7);

		if (farm)
		{
			item->SetForceAttribute(2,5, 30);
			item->SetForceAttribute(3,7, 30);
			item->SetForceAttribute(4,9, 30);
			item->SetForceAttribute(5,10, 30);
			item->SetForceAttribute(6,11, 30);
			item->SetForceAttribute(7,12, 30);
			item->SetForceAttribute(8,13, 30);
			item->SetForceAttribute(9,14, 30);
		}
		else
		{
			item->SetForceAttribute(2, 1, 30);
			item->SetForceAttribute(3, 2, 30);
			item->SetForceAttribute(4, 3, 30);
			item->SetForceAttribute(5, 4, 30);
			item->SetForceAttribute(6, 5, 30);
			item->SetForceAttribute(7, 6, 30);
			item->SetForceAttribute(8, 9, 30);
			item->SetForceAttribute(9, 10, 30);
		}
	}
}

#ifdef ENABLE_BOT_PLAYER
#include "BotPlayer.h"
ACMD(do_BotCharacter)
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	int32_t spawn_count = 1;

	if (*arg1 && str_to_number(spawn_count, arg1) && spawn_count < 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "输入的虚拟玩家数量无效,请输入1或更大值.");
		return;
	}

	CBotCharacterManager::instance().BotSpawn(ch, spawn_count);
}


ACMD(do_BotSpawnA)  // 生成盛唐国玩家
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	int32_t spawn_count = 1;

	if (*arg1 && str_to_number(spawn_count, arg1) && spawn_count < 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "输入的生成的玩家数量需要大于 1 ");
		return;
	}

	ch->ChatPacket(CHAT_TYPE_INFO, "正在生成盛唐国虚拟玩家…");
	CBotCharacterManager::instance().BotSpawnShinsoo(ch, spawn_count);
}

ACMD(do_BotSpawnB)  //生成秦皇国玩家
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	int32_t spawn_count = 1;

	if (*arg1 && str_to_number(spawn_count, arg1) && spawn_count < 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "输入的生成的玩家数量需要大于 1 ");
		return;
	}

	ch->ChatPacket(CHAT_TYPE_INFO, "正在生成秦皇国虚拟玩家…");
	CBotCharacterManager::instance().BotSpawnChunjo(ch, spawn_count);
}

ACMD(do_BotSpawnC)  //生成汉武国玩家
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	int32_t spawn_count = 1;

	if (*arg1 && str_to_number(spawn_count, arg1) && spawn_count < 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "输入的机器人数量请大于 1 ");
		return;
	}

	ch->ChatPacket(CHAT_TYPE_INFO, "正在生成汉武国虚拟玩家…");
	CBotCharacterManager::instance().BotSpawnJinno(ch, spawn_count);
}

ACMD(do_BotCharacterDelete)
{
	if (ch)
	{
		CBotCharacterManager::instance().BotFullRemove();
	}
}

ACMD(do_BotCharacterDeleteName)
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "使用: /bot_delete_name <虚拟玩家名字> -删除指定虚拟玩家");
		return;
	}

	CBotCharacterManager::instance().BotCharacterRemove(arg1);
}

ACMD(do_BotCharacterTotal)
{
	if (ch)
	{
		const int botCount = CBotCharacterManager::instance().BotCharacterCount();
		ch->ChatPacket(CHAT_TYPE_INFO, "在线玩家: %d    (PC)", botCount);
	}
}

ACMD(do_BotReload)
{
	if (!ch)
		return;

	ch->ChatPacket(CHAT_TYPE_INFO, "正在重新加载机虚拟玩家资料...");
	CBotCharacterManager::instance().Reload();
	ch->ChatPacket(CHAT_TYPE_INFO, "机器人设置已重新加载完毕!");
}
#endif
