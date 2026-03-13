#include "stdafx.h"

#include <stack>

#include "utils.h"
#include "config.h"
#include "char.h"
#include "char_manager.h"
#include "item_manager.h"
#include "desc.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "packet.h"
#include "affect.h"
#include "skill.h"
#include "start_position.h"
#include "mob_manager.h"
#include "db.h"

#include "vector.h"
#include "buffer_manager.h"
#include "questmanager.h"
#include "fishing.h"
#include "party.h"
#include "dungeon.h"
#include "refine.h"
#include "unique_item.h"
#include "war_map.h"
#include "marriage.h"
#include "polymorph.h"
#include "blend_item.h"
#include "BattleArena.h"
#include "arena.h"
#include "dev_log.h"
#include "safebox.h"
#include "shop.h"
#ifdef ENABLE_NEWSTUFF
#include "pvp.h"
#endif
#include "../../common/VnumHelper.h"
#include "DragonSoul.h"
#include "buff_on_attributes.h"
#include "belt_inventory_helper.h"
#include "../../common/Controls.h"
#ifdef ENABLE_SWITCHBOT
#include "new_switchbot.h"
#endif
#ifdef ENABLE_NEW_PET_SYSTEM
#include "New_PetSystem.h"
#endif

#ifdef ENABLE_EXTENDED_BATTLE_PASS
#include "battlepass_manager.h"
#endif
#ifdef ENABLE_BUFFI_SYSTEM
#include "BuffiSystem.h"
#endif
#ifdef __KINGDOMS_WAR__
	#include "new_events.h"
#endif
#ifdef TOURNAMENT_PVP_SYSTEM
	#include "tournament.h"
#endif
const int ITEM_BROKEN_METIN_VNUM = 28960;
#define ENABLE_EFFECT_EXTRAPOT
#define ENABLE_BOOKS_STACKFIX
#define ENABLE_ITEM_RARE_ATTR_LEVEL_PCT

const BYTE g_aBuffOnAttrPoints[] = { POINT_ENERGY, POINT_COSTUME_ATTR_BONUS };

struct FFindStone
{
	std::map<DWORD, LPCHARACTER> m_mapStone;

	void operator()(LPENTITY pEnt)
	{
		if (pEnt->IsType(ENTITY_CHARACTER) == true)
		{
			LPCHARACTER pChar = (LPCHARACTER)pEnt;

			if (pChar->IsStone() == true)
			{
				m_mapStone[(DWORD)pChar->GetVID()] = pChar;
			}
		}
	}
};

//@Lightwork#140_START
static bool BLOCKED_ATTR_LIST(int vnum)
{
	switch (vnum)
	{
	case 50201:
	case 50202:
	case 12000:
	case 12001:
	case 12002:
	case 12003:
	case 11911:
	case 11912:
	case 11913:
	case 11914:
		return true;
	}

	return false;
}
//@Lightwork#140_END

static bool IS_SUMMON_ITEM (int vnum)
{
	switch (vnum)
	{
		case 22000:
		case 22010:
		case 22011:
		case 22020:
		case 70302:
			// case ITEM_MARRIAGE_RING:
			return true;
	}

	return false;
}
static bool IS_MONKEY_DUNGEON(int map_index)
{
	switch (map_index)
	{
		case 5:
		case 25:
		case 45:
		case 107:
		case 108:
		case 109:
			return true;;
	}

	return false;
}

#ifdef ENABLE_BLOCK_ITEMS_ON_WAR_MAP
static bool IS_ENABLE_ITEM_BLOCK (int vnum)
{
	switch (vnum)
	{
		case 70020:
		case 71020:
		case 71018:
		case 71108:
		case 90034:
		case 50804:
		case 50816:
			return true;
	}

	return false;
}
//ÏÞÖÆÕ½³¡µØÍ¼
bool IS_ENABLE_MAP_BLOCK(int map_index)
{
	switch (map_index)
	{
		case 26:
		case 101:
		case 110:
		case 111:
		case 103:
		case 202:
		case 211:
		case 241:
			return false;
	}

	return true;
}
//ÏÞÖÆPVPµØÍ¼
static bool IS_ENABLE_ITEM_BLOCK_26 (int vnum)
{
	switch (vnum)
	{
		case 70020:
		case 71020:
		case 71018:
		case 71108:
		case 90034:
		case 50804:
		case 50816:
		case 27001:
		case 27002:
		case 27003:
		case 72723:
		case 72724:
		case 72725:
		case 72726:
			return true;
	}

	return false;
}
bool IS_ENABLE_MAP_BLOCK_26(int map_index)
{
	switch (map_index)
	{
		case 26:
			return false;
	}

	return true;
}
#endif
bool IS_SUMMONABLE_ZONE(int map_index)
{

	if (IS_MONKEY_DUNGEON(map_index))
		return false;
	
	// if (IS_CASTLE_MAP(map_index))
		// return false;

	switch (map_index)
	{
	case 26:
	case 67:
	case 68:
	case 69:
	case 71:
	case 72:
	case 73:
	case 74:
	case 75:
	case 76:
	case 77:
	case 78:
	case 80:
	case 82:
	case 83:
	case 84:
	case 110:
	case 112:
	case 113:
	case 115:
	case 181:
	case 182:
	case 183:
	case 195:
	case 196:
	case 197:
	case 198:
	case 200:
	case 202:
	case 300:
	case 301:
	case 302:
	case 303:
	case 304:
	case 305:
	case 306:
	case 307:
	case 308:
	case 311:
	case 312:
	case 313:
	case 314:
	case 321:
	case 322:
	case 323:
	case 324:
		return false;
	}

	if (CBattleArena::IsBattleArenaMap(map_index)) return false;

	if (map_index > 10000) return false;

	return true;
}

#ifndef ENABLE_INVENTORY_REWORK
static bool FN_check_item_socket(LPITEM item)
{
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
	{
		if (item->GetSocket(i) != item->GetProto()->alSockets[i])
			return false;
	}

	return true;
}
#endif

static void FN_copy_item_socket(LPITEM dest, LPITEM src)
{
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
	{
		dest->SetSocket(i, src->GetSocket(i));
	}
}
static bool FN_check_item_sex(LPCHARACTER ch, LPITEM item)
{
	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_MALE))
	{
		if (SEX_MALE == GET_SEX(ch))
			return false;
	}

	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_FEMALE))
	{
		if (SEX_FEMALE == GET_SEX(ch))
			return false;
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////
// ITEM HANDLING
/////////////////////////////////////////////////////////////////////////////
bool CHARACTER::CanHandleItem(bool bSkipCheckRefine, bool bSkipObserver)
{
	if (!bSkipObserver)
		if (m_bIsObserver)
			return false;

	if (GetMyShop())
		return false;

	if (!bSkipCheckRefine)
		if (m_bUnderRefine)
			return false;

	if (IsCubeOpen() || NULL != DragonSoul_RefineWindow_GetOpener())
		return false;

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	if ((m_bAcceCombination) || (m_bAcceAbsorption))
		return false;
#endif
#ifdef ENABLE_AURA_SYSTEM
	if ((m_bAuraRefine) || (m_bAuraAbsorption))
		return false;
#endif
#ifdef ENABLE_6TH_7TH_ATTR
	if (Is67AttrOpen())
		return false;
#endif
	// if (IsWarping())
		// return false;

	return true;
}

LPITEM CHARACTER::GetInventoryItem(WORD wCell) const
{
	return GetItem(TItemPos(INVENTORY, wCell));
}

#ifdef ENABLE_SPECIAL_STORAGE
LPITEM CHARACTER::GetUpgradeInventoryItem(WORD wCell) const
{
	return GetItem(TItemPos(UPGRADE_INVENTORY, wCell));
}

LPITEM CHARACTER::GetBookInventoryItem(WORD wCell) const
{
	return GetItem(TItemPos(BOOK_INVENTORY, wCell));
}

LPITEM CHARACTER::GetStoneInventoryItem(WORD wCell) const
{
	return GetItem(TItemPos(STONE_INVENTORY, wCell));
}

LPITEM CHARACTER::GetChestInventoryItem(WORD wCell) const
{
	return GetItem(TItemPos(CHEST_INVENTORY, wCell));
}
#endif

#ifdef ENABLE_6TH_7TH_ATTR
LPITEM CHARACTER::GetAttrInventoryItem() const
{
	return m_pointsInstant.pAttrItems;
}
#endif

LPITEM CHARACTER::GetItem(TItemPos Cell) const
{
	if (!IsValidItemPosition(Cell))
		return nullptr;
	WORD wCell = Cell.cell;
	BYTE window_type = Cell.window_type;
	switch (window_type)
	{
	case INVENTORY:
	case EQUIPMENT:
		if (wCell >= INVENTORY_AND_EQUIP_SLOT_MAX)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid item cell %d", wCell);
			return nullptr;
		}
		return m_pointsInstant.pItems[wCell];
	case DRAGON_SOUL_INVENTORY:
		if (wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid DS item cell %d", wCell);
			return nullptr;
		}
		return m_pointsInstant.pDSItems[wCell];
#ifdef ENABLE_SPECIAL_STORAGE
	case UPGRADE_INVENTORY:
		if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid SSU item cell %d", wCell);
			return nullptr;
		}
		return m_pointsInstant.pSSUItems[wCell];

	case BOOK_INVENTORY:
		if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid SSB item cell %d", wCell);
			return nullptr;
		}
		return m_pointsInstant.pSSBItems[wCell];

	case STONE_INVENTORY:
		if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid SSS item cell %d", wCell);
			return nullptr;
		}
		return m_pointsInstant.pSSSItems[wCell];

	case CHEST_INVENTORY:
		if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid SSC item cell %d", wCell);
			return nullptr;
		}
		return m_pointsInstant.pSSCItems[wCell];
#endif
#ifdef ENABLE_SWITCHBOT
	case SWITCHBOT:
		if (wCell >= SWITCHBOT_SLOT_COUNT)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid switchbot item cell %d", wCell);
			return nullptr;
		}
		return m_pointsInstant.pSwitchbotItems[wCell];
#endif

#ifdef ENABLE_BUFFI_SYSTEM
	case BUFFI_INVENTORY:
		if (wCell >= BUFFI_MAX_SLOT)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid buffi item cell %d", wCell);
			return nullptr;
		}
		return m_pointsInstant.pBuffiItems[wCell];
#endif

	default:
		return nullptr;
	}
	return nullptr;
}

void CHARACTER::SetItem(TItemPos Cell, LPITEM pItem
#ifdef ENABLE_HIGHLIGHT_SYSTEM
	, bool isHighLight
#endif
)
{
	WORD wCell = Cell.cell;
	BYTE window_type = Cell.window_type;
	if ((unsigned long)((CItem*)pItem) == 0xff || (unsigned long)((CItem*)pItem) == 0xffffffff)
	{
		sys_err("!!! FATAL ERROR !!! item == 0xff (char: %s cell: %u)", GetName(), wCell);
		core_dump();//ÒÆ³ý´ËÏî¿ÉÒÔ·ÀÖ¹·þÎñÆ÷±ÀÀ£
		return;
	}

	if (pItem && pItem->GetOwner())
	{
		assert(!"GetOwner exist");
		return;
	}

	switch (window_type)
	{
	case INVENTORY:
	case EQUIPMENT:
	{
		if (wCell >= INVENTORY_AND_EQUIP_SLOT_MAX)
		{
			sys_err("CHARACTER::SetItem: invalid item cell %d", wCell);
			return;
		}

		LPITEM pOld = m_pointsInstant.pItems[wCell];

		if (pOld)
		{
			if (wCell < INVENTORY_MAX_NUM)
			{
				for (int i = 0; i < pOld->GetSize(); ++i)
				{
					int p = wCell + (i * 5);

					if (p >= INVENTORY_MAX_NUM)
						continue;

					if (m_pointsInstant.pItems[p] && m_pointsInstant.pItems[p] != pOld)
						continue;

					m_pointsInstant.bItemGrid[p] = 0;
				}
			}
			else
				m_pointsInstant.bItemGrid[wCell] = 0;
		}

		if (pItem)
		{
			if (wCell < INVENTORY_MAX_NUM)
			{
				for (int i = 0; i < pItem->GetSize(); ++i)
				{
					int p = wCell + (i * 5);

					if (p >= INVENTORY_MAX_NUM)
						continue;

					m_pointsInstant.bItemGrid[p] = wCell + 1;
				}
			}
			else
				m_pointsInstant.bItemGrid[wCell] = wCell + 1;
		}

		m_pointsInstant.pItems[wCell] = pItem;
	}
	break;

	case DRAGON_SOUL_INVENTORY:
	{
		LPITEM pOld = m_pointsInstant.pDSItems[wCell];

		if (pOld)
		{
			if (wCell < DRAGON_SOUL_INVENTORY_MAX_NUM)
			{
				for (int i = 0; i < pOld->GetSize(); ++i)
				{
					int p = wCell + (i * DRAGON_SOUL_BOX_COLUMN_NUM);

					if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
						continue;

					if (m_pointsInstant.pDSItems[p] && m_pointsInstant.pDSItems[p] != pOld)
						continue;

					m_pointsInstant.wDSItemGrid[p] = 0;
				}
			}
			else
				m_pointsInstant.wDSItemGrid[wCell] = 0;
		}

		if (pItem)
		{
			if (wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
			{
				sys_err("CHARACTER::SetItem: invalid DS item cell %d", wCell);
				return;
			}

			if (wCell < DRAGON_SOUL_INVENTORY_MAX_NUM)
			{
				for (int i = 0; i < pItem->GetSize(); ++i)
				{
					int p = wCell + (i * DRAGON_SOUL_BOX_COLUMN_NUM);

					if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
						continue;

					m_pointsInstant.wDSItemGrid[p] = wCell + 1;
				}
			}
			else
				m_pointsInstant.wDSItemGrid[wCell] = wCell + 1;
		}

		m_pointsInstant.pDSItems[wCell] = pItem;
	}
	break;
#ifdef ENABLE_SPECIAL_STORAGE
	case UPGRADE_INVENTORY:
	{
		LPITEM pOld = m_pointsInstant.pSSUItems[wCell];
		if (pOld)
		{
			if (wCell < SPECIAL_INVENTORY_MAX_NUM)
			{
				for (int i = 0; i < pOld->GetSize(); ++i)
				{
					int p = wCell + (i * 5);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						continue;

					if (m_pointsInstant.pSSUItems[p] && m_pointsInstant.pSSUItems[p] != pOld)
						continue;

					m_pointsInstant.wSSUItemGrid[p] = 0;
				}
			}
			else
				m_pointsInstant.wSSUItemGrid[wCell] = 0;
		}

		if (pItem)
		{
			if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
			{
				sys_err("CHARACTER::SetItem: invalid SSU item cell %d", wCell);
				return;
			}

			if (wCell < SPECIAL_INVENTORY_MAX_NUM)
			{
				for (int i = 0; i < pItem->GetSize(); ++i)
				{
					int p = wCell + (i * 5);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						continue;

					m_pointsInstant.wSSUItemGrid[p] = wCell + 1;
				}
			}
			else
				m_pointsInstant.wSSUItemGrid[wCell] = wCell + 1;
		}

		m_pointsInstant.pSSUItems[wCell] = pItem;
	}
	break;

	case BOOK_INVENTORY:
	{
		LPITEM pOld = m_pointsInstant.pSSBItems[wCell];
		if (pOld)
		{
			if (wCell < SPECIAL_INVENTORY_MAX_NUM)
			{
				for (int i = 0; i < pOld->GetSize(); ++i)
				{
					int p = wCell + (i * 5);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						continue;

					if (m_pointsInstant.pSSBItems[p] && m_pointsInstant.pSSBItems[p] != pOld)
						continue;

					m_pointsInstant.wSSBItemGrid[p] = 0;
				}
			}
			else
				m_pointsInstant.wSSBItemGrid[wCell] = 0;
		}

		if (pItem)
		{
			if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
			{
				sys_err("CHARACTER::SetItem: invalid SSB item cell %d", wCell);
				return;
			}

			if (wCell < SPECIAL_INVENTORY_MAX_NUM)
			{
				for (int i = 0; i < pItem->GetSize(); ++i)
				{
					int p = wCell + (i * 5);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						continue;

					m_pointsInstant.wSSBItemGrid[p] = wCell + 1;
				}
			}
			else
				m_pointsInstant.wSSBItemGrid[wCell] = wCell + 1;
		}

		m_pointsInstant.pSSBItems[wCell] = pItem;
	}
	break;

	case STONE_INVENTORY:
	{
		LPITEM pOld = m_pointsInstant.pSSSItems[wCell];
		if (pOld)
		{
			if (wCell < SPECIAL_INVENTORY_MAX_NUM)
			{
				for (int i = 0; i < pOld->GetSize(); ++i)
				{
					int p = wCell + (i * 5);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						continue;

					if (m_pointsInstant.pSSSItems[p] && m_pointsInstant.pSSSItems[p] != pOld)
						continue;

					m_pointsInstant.wSSSItemGrid[p] = 0;
				}
			}
			else
				m_pointsInstant.wSSSItemGrid[wCell] = 0;
		}

		if (pItem)
		{
			if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
			{
				sys_err("CHARACTER::SetItem: invalid SSS item cell %d", wCell);
				return;
			}

			if (wCell < SPECIAL_INVENTORY_MAX_NUM)
			{
				for (int i = 0; i < pItem->GetSize(); ++i)
				{
					int p = wCell + (i * 5);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						continue;

					m_pointsInstant.wSSSItemGrid[p] = wCell + 1;
				}
			}
			else
				m_pointsInstant.wSSSItemGrid[wCell] = wCell + 1;
		}

		m_pointsInstant.pSSSItems[wCell] = pItem;
	}
	break;

	case CHEST_INVENTORY:
	{
		LPITEM pOld = m_pointsInstant.pSSCItems[wCell];
		if (pOld)
		{
			if (wCell < SPECIAL_INVENTORY_MAX_NUM)
			{
				for (int i = 0; i < pOld->GetSize(); ++i)
				{
					int p = wCell + (i * 5);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						continue;

					if (m_pointsInstant.pSSCItems[p] && m_pointsInstant.pSSCItems[p] != pOld)
						continue;

					m_pointsInstant.wSSCItemGrid[p] = 0;
				}
			}
			else
				m_pointsInstant.wSSCItemGrid[wCell] = 0;
		}

		if (pItem)
		{
			if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
			{
				sys_err("CHARACTER::SetItem: invalid SSC item cell %d", wCell);
				return;
			}

			if (wCell < SPECIAL_INVENTORY_MAX_NUM)
			{
				for (int i = 0; i < pItem->GetSize(); ++i)
				{
					int p = wCell + (i * 5);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						continue;

					m_pointsInstant.wSSCItemGrid[p] = wCell + 1;
				}
			}
			else
				m_pointsInstant.wSSCItemGrid[wCell] = wCell + 1;
		}

		m_pointsInstant.pSSCItems[wCell] = pItem;
	}
	break;
#endif

#ifdef ENABLE_SWITCHBOT
	case SWITCHBOT:
	{
		LPITEM pOld = m_pointsInstant.pSwitchbotItems[wCell];
		if (pItem && pOld)
		{
			return;
		}

		if (wCell >= SWITCHBOT_SLOT_COUNT)
		{
			sys_err("CHARACTER::SetItem: invalid switchbot item cell %d", wCell);
			return;
		}

		if (pItem)
		{
			CSwitchbotManager::Instance().RegisterItem(GetPlayerID(), pItem->GetID(), wCell);
		}
		else
		{
			CSwitchbotManager::Instance().UnregisterItem(GetPlayerID(), wCell);
		}

		m_pointsInstant.pSwitchbotItems[wCell] = pItem;
	}
	break;
#endif
#ifdef ENABLE_6TH_7TH_ATTR
	case ATTR_INVENTORY:
	{
		if (wCell >= 1)
		{
			sys_err("CHARACTER::SetItem: invalid ATTR_INVENTORY item cell %d", wCell);
			return;
		}

		m_pointsInstant.pAttrItems = pItem;
	}
	break;
#endif

#ifdef ENABLE_BUFFI_SYSTEM
	case BUFFI_INVENTORY:
	{
		if (wCell >= BUFFI_MAX_SLOT) 
		{
			sys_err("CHARACTER::SetItem: invalid BUFFI_MAX_SLOT item cell %d", wCell);
			return;
		}

		LPITEM pOld = m_pointsInstant.pBuffiItems[wCell];

		if (pOld && pItem) 
		{
			return;
		}

		m_pointsInstant.pBuffiItems[wCell] = pItem;
		if (GetBuffiSystem())
			GetBuffiSystem()->UpdateBuffiItem();
	}
	break;
#endif

	default:
		sys_err("Invalid Inventory type %d", window_type);
		return;
	}

	if (GetDesc())
	{
		if (pItem)
		{
			TPacketGCItemSet pack;
			pack.header = HEADER_GC_ITEM_SET;
			pack.Cell = Cell;
			pack.count = pItem->GetCount();
			pack.vnum = pItem->GetVnum();
			pack.flags = pItem->GetFlag();
			pack.anti_flags = pItem->GetAntiFlag();
#ifdef ENABLE_HIGHLIGHT_SYSTEM
			if (isHighLight)
				pack.highlight = true;
			else
				pack.highlight = (Cell.window_type == DRAGON_SOUL_INVENTORY);
#else
			pack.highlight = (Cell.window_type == DRAGON_SOUL_INVENTORY);
#endif

			thecore_memcpy(pack.alSockets, pItem->GetSockets(), sizeof(pack.alSockets));
			thecore_memcpy(pack.aAttr, pItem->GetAttributes(), sizeof(pack.aAttr));

			GetDesc()->Packet(&pack, sizeof(TPacketGCItemSet));
		}
		else
		{
			TPacketGCItemDelDeprecated pack;
			pack.header = HEADER_GC_ITEM_DEL;
			pack.Cell = Cell;
			pack.count = 0;
			pack.vnum = 0;
			memset(pack.alSockets, 0, sizeof(pack.alSockets));
			memset(pack.aAttr, 0, sizeof(pack.aAttr));

			GetDesc()->Packet(&pack, sizeof(TPacketGCItemDelDeprecated));
		}
	}

	if (pItem)
	{
		pItem->SetCell(this, wCell);
		switch (window_type)
		{
		case INVENTORY:
		case EQUIPMENT:
			if ((wCell < INVENTORY_MAX_NUM) || (BELT_INVENTORY_SLOT_START <= wCell && BELT_INVENTORY_SLOT_END > wCell))
				pItem->SetWindow(INVENTORY);
			else
				pItem->SetWindow(EQUIPMENT);
			break;
		case DRAGON_SOUL_INVENTORY:
			pItem->SetWindow(DRAGON_SOUL_INVENTORY);
			break;
#ifdef ENABLE_SPECIAL_STORAGE
		case UPGRADE_INVENTORY:
			pItem->SetWindow(UPGRADE_INVENTORY);
			break;
		case BOOK_INVENTORY:
			pItem->SetWindow(BOOK_INVENTORY);
			break;
		case STONE_INVENTORY:
			pItem->SetWindow(STONE_INVENTORY);
			break;
		case CHEST_INVENTORY:
			pItem->SetWindow(CHEST_INVENTORY);
			break;
#endif
#ifdef ENABLE_SWITCHBOT
		case SWITCHBOT:
			pItem->SetWindow(SWITCHBOT);
			break;
#endif
#ifdef ENABLE_6TH_7TH_ATTR
		case ATTR_INVENTORY:
			pItem->SetWindow(ATTR_INVENTORY);
			break;
#endif
#ifdef ENABLE_BUFFI_SYSTEM
		case BUFFI_INVENTORY:
			pItem->SetWindow(BUFFI_INVENTORY);
			break;
#endif
		}
	}
}

LPITEM CHARACTER::GetWear(CELL_UINT bCell) const
{
	if (bCell >= WEAR_MAX_NUM + DRAGON_SOUL_DECK_MAX_NUM * DS_SLOT_MAX)
	{
		sys_err("CHARACTER::GetWear: invalid wear cell %d", bCell);
		return NULL;
	}

	return m_pointsInstant.pItems[INVENTORY_MAX_NUM + bCell];
}

void CHARACTER::SetWear(CELL_UINT bCell, LPITEM item)
{
	if (bCell >= WEAR_MAX_NUM + DRAGON_SOUL_DECK_MAX_NUM * DS_SLOT_MAX)
	{
		sys_err("CHARACTER::SetItem: invalid item cell %d", bCell);
		return;
	}

#ifdef ENABLE_HIGHLIGHT_SYSTEM
	SetItem(TItemPos(INVENTORY, INVENTORY_MAX_NUM + bCell), item, false);
#else
	SetItem(TItemPos(INVENTORY, INVENTORY_MAX_NUM + bCell), item);
#endif

	// if (!item && bCell == WEAR_WEAPON)
	// {
		// if (IsAffectFlag(AFF_GWIGUM))
			// RemoveAffect(SKILL_GWIGEOM);

		// if (IsAffectFlag(AFF_GEOMGYEONG))
			// RemoveAffect(SKILL_GEOMKYUNG);
	// }
}

void CHARACTER::ClearItem()
{
	int		i;
	LPITEM	item;

	for (i = 0; i < INVENTORY_AND_EQUIP_SLOT_MAX; ++i)
	{
		if ((item = GetInventoryItem(i)))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);

			SyncQuickslot(QUICKSLOT_TYPE_ITEM, i, UINT16_MAX);
		}
	}
	for (i = 0; i < DRAGON_SOUL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(DRAGON_SOUL_INVENTORY, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}
#ifdef ENABLE_SPECIAL_STORAGE
	for (i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(UPGRADE_INVENTORY, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}
	for (i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(BOOK_INVENTORY, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}
	for (i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(STONE_INVENTORY, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}
	for (i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(CHEST_INVENTORY, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}
#endif
#ifdef ENABLE_SWITCHBOT
	for (i = 0; i < SWITCHBOT_SLOT_COUNT; ++i)
	{
		if ((item = GetItem(TItemPos(SWITCHBOT, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}
#endif

#ifdef ENABLE_BUFFI_SYSTEM
	for (i = 0; i < BUFFI_MAX_SLOT; ++i)
	{
		if ((item = GetItem(TItemPos(BUFFI_INVENTORY, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}
#endif

#ifdef ENABLE_6TH_7TH_ATTR
	item = GetAttrInventoryItem();
	if (item)
	{
		item->SetSkipSave(true);
		ITEM_MANAGER::Instance().FlushDelayedSave(item);

		item->RemoveFromCharacter();
		M2_DESTROY_ITEM(item);
	}
#endif
}

bool CHARACTER::IsEmptyItemGrid(TItemPos Cell, BYTE bSize, int iExceptionCell) const
{
	switch (Cell.window_type)
	{
	case INVENTORY:
	{
		WORD bCell = Cell.cell;

		++iExceptionCell;

		if (Cell.IsBeltInventoryPosition())
		{
			LPITEM beltItem = GetWear(WEAR_BELT);

			if (NULL == beltItem)
				return false;

			if (false == CBeltInventoryHelper::IsAvailableCell(bCell - BELT_INVENTORY_SLOT_START, beltItem->GetValue(0)))
				return false;

			if (m_pointsInstant.bItemGrid[bCell])
			{
				if (m_pointsInstant.bItemGrid[bCell] == iExceptionCell)
					return true;

				return false;
			}

			if (bSize == 1)
				return true;
		}
		else if (bCell >= INVENTORY_MAX_NUM)
			return false;

		if (m_pointsInstant.bItemGrid[bCell])
		{
			if (m_pointsInstant.bItemGrid[bCell] == iExceptionCell)
			{
				if (bSize == 1)
					return true;

				int j = 1;
				BYTE bPage = bCell / (INVENTORY_MAX_NUM / INVENTORY_PAGE_COUNT);

				do
				{
					BYTE p = bCell + (5 * j);

					if (p >= INVENTORY_MAX_NUM)
						return false;

					if (p / (INVENTORY_MAX_NUM / INVENTORY_PAGE_COUNT) != bPage)
						return false;

					if (m_pointsInstant.bItemGrid[p])
						if (m_pointsInstant.bItemGrid[p] != iExceptionCell)
							return false;
				} while (++j < bSize);

				return true;
			}
			else
				return false;
		}

		if (1 == bSize)
			return true;
		else
		{
			int j = 1;
			BYTE bPage = bCell / (INVENTORY_MAX_NUM / INVENTORY_PAGE_COUNT);

			do
			{
				CELL_UINT p = bCell + (5 * j);

				if (p >= INVENTORY_MAX_NUM)
					return false;

				if (p / (INVENTORY_MAX_NUM / INVENTORY_PAGE_COUNT) != bPage)
					return false;

				if (m_pointsInstant.bItemGrid[p])
					if (m_pointsInstant.bItemGrid[p] != iExceptionCell)
						return false;
			} while (++j < bSize);

			return true;
		}
	}
	break;
	case DRAGON_SOUL_INVENTORY:
	{
		WORD wCell = Cell.cell;
		if (wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
			return false;

		iExceptionCell++;

		if (m_pointsInstant.wDSItemGrid[wCell])
		{
			if (m_pointsInstant.wDSItemGrid[wCell] == iExceptionCell)
			{
				if (bSize == 1)
					return true;

				int j = 1;

				do
				{
					int p = wCell + (DRAGON_SOUL_BOX_COLUMN_NUM * j);

					if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
						return false;

					if (m_pointsInstant.wDSItemGrid[p])
						if (m_pointsInstant.wDSItemGrid[p] != iExceptionCell)
							return false;
				} while (++j < bSize);

				return true;
			}
			else
				return false;
		}

		if (1 == bSize)
			return true;
		else
		{
			int j = 1;

			do
			{
				int p = wCell + (DRAGON_SOUL_BOX_COLUMN_NUM * j);

				if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
					return false;

				if (m_pointsInstant.bItemGrid[p])
					if (m_pointsInstant.wDSItemGrid[p] != iExceptionCell)
						return false;
			} while (++j < bSize);

			return true;
		}
	}
	break;

#ifdef ENABLE_SPECIAL_STORAGE
	case UPGRADE_INVENTORY:
	{
		WORD wCell = Cell.cell;
		if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
			return false;

		iExceptionCell++;

		if (m_pointsInstant.wSSUItemGrid[wCell])
		{
			if (m_pointsInstant.wSSUItemGrid[wCell] == iExceptionCell)
			{
				if (bSize == 1)
					return true;

				int j = 1;
				BYTE bPage = wCell / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT);

				do
				{
					CELL_UINT p = wCell + (5 * j);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						return false;

					if (p / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT) != bPage)
						return false;

					if (m_pointsInstant.wSSUItemGrid[p])
						if (m_pointsInstant.wSSUItemGrid[p] != iExceptionCell)
							return false;
				} while (++j < bSize);

				return true;
			}
			else
				return false;
		}

		if (1 == bSize)
			return true;
		else
		{
			int j = 1;
			BYTE bPage = wCell / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT);

			do
			{
				UINT p = wCell + (5 * j);

				if (p >= SPECIAL_INVENTORY_MAX_NUM)
					return false;

				if (p / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT) != bPage)
					return false;

				if (m_pointsInstant.bItemGrid[p])
					if (m_pointsInstant.wSSUItemGrid[p] != iExceptionCell)
						return false;
			} while (++j < bSize);

			return true;
		}
	}
	break;

	case BOOK_INVENTORY:
	{
		WORD wCell = Cell.cell;
		if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
			return false;

		iExceptionCell++;

		if (m_pointsInstant.wSSBItemGrid[wCell])
		{
			if (m_pointsInstant.wSSBItemGrid[wCell] == iExceptionCell)
			{
				if (bSize == 1)
					return true;

				int j = 1;
				BYTE bPage = wCell / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT);

				do
				{
					UINT p = wCell + (5 * j);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						return false;

					if (p / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT) != bPage)
						return false;

					if (m_pointsInstant.wSSBItemGrid[p])
						if (m_pointsInstant.wSSBItemGrid[p] != iExceptionCell)
							return false;
				} while (++j < bSize);

				return true;
			}
			else
				return false;
		}

		if (1 == bSize)
			return true;
		else
		{
			int j = 1;
			BYTE bPage = wCell / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT);

			do
			{
				UINT p = wCell + (5 * j);

				if (p >= SPECIAL_INVENTORY_MAX_NUM)
					return false;

				if (p / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT) != bPage)
					return false;

				if (m_pointsInstant.bItemGrid[p])
					if (m_pointsInstant.wSSBItemGrid[p] != iExceptionCell)
						return false;
			} while (++j < bSize);

			return true;
		}
	}
	break;

	case STONE_INVENTORY:
	{
		WORD wCell = Cell.cell;
		if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
			return false;

		iExceptionCell++;

		if (m_pointsInstant.wSSSItemGrid[wCell])
		{
			if (m_pointsInstant.wSSSItemGrid[wCell] == iExceptionCell)
			{
				if (bSize == 1)
					return true;

				int j = 1;
				BYTE bPage = wCell / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT);

				do
				{
					UINT p = wCell + (5 * j);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						return false;

					if (p / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT) != bPage)
						return false;

					if (m_pointsInstant.wSSSItemGrid[p])
						if (m_pointsInstant.wSSSItemGrid[p] != iExceptionCell)
							return false;
				} while (++j < bSize);

				return true;
			}
			else
				return false;
		}

		if (1 == bSize)
			return true;
		else
		{
			int j = 1;
			BYTE bPage = wCell / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT);

			do
			{
				UINT p = wCell + (5 * j);

				if (p >= SPECIAL_INVENTORY_MAX_NUM)
					return false;

				if (p / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT) != bPage)
					return false;

				if (m_pointsInstant.bItemGrid[p])
					if (m_pointsInstant.wSSSItemGrid[p] != iExceptionCell)
						return false;
			} while (++j < bSize);

			return true;
		}
	}
	break;

	case CHEST_INVENTORY:
	{
		WORD wCell = Cell.cell;
		if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
			return false;

		iExceptionCell++;

		if (m_pointsInstant.wSSCItemGrid[wCell])
		{
			if (m_pointsInstant.wSSCItemGrid[wCell] == iExceptionCell)
			{
				if (bSize == 1)
					return true;

				int j = 1;
				BYTE bPage = wCell / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT);

				do
				{
					UINT p = wCell + (5 * j);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						return false;

					if (p / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT) != bPage)
						return false;

					if (m_pointsInstant.wSSCItemGrid[p])
						if (m_pointsInstant.wSSCItemGrid[p] != iExceptionCell)
							return false;
				} while (++j < bSize);

				return true;
			}
			else
				return false;
		}

		if (1 == bSize)
			return true;
		else
		{
			int j = 1;
			BYTE bPage = wCell / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT);

			do
			{
				UINT p = wCell + (5 * j);

				if (p >= SPECIAL_INVENTORY_MAX_NUM)
					return false;

				if (p / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT) != bPage)
					return false;

				if (m_pointsInstant.bItemGrid[p])
					if (m_pointsInstant.wSSCItemGrid[p] != iExceptionCell)
						return false;
			} while (++j < bSize);

			return true;
		}
	}
#endif
#ifdef ENABLE_SWITCHBOT
	case SWITCHBOT:
	{
		uint16_t wCell = Cell.cell;
		if (wCell >= SWITCHBOT_SLOT_COUNT) {
			return false;
		}

		if (m_pointsInstant.pSwitchbotItems[wCell]){
			return false;
		}
		return true;
	}
#endif

#ifdef ENABLE_BUFFI_SYSTEM
	case BUFFI_INVENTORY:
	{
		WORD wCell = Cell.cell;
		if (wCell >= BUFFI_MAX_SLOT)
		{
			return false;
		}

		if (m_pointsInstant.pBuffiItems[wCell])
		{
			return false;
		}

		return true;
	}
#endif

	}
	return false;
}

int CHARACTER::GetEmptyInventory(BYTE size) const
{
	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos(INVENTORY, i), size))
			return i;
	return -1;
}

#ifdef ENABLE_SPECIAL_STORAGE
int CHARACTER::GetEmptyUpgradeInventory(LPITEM pItem) const
{
	if (NULL == pItem || !pItem->IsUpgradeItem())
		return -1;

	BYTE bSize = pItem->GetSize();

	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos(UPGRADE_INVENTORY, i), bSize))
			return i;

	return -1;
}

int CHARACTER::GetEmptyBookInventory(LPITEM pItem) const
{
	if (NULL == pItem || !pItem->IsBook())
		return -1;

	BYTE bSize = pItem->GetSize();

	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos(BOOK_INVENTORY, i), bSize))
			return i;

	return -1;
}

int CHARACTER::GetEmptyStoneInventory(LPITEM pItem) const
{
	if (NULL == pItem || !pItem->IsStone())
		return -1;

	BYTE bSize = pItem->GetSize();

	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos(STONE_INVENTORY, i), bSize))
			return i;

	return -1;
}

int CHARACTER::GetEmptyChestInventory(LPITEM pItem) const
{
	if (NULL == pItem || !pItem->IsChest())
		return -1;
	BYTE bSize = pItem->GetSize();
	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos(CHEST_INVENTORY, i), bSize))
			return i;
	return -1;
}

#ifdef ENABLE_CHEST_OPEN_RENEWAL
int CHARACTER::IsEmptyUpgradeInventory(BYTE freeSize) const
{
	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos(UPGRADE_INVENTORY, i), freeSize))
			return i;

	return -1;
}

int CHARACTER::IsEmptyBookInventory(BYTE freeSize) const
{
	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos(BOOK_INVENTORY, i), freeSize))
			return i;

	return -1;
}

int CHARACTER::IsEmptyStoneInventory(BYTE freeSize) const
{
	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos(STONE_INVENTORY, i), freeSize))
			return i;

	return -1;
}

int CHARACTER::IsEmptyChestInventory(BYTE freeSize) const
{
	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos(CHEST_INVENTORY, i), freeSize))
			return i;
	return -1;
}
#endif

#endif

#ifdef ENABLE_SPLIT_ITEMS_FAST
int CHARACTER::GetEmptyInventoryFromIndex(WORD index, BYTE itemSize, BYTE windowtype) const //SPLIT ITEMS
{
	if (index > INVENTORY_MAX_NUM)
		return -1;

	for (WORD i = index; i < INVENTORY_MAX_NUM; ++i)
	{
		if (IsEmptyItemGrid(TItemPos(windowtype, i), itemSize))
			return i;
	}
	return -1;
}
#endif

int CHARACTER::GetEmptyDragonSoulInventory(LPITEM pItem) const
{
	if (NULL == pItem || !pItem->IsDragonSoul())
		return -1;
	if (!DragonSoul_IsQualified())
	{
		return -1;
	}
	BYTE bSize = pItem->GetSize();
	WORD wBaseCell = DSManager::instance().GetBasePosition(pItem);

	if (WORD_MAX == wBaseCell)
		return -1;

	for (int i = 0; i < DRAGON_SOUL_BOX_SIZE; ++i)
		if (IsEmptyItemGrid(TItemPos(DRAGON_SOUL_INVENTORY, i + wBaseCell), bSize))
			return i + wBaseCell;

	return -1;
}

void CHARACTER::CopyDragonSoulItemGrid(std::vector<WORD>& vDragonSoulItemGrid) const
{
	vDragonSoulItemGrid.resize(DRAGON_SOUL_INVENTORY_MAX_NUM);

	std::copy(m_pointsInstant.wDSItemGrid, m_pointsInstant.wDSItemGrid + DRAGON_SOUL_INVENTORY_MAX_NUM, vDragonSoulItemGrid.begin());
}

int CHARACTER::CountEmptyInventory() const
{
	int	count = 0;

	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
		if (GetInventoryItem(i))
			count += GetInventoryItem(i)->GetSize();

	return (INVENTORY_MAX_NUM - count);
}

void TransformRefineItem(LPITEM pkOldItem, LPITEM pkNewItem)
{
	// ACCESSORY_REFINE
	if (pkOldItem->IsAccessoryForSocket())
	{
		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		{
			pkNewItem->SetSocket(i, pkOldItem->GetSocket(i));
		}
		//pkNewItem->StartAccessorySocketExpireEvent();
	}
	// END_OF_ACCESSORY_REFINE
	else
	{
		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		{
			if (!pkOldItem->GetSocket(i))
				break;
			else
				pkNewItem->SetSocket(i, 1);
		}

		int slot = 0;

		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		{
			long socket = pkOldItem->GetSocket(i);

			if (socket > 2 && socket != ITEM_BROKEN_METIN_VNUM)
				pkNewItem->SetSocket(slot++, socket);
		}
	}

	pkOldItem->CopyAttributeTo(pkNewItem);
}

void NotifyRefineSuccess(LPCHARACTER ch, LPITEM item, const char* way)
{
	if (NULL != ch && item != NULL)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "RefineSuceeded");
	}
}

void NotifyRefineFail(LPCHARACTER ch, LPITEM item, const char* way, int success = 0)
{
	if (NULL != ch && NULL != item)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "RefineFailed");
	}
}

#ifdef ENABLE_ANNOUNCEMENT_REFINE_SUCCES
static void RefineAnnouncement(LPCHARACTER ch, LPITEM pkNewItem)
{
	if (pkNewItem->GetRefineLevel() >= ENABLE_ANNOUNCEMENT_REFINE_SUCCES_MIN_LEVEL)
	{
		char buf[1024];
		snprintf(buf, sizeof(buf), "[%s]", pkNewItem->GetName());
		char szBuf[QUERY_MAX_LEN];
		snprintf(szBuf, sizeof(szBuf), "<¸ÄÁ¼³É¹¦> Íæ¼Ò[%s]¸ÄÁ¼-> %s ³É¹¦ÁË,ÔÙ½ÓÔÙÀ÷!!!", ch->GetName(), buf); 
		BroadcastNotice(szBuf);
	}
}

static void RefineAnnouncement_fail(LPCHARACTER ch, LPITEM pkNewItem)
{
	if (pkNewItem->GetRefineLevel() >= ENABLE_ANNOUNCEMENT_REFINE_FAIL_MIN_LEVEL)
	{
		char buf[1024];
		snprintf(buf, sizeof(buf), "[%s]", pkNewItem->GetName());
		char szBuf[QUERY_MAX_LEN];
		snprintf(szBuf, sizeof(szBuf), "<¸ÄÁ¼Ê§°Ü> Íæ¼Ò[%s]¸ÄÁ¼-> %s Ê§°ÜÁË,²»Òª·ÅÆú!!!", ch->GetName(), buf); 
		BroadcastNotice(szBuf);
	}
}

#endif

void CHARACTER::SetRefineNPC(LPCHARACTER ch)
{
	if (ch != NULL)
	{
		m_dwRefineNPCVID = ch->GetVID();
	}
	else
	{
		m_dwRefineNPCVID = 0;
	}
}

bool CHARACTER::DoRefine(LPITEM item, bool bMoneyOnly)
{
	if (!CanHandleItem(true))
	{
		ClearRefineMode();
		return false;
	}

	if (quest::CQuestManager::instance().GetEventFlag("update_refine_time") != 0)
	{
		if (get_global_time() < quest::CQuestManager::instance().GetEventFlag("update_refine_time") + (60 * 5))
		{
			return false;
		}
	}
#if defined(ENABLE_MAP_195_ALIGNMENT)
	if (GetMapIndex() == 195)
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Prisoner dungeon cannot be improved ."));
		return false;
	}
#endif
	const TRefineTable* prt = CRefineManager::instance().GetRefineRecipe(item->GetRefineSet());

	if (!prt)
		return false;

	DWORD result_vnum = item->GetRefinedVnum();

	// REFINE_COST
#ifdef ENABLE_EXTENDED_YANG_LIMIT
	int64_t cost = ComputeRefineFee(prt->cost);
#else
	int cost = ComputeRefineFee(prt->cost);
#endif

	int RefineChance = GetQuestFlag("main_quest_lv7.refine_chance");
	if (RefineChance > 0)
	{
		if (!item->CheckItemUseLevel(20) || item->GetType() != ITEM_WEAPON)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¹«·á °³·® ±âÈ¸´Â 20 ÀÌÇÏÀÇ ¹«±â¸¸ °¡´ÉÇÕ´Ï´Ù"));
			return false;
		}

		cost = 0;
		SetQuestFlag("main_quest_lv7.refine_chance", RefineChance - 1);
	}
	// END_OF_REFINE_COST
	
	if (result_vnum == 0)
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("´õ ÀÌ»ó °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	if (item->GetType() == ITEM_USE && item->GetSubType() == USE_TUNING)
		return false;

	TItemTable* pProto = ITEM_MANAGER::instance().GetTable(item->GetRefinedVnum());

	if (!pProto)
	{
		sys_err("DoRefine NOT GET ITEM PROTO %d", item->GetRefinedVnum());
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀÌ ¾ÆÀÌÅÛÀº °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	// REFINE_COST
	if (GetGold() < cost)
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("°³·®À» ÇÏ±â À§ÇÑ µ·ÀÌ ºÎÁ·ÇÕ´Ï´Ù."));
		return false;
	}

	if (!bMoneyOnly)
	{
		for (int i = 0; i < prt->material_count; ++i)
		{
			if (CountSpecifyItem(prt->materials[i].vnum) < prt->materials[i].count)
			{
				if (test_server)
				{
					ChatPacket(CHAT_TYPE_INFO, "Find %d, count %d, require %d", prt->materials[i].vnum, CountSpecifyItem(prt->materials[i].vnum), prt->materials[i].count);
				}
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("°³·®À» ÇÏ±â À§ÇÑ Àç·á°¡ ºÎÁ·ÇÕ´Ï´Ù."));
				return false;
			}
		}

		for (int i = 0; i < prt->material_count; ++i)
			RemoveSpecifyItem(prt->materials[i].vnum, prt->materials[i].count);
	}

	int prob = number(1, 100);
	#ifdef ENABLE_DEVILTOWER_SYSTEM
	int success_prob = prt->prob;
	if (IsRefineThroughGuild())
		//°ï»áÌú½³¸ÄÁ¼±ÈÆÕÍ¨ÀÏÍõ¸ß5%
		success_prob += 5;

	if (bMoneyOnly)
	{
		success_prob = ENABLE_DEVILTOWER_SYSTEM;
	}
	#else
	if (IsRefineThroughGuild() || bMoneyOnly)
		prob -= 10;
	#endif
	// END_OF_REFINE_COST

	#ifdef ENABLE_DEVILTOWER_SYSTEM
	if (prob <= success_prob)
	#else
	if (prob <= prt->prob)
	#endif
	{
		LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(result_vnum, 1, 0, false);

		if (pkNewItem)
		{
			ITEM_MANAGER::CopyAllAttrTo(item, pkNewItem);

			CELL_UINT bCell = item->GetCell();

			// DETAIL_REFINE_LOG
			NotifyRefineSuccess(this, item, IsRefineThroughGuild() ? "GUILD" : "POWER");
			ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE SUCCESS)");
			// END_OF_DETAIL_REFINE_LOG ÍõÌú½³
#ifdef ENABLE_ANNOUNCEMENT_REFINE_SUCCES
			RefineAnnouncement(this, pkNewItem);
#endif
			pkNewItem->AddToCharacter(this, TItemPos(INVENTORY, bCell));
			ITEM_MANAGER::instance().FlushDelayedSave(pkNewItem);
			PayRefineFee(cost);
#ifdef ENABLE_EXTENDED_BATTLE_PASS
			UpdateExtBattlePassMissionProgress(BP_ITEM_REFINE, 1, item->GetVnum());
#endif
		}
		else
		{
			// DETAIL_REFINE_LOG

			sys_err("cannot create item %u", result_vnum);
			NotifyRefineFail(this, item, IsRefineThroughGuild() ? "GUILD" : "POWER");
			// END_OF_DETAIL_REFINE_LOG
		}
	}
	else
	{
#ifdef ENABLE_ANNOUNCEMENT_REFINE_SUCCES
		LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(result_vnum, 1, 0, false);
#endif
		NotifyRefineFail(this, item, IsRefineThroughGuild() ? "GUILD" : "POWER");
		ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE FAIL)");
		//ÍõÌú½³
#ifdef ENABLE_ANNOUNCEMENT_REFINE_SUCCES
		RefineAnnouncement_fail(this, pkNewItem);
#endif
		//PointChange(POINT_GOLD, -cost);
		PayRefineFee(cost);
	}

	return true;
}

enum enum_RefineScrolls
{
	CHUKBOK_SCROLL = 0,	// ÓÀºãÖý¼þ
	HYUNIRON_CHN = 1,	// ÐþÌú
	YONGSIN_SCROLL = 2, // ÁúÉñµÄ×£¸£Êé
	MUSIN_SCROLL = 3,	// ÎäÉñµÄ×£¸£Êé
	YAGONG_SCROLL = 4,	// ¸ßÊÖÖ¸ÄÏÕë
	MEMO_SCROLL = 5,	// Ç¿ÕßµÄÁôÑÔ
	BDRAGON_SCROLL = 6,	// ÏÈÈËµÄÁôÑÔ
};

bool CHARACTER::DoRefineWithScroll(LPITEM item)
{

#if defined(ENABLE_MAP_195_ALIGNMENT)
	if (GetMapIndex() == 195)
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Prisoner dungeon cannot be improved ."));
		return false;
	}
#endif
	if (!CanHandleItem(true))
	{
		ClearRefineMode();
		return false;
	}
	
	if (quest::CQuestManager::instance().GetEventFlag("update_refine_time") != 0)
	{
		if (get_global_time() < quest::CQuestManager::instance().GetEventFlag("update_refine_time") + (60 * 5))
		{
			sys_log(0, "can't refine %d %s", GetPlayerID(), GetName());
			return false;
		}
	}

	ClearRefineMode();

	const TRefineTable* prt = CRefineManager::instance().GetRefineRecipe(item->GetRefineSet());

	if (!prt)
		return false;

	LPITEM pkItemScroll;

	if (m_iRefineAdditionalCell < 0)
		return false;

	pkItemScroll = GetInventoryItem(m_iRefineAdditionalCell);

	if (!pkItemScroll)
		return false;

	if (!(pkItemScroll->GetType() == ITEM_USE && pkItemScroll->GetSubType() == USE_TUNING))
		return false;

	if (pkItemScroll->GetVnum() == item->GetVnum())
		return false;

	DWORD result_vnum = item->GetRefinedVnum();
	DWORD result_fail_vnum = item->GetRefineFromVnum();

	if (result_vnum == 0)
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("´õ ÀÌ»ó °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	// MUSIN_SCROLL
	if (pkItemScroll->GetValue(0) == MUSIN_SCROLL)
	{
		if (item->GetRefineLevel() >= 4)
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀÌ °³·®¼­·Î ´õ ÀÌ»ó °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}
	}
	// END_OF_MUSIC_SCROLL

	else if (pkItemScroll->GetValue(0) == MEMO_SCROLL)
	{
		if (item->GetRefineLevel() != pkItemScroll->GetValue(1))
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀÌ °³·®¼­·Î °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}
	}
	else if (pkItemScroll->GetValue(0) == BDRAGON_SCROLL)
	{
		if (item->GetType() != ITEM_METIN || item->GetRefineLevel() != 4)
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀÌ ¾ÆÀÌÅÛÀ¸·Î °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}
	}

	TItemTable* pProto = ITEM_MANAGER::instance().GetTable(item->GetRefinedVnum());

	if (!pProto)
	{
		sys_err("DoRefineWithScroll NOT GET ITEM PROTO %d", item->GetRefinedVnum());
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀÌ ¾ÆÀÌÅÛÀº °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	if (GetGold() < prt->cost)
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("°³·®À» ÇÏ±â À§ÇÑ µ·ÀÌ ºÎÁ·ÇÕ´Ï´Ù."));
		return false;
	}

	for (int i = 0; i < prt->material_count; ++i)
	{
		if (CountSpecifyItem(prt->materials[i].vnum) < prt->materials[i].count)
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("°³·®À» ÇÏ±â À§ÇÑ Àç·á°¡ ºÎÁ·ÇÕ´Ï´Ù."));
			return false;
		}
	}

	for (int i = 0; i < prt->material_count; ++i)
		RemoveSpecifyItem(prt->materials[i].vnum, prt->materials[i].count);

	int prob = number(1, 100);
	int success_prob = prt->prob;
	bool bDestroyWhenFail = false;

	const char* szRefineType = "SCROLL";
	if (pkItemScroll->GetValue(0) == HYUNIRON_CHN ||
		pkItemScroll->GetValue(0) == YONGSIN_SCROLL || 
		pkItemScroll->GetValue(0) == YAGONG_SCROLL)
	{
		if (pkItemScroll->GetValue(0) == YONGSIN_SCROLL)
		{
#ifdef ENABLE_YONGSIN_YAGONG
			success_prob = prt->prob + ENABLE_YONGSIN_SCROLL;
#else
			const char hyuniron_prob[9] = { 100, 75, 65, 55, 45, 40, 35, 25, 20 };//ÁúÉñµÄ×£¸£
			success_prob = hyuniron_prob[MINMAX (0, item->GetRefineLevel(), 8)];
#endif
		}
		else if (pkItemScroll->GetValue(0) == YAGONG_SCROLL)
		{
#ifdef ENABLE_YONGSIN_YAGONG
			success_prob = prt->prob + ENABLE_YAGONG_SCROLL;
#else
			const char yagong_prob[9] = { 100, 100, 90, 80, 70, 60, 50, 30, 20 };//¸ßÊÖÖ¸ÄÏÕë
			success_prob = yagong_prob[MINMAX (0, item->GetRefineLevel(), 8)];
#endif
		}
		else if (pkItemScroll->GetValue(0) == HYUNIRON_CHN)  {} // @fixme121
		else
		{
			// sys_err("REFINE : Unknown refine scroll item. Value0: %d", pkItemScroll->GetValue(0));
		}

		if (pkItemScroll->GetValue(0) == HYUNIRON_CHN)
			bDestroyWhenFail = true;

		// DETAIL_REFINE_LOG
		if (pkItemScroll->GetValue(0) == HYUNIRON_CHN)
		{
			szRefineType = "HYUNIRON";
		}
		else if (pkItemScroll->GetValue(0) == YONGSIN_SCROLL)
		{
			szRefineType = "GOD_SCROLL";
		}
		else if (pkItemScroll->GetValue(0) == YAGONG_SCROLL)
		{
			szRefineType = "YAGONG_SCROLL";
		}
		// END_OF_DETAIL_REFINE_LOG
	}

	// DETAIL_REFINE_LOG
	if (pkItemScroll->GetValue(0) == MUSIN_SCROLL)
	{
		success_prob = 100;

		szRefineType = "MUSIN_SCROLL";
	}
	// END_OF_DETAIL_REFINE_LOG
	else if (pkItemScroll->GetValue(0) == MEMO_SCROLL)
	{
		success_prob = 100;
		szRefineType = "MEMO_SCROLL";
	}
	else if (pkItemScroll->GetValue(0) == BDRAGON_SCROLL)
	{
		success_prob = 80;
		szRefineType = "BDRAGON_SCROLL";
	}

	pkItemScroll->SetCount(pkItemScroll->GetCount() - 1);
//¶ÍÔì¼¼ÄÜÔö¼Ó¸ÄÁ¼³É¹¦ÂÊ
#ifdef ENABLE_PASSIVE_SKILLS
	int upgradeSkillLevel{ GetSkillLevel(SKILL_PASSIVE_UPGRADING) };

	if ((upgradeSkillLevel / 5) >= 1)
		success_prob += upgradeSkillLevel / 5;
#endif

	if (prob <= success_prob)
	{
		LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(result_vnum, 1, 0, false);

		if (pkNewItem)
		{
			ITEM_MANAGER::CopyAllAttrTo(item, pkNewItem);

			CELL_UINT bCell = item->GetCell();

			NotifyRefineSuccess(this, item, szRefineType);
			ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE SUCCESS)");
#ifdef ENABLE_ANNOUNCEMENT_REFINE_SUCCES
			RefineAnnouncement(this, pkNewItem);
#endif
			pkNewItem->AddToCharacter(this, TItemPos(INVENTORY, bCell));
			ITEM_MANAGER::instance().FlushDelayedSave(pkNewItem);
			PayRefineFee(prt->cost);
#ifdef ENABLE_EXTENDED_BATTLE_PASS
			UpdateExtBattlePassMissionProgress(BP_ITEM_REFINE, 1, item->GetVnum());
#endif
#ifdef ENABLE_PLAYER_STATS_SYSTEM
			PointChange(POINT_UPGRADE_ITEM, 1);
#endif
		}
		else
		{
			sys_err("cannot create item %u", result_vnum);
			NotifyRefineFail(this, item, szRefineType);
		}
	}

	else if (!bDestroyWhenFail && result_fail_vnum)
	{
		LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(result_fail_vnum, 1, 0, false);

		if (pkNewItem)
		{
			ITEM_MANAGER::CopyAllAttrTo(item, pkNewItem);

			CELL_UINT bCell = item->GetCell();

			NotifyRefineFail(this, item, szRefineType, -1);
			ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE FAIL)");
#ifdef ENABLE_ANNOUNCEMENT_REFINE_SUCCES
			RefineAnnouncement_fail(this, pkNewItem);
#endif
			pkNewItem->AddToCharacter(this, TItemPos(INVENTORY, bCell));
			ITEM_MANAGER::instance().FlushDelayedSave(pkNewItem);
			//PointChange(POINT_GOLD, -prt->cost);
			PayRefineFee(prt->cost);
		}
		else
		{
			sys_err("cannot create item %u", result_fail_vnum);
			NotifyRefineFail(this, item, szRefineType);

		}
	}
	else
	{
		NotifyRefineFail(this, item, szRefineType);

		PayRefineFee(prt->cost);
	}

	return true;
}

bool CHARACTER::RefineInformation(CELL_UINT bCell, BYTE bType, int iAdditionalCell)
{
#if defined(ENABLE_MAP_195_ALIGNMENT)
	if (GetMapIndex() == 195)
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Prisoner dungeon cannot be improved ."));
		return false;
	}
#endif
	if (bCell > INVENTORY_MAX_NUM)
		return false;

	LPITEM item = GetInventoryItem(bCell);

	if (!item)
		return false;

	if (bType == REFINE_TYPE_MONEY_ONLY && !GetQuestFlag("deviltower_zone.can_refine"))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»ç±Í Å¸¿ö ¿Ï·á º¸»óÀº ÇÑ¹ø±îÁö »ç¿ë°¡´ÉÇÕ´Ï´Ù."));
		return false;
	}

	TPacketGCRefineInformation p;

	p.header = HEADER_GC_REFINE_INFORMATION;
	p.pos = bCell;
	p.src_vnum = item->GetVnum();
	p.result_vnum = item->GetRefinedVnum();
	p.type = bType;

	if (p.result_vnum == 0)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾ÆÀÌÅÛÀº °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	if (item->GetType() == ITEM_USE && item->GetSubType() == USE_TUNING)
	{
		if (bType == 0)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾ÆÀÌÅÛÀº ÀÌ ¹æ½ÄÀ¸·Î´Â °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}
		else
		{
			LPITEM itemScroll = GetInventoryItem(iAdditionalCell);
			if (!itemScroll || item->GetVnum() == itemScroll->GetVnum())
			{
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("°°Àº °³·®¼­¸¦ ÇÕÄ¥ ¼ö´Â ¾ø½À´Ï´Ù."));
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Ãàº¹ÀÇ ¼­¿Í ÇöÃ¶À» ÇÕÄ¥ ¼ö ÀÖ½À´Ï´Ù."));
				return false;
			}
		}
	}

	CRefineManager& rm = CRefineManager::instance();

	const TRefineTable* prt = rm.GetRefineRecipe(item->GetRefineSet());

	if (!prt)
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀÌ ¾ÆÀÌÅÛÀº °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	//MAIN_QUEST_LV7
	if (GetQuestFlag("main_quest_lv7.refine_chance") > 0)
	{
		if (!item->CheckItemUseLevel(20) || item->GetType() != ITEM_WEAPON)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¹«·á °³·® ±âÈ¸´Â 20 ÀÌÇÏÀÇ ¹«±â¸¸ °¡´ÉÇÕ´Ï´Ù"));
			return false;
		}
		p.cost = 0;
	}
	else
		p.cost = ComputeRefineFee(prt->cost);

	p.prob = prt->prob;

	//¼ì²âµ½ÍöÁéËþÃâ¸ÄÁ¼²ÄÁÏ
	if (bType == REFINE_TYPE_MONEY_ONLY)
	{
		p.material_count = 0;
		memset (p.materials, 0, sizeof (p.materials));
	}
	else
	{
		p.material_count = prt->material_count;
		thecore_memcpy (&p.materials, prt->materials, sizeof (prt->materials));
	}

	GetDesc()->Packet(&p, sizeof(TPacketGCRefineInformation));

	SetRefineMode(iAdditionalCell);
	return true;
}

bool CHARACTER::RefineItem(LPITEM pkItem, LPITEM pkTarget)
{
#if defined(ENABLE_MAP_195_ALIGNMENT)
	if (GetMapIndex() == 195)
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Prisoner dungeon cannot be improved ."));
		return false;
	}
#endif
	if (!CanHandleItem())
		return false;

	if (pkItem->GetSubType() == USE_TUNING)
	{
		// MUSIN_SCROLL
		if (pkItem->GetValue(0) == MUSIN_SCROLL)
			RefineInformation(pkTarget->GetCell(), REFINE_TYPE_MUSIN, pkItem->GetCell());
		// END_OF_MUSIN_SCROLL
		else if (pkItem->GetValue(0) == HYUNIRON_CHN)
			RefineInformation(pkTarget->GetCell(), REFINE_TYPE_HYUNIRON, pkItem->GetCell());
		else if (pkItem->GetValue(0) == BDRAGON_SCROLL)
		{
			if (pkTarget->GetRefineSet() != 702) return false;
			RefineInformation(pkTarget->GetCell(), REFINE_TYPE_BDRAGON, pkItem->GetCell());
		}
		else
		{
			if (pkTarget->GetRefineSet() == 501) return false;
			RefineInformation(pkTarget->GetCell(), REFINE_TYPE_SCROLL, pkItem->GetCell());
		}
	}
	else if (pkItem->GetSubType() == USE_DETACHMENT && IS_SET(pkTarget->GetFlag(), ITEM_FLAG_REFINEABLE))
	{
		bool bHasMetinStone = false;

		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
		{
			long socket = pkTarget->GetSocket(i);
			if (socket > 2 && socket != ITEM_BROKEN_METIN_VNUM)
			{
				bHasMetinStone = true;
				break;
			}
		}

		if (bHasMetinStone)
		{
			for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
			{
				long socket = pkTarget->GetSocket(i);
				if (socket > 2 && socket != ITEM_BROKEN_METIN_VNUM)
				{
					AutoGiveItem(socket);
					pkTarget->SetSocket(i, ITEM_BROKEN_METIN_VNUM);
				}
			}
			pkItem->SetCount(pkItem->GetCount() - 1);
			return true;
		}
		else
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("»©³¾ ¼ö ÀÖ´Â ¸ÞÆ¾¼®ÀÌ ¾ø½À´Ï´Ù."));
			return false;
		}
	}

	return false;
}

EVENTFUNC(kill_campfire_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);

	if (info == NULL)
	{
		sys_err("kill_campfire_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER	ch = info->ch;

	if (ch == NULL) { // <Factor>
		return 0;
	}
	ch->m_pkMiningEvent = NULL;
	M2_DESTROY_CHARACTER(ch);
	return 0;
}

bool CHARACTER::GiveRecallItem(LPITEM item)
{
	int idx = GetMapIndex();
	int iEmpireByMapIndex = -1;

	if (idx < 20)
		iEmpireByMapIndex = 1;
	else if (idx < 40)
		iEmpireByMapIndex = 2;
	else if (idx < 60)
		iEmpireByMapIndex = 3;
	else if (idx < 10000)
		iEmpireByMapIndex = 0;

	switch (idx)
	{
	case 66:
	case 216:
		iEmpireByMapIndex = -1;
		break;
	}

	if (iEmpireByMapIndex && GetEmpire() != iEmpireByMapIndex)
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("±â¾ïÇØ µÑ ¼ö ¾ø´Â À§Ä¡ ÀÔ´Ï´Ù."));
		return false;
	}

	int pos;

	if (item->GetCount() == 1)
	{
		item->SetSocket(0, GetX());
		item->SetSocket(1, GetY());
	}
	else if ((pos = GetEmptyInventory(item->GetSize())) != -1)
	{
		LPITEM item2 = ITEM_MANAGER::instance().CreateItem(item->GetVnum(), 1);

		if (NULL != item2)
		{
			item2->SetSocket(0, GetX());
			item2->SetSocket(1, GetY());
			item2->AddToCharacter(this, TItemPos(INVENTORY, pos));

			item->SetCount(item->GetCount() - 1);
		}
	}
	else
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼ÒÁöÇ°¿¡ ºó °ø°£ÀÌ ¾ø½À´Ï´Ù."));
		return false;
	}

	return true;
}

void CHARACTER::ProcessRecallItem(LPITEM item)
{
	int idx;

	if ((idx = SECTREE_MANAGER::instance().GetMapIndex(item->GetSocket(0), item->GetSocket(1))) == 0)
		return;

	int iEmpireByMapIndex = -1;

	if (idx < 20)
		iEmpireByMapIndex = 1;
	else if (idx < 40)
		iEmpireByMapIndex = 2;
	else if (idx < 60)
		iEmpireByMapIndex = 3;
	else if (idx < 10000)
		iEmpireByMapIndex = 0;

	switch (idx)
	{
	case 84:
	case 216:
		iEmpireByMapIndex = -1;
		break;

	case 321:
	case 322:
	case 323:
	case 324:
		if (GetLevel() < 90)
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÀÌÅÛÀÇ ·¹º§ Á¦ÇÑº¸´Ù ·¹º§ÀÌ ³·½À´Ï´Ù."));
			return;
		}
		else
			break;
	}

	if (iEmpireByMapIndex && GetEmpire() != iEmpireByMapIndex)
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("±â¾ïµÈ À§Ä¡°¡ Å¸Á¦±¹¿¡ ¼ÓÇØ ÀÖ¾î¼­ ±ÍÈ¯ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		item->SetSocket(0, 0);
		item->SetSocket(1, 0);
	}
	else
	{
		WarpSet(item->GetSocket(0), item->GetSocket(1));
		item->SetCount(item->GetCount() - 1);
	}
}

void CHARACTER::__OpenPrivateShop()
{
#ifdef ENABLE_OPEN_SHOP_WITH_ARMOR
	ChatPacket(CHAT_TYPE_COMMAND, "OpenPrivateShop");
#else
	unsigned bodyPart = GetPart(PART_MAIN);
	switch (bodyPart)
	{
	case 0:
	case 1:
	case 2:
		ChatPacket(CHAT_TYPE_COMMAND, "OpenPrivateShop");
		break;
	default:
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("°©¿ÊÀ» ¹þ¾î¾ß °³ÀÎ »óÁ¡À» ¿­ ¼ö ÀÖ½À´Ï´Ù."));
		break;
	}
#endif
}

// MYSHOP_PRICE_LIST
#ifdef ENABLE_EXTENDED_YANG_LIMIT
void CHARACTER::SendMyShopPriceListCmd(DWORD dwItemVnum, int64_t dwItemPrice)
{
	char szLine[256];
	snprintf(szLine, sizeof(szLine), "MyShopPriceList %u %lld", dwItemVnum, dwItemPrice);
#else
void CHARACTER::SendMyShopPriceListCmd(DWORD dwItemVnum, DWORD dwItemPrice)
{
	char szLine[256];
	snprintf(szLine, sizeof(szLine), "MyShopPriceList %u %u", dwItemVnum, dwItemPrice);
#endif
	ChatPacket(CHAT_TYPE_COMMAND, szLine);
	sys_log(0, szLine);
}

void CHARACTER::UseSilkBotaryReal(const TPacketMyshopPricelistHeader * p)
{
	const TItemPriceInfo* pInfo = (const TItemPriceInfo*)(p + 1);

	if (!p->byCount)

		SendMyShopPriceListCmd(1, 0);
	else {
		for (int idx = 0; idx < p->byCount; idx++)
			SendMyShopPriceListCmd(pInfo[idx].dwVnum, pInfo[idx].dwPrice);
	}

	__OpenPrivateShop();
}

void CHARACTER::UseSilkBotary(void)
{
	if (m_bNoOpenedShop) {
		DWORD dwPlayerID = GetPlayerID();
		db_clientdesc->DBPacket(HEADER_GD_MYSHOP_PRICELIST_REQ, GetDesc()->GetHandle(), &dwPlayerID, sizeof(DWORD));
		m_bNoOpenedShop = false;
	}
	else {
		__OpenPrivateShop();
	}
}
// END_OF_MYSHOP_PRICE_LIST

int CalculateConsume(LPCHARACTER ch)
{
	static const int WARP_NEED_LIFE_PERCENT = 30;
	static const int WARP_MIN_LIFE_PERCENT = 10;
	// CONSUME_LIFE_WHEN_USE_WARP_ITEM
	int consumeLife = 0;
	{
		// CheckNeedLifeForWarp
		const HP_LL curLife = ch->GetHP();
		const int needPercent = WARP_NEED_LIFE_PERCENT;
		const HP_LL needLife = ch->GetMaxHP() * needPercent / 100;
		if (curLife < needLife)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("³²Àº »ý¸í·Â ¾çÀÌ ¸ðÀÚ¶ó »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return -1;
		}

		consumeLife = needLife;

		const int minPercent = WARP_MIN_LIFE_PERCENT;
		const HP_LL minLife = ch->GetMaxHP() * minPercent / 100;
		if (curLife - needLife < minLife)
			consumeLife = curLife - minLife;

		if (consumeLife < 0)
			consumeLife = 0;
	}
	// END_OF_CONSUME_LIFE_WHEN_USE_WARP_ITEM
	return consumeLife;
}

int CalculateConsumeSP(LPCHARACTER lpChar)
{
	static const int NEED_WARP_SP_PERCENT = 30;

	const int curSP = lpChar->GetSP();
	const int needSP = lpChar->GetMaxSP() * NEED_WARP_SP_PERCENT / 100;

	if (curSP < needSP)
	{
		lpChar->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("³²Àº Á¤½Å·Â ¾çÀÌ ¸ðÀÚ¶ó »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return -1;
	}

	return needSP;
}

bool CHARACTER::UseItemEx(LPITEM item, TItemPos DestCell
#ifdef ENABLE_CHEST_OPEN_RENEWAL
	, MAX_COUNT item_open_count
#endif
)
{
	int iLimitRealtimeStartFirstUseFlagIndex = -1;
	//int iLimitTimerBasedOnWearFlagIndex = -1;

	WORD wDestCell = DestCell.cell;
	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
	{
		long limitValue = item->GetProto()->aLimits[i].lValue;

		switch (item->GetProto()->aLimits[i].bType)
		{
		case LIMIT_LEVEL:
			if (GetLevel() < limitValue)
			{
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÀÌÅÛÀÇ ·¹º§ Á¦ÇÑº¸´Ù ·¹º§ÀÌ ³·½À´Ï´Ù."));
				return false;
			}
			break;

		case LIMIT_REAL_TIME_START_FIRST_USE:
			iLimitRealtimeStartFirstUseFlagIndex = i;
			break;

		case LIMIT_TIMER_BASED_ON_WEAR:
			//iLimitTimerBasedOnWearFlagIndex = i;
			break;
		}
	}

	if (CArenaManager::instance().IsLimitedItem(GetMapIndex(), item->GetVnum()) == true)
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
		return false;
	}
#ifdef ENABLE_BLOCK_ITEMS_ON_WAR_MAP
	//°ï»áÕ½³¡
	if (IS_ENABLE_ITEM_BLOCK(item->GetVnum()))
	{
		if (false == IS_ENABLE_MAP_BLOCK(GetMapIndex()))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("µ±Ç°»î¶¯µØÍ¼ÒÑÏÞÖÆÊ¹ÓÃ¸ÃÎïÆ·"));
			return false;
		}
	}
	//PVPÕù°ÔÈüµØÍ¼
	if (IS_ENABLE_ITEM_BLOCK_26(item->GetVnum()))
	{
		if (false == IS_ENABLE_MAP_BLOCK_26(GetMapIndex()))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("µ±Ç°»î¶¯µØÍ¼ÒÑÏÞÖÆÊ¹ÓÃ¸ÃÎïÆ·"));
			return false;
		}
	}
#endif
	// @fixme402 (IsLoadedAffect to block affect hacking)
	if (!IsLoadedAffect())
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Effect not loaded"));
		return false;
	}

	// @fixme141 BEGIN
	if (TItemPos(item->GetWindow(), item->GetCell()).IsBeltInventoryPosition())
	{
		LPITEM beltItem = GetWear(WEAR_BELT);

		if (NULL == beltItem)
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<Belt> You can't use this item if you have no equipped belt."));
			return false;
		}

		if (false == CBeltInventoryHelper::IsAvailableCell(item->GetCell() - BELT_INVENTORY_SLOT_START, beltItem->GetValue(0)))
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<Belt> You can't use this item if you don't upgrade your belt."));
			return false;
		}
	}
	// @fixme141 END

	if (-1 != iLimitRealtimeStartFirstUseFlagIndex)
	{
		if (0 == item->GetSocket(1))
		{
			long duration = (0 != item->GetSocket(0)) ? item->GetSocket(0) : item->GetProto()->aLimits[iLimitRealtimeStartFirstUseFlagIndex].lValue;

			if (0 == duration)
				duration = 60 * 60 * 24 * 7;

			item->SetSocket(0, time(0) + duration);
			item->StartRealTimeExpireEvent();
		}

		if (false == item->IsEquipped())
			item->SetSocket(1, item->GetSocket(1) + 1);
	}

	switch (item->GetType())
	{
		case ITEM_HAIR:
		{
			return ItemProcess_Hair(item, wDestCell);
			break;
		}

		case ITEM_POLYMORPH:
		{
			return ItemProcess_Polymorph(item);
			break;
		}

		case ITEM_QUEST:
		{
#ifdef ENABLE_QUEST_DND_EVENT
			if (IS_SET(item->GetFlag(), ITEM_FLAG_APPLICABLE))
			{
				LPITEM item2;

				if (!GetItem(DestCell) || !(item2 = GetItem(DestCell)))
					return false;

				if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
					return false;

				quest::CQuestManager::instance().DND(GetPlayerID(), item, item2, false);
				return true;
			}
#endif

			if (GetArena() != NULL || IsObserverMode() == true)
			{
				if (item->GetVnum() == 50051 || item->GetVnum() == 50052 || item->GetVnum() == 50053)
				{
					ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("YOU_CANNOT_USE_THIS_WHILE_RIDING"));
					return false;
				}
			}
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
			if (GetWear(WEAR_COSTUME_MOUNT))
			{
				if (item->GetVnum() == 50051 || item->GetVnum() == 50052 || item->GetVnum() == 50053)
				{
					ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Wearing a mount and mount skin, one cannot ride a horse"));
					return false;
				}
			}
#endif
			if (!IS_SET(item->GetFlag(), ITEM_FLAG_QUEST_USE | ITEM_FLAG_QUEST_USE_MULTIPLE))
			{
				if (item->GetSIGVnum() == 0)
				{
					quest::CQuestManager::instance().UseItem(GetPlayerID(), item, false);
				}
				else
				{
					quest::CQuestManager::instance().SIGUse(GetPlayerID(), item->GetSIGVnum(), item, false);
				}
			}
			break;
		}
		case ITEM_CAMPFIRE:
		{

			if (GetMapIndex() == 113) // lightwork campfire ox fixed
			{
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("OX map cannot summon campfire"));
				return false;
			}
			if (GetMapIndex() == 26) // lightwork campfire ox fixed
			{
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("OX map cannot summon campfire"));
				return false;
			}

			float fx, fy;
			GetDeltaByDegree(GetRotation(), 100.0f, &fx, &fy);

			LPSECTREE tree = SECTREE_MANAGER::instance().Get(GetMapIndex(), (long)(GetX() + fx), (long)(GetY() + fy));

			if (!tree)
			{
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¸ð´ÚºÒÀ» ÇÇ¿ï ¼ö ¾ø´Â ÁöÁ¡ÀÔ´Ï´Ù."));
				return false;
			}

			if (tree->IsAttr((long)(GetX() + fx), (long)(GetY() + fy), ATTR_WATER))
			{
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¹° ¼Ó¿¡ ¸ð´ÚºÒÀ» ÇÇ¿ï ¼ö ¾ø½À´Ï´Ù."));
				return false;
			}

			if ((m_campFireUseTime + 30) > get_global_time()) //lightwork campfire spam fixed
			{
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Please wait %d seconds and try again"),(m_campFireUseTime + 30) - get_global_time());
					// NewChatPacket(WAIT_TO_USE_AGAIN, "%d",(m_campFireUseTime + 30) - get_global_time());
				return false;
			}

			m_campFireUseTime = get_global_time(); //lightwork campfire spam fixed

			LPCHARACTER campfire = CHARACTER_MANAGER::instance().SpawnMob(fishing::CAMPFIRE_MOB, GetMapIndex(), (long)(GetX() + fx), (long)(GetY() + fy), 0, false, number(0, 359));

			char_event_info* info = AllocEventInfo<char_event_info>();

			info->ch = campfire;

			campfire->m_pkMiningEvent = event_create(kill_campfire_event, info, PASSES_PER_SEC(40));

			item->SetCount(item->GetCount() - 1);
			break;
		}
	

		case ITEM_UNIQUE:
		{
			switch (item->GetSubType())
			{
				case USE_ABILITY_UP:
				{
					switch (item->GetValue(0))
					{
					case APPLY_MOV_SPEED:
						if (FindAffect(AFFECT_UNIQUE_ABILITY, POINT_MOV_SPEED))// @fixme171
							return false;
						AddAffect(AFFECT_UNIQUE_ABILITY, POINT_MOV_SPEED, item->GetValue(2), AFF_MOV_SPEED_POTION, item->GetValue(1), 0, true, true);
						break;

					case APPLY_ATT_SPEED:
						if (FindAffect(AFFECT_UNIQUE_ABILITY, POINT_ATT_SPEED))// @fixme171
							return false;
						AddAffect(AFFECT_UNIQUE_ABILITY, POINT_ATT_SPEED, item->GetValue(2), AFF_ATT_SPEED_POTION, item->GetValue(1), 0, true, true);
						break;

					case APPLY_STR:
						if (FindAffect(AFFECT_UNIQUE_ABILITY, POINT_ST))// @fixme171
							return false;
						AddAffect(AFFECT_UNIQUE_ABILITY, POINT_ST, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
						break;

					case APPLY_DEX:
						if (FindAffect(AFFECT_UNIQUE_ABILITY, POINT_DX))// @fixme171
							return false;
						AddAffect(AFFECT_UNIQUE_ABILITY, POINT_DX, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
						break;

					case APPLY_CON:
						if (FindAffect(AFFECT_UNIQUE_ABILITY, POINT_HT))// @fixme171
							return false;
						AddAffect(AFFECT_UNIQUE_ABILITY, POINT_HT, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
						break;

					case APPLY_INT:
						if (FindAffect(AFFECT_UNIQUE_ABILITY, POINT_IQ))// @fixme171
							return false;
						AddAffect(AFFECT_UNIQUE_ABILITY, POINT_IQ, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
						break;

					case APPLY_CAST_SPEED:
						if (FindAffect(AFFECT_UNIQUE_ABILITY, POINT_CASTING_SPEED))// @fixme171
							return false;
						AddAffect(AFFECT_UNIQUE_ABILITY, POINT_CASTING_SPEED, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
						break;

					case APPLY_RESIST_MAGIC:
						if (FindAffect(AFFECT_UNIQUE_ABILITY, POINT_RESIST_MAGIC))// @fixme171
							return false;
						AddAffect(AFFECT_UNIQUE_ABILITY, POINT_RESIST_MAGIC, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
						break;

					case APPLY_ATT_GRADE_BONUS:
						if (FindAffect(AFFECT_UNIQUE_ABILITY, POINT_ATT_GRADE_BONUS))// @fixme171
							return false;
						AddAffect(AFFECT_UNIQUE_ABILITY, POINT_ATT_GRADE_BONUS,
							item->GetValue(2), 0, item->GetValue(1), 0, true, true);
						break;

					case APPLY_DEF_GRADE_BONUS:
						if (FindAffect(AFFECT_UNIQUE_ABILITY, POINT_DEF_GRADE_BONUS))// @fixme171
							return false;
						AddAffect(AFFECT_UNIQUE_ABILITY, POINT_DEF_GRADE_BONUS,
							item->GetValue(2), 0, item->GetValue(1), 0, true, true);
						break;
					}
				}

				if (GetDungeon())
					GetDungeon()->UsePotion(this);

				if (GetWarMap())
					GetWarMap()->UsePotion(this, item);

				item->SetCount(item->GetCount() - 1);
				break;

				default:
				{
					if (item->GetSubType() == USE_SPECIAL)
					{
						switch (item->GetVnum())
						{
						case 71049:
							UseSilkBotary();
							break;
						}
					}
					else
					{
						if (!item->IsEquipped())
						{
							if (item->GetCount() > 1)
							{
								LPITEM uniqueItem{ GetWear(WEAR_UNIQUE1) };
								LPITEM uniqueItem2{ GetWear(WEAR_UNIQUE2) };

								if (uniqueItem && uniqueItem->GetVnum() == item->GetVnum())
									return false;

								if (uniqueItem2 && uniqueItem2->GetVnum() == item->GetVnum())
									return false;


								if (item->GetValue(1) &&
									((uniqueItem && uniqueItem->GetValue(1) == item->GetValue(1)) ||
									(uniqueItem2 && uniqueItem2->GetValue(1) == item->GetValue(1))))
								{
									ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("°°Àº Á¾·ùÀÇ À¯´ÏÅ© ¾ÆÀÌÅÛ µÎ °³¸¦ µ¿½Ã¿¡ ÀåÂøÇÒ ¼ö ¾ø½À´Ï´Ù."));
									return false; // same special group control
								}
										
								MAX_COUNT count = item->GetCount();
								LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(item->GetVnum(), count-1, 0, false);

								if (!pkNewItem)
								{
									sys_err("%d can't be created. ItemUnique", item->GetVnum());
									return false;
								}

								CELL_UINT bCell = item->GetCell();
								item->SetCount(1);

								if (!EquipItem(item))
								{
									item->SetCount(count);
									M2_DESTROY_ITEM(pkNewItem);
									return false;
								}

								pkNewItem->AddToCharacter(this, TItemPos(INVENTORY, bCell));
								ITEM_MANAGER::instance().FlushDelayedSave(pkNewItem);

							}
							else
								EquipItem(item);
						}
							
						else
							UnequipItem(item);
					}
				}
				break;
			}
			break;
		}

		// case ITEM_COSTUME:
		// {
// #ifdef ENABLE_ACCE_COSTUME_SKIN
			// if (item->GetSubType() == COSTUME_WING)
			// {
				// LPITEM acce = GetWear(WEAR_COSTUME_ACCE);
				// if (acce)
				// {
					// if (!item->IsEquipped())
					// {
						// EquipItem(item);
					// }
					// else
					// {
						// UnequipItem(item);
					// }
				// }
				// else
				// {
					// ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("CANT_WEAR_SASH_SKIN_IF_NOT_WEAR_SASH"));
					// return false;
				// }
			// }
// #endif
			// else
			// {
				// if (!item->IsEquipped())
					// EquipItem(item);
				// else
					// UnequipItem(item);
			// }
			// break;
		// }
#ifdef ENABLE_ACCE_COSTUME_SKIN
		case ITEM_COSTUME:
		{

			if (!item->IsEquipped())
				EquipItem(item);
			else
				UnequipItem(item);
			break;
		}
#endif

		case ITEM_WEAPON:
		case ITEM_ARMOR:
		case ITEM_ROD:
		case ITEM_RING:
		case ITEM_BELT:
#ifdef ENABLE_PET_COSTUME_SYSTEM
		case ITEM_PET:
#endif
#ifdef ENABLE_CROWN_SYSTEM
		case ITEM_CROWN:
#endif
#ifdef ENABLE_ITEMS_SHINING
		case ITEM_SHINING:
#endif
#ifdef ENABLE_BOOSTER_ITEM
		case ITEM_BOOSTER:
#endif
#ifdef ENABLE_PENDANT_ITEM
		case ITEM_PENDANT:
#endif
#ifdef ENABLE_GLOVE_ITEM
		case ITEM_GLOVE:
#endif
#ifdef ENABLE_RINGS
		case ITEM_RINGS:
#endif
#ifdef ENABLE_DREAMSOUL
		case ITEM_DREAMSOUL:
#endif
		case ITEM_PICK:
		{
			if (!item->IsEquipped())
				EquipItem(item);
			else
				UnequipItem(item);
			break;
		}

		case ITEM_DS:
		{
			if (!item->IsEquipped())
				return false;
			return DSManager::instance().PullOut(this, NPOS, item);
			break;
		}
		case ITEM_SPECIAL_DS:
		{
			if (!item->IsEquipped())
				EquipItem(item);
			else
				UnequipItem(item);
			break;
		}

#ifdef ENABLE_MOUNT_PET_ACCE_AURA_COSTUME_SYSTEM
//»Ö¸´Ô­°æ
#ifdef ENABLE_MOUNT_SKIN
		case ITEM_MOUNT_SKIN:
		{
			if (true == IsRiding())
			{
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("CANT_DO_THIS_WHILE_MOUNTING"));
				return false;
			}
			LPITEM mount = GetWear(WEAR_COSTUME_MOUNT);
			if (mount)
			{
				if (!item->IsEquipped())
				{
					EquipItem(item);
				}
				else
				{
					UnequipItem(item);
				}
			}
			else
			{
				if (item->IsEquipped())
				{
					UnequipItem(item);
				}
				else
				{
					ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("CANT_WEAR_MOUNT_SKIN_IF_NOT_WEAR_MOUNT"));
					return false;
				}
			}
			break;
		}
#endif

#ifdef ENABLE_PET_SKIN
		case ITEM_PET_SKIN:
		{
			LPITEM pkPet = GetWear(WEAR_PET);
	
			if (pkPet)//normal Pet takiliyyken 
			{
				if (!item->IsEquipped())
				{
					EquipItem(item);
				}
				else
				{
					UnequipItem(item);
				}
			}
			else//takili degilse return
			{
				if (item->IsEquipped())
				{
					UnequipItem(item);
				}
				else
				{
					ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("CANT_WEAR_PET_SKIN_IF_NOT_WEAR_PET"));
					return false;
				}
			}
			break;
		}
#endif

#ifdef ENABLE_AURA_SKIN
		case ITEM_AURA_SKIN:
		{
			LPITEM aura = GetWear(WEAR_COSTUME_AURA);
			if (aura)
			{
				if (!item->IsEquipped())
				{
					EquipItem(item);
				}
				else
				{
					UnequipItem(item);
				}
			}
			else
			{
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("CANT_WEAR_AURA_SKIN_IF_NOT_WEAR_AURA"));
				return false;
			}
			break;
		}
#endif
#else
#ifdef ENABLE_MOUNT_SKIN
		case ITEM_MOUNT_SKIN:
		{
			if (true == IsRiding())//binekteyse degistiremez
			{
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("CANT_DO_THIS_WHILE_MOUNTING"));
				return false;
			}

			if (!item->IsEquipped())
			{
				EquipItem(item);
			}
			else
			{
				UnequipItem(item);
			}

			break;
		}
#endif
#ifdef ENABLE_PET_SKIN
		case ITEM_PET_SKIN:
		{
			if (!item->IsEquipped())
			{
				EquipItem(item);
			}
			else
			{
				UnequipItem(item);
			}
			break;
		}
#endif
#ifdef ENABLE_AURA_SKIN
		case ITEM_AURA_SKIN:
		{
			if (!item->IsEquipped())
			{
				EquipItem(item);
			}
			else
			{
				UnequipItem(item);
			}
			break;
		}
#endif
#endif

		case ITEM_FISH:
		{
			if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
			{
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
				return false;
			}
			if (item->GetSubType() == FISH_ALIVE)
				fishing::UseFish(this, item);
			break;
		}
		
		case ITEM_TREASURE_BOX:
		{
			return false;

		}
		break;
		
		case ITEM_TREASURE_KEY:
		{
			LPITEM item2;

			if (!GetItem (DestCell) || ! (item2 = GetItem (DestCell)))
			{
				return false;
			}

			if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
			{
				return false;
			}

			if (item2->GetType() != ITEM_TREASURE_BOX)
			{
				ChatPacket (CHAT_TYPE_TALKING, LC_TEXT ("¿­¼è·Î ¿©´Â ¹°°ÇÀÌ ¾Æ´Ñ°Í °°´Ù."));
				return false;
			}

				if (item->GetValue(0) == item2->GetValue(0))
				{
					//ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("¿­¼è´Â ¸ÂÀ¸³ª ¾ÆÀÌÅÛ ÁÖ´Â ºÎºÐ ±¸ÇöÀÌ ¾ÈµÇ¾ú½À´Ï´Ù."));
					DWORD dwBoxVnum = item2->GetVnum();
					std::vector <DWORD> dwVnums;
					std::vector <DWORD> dwCounts;
					std::vector <LPITEM> item_gets(0);
					int count = 0;

					if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count))
					{
						item->SetCount(item->GetCount() - 1);//½ðÏä×Ó
						item2->SetCount(item2->GetCount() - 1);//½ðÔ¿³×

						for (int i = 0; i < count; i++){
							switch (dwVnums[i])
							{
								case CSpecialItemGroup::GOLD:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("µ· %d ³ÉÀ» È¹µæÇß½À´Ï´Ù."), dwCounts[i]);
									break;
								case CSpecialItemGroup::EXP:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ºÎÅÍ ½ÅºñÇÑ ºûÀÌ ³ª¿É´Ï´Ù."));
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%dÀÇ °æÇèÄ¡¸¦ È¹µæÇß½À´Ï´Ù."), dwCounts[i]);
									break;
								case CSpecialItemGroup::MOB:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ¸ó½ºÅÍ°¡ ³ªÅ¸³µ½À´Ï´Ù!"));
									break;
								case CSpecialItemGroup::SLOW:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ³ª¿Â »¡°£ ¿¬±â¸¦ µéÀÌ¸¶½ÃÀÚ ¿òÁ÷ÀÌ´Â ¼Óµµ°¡ ´À·ÁÁ³½À´Ï´Ù!"));
									break;
								case CSpecialItemGroup::DRAIN_HP:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ°¡ °©ÀÚ±â Æø¹ßÇÏ¿´½À´Ï´Ù! »ý¸í·ÂÀÌ °¨¼ÒÇß½À´Ï´Ù."));
									break;
								case CSpecialItemGroup::POISON:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ³ª¿Â ³ì»ö ¿¬±â¸¦ µéÀÌ¸¶½ÃÀÚ µ¶ÀÌ ¿Â¸öÀ¸·Î ÆÛÁý´Ï´Ù!"));
									break;
#ifdef ENABLE_WOLFMAN_CHARACTER
								case CSpecialItemGroup::BLEEDING:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ³ª¿Â ³ì»ö ¿¬±â¸¦ µéÀÌ¸¶½ÃÀÚ µ¶ÀÌ ¿Â¸öÀ¸·Î ÆÛÁý´Ï´Ù!"));
									break;
#endif
								case CSpecialItemGroup::MOB_GROUP:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ¸ó½ºÅÍ°¡ ³ªÅ¸³µ½À´Ï´Ù!"));
									break;
								default:
									if (item_gets[i])
									{
										if (dwCounts[i] > 1)
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ %s °¡ %d °³ ³ª¿Ô½À´Ï´Ù."), item_gets[i]->GetName(), dwCounts[i]);
										else
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ %s °¡ ³ª¿Ô½À´Ï´Ù."), item_gets[i]->GetName());

									}
							}
						}
					}
					else
					{
						ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("¿­¼è°¡ ¸ÂÁö ¾Ê´Â °Í °°´Ù."));
						return false;
					}
				}
				else
				{
					ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("¿­¼è°¡ ¸ÂÁö ¾Ê´Â °Í °°´Ù."));
					return false;
				}
			}
			break;

#ifdef ENABLE_CHEST_OPEN_RENEWAL
		case ITEM_GIFTBOX:
		{
			DWORD dwBoxVnum = item->GetVnum();
			if( (dwBoxVnum >= 50255 && dwBoxVnum <= 50260) || (dwBoxVnum >= 51501 && dwBoxVnum <= 51519) )
			{
				if( !(this->DragonSoul_IsQualified()) )
				{
					ChatPacket(CHAT_TYPE_INFO,LC_TEXT("¸ÕÀú ¿ëÈ¥¼® Äù½ºÆ®¸¦ ¿Ï·áÇÏ¼Å¾ß ÇÕ´Ï´Ù."));
					return false;
				}
			}

			bool multiopen = false;
			if (item_open_count > 1)
			{
				multiopen = true;
			}

			if (multiopen)
			{
				if (!IsActivateTime(OPEN_CHEST_CHECK_TIME, 1))
				{
					ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("The operation failed, please wait a moment and try again"));
					return false;
				}
			}

			const CSpecialItemGroup* pGroup = ITEM_MANAGER::Instance().GetSpecialItemGroup(item->GetVnum());
			if (item->GetType() == ITEM_GIFTBOX && pGroup)
			{
				switch (item->GetVnum())
				{
					case 51511:
					case 51512:
					case 51513:
					case 51514:
					case 51515:
					case 51516:
					case 51519:
					{
						if (!ChestDSInventoryEmpety(item->GetVnum()))
						{
							ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("NEED_2SPACE_IN_DS_INV"));
							return false;
						}
						ChestOpenDragonSoul(item, multiopen);
						if (multiopen)
						{
							SetActivateTime(OPEN_CHEST_CHECK_TIME);
						}
						return true;
					}
				}
				
				if (!ChestInventoryEmpetyFull())
				{
					ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("NEED_2SPACE_IN_INVS"));
					return false;
				}

				switch (pGroup->GetOpenType())
				{
					case CSpecialItemGroup::CR_CONST:
					{
						if (multiopen)
							ChestOpenConst(item);
						else
							ChestOpenConstSingle(item);
						break;
					}
					case CSpecialItemGroup::CR_STONE:
					{
						if (multiopen)
							ChestOpenNewPct(item, CHEST_TYPE_STONE);
						else
							ChestOpenNewPctSingle(item);
						break;
					}
					case CSpecialItemGroup::CR_NEW_PCT:
					{
						if (multiopen)
							ChestOpenNewPct(item,CHEST_TYPE_NEWPCT);
						else
							ChestOpenNewPctSingle(item);
						break;
					}
					case CSpecialItemGroup::CR_NORM:
					{
						if (multiopen)
							ChestOpenNewPct(item, CHEST_TYPE_NORM);
						else
							ChestOpenNewPctSingle(item);
						break;
					}
					default:break;
				}
				if (multiopen)
				{
					SetActivateTime(OPEN_CHEST_CHECK_TIME);
				}
			}
			else
			{
				ChatPacket (CHAT_TYPE_TALKING, LC_TEXT ("¾Æ¹«°Íµµ ¾òÀ» ¼ö ¾ø¾ú½À´Ï´Ù."));
				return false;
			}
			break;
		}
#endif

		case ITEM_SKILLFORGET:
		{
			if (!item->GetSocket(0))
			{
#ifdef ENABLE_BOOKS_STACKFIX
				item->SetCount(item->GetCount() - 1);
#else
				ITEM_MANAGER::instance().RemoveItem(item);
#endif
				return false;
			}

			DWORD dwVnum = item->GetSocket(0);

			if (SkillLevelDown(dwVnum))
			{
#ifdef ENABLE_BOOKS_STACKFIX
				item->SetCount(item->GetCount() - 1);
#else
				ITEM_MANAGER::instance().RemoveItem(item);
#endif
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("½ºÅ³ ·¹º§À» ³»¸®´Âµ¥ ¼º°øÇÏ¿´½À´Ï´Ù."));
			}
			else
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("½ºÅ³ ·¹º§À» ³»¸± ¼ö ¾ø½À´Ï´Ù."));
			break;
		}

		case ITEM_USE:
		{

#ifdef ENABLE_NEW_PET_SYSTEM
			if (item->GetVnum() == 55106)
			{
				if (GetNewPetSystem() && GetNewPetSystem()->IsSummon())
				{
					if (GetNewPetSystem()->GetLevel() >= 1 && GetNewPetSystem()->GetLevel() <= 40 && GetNewPetSystem()->CanExp())
					{
						GetNewPetSystem()->SetExp(1000000);
						item->SetCount(item->GetCount() - 1);
						return true;
					}
					if (GetNewPetSystem()->GetLevel() >= 41 && GetNewPetSystem()->GetLevel() <= 60 && GetNewPetSystem()->CanExp())
					{
						GetNewPetSystem()->SetExp(5000000);
						item->SetCount(item->GetCount() - 1);
						return true;
					}
					if (GetNewPetSystem()->GetLevel() >= 61 && GetNewPetSystem()->GetLevel() <= 80 && GetNewPetSystem()->CanExp())
					{
						GetNewPetSystem()->SetExp(50000000);
						item->SetCount(item->GetCount() - 1);
						return true;
					}
					if (GetNewPetSystem()->GetLevel() >= 81 && GetNewPetSystem()->GetLevel() <= 99 && GetNewPetSystem()->CanExp())
					{
						GetNewPetSystem()->SetExp(100000000);
						item->SetCount(item->GetCount() - 1);
						return true;
					}
					if (GetNewPetSystem()->GetLevel() >= 100 && GetNewPetSystem()->GetLevel() < 120 && GetNewPetSystem()->CanExp())
					{
						GetNewPetSystem()->SetExp(200000000);
						item->SetCount(item->GetCount() - 1);
						return true;
					}
					if (GetNewPetSystem()->GetLevel() == 120 )
					{
						ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("The pet has reached the highest level"));
						return true;
					}
				}
			}
#endif
#ifdef ENABLE_ITEM_ADD_TIME_STATUS
			if(item->GetVnum() >= 90120 && item->GetVnum() <= 90123){
				int days[4] = {7, 15, 30, 90};
				int pos = item->GetVnum() - 90120;
				if(pos < 0 || pos > 3)
					return false;
				std::vector<const char*> fields; 
				fields.push_back("gold_expire"); 
				fields.push_back("silver_expire");
				fields.push_back("safebox_expire"); 
				fields.push_back("autoloot_expire"); 
				fields.push_back("fish_mind_expire"); 
				fields.push_back("marriage_fast_expire"); 
				fields.push_back("money_drop_rate_expire");
				char szQuery[1024]; // ÒÆ³ýÎÞÓÃµÄszYearÊý×é
				int add_days = days[pos]; // Ã÷È·ÐÂÔöÌìÊý±äÁ¿£¬ÌáÉý¿É¶ÁÐÔ
				size_t vSize = fields.size();
				for(size_t i = 0; i < vSize; ++i){
					const char* field_name = fields[i];
					// ºËÐÄÐÞ¸Ä£ºÊ¹ÓÃMySQLµÄGREATESTº¯ÊýÈ¡¡¸Ô­ÓÐ¹ýÆÚÊ±¼ä¡¹ºÍ¡¸µ±Ç°Ê±¼ä¡¹µÄ×î´óÖµ£¬ÔÙ¼ÓÌìÊý
					snprintf(szQuery, sizeof(szQuery),
							 "UPDATE %s.account "
							 "SET %s = GREATEST(IFNULL(%s, NOW()), NOW()) + INTERVAL %d DAY "
							 "WHERE id = %d",
							 gB_AccountSQL.c_str(),
							 field_name,
							 field_name,
							 add_days,
							 GetDesc()->GetAccountTable().id);
					// Ö´ÐÐ¸üÐÂSQL
					std::unique_ptr<SQLMsg> msg(DBManager::instance().DirectQuery(szQuery));
				}
				item->SetCount(item->GetCount() - 1);
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Successfully %d addedstatus."), add_days); // ÓÅ»¯ÌáÊ¾ÎÄ°¸
				return true;
			}
#endif
			switch (item->GetSubType())
			{
				case USE_TIME_CHARGE_PER:
				{
					LPITEM pDestItem = GetItem(DestCell);
					if (NULL == pDestItem)
					{
						return false;
					}

					if (pDestItem->IsDragonSoul())
					{
						int ret;
						if (item->GetVnum() == DRAGON_HEART_VNUM)
						{
							ret = pDestItem->GiveMoreTime_Per((float)item->GetSocket(ITEM_SOCKET_CHARGING_AMOUNT_IDX));
						}
						else
						{
							ret = pDestItem->GiveMoreTime_Per((float)item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
						}
						if (ret > 0)
						{
							ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("X_SECONDS_ARE_ADDED %d "), ret);
							item->SetCount(item->GetCount() - 1);
							return true;
						}
						else
						{
							ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ADDING_TIME_FAILED"));
							return false;
						}
					}
					else
						return false;
					break;
				}

				case USE_TIME_CHARGE_FIX:
				{
					LPITEM pDestItem = GetItem(DestCell);
					if (NULL == pDestItem)
					{
						return false;
					}

					if (pDestItem->IsDragonSoul())
					{
						int ret = pDestItem->GiveMoreTime_Fix(item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
						if (ret)
						{
							ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("X_SECONDS_ARE_ADDED %d "), ret);
							item->SetCount(item->GetCount() - 1);
							return true;
						}
						else
						{
							ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ADDING_TIME_FAILED"));
							return false;
						}
					}
					else
						return false;
					break;
				}

				case USE_SPECIAL:
				{
					switch (item->GetVnum())
					{
						
						case 50216:	//µ°¾Æ
							{
								if (FindAffect(AFFECT_NOG_ABILITY))
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
									return false;
								}
								long time = item->GetValue(0);
								long moveSpeedPer	= item->GetValue(1);
								long attPer	= item->GetValue(2);
								long expPer			= item->GetValue(3);
								AddAffect(AFFECT_NOG_ABILITY, POINT_MOV_SPEED, moveSpeedPer, AFF_MOV_SPEED_POTION, time, 0, true, true);
								AddAffect(AFFECT_NOG_ABILITY, POINT_MALL_ATTBONUS, attPer, AFF_NONE, time, 0, true, true);
								AddAffect(AFFECT_NOG_ABILITY, POINT_MALL_EXPBONUS, expPer, AFF_NONE, time, 0, true, true);
								item->SetCount(item->GetCount() - 1);
							}
							break;
						case 50183:	//ÌÇ
							{
								if (FindAffect(AFFECT_RAMADAN_ABILITY))
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
									return false;
								}
								// @fixme147 END
								long time = item->GetValue(0);
								long moveSpeedPer	= item->GetValue(1);
								long attPer	= item->GetValue(2);
								long expPer	= item->GetValue(3);
								AddAffect(AFFECT_RAMADAN_ABILITY, POINT_MOV_SPEED, moveSpeedPer, AFF_MOV_SPEED_POTION, time, 0, true, true);
								AddAffect(AFFECT_RAMADAN_ABILITY, POINT_MALL_ATTBONUS, attPer, AFF_NONE, time, 0, true, true);
								AddAffect(AFFECT_RAMADAN_ABILITY, POINT_MALL_EXPBONUS, expPer, AFF_NONE, time, 0, true, true);
								item->SetCount(item->GetCount() - 1);
							}
							break;
#ifdef ENABLE_EXTENDED_BATTLE_PASS
						case 93100:
						{
							int GetBPPremiumETime = CBattlePassManager::instance().BattlePassGetPremiumData();
							//if (GetBPPremiumETime != NULL)
							if (GetBPPremiumETime > 0) // ¾ÝÎÒÁË½â£¬Ëü»áÕ¼ÓÃ¸ß¼¶Õ½¶·Í¨ÐÐÖ¤µÄÊ±¼ä£¬È»ºóÔÚÓÐÊ±¼äµÄÇé¿öÏÂ¸øÓèËü¡£
							{
								if (!FindAffect(AFFECT_BATTLEPASS))
								{
									AddAffect(AFFECT_BATTLEPASS, POINT_NONE, 0, AFF_BATTLEPASS, abs(time(0) - GetBPPremiumETime), 0, true, false);
									// CBattlePassManager::instance().BattlePassRequestOpen(this);
									item->SetCount(item->GetCount() - 1);
									UpdatePacket();

									WarpSet(GetX(), GetY());
									Stop();

								}
								else
								{
									ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
									return false;
								}
							}
							break;
						}
#endif
					//½á»é½äÖ¸
						case 70302:
						{
							marriage::TMarriage* pMarriage = marriage::CManager::instance().Get(GetPlayerID());
							if (pMarriage)
							{
								if (pMarriage->ch1 != NULL)
								{
									if (CArenaManager::instance().IsArenaMap(pMarriage->ch1->GetMapIndex()) == true)
									{
										ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
										break;
									}
#ifdef TOURNAMENT_PVP_SYSTEM
									if (CTournamentPvP::instance().IsTournamentMap(pMarriage->ch1, TOURNAMENT_BLOCK_RING_MARRIAGE))
										return false;
#endif
								}

								if (pMarriage->ch2 != NULL)
								{
									if (CArenaManager::instance().IsArenaMap(pMarriage->ch2->GetMapIndex()) == true)
									{
										ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
										break;
									}
#ifdef TOURNAMENT_PVP_SYSTEM
									if (CTournamentPvP::instance().IsTournamentMap(pMarriage->ch2, TOURNAMENT_BLOCK_RING_MARRIAGE))
										return false;
#endif
								}
								//ÏûºÄÀ¶
								// int consumeSP = CalculateConsumeSP(this);

								// if (consumeSP < 0)
									// return false;

								// PointChange(POINT_SP, -consumeSP, false);

								WarpToPID(pMarriage->GetOther(GetPlayerID()));
							}
							else
							ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("°áÈ¥ »óÅÂ°¡ ¾Æ´Ï¸é °áÈ¥¹ÝÁö¸¦ »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
							break;
						}

					//ÓÂÆøÅû·ç
					case 70038:
					case 76007:
					case 70057:
					{
						AggregateMonster();	//¾ÛºÏ¹ÖÎï
						item->SetCount (item->GetCount() - 1);

#ifdef ENABLE_OFFICIAL_STUFF
						if (GetCapeEffectPulse() + 50 > thecore_pulse())
							return true;

						SpecificEffectPacket(1);
						SetCapeEffectPulse(thecore_pulse());
#endif
						return true;
					}

					case 70008:
						ForgetMyAttacker();
						item->SetCount (item->GetCount() - 1);
						break;

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
					//90000
					case ACCE_REVERSAL_VNUM_1:
					//39046
					case ACCE_REVERSAL_VNUM_2:
					{
						LPITEM item2;
						if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
							return false;

						if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
							return false;

						if (!CleanAcceAttr(item, item2))
							return false;
						item->SetCount(item->GetCount() - 1);
						break;
					}
#endif


					case 27989:
					case 76006:
					{
						LPSECTREE_MAP pMap = SECTREE_MANAGER::instance().GetMap (GetMapIndex());

						if (pMap != NULL)
						{
							item->SetSocket (0, item->GetSocket (0) + 1);

							FFindStone f;

							pMap->for_each (f);

							if (f.m_mapStone.size() > 0)
							{
								std::map<DWORD, LPCHARACTER>::iterator stone = f.m_mapStone.begin();

								DWORD max = UINT_MAX;
								LPCHARACTER pTarget = stone->second;

								while (stone != f.m_mapStone.end())
								{
									DWORD dist = (DWORD)DISTANCE_SQRT (GetX() - stone->second->GetX(), GetY() - stone->second->GetY());

									if (dist != 0 && max > dist)
									{
										max = dist;
										pTarget = stone->second;
									}
									stone++;
								}

								if (pTarget != NULL)
								{
									int val = 3;

									if (max < 10000)
									{
										val = 2;
									}
									else if (max < 70000)
									{
										val = 1;
									}

									ChatPacket (CHAT_TYPE_COMMAND, "StoneDetect %u %d %d", (DWORD)GetVID(), val,
												(int)GetDegreeFromPositionXY (GetX(), pTarget->GetY(), pTarget->GetX(), GetY()));
								}
								else
								{
									ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("°¨Áö±â¸¦ ÀÛ¿ëÇÏ¿´À¸³ª °¨ÁöµÇ´Â ¿µ¼®ÀÌ ¾ø½À´Ï´Ù."));
								}
							}
							else
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("°¨Áö±â¸¦ ÀÛ¿ëÇÏ¿´À¸³ª °¨ÁöµÇ´Â ¿µ¼®ÀÌ ¾ø½À´Ï´Ù."));
							}

							if (item->GetSocket (0) >= 6)
							{
								ChatPacket (CHAT_TYPE_COMMAND, "StoneDetect %u 0 0", (DWORD)GetVID());
								// ITEM_MANAGER::instance().RemoveItem (item);
								item->SetCount(item->GetCount() - 1);
							}
						}
						break;
					}
					break;

					case 27996:
					{
						item->SetCount(item->GetCount() - 1);
						AttackedByPoison(NULL); // @warme008
						break;
					}
					
					case 27987:
						{
							item->SetCount(item->GetCount() - 1);

							int r = number(1, 100);

							if (r <= 50)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Á¶°³¿¡¼­ µ¹Á¶°¢ÀÌ ³ª¿Ô½À´Ï´Ù."));
								AutoGiveItem(27990);
							}
							else
							{
								const int prob_table_gb2312[] =
								{
									95, 97, 99
								};

								const int * prob_table = prob_table_gb2312;

								if (r <= prob_table[0])
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Á¶°³°¡ ÈçÀûµµ ¾øÀÌ »ç¶óÁý´Ï´Ù."));
								}
								else if (r <= prob_table[1])
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Á¶°³¿¡¼­ ¹éÁøÁÖ°¡ ³ª¿Ô½À´Ï´Ù."));
									AutoGiveItem(27992);
								}
								else if (r <= prob_table[2])
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Á¶°³¿¡¼­ Ã»ÁøÁÖ°¡ ³ª¿Ô½À´Ï´Ù."));
									AutoGiveItem(27993);
								}
								else
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Á¶°³¿¡¼­ ÇÇÁøÁÖ°¡ ³ª¿Ô½À´Ï´Ù."));
									AutoGiveItem(27994);
								}
							}
						}
						break;

					case 71013:
					{
						CreateFly(number(FLY_FIREWORK1, FLY_FIREWORK6), this);
						item->SetCount(item->GetCount() - 1);
						break;
					}

					case 50100:
					case 50101:
					case 50102:
					case 50103:
					case 50104:
					case 50105:
					case 50106:
					{
						CreateFly(item->GetVnum() - 50100 + FLY_FIREWORK1, this);
						item->SetCount(item->GetCount() - 1);
						break;
					}

					case fishing::FISH_MIND_PILL_VNUM:
					{
						AddAffect(AFFECT_FISH_MIND_PILL, POINT_NONE, 0, AFF_FISH_MIND, 20 * 60, 0, true);
						item->SetCount(item->GetCount() - 1);
						break;
					}

					case 71107:
					{
#if defined(ENABLE_MAP_195_ALIGNMENT)
						if (GetMapIndex() == 195)
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ALIGNMENT_MAC195_USE_71107"));
							return false;
						}
#endif

						int alig = GetAlignment();
						if (alig >= 200000)
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼±¾ÇÄ¡¸¦ ´õ ÀÌ»ó ¿Ã¸± ¼ö ¾ø½À´Ï´Ù."));
							return false;
						}

						UpdateAlignment(20000);
						item->SetCount(item->GetCount() - 1);
						ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("¸¶À½ÀÌ ¸¼¾ÆÁö´Â±º. °¡½¿À» Áþ´©¸£´ø ¹«¾ð°¡°¡ Á» °¡º­¿öÁø ´À³¦ÀÌ¾ß."));
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼±¾ÇÄ¡°¡2000Áõ°¡ÇÏ¿´½À´Ï´Ù."));
						break;
					}

					case 50061:
					{
						if (IsPolymorphed())
						{
							ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("º¯½ÅÁß¿¡´Â Ã¥À» ÀÐÀ»¼ö ¾ø½À´Ï´Ù."));
							return false;

						}
						DWORD dwSkillVnum = item->GetValue (0);
						int iPct = MINMAX (0, item->GetValue (1), 100);

						if (GetSkillLevel (dwSkillVnum) >= 10)
						{
							ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("´õ ÀÌ»ó ¼ö·ÃÇÒ ¼ö ¾ø½À´Ï´Ù."));
							return false;
						}

						if (LearnSkillByBook (dwSkillVnum, iPct))
						{
							item->SetCount (item->GetCount() - 1);
						}
					}
					break;
					
					case 50060:
						{
							if (IsPolymorphed())
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("º¯½ÅÁß¿¡´Â Ã¥À» ÀÐÀ»¼ö ¾ø½À´Ï´Ù."));
								return false;

							}
							int iPct = MINMAX(0, item->GetValue(1), 100);

							if (GetLevel() < 50)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÁ÷ ½Â¸¶ ½ºÅ³À» ¼ö·ÃÇÒ ¼ö ÀÖ´Â ·¹º§ÀÌ ¾Æ´Õ´Ï´Ù."));
								return false;
							}

							if (GetPoint(POINT_HORSE_SKILL) >= 20 ||
									GetSkillLevel(SKILL_HORSE_WILDATTACK) + GetSkillLevel(SKILL_HORSE_CHARGE) + GetSkillLevel(SKILL_HORSE_ESCAPE) >= 60 ||
									GetSkillLevel(SKILL_HORSE_WILDATTACK_RANGE) + GetSkillLevel(SKILL_HORSE_CHARGE) + GetSkillLevel(SKILL_HORSE_ESCAPE) >= 60)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´õ ÀÌ»ó ½Â¸¶ ¼ö·Ã¼­¸¦ ÀÐÀ» ¼ö ¾ø½À´Ï´Ù."));
								return false;
							}

							if (number(1, 100) <= iPct)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("½Â¸¶ ¼ö·Ã¼­¸¦ ÀÐ¾î ½Â¸¶ ½ºÅ³ Æ÷ÀÎÆ®¸¦ ¾ò¾ú½À´Ï´Ù."));
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾òÀº Æ÷ÀÎÆ®·Î´Â ½Â¸¶ ½ºÅ³ÀÇ ·¹º§À» ¿Ã¸± ¼ö ÀÖ½À´Ï´Ù."));
								PointChange(POINT_HORSE_SKILL, 1);
							}
							else
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("½Â¸¶ ¼ö·Ã¼­ ÀÌÇØ¿¡ ½ÇÆÐÇÏ¿´½À´Ï´Ù."));
							}
							item->SetCount(item->GetCount() - 1);
						}
						break;

					case 50600:
						{
							if (IsPolymorphed())
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("º¯½ÅÁß¿¡´Â Ã¥À» ÀÐÀ»¼ö ¾ø½À´Ï´Ù."));
								return false;

							}
							DWORD dwSkillVnum = SKILL_MINING;
							int iPct = MINMAX(0, item->GetValue(1), 100);

							if (GetSkillLevel(dwSkillVnum)>=40)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´õ ÀÌ»ó ¼ö·ÃÇÒ ¼ö ¾ø½À´Ï´Ù."));
								return false;
							}

							if (LearnSkillByBook(dwSkillVnum, iPct))
							{
								item->SetCount(item->GetCount() - 1);
								// int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
								// if (distribution_test_server) iReadDelay /= 3;

								// SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
							}
						}
						break;

					case 50314: case 50315: case 50316:
					case 50323: case 50324:
					case 50325: case 50326:
						{
							if (IsPolymorphed() == true)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("µÐ°© Áß¿¡´Â ´É·ÂÀ» ¿Ã¸± ¼ö ¾ø½À´Ï´Ù."));
								return false;
							}

							int iSkillLevelLowLimit = item->GetValue(0);
							int iSkillLevelHighLimit = item->GetValue(1);
							int iPct = MINMAX(0, item->GetValue(2), 100);
							int iLevelLimit = item->GetValue(3);
							DWORD dwSkillVnum = 0;

							switch (item->GetVnum())
							{
								case 50314: case 50315: case 50316:
									dwSkillVnum = SKILL_POLYMORPH;
									break;

								case 50323: case 50324:
									dwSkillVnum = SKILL_ADD_HP;
									break;

								case 50325: case 50326:
									dwSkillVnum = SKILL_RESIST_PENETRATE;
									break;

								default:
									return false;
							}

							if (0 == dwSkillVnum)
								return false;

							if (GetLevel() < iLevelLimit)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ Ã¥À» ÀÐÀ¸·Á¸é ·¹º§À» ´õ ¿Ã·Á¾ß ÇÕ´Ï´Ù."));
								return false;
							}

							if (GetSkillLevel(dwSkillVnum) >= 40)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´õ ÀÌ»ó ¼ö·ÃÇÒ ¼ö ¾ø½À´Ï´Ù."));
								return false;
							}

							if (GetSkillLevel(dwSkillVnum) < iSkillLevelLowLimit)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ Ã¥Àº ³Ê¹« ¾î·Á¿ö ÀÌÇØÇÏ±â°¡ Èûµì´Ï´Ù."));
								return false;
							}

							if (GetSkillLevel(dwSkillVnum) >= iSkillLevelHighLimit)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ Ã¥À¸·Î´Â ´õ ÀÌ»ó ¼ö·ÃÇÒ ¼ö ¾ø½À´Ï´Ù."));
								return false;
							}

							if (LearnSkillByBook(dwSkillVnum, iPct))
							{
								item->SetCount(item->GetCount() - 1);
								// int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
								// if (distribution_test_server) iReadDelay /= 3;

								// SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
							}
						}
						break;
						
					case 50301:
					case 50302:
					case 50303:
						{
							if (IsPolymorphed() == true)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("µÐ°© Áß¿¡´Â ´É·ÂÀ» ¿Ã¸± ¼ö ¾ø½À´Ï´Ù."));
								return false;
							}

							int lv = GetSkillLevel(SKILL_LEADERSHIP);

							if (lv < item->GetValue(0))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ Ã¥Àº ³Ê¹« ¾î·Á¿ö ÀÌÇØÇÏ±â°¡ Èûµì´Ï´Ù."));
								return false;
							}

							if (lv >= item->GetValue(1))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ Ã¥Àº ¾Æ¹«¸® ºÁµµ µµ¿òÀÌ µÉ °Í °°Áö ¾Ê½À´Ï´Ù."));
								return false;
							}

							if (LearnSkillByBook(SKILL_LEADERSHIP))
							{
								item->SetCount(item->GetCount() - 1);
								// int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
								// if (distribution_test_server) iReadDelay /= 3;

								// SetSkillNextReadTime(SKILL_LEADERSHIP, get_global_time() + iReadDelay);
							}
						}
						break;
						
					case 50304:
					case 50305:
					case 50306:
						{
							if (IsPolymorphed())
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("º¯½ÅÁß¿¡´Â Ã¥À» ÀÐÀ»¼ö ¾ø½À´Ï´Ù."));
								return false;

							}
							if (GetSkillLevel(SKILL_COMBO) == 0 && GetLevel() < 30)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("·¹º§ 30ÀÌ µÇ±â Àü¿¡´Â ½ÀµæÇÒ ¼ö ÀÖÀ» °Í °°Áö ¾Ê½À´Ï´Ù."));
								return false;
							}

							if (GetSkillLevel(SKILL_COMBO) == 1 && GetLevel() < 50)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("·¹º§ 50ÀÌ µÇ±â Àü¿¡´Â ½ÀµæÇÒ ¼ö ÀÖÀ» °Í °°Áö ¾Ê½À´Ï´Ù."));
								return false;
							}

							if (GetSkillLevel(SKILL_COMBO) >= 2)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¿¬°è±â´Â ´õÀÌ»ó ¼ö·ÃÇÒ ¼ö ¾ø½À´Ï´Ù."));
								return false;
							}

							int iPct = item->GetValue(0);

							if (LearnSkillByBook(SKILL_COMBO, iPct))
							{
								item->SetCount(item->GetCount() - 1);
								// int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
								// if (distribution_test_server) iReadDelay /= 3;

								// SetSkillNextReadTime(SKILL_COMBO, get_global_time() + iReadDelay);
							}
						}
						break;

					case 70102:
					case 70103:
					{
#if defined(ENABLE_MAP_195_ALIGNMENT)
						if (GetMapIndex() == 195)
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ALIGNMENT_MAC195_USE_71107"));
							return false;
						}
#endif
						if (GetAlignment() >= 0)
							return false;

						int delta = MIN(-GetAlignment(), item->GetValue(0));
						UpdateAlignment(delta);
						item->SetCount(item->GetCount() - 1);
						break;
					}

					case 71109:
					case 72719:
					{
						LPITEM item2;

						if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
							return false;

						if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
							return false;

						if (item2->GetSocketCount() == 0)
							return false;

						switch (item2->GetType())
						{
						case ITEM_WEAPON:
							break;
						case ITEM_ARMOR:
							switch (item2->GetSubType())
							{
								case ARMOR_EAR:
								case ARMOR_WRIST:
								case ARMOR_NECK:
									ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("»©³¾ ¿µ¼®ÀÌ ¾ø½À´Ï´Ù"));
								return false;
							}
							break;

						default:
							return false;
						}

						std::stack<long> socket;

						for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
							socket.push(item2->GetSocket(i));

						int idx = ITEM_SOCKET_MAX_NUM - 1;

						while (socket.size() > 0)
						{
							if (socket.top() > 2 && socket.top() != ITEM_BROKEN_METIN_VNUM)
								break;
							idx--;
							socket.pop();
						}

						if (socket.size() == 0)
						{
							ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("»©³¾ ¿µ¼®ÀÌ ¾ø½À´Ï´Ù"));
							return false;
						}

						LPITEM pItemReward = AutoGiveItem(socket.top());

						if (pItemReward != NULL)
						{
							item2->SetSocket(idx, 1);
							item->SetCount(item->GetCount() - 1);
						}
						break;
					}
					
					case 80010:
					{
						if (GetExchange() || GetShop() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´Ù¸¥ °Å·¡Áß(Ã¢°í,±³È¯,»óÁ¡)¿¡´Â °³ÀÎ»óÁ¡À» »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù2."));
							return false;
						}
						if ((GetGold() + 5000000) <= GOLD_MAX-1)
						{
							PointChange(POINT_GOLD, +5000000, false);
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Get_500_wan_gold"));//500Íò½ð×©
							item->SetCount(item->GetCount() - 1);
							break;
						}
						else
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÀ¯ µ·ÀÌ 20¾ï³ÉÀ» ³Ñ¾î °Å·¡¸¦ ÇÛ¼ö°¡ ¾ø½À´Ï´Ù."));
							return false;
						}
					}
					
					case 80011:
					{
						if (GetExchange() || GetShop() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´Ù¸¥ °Å·¡Áß(Ã¢°í,±³È¯,»óÁ¡)¿¡´Â °³ÀÎ»óÁ¡À» »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù2."));
							return false;
						}
						if ((GetGold() + 10000000) <= GOLD_MAX-1)
						{
							PointChange(POINT_GOLD, +10000000, false);
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Get_1000_wan_gold"));//1000Íò½ð×©
							item->SetCount(item->GetCount() - 1);
							break;
						}
						else
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÀ¯ µ·ÀÌ 20¾ï³ÉÀ» ³Ñ¾î °Å·¡¸¦ ÇÛ¼ö°¡ ¾ø½À´Ï´Ù."));
							return false;
						}
					}
					
					case 80012:
					{
						if (GetExchange() || GetShop() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´Ù¸¥ °Å·¡Áß(Ã¢°í,±³È¯,»óÁ¡)¿¡´Â °³ÀÎ»óÁ¡À» »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù2."));
							return false;
						}
						if ((GetGold() + 20000000) <= GOLD_MAX-1)
						{
							PointChange(POINT_GOLD, +20000000, false);
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Get_2000_wan_gold"));//2000Íò½ð×©
							item->SetCount(item->GetCount() - 1);
							break;
						}
						else
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÀ¯ µ·ÀÌ 20¾ï³ÉÀ» ³Ñ¾î °Å·¡¸¦ ÇÛ¼ö°¡ ¾ø½À´Ï´Ù."));
							return false;
						}
					}
					
					case 90025:
					{
						if (GetExchange() || GetShop() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´Ù¸¥ °Å·¡Áß(Ã¢°í,±³È¯,»óÁ¡)¿¡´Â °³ÀÎ»óÁ¡À» »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù2."));
							return false;
						}
						if ((GetGold() + 100000000) <= GOLD_MAX-1)
						{
							PointChange(POINT_GOLD, +100000000, false);
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Get_1_million_gold"));//½ð×©ÒøÆ± 1ÒÚÁ½
							item->SetCount(item->GetCount() - 1);
							break;
						}
						else
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÀ¯ µ·ÀÌ 20¾ï³ÉÀ» ³Ñ¾î °Å·¡¸¦ ÇÛ¼ö°¡ ¾ø½À´Ï´Ù."));
							return false;
						}
					}
					case 90026:
					{
						if (GetExchange() || GetShop() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´Ù¸¥ °Å·¡Áß(Ã¢°í,±³È¯,»óÁ¡)¿¡´Â °³ÀÎ»óÁ¡À» »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù2."));
							return false;
						}
						if ((GetGold() + 200000000) <= GOLD_MAX-1)
						{
							PointChange(POINT_GOLD, +200000000, false);
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Get_2_million_gold"));//½ð×©ÒøÆ± 2ÒÚÁ½
							item->SetCount(item->GetCount() - 1);
							break;
						}
						else
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÀ¯ µ·ÀÌ 20¾ï³ÉÀ» ³Ñ¾î °Å·¡¸¦ ÇÛ¼ö°¡ ¾ø½À´Ï´Ù."));
							return false;
						}
					}
					case 90027:
					{
						if (GetExchange() || GetShop() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´Ù¸¥ °Å·¡Áß(Ã¢°í,±³È¯,»óÁ¡)¿¡´Â °³ÀÎ»óÁ¡À» »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù2."));
							return false;
						}
						if ((GetGold() + 400000000) <= GOLD_MAX-1)
						{
							PointChange(POINT_GOLD, +400000000, false);
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Get_4_million_gold"));//½ð×©ÒøÆ± 4ÒÚÁ½
							item->SetCount(item->GetCount() - 1);
							break;
						}
						else
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÀ¯ µ·ÀÌ 20¾ï³ÉÀ» ³Ñ¾î °Å·¡¸¦ ÇÛ¼ö°¡ ¾ø½À´Ï´Ù."));
							return false;
						}
					}
					case 90028:
					{
						if (GetExchange() || GetShop() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´Ù¸¥ °Å·¡Áß(Ã¢°í,±³È¯,»óÁ¡)¿¡´Â °³ÀÎ»óÁ¡À» »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù2."));
							return false;
						}
						
						if ((GetGold() + 5000000000) <= GOLD_MAX-1)
						{
							PointChange(POINT_GOLD, +5000000000, false);
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Get_50_million_gold"));//½ð×©ÒøÆ± 50ÒÚÁ½
							item->SetCount(item->GetCount() - 1);
							break;
						}
						else
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÀ¯ µ·ÀÌ 20¾ï³ÉÀ» ³Ñ¾î °Å·¡¸¦ ÇÛ¼ö°¡ ¾ø½À´Ï´Ù."));
							return false;
						}
					}
				
					case 90029:
					{
						if (GetExchange() || GetShop() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´Ù¸¥ °Å·¡Áß(Ã¢°í,±³È¯,»óÁ¡)¿¡´Â °³ÀÎ»óÁ¡À» »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù2."));
							return false;
						}
						
						if ((GetGold() + 10000000000) <= GOLD_MAX-1)
						{
							PointChange(POINT_GOLD, +10000000000, false);
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Get_100_million_gold"));//½ð×©ÒøÆ± 100ÒÚÁ½
							item->SetCount(item->GetCount() - 1);
							break;
						}
						else
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÀ¯ µ·ÀÌ 20¾ï³ÉÀ» ³Ñ¾î °Å·¡¸¦ ÇÛ¼ö°¡ ¾ø½À´Ï´Ù."));
							return false;
						}
					}
				
					case 90030:
					{
						if (GetExchange() || GetShop() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´Ù¸¥ °Å·¡Áß(Ã¢°í,±³È¯,»óÁ¡)¿¡´Â °³ÀÎ»óÁ¡À» »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù2."));
							return false;
						}
						
						if ((GetGold() + 50000000000) <= GOLD_MAX-1)
						{
							PointChange(POINT_GOLD, +50000000000, false);
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Get_500_million_gold"));//½ð×©ÒøÆ± 500ÒÚÁ½
							item->SetCount(item->GetCount() - 1);
							break;
						}
						else
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÀ¯ µ·ÀÌ 20¾ï³ÉÀ» ³Ñ¾î °Å·¡¸¦ ÇÛ¼ö°¡ ¾ø½À´Ï´Ù."));
							return false;
						}
					}

					
					case 80030:
						{
							if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
							{
								ChatPacket(CHAT_TYPE_INFO, "ÒòÎª¿ªÆô½»Ò×»ò²Ö¿â£¬ÎÞ·¨Ê¹ÓÃ³äÖµÈÙÓþµã");
								return false;
							}
							SetQuestFlag("forked_new.win_point", GetQuestFlag("forked_new.win_point") + 1);
							item->SetCount(item->GetCount() - 1);
							ChatPacket(CHAT_TYPE_INFO, "ÒÑ³É¹¦³äÖµ1ÈÙÓþµãÇëÕÒÀÖÊ¦²éÑ¯!");
						}
						
						break;

					case 80031:
						{
							if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
							{
								ChatPacket(CHAT_TYPE_INFO, "ÒòÎª¿ªÆô½»Ò×»ò²Ö¿â£¬ÎÞ·¨Ê¹ÓÃ³äÖµÈÙÓþµã");
								return false;
							}
							SetQuestFlag("forked_new.win_point", GetQuestFlag("forked_new.win_point") + 10);
							item->SetCount(item->GetCount() - 1);
							ChatPacket(CHAT_TYPE_INFO, "ÒÑ³É¹¦³äÖµ10ÈÙÓþµãÇëÕÒÀÖÊ¦²éÑ¯!");
						}
						
						break;

					case 80032:
						{
							if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
							{
								ChatPacket(CHAT_TYPE_INFO, "ÒòÎª¿ªÆô½»Ò×»ò²Ö¿â£¬ÎÞ·¨Ê¹ÓÃ³äÖµÈÙÓþµã");
								return false;
							}
							SetQuestFlag("forked_new.win_point", GetQuestFlag("forked_new.win_point") + 20);
							item->SetCount(item->GetCount() - 1);
							ChatPacket(CHAT_TYPE_INFO, "ÒÑ³É¹¦³äÖµ20ÈÙÓþµãÇëÕÒÀÖÊ¦²éÑ¯!");
						}
						
						break;

					case 80033:
						{
							if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
							{
								ChatPacket(CHAT_TYPE_INFO, "ÒòÎª¿ªÆô½»Ò×»ò²Ö¿â£¬ÎÞ·¨Ê¹ÓÃ³äÖµÈÙÓþµã");
								return false;
							}
							SetQuestFlag("forked_new.win_point", GetQuestFlag("forked_new.win_point") + 50);
							item->SetCount(item->GetCount() - 1);
							ChatPacket(CHAT_TYPE_INFO, "ÒÑ³É¹¦³äÖµ50ÈÙÓþµãÇëÕÒÀÖÊ¦²éÑ¯!");
						}
						
						break;

					case 80034:
						{
							if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
							{
								ChatPacket(CHAT_TYPE_INFO, "ÒòÎª¿ªÆô½»Ò×»ò²Ö¿â£¬ÎÞ·¨Ê¹ÓÃ³äÖµÈÙÓþµã");
								return false;
							}

							SetQuestFlag("forked_new.win_point", GetQuestFlag("forked_new.win_point") + 100);
							item->SetCount(item->GetCount() - 1);
							ChatPacket(CHAT_TYPE_INFO, "ÒÑ³É¹¦³äÖµ100ÈÙÓþµãÇëÕÒÀÖÊ¦²éÑ¯!");
							
						}
						
						break;

					case 80035:
						{
							if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen() )
							{
								ChatPacket(CHAT_TYPE_INFO, "ÒòÎª¿ªÆô½»Ò×»ò²Ö¿â£¬ÎÞ·¨Ê¹ÓÃ³äÖµÈÙÓþµã");
								return false;
							}
							SetQuestFlag("forked_new.win_point", GetQuestFlag("forked_new.win_point") + 200);
							item->SetCount(item->GetCount() - 1);
							ChatPacket(CHAT_TYPE_INFO, "ÒÑ³É¹¦³äÖµ200ÈÙÓþµãÇëÕÒÀÖÊ¦²éÑ¯!");
						}
						
						break;
						
					case 80036:
						{
							if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
							{
								ChatPacket(CHAT_TYPE_INFO, "ÒòÎª¿ªÆô½»Ò×»ò²Ö¿â£¬ÎÞ·¨Ê¹ÓÃ³äÖµÈÙÓþµã");
								return false;
							}
							SetQuestFlag("forked_new.win_point", GetQuestFlag("forked_new.win_point") + 500);
							item->SetCount(item->GetCount() - 1);
							ChatPacket(CHAT_TYPE_INFO, "ÒÑ³É¹¦³äÖµ500ÈÙÓþµãÇëÕÒÀÖÊ¦²éÑ¯!");
						}
						
						break;

					case 70201:
					case 70202:
					case 70203:
					case 70204:
					case 70205:
					case 70206:
					{
						// NEW_HAIR_STYLE_ADD
						if (GetPart (PART_HAIR) >= 1001)
						{
							ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÇöÀç Çì¾î½ºÅ¸ÀÏ¿¡¼­´Â ¿°»ö°ú Å»»öÀÌ ºÒ°¡´ÉÇÕ´Ï´Ù."));
						}
						// END_NEW_HAIR_STYLE_ADD
						else
						{
							quest::CQuestManager& q = quest::CQuestManager::instance();
							quest::PC* pPC = q.GetPC (GetPlayerID());

							if (pPC)
							{
								int last_dye_level = pPC->GetFlag ("dyeing_hair.last_dye_level");

								if (last_dye_level == 0 || last_dye_level + 3 <= GetLevel() || item->GetVnum() == 70201)
								{
									SetPart(PART_HAIR, item->GetVnum() - 70201);

									if (item->GetVnum() == 70201)
									{
										pPC->SetFlag ("dyeing_hair.last_dye_level", 0);
									}
									else
									{
										pPC->SetFlag ("dyeing_hair.last_dye_level", GetLevel());
									}

									item->SetCount (item->GetCount() - 1);
									UpdatePacket();
								}
								else
								{
									ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("%d ·¹º§ÀÌ µÇ¾î¾ß ´Ù½Ã ¿°»öÇÏ½Ç ¼ö ÀÖ½À´Ï´Ù."), last_dye_level + 3);
								}
							}
						}
					}
					break;

					//50024
					case ITEM_VALENTINE_ROSE:
					//50025
					case ITEM_VALENTINE_CHOCOLATE:
					{
						DWORD dwBoxVnum = item->GetVnum();
						std::vector <DWORD> dwVnums;
						std::vector <DWORD> dwCounts;
						std::vector <LPITEM> item_gets (0);
						int count = 0;


						if (((item->GetVnum() == ITEM_VALENTINE_ROSE) && (SEX_MALE == GET_SEX (this))) ||
						((item->GetVnum() == ITEM_VALENTINE_CHOCOLATE) && (SEX_FEMALE == GET_SEX (this))))
						{

							ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼ºº°ÀÌ ¸ÂÁö¾Ê¾Æ ÀÌ ¾ÆÀÌÅÛÀ» ¿­ ¼ö ¾ø½À´Ï´Ù."));
							return false;
						}


						if (GiveItemFromSpecialItemGroup (dwBoxVnum, dwVnums, dwCounts, item_gets, count))
						{
							item->SetCount (item->GetCount() - 1);
						}
					}
					break;
					//50032
					case ITEM_WHITEDAY_CANDY:
					//50031
					case ITEM_WHITEDAY_ROSE:
					{
						DWORD dwBoxVnum = item->GetVnum();
						std::vector <DWORD> dwVnums;
						std::vector <DWORD> dwCounts;
						std::vector <LPITEM> item_gets (0);
						int count = 0;

						if (((item->GetVnum() == ITEM_WHITEDAY_CANDY) && (SEX_MALE == GET_SEX (this))) ||
						((item->GetVnum() == ITEM_WHITEDAY_ROSE) && (SEX_FEMALE == GET_SEX (this))))
						{

							ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼ºº°ÀÌ ¸ÂÁö¾Ê¾Æ ÀÌ ¾ÆÀÌÅÛÀ» ¿­ ¼ö ¾ø½À´Ï´Ù."));
							return false;
						}


						if (GiveItemFromSpecialItemGroup (dwBoxVnum, dwVnums, dwCounts, item_gets, count))
						{
							item->SetCount (item->GetCount() - 1);
						}
					}
					break;

					//70014 ÖØÉúÒ©Ë®
					case ITEM_GIVE_STAT_RESET_COUNT_VNUM:
					{
						PointChange (POINT_STAT_RESET_COUNT, 1);
						item->SetCount (item->GetCount() - 1);
					}
					break;
					
					case 50107:
					{
						if (CArenaManager::instance().IsArenaMap (GetMapIndex()) == true)
						{
							ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
							return false;
						}
						EffectPacket (SE_CHINA_FIREWORK);
						AddAffect (AFFECT_CHINA_FIREWORK, POINT_STUN_PCT, 30, AFF_CHINA_FIREWORK, 5 * 60, 0, true);
						item->SetCount (item->GetCount() - 1);
					}
					break;

					case 50108:
					{
						if (CArenaManager::instance().IsArenaMap (GetMapIndex()) == true)
						{
							ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
							return false;
						}
						EffectPacket (SE_SPIN_TOP);
						AddAffect (AFFECT_CHINA_FIREWORK, POINT_STUN_PCT, 30, AFF_CHINA_FIREWORK, 5 * 60, 0, true);
						item->SetCount (item->GetCount() - 1);
					}
					break;

					case ITEM_WONSO_BEAN_VNUM:
						PointChange (POINT_HP, GetMaxHP() - GetHP());
						item->SetCount (item->GetCount() - 1);
						break;

					case ITEM_WONSO_SUGAR_VNUM:
						PointChange (POINT_SP, GetMaxSP() - GetSP());
						item->SetCount (item->GetCount() - 1);
						break;

					case ITEM_WONSO_FRUIT_VNUM:
						PointChange (POINT_STAMINA, GetMaxStamina() - GetStamina());
						item->SetCount (item->GetCount() - 1);
						break;

					//50026 ½ð±Ò
					case ITEM_ELK_VNUM:
					{
						int iGold = item->GetSocket (0);
						// ITEM_MANAGER::instance().RemoveItem (item);
						item->SetCount (item->GetCount() - 1);
						ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("µ· %d ³ÉÀ» È¹µæÇß½À´Ï´Ù."), iGold);
						PointChange (POINT_GOLD, iGold);
					}
					break;

					case ITEM_AUTO_HP_RECOVERY_S:
					case ITEM_AUTO_HP_RECOVERY_M:
					case ITEM_AUTO_HP_RECOVERY_L:
					case ITEM_AUTO_HP_RECOVERY_X:
					case ITEM_AUTO_SP_RECOVERY_S:
					case ITEM_AUTO_SP_RECOVERY_M:
					case ITEM_AUTO_SP_RECOVERY_L:
					case ITEM_AUTO_SP_RECOVERY_X:


					case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_XS:
					case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_S:
					case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_XS:
					case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_S:
					case FUCKING_BRAZIL_ITEM_AUTO_SP_RECOVERY_S:
					case FUCKING_BRAZIL_ITEM_AUTO_HP_RECOVERY_S:
					{
						if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
							return false;
						}


						EAffectTypes type = AFFECT_NONE;
						bool isSpecialPotion = false;

						switch (item->GetVnum())
						{
						case ITEM_AUTO_HP_RECOVERY_X:
							isSpecialPotion = true;

						case ITEM_AUTO_HP_RECOVERY_S:
						case ITEM_AUTO_HP_RECOVERY_M:
						case ITEM_AUTO_HP_RECOVERY_L:
						case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_XS:
						case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_S:
						case FUCKING_BRAZIL_ITEM_AUTO_HP_RECOVERY_S:
							type = AFFECT_AUTO_HP_RECOVERY;
							break;

						case ITEM_AUTO_SP_RECOVERY_X:
							isSpecialPotion = true;

						case ITEM_AUTO_SP_RECOVERY_S:
						case ITEM_AUTO_SP_RECOVERY_M:
						case ITEM_AUTO_SP_RECOVERY_L:
						case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_XS:
						case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_S:
						case FUCKING_BRAZIL_ITEM_AUTO_SP_RECOVERY_S:
							type = AFFECT_AUTO_SP_RECOVERY;
							break;
						}

						if (AFFECT_NONE == type)
							break;

						if (item->GetCount() > 1)
						{
							int pos = GetEmptyInventory(item->GetSize());

							if (-1 == pos)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÁöÇ°¿¡ ºó °ø°£ÀÌ ¾ø½À´Ï´Ù."));
								break;
							}

							item->SetCount(item->GetCount() - 1);

							LPITEM item2 = ITEM_MANAGER::instance().CreateItem(item->GetVnum(), 1);
							item2->AddToCharacter(this, TItemPos(INVENTORY, pos));

							if (item->GetSocket(1) != 0)
							{
								item2->SetSocket(1, item->GetSocket(1));
							}

							item = item2;
						}

						CAffect* pAffect = FindAffect(type);

						if (NULL == pAffect)
						{
							EPointTypes bonus = POINT_NONE;

							if (true == isSpecialPotion)
							{
								if (type == AFFECT_AUTO_HP_RECOVERY)
								{
									bonus = POINT_MAX_HP_PCT;
								}
								else if (type == AFFECT_AUTO_SP_RECOVERY)
								{
									bonus = POINT_MAX_SP_PCT;
								}
							}

							AddAffect(type, bonus, 4, item->GetID(), INFINITE_AFFECT_DURATION, 0, true, false);

							item->Lock(true);
							item->SetSocket(0, true);

							AutoRecoveryItemProcess(type);
						}
						else
						{
							if (item->GetID() == pAffect->dwFlag)
							{
								RemoveAffect(pAffect);

								item->Lock(false);
								item->SetSocket(0, false);
							}
							else
							{
								LPITEM old = FindItemByID(pAffect->dwFlag);

								if (NULL != old)
								{
									old->Lock(false);
									old->SetSocket(0, false);
								}

								RemoveAffect(pAffect);

								EPointTypes bonus = POINT_NONE;

								if (true == isSpecialPotion)
								{
									if (type == AFFECT_AUTO_HP_RECOVERY)
									{
										bonus = POINT_MAX_HP_PCT;
									}
									else if (type == AFFECT_AUTO_SP_RECOVERY)
									{
										bonus = POINT_MAX_SP_PCT;
									}
								}

								AddAffect(type, bonus, 4, item->GetID(), INFINITE_AFFECT_DURATION, 0, true, false);

								item->Lock(true);
								item->SetSocket(0, true);

								AutoRecoveryItemProcess(type);
							}
						}
					}
					break;
#ifdef ENABLE_NEW_PET_SYSTEM
					//Pet Item
					case 55701:
					{
						CNewPet* newPet = GetNewPetSystem();
						if (!newPet)
							return false;

						LPITEM petItem = GetWear(WEAR_NEW_PET);
						if (petItem)
						{
							if (petItem == item)
							{
								if (UnequipItem(item))
								{
									newPet->PetEquip(item, false);
								}
							}
							else
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("UNEQUIP_YOUR_PET"));
								return false;
							}
						}
						else
						{
							if (EquipItem(item))
							{
								newPet->PetEquip(item, true);
							}
						}
						return true;
					}

					case 55401:	//Pet egg
					{
						CNewPet* newPet = GetNewPetSystem();
						if (!newPet)
							return false;

						newPet->EggOpen(item);
						return true;
					}

					//Pet Feed Items
					case 55101:
					case 55102:
					case 55103:
					case 55104:
					case 55105:
					case 55106:
					{
						//55-120
						return false;	
						CNewPet* newPet = GetNewPetSystem();
						if (!newPet)
							return false;
						newPet->Feed(item);
						return true;
					}
					case 55107:
					{
						CNewPet* newPet = GetNewPetSystem();
						if (!newPet)
							return false;
						newPet->IncreaseSkillSlot(item);
						return true;
					}
					
					case 55150:
					case 55151:
					case 55152:
					case 55153:
					case 55154:
					case 55155:
					case 55156:
					case 55157:
					case 55158:
					case 55159:
					case 55160:
					case 55161:
					case 55162:
					case 55163:
					case 55164:
					case 55165:
					{
						CNewPet* newPet = GetNewPetSystem();
						if (!newPet)
							return false;
						// newPet->SetSkill(item);
						// return true;
						if (number(1, 100) <= 30)
						{
							newPet->SetSkill(item);
							item->SetCount(item->GetCount() - 1);
							ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Successful cultivation"));
						}
						else
						{
							item->SetCount(item->GetCount() - 1);
							ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("The practice failed"));
						}
						return true;
					}
#endif

#ifdef ENABLE_PREMIUM_MEMBER_SYSTEM
					case 37000:
					case 37001:
					case 37002:
					case 37003:
					case 37004:
					case 37005:
					case 37006:
					case 37007:
					{
						if (FindAffect(AFFECT_PREMIUM))
						{
							ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("PREMIUM_ALREADY_MEMBER"));
							return false;
						}

						long time{ item->GetValue(0) };

						if (item->GetVnum() == 37003)
							time = INFINITE_AFFECT_DURATION_REALTIME;
						else if (item->GetVnum() == 37007)
							time = INFINITE_AFFECT_DURATION_REALTIME;

						AddAffect(AFFECT_PREMIUM, POINT_NONE, 0, AFF_PREMIUM, time, 0, false);
#ifdef ENABLE_ADD_VIP_GIVEITEM
						AutoGiveItem(90002);
#endif
						item->SetCount(item->GetCount() - 1);
						UpdatePacket();


						WarpSet(GetX(), GetY());
						Stop();
					}
					break;

#endif

#ifdef ENABLE_AUTO_PICK_UP
					case 37008:
					case 37009:
					case 37010:
					case 37011:
					case 37012:
					case 37013:
					case 37014:
					case 37015:
					{
						if (FindAffect(AFFECT_AUTO_PICK_UP))
							return false;

						long time{ item->GetValue(0) };

						if (item->GetVnum() == 37011)
							time = INFINITE_AFFECT_DURATION_REALTIME;
						else if (item->GetVnum() == 37015)
							time = INFINITE_AFFECT_DURATION_REALTIME;

						AddAffect(AFFECT_AUTO_PICK_UP, POINT_NONE, 0, 0, time, 1, false);
						item->SetCount(item->GetCount() - 1);
						UpdatePacket();

						WarpSet(GetX(), GetY());
						Stop();

					}
					break;
#endif

#ifdef ENABLE_BUFFI_SYSTEM
					case 71992:
					case 71993:
					case 71994:
					case 71995:
					case 71996:
					case 71997:
					case 71998:
					case 71999:
					case 64005:
					case 64010:
					{
						if (!IsActivateTime(BUFFI_SUMMON_TIME, 5))
						{
							ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("You must wait a bit to perform this action."));
							return false;
						}
			
						if (GetQuestFlag("buffi.summon"))
						{
							if (!item->GetSocket(3))
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
								return false;
							}
							else
							{
								if (GetBuffiSystem())
									GetBuffiSystem()->UnSummon();

								item->Lock(false);
								item->SetSocket(3, false);
								SetQuestFlag("buffi.summon", 0);
								SetQuestFlag("buffi.itemid", 0);
							}
						}
						else
						{
							if (!GetBuffiSystem())
								return false;

							if (item->GetVnum() == 71999 || item->GetVnum() == 71995)
							{
								SetQuestFlag("buffi.summon", 31);
							}
							else
							{
								if (get_global_time() >= item->GetSocket(0))
									return false;

								SetQuestFlag("buffi.summon", static_cast<int>(item->GetSocket(0)));
							}

							SetQuestFlag("buffi.itemid", static_cast<int>(item->GetID()));
							item->Lock(true);
							item->SetSocket(3, true);
							GetBuffiSystem()->Summon();
						}
						SetActivateTime(BUFFI_SUMMON_TIME);

						break;
					}
#endif
#ifdef ENABLE_ADDSTONE_NOT_FAIL_ITEM
					case 24600:
					{
						if (FindAffect(AFFECT_ADDSTONE_SUCCESS_ITEM_USED))
						{
							ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
							return false;
						}

						long time = INFINITE_AFFECT_DURATION_REALTIME;
						AddAffect(AFFECT_ADDSTONE_SUCCESS_ITEM_USED, POINT_NONE, 0, 0, time, 0, false);
						item->SetCount(item->GetCount() - 1);
						UpdatePacket();
					}
					break;
#endif

#ifdef ENABLE_METIN_QUEUE
					case 37016:
					case 37017:
					case 37018:
					case 37019:
					case 37020:
					case 37021:
					case 37022:
					case 37023:
					{
						if (FindAffect(AFFECT_METIN_QUEUE))
						{
							ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
							return false;
						}

						long time = 0;

						switch (item->GetVnum())
						{

						case 37016:
						case 37020:
						{
							time = (86400*7);
							break;
						}


						case 37017:
						case 37021:
						{
							time = (86400 * 14);
							break;
						}


						case 37018:
						case 37022:
						{
							time = (86400 * 30);
							break;
						}

						case 37019:
						case 37023:
						{
							time = INFINITE_AFFECT_DURATION_REALTIME;
							break;
						}

						default:
							return false;
						}

						AddAffect(AFFECT_METIN_QUEUE, POINT_NONE, 0, AFF_METIN_QUEUE, time, 0, false);
						item->SetCount(item->GetCount() - 1);
						UpdatePacket();
					}
					break;

#endif

#ifdef ENABLE_6TH_7TH_ATTR
					case 24601:
					case 24602:
					{
						Decrease67AttrCooldown(item);
						break;
					}
#ifdef ENABLE_EVENT_MANAGER
					case 79505:
					case 79512:
					case 79603:
					case 50096:
					{
						ConvertEventItems(item);
						break;
					}
#endif
					
#endif
					default: break;
					}//switch getvnum
				}

				case USE_POTION_NODELAY:
				{
					if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
					{
						if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit") > 0)
						{
							ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
							return false;
						}

						switch (item->GetVnum())
						{
						case 70020:
						case 71018:
						case 71019:
						case 71020:
						{
							if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit_count") < 10000)
							{
								if (m_nPotionLimit <= 0)
								{
									ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("»ç¿ë Á¦ÇÑ·®À» ÃÊ°úÇÏ¿´½À´Ï´Ù."));
									return false;
								}
							}
							break;
						}
						default:
						{
							ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
							return false;
						}
						}
					}
					bool used = false;

					if (item->GetValue(0) != 0)
					{
						if (GetHP() < GetMaxHP())
						{
							PointChange(POINT_HP, item->GetValue(0) * (100 + GetPoint(POINT_POTION_BONUS)) / 100);
							EffectPacket(SE_HPUP_RED);
							used = TRUE;
						}
					}

					if (item->GetValue(1) != 0)
					{
						if (GetSP() < GetMaxSP())
						{
							PointChange(POINT_SP, item->GetValue(1) * (100 + GetPoint(POINT_POTION_BONUS)) / 100);
							EffectPacket(SE_SPUP_BLUE);
							used = TRUE;
						}
					}

					if (item->GetValue(3) != 0)
					{
						if (GetHP() < GetMaxHP())
						{
							PointChange(POINT_HP, item->GetValue(3) * GetMaxHP() / 100);
							EffectPacket(SE_HPUP_RED);
							used = TRUE;
						}
					}

					if (item->GetValue(4) != 0)
					{
						if (GetSP() < GetMaxSP())
						{
							PointChange(POINT_SP, item->GetValue(4) * GetMaxSP() / 100);
							EffectPacket(SE_SPUP_BLUE);
							used = TRUE;
						}
					}

					if (used)
					{
						if (GetDungeon())
							GetDungeon()->UsePotion(this);

						if (GetWarMap())
							GetWarMap()->UsePotion(this, item);

						m_nPotionLimit--;

						//RESTRICT_USE_SEED_OR_MOONBOTTLE
						item->SetCount(item->GetCount() - 1);
						//END_RESTRICT_USE_SEED_OR_MOONBOTTLE
					}
					break;
				}

				if (item->GetVnum() >= 27863 && item->GetVnum() <= 27883)
				{
					if (CArenaManager::instance().IsArenaMap (GetMapIndex()) == true)
					{
						ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
						return false;
					}

				}

				case USE_POTION:
				{
					if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
					{
						if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit") > 0)
						{
							ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
							return false;
						}

						switch (item->GetVnum())
						{
							case 27001:
							case 27002:
							case 27003:
							case 27004:
							case 27005:
							case 27006:
							{
								if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit_count") < 10000)
								{
									if (m_nPotionLimit <= 0)
									{
										ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("»ç¿ë Á¦ÇÑ·®À» ÃÊ°úÇÏ¿´½À´Ï´Ù."));
										return false;
									}
								}
								break;
							}

							default:
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
								return false;
							}
						}
					}

					if (item->GetValue(1) != 0)
					{
						if (GetPoint(POINT_SP_RECOVERY) + GetSP() >= GetMaxSP())
						{
							return false;
						}

						PointChange(POINT_SP_RECOVERY, item->GetValue(1) * MIN(200, (100 + GetPoint(POINT_POTION_BONUS))) / 100);
						StartAffectEvent();
						EffectPacket(SE_SPUP_BLUE);
					}

					if (item->GetValue(0) != 0)
					{
						if (GetPoint(POINT_HP_RECOVERY) + GetHP() >= GetMaxHP())
						{
							return false;
						}

						PointChange(POINT_HP_RECOVERY, item->GetValue(0) * MIN(200, (100 + GetPoint(POINT_POTION_BONUS))) / 100);
						StartAffectEvent();
						EffectPacket(SE_HPUP_RED);
					}

					if (GetDungeon())
						GetDungeon()->UsePotion(this);

					if (GetWarMap())
						GetWarMap()->UsePotion(this, item);

					item->SetCount(item->GetCount() - 1);
					m_nPotionLimit--;
					break;
				}

				case USE_POTION_CONTINUE:
				{
					if (item->GetValue(0) != 0)
					{
						AddAffect(AFFECT_HP_RECOVER_CONTINUE, POINT_HP_RECOVER_CONTINUE, item->GetValue(0), 0, item->GetValue(2), 0, true);
					}
					else if (item->GetValue(1) != 0)
					{
						AddAffect(AFFECT_SP_RECOVER_CONTINUE, POINT_SP_RECOVER_CONTINUE, item->GetValue(1), 0, item->GetValue(2), 0, true);
					}
					else
						return false;

					if (GetDungeon())
						GetDungeon()->UsePotion(this);

					if (GetWarMap())
						GetWarMap()->UsePotion(this, item);

					item->SetCount(item->GetCount() - 1);
					break;
				}

				case USE_ABILITY_UP:
				{
					switch (item->GetValue(0))
					{
						case APPLY_MOV_SPEED:
						{
							if (FindAffect(AFFECT_MOV_SPEED))//@Lightwork#13
								return false;
							AddAffect(AFFECT_MOV_SPEED, POINT_MOV_SPEED, item->GetValue(2), AFF_MOV_SPEED_POTION, item->GetValue(1), 0, true);
#ifdef ENABLE_EFFECT_EXTRAPOT
							EffectPacket(SE_DXUP_PURPLE);
#endif
							break;
						}

						case APPLY_ATT_SPEED:
						{
							if (FindAffect(AFFECT_ATT_SPEED))//@Lightwork#13
								return false;
							AddAffect(AFFECT_ATT_SPEED, POINT_ATT_SPEED, item->GetValue(2), AFF_ATT_SPEED_POTION, item->GetValue(1), 0, true);
#ifdef ENABLE_EFFECT_EXTRAPOT
							EffectPacket(SE_SPEEDUP_GREEN);
#endif
							break;
						}

						case APPLY_STR:
						{
							AddAffect(AFFECT_STR, POINT_ST, item->GetValue(2), 0, item->GetValue(1), 0, true);
							break;
						}

						case APPLY_DEX:
						{
							AddAffect(AFFECT_DEX, POINT_DX, item->GetValue(2), 0, item->GetValue(1), 0, true);
							break;
						}

						case APPLY_CON:
						{
							AddAffect(AFFECT_CON, POINT_HT, item->GetValue(2), 0, item->GetValue(1), 0, true);
							break;
						}

						case APPLY_INT:
						{
							AddAffect(AFFECT_INT, POINT_IQ, item->GetValue(2), 0, item->GetValue(1), 0, true);
							break;
						}

						case APPLY_CAST_SPEED:
						{
							AddAffect(AFFECT_CAST_SPEED, POINT_CASTING_SPEED, item->GetValue(2), 0, item->GetValue(1), 0, true);
							break;
						}

						case APPLY_ATT_GRADE_BONUS:
						{
							AddAffect(AFFECT_ATT_GRADE, POINT_ATT_GRADE_BONUS,item->GetValue(2), 0, item->GetValue(1), 0, true);
							break;
						}

						case APPLY_DEF_GRADE_BONUS:
						{
							AddAffect(AFFECT_DEF_GRADE, POINT_DEF_GRADE_BONUS,item->GetValue(2), 0, item->GetValue(1), 0, true);
							break;
						}
					}
					if (GetDungeon())
						GetDungeon()->UsePotion(this);

					if (GetWarMap())
						GetWarMap()->UsePotion(this, item);

					item->SetCount(item->GetCount() - 1);
					break;
				}
				//¼ÇÒä¾í
				case USE_TALISMAN:
				{
					const int TOWN_PORTAL = 1;
					const int MEMORY_PORTAL = 2;

					if (GetMapIndex() == 200 || GetMapIndex() == 113 || GetMapIndex() == 195)
					{
						ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÇöÀç À§Ä¡¿¡¼­ »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
						return false;
					}

					if (CArenaManager::instance().IsArenaMap (GetMapIndex()) == true)
					{
						ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
						return false;
					}

					if (m_pkWarpEvent)
					{
						ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀÌµ¿ÇÒ ÁØºñ°¡ µÇ¾îÀÖÀ½À¸·Î ±ÍÈ¯ºÎ¸¦ »ç¿ëÇÒ¼ö ¾ø½À´Ï´Ù"));
						return false;
					}

					int consumeLife = CalculateConsume (this);

					if (consumeLife < 0)
					{
						return false;
					}

					if (item->GetValue (0) == TOWN_PORTAL)
					{
						if (item->GetSocket (0) == 0)
						{
							if (!GetDungeon())
								if (!GiveRecallItem (item))
								{
									return false;
								}

							PIXEL_POSITION posWarp;

							if (SECTREE_MANAGER::instance().GetRecallPositionByEmpire (GetMapIndex(), GetEmpire(), posWarp))
							{
								PointChange (POINT_HP, -consumeLife, false);
								WarpSet (posWarp.x, posWarp.y);
							}
							else
							{
								sys_err ("CHARACTER::UseItem : cannot find spawn position (name %s, %d x %d)", GetName(), GetX(), GetY());
							}
						}
						else
						{
							if (test_server)
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¿ø·¡ À§Ä¡·Î º¹±Í"));
							}

							ProcessRecallItem (item);
						}
					}
					else if (item->GetValue (0) == MEMORY_PORTAL)
					{
						if (item->GetSocket (0) == 0)
						{
							if (GetDungeon())
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("´øÀü ¾È¿¡¼­´Â %s%s »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."), item->GetName(), "");
								return false;
							}

							if (!GiveRecallItem (item))
							{
								return false;
							}
						}
						else
						{
							PointChange (POINT_HP, -consumeLife, false);
							ProcessRecallItem (item);
						}
					}
				}
				break;

				case USE_TUNING:
				case USE_DETACHMENT:
				{
					LPITEM item2;

					if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
						return false;

					if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
						return false;

					if (item2->GetVnum() >= 28330 && item2->GetVnum() <= 28343)
					{
						ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("+3 ¿µ¼®Àº ÀÌ ¾ÆÀÌÅÛÀ¸·Î °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù"));
						return false;
					}

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
					if (item->GetValue(0) == ACCE_CLEAN_ATTR_VALUE0
						|| item->GetVnum() == ACCE_REVERSAL_VNUM_1
						|| item->GetVnum() == ACCE_REVERSAL_VNUM_2
						)
					{
						if (item2->GetSubType() == COSTUME_ACCE)
						{
							if (!CleanAcceAttr(item, item2))
								return false;
						}

#ifdef ENABLE_AURA_SYSTEM
						else if (item2->GetSubType() == COSTUME_AURA)
						{
							if (!CleanAuraAttr(item, item2))
								return false;
						}
#endif
						item->SetCount(item->GetCount() - 1);
						return true;
					}
#endif
					if (item2->GetVnum() >= 28630 && item2->GetVnum() <= 28643)
					{
						switch (item->GetVnum())
						{
							case 70028:
							case 25040:
							case 72301:
							case 76016:
							case 70039:
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¿µ¼®Àº ÀÌ ¾ÆÀÌÅÛÀ¸·Î °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù2"));
								return false;
							}
							break;
						}
					}

					if (item2->GetVnum() >= 28430 && item2->GetVnum() <= 28443)
					{
						if (item->GetVnum() == 71056)
						{
							RefineItem(item, item2);
						}
						else
						{
							ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¿µ¼®Àº ÀÌ ¾ÆÀÌÅÛÀ¸·Î °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù"));
						}
					}
					else
					{
						RefineItem(item, item2);
					}
					break;
				}
				//70063 or 70064
				//Ê±×°ÊôÐÔ×Ô¶¨Òå°æ
				case USE_CHANGE_COSTUME_ATTR:
				case USE_RESET_COSTUME_ATTR:
				{
					LPITEM item2;
					if (!IsValidItemPosition (DestCell) || ! (item2 = GetItem (DestCell)))
					{
						return false;
					}

					if (item2->IsEquipped())
					{
						BuffOnAttr_RemoveBuffsFromItem (item2);
					}

					if (ITEM_COSTUME != item2->GetType())
					{
						ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
						return false;
					}

					if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
					{
						return false;
					}

					if (item2->GetAttributeSetIndex() == -1)
					{
						ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
						return false;
					}

					switch (item->GetSubType())
					{	//Ê±×°×ª»»ÃØ¼®
						case USE_CHANGE_COSTUME_ATTR:
						{
							if (item2->GetAttributeCount() == 0)
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("º¯°æÇÒ ¼Ó¼ºÀÌ ¾ø½À´Ï´Ù2."));
								return false;
							}

							item2->ChangeAttribute (aiCostumeAttributeLevelPercent);
							ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼Ó¼ºÀ» º¯°æÇÏ¿´½À´Ï´Ù."));
							item->SetCount (item->GetCount() - 1);
						}
						break;
						//Ê±×°³õÊ¼»¯ÃØ¼® ÐÂ
						case USE_RESET_COSTUME_ATTR:
						{
							item2->ClearAttribute();
							BYTE i;
							for (i = 0; i < COSTUME_ATTRIBUTE_MAX_NUM; i++)
							{
								if (number (1, 100) <= aiCostumeAttributeAddPercent[item2->GetAttributeCount()])
								{
									item2->AddAttribute();
								}
								else
								{
									break;
								}
							}

							ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼Ó¼ºÀ» º¯°æÇÏ¿´½À´Ï´Ù."));
							item->SetCount (item->GetCount() - 1);
						}
					}
				}
				break;

#ifdef ENABLE_COSTUME_ATTR
				case USE_CHANGE_ATTRIBUTE2:
				{
					LPITEM item2;
					if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
						return false;

					if (item2->IsExchanging() || item2->IsEquipped())
						return false;

					if (ITEM_COSTUME != item2->GetType())
					{
						ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
						return false;
					}

					if (item2->GetAttributeSetIndex(true) != ATTRIBUTE_SET_RARE_COSTUME)
					{
						ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
						return false;
					}

					if (item2->GetLimitType(0) == LIMIT_REAL_TIME || item2->GetLimitType(1) == LIMIT_REAL_TIME)
					{
						ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Fashion that cannot change the time system"));
						return false;
					}

					ChangeCostumeAttr(item2, item);
					return true;
				}
#endif
				//  ACCESSORY_REFINE & ADD/CHANGE_ATTRIBUTES
				case USE_PUT_INTO_BELT_SOCKET:
				case USE_PUT_INTO_ACCESSORY_SOCKET:
				case USE_ADD_ACCESSORY_SOCKET:
				case USE_CLEAN_SOCKET:
				case USE_CHANGE_ATTRIBUTE:
				case USE_ADD_ATTRIBUTE:
				case USE_ADD_ATTRIBUTE2:
				{
					LPITEM item2;
					if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
						return false;

					if (ITEM_COSTUME == item2->GetType())
					{
						ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
						return false;
					}

					if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
					{
						return false;
					}
					/*if (item2->IsEquipped())
					{
						BuffOnAttr_RemoveBuffsFromItem(item2);
					}*/

					switch (item->GetSubType())
					{
						case USE_CLEAN_SOCKET:
						{
							int i;
							for (i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
							{
								if (item2->GetSocket(i) == ITEM_BROKEN_METIN_VNUM)
									break;
							}

							if (i == ITEM_SOCKET_MAX_NUM)
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Ã»¼ÒÇÒ ¼®ÀÌ ¹ÚÇôÀÖÁö ¾Ê½À´Ï´Ù."));
								return false;
							}

							int j = 0;

							for (i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
							{
								if (item2->GetSocket(i) != ITEM_BROKEN_METIN_VNUM && item2->GetSocket(i) != 0)
									item2->SetSocket(j++, item2->GetSocket(i));
							}

							for (; j < ITEM_SOCKET_MAX_NUM; ++j)
							{
								if (item2->GetSocket(j) > 0)
									item2->SetSocket(j, 1);
							}

							item->SetCount(item->GetCount() - 1);
							break;
						}

						//71084
						case USE_CHANGE_ATTRIBUTE:
						{
#ifdef ENABLE_6TH_7TH_ATTR
							if (item->GetVnum() == 72346 || item->GetVnum() == 71052 || item->GetVnum() == 72351)
							{
								Change67Attr(item, item2);
								return true;
							}

#endif
							if (item2->GetAttributeSetIndex() == -1)
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
								return false;
							}

							if (item2->GetAttributeCount() == 0)
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("NO_ATTRIBUTES_TO_CHANE_ON_THE_ITEM"));
								return false;
							}

							//@Lightwork#140_START
							if (BLOCKED_ATTR_LIST(item2->GetVnum()))
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
								return false;
							}
							//@Lightwork#140_END


#ifdef ENABLE_GREEN_ATTRIBUTE_CHANGER
							if (item->GetVnum() == 71151)
							{
								if ((item2->GetType() == ITEM_WEAPON)
									|| (item2->GetType() == ITEM_ARMOR && item2->GetSubType() == ARMOR_BODY))
								{
									bool bCanUse = true;
									for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
									{
										if (item2->GetLimitType(i) == LIMIT_LEVEL && item2->GetLimitValue(i) > 40)
										{
											bCanUse = false;
											break;
										}
									}
									if (false == bCanUse)
									{
										ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Àû¿ë ·¹º§º¸´Ù ³ô¾Æ »ç¿ëÀÌ ºÒ°¡´ÉÇÕ´Ï´Ù."));
										break;
									}
								}
								else
								{
									ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¹«±â¿Í °©¿Ê¿¡¸¸ »ç¿ë °¡´ÉÇÕ´Ï´Ù."));
									break;
								}
							}
#endif
							item2->ChangeAttribute();
#ifdef ENABLE_PLAYER_STATS_SYSTEM
							PointChange(POINT_USE_ENCHANTED_ITEM, 1);
#endif
							ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼Ó¼ºÀ» º¯°æÇÏ¿´½À´Ï´Ù."));
							item->SetCount(item->GetCount() - 1);
							break;
						}

					 //71085 ÎïÆ·ÊôÐÔ×·¼Ó
					case USE_ADD_ATTRIBUTE:
						if (item2->GetAttributeSetIndex() == -1)
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
							return false;
						}
						if (item->GetVnum() == 71051)
						{
							Add67Attr(item, item2);
							return true;
						}
						if (item2->GetAttributeCount() < 4)
						{
							if (item->GetVnum() == 71152 || item->GetVnum() == 76024)
							{
								if ((item2->GetType() == ITEM_WEAPON) || (item2->GetType() == ITEM_ARMOR && item2->GetSubType() == ARMOR_BODY))
								{
									bool bCanUse = true;
									for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
									{
										if (item2->GetLimitType(i) == LIMIT_LEVEL && item2->GetLimitValue(i) > 40)
										{
											bCanUse = false;
											break;
										}
									}
									if (false == bCanUse)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Àû¿ë ·¹º§º¸´Ù ³ô¾Æ »ç¿ëÀÌ ºÒ°¡´ÉÇÕ´Ï´Ù."));
										break;
									}
								}
								else
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¹«±â¿Í °©¿Ê¿¡¸¸ »ç¿ë °¡´ÉÇÕ´Ï´Ù."));
									break;
								}
							}
							
							if (number (1, 100) <= aiItemAttributeAddPercent[item2->GetAttributeCount()])
							{
								item2->AddAttribute();
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼º Ãß°¡¿¡ ¼º°øÇÏ¿´½À´Ï´Ù."));
							}
							else
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼º Ãß°¡¿¡ ½ÇÆÐÇÏ¿´½À´Ï´Ù."));
							}
							
								item->SetCount(item->GetCount() - 1);
						}
						else
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´õ ÀÌ»ó ÀÌ ¾ÆÀÌÅÛÀ» ÀÌ¿ëÇÏ¿© ¼Ó¼ºÀ» Ãß°¡ÇÒ ¼ö ¾ø½À´Ï´Ù."));
						}
						break;

						//×£¸£Ö®Öé70024
						case USE_ADD_ATTRIBUTE2:
						{
							if (item2->GetAttributeSetIndex() == -1)
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
								return false;
							}

							if (item2->GetAttributeCount() == 4)
							{
								if (number(1, 100) <= aiItemAttributeAddPercent[item2->GetAttributeCount()])
								{
									item2->AddAttribute();
									ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼Ó¼º Ãß°¡¿¡ ¼º°øÇÏ¿´½À´Ï´Ù."));
								}
								else
								{
									ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼Ó¼º Ãß°¡¿¡ ½ÇÆÐÇÏ¿´½À´Ï´Ù."));
								}

								item->SetCount(item->GetCount() - 1);
							}
							else if (item2->GetAttributeCount() == 5)
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("´õ ÀÌ»ó ÀÌ ¾ÆÀÌÅÛÀ» ÀÌ¿ëÇÏ¿© ¼Ó¼ºÀ» Ãß°¡ÇÒ ¼ö ¾ø½À´Ï´Ù."));
							}
							else if (item2->GetAttributeCount() < 4)
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¸ÕÀú Àç°¡ºñ¼­¸¦ ÀÌ¿ëÇÏ¿© ¼Ó¼ºÀ» Ãß°¡½ÃÄÑ ÁÖ¼¼¿ä."));
							}
							else
							{
								sys_err("ADD_ATTRIBUTE2 : Item has wrong AttributeCount(%d) item id %d owner %s itemvnum %d", item2->GetAttributeCount(), item2->GetID(), item2->GetOwner()->GetName(), item2->GetVnum());
							}
							break;
						}
						//×êÊ¯
						case USE_ADD_ACCESSORY_SOCKET:
						{
#ifdef ENABLE_JEWELS_RENEWAL
							UseAddJewels(item, item2);
							return true;
#endif
							if (item2->IsAccessoryForSocket())
							{
								if (item2->GetAccessorySocketMaxGrade() < ITEM_ACCESSORY_SOCKET_MAX_NUM)
								{
#ifdef ENABLE_ADDSTONE_FAILURE
									if (number(1, 100) <= 50)	//¿óÊ¯¿ª¿×
#else
									if (1)
#endif
									{
										item2->SetAccessorySocketMaxGrade(item2->GetAccessorySocketMaxGrade() + 1);
										ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼ÒÄÏÀÌ ¼º°øÀûÀ¸·Î Ãß°¡µÇ¾ú½À´Ï´Ù."));
									}
									else
									{
										ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼ÒÄÏ Ãß°¡¿¡ ½ÇÆÐÇÏ¿´½À´Ï´Ù."));
									}

									item->SetCount(item->GetCount() - 1);
								}
								else
								{
									ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀÌ ¾×¼¼¼­¸®¿¡´Â ´õÀÌ»ó ¼ÒÄÏÀ» Ãß°¡ÇÒ °ø°£ÀÌ ¾ø½À´Ï´Ù."));
								}
							}
							else
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀÌ ¾ÆÀÌÅÛÀ¸·Î ¼ÒÄÏÀ» Ãß°¡ÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
							}
							break;
						}

						case USE_PUT_INTO_BELT_SOCKET:
						case USE_PUT_INTO_ACCESSORY_SOCKET:
						{
#ifdef ENABLE_JEWELS_RENEWAL
							UsePutIntoJewels(item, item2);
							return true;
#endif
							if (item2->IsAccessoryForSocket() && item->CanPutInto(item2))
							{
								if (item2->GetAccessorySocketGrade() < item2->GetAccessorySocketMaxGrade())
								{
									if (number(1, 100) <= aiAccessorySocketPutPct[item2->GetAccessorySocketGrade()])
									{
										item2->SetAccessorySocketGrade(item2->GetAccessorySocketGrade() + 1);
										ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀåÂø¿¡ ¼º°øÇÏ¿´½À´Ï´Ù."));
									}
									else
									{
										ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀåÂø¿¡ ½ÇÆÐÇÏ¿´½À´Ï´Ù."));
									}

									item->SetCount(item->GetCount() - 1);
								}
								else
								{
									if (item2->GetAccessorySocketMaxGrade() == 0)
										ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¸ÕÀú ´ÙÀÌ¾Æ¸óµå·Î ¾Ç¼¼¼­¸®¿¡ ¼ÒÄÏÀ» Ãß°¡ÇØ¾ßÇÕ´Ï´Ù."));
									else if (item2->GetAccessorySocketMaxGrade() < ITEM_ACCESSORY_SOCKET_MAX_NUM)
									{
										ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀÌ ¾×¼¼¼­¸®¿¡´Â ´õÀÌ»ó ÀåÂøÇÒ ¼ÒÄÏÀÌ ¾ø½À´Ï´Ù."));
										ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("´ÙÀÌ¾Æ¸óµå·Î ¼ÒÄÏÀ» Ãß°¡ÇØ¾ßÇÕ´Ï´Ù."));
									}
									else
										ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀÌ ¾×¼¼¼­¸®¿¡´Â ´õÀÌ»ó º¸¼®À» ÀåÂøÇÒ ¼ö ¾ø½À´Ï´Ù."));
								}
							}
							else
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀÌ ¾ÆÀÌÅÛÀ» ÀåÂøÇÒ ¼ö ¾ø½À´Ï´Ù."));
							}
							break;
						}
						default: break;
					}//switch subtype

					if (item2->IsEquipped())
					{
						BuffOnAttr_AddBuffsFromItem(item2);
					}
					break;
				}

				case USE_BAIT:
				{
					if (m_pkFishingEvent)
					{
						ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("³¬½Ã Áß¿¡ ¹Ì³¢¸¦ °¥¾Æ³¢¿ï ¼ö ¾ø½À´Ï´Ù."));
						return false;
					}

					LPITEM weapon = GetWear(WEAR_WEAPON);

					if (!weapon || weapon->GetType() != ITEM_ROD)
						return false;

					if (weapon->GetSocket(2))
					{
						ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀÌ¹Ì ²ÈÇôÀÖ´ø ¹Ì³¢¸¦ »©°í %s¸¦ ³¢¿ó´Ï´Ù."), item->GetName());
					}
					else
					{
						ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("³¬½Ã´ë¿¡ %s¸¦ ¹Ì³¢·Î ³¢¿ó´Ï´Ù."), item->GetName());
					}

					weapon->SetSocket(2, item->GetValue(0));
					item->SetCount(item->GetCount() - 1);
					break;
				}
			

				case USE_AFFECT:
				{
					if (FindAffect(item->GetValue(0), aApplyInfo[item->GetValue(1)].bPointType))
					{
						ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
					}
					else
					{
						AddAffect(item->GetValue(0), aApplyInfo[item->GetValue(1)].bPointType, item->GetValue(2), 0, item->GetValue(3), 0, false);
						item->SetCount(item->GetCount() - 1);
					}
					break;
				}

#ifdef ENABLE_ATTR_RARE_RENEWAL
				//×ª»»Óë×·¼ÓÆ¤·ôÊôÐÔ,Ê±×°,»¤·û,¹â»·µÈ
				case USE_ADD_RARE_ATTRIBUTE:
				case USE_CHANGE_RARE_ATTRIBUTE:
				{
					LPITEM item2;

					if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
						return false;

					if (item2->IsEquipped() || item2->IsExchanging())
						return false;

					switch (item->GetSubType())
					{
						case USE_CHANGE_RARE_ATTRIBUTE:
						{
							int attrIndex = item2->GetAttributeSetIndex(true);
							if (attrIndex == -1)
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
								return false;
							}

							if (!item->IsRareAttrItem(attrIndex))
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
								return false;
							}

							if (!item2->ChangeRareAttribute())
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("NO_ATTRIBUTES_TO_CHANE_ON_THE_ITEM_01"));
								return false;
							}

							ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("CHANGE_ATTRIBUTE_WAS_SUCCESFULL"));
							break;
						}

						case USE_ADD_RARE_ATTRIBUTE:
						{
							int attrIndex = item2->GetAttributeSetIndex(true);
							if (attrIndex == -1)
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
								return false;
							}

							if (!item->IsRareAttrItem(attrIndex))
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
								return false;
							}
							
							if (item2->GetRareAttrCount() >= 5 )
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("NO_ATTRIBUTES_TO_CHANE_ON_THE_ITEM"));
								return false;
							}

							if (number (1, 100) <= aiItemAttributeAddRareAttrItem[item2->GetRareAttrCount()])
							{
								if (!item2->AddRareAttribute())
								{
									ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("NO_ATTRIBUTES_TO_CHANE_ON_THE_ITEM"));
									return false;
								}
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT ("ATTRIBUTES_TO_CHANE_SUCCESFULL"));
								
							}
							else
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ATTRIBUTES_TO_CHANE_IT_WAS_FAILED"));
							}
							break;
						}

						default:
							return false;
					}
#ifdef ENABLE_PLAYER_STATS_SYSTEM
					PointChange(POINT_USE_ENCHANTED_ITEM, 1);
#endif
					item->SetCount(item->GetCount() - 1);
					break;
				}

			}//switch Subtype Item Type Use
			break;
		}
#endif

		case ITEM_METIN:
		{
			LPITEM item2;

			if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
				return false;

			if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
				return false;

#ifdef ENABLE_ADDSTONE_NOT_FAIL_ITEM
			const auto hasSuccesItem = FindAffect(AFFECT_ADDSTONE_SUCCESS_ITEM_USED);
#endif

			if (item2->GetType() == ITEM_PICK) return false;
			if (item2->GetType() == ITEM_ROD) return false;

			int i;

			for (i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
			{
				DWORD dwVnum;

				if ((dwVnum = item2->GetSocket(i)) <= 2)
					continue;

				TItemTable* p = ITEM_MANAGER::instance().GetTable(dwVnum);

				if (!p)
					continue;

				if (item->GetValue(5) == p->alValues[5])
				{
					ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("°°Àº Á¾·ùÀÇ ¸ÞÆ¾¼®Àº ¿©·¯°³ ºÎÂøÇÒ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
			}

			if (item2->GetType() == ITEM_ARMOR)
			{
				if (!IS_SET(item->GetWearFlag(), WEARABLE_BODY) || !IS_SET(item2->GetWearFlag(), WEARABLE_BODY))
				{
					ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀÌ ¸ÞÆ¾¼®Àº Àåºñ¿¡ ºÎÂøÇÒ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
			}
			else if (item2->GetType() == ITEM_WEAPON)
			{
				if (!IS_SET(item->GetWearFlag(), WEARABLE_WEAPON))
				{
					ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀÌ ¸ÞÆ¾¼®Àº ¹«±â¿¡ ºÎÂøÇÒ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
			}
			else
			{
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ºÎÂøÇÒ ¼ö ÀÖ´Â ½½·ÔÀÌ ¾ø½À´Ï´Ù."));
				return false;
			}

			for (i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
			{
				if (item2->GetSocket(i) >= 1 && item2->GetSocket(i) <= 2 && item2->GetSocket(i) >= item->GetValue(2))
				{
#ifdef ENABLE_ADDSTONE_FAILURE
#ifdef ENABLE_ADDSTONE_NOT_FAIL_ITEM
					if (hasSuccesItem ? 1 : (number(1, 100) <= ENABLE_ADDSTONE_FAILURE))
#else
					if (number(1, 100) <= 30)
#endif
#else
					if (1)
#endif
					{
						ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¸ÞÆ¾¼® ºÎÂø¿¡ ¼º°øÇÏ¿´½À´Ï´Ù."));
						item2->SetSocket(i, item->GetVnum());
					}
					else
					{
						ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¸ÞÆ¾¼® ºÎÂø¿¡ ½ÇÆÐÇÏ¿´½À´Ï´Ù."));
						item2->SetSocket(i, ITEM_BROKEN_METIN_VNUM);
					}
					item->SetCount(item->GetCount() - 1); // Lightwork@332 stack fix
					break;
				}
			}

#ifdef ENABLE_ADDSTONE_NOT_FAIL_ITEM
			if (hasSuccesItem)
				RemoveAffect(AFFECT_ADDSTONE_SUCCESS_ITEM_USED);
#endif

			if (i == ITEM_SOCKET_MAX_NUM)
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ºÎÂøÇÒ ¼ö ÀÖ´Â ½½·ÔÀÌ ¾ø½À´Ï´Ù."));
			break;
		}
		

		case ITEM_AUTOUSE:
		case ITEM_MATERIAL:
		case ITEM_SPECIAL:
		case ITEM_TOOL:
			break;

		case ITEM_TOTEM:
		{
			if (!item->IsEquipped())
				EquipItem(item);
			break;
		}
	

		case ITEM_BLEND:
			{
				if (Blend_Item_find(item->GetVnum()))
				{
					int affect_type = AFFECT_BLEND;
					int apply_type = aApplyInfo[item->GetSocket(0)].bPointType;
					int apply_value = item->GetSocket(1);
					int apply_duration = item->GetSocket(2);

					if (FindAffect(affect_type, apply_type))
					{
						ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
					}
					else
					{
					if (FindAffect(AFFECT_EXP_BONUS_EURO_FREE, POINT_RESIST_MAGIC))
					{
						ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
					}
					else
					{
						AddAffect(affect_type, apply_type, apply_value, 0, apply_duration, 0, false);
						item->SetCount(item->GetCount() - 1);
					}
				}
			}
		}
		break;

		case ITEM_EXTRACT:
		{
			LPITEM pDestItem = GetItem (DestCell);
			if (NULL == pDestItem)
			{
				return false;
			}
			switch (item->GetSubType())
			{
				case EXTRACT_DRAGON_SOUL:
					if (pDestItem->IsDragonSoul())
					{
						return DSManager::instance().PullOut (this, NPOS, pDestItem, item);
					}
					return false;
				case EXTRACT_DRAGON_HEART:
					if (item->GetVnum() == 71053)
					{
						if (pDestItem->IsDragonSoul())
						{
							if (DSManager::instance().IsActiveDragonSoul (pDestItem) == true)
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("SIMYA_GIYILIYOR")); //Î´´ïµ½Ê¹ÓÃÁú»êÊ¯È¨ÏÞ
								return false;
							}

							if (item->IsEquipped() || item->IsExchanging())
								return false;

							pDestItem->SetForceAttribute (0, 0, 0);
							pDestItem->SetForceAttribute (1, 0, 0);
							pDestItem->SetForceAttribute (2, 0, 0);
							pDestItem->SetForceAttribute (3, 0, 0);
							pDestItem->SetForceAttribute (4, 0, 0);
							pDestItem->SetForceAttribute (5, 0, 0);
							pDestItem->SetForceAttribute (6, 0, 0);

							bool ret = DSManager::instance().PutAttributes (pDestItem);
							if (ret == true)
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("SIMYA_DEGISTIRME_BASARILI!")); //Áú»êÊ¯ÊôÐÔ×ª»»³É¹¦
								item->SetCount (item->GetCount() - 1);
							}
							else
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("SIMYA_DEGISTIRME_BASARISIZ!"));
							}
						}
					}
					else
					{
						if (pDestItem->IsDragonSoul())
						{
							return DSManager::instance().ExtractDragonHeart (this, pDestItem, item);
						}
					}
					return false;
				default:
					return false;
			}
		}
		break;

		case ITEM_NONE:
			sys_err ("Item type NONE %s", item->GetName());
			break;

		default:
			sys_log (0, "UseItemEx: Unknown type %s %d", item->GetName(), item->GetType());
			return false;
	}

	return true;
}


int g_nPortalLimitTime = 5;
//ËÀÍöÔ­µØ¸´»îÊ±¼ä
int g_WarplLimitTime = 10;

bool CHARACTER::UseItem(TItemPos Cell, TItemPos DestCell
#ifdef ENABLE_CHEST_OPEN_RENEWAL
	, MAX_COUNT item_open_count
#endif
)
{
	LPITEM item;

	if (!CanHandleItem())
		return false;

	if (!IsValidItemPosition(Cell) || !(item = GetItem(Cell)))
		return false;

	if (item->IsExchanging())
		return false;

#ifdef ENABLE_SWITCHBOT
	if (Cell.IsSwitchbotPosition())
	{
		CSwitchbot* pkSwitchbot = CSwitchbotManager::Instance().FindSwitchbot(GetPlayerID());
		if (pkSwitchbot && pkSwitchbot->IsActive(Cell.cell))
		{
			return false;
		}

		int32_t iEmptyCell = GetEmptyInventory(item->GetSize());

		if (iEmptyCell == -1)
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼ÒÁöÇ°¿¡ ºó °ø°£ÀÌ ¾ø½À´Ï´Ù."));
			return false;
		}

		MoveItem(Cell, TItemPos(INVENTORY, iEmptyCell), item->GetCount());
		return true;
	}
#endif

#ifdef ENABLE_BUFFI_SYSTEM
	if (Cell.IsBuffiEquipPosition())
	{
		int iEmptyCell = GetEmptyInventory(item->GetSize());

		if (iEmptyCell == -1)
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼ÒÁöÇ°¿¡ ºó °ø°£ÀÌ ¾ø½À´Ï´Ù."));
			return false;
		}

		MoveItem(Cell, TItemPos(INVENTORY, iEmptyCell), item->GetCount());
		return true;
	}
#endif
//´Ó±ðµÄÓÎÏ·Ô´ÒÆÖ²¹ýÀ´µÄ Î´¾­²âÊÔ
//´ËÐÞ¸´³ÌÐò½â¾öÁË Lua ½Å±¾ÖÐÒò ESC/ÍË³öÐÐÎªÒýÆðµÄ´íÎó¡£
// #ifdef ENABLE_LUA_ESC_BEHAVIOR_FIX
	// if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning() == true)
	// {
		// GetDesc()->DelayedDisconnect(3);
		// return false;
	// }
// #endif

	if (!item->CanUsedBy(this))
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("±ºÁ÷ÀÌ ¸ÂÁö¾Ê¾Æ ÀÌ ¾ÆÀÌÅÛÀ» »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	if (false == FN_check_item_sex(this, item))
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼ºº°ÀÌ ¸ÂÁö¾Ê¾Æ ÀÌ ¾ÆÀÌÅÛÀ» »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}
#ifdef __KINGDOMS_WAR__
	if ((GetMapIndex() == 211) && (KingdomsWar::IS_UNACCPETABLE_ITEM(item->GetVnum()) == 1) && (GetGMLevel() == GM_PLAYER))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(KingdomsWar::MSG[4]));
		return false;
	}
#endif
#ifdef TOURNAMENT_PVP_SYSTEM
	if (CTournamentPvP::instance().IsLimitedItem(this, item->GetVnum()))
		return false;
	
	if (!CTournamentPvP::instance().CanUseItem(this, item))
		return false;
#endif
	if (item->GetVnum() == ITEM_MARRIAGE_RING)
	{
		if (false == IS_SUMMONABLE_ZONE(GetMapIndex()) || !CanWarp())
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("»ç¿ëÇÒ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}
	}

	if (IS_SUMMON_ITEM (item->GetVnum()))
	{
		if (false == IS_SUMMONABLE_ZONE (GetMapIndex()))
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("»ç¿ëÇÒ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}

		if (item->GetVnum() != 70302)
		{
			PIXEL_POSITION posWarp;

			int x = 0;
			int y = 0;

			double nDist = 0;
			const double nDistant = 5000.0;

			if (item->GetVnum() == 22010)
			{
				x = item->GetSocket (0) - GetX();
				y = item->GetSocket (1) - GetY();
			}

			else if (item->GetVnum() == 22000)
			{
				SECTREE_MANAGER::instance().GetRecallPositionByEmpire (GetMapIndex(), GetEmpire(), posWarp);

				if (item->GetSocket (0) == 0)
				{
					x = posWarp.x - GetX();
					y = posWarp.y - GetY();
				}
				else
				{
					x = item->GetSocket (0) - GetX();
					y = item->GetSocket (1) - GetY();
				}
			}

			nDist = sqrt (pow ((float)x, 2) + pow ((float)y, 2));

			if (nDistant > nDist)
			{
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀÌµ¿ µÇ¾îÁú À§Ä¡¿Í ³Ê¹« °¡±î¿ö ±ÍÈ¯ºÎ¸¦ »ç¿ëÇÒ¼ö ¾ø½À´Ï´Ù."));
				return false;
			}
		}
	}


#ifdef ENABLE_EXTENDED_BATTLE_PASS
	bool bpRet = (item && ((item->GetType() == ITEM_USE) || (item->GetType() == ITEM_SKILLBOOK) || (item->GetType() == ITEM_GIFTBOX)));
#endif

	bool ret = UseItemEx(item, DestCell
#ifdef ENABLE_CHEST_OPEN_RENEWAL
		, item_open_count);
#endif
#ifdef ENABLE_EXTENDED_BATTLE_PASS
	if (ret && bpRet)
		UpdateExtBattlePassMissionProgress(BP_ITEM_USE, item_open_count, item->GetVnum());
#endif

	return ret;
}

#ifdef ENABLE_DROP_DIALOG_EXTENDED_SYSTEM
bool CHARACTER::DeleteItem(TItemPos Cell)
{
	LPITEM item = NULL;

	if (!CanHandleItem())
		return false;

	if (!IsValidItemPosition(Cell) || !(item = GetItem(Cell)))
		return false;

	if (true == item->isLocked() || item->IsEquipped())
		return false;

	if (!CanWarp())
		return false;

	// ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("%s ITEM_DELETED"), item->GetName());
	ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("%s ITEM_DELETED %d"), item->GetName(),item->GetCount());

#ifdef ENABLE_ITEM_SELL_LOG
	TItemSellLog log;
	memcpy(&log.alSockets, item->GetSockets(), sizeof(log.alSockets));
	memcpy(&log.aAttr, item->GetAttributes(), sizeof(log.aAttr));
	log.dwVnum = item->GetVnum();
	log.dwCount = item->GetCount();
	log.pid = GetPlayerID();
	snprintf(log.logType, sizeof(log.logType), "ITEM_DELETE");
	DBManager::instance().DirectQuery(ItemSellLogQuery(log).c_str());
#endif
#ifdef ENABLE_EXTENDED_BATTLE_PASS
	UpdateExtBattlePassMissionProgress(BP_ITEM_DESTROY, 1, item->GetVnum());
#endif
	ITEM_MANAGER::instance().RemoveItem(item);

	return true;
}

bool CHARACTER::SellItem(TItemPos Cell)
{
	LPITEM item = NULL;

	if (!CanHandleItem())
		return false;

	if (!IsValidItemPosition(Cell) || !(item = GetItem(Cell)))
		return false;

	if (true == item->isLocked() || item->IsEquipped())
		return false;

	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_SELL))
		return false;

	if (!CanWarp())
		return false;

	int64_t dwPrice;
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	MAX_COUNT bCount;
#else
	BYTE bCount;
#endif

	bCount = item->GetCount();
	dwPrice = item->GetShopBuyPrice();

	if (IS_SET(item->GetFlag(), ITEM_FLAG_COUNT_PER_1GOLD))
	{
		if (dwPrice == 0)
			dwPrice = bCount;
		else
			dwPrice = bCount / dwPrice;
	}
	else
		dwPrice *= bCount;

	const int64_t nTotalMoney = static_cast<int64_t>(GetGold()) + static_cast<int64_t>(dwPrice);

	if (GOLD_MAX <= nTotalMoney)
	{
		sys_err("[OVERFLOW_GOLD] id %u name %s gold %lld", GetPlayerID(), GetName(), GetGold());
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("20¾ï³ÉÀÌ ÃÊ°úÇÏ¿© ¹°Ç°À» ÆÈ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

#ifdef ENABLE_ITEM_SELL_LOG
	TItemSellLog log;
	memcpy(&log.alSockets, item->GetSockets(), sizeof(log.alSockets));
	memcpy(&log.aAttr, item->GetAttributes(), sizeof(log.aAttr));
	log.dwVnum = item->GetVnum();
	log.dwCount = item->GetCount();
	log.pid = GetPlayerID();
	snprintf(log.logType, sizeof(log.logType), "ITEM_SELL");
	DBManager::instance().DirectQuery(ItemSellLogQuery(log).c_str());
#endif

	ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("%s ITEM_IS_SOLD %d"), item->GetName(),item->GetCount());
	// NewChatPacket(ITEM_IS_SOLD, "%s", item->GetName());

	ITEM_MANAGER::instance().RemoveItem(item);
	PointChange(POINT_GOLD, dwPrice, false);
	return true;
}

#else
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
bool CHARACTER::DropItem(TItemPos Cell, MAX_COUNT bCount)
#else
bool CHARACTER::DropItem(TItemPos Cell, BYTE bCount)
#endif
{
	LPITEM item = NULL;

	if (!CanHandleItem())
	{
		if (NULL != DragonSoul_RefineWindow_GetOpener())
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("°­È­Ã¢À» ¿¬ »óÅÂ¿¡¼­´Â ¾ÆÀÌÅÛÀ» ¿Å±æ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	if (IsDead())
		return false;

	if (!IsValidItemPosition(Cell) || !(item = GetItem(Cell)))
		return false;

	if (item->IsExchanging())
		return false;

	if (true == item->isLocked())
		return false;

	if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning() == true)
		return false;

	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_DROP | ITEM_ANTIFLAG_GIVE))
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¹ö¸± ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
		return false;
	}

	if (bCount == 0 || bCount > item->GetCount())
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
		bCount = (MAX_COUNT)item->GetCount();
#else
		bCount = (BYTE)item->GetCount();
#endif

	SyncQuickslot(QUICKSLOT_TYPE_ITEM, Cell.cell, UINT16_MAX);

	LPITEM pkItemToDrop;

	if (bCount == item->GetCount())
	{
		item->RemoveFromCharacter();
		pkItemToDrop = item;
	}
	else
	{
		if (bCount == 0)
		{
			return false;
		}

		item->SetCount(item->GetCount() - bCount);
		ITEM_MANAGER::instance().FlushDelayedSave(item);

		pkItemToDrop = ITEM_MANAGER::instance().CreateItem(item->GetVnum(), bCount);

		FN_copy_item_socket(pkItemToDrop, item);

	}

	PIXEL_POSITION pxPos = GetXYZ();

	if (pkItemToDrop->AddToGround(GetMapIndex(), pxPos))
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¶³¾îÁø ¾ÆÀÌÅÛÀº 3ºÐ ÈÄ »ç¶óÁý´Ï´Ù."));
#ifdef ENABLE_NEWSTUFF
		pkItemToDrop->StartDestroyEvent(g_aiItemDestroyTime[ITEM_DESTROY_TIME_DROPITEM]);
#else
		pkItemToDrop->StartDestroyEvent();
#endif

		ITEM_MANAGER::instance().FlushDelayedSave(pkItemToDrop);

	}

	return true;
}
#endif

bool CHARACTER::DropGold(int gold) // Lightwork@DropYangFix
{
	ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("CANT_DROP_GOLD"));
	return false;
}


bool CHARACTER::MoveItem(TItemPos Cell, TItemPos DestCell,
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	MAX_COUNT count
#else
	BYTE count
#endif
#ifdef ENABLE_SPLIT_ITEMS_FAST
	, bool isSplitItems
#endif
)
{
	LPITEM item = NULL;

	if (!IsValidItemPosition(Cell))
		return false;

	if (!(item = GetItem(Cell)))
		return false;

	if (item->IsExchanging())
		return false;

	if (item->GetCount() < count)
		return false;

	if (Cell.cell == DestCell.cell && Cell.window_type == DestCell.window_type)//c2fix1
		return false;

	if (INVENTORY == Cell.window_type && Cell.cell >= INVENTORY_MAX_NUM && IS_SET(item->GetFlag(), ITEM_FLAG_IRREMOVABLE))
		return false;

#ifdef ENABLE_SPECIAL_STORAGE //moveitem fix
	if (!item->IsUpgradeItem() && DestCell.window_type == UPGRADE_INVENTORY)
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("CANT_MOVE_ITEM_TO_THIS_INVENTORY"));
		return false;
	}

	if (!item->IsBook() && DestCell.window_type == BOOK_INVENTORY)
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("CANT_MOVE_ITEM_TO_THIS_INVENTORY"));
		return false;
	}

	if (!item->IsStone() && DestCell.window_type == STONE_INVENTORY)
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("CANT_MOVE_ITEM_TO_THIS_INVENTORY"));
		return false;
	}

	if (!item->IsChest() && DestCell.window_type == CHEST_INVENTORY)
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("CANT_MOVE_ITEM_TO_THIS_INVENTORY"));
		return false;
	}
#endif

#ifdef ENABLE_SWITCHBOT
	if (Cell.IsSwitchbotPosition() && CSwitchbotManager::Instance().IsActive(GetPlayerID(), Cell.cell))
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("YOU_CANNOT_MOVE_ACTIVE_SWITCHBOT_ITEMS"));
		return false;
	}

	if (DestCell.IsSwitchbotPosition() && !SwitchbotHelper::IsValidItem(item))
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("YOU_CANNOT_ADD_THIS_ITEM_TO_SWITCHBOT"));
		return false;
	}
#endif

	if (true == item->isLocked())
		return false;

	if (!IsValidItemPosition(DestCell))
	{
		return false;
	}

	if (!CanHandleItem())
	{
		if (NULL != DragonSoul_RefineWindow_GetOpener())
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("°­È­Ã¢À» ¿¬ »óÅÂ¿¡¼­´Â ¾ÆÀÌÅÛÀ» ¿Å±æ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	if (DestCell.IsBeltInventoryPosition() && false == CBeltInventoryHelper::CanMoveIntoBeltInventory(item))
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀÌ ¾ÆÀÌÅÛÀº º§Æ® ÀÎº¥Åä¸®·Î ¿Å±æ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}
#ifdef ENABLE_BUFFI_SYSTEM
	if (DestCell.IsBuffiEquipPosition())
	{
		if (Cell.IsDefaultInventoryPosition())
		{
			if (!CanBuffiEquipItem(DestCell, item))
				return false;
		}
		else
		{
			return false;
		}
	}

	if (Cell.IsBuffiEquipPosition() && !DestCell.IsDefaultInventoryPosition())
	{
		return false;
	}
#endif

	if (Cell.IsEquipPosition())
	{
#ifdef ENABLE_NEW_PET_SYSTEM
		if (item->GetVnum() == 55701)
			return false;
#endif
		if (!CanUnequipNow(item))
			return false;

		int iWearCell = item->FindEquipCell(this);
		switch (iWearCell)
		{
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
			case WEAR_WEAPON:
			{
				LPITEM costumeWeapon = GetWear(WEAR_COSTUME_WEAPON);
				if (costumeWeapon && !UnequipItem(costumeWeapon))
				{
					ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼ÒÁöÇ°¿¡ ºó °ø°£ÀÌ ¾ø½À´Ï´Ù."));
					return false;
				}

				if (!IsEmptyItemGrid(DestCell, item->GetSize(), Cell.cell))
					return UnequipItem(item);
				break;
			}
#endif
#ifdef ENABLE_MOUNT_SKIN
			case WEAR_COSTUME_MOUNT:
			{
				LPITEM mountCostume = GetWear(WEAR_MOUNT_SKIN);
				if (mountCostume && !UnequipItem(mountCostume))
				{
					ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼ÒÁöÇ°¿¡ ºó °ø°£ÀÌ ¾ø½À´Ï´Ù."));
					return false;
				}

				if (!IsEmptyItemGrid(DestCell, item->GetSize(), Cell.cell))
					return UnequipItem(item);
				break;
			}
#endif
#ifdef ENABLE_PET_SKIN
			case WEAR_PET:
			{
				LPITEM petCostume = GetWear(WEAR_PET_SKIN);
				if (petCostume && !UnequipItem(petCostume))
				{
					ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼ÒÁöÇ°¿¡ ºó °ø°£ÀÌ ¾ø½À´Ï´Ù."));
					return false;
				}

				if (!IsEmptyItemGrid(DestCell, item->GetSize(), Cell.cell))
					return UnequipItem(item);
				break;
			}
#endif
#ifdef ENABLE_AURA_SKIN
			case WEAR_COSTUME_AURA:
			{
				LPITEM auraCostume = GetWear(WEAR_AURA_SKIN);
				if (auraCostume && !UnequipItem(auraCostume))
				{
					ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼ÒÁöÇ°¿¡ ºó °ø°£ÀÌ ¾ø½À´Ï´Ù."));
					return false;
				}

				if (!IsEmptyItemGrid(DestCell, item->GetSize(), Cell.cell))
					return UnequipItem(item);
				break;
			}
#endif
#ifdef ENABLE_ACCE_COSTUME_SKIN
			case WEAR_COSTUME_ACCE:
			{
				LPITEM acceCostume = GetWear(WEAR_COSTUME_WING);
				if (acceCostume && !UnequipItem(acceCostume))
				{
					ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼ÒÁöÇ°¿¡ ºó °ø°£ÀÌ ¾ø½À´Ï´Ù."));
					return false;
				}

				if (!IsEmptyItemGrid(DestCell, item->GetSize(), Cell.cell))
					return UnequipItem(item);
				break;
			}
#endif
			default:
				break;
		}
	}

	if (DestCell.IsEquipPosition())
	{
#ifdef ENABLE_NEW_PET_SYSTEM
		if (item->GetVnum() == 55701)
			return false;
#endif
		if (GetItem(DestCell))
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀÌ¹Ì Àåºñ¸¦ Âø¿ëÇÏ°í ÀÖ½À´Ï´Ù."));
			return false;
		}

		EquipItem(item, DestCell.cell - INVENTORY_MAX_NUM);
	}
	else
	{
		if (item->IsDragonSoul())
		{
			if (item->IsEquipped())
			{
				return DSManager::instance().PullOut(this, DestCell, item);
			}
			else
			{
				if (DestCell.window_type != DRAGON_SOUL_INVENTORY)
				{
					return false;
				}

				if (!DSManager::instance().IsValidCellForThisItem(item, DestCell))
					return false;
			}
		}

		else if (DRAGON_SOUL_INVENTORY == DestCell.window_type)
			return false;

		LPITEM item2;

		if ((item2 = GetItem(DestCell)) && item != item2 && item2->IsStackable() &&
			!IS_SET(item2->GetAntiFlag(), ITEM_ANTIFLAG_STACK) &&
			item2->GetVnum() == item->GetVnum())
		{

			for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
				if (item2->GetSocket(i) != item->GetSocket(i))
					return false;

			if (count == 0)
				count = item->GetCount();

			count = MIN(g_bItemCountLimit - item2->GetCount(), count);

			item->SetCount(item->GetCount() - count);
			item2->SetCount(item2->GetCount() + count);

			return true;
		}

		if (!IsEmptyItemGrid(DestCell, item->GetSize(), Cell.cell))
			return false;

		if (count == 0 || count >= item->GetCount() || !item->IsStackable() || IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
		{
			item->RemoveFromCharacter();
#ifdef ENABLE_HIGHLIGHT_SYSTEM
			SetItem(DestCell, item, true);
#else
			SetItem(DestCell, item);
#endif
			if (INVENTORY == Cell.window_type && INVENTORY == DestCell.window_type)
				SyncQuickslot(QUICKSLOT_TYPE_ITEM, Cell.cell, DestCell.cell);
		}
		else if (count < item->GetCount())
		{
			item->SetCount(item->GetCount() - count);
			LPITEM item2 = ITEM_MANAGER::instance().CreateItem(item->GetVnum(), count);

			FN_copy_item_socket(item2, item);

#ifdef ENABLE_HIGHLIGHT_SYSTEM
#ifdef ENABLE_SPLIT_ITEMS_FAST // lightwork ÐÞ¸´ÁË²ð·ÖÏîÄ¿ºóµÄÍ»³öÏÔÊ¾
			if (isSplitItems)
				item2->AddToCharacter(this, DestCell, false);
			else
				item2->AddToCharacter(this, DestCell);
#else
			item2->AddToCharacter(this, DestCell);
#endif
#else
			item2->AddToCharacter(this, DestCell);
#endif
		}
	}
	return true;
}

namespace NPartyPickupDistribute
{
	struct FFindOwnership
	{
		LPITEM item;
		LPCHARACTER owner;

		FFindOwnership(LPITEM item)
			: item(item), owner(NULL)
		{
		}

		void operator () (LPCHARACTER ch)
		{
			if (item->IsOwnership(ch))
				owner = ch;
		}
	};

	struct FCountNearMember
	{
		int		total;
		int		x, y;

		FCountNearMember(LPCHARACTER center)
			: total(0), x(center->GetX()), y(center->GetY())
		{
		}

		void operator () (LPCHARACTER ch)
		{
			if (DISTANCE_APPROX(ch->GetX() - x, ch->GetY() - y) <= PARTY_DEFAULT_RANGE)
				total += 1;
		}
	};

	struct FMoneyDistributor
	{
		int		total;
		LPCHARACTER	c;
		int		x, y;
		int		iMoney;

		FMoneyDistributor(LPCHARACTER center, int iMoney)
			: total(0), c(center), x(center->GetX()), y(center->GetY()), iMoney(iMoney)
		{
		}

		void operator ()(LPCHARACTER ch)
		{
			if (ch != c)
				if (DISTANCE_APPROX(ch->GetX() - x, ch->GetY() - y) <= PARTY_DEFAULT_RANGE)
				{
					ch->PointChange(POINT_GOLD, iMoney, true);
				}
		}
	};
}

#ifdef ENABLE_EXTENDED_YANG_LIMIT
void CHARACTER::GiveGold(int64_t iAmount)
#else
void CHARACTER::GiveGold(int iAmount)
#endif
{
	if (iAmount <= 0)
		return;

	if (GetParty())
	{
		LPPARTY pParty = GetParty();

#ifdef ENABLE_EXTENDED_YANG_LIMIT
		int64_t dwTotal = iAmount;
		int64_t dwMyAmount = dwTotal;
#else
		DWORD dwTotal = iAmount;
		DWORD dwMyAmount = dwTotal;
#endif

		NPartyPickupDistribute::FCountNearMember funcCountNearMember(this);
		pParty->ForEachOnlineMember(funcCountNearMember);

		if (funcCountNearMember.total > 1)
		{
			DWORD dwShare = dwTotal / funcCountNearMember.total;
			dwMyAmount -= dwShare * (funcCountNearMember.total - 1);

			NPartyPickupDistribute::FMoneyDistributor funcMoneyDist(this, dwShare);

			pParty->ForEachOnlineMember(funcMoneyDist);
		}

		PointChange(POINT_GOLD, dwMyAmount, true);

#ifdef ENABLE_EXTENDED_BATTLE_PASS
		UpdateExtBattlePassMissionProgress(YANG_COLLECT, dwMyAmount, GetMapIndex());
#endif
	}
	else
	{
		PointChange(POINT_GOLD, iAmount, true);
#ifdef ENABLE_EXTENDED_BATTLE_PASS
		UpdateExtBattlePassMissionProgress(YANG_COLLECT, iAmount, GetMapIndex());
#endif
	}
}

bool CHARACTER::PickupItem(DWORD dwVID)
{
	LPITEM item = ITEM_MANAGER::instance().FindByVID(dwVID);

	if (IsObserverMode())
		return false;

	if (!item || !item->GetSectree())
		return false;

	// if (m_bUnderRefine == true)
	// {
		// ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("REFINING_ITEM_NO_PICKUP_GROUND"));
		// return false;
	// }

	if (item->DistanceValid(this))
	{
		// @fixme150 BEGIN
		if (item->GetType() == ITEM_QUEST)
		{
			if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning() == true)
			{
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("You cannot pickup this item if you're using quests"));
				return false;
			}
		}
		// @fixme150 END

		if (item->IsOwnership(this))
		{
			if (item->GetType() == ITEM_ELK)
			{
				GiveGold(item->GetCount());
				item->RemoveFromGround();

				M2_DESTROY_ITEM(item);

				Save();
			}

			else
			{
				if (item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
				{
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
					MAX_COUNT bCount = item->GetCount();
#else
					BYTE bCount = item->GetCount();
#endif

					for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
					{
						LPITEM item2 = GetInventoryItem(i);

						if (!item2)
							continue;

						if (item2->GetVnum() == item->GetVnum())
						{
							int j;

							for (j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
								if (item2->GetSocket(j) != item->GetSocket(j))
									break;

							if (j != ITEM_SOCKET_MAX_NUM)
								continue;

#ifdef ENABLE_EXTENDED_COUNT_ITEMS
							MAX_COUNT bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
#else
							BYTE bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
#endif
							bCount -= bCount2;

							item2->SetCount(item2->GetCount() + bCount2);

							if (bCount == 0)
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÀÌÅÛ È¹µæ: %s backpack"), item2->GetName());
								M2_DESTROY_ITEM(item);
								if (item2->GetType() == ITEM_QUEST)
									quest::CQuestManager::instance().PickupItem(GetPlayerID(), item2);
								return true;
							}
						}
					}

#ifdef ENABLE_EXTENDED_BATTLE_PASS
					if (item->IsOwnership(this))
						UpdateExtBattlePassMissionProgress(BP_ITEM_COLLECT, bCount, item->GetVnum());
#endif
					item->SetCount(bCount);


				}

#ifdef ENABLE_SPECIAL_STORAGE
				if (item->IsUpgradeItem() && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
				{
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
					MAX_COUNT bCount = item->GetCount();
#else
					BYTE bCount = item->GetCount();
#endif

					for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
					{
						LPITEM item2 = GetUpgradeInventoryItem(i);

						if (!item2)
							continue;

						if (item2->GetVnum() == item->GetVnum())
						{
							int j;

							for (j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
								if (item2->GetSocket(j) != item->GetSocket(j))
									break;

							if (j != ITEM_SOCKET_MAX_NUM)
								continue;

#ifdef ENABLE_EXTENDED_COUNT_ITEMS
							MAX_COUNT bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
#else
							BYTE bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
#endif

							bCount -= bCount2;

							item2->SetCount(item2->GetCount() + bCount2);

							if (bCount == 0)
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÀÌÅÛ È¹µæ: %s Material"), item2->GetName());
								M2_DESTROY_ITEM(item);
								return true;
							}
						}
					}

#ifdef ENABLE_EXTENDED_BATTLE_PASS
					if (item->IsOwnership(this))
						UpdateExtBattlePassMissionProgress(BP_ITEM_COLLECT, bCount, item->GetVnum());
#endif

					item->SetCount(bCount);

				}
				else if (item->IsBook() && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
				{
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
					MAX_COUNT bCount = item->GetCount();
#else
					BYTE bCount = item->GetCount();
#endif

					for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
					{
						LPITEM item2 = GetBookInventoryItem(i);

						if (!item2)
							continue;

						if (item2->GetVnum() == item->GetVnum())
						{
							int j;

							for (j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
								if (item2->GetSocket(j) != item->GetSocket(j))
									break;

							if (j != ITEM_SOCKET_MAX_NUM)
								continue;

#ifdef ENABLE_EXTENDED_COUNT_ITEMS
							MAX_COUNT bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
#else
							BYTE bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
#endif

							bCount -= bCount2;

							item2->SetCount(item2->GetCount() + bCount2);

							if (bCount == 0)
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÀÌÅÛ È¹µæ: % sbooks"), item2->GetName());
								M2_DESTROY_ITEM(item);

								return true;
							}
						}
					}
#ifdef ENABLE_EXTENDED_BATTLE_PASS
					if (item->IsOwnership(this))
						UpdateExtBattlePassMissionProgress(BP_ITEM_COLLECT, bCount, item->GetVnum());
#endif
					item->SetCount(bCount);

				}
				else if (item->IsStone() && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
				{
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
					MAX_COUNT bCount = item->GetCount();
#else
					BYTE bCount = item->GetCount();
#endif

					for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
					{
						LPITEM item2 = GetStoneInventoryItem(i);

						if (!item2)
							continue;

						if (item2->GetVnum() == item->GetVnum())
						{
							int j;

							for (j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
								if (item2->GetSocket(j) != item->GetSocket(j))
									break;

							if (j != ITEM_SOCKET_MAX_NUM)
								continue;

#ifdef ENABLE_EXTENDED_COUNT_ITEMS
							MAX_COUNT bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
#else
							BYTE bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
#endif

							bCount -= bCount2;

							item2->SetCount(item2->GetCount() + bCount2);

							if (bCount == 0)
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÀÌÅÛ È¹µæ: %s stone"), item2->GetName());
								M2_DESTROY_ITEM(item);

								return true;
							}
						}
					}
#ifdef ENABLE_EXTENDED_BATTLE_PASS
					if (item->IsOwnership(this))
						UpdateExtBattlePassMissionProgress(BP_ITEM_COLLECT, bCount, item->GetVnum());
#endif
					item->SetCount(bCount);

				}
				else if (item->IsChest() && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
				{
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
					MAX_COUNT bCount = item->GetCount();
#else
					BYTE bCount = item->GetCount();
#endif

					for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
					{
						LPITEM item2 = GetChestInventoryItem(i);

						if (!item2)
							continue;

						if (item2->GetVnum() == item->GetVnum())
						{
							int j;

							for (j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
								if (item2->GetSocket(j) != item->GetSocket(j))
									break;

							if (j != ITEM_SOCKET_MAX_NUM)
								continue;

#ifdef ENABLE_EXTENDED_COUNT_ITEMS
							MAX_COUNT bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
#else
							BYTE bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
#endif

							bCount -= bCount2;

							item2->SetCount(item2->GetCount() + bCount2);

							if (bCount == 0)
							{
								ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÀÌÅÛ È¹µæ: %s sundries"), item2->GetName());
								M2_DESTROY_ITEM(item);


								return true;
							}
						}
					}

#ifdef ENABLE_EXTENDED_BATTLE_PASS
					if (item->IsOwnership(this))
						UpdateExtBattlePassMissionProgress(BP_ITEM_COLLECT, bCount, item->GetVnum());
#endif

					item->SetCount(bCount);

				}
#endif

				int iEmptyCell;
				if (item->IsDragonSoul())
				{
					if ((iEmptyCell = GetEmptyDragonSoulInventory(item)) == -1)
					{
						ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
						return false;
					}
				}
#ifdef ENABLE_SPECIAL_STORAGE
				else if (item->IsUpgradeItem())
				{
					if ((iEmptyCell = GetEmptyUpgradeInventory(item)) == -1)
					{
						ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
						return false;
					}
				}
				else if (item->IsBook())
				{
					if ((iEmptyCell = GetEmptyBookInventory(item)) == -1)
					{
						ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
						return false;
					}
				}
				else if (item->IsStone())
				{
					if ((iEmptyCell = GetEmptyStoneInventory(item)) == -1)
					{
						ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
						return false;
					}
				}
				else if (item->IsChest())
				{
					if ((iEmptyCell = GetEmptyChestInventory(item)) == -1)
					{
						ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
						return false;
					}
				}
#endif
				else
				{
					if ((iEmptyCell = GetEmptyInventory(item->GetSize())) == -1)
					{
						ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
						return false;
					}
				}

				item->RemoveFromGround();

				if (item->IsDragonSoul())
				{
					item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyCell));
					ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÀÌÅÛ È¹µæ: %s dragon_soul"), item->GetName());
				}
#ifdef ENABLE_SPECIAL_STORAGE
				else if (item->IsUpgradeItem())
				{
					item->AddToCharacter(this, TItemPos(UPGRADE_INVENTORY, iEmptyCell));
					ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÀÌÅÛ È¹µæ: %s Material"), item->GetName());
				}
					
				else if (item->IsBook())
				{
					item->AddToCharacter(this, TItemPos(BOOK_INVENTORY, iEmptyCell));
					ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÀÌÅÛ È¹µæ: % sbooks"), item->GetName());
				}
				else if (item->IsStone())
				{
					item->AddToCharacter(this, TItemPos(STONE_INVENTORY, iEmptyCell));
					ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÀÌÅÛ È¹µæ: %s stone"), item->GetName());
				}
				else if (item->IsChest())
				{
					item->AddToCharacter(this, TItemPos(CHEST_INVENTORY, iEmptyCell));
					ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÀÌÅÛ È¹µæ: %s sundries"), item->GetName());
				}
#endif
				else
				{
					item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));
					ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÀÌÅÛ È¹µæ: %s backpack"), item->GetName());
				}

				if (item->GetType() == ITEM_QUEST)
					quest::CQuestManager::instance().PickupItem(GetPlayerID(), item);
			}

			//Motion(MOTION_PICKUP);
			return true;
		}
		else if (!IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_GIVE | ITEM_ANTIFLAG_DROP) && GetParty())
		{
			NPartyPickupDistribute::FFindOwnership funcFindOwnership(item);

			GetParty()->ForEachOnlineMember(funcFindOwnership);

			LPCHARACTER owner = funcFindOwnership.owner;
			// @fixme115
			if (!owner)
				return false;

			int iEmptyCell;

			if (item->IsDragonSoul())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyDragonSoulInventory(item)) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptyDragonSoulInventory(item)) == -1)
					{
						owner->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
						return false;
					}
				}
			}
#ifdef ENABLE_SPECIAL_STORAGE
			else if (item->IsUpgradeItem())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyUpgradeInventory(item)) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptyUpgradeInventory(item)) == -1)
					{
						owner->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
						return false;
					}
				}
			}
			else if (item->IsBook())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyBookInventory(item)) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptyBookInventory(item)) == -1)
					{
						owner->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
						return false;
					}
				}
			}
			else if (item->IsStone())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyStoneInventory(item)) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptyStoneInventory(item)) == -1)
					{
						owner->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
						return false;
					}
				}
			}
			else if (item->IsChest())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyChestInventory(item)) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptyChestInventory(item)) == -1)
					{
						owner->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
						return false;
					}
				}
			}
#endif

			else
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyInventory(item->GetSize())) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptyInventory(item->GetSize())) == -1)
					{
						owner->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
						return false;
					}
				}
			}

			item->RemoveFromGround();

			if (item->IsDragonSoul())
			{
				item->AddToCharacter(owner, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyCell));
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÀÌÅÛ È¹µæ: %s dragon_soul"), item->GetName());
			}
#ifdef ENABLE_SPECIAL_STORAGE
			else if (item->IsUpgradeItem())
			{
				item->AddToCharacter(owner, TItemPos(UPGRADE_INVENTORY, iEmptyCell));
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÀÌÅÛ È¹µæ: %s Material"), item->GetName());
			}

			else if (item->IsBook())
			{
				item->AddToCharacter(owner, TItemPos(BOOK_INVENTORY, iEmptyCell));
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÀÌÅÛ È¹µæ: % sbooks"), item->GetName());
			}
			else if (item->IsStone())
			{
				item->AddToCharacter(owner, TItemPos(STONE_INVENTORY, iEmptyCell));
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÀÌÅÛ È¹µæ: %s stone"), item->GetName());
			}
			else if (item->IsChest())
			{
				item->AddToCharacter(owner, TItemPos(CHEST_INVENTORY, iEmptyCell));
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÀÌÅÛ È¹µæ: %s sundries"), item->GetName());
			}
#endif
			else
			{
				item->AddToCharacter(owner, TItemPos(INVENTORY, iEmptyCell));
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÀÌÅÛ È¹µæ: %s backpack"), item->GetName());
			}

			//ÐÞ¸´×é¶ÓÊ°È¡¹éÊô´íÎó
			if (owner != this)
			{
				if (item->IsDragonSoul())
				{
					item->AddToCharacter(owner, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyCell));
					owner->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÀÌÅÛ È¹µæ: %s ´ÔÀ¸·ÎºÎÅÍ %s dragon_soul"), GetName(), item->GetName());
					ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÀÌÅÛ Àü´Þ: %s ´Ô¿¡°Ô %s"), owner->GetName(), item->GetName());
				}
#ifdef ENABLE_SPECIAL_STORAGE
				else if (item->IsUpgradeItem())
				{
					item->AddToCharacter(owner, TItemPos(UPGRADE_INVENTORY, iEmptyCell));
					owner->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÀÌÅÛ È¹µæ: %s ´ÔÀ¸·ÎºÎÅÍ %s Material"), GetName(), item->GetName());
					ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÀÌÅÛ Àü´Þ: %s ´Ô¿¡°Ô %s"), owner->GetName(), item->GetName());
				}

				else if (item->IsBook())
				{
					item->AddToCharacter(owner, TItemPos(BOOK_INVENTORY, iEmptyCell));
					owner->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÀÌÅÛ È¹µæ: %s ´ÔÀ¸·ÎºÎÅÍ %s sbooks"), GetName(), item->GetName());
					ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÀÌÅÛ Àü´Þ: %s ´Ô¿¡°Ô %s"), owner->GetName(), item->GetName());
				}
				else if (item->IsStone())
				{
					item->AddToCharacter(owner, TItemPos(STONE_INVENTORY, iEmptyCell));
					owner->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÀÌÅÛ È¹µæ: %s ´ÔÀ¸·ÎºÎÅÍ %s stone"), GetName(), item->GetName());
					ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÀÌÅÛ Àü´Þ: %s ´Ô¿¡°Ô %s"), owner->GetName(), item->GetName());
				}
				else if (item->IsChest())
				{
					item->AddToCharacter(owner, TItemPos(CHEST_INVENTORY, iEmptyCell));
					owner->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÀÌÅÛ È¹µæ: %s ´ÔÀ¸·ÎºÎÅÍ %s sundries"), GetName(), item->GetName());
					ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÀÌÅÛ Àü´Þ: %s ´Ô¿¡°Ô %s"), owner->GetName(), item->GetName());
				}
#endif
				else
				{
					item->AddToCharacter(owner, TItemPos(INVENTORY, iEmptyCell));
					owner->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÀÌÅÛ È¹µæ: %s ´ÔÀ¸·ÎºÎÅÍ %s backpack"), GetName(), item->GetName());
					ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÀÌÅÛ Àü´Þ: %s ´Ô¿¡°Ô %s"), owner->GetName(), item->GetName());
				}
			}

			//NED
			if (item->GetType() == ITEM_QUEST)
				quest::CQuestManager::instance().PickupItem(owner->GetPlayerID(), item);

			return true;
		}
	}

	return false;
}

bool CHARACTER::SwapItem(CELL_UINT bCell, CELL_UINT bDestCell)
{
	if (!CanHandleItem())
		return false;
	TItemPos srcCell(INVENTORY, bCell), destCell(INVENTORY, bDestCell);

	if (srcCell.IsDragonSoulEquipPosition() || destCell.IsDragonSoulEquipPosition())
		return false;

	if (bCell == bDestCell)
		return false;

	if (srcCell.IsEquipPosition() && destCell.IsEquipPosition())
		return false;

	LPITEM item1, item2;

	if (srcCell.IsEquipPosition())
	{
		item1 = GetInventoryItem(bDestCell);
		item2 = GetInventoryItem(bCell);
	}
	else
	{
		item1 = GetInventoryItem(bCell);
		item2 = GetInventoryItem(bDestCell);
	}

	if (!item1 || !item2)
		return false;

	if (item1 == item2)
	{
		return false;
	}

	if (!IsEmptyItemGrid(TItemPos(INVENTORY, item1->GetCell()), item2->GetSize(), item1->GetCell()))
		return false;

	if (TItemPos(EQUIPMENT, item2->GetCell()).IsEquipPosition())
	{
		BYTE bEquipCell = item2->GetCell() - INVENTORY_MAX_NUM;
		BYTE bInvenCell = item1->GetCell();

		if (item2->IsDragonSoul() || item2->GetType() == ITEM_BELT) // @fixme117
		{
			if (false == CanUnequipNow(item2) || false == CanEquipNow(item1))
				return false;
		}

		if (bEquipCell != item1->FindEquipCell(this))
			return false;

		item2->RemoveFromCharacter();

		if (item1->EquipTo(this, bEquipCell))
		{
#ifdef ENABLE_HIGHLIGHT_SYSTEM
			item2->AddToCharacter(this, TItemPos(INVENTORY, bInvenCell), false);
#else
			item2->AddToCharacter(this, TItemPos(INVENTORY, bInvenCell));
#endif
			//µ¼ÖÂÊôÐÔÍ³¼Æ³öÏÖ´íÎóµÄµØ·½ ÊôÐÔÖµ ÊôÐÔ´íÎó
			// item2->ModifyPoints(false);	//ÎïÆ·½»»»ÐÞ¸´ ds_aim //lightwork17 ÐÞ¸´¿ªÊ¼
			// ComputePoints();				//ÎïÆ·½»»»ÐÞ¸´ ds_aim //lightwork17 ½áÊø
		}
		else
		{
			sys_err("SwapItem cannot equip %s! item1 %s", item2->GetName(), item1->GetName());
		}
	}
	else
	{
		CELL_UINT bCell1 = item1->GetCell();
		CELL_UINT bCell2 = item2->GetCell();

		item1->RemoveFromCharacter();
		item2->RemoveFromCharacter();

#ifdef ENABLE_HIGHLIGHT_SYSTEM
		item1->AddToCharacter(this, TItemPos(INVENTORY, bCell2), false);
		item2->AddToCharacter(this, TItemPos(INVENTORY, bCell1), false);
#else
		item1->AddToCharacter(this, TItemPos(INVENTORY, bCell2));
		item2->AddToCharacter(this, TItemPos(INVENTORY, bCell1));
#endif
	}

	return true;
}

bool CHARACTER::UnequipItem(LPITEM item)
{
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
	int iWearCell = item->FindEquipCell(this);
	if (iWearCell == WEAR_WEAPON)
	{
		LPITEM costumeWeapon = GetWear(WEAR_COSTUME_WEAPON);
		if (costumeWeapon && !UnequipItem(costumeWeapon))
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("You cannot unequip the costume weapon. Not enough space."));
			return false;
		}
	}
#endif
	else if (iWearCell == WEAR_BODY)
	{
		LPITEM costumeBody = GetWear(WEAR_COSTUME_BODY);
		if (costumeBody && !UnequipItem(costumeBody))
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Fashion clothes in use cannot be moved to the warehouse"));
			return false;
		}
	}

#ifdef ENABLE_MOUNT_PET_ACCE_AURA_COSTUME_SYSTEM
#ifdef ENABLE_MOUNT_SKIN
	if (item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_MOUNT)
	{
		LPITEM pkMountSkin = GetWear(WEAR_MOUNT_SKIN);

		if (pkMountSkin)
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("UNEQUIP_MOUNT_SKIN"));
			return false;
		}
	}
#endif
#ifdef ENABLE_PET_SKIN
	else if (item->GetType() == ITEM_PET)//pet skin
	{
		LPITEM pkMountSkin = GetWear(WEAR_PET_SKIN);

		if (pkMountSkin)
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("UNEQUIP_PET_SKIN"));
			return false;
		}
	}
#endif
#ifdef ENABLE_ACCE_COSTUME_SKIN
	else if (item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_ACCE)
	{
		LPITEM pkAcceSkin = GetWear(WEAR_COSTUME_WING);

		if (pkAcceSkin)
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("UNEQUIP_COSTUME_WING"));
			return false;
		}
	}
#endif
#ifdef ENABLE_AURA_SKIN
	else if (item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_AURA)
	{
		LPITEM pkAuraSkin = GetWear(WEAR_AURA_SKIN);

		if (pkAuraSkin)
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("UNEQUIP_COSTUME_AURA_SKIN"));
			return false;
		}
	}
#endif
#endif
	// µ±ÐèÒªÕªÏÂ»¤ÓÓ½äÖ¸ºÍ¶á±¦ÊÖÌ×Ê±£¬É¾³ý»¤ÓÓ½äÖ¸»ò¶á±¦ÊÖÌ×
	if (item->GetType() == ITEM_UNIQUE)
	{
#ifdef ENABLE_EXP_OR_ITEM_PLUS_SYSTEM
		if (item->GetVnum() == UNIQUE_ITEM_DOUBLE_EXP ||
			item->GetVnum() == UNIQUE_ITEM_DOUBLE_EXP_PLUS ||
			item->GetVnum() == UNIQUE_ITEM_DOUBLE_ITEM ||
			item->GetVnum() == UNIQUE_ITEM_DOUBLE_ITEM_PLUS)
		{
			LPITEM wearItem = GetWear(WEAR_UNIQUE1);
			LPITEM wearItem2 = GetWear(WEAR_UNIQUE2);

			if (wearItem)
			{
				if (item->GetVnum() == wearItem->GetVnum())
				{
					ITEM_MANAGER::instance().RemoveItem(item);
					return true;
				}
			}
			if (wearItem2)
			{
				if (item->GetVnum() == wearItem2->GetVnum())
				{
					ITEM_MANAGER::instance().RemoveItem(item);
					return true;
				}
			}
		}
#endif
#ifdef ENABLE_NEW_PET_SYSTEM
		if (item->GetVnum() == 55140)
		{
			if (GetNewPetSystem())
				GetNewPetSystem()->SetExpRing(false);
		}
#endif
	}

	if (false == CanUnequipNow(item))
		return false;
		

	int pos;
	if (item->IsDragonSoul())
		pos = GetEmptyDragonSoulInventory(item);
	else
		pos = GetEmptyInventory(item->GetSize());

	// HARD CODING
	if (item->GetVnum() == UNIQUE_ITEM_HIDE_ALIGNMENT_TITLE)
		ShowAlignment(true);

	item->RemoveFromCharacter();
	if (item->IsDragonSoul())
	{
#ifdef ENABLE_HIGHLIGHT_SYSTEM
		item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, pos), false);
#else
		item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, pos));
#endif
	}
	else
#ifdef ENABLE_HIGHLIGHT_SYSTEM
		item->AddToCharacter(this, TItemPos(INVENTORY, pos), false);
#else
		item->AddToCharacter(this, TItemPos(INVENTORY, pos));
#endif

#ifdef ENABLE_MOUNT_SKIN
	if (item->GetType() == ITEM_MOUNT_SKIN)//binek skin cikinca normal binegi cagirir fonksyon sonunda kalmasi gerek
	{
		LPITEM pkMount = GetWear(WEAR_COSTUME_MOUNT);

		if (pkMount)
		{
			MountSummon(pkMount);
		}
#ifdef ENABLE_HIDE_COSTUME
		SetMountSkinHidden(false);
#endif
	}
#endif
#ifdef ENABLE_PET_SKIN
	else if (item->GetType() == ITEM_PET_SKIN)//pet skin cikinca normal pet cagirir fonksyon sonunda kalmasi gerek
	{
		LPITEM pkPet = GetWear(WEAR_PET);
		if (pkPet)
		{
			PetSummon(pkPet);
		}
	}
#endif
#ifdef ENABLE_ACCE_COSTUME_SKIN
	else if (item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_WING)
	{
		LPITEM pkAcce = GetWear(WEAR_COSTUME_ACCE);

		if (pkAcce)
		{
			SetPart(PART_ACCE, pkAcce->GetVnum());
			UpdatePacket();
		}
	}
#endif
#ifdef ENABLE_BOOSTER_ITEM
	else if (item->GetType() == ITEM_BOOSTER)
	{
		CalcuteBoosterSetBonus();
	}
#endif
	CheckMaximumPoints();

#ifdef ENABLE_POWER_RANKING
	CalcutePowerRank();
#endif

	return true;
}

bool CHARACTER::EquipItem(LPITEM item, int iCandidateCell)
{
	if (item->IsExchanging())
		return false;

	if (false == item->IsEquipable())
		return false;

	if (false == CanEquipNow(item))
		return false;

	int iWearCell = item->FindEquipCell(this, iCandidateCell);

	if (iWearCell < 0)
		return false;

	//@fixme1001 START
	if (item->GetCount() > 1 && iWearCell != WEAR_ARROW)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You must separate it first (Shift + Left Click)"));
		return false;
	}
	//@fixme1001 END
	
	if (iWearCell == WEAR_BODY && IsRiding() && (item->GetVnum() >= 11901 && item->GetVnum() <= 11904))
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¸»À» Åº »óÅÂ¿¡¼­ ¿¹º¹À» ÀÔÀ» ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	if (iWearCell != WEAR_ARROW && IsPolymorphed())
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("µÐ°© Áß¿¡´Â Âø¿ëÁßÀÎ Àåºñ¸¦ º¯°æÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	if (FN_check_item_sex(this, item) == false)
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼ºº°ÀÌ ¸ÂÁö¾Ê¾Æ ÀÌ ¾ÆÀÌÅÛÀ» »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

#ifndef ENABLE_MOUNT_COSTUME_SYSTEM
	if (item->IsRideItem() && IsRiding())
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀÌ¹Ì Å»°ÍÀ» ÀÌ¿ëÁßÀÔ´Ï´Ù."));
		return false;
	}
#endif

	if (((GetParty() && GetParty()->GetLeader() != this) || !GetParty()) && (item->GetVnum() == UNIQUE_ITEM_PARTY_BONUS_EXP || item->GetVnum() == 71012 || item->GetVnum() == 72043 || item->GetVnum() == 72044 || item->GetVnum() == 72045 || item->GetVnum() == 76011))
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Only the team leader can perform this operation"));
		return false; /* lightwork correction mert party fix*/
	}

#ifdef ENABLE_COSTUME_RELATED_FIXES
	if (GetWear(WEAR_BODY) && GetWear(WEAR_BODY)->GetVnum() >= 11901 && GetWear(WEAR_BODY)->GetVnum() <= 11904 && item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_BODY)
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Do not operate while wearing a formal dress or wedding gown"));
		return false;
	}

	if (GetWear(WEAR_COSTUME_BODY) && item->GetVnum() >= 11901 && item->GetVnum() <= 11904)
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("You can't do this in a wedding dress"));
		return false;
	}
#endif

	// DWORD dwCurTime = get_dword_time();

	// if (iWearCell != WEAR_ARROW
		// && (dwCurTime - GetLastAttackTime() <= 1500 || dwCurTime - m_dwLastSkillTime <= 1500))
	// {
		// ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("°¡¸¸È÷ ÀÖÀ» ¶§¸¸ Âø¿ëÇÒ ¼ö ÀÖ½À´Ï´Ù."));
		// return false;
	// }

#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
	if (iWearCell == WEAR_WEAPON)
	{
		if (item->GetType() == ITEM_WEAPON)
		{
			LPITEM costumeWeapon = GetWear(WEAR_COSTUME_WEAPON);
			if (costumeWeapon && costumeWeapon->GetValue(3) != item->GetSubType() && !UnequipItem(costumeWeapon))
			{
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("You cannot unequip the costume weapon. Not enough space."));
				return false;
			}
		}
		else //fishrod/pickaxe
		{
			LPITEM costumeWeapon = GetWear(WEAR_COSTUME_WEAPON);
			if (costumeWeapon && !UnequipItem(costumeWeapon))
			{
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("You cannot unequip the costume weapon. Not enough space."));
				return false;
			}
		}
	}
	else if (iWearCell == WEAR_COSTUME_WEAPON)
	{
		if (item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_WEAPON)
		{
			LPITEM pkWeapon = GetWear(WEAR_WEAPON);

			if (!pkWeapon || pkWeapon->GetType() != ITEM_WEAPON || item->GetValue(3) != pkWeapon->GetSubType())
			{
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Please wear the main weapon first, then the costume weapon"));
				return false;
			}
		}
	}
#endif
#ifdef ENABLE_MOUNT_PET_ACCE_AURA_COSTUME_SYSTEM
#ifdef ENABLE_MOUNT_SKIN
	if (item->GetType() == ITEM_MOUNT_SKIN)
	{
		LPITEM pkWearMount = GetWear(WEAR_COSTUME_MOUNT);
		if (!pkWearMount)
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("CANT_DO_THIS_IF_NOT_WEAR_MOUNT"));
			return false;
		}
	}
#endif
#ifdef ENABLE_PET_SKIN
	else if (item->GetType() == ITEM_PET_SKIN)
	{
		LPITEM pkWearPet = GetWear(WEAR_PET);
		if (!pkWearPet)
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("CANT_DO_THIS_IF_NOT_WEAR_PET"));
			return false;
		}
	}
#endif
#ifdef ENABLE_ACCE_COSTUME_SKIN
	else if (item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_WING)
	{
		LPITEM pkWearAcce = GetWear(WEAR_COSTUME_ACCE);
		if (!pkWearAcce)
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("CANT_DO_THIS_IF_NOT_WEAR_SASH"));
			return false;
		}
	}
#endif
#ifdef ENABLE_AURA_SKIN
	else if (item->GetType() == ITEM_AURA_SKIN)
	{
		LPITEM pkWearAura = GetWear(WEAR_COSTUME_AURA);
		if (!pkWearAura)
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("CANT_DO_THIS_IF_NOT_WEAR_AURA"));
			return false;
		}
	}
#endif
#endif
	if (item->IsDragonSoul())
	{
		if (GetInventoryItem(INVENTORY_MAX_NUM + iWearCell))
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ALREADY_WEAR_SAME_TYPE_DS"));
			return false;
		}

		if (!item->EquipTo(this, iWearCell))
		{
			return false;
		}
	}

	else
	{
		if (GetWear(iWearCell) && !IS_SET(GetWear(iWearCell)->GetFlag(), ITEM_FLAG_IRREMOVABLE))
		{
			if (false == SwapItem(item->GetCell(), INVENTORY_MAX_NUM + iWearCell))
			{
				return false;
			}
		}
		else
		{
			BYTE bOldCell = item->GetCell();

			if (item->EquipTo(this, iWearCell))
			{
				SyncQuickslot(QUICKSLOT_TYPE_ITEM, bOldCell, iWearCell);
			}
		}
	}

#ifdef ENABLE_NEW_PET_SYSTEM
	if (item->GetVnum() == 55140)
	{
		if (GetNewPetSystem())
			GetNewPetSystem()->SetExpRing(true);
	}
#endif

	if (true == item->IsEquipped())
	{
		if (-1 != item->GetProto()->cLimitRealTimeFirstUseIndex)
		{
			if (0 == item->GetSocket(1))
			{
				long duration = (0 != item->GetSocket(0)) ? item->GetSocket(0) : item->GetProto()->aLimits[(unsigned char)(item->GetProto()->cLimitRealTimeFirstUseIndex)].lValue;

				if (0 == duration)
					duration = 60 * 60 * 24 * 7;

				item->SetSocket(0, time(0) + duration);
				item->StartRealTimeExpireEvent();
			}

			item->SetSocket(1, item->GetSocket(1) + 1);
		}

		if (item->GetVnum() == UNIQUE_ITEM_HIDE_ALIGNMENT_TITLE)
			ShowAlignment(false);

		if (ITEM_UNIQUE == item->GetType() && 0 != item->GetSIGVnum())
		{
			const CSpecialItemGroup* pGroup = ITEM_MANAGER::instance().GetSpecialItemGroup(item->GetSIGVnum());
			if (NULL != pGroup)
			{
				const CSpecialAttrGroup* pAttrGroup = ITEM_MANAGER::instance().GetSpecialAttrGroup(pGroup->GetAttrVnum(item->GetVnum()));
				if (NULL != pAttrGroup)
				{
					const std::string& std = pAttrGroup->m_stEffectFileName;
					SpecificEffectPacket(std.c_str());
				}
			}
		}
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		else if (item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_ACCE)
			this->EffectPacket(SE_EFFECT_ACCE_EQUIP);
#endif

		if (item->IsNewMountItem()) // @fixme152
			quest::CQuestManager::instance().SIGUse(GetPlayerID(), quest::QUEST_NO_NPC, item, false);
	}

#ifdef ENABLE_ITEMS_SHINING
	if (item->GetType() == ITEM_SHINING)
	{
		this->UpdatePacket();
	}	
#endif
#ifdef ENABLE_BOOSTER_ITEM
	else if (item->GetType() == ITEM_BOOSTER)
	{
		CalcuteBoosterSetBonus();
	}
#endif
#ifdef ENABLE_POWER_RANKING
	CalcutePowerRank();
#endif

	return true;
}

void CHARACTER::BuffOnAttr_AddBuffsFromItem(LPITEM pItem)
{
	for (size_t i = 0; i < sizeof(g_aBuffOnAttrPoints) / sizeof(g_aBuffOnAttrPoints[0]); i++)
	{
		TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.find(g_aBuffOnAttrPoints[i]);
		if (it != m_map_buff_on_attrs.end())
		{
			it->second->AddBuffFromItem(pItem);
		}
	}
}

void CHARACTER::BuffOnAttr_RemoveBuffsFromItem(LPITEM pItem)
{
	for (size_t i = 0; i < sizeof(g_aBuffOnAttrPoints) / sizeof(g_aBuffOnAttrPoints[0]); i++)
	{
		TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.find(g_aBuffOnAttrPoints[i]);
		if (it != m_map_buff_on_attrs.end())
		{
			it->second->RemoveBuffFromItem(pItem);
		}
	}
}

void CHARACTER::BuffOnAttr_ClearAll()
{
	for (TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.begin(); it != m_map_buff_on_attrs.end(); it++)
	{
		CBuffOnAttributes* pBuff = it->second;
		if (pBuff)
		{
			pBuff->Initialize();
		}
	}
}

void CHARACTER::BuffOnAttr_ValueChange(BYTE bType, BYTE bOldValue, BYTE bNewValue)
{
	TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.find(bType);

	if (0 == bNewValue)
	{
		if (m_map_buff_on_attrs.end() == it)
			return;
		else
			it->second->Off();
	}
	else if (0 == bOldValue)
	{
		CBuffOnAttributes* pBuff = NULL;
		if (m_map_buff_on_attrs.end() == it)
		{
			switch (bType)
			{
			case POINT_ENERGY:
			{
				static BYTE abSlot[] = { WEAR_BODY, WEAR_HEAD, WEAR_FOOTS, WEAR_WRIST, WEAR_WEAPON, WEAR_NECK, WEAR_EAR, WEAR_SHIELD };
				static std::vector <BYTE> vec_slots(abSlot, abSlot + _countof(abSlot));
				pBuff = M2_NEW CBuffOnAttributes(this, bType, &vec_slots);
			}
			break;
			case POINT_COSTUME_ATTR_BONUS:
			{
				static BYTE abSlot[] = {
					WEAR_COSTUME_BODY,
					WEAR_COSTUME_HAIR,
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
					WEAR_COSTUME_MOUNT,
#endif
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
					WEAR_COSTUME_WEAPON,
#endif
#ifdef ENABLE_ACCE_COSTUME_SKIN
					WEAR_COSTUME_WING,
#endif
				};
				static std::vector <BYTE> vec_slots(abSlot, abSlot + _countof(abSlot));
				pBuff = M2_NEW CBuffOnAttributes(this, bType, &vec_slots);
			}
			break;
			default:
				break;
			}
			m_map_buff_on_attrs.insert(TMapBuffOnAttrs::value_type(bType, pBuff));
		}
		else
			pBuff = it->second;
		if (pBuff != NULL)
			pBuff->On(bNewValue);
	}
	else
	{
		assert(m_map_buff_on_attrs.end() != it);
		it->second->ChangeBuffValue(bNewValue);
	}
}

LPITEM CHARACTER::FindSpecifyItem(DWORD vnum) const
{
	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
		if (GetInventoryItem(i) && GetInventoryItem(i)->GetVnum() == vnum)
			return GetInventoryItem(i);

	return NULL;
}

LPITEM CHARACTER::FindItemByID(DWORD id) const
{
	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
	{
		if (NULL != GetInventoryItem(i) && GetInventoryItem(i)->GetID() == id)
			return GetInventoryItem(i);
	}

	for (int i = BELT_INVENTORY_SLOT_START; i < BELT_INVENTORY_SLOT_END; ++i)
	{
		if (NULL != GetInventoryItem(i) && GetInventoryItem(i)->GetID() == id)
			return GetInventoryItem(i);
	}

	return NULL;
}

int CHARACTER::CountSpecifyTypeItem(BYTE type) const
{
	int	count = 0;

	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
	{
		LPITEM pItem = GetInventoryItem(i);
		if (pItem != NULL && pItem->GetType() == type)
		{
			count += pItem->GetCount();
		}
	}

	return count;
}

void CHARACTER::RemoveSpecifyTypeItem(BYTE type, DWORD count)
{
	if (0 == count)
		return;

	for (UINT i = 0; i < INVENTORY_MAX_NUM; ++i)
	{
		if (NULL == GetInventoryItem(i))
			continue;

		if (GetInventoryItem(i)->GetType() != type)
			continue;

		if (m_pkMyShop)
		{
			bool isItemSelling = m_pkMyShop->IsSellingItem(GetInventoryItem(i)->GetID());
			if (isItemSelling)
				continue;
		}

		if (count >= GetInventoryItem(i)->GetCount())
		{
			count -= GetInventoryItem(i)->GetCount();
			GetInventoryItem(i)->SetCount(0);

			if (0 == count)
				return;
		}
		else
		{
			GetInventoryItem(i)->SetCount(GetInventoryItem(i)->GetCount() - count);
			return;
		}
	}
}

void CHARACTER::AutoGiveItem(LPITEM item, bool longOwnerShip)
{
	if (NULL == item)
	{
		sys_err("NULL point.");
		return;
	}
	if (item->GetOwner())
	{
		sys_err("item %d 's owner exists!", item->GetID());
		return;
	}

	int cell;
	if (item->IsDragonSoul())
	{
		cell = GetEmptyDragonSoulInventory(item);
	}
#ifdef ENABLE_SPECIAL_STORAGE
	else if (item->IsUpgradeItem())
	{
		cell = GetEmptyUpgradeInventory(item);
	}
	else if (item->IsBook())
	{
		cell = GetEmptyBookInventory(item);
	}
	else if (item->IsStone())
	{
		cell = GetEmptyStoneInventory(item);
	}
	else if (item->IsChest())
	{
		cell = GetEmptyChestInventory(item);
	}
#endif
	else
	{
		cell = GetEmptyInventory(item->GetSize());
	}

	if (cell != -1)
	{
		if (item->IsDragonSoul())
			item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, cell));
#ifdef ENABLE_SPECIAL_STORAGE
		else if (item->IsUpgradeItem())
			item->AddToCharacter(this, TItemPos(UPGRADE_INVENTORY, cell));
		else if (item->IsBook())
			item->AddToCharacter(this, TItemPos(BOOK_INVENTORY, cell));
		else if (item->IsStone())
			item->AddToCharacter(this, TItemPos(STONE_INVENTORY, cell));
		else if (item->IsChest())
			item->AddToCharacter(this, TItemPos(CHEST_INVENTORY, cell));
#endif
		else
			item->AddToCharacter(this, TItemPos(INVENTORY, cell));

		if (item->GetType() == ITEM_USE && item->GetSubType() == USE_POTION)
		{
			TQuickslot* pSlot;

			if (GetQuickslot(0, &pSlot) && pSlot->type == QUICKSLOT_TYPE_NONE)
			{
				TQuickslot slot;
				slot.type = QUICKSLOT_TYPE_ITEM;
				slot.pos = cell;
				SetQuickslot(0, slot);
			}
		}
	}
	else
	{
		item->AddToGround(GetMapIndex(), GetXYZ());
#ifdef ENABLE_NEWSTUFF
		item->StartDestroyEvent(g_aiItemDestroyTime[ITEM_DESTROY_TIME_AUTOGIVE]);
#else
		item->StartDestroyEvent();
#endif

		if (longOwnerShip)
			item->SetOwnership(this, 300);
		else
			item->SetOwnership(this, 60);
	}
}


bool CHARACTER::GiveItem(LPCHARACTER victim, TItemPos Cell)
{
	if (!CanHandleItem())
		return false;

	// @fixme150 BEGIN
	if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning() == true)
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("CLOSE_OTHER_WINDOWS_TO_DO_THIS02"));
		return false;
	}
	// @fixme150 END

	LPITEM item = GetItem(Cell);

	if (item && !item->IsExchanging())
	{
		if (victim->CanReceiveItem(this, item))
		{
			victim->ReceiveItem(this, item);
			return true;
		}
	}

	return false;
}

bool CHARACTER::CanReceiveItem(LPCHARACTER from, LPITEM item) const
{
	if (IsPC())
		return false;

	// TOO_LONG_DISTANCE_EXCHANGE_BUG_FIX
	if (DISTANCE_APPROX(GetX() - from->GetX(), GetY() - from->GetY()) > 2000)
		return false;
	// END_OF_TOO_LONG_DISTANCE_EXCHANGE_BUG_FIX

	switch (GetRaceNum())
	{
	case fishing::CAMPFIRE_MOB:
		if (item->GetType() == ITEM_FISH &&
			(item->GetSubType() == FISH_ALIVE || item->GetSubType() == FISH_DEAD))
			return true;
		break;

	case fishing::FISHER_MOB:
		if (item->GetType() == ITEM_ROD)
			return true;
		break;

		// BUILDING_NPC
	case BLACKSMITH_WEAPON_MOB:
	case DEVILTOWER_BLACKSMITH_WEAPON_MOB:
		if (item->GetType() == ITEM_WEAPON &&
			item->GetRefinedVnum())
			return true;
		else
			return false;
		break;

	case BLACKSMITH_ARMOR_MOB:
	case DEVILTOWER_BLACKSMITH_ARMOR_MOB:
		if (item->GetType() == ITEM_ARMOR &&
			(item->GetSubType() == ARMOR_BODY || item->GetSubType() == ARMOR_SHIELD || item->GetSubType() == ARMOR_HEAD) &&
			item->GetRefinedVnum())
			return true;
		else
			return false;
		break;

	case BLACKSMITH_ACCESSORY_MOB:
	case DEVILTOWER_BLACKSMITH_ACCESSORY_MOB:
		if (item->GetType() == ITEM_ARMOR &&
			!(item->GetSubType() == ARMOR_BODY || item->GetSubType() == ARMOR_SHIELD || item->GetSubType() == ARMOR_HEAD) &&
			item->GetRefinedVnum())
			return true;
		else
			return false;
		break;
		// END_OF_BUILDING_NPC

	case BLACKSMITH_MOB:
		if (item->GetRefinedVnum() && item->GetRefineSet() < 500)
		{
			return true;
		}
		else
		{
			return false;
		}

	case BLACKSMITH2_MOB:
		if (item->GetRefineSet() >= 500)
		{
			return true;
		}
		else
		{
			return false;
		}

	case ALCHEMIST_MOB:
		if (item->GetRefinedVnum())
			return true;
		break;

	case 20101:
	case 20102:
	case 20103:

		if (item->GetVnum() == ITEM_REVIVE_HORSE_1)
		{
			if (!IsDead())
			{
				from->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Á×Áö ¾ÊÀº ¸»¿¡°Ô ¼±ÃÊ¸¦ ¸ÔÀÏ ¼ö ¾ø½À´Ï´Ù."));
				return false;
			}
			return true;
		}
		else if (item->GetVnum() == ITEM_HORSE_FOOD_1)
		{
			if (IsDead())
			{
				from->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Á×Àº ¸»¿¡°Ô »ç·á¸¦ ¸ÔÀÏ ¼ö ¾ø½À´Ï´Ù."));
				return false;
			}
			return true;
		}
		else if (item->GetVnum() == ITEM_HORSE_FOOD_2 || item->GetVnum() == ITEM_HORSE_FOOD_3)
		{
			return false;
		}
		break;
	case 20104:
	case 20105:
	case 20106:

		if (item->GetVnum() == ITEM_REVIVE_HORSE_2)
		{
			if (!IsDead())
			{
				from->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Á×Áö ¾ÊÀº ¸»¿¡°Ô ¼±ÃÊ¸¦ ¸ÔÀÏ ¼ö ¾ø½À´Ï´Ù."));
				return false;
			}
			return true;
		}
		else if (item->GetVnum() == ITEM_HORSE_FOOD_2)
		{
			if (IsDead())
			{
				from->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Á×Àº ¸»¿¡°Ô »ç·á¸¦ ¸ÔÀÏ ¼ö ¾ø½À´Ï´Ù."));
				return false;
			}
			return true;
		}
		else if (item->GetVnum() == ITEM_HORSE_FOOD_1 || item->GetVnum() == ITEM_HORSE_FOOD_3)
		{
			return false;
		}
		break;
	case 20107:
	case 20108:
	case 20109:

		if (item->GetVnum() == ITEM_REVIVE_HORSE_3)
		{
			if (!IsDead())
			{
				from->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Á×Áö ¾ÊÀº ¸»¿¡°Ô ¼±ÃÊ¸¦ ¸ÔÀÏ ¼ö ¾ø½À´Ï´Ù."));
				return false;
			}
			return true;
		}
		else if (item->GetVnum() == ITEM_HORSE_FOOD_3)
		{
			if (IsDead())
			{
				from->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Á×Àº ¸»¿¡°Ô »ç·á¸¦ ¸ÔÀÏ ¼ö ¾ø½À´Ï´Ù."));
				return false;
			}
			return true;
		}
		else if (item->GetVnum() == ITEM_HORSE_FOOD_1 || item->GetVnum() == ITEM_HORSE_FOOD_2)
		{
			return false;
		}
		break;
	}

	//if (IS_SET(item->GetFlag(), ITEM_FLAG_QUEST_GIVE))
	{
		return true;
	}

	return false;
}

void CHARACTER::ReceiveItem(LPCHARACTER from, LPITEM item)
{
	if (IsPC())
		return;

	switch (GetRaceNum())
	{
	case fishing::CAMPFIRE_MOB:
		if (item->GetType() == ITEM_FISH && (item->GetSubType() == FISH_ALIVE || item->GetSubType() == FISH_DEAD))
			fishing::Grill(from, item);
		else
		{
			// TAKE_ITEM_BUG_FIX
			from->SetQuestNPCID(GetVID());
			// END_OF_TAKE_ITEM_BUG_FIX
			quest::CQuestManager::instance().TakeItem(from->GetPlayerID(), GetRaceNum(), item);
		}
		break;

		// DEVILTOWER_NPC
	case DEVILTOWER_BLACKSMITH_WEAPON_MOB:
	case DEVILTOWER_BLACKSMITH_ARMOR_MOB:
	case DEVILTOWER_BLACKSMITH_ACCESSORY_MOB:
#ifdef ENABLE_DEVILTOWER_LIMIT
		if (item->GetRefinedVnum() != 0 && item->GetRefineSet() != 0 && item->GetRefineSet() < ENABLE_DEVILTOWER_LIMIT)
#else
		if (item->GetRefinedVnum() != 0 && item->GetRefineSet() != 0 && item->GetRefineSet() < 500)
#endif
		{
			from->SetRefineNPC(this);
			from->RefineInformation(item->GetCell(), REFINE_TYPE_MONEY_ONLY);
		}
		else
		{
			from->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀÌ ¾ÆÀÌÅÛÀº °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		}
		break;
		// END_OF_DEVILTOWER_NPC

	case BLACKSMITH_MOB:
	case BLACKSMITH2_MOB:
	case BLACKSMITH_WEAPON_MOB:
	case BLACKSMITH_ARMOR_MOB:
	case BLACKSMITH_ACCESSORY_MOB:
		if (item->GetRefinedVnum())
		{
			from->SetRefineNPC(this);
			from->RefineInformation(item->GetCell(), REFINE_TYPE_NORMAL);
		}
		else
		{
			from->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀÌ ¾ÆÀÌÅÛÀº °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		}
		break;

	case 20101:
	case 20102:
	case 20103:
	case 20104:
	case 20105:
	case 20106:
	case 20107:
	case 20108:
	case 20109:
		if (item->GetVnum() == ITEM_REVIVE_HORSE_1 ||
			item->GetVnum() == ITEM_REVIVE_HORSE_2 ||
			item->GetVnum() == ITEM_REVIVE_HORSE_3)
		{
			from->ReviveHorse();
			item->SetCount(item->GetCount() - 1);
			from->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¸»¿¡°Ô ¼±ÃÊ¸¦ ÁÖ¾ú½À´Ï´Ù."));
		}
		else if (item->GetVnum() == ITEM_HORSE_FOOD_1 ||
			item->GetVnum() == ITEM_HORSE_FOOD_2 ||
			item->GetVnum() == ITEM_HORSE_FOOD_3)
		{
			from->FeedHorse();
			from->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¸»¿¡°Ô »ç·á¸¦ ÁÖ¾ú½À´Ï´Ù."));
			item->SetCount(item->GetCount() - 1);
			EffectPacket(SE_HPUP_RED);
		}
		break;

	default:
		from->SetQuestNPCID(GetVID());
		quest::CQuestManager::instance().TakeItem(from->GetPlayerID(), GetRaceNum(), item);
		break;
	}
}

bool CHARACTER::IsEquipUniqueItem(DWORD dwItemVnum) const
{
	{
		LPITEM u = GetWear(WEAR_UNIQUE1);

		if (u && u->GetVnum() == dwItemVnum)
			return true;
	}

	{
		LPITEM u = GetWear(WEAR_UNIQUE2);

		if (u && u->GetVnum() == dwItemVnum)
			return true;
	}

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	{
		LPITEM u = GetWear(WEAR_COSTUME_MOUNT);
		if (u && u->GetVnum() == dwItemVnum)
			return true;
	}
#endif

	if (dwItemVnum == UNIQUE_ITEM_RING_OF_LANGUAGE)
		return IsEquipUniqueItem(UNIQUE_ITEM_RING_OF_LANGUAGE_SAMPLE);

	return false;
}

// CHECK_UNIQUE_GROUP
bool CHARACTER::IsEquipUniqueGroup(DWORD dwGroupVnum) const
{
	{
		LPITEM u = GetWear(WEAR_UNIQUE1);

		if (u && u->GetSpecialGroup() == (int)dwGroupVnum)
			return true;
	}

	{
		LPITEM u = GetWear(WEAR_UNIQUE2);

		if (u && u->GetSpecialGroup() == (int)dwGroupVnum)
			return true;
	}

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	{
		LPITEM u = GetWear(WEAR_COSTUME_MOUNT);

		if (u && u->GetSpecialGroup() == (int)dwGroupVnum)
			return true;
	}
#endif

	return false;
}
// END_OF_CHECK_UNIQUE_GROUP

void CHARACTER::SetRefineMode(int iAdditionalCell)
{
	m_iRefineAdditionalCell = iAdditionalCell;
	m_bUnderRefine = true;
}

void CHARACTER::ClearRefineMode()
{
	m_bUnderRefine = false;
	SetRefineNPC(NULL);
}

bool CHARACTER::GiveItemFromSpecialItemGroup(DWORD dwGroupNum, std::vector<DWORD>&dwItemVnums,
	std::vector<DWORD>&dwItemCounts, std::vector <LPITEM>&item_gets, int& count)
{
	const CSpecialItemGroup* pGroup = ITEM_MANAGER::instance().GetSpecialItemGroup(dwGroupNum);

	if (!pGroup)
	{
		sys_err("cannot find special item group %d", dwGroupNum);
		return false;
	}

	std::vector <int> idxes;
	int n = pGroup->GetMultiIndex(idxes);

	bool bSuccess;

	for (int i = 0; i < n; i++)
	{
		bSuccess = false;
		int idx = idxes[i];
		DWORD dwVnum = pGroup->GetVnum(idx);
		DWORD dwCount = pGroup->GetCount(idx);
		int	iRarePct = pGroup->GetRarePct(idx);
		LPITEM item_get = NULL;
		switch (dwVnum)
		{
		case CSpecialItemGroup::GOLD:
			PointChange(POINT_GOLD, dwCount);
			bSuccess = true;
			break;

		case CSpecialItemGroup::EXP:
		{
			PointChange(POINT_EXP, dwCount);
			bSuccess = true;
		}
		break;

		case CSpecialItemGroup::MOB:
		{
			int x = GetX() + number(-500, 500);
			int y = GetY() + number(-500, 500);

			LPCHARACTER ch = CHARACTER_MANAGER::instance().SpawnMob(dwCount, GetMapIndex(), x, y, 0, true, -1);
			if (ch)
				ch->SetAggressive();
			bSuccess = true;
		}
		break;
		case CSpecialItemGroup::SLOW:
		{
			AddAffect(AFFECT_SLOW, POINT_MOV_SPEED, -(int)dwCount, AFF_SLOW, 300, 0, true);
			bSuccess = true;
		}
		break;
		case CSpecialItemGroup::DRAIN_HP:
		{
			HP_LL iDropHP = GetMaxHP() * dwCount / 100;
			iDropHP = MIN(iDropHP, GetHP() - 1);
			PointChange(POINT_HP, -iDropHP);
			bSuccess = true;
		}
		break;
		case CSpecialItemGroup::POISON:
		{
			AttackedByPoison(NULL);
			bSuccess = true;
		}
		break;
#ifdef ENABLE_WOLFMAN_CHARACTER
		case CSpecialItemGroup::BLEEDING:
		{
			AttackedByBleeding(NULL);
			bSuccess = true;
		}
		break;
#endif
		case CSpecialItemGroup::MOB_GROUP:
		{
			int sx = GetX() - number(300, 500);
			int sy = GetY() - number(300, 500);
			int ex = GetX() + number(300, 500);
			int ey = GetY() + number(300, 500);
			CHARACTER_MANAGER::instance().SpawnGroup(dwCount, GetMapIndex(), sx, sy, ex, ey, NULL, true);

			bSuccess = true;
		}
		break;
		default:
		{
			item_get = AutoGiveItem(dwVnum, dwCount, iRarePct);

			if (item_get)
			{
				bSuccess = true;
			}
		}
		break;
		}

		if (bSuccess)
		{
			dwItemVnums.push_back(dwVnum);
			dwItemCounts.push_back(dwCount);
			item_gets.push_back(item_get);
			count++;
		}
		else
		{
			return false;
		}
	}
	return bSuccess;
}

// NEW_HAIR_STYLE_ADD
bool CHARACTER::ItemProcess_Hair(LPITEM item, int iDestCell)
{
	if (item->CheckItemUseLevel(GetLevel()) == false)
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¾ÆÁ÷ ÀÌ ¸Ó¸®¸¦ »ç¿ëÇÒ ¼ö ¾ø´Â ·¹º§ÀÔ´Ï´Ù."));
		return false;
	}

	DWORD hair = item->GetVnum();

	switch (GetJob())
	{
	case JOB_WARRIOR:
		hair -= 72000;
		break;

	case JOB_ASSASSIN:
		hair -= 71250;
		break;

	case JOB_SURA:
		hair -= 70500;
		break;

	case JOB_SHAMAN:
		hair -= 69750;
		break;
#ifdef ENABLE_WOLFMAN_CHARACTER
	case JOB_WOLFMAN:
		break;
#endif
	default:
		return false;
		break;
	}

	if (hair == GetPart(PART_HAIR))
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("µ¿ÀÏÇÑ ¸Ó¸® ½ºÅ¸ÀÏ·Î´Â ±³Ã¼ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return true;
	}

	item->SetCount(item->GetCount() - 1);

	SetPart(PART_HAIR, hair);
	UpdatePacket();

	return true;
}
// END_NEW_HAIR_STYLE_ADD

bool CHARACTER::ItemProcess_Polymorph(LPITEM item)
{
#if defined(ENABLE_MAP_195_ALIGNMENT)	
	if (GetMapIndex() == 195)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ALIGNMENT_MAC195_USE_POLYMORPH_ITEM"));
		return false;
	}
#endif
#ifdef TOURNAMENT_PVP_SYSTEM
	if (CTournamentPvP::instance().IsTournamentMap(this, TOURNAMENT_BLOCK_POLY))	
		return false;
#endif
	if (IsPolymorphed())
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀÌ¹Ì µÐ°©ÁßÀÎ »óÅÂÀÔ´Ï´Ù."));
		return false;
	}

	if (true == IsRiding())
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("µÐ°©ÇÒ ¼ö ¾ø´Â »óÅÂÀÔ´Ï´Ù."));
		return false;
	}

	DWORD dwVnum = item->GetSocket(0);

	if (dwVnum == 0)
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Àß¸øµÈ µÐ°© ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
		item->SetCount(item->GetCount() - 1);
		return false;
	}

	const CMob* pMob = CMobManager::instance().Get(dwVnum);

	if (pMob == NULL)
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Àß¸øµÈ µÐ°© ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
		item->SetCount(item->GetCount() - 1);
		return false;
	}

	switch (item->GetVnum())
	{
		case 70104:
		case 70105:
		case 70106:
		case 70107:
		case 71093:
	{
		if (IsRiding() || IsDead())
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("³ªº¸´Ù ³Ê¹« ³ôÀº ·¹º§ÀÇ ¸ó½ºÅÍ·Î´Â º¯½Å ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}
		if (GetMapIndex() == 303)
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("The mine cannot be transformed"));
			return false;
		}
		int iPolymorphLevelLimit = MAX(0, 20 - GetLevel() * 3 / 10);
		if (pMob->m_table.bLevel >= GetLevel() + iPolymorphLevelLimit)
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("³ªº¸´Ù ³Ê¹« ³ôÀº ·¹º§ÀÇ ¸ó½ºÅÍ·Î´Â º¯½Å ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}

		int iDuration = GetSkillLevel(POLYMORPH_SKILL_ID) == 0 ? 5 : (5 + (5 + GetSkillLevel(POLYMORPH_SKILL_ID) / 40 * 25));
		iDuration *= 60;

		DWORD dwBonus = 0;

		dwBonus = (2 + GetSkillLevel(POLYMORPH_SKILL_ID) / 40) * 100;

#ifdef ENABLE_TRANSFORMATION_SKILL_CLOSE_FIX
		if (IsAffectFlag(AFF_GEOMGYEONG))
			RemoveAffect(SKILL_GEOMKYUNG);

		if (IsAffectFlag(AFF_GWIGUM))
			RemoveAffect(SKILL_GWIGEOM);
#endif
		
		AddAffect(AFFECT_POLYMORPH, POINT_POLYMORPH, dwVnum, AFF_POLYMORPH, iDuration, 0, true);
		AddAffect(AFFECT_POLYMORPH, POINT_ATT_BONUS, dwBonus, AFF_POLYMORPH, iDuration, 0, false);

		item->SetCount(item->GetCount() - 1);
	}
	break;

	case 50322:
	{
		if (CPolymorphUtils::instance().PolymorphCharacter(this, item, pMob) == true)
		{
			CPolymorphUtils::instance().UpdateBookPracticeGrade(this, item);
		}
	}
	break;

	default:
		sys_err("POLYMORPH invalid item passed PID(%d) vnum(%d)", GetPlayerID(), item->GetOriginalVnum());
		return false;
	}

	return true;
}

bool CHARACTER::CanDoCube() const
{
	if (m_bIsObserver)	return false;
	if (GetShop())		return false;
	if (GetMyShop())	return false;
	if (m_bUnderRefine)	return false;
	// if (IsWarping()) return false;
	return true;
}

void CHARACTER::AutoRecoveryItemProcess(const EAffectTypes type)
{
	if (true == IsDead() || true == IsStun())
		return;

	if (false == IsPC())
		return;

	if (AFFECT_AUTO_HP_RECOVERY != type && AFFECT_AUTO_SP_RECOVERY != type)
		return;

	if (NULL != FindAffect(AFFECT_STUN))
		return;

	{
		const DWORD stunSkills[] = { SKILL_TANHWAN, SKILL_GEOMPUNG, SKILL_BYEURAK, SKILL_GIGUNG };

		for (size_t i = 0; i < sizeof(stunSkills) / sizeof(DWORD); ++i)
		{
			const CAffect* p = FindAffect(stunSkills[i]);

			if (NULL != p && AFF_STUN == p->dwFlag)
				return;
		}
	}

	const CAffect* pAffect = FindAffect(type);
	const size_t idx_of_amount_of_used = 1;
	const size_t idx_of_amount_of_full = 2;
	if (NULL != pAffect)
	{
		// if (pAffect->lSPCost == 0)//×Ô¶¯Ò©Ë®¶Ô±ÈÃ»ÓÐÕâ¸ö
			// return;

		LPITEM pItem = FindItemByID(pAffect->dwFlag);

		if (NULL != pItem && true == pItem->GetSocket(0))
		{
			if (!CArenaManager::instance().IsArenaMap(GetMapIndex())
			)
			{
				const long amount_of_used = pItem->GetSocket(idx_of_amount_of_used);
				const long amount_of_full = pItem->GetSocket(idx_of_amount_of_full);

				const int32_t avail = amount_of_full - amount_of_used;

				int32_t amount = 0;

				if (AFFECT_AUTO_HP_RECOVERY == type)
				{
					amount = GetMaxHP() - (GetHP() + GetPoint(POINT_HP_RECOVERY));
				}
				else if (AFFECT_AUTO_SP_RECOVERY == type)
				{
					amount = GetMaxSP() - (GetSP() + GetPoint(POINT_SP_RECOVERY));
				}

				if (amount > 0)
				{
					if (avail > amount)
					{
						const int pct_of_used = amount_of_used * 100 / amount_of_full;
						const int pct_of_will_used = (amount_of_used + amount) * 100 / amount_of_full;

						bool bLog = false;


						if ((pct_of_will_used / 10) - (pct_of_used / 10) >= 1)
							bLog = true;
						pItem->SetSocket(idx_of_amount_of_used, amount_of_used + amount, bLog);
					}
					else
					{
						amount = avail;

						ITEM_MANAGER::instance().RemoveItem(pItem);
					}

					if (AFFECT_AUTO_HP_RECOVERY == type)
					{
						PointChange(POINT_HP_RECOVERY, amount);
						EffectPacket(SE_AUTO_HPUP);
					}
					else if (AFFECT_AUTO_SP_RECOVERY == type)
					{
						PointChange(POINT_SP_RECOVERY, amount);
						EffectPacket(SE_AUTO_SPUP);
					}
				}
			}
			else
			{
				pItem->Lock(false);
				pItem->SetSocket(0, false);
				RemoveAffect(const_cast<CAffect*>(pAffect));
			}
		}
		else
		{
			RemoveAffect(const_cast<CAffect*>(pAffect));
		}
	}
}

bool CHARACTER::IsValidItemPosition(TItemPos Pos) const
{
	BYTE window_type = Pos.window_type;
	WORD cell = Pos.cell;

	switch (window_type)
	{
	case RESERVED_WINDOW:
		return false;

	case INVENTORY:
	case EQUIPMENT:
		return cell < (INVENTORY_AND_EQUIP_SLOT_MAX);

	case DRAGON_SOUL_INVENTORY:
		return cell < (DRAGON_SOUL_INVENTORY_MAX_NUM);
#ifdef ENABLE_SPECIAL_STORAGE
	case UPGRADE_INVENTORY:
	case BOOK_INVENTORY:
	case STONE_INVENTORY:
	case CHEST_INVENTORY:
		return cell < (SPECIAL_INVENTORY_MAX_NUM);
#endif
#ifdef ENABLE_SWITCHBOT
	case SWITCHBOT:
		return cell < SWITCHBOT_SLOT_COUNT;
#endif
#ifdef ENABLE_BUFFI_SYSTEM
	case BUFFI_INVENTORY:
		return cell < BUFFI_MAX_SLOT;
#endif
	case SAFEBOX:
		if (NULL != m_pkSafebox)
			return m_pkSafebox->IsValidPosition(cell);
		else
			return false;

	case MALL:
		if (NULL != m_pkMall)
			return m_pkMall->IsValidPosition(cell);
		else
			return false;

	default:
		return false;
	}
}

#define VERIFY_MSG(exp, msg)  \
	if (true == (exp)) { \
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(msg)); \
			return false; \
	}

bool CHARACTER::CanEquipNow(const LPITEM item, const TItemPos & srcCell, const TItemPos & destCell) /*const*/
{
	const TItemTable* itemTable = item->GetProto();
	//BYTE itemType = item->GetType();
	//BYTE itemSubType = item->GetSubType();

	switch (GetJob())
	{
	case JOB_WARRIOR:
		if (item->GetAntiFlag() & ITEM_ANTIFLAG_WARRIOR)
			return false;
		break;

	case JOB_ASSASSIN:
		if (item->GetAntiFlag() & ITEM_ANTIFLAG_ASSASSIN)
			return false;
		break;

	case JOB_SHAMAN:
		if (item->GetAntiFlag() & ITEM_ANTIFLAG_SHAMAN)
			return false;
		break;

	case JOB_SURA:
		if (item->GetAntiFlag() & ITEM_ANTIFLAG_SURA)
			return false;
		break;
#ifdef ENABLE_WOLFMAN_CHARACTER
	case JOB_WOLFMAN:
		if (item->GetAntiFlag() & ITEM_ANTIFLAG_WOLFMAN)
			return false;
		break;
#endif
	}

	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
	{
		long limit = itemTable->aLimits[i].lValue;
		switch (itemTable->aLimits[i].bType)
		{
		case LIMIT_LEVEL:
			if (GetLevel() < limit)
			{
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("·¹º§ÀÌ ³·¾Æ Âø¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
				return false;
			}
			break;

		case LIMIT_STR:
			if (GetPoint (POINT_ST) < limit)
			{
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("±Ù·ÂÀÌ ³·¾Æ Âø¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
				return false;
			}
			break;

		case LIMIT_INT:
			if (GetPoint (POINT_IQ) < limit)
			{
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Áö´ÉÀÌ ³·¾Æ Âø¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
				return false;
			}
			break;

		case LIMIT_DEX:
			if (GetPoint (POINT_DX) < limit)
			{
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¹ÎÃ¸ÀÌ ³·¾Æ Âø¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
				return false;
			}
			break;

		case LIMIT_CON:
			if (GetPoint (POINT_HT) < limit)
			{
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Ã¼·ÂÀÌ ³·¾Æ Âø¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
				return false;
			}
			break;
		}
	}

	if (item->GetWearFlag() & WEARABLE_UNIQUE)
	{
		if ((GetWear(WEAR_UNIQUE1) && GetWear(WEAR_UNIQUE1)->IsSameSpecialGroup(item)) ||
			(GetWear(WEAR_UNIQUE2) && GetWear(WEAR_UNIQUE2)->IsSameSpecialGroup(item)) ||

			(item->GetValue(1) &&
			((GetWear(WEAR_UNIQUE1) && GetWear(WEAR_UNIQUE1)->GetValue(1) == item->GetValue(1)) ||
			(GetWear(WEAR_UNIQUE2) && GetWear(WEAR_UNIQUE2)->GetValue(1) == item->GetValue(1)))) ||

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
			(GetWear(WEAR_COSTUME_MOUNT) && GetWear(WEAR_COSTUME_MOUNT)->IsSameSpecialGroup(item))
			)
#endif
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("°°Àº Á¾·ùÀÇ À¯´ÏÅ© ¾ÆÀÌÅÛ µÎ °³¸¦ µ¿½Ã¿¡ ÀåÂøÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}

		if (marriage::CManager::instance().IsMarriageUniqueItem(item->GetVnum()) &&
			!marriage::CManager::instance().IsMarried(GetPlayerID()))
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("°áÈ¥ÇÏÁö ¾ÊÀº »óÅÂ¿¡¼­ ¿¹¹°À» Âø¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}
	}
#ifdef ENABLE_WEDDING_COSTUME_EQUIP_FIX
	if (item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_BODY)
	{
		LPITEM atakanxd = GetWear(WEAR_BODY);
		if (atakanxd && (atakanxd->GetVnum() >= 11901 && atakanxd->GetVnum() <= 11914))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot wear a wedding dress while riding a horse."));
			return false;
		}
	}

	if (item->GetVnum() >= 11901 && item->GetVnum() <= 11914)
	{
		LPITEM atakan = GetWear(WEAR_COSTUME_BODY);
		if (atakan && (atakan->GetType() == ITEM_COSTUME && atakan->GetSubType() == COSTUME_BODY))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot wear a wedding dress while riding a horse."));
			return false;
		}
	}
#endif
#ifdef ENABLE_DS_SET_BONUS
	if ((DragonSoul_IsDeckActivated()) && (item->IsDragonSoul())) {
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("DS_DEACTIVATE_REQ"));
		return false;
	}
#endif

	return true;
}

bool CHARACTER::CanUnequipNow(const LPITEM item, const TItemPos & srcCell, const TItemPos & destCell) /*const*/
{
	if (ITEM_BELT == item->GetType())
		VERIFY_MSG(CBeltInventoryHelper::IsExistItemInBeltInventory(this), "Çë½«Ñü´ø°ü¹üÖÐ´¢´æÒ©¼ÁÒÆ³ýºó³¢ÊÔ");

	if (IS_SET(item->GetFlag(), ITEM_FLAG_IRREMOVABLE))
		return false;

#ifdef ENABLE_DS_SET_BONUS
	if ((DragonSoul_IsDeckActivated()) && (item->IsDragonSoul())) {
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("DS_DEACTIVATE_REQ"));
		return false;
	}
#endif

	{
		int pos = -1;

		if (item->IsDragonSoul())
			pos = GetEmptyDragonSoulInventory(item);
		else
			pos = GetEmptyInventory(item->GetSize());

		VERIFY_MSG(-1 == pos, "¼ÒÁöÇ°¿¡ ºó °ø°£ÀÌ ¾ø½À´Ï´Ù.");
	}



	return true;
}

#ifdef ENABLE_ITEM_TRANSACTIONS
void CHARACTER::ItemTransactionSell(std::vector<TItemTransactionsInfo>& v_item)
{
	int64_t totalPrice = 0;
	int64_t CharacterMoney = GetGold();

	for (const TItemTransactionsInfo& info : v_item)
	{
		LPITEM item = WindowTypeGetItem(info.pos, info.window);
		if (!item)
			continue;

		if (true == item->isLocked() || item->IsEquipped())
			continue;

		if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_SELL))
			continue;

		MAX_COUNT bCount = item->GetCount();
		int64_t itemPrice = item->GetShopBuyPrice();
		itemPrice *= bCount;

		const int64_t nTotalMoney = static_cast<int64_t>(CharacterMoney) + static_cast<int64_t>(itemPrice) + static_cast<int64_t>(totalPrice);
		if (GOLD_MAX <= nTotalMoney)
		{
			break;
		}
		totalPrice += itemPrice;
#ifdef ENABLE_ITEM_SELL_LOG
		TItemSellLog log;
		memcpy(&log.alSockets, item->GetSockets(), sizeof(log.alSockets));
		memcpy(&log.aAttr, item->GetAttributes(), sizeof(log.aAttr));
		log.dwVnum = item->GetVnum();
		log.dwCount = item->GetCount();
		log.pid = GetPlayerID();
		snprintf(log.logType, sizeof(log.logType), "ITEM_SELL");
		DBManager::instance().DirectQuery(ItemSellLogQuery(log).c_str());
#endif
		ITEM_MANAGER::instance().RemoveItem(item);
	}
	PointChange(POINT_GOLD, totalPrice, false);
}

void CHARACTER::ItemTransactionDelete(std::vector<TItemTransactionsInfo>& v_item)
{
	for (const TItemTransactionsInfo& info : v_item)
	{
		LPITEM item = WindowTypeGetItem(info.pos, info.window);
		if (!item)
			continue;

		if (true == item->isLocked() || item->IsEquipped())
			continue;

#ifdef ENABLE_ITEM_SELL_LOG
		TItemSellLog log;
		memcpy(&log.alSockets, item->GetSockets(), sizeof(log.alSockets));
		memcpy(&log.aAttr, item->GetAttributes(), sizeof(log.aAttr));
		log.dwVnum = item->GetVnum();
		log.dwCount = item->GetCount();
		log.pid = GetPlayerID();
		snprintf(log.logType, sizeof(log.logType), "ITEM_DELETE");
		DBManager::instance().DirectQuery(ItemSellLogQuery(log).c_str());
#endif

		ITEM_MANAGER::instance().RemoveItem(item);
	
	}
}

#endif

#ifdef ENABLE_JEWELS_RENEWAL
void CHARACTER::UseAddJewels(LPITEM item, LPITEM targetItem)
{
	if ((targetItem->IsAccessoryForSocket() && item->GetVnum() == 50621) || targetItem->CanPutIntoNew(item))
	{
		if (targetItem->GetAccessorySocketMaxGrade() < ITEM_ACCESSORY_SOCKET_MAX_NUM)
		{
			if (number(1, 100) <= 50)
			{
				targetItem->SetAccessorySocketMaxGrade(targetItem->GetAccessorySocketMaxGrade() + 1);
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼ÒÄÏÀÌ ¼º°øÀûÀ¸·Î Ãß°¡µÇ¾ú½À´Ï´Ù."));
			}
			else
			{
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("¼ÒÄÏ Ãß°¡¿¡ ½ÇÆÐÇÏ¿´½À´Ï´Ù."));
			}

			item->SetCount(item->GetCount() - 1);
		}
		else
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀÌ ¾×¼¼¼­¸®¿¡´Â ´õÀÌ»ó ¼ÒÄÏÀ» Ãß°¡ÇÒ °ø°£ÀÌ ¾ø½À´Ï´Ù."));
		}
	}
	else
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ÀÌ ¾ÆÀÌÅÛÀ¸·Î ¼ÒÄÏÀ» Ãß°¡ÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
	}
}

void CHARACTER::UsePutIntoJewels(LPITEM item, LPITEM targetItem)
{
	int openSocketCount = targetItem->GetAccessorySocketGrade();
	bool isPermaEq = targetItem->IsPermaEquipment();

	bool isPermaJewels = item->isPermaJewels();
	

	if (targetItem->IsAccessoryForSocket() && item->CanPutInto(targetItem))
	{
		if (openSocketCount < targetItem->GetAccessorySocketMaxGrade())
		{
			if (isPermaJewels)
			{
				if (openSocketCount != 0 && !isPermaEq)
				{
					ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("PERMANENT_JEWEWL_INFO_01"));
					return;
				}

				if (!isPermaEq)
					targetItem->SetSocket(2, 62);
			}
			else
			{
				if (openSocketCount != 0 && isPermaEq)
				{
					ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("PERMANENT_JEWEWL_INFO_02"));
					return;
				}
			}
		}
		else
		{
			if (targetItem->GetAccessorySocketMaxGrade() == ITEM_ACCESSORY_SOCKET_MAX_NUM)
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ADD_JEWEL_INFO_04"));
			else
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ADD_JEWEL_INFO_01"));
			return;
		}
	}
	else
	{
		if (!targetItem->CanPutIntoNew(item))
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("PERMANENT_JEWEWL_INFO_03"));
			return;
		}

		if (openSocketCount < targetItem->GetAccessorySocketMaxGrade())
		{
			if (isPermaJewels)
			{
				if (openSocketCount != 0 && !isPermaEq)
				{
					ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("PERMANENT_JEWEWL_INFO_01"));
					return;
				}

				if (!isPermaEq)
					targetItem->SetSocket(2, 62);
			}
			else
			{
				if (openSocketCount != 0 && isPermaEq)
				{
					ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("PERMANENT_JEWEWL_INFO_02"));
					return;
				}
			}
		}
		else
		{
			if (targetItem->GetAccessorySocketMaxGrade() == ITEM_ACCESSORY_SOCKET_MAX_NUM)
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ADD_JEWEL_INFO_04"));
			else
				ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ADD_JEWEL_INFO_01"));
			return;
		}
	}

	targetItem->SetAccessorySocketGrade(openSocketCount + 1);
	ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ADDING_JEWEL_SUCCESFULL"));
	item->SetCount(item->GetCount() - 1);
}
#endif

#ifdef ENABLE_LEADERSHIP_EXTENSION
void CHARACTER::CheckPartyBonus()
{
	if (!IsPC() || GetParty())
		return;

	int bonusID = GetQuestFlag("party.bonus_id");
	float k = (float) GetSkillPowerByLevel( MIN(SKILL_MAX_LEVEL, GetLeadershipSkillLevel() ) )/ 100.0f;
	
	if (bonusID)	
	{
		if(FindAffect(AFFECT_PARTY_BONUS))
		{
			RemoveAffect(AFFECT_PARTY_BONUS);
			AddAffect(AFFECT_PARTY_BONUS, bonusID, 0, 0, INFINITE_AFFECT_DURATION_REALTIME, 0, false);
		}
		else
			AddAffect(AFFECT_PARTY_BONUS, bonusID, 0, 0, INFINITE_AFFECT_DURATION_REALTIME, 0, false);
	}
	else
	{
		if(FindAffect(AFFECT_PARTY_BONUS))
			RemoveAffect(AFFECT_PARTY_BONUS);
	}

	switch (bonusID)
	{
		case PARTY_ROLE_ATTACKER:
			{
				int iBonus = (int) (10 + 60 * k);

				if (GetPoint(POINT_PARTY_ATTACKER_BONUS) != iBonus)
				{
					PointChange(POINT_PARTY_ATTACKER_BONUS, iBonus - GetPoint(POINT_PARTY_ATTACKER_BONUS));
				}
			}
			break;

		case PARTY_ROLE_TANKER:
			{
				int iBonus = (int) (50 + 1450 * k);

				if (GetPoint(POINT_PARTY_TANKER_BONUS) != iBonus)
				{
					PointChange(POINT_PARTY_TANKER_BONUS, iBonus - GetPoint(POINT_PARTY_TANKER_BONUS));
				}
			}
			break;

		case PARTY_ROLE_BUFFER:
			{
				int iBonus = (int) (5 + 45 * k);

				if (GetPoint(POINT_PARTY_BUFFER_BONUS) != iBonus)
				{
					PointChange(POINT_PARTY_BUFFER_BONUS, iBonus - GetPoint(POINT_PARTY_BUFFER_BONUS));
				}
			}
			break;

		case PARTY_ROLE_SKILL_MASTER:
			{
				int iBonus = (int) (10*k);

				if (GetPoint(POINT_PARTY_SKILL_MASTER_BONUS) != iBonus)
				{
					PointChange(POINT_PARTY_SKILL_MASTER_BONUS, iBonus - GetPoint(POINT_PARTY_SKILL_MASTER_BONUS));
				}
			}
			break;
		case PARTY_ROLE_HASTE:
			{
				int iBonus = (int) (1+5*k);
				if (GetPoint(POINT_PARTY_HASTE_BONUS) != iBonus)
				{
					PointChange(POINT_PARTY_HASTE_BONUS, iBonus - GetPoint(POINT_PARTY_HASTE_BONUS));
				}
			}
			break;
		case PARTY_ROLE_DEFENDER:
			{
				int iBonus = (int) (5+30*k);
				if (GetPoint(POINT_PARTY_DEFENDER_BONUS) != iBonus)
				{
					PointChange(POINT_PARTY_DEFENDER_BONUS, iBonus - GetPoint(POINT_PARTY_DEFENDER_BONUS));
				}
			}
			break;
			
		default:
			break;
	}
}
#endif

#ifdef ENABLE_6TH_7TH_ATTR
void CHARACTER::Give67AttrItem()
{
	LPITEM item = GetAttrInventoryItem();
	if (!item)
		return;

	int emptyPos = GetEmptyInventory(item->GetSize());

	if (emptyPos == -1)
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Not enough space in inventory"));
		return;
	}	

	item->RemoveFromCharacter();
	item->AddToCharacter(this, TItemPos(INVENTORY, emptyPos));
}

void CHARACTER::Add67Attr(LPITEM addAttr, LPITEM item)
{
	if (!addAttr || !item)
		return;
	
	if (item->GetAttributeSetIndex() == -1)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
		return;
	}

	if (item->GetAttributeCount() < 5)
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ADD_67ATTRIBUTE_OF_THIS_ITEM"));
		return;
	}

	if (item->Get67AttrCount() == 2)
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ADD_67ATTRIBUTE_NO"));
		return;
	}

	addAttr->SetCount(addAttr->GetCount() - 1);
	item->Add67Attr();
	ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ADD_SUCCESFULL6_7"));
}

void CHARACTER::Change67Attr(LPITEM changer, LPITEM item)
{
	if (!changer || !item)
		return;

	if (item->Get67AttrIdx() == -1)
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("CANT_CHANGE_THE_ATTRIBUTE_OF_THIS_ITEM"));
		return;
	}

	if (item->Get67AttrCount() != 2)
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("CANT_CHANGE_THE_ATTRIBUTE_OF_THIS_ITEM2"));
		return;
	}

	if (changer->GetVnum() == 72351)
	{
		if (number(0, 100) <= 20)
		{
			ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("IT_WAS_FAILED_SHIBAI6_7"));
			changer->SetCount(changer->GetCount() - 1);
			return;
		}
	}

	changer->SetCount(changer->GetCount() - 1);
	item->Change67Attr();
	ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("IT_WAS_SUCCESFULL6_7"));
#ifdef ENABLE_PLAYER_STATS_SYSTEM
	PointChange(POINT_USE_ENCHANTED_ITEM, 1);
#endif
}

void CHARACTER::Decrease67AttrCooldown(LPITEM cooldownItem)
{
	if (!cooldownItem)
		return;

	const int currentTime = get_global_time();
	const int endTime = GetQuestFlag("67attr.time");

	int remainingTime = endTime - currentTime;

	if (remainingTime <= 0)
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("LENGQUESHIJIANWUPIN."));
		return;
	}
		
	switch (cooldownItem->GetVnum())
	{
		case 24601: // 25 percent item
		{
			remainingTime = remainingTime * 3 / 4;
			break;
		}
		case 24602:// 50 percent item
		{
			remainingTime = remainingTime / 2;
			break;
		}
	default:
		return;
	}

	cooldownItem->SetCount(cooldownItem->GetCount() - 1);
	SetQuestFlag("67attr.time", currentTime + remainingTime);
}

#endif

#ifdef ENABLE_BUFFI_SYSTEM
bool CHARACTER::CanBuffiEquipItem(TItemPos& cell, LPITEM item)
{
	int wearCell = item->FindBuffiEquipCell();
	if (wearCell == -1)
		return false;

	if (GetItem(TItemPos(BUFFI_INVENTORY, wearCell)))
		return false;

	if (item->GetAntiFlag() & ITEM_ANTIFLAG_SHAMAN)
		return false;

	if (item->GetAntiFlag() & ITEM_ANTIFLAG_FEMALE)
		return false;

	if (item->GetVnum() == 11903 || item->GetVnum() == 11904 )
		return false;

	cell.cell = wearCell;
	return true;
}
#endif

#ifdef ENABLE_COSTUME_ATTR
void CHARACTER::ChangeCostumeAttr(LPITEM costume, LPITEM changer)
{
	if (changer->GetValue(0) == 1)
	{
		const int deleteIdx = changer->GetValue(1);
		if (costume->GetAttributeType(deleteIdx) == 0)
			return;

		costume->SetAttribute(deleteIdx, 0, 0);
		changer->SetCount(changer->GetCount() - 1);
	}
	else if (changer->GetValue(0) == 2)
	{
		const BYTE changeType = changer->GetValue(1);
		const BYTE subType = costume->GetSubType();

		if ((changeType == 1 && subType != COSTUME_HAIR) || (changeType == 2 && subType != COSTUME_BODY) || (changeType == 3 && subType != COSTUME_WEAPON))
			return;

		TItemApply changerBonus = changer->GetProto()->aApplies[0];
		int applyIdx = costume->FindAttribute(changerBonus.bType);
		if (changer->GetValue(2) == 1)
		{
			if (applyIdx == -1)
				return;

			if (costume->GetAttributeValue(applyIdx) == changerBonus.lValue)
				return;

			costume->SetAttribute(applyIdx, changerBonus.bType, changerBonus.lValue);
			changer->SetCount(changer->GetCount() - 1);
		}
		else
		{
			if (applyIdx != -1)
				return;

			applyIdx = costume->FindAttribute(0);
			if (applyIdx == -1)
				return;

			costume->SetAttribute(applyIdx, changerBonus.bType, changerBonus.lValue);
			changer->SetCount(changer->GetCount() - 1);
		}
	}
	else
	{
		return;
	}
#ifdef ENABLE_PLAYER_STATS_SYSTEM
	PointChange(POINT_USE_ENCHANTED_ITEM, 1);
#endif
}
#endif

#ifdef ENABLE_EVENT_MANAGER

void CHARACTER::ConvertEventItems(LPITEM item)
{
	if (!item)
		return;

	BYTE neededItemCount{};
	const DWORD itemVnum{ item->GetVnum() };
	DWORD giveItemVnum{};

	switch (item->GetVnum())
	{
	case 79505:
	{
		neededItemCount = 24;
		giveItemVnum = 79506;
		break;
	}	
	case 79512:
	{
		neededItemCount = 18;
		giveItemVnum = 79513;
		break;
	}	
	case 79603:
	{
		neededItemCount = 25;
		giveItemVnum = 79604;
		break;
	}	
	case 50096:
	{
		neededItemCount = 50;
		giveItemVnum = 50265;
		break;
	}

	default:
		return;
	}

	if (CountSpecifyItem(itemVnum) < neededItemCount)
	{
		ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("NOT_ENOUGH_MATERIAL"));
		return;
	}

	RemoveSpecifyItem(itemVnum, neededItemCount);
	AutoGiveItem(giveItemVnum, 1);
}

#endif