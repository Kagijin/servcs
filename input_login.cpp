#include "stdafx.h"
#include "constants.h"
#include "config.h"
#include "utils.h"
#include "input.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "cmd.h"
#include "buffer_manager.h"
#include "protocol.h"
#include "pvp.h"
#include "start_position.h"
#include "messenger_manager.h"
#include "guild_manager.h"
#include "party.h"
#include "dungeon.h"
#include "war_map.h"
#include "questmanager.h"
#include "building.h"
#include "wedding.h"
#include "affect.h"
#include "arena.h"
#include "OXEvent.h"
#include "priv_manager.h"
#include "dev_log.h"

#include "horsename_manager.h"
#include "MarkManager.h"
#include "../../common/Controls.h"
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
#include "MountSystem.h"
#endif
#ifdef ENABLE_OFFLINE_SHOP
#include "new_offlineshop.h"
#include "new_offlineshop_manager.h"
#endif
#ifdef ENABLE_SWITCHBOT
#include "new_switchbot.h"
#endif
#ifdef ENABLE_HWID
#include "hwid_manager.h"
#endif

#ifdef ENABLE_BUFFI_SYSTEM
#include "BuffiSystem.h"
#endif

#ifdef TOURNAMENT_PVP_SYSTEM
	#include "tournament.h"
#endif

#ifdef ENABLE_WOLFMAN_CHARACTER

// #define USE_LYCAN_CREATE_POSITION
#ifdef USE_LYCAN_CREATE_POSITION

DWORD g_lycan_create_position[4][2] =
{
	{		0,		0 },
	{ 768000 + 38300, 896000 + 35500 },
	{ 819200 + 38300, 896000 + 35500 },
	{ 870400 + 38300, 896000 + 35500 },
};

inline DWORD LYCAN_CREATE_START_X(BYTE e, BYTE job)
{
	if (1 <= e && e <= 3)
		return (job == JOB_WOLFMAN) ? g_lycan_create_position[e][0] : g_create_position[e][0];
	return 0;
}

inline DWORD LYCAN_CREATE_START_Y(BYTE e, BYTE job)
{
	if (1 <= e && e <= 3)
		return (job == JOB_WOLFMAN) ? g_lycan_create_position[e][1] : g_create_position[e][1];
	return 0;
}

#endif

#endif
static void _send_bonus_info (LPCHARACTER ch)
{
	int	item_drop_bonus = 0;
	int gold_drop_bonus = 0;
	int gold10_drop_bonus	= 0;
	int exp_bonus		= 0;

	item_drop_bonus		= CPrivManager::instance().GetPriv (ch, PRIV_ITEM_DROP);
	gold_drop_bonus		= CPrivManager::instance().GetPriv (ch, PRIV_GOLD_DROP);
	gold10_drop_bonus	= CPrivManager::instance().GetPriv (ch, PRIV_GOLD10_DROP);
	exp_bonus			= CPrivManager::instance().GetPriv (ch, PRIV_EXP_PCT);

	if (item_drop_bonus)
	{
		ch->ChatPacket (CHAT_TYPE_NOTICE, LC_TEXT ("酒捞袍 靛酚伏  %d%% 眠啊 捞亥飘 吝涝聪促."), item_drop_bonus);
	}
	if (gold_drop_bonus)
	{
		ch->ChatPacket (CHAT_TYPE_NOTICE, LC_TEXT ("榜靛 靛酚伏 %d%% 眠啊 捞亥飘 吝涝聪促."), gold_drop_bonus);
	}
	if (gold10_drop_bonus)
	{
		ch->ChatPacket (CHAT_TYPE_NOTICE, LC_TEXT ("措冠榜靛 靛酚伏 %d%% 眠啊 捞亥飘 吝涝聪促."), gold10_drop_bonus);
	}
	if (exp_bonus)
	{
		ch->ChatPacket (CHAT_TYPE_NOTICE, LC_TEXT ("版氰摹 %d%% 眠啊 裙垫 捞亥飘 吝涝聪促."), exp_bonus);
	}
}

void CInputLogin::Login(LPDESC d, const char* data)
{
	TPacketCGLogin* pinfo = (TPacketCGLogin*)data;

	char login[LOGIN_MAX_LEN + 1];
	trim_and_lower(pinfo->login, login, sizeof(login));

	TPacketGCLoginFailure failurePacket;

	if (!test_server)
	{
		failurePacket.header = HEADER_GC_LOGIN_FAILURE;
		strlcpy(failurePacket.szStatus, "VERSION", sizeof(failurePacket.szStatus));
		d->Packet(&failurePacket, sizeof(TPacketGCLoginFailure));
		return;
	}

	if (g_bNoMoreClient)
	{
		failurePacket.header = HEADER_GC_LOGIN_FAILURE;
		strlcpy(failurePacket.szStatus, "SHUTDOWN", sizeof(failurePacket.szStatus));
		d->Packet(&failurePacket, sizeof(TPacketGCLoginFailure));
		return;
	}

	if (g_iUserLimit > 0)
	{
		int iTotal;
		int* paiEmpireUserCount;
		int iLocal;

		DESC_MANAGER::instance().GetUserCount(iTotal, &paiEmpireUserCount, iLocal);

		if (g_iUserLimit <= iTotal)
		{
			failurePacket.header = HEADER_GC_LOGIN_FAILURE;
			strlcpy(failurePacket.szStatus, "FULL", sizeof(failurePacket.szStatus));
			d->Packet(&failurePacket, sizeof(TPacketGCLoginFailure));
			return;
		}
	}

	TLoginPacket login_packet;

	strlcpy(login_packet.login, login, sizeof(login_packet.login));
	strlcpy(login_packet.passwd, pinfo->passwd, sizeof(login_packet.passwd));

	db_clientdesc->DBPacket(HEADER_GD_LOGIN, d->GetHandle(), &login_packet, sizeof(TLoginPacket));
}

void CInputLogin::LoginByKey(LPDESC d, const char* data)
{
	TPacketCGLogin2* pinfo = (TPacketCGLogin2*)data;

	char login[LOGIN_MAX_LEN + 1];
	trim_and_lower(pinfo->login, login, sizeof(login));

	if (g_bNoMoreClient)
	{
		TPacketGCLoginFailure failurePacket;

		failurePacket.header = HEADER_GC_LOGIN_FAILURE;
		strlcpy(failurePacket.szStatus, "SHUTDOWN", sizeof(failurePacket.szStatus));
		d->Packet(&failurePacket, sizeof(TPacketGCLoginFailure));
		return;
	}

	if (g_iUserLimit > 0)
	{
		int iTotal;
		int* paiEmpireUserCount;
		int iLocal;

		DESC_MANAGER::instance().GetUserCount(iTotal, &paiEmpireUserCount, iLocal);

		if (g_iUserLimit <= iTotal)
		{
			TPacketGCLoginFailure failurePacket;

			failurePacket.header = HEADER_GC_LOGIN_FAILURE;
			strlcpy(failurePacket.szStatus, "FULL", sizeof(failurePacket.szStatus));

			d->Packet(&failurePacket, sizeof(TPacketGCLoginFailure));
			return;
		}
	}

	sys_log(0, "LOGIN_BY_KEY: %s key %u", login, pinfo->dwLoginKey);

	d->SetLoginKey(pinfo->dwLoginKey);
#ifndef _IMPROVED_PACKET_ENCRYPTION_
	d->SetSecurityKey(pinfo->adwClientKey);
#endif

	TPacketGDLoginByKey ptod;
	strlcpy(ptod.szLogin, login, sizeof(ptod.szLogin));
	ptod.dwLoginKey = pinfo->dwLoginKey;
	thecore_memcpy(ptod.adwClientKey, pinfo->adwClientKey, sizeof(DWORD) * 4);
	strlcpy(ptod.szIP, d->GetHostName(), sizeof(ptod.szIP));

	db_clientdesc->DBPacket(HEADER_GD_LOGIN_BY_KEY, d->GetHandle(), &ptod, sizeof(TPacketGDLoginByKey));
}

void CInputLogin::ChangeName(LPDESC d, const char* data)
{
	TPacketCGChangeName* p = (TPacketCGChangeName*)data;
	const TAccountTable& c_r = d->GetAccountTable();

	if (!c_r.id)
	{
		sys_err("no account table");
		return;
	}
	//新增代码固定
	if (p->index >= PLAYER_PER_ACCOUNT)	// FIXME overflow
	{
		sys_err("index overflow %d, login: %s", p->index, c_r.login);
		// d->SetPhase(PHASE_CLOSE);
		return;
	}
	
	if (!c_r.players[p->index].bChangeName)
		return;

	// @fixme240 BEGIN
	if (strlen(p->name) > 20 || strlen(p->name) < 3)
	{
		TPacketGCCreateFailure pack;
		pack.header = HEADER_GC_CHARACTER_CREATE_FAILURE;
		pack.bType = 1;
		d->Packet(&pack, sizeof(pack));
		return;
	}
	// @fixme240 END


	if (!check_name(p->name))
	{
		TPacketGCCreateFailure pack;
		pack.header = HEADER_GC_CHARACTER_CREATE_FAILURE;
		pack.bType = 0;
		d->Packet(&pack, sizeof(pack));
		return;
	}

	TPacketGDChangeName pdb;

	pdb.pid = c_r.players[p->index].dwID;
	strlcpy(pdb.name, p->name, sizeof(pdb.name));
	db_clientdesc->DBPacket(HEADER_GD_CHANGE_NAME, d->GetHandle(), &pdb, sizeof(TPacketGDChangeName));
}
//选择角色
void CInputLogin::CharacterSelect(LPDESC d, const char* data)
{
	struct command_player_select* pinfo = (struct command_player_select*)data;
	const TAccountTable& c_r = d->GetAccountTable();

	sys_log(0, "player_select: login: %s index: %d", c_r.login, pinfo->index);

	if (!c_r.id)
	{
		sys_err("no account table");
		return;
	}

	//@fixme239 BEGIN
	if (d->GetEmpire() < 0 || d->GetEmpire() > 3)
	{
		d->SetPhase(PHASE_CLOSE);
		sys_err("Empire ID is greater than 3.");
		return;
	}
	//@fixme239 END

	if (pinfo->index >= PLAYER_PER_ACCOUNT)
	{
		sys_err("index overflow %d, login: %s", pinfo->index, c_r.login);
		return;
	}

	if (!c_r.players[pinfo->index].dwID) // FIXME
	{
		sys_err("No player id for login %s", c_r.login);
		d->SetPhase(PHASE_CLOSE);
		return;
	}

	for (const auto& player : c_r.players)
	{
		if (player.bChangeName)
		{
			sys_err("%s: is trying to select the player again, wtf?", c_r.login); //fixme
			d->SetPhase(PHASE_CLOSE);
			return;
		}
	}

	if (c_r.players[pinfo->index].bChangeName)
	{
		sys_err("name must be changed idx %d, login %s, name %s",pinfo->index, c_r.login, c_r.players[pinfo->index].szName);
		return;
	}

	TPlayerLoadPacket player_load_packet;

	player_load_packet.account_id = c_r.id;
	player_load_packet.player_id = c_r.players[pinfo->index].dwID;
	player_load_packet.account_index = pinfo->index;

	db_clientdesc->DBPacket(HEADER_GD_PLAYER_LOAD, d->GetHandle(), &player_load_packet, sizeof(TPlayerLoadPacket));
}



