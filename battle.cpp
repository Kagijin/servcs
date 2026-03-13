#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "desc.h"
#include "char.h"
#include "char_manager.h"
#include "battle.h"
#include "item.h"
#include "item_manager.h"
#include "mob_manager.h"
#include "vector.h"
#include "packet.h"
#include "pvp.h"
#include "profiler.h"
#include "guild.h"
#include "affect.h"
#include "unique_item.h"
#include "lua_incl.h"
#include "arena.h"
#include "sectree.h"
#include "ani.h"
#include "locale_service.h"
#include "../../common/Controls.h"

DAM_LL battle_hit(LPCHARACTER ch, LPCHARACTER victim, DAM_LL& iRetDam);

static long long GetResistPoint(LPCHARACTER ch, BYTE point, double value = 0.55)
{
	return (ch->GetPoint(point) * value);
}

bool battle_distance_valid_by_xy(long x, long y, long tx, long ty)
{
	long distance = DISTANCE_APPROX(x - tx, y - ty);

	if (distance > 170)
		return false;

	return true;
}

bool battle_distance_valid(LPCHARACTER ch, LPCHARACTER victim)
{
	return battle_distance_valid_by_xy(ch->GetX(), ch->GetY(), victim->GetX(), victim->GetY());
}

bool timed_event_cancel(LPCHARACTER ch)
{
	if (ch->m_pkTimedEvent)
	{
		event_cancel(&ch->m_pkTimedEvent);
		return true;
	}

	return false;
}

#ifdef NEW_ICEDAMAGE_SYSTEM
bool battle_is_icedamage(LPCHARACTER pAttacker, LPCHARACTER pVictim)
{
	if (pAttacker && pAttacker->IsPC())
	{
		DWORD race = pAttacker->GetRaceNum();
		const DWORD tmp_dwNDRFlag = pVictim->GetNoDamageRaceFlag();
		if (tmp_dwNDRFlag &&
			(race < MAIN_RACE_MAX_NUM) &&
			(IS_SET(tmp_dwNDRFlag, 1 << race))
			)
		{
			return false;
		}
		const std::set<DWORD>& tmp_setNDAFlag = pVictim->GetNoDamageAffectFlag();
		if (tmp_setNDAFlag.size())
		{
			for (std::set<DWORD>::iterator it = tmp_setNDAFlag.begin(); it != tmp_setNDAFlag.end(); ++it)
			{
				if (!pAttacker->IsAffectFlag(*it))
				{
					return false;
				}
			}
		}
	}
	return true;
}
#endif

bool battle_is_attackable(LPCHARACTER ch, LPCHARACTER victim)
{
	//禁止玩家在以下地图进行PK
#ifdef ENABLE_BLOCK_PLAYER_MAP_PK
	if (ch->GetMapIndex() == 303 )
	{
		if (ch->IsPC() && victim->IsPC())
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("PVP is not allowed in the ore field"));
			return false;
		}
	}
#endif
	if (victim->IsDead() || victim->GetMyShop() || victim->IsObserverMode()) // @Lightwork#3
		return false;
		
	if(!ch->IsGuardNPC())
	{
		SECTREE* sectree = NULL;

		sectree = ch->GetSectree();
		if (sectree && sectree->IsAttr(ch->GetX(), ch->GetY(), ATTR_BANPK))
			return false;

		sectree = victim->GetSectree();
		if (sectree && sectree->IsAttr(victim->GetX(), victim->GetY(), ATTR_BANPK))
			return false;
	}
#ifdef NEW_ICEDAMAGE_SYSTEM
	if (!battle_is_icedamage(ch, victim))
		return false;
#endif
	if (ch->IsStun() || ch->IsDead()) // Lightwork correction
		return false;

	if (ch->IsPC() && victim->IsPC())
	{
		CGuild* g1 = ch->GetGuild();
		CGuild* g2 = victim->GetGuild();

		if (g1 && g2)
		{
			if (g1->UnderWar(g2->GetID()))
				return true;
		}
	}

	if (CArenaManager::instance().CanAttack(ch, victim) == true)
		return true;

	return CPVPManager::instance().CanAttack(ch, victim);
}

DAM_LL battle_melee_attack(LPCHARACTER ch, LPCHARACTER victim)
{
	if (!victim || ch == victim)
		return BATTLE_NONE;

	if (!battle_is_attackable(ch, victim))
		return BATTLE_NONE;

	int distance = DISTANCE_APPROX(ch->GetX() - victim->GetX(), ch->GetY() - victim->GetY());

	if (!victim->IsBuilding())
	{
		DAM_LL max = 325; // Lightwork 命中修正325--默认值 300

		if (false == ch->IsPC()
#ifdef ENABLE_BOT_PLAYER
			&& !ch->IsBotCharacter()
#endif
			)
		{
			max = (DAM_LL)(ch->GetMobAttackRange() * 1.15f);
		}
		else
		{
			if (false == victim->IsPC() && BATTLE_TYPE_MELEE == victim->GetMobBattleType()
#ifdef ENABLE_BOT_PLAYER
				&& !victim->IsBotCharacter()
#endif
			)
				max = MAX(325, (DAM_LL)(victim->GetMobAttackRange() * 1.15f));
		}

		if (distance > max)
		{
			return BATTLE_NONE;
		}
	}

	//怪物攻击时可以正常退出游戏
	if (victim->IsPC() && ch->IsPC()) 
	{
		if (timed_event_cancel(ch))
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("傈捧啊 矫累 登绢 秒家 登菌嚼聪促."));//战斗开始了,所以被取消

		if (timed_event_cancel(victim))
			victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("傈捧啊 矫累 登绢 秒家 登菌嚼聪促."));//战斗开始了,所以被取消
	}

	ch->SetPosition(POS_FIGHTING);
	ch->SetVictim(victim);

	const PIXEL_POSITION& vpos = victim->GetXYZ();
	ch->SetRotationToXY(vpos.x, vpos.y);

	DAM_LL dam;
	DAM_LL ret = battle_hit(ch, victim, dam);
	return (ret);
}

void battle_end_ex(LPCHARACTER ch)
{
	if (ch->IsPosition(POS_FIGHTING))
		ch->SetPosition(POS_STANDING);
}

void battle_end(LPCHARACTER ch)
{
	battle_end_ex(ch);
}

// AG = Attack Grade
// AL = Attack Limit
DAM_LL CalcBattleDamage(DAM_LL iDam, int iAttackerLev, int iVictimLev)
{
	if (iDam < 3)
		iDam = number(1, 5);

	//return CALCULATE_DAMAGE_LVDELTA(iAttackerLev, iVictimLev, iDam);
	return iDam;
}

DAM_LL CalcMagicDamageWithValue(DAM_LL iDam, LPCHARACTER pkAttacker, LPCHARACTER pkVictim)
{
	return CalcBattleDamage(iDam, pkAttacker->GetLevel(), pkVictim->GetLevel());
}

DAM_LL CalcMagicDamage(LPCHARACTER pkAttacker, LPCHARACTER pkVictim)
{
	DAM_LL iDam = 0;

	if (pkAttacker->IsNPC())
	{
		iDam = CalcMeleeDamage(pkAttacker, pkVictim, false, false);
	}

	iDam += pkAttacker->GetPoint(POINT_PARTY_ATTACKER_BONUS);

	return CalcMagicDamageWithValue(iDam, pkAttacker, pkVictim);
}

DAM_DOUBLE CalcAttackRating(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, bool bIgnoreTargetRating)
{
	int iARSrc;
	int iERSrc;

	{
		int attacker_dx = pkAttacker->GetPolymorphPoint(POINT_DX);
		int attacker_lv = pkAttacker->GetLevel();

		int victim_dx = pkVictim->GetPolymorphPoint(POINT_DX);
		int victim_lv = pkAttacker->GetLevel();

		iARSrc = MIN(90, (attacker_dx * 4 + attacker_lv * 2) / 6);
		iERSrc = MIN(90, (victim_dx * 4 + victim_lv * 2) / 6);
	}

	DAM_DOUBLE fAR = ((DAM_DOUBLE)iARSrc + 210.0f) / 300.0f;

	if (bIgnoreTargetRating)
		return fAR;

	DAM_DOUBLE fER = ((DAM_DOUBLE)(iERSrc * 2 + 5) / (iERSrc + 95)) * 3.0f / 10.0f;

	return fAR - fER;
}

DAM_LL CalcAttBonus(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, DAM_LL iAtk)
{
	if (pkVictim->IsNPC())
	{
		if (pkVictim->IsStone())
		{
#ifdef ENABLE_EXTRA_APPLY_BONUS
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_STONE)) / 50;
#endif
		}
		else
		{
			if (pkVictim->IsRaceFlag(RACE_FLAG_ANIMAL))
				iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_ANIMAL)) / 100;
			else if (pkVictim->IsRaceFlag(RACE_FLAG_UNDEAD))
				iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_UNDEAD)) / 100;
			else if (pkVictim->IsRaceFlag(RACE_FLAG_DEVIL))
				iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_DEVIL)) / 100;
			else if (pkVictim->IsRaceFlag(RACE_FLAG_ORC))
				iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_ORC)) / 100;
			else if (pkVictim->IsRaceFlag(RACE_FLAG_MILGYO))
				iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_MILGYO)) / 100;
			//新补充动物风火电冰属性
			else if (pkVictim->IsRaceFlag(RACE_FLAG_INSECT))
				iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_INSECT)) / 100;
			else if (pkVictim->IsRaceFlag(RACE_FLAG_FIRE))
				iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_FIRE)) / 100;
			else if (pkVictim->IsRaceFlag(RACE_FLAG_ICE))
				iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_ICE)) / 100;
			else if (pkVictim->IsRaceFlag(RACE_FLAG_DESERT))
				iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_DESERT)) / 100;
			else if (pkVictim->IsRaceFlag(RACE_FLAG_TREE))
				iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_TREE)) / 100;
#ifdef ENABLE_EXTRA_APPLY_BONUS
			if (pkVictim->IsRaceFlag(RACE_FLAG_ATT_FIRE))
				iAtk += (iAtk * (pkAttacker->GetPoint(POINT_ATT_FIRE) + pkAttacker->GetPoint(POINT_ATTBONUS_ELEMENT))) / 100;
			else if (pkVictim->IsRaceFlag(RACE_FLAG_ATT_ICE))
				iAtk += (iAtk * (pkAttacker->GetPoint(POINT_ATT_ICE) + pkAttacker->GetPoint(POINT_ATTBONUS_ELEMENT))) / 100;
			else if (pkVictim->IsRaceFlag(RACE_FLAG_ATT_WIND))
				iAtk += (iAtk * (pkAttacker->GetPoint(POINT_ATT_WIND) + pkAttacker->GetPoint(POINT_ATTBONUS_ELEMENT))) / 100;
			else if (pkVictim->IsRaceFlag(RACE_FLAG_ATT_EARTH))
				iAtk += (iAtk * (pkAttacker->GetPoint(POINT_ATT_EARTH) + pkAttacker->GetPoint(POINT_ATTBONUS_ELEMENT))) / 100;
			else if (pkVictim->IsRaceFlag(RACE_FLAG_ATT_DARK))
				iAtk += (iAtk * (pkAttacker->GetPoint(POINT_ATT_DARK) + pkAttacker->GetPoint(POINT_ATTBONUS_ELEMENT))) / 100;
			else if (pkVictim->IsRaceFlag(RACE_FLAG_ATT_ELEC))
				iAtk += (iAtk * (pkAttacker->GetPoint(POINT_ATT_ELEC) + pkAttacker->GetPoint(POINT_ATTBONUS_ELEMENT))) / 100;

			if (pkVictim->GetMobRank() == MOB_RANK_BOSS)
				iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_BOSS)) / 50;
#endif
		}

		iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_MONSTER)) / 70;
	}
	else if (pkVictim->IsPC()
#ifdef ENABLE_BOT_PLAYER
		|| pkVictim->IsBotCharacter()
#endif
		)
	{
		iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_HUMAN)) / 100;

		switch (pkVictim->GetJob())
		{
		case JOB_WARRIOR:
#ifdef ENABLE_EXTRA_APPLY_BONUS
			iAtk += (iAtk * (pkAttacker->GetPoint(POINT_ATTBONUS_WARRIOR) + pkAttacker->GetPoint(POINT_ATTBONUS_PVP))) / 100;
#else
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_WARRIOR)) / 100;
#endif
			break;

		case JOB_ASSASSIN:
#ifdef ENABLE_EXTRA_APPLY_BONUS
			iAtk += (iAtk * (pkAttacker->GetPoint(POINT_ATTBONUS_ASSASSIN) + pkAttacker->GetPoint(POINT_ATTBONUS_PVP))) / 100;
#else
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_ASSASSIN)) / 100;
#endif
			break;

		case JOB_SURA:
#ifdef ENABLE_EXTRA_APPLY_BONUS
			iAtk += (iAtk * (pkAttacker->GetPoint(POINT_ATTBONUS_SURA) + pkAttacker->GetPoint(POINT_ATTBONUS_PVP))) / 100;
#else
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_SURA)) / 100;
#endif
			break;

		case JOB_SHAMAN:
#ifdef ENABLE_EXTRA_APPLY_BONUS
			iAtk += (iAtk * (pkAttacker->GetPoint(POINT_ATTBONUS_SHAMAN) + pkAttacker->GetPoint(POINT_ATTBONUS_PVP))) / 100;
#else
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_SHAMAN)) / 100;
#endif
			break;
#ifdef ENABLE_WOLFMAN_CHARACTER
		case JOB_WOLFMAN:
#ifdef ENABLE_EXTRA_APPLY_BONUS
			iAtk += (iAtk * (pkAttacker->GetPoint(POINT_ATTBONUS_WOLFMAN) + pkAttacker->GetPoint(POINT_ATTBONUS_PVP))) / 100;
#else
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_WOLFMAN)) / 100;
#endif
			break;
#endif
		}
	}

	if (pkAttacker->IsPC() == true
#ifdef ENABLE_BOT_PLAYER
		|| pkAttacker->IsBotCharacter()
#endif
		)
	{

		switch (pkAttacker->GetJob())
		{
		case JOB_WARRIOR:
#ifdef ENABLE_EXTRA_APPLY_BONUS
			iAtk -= (iAtk * (GetResistPoint(pkVictim,POINT_RESIST_WARRIOR) + GetResistPoint(pkVictim, POINT_DEFBONUS_PVP))) / 100;
#else
			iAtk -= (iAtk * pkVictim->GetPoint(POINT_RESIST_WARRIOR)) / 100;
#endif
			break;

		case JOB_ASSASSIN:
#ifdef ENABLE_EXTRA_APPLY_BONUS
			iAtk -= (iAtk * (GetResistPoint(pkVictim, POINT_RESIST_ASSASSIN) + GetResistPoint(pkVictim, POINT_DEFBONUS_PVP))) / 100;
#else
			iAtk -= (iAtk * pkVictim->GetPoint(POINT_RESIST_ASSASSIN)) / 100;
#endif
			break;

		case JOB_SURA:
#ifdef ENABLE_EXTRA_APPLY_BONUS
			iAtk -= (iAtk * (GetResistPoint(pkVictim, POINT_RESIST_SURA) + GetResistPoint(pkVictim, POINT_DEFBONUS_PVP))) / 100;
#else
			iAtk -= (iAtk * pkVictim->GetPoint(POINT_RESIST_SURA)) / 100;
#endif
			break;

		case JOB_SHAMAN:
#ifdef ENABLE_EXTRA_APPLY_BONUS
			iAtk -= (iAtk * (GetResistPoint(pkVictim, POINT_RESIST_SHAMAN) + GetResistPoint(pkVictim, POINT_DEFBONUS_PVP))) / 100;
#else
			iAtk -= (iAtk * pkVictim->GetPoint(POINT_RESIST_SHAMAN)) / 100;
#endif
			break;

#ifdef ENABLE_WOLFMAN_CHARACTER
		case JOB_WOLFMAN:
#ifdef ENABLE_EXTRA_APPLY_BONUS
			iAtk -= (iAtk * (GetResistPoint(pkVictim, POINT_RESIST_WOLFMAN) + GetResistPoint(pkVictim, POINT_DEFBONUS_PVP))) / 100;
#else
			iAtk -= (iAtk * pkVictim->GetPoint(POINT_RESIST_WOLFMAN)) / 100;
#endif
			break;
#endif
		}
	}
#ifdef ENABLE_BOT_PLAYER
	if ((pkAttacker->IsNPC() && pkVictim->IsPC()) || pkVictim->IsBotCharacter())
#else
	if (pkAttacker->IsNPC() && pkVictim->IsPC())
#endif
	{
		iAtk = (iAtk * CHARACTER_MANAGER::instance().GetMobDamageRate(pkAttacker)) / 100;
#ifdef ENABLE_EXTRA_APPLY_BONUS
		if (pkAttacker->IsRaceFlag(RACE_FLAG_ATT_ELEC))
			iAtk -= (iAtk * (pkVictim->GetPoint(POINT_DEFBONUS_ELEMENT) + pkVictim->GetPoint(POINT_RESIST_ELEC))) / 150;
		else if (pkAttacker->IsRaceFlag(RACE_FLAG_ATT_FIRE))
			iAtk -= (iAtk * (pkVictim->GetPoint(POINT_DEFBONUS_ELEMENT) + pkVictim->GetPoint(POINT_RESIST_FIRE))) / 150;
		else if (pkAttacker->IsRaceFlag(RACE_FLAG_ATT_ICE))
			iAtk -= (iAtk * (pkVictim->GetPoint(POINT_DEFBONUS_ELEMENT) + pkVictim->GetPoint(POINT_RESIST_ICE))) / 150;
		else if (pkAttacker->IsRaceFlag(RACE_FLAG_ATT_WIND))
			iAtk -= (iAtk * (pkVictim->GetPoint(POINT_DEFBONUS_ELEMENT) + pkVictim->GetPoint(POINT_RESIST_WIND))) / 150;
		else if (pkAttacker->IsRaceFlag(RACE_FLAG_ATT_EARTH))
			iAtk -= (iAtk * (pkVictim->GetPoint(POINT_DEFBONUS_ELEMENT) + pkVictim->GetPoint(POINT_RESIST_EARTH))) / 150;
		else if (pkAttacker->IsRaceFlag(RACE_FLAG_ATT_DARK))
			iAtk -= (iAtk * (pkVictim->GetPoint(POINT_DEFBONUS_ELEMENT) + pkVictim->GetPoint(POINT_RESIST_DARK))) / 150;

		iAtk -= (iAtk * pkVictim->GetPoint(POINT_DEFBONUS_PVM)) / 125;
#else
		if (pkAttacker->IsRaceFlag(RACE_FLAG_ATT_ELEC))
			iAtk -= (iAtk * 30 * pkVictim->GetPoint(POINT_RESIST_ELEC)) / 10000;
		else if (pkAttacker->IsRaceFlag(RACE_FLAG_ATT_FIRE))
			iAtk -= (iAtk * 30 * pkVictim->GetPoint(POINT_RESIST_FIRE)) / 10000;
		else if (pkAttacker->IsRaceFlag(RACE_FLAG_ATT_ICE))
			iAtk -= (iAtk * 30 * pkVictim->GetPoint(POINT_RESIST_ICE)) / 10000;
		else if (pkAttacker->IsRaceFlag(RACE_FLAG_ATT_WIND))
			iAtk -= (iAtk * 30 * pkVictim->GetPoint(POINT_RESIST_WIND)) / 10000;
		else if (pkAttacker->IsRaceFlag(RACE_FLAG_ATT_EARTH))
			iAtk -= (iAtk * 30 * pkVictim->GetPoint(POINT_RESIST_EARTH)) / 10000;
		else if (pkAttacker->IsRaceFlag(RACE_FLAG_ATT_DARK))
			iAtk -= (iAtk * 30 * pkVictim->GetPoint(POINT_RESIST_DARK)) / 10000;
#endif
	}

	return iAtk;
}

void Item_GetDamage(LPITEM pkItem, DAM_LL* pdamMin, DAM_LL* pdamMax)
{
	*pdamMin = 0;
	*pdamMax = 1;

	if (!pkItem)
		return;

	switch (pkItem->GetType())
	{
	case ITEM_ROD:
	case ITEM_PICK:
		return;
	}

	if (pkItem->GetType() != ITEM_WEAPON)
		sys_err("Item_GetDamage - !ITEM_WEAPON vnum=%d, type=%d", pkItem->GetOriginalVnum(), pkItem->GetType());

	*pdamMin = pkItem->GetValue(3);
	*pdamMax = pkItem->GetValue(4);
}

DAM_LL CalcMeleeDamage(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, bool bIgnoreDefense, bool bIgnoreTargetRating)
{
	LPITEM pWeapon = pkAttacker->GetWear(WEAR_WEAPON);
	bool bPolymorphed = pkAttacker->IsPolymorphed();

	if (pWeapon && !(bPolymorphed && !pkAttacker->IsPolyMaintainStat()))
	{
		if (pWeapon->GetType() != ITEM_WEAPON)
			return 0;

		switch (pWeapon->GetSubType())
		{
		case WEAPON_SWORD:
		case WEAPON_DAGGER:
		case WEAPON_TWO_HANDED:
		case WEAPON_BELL:
		case WEAPON_FAN:
		case WEAPON_MOUNT_SPEAR:
#ifdef ENABLE_WOLFMAN_CHARACTER
		case WEAPON_CLAW:
#endif
			break;

		case WEAPON_BOW:
			// sys_err("CalcMeleeDamage should not handle bows (name: %s)", pkAttacker->GetName());
			return 0;

		default:
			return 0;
		}
	}

	DAM_LL iDam = 0;
	DAM_DOUBLE fAR = CalcAttackRating(pkAttacker, pkVictim, bIgnoreTargetRating);
	DAM_LL iDamMin = 0, iDamMax = 0;


	if (bPolymorphed && !pkAttacker->IsPolyMaintainStat())
	{
		// MONKEY_ROD_ATTACK_BUG_FIX
		Item_GetDamage(pWeapon, &iDamMin, &iDamMax);
		// END_OF_MONKEY_ROD_ATTACK_BUG_FIX

		DWORD dwMobVnum = pkAttacker->GetPolymorphVnum();
		const CMob* pMob = CMobManager::instance().Get(dwMobVnum);

		if (pMob)
		{
			int iPower = pkAttacker->GetPolymorphPower();
			iDamMin += pMob->m_table.dwDamageRange[0] * iPower / 100;
			iDamMax += pMob->m_table.dwDamageRange[1] * iPower / 100;
		}
	}
	else if (pWeapon)
	{
		// MONKEY_ROD_ATTACK_BUG_FIX
		Item_GetDamage(pWeapon, &iDamMin, &iDamMax);
		// END_OF_MONKEY_ROD_ATTACK_BUG_FIX
	}
	else if (pkAttacker->IsNPC())
	{
		iDamMin = pkAttacker->GetMobDamageMin();
		iDamMax = pkAttacker->GetMobDamageMax();
	}

	iDam = number(iDamMin, iDamMax) * 2;
	DAM_LL iAtk = 0;
#ifdef ENABLE_EXTRA_APPLY_BONUS
	if (pkAttacker->IsPC() && pkVictim->IsPvM())
	{
		iAtk = (pkAttacker->GetPoint(POINT_ATT_GRADE) + pkAttacker->GetPoint(POINT_ATTBONUS_PVM_STR)) + iDam - (pkAttacker->GetLevel() * 2);
}
	else
	{
		iAtk = pkAttacker->GetPoint(POINT_ATT_GRADE) + iDam - (pkAttacker->GetLevel() * 2);
	}
#else
	iAtk = pkAttacker->GetPoint(POINT_ATT_GRADE) + iDam - (pkAttacker->GetLevel() * 2);
#endif
	iAtk = (DAM_LL)(iAtk * fAR);
	iAtk += pkAttacker->GetLevel() * 2; // and add again

	if (pWeapon)
	{
		iAtk += pWeapon->GetValue(5) * 2;
	}

	iAtk += pkAttacker->GetPoint(POINT_PARTY_ATTACKER_BONUS); // party attacker role bonus
#ifdef ENABLE_EXTRA_APPLY_BONUS
	if (pkAttacker->IsPC() && pkVictim->IsPvM())
	{
		iAtk = (DAM_LL)(iAtk * (100 + (pkAttacker->GetPoint(POINT_ATT_BONUS) + pkAttacker->GetPoint(POINT_MELEE_MAGIC_ATT_BONUS_PER) + pkAttacker->GetPoint(POINT_ATTBONUS_PVM_BERSERKER))) / 100);
	}
	else
	{
		iAtk = (DAM_LL)(iAtk * (100 + (pkAttacker->GetPoint(POINT_ATT_BONUS) + pkAttacker->GetPoint(POINT_MELEE_MAGIC_ATT_BONUS_PER))) / 100);
	}
#else
	iAtk = (DAM_LL)(iAtk * (100 + (pkAttacker->GetPoint(POINT_ATT_BONUS) + pkAttacker->GetPoint(POINT_MELEE_MAGIC_ATT_BONUS_PER))) / 100);
#endif

	iAtk = CalcAttBonus(pkAttacker, pkVictim, iAtk);

	DAM_LL iDef = 0;

	if (!bIgnoreDefense)
	{
		iDef = (pkVictim->GetPoint(POINT_DEF_GRADE) * (100 + pkVictim->GetPoint(POINT_DEF_BONUS)) / 100);

		if (!pkAttacker->IsPC())
			iDef += pkVictim->GetMarriageBonus(UNIQUE_ITEM_MARRIAGE_DEFENSE_BONUS);
	}

	if (pkAttacker->IsNPC())
		iAtk = (DAM_LL)(iAtk * pkAttacker->GetMobDamageMultiply());

	iDam = MAX(0, iAtk - iDef);

	return CalcBattleDamage(iDam, pkAttacker->GetLevel(), pkVictim->GetLevel());
}

DAM_LL CalcArrowDamage(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, LPITEM pkBow, LPITEM pkArrow, bool bIgnoreDefense)
{
	if (!pkBow || pkBow->GetType() != ITEM_WEAPON || pkBow->GetSubType() != WEAPON_BOW)
		return 0;

	if (!pkArrow)
		return 0;

	int iDist = (int)(DISTANCE_SQRT(pkAttacker->GetX() - pkVictim->GetX(), pkAttacker->GetY() - pkVictim->GetY()));
	int iGap = (iDist / 100) - 5 - pkAttacker->GetPoint(POINT_BOW_DISTANCE);
	int iPercent = 100 - (iGap * 5);

	if (iPercent <= 0)
		return 0;
	else if (iPercent > 100)
		iPercent = 100;

	DAM_LL iDam = 0;

	DAM_DOUBLE fAR = CalcAttackRating(pkAttacker, pkVictim, false);
	iDam = number(pkBow->GetValue(3), pkBow->GetValue(4)) * 2 + pkArrow->GetValue(3);
	DAM_LL iAtk;

	// level must be ignored when multiply by fAR, so subtract it before calculation.
	iAtk = pkAttacker->GetPoint(POINT_ATT_GRADE) + iDam - (pkAttacker->GetLevel() * 2);
	iAtk = (DAM_LL)(iAtk * fAR);
	iAtk += pkAttacker->GetLevel() * 2; // and add again

	// Refine Grade
	iAtk += pkBow->GetValue(5) * 2;

	iAtk += pkAttacker->GetPoint(POINT_PARTY_ATTACKER_BONUS);
	iAtk = (DAM_LL)(iAtk * (100 + (pkAttacker->GetPoint(POINT_ATT_BONUS) + pkAttacker->GetPoint(POINT_MELEE_MAGIC_ATT_BONUS_PER))) / 100);

	iAtk = CalcAttBonus(pkAttacker, pkVictim, iAtk);

	DAM_LL iDef = 0;

	if (!bIgnoreDefense)
		iDef = (pkVictim->GetPoint(POINT_DEF_GRADE) * (100 + pkAttacker->GetPoint(POINT_DEF_BONUS)) / 100);

	if (pkAttacker->IsNPC())
		iAtk = (DAM_LL)(iAtk * pkAttacker->GetMobDamageMultiply());

	iDam = MAX(0, iAtk - iDef);

	DAM_LL iPureDam = iDam;

	iPureDam = (iPureDam * iPercent) / 100;
	return iPureDam;
	//return iDam;
}

void NormalAttackAffect(LPCHARACTER pkAttacker, LPCHARACTER pkVictim)
{
// #ifdef ENABLE_BOT_PLAYER
	// if (pkVictim->IsBotCharacter())
		// return;
// #endif
	
	if (pkAttacker->GetPoint(POINT_POISON_PCT) && !pkVictim->IsAffectFlag(AFF_POISON))
	{
		if (number(1, 100) <= pkAttacker->GetPoint(POINT_POISON_PCT))
			pkVictim->AttackedByPoison(pkAttacker);
	}
#ifdef ENABLE_WOLFMAN_CHARACTER
	if (pkAttacker->GetPoint(POINT_BLEEDING_PCT) && !pkVictim->IsAffectFlag(AFF_BLEEDING))
	{
		if (number(1, 100) <= pkAttacker->GetPoint(POINT_BLEEDING_PCT))
			pkVictim->AttackedByBleeding(pkAttacker);
	}
#endif

	int iStunDuration = 2;
#ifdef ENABLE_BOT_PLAYER
	if ((pkAttacker->IsPC() || pkAttacker->IsBotCharacter()) && !pkVictim->IsPC())
#else
	if (pkAttacker->IsPC() && !pkVictim->IsPC())
#endif
		iStunDuration = 4;

	AttackAffect(pkAttacker, pkVictim, POINT_STUN_PCT, IMMUNE_STUN, AFFECT_STUN, POINT_NONE, 0, AFF_STUN, iStunDuration, "STUN");
	AttackAffect(pkAttacker, pkVictim, POINT_SLOW_PCT, IMMUNE_SLOW, AFFECT_SLOW, POINT_MOV_SPEED, -30, AFF_SLOW, 20, "SLOW");
}

DAM_LL battle_hit(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, DAM_LL& iRetDam)
{
	DAM_LL iDam = CalcMeleeDamage(pkAttacker, pkVictim);

	if (iDam <= 0)
		return (BATTLE_DAMAGE);

	NormalAttackAffect(pkAttacker, pkVictim);
	LPITEM pkWeapon = pkAttacker->GetWear(WEAR_WEAPON);

	double AletBalyozGibi = 1.0;
	if (pkVictim->IsPC())
		AletBalyozGibi = 0.8;

	if (pkWeapon)
		switch (pkWeapon->GetSubType())
		{
		case WEAPON_SWORD:
			iDam = iDam * (100 - GetResistPoint(pkVictim, POINT_RESIST_SWORD, AletBalyozGibi)) / 100;
			break;

		case WEAPON_TWO_HANDED:
			iDam = iDam * (100 - GetResistPoint(pkVictim, POINT_RESIST_TWOHAND, AletBalyozGibi)) / 100;
			break;

		case WEAPON_DAGGER:
			iDam = iDam * (100 - GetResistPoint(pkVictim, POINT_RESIST_DAGGER, AletBalyozGibi)) / 100;
			break;

		case WEAPON_BELL:
			iDam = iDam * (100 - GetResistPoint(pkVictim, POINT_RESIST_BELL, AletBalyozGibi)) / 100;
			break;

		case WEAPON_FAN:
			iDam = iDam * (100 - GetResistPoint(pkVictim, POINT_RESIST_FAN, AletBalyozGibi)) / 100;
			break;

		case WEAPON_BOW:
			iDam = iDam * (100 - GetResistPoint(pkVictim, POINT_RESIST_BOW, AletBalyozGibi)) / 100;
			break;
#ifdef ENABLE_WOLFMAN_CHARACTER
		case WEAPON_CLAW:
			iDam = iDam * (100 - GetResistPoint(pkVictim, POINT_RESIST_CLAW, AletBalyozGibi)) / 100;
#if defined(ENABLE_WOLFMAN_CHARACTER) && defined(USE_ITEM_CLAW_AS_DAGGER)
			iDam = iDam * (100 - pkVictim->GetPoint(POINT_RESIST_DAGGER)) / 100;
#endif
			break;
#endif
		}

	DAM_DOUBLE attMul = pkAttacker->GetAttMul();
	DAM_DOUBLE tempIDam = iDam;
	iDam = attMul * tempIDam + 0.5f;

	iRetDam = iDam;

	if (pkVictim->Damage(pkAttacker, iDam, DAMAGE_TYPE_NORMAL))
		return (BATTLE_DEAD);

	return (BATTLE_DAMAGE);
}

#ifdef ENABLE_ATTACK_SPEED_LIMIT
DWORD GET_ATTACK_SPEED(LPCHARACTER ch)
{
	if (NULL == ch)
		return 1000;

	LPITEM item = ch->GetWear(WEAR_WEAPON);
	DWORD default_bonus = SPEEDHACK_LIMIT_BONUS;
	DWORD riding_bonus = 0;

	if (ch->IsRiding())
	{
		riding_bonus = 50;
	}

	DWORD ani_speed = ani_attack_speed(ch);
	DWORD real_speed = (ani_speed * 100) / (default_bonus + ch->GetPoint(POINT_ATT_SPEED) + riding_bonus);

	if (item && item->GetSubType() == WEAPON_DAGGER)
		real_speed /= 1.2;
#ifdef ENABLE_WOLFMAN_CHARACTER
	else if (item && item->GetSubType() == WEAPON_CLAW)
		real_speed /= 1.5;
#endif

	return real_speed;
}

void SET_ATTACK_TIME(LPCHARACTER ch, LPCHARACTER victim, DWORD current_time)
{
	if (NULL == ch || NULL == victim)
		return;

	if (!ch->IsPC())
		return;

	ch->m_kAttackLog.dwVID = victim->GetVID();
	ch->m_kAttackLog.dwTime = current_time;
}

void SET_ATTACKED_TIME(LPCHARACTER ch, LPCHARACTER victim, DWORD current_time)
{
	if (NULL == ch || NULL == victim)
		return;

	if (!ch->IsPC())
		return;

	victim->m_AttackedLog.dwPID		= ch->GetPlayerID();
	victim->m_AttackedLog.dwAttackedTime= current_time;
}

bool IS_SPEED_HACK(LPCHARACTER ch, LPCHARACTER victim, DWORD current_time)
{
#ifdef __FIX_PRO_DAMAGE__
	if (victim)
	{
		LPITEM pkWeapon = ch->GetWear (WEAR_WEAPON);
		if (!ch->IsPolymorphed() && ch->GetJob() == JOB_ASSASSIN && pkWeapon && pkWeapon->GetSubType() == WEAPON_BOW)
		{
			DWORD time_attack_bow = current_time - ch->GetBowTime();
			ch->SetBowTime (current_time);

			int limit_as = 1000;
			ch->IsRiding() ? limit_as = 800 : limit_as = 550;

			if (time_attack_bow < limit_as)
			{
				if (test_server)
				{
					ch->ChatPacket (CHAT_TYPE_INFO, "@@BAD AS BOW = %d | limit = %d", time_attack_bow, limit_as);
				}
				return true;
			}
		}
	}
#endif
	if (ch->m_kAttackLog.dwVID == victim->GetVID())
	{
		if (current_time - ch->m_kAttackLog.dwTime < GET_ATTACK_SPEED(ch))
		{
			INCREASE_SPEED_HACK_COUNT(ch);
			SET_ATTACK_TIME(ch, victim, current_time);
			SET_ATTACKED_TIME(ch, victim, current_time);
			return true;
		}
	}

	SET_ATTACK_TIME(ch, victim, current_time);

	if (victim->m_AttackedLog.dwPID == ch->GetPlayerID())
	{
		if (current_time - victim->m_AttackedLog.dwAttackedTime < GET_ATTACK_SPEED(ch))
		{
			INCREASE_SPEED_HACK_COUNT(ch);
			SET_ATTACKED_TIME(ch, victim, current_time);
			return true;
		}
	}

	SET_ATTACKED_TIME(ch, victim, current_time);
	return false;
}
#endif

