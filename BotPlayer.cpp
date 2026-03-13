#include "stdafx.h"

#ifdef ENABLE_BOT_PLAYER
#include "BotPlayer.h"
#include "char.h"
#include "char_manager.h"
#include "item.h"
#include "item_manager.h"
#include "config.h"
#include "p2p.h"
#include "sectree.h"
#include "sectree_manager.h"
#include "vector.h"
#include "utils.h"
#include "desc.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "packet.h"
#include "entity.h"
#include "questmanager.h"
#include "../../common/length.h"
#include "constants.h"
// 前向声明
extern void SendShout(const char* szText, BYTE bEmpire);

#include "PetSystem.h"
#include "horsename_manager.h"
#include "horse_rider.h"
#include "motion.h"
#include "skill.h"
#include "ani.h"
#include "battle.h"
#include "locale_service.h"
#include "buffer_manager.h"
#include "locale.hpp"
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/error/en.h>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>


#define ENABLE_RANDOM_ATTRIBUTES_		// 启用装备随机5条极品属性
#define ENABLE_WANDERING_MODE_			// 启用机器人随机游荡模式
// #define ENABLE_HAIR_MODULE_			// 启用随机发型生成
// #define ENABLE_MOVEPACKE_ATTACK_		// 启用攻击动画显示
// #define ENABLE_NEW_ONLINE_PLAYER		// 启用机器人上线公告（系统全服提示）
// #define ENABLE_BOT_USE_SKILL_		// 启用机器人使用职业技能

static const std::string jobArray[] = { "猛将", "刺客", "修罗", "法师" };// 4大职业
static const std::string empireArray[] = { "盛唐国", "秦皇国", "汉武国" };// 3大对立国家

EVENTINFO(bot_character_event_info)
{
	DynamicCharacterPtr botCharacter;//机器人角色指针
	uint32_t savedMountVnum;		//坐骑编号（上马/下马缓存）
	bool wasMounted;				// 是否处于骑乘状态
	DWORD m_dwSkillNextTime;		// 技能CD冷却时间戳（防技能连发）
	long startX, startY; 			// startX/startY 是机器人/挂机锚点/机器人游荡/打怪走远后会自动返回该坐标

	bot_character_event_info() :
		botCharacter(),
		savedMountVnum(0),
		wasMounted(false),
		m_dwSkillNextTime(0),
		startX(0), startY(0)
	{
	}
};

class FuncFindVictim
{
public:
	FuncFindVictim(LPCHARACTER pkChr, int iMaxDistance) : m_pkChr(pkChr),
		m_iMinDistance(INT_MAX),
		m_iMaxDistance(iMaxDistance),
		m_lx(pkChr->GetX()),
		m_ly(pkChr->GetY()),
		m_pkChrVictim(NULL),
		m_bFoundBoss(false) {};

	void operator()(LPENTITY ent)
	{
		if (!ent->IsType(ENTITY_CHARACTER))
			return;
		LPCHARACTER pkChr = (LPCHARACTER)ent;
		if (!pkChr)
			return;

		// 跳过死亡目标
		if (pkChr->IsDead())
			return;

		int iDistance = DISTANCE_APPROX(m_lx - pkChr->GetX(), m_ly - pkChr->GetY());

		//目标优先系统 true or false
		bool isValidTarget = false;
		int targetPriority = 0; // 0=低，1=中，2=高，3=非常高

		
		// 自动攻击陨石(最高优先级)
#ifdef ENABLE_STANDING_MOUNT
		// if (pkChr->IsStone() || (pkChr->IsMonster() && pkChr->GetMobRank() >= MOB_RANK_BOSS))
		if (pkChr->IsStone() || pkChr->IsMonster())
		// if (pkChr->IsMonster() && (pkChr->GetMobRank() >= MOB_RANK_BOSS))
		{
			isValidTarget = true;
			targetPriority = 3;
		}
#else
		if (pkChr->IsMonster() && (pkChr->GetMobRank() >= MOB_RANK_BOSS))
		// if (pkChr->IsMonster() && (pkChr->GetMobRank() >= MOB_RANK_BOSS))
		{
			isValidTarget = true;
			targetPriority = 3;
		}
#endif
		// 2. 自动攻击敌对国家的玩家(高优先级)
		else if (pkChr->IsPC() && !pkChr->IsBotCharacter() && pkChr->GetEmpire() != m_pkChr->GetEmpire())
		{
			isValidTarget = false; // 原为false → 改为true
			targetPriority = 2;
		}
		// 3. 自动攻击敌方国家机器人(中优先级)
		else if (pkChr->IsPC() && pkChr->IsBotCharacter() && pkChr->GetEmpire() != m_pkChr->GetEmpire())
		{
			isValidTarget = false; // 原为false → 改为true
			targetPriority = 1;
		}
		// 4. 普通怪物(低优先级)
		else if (pkChr->IsMonster())
		{
			isValidTarget = true;
			targetPriority = 0;
		}

		if (!isValidTarget)
			return;

		// 基于优先级的目标选择
		if (iDistance <= m_iMaxDistance)
		{
			// 已发现优先级更高的目标
			if (targetPriority > m_iCurrentPriority)
			{
				m_pkChrVictim = pkChr;
				m_iMinDistance = iDistance;
				m_iCurrentPriority = targetPriority;
				m_bFoundBoss = (targetPriority >= 3);
			}
			// 这是一个同等重要但未来更易实现的目标
			else if (targetPriority == m_iCurrentPriority && iDistance < m_iMinDistance)
		{
			m_pkChrVictim = pkChr;
			m_iMinDistance = iDistance;
				if (targetPriority >= 3)
				m_bFoundBoss = true;
			}
		}
	}
	LPCHARACTER GetVictim() { return (m_pkChrVictim); }
private:
	LPCHARACTER m_pkChr;
	int m_iMinDistance;
	int m_iMaxDistance;
	long m_lx;
	long m_ly;
	LPCHARACTER m_pkChrVictim;
	bool m_bFoundBoss;
	int m_iCurrentPriority = -1; // 目标优先级
};

// 职业的技能定义(每个角色有2个职业选项)
static const std::vector<DWORD> GetClassSkills(BYTE job, BYTE skillGroup)
{
	// 猛将- 剑猛:1, 气猛:2
	if (job == JOB_WARRIOR)
	{
		if (skillGroup == 1)
		{
			//Group 1
			//技能 1，2，3，4，5
			return {SKILL_SAMYEON, SKILL_PALBANG, SKILL_JEONGWI, SKILL_GEOMKYUNG, SKILL_TANHWAN};
			// return {SKILL_GEOMKYUNG, SKILL_JEONGWI, SKILL_PALBANG, SKILL_SAMYEON, SKILL_TANHWAN};
		}
		else // Group 2
		{
			//技能 16，17，18，19，20
			return {SKILL_GIGONGCHAM, SKILL_GYOKSAN, SKILL_DAEJINGAK, SKILL_CHUNKEON, SKILL_GEOMPUNG};
		}
	}
	// 刺客- 双刀：1，弓箭：2
	else if (job == JOB_ASSASSIN)
	{
		if (skillGroup == 1)
		{
			//技能 31，32，33，34，35
			return {SKILL_AMSEOP, SKILL_GUNGSIN, SKILL_CHARYUN, SKILL_EUNHYUNG, SKILL_SANGONG};
		}
		else
		{
			//技能 46，47，48，49，50
			return {SKILL_YEONSA, SKILL_KWANKYEOK, SKILL_HWAJO, SKILL_GYEONGGONG, SKILL_GIGUNG};
		}
	}
	// 修罗 - 黑魔:1, 幻舞:2
	else if (job == JOB_SURA)
	{
		if (skillGroup == 1)
		{
			//技能 61，62，63，64，65，66
			return {SKILL_SWAERYUNG, SKILL_YONGKWON, SKILL_GWIGEOM, SKILL_TERROR, SKILL_JUMAGAP, SKILL_PABEOB};
		}
		else
		{
			//技能 76，77，78，79，80，81
			return {SKILL_MARYUNG, SKILL_HWAYEOMPOK, SKILL_MUYEONG, SKILL_MANASHILED, SKILL_TUSOK, SKILL_MAHWAN};
		}
	}
	// 法师 - 潜龙：1，狂雷：2
	else if (job == JOB_SHAMAN)
	{
		if (skillGroup == 1)
		{
			//技能 91，92，93，94，95，96
			return {SKILL_BIPABU, SKILL_YONGBI, SKILL_PAERYONG, SKILL_HOSIN, SKILL_REFLECT, SKILL_GICHEON};
		}
		else
		{
			//技能 106，107，108，109，110，111
			return {SKILL_NOEJEON, SKILL_BYEURAK, SKILL_CHAIN, SKILL_JEONGEOP, SKILL_KWAESOK, SKILL_JEUNGRYEOK};
		}
	}

	return {};
}

struct FuncFindMultiVictim
{
	LPCHARACTER m_me;
	std::vector<LPCHARACTER> m_targets;
	int m_iRadius;

	FuncFindMultiVictim(LPCHARACTER ch, int radius) : m_me(ch), m_iRadius(radius) {}

	void operator()(LPENTITY ent)
	{
		if (!ent->IsType(ENTITY_CHARACTER))
			return;

		LPCHARACTER pkChr = (LPCHARACTER)ent;

		if (!pkChr || pkChr == m_me || pkChr->IsDead())
			return;

		// 只攻击怪物和陨石
#ifdef ENABLE_STANDING_MOUNT
		if (!pkChr->IsMonster() && !pkChr->IsStone()) return;
#else
		if (!pkChr->IsMonster())
			return;
#endif
		int iDist = DISTANCE_APPROX(m_me->GetX() - pkChr->GetX(), m_me->GetY() - pkChr->GetY());
		if (iDist <= m_iRadius)
		{
			m_targets.push_back(pkChr);
		}
	}
};

// 辅助函数，用于在目标附近找到一个有效位置
static bool GetApproachPosition(LPCHARACTER bot, LPCHARACTER victim, long& outX, long& outY)
{
	if (!bot || !victim)
		return false;

	long vicX = victim->GetX();
	long vicY = victim->GetY();
	int mapIndex = victim->GetMapIndex();

	// 1. 尝试确定目标的精确位置
	if (SECTREE_MANAGER::instance().IsMovablePosition(mapIndex, vicX, vicY))
	{
		outX = vicX;
		outY = vicY;
		return true;
	}

	// 2. 如果目标位置无效（例如，位于墙壁内部），则在视线范围内搜索最近的有效位置
	long botX = bot->GetX();
	long botY = bot->GetY();

	float dx = (float)(vicX - botX);
	float dy = (float)(vicY - botY);
	float dist = sqrt(dx * dx + dy * dy);

	// if (dist <= 1.0f) return false;
	//【修复】致命逻辑：阈值从1.0改为50，避免贴脸目标时卡死不动
	if (dist <= 1.0f) return false;

	float dirX = dx / dist;
	float dirY = dy / dist;

	// 程序会向机器人方向扫描目标区域，以找到第一个有效的地面位置。
	// 起始位置略微偏离目标（50个单位）
	for (float k = 50.0f; k < dist; k += 50.0f)
	{
		long checkX = vicX - (long)(dirX * k);
		long checkY = vicY - (long)(dirY * k);

		if (SECTREE_MANAGER::instance().IsMovablePosition(mapIndex, checkX, checkY))
		{
			outX = checkX;
			outY = checkY;
			return true;
		}
	}

	return false;
}

EVENTFUNC(bot_character_event)
{
	auto info = dynamic_cast<bot_character_event_info*>(event->info);
	if (!info)
		return 0;

	LPCHARACTER botCharacter = info->botCharacter;
	if (!botCharacter)
		return 0;

	// 新增：机器人死亡自动复活（核心功能）
	if (botCharacter->IsDead())
	{
		botCharacter->IsRevive(); // 复活
		botCharacter->SetHP(botCharacter->GetMaxHP()); // 满血
		botCharacter->SetSP(botCharacter->GetMaxSP()); // 满蓝
		botCharacter->PointChange(POINT_STAMINA, 30000); // 满耐力
		sys_log(0, "机器人 %s 已死亡，自动复活并满血满蓝！", botCharacter->GetName());
		return PASSES_PER_SEC(5.0f); // 复活后停顿3秒，再继续战斗
	}
	// 新增：机器人死亡自动复活（核心功能）

	// 使用坐骑时，每次都要佩戴好武器
#ifdef ENABLE_STANDING_MOUNT
	if (botCharacter->GetMountVnum() && IS_STANDING_MOUNT_VNUM(botCharacter->GetMountVnum()))
	{
		LPITEM __w = botCharacter->GetWear(WEAR_WEAPON);
		if (__w)
		{
			WORD __partWeapon = botCharacter->GetPart(PART_WEAPON);
			if (__partWeapon != __w->GetVnum())
				botCharacter->SetPart(PART_WEAPON, __w->GetVnum());
		}
		else
		{
			int savedWeaponVnum = CBotCharacterManager::instance().GetBotWeaponVnum(botCharacter->GetName());
			if (savedWeaponVnum > 0 && botCharacter->GetPart(PART_WEAPON) != savedWeaponVnum)
				botCharacter->SetPart(PART_WEAPON, savedWeaponVnum);
		}
	}
#endif

	auto& botManager = CBotCharacterManager::instance();

	LPDESC d = DESC_MANAGER::instance().FindByCharacterName(botCharacter->GetName());
	LPCHARACTER tch = d ? d->GetCharacter() : nullptr;
	if (tch)
	{
		botManager.BotCharacterRemove(tch->GetName());
		sys_err("Real player entered bot %s has been deleted.", tch->GetName());
		return 0;
	}

	FuncFindVictim f(botCharacter, 3000);//发现目标 引怪距离设置 1位置 (Increased to 3000 to detect archers)
	if (botCharacter->GetSectree())
	{
		botCharacter->GetSectree()->ForEachAround(f);
	}

	auto victim = f.GetVictim();
	if (victim)
	{	//【优化】性能提升：缓存距离值，避免重复计算
		// int distance = DISTANCE_APPROX(botCharacter->GetX() - victim->GetX(), botCharacter->GetY() - victim->GetY());
		// const int distance = DISTANCE_APPROX(botCharacter->GetX() - victim->GetX(), botCharacter->GetY() - victim->GetY());
		// 修改后缓存变量
		const long botX = botCharacter->GetX();
		const long botY = botCharacter->GetY();
		const long vicX = victim->GetX();
		const long vicY = victim->GetY();
		const int distance = DISTANCE_APPROX(botX - vicX, botY - vicY);
		//机器人攻击范围
		int attackRange = 170;
		BYTE job = botCharacter->GetJob();
		if (job == JOB_ASSASSIN && botCharacter->GetWear(WEAR_WEAPON))
		{
			LPITEM weapon = botCharacter->GetWear(WEAR_WEAPON);
			if (weapon && weapon->GetSubType() == WEAPON_BOW)
				attackRange = 800; // 弓箭手的射程
		}

#ifdef ENABLE_STANDING_MOUNT
		// 只有在攻击首领、陨石时才可以使用坐骑
		// bool shouldMount = victim->IsStone() || (victim->IsMonster() && victim->GetMobRank() >= MOB_RANK_BOSS);
		bool shouldMount = victim->IsStone() || victim->IsMonster();
#endif

#ifdef ENABLE_STANDING_MOUNT
		// ENABLE_STANDING_MOUNT:所有坐骑均可使用(PVP不适用)
		if (shouldMount && !botCharacter->IsRiding())
		{
			//【修复】坐骑状态保存逻辑：先标记为已骑乘，再保存编号
			info->wasMounted = true;
			info->savedMountVnum = botCharacter->GetMountVnum();

			// 保存当前武器的VNUM值以便在需要时重置
			if (LPITEM __w = botCharacter->GetWear(WEAR_WEAPON))
				CBotCharacterManager::instance().SetBotWeaponVnum(botCharacter->GetName(), __w->GetVnum());
			uint32_t standingMountVnums[] = {20104, 20105};//随机坐骑
			uint32_t selectedMountVnum = standingMountVnums[number(0, 1)];
			botCharacter->HorseSummon(true, true, selectedMountVnum);
			botCharacter->StartRiding();
			{
				int savedWeaponVnum = CBotCharacterManager::instance().GetBotWeaponVnum(botCharacter->GetName());
				WORD partWeapon = botCharacter->GetPart(PART_WEAPON);
				LPITEM wearWeapon = botCharacter->GetWear(WEAR_WEAPON);
				DWORD ensureVnum = wearWeapon ? wearWeapon->GetVnum() : (savedWeaponVnum > 0 ? (DWORD)savedWeaponVnum : 0);
				if (ensureVnum > 0 && partWeapon != ensureVnum)
					botCharacter->SetPart(PART_WEAPON, (WORD)ensureVnum);
			}

			sys_log(0, "机器人 %s: 发现BOSS和陨石开始召唤坐骑 (VNUM: %d)", botCharacter->GetName(), selectedMountVnum);
		}
		else if (!shouldMount && botCharacter->IsRiding())
		{
			// 目标已死亡下马步行
			botCharacter->StopRiding();//下马步行
			botCharacter->HorseSummon(false);//下马后隐藏马匹
			if (info->wasMounted && info->savedMountVnum > 0)
			{
				botCharacter->HorseSummon(true, true, info->savedMountVnum);
				botCharacter->StartRiding();
				sys_log(0, "机器人 %s: 重新召唤使用坐骑 (VNUM: %d)", botCharacter->GetName(), info->savedMountVnum);
			}
			else
			{
				sys_log(0, "机器人 %s: 下马(步行)", botCharacter->GetName());
			}

			// 重置挂载状态
			info->wasMounted = false;
			info->savedMountVnum = 0;
		}
#endif

		// 移动以更接近目标
		if (distance > attackRange)
		{
			// 向目标移动
			long moveX, moveY;
			// 使用新功能找到通往弓箭手/怪物的有效路径
			if (GetApproachPosition(botCharacter, victim, moveX, moveY))
			{
				float dx = moveX - botCharacter->GetX();
				float dy = moveY - botCharacter->GetY();
				botCharacter->SetRotation(GetDegreeFromPosition(dx, dy));

				if (botCharacter->Goto(moveX, moveY))
				{
					// goto() 函数已经发送了移动数据包
					botCharacter->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
				}

				return PASSES_PER_SEC(0.1f); // 接近目标时保持冷静的等待
			}
		}

		// 接近目标开始攻击
		if (distance <= attackRange)
		{
			// 如果目标位在无效位置,尝试将弓箭手引开(诱导)
			if (!SECTREE_MANAGER::instance().IsMovablePosition(victim->GetMapIndex(), victim->GetX(), victim->GetY()))
			{
				float fx, fy;
				GetDeltaByDegree(number(0, 359), 300, &fx, &fy); // 向随机方向移动 500 个单位
				long destX = botCharacter->GetX() + (long)fx;
				long destY = botCharacter->GetY() + (long)fy;
				if (botCharacter->Goto(destX, destY))
					botCharacter->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
				return PASSES_PER_SEC(0.10f);//接近目标开始攻击
			}

			// 转向目标
			float dx = victim->GetX() - botCharacter->GetX();
			float dy = victim->GetY() - botCharacter->GetY();
			botCharacter->SetRotation(GetDegreeFromPosition(dx, dy));
#ifdef ENABLE_BOT_USE_SKILL_
	//使用机器人逻辑进行技能使用和计时器控制。
	const DWORD now = get_dword_time();
	bool canUseSkill = (info->m_dwSkillNextTime <= now);

	// sys_log(0, "Bot %s: [SKILL CHECK] canUseSkill=%d, m_dwSkillNextTime=%u, now=%u, victim=%s", botCharacter->GetName(), canUseSkill ? 1 : 0, info->m_dwSkillNextTime, now, victim->GetName());

	if (canUseSkill)
	{
		// 更新计时器(每次使用技能后等待3秒，技能冷却)
		info->m_dwSkillNextTime = now + 3000;

		//根据机器人的类别获取技能列表。
		BYTE skillGroup = number(1, 2);
		auto skills = GetClassSkills(job, skillGroup);

		// sys_log(0, "Bot %s: [SKILL CHECK] Job=%d, SkillGroup=%d, Skills count=%zu", botCharacter->GetName(), job, skillGroup, skills.size());

		// SP 控制 - 补全修复：SP低于40%就自动回满，彻底解决技能释放缺蓝问题
		int currentSP = botCharacter->GetSP();
		int maxSP = botCharacter->GetMaxSP();
		int spPercent = (maxSP > 0) ? (currentSP * 100 / maxSP) : 0;

		if (spPercent < 40)
		{
			botCharacter->SetSP(maxSP);
			botCharacter->PointChange(POINT_SP, maxSP - currentSP);
			// sys_log(0, "Bot %s: [skill SP] SP不足(%d%%), 自动补满至 %d", botCharacter->GetName(), spPercent, maxSP);
		}

		if (skills.empty())
		{
			sys_log(0, "Bot %s: [Skill skip] Skill list is empty!", botCharacter->GetName());
		}
		else
		{
			// 遍历技能列表，使用第一个可用的技能
			for (DWORD skillVnum : skills)
			{
				BYTE skillLevel = botCharacter->GetSkillLevel(skillVnum);

				// sys_log(0, "Bot %s: [Skill iteration] Skill %d, Level %d", botCharacter->GetName(), skillVnum, skillLevel);

				// 技能等级必须大于0才能使用
				if (skillLevel == 0)
				{
					// sys_log(0, "Bot %s: [Skill skip] skillVnum %d 等级为0，跳过", botCharacter->GetName(), skillVnum);
					continue;
				}

				// 使用技能 - 调用战斗核心函数
				// sys_log(0, "Bot %s: [SKILL USE] 使用技能! 目标:%s, 技能ID:%d, 等级:%d, SP:%d/%d", botCharacter->GetName(), victim->GetName(), skillVnum, skillLevel, botCharacter->GetSP(), botCharacter->GetMaxSP());

				int result = botCharacter->ComputeSkill(skillVnum, victim, skillLevel);

				// sys_log(0, "Bot %s: [技能结果] ComputeSkill 返回值: %d", botCharacter->GetName(), result);

				if (result != BATTLE_NONE)
				{
					// 技能释放成功，打印日志并退出循环
					bool isPvPTarget = victim->IsPC();
					bool isBoss = (victim->IsMonster() && victim->GetMobRank() >= MOB_RANK_BOSS);
					// sys_log(0, "Bot %s (%s) 使用技能攻击目标 %s: %s (技能ID: %d, 等级: %d)",
						// botCharacter->GetName(),
						// empireArray[botCharacter->GetEmpire() - 1].c_str(),
						// isPvPTarget ? "敌对玩家" : (isBoss ? "首领怪物" : "普通怪物"),
						// victim->GetName(),
						// skillVnum, skillLevel);

					return PASSES_PER_SEC(1.0f);
				}
				else
				{
					sys_log(0, "Bot %s: [技能失败] 技能 %d 无法使用，尝试下一个",botCharacter->GetName(), skillVnum);
				}
			}
		}
	}
#endif
			// 普通攻击 - 专业连击系统
			WORD currentWeapon = botCharacter->GetPart(PART_WEAPON);
			if (currentWeapon == 0)
			{
				int savedWeaponVnum = CBotCharacterManager::instance().GetBotWeaponVnum(botCharacter->GetName());
				if (savedWeaponVnum > 0)
				{
					botCharacter->SetPart(PART_WEAPON, savedWeaponVnum);
					// sys_log(0, "机器人 %s：武器丢失了,已根据任务标记重置(武器：%d)", botCharacter->GetName(), savedWeaponVnum);
				}
				else
				{
					// 如果没有任务标记，请根据职业选择武器
					BYTE job = botCharacter->GetJob();
					uint32_t weaponVnum = 0;

					//【修复】武器随机假随机问题：为每个职业配置真实的随机武器列表
					if (job == JOB_WARRIOR) weaponVnum = number(0,1) ? 16 : 17;
					else if (job == JOB_ASSASSIN) weaponVnum = number(0,1) ? 1006 : 1007;
					else if (job == JOB_SURA) weaponVnum = number(0,1) ? 16 : 18;
					else if (job == JOB_SHAMAN) weaponVnum = number(0,1) ? 7006 : 7007;

					if (weaponVnum > 0)
					{
						botCharacter->SetPart(PART_WEAPON, weaponVnum);
						CBotCharacterManager::instance().SetBotWeaponVnum(botCharacter->GetName(), weaponVnum);
						// sys_log(0, "Bot %s: Weapon lost, reset according to job (Weapon: %d, Job: %d)",
							// botCharacter->GetName(), weaponVnum, job);
					}
				}
			}

#ifdef ENABLE_STANDING_MOUNT
			if (botCharacter->GetMountVnum() && IS_STANDING_MOUNT_VNUM(botCharacter->GetMountVnum()))
			{
				LPITEM __w = botCharacter->GetWear(WEAR_WEAPON);
				if (__w)
				{
					WORD __partWeapon = botCharacter->GetPart(PART_WEAPON);
					if (__partWeapon != __w->GetVnum())
					{
						botCharacter->SetPart(PART_WEAPON, __w->GetVnum());
						// sys_log(0, "Bot %s: In the standing mount, the PART_WEAPON was corrected before the attack. (%u)", botCharacter->GetName(), __w->GetVnum());
					}
				}
				else
				{
					int savedWeaponVnum = CBotCharacterManager::instance().GetBotWeaponVnum(botCharacter->GetName());
					if (savedWeaponVnum > 0 && botCharacter->GetPart(PART_WEAPON) != savedWeaponVnum)
					{
						botCharacter->SetPart(PART_WEAPON, savedWeaponVnum);
						// sys_log(0, "Bot %s: In the standing mount, the attack was restored from the PART_WEAPON quest flag before the attack. (%d)",
							// botCharacter->GetName(), savedWeaponVnum);
					}
				}
			}
#endif

			// Attack()函数发送数据包
			//【修复】扁平化嵌套逻辑：移除重复的distance判断，解决普攻无法执行问题
			FuncFindMultiVictim aggroCollector(botCharacter, 3000);
			if (botCharacter->GetSectree()) botCharacter->GetSectree()->ForEachAround(aggroCollector);

			if (!aggroCollector.m_targets.empty())
			{
				std::shuffle(aggroCollector.m_targets.begin(), aggroCollector.m_targets.end(), std::mt19937(std::random_device()()));
				for (auto target : aggroCollector.m_targets)
				{
#ifdef ENABLE_OFFICIAL_STUFF
					
					if (botCharacter->GetCapeEffectPulse() + 50 > thecore_pulse()) break;
#endif
					if (target && !target->IsDead() && target->IsMonster())
					{
#ifdef ENABLE_OFFICIAL_STUFF
						botCharacter->SetCapeEffectPulse(thecore_pulse());
#endif
						botCharacter->AggregateMonster();//引怪设置
						botCharacter->GetMaxStamina();
						target->SetVictim(botCharacter);
					}
				}
			}
			// 1. 寻找附近所有目标(开始攻击)
			// 范围攻击周边怪物的半径为300个单位(根据需要进行调整，以类似于内挂）
			FuncFindMultiVictim splashCollector(botCharacter, 300);//攻击目标范围
			if (botCharacter->GetSectree()) botCharacter->GetSectree()->ForEachAround(splashCollector);

			bool attackedAny = false;
			for (auto target : splashCollector.m_targets)
			{
				if (battle_is_attackable(botCharacter, target))
				{
					int iRet = battle_melee_attack(botCharacter, target);
					if (iRet != BATTLE_NONE)
					{
						attackedAny = true;
						if (number(1, 100) <= 8) target->CreateFly(FLY_EXP, botCharacter);//经验值飞溅效果
						if (number(1, 100) <= 5) target->EffectPacket(SE_CRITICAL);//双倍爆炸效果

						if (number(1, 100) <= 2) botCharacter->EffectPacket(SE_AUTO_HPUP);//自动药水效果
						// if (number(1, 100) <= 5) target->CreateFly(FLY_HP_SMALL, botCharacter);//伤害转生命值
						// if (number(1, 100) <= 5) target->CreateFly(FLY_SP_SMALL, botCharacter);//伤害转精力值
					}
				}
			}

			if (attackedAny)
			{
#ifdef ENABLE_MOVEPACKE_ATTACK_
				botCharacter->SendMovePacket(FUNC_COMBO, MOTION_COMBO_ATTACK_1, botCharacter->GetX(), botCharacter->GetY(), 0, get_dword_time());
#endif
				return PASSES_PER_SEC(0.1f);
			}

			{
#ifdef ENABLE_STANDING_MOUNT
				if (botCharacter->GetMountVnum() && IS_STANDING_MOUNT_VNUM(botCharacter->GetMountVnum()))
				{
					int savedWeaponVnum = CBotCharacterManager::instance().GetBotWeaponVnum(botCharacter->GetName());
					LPITEM wearWeapon = botCharacter->GetWear(WEAR_WEAPON);
					WORD want = wearWeapon ? (WORD)wearWeapon->GetVnum() : (savedWeaponVnum > 0 ? (WORD)savedWeaponVnum : 0);
					if (want > 0 && botCharacter->GetPart(PART_WEAPON) != want)
						botCharacter->SetPart(PART_WEAPON, want);
				}
#endif
				// bool isPvPTarget = victim->IsPC();
				// if (isPvPTarget && number(1, 10) == 1)
				// {
					// sys_log(0, "Bot %s (%s) The enemy is attacking player %s (%s)! (HP: %d/%d)",
						// botCharacter->GetName(), empireArray[botCharacter->GetEmpire() - 1].c_str(),
						// victim->GetName(), empireArray[victim->GetEmpire() - 1].c_str(),
						// victim->GetHP(), victim->GetMaxHP());
				// }

				int currentCombo = botCharacter->GetComboSequence();
				int nextCombo = (currentCombo % 5) + 1;
				botCharacter->SetComboSequence(nextCombo);

#ifdef ENABLE_MOVEPACKE_ATTACK_
				int comboIndex = (nextCombo - 1) % 5;
#endif

				DWORD dwTime = get_dword_time();
				int interval = dwTime - botCharacter->GetLastComboTime();
				botCharacter->SetValidComboInterval(interval);
				botCharacter->SetLastComboTime(dwTime);
#ifdef ENABLE_MOVEPACKE_ATTACK_
				botCharacter->SendMovePacket(FUNC_COMBO, MOTION_COMBO_ATTACK_1 + comboIndex, botCharacter->GetX(), botCharacter->GetY(), 0, dwTime);
#endif
				return PASSES_PER_SEC(0.1f);
			}
		}
	}
	else
	{
		//【修复】函数名错误：IsHorseRiding()改为IsRiding()
		if (botCharacter->IsRiding() && !victim && std::rand() % 5 == 0)
			botCharacter->StopRiding();
		botCharacter->HorseSummon(false);

		long distToHome = DISTANCE_APPROX(botCharacter->GetX() - info->startX, botCharacter->GetY() - info->startY);
		if (distToHome > 3000)
		{
			if (botCharacter->Goto(info->startX, info->startY))
			{
				botCharacter->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
			}
		}
#ifdef ENABLE_WANDERING_MODE_
		else if (number(0, 2) == 1)
		{
			int iDist[4] = { 200, 300, 500, 1000 };
			for (int iDistIdx = 2; iDistIdx >= 0; --iDistIdx)
			{
				for (int iTryCount = 0; iTryCount < 8; ++iTryCount)
				{
					botCharacter->SetRotation(number(0, 359));
					float fx, fy;
					float fDist = number(iDist[iDistIdx], iDist[iDistIdx + 1]);
					GetDeltaByDegree(botCharacter->GetRotation(), fDist, &fx, &fy);

					bool isBlock = false;
					for (int j = 1; j <= 100; ++j)
					{
						if (!SECTREE_MANAGER::instance().IsMovablePosition(botCharacter->GetMapIndex(), botCharacter->GetX() + (int)fx * j / 100, botCharacter->GetY() + (int)fy * j / 100))
						{
							isBlock = true;
							break;
						}
					}

					if (isBlock) continue;
					int iDestX = botCharacter->GetX() + (int)fx;
					int iDestY = botCharacter->GetY() + (int)fy;
					if (botCharacter->Goto(iDestX, iDestY))
						botCharacter->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
				}
			}
		}
#endif
	}

	return PASSES_PER_SEC(victim ? (float)0.1 : (int)5);
}

void BotCharacter::SetStartEvent(LPCHARACTER botCharacter, long startX, long startY)
{
	if (!botCharacter)
		return;

	auto info = AllocEventInfo<bot_character_event_info>();
	info->botCharacter = botCharacter;
	info->startX = startX;
	info->startY = startY;

	// 启动机器人事件（以获得平衡的回应）
	m_botCharacterEvent = event_create(bot_character_event, info, PASSES_PER_SEC(0.15f));
}

EVENTFUNC(bot_character_chat_event)
{
	auto info = dynamic_cast<bot_character_event_info*>(event->info);
	if (!info)
		return 0;

	LPCHARACTER botCharacter = info->botCharacter;
	if (!botCharacter)
		return 0;

	auto& botManager = CBotCharacterManager::instance();

	LPDESC d = DESC_MANAGER::instance().FindByCharacterName(botCharacter->GetName());
	LPCHARACTER tch = d ? d->GetCharacter() : nullptr;
	if (tch)
	{
		botManager.BotCharacterRemove(tch->GetName());
		sys_err("Real player entered bot %s has been deleted.", tch->GetName());
		return 0;
	}

	//选择一条随机聊天消息
	const std::vector<std::string>& chatMessages = botManager.GetChatMessages();
	const char* message = "你好!";
	if (!chatMessages.empty())
	{
		int randomIndex = number(0, chatMessages.size() - 1);
		message = chatMessages[randomIndex].c_str();
	}

	char cleanMessage[256];
	strlcpy(cleanMessage, message, sizeof(cleanMessage));
	char shoutMessage[CHAT_MAX_LEN + 1];
	std::string strEmpireFlagToken;
	switch (botCharacter->GetEmpire())
	{
		case 1: strEmpireFlagToken = "|cFFff0000|H|h[红]|h|r"; break;
		case 2: strEmpireFlagToken = "|cFFffff00|H|h[黄]|h|r"; break;
		case 3: strEmpireFlagToken = "|cFF0080ff|H|h[蓝]|h|r"; break;
	}

	snprintf(shoutMessage, sizeof(shoutMessage), "%s |Hpm:%s|h%s|h|r : %s",
			 strEmpireFlagToken.c_str(),
			 botCharacter->GetName(),
			 botCharacter->GetName(),
			 cleanMessage);

	// 发送呐喊数据包
	TPacketGGShout p;
	p.bHeader = HEADER_GG_SHOUT;
	p.bEmpire = botCharacter->GetEmpire();
	strlcpy(p.szText, shoutMessage, sizeof(p.szText));
	P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGShout));
	SendShout(shoutMessage, botCharacter->GetEmpire());

	sys_log(0, "Bot %s Shouting: %s", botCharacter->GetName(), cleanMessage);

	//随机呐喊时间 间隔 1.3-8 分钟（90, 480 秒）
	int randomInterval = number(90, 300);
	return PASSES_PER_SEC(randomInterval);
}

void BotCharacter::SetStartChatEvent(LPCHARACTER botCharacter)
{
	if (!botCharacter)
		return;

	auto info = AllocEventInfo<bot_character_event_info>();
	info->botCharacter = botCharacter;
	m_botCharacterChatEvent = event_create(bot_character_chat_event, info, number(1, 10));
}
//聊天设置
void CBotCharacterManager::NoticePacket(LPCHARACTER ch, const char* szNotice)
{
	if (!ch)
		return;

	if (!szNotice || szNotice[0] == '\0')
	{
		sys_err("Invalid notice string.");
		return;
	}

	char cleanNotice[1024];
	strlcpy(cleanNotice, szNotice, sizeof(cleanNotice));


	char noticeMessage[CHAT_MAX_LEN + 1];
	strlcpy(noticeMessage, cleanNotice, sizeof(noticeMessage));

	//呐喊
	std::string strEmpireFlagToken;
	switch (ch->GetEmpire())
	{
		case 1: strEmpireFlagToken = "|cFFff0000|H|h[红]|h|r"; break;
		case 2: strEmpireFlagToken = "|cFFffff00|H|h[黄]|h|r"; break;
		case 3: strEmpireFlagToken = "|cFF0080ff|H|h[蓝]|h|r"; break;
	}

	snprintf(noticeMessage, sizeof(noticeMessage), "%s |Hpm:%s|h%s|h|r : %s",
			 strEmpireFlagToken.c_str(),
			 ch->GetName(),
			 ch->GetName(),
			 cleanNotice);
// #endif

	TPacketGGShout p;
	p.bHeader = HEADER_GG_SHOUT;
	p.bEmpire = ch->GetEmpire();
	strlcpy(p.szText, noticeMessage, sizeof(p.szText));
	P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGShout));
	SendShout(noticeMessage, ch->GetEmpire());
}

void CBotCharacterManager::InitializeBotNames()
{
	// const std::string filename = LocaleService_GetBasePath() + "/bot_player/bot_name.txt";

	char szNormalBattlePassFileName[256];
	snprintf(szNormalBattlePassFileName, sizeof(szNormalBattlePassFileName), "%s/bot_player/bot_name.txt", LocaleService_GetBasePath().c_str());

	std::ifstream ifs(szNormalBattlePassFileName);

	if (!ifs.is_open())
	{
		sys_err("bot_name.txt file could not be opened.");
		// 默认机器人随机名称
		m_botNames = {"Bot1", "Bot2", "Bot3", "Bot4", "Bot5", "Bot6", "Bot7", "Bot8", "Bot9", "Bot10"};
		return;
	}

	std::string line;
	int lineNumber = 0;
	int skippedCount = 0;

	while (std::getline(ifs, line))
	{
		lineNumber++;

		// 省略空行和注释。
		if (line.empty() || line[0] == '#' || line[0] == '/')
			continue;

		// 删除行首和行尾的空格。
		line.erase(0, line.find_first_not_of(" \t\r\n"));
		line.erase(line.find_last_not_of(" \t\r\n") + 1);

		if (!line.empty())
		{
			// 机器人名称长度检查 - CHARACTER_NAME_MAX_LEN（24 个字符）限制
			if (line.length() > CHARACTER_NAME_MAX_LEN)
			{
				std::string truncatedName = line.substr(0, CHARACTER_NAME_MAX_LEN);
				sys_log(0, "bot_name.txt line %d: The name is too long, it has been shortened: '%s' -> '%s' (Max: %d characters)",
				        lineNumber, line.c_str(), truncatedName.c_str(), CHARACTER_NAME_MAX_LEN);
				line = truncatedName;
				skippedCount++;
			}

			m_botNames.push_back(line);
		}
	}

	if (m_botNames.empty())
	{
		sys_err("No bot names found in bot_name.txt, using defaults.");
		m_botNames = {"Bot1", "Bot2", "Bot3", "Bot4", "Bot5", "Bot6", "Bot7", "Bot8", "Bot9", "Bot10"};
	}

	sys_log(0, "Loaded %zu bot names from bot_name.txt (%d names truncated to %d chars)",
	        m_botNames.size(), skippedCount, CHARACTER_NAME_MAX_LEN);
}

void CBotCharacterManager::InitializeChatMessages()
{
	// const std::string filename = LocaleService_GetBasePath() + "/bot_player/bot_player_chat.txt";

	char szNormalBattlePassFileName[256];
	snprintf(szNormalBattlePassFileName, sizeof(szNormalBattlePassFileName), "%s/bot_player/bot_player_chat.txt", LocaleService_GetBasePath().c_str());
	std::ifstream ifs(szNormalBattlePassFileName);

	if (!ifs.is_open())
	{
		sys_err("bot_player_chat.txt file could not be opened.");
		// 添加默认呐喊消息
		m_chatMessages = {"你好！", "你好吗？", "开服火爆!", "祝你好运!", "恭喜恭喜老板发财!"};
		return;
	}

	std::string line;
	while (std::getline(ifs, line))
	{
		// 省略空行和注释
		if (line.empty() || line[0] == '#' || line[0] == '/')
			continue;

		// 删除行首和行尾的空格
		line.erase(0, line.find_first_not_of(" \t\r\n"));
		line.erase(line.find_last_not_of(" \t\r\n") + 1);

		if (!line.empty())
		{
			m_chatMessages.push_back(line);
		}
	}

	if (m_chatMessages.empty())
	{
		sys_err("文件 bot_player_chat.txt 中未找到聊天消息！");
		m_chatMessages = {"你好！", "你好吗？", "开服火爆!", "祝你好运!", "恭喜老板发财!"};
	}

	sys_log(0, "Loaded %zu chat messages from bot_player_chat.txt", m_chatMessages.size());
}

void CBotCharacterManager::Initialize()
{
	if (g_bAuthServer)
		return;

	// 上传机器人名称
	InitializeBotNames();

	// 上传聊天记录
	InitializeChatMessages();

	// 从JSON加载装备模板
	// const std::string filename = LocaleService_GetBasePath() + "/bot_player/bot_player.json";

	char filename[256];
	snprintf(filename, sizeof(filename), "%s/bot_player/bot_player.json", LocaleService_GetBasePath().c_str());

	std::ifstream ifs(filename);

	if (!ifs.is_open())
	{
		sys_err("bot_player.json file could not be opened. Using default items.");
		sys_log(0, "Bot system initialized with %zu bot names (no JSON)", m_botNames.size());
		return;
	}

	rapidjson::IStreamWrapper isw(ifs);
	rapidjson::Document doc;
	doc.ParseStream(isw);

	if (!doc.IsArray())
	{
		sys_err("JSON is not formatted as an array.");
		sys_log(0, "Bot system initialized with %zu bot names (invalid JSON)", m_botNames.size());
		return;
	}

	// 从JSON中读取项目模板
	sys_log(0, "JSON'dan %zu adet template being read...", doc.GetArray().Size());

	for (const auto& player : doc.GetArray())
	{
		if (!(player.HasMember("job") && player.HasMember("level") &&
			player["job"].IsInt() && player["level"].IsInt()))
		{
			sys_err("The JSON template contains missing fields, skipping...");
			continue;
		}

		std::unique_ptr<TBotCharacterInfo> playerInfo(new TBotCharacterInfo());
		playerInfo->name = ""; // 名称将取自 bot_name.txt 文件
		playerInfo->job = static_cast<BYTE>(player["job"].GetInt());
		playerInfo->level = static_cast<BYTE>(player["level"].GetInt());
		playerInfo->alignment = player.HasMember("alignment") && player["alignment"].IsInt() ? player["alignment"].GetInt() : 0;
		// playerInfo->mountVnum = player.HasMember("mount_vnum") && player["mount_vnum"].IsInt() ? player["mount_vnum"].GetInt() : 0;

		playerInfo->itemWeapon = player.HasMember("item_weapon") && player["item_weapon"].IsInt() ? static_cast<DWORD>(player["item_weapon"].GetInt()) : 0;
		playerInfo->itemArmor = player.HasMember("item_armor") && player["item_armor"].IsInt() ? static_cast<DWORD>(player["item_armor"].GetInt()) : 0;
		playerInfo->itemHair = player.HasMember("item_hair") && player["item_hair"].IsInt() ? static_cast<DWORD>(player["item_hair"].GetInt()) : 0;
		playerInfo->itemNeck = player.HasMember("item_neck") && player["item_neck"].IsInt() ? static_cast<DWORD>(player["item_neck"].GetInt()) : 0;
		playerInfo->itemEar = player.HasMember("item_ear") && player["item_ear"].IsInt() ? static_cast<DWORD>(player["item_ear"].GetInt()) : 0;
		playerInfo->itemFoots = player.HasMember("item_foots") && player["item_foots"].IsInt() ? static_cast<DWORD>(player["item_foots"].GetInt()) : 0;
		playerInfo->itemWrist = player.HasMember("item_wrist") && player["item_wrist"].IsInt() ? static_cast<DWORD>(player["item_wrist"].GetInt()) : 0;
		playerInfo->itemShield = player.HasMember("item_shield") && player["item_shield"].IsInt() ? static_cast<DWORD>(player["item_shield"].GetInt()) : 0;
		playerInfo->itemHead = player.HasMember("item_head") && player["item_head"].IsInt() ? static_cast<DWORD>(player["item_head"].GetInt()) : 0;
		// playerInfo->itemAura = player.HasMember("item_aura") && player["item_aura"].IsInt() ? static_cast<DWORD>(player["item_aura"].GetInt()) : 0;
		// playerInfo->itemAcce = player.HasMember("item_acce") && player["item_acce"].IsInt() ? static_cast<DWORD>(player["item_acce"].GetInt()) : 0;


		std::string templateKey = "template_" + std::to_string(m_mapBotCharacterInfo.size());
		sys_log(0, "Template '%s' being added: job=%d, weapon=%d, armor=%d, shield=%d",templateKey.c_str(), playerInfo->job, playerInfo->itemWeapon, playerInfo->itemArmor, playerInfo->itemShield);
		m_mapBotCharacterInfo.emplace(templateKey, std::move(playerInfo));
	}

	sys_log(0, "机器人系统已初始化，包含 %zu 个机器人名称和 %zu 个物品模板。", m_botNames.size(), m_mapBotCharacterInfo.size());
}

void CBotCharacterManager::Reload()
{
	sys_log(0, "Reloading bot system...");

	// 清理现有机器人模板
	m_mapBotCharacterInfo.clear();
	m_botNames.clear();
	m_chatMessages.clear();
	m_botLanguages.clear();
	
	// 重新加载机器人文件
	Initialize();

	sys_log(0, "机器人系统已成功重新加载！");
	sys_log(0, "已加载：%zu 角色名称、%zu 呐喊消息、%zu 物品模板",m_botNames.size(), m_chatMessages.size(), m_mapBotCharacterInfo.size());
}

void CBotCharacterManager::BotSpawn(LPCHARACTER ch, int32_t spawn_count, int8_t empire_id)
{
	if (!ch)
		return;

	if (spawn_count >= 200)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "每次召唤机器人最大数量不能超过200个.");
		return;
	}

	// 帝国ID控制
	if (empire_id < 0 || empire_id > 3)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "国家ID 0=随机国家、1=盛唐国、2=秦皇国、3=汉武国");
		return;
	}

	bool useRandomEmpire = (empire_id == 0); // 0 = 生成机器人随机国家
	int8_t baseEmpireId = empire_id;		// 对于固定帝国（1、2、3）

	if (m_botNames.empty())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "无法加载虚拟玩家名称列表");
		return;
	}

	// 可控制的最大机器人数量 - 基于 bot_name.txt 中的名称数量。
	if (m_botCharacters.size() >= m_botNames.size())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "虚拟玩家人数量已达上限:% 无法再生成更多虚拟玩家.", m_botNames.size());
		return;
	}

	// 将机器人名称打乱。
	std::random_device rd;
	std::mt19937 g(rd());
	std::vector<std::string> shuffledNames = m_botNames;
	std::shuffle(shuffledNames.begin(), shuffledNames.end(), g);

	int spawnedCount = 0;
	auto nameIt = shuffledNames.begin();

	while (spawnedCount < spawn_count && nameIt != shuffledNames.end())
	{
		const std::string& botNameOriginal = *nameIt;
		++nameIt;

		// 机器人名称长度检查 - CHARACTER_NAME_MAX_LEN(24个字符)限制
		std::string botName = botNameOriginal;
		if (botName.length() > CHARACTER_NAME_MAX_LEN)
		{
			botName = botName.substr(0, CHARACTER_NAME_MAX_LEN);
			sys_log(0, "The bot name is too long, it has been shortened: '%s' -> '%s' (Max: %d characters)",
			        botNameOriginal.c_str(), botName.c_str(), CHARACTER_NAME_MAX_LEN);
		}

		// 如果已存在同名机器人则跳过
		if (m_botCharacters.find(botName) != m_botCharacters.end())
			continue;

		// 检查真实玩家名称——如果与虚拟玩家名称相同则将其注销.
		LPDESC existingDesc = DESC_MANAGER::instance().FindByCharacterName(botName.c_str());
		if (existingDesc && existingDesc->GetCharacter())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "机器人名称“%s”正被真实玩家使用,该机器人玩家已自动注销...", botName.c_str());
			continue;
		}

		auto botCharacter = CHARACTER_MANAGER::instance().CreateCharacter("");
		if (!botCharacter)
			continue;

		std::unique_ptr<BotCharacter> pBotPlayer(new BotCharacter());
		pBotPlayer->SetBotCharacter(botCharacter);

		botCharacter->SetBotCharacter(true);
		botCharacter->SetName(botName);  // 从 bot_name.txt 文件中获取机器人名称（字符数限制为 CHARACTER_NAME_MAX_LEN）
		botCharacter->SetCharType(CHAR_TYPE_PC);
		
		// 自动随机职业和等级生成
		uint8_t randomJob = number(JOB_WARRIOR, JOB_SHAMAN);//
		uint8_t bot_level = number(34, 37); // 随机等级设置
		int alignment = number(1000, 3000); // 随机善恶值设置


		const char* jobName = "Unknown";
		switch (randomJob)//随机职业自定义
		{
			case JOB_WARRIOR: jobName = "猛将"; break;
			case JOB_ASSASSIN: jobName = "刺客"; break;
			case JOB_SURA: jobName = "修罗"; break;
			case JOB_SHAMAN: jobName = "法师"; break;
		}
		sys_log(0, "Bot %s: 自动生成职业 - 职业: %d (%s), 等级: %d", botName.c_str(), randomJob, jobName, bot_level);

		//基于性别和种族的职位
		BYTE byRace = randomJob;
		BYTE bySex = (number(0, 1) == 1 ? SEX_MALE : SEX_FEMALE);
		switch (randomJob)//随机职业自定义
		{
			case JOB_WARRIOR:
				byRace = (bySex == SEX_MALE) ? MAIN_RACE_WARRIOR_M : MAIN_RACE_WARRIOR_W;
				break;
			case JOB_ASSASSIN:
				byRace = (bySex == SEX_MALE) ? MAIN_RACE_ASSASSIN_M : MAIN_RACE_ASSASSIN_W;
				break;
			case JOB_SURA:
				byRace = (bySex == SEX_MALE) ? MAIN_RACE_SURA_M : MAIN_RACE_SURA_W;
				break;
			case JOB_SHAMAN:
				byRace = (bySex == SEX_MALE) ? MAIN_RACE_SHAMAN_M : MAIN_RACE_SHAMAN_W;
				break;
		}

		botCharacter->SetRace(byRace);
		botCharacter->SetLevel(bot_level);

		sys_log(0, "Bot %s: 种族设置 - Job: %d, 种族: %d, 性别: %s", botName.c_str(), randomJob, byRace, bySex == SEX_MALE ? "男性" : "女性");

		// 生成的角色归属国家 随机或固定
		int8_t botEmpire = useRandomEmpire ? number(1, 3) : baseEmpireId;
		botCharacter->SetEmpire(botEmpire);
		
		sys_log(0, "Bot %s: 国家: %s (模式: %s)", botName.c_str(), empireArray[botEmpire - 1].c_str(), useRandomEmpire ? "随机" : "固定");
		
		botCharacter->UpdateAlignment(alignment * 10);	//善恶值
		botCharacter->SetPoint(POINT_MOV_SPEED, 200);	//移动速度
		botCharacter->SetPoint(POINT_ATT_SPEED, 260);	//攻击速度

		//基础属性
		botCharacter->SetRealPoint(POINT_ST, 30);
		botCharacter->SetRealPoint(POINT_HT, 30);
		botCharacter->SetRealPoint(POINT_DX, 30);
		botCharacter->SetRealPoint(POINT_IQ, 30);

		botCharacter->SetPoint(POINT_ST, 40);
		botCharacter->SetPoint(POINT_HT, 30);
		botCharacter->SetPoint(POINT_DX, 30);
		botCharacter->SetPoint(POINT_IQ, 30);

		int botHP = 24000;	// 机器人玩家的固定生命值
		int randomSP = number(24000, 35000);//精力值随机
		
		botCharacter->SetPoint(POINT_BOW_DISTANCE, 200);
		botCharacter->SetMaxHP(botHP);
		botCharacter->SetMaxSP(randomSP);
		botCharacter->SetHP(botHP);
		botCharacter->SetSP(randomSP);
		botCharacter->PointChange(POINT_HP, botHP);
		botCharacter->PointChange(POINT_SP, randomSP);
		botCharacter->PointChange(POINT_STAMINA, 30000);//耐力
		botCharacter->SetPoint(POINT_DEF_GRADE, 100);	//基础防御力
		botCharacter->SetPoint(POINT_DEF_BONUS, 0);		//防御加成点
		botCharacter->SetPoint(POINT_RESIST_NORMAL_DAMAGE, 20);		//普通伤害减免
		botCharacter->SetPoint(POINT_NORMAL_HIT_DEFEND_BONUS, 20);	//减少物理伤害
		botCharacter->SetPoint(POINT_SKILL_DEFEND_BONUS, 20);		//减少技能伤害
		botCharacter->SetPoint(POINT_RESIST_SWORD, 20);		//减少单刀
		botCharacter->SetPoint(POINT_RESIST_TWOHAND, 20);	//减少重刀
		botCharacter->SetPoint(POINT_RESIST_DAGGER, 20);	//减少双刀
		botCharacter->SetPoint(POINT_RESIST_BELL, 20);		//减少铃铛
		botCharacter->SetPoint(POINT_RESIST_FAN, 20);		//减少扇子
		botCharacter->SetPoint(POINT_RESIST_BOW, 20);		//减少弓箭
		botCharacter->SetPoint(POINT_RESIST_WARRIOR, 20);	//减少猛将
		botCharacter->SetPoint(POINT_RESIST_ASSASSIN, 20);	//减少刺客
		botCharacter->SetPoint(POINT_RESIST_SURA, 20);		//减少修罗
		botCharacter->SetPoint(POINT_RESIST_SHAMAN, 20);	//减少法师
		// botCharacter->SetPoint(POINT_RESIST_CRITICAL, 20);	//减少无视防御
		// botCharacter->SetPoint(POINT_RESIST_PENETRATE, 20);	//减少双倍破坏
		// botCharacter->SetPoint(POINT_CRITICAL_PCT, 100);	//无视防御
		// botCharacter->SetPoint(POINT_PENETRATE_PCT, 100);//双倍破坏

		botCharacter->ComputePoints();

		// 在 ComputePoints() 之后重新调整生命值和防御值
		const int botFixedHP = 24000;
		botCharacter->SetMaxHP(botFixedHP);

		if (botCharacter->GetHP() != botFixedHP)
		{
			botCharacter->SetHP(botFixedHP);
			botCharacter->PointChange(POINT_HP, botFixedHP - botCharacter->GetHP());
		}

		botCharacter->SetPoint(POINT_DEF_GRADE, 100);	//高基础防御力
		botCharacter->SetPoint(POINT_DEF_BONUS, 0);

		// 减少物理技能
		botCharacter->SetPoint(POINT_RESIST_NORMAL_DAMAGE, 10);
		botCharacter->SetPoint(POINT_NORMAL_HIT_DEFEND_BONUS, 20);
		botCharacter->SetPoint(POINT_SKILL_DEFEND_BONUS, 20);

		// 减少职业属性
		botCharacter->SetPoint(POINT_RESIST_WARRIOR, 25);    // 对猛将100%减少
		botCharacter->SetPoint(POINT_RESIST_ASSASSIN, 25);   // 对刺客100%减少
		botCharacter->SetPoint(POINT_RESIST_SURA, 25);       // 对修罗100%减少
		botCharacter->SetPoint(POINT_RESIST_SHAMAN, 25);     // 对法师100%减少
		botCharacter->SetPoint(POINT_ATTBONUS_MONSTER, 30);  // 对怪物强悍30%
		botCharacter->SetPoint(POINT_ATTBONUS_STONE, 30);	 // 对陨石强悍30%
		// 机器人生成时随机坐标范围
		long showX = ch->GetX() + number(-500, 500);
		long showY = ch->GetY() + number(-500, 500);
		int32_t playerMapIndex = ch->GetMapIndex();

		// ch->ChatPacket(CHAT_TYPE_INFO, "机器人生成位置：x:%ld y:%ld（地图：%d", showX, showY, playerMapIndex);

		if (!botCharacter->Show(playerMapIndex, showX, showY, 0))
		{
			M2_DESTROY_CHARACTER(botCharacter);
			sys_err("无法显示机器人角色.");
			continue;
		}

		botCharacter->Stop();
		botCharacter->ReviveInvisible(3);

		// 基于等级和类别的自动物品系统。
		{
			uint32_t weaponVnum = 0;
			uint32_t armorVnum = 0;
			uint32_t hairVnum = 0;
			uint32_t neckVnum = 0;
			uint32_t earVnum = 0;
			uint32_t footsVnum = 0;
			uint32_t wristVnum = 0;
			uint32_t shieldVnum = 0;
			uint32_t headVnum = 0;

			// 性别检查
			bool isMale = (bySex == SEX_MALE);

			// 根据等级确定物品等级
			int itemTier = 0;			//初级装备
			if (bot_level >= 99)		//高级装备+9
				itemTier = 4;
			else if (bot_level >= 75)	//高级装备
				itemTier = 3;
			else if (bot_level >= 34)	//中级装备
				itemTier = 2;
			else if (bot_level >= 26)	//中级装备
				itemTier = 1;
				
				
	
			switch (randomJob)
			{
			case JOB_WARRIOR: //猛将随机装备
				{
					if (itemTier == 1)
					{
						// 猛将26级中级装备
						uint32_t highWeapons[] = {57, 56, 65, 66, 3057, 3056, 3047, 3046};
						weaponVnum = highWeapons[number(0, 7)];

						uint32_t warriorArmors[] = {11225, 11226, 11236, 11235};
						armorVnum = warriorArmors[number(0, 3)];
						
						uint32_t highNeck[] = {16066, 16065, 16026, 16025};
						neckVnum = highNeck[number(0, 3)];
						
						uint32_t highEar[] = {17025, 17026, 17045, 17046, 17065, 17066, 17026};
						earVnum = highEar[number(0, 6)];
						
						uint32_t highFoots[] = {15024, 15025, 15045, 15046, 15085, 15086, 15065};
						footsVnum = highFoots[number(0, 6)];
						
						uint32_t highWrist[] = {14046, 14045, 14065, 14066, 14025, 14026};
						wristVnum = highWrist[number(0, 5)];
						
						uint32_t highShield[] = {13026, 13027, 13007, 13008, 13024};
						shieldVnum = highShield[number(0, 4)];
						
						uint32_t highHead[] = {12225, 12226, 12206, 12207};
						headVnum = highHead[number(0, 3)];
					}
					else if (itemTier == 2)
					{
						// 猛将34级中级装备
						uint32_t highWeapons[] = {294, 295, 3214, 3215};
						weaponVnum = highWeapons[number(0, 3)];

						uint32_t warriorArmors[] = {11245, 11245, 11246};
						armorVnum = warriorArmors[number(0, 2)];
						
						uint32_t highNeck[] = {16104, 16105, 16084, 16085, 16064, 16065};
						neckVnum = highNeck[number(0, 5)];
						
						uint32_t highEar[] = {17104, 17105, 17106, 17084, 17085, 17025};
						earVnum = highEar[number(0, 5)];
						
						uint32_t highFoots[] = {15064, 15065, 15084, 15085, 15104, 15105};
						footsVnum = highFoots[number(0, 5)];
						
						uint32_t highWrist[] = {14044, 14045, 14064, 14065, 14024, 14025, 14104, 14105};
						wristVnum = highWrist[number(0, 7)];

						uint32_t highShield[] = {13024, 13025, 13004, 13005};
						shieldVnum = highShield[number(0, 3)];
						
						uint32_t highHead[] = {12224, 12225, 12226};
						headVnum = highHead[number(0, 2)];
					}
					else if (itemTier == 3)
					{
						// 猛将高级装备
						uint32_t highWeapons[] = {184, 186, 185, 3165, 3166, 3167, 146, 145, 165, 166};
						weaponVnum = highWeapons[number(0, 9)];
						
						uint32_t warriorArmors[] = {11295, 11296, 11286, 11285};
						armorVnum = warriorArmors[number(0, 3)];
						
						uint32_t highNeck[] = {16205, 16206, 16185, 16186, 16165, 16166, 16146, 16147};
						neckVnum = highNeck[number(0, 7)];
						
						uint32_t highEar[] = {17106, 17107, 17108, 17167, 17166, 17165, 17186, 17187, 17205, 17206, 17207, 17208};
						earVnum = highEar[number(0, 11)];
						
						uint32_t highFoots[] = {15085, 15086, 15165, 15166, 15205, 15206};
						footsVnum = highFoots[number(0, 5)];
						
						uint32_t highWrist[] = {14147, 14146, 14167, 14066, 14205, 14206, 14185, 14186};
						wristVnum = highWrist[number(0, 7)];
						
						uint32_t highShield[] = {13065, 13066, 13067, 13085, 13086, 13087, 13105, 13106, 13107, 13125, 13126};
						shieldVnum = highShield[number(0, 10)];
						
						uint32_t highHead[] = {12245, 12246, 12247, 12248};
						headVnum = highHead[number(0, 3)];
					}
					else if (itemTier == 4)
					{
						// 猛将高级装备+9
						uint32_t highWeapons[] = {189, 3169, 149,169};
						weaponVnum = highWeapons[number(0,3)];
						
						uint32_t warriorArmors[] = {11299, 11289};
						armorVnum = warriorArmors[number(0, 1)];
						
						uint32_t highNeck[] = {16205, 16206, 16185, 16186, 16165, 16166, 16146, 16147};
						neckVnum = highNeck[number(0, 7)];
						
						uint32_t highEar[] = {17106, 17107, 17108, 17167, 17166, 17165, 17186, 17187, 17205, 17206, 17207, 17208};
						earVnum = highEar[number(0, 11)];
						
						uint32_t highFoots[] = {15085, 15086, 15165, 15166, 15205, 15206};
						footsVnum = highFoots[number(0, 5)];
						
						uint32_t highWrist[] = {14147, 14146, 14167, 14066, 14205, 14206, 14185, 14186};
						wristVnum = highWrist[number(0, 7)];
						
						uint32_t highShield[] = {13065, 13066, 13067, 13085, 13086, 13087, 13105, 13106, 13107, 13125, 13126};
						shieldVnum = highShield[number(0, 10)];
						
						uint32_t highHead[] = {12245, 12246, 12247, 12248};
						headVnum = highHead[number(0, 3)];
					}
					else
					{
						// 猛将初级装备
						weaponVnum = 16;
						armorVnum = 11206;
						// neckVnum = 16009;
						// earVnum = 17009;
						footsVnum = 15004;
						// wristVnum = 14009;
						shieldVnum = 13004;
						// headVnum = 12209;	//紫藤盔+9
					}

					if (isMale)
					{
						// 男性发型
						uint32_t maleHairs[] = {74001, 74002, 74003, 74004, 74005, 74006, 74007, 74008, 74009, 74010, 74011, 74012};
						hairVnum = maleHairs[number(0, 11)];
					}
					else
					{
						// 女性发型
						uint32_t femaleHairs[] = {75001, 75002, 75003, 75004, 75005, 75006, 75007, 75008, 75009, 75010, 75011, 75012};
						hairVnum = femaleHairs[number(0, 11)];
					}
				}
				break;

			case JOB_ASSASSIN:
				{
					if (itemTier == 1)
					{
						//刺客26级中级装备
						uint32_t highDaggers[] = {56, 55, 65, 66, 1026, 4025, 4026};
						weaponVnum = highDaggers[number(0, 5)];

						uint32_t ninjaArmors[] = {11426, 11427, 11435, 11436};
						armorVnum = ninjaArmors[number(0, 3)];

						uint32_t highNeck[] = {16066, 16065, 16026, 16025};
						neckVnum = highNeck[number(0, 3)];
						
						uint32_t highEar[] = {17025, 17026, 17045, 17046, 17065, 17066, 17026};
						earVnum = highEar[number(0, 6)];
						
						uint32_t highFoots[] = {15024, 15025, 15045, 15046, 15085, 15086, 15065};
						footsVnum = highFoots[number(0, 6)];

						uint32_t highWrist[] = {14046, 14045, 14065, 14066, 14025, 14026};
						wristVnum = highWrist[number(0, 5)];
						
						uint32_t highShield[] = {13026, 13027, 13007, 13008, 13024};
						shieldVnum = highShield[number(0, 4)];
						
						uint32_t highHead[] = {12365, 12366, 12346, 12347};
						headVnum = highHead[number(0, 3)];
					}
					else if (itemTier == 2)
					{
						//刺客34级中级装备
						uint32_t highDaggers[] = {1174, 1175, 294, 295};
						weaponVnum = highDaggers[number(0, 3)];

						uint32_t ninjaArmors[] = {11444, 11445, 11446};
						armorVnum = ninjaArmors[number(0, 2)];

						uint32_t highNeck[] = {16104, 16105, 16084, 16085, 16064, 16065};
						neckVnum = highNeck[number(0, 5)];
						
						uint32_t highEar[] = {17104, 17105, 17084, 17085, 17025};
						earVnum = highEar[number(0, 4)];
						
						uint32_t highFoots[] = {15064, 15065, 15084, 15085, 15104, 15105};
						footsVnum = highFoots[number(0, 5)];
						
						uint32_t highWrist[] = {14044, 14045, 14064, 14065, 14024, 14025, 14104, 14105};
						wristVnum = highWrist[number(0, 7)];
						
						uint32_t highShield[] = {13024, 13025, 13006, 13026};
						shieldVnum = highShield[number(0, 3)];
						
						uint32_t highHead[] = {12364, 12365};
						headVnum = highHead[number(0, 1)];
					}
					else if (itemTier == 3)
					{
						//刺客高级装备
						uint32_t highDaggers[] = {1105, 1106, 1116, 1115, 1135, 1136};
						weaponVnum = highDaggers[number(0, 5)];

						uint32_t ninjaArmors[] = {11495, 11496, 11485, 11486, 11475, 11476};
						armorVnum = ninjaArmors[number(0, 5)];
						
						uint32_t highNeck[] = {16205, 16206, 16185, 16186, 16165, 16166, 16146, 16147};
						neckVnum = highNeck[number(0, 7)];
						
						uint32_t highEar[] = {17106, 17107, 17108, 17167, 17166, 17165, 17186, 17187, 17205, 17206, 17207, 17208};
						earVnum = highEar[number(0, 11)];
						
						uint32_t highFoots[] = {15085, 15086, 15165, 15166, 15205, 15206};
						footsVnum = highFoots[number(0, 5)];
						
						uint32_t highWrist[] = {14147, 14146, 14167, 14066, 14205, 14206, 14185, 14186};
						wristVnum = highWrist[number(0, 7)];
						
						uint32_t highShield[] = {13065, 13066, 13067, 13085, 13086, 13087, 13105, 13106, 13107, 13125, 13126};
						shieldVnum = highShield[number(0, 10)];
						
						uint32_t highHead[] = {12385, 12386, 12387, 12388};
						headVnum = highHead[number(0, 3)];
					}
					else if (itemTier == 4)
					{
						//刺客高级装备+9
						uint32_t highDaggers[] = {1109, 1119, 1139};
						weaponVnum = highDaggers[number(0, 2)];

						uint32_t ninjaArmors[] = {11499,11489,11479};
						armorVnum = ninjaArmors[number(0, 2)];
						
						uint32_t highNeck[] = {16205, 16206, 16185, 16186, 16165, 16166, 16146, 16147};
						neckVnum = highNeck[number(0, 7)];
						
						uint32_t highEar[] = {17106, 17107, 17108, 17167, 17166, 17165, 17186, 17187, 17205, 17206, 17207, 17208};
						earVnum = highEar[number(0, 11)];
						
						uint32_t highFoots[] = {15085, 15086, 15165, 15166, 15205, 15206};
						footsVnum = highFoots[number(0, 5)];
						
						uint32_t highWrist[] = {14147, 14146, 14167, 14066, 14205, 14206, 14185, 14186};
						wristVnum = highWrist[number(0, 7)];
						
						uint32_t highShield[] = {13065, 13066, 13067, 13085, 13086, 13087, 13105, 13106, 13107, 13125, 13126};
						shieldVnum = highShield[number(0, 10)];
						
						uint32_t highHead[] = {12385, 12386, 12387, 12388};
						headVnum = highHead[number(0, 3)];
					}
					else
					{
						//刺客初级装备
						weaponVnum = 1006;
						armorVnum = 11406;
						// neckVnum = 16006;
						// earVnum = 17006;
						footsVnum = 15004;
						// wristVnum = 14006;
						shieldVnum = 13004;
						// headVnum = 12346;	//五纶巾+9
					}

					if (isMale)
					{
						// 男性发型
						uint32_t maleHairs[] = {75201, 75202, 75203, 75204, 75205, 75206, 75207, 75208, 75209, 75210, 75211, 75212};
						hairVnum = maleHairs[number(0, 11)];
					}
					else
					{
						// 女性发型
						uint32_t femaleHairs[] = {74251, 74252, 74253, 74254, 74255, 74256, 74257, 74258, 74259, 74260, 74261, 74262};
						hairVnum = femaleHairs[number(0, 11)];
					}
				}
				break;

			case JOB_SURA:
				{
					if (itemTier == 1)
					{
						//修罗26级中级装备
						uint32_t highSwords[] = {57, 56, 65, 66};
						weaponVnum = highSwords[number(0, 3)];

						uint32_t suraArmors[] = {11625, 11626, 11635, 11636};
						armorVnum = suraArmors[number(0, 3)];
						
						uint32_t highNeck[] = {16066, 16065, 16026, 16025};
						neckVnum = highNeck[number(0, 3)];
						
						uint32_t highEar[] = {17025, 17026, 17045, 17046, 17065, 17066, 17026};
						earVnum = highEar[number(0, 6)];
						
						uint32_t highFoots[] = {15024, 15025, 15045, 15046, 15085, 15086, 15065};
						footsVnum = highFoots[number(0, 6)];
						
						uint32_t highWrist[] = {14046, 14045, 14065, 14066, 14025, 14026};
						wristVnum = highWrist[number(0, 5)];
						
						uint32_t highShield[] = {13026, 13027, 13007, 13008, 13024};
						shieldVnum = highShield[number(0, 4)];
						
						uint32_t highHead[] = {12486, 12487, 12505, 12506};
						headVnum = highHead[number(0, 3)];
					}
					else if (itemTier == 2)
					{
						//修罗34级中级装备
						uint32_t highSwords[] = {294, 295, 296};
						weaponVnum = highSwords[number(0, 2)];

						uint32_t suraArmors[] = {11644, 11645, 11643};
						armorVnum = suraArmors[number(0, 2)];
						
						uint32_t highNeck[] = {16104, 16105, 16084, 16085, 16064, 16065};
						neckVnum = highNeck[number(0, 5)];
						
						uint32_t highEar[] = {17104, 17105, 17103, 17084, 17085, 17024};
						earVnum = highEar[number(0, 5)];
						
						uint32_t highFoots[] = {15064, 15065, 15084, 15085, 15104, 15105};
						footsVnum = highFoots[number(0, 5)];
						
						uint32_t highWrist[] = {14044, 14045, 14064, 14065, 14024, 14025, 14104, 14105};
						wristVnum = highWrist[number(0, 7)];
						
						uint32_t highShield[] = {13024, 13025, 13006};
						shieldVnum = highShield[number(0, 2)];
						
						uint32_t highHead[] = {12484, 12485};
						headVnum = highHead[number(0, 1)];
					}
					else if (itemTier == 3)
					{
						//修罗高级装备

						uint32_t highSwords[] = {184, 185, 146, 145, 165, 166, 245};
						weaponVnum = highSwords[number(0, 6)];

						uint32_t suraArmors[] = {11685, 11686, 11695, 11696, 11676};
						armorVnum = suraArmors[number(0, 4)];
						
						uint32_t highNeck[] = {16205, 16206, 16185, 16186, 16165, 16166, 16146, 16147};
						neckVnum = highNeck[number(0, 7)];
						
						uint32_t highEar[] = {17106, 17107, 17108, 17167, 17166, 17165, 17186, 17187, 17205, 17206, 17207, 17208};
						earVnum = highEar[number(0, 11)];
						
						uint32_t highFoots[] = {15085, 15086, 15165, 15166, 15205, 15206};
						footsVnum = highFoots[number(0, 5)];
						
						uint32_t highWrist[] = {14147, 14146, 14167, 14066, 14205, 14206, 14185, 14186};
						wristVnum = highWrist[number(0, 7)];
						
						uint32_t highShield[] = {13065, 13066, 13067, 13085, 13086, 13087, 13105, 13106, 13107, 13125, 13126};
						shieldVnum = highShield[number(0, 10)];
						
						uint32_t highHead[] = {12525, 12526, 12527, 12528};
						headVnum = highHead[number(0, 3)];
					}
					else if (itemTier == 4)
					{
						//修罗高级装备+9

						uint32_t highSwords[] = {189, 199, 169, 249};
						weaponVnum = highSwords[number(0, 3)];

						uint32_t suraArmors[] = {11689, 11699, 11679};
						armorVnum = suraArmors[number(0, 2)];
						
						uint32_t highNeck[] = {16205, 16206, 16185, 16186, 16165, 16166, 16146, 16147};
						neckVnum = highNeck[number(0, 7)];
						
						uint32_t highEar[] = {17106, 17107, 17108, 17167, 17166, 17165, 17186, 17187, 17205, 17206, 17207, 17208};
						earVnum = highEar[number(0, 11)];
						
						uint32_t highFoots[] = {15085, 15086, 15165, 15166, 15205, 15206};
						footsVnum = highFoots[number(0, 5)];
						
						uint32_t highWrist[] = {14147, 14146, 14167, 14066, 14205, 14206, 14185, 14186};
						wristVnum = highWrist[number(0, 7)];
						
						uint32_t highShield[] = {13065, 13066, 13067, 13085, 13086, 13087, 13105, 13106, 13107, 13125, 13126};
						shieldVnum = highShield[number(0, 10)];
						
						uint32_t highHead[] = {12525, 12526, 12527, 12528};
						headVnum = highHead[number(0, 3)];
					}
					else
					{
						//修罗新手装备
						weaponVnum = 16;
						armorVnum = 11606;
						// neckVnum = 16009;
						// earVnum = 17009;
						footsVnum = 15004;
						// wristVnum = 14009;
						shieldVnum = 13004;
						// headVnum = 12489;	//赤血盔+9
					}

					if (isMale)
					{
						//男性发型
						uint32_t maleHairs[] = {74501, 74502, 74503, 74504, 74505, 74506, 74507, 74508, 74509, 74510, 74511, 74512};
						hairVnum = maleHairs[number(0, 11)];
					}
					else
					{
						//女性发型
						uint32_t femaleHairs[] = {75401, 75402, 75403, 75404, 75405, 75406, 75407, 75408, 75409, 75410, 75411, 75412};
						hairVnum = femaleHairs[number(0, 11)];
					}
				}
				break;

			case JOB_SHAMAN: 
				{
					if (itemTier == 1)
					{
						//法师26级中级装备
						uint32_t highBells[] = {5025, 5026, 7046, 7045, 7055, 7056};
						weaponVnum = highBells[number(0, 5)];

						uint32_t shamanArmors[] = {11825, 11826, 11835, 11836};
						armorVnum = shamanArmors[number(0, 3)];
						
						uint32_t highNeck[] = {16066, 16065, 16026, 16025};
						neckVnum = highNeck[number(0, 3)];
						
						uint32_t highEar[] = {17025, 17026, 17045, 17046, 17065, 17066, 17026};
						earVnum = highEar[number(0, 6)];
						
						uint32_t highFoots[] = {15024, 15025, 15045, 15046, 15085, 15086, 15065};
						footsVnum = highFoots[number(0, 6)];
						
						uint32_t highWrist[] = {14046, 14045, 14065, 14066, 14025, 14026};
						wristVnum = highWrist[number(0, 5)];
						
						uint32_t highShield[] = {13026, 13027, 13007, 13008, 13024};
						shieldVnum = highShield[number(0, 4)];
						
						uint32_t highHead[] = {12626, 12627, 12645, 12646};
						headVnum = highHead[number(0, 3)];

					}
					else if (itemTier == 2)
					{
						//法师34级中级装备
						uint32_t highBells[] = {5114, 5115, 5116, 7164, 7165};
						weaponVnum = highBells[number(0, 4)];

						uint32_t shamanArmors[] = {11844, 11845};
						armorVnum = shamanArmors[number(0, 1)];
						
						uint32_t highNeck[] = {16104, 16105, 16084, 16085, 16064, 16065};
						neckVnum = highNeck[number(0, 5)];
						
						uint32_t highEar[] = {17104, 17103, 17105, 17084, 17085};
						earVnum = highEar[number(0, 4)];
						
						uint32_t highFoots[] = {15064, 15065, 15084, 15085, 15104, 15105};
						footsVnum = highFoots[number(0, 5)];
						
						uint32_t highWrist[] = {14044, 14045, 14064, 14065, 14024, 14025, 14104, 14105};
						wristVnum = highWrist[number(0, 7)];
						
						uint32_t highShield[] = {13024, 13025, 13006};
						shieldVnum = highShield[number(0, 2)];
						
						uint32_t highHead[] = {12643, 12644, 12645};
						headVnum = highHead[number(0, 2)];
					}
					else if (itemTier == 3)
					{
						//法师高级装备
						uint32_t highBells[] = {7145, 7146, 5105, 5106, 7125, 7126};
						weaponVnum = highBells[number(0, 5)];
						
						uint32_t shamanArmors[] = {11885, 11886, 11895, 11896, 11876};
						armorVnum = shamanArmors[number(0, 4)];
						
						uint32_t highNeck[] = {16205, 16206, 16185, 16186, 16165, 16166, 16146, 16147};
						neckVnum = highNeck[number(0, 7)];
						
						uint32_t highEar[] = {17106, 17107, 17108, 17167, 17166, 17165, 17186, 17187, 17205, 17206, 17207, 17208};
						earVnum = highEar[number(0, 11)];
						
						uint32_t highFoots[] = {15085, 15086, 15165, 15166, 15205, 15206};
						footsVnum = highFoots[number(0, 5)];
						
						uint32_t highWrist[] = {14147, 14146, 14167, 14066, 14205, 14206, 14185, 14186};
						wristVnum = highWrist[number(0, 7)];
						
						uint32_t highShield[] = {13065, 13066, 13067, 13085, 13086, 13087, 13105, 13106, 13107, 13125, 13126};
						shieldVnum = highShield[number(0, 10)];
						
						uint32_t highHead[] = {12665, 12666, 12667, 12668};
						headVnum = highHead[number(0, 3)];
					}
					
					else if (itemTier == 4)
					{
						//法师高级装备+9
						uint32_t highBells[] = {7149, 5109, 7129};
						weaponVnum = highBells[number(0, 2)];
						
						uint32_t shamanArmors[] = {11889, 11899,11879};
						armorVnum = shamanArmors[number(0, 2)];
						
						uint32_t highNeck[] = {16205, 16206, 16185, 16186, 16165, 16166, 16146, 16147};
						neckVnum = highNeck[number(0, 7)];
						
						uint32_t highEar[] = {17106, 17107, 17108, 17167, 17166, 17165, 17186, 17187, 17205, 17206, 17207, 17208};
						earVnum = highEar[number(0, 11)];
						
						uint32_t highFoots[] = {15085, 15086, 15165, 15166, 15205, 15206};
						footsVnum = highFoots[number(0, 5)];
						
						uint32_t highWrist[] = {14147, 14146, 14167, 14066, 14205, 14206, 14185, 14186};
						wristVnum = highWrist[number(0, 7)];
						
						uint32_t highShield[] = {13065, 13066, 13067, 13085, 13086, 13087, 13105, 13106, 13107, 13125, 13126};
						shieldVnum = highShield[number(0, 10)];
						
						uint32_t highHead[] = {12665, 12666, 12667, 12668};
						headVnum = highHead[number(0, 3)];
					}
					else
					{
						//法师新手装备
						weaponVnum = 7006;
						armorVnum = 11806;
						// neckVnum = 16009;
						// earVnum = 17009;
						footsVnum = 15004;
						// wristVnum = 14009;
						shieldVnum = 13004;
						// headVnum = 12629;	//炽炎帽+9
					}

					if (isMale)
					{
						// 男性发型
						uint32_t maleHairs[] = {75601, 75602, 75603, 75604, 75605, 75606, 75607, 75608, 75609, 75610, 75611, 75612};
						hairVnum = maleHairs[number(0, 11)];
					}
					else
					{
						// 女性发型
						uint32_t femaleHairs[] = {74751, 74752, 74753, 74754, 74755, 74756, 74757, 74758, 74759, 74760, 74761, 74762};
						hairVnum = femaleHairs[number(0, 11)];
					}
				}
				break;
			}
			
			if (weaponVnum > 0)// 机器人佩戴武器模块
			{
				if (LPITEM w = ITEM_MANAGER::instance().CreateItem(weaponVnum, 1))
				{
					if (!w->EquipTo(botCharacter, w->FindEquipCell(botCharacter)))
					{
						M2_DESTROY_ITEM(w);
					}
					else
					{
						CBotCharacterManager::instance().SetBotWeaponVnum(botCharacter->GetName(), weaponVnum);
					}
				}
			}
			
			if (armorVnum > 0)// 机器人佩戴盔甲模块
			{
				if (LPITEM a = ITEM_MANAGER::instance().CreateItem(armorVnum, 1))
				{
					if (!a->EquipTo(botCharacter, a->FindEquipCell(botCharacter)))
						M2_DESTROY_ITEM(a);
				}
			}

			if (neckVnum > 0)// 机器人佩戴项链模块
			{
				if (LPITEM a = ITEM_MANAGER::instance().CreateItem(neckVnum, 1))
				{
					if (!a->EquipTo(botCharacter, a->FindEquipCell(botCharacter)))
						M2_DESTROY_ITEM(a);
				}
			}
			if (earVnum > 0)// 机器人佩戴耳环模块
			{
				if (LPITEM a = ITEM_MANAGER::instance().CreateItem(earVnum, 1))
				{
					if (!a->EquipTo(botCharacter, a->FindEquipCell(botCharacter)))
						M2_DESTROY_ITEM(a);
				}
			}
			if (footsVnum > 0)// 机器人佩戴鞋子模块
			{
				if (LPITEM a = ITEM_MANAGER::instance().CreateItem(footsVnum, 1))
				{
					if (!a->EquipTo(botCharacter, a->FindEquipCell(botCharacter)))
						M2_DESTROY_ITEM(a);
				}
			}
			if (wristVnum > 0)// 机器人佩戴手镯模块
			{
				if (LPITEM a = ITEM_MANAGER::instance().CreateItem(wristVnum, 1))
				{
					if (!a->EquipTo(botCharacter, a->FindEquipCell(botCharacter)))
						M2_DESTROY_ITEM(a);
				}
			}
			if (shieldVnum > 0)// 机器人佩戴盾牌模块
			{
				if (LPITEM a = ITEM_MANAGER::instance().CreateItem(shieldVnum, 1))
				{
					if (!a->EquipTo(botCharacter, a->FindEquipCell(botCharacter)))
						M2_DESTROY_ITEM(a);
				}
			}
			if (headVnum > 0)// 机器人佩戴头盔模块
			{
				if (LPITEM a = ITEM_MANAGER::instance().CreateItem(headVnum, 1))
				{
					if (!a->EquipTo(botCharacter, a->FindEquipCell(botCharacter)))
						M2_DESTROY_ITEM(a);
				}
			}

#ifdef ENABLE_HAIR_MODULE_
			if (hairVnum > 0)// 机器人佩戴生成发型模块
			{
				if (LPITEM a = ITEM_MANAGER::instance().CreateItem(hairVnum, 1))
				{
					if (!a->EquipTo(botCharacter, a->FindEquipCell(botCharacter)))
						M2_DESTROY_ITEM(a);
				}
			}
#endif
			sys_log(0, "机器人 %s: 自动生成装备 - 性别: %s, 武器: %d, 盔甲: %d, 发型: %d", botCharacter->GetName(), isMale ? "男性" : "女性", weaponVnum, armorVnum, hairVnum);
		}

#ifdef ENABLE_RANDOM_ATTRIBUTES_
		EquipItemAttributes(botCharacter);
		// ch->SkillLevelPacket();
#endif
		//更新视图
		sys_log(0, "机器人 %s: UpdatePacket() 已生成....", botCharacter->GetName());
		botCharacter->UpdatePacket();
		pBotPlayer->SetStartEvent(botCharacter, showX, showY);
		if(botCharacter->GetLevel()>=15) pBotPlayer->SetStartChatEvent(botCharacter);//呐喊等级 喊话等级
		m_botCharacters.emplace(botName, std::move(pBotPlayer));

#ifdef ENABLE_NEW_ONLINE_PLAYER
		char notice[256];
		snprintf(notice, sizeof(notice), "《%s》诞生了一名武学奇才，【%s】统一中原,称霸武林 指日可待.",
			empireArray[botCharacter->GetEmpire() - 1].c_str(),
			botCharacter->GetName()
		);
		BroadcastNotice(notice);
#endif
		botCharacter->SetHorseLevel(11);//生成时11级马
		++spawnedCount;
	}
}

// 基于帝国的机器人生成功能
void CBotCharacterManager::BotSpawnShinsoo(LPCHARACTER ch, int32_t spawn_count)
{
	BotSpawn(ch, spawn_count, 1); // 盛唐国
}

void CBotCharacterManager::BotSpawnChunjo(LPCHARACTER ch, int32_t spawn_count)
{
	BotSpawn(ch, spawn_count, 2); // 秦皇国
}

void CBotCharacterManager::BotSpawnJinno(LPCHARACTER ch, int32_t spawn_count)
{
	BotSpawn(ch, spawn_count, 3); // 汉武国
}

void CBotCharacterManager::BotFullRemove()
{
	for (auto& [_, botCharacter] : m_botCharacters)
	{
		if (botCharacter && botCharacter->GetBotCharacter())
			M2_DESTROY_CHARACTER(botCharacter->GetBotCharacter());
	}
	m_botCharacters.clear();
}

void CBotCharacterManager::BotCharacterRemove(const char* c_szName)
{
	if (!c_szName) return;
	if (auto it = m_botCharacters.find(c_szName); it != m_botCharacters.end())
	{
		if (it->second && it->second->GetBotCharacter())
			M2_DESTROY_CHARACTER(it->second->GetBotCharacter());

		m_botCharacters.erase(it);
	}

	// 修正机器人的语法错误
	m_botLanguages.erase(c_szName);
}

bool CBotCharacterManager::IsBotCharacter(const char* c_szName) const
{
	return m_botCharacters.find(c_szName) != m_botCharacters.end();
}

LPCHARACTER CBotCharacterManager::GetBotCharacter(const char* c_szName) const
{
	if (const auto it = m_botCharacters.find(c_szName); it != m_botCharacters.end())
		return it->second->GetBotCharacter();
	return nullptr;
}

void CBotCharacterManager::EquipItem(LPCHARACTER ch)
{
	if (!ch)
		return;

	auto it = m_mapBotCharacterInfo.find(ch->GetName());
	if (it == m_mapBotCharacterInfo.end())
		return;

	const TBotCharacterInfo& playerInfo = *it->second;

	auto EquipItem = [ch](DWORD vnum)
	{
		if (vnum > 0)
		{
			if (LPITEM item = ITEM_MANAGER::instance().CreateItem(vnum);
				item && !item->EquipTo(ch, item->FindEquipCell(ch)))
			{
				M2_DESTROY_ITEM(item);
			}
		}
	};

	EquipItem(playerInfo.itemWeapon);
	EquipItem(playerInfo.itemArmor);
	EquipItem(playerInfo.itemHair);
	EquipItem(playerInfo.itemNeck);
	EquipItem(playerInfo.itemEar);
	EquipItem(playerInfo.itemFoots);
	EquipItem(playerInfo.itemWrist);
	EquipItem(playerInfo.itemShield);
	EquipItem(playerInfo.itemHead);
}

void CBotCharacterManager::EquipItemAttributes(LPCHARACTER ch)
{
	if (!ch)
		return;

	BYTE job = ch->GetJob();
	LPITEM item = nullptr;

	switch (job)
	{
		case JOB_WARRIOR:
		case JOB_ASSASSIN:
		case JOB_SURA:
		case JOB_SHAMAN:
	{
		//  -- 头盔
		item = ch->GetWear(WEAR_HEAD);
		if (item)
		{
			item->ClearAttribute();
			item->SetForceAttribute(0, APPLY_ATT_SPEED, 8); //攻击速度
			item->SetForceAttribute(1, APPLY_HP_REGEN, 20); //生命值复活量
			item->SetForceAttribute(2, APPLY_SP_REGEN, 30); //精力值复活量
			item->SetForceAttribute(3, APPLY_DODGE, 12);	//对鬼族追加伤害
			item->SetForceAttribute(4, APPLY_STEAL_SP, 10); //将伤害转为精神力
			item->ChangeAttribute();	//更改随机属性
		}

		// -- 武器
		item = ch->GetWear(WEAR_WEAPON);
		if (item)
		{
			item->ClearAttribute();
			item->SetForceAttribute(0, APPLY_CAST_SPEED, 10);	// 释放速度
			item->SetForceAttribute(1, APPLY_CRITICAL_PCT, 10);	// 无视防御
			item->SetForceAttribute(2, APPLY_PENETRATE_PCT, 5);	// 双倍伤害
			item->SetForceAttribute(3, APPLY_ATTBONUS_DEVIL, 20);// 对恶魔追加伤害
			item->SetForceAttribute(4, APPLY_STR, 8);			//力量
			item->ChangeAttribute();	//更改随机属性
		}

		// -- 盾牌
		item = ch->GetWear(WEAR_SHIELD);
		if (item)
		{
			item->ClearAttribute();
			item->SetForceAttribute(0, APPLY_IMMUNE_STUN, 1);	// 不会被击晕
			item->SetForceAttribute(1, APPLY_BLOCK, 12);		// 几率减少物理伤害
			item->SetForceAttribute(2, APPLY_REFLECT_MELEE, 10);// 伤害反弹
			item->SetForceAttribute(3, APPLY_IMMUNE_STUN, 1);	// 不会被击晕
			item->SetForceAttribute(4, APPLY_CON, 12);			// 体力
			item->ChangeAttribute();	//更改随机属性
		}

		//  -- 衣服
		item = ch->GetWear(WEAR_BODY);
		if (item)
		{
			item->ClearAttribute();
			item->SetForceAttribute(0, APPLY_MAX_HP, 1500);			// 最大生命值
			item->SetForceAttribute(1, APPLY_ATT_GRADE_BONUS, 30);	// 攻击力+50
			item->SetForceAttribute(2, APPLY_CAST_SPEED, 10);		// 释放速度
			item->SetForceAttribute(3, APPLY_STEAL_HP, 10);			// PH%恢复
			item->SetForceAttribute(4, APPLY_REFLECT_MELEE, 10);	// 伤害反弹
			item->ChangeAttribute();	//更改随机属性
		}

		//  -- 鞋子
		item = ch->GetWear(WEAR_FOOTS);
		if (item)
		{
			item->ClearAttribute();
			item->SetForceAttribute(0, APPLY_MAX_HP, 2000);		// 最大生命值
			item->SetForceAttribute(1, APPLY_RESIST_SWORD, 12);	//减少单刀伤害
			item->SetForceAttribute(2, APPLY_POISON_REDUCE, 8);	// 抗毒
			item->SetForceAttribute(3, APPLY_ATT_SPEED, 8);		// 攻击速度
			item->SetForceAttribute(4, APPLY_CRITICAL_PCT, 10);	// 双倍破坏
			item->ChangeAttribute();	//更改随机属性
		}

		//  -- 手镯
		item = ch->GetWear(WEAR_WRIST);
		if (item)
		{
			item->ClearAttribute();
			item->SetForceAttribute(0, APPLY_PENETRATE_PCT, 10);	// 无视防御
			item->SetForceAttribute(1, APPLY_MAX_HP, 1500);		// 最大精力值
			item->SetForceAttribute(2, APPLY_ATTBONUS_HUMAN, 10);// 对人形系追加伤害
			item->SetForceAttribute(3, APPLY_STEAL_HP, 10);		// 生命恢复
			item->SetForceAttribute(4, APPLY_RESIST_FIRE, 12);	// 减少火焰攻击伤害
			item->ChangeAttribute();	//更改随机属性
		}

		//  -- 项链
		item = ch->GetWear(WEAR_NECK);
		if (item)
		{
			item->ClearAttribute();
			item->SetForceAttribute(0, APPLY_STUN_PCT, 8);		//几率使敌人晕倒 
			item->SetForceAttribute(1, APPLY_MAX_SP, 80);		//最大精力值
			item->SetForceAttribute(2, APPLY_RESIST_DAGGER, 12);//减少双刀伤害
			item->SetForceAttribute(3, APPLY_PENETRATE_PCT, 10);	//无视防御
			item->SetForceAttribute(4, APPLY_STEAL_SP, 10);		//将伤害转为精神力
			item->ChangeAttribute();	//更改随机属性
		}

		//  -- 耳环
		item = ch->GetWear(WEAR_EAR);
		if (item)
		{
			item->ClearAttribute();
			item->SetForceAttribute(0, APPLY_RESIST_BELL, 12);	// 减少铃铛伤害
			item->SetForceAttribute(1, APPLY_ATTBONUS_HUMAN, 10);//对人形系追加
			item->SetForceAttribute(2, APPLY_POISON_REDUCE, 5);	// 抗毒
			item->SetForceAttribute(3, APPLY_ATTBONUS_DEVIL, 10);// 对恶魔追加伤害
			item->SetForceAttribute(4, APPLY_MOV_SPEED, 10);	// 对僵尸追加伤害
			item->ChangeAttribute();	//更改随机属性
		}
	}
	break;
	}

	// 辅助技能
	ch->SetSkillLevel(SKILL_HORSE_SUMMON, 10);				// 战马
	ch->SetSkillLevel(SKILL_LEADERSHIP, SKILL_MAX_LEVEL);	// 领导
	ch->SetSkillLevel(SKILL_COMBO, 2);						// 技能组
	ch->SetSkillLevel(SKILL_CREATE, SKILL_MAX_LEVEL);		// 钓鱼
	ch->SetSkillLevel(SKILL_MINING, SKILL_MAX_LEVEL);		// 采矿
	ch->SetSkillLevel(SKILL_POLYMORPH, SKILL_MAX_LEVEL);	// 幻化
	ch->SetSkillLevel(SKILL_HORSE, 11);						// 马匹等级
	
	// 向客户端发送技能
	
// 循环前添加：打印职业和是否进入分支
	sys_log(0, "Bot %s: Job=%d, Entering skill group loop", ch->GetName(), job);
	for (BYTE group = 1; group <= 2; group++)
	{
		// 循环内添加：打印当前分组
		sys_log(0, "Bot %s: Processing skill group %d", ch->GetName(), group);
		auto skills = GetClassSkills(job, group);
		// 打印技能列表长度
		sys_log(0, "Bot %s: Group %d skills count = %zu", ch->GetName(), group, skills.size());
		for (DWORD skillVnum : skills)
		{
			BYTE skillLevel = number(20, 31);
			ch->SetSkillLevel(skillVnum, skillLevel);
			// 打印每个技能的设置结果
			sys_log(0, "Bot %s: Set skill %d to level %d", ch->GetName(), skillVnum, skillLevel);
		}
	}

	ch->UpdatePacket(); //刷新角色属性包
	ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
	ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
	
	const int botFixedHP = 24000;
	ch->SetMaxHP(botFixedHP);
	ch->SetHP(botFixedHP);
	ch->PointChange(POINT_HP, 0);
	ch->ComputePoints();
	// 防御属性
	ch->SetPoint(POINT_DEF_GRADE, 100);				// 防御力
	ch->SetPoint(POINT_DEF_BONUS, 0);				// 防御属性
	ch->SetPoint(POINT_NORMAL_HIT_DEFEND_BONUS, 20);// 物理伤害30%
	ch->SetPoint(POINT_RESIST_ASSASSIN, 25);	// 减少猛将25%
	ch->SetPoint(POINT_RESIST_ASSASSIN, 25);	// 减少刺客25%
	ch->SetPoint(POINT_RESIST_SURA, 25);		// 减少修罗25%
	ch->SetPoint(POINT_RESIST_SHAMAN, 25);		// 减少法师25%
// #ifdef ENABLE_WOLFMAN_CHARACTER
	// ch->SetPoint(POINT_RESIST_WOLFMAN, 25);		// 减少狼人25%
// #endif
}
#endif