bool RaceToJob(unsigned race, unsigned* ret_job)
{
	*ret_job = 0;

	if (race >= MAIN_RACE_MAX_NUM)
		return false;

	switch (race)
	{
	case MAIN_RACE_WARRIOR_M:
		*ret_job = JOB_WARRIOR;
		break;

	case MAIN_RACE_WARRIOR_W:
		*ret_job = JOB_WARRIOR;
		break;

	case MAIN_RACE_ASSASSIN_M:
		*ret_job = JOB_ASSASSIN;
		break;

	case MAIN_RACE_ASSASSIN_W:
		*ret_job = JOB_ASSASSIN;
		break;

	case MAIN_RACE_SURA_M:
		*ret_job = JOB_SURA;
		break;

	case MAIN_RACE_SURA_W:
		*ret_job = JOB_SURA;
		break;

	case MAIN_RACE_SHAMAN_M:
		*ret_job = JOB_SHAMAN;
		break;

	case MAIN_RACE_SHAMAN_W:
		*ret_job = JOB_SHAMAN;
		break;
#ifdef ENABLE_WOLFMAN_CHARACTER
	case MAIN_RACE_WOLFMAN_M:
		*ret_job = JOB_WOLFMAN;
		break;
#endif
	default:
		return false;
		break;
	}
	return true;
}

bool NewPlayerTable2(TPlayerTable* table, const char* name, BYTE race, BYTE shape, BYTE bEmpire)
{
	if (race >= MAIN_RACE_MAX_NUM)
	{
		sys_err("NewPlayerTable2.OUT_OF_RACE_RANGE(%d >= max(%d))\n", race, MAIN_RACE_MAX_NUM);
		return false;
	}

	unsigned job;

	if (!RaceToJob(race, &job))
	{
		sys_err("NewPlayerTable2.RACE_TO_JOB_ERROR(%d)\n", race);
		return false;
	}

	memset(table, 0, sizeof(TPlayerTable));

	strlcpy(table->name, name, sizeof(table->name));

	table->level		= 1;
	table->job = race;
	table->voice = 0;
	table->part_base = shape;

	table->st = JobInitialPoints[job].st;
	table->dx = JobInitialPoints[job].dx;
	table->ht = JobInitialPoints[job].ht;
	table->iq = JobInitialPoints[job].iq;

	table->hp = JobInitialPoints[job].max_hp + table->ht * JobInitialPoints[job].hp_per_ht;
	table->sp = JobInitialPoints[job].max_sp + table->iq * JobInitialPoints[job].sp_per_iq;
	table->stamina = JobInitialPoints[job].max_stamina;


	table->x = CREATE_START_X(bEmpire) + number(-300, 300);
	table->y = CREATE_START_Y(bEmpire) + number(-300, 300);

	table->z = 0;
	table->dir = 0;
	table->playtime = 0;
	table->gold = 0;
	table->skill_group = 0;
// #ifdef ENABLE_BASIC_ITEM_SYSTEM
	// memset(table->skills, 0, sizeof(table->skills));

	// table->parts[PART_MAIN] = GiveBasicArmorParts(race);
	// table->horse.bLevel = 11;
	// table->horse.bRiding = 10;
	// table->horse.sStamina = 30;
	// table->horse.sHealth = 18;

	// TPlayerSkill tempSkill{};
	// tempSkill.bLevel = 20;
	// tempSkill.bMasterType = SKILL_MASTER;

	// table->skills[SKILL_BUFFI_1] = tempSkill;
	// table->skills[SKILL_BUFFI_2] = tempSkill;
	// table->skills[SKILL_BUFFI_3] = tempSkill;
	// table->skills[SKILL_BUFFI_4] = tempSkill;
	// table->skills[SKILL_BUFFI_5] = tempSkill;

	// table->skills[SKILL_HORSE_WILDATTACK] = tempSkill;
	// table->skills[SKILL_HORSE_CHARGE] = tempSkill;
	// table->skills[SKILL_HORSE_ESCAPE] = tempSkill;
	// table->skills[SKILL_HORSE_WILDATTACK_RANGE] = tempSkill;

	// table->skills[SKILL_COMBO].bLevel = 2;
	// table->skills[SKILL_COMBO].bMasterType = SKILL_NORMAL;
// #endif
	
	return true;
}

void CInputLogin::CharacterCreate(LPDESC d, const char* data)
{
	struct command_player_create* pinfo = (struct command_player_create*)data;
	const TAccountTable& c_rAccountTable = d->GetAccountTable();


	if (!c_rAccountTable.id)
	{
		sys_err("no account table");
		d->SetPhase(PHASE_CLOSE);
		return;
	}

	TPlayerCreatePacket player_create_packet;

	sys_log(0, "PlayerCreate: name %s pos %d job %d shape %d",
		pinfo->name,
		pinfo->index,
		pinfo->job,
		pinfo->shape);

	TPacketGCLoginFailure packFailure;
	memset(&packFailure, 0, sizeof(packFailure));
	packFailure.header = HEADER_GC_CHARACTER_CREATE_FAILURE;


	if (!check_name(pinfo->name) || pinfo->shape > 1)
	{
		d->Packet(&packFailure, sizeof(packFailure));
		return;
	}
	// Fix
	if (pinfo->index >= PLAYER_PER_ACCOUNT)
	{
		sys_err("index overflow %d, login: %s", pinfo->index, c_rAccountTable.login);
		d->SetPhase(PHASE_CLOSE);
		return;
	}

	// Fix
	if (c_rAccountTable.players[pinfo->index].dwID != 0)
	{
		sys_err("login(%s) no empty character slot(%u)", c_rAccountTable.login, pinfo->index);
		d->SetPhase(PHASE_CLOSE);
		return;
	}
	
	if (strlen(pinfo->name) > 16)// @Lightwork#17
	{
		d->Packet(&packFailure, sizeof(packFailure));
		return;
	}


	if (0 == strcmp(c_rAccountTable.login, pinfo->name))
	{
		TPacketGCCreateFailure pack;
		pack.header = HEADER_GC_CHARACTER_CREATE_FAILURE;
		pack.bType = 1;

		d->Packet(&pack, sizeof(pack));
		return;
	}

	memset(&player_create_packet, 0, sizeof(TPlayerCreatePacket));

	if (!NewPlayerTable2(&player_create_packet.player_table, pinfo->name, pinfo->job, pinfo->shape, d->GetEmpire()))
	{
		sys_err("player_prototype error: job %d face %d ", pinfo->job);
		d->Packet(&packFailure, sizeof(packFailure));
		return;
	}

	trim_and_lower(c_rAccountTable.login, player_create_packet.login, sizeof(player_create_packet.login));
	strlcpy(player_create_packet.passwd, c_rAccountTable.passwd, sizeof(player_create_packet.passwd));

	player_create_packet.account_id = c_rAccountTable.id;
	player_create_packet.account_index = pinfo->index;

	sys_log(0, "PlayerCreate: name %s account_id %d, TPlayerCreatePacketSize(%d), Packet->Gold %d",
		pinfo->name,
		pinfo->index,
		sizeof(TPlayerCreatePacket),
		player_create_packet.player_table.gold);

	db_clientdesc->DBPacket(HEADER_GD_PLAYER_CREATE, d->GetHandle(), &player_create_packet, sizeof(TPlayerCreatePacket));
}
//删除角色
void CInputLogin::CharacterDelete(LPDESC d, const char* data)
{
	struct command_player_delete* pinfo = (struct command_player_delete*)data;
	const TAccountTable& c_rAccountTable = d->GetAccountTable();

	if (!c_rAccountTable.id)
	{
		sys_err("PlayerDelete: no login data");
		return;
	}

	sys_log(0, "PlayerDelete: login: %s index: %d, social_id %s", c_rAccountTable.login, pinfo->index, pinfo->private_code);

	if (pinfo->index >= PLAYER_PER_ACCOUNT)
	{
		sys_err("PlayerDelete: index overflow %d, login: %s", pinfo->index, c_rAccountTable.login);
		return;
	}

	if (!c_rAccountTable.players[pinfo->index].dwID)
	{
		sys_err("PlayerDelete: Wrong Social ID index %d, login: %s", pinfo->index, c_rAccountTable.login);
		d->Packet(encode_byte(HEADER_GC_CHARACTER_DELETE_WRONG_SOCIAL_ID), 1);
		return;
	}

	TPlayerDeletePacket	player_delete_packet;

	trim_and_lower(c_rAccountTable.login, player_delete_packet.login, sizeof(player_delete_packet.login));
	player_delete_packet.player_id = c_rAccountTable.players[pinfo->index].dwID;
	player_delete_packet.account_index = pinfo->index;
	strlcpy(player_delete_packet.private_code, pinfo->private_code, sizeof(player_delete_packet.private_code));

	db_clientdesc->DBPacket(HEADER_GD_PLAYER_DELETE, d->GetHandle(), &player_delete_packet, sizeof(TPlayerDeletePacket));
}

#pragma pack(1)
typedef struct SPacketGTLogin
{
	BYTE header;
	WORD empty;
	DWORD id;
} TPacketGTLogin;
#pragma pack()
//进入游戏核心函数
//这是最复杂的函数，处理玩家从登录阶段进入游戏内的全流程
void CInputLogin::Entergame(LPDESC d, const char* data)
{
	LPCHARACTER ch;

	if (!(ch = d->GetCharacter()))
	{
		d->SetPhase(PHASE_CLOSE);
		sys_err("Entergame is not a player.(wzy26022)");
		return;
	}

	PIXEL_POSITION pos = ch->GetXYZ();

	if (!SECTREE_MANAGER::instance().GetMovablePosition(ch->GetMapIndex(), pos.x, pos.y, pos))
	{
		PIXEL_POSITION pos2;
		SECTREE_MANAGER::instance().GetRecallPositionByEmpire(ch->GetMapIndex(), ch->GetEmpire(), pos2);

		sys_err("!GetMovablePosition (name %s %dx%d map %d changed to %dx%d)",ch->GetName(),pos.x, pos.y,ch->GetMapIndex(),pos2.x, pos2.y);
		pos = pos2;
	}
	// 1. 公会系统：角色登录公会（同步公会状态、职位、战盟信息）
	CGuildManager::instance().LoginMember(ch);
	// 2. 角色可见性：在指定坐标显示角色（让其他玩家能看到）
	ch->Show(ch->GetMapIndex(), pos.x, pos.y, pos.z);

	SECTREE_MANAGER::instance().SendNPCPosition(ch);

	// 4. 复活隐身保护时间5秒,防止刚进游戏就被攻击）
	ch->ReviveInvisible(5);

	d->SetPhase(PHASE_GAME);
	// 1. 处理角色的奖励道具（比如新手礼包、活动奖励）
	if (ch->GetItemAward_cmd())
	{
		quest::CQuestManager::instance().ItemInformer(ch->GetPlayerID(), ch->GetItemAward_vnum());
	}
	sys_log(0, "ENTERGAME: %s %dx%dx%d %s map_index %d",ch->GetName(), ch->GetX(), ch->GetY(), ch->GetZ(), d->GetHostName(), ch->GetMapIndex()); 
	// 2. 角色有坐骑等级时，自动骑乘
	if (ch->GetHorseLevel() > 0)
	{
		ch->EnterHorse();
	}
	// 重置在线时长（统计玩家在线时间）
	ch->ResetPlayTime();
	// 启动自动存档事件（定期保存角色数据，防止数据丢失）
	ch->StartSaveEvent();
	// 启动生命/法力恢复事件（每秒回血回蓝）
	ch->StartRecoveryEvent();
	// 启用攻击速度限制检测（防攻速外挂）
#ifdef ENABLE_ATTACK_SPEED_LIMIT
	ch->StartCheckSpeedHackEvent();
#endif
	// PVP系统：角色接入PVP管理器（同步PVP排名、敌对状态）
	CPVPManager::instance().Connect(ch);
	CPVPManager::instance().SendList(d);
	// 信使系统：登录信使（同步好友列表、未读消息）
	MessengerManager::instance().Login(ch->GetName());
	// 队伍系统：同步角色的队伍状态（是否在队、队友信息）
	CPartyManager::instance().SetParty(ch);
	// 公会系统：发送公会战信息（是否有正在进行的公会战）
	CGuildManager::instance().SendGuildWar(ch);
	// 建筑系统：发送角色当前地图的土地/建筑列表（比如领地、城堡）
	building::CManager::instance().SendLandList(d, ch->GetMapIndex());
	// 婚姻系统：登录婚姻系统（同步伴侣状态、婚礼buff）
	marriage::CManager::instance().Login(ch);

	TPacketGCTime p;
	p.bHeader = HEADER_GC_TIME;
	p.time = get_global_time();
	d->Packet(&p, sizeof(p));

	TPacketGCChannel p2;
	p2.header = HEADER_GC_CHANNEL;
	p2.channel = g_bChannel;
	d->Packet(&p2, sizeof(p2));
	_send_bonus_info (ch);//发送服务器爆率信息提示

#ifdef ENABLE_SWITCHBOT
	CSwitchbotManager::Instance().EnterGame(ch);
#endif

	for (int i = 0; i <= PREMIUM_MAX_NUM; ++i)
	{
		int remain = ch->GetPremiumRemainSeconds(i);

		if (remain <= 0)
			continue;

		ch->AddAffect(AFFECT_PREMIUM_START + i, POINT_NONE, 0, 0, remain, 0, true);
	}
	//GM 控制台启用
	if (ch->IsGM() == true)
		ch->ChatPacket(CHAT_TYPE_COMMAND, "ConsoleEnable");

	if (ch->GetMapIndex() >= 10000)
	{
		if (CWarMapManager::instance().IsWarMap(ch->GetMapIndex()))
			ch->SetWarMap(CWarMapManager::instance().Find(ch->GetMapIndex()));
		else if (marriage::WeddingManager::instance().IsWeddingMap(ch->GetMapIndex()))
			ch->SetWeddingMap(marriage::WeddingManager::instance().Find(ch->GetMapIndex()));
		else {
			ch->SetDungeon(CDungeonManager::instance().FindByMapIndex(ch->GetMapIndex()));
		}
	}
	else if (CArenaManager::instance().IsArenaMap(ch->GetMapIndex()) == true)
	{
		int memberFlag = CArenaManager::instance().IsMember(ch->GetMapIndex(), ch->GetPlayerID());
		// 分支1：观察者模式
		if (memberFlag == MEMBER_OBSERVER)
		{	
			ch->SetObserverMode(true); // 启用观察者模式（无法被攻击、移动限制）
			ch->SetArenaObserverMode(true); // 竞技场专属观察者标记
			if (CArenaManager::instance().RegisterObserverPtr(ch, ch->GetMapIndex(), ch->GetX() / 100, ch->GetY() / 100))
			{
				sys_log(0, "ARENA : Observer add failed");
			}
			// 强制下马（观察者不能骑乘）
			if (ch->IsHorseRiding() == true)
			{
				ch->StopRiding();
				ch->HorseSummon(false);
			}
		}
		// 分支2：决斗者模式
		else if (memberFlag == MEMBER_DUELIST)
		{
			// 发送决斗开始数据包给客户端
			TPacketGCDuelStart duelStart;
			duelStart.header = HEADER_GC_DUEL_START;
			duelStart.wSize = sizeof(TPacketGCDuelStart);

			ch->GetDesc()->Packet(&duelStart, sizeof(TPacketGCDuelStart));
			// 强制下马（决斗不能骑乘）
			if (ch->IsHorseRiding() == true)
			{
				ch->StopRiding();
				ch->HorseSummon(false);
			}
			// 解散/退出队伍（竞技场禁止组队）
			LPPARTY pParty = ch->GetParty();
			if (pParty != NULL)
			{
				if (pParty->GetMemberCount() == 2)
				{
					CPartyManager::instance().DeleteParty(pParty); // 2人队直接解散
				}
				else
				{
					pParty->Quit(ch->GetPlayerID()); // 多人队则退出
				}
			}
		}
		 // 分支3：非竞技场成员（非法进入）
		else if (memberFlag == MEMBER_NO)
		{
			// 普通玩家（非GM）强制传送回帝国出生点
			if (ch->GetGMLevel() == GM_PLAYER)
				ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
		}
	}
	//进入OX地图不能召唤坐骑
	else if (ch->GetMapIndex() == 113 )
	{
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
		// 多重判定，确保角色完全下马
		if (ch->IsHorseRiding() == true)
		{
			//停止骑乘状态
			ch->StopRiding(); 
			//取消坐骑召唤
			ch->HorseSummon(false); 
		}
			//如果已召唤坐骑编码
		if (ch->GetMountVnum()) 
		{
			//停止骑乘状态
			ch->StopRiding(); 
		}
			//最终判定骑乘状态隐藏坐骑
		if (ch->IsRiding()) 
		{
			//停止骑乘状态
			ch->StopRiding();
		}
#endif
		if (COXEventManager::instance().Enter(ch) == false)
		{
			 // OX地图进入逻辑：非GM玩家进入失败则传送回帝国出生点
			if (ch->GetGMLevel() == GM_PLAYER)
				ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
		}
	}
	else
	{	// 战争地图/婚礼地图：直接传送回帝国出生点
		if (CWarMapManager::instance().IsWarMap(ch->GetMapIndex()) || marriage::WeddingManager::instance().IsWeddingMap(ch->GetMapIndex()))
		{
			ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
		}
	}

	if (ch->GetHorseLevel() > 0)
	{
		DWORD pid = ch->GetPlayerID();
		// 如果角色有坐骑等级但未加载坐骑名称，从数据库拉取
		if (pid != 0 && CHorseNameManager::instance().GetHorseName(pid) == NULL)
			db_clientdesc->DBPacket(HEADER_GD_REQ_HORSE_NAME, 0, &pid, sizeof(DWORD));

		// @fixme182 BEGIN
		// 同步坐骑等级 + 技能等级包推送（兼容新旧加载系统）
		ch->SetHorseLevel(ch->GetHorseLevel());
#ifndef ENABLE_LOADING_RENEWAL
		ch->SkillLevelPacket();
#endif
		// @fixme182 END
	}
#ifdef TOURNAMENT_PVP_SYSTEM
	CTournamentPvP::instance().OnLogin(ch);
#endif
	if (ch->IsPC())
	{

#ifdef ENABLE_OFFLINE_SHOP
		offlineshop::CShop* pkShop = offlineshop::GetManager().GetShopByOwnerID(ch->GetPlayerID());
		if (pkShop)
		{
			ch->SetOfflineShop(pkShop);
		}
#endif
#ifdef ENABLE_NEW_PET_SYSTEM
		if (ch->GetWear(WEAR_NEW_PET))
		{
			ch->SummonNewPet();
		}
			
#endif
#ifdef ENABLE_LOADING_RENEWAL
		ch->SetLoadingState(0);
		ch->ComputePoints();
		ch->SkillLevelPacket();
#endif

#ifdef ENABLE_BIOLOG_QUEST_SYSTEM
	int biodurum = ch->GetQuestFlag("bio.durum");
	if (biodurum == 0 && ch->GetLevel() >= 30)
	{
		ch->SetQuestFlag("bio.durum", 1);
		ch->SetQuestFlag("bio.verilen", 0);
		ch->SetQuestFlag("bio.ruhtasi", 0);
		ch->SetQuestFlag("bio.kalan", 0);

		int bioverilen = ch->GetQuestFlag("bio.verilen");
		int biokalan = ch->GetQuestFlag("bio.kalan");
		int biostate = ch->GetQuestFlag("bio.ruhtasi");
		biodurum = ch->GetQuestFlag("bio.durum");
		ch->ChatPacket(CHAT_TYPE_COMMAND, "biyolog_update %d %d %d %d %d", biodurum, biostate, bioverilen, BiyologSistemi[biodurum][1], biokalan);
	}
	else
	{
		int bioverilen = ch->GetQuestFlag("bio.verilen");
		int biokalan = ch->GetQuestFlag("bio.kalan")-get_global_time();
		int biostate = ch->GetQuestFlag("bio.ruhtasi");
		biodurum = ch->GetQuestFlag("bio.durum");
		ch->ChatPacket(CHAT_TYPE_COMMAND, "biyolog_update %d %d %d %d %d", biodurum, biostate, bioverilen, BiyologSistemi[biodurum][1], biokalan);
	}
#endif
#ifndef ENABLE_NAMING_SCROLL
		// if (ch->GetMapIndex() != 113  || ch->GetMapIndex() != 26 || ch->GetMapIndex() != 112 || ch->GetMapIndex() != 195 || ch->GetMapIndex() != 202)
		// 核心判定：排除 113/26/195/202 这几个禁止坐骑的地图
		if (ch->GetMapIndex() != 113 && ch->GetMapIndex() != 26 && ch->GetMapIndex() != 195 && ch->GetMapIndex() != 202)	//进入战场不能召唤坐骑
		{
		// 宠物系统：检查并召唤宠物
#ifdef ENABLE_PET_COSTUME_SYSTEM
			ch->CheckPet();
#endif
		// 坐骑系统：非竞技场地图则恢复坐骑
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
			if (CArenaManager::instance().IsArenaMap(ch->GetMapIndex()) == false)
			{
				ch->CheckMount();
			}
#endif
#ifdef ENABLE_BUFFI_SYSTEM
			// Buffi系统：检查并召唤Buff
			int buffiTime = ch->GetQuestFlag("buffi.summon");
			if ((buffiTime > get_global_time() || buffiTime == 31) && ch->GetBuffiSystem())
			{
				ch->GetBuffiSystem()->Summon();
			}
			else if (buffiTime)
			{
				ch->SetQuestFlag("buffi.summon", 0);
			}
#endif
		}
#endif

#ifdef ENABLE_CHAR_SETTINGS
		ch->SendCharSettingsPacket();
#endif
#ifdef ENABLE_FARM_BLOCK
		CHwidManager::Instance().GetFarmBlock(ch);
#endif
#ifdef ENABLE_EVENT_MANAGER
		CHARACTER_MANAGER::Instance().SendDataPlayer(ch);
#endif

#ifdef ENABLE_LEADERSHIP_EXTENSION
		ch->CheckPartyBonus();
#endif
#ifdef ENABLE_DISTANCE_SKILL_SELECT
		if (ch->GetLevel() >= 5 && ch->GetSkillGroup() == 0)
		{
			ch->ChatPacket(CHAT_TYPE_COMMAND, "SELECT_JOB");
		}
#endif

	}
}

void CInputLogin::Empire(LPDESC d, const char* c_pData)
{
	const TPacketCGEmpire* p = reinterpret_cast<const TPacketCGEmpire*>(c_pData);

	if (EMPIRE_MAX_NUM <= p->bEmpire)
	{
		d->SetPhase(PHASE_CLOSE);
		return;
	}

	const TAccountTable& r = d->GetAccountTable();

	if (r.bEmpire != 0)
	{
		for (int i = 0; i < PLAYER_PER_ACCOUNT; ++i)
		{
			if (0 != r.players[i].dwID)
			{
				sys_err("EmpireSelectFailed %d", r.players[i].dwID);
				return;
			}
		}
	}

	TEmpireSelectPacket pd;

	pd.dwAccountID = r.id;
	pd.bEmpire = p->bEmpire;

	db_clientdesc->DBPacket(HEADER_GD_EMPIRE_SELECT, d->GetHandle(), &pd, sizeof(pd));
}

int CInputLogin::GuildSymbolUpload(LPDESC d, const char* c_pData, size_t uiBytes)
{
	if (uiBytes < sizeof(TPacketCGGuildSymbolUpload))
		return -1;

	TPacketCGGuildSymbolUpload* p = (TPacketCGGuildSymbolUpload*)c_pData;

	if (uiBytes < p->size)
		return -1;

	int iSymbolSize = p->size - sizeof(TPacketCGGuildSymbolUpload);

	if (iSymbolSize <= 0 || iSymbolSize > 64 * 1024)
	{
		d->SetPhase(PHASE_CLOSE);
		return 0;
	}

	if (!building::CManager::instance().FindLandByGuild(p->guild_id))
	{
		d->SetPhase(PHASE_CLOSE);
		return 0;
	}

	CGuildMarkManager::instance().UploadSymbol(p->guild_id, iSymbolSize, (const BYTE*)(c_pData + sizeof(*p)));
	CGuildMarkManager::instance().SaveSymbol(GUILD_SYMBOL_FILENAME);
	return iSymbolSize;
}

void CInputLogin::GuildSymbolCRC(LPDESC d, const char* c_pData)
{
	const TPacketCGSymbolCRC& CGPacket = *((TPacketCGSymbolCRC*)c_pData);
	const CGuildMarkManager::TGuildSymbol* pkGS = CGuildMarkManager::instance().GetGuildSymbol(CGPacket.guild_id);

	if (!pkGS)
		return;

	if (pkGS->raw.size() != CGPacket.size || pkGS->crc != CGPacket.crc)
	{
		TPacketGCGuildSymbolData GCPacket;

		GCPacket.header = HEADER_GC_SYMBOL_DATA;
		GCPacket.size = sizeof(GCPacket) + pkGS->raw.size();
		GCPacket.guild_id = CGPacket.guild_id;

		d->BufferedPacket(&GCPacket, sizeof(GCPacket));
		d->Packet(&pkGS->raw[0], pkGS->raw.size());
	}
}

void CInputLogin::GuildMarkUpload(LPDESC d, const char* c_pData)
{
	TPacketCGMarkUpload* p = (TPacketCGMarkUpload*)c_pData;
	CGuildManager& rkGuildMgr = CGuildManager::instance();
	CGuild* pkGuild;

	if (!(pkGuild = rkGuildMgr.FindGuild(p->gid)))
	{
		sys_err("MARK_SERVER: GuildMarkUpload: no guild. gid %u", p->gid);
		return;
	}

	if (pkGuild->GetLevel() < guild_mark_min_level)
	{
		return;
	}

	CGuildMarkManager& rkMarkMgr = CGuildMarkManager::instance();

	bool isEmpty = true;

	for (DWORD iPixel = 0; iPixel < SGuildMark::SIZE; ++iPixel)
		if (*((DWORD*)p->image + iPixel) != 0x00000000)
			isEmpty = false;

	if (isEmpty)
		rkMarkMgr.DeleteMark(p->gid);
	else
		rkMarkMgr.SaveMark(p->gid, p->image);
}

void CInputLogin::GuildMarkIDXList(LPDESC d, const char* c_pData)
{
	CGuildMarkManager& rkMarkMgr = CGuildMarkManager::instance();

	DWORD bufSize = sizeof(WORD) * 2 * rkMarkMgr.GetMarkCount();
	char* buf = NULL;

	if (bufSize > 0)
	{
		buf = (char*)malloc(bufSize);
		rkMarkMgr.CopyMarkIdx(buf);
	}

	TPacketGCMarkIDXList p;
	p.header = HEADER_GC_MARK_IDXLIST;
	p.bufSize = sizeof(p) + bufSize;
	p.count = rkMarkMgr.GetMarkCount();

	if (buf)
	{
		d->BufferedPacket(&p, sizeof(p));
		d->LargePacket(buf, bufSize);
		free(buf);
	}
	else
		d->Packet(&p, sizeof(p));
}

void CInputLogin::GuildMarkCRCList(LPDESC d, const char* c_pData)
{
	TPacketCGMarkCRCList* pCG = (TPacketCGMarkCRCList*)c_pData;

	std::map<BYTE, const SGuildMarkBlock*> mapDiffBlocks;
	CGuildMarkManager::instance().GetDiffBlocks(pCG->imgIdx, pCG->crclist, mapDiffBlocks);

	DWORD blockCount = 0;
	TEMP_BUFFER buf(1024 * 1024);

	for (itertype(mapDiffBlocks) it = mapDiffBlocks.begin(); it != mapDiffBlocks.end(); ++it)
	{
		BYTE posBlock = it->first;
		const SGuildMarkBlock& rkBlock = *it->second;

		buf.write(&posBlock, sizeof(BYTE));
		buf.write(&rkBlock.m_sizeCompBuf, sizeof(DWORD));
		buf.write(rkBlock.m_abCompBuf, rkBlock.m_sizeCompBuf);

		++blockCount;
	}

	TPacketGCMarkBlock pGC;

	pGC.header = HEADER_GC_MARK_BLOCK;
	pGC.imgIdx = pCG->imgIdx;
	pGC.bufSize = buf.size() + sizeof(TPacketGCMarkBlock);
	pGC.count = blockCount;

	if (buf.size() > 0)
	{
		d->BufferedPacket(&pGC, sizeof(TPacketGCMarkBlock));
		d->LargePacket(buf.read_peek(), buf.size());
	}
	else
		d->Packet(&pGC, sizeof(TPacketGCMarkBlock));
}
//数据包分发 Analyze
int CInputLogin::Analyze(LPDESC d, BYTE bHeader, const char* c_pData)
{
	int iExtraLen = 0;

	switch (bHeader)
	{
	case HEADER_CG_PONG:
		Pong(d);
		break;

	case HEADER_CG_TIME_SYNC:
		Handshake(d, c_pData);
		break;

	case HEADER_CG_LOGIN:
		Login(d, c_pData);
		break;

	case HEADER_CG_LOGIN2:
		LoginByKey(d, c_pData);
		break;

	case HEADER_CG_CHARACTER_SELECT:
		CharacterSelect(d, c_pData);
		break;

	case HEADER_CG_CHARACTER_CREATE:
		CharacterCreate(d, c_pData);
		break;

	case HEADER_CG_CHARACTER_DELETE:
		CharacterDelete(d, c_pData);
		break;

	case HEADER_CG_ENTERGAME:
		Entergame(d, c_pData);
		break;

	case HEADER_CG_EMPIRE:
		Empire(d, c_pData);
		break;
		///////////////////////////////////////
		// Guild Mark
		/////////////////////////////////////
	case HEADER_CG_MARK_CRCLIST:
		GuildMarkCRCList(d, c_pData);
		break;

	case HEADER_CG_MARK_IDXLIST:
		GuildMarkIDXList(d, c_pData);
		break;

	case HEADER_CG_MARK_UPLOAD:
		GuildMarkUpload(d, c_pData);
		break;

		//////////////////////////////////////
		// Guild Symbol
		/////////////////////////////////////
	case HEADER_CG_GUILD_SYMBOL_UPLOAD:
		if ((iExtraLen = GuildSymbolUpload(d, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;

	case HEADER_CG_SYMBOL_CRC:
		GuildSymbolCRC(d, c_pData);
		break;

	case HEADER_CG_MOVE:
		break;

	/* Lightwork@588 */
	case HEADER_CG_MARK_LOGIN:
		break;

	case HEADER_CG_HACK:
		break;

	case HEADER_CG_CHANGE_NAME:
		ChangeName(d, c_pData);
		break;
		// @fixme120
	case HEADER_CG_ITEM_USE:
	case HEADER_CG_TARGET:
		break;

	/* wzy26022@0928 */
	case HEADER_CG_ATTACK:
		break;

	// FIX HEADER 63 or 64 Analyze: login phase does not handle this packet! header 63
	case HEADER_CG_SCRIPT_SELECT_ITEM:
	case HEADER_CG_DRAGON_SOUL_REFINE:
		break;

	/* wzy26022@0928 */
	case HEADER_CG_ANSWER_MAKE_GUILD:
		break;

	/* wzy26022@0429 */
	// case HEADER_CG_SYNC_POSITION:
	// case HEADER_GC_SPECIFIC_EFFECT2:
		// break;

	/* 内挂自动登陆钓鱼 wzy26022@0429 */
	// case HEADER_CG_FISHING:

	/* 内挂自动聊天 wzy26022@0429 */
	// case HEADER_CG_CHAT:
		// break;

	/* wzy26022@0429 */
	// case HEADER_GC_KEY_AGREEMENT_COMPLETED:
	// case HEADER_GC_TIME_SYNC:
		// break;

	/*wzy26022@0429 */
	// case HEADER_GC_HANDSHAKE:
		// break;
	//新增拦截错误数据包

	default:
		sys_err("login phase does not handle this packet! header %d", bHeader);
		//d->SetPhase(PHASE_CLOSE);
		return (0);
	}

	return (iExtraLen);
}