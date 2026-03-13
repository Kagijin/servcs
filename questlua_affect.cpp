#include "stdafx.h"
#include "config.h"
#include "questmanager.h"
#include "sectree_manager.h"
#include "char.h"
#include "affect.h"
#include "db.h"

namespace quest
{
	//
	// "affect" Lua functions
	//
	ALUA(affect_add)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			sys_err("invalid argument");
			return 0;
		}

		CQuestManager& q = CQuestManager::instance();

		BYTE applyOn = (BYTE)lua_tonumber(L, 1);

		LPCHARACTER ch = q.GetCurrentCharacterPtr();

		if (applyOn >= MAX_APPLY_NUM || applyOn < 1)
		{
			sys_err("apply is out of range : %d", applyOn);
			return 0;
		}

		if (ch->FindAffect(AFFECT_QUEST_START_IDX, applyOn))
			return 0;

		long value = (long)lua_tonumber(L, 2);
		long duration = (long)lua_tonumber(L, 3);

		ch->AddAffect(AFFECT_QUEST_START_IDX, aApplyInfo[applyOn].bPointType, value, 0, duration, 0, false);

		return 0;
	}

	ALUA(affect_remove)
	{
		CQuestManager& q = CQuestManager::instance();
		int iType;

		if (lua_isnumber(L, 1))
		{
			iType = (int)lua_tonumber(L, 1);

			if (iType == 0)
				iType = q.GetCurrentPC()->GetCurrentQuestIndex() + AFFECT_QUEST_START_IDX;
		}
		else
			iType = q.GetCurrentPC()->GetCurrentQuestIndex() + AFFECT_QUEST_START_IDX;

		q.GetCurrentCharacterPtr()->RemoveAffect(iType);

		return 0;
	}

	ALUA(affect_remove_bad)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		ch->RemoveBadAffect();
		return 0;
	}

	ALUA(affect_remove_good)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		ch->RemoveGoodAffect();
		return 0;
	}

	ALUA(affect_add_hair)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			sys_err("invalid argument");
			return 0;
		}

		CQuestManager& q = CQuestManager::instance();

		BYTE applyOn = (BYTE)lua_tonumber(L, 1);

		LPCHARACTER ch = q.GetCurrentCharacterPtr();

		if (applyOn >= MAX_APPLY_NUM || applyOn < 1)
		{
			sys_err("apply is out of range : %d", applyOn);
			return 0;
		}

		long value = (long)lua_tonumber(L, 2);
		long duration = (long)lua_tonumber(L, 3);

		ch->AddAffect(AFFECT_HAIR, aApplyInfo[applyOn].bPointType, value, 0, duration, 0, false);

		return 0;
	}

	ALUA(affect_remove_hair)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		CAffect* pkAff = ch->FindAffect(AFFECT_HAIR);

		if (pkAff != NULL)
		{
			lua_pushnumber(L, pkAff->lDuration);
			ch->RemoveAffect(pkAff);
		}
		else
		{
			lua_pushnumber(L, 0);
		}

		return 1;
	}

	// usage :	applyOn = affect.get_apply(AFFECT_TYPE)
	ALUA(affect_get_apply_on)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!lua_isnumber(L, 1))
		{
			sys_err("invalid argument");
			return 0;
		}

		DWORD affectType = lua_tonumber(L, 1);

		CAffect* pkAff = ch->FindAffect(affectType);

		if (pkAff != NULL)
			lua_pushnumber(L, pkAff->bApplyOn);
		else
			lua_pushnil(L);

		return 1;
	}

	ALUA(affect_add_collect)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			sys_err("invalid argument");
			return 0;
		}

		CQuestManager& q = CQuestManager::instance();

		BYTE applyOn = (BYTE)lua_tonumber(L, 1);

		LPCHARACTER ch = q.GetCurrentCharacterPtr();

		if (applyOn >= MAX_APPLY_NUM || applyOn < 1)
		{
			sys_err("apply is out of range : %d", applyOn);
			return 0;
		}

		long value = (long)lua_tonumber(L, 2);
		long duration = (long)lua_tonumber(L, 3);

		ch->AddAffect(AFFECT_COLLECT, aApplyInfo[applyOn].bPointType, value, 0, duration, 0, false);

		return 0;
	}

	ALUA(affect_add_collect_point)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			sys_err("invalid argument");
			return 0;
		}

		CQuestManager& q = CQuestManager::instance();

		BYTE point_type = (BYTE)lua_tonumber(L, 1);

		LPCHARACTER ch = q.GetCurrentCharacterPtr();

		if (point_type >= POINT_MAX_NUM || point_type < 1)
		{
			sys_err("point is out of range : %d", point_type);
			return 0;
		}

		long value = (long)lua_tonumber(L, 2);
		long duration = (long)lua_tonumber(L, 3);

		ch->AddAffect(AFFECT_COLLECT, point_type, value, 0, duration, 0, false);

		return 0;
	}

	ALUA(affect_remove_collect)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (ch != NULL)
		{
			BYTE bApply = (BYTE)lua_tonumber(L, 1);

			if (bApply >= MAX_APPLY_NUM) return 0;

			bApply = aApplyInfo[bApply].bPointType;
			long value = (long)lua_tonumber(L, 2);

			const std::list<CAffect*>& rList = ch->GetAffectContainer();
			const CAffect* pAffect = NULL;

			for (std::list<CAffect*>::const_iterator iter = rList.begin(); iter != rList.end(); ++iter)
			{
				pAffect = *iter;

				if (pAffect->dwType == AFFECT_COLLECT)
				{
					if (pAffect->bApplyOn == bApply && pAffect->lApplyValue == value)
					{
						break;
					}
				}

				pAffect = NULL;
			}

			if (pAffect != NULL)
			{
				ch->RemoveAffect(const_cast<CAffect*>(pAffect));
			}
		}

		return 0;
	}

	ALUA(affect_remove_all_collect)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (ch != NULL)
		{
			ch->RemoveAffect(AFFECT_COLLECT);
		}

		return 0;
	}

	ALUA(affect_add_affect)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();

		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4) || !ch)
			return 0;

		DWORD affectType = (DWORD)lua_tonumber(L, 1);
		BYTE pointType = (BYTE)lua_tonumber(L, 2);
		long pointValue = (long)lua_tonumber(L, 3);
		long duration = (long)lua_tonumber(L, 4);
	
		if (pointType >= POINT_MAX_NUM)
			return 0;

		ch->RemoveAffect(affectType);
		ch->AddAffect(affectType, pointType, pointValue, AFF_NONE, duration, 0, false, false);
		return 0;
	}

#ifdef ENABLE_BIOLOG_QUEST_SYSTEM
	/**30 Lv Biyolog**/
	ALUA(affect_add_30)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if ( ch != NULL )
		{
			ch->RemoveAffect(AFFECT_BIO);
			ch->AddAffect(AFFECT_BIO, aApplyInfo[8].bPointType, 10, 0, 60*60*24*365*60, 0, false);
		}

		return 0;
	}

	/**40 Lv Biyolog**/
	ALUA(affect_add_40)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if ( ch != NULL )
		{
			ch->RemoveAffect(AFFECT_BIO);
			ch->AddAffect(AFFECT_BIO, aApplyInfo[7].bPointType, 5, 0, 60*60*24*365*60, 0, false);
		}

		return 0;
	}

	/**50 Lv Biyolog**/
	ALUA(affect_add_50)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if ( ch != NULL )
		{
			ch->RemoveAffect(AFFECT_BIO);
			ch->AddAffect(AFFECT_BIO, aApplyInfo[54].bPointType, 60, 0, 60*60*24*365*60, 0, false);
		}

		return 0;
	}

	/**60 Lv Biyolog**/
	ALUA(affect_add_60)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if ( ch != NULL )
		{
			ch->RemoveAffect(AFFECT_BIO);
			ch->AddAffect(AFFECT_BIO, aApplyInfo[53].bPointType, 50, 0, 60*60*24*365*60, 0, false);
		}

		return 0;
	}
	/**70 Lv Biyolog**/
	ALUA(affect_add_70)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if ( ch != NULL )
		{
			ch->RemoveAffect(AFFECT_BIO);
			ch->AddAffect(AFFECT_BIO, aApplyInfo[8].bPointType, 11, 0, 60*60*24*365*60, 0, false);
			ch->AddAffect(AFFECT_BIO, aApplyInfo[7].bPointType, 10, 0, 60*60*24*365*60, 0, false);
		}

		return 0;
	}

	/**80 Lv Biyolog**/
	ALUA(affect_add_80)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if ( ch != NULL )
		{
			ch->RemoveAffect(AFFECT_BIO);
			ch->AddAffect(AFFECT_BIO, aApplyInfo[7].bPointType,  6, 0, 60*60*24*365*60, 0, false);
			ch->AddAffect(AFFECT_BIO, aApplyInfo[64].bPointType, 10, 0, 60*60*24*365*60, 0, false);
		}

		return 0;
	}

	/**85 Lv Biyolog**/
	ALUA(affect_add_85)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if ( ch != NULL )
		{
			ch->RemoveAffect(AFFECT_BIO);
			ch->AddAffect(AFFECT_BIO, aApplyInfo[76].bPointType, 10, 0, 60*60*24*365*60, 0, false);
			ch->AddAffect(AFFECT_BIO, aApplyInfo[77].bPointType, 10, 0, 60*60*24*365*60, 0, false);
			ch->AddAffect(AFFECT_BIO, aApplyInfo[78].bPointType, 10, 0, 60*60*24*365*60, 0, false);
			ch->AddAffect(AFFECT_BIO, aApplyInfo[79].bPointType, 10, 0, 60*60*24*365*60, 0, false);
		}

		return 0;
	}

	/**90 Lv Biyolog**/
	ALUA(affect_add_90)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if ( ch != NULL )
		{
			ch->RemoveAffect(AFFECT_BIO);
			ch->AddAffect(AFFECT_BIO, aApplyInfo[59].bPointType, 8, 0, 60*60*24*365*60, 0, false);
			ch->AddAffect(AFFECT_BIO, aApplyInfo[60].bPointType, 8, 0, 60*60*24*365*60, 0, false);
			ch->AddAffect(AFFECT_BIO, aApplyInfo[61].bPointType, 8, 0, 60*60*24*365*60, 0, false);
			ch->AddAffect(AFFECT_BIO, aApplyInfo[62].bPointType, 8, 0, 60*60*24*365*60, 0, false);
		}

		return 0;
	}

	/**92 Lv Biyolog**/
	ALUA(affect_add_92_1)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if ( ch != NULL )
		{
			ch->RemoveAffect(AFFECT_BIO_92);
			ch->AddAffect(AFFECT_BIO_92, aApplyInfo[1].bPointType, 2000, 0, 60*60*24*365*60, 0, false);
		}

		return 0;
	}

	ALUA(affect_add_92_2)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if ( ch != NULL )
		{
			ch->RemoveAffect(AFFECT_BIO_92);
			ch->AddAffect(AFFECT_BIO_92, aApplyInfo[54].bPointType, 120, 0, 60*60*24*365*60, 0, false);
		}

		return 0;
	}

	ALUA(affect_add_92_3)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if ( ch != NULL )
		{
			ch->RemoveAffect(AFFECT_BIO_92);
			ch->AddAffect(AFFECT_BIO_92, aApplyInfo[53].bPointType, 50, 0, 60*60*24*365*60, 0, false);
		}

		return 0;
	}

	/**94 Lv Biyolog**/
	ALUA(affect_add_94_1)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if ( ch != NULL )
		{
			ch->RemoveAffect(AFFECT_BIO_94);
			ch->AddAffect(AFFECT_BIO_94, aApplyInfo[1].bPointType, 2100, 0, 60*60*24*365*60, 0, false);
		}

		return 0;
	}

	ALUA(affect_add_94_2)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if ( ch != NULL )
		{
			ch->RemoveAffect(AFFECT_BIO_94);
			ch->AddAffect(AFFECT_BIO_94, aApplyInfo[54].bPointType, 140, 0, 60*60*24*365*60, 0, false);
		}

		return 0;
	}

	ALUA(affect_add_94_3)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if ( ch != NULL )
		{
			ch->RemoveAffect(AFFECT_BIO_94);
			ch->AddAffect(AFFECT_BIO_94, aApplyInfo[53].bPointType, 60, 0, 60*60*24*365*60, 0, false);
		}

		return 0;
	}
	/**94 Lv Biyolog**/
#endif
	void RegisterAffectFunctionTable()
	{
		luaL_reg affect_functions[] =
		{
			{ "add",		affect_add		},
			{ "remove",		affect_remove		},
			{ "remove_bad",	affect_remove_bad	},
			{ "remove_good",	affect_remove_good	},
			{ "add_hair",		affect_add_hair		},
			{ "remove_hair",	affect_remove_hair		},
			{ "add_collect",		affect_add_collect		},
			{ "add_collect_point",		affect_add_collect_point		},
			{ "remove_collect",		affect_remove_collect	},
			{ "remove_all_collect",	affect_remove_all_collect	},
			{ "get_apply_on",	affect_get_apply_on },
#ifdef ENABLE_BIOLOG_QUEST_SYSTEM
			{ "add_30",		affect_add_30	},
			{ "add_40",		affect_add_40	},
			{ "add_50",		affect_add_50	},
			{ "add_60",		affect_add_60	},
			{ "add_70",		affect_add_70	},
			{ "add_80",		affect_add_80	},
			{ "add_85",		affect_add_85	},
			{ "add_90",		affect_add_90	},
			{ "add_92_1",	affect_add_92_1	},
			{ "add_92_2",	affect_add_92_2	},
			{ "add_92_3",	affect_add_92_3	},
			{ "add_94_1",	affect_add_94_1	},
			{ "add_94_2",	affect_add_94_2	},
			{ "add_94_3",	affect_add_94_3	},
#endif
			{ "add_affect",	affect_add_affect },
			{ NULL,		NULL			}
		};

		CQuestManager::instance().AddLuaFunctionTable("affect", affect_functions);
	}
};