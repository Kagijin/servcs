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
#include "../../common/length.h"  // ENABLE_MULTI_LANGUAGE_SYSTEM 必要的

// Forward declarations
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

// #define ENABLE_HAIR_MODULE__			//启用随机发型模块
// #define ENABLE_MOVEPACKE_ATTACK__	//启用显示攻击动画
// #define ENABLE_NEW_ONLINE_PLAYER		//启用生成机器人模拟新手上线公告
// #define ENABLE_USE_POTION__			//启用高级机器人使用药水补给系统
#define ENABLE_RANDOMJOB_WARRIOR_SURA	//启用只随机两个职业猛将和修罗
// #define ENABLE_WANDERING_MODE__		//开启机器人游荡模式

// 帝国地图已不再可用 - 机器人会在生成它们的玩家旁边生成。
// const std::unordered_map<int8_t, std::tuple<int32_t, int32_t, int32_t>> empire_map = {
// 	{1, {1, 459800, 953900}},
// 	{2, {21, 52070, 166600}},
// 	{3, {41, 957300, 255200}}
// };

// 帝国和职业名称（用于PvP日志）
static const std::string jobArray[] = { "猛将", "刺客", "修罗", "法师" };
static const std::string empireArray[] = { "盛唐国", "秦皇国", "汉武国" };

EVENTINFO(bot_character_event_info)
{
	DynamicCharacterPtr botCharacter;
	uint32_t savedMountVnum;
	bool wasMounted;
	DWORD m_dwSkillNextTime;
	long startX; // Variáveis para guardar o local de nascimento do bot
	long startY;

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

		// 1. 自动攻击陨石(最高优先级)
		// if (pkChr->IsStone() || (pkChr->IsMonster() && pkChr->GetMobRank() >= MOB_RANK_BOSS))
		if (pkChr->IsMonster() && (pkChr->GetMobRank() >= MOB_RANK_BOSS))
		{
			isValidTarget = false;
			targetPriority = 3;
		}

		// 2. 自动攻击敌对国家的玩家(高优先级)
		else if (pkChr->IsPC() && !pkChr->IsBotCharacter() && pkChr->GetEmpire() != m_pkChr->GetEmpire())
		{
			isValidTarget = false;
			targetPriority = 2;
		}
		// 3. 自动攻击敌方国家机器人(中优先级)
		else if (pkChr->IsPC() && pkChr->IsBotCharacter() && pkChr->GetEmpire() != m_pkChr->GetEmpire())
		{
			isValidTarget = false;
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
		}
		else // Grup 2
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
		// if (!pkChr->IsMonster() && !pkChr->IsStone()) return;
		if (!pkChr->IsMonster()) 
			return;

		int iDist = DISTANCE_APPROX(m_me->GetX() - pkChr->GetX(), m_me->GetY() - pkChr->GetY());
		if (iDist <= m_iRadius)
		{
			m_targets.push_back(pkChr);
		}
	}
};

EVENTFUNC(bot_character_event)
{
	auto info = dynamic_cast<bot_character_event_info*>(event->info);
	if (!info)
		return 0;

	LPCHARACTER botCharacter = info->botCharacter;
	if (!botCharacter)
		return 0;

	// 站立射击时，每过一秒都要固定好靴子的武器支架。
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

	// 自动为机器人角色使用药水 - 每次事件后检查
	// 当生命值低于70%时使用生命药水(主动)
#ifdef ENABLE_USE_POTION__
	if (botCharacter->GetMaxHP() > 0)
	{
		int currentHP = botCharacter->GetHP();
		int maxHP = botCharacter->GetMaxHP();
		int hpPercent = (maxHP > 0) ? (currentHP * 100 / maxHP) : 0;

		if (hpPercent < 70)
		{
			sys_log(0, "Bot %s: [POTION CHECK] HP dü?ük! HP: %d/%d (%d%%)",
			        botCharacter->GetName(), currentHP, maxHP, hpPercent);


			bool potionUsed = false;// 在腰带物品栏中找到并使用生命药水
			for (int i = 0; i < 3; i++)
			{
				int slotIndex = BELT_INVENTORY_SLOT_START + i;
				LPITEM hpPotion = botCharacter->GetInventoryItem(slotIndex);

				if (hpPotion)
				{
					sys_log(0, "Bot %s: [POTION CHECK] Slot %d: VNUM=%d, Count=%d",
					        botCharacter->GetName(), i, hpPotion->GetVnum(), hpPotion->GetCount());

					if (hpPotion->GetCount() > 0)
					{
						sys_log(0, "Bot %s: [POTION USE] HP potion kullan?l?yor (Slot: %d, VNUM: %d)",
						        botCharacter->GetName(), i, hpPotion->GetVnum());

						bool useResult = botCharacter->UseItemEx(hpPotion, TItemPos(BELT_INVENTORY, i));
						sys_log(0, "Bot %s: [POTION USE] UseItemEx sonucu: %s (HP: %d/%d)",
						        botCharacter->GetName(), useResult ? "SUCCESSFUL" : "UNSUCCESSFUL",
						        botCharacter->GetHP(), botCharacter->GetMaxHP());
						potionUsed = true;
						break;
					}
				}
			}

			if (!potionUsed)
			{
				sys_log(0, "Bot %s: [POTION WARNING] HP potion bulunamad? veya kullan?lamad?!",
				        botCharacter->GetName());
			}
		}
	}

	// SP值低于50%时使用SP药水
	if (botCharacter->GetMaxSP() > 0 && botCharacter->GetSP() < (botCharacter->GetMaxSP() * 50 / 100))
	{

		for (int i = 3; i < 6; i++)//在腰带物品栏中找到并使用SP药水
		{
			int slotIndex = BELT_INVENTORY_SLOT_START + i;
			LPITEM spPotion = botCharacter->GetInventoryItem(slotIndex);
			if (spPotion && spPotion->GetCount() > 0)
			{
				botCharacter->UseItemEx(spPotion, TItemPos(BELT_INVENTORY, i));
				sys_log(0, "Bot %s: SP potion kulland? (SP: %d/%d, Slot: %d)",
				        botCharacter->GetName(), botCharacter->GetSP(), botCharacter->GetMaxSP(), i);
				break;
			}
		}
	}
#endif

	FuncFindVictim f(botCharacter, 2000);//发现目标 引怪距离设置 1位置
	if (botCharacter->GetSectree())
	{
		botCharacter->GetSectree()->ForEachAround(f);
	}

	auto victim = f.GetVictim();
	if (victim)
	{
		int distance = DISTANCE_APPROX(botCharacter->GetX() - victim->GetX(), botCharacter->GetY() - victim->GetY());

		//机器人攻击范围(普通：170 - 弓箭手：800)
		int attackRange = 400;
		BYTE job = botCharacter->GetJob();
		if (job == JOB_ASSASSIN && botCharacter->GetWear(WEAR_WEAPON))
		{
			LPITEM weapon = botCharacter->GetWear(WEAR_WEAPON);
			if (weapon && weapon->GetSubType() == WEAPON_BOW)
				attackRange = 800; // 弓箭手的射程
		}

#ifdef ENABLE_STANDING_MOUNT
		// 只有在攻击首领、陨石时才可以使用坐骑
		bool shouldMount = victim->IsStone() || (victim->IsMonster() && victim->GetMobRank() >= MOB_RANK_BOSS);
#endif

#ifdef ENABLE_STANDING_MOUNT
		// ENABLE_STANDING_MOUNT:所有坐骑均可使用站立坐骑（PVP要求不适用）
		if (shouldMount && !botCharacter->IsRiding())
		{
			// 记录坐骑在袭击发生前的状态。
			if (!info->wasMounted)
			{
				info->wasMounted = false;
				info->savedMountVnum = 0;
			}
			// 保存当前武器的 VNUM 值（以便在需要时重置）。
			if (LPITEM __w = botCharacter->GetWear(WEAR_WEAPON))
				CBotCharacterManager::instance().SetBotWeaponVnum(botCharacter->GetName(), __w->GetVnum());

			// 可骑乘的坐骑VNUM编号：20107, 20108, 20109
			uint32_t standingMountVnums[] = {20107, 20108, 20109};
			uint32_t selectedMountVnum = standingMountVnums[number(0, 2)]; // 从1-3个坐骑中随机选

			// 使用马匹系统（VNUM 为 5 的怪物）召唤站立坐骑
			botCharacter->HorseSummon(true, true, selectedMountVnum);
			botCharacter->StartRiding();
			// 将坐骑安装到固定支架上后，立即确保 PART_WEAPON 的安全。
			{
				int savedWeaponVnum = CBotCharacterManager::instance().GetBotWeaponVnum(botCharacter->GetName());
				WORD partWeapon = botCharacter->GetPart(PART_WEAPON);
				LPITEM wearWeapon = botCharacter->GetWear(WEAR_WEAPON);
				DWORD ensureVnum = wearWeapon ? wearWeapon->GetVnum() : (savedWeaponVnum > 0 ? (DWORD)savedWeaponVnum : 0);
				if (ensureVnum > 0 && partWeapon != ensureVnum)
					botCharacter->SetPart(PART_WEAPON, (WORD)ensureVnum);
			}

			sys_log(0, "Bot %s: Standing Mount added for Boss/Metin (VNUM: %d)",
			        botCharacter->GetName(), selectedMountVnum);
		}
		else if (!shouldMount && botCharacter->IsRiding())
		{
			// 目标已死亡或为普通怪物——已从马上下来
			botCharacter->StopRiding();

			// 返回到你之前的坐骑位置（如果你之前已经安装过）。
			if (info->wasMounted && info->savedMountVnum > 0)
			{
				botCharacter->HorseSummon(true, true, info->savedMountVnum);
				botCharacter->StartRiding();
				sys_log(0, "Bot %s: Returned to the old mount (VNUM: %d)",
				        botCharacter->GetName(), info->savedMountVnum);
			}
			else
			{
				sys_log(0, "Bot %s: Dismounted (on foot)",
				        botCharacter->GetName());
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
		if (SECTREE_MANAGER::instance().IsMovablePosition(victim->GetMapIndex(), victim->GetX(), victim->GetY()))
		{
				float dx = victim->GetX() - botCharacter->GetX();
				float dy = victim->GetY() - botCharacter->GetY();
				botCharacter->SetRotation(GetDegreeFromPosition(dx, dy));

				if (botCharacter->Goto(victim->GetX(), victim->GetY()))
				{
					// goto() 函数已经发送了移动数据包
					botCharacter->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
				}

				return PASSES_PER_SEC(0.1f); // 接近目标时保持冷静的等待
			}
		}

		// 接近目标开始攻击
		if (distance <= attackRange && SECTREE_MANAGER::instance().IsMovablePosition(victim->GetMapIndex(), victim->GetX(), victim->GetY()))
		{
			// Hedefe d?n
			float dx = victim->GetX() - botCharacter->GetX();
			float dy = victim->GetY() - botCharacter->GetY();
			botCharacter->SetRotation(GetDegreeFromPosition(dx, dy));

			//使用 DitoSystem 逻辑进行技能使用和计时器控制。
			const DWORD now = get_dword_time();
			bool canUseSkill = (info->m_dwSkillNextTime <= now);

			sys_log(0, "Bot %s: [SKILL CHECK] canUseSkill=%d, m_dwSkillNextTime=%u, now=%u, victim=%s", botCharacter->GetName(), canUseSkill ? 1 : 0, info->m_dwSkillNextTime, now, victim->GetName());

			if (canUseSkill)
			{
				// 更新计时器(每次使用技能后等待3秒)
				info->m_dwSkillNextTime = now + 3000;

				//根据机器人的类别获取技能列表。
				BYTE skillGroup = number(1, 2);
				auto skills = GetClassSkills(job, skillGroup);

				sys_log(0, "Bot %s: [SKILL CHECK] Job=%d, SkillGroup=%d, Skills count=%zu", botCharacter->GetName(), job, skillGroup, skills.size());

				// SP 控制
				int currentSP = botCharacter->GetSP();
				int maxSP = botCharacter->GetMaxSP();

				if (currentSP < 30)
				{
					sys_log(0, "Bot %s: [SKILL SKIP] Insufficient SP (%d < 30), switching to normal attack.", botCharacter->GetName(), currentSP);
				}
				else if (skills.empty())
				{
					sys_log(0, "Bot %s: [SKILL SKIP] Skill list is empty!", botCharacter->GetName());
				}
				else
				{

					// Dito系统逻辑：按顺序检查技能列表，并使用第一个可用技能
					for (DWORD skillVnum : skills)
					{
						BYTE skillLevel = botCharacter->GetSkillLevel(skillVnum);

						sys_log(0, "Bot %s: [SKILL ITER] Skill %d, Level %d", botCharacter->GetName(), skillVnum, skillLevel);

						// 技能等级控制
						if (skillLevel == 0)
						{
							sys_log(0, "Bot %s: [SKILL SKIP] Skill level %d is 0, skipping.", botCharacter->GetName(), skillVnum);
							continue;
						}

						// DitoSystem 中 SP 被绕过，因此我们为机器人有足够的SP
						// int currentSP = botCharacter->GetSP();
						// int maxSP = botCharacter->GetMaxSP();

						// 如果SP不足,请填写
						if (currentSP < maxSP * 10 / 100)
						{
							botCharacter->PointChange(POINT_SP, maxSP - currentSP);
							sys_log(0, "Bot %s: [SKILL SP] SP is low, replenishing (%d -> %d)", botCharacter->GetName(), currentSP, maxSP);
						}

						// 使用技能 - DitoSystem 逻辑：ComputeSkill(skillIdx, target, skill Level)
						sys_log(0, "Bot %s: [SKILL USE] Skill is being used! Target:%s, Skill:%d, ??Level:%d, SP:%d/%d",
							botCharacter->GetName(), victim->GetName(), skillVnum, skillLevel,
							botCharacter->GetSP(), botCharacter->GetMaxSP());

						// ComputeSkill 调用 - 如 DitoSystem 中所述(带结果检查)
						int result = botCharacter->ComputeSkill(skillVnum, victim, skillLevel);

						// 检查技能使用结果
						sys_log(0, "Bot %s: [SKILL RESULT] ComputeSkill result: %d", botCharacter->GetName(), result);

						if (result != BATTLE_NONE)
						{
							// 机器人使用技能检测
							bool isPvPTarget = victim->IsPC();
							bool isBoss = (victim->IsMonster() && victim->GetMobRank() >= MOB_RANK_BOSS);
							sys_log(0, "Bot %s (%s) attacked character %s %s with a skill! (Skill: %d, Level: %d, Result: %d)",
								botCharacter->GetName(),
								empireArray[botCharacter->GetEmpire() - 1].c_str(),
								isPvPTarget ? "enemy" : (isBoss ? "boss" : "monster"),
								victim->GetName(),
								skillVnum, skillLevel, result);

							// 使用技能后退出循环(DitoSystem 逻辑)
							return PASSES_PER_SEC(1.0f);
						}
						else
						{
							// 技能无法使用，尝试下一个技能
							sys_log(0, "Bot %s: [SKILL FAIL] Skill %d could not be used (Result: BATTLE_NONE), trying the next skill.",botCharacter->GetName(), skillVnum);
						}
					}
				}
			}

			// 普通攻击 - 专业连击系统
			// 机器人角色武器保护 - 攻击前检查
			WORD currentWeapon = botCharacter->GetPart(PART_WEAPON);
			if (currentWeapon == 0)
			{
				// 武器丢失；从任务标记中获取武器编号并重置它。
				int savedWeaponVnum = CBotCharacterManager::instance().GetBotWeaponVnum(botCharacter->GetName());
				if (savedWeaponVnum > 0)
				{
					botCharacter->SetPart(PART_WEAPON, savedWeaponVnum);
					sys_log(0, "Bot %s: Weapon lost, reset from quest flag (Weapon: %d)",
						botCharacter->GetName(), savedWeaponVnum);
				}
				else
				{
					// 如果没有任务标记，请根据职业选择武器
					BYTE job = botCharacter->GetJob();
					uint32_t weaponVnum = 0;

					if (job == JOB_WARRIOR)
						weaponVnum = number(0, 1) == 0 ? 16 : 16;
					else if (job == JOB_ASSASSIN)
						weaponVnum = number(0, 1) == 0 ? 1006 : 1006;
					else if (job == JOB_SURA)
						weaponVnum = 16;
					else if (job == JOB_SHAMAN)
						weaponVnum = number(0, 1) == 0 ? 7006 : 7006;

					if (weaponVnum > 0)
					{
						botCharacter->SetPart(PART_WEAPON, weaponVnum);
						CBotCharacterManager::instance().SetBotWeaponVnum(botCharacter->GetName(), weaponVnum);
						sys_log(0, "Bot %s: Weapon lost, reset according to job (Weapon: %d, Job: %d)",
							botCharacter->GetName(), weaponVnum, job);
					}
				}
			}
			// 骑乘站立时，攻击前先将队伍与装备的武器同步。
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
						sys_log(0, "Bot %s: In the standing mount, the PART_WEAPON was corrected before the attack. (%u)", botCharacter->GetName(), __w->GetVnum());
					}
				}
				else
				{
					int savedWeaponVnum = CBotCharacterManager::instance().GetBotWeaponVnum(botCharacter->GetName());
					if (savedWeaponVnum > 0 && botCharacter->GetPart(PART_WEAPON) != savedWeaponVnum)
					{
						botCharacter->SetPart(PART_WEAPON, savedWeaponVnum);
						sys_log(0, "Bot %s: In the standing mount, the attack was restored from the PART_WEAPON quest flag before the attack. (%d)",
							botCharacter->GetName(), savedWeaponVnum);
					}
				}
			}
#endif

			// Attack() 函数发送了数据包
		// 实现内挂真玩家攻击逻辑
		if (distance <= attackRange)
		{
			// 将机器人转向主要目标。
			float dx = victim->GetX() - botCharacter->GetX();
			float dy = victim->GetY() - botCharacter->GetY();
			botCharacter->SetRotation(GetDegreeFromPosition(dx, dy));

			

			// 1. 设置引怪数量(3到5个之间)
			int iPullLimit = number(2, 5);
			int iCountPulled = 0;

			// 2.引怪搜索怪物半径3000内的怪物
			FuncFindMultiVictim aggroCollector(botCharacter, 3000); //搜索引怪 2位置
			if (botCharacter->GetSectree())
			{
				botCharacter->GetSectree()->ForEachAround(aggroCollector);
			}

			// 3. 发现附近有怪物，则打乱列表以随机引指定怪物数量
			if (!aggroCollector.m_targets.empty())
			{
				// 如果出现错误，您需要添加 <algorithm> 和 <random>，但您的文件中已经包含它们了
				std::shuffle(aggroCollector.m_targets.begin(), aggroCollector.m_targets.end(), std::mt19937(std::random_device()()));

				for (auto target : aggroCollector.m_targets)
				{
					if (iCountPulled >= iPullLimit)
						break;

					// 检查它是否是怪物,是否存活,以及（可选）是否正在攻击机器人
					if (target && !target->IsDead() && target->IsMonster())
					{
						// 如果怪物已经有了其他目标，你可以决定是否抢夺仇恨。
						botCharacter->AggregateMonster(); // 直接引怪
						target->SetVictim(botCharacter);
						iCountPulled++;
					}
				}
			}

			// 1. 寻找附近所有目标(开始攻击)
			// 范围攻击周边怪物的半径为300个单位(根据需要进行调整，以类似于内挂）
			FuncFindMultiVictim splashCollector(botCharacter, 400);//攻击目标范围
			if (botCharacter->GetSectree())
			{
				botCharacter->GetSectree()->ForEachAround(splashCollector);
			}

			bool attackedAny = false;

			// 2. 遍历所有找到的目标
			for (auto target : splashCollector.m_targets)
			{
				// 检查目标是否可被攻击（使用 battle.cpp 函数）
				if (battle_is_attackable(botCharacter, target))
				{
					// 对于直接攻击
					// 注意：战斗近战攻击会根据属性计算实际伤害。
					int iRet = battle_melee_attack(botCharacter, target);

					if (iRet != BATTLE_NONE)
					{
						attackedAny = true;

						if (number(1, 100) <= 10)
						{
							target->CreateFly(FLY_EXP, botCharacter);//模拟经验球飞溅
						}
						
						if (number(1, 100) <= 5)
						{
							target->EffectPacket(SE_CRITICAL);//双倍破坏效果
						}
					}
				}
			}

			if (attackedAny)
			{
				// 发送机器人攻击动画（仅视觉效果，伤害已在上方造成）
#ifdef ENABLE_MOVEPACKE_ATTACK__
				botCharacter->SendMovePacket(FUNC_COMBO, MOTION_COMBO_ATTACK_1, botCharacter->GetX(), botCharacter->GetY(), 0, get_dword_time());
#endif
				// 返回快速攻击速度(类似内挂机器人)
				return PASSES_PER_SEC(0.15f);
			}
		}
			{
				//即使在站立坐骑状态下受到攻击，也要保护 PART_WEAPON
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
				//
				bool isPvPTarget = victim->IsPC();
				if (isPvPTarget && number(1, 10) == 1)
				{
					sys_log(0, "Bot %s (%s) The enemy is attacking player %s (%s)! (HP: %d/%d)",
						botCharacter->GetName(),
						empireArray[botCharacter->GetEmpire() - 1].c_str(),
						victim->GetName(),
						empireArray[victim->GetEmpire() - 1].c_str(),
						victim->GetHP(), victim->GetMaxHP());
				}

				//不断增加连击序列（第 1-5 轮）
				int currentCombo = botCharacter->GetComboSequence();
				int nextCombo = (currentCombo % 5) + 1;

				botCharacter->SetComboSequence(nextCombo);

#ifdef ENABLE_MOVEPACKE_ATTACK__
				//计算组合指数（介于 0-4 之间）
				int comboIndex = (nextCombo - 1) % 5;
#endif

				DWORD dwTime = get_dword_time();
				int interval = dwTime - botCharacter->GetLastComboTime();
				botCharacter->SetValidComboInterval(interval);
				botCharacter->SetLastComboTime(dwTime);
#ifdef ENABLE_MOVEPACKE_ATTACK__
				// 发送攻击动作到客户端 - SendMovePacket
				botCharacter->SendMovePacket(FUNC_COMBO, MOTION_COMBO_ATTACK_1 + comboIndex, botCharacter->GetX(), botCharacter->GetY(), 0, dwTime);
				// botCharacter->SendEffect(SE_PENETRATE);//发送双倍破坏数据包
#endif
				//平衡攻击动作间隔时间
				return PASSES_PER_SEC(0.3f); // 平衡的连击攻击速度
			}
		}
	}
	else
	{
		// 周边没有怪物，偶尔下马休息一下
		if (botCharacter->IsHorseRiding() && !victim && std::rand() % 4 == 0)
			botCharacter->StopRiding();

		/* 机器人返回出生地设置 */
		// 1. 计算出生地坐标距离
		long distToHome = DISTANCE_APPROX(botCharacter->GetX() - info->startX, botCharacter->GetY() - info->startY);

		// 2. 检查距离太远(例如:超过2000个单位/30米)将返回出生地
		if (distToHome > 2000) //超过距离返回坐标
		{
			// 检查路线是否有效，然后返回起始坐标范围
			if (botCharacter->Goto(info->startX, info->startY))
			{
				botCharacter->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
			}
		}
#ifdef ENABLE_WANDERING_MODE__
		// 3. 如果距离挂机坐标很近,机器人会继续随机游荡
		else if (number(0, 2) == 1)
		{
			/* 机器人活动范围设置 */
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

					if (isBlock)
						continue;

					int iDestX = botCharacter->GetX() + (int)fx;
					int iDestY = botCharacter->GetY() + (int)fy;

					if (botCharacter->Goto(iDestX, iDestY))
						botCharacter->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);//发送移动数据包
				}
			}
		}
#endif
	}

	// 机器人发现目标相应速度(0.15 秒)，(20秒)是自由活动间隔时间
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

	// 删除土耳其字符
	char cleanMessage[256];
	strlcpy(cleanMessage, message, sizeof(cleanMessage));

	// 使用 `shout` 函数发送消息（input_main.cpp 格式）
	char shoutMessage[CHAT_MAX_LEN + 1];
	// 帝国旗帜
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

	//随机间隔 1-5 分钟（60-300 秒）
	int randomInterval = number(60, 300);
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
		sys_err("No chat messages found in bot_player_chat.txt, using defaults.");
		m_chatMessages = {"你好！", "你好吗？", "开服火爆!", "祝你好运!", "恭喜恭喜老板发财!"};
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

	// 从JSON加载项目模板
	// const std::string filename = LocaleService_GetBasePath() + "/bot_player/bot_player.json";

	char szNormalBattlePassFileName[256];
	snprintf(szNormalBattlePassFileName, sizeof(szNormalBattlePassFileName), "%s/bot_player/bot_player.json", LocaleService_GetBasePath().c_str());

	std::ifstream ifs(szNormalBattlePassFileName);

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

	// 从JSON中读取项目模板。
	sys_log(0, "JSON'dan %zu adet template okunuyor...", doc.GetArray().Size());

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
		playerInfo->mountVnum = player.HasMember("mount_vnum") && player["mount_vnum"].IsInt() ? player["mount_vnum"].GetInt() : 0;


		playerInfo->itemWeapon = player.HasMember("item_weapon") && player["item_weapon"].IsInt() ? static_cast<DWORD>(player["item_weapon"].GetInt()) : 0;
		playerInfo->itemArmor = player.HasMember("item_armor") && player["item_armor"].IsInt() ? static_cast<DWORD>(player["item_armor"].GetInt()) : 0;
		playerInfo->itemHair = player.HasMember("item_hair") && player["item_hair"].IsInt() ? static_cast<DWORD>(player["item_hair"].GetInt()) : 0;
		playerInfo->itemNeck = player.HasMember("item_neck") && player["item_neck"].IsInt() ? static_cast<DWORD>(player["item_neck"].GetInt()) : 0;
		playerInfo->itemEar = player.HasMember("item_ear") && player["item_ear"].IsInt() ? static_cast<DWORD>(player["item_ear"].GetInt()) : 0;
		playerInfo->itemFoots = player.HasMember("item_foots") && player["item_foots"].IsInt() ? static_cast<DWORD>(player["item_foots"].GetInt()) : 0;
		playerInfo->itemWrist = player.HasMember("item_wrist") && player["item_wrist"].IsInt() ? static_cast<DWORD>(player["item_wrist"].GetInt()) : 0;
		playerInfo->itemShield = player.HasMember("item_shield") && player["item_shield"].IsInt() ? static_cast<DWORD>(player["item_shield"].GetInt()) : 0;
		playerInfo->itemHead = player.HasMember("item_head") && player["item_head"].IsInt() ? static_cast<DWORD>(player["item_head"].GetInt()) : 0;
		playerInfo->itemAura = player.HasMember("item_aura") && player["item_aura"].IsInt() ? static_cast<DWORD>(player["item_aura"].GetInt()) : 0;
		playerInfo->itemAcce = player.HasMember("item_acce") && player["item_acce"].IsInt() ? static_cast<DWORD>(player["item_acce"].GetInt()) : 0;


		std::string templateKey = "template_" + std::to_string(m_mapBotCharacterInfo.size());
		sys_log(0, "Template '%s' being added: job=%d, weapon=%d, armor=%d, shield=%d",
		        templateKey.c_str(), playerInfo->job, playerInfo->itemWeapon,
		        playerInfo->itemArmor, playerInfo->itemShield);
		m_mapBotCharacterInfo.emplace(templateKey, std::move(playerInfo));
	}

	sys_log(0, "Bot system initialized with %zu bot names and %zu item templates", m_botNames.size(), m_mapBotCharacterInfo.size());
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

	sys_log(0, "Bot system reloaded successfully!");
	sys_log(0, "Loaded: %zu names, %zu chat messages, %zu templates",
	        m_botNames.size(), m_chatMessages.size(), m_mapBotCharacterInfo.size());
}

void CBotCharacterManager::BotSpawn(LPCHARACTER ch, int32_t spawn_count, int8_t empire_id)
{
	if (!ch)
		return;

	if (spawn_count >= 300)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "每次召唤机器人最大数量不能超过300个.");
		return;
	}

	// 帝国ID控制（0=随机，1=Shinsoo，2=Chunjo，3=Jinno）
	if (empire_id < 0 || empire_id > 3)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "国家ID 0=随机国家、1=盛唐国、2=秦皇国、3=汉武国");
		return;
	}

	bool useRandomEmpire = (empire_id == 0); // 0 = 每个机器人随机生成一个帝国
	int8_t baseEmpireId = empire_id; // 对于固定帝国（1、2、3）

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

		// 机器人名称长度检查 - CHARACTER_NAME_MAX_LEN（24 个字符）限制
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
			ch->ChatPacket(CHAT_TYPE_INFO, "虚拟玩家“%s”名称正被真实玩家使用,该虚拟玩家正在退出登录.", botName.c_str());
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
		
		// 自动随机职业和等级生成系统
		// 0=战士，1=刺客，2=修罗，3=萨满
		uint8_t randomJob = number(JOB_WARRIOR, JOB_SHAMAN);//
		// uint8_t bot_level = 99; // 固定等级设置
		uint8_t bot_level = number(6, 10); // 随机5-10级
		int alignment = number(100, 2000); // 随机善恶值

		const char* jobName = "Unknown";
		switch (randomJob)//随机职业自定义
		{
			case JOB_WARRIOR: jobName = "warrior"; break;
			case JOB_ASSASSIN: jobName = "assassin"; break;
			case JOB_SURA: jobName = "sura"; break;
			case JOB_SHAMAN: jobName = "shaman"; break;
		}
		sys_log(0, "Bot %s: Automatically generated - Job: %d (%s), Level: %d",
			botName.c_str(), randomJob, jobName, bot_level);

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

		sys_log(0, "Bot %s: Race set - Job: %d, Race: %d, Sex: %s", botName.c_str(), randomJob, byRace, bySex == SEX_MALE ? "男性" : "女性");

		// 生成的角色归属国家 随机或固定
		int8_t botEmpire = useRandomEmpire ? number(1, 3) : baseEmpireId;
		botCharacter->SetEmpire(botEmpire);

		sys_log(0, "Bot %s: Empire atand? - %s (Mode: %s)", botName.c_str(), empireArray[botEmpire - 1].c_str(), useRandomEmpire ? "随机" : "固定");

		botCharacter->UpdateAlignment(alignment * 10);
		botCharacter->SetPoint(POINT_MOV_SPEED, 200);  // 移动速度 250
		botCharacter->SetPoint(POINT_ATT_SPEED, 260);

		//基础属性
		botCharacter->SetRealPoint(POINT_ST, 30);
		botCharacter->SetRealPoint(POINT_HT, 30);
		botCharacter->SetRealPoint(POINT_DX, 30);
		botCharacter->SetRealPoint(POINT_IQ, 30);

		botCharacter->SetPoint(POINT_ST, 40);
		botCharacter->SetPoint(POINT_HT, 30);
		botCharacter->SetPoint(POINT_DX, 30);
		botCharacter->SetPoint(POINT_IQ, 30);

		// 机器人玩家的生命值固定为 24000技能点数随机。
		int botHP = 24000; // 机器人玩家的固定生命值
		int randomSP = number(35000, 40000);

		botCharacter->SetPoint(POINT_BOW_DISTANCE, 200);
		botCharacter->SetMaxHP(botHP);
		botCharacter->SetMaxSP(randomSP);
		botCharacter->SetHP(botHP);
		botCharacter->SetSP(randomSP);
		botCharacter->PointChange(POINT_HP, botHP);
		botCharacter->PointChange(POINT_SP, randomSP);
		botCharacter->PointChange(POINT_STAMINA, 10000);

		// 整体防御值 - 机器人玩家对所有角色都有 100% 的防御力。
		botCharacter->SetPoint(POINT_DEF_GRADE, 100);       // 极高的基础防御力（几乎可以抵挡所有伤害
		botCharacter->SetPoint(POINT_DEF_BONUS, 0);          //防御加成

		// Hasar azaltma de?erleri - %100 koruma i?in
		botCharacter->SetPoint(POINT_RESIST_NORMAL_DAMAGE, 30);   // 100% 普通伤害减免
		botCharacter->SetPoint(POINT_NORMAL_HIT_DEFEND_BONUS, 30); // 99% 减少物理伤害
		botCharacter->SetPoint(POINT_SKILL_DEFEND_BONUS, 30);      // %99 减少技能伤害

		// 所有防御性武器均为 100（剑、双手武器、匕首、扇子、铃铛、弓、爪子）
		botCharacter->SetPoint(POINT_RESIST_SWORD, 30);      //
		botCharacter->SetPoint(POINT_RESIST_TWOHAND, 30);    //
		botCharacter->SetPoint(POINT_RESIST_DAGGER, 30);     //
		botCharacter->SetPoint(POINT_RESIST_BELL, 30);       //
		botCharacter->SetPoint(POINT_RESIST_FAN, 30);        //
		botCharacter->SetPoint(POINT_RESIST_BOW, 30);        //


		botCharacter->SetPoint(POINT_RESIST_WARRIOR, 25);    //
		botCharacter->SetPoint(POINT_RESIST_ASSASSIN, 25);   //
		botCharacter->SetPoint(POINT_RESIST_SURA, 25);       //
		botCharacter->SetPoint(POINT_RESIST_SHAMAN, 25);    //

		// 特殊防御 100
		botCharacter->SetPoint(POINT_RESIST_CRITICAL, 30);   // 减少无视防御
		botCharacter->SetPoint(POINT_RESIST_PENETRATE, 30);  // 减少双倍破坏
		// botCharacter->SetPoint(POINT_CRITICAL_PCT, 100);   // 无视防御
		// botCharacter->SetPoint(POINT_PENETRATE_PCT, 100);  // 双倍破坏

		botCharacter->ComputePoints();

		// 在 ComputePoints() 之后重新调整生命值和防御值（机器人玩家使用固定值）
		const int botFixedHP = 24000;
		botCharacter->SetMaxHP(botFixedHP);
		// 将HP设置为MaxHP（如果MaxHP已更改，则HP也需要更新）。

		if (botCharacter->GetHP() != botFixedHP)
		{
			botCharacter->SetHP(botFixedHP);
			botCharacter->PointChange(POINT_HP, botFixedHP - botCharacter->GetHP());
		}

		botCharacter->SetPoint(POINT_DEF_GRADE, 100);       //级高基础防御力
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
		botCharacter->SetPoint(POINT_ATTBONUS_MONSTER, 20); // 对怪物强悍100%

		// 将机器人生成在生成它的玩家旁边
		long showX = ch->GetX() + number(-500, 500);
		long showY = ch->GetY() + number(-500, 500);
		int32_t playerMapIndex = ch->GetMapIndex();

		// ch->ChatPacket(CHAT_TYPE_INFO, "机器人生成位置：x:%ld y:%ld（地图：%d", showX, showY, playerMapIndex);

		if (!botCharacter->Show(playerMapIndex, showX, showY, 0))
		{
			M2_DESTROY_CHARACTER(botCharacter);
			sys_err("Cannot show bot character.");
			continue;
		}

		botCharacter->Stop();
		botCharacter->ReviveInvisible(5);

		// 基于等级和类别的自动物品系统。
		{
			// S?n?fa g?re silah se?imi
			uint32_t weaponVnum = 0;
			uint32_t armorVnum = 0;
			uint32_t hairVnum = 0;

			// 性别鉴定
			bool isMale = (bySex == SEX_MALE);

			// 根据等级确定物品等级
			int itemTier = 0; // 0=低，1=中，2=高
			if (bot_level >= 6)
				itemTier = 2;
			else if (bot_level >= 99)
				itemTier = 1;
			// else if (bot_level >= 75)
				// itemTier = 3;

			switch (randomJob)
			{
			case JOB_WARRIOR: //猛将随机装备
				{
					if (itemTier == 2)
					{
						// 猛将武器
						uint32_t highWeapons[] = {16, 16};
						weaponVnum = highWeapons[number(0, 1)];
					}
					else if (itemTier == 1)
					{
						// 初级+9
						weaponVnum = number(0, 1) == 0 ? 16 : 16;
					}
					else
					{
						// 初级+0
						weaponVnum = number(0, 1) == 0 ? 16 : 16;
					}

					// 猛将盔甲
					uint32_t warriorArmors[] = {11206, 11206};
					armorVnum = warriorArmors[number(0, 1)];
					// 猛将发型
					if (isMale)
					{
						// 男性发型：基础款(21-44)
						uint32_t maleHairs[] = {74001, 74002, 74003, 74004, 74005, 74006, 74007, 74008, 74009, 74010, 74011, 74012};
						hairVnum = maleHairs[number(0, 11)];
					}
					else
					{
						// 女性发型：基础款(901-906)
						uint32_t femaleHairs[] = {75001, 75002, 75003, 75004, 75005, 75006, 75007, 75008, 75009, 75010, 75011, 75012};
						hairVnum = femaleHairs[number(0, 11)];
					}
				}
				break;

			case JOB_ASSASSIN: // 刺客装备
				{
					if (itemTier == 2)
					{
						// 刺客武器—刀或弓
						bool useBow = (number(0, 1) == 1);
						if (useBow)
						{
							//百弈
							uint32_t highBows[] = {1006, 1006};
							weaponVnum = highBows[number(0, 1)];
						}
						else
						{
							//锋刀
							uint32_t highDaggers[] = {1006, 1006};
							weaponVnum = highDaggers[number(0, 1)];
						}
					}
					else if (itemTier == 1)
					{
						// 初级+9
						weaponVnum = number(0, 1) == 0 ? 1006 : 1006;
					}
					else
					{
						// 初级+0
						weaponVnum = number(0, 1) == 0 ? 1006 : 1006;
					}

					//刺客盔甲
					uint32_t ninjaArmors[] = {11406, 11406};
					armorVnum = ninjaArmors[number(0, 1)];

					//刺客发型——男女区别
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

			case JOB_SURA: //修罗
				{
					if (itemTier == 2)
					{
						//高级修罗武器
						uint32_t highSwords[] = {16, 16};
						weaponVnum = highSwords[number(0, 1)];
					}
					else if (itemTier == 1)
					{
						//初级+9
						weaponVnum = 16;
					}
					else
					{
						//初级+0
						weaponVnum = 16;
					}

					//修罗盔甲
					uint32_t suraArmors[] = {11606, 11606};
					armorVnum = suraArmors[number(0, 1)];

					//修罗发型——男女区别
					if (isMale)
					{
						//男性发型：基础款
						uint32_t maleHairs[] = {74501, 74502, 74503, 74504, 74505, 74506, 74507, 74508, 74509, 74510, 74511, 74512};
						hairVnum = maleHairs[number(0, 11)];
					}
					else
					{
						//女性发型：基础款
						uint32_t femaleHairs[] = {75401, 75402, 75403, 75404, 75405, 75406, 75407, 75408, 75409, 75410, 75411, 75412};
						hairVnum = femaleHairs[number(0, 11)];
					}
				}
				break;

			case JOB_SHAMAN: //法师
				{
					if (itemTier == 2)
					{
						//法师武器- 铃铛或扇子
						bool useFan = (number(0, 1) == 1);
						if (useFan)
						{
							uint32_t highFans[] = {7006, 7006};
							weaponVnum = highFans[number(0, 1)];
						}
						else
						{
							uint32_t highBells[] = {7006, 7006};
							weaponVnum = highBells[number(0, 1)];
						}
					}
					else if (itemTier == 1)
					{
						//初级+9
						weaponVnum = number(0, 1) == 0 ? 7006 : 7006;
					}
					else
					{
						//初级+0
						weaponVnum = number(0, 1) == 0 ? 7006 : 7006;
					}

					// 法师盔甲
					uint32_t shamanArmors[] = {11806, 11806};
					armorVnum = shamanArmors[number(0, 1)];

					// 法师发型——男女区别
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
			// 将物品像真正的物品一样进行搭配（而不是仅仅将它们作为单独的部件摆放）
			if (weaponVnum > 0)
			{
				if (LPITEM w = ITEM_MANAGER::instance().CreateItem(weaponVnum, 1))
				{
					// 找到合适的敷料盒并套上
					if (!w->EquipTo(botCharacter, w->FindEquipCell(botCharacter)))
					{
						M2_DESTROY_ITEM(w);
					}
					else
					{
						//(出于安全考虑)将武器VNUM写入缓存。
						CBotCharacterManager::instance().SetBotWeaponVnum(botCharacter->GetName(), weaponVnum);
					}
				}
			}
			if (armorVnum > 0)
			{
				if (LPITEM a = ITEM_MANAGER::instance().CreateItem(armorVnum, 1))
				{
					if (!a->EquipTo(botCharacter, a->FindEquipCell(botCharacter)))
						M2_DESTROY_ITEM(a);
				}
			}
			// 发型系统 模块 已禁用；需要时可以打开
#ifdef ENABLE_HAIR_MODULE__
			if (hairVnum > 0)
			{
				if (LPITEM a = ITEM_MANAGER::instance().CreateItem(hairVnum, 1))
				{
					if (!a->EquipTo(botCharacter, a->FindEquipCell(botCharacter)))
						M2_DESTROY_ITEM(a);
				}
			}
#endif
			sys_log(0, "Bot %s: Automatic items added - Gender: %s, Weapon: %d, Armor: %d, Hair: %d",
				botCharacter->GetName(), isMale ? "男性" : "女性", weaponVnum, armorVnum, hairVnum);
		}

		// 机器人角色物品栏系统 - 药水和物品使用
		//根据机器人等级添加药水和物品
#ifdef ENABLE_USE_POTION__
		{
			int botLevel = botCharacter->GetLevel();
			uint32_t hpPotionVnum = 0;
			if (botLevel >= 80)
				hpPotionVnum = 27003; //生命药水
			else if (botLevel >= 50)
				hpPotionVnum = 27002; //生命药水
			else
				hpPotionVnum = 27001; //生命药水

			uint32_t spPotionVnum = 0;
			if (botLevel >= 80)
				spPotionVnum = 27006; //魔法瓶
			else if (botLevel >= 50)
				spPotionVnum = 27005; //魔法瓶
			else
				spPotionVnum = 27004; //魔法瓶


			for (int i = 0; i < 3; i++) // 3瓶生命药水 // 将药水添加到腰带物品栏中
			{
				LPITEM hpPotion = ITEM_MANAGER::instance().CreateItem(hpPotionVnum, 1);
				if (hpPotion)
				{
					hpPotion->AddToCharacter(botCharacter, TItemPos(BELT_INVENTORY, i));
					hpPotion->SetCount(99); //最大堆叠
				}
			}

			for (int i = 3; i < 6; i++) // 3瓶SP药水
			{
				LPITEM spPotion = ITEM_MANAGER::instance().CreateItem(spPotionVnum, 1);
				if (spPotion)
				{
					spPotion->AddToCharacter(botCharacter, TItemPos(BELT_INVENTORY, i));
					spPotion->SetCount(99); //最大堆叠
				}
			}


			if (botLevel >= 60)//按机器人等级自动使用药水
			{
				uint32_t extraItems[] = {27003, 27004, 27006}; // 高级机器人的附加药水
				for (int i = 6; i < 9 && i < BELT_INVENTORY_SLOT_COUNT; i++)
				{
					uint32_t extraItemVnum = extraItems[number(0, 2)];
					LPITEM extraItem = ITEM_MANAGER::instance().CreateItem(extraItemVnum, 1);
					if (extraItem)
					{
						extraItem->AddToCharacter(botCharacter, TItemPos(BELT_INVENTORY, i));
						extraItem->SetCount(10); //数量有限
					}
				}
			}

			sys_log(0, "Bot %s: Real Belt inventory filled for Level %d (HP: %d, SP: %d)",
				botCharacter->GetName(), botLevel, hpPotionVnum, spPotionVnum);
		}
#endif
		//更新视图
		sys_log(0, "Bot %s: UpdatePacket() being called....", botCharacter->GetName());
		botCharacter->UpdatePacket();
		sys_log(0, "Bot %s: UpdatePacket() completed", botCharacter->GetName());

		// Passa o showX e showY que foram calculados algumas linhas acima
		pBotPlayer->SetStartEvent(botCharacter, showX, showY);

		if (botCharacter->GetLevel() >= 15)
			pBotPlayer->SetStartChatEvent(botCharacter);

		m_botCharacters.emplace(botName, std::move(pBotPlayer));

		// 机器人模拟玩家新手上线公告 - 目前已禁用(避免不必要的垃圾信息)
#ifdef ENABLE_NEW_ONLINE_PLAYER
		char notice[256];
		snprintf(notice, sizeof(notice), "《%s》诞生了一名武学奇才，【%s】统一中原,称霸武林 指日可待.",
			empireArray[botCharacter->GetEmpire() - 1].c_str(),
			botCharacter->GetName()
		);
		BroadcastNotice(notice);
#endif

		botCharacter->SetHorseLevel(11);

		// 出于性能考虑，机器人生成时不直接佩戴坐骑
		// 当发现陨石或者BOSS时使用坐骑
		sys_log(0, "Bot %s: Bineksiz spawn edildi (performans optimizasyonu)", botCharacter->GetName());

		++spawnedCount;
	}

	// if (useRandomEmpire)
	// {
		// ch->ChatPacket(CHAT_TYPE_INFO, "本次总共创建了%d个机器人(随机国家模式)", spawnedCount);
	// }
	// else
	// {
		// ch->ChatPacket(CHAT_TYPE_INFO, "总共创建了 %d 个机器人(国家: %s) ",
					   // spawnedCount, empireArray[baseEmpireId - 1].c_str());
	// }
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
	if (auto it = m_botCharacters.find(c_szName); it != m_botCharacters.end())
	{
		if (it->second && it->second->GetBotCharacter())
			M2_DESTROY_CHARACTER(it->second->GetBotCharacter());

		m_botCharacters.erase(it);
	}

	// 另外，还要修正一下机器人的语法错误。
	m_botLanguages.erase(c_szName);
}

bool CBotCharacterManager::IsBotCharacter(const char* c_szName) const
{
	return m_botCharacters.find(c_szName) != m_botCharacters.end();
}


void CBotCharacterManager::ForwardPMToGMs(LPCHARACTER sender, const char* botName, const char* message)
{
	if (!sender || !botName || !message)
		return;

	char cleanMessage[CHAT_MAX_LEN + 1];
	strlcpy(cleanMessage, message, sizeof(cleanMessage));


	const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();// 搜索所有玩家并找出GM

	int gmCount = 0;
	for (auto it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
	{
		LPDESC d = *it;
		if (!d || !d->GetCharacter())
			continue;

		LPCHARACTER gm = d->GetCharacter();


		if (!gm->GetGMLevel())//仅发送给GM
			continue;


		SetGMBotReplySession(gm, sender->GetName(), botName);


		char gmNotification[CHAT_MAX_LEN + 256];
		snprintf(gmNotification, sizeof(gmNotification),
				"[机器人消息] %s -> %s: %s (Cevap icin: /w %s mesajiniz)",
				sender->GetName(), botName, cleanMessage, sender->GetName());

		TPacketGCWhisper pack;
		pack.bHeader = HEADER_GC_WHISPER;
		pack.bType = WHISPER_TYPE_SYSTEM;
		pack.wSize = sizeof(TPacketGCWhisper) + strlen(gmNotification) + 1;
		strlcpy(pack.szNameFrom, botName, sizeof(pack.szNameFrom)); // 显示机器人名称

		TEMP_BUFFER buf;
		buf.write(&pack, sizeof(pack));
		buf.write(gmNotification, strlen(gmNotification) + 1);
		d->Packet(buf.read_peek(), buf.size());

		gmCount++;
	}

	sys_log(0, "BOT PM forwarded to %d GM(s): %s -> %s: %s",
	        gmCount, sender->GetName(), botName, cleanMessage);
}

// GM 会话管理
void CBotCharacterManager::SetGMBotReplySession(LPCHARACTER gm, const std::string& playerName, const std::string& botName)
{
	if (!gm)
		return;

	BotPMSession session;
	session.playerName = playerName;
	session.botName = botName;
	session.timestamp = time(0);

	m_gmBotSessions[gm->GetName()] = session;

	sys_log(0, "GM %s bot reply session set: Player=%s Bot=%s",
	        gm->GetName(), playerName.c_str(), botName.c_str());
}

bool CBotCharacterManager::GetGMBotReplySession(LPCHARACTER gm, std::string& outPlayerName, std::string& outBotName)
{
	if (!gm)
		return false;

	auto it = m_gmBotSessions.find(gm->GetName());
	if (it == m_gmBotSessions.end())
		return false;


	if (time(0) - it->second.timestamp > 300)
	{
		m_gmBotSessions.erase(it);
		return false;
	}

	outPlayerName = it->second.playerName;
	outBotName = it->second.botName;
	return true;
}

void CBotCharacterManager::ClearGMBotReplySession(LPCHARACTER gm)
{
	if (!gm)
		return;

	m_gmBotSessions.erase(gm->GetName());
}

void CBotCharacterManager::TalkingMessage(LPCHARACTER ch, const char* c_szMessage)
{
	if (!ch)
		return;

	char cleanMessage[CHAT_MAX_LEN + 1];
	strlcpy(cleanMessage, c_szMessage, sizeof(cleanMessage));

	char chatbuf[CHAT_MAX_LEN + 1];
	// int len = ... (Removido pois n?o é usado)
	snprintf(chatbuf, sizeof(chatbuf), "%s : %s", ch->GetName(), cleanMessage);
	std::string text = chatbuf;
	if (text.empty())
		return;

	struct packet_chat pack_chat;
	pack_chat.header = HEADER_GC_CHAT;
	pack_chat.size = sizeof(struct packet_chat) + text.size() + 1;
	pack_chat.type = CHAT_TYPE_TALKING;
	pack_chat.id = ch->GetVID();
	pack_chat.bEmpire = 0;
	TEMP_BUFFER buf;
	buf.write(&pack_chat, sizeof(struct packet_chat));
	buf.write(text.c_str(), text.size() + 1);
	ch->PacketAround(buf.read_peek(), buf.size());
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
			item->SetForceAttribute(1, APPLY_HP_REGEN, 30); //生命值复活量
			item->SetForceAttribute(2, APPLY_SP_REGEN, 30); //精力值复活量
			item->SetForceAttribute(3, APPLY_DODGE, 15);	//对鬼族追加伤害
			item->SetForceAttribute(4, APPLY_ATTBONUS_STONE, 25); //对陨石20%
		}

		// -- 武器
		item = ch->GetWear(WEAR_WEAPON);
		if (item)
		{
			item->ClearAttribute();
			item->SetForceAttribute(0, APPLY_CAST_SPEED, 20);	  // 释放速度
			item->SetForceAttribute(1, APPLY_CRITICAL_PCT, 10);	  // 无视防御
			item->SetForceAttribute(2, APPLY_PENETRATE_PCT, 10);  // 双倍伤害
			item->SetForceAttribute(3, APPLY_ATTBONUS_DEVIL, 20); // 对恶魔追加伤害
			item->SetForceAttribute(4, APPLY_ATTBONUS_PVM_STR, 50);	//对怪物攻击力
		}

		// -- 盾牌
		item = ch->GetWear(WEAR_SHIELD);
		if (item)
		{
			item->ClearAttribute();
			// item->SetForceAttribute(0, APPLY_IMMUNE_STUN, 1);	 // 不会击晕
			item->SetForceAttribute(0, APPLY_ATTBONUS_MONSTER, 20);	// 对怪物强悍
			item->SetForceAttribute(1, APPLY_BLOCK, 15);		 // 几率减少物理伤害
			item->SetForceAttribute(2, APPLY_REFLECT_MELEE, 10); // 伤害反弹
			item->SetForceAttribute(3, APPLY_IMMUNE_STUN, 1);	 // 不会被击晕
			item->SetForceAttribute(4, APPLY_IMMUNE_SLOW, 1);	 // 不会被缓慢
		}

		//  -- 衣服
		item = ch->GetWear(WEAR_BODY);
		if (item)
		{
			item->ClearAttribute();
			item->SetForceAttribute(0, APPLY_MAX_HP, 2000);		   // 最大生命值
			item->SetForceAttribute(1, APPLY_ATT_GRADE_BONUS, 50); // 攻击力+50
			item->SetForceAttribute(2, APPLY_CAST_SPEED, 20);	   // 释放速度
			item->SetForceAttribute(3, APPLY_STEAL_HP, 10);		   // PH%恢复
			item->SetForceAttribute(4, APPLY_REFLECT_MELEE, 10);   // 伤害反弹
		}

		//  -- 鞋子
		item = ch->GetWear(WEAR_FOOTS);
		if (item)
		{
			item->ClearAttribute();
			item->SetForceAttribute(0, APPLY_MAX_HP, 2000);		// 最大生命值
			item->SetForceAttribute(1, APPLY_MAX_SP, 1000);		// 最大精力值
			item->SetForceAttribute(2, APPLY_MOV_SPEED, 20);	// 移动速度
			item->SetForceAttribute(3, APPLY_ATT_SPEED, 8);		// 攻击速度
			item->SetForceAttribute(4, APPLY_CRITICAL_PCT, 10); // 无视防御
		}

		//  -- 手镯
		item = ch->GetWear(WEAR_WRIST);
		if (item)
		{
			item->ClearAttribute();
			item->SetForceAttribute(0, APPLY_MAX_HP, 2000);		 // 最大生命值
			item->SetForceAttribute(1, APPLY_MAX_SP, 1000);		 // 最大精力值
			item->SetForceAttribute(2, APPLY_PENETRATE_PCT, 10); // 双倍破坏
			item->SetForceAttribute(3, APPLY_STEAL_HP, 10);		 // 生命恢复
			item->SetForceAttribute(4, APPLY_MANA_BURN_PCT, 100); // 耐力值
		}

		//  -- 项链
		item = ch->GetWear(WEAR_NECK);
		if (item)
		{
			item->ClearAttribute();
			item->SetForceAttribute(0, APPLY_MAX_HP, 2000);		 // 最大生命值
			item->SetForceAttribute(1, APPLY_MAX_SP, 1000);		 // 最大精力值
			item->SetForceAttribute(2, APPLY_CRITICAL_PCT, 10);	 // 无视防御
			item->SetForceAttribute(3, APPLY_PENETRATE_PCT, 10); // 双倍破坏
			item->SetForceAttribute(4, APPLY_STEAL_SP, 10);		 // %生命恢复
		}

		//  -- 耳环
		item = ch->GetWear(WEAR_EAR);
		if (item)
		{
			item->ClearAttribute();
			item->SetForceAttribute(0, APPLY_MOV_SPEED, 20);	   // 移动速度
			item->SetForceAttribute(1, APPLY_ATTBONUS_HUMAN, 10);   //对人形系追加
			item->SetForceAttribute(2, APPLY_POISON_REDUCE, 5);	   // 抗毒
			item->SetForceAttribute(3, APPLY_ATTBONUS_DEVIL, 20);  // 对恶魔追加伤害
			item->SetForceAttribute(4, APPLY_ATTBONUS_UNDEAD, 20); // 对僵尸追加伤害
		}
	}
	break;
	}

	// 辅助技能
	ch->SetSkillLevel(SKILL_HORSE_SUMMON, 10);			  // 战马
	ch->SetSkillLevel(SKILL_LEADERSHIP, SKILL_MAX_LEVEL); // 领导
	ch->SetSkillLevel(SKILL_COMBO, 2);					  // 技能组
	ch->SetSkillLevel(SKILL_CREATE, SKILL_MAX_LEVEL);     // 钓鱼
	ch->SetSkillLevel(SKILL_MINING, SKILL_MAX_LEVEL);	  // 采矿
	ch->SetSkillLevel(SKILL_POLYMORPH, SKILL_MAX_LEVEL);  // 幻化
	ch->SetSkillLevel(SKILL_HORSE, 11);			       // 马匹等级

	// 生成职业技能 - 两个职业技能
	//（战斗中随机选择）
	for (BYTE group = 1; group <= 2; group++)
	{
		auto skills = GetClassSkills(job, group);
		for (DWORD skillVnum : skills)
		{
			//将每个技能的等级随机设置为 30-40 级
			BYTE skillLevel = number(10, 20);
			ch->SetSkillLevel(skillVnum, skillLevel);
		}
	}

	sys_log(0, "Bot %s: Job=%d, Skills assigned from both groups",
	        ch->GetName(), job);

	// DitoSystem 逻辑：调用技能等级包（向客户端发送技能）
	ch->SkillLevelPacket();

	ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
	ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());

	ch->ComputePoints();

	// 在 ComputePoints() 之后重新调整生命值和防御值（机器人玩家使用固定值）
	const int botFixedHP = 24000;
	ch->SetMaxHP(botFixedHP);
	// 将HP设置为MaxHP（如果MaxHP已更改，则HP也需要更新）。
	if (ch->GetHP() != botFixedHP)
	{
		ch->SetHP(botFixedHP);
		ch->PointChange(POINT_HP, botFixedHP - ch->GetHP());
	}

	// 防御属性
	ch->SetPoint(POINT_DEF_GRADE, 100);				// 防御力
	ch->SetPoint(POINT_DEF_BONUS, 0);				// 防御属性
	ch->SetPoint(POINT_RESIST_NORMAL_DAMAGE, 20);	// 减少物理伤害30%
	ch->SetPoint(POINT_NORMAL_HIT_DEFEND_BONUS, 20);// 物理伤害30%
	ch->SetPoint(POINT_SKILL_DEFEND_BONUS, 20);		// 减少技能伤害

	// 减少职业属性
	ch->SetPoint(POINT_RESIST_WARRIOR, 25);		// 减少猛将25%
	ch->SetPoint(POINT_RESIST_ASSASSIN, 25);	// 减少刺客25%
	ch->SetPoint(POINT_RESIST_SURA, 25);		// 减少修罗25%
	ch->SetPoint(POINT_RESIST_SHAMAN, 25);		// 减少法师25%
// #ifdef ENABLE_WOLFMAN_CHARACTER
	// ch->SetPoint(POINT_RESIST_WOLFMAN, 25);		// 减少狼人25%
// #endif
}
#endif
