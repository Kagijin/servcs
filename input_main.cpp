#include "stdafx.h"
#include "constants.h"
#include "config.h"
#include "utils.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "buffer_manager.h"
#include "packet.h"
#include "protocol.h"
#include "char.h"
#include "char_manager.h"
#include "item.h"
#include "item_manager.h"
#include "cmd.h"
#include "shop.h"
#include "shop_manager.h"
#include "safebox.h"
#include "regen.h"
#include "battle.h"
#include "exchange.h"
#include "questmanager.h"
#include "profiler.h"
#include "messenger_manager.h"
#include "party.h"
#include "p2p.h"
#include "affect.h"
#include "guild.h"
#include "guild_manager.h"

#include "banword.h"
#include "unique_item.h"
#include "building.h"
#include "locale_service.h"
#include "gm.h"
#include "spam.h"
#include "ani.h"
#include "motion.h"
#include "OXEvent.h"
#include "locale_service.h"
#include "DragonSoul.h"
#include "belt_inventory_helper.h" // @fixme119
#include "../../common/Controls.h"
#include "input.h"
#ifdef ENABLE_SWITCHBOT
#include "new_switchbot.h"
#endif

#ifdef ENABLE_IN_GAME_LOG_SYSTEM
#include "InGameLogManager.h"
#endif
#ifdef ENABLE_NEW_PET_SYSTEM
#include "New_PetSystem.h"
#endif
#ifdef ENABLE_EXTENDED_BATTLE_PASS
#include "battlepass_manager.h"
#endif
#ifdef ENABLE_MINI_GAME_BNW
#include "minigame_bnw.h"
#endif
#ifdef ENABLE_MINI_GAME_CATCH_KING
#include "minigame_catchking.h"
#endif
#ifdef ENABLE_BOT_PLAYER
#include "BotPlayer.h"
#endif
#ifdef ENABLE_GOLDTIGER_BETTING
extern bool Quiz_IsActive();
extern void Quiz_CheckAnswer(LPCHARACTER ch, const char* msg);
#endif

#define ENABLE_CHAT_COLOR_SYSTEM
#define ENABLE_CHAT_SPAMLIMIT
#define ENABLE_WHISPER_CHAT_SPAMLIMIT
#define ENABLE_CHECK_GHOSTMODE

static int __deposit_limit()
{
	return (1000 * 10000);
}

void SendBlockChatInfo(LPCHARACTER ch, int sec)
{
	if (sec <= 0)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("채팅 금지 상태입니다."));
		return;
	}

	long hour = sec / 3600;
	sec -= hour * 3600;
	long min = (sec / 60);
	sec -= min * 60;

	char buf[128 + 1];
	if (hour > 0 && min > 0)
		snprintf (buf, sizeof (buf), LC_TEXT ("%d 시간 %d 분 %d 초 동안 채팅금지 상태입니다"), hour, min, sec);
	else if (hour > 0 && min == 0)
		snprintf (buf, sizeof (buf), LC_TEXT ("%d 시간 %d 초 동안 채팅금지 상태입니다"), hour, sec);
	else if (hour == 0 && min > 0)
		snprintf (buf, sizeof (buf), LC_TEXT ("%d 분 %d 초 동안 채팅금지 상태입니다"), min, sec);
	else
		snprintf (buf, sizeof (buf), LC_TEXT ("%d 초 동안 채팅금지 상태입니다"), sec);

	ch->ChatPacket(CHAT_TYPE_INFO, buf);
}

EVENTINFO(spam_event_info)
{
	char host[MAX_HOST_LENGTH + 1];

	spam_event_info()
	{
		::memset(host, 0, MAX_HOST_LENGTH + 1);
	}
};

typedef boost::unordered_map<std::string, std::pair<unsigned int, LPEVENT> > spam_score_of_ip_t;
spam_score_of_ip_t spam_score_of_ip;

EVENTFUNC(block_chat_by_ip_event)
{
	spam_event_info* info = dynamic_cast<spam_event_info*>(event->info);

	if (info == NULL)
	{
		sys_err("block_chat_by_ip_event> <Factor> Null pointer");
		return 0;
	}

	const char* host = info->host;

	spam_score_of_ip_t::iterator it = spam_score_of_ip.find(host);

	if (it != spam_score_of_ip.end())
	{
		it->second.first = 0;
		it->second.second = NULL;
	}

	return 0;
}

bool SpamBlockCheck(LPCHARACTER ch, const char* const buf, const size_t buflen)
{
	if (ch->GetLevel() < g_iSpamBlockMaxLevel)
	{
		spam_score_of_ip_t::iterator it = spam_score_of_ip.find(ch->GetDesc()->GetHostName());

		if (it == spam_score_of_ip.end())
		{
			spam_score_of_ip.insert(std::make_pair(ch->GetDesc()->GetHostName(), std::make_pair(0, (LPEVENT)NULL)));
			it = spam_score_of_ip.find(ch->GetDesc()->GetHostName());
		}

		if (it->second.second)
		{
			SendBlockChatInfo(ch, event_time(it->second.second) / passes_per_sec);
			return true;
		}

		unsigned int score;
		const char* word = SpamManager::instance().GetSpamScore(buf, buflen, score);

		it->second.first += score;

		if (word)
			sys_log(0, "SPAM_SCORE: %s text: %s score: %u total: %u word: %s", ch->GetName(), buf, score, it->second.first, word);

		if (it->second.first >= g_uiSpamBlockScore)
		{
			spam_event_info* info = AllocEventInfo<spam_event_info>();
			strlcpy(info->host, ch->GetDesc()->GetHostName(), sizeof(info->host));

			it->second.second = event_create(block_chat_by_ip_event, info, PASSES_PER_SEC(g_uiSpamBlockDuration));
			sys_log(0, "SPAM_IP: %s for %u seconds", info->host, g_uiSpamBlockDuration);

			SendBlockChatInfo(ch, event_time(it->second.second) / passes_per_sec);

			return true;
		}
	}

	return false;
}

enum
{
	TEXT_TAG_PLAIN,
	TEXT_TAG_TAG, // ||
	TEXT_TAG_COLOR, // |cffffffff
	TEXT_TAG_HYPERLINK_START, // |H
	TEXT_TAG_HYPERLINK_END, // |h ex) |Hitem:1234:1:1:1|h
	TEXT_TAG_RESTORE_COLOR,
};

int GetTextTag(const char* src, int maxLen, int& tagLen, std::string& extraInfo)
{
	tagLen = 1;

	if (maxLen < 2 || *src != '|')
		return TEXT_TAG_PLAIN;

	const char* cur = ++src;

	if (*cur == '|')
	{
		tagLen = 2;
		return TEXT_TAG_TAG;
	}
	else if (*cur == 'c') // color |cffffffffblahblah|r
	{
		tagLen = 2;
		return TEXT_TAG_COLOR;
	}
	else if (*cur == 'H')
	{
		tagLen = 2;
		return TEXT_TAG_HYPERLINK_START;
	}
	else if (*cur == 'h') // end of hyperlink
	{
		tagLen = 2;
		return TEXT_TAG_HYPERLINK_END;
	}

	return TEXT_TAG_PLAIN;
}

void GetTextTagInfo(const char* src, int src_len, int& hyperlinks, bool& colored)
{
	colored = false;
	hyperlinks = 0;

	int len;
	std::string extraInfo;

	for (int i = 0; i < src_len;)
	{
		int tag = GetTextTag(&src[i], src_len - i, len, extraInfo);

		if (tag == TEXT_TAG_HYPERLINK_START)
			++hyperlinks;

		if (tag == TEXT_TAG_COLOR)
			colored = true;

		i += len;
	}
}

int ProcessTextTag(LPCHARACTER ch, const char* c_pszText, size_t len)
{
	int hyperlinks;
	bool colored;

	GetTextTagInfo(c_pszText, len, hyperlinks, colored);

	if (colored == true && hyperlinks == 0)
		return 4;

#ifdef ENABLE_NEWSTUFF
	if (g_bDisablePrismNeed)
	{
		return 0;
	}
#endif

	int nPrismCount = ch->CountSpecifyItem (ITEM_PRISM);

	if (nPrismCount < hyperlinks)
		return 1;

	if (!ch->GetMyShop())
	{
		ch->RemoveSpecifyItem (ITEM_PRISM, hyperlinks);
		return 0;
	} else
	{
		int sellingNumber = ch->GetMyShop()->GetNumberByVnum (ITEM_PRISM);
		if (nPrismCount - sellingNumber < hyperlinks)
		{
			return 2;
		} else
		{
			ch->RemoveSpecifyItem (ITEM_PRISM, hyperlinks);
			return 0;
		}
	}
	//굶都덜쯤칵훰0 써棺壇槨4페儉都덜쯤
	return 4;
}

int CInputMain::Whisper(LPCHARACTER ch, const char* data, size_t uiBytes)
{
	const TPacketCGWhisper* pinfo = reinterpret_cast<const TPacketCGWhisper*>(data);

	if (uiBytes < pinfo->wSize)
		return -1;

	int iExtraLen = pinfo->wSize - sizeof(TPacketCGWhisper);

	if (iExtraLen < 0)
	{
		sys_err("invalid packet length (len %d size %u buffer %u)", iExtraLen, pinfo->wSize, uiBytes);
		ch->GetDesc()->SetPhase(PHASE_CLOSE);
		return -1;
	}
#ifdef __ENABLE_OX_BLOCK__
	if (ch->GetMapIndex() == 113)
	{
		if (int(quest::CQuestManager::instance().GetEventFlag("ox_chat_whisper")) == 1)
		{
			if (ch->GetGMLevel() == GM_PLAYER)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ox_whisper_block"));
				return iExtraLen;
			}
		}
	}
#endif
#ifdef ENABLE_WHISPER_CHAT_SPAMLIMIT
	if (ch->IncreaseChatCounter() >= 10)
	{
		ch->GetDesc()->DelayedDisconnect(0);
		return (iExtraLen);
	}
#endif
#if defined(ENABLE_MAP_195_ALIGNMENT)
	if (ch->GetMapIndex() == 195  && (ch->GetGMLevel() < GM_IMPLEMENTOR))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ALIGNMENT_MAC_195_WHISPER"));
		return (iExtraLen);
	}
#endif
	if (ch->FindAffect(AFFECT_BLOCK_CHAT))
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("채팅 금지 상태입니다."));
		return (iExtraLen);
	}

	LPCHARACTER pkChr = CHARACTER_MANAGER::instance().FindPC(pinfo->szNameTo);

	if (pkChr == ch)
		return (iExtraLen);

	LPDESC pkDesc = NULL;
	// BYTE bOpponentEmpire = 0;
	
	if (ch->IsBlockMode(BLOCK_WHISPER))
	{
		if (ch->GetDesc())
		{
			TPacketGCWhisper pack;
			pack.bHeader = HEADER_GC_WHISPER;
			pack.bType = WHISPER_TYPE_SENDER_BLOCKED;
			pack.wSize = sizeof(TPacketGCWhisper);
			strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));
			ch->GetDesc()->Packet(&pack, sizeof(pack));
		}
		return iExtraLen;
	}

	if (!pkChr)
	{
		CCI* pkCCI = P2P_MANAGER::instance().Find(pinfo->szNameTo);

		if (pkCCI)
		{
			pkDesc = pkCCI->pkDesc;
			pkDesc->SetRelay(pinfo->szNameTo);
			// bOpponentEmpire = pkCCI->bEmpire;
		}
	}
	else
	{
		pkDesc = pkChr->GetDesc();
	}

	if (!pkDesc)
	{
		if (ch->GetDesc())
		{
			TPacketGCWhisper pack;

			pack.bHeader = HEADER_GC_WHISPER;
			pack.bType = WHISPER_TYPE_NOT_EXIST;
			pack.wSize = sizeof(TPacketGCWhisper);
			strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));
			ch->GetDesc()->Packet(&pack, sizeof(TPacketGCWhisper));
			sys_log(0, "WHISPER: no player");
		}
	}
	else
	{
		if (ch->IsBlockMode(BLOCK_WHISPER))
		{
			if (ch->GetDesc())
			{
				TPacketGCWhisper pack;
				pack.bHeader = HEADER_GC_WHISPER;
				pack.bType = WHISPER_TYPE_SENDER_BLOCKED;
				pack.wSize = sizeof(TPacketGCWhisper);
				strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));
				ch->GetDesc()->Packet(&pack, sizeof(pack));
			}
		}
		else if (pkChr && pkChr->IsBlockMode(BLOCK_WHISPER))
		{
			if (ch->GetDesc())
			{
				TPacketGCWhisper pack;
				pack.bHeader = HEADER_GC_WHISPER;
				pack.bType = WHISPER_TYPE_TARGET_BLOCKED;
				pack.wSize = sizeof(TPacketGCWhisper);
				strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));
				ch->GetDesc()->Packet(&pack, sizeof(pack));
			}
		}
		else
		{
			BYTE bType = WHISPER_TYPE_NORMAL;
			
			char buf[CHAT_MAX_LEN + 1];
			strlcpy(buf, data + sizeof(TPacketCGWhisper), MIN(iExtraLen + 1, sizeof(buf)));
			const size_t buflen = strlen(buf);

			if (true == SpamBlockCheck(ch, buf, buflen))
			{
				if (!pkChr)
				{
					CCI* pkCCI = P2P_MANAGER::instance().Find(pinfo->szNameTo);

					if (pkCCI)
					{
						pkDesc->SetRelay("");
					}
				}
				return iExtraLen;
			}
			CBanwordManager::instance().ConvertString(buf, buflen);

			int processReturn = ProcessTextTag (ch, buf, buflen);

			if (0 != processReturn)
			{
				if (ch->GetDesc())
				{
					TItemTable* pTable = ITEM_MANAGER::instance().GetTable (ITEM_PRISM);

					if (pTable)
					{
						char buf[128];
						int len;
						if (3 == processReturn)
						{
							len = snprintf (buf, sizeof (buf), LC_TEXT ("사용할수 없습니다."), pTable->szLocaleName);
						}
						else
						{
							len = snprintf (buf, sizeof (buf), LC_TEXT ("%s이 필요합니다."), pTable->szLocaleName);
						}

						if (len < 0 || len >= (int)sizeof (buf))
						{
							len = sizeof (buf) - 1;
						}

						++len;

						TPacketGCWhisper pack;

						pack.bHeader = HEADER_GC_WHISPER;
						pack.bType = WHISPER_TYPE_ERROR;
						pack.wSize = sizeof (TPacketGCWhisper) + len;
						strlcpy (pack.szNameFrom, pinfo->szNameTo, sizeof (pack.szNameFrom));

						ch->GetDesc()->BufferedPacket (&pack, sizeof (pack));
						ch->GetDesc()->Packet (buf, len);

						sys_log (0, "WHISPER: not enough %s: char: %s", pTable->szLocaleName, ch->GetName());
					}
				}

				pkDesc->SetRelay ("");
				return (iExtraLen);
			}
			
			if (ch->IsGM())
				bType = (bType & 0xF0) | WHISPER_TYPE_GM;

			if (buflen > 0)
			{
				TPacketGCWhisper pack;

				pack.bHeader = HEADER_GC_WHISPER;
				pack.wSize = sizeof(TPacketGCWhisper) + buflen;
				pack.bType = bType;
				strlcpy(pack.szNameFrom, ch->GetName(), sizeof(pack.szNameFrom));

				TEMP_BUFFER tmpbuf;

				tmpbuf.write(&pack, sizeof(pack));
				tmpbuf.write(buf, buflen);

				pkDesc->Packet(tmpbuf.read_peek(), tmpbuf.size());
			}
		}
	}
	if (pkDesc)
		pkDesc->SetRelay("");

	return (iExtraLen);
}

struct RawPacketToCharacterFunc
{
	const void* m_buf;
	int	m_buf_len;

	RawPacketToCharacterFunc(const void* buf, int buf_len) : m_buf(buf), m_buf_len(buf_len)
	{
	}

	void operator () (LPCHARACTER c)
	{
		if (!c->GetDesc())
			return;

		c->GetDesc()->Packet(m_buf, m_buf_len);
	}
};

struct FEmpireChatPacket
{
	packet_chat& p;
	const char* orig_msg;
	int orig_len;

	int iMapIndex;
	int namelen;

	FEmpireChatPacket(packet_chat& p, const char* chat_msg, int len, int iMapIndex, int iNameLen)
		: p(p), orig_msg(chat_msg), orig_len(len), iMapIndex(iMapIndex), namelen(iNameLen)
	{
	}

	void operator () (LPDESC d)
	{
		if (!d->GetCharacter())
			return;

		if (d->GetCharacter()->GetMapIndex() != iMapIndex)
			return;

		d->BufferedPacket(&p, sizeof(packet_chat));
		d->Packet(orig_msg, orig_len);
	}
};

int CInputMain::Chat(LPCHARACTER ch, const char* data, size_t uiBytes)
{
	const TPacketCGChat* pinfo = reinterpret_cast<const TPacketCGChat*>(data);

	if (uiBytes < pinfo->size)
		return -1;

	const int iExtraLen = pinfo->size - sizeof(TPacketCGChat);

	if (iExtraLen < 0)
	{
		sys_err("invalid packet length (len %d size %u buffer %u)", iExtraLen, pinfo->size, uiBytes);
		ch->GetDesc()->SetPhase(PHASE_CLOSE);
		return -1;
	}
	
#ifdef __ENABLE_OX_BLOCK__
	if (quest::CQuestManager::instance().GetEventFlag("ox_chat_public") == 1)
	{ 
		if(ch->GetMapIndex() == 113 && (pinfo->type == CHAT_TYPE_TALKING || pinfo->type == CHAT_TYPE_PARTY || pinfo->type == CHAT_TYPE_GUILD || pinfo->type == CHAT_TYPE_SHOUT))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ox_chat_block"));
			return iExtraLen;
		}
	}
#endif

	char buf[CHAT_MAX_LEN - (CHARACTER_NAME_MAX_LEN + 3) + 1];
	strlcpy(buf, data + sizeof(TPacketCGChat), MIN(iExtraLen + 1, sizeof(buf)));
	const size_t buflen = strlen(buf);

	if (buflen > 1 && *buf == '/')
	{
		interpret_command(ch, buf + 1, buflen - 1);
		return iExtraLen;
	}
#ifdef ENABLE_CHAT_SPAMLIMIT
	if (ch->IncreaseChatCounter() >= 4)
	{
		if (ch->GetChatCounter() == 10)
			ch->GetDesc()->DelayedDisconnect(0);
		return iExtraLen;
	}
#else
	if (ch->IncreaseChatCounter() >= 10)
	{
		if (ch->GetChatCounter() == 10)
		{
			sys_log(0, "CHAT_HACK: %s", ch->GetName());
			ch->GetDesc()->DelayedDisconnect(5);
		}

		return iExtraLen;
	}
#endif

	const CAffect* pAffect = ch->FindAffect(AFFECT_BLOCK_CHAT);

	if (pAffect != NULL)
	{
		SendBlockChatInfo(ch, pAffect->lDuration);
		return iExtraLen;
	}

	if (true == SpamBlockCheck(ch, buf, buflen))
	{
		return iExtraLen;
	}
#if defined(ENABLE_MAP_195_ALIGNMENT)	
	if (ch->GetMapIndex() == 195 && (ch->GetGMLevel() < GM_IMPLEMENTOR))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ALIGNMENT_MAC_195_CHAT"));
		return iExtraLen;
	}
#endif
	// @fixme133 begin
	CBanwordManager::instance().ConvertString(buf, buflen);

	int processReturn = ProcessTextTag (ch, buf, buflen);
	if (0 != processReturn)
	{
		const TItemTable* pTable = ITEM_MANAGER::instance().GetTable (ITEM_PRISM);

		if (NULL != pTable)
		{
			if (3 == processReturn)
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("사용할수 없습니다."), pTable->szLocaleName);
			}
			else
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("%s이 필요합니다."), pTable->szLocaleName);
			}

		}

		return iExtraLen;
	}
#ifdef ENABLE_GOLDTIGER_BETTING
	if (Quiz_IsActive())
	{
		Quiz_CheckAnswer(ch, buf);
		return iExtraLen;
	}
#endif
	// @fixme133 end
	char chatbuf[CHAT_MAX_LEN + 1];
#ifdef ENABLE_CHAT_COLOR_SYSTEM
	static const char* colorbuf[] = {"|cFF66ff33|H|h[밗잿逃]|h|r", "|cFFff0000|H|h[븐]|h|r", "|cFFffff00|H|h[뼝]|h|r", "|cFF0080ff|H|h[융]|h|r"};
	int len = snprintf (chatbuf, sizeof (chatbuf), "%s %s : %s", (ch->IsGM() ? colorbuf[0] : colorbuf[MINMAX (0, ch->GetEmpire(), 3)]), ch->GetName(), buf);
#else
	int len = snprintf (chatbuf, sizeof (chatbuf), "%s : %s", ch->GetName(), buf);
#endif
	if (len < 0 || len >= (int)sizeof(chatbuf))
		len = sizeof(chatbuf) - 1;

	if (pinfo->type == CHAT_TYPE_SHOUT)
	{
#ifdef ENABLE_CHAT_COLOR_SYSTEM
	char const* color;

	if (ch->GetChatColor() == 1)
		color = "99ccff";//융 ?
	else if (ch->GetChatColor() == 2) 
		color = "ff66ff";//뎅븐
	else if (ch->GetChatColor() == 3)
		color = "FFFF00";//뼝 ?
	else if (ch->GetChatColor() == 4)
		color = "00FF00";//쫄 ?
	else if (ch->GetChatColor() == 5)
		color = "FFA500";//쏜 ?
	else if (ch->GetChatColor() == 6)
		color = "ccff00";//좋쫄
	else if (ch->GetChatColor() == 7)
		color = "40E0D0";//융 ?
	else if (ch->GetChatColor() == 8)
		color = "cb9fe8";//뢴凜
	else if (ch->GetChatColor() == 9)
		color = "3366ff";//융 ?
	else
	{
		if (pinfo->type == CHAT_TYPE_SHOUT)
			color = "A7FFD4";
		else
			color = "FFFFFF";
	}
	// if (pinfo->bEmoticon == false
// #ifdef ENABLE_PREMIUM_MEMBER_SYSTEM
		// && ch->FindAffect(AFFECT_PREMIUM)
// #endif
		// )
		
	if (pinfo->bEmoticon == false)
		len = snprintf(chatbuf, sizeof(chatbuf), "%s %s |H%s%s|h|r:|cFF%s%s|r", (ch->IsGM() ? colorbuf[0] : colorbuf[MINMAX (0, ch->GetEmpire(), 3)]), ch->GetName(), "whisper:", ch->GetName(), color, buf);
	else
		len = snprintf(chatbuf, sizeof(chatbuf), "%s %s |H%s%s|h|r:%s", (ch->IsGM() ? colorbuf[0] : colorbuf[MINMAX (0, ch->GetEmpire(), 3)]), ch->GetName(), "whisper:", ch->GetName(),  buf);
#else
#ifdef ENABLE_CHAT_FLAG_AND_PM
		len = snprintf(chatbuf, sizeof(chatbuf), "%s |H%s%s|h|r:%s", ch->GetDesc(), ch->GetName(), "whisper:", ch->GetName(), buf);
#endif
#endif

		if (ch->GetLevel() < g_iShoutLimitLevel)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("외치기는 레벨 %d 이상만 사용 가능 합니다."), g_iShoutLimitLevel);
			return (iExtraLen);
		}
#ifdef __ENABLE_OX_BLOCK__
		bool bDisableShout = quest::CQuestManager::instance().GetEventFlag("all_public_chat");
		if (bDisableShout == true && !ch->IsGM())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("all_map_off_public_chat"));
			return (iExtraLen);
		}
#endif
		if (thecore_heart->pulse - (int)ch->GetLastShoutPulse() < passes_per_sec * 15)
			return (iExtraLen);

		ch->SetLastShoutPulse(thecore_heart->pulse);
#ifdef ENABLE_EXTENDED_BATTLE_PASS
		ch->UpdateExtBattlePassMissionProgress(CHAT_SHOUT, 1, 0);
#endif
		TPacketGGShout p;

		p.bHeader = HEADER_GG_SHOUT;
		p.bEmpire = ch->GetEmpire();
		strlcpy(p.szText, chatbuf, sizeof(p.szText));

		P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGShout));

		SendShout(chatbuf, ch->GetEmpire());

		return (iExtraLen);
	}

	TPacketGCChat pack_chat;

	pack_chat.header = HEADER_GC_CHAT;
	pack_chat.size = sizeof(TPacketGCChat) + len;
	pack_chat.type = pinfo->type;
	pack_chat.id = ch->GetVID();

	switch (pinfo->type)
	{
	case CHAT_TYPE_TALKING:
	{
		const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();
		std::for_each(c_ref_set.begin(), c_ref_set.end(),
			FEmpireChatPacket(pack_chat,
				chatbuf,
				len,
				ch->GetMapIndex(), strlen(ch->GetName())));
	}
	break;

	case CHAT_TYPE_PARTY:
	{
		if (!ch->GetParty())
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("파티 중이 아닙니다."));
		else
		{
			TEMP_BUFFER tbuf;

			tbuf.write(&pack_chat, sizeof(pack_chat));
			tbuf.write(chatbuf, len);

			RawPacketToCharacterFunc f(tbuf.read_peek(), tbuf.size());
			ch->GetParty()->ForEachOnlineMember(f);
		}
	}
	break;

	case CHAT_TYPE_GUILD:
	{
		if (!ch->GetGuild())
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("길드에 가입하지 않았습니다."));
		}
		else
		{
			ch->GetGuild()->Chat(chatbuf);
		}
	}
	break;

	default:
		sys_err("Unknown chat type %d", pinfo->type);
		break;
	}

	return (iExtraLen);
}

void CInputMain::ItemUse(LPCHARACTER ch, const char* data)
{
#ifdef ENABLE_CHEST_OPEN_RENEWAL
	if (((struct command_item_use*)data)->item_open_count == 0)
		ch->UseItem(((struct command_item_use*)data)->Cell);
	else
		ch->UseItem(((struct command_item_use*)data)->Cell, NPOS, ((struct command_item_use*)data)->item_open_count);
#else
	ch->UseItem(((struct command_item_use*)data)->Cell);
#endif
}

void CInputMain::ItemToItem(LPCHARACTER ch, const char* pcData)
{
	TPacketCGItemUseToItem* p = (TPacketCGItemUseToItem*)pcData;
	if (ch)
		ch->UseItem(p->Cell, p->TargetCell);
}

#ifdef ENABLE_DROP_DIALOG_EXTENDED_SYSTEM
void CInputMain::ItemDelete(LPCHARACTER ch, const char* data)
{
	struct command_item_delete* pinfo = (struct command_item_delete*)data;

	if (ch)
		ch->DeleteItem(pinfo->Cell);
}

void CInputMain::ItemSell(LPCHARACTER ch, const char* data)
{
	struct command_item_sell* pinfo = (struct command_item_sell*)data;

	if (ch)
		ch->SellItem(pinfo->Cell);
}
#else
void CInputMain::ItemDrop(LPCHARACTER ch, const char* data)
{
	struct command_item_drop* pinfo = (struct command_item_drop*)data;
	if (!ch)
		return;

	if (pinfo->gold > 0)
		ch->DropGold(pinfo->gold);
	else
		ch->DropItem(pinfo->Cell);
}

void CInputMain::ItemDrop2(LPCHARACTER ch, const char* data)
{
	TPacketCGItemDrop2* pinfo = (TPacketCGItemDrop2*)data;
	if (!ch)
		return;
	if (pinfo->gold > 0)
		ch->DropGold(pinfo->gold);
	else
		ch->DropItem(pinfo->Cell, pinfo->count);
}
#endif

void CInputMain::ItemMove(LPCHARACTER ch, const char* data)
{
	struct command_item_move* pinfo = (struct command_item_move*)data;

	if (ch)
		ch->MoveItem(pinfo->Cell, pinfo->CellTo, pinfo->count);
}

void CInputMain::ItemPickup(LPCHARACTER ch, const char* data)
{
	struct command_item_pickup* pinfo = (struct command_item_pickup*)data;
	if (ch)
		ch->PickupItem(pinfo->vid);
}

void CInputMain::QuickslotAdd(LPCHARACTER ch, const char* data)
{
	struct command_quickslot_add* pinfo = (struct command_quickslot_add*)data;

	if (pinfo->slot.type == QUICKSLOT_TYPE_ITEM)// @fixme182
	{
		LPITEM item = NULL;
		TItemPos srcCell(INVENTORY, pinfo->slot.pos);
		if (!(item = ch->GetItem(srcCell)))
			return;
		if (item->GetType() != ITEM_USE && item->GetType() != ITEM_QUEST)
			return;
	}

	ch->SetQuickslot(pinfo->pos, pinfo->slot);
}

void CInputMain::QuickslotDelete(LPCHARACTER ch, const char* data)
{
	struct command_quickslot_del* pinfo = (struct command_quickslot_del*)data;
	ch->DelQuickslot(pinfo->pos);
}

void CInputMain::QuickslotSwap(LPCHARACTER ch, const char* data)
{
	struct command_quickslot_swap* pinfo = (struct command_quickslot_swap*)data;
	ch->SwapQuickslot(pinfo->pos, pinfo->change_pos);
}

int CInputMain::Messenger(LPCHARACTER ch, const char* c_pData, size_t uiBytes)
{
	TPacketCGMessenger* p = (TPacketCGMessenger*)c_pData;

	if (uiBytes < sizeof(TPacketCGMessenger))
		return -1;

	c_pData += sizeof(TPacketCGMessenger);
	uiBytes -= sizeof(TPacketCGMessenger);

	switch (p->subheader)
	{
	case MESSENGER_SUBHEADER_CG_ADD_BY_VID:
	{
		if (uiBytes < sizeof(TPacketCGMessengerAddByVID))
			return -1;

		TPacketCGMessengerAddByVID* p2 = (TPacketCGMessengerAddByVID*)c_pData;
		LPCHARACTER ch_companion = CHARACTER_MANAGER::instance().Find(p2->vid);

		if (!ch_companion)
			return sizeof(TPacketCGMessengerAddByVID);

		if (ch->IsObserverMode())
			return sizeof(TPacketCGMessengerAddByVID);

		if (ch_companion->IsBlockMode(BLOCK_MESSENGER_INVITE))
		{
			// ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("상대방이 메신져 추가 거부 상태입니다."));
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("%s 님으로 부터 친구 등록을 거부 당했습니다."), ch_companion->GetName());
			return sizeof(TPacketCGMessengerAddByVID);
		}
		
#ifdef ENABLE_BOT_PLAYER
		if (ch_companion->IsBotCharacter())//앳없降좔榴檄
		{
			// ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("상대방이 메신져 추가 거부 상태입니다."));
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("%s 님으로 부터 친구 등록을 거부 당했습니다."), ch_companion->GetName());
			return sizeof(TPacketCGMessengerAddByVID);
		}
#endif
#if defined(ENABLE_MAP_195_ALIGNMENT)	
		//掉뒨릴굶綠掘齡警속봤堂
		if (ch_companion->GetMapIndex() == 195)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ALIGNMENT_MAP195_NO_MESSENGER"));
			return sizeof(TPacketCGMessengerAddByVID);
		}
#endif
		LPDESC d = ch_companion->GetDesc();

		if (!d)
			return sizeof(TPacketCGMessengerAddByVID);

		if (ch->GetGMLevel() == GM_PLAYER && ch_companion->GetGMLevel() != GM_PLAYER)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<메신져> 운영자는 메신져에 추가할 수 없습니다."));
			return sizeof(TPacketCGMessengerAddByVID);
		}

		if (ch->GetDesc() == d)
			return sizeof(TPacketCGMessengerAddByVID);

		MessengerManager::instance().RequestToAdd(ch, ch_companion);
		//MessengerManager::instance().AddToList(ch->GetName(), ch_companion->GetName());
	}
	return sizeof(TPacketCGMessengerAddByVID);

	case MESSENGER_SUBHEADER_CG_ADD_BY_NAME:
	{
		if (uiBytes < CHARACTER_NAME_MAX_LEN)
			return -1;

		char name[CHARACTER_NAME_MAX_LEN + 1];
		strlcpy(name, c_pData, sizeof(name));

		if (ch->GetGMLevel() == GM_PLAYER && gm_get_level(name) != GM_PLAYER)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<메신져> 운영자는 메신져에 추가할 수 없습니다."));
			return CHARACTER_NAME_MAX_LEN;
		}

		LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(name);

		if (!tch)
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("%s 님은 접속되 있지 않습니다."), name);
			
		else
		{
			if (tch == ch)
				return CHARACTER_NAME_MAX_LEN;
#ifdef ENABLE_BOT_PLAYER
			if (tch->IsBotCharacter())
			{	//뚤렘뇹黨앳없降좔榴檄
				// ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("상대방이 메신져 추가 거부 상태입니다."));
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("%s 님으로 부터 친구 등록을 거부 당했습니다."), tch->GetName());
				return CHARACTER_NAME_MAX_LEN;
			}
#endif
#if defined(ENABLE_MAP_195_ALIGNMENT)	
			//	掉뒨릴굶綠掘齡警속봤堂
			if (tch->GetMapIndex() == 195)
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ALIGNMENT_MAP195_NO_MESSENGER"));
				return sizeof(CHARACTER_NAME_MAX_LEN);
			}
#endif
			if (tch->IsBlockMode(BLOCK_MESSENGER_INVITE) == true)
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("%s 님으로 부터 친구 등록을 거부 당했습니다."), tch->GetName());
			}
			else
			{
				MessengerManager::instance().RequestToAdd(ch, tch);
				//MessengerManager::instance().AddToList(ch->GetName(), tch->GetName());
			}
		}
	}
	return CHARACTER_NAME_MAX_LEN;

	case MESSENGER_SUBHEADER_CG_REMOVE:
	{
		if (uiBytes < CHARACTER_NAME_MAX_LEN)
			return -1;

		char char_name[CHARACTER_NAME_MAX_LEN + 1];
		strlcpy(char_name, c_pData, sizeof(char_name));
		MessengerManager::instance().RemoveFromList(ch->GetName(), char_name);
#ifdef ENABLE_FRIEND_LIST_REMOVE_FIX
		MessengerManager::instance().RemoveFromList(char_name, ch->GetName()); // @fixme183
#endif
	}
	return CHARACTER_NAME_MAX_LEN;

#ifdef ENABLE_TELEPORT_TO_A_FRIEND
	case MESSENGER_SUBHEADER_CG_REQUEST_WARP_BY_NAME:
	{
		if (uiBytes < CHARACTER_NAME_MAX_LEN)
		{
			return -1;
		}

		if (!ch->IsActivateTime(FRIEND_TELEPORT_CHECK_TIME, 5)
			|| ch->WindowOpenCheck()
			|| ch->ActivateCheck()
			)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("You need to wait a few seconds"));
			return CHARACTER_NAME_MAX_LEN;
		}

		char name[CHARACTER_NAME_MAX_LEN + 1];
		strlcpy(name, c_pData, sizeof(name));
		
		LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(name);

		if (!tch)
		{
			CCI* pch = P2P_MANAGER::Instance().Find(name);
			if (!pch)
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("%s 님은 접속되 있지 않습니다."), name);
				return CHARACTER_NAME_MAX_LEN;
			}

			if (!ch->WarpToPlayerMapLevelControl(pch->lMapIndex))
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Teleport to player map level control"));
				return CHARACTER_NAME_MAX_LEN;
			}

			TPacketGGTeleportRequest request{};
			request.header = HEADER_GG_TELEPORT_REQUEST;
			request.subHeader = SUBHEADER_GG_TELEPORT_REQUEST;
			strlcpy(request.sender, ch->GetName(), CHARACTER_NAME_MAX_LEN + 1);
			strlcpy(request.target, name, CHARACTER_NAME_MAX_LEN + 1);

			P2P_MANAGER::Instance().Send(&request, sizeof(TPacketGGTeleportRequest));
		}
		else
		{
			if (tch == ch)
			{
				return CHARACTER_NAME_MAX_LEN;
			}

			if (!ch->WarpToPlayerMapLevelControl(tch->GetMapIndex()))
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Teleport to player map level control"));
				return CHARACTER_NAME_MAX_LEN;
			}

			if (tch->IsBlockMode(BLOCK_WARP_REQUEST))
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("%s declined the teleport request."));
				return CHARACTER_NAME_MAX_LEN;
			}

			tch->ChatPacket(CHAT_TYPE_COMMAND, "RequestWarpToCharacter %s", ch->GetName());
		}
		ch->SetActivateTime(FRIEND_TELEPORT_CHECK_TIME);
	}
	return CHARACTER_NAME_MAX_LEN;
			
	case MESSENGER_SUBHEADER_CG_SUMMON_BY_NAME:
	{
		if (uiBytes < CHARACTER_NAME_MAX_LEN)
		{
			return -1;
		}

		char name[CHARACTER_NAME_MAX_LEN + 1];
		strlcpy(name, c_pData, sizeof(name));

		LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(name);

		if (strcmp(name, ch->GetName()) == 0)
		{
			return CHARACTER_NAME_MAX_LEN;
		}

		if (!tch)
		{
			CCI* pch = P2P_MANAGER::Instance().Find(name);
			if (!pch)
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("%s 님은 접속되 있지 않습니다."), name);
				return CHARACTER_NAME_MAX_LEN;
			}

			TPacketGGTeleportRequest request{};
			request.header = HEADER_GG_TELEPORT_REQUEST;
			request.subHeader = SUBHEADER_GG_TELEPORT_ANSWER;
			strlcpy(request.sender, ch->GetName(), CHARACTER_NAME_MAX_LEN + 1);
			strlcpy(request.target, name, CHARACTER_NAME_MAX_LEN + 1);

			P2P_MANAGER::Instance().Send(&request, sizeof(TPacketGGTeleportRequest));
		}
		else
		{
			tch->WarpToPlayer(ch);
		}
	}
	return CHARACTER_NAME_MAX_LEN;
#endif


	default:
		sys_err("CInputMain::Messenger : Unknown subheader %d : %s", p->subheader, ch->GetName());
		break;
	}

	return 0;
}

#ifdef ENABLE_SPECIAL_STORAGE
typedef struct fckOFF
{
	BYTE		bySlot;
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	MAX_COUNT	byCount;
#else
	BYTE		byCount;
#endif
#ifdef ENABLE_SPECIAL_STORAGE
	BYTE		byType;
#endif
} TfckOFF;
#endif

int CInputMain::Shop(LPCHARACTER ch, const char* data, size_t uiBytes)
{
	TPacketCGShop* p = (TPacketCGShop*)data;

	if (uiBytes < sizeof(TPacketCGShop))
		return -1;

	const char* c_pData = data + sizeof(TPacketCGShop);
	uiBytes -= sizeof(TPacketCGShop);

	switch (p->subheader)
	{
	case SHOP_SUBHEADER_CG_END:
		sys_log(1, "INPUT: %s SHOP: END", ch->GetName());
		CShopManager::instance().StopShopping(ch);
		return 0;

	case SHOP_SUBHEADER_CG_BUY:
	{
		if (uiBytes < sizeof(BYTE) + sizeof(BYTE))
			return -1;

		BYTE bPos = *(c_pData + 1);
		sys_log(1, "INPUT: %s SHOP: BUY %d", ch->GetName(), bPos);
		CShopManager::instance().Buy(ch, bPos);
		return (sizeof(BYTE) + sizeof(BYTE));
	}

	case SHOP_SUBHEADER_CG_SELL:
	{
		if (uiBytes < sizeof(BYTE))
			return -1;

		BYTE pos = *c_pData;

		sys_log(0, "INPUT: %s SHOP: SELL", ch->GetName());
		CShopManager::instance().Sell(ch, pos);
		return sizeof(BYTE);
	}

	case SHOP_SUBHEADER_CG_SELL2:
	{
#ifdef ENABLE_SPECIAL_STORAGE
		if (uiBytes < sizeof(TfckOFF))
			return -1;

		TfckOFF* p2 = (TfckOFF*)c_pData;

		sys_log(0, "INPUT: %s SHOP: SELL2", ch->GetName());
#ifdef ENABLE_SPECIAL_STORAGE
		CShopManager::instance().Sell(ch, p2->bySlot, p2->byCount, p2->byType);
#else
		CShopManager::instance().Sell(ch, p2->bySlot, p2->byCount);
#endif
		return sizeof(TfckOFF);
#else
		if (uiBytes < sizeof(BYTE) + sizeof(WORD))
			return -1;

		BYTE pos = *(c_pData++);
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
		MAX_COUNT count = *(c_pData);
#else
		BYTE count = *(c_pData);
#endif

		sys_log(0, "INPUT: %s SHOP: SELL2", ch->GetName());
		CShopManager::instance().Sell(ch, pos, count);
		return sizeof(BYTE) + sizeof(WORD);
#endif
	}

#ifdef ENABLE_MULTIPLE_BUY_ITEMS
	case SHOP_SUBHEADER_CG_MULTIBUY:
	{
		size_t size = sizeof(uint8_t) + sizeof(uint8_t);
		if (uiBytes < size) {
			return -1;
		}

		uint8_t p = *(c_pData++);
		uint8_t c = *(c_pData);
		sys_log(1, "INPUT: %s SHOP: MULTIPLE BUY %d COUNT %d", ch->GetName(), p, c);
		CShopManager::instance().MultipleBuy(ch, p, c);
		return size;
	}
#endif

	default:
		sys_err("CInputMain::Shop : Unknown subheader %d : %s", p->subheader, ch->GetName());
		break;
	}

	return 0;
}

void CInputMain::OnClick(LPCHARACTER ch, const char* data)
{
	struct command_on_click* pinfo = (struct command_on_click*)data;
	LPCHARACTER			victim;

	if ((victim = CHARACTER_MANAGER::instance().Find(pinfo->vid)))
		victim->OnClick(ch);
}

void CInputMain::Exchange(LPCHARACTER ch, const char* data)
{
	struct command_exchange* pinfo = (struct command_exchange*)data;
	LPCHARACTER	to_ch = NULL;

	if (!ch->CanHandleItem())
		return;

	int iPulse = thecore_pulse();

	if ((to_ch = CHARACTER_MANAGER::instance().Find(pinfo->arg1)))
	{
		if (iPulse - to_ch->GetActivateTime(SAFEBOX_CHECK_TIME) < PASSES_PER_SEC(g_nPortalLimitTime))
		{
			to_ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("거래 후 %d초 이내에 창고를 열수 없습니다."), g_nPortalLimitTime);
			return;
		}

		if (true == to_ch->IsDead())
		{
			return;
		}
	}

	if (iPulse - ch->GetActivateTime(SAFEBOX_CHECK_TIME) < PASSES_PER_SEC(g_nPortalLimitTime))
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("거래 후 %d초 이내에 창고를 열수 없습니다."), g_nPortalLimitTime);
		return;
	}

	switch (pinfo->sub_header)
	{
	case EXCHANGE_SUBHEADER_CG_START:	// arg1 == vid of target character
		if (!ch->GetExchange())
		{
			if ((to_ch = CHARACTER_MANAGER::instance().Find(pinfo->arg1)))
			{
				if (iPulse - ch->GetActivateTime(SAFEBOX_CHECK_TIME) < PASSES_PER_SEC(g_nPortalLimitTime))
				{
					ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("거래 후 %d초 이내에 창고를 열수 없습니다."), g_nPortalLimitTime);
					return;
				}

				if (iPulse - to_ch->GetActivateTime(SAFEBOX_CHECK_TIME) < PASSES_PER_SEC(g_nPortalLimitTime))
				{
					ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("거래 후 %d초 이내에 창고를 열수 없습니다."), g_nPortalLimitTime);
					return;
				}

				if (ch->GetGold() >= GOLD_MAX)
				{
					ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("액수가 20억 냥을 초과하여 거래를 할수가 없습니다.."));

#ifdef ENABLE_EXTENDED_YANG_LIMIT
					sys_err("[OVERFLOG_GOLD] START (%lld) id %u name %s ", ch->GetGold(), ch->GetPlayerID(), ch->GetName());
#else
					sys_err("[OVERFLOG_GOLD] START (%u) id %u name %s ", ch->GetGold(), ch->GetPlayerID(), ch->GetName());
#endif
					return;
				}

				if (to_ch->IsPC())
				{
					if (quest::CQuestManager::instance().GiveItemToPC(ch->GetPlayerID(), to_ch))
					{
						sys_log(0, "Exchange canceled by quest %s %s", ch->GetName(), to_ch->GetName());
						return;
					}
				}

				if (ch->GetMyShop() || ch->IsOpenSafebox() || ch->GetShopOwner() || ch->IsCubeOpen() || ch->IsAcceOpened() || ch->IsOpenOfflineShop() || ch->ActivateCheck()
#ifdef ENABLE_AURA_SYSTEM
					|| ch->isAuraOpened(true) || ch->isAuraOpened(false)
#endif
#ifdef ENABLE_6TH_7TH_ATTR
					|| ch->Is67AttrOpen()
#endif
					)
				{
					ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Make sure you don't have any open windows1"));
					return;
				}
				ch->ExchangeStart(to_ch);
			}
		}
		break;

	case EXCHANGE_SUBHEADER_CG_ITEM_ADD:	// arg1 == position of item, arg2 == position in exchange window
		if (ch->GetExchange())
		{
			if (ch->GetExchange()->GetCompany()->GetAcceptStatus() != true)
#if defined(ENABLE_CHECKINOUT_UPDATE)
				ch->GetExchange()->AddItem(pinfo->Pos, pinfo->arg2, pinfo->SelectPosAuto);
#else
				ch->GetExchange()->AddItem(pinfo->Pos, pinfo->arg2);
#endif
		}
		break;

	case EXCHANGE_SUBHEADER_CG_ITEM_DEL:	// arg1 == position of item
		if (ch->GetExchange())
		{
			if (ch->GetExchange()->GetCompany()->GetAcceptStatus() != true)
				ch->GetExchange()->RemoveItem(pinfo->arg1);
		}
		break;

#ifdef ENABLE_EXTENDED_YANG_LIMIT
	case EXCHANGE_SUBHEADER_CG_ELK_ADD:	// arg1 == amount of gold
		if (ch->GetExchange())
		{
			const int64_t nTotalGold = static_cast<int64_t>(ch->GetExchange()->GetCompany()->GetOwner()->GetGold()) + static_cast<int64_t>(pinfo->arg1);

			if (GOLD_MAX <= nTotalGold)
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("상대방의 총금액이 20억 냥을 초과하여 거래를 할수가 없습니다.."));

				sys_err("[OVERFLOW_GOLD] ELK_ADD (%lld) id %u name %s ",
					ch->GetExchange()->GetCompany()->GetOwner()->GetGold(),
					ch->GetExchange()->GetCompany()->GetOwner()->GetPlayerID(),
					ch->GetExchange()->GetCompany()->GetOwner()->GetName());

				return;
			}

			if (ch->GetExchange()->GetCompany()->GetAcceptStatus() != true)
				ch->GetExchange()->AddGold(pinfo->arg1);
		}
		break;
#else
	case EXCHANGE_SUBHEADER_CG_ELK_ADD:	// arg1 == amount of gold
		if (ch->GetExchange())
		{
			const int64_t nTotalGold = static_cast<int64_t>(ch->GetExchange()->GetCompany()->GetOwner()->GetGold()) + static_cast<int64_t>(pinfo->arg1);

			if (GOLD_MAX <= nTotalGold)
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("상대방의 총금액이 20억 냥을 초과하여 거래를 할수가 없습니다.."));

				sys_err("[OVERFLOW_GOLD] ELK_ADD (%u) id %u name %s ",
					ch->GetExchange()->GetCompany()->GetOwner()->GetGold(),
					ch->GetExchange()->GetCompany()->GetOwner()->GetPlayerID(),
					ch->GetExchange()->GetCompany()->GetOwner()->GetName());

				return;
			}

			if (ch->GetExchange()->GetCompany()->GetAcceptStatus() != true)
				ch->GetExchange()->AddGold(pinfo->arg1);
		}
		break;
#endif

	case EXCHANGE_SUBHEADER_CG_ACCEPT:	// arg1 == not used
		if (ch->GetExchange())
		{
			sys_log(0, "CInputMain()::Exchange() ==> ACCEPT ");
			ch->GetExchange()->Accept(true);
		}

		break;

	case EXCHANGE_SUBHEADER_CG_CANCEL:	// arg1 == not used
		if (ch->GetExchange())
			ch->GetExchange()->Cancel();
		break;
	}
}

void CInputMain::Position(LPCHARACTER ch, const char* data)
{
	struct command_position* pinfo = (struct command_position*)data;

	switch (pinfo->position)
	{
	case POSITION_GENERAL:
		ch->Standup();
		break;

	case POSITION_SITTING_CHAIR:
		ch->Sitdown(0);
		break;

	case POSITION_SITTING_GROUND:
		ch->Sitdown(1);
		break;
	}
}
//2025-11-05
static const int ComboSequenceBySkillLevel[3][8] =
{
	// 0   1   2   3   4   5   6   7
	{ 14, 15, 16, 17,  0,  0,  0,  0 },
	{ 14, 15, 16, 18, 20,  0,  0,  0 },
	{ 14, 15, 16, 18, 19, 17,  0,  0 },
};

#define COMBO_HACK_ALLOWABLE_MS	100

bool CheckComboHack (LPCHARACTER ch, BYTE bArg, DWORD dwTime, bool CheckSpeedHack)
{
	if (!gHackCheckEnable)
	{
		return false;
	}

	if (ch->IsStun() || ch->IsDead())
	{
		return false;
	}
	int ComboInterval = dwTime - ch->GetLastComboTime();
	int HackScalar = 0;
	if (bArg == 14)
	{
		if (CheckSpeedHack && ComboInterval > 0 && ComboInterval < ch->GetValidComboInterval() - COMBO_HACK_ALLOWABLE_MS)
		{

		}
		ch->SetComboSequence(1);//劤藤2025-12-22
		ch->SetValidComboInterval ((int) (ani_combo_speed (ch, 1) / (ch->GetPoint (POINT_ATT_SPEED) / 100.f)));
		ch->SetLastComboTime (dwTime);
	}
	//닒侶쟁역迦2025-11-05
	else if (bArg > 14 && bArg < 22)
	{
		int idx = MIN(2, ch->GetComboIndex());

		if (ch->GetComboSequence() > 5)
		{
			HackScalar = 1;
			ch->SetValidComboInterval(300);
			sys_log(0, "COMBO_HACK: 5 %s combo_seq:%d", ch->GetName(), ch->GetComboSequence());
		}

		else if (bArg == 21 &&
				 idx == 2 &&
				 ch->GetComboSequence() == 5 &&
				 ch->GetJob() == JOB_ASSASSIN &&
				 ch->GetWear(WEAR_WEAPON) &&
				 ch->GetWear(WEAR_WEAPON)->GetSubType() == WEAPON_DAGGER)
			ch->SetValidComboInterval(300);
#ifdef ENABLE_WOLFMAN_CHARACTER
		else if (bArg == 21 && 
				idx == 2 && 
				ch->GetComboSequence() == 5 && 
				ch->GetJob() == JOB_WOLFMAN && 
				ch->GetWear(WEAR_WEAPON) && 
				ch->GetWear(WEAR_WEAPON)->GetSubType() == WEAPON_CLAW)
			ch->SetValidComboInterval(300);
#endif
		else if (ComboSequenceBySkillLevel[idx][ch->GetComboSequence()] != bArg)
		{
			HackScalar = 1;
			ch->SetValidComboInterval(300);

			sys_log(0, "COMBO_HACK: 3 %s arg:%u valid:%u combo_idx:%d combo_seq:%d",
					ch->GetName(),
					bArg,
					ComboSequenceBySkillLevel[idx][ch->GetComboSequence()],
					idx,
					ch->GetComboSequence());
		}
		else
		{
			if (CheckSpeedHack && ComboInterval < ch->GetValidComboInterval() - COMBO_HACK_ALLOWABLE_MS)
			{
				HackScalar = 1 + (ch->GetValidComboInterval() - ComboInterval) / 100;

				sys_log(0, "COMBO_HACK: 2 %s arg:%u interval:%d valid:%u atkspd:%u riding:%s",
						ch->GetName(),
						bArg,
						ComboInterval,
						ch->GetValidComboInterval(),
						ch->GetPoint(POINT_ATT_SPEED),
						ch->IsRiding() ? "yes" : "no");
			}

			//if (ch->IsHorseRiding())
			if (ch->IsRiding())
				ch->SetComboSequence(ch->GetComboSequence() == 1 ? 2 : 1);
			else
				ch->SetComboSequence(ch->GetComboSequence() + 1);

			ch->SetValidComboInterval((int) (ani_combo_speed(ch, bArg - 13) / (ch->GetPoint(POINT_ATT_SPEED) / 100.f)));
			ch->SetLastComboTime(dwTime);
		}
	}
	//닒侶쟁써監2025-11-05
	else if (bArg == 13)
	{
		if (CheckSpeedHack && ComboInterval > 0 && ComboInterval < ch->GetValidComboInterval() - COMBO_HACK_ALLOWABLE_MS)
		{

		}

		if (ch->GetRaceNum() >= MAIN_RACE_MAX_NUM)
		{

			float normalAttackDuration = CMotionManager::instance().GetNormalAttackDuration (ch->GetRaceNum());
			int k = (int) (normalAttackDuration / ((float)ch->GetPoint (POINT_ATT_SPEED) / 100.f) * 900.f);
			ch->SetValidComboInterval (k);
			ch->SetLastComboTime (dwTime);
		}
		else
		{

		}
	}
	else
	{

		if (ch->GetDesc()->DelayedDisconnect (number (2, 9)))
		{
			sys_log (0, "HACKER: %s arg %u", ch->GetName(), bArg);
		}

		HackScalar = 10;
		ch->SetValidComboInterval (300);
	}

	if (HackScalar)
	{

		if (get_dword_time() - ch->GetLastMountTime() > 1500)
		{
			ch->IncreaseComboHackCount (1 + HackScalar);
		}

		ch->SkipComboAttackByTime (ch->GetValidComboInterval());
	}

	return HackScalar;

}

void CInputMain::Move(LPCHARACTER ch, const char* data)
{
	if (!ch->CanMove())
		return;

	struct command_move* pinfo = (struct command_move*)data;

	if (pinfo->bFunc >= FUNC_MAX_NUM && !(pinfo->bFunc & 0x80))
	{
		sys_err("invalid move type: %s", ch->GetName());
		return;
	}

	{

		const float fDist = DISTANCE_SQRT ((ch->GetX() - pinfo->lX) / 100, (ch->GetY() - pinfo->lY) / 100);
		if (((false == ch->IsRiding() && fDist > 40) || fDist > 60) && OXEVENT_MAP_INDEX != ch->GetMapIndex()) // @fixme106 (changed 40 to 60)
		{
			sys_log (0, "MOVE: %s trying to move too far (dist: %.1fm) Riding(%d)", ch->GetName(), fDist, ch->IsRiding());
			ch->Show (ch->GetMapIndex(), ch->GetX(), ch->GetY(), ch->GetZ());//盧땡醵똑법우삔굳윗쀼覩뒈
			ch->Stop();
			return;
		}
	
#ifdef ENABLE_CHECK_GHOSTMODE
		if (ch->IsPC() && ch->IsDead())
		{
			ch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY(), ch->GetZ());
			ch->Stop();
			return;
		}
#endif
		DWORD dwCurTime = get_dword_time();
		bool CheckSpeedHack = (false == ch->GetDesc()->IsHandshaking() && dwCurTime - ch->GetDesc()->GetClientTime() > 7000);
#ifdef __FIX_PRO_DAMAGE__
		if (OXEVENT_MAP_INDEX != ch->GetMapIndex() && ch->CheckSyncPosition (true))
		{
			sys_log (0, "#(HACK)# (%s) sync_check error", ch->GetName());
			ch->Show (ch->GetMapIndex(), ch->GetX(), ch->GetY(), ch->GetZ());
			ch->Stop();
			return;
		}
#endif
		if (CheckSpeedHack)
		{
			int iDelta = (int) (pinfo->dwTime - ch->GetDesc()->GetClientTime());
			int iServerDelta = (int) (dwCurTime - ch->GetDesc()->GetClientTime());
			iDelta = (int) (dwCurTime - pinfo->dwTime);
			if (iDelta >= 30000)
			{
				sys_log (0, "SPEEDHACK: slow timer name %s delta %d", ch->GetName(), iDelta);
				ch->GetDesc()->DelayedDisconnect (3);
			}

			else if (iDelta < - (iServerDelta / 50))
			{
				sys_log (0, "SPEEDHACK: DETECTED! %s (delta %d %d)", ch->GetName(), iDelta, iServerDelta);
				ch->GetDesc()->DelayedDisconnect (3);
			}
		}

		if (pinfo->bFunc == FUNC_COMBO && g_bCheckMultiHack)
		{
			CheckComboHack (ch, pinfo->bArg, pinfo->dwTime, CheckSpeedHack);
		}
	}

	if (pinfo->bFunc == FUNC_MOVE)
	{
		if (ch->GetLimitPoint(POINT_MOV_SPEED) == 0)
			return;

		ch->SetRotation(pinfo->bRot * 5);
		ch->ResetStopTime();
#ifdef __FIX_PRO_DAMAGE__
		if (OXEVENT_MAP_INDEX != ch->GetMapIndex())
		{
			int fDist = DISTANCE_SQRT ((ch->GetX() - pinfo->lX) / 100, (ch->GetY() - pinfo->lY) / 100);
			ch->SetSyncPosition (pinfo->lX, pinfo->lY);

			if (((false == ch->IsRiding() && fDist > 25) || fDist > 40))
			{
				sys_log (0, "#(HACK)# (%s) sync fDist = %d, limit = 16", ch->GetName(), fDist);
				ch->Show (ch->GetMapIndex(), ch->GetX(), ch->GetY(), ch->GetZ());
				ch->Stop();
				return;
			}

			int il = 0;
			if (((false == ch->IsRiding() && fDist > 20) || fDist > 35))
			{
				il = DISTANCE_SQRT ((ch->GetX() - pinfo->lX) / 100, (ch->GetY() - pinfo->lY) / 100) * 3;
			}

			ch->CheckSyncPosition() ? il += 1 : il += 3;
			ch->SetSyncCount (il);
		}
#endif
		ch->Goto(pinfo->lX, pinfo->lY);
	}
	else
	{
		if (pinfo->bFunc == FUNC_ATTACK || pinfo->bFunc == FUNC_COMBO)
			ch->OnMove(true);
		else if (pinfo->bFunc & FUNC_SKILL)
		{
			const int MASK_SKILL_MOTION = 0x7F;
			unsigned int motion = pinfo->bFunc & MASK_SKILL_MOTION;

			if (!ch->IsUsableSkillMotion(motion))
			{
				ch->GetDesc()->DelayedDisconnect(number(150, 500));
			}
			ch->OnMove();
		}

		ch->SetRotation(pinfo->bRot * 5);
		ch->ResetStopTime();

		ch->Move(pinfo->lX, pinfo->lY);
		ch->Stop();
		ch->StopStaminaConsume();
	}

	TPacketGCMove pack;

	pack.bHeader = HEADER_GC_MOVE;
	pack.bFunc = pinfo->bFunc;
	pack.bArg = pinfo->bArg;
	pack.bRot = pinfo->bRot;
	pack.dwVID = ch->GetVID();
	pack.lX = pinfo->lX;
	pack.lY = pinfo->lY;
	pack.dwTime = pinfo->dwTime;
	pack.dwDuration = (pinfo->bFunc == FUNC_MOVE) ? ch->GetCurrentMoveDuration() : 0;

	ch->PacketAround(&pack, sizeof(TPacketGCMove), ch);
}

void CInputMain::Attack(LPCHARACTER ch, const BYTE header, const char* data)
{
	if (NULL == ch)
		return;

	struct type_identifier
	{
		BYTE header;
		BYTE type;
	};

	const struct type_identifier* const type = reinterpret_cast<const struct type_identifier*>(data);

	if (type->type > 0)
	{
		if (false == ch->CanUseSkill(type->type))
		{
			return;
		}

		switch (type->type)
		{
		case SKILL_GEOMPUNG:
		case SKILL_SANGONG:
		case SKILL_YEONSA:
		case SKILL_KWANKYEOK:
		case SKILL_HWAJO:
		case SKILL_GIGUNG:
		case SKILL_PABEOB:
		case SKILL_MARYUNG:
		case SKILL_TUSOK:
		case SKILL_MAHWAN:
		case SKILL_BIPABU:
		case SKILL_NOEJEON:
		case SKILL_CHAIN:
		case SKILL_HORSE_WILDATTACK_RANGE:
			if (HEADER_CG_SHOOT != type->header)
			{
				return;
			}
			break;
		}
	}

	switch (header)
	{
	case HEADER_CG_ATTACK:
	{
		if (NULL == ch->GetDesc())
			return;

		const TPacketCGAttack* const packMelee = reinterpret_cast<const TPacketCGAttack*>(data);

		ch->GetDesc()->AssembleCRCMagicCube(packMelee->bCRCMagicCubeProcPiece, packMelee->bCRCMagicCubeFilePiece);

		LPCHARACTER	victim = CHARACTER_MANAGER::instance().Find(packMelee->dwVID);

		if (NULL == victim || ch == victim)
			return;

		switch (victim->GetCharType())
		{
		case CHAR_TYPE_NPC:
		case CHAR_TYPE_WARP:
		case CHAR_TYPE_GOTO:
			return;
		}

		if (packMelee->bType > 0)
		{
			if (false == ch->CheckSkillHitCount(packMelee->bType, victim->GetVID()))
			{
				return;
			}
		}

		ch->Attack(victim, packMelee->bType);
	}
	break;

	case HEADER_CG_SHOOT:
	{
		const TPacketCGShoot* const packShoot = reinterpret_cast<const TPacketCGShoot*>(data);

		ch->Shoot(packShoot->bType);
	}
	break;
	}
}

int CInputMain::SyncPosition(LPCHARACTER ch, const char* c_pcData, size_t uiBytes)
{
	const TPacketCGSyncPosition* pinfo = reinterpret_cast<const TPacketCGSyncPosition*>(c_pcData);

	if (uiBytes < pinfo->wSize)
		return -1;

	int iExtraLen = pinfo->wSize - sizeof(TPacketCGSyncPosition);

	if (iExtraLen < 0)
	{
		sys_err("invalid packet length (len %d size %u buffer %u)", iExtraLen, pinfo->wSize, uiBytes);
		ch->GetDesc()->SetPhase(PHASE_CLOSE);
		return -1;
	}

	if (0 != (iExtraLen % sizeof(TPacketCGSyncPositionElement)))
	{
		sys_err("invalid packet length %d (name: %s)", pinfo->wSize, ch->GetName());
		return iExtraLen;
	}

	int iCount = iExtraLen / sizeof(TPacketCGSyncPositionElement);

	if (iCount <= 0)
		return iExtraLen;

	static const int nCountLimit = 20;

	if (iCount > nCountLimit)
	{
		sys_err("Too many SyncPosition Count(%d) from Name(%s)", iCount, ch->GetName());
		//ch->GetDesc()->SetPhase(PHASE_CLOSE);
		//return -1;
		iCount = nCountLimit;
	}

	TEMP_BUFFER tbuf;
	LPBUFFER lpBuf = tbuf.getptr();

	TPacketGCSyncPosition* pHeader = (TPacketGCSyncPosition*)buffer_write_peek(lpBuf);
	buffer_write_proceed(lpBuf, sizeof(TPacketGCSyncPosition));

	const TPacketCGSyncPositionElement* e =
		reinterpret_cast<const TPacketCGSyncPositionElement*>(c_pcData + sizeof(TPacketCGSyncPosition));

	timeval tvCurTime;
	gettimeofday(&tvCurTime, NULL);

	for (int i = 0; i < iCount; ++i, ++e)
	{
		LPCHARACTER victim = CHARACTER_MANAGER::instance().Find(e->dwVID);

		if (!victim)
			continue;

		switch (victim->GetCharType())
		{
		case CHAR_TYPE_NPC:
		case CHAR_TYPE_WARP:
		case CHAR_TYPE_GOTO:
			continue;
		}

		if (!victim->SetSyncOwner(ch))
			continue;

		const float fDistWithSyncOwner = DISTANCE_SQRT((victim->GetX() - ch->GetX()) / 100, (victim->GetY() - ch->GetY()) / 100);
		static const float fLimitDistWithSyncOwner = 2500.f + 1000.f;

		if (fDistWithSyncOwner > fLimitDistWithSyncOwner)
		{
			if (ch->GetSyncHackCount() < g_iSyncHackLimitCount)
			{
				ch->SetSyncHackCount(ch->GetSyncHackCount() + 1);
				continue;
			}
			else
			{
				sys_err("Too far SyncPosition DistanceWithSyncOwner(%f)(%s) from Name(%s) CH(%d,%d) VICTIM(%d,%d) SYNC(%d,%d)",
					fDistWithSyncOwner, victim->GetName(), ch->GetName(), ch->GetX(), ch->GetY(), victim->GetX(), victim->GetY(),
					e->lX, e->lY);

				ch->GetDesc()->SetPhase(PHASE_CLOSE);

				return -1;
			}
		}

		const float fDist = DISTANCE_SQRT((victim->GetX() - e->lX) / 100, (victim->GetY() - e->lY) / 100);
		static const long g_lValidSyncInterval = 100 * 1000; // 100ms
		const timeval& tvLastSyncTime = victim->GetLastSyncTime();
		timeval* tvDiff = timediff(&tvCurTime, &tvLastSyncTime);

		if (tvDiff->tv_sec == 0 && tvDiff->tv_usec < g_lValidSyncInterval)
		{
			if (ch->GetSyncHackCount() < g_iSyncHackLimitCount)
			{
				ch->SetSyncHackCount(ch->GetSyncHackCount() + 1);
				continue;
			}
			else
			{
				sys_err("Too often SyncPosition Interval(%ldms)(%s) from Name(%s) VICTIM(%d,%d) SYNC(%d,%d)",
					tvDiff->tv_sec * 1000 + tvDiff->tv_usec / 1000, victim->GetName(), ch->GetName(), victim->GetX(), victim->GetY(),
					e->lX, e->lY);

				ch->GetDesc()->SetPhase(PHASE_CLOSE);//2025-10-18璟苟窟

				return -1;
			}
		}
		else if (fDist > 25.0f)
		{
			sys_err("Too far SyncPosition Distance(%f)(%s) from Name(%s) CH(%d,%d) VICTIM(%d,%d) SYNC(%d,%d)",
				fDist, victim->GetName(), ch->GetName(), ch->GetX(), ch->GetY(), victim->GetX(), victim->GetY(),
				e->lX, e->lY);

			ch->GetDesc()->SetPhase(PHASE_CLOSE);

			return -1;
		}
		else
		{
			victim->SetLastSyncTime(tvCurTime);
			victim->Sync(e->lX, e->lY);
			buffer_write(lpBuf, e, sizeof(TPacketCGSyncPositionElement));
		}
	}

	if (buffer_size(lpBuf) != sizeof(TPacketGCSyncPosition))
	{
		pHeader->bHeader = HEADER_GC_SYNC_POSITION;
		pHeader->wSize = buffer_size(lpBuf);

		ch->PacketAround(buffer_read_peek(lpBuf), buffer_size(lpBuf), ch);
	}

	return iExtraLen;
}

void CInputMain::FlyTarget(LPCHARACTER ch, const char* pcData, BYTE bHeader)
{
	TPacketCGFlyTargeting* p = (TPacketCGFlyTargeting*)pcData;
	ch->FlyTarget(p->dwTargetVID, p->x, p->y, bHeader);
}

void CInputMain::UseSkill(LPCHARACTER ch, const char* pcData)
{
	TPacketCGUseSkill* p = (TPacketCGUseSkill*)pcData;
	ch->UseSkill(p->dwVnum, CHARACTER_MANAGER::instance().Find(p->dwVID));
}

void CInputMain::ScriptButton(LPCHARACTER ch, const void* c_pData)
{
	TPacketCGScriptButton* p = (TPacketCGScriptButton*)c_pData;

	quest::PC* pc = quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID());
	if (pc && pc->IsConfirmWait())
	{
		quest::CQuestManager::instance().Confirm(ch->GetPlayerID(), quest::CONFIRM_TIMEOUT);
	}
	else if (p->idx & 0x80000000)
	{
		quest::CQuestManager::Instance().QuestInfo(ch->GetPlayerID(), p->idx & 0x7fffffff);
	}
	else
	{
		quest::CQuestManager::Instance().QuestButton(ch->GetPlayerID(), p->idx);
	}
}

void CInputMain::ScriptAnswer(LPCHARACTER ch, const void* c_pData)
{
	TPacketCGScriptAnswer* p = (TPacketCGScriptAnswer*)c_pData;

	if (p->answer > 250)
	{
		quest::CQuestManager::Instance().Resume(ch->GetPlayerID());
	}
	else
	{
		quest::CQuestManager::Instance().Select(ch->GetPlayerID(), p->answer);
	}
}

// SCRIPT_SELECT_ITEM
void CInputMain::ScriptSelectItem(LPCHARACTER ch, const void* c_pData)
{
	TPacketCGScriptSelectItem* p = (TPacketCGScriptSelectItem*)c_pData;
	sys_log(0, "QUEST ScriptSelectItem pid %d answer %d", ch->GetPlayerID(), p->selection);
	quest::CQuestManager::Instance().SelectItem(ch->GetPlayerID(), p->selection);
}
// END_OF_SCRIPT_SELECT_ITEM

void CInputMain::QuestInputString(LPCHARACTER ch, const void* c_pData)
{
	TPacketCGQuestInputString* p = (TPacketCGQuestInputString*)c_pData;

	char msg[65];
	strlcpy(msg, p->msg, sizeof(msg));
	sys_log(0, "QUEST InputString pid %u msg %s", ch->GetPlayerID(), msg);
	quest::CQuestManager::Instance().Input(ch->GetPlayerID(), msg);
}

void CInputMain::QuestConfirm(LPCHARACTER ch, const void* c_pData)
{
	TPacketCGQuestConfirm* p = (TPacketCGQuestConfirm*)c_pData;
	LPCHARACTER ch_wait = CHARACTER_MANAGER::instance().FindByPID(p->requestPID);
	if (p->answer)
		p->answer = quest::CONFIRM_YES;
	sys_log(0, "QuestConfirm from %s pid %u name %s answer %d", ch->GetName(), p->requestPID, (ch_wait)?ch_wait->GetName():"", p->answer);
	if (ch_wait)
	{
		quest::CQuestManager::Instance().Confirm(ch_wait->GetPlayerID(), (quest::EQuestConfirmType)p->answer, ch->GetPlayerID());
	}
}

void CInputMain::Target(LPCHARACTER ch, const char* pcData)
{
	TPacketCGTarget* p = (TPacketCGTarget*)pcData;

	building::LPOBJECT pkObj = building::CManager::instance().FindObjectByVID(p->dwVID);

	if (pkObj)
	{
		TPacketGCTarget pckTarget;
		pckTarget.header = HEADER_GC_TARGET;
		pckTarget.dwVID = p->dwVID;
		ch->GetDesc()->Packet(&pckTarget, sizeof(TPacketGCTarget));
	}
	else
		ch->SetTarget(CHARACTER_MANAGER::instance().Find(p->dwVID));
}

void CInputMain::Warp(LPCHARACTER ch, const char* pcData)
{
	ch->WarpEnd();
}

void CInputMain::SafeboxCheckin(LPCHARACTER ch, const char* c_pData)
{
	if (quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID())->IsRunning() == true)
		return;

	TPacketCGSafeboxCheckin* p = (TPacketCGSafeboxCheckin*)c_pData;

	if (!ch->CanHandleItem())
		return;

	CSafebox* pkSafebox = ch->GetSafebox();
	LPITEM pkItem = ch->GetItem(p->ItemPos);

	if (!pkSafebox || !pkItem)
		return;

	if (pkItem->GetCell() >= INVENTORY_MAX_NUM && IS_SET(pkItem->GetFlag(), ITEM_FLAG_IRREMOVABLE))
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<창고> 창고로 옮길 수 없는 아이템 입니다."));
		return;
	}

	// @lightworkfix END
	if (pkItem->IsEquipped())
	{
		return;
	}
	// @lightworkfix END

	if (!pkSafebox->IsEmpty(p->bSafePos, pkItem->GetSize()))
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<창고> 옮길 수 없는 위치입니다."));
		return;
	}
#ifdef ENABLE_SPECIAL_STORAGE
	if (pkItem->GetWindow() != INVENTORY)
	{
		if (true == pkItem->IsUpgradeItem() || true == pkItem->IsBook() || true == pkItem->IsStone() || true == pkItem->IsChest())
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("You can't move items from your special inventory"));
			
			return;
		}
	}
#endif
	if (pkItem->GetVnum() == UNIQUE_ITEM_SAFEBOX_EXPAND)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<창고> 창고로 옮길 수 없는 아이템 입니다."));
		return;
	}

	if (IS_SET(pkItem->GetAntiFlag(), ITEM_ANTIFLAG_SAFEBOX))
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<창고> 창고로 옮길 수 없는 아이템 입니다."));
		return;
	}

	if (true == pkItem->isLocked())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<창고> 창고로 옮길 수 없는 아이템 입니다."));
		return;
	}

	// @fixme140 BEGIN
	if (ITEM_BELT == pkItem->GetType() && CBeltInventoryHelper::IsExistItemInBeltInventory(ch))
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("벨트 인벤토리에 아이템이 존재하면 해제할 수 없습니다."));
		return;
	}
	// @fixme140 END

	pkItem->RemoveFromCharacter();
#ifdef ENABLE_SPECIAL_STORAGE
	if (!pkItem->IsDragonSoul() && !pkItem->IsUpgradeItem() && !pkItem->IsBook() && !pkItem->IsStone() && !pkItem->IsChest())
#else
	if (!pkItem->IsDragonSoul())
#endif
		ch->SyncQuickslot(QUICKSLOT_TYPE_ITEM, p->ItemPos.cell, UINT16_MAX);
	pkSafebox->Add(p->bSafePos, pkItem);
}

void CInputMain::SafeboxCheckout(LPCHARACTER ch, const char* c_pData, bool bMall)
{
	TPacketCGSafeboxCheckout* p = (TPacketCGSafeboxCheckout*)c_pData;

	if (!ch->CanHandleItem())
		return;

	CSafebox* pkSafebox;

	if (bMall)
		pkSafebox = ch->GetMall();
	else
		pkSafebox = ch->GetSafebox();

	if (!pkSafebox)
		return;

	LPITEM pkItem = pkSafebox->Get(p->bSafePos);

	if (!pkItem)
		return;

	if (!ch->IsEmptyItemGrid(p->ItemPos, pkItem->GetSize()))
		return;

	if (pkItem->IsDragonSoul())
	{
		if (bMall)
		{
			DSManager::instance().DragonSoulItemInitialize(pkItem);
		}

		if (DRAGON_SOUL_INVENTORY != p->ItemPos.window_type)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<창고> 옮길 수 없는 위치입니다."));
			return;
		}

		TItemPos DestPos = p->ItemPos;
		if (!DSManager::instance().IsValidCellForThisItem(pkItem, DestPos))
		{
			int iCell = ch->GetEmptyDragonSoulInventory(pkItem);
			if (iCell < 0)
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<창고> 옮길 수 없는 위치입니다."));
				return;
			}
			DestPos = TItemPos(DRAGON_SOUL_INVENTORY, iCell);
		}

		pkSafebox->Remove(p->bSafePos);
		pkItem->AddToCharacter(ch, DestPos);
		ITEM_MANAGER::instance().FlushDelayedSave(pkItem);
	}

#ifdef ENABLE_SPECIAL_STORAGE
	else if (p->ItemPos.window_type != INVENTORY)
	{
		if (pkItem->IsUpgradeItem())
		{
			if (UPGRADE_INVENTORY != p->ItemPos.window_type)
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<창고> 옮길 수 없는 위치입니다."));
				return;
			}

			TItemPos DestPos = p->ItemPos;

			int iCell = ch->GetEmptyUpgradeInventory(pkItem);
			if (iCell < 0)
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<창고> 옮길 수 없는 위치입니다."));
				return;
			}
			DestPos = TItemPos(UPGRADE_INVENTORY, iCell);

			pkSafebox->Remove(p->bSafePos);
			pkItem->AddToCharacter(ch, DestPos);
			ITEM_MANAGER::instance().FlushDelayedSave(pkItem);
		}
		else if (pkItem->IsBook())
		{
			if (BOOK_INVENTORY != p->ItemPos.window_type)
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<창고> 옮길 수 없는 위치입니다."));
				return;
			}

			TItemPos DestPos = p->ItemPos;

			int iCell = ch->GetEmptyBookInventory(pkItem);
			if (iCell < 0)
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<창고> 옮길 수 없는 위치입니다."));
				return;
			}
			DestPos = TItemPos(BOOK_INVENTORY, iCell);

			pkSafebox->Remove(p->bSafePos);
			pkItem->AddToCharacter(ch, DestPos);
			ITEM_MANAGER::instance().FlushDelayedSave(pkItem);
		}
		else if (pkItem->IsStone())
		{
			if (STONE_INVENTORY != p->ItemPos.window_type)
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<창고> 옮길 수 없는 위치입니다."));
				return;
			}

			TItemPos DestPos = p->ItemPos;

			int iCell = ch->GetEmptyStoneInventory(pkItem);
			if (iCell < 0)
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<창고> 옮길 수 없는 위치입니다."));
				return;
			}
			DestPos = TItemPos(STONE_INVENTORY, iCell);

			pkSafebox->Remove(p->bSafePos);
			pkItem->AddToCharacter(ch, DestPos);
			ITEM_MANAGER::instance().FlushDelayedSave(pkItem);
		}
		else if (pkItem->IsChest())
		{
			if (CHEST_INVENTORY != p->ItemPos.window_type)
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<창고> 옮길 수 없는 위치입니다."));
				return;
			}

			TItemPos DestPos = p->ItemPos;

			int iCell = ch->GetEmptyChestInventory(pkItem);
			if (iCell < 0)
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<창고> 옮길 수 없는 위치입니다."));
				return;
			}
			DestPos = TItemPos(CHEST_INVENTORY, iCell);

			pkSafebox->Remove(p->bSafePos);
			pkItem->AddToCharacter(ch, DestPos);
			ITEM_MANAGER::instance().FlushDelayedSave(pkItem);
		}
	}
#endif
	else
	{
#ifdef ENABLE_SPECIAL_STORAGE
		if (DRAGON_SOUL_INVENTORY == p->ItemPos.window_type ||
			UPGRADE_INVENTORY == p->ItemPos.window_type ||
			BOOK_INVENTORY == p->ItemPos.window_type ||
			STONE_INVENTORY == p->ItemPos.window_type ||
			CHEST_INVENTORY == p->ItemPos.window_type)
#else
		if (DRAGON_SOUL_INVENTORY == p->ItemPos.window_type)
#endif
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<창고> 옮길 수 없는 위치입니다."));
			return;
		}
		// @fixme119
		if (p->ItemPos.IsBeltInventoryPosition() && false == CBeltInventoryHelper::CanMoveIntoBeltInventory(pkItem))
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("이 아이템은 벨트 인벤토리로 옮길 수 없습니다."));
			return;
		}

		pkSafebox->Remove(p->bSafePos);
		pkItem->AddToCharacter(ch, p->ItemPos);
		ITEM_MANAGER::instance().FlushDelayedSave(pkItem);
	}

	DWORD dwID = pkItem->GetID();
	db_clientdesc->DBPacketHeader(HEADER_GD_ITEM_FLUSH, 0, sizeof(DWORD));
	db_clientdesc->Packet(&dwID, sizeof(DWORD));
}

void CInputMain::SafeboxItemMove(LPCHARACTER ch, const char* data)
{
	struct command_item_move* pinfo = (struct command_item_move*)data;

	if (!ch->CanHandleItem())
		return;

	if (!ch->GetSafebox())
		return;

	ch->GetSafebox()->MoveItem(pinfo->Cell.cell, pinfo->CellTo.cell, pinfo->count);
}

// PARTY_JOIN_BUG_FIX
void CInputMain::PartyInvite(LPCHARACTER ch, const char* c_pData)
{

	if (!ch) //@Lightwork#26
	{
		return;
	}

	if (ch->GetArena())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("대련장에서 사용하실 수 없습니다."));
		return;
	}

	TPacketCGPartyInvite* p = (TPacketCGPartyInvite*)c_pData;

	LPCHARACTER pInvitee = CHARACTER_MANAGER::instance().Find(p->vid);


	if (!pInvitee || !ch->GetDesc() || !pInvitee->GetDesc() || !pInvitee->IsPC() || !ch->IsPC()) //@Lightwork#26
	{
		sys_err("PARTY Cannot find invited character");
		return;
	}

#ifdef ENABLE_BOT_PLAYER
	if (pInvitee->IsBotCharacter())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<파티> %s 님이 파티 거부 상태입니다."), pInvitee->GetName());
		return;
	}
#endif

	ch->PartyInvite(pInvitee);
}

void CInputMain::PartyInviteAnswer(LPCHARACTER ch, const char* c_pData)
{
	if (!ch) //@Lightwork#26
	{
		return;
	}

	if (ch->GetArena())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("대련장에서 사용하실 수 없습니다."));
		return;
	}

	TPacketCGPartyInviteAnswer* p = (TPacketCGPartyInviteAnswer*)c_pData;

	LPCHARACTER pInviter = CHARACTER_MANAGER::instance().Find(p->leader_vid);

	//if (!pInviter)
	if (!pInviter || !pInviter->IsPC()) //@Lightwork#26
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<파티> 파티요청을 한 캐릭터를 찾을수 없습니다."));
	else if (!p->accept)
		pInviter->PartyInviteDeny(ch->GetPlayerID());
	else
		pInviter->PartyInviteAccept(ch);
}
// END_OF_PARTY_JOIN_BUG_FIX

void CInputMain::PartySetState(LPCHARACTER ch, const char* c_pData)
{
	if (!CPartyManager::instance().IsEnablePCParty())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<파티> 서버 문제로 파티 관련 처리를 할 수 없습니다."));
		return;
	}

	TPacketCGPartySetState* p = (TPacketCGPartySetState*)c_pData;

	if (!ch->GetParty())
		return;

	if (ch->GetParty()->GetLeaderPID() != ch->GetPlayerID())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<파티> 리더만 변경할 수 있습니다."));
		return;
	}

	if (!ch->GetParty()->IsMember(p->pid))
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<파티> 상태를 변경하려는 사람이 파티원이 아닙니다."));
		return;
	}

	DWORD pid = p->pid;

	switch (p->byRole)
	{
	case PARTY_ROLE_NORMAL:
		break;

	case PARTY_ROLE_ATTACKER:
	case PARTY_ROLE_TANKER:
	case PARTY_ROLE_BUFFER:
	case PARTY_ROLE_SKILL_MASTER:
	case PARTY_ROLE_HASTE:
	case PARTY_ROLE_DEFENDER:
		if (ch->GetParty()->SetRole(pid, p->byRole, p->flag))
		{
			TPacketPartyStateChange pack;
			pack.dwLeaderPID = ch->GetPlayerID();
			pack.dwPID = p->pid;
			pack.bRole = p->byRole;
			pack.bFlag = p->flag;
			db_clientdesc->DBPacket(HEADER_GD_PARTY_STATE_CHANGE, 0, &pack, sizeof(pack));
		}

		break;

	default:
		sys_err("wrong byRole in PartySetState Packet name %s state %d", ch->GetName(), p->byRole);
		break;
	}
}

void CInputMain::PartyRemove(LPCHARACTER ch, const char* c_pData)
{
	if (ch->GetArena())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("대련장에서 사용하실 수 없습니다."));
		return;
	}

	if (!CPartyManager::instance().IsEnablePCParty())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<파티> 서버 문제로 파티 관련 처리를 할 수 없습니다."));
		return;
	}

	if (ch->GetDungeon())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<파티> 던전 안에서는 파티에서 추방할 수 없습니다."));
		return;
	}

	TPacketCGPartyRemove* p = (TPacketCGPartyRemove*)c_pData;

	if (!ch->GetParty())
		return;

	LPPARTY pParty = ch->GetParty();
	if (pParty->GetLeaderPID() == ch->GetPlayerID())
	{
		if (ch->GetDungeon())
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<파티> 던젼내에서는 파티를 나갈 수 없습니다."));
		}
		else
		{
			if (pParty->IsPartyInDungeon(351))
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<파티>던전 안에 파티원이 있어 파티를 해산 할 수 없습니다."));
				return;
			}

			// leader can remove any member
			if (p->pid == ch->GetPlayerID() || pParty->GetMemberCount() == 2)
			{
				// party disband
				CPartyManager::instance().DeleteParty(pParty);
			}
			else
			{
				LPCHARACTER B = CHARACTER_MANAGER::instance().FindByPID(p->pid);
				if (B)
				{
					//pParty->SendPartyRemoveOneToAll(B);
					B->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<파티> 파티에서 추방당하셨습니다."));
					//pParty->Unlink(B);
					//CPartyManager::instance().SetPartyMember(B->GetPlayerID(), NULL);
				}
				pParty->Quit(p->pid);
			}
		}
	}
	else
	{
		// otherwise, only remove itself
		if (p->pid == ch->GetPlayerID())
		{
			if (ch->GetDungeon())
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<파티> 던젼내에서는 파티원을 추방할 수 없습니다."));
			}
			else
			{
				if (pParty->GetMemberCount() == 2)
				{
					// party disband
					CPartyManager::instance().DeleteParty(pParty);
				}
				else
				{
					ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<파티> 파티에서 나가셨습니다."));
					pParty->Quit(ch->GetPlayerID());
				}
			}
		}
		else
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<파티> 다른 파티원을 탈퇴시킬 수 없습니다."));
		}
	}
}

void CInputMain::AnswerMakeGuild(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGAnswerMakeGuild* p = (TPacketCGAnswerMakeGuild*)c_pData;

#ifdef ENABLE_GUIID_GOLD
	if (ch->GetGold() < LIMIT_GUIID_GOLD)
		return;
#else
	if (ch->GetGold() < 200000)
		return;
#endif

	if (ch->GetLevel() < 40)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Creating a guild level requires reaching level 40 or above"));
		return;
	}

	if (get_global_time() - ch->GetQuestFlag("guild_manage.new_disband_time") <
		CGuildManager::instance().GetDisbandDelay())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<길드> 해산한 후 %d일 이내에는 길드를 만들 수 없습니다."),
						quest::CQuestManager::instance().GetEventFlag ("guild_disband_delay"));
		return;
	}

	if (get_global_time() - ch->GetQuestFlag("guild_manage.new_withdraw_time") <
		CGuildManager::instance().GetWithdrawDelay())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<길드> 탈퇴한 후 %d일 이내에는 길드를 만들 수 없습니다."),
						quest::CQuestManager::instance().GetEventFlag ("guild_withdraw_delay"));
		return;
	}

	if (ch->GetGuild())
		return;

	CGuildManager& gm = CGuildManager::instance();

	TGuildCreateParameter cp;
	memset(&cp, 0, sizeof(cp));

	cp.master = ch;
	strlcpy(cp.name, p->guild_name, sizeof(cp.name));

	if (cp.name[0] == 0 || !check_name(cp.name))
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("적합하지 않은 길드 이름 입니다."));
		return;
	}

	DWORD dwGuildID = gm.CreateGuild(cp);

	if (dwGuildID)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<길드> [%s] 길드가 생성되었습니다."), cp.name);

#ifdef ENABLE_GUIID_GOLD
		int GuildCreateFee = LIMIT_GUIID_GOLD;
#else
		int GuildCreateFee = 200000;
#endif

		ch->PointChange(POINT_GOLD, -GuildCreateFee);
		//ch->SendGuildName(dwGuildID);
	}
	else
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<길드> 길드 생성에 실패하였습니다."));
}

void CInputMain::PartyUseSkill(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGPartyUseSkill* p = (TPacketCGPartyUseSkill*)c_pData;
	if (!ch->GetParty())
		return;

	if (ch->GetPlayerID() != ch->GetParty()->GetLeaderPID())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<파티> 파티 기술은 파티장만 사용할 수 있습니다."));
		return;
	}

	switch (p->bySkillIndex)
	{
	case PARTY_SKILL_HEAL:
		ch->GetParty()->HealParty();
		break;
	case PARTY_SKILL_WARP:
	{
		LPCHARACTER pch = CHARACTER_MANAGER::instance().Find(p->vid);
		if (pch->IsPC()) //@Lightwork#26
			ch->GetParty()->SummonToLeader(pch->GetPlayerID());
		else
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<파티> 소환하려는 대상을 찾을 수 없습니다."));
	}
	break;
	}
}

void CInputMain::PartyParameter(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGPartyParameter* p = (TPacketCGPartyParameter*)c_pData;

#ifdef ENABLE_EXP_GROUP_FIX
	if (ch->GetParty() && ch->GetParty()->GetLeaderPID() == ch->GetPlayerID())
#else
	if (ch->GetParty())
#endif
		ch->GetParty()->SetParameter(p->bDistributeMode);
}

size_t GetSubPacketSize(const GUILD_SUBHEADER_CG& header)
{
	switch (header)
	{
	case GUILD_SUBHEADER_CG_DEPOSIT_MONEY:				return sizeof(int);
	case GUILD_SUBHEADER_CG_WITHDRAW_MONEY:				return sizeof(int);
	case GUILD_SUBHEADER_CG_ADD_MEMBER:					return sizeof(DWORD);
	case GUILD_SUBHEADER_CG_REMOVE_MEMBER:				return sizeof(DWORD);
	case GUILD_SUBHEADER_CG_CHANGE_GRADE_NAME:			return 10;
	case GUILD_SUBHEADER_CG_CHANGE_GRADE_AUTHORITY:		return sizeof(BYTE) + sizeof(BYTE);
	case GUILD_SUBHEADER_CG_OFFER:						return sizeof(DWORD);
	case GUILD_SUBHEADER_CG_CHARGE_GSP:					return sizeof(int);
	case GUILD_SUBHEADER_CG_POST_COMMENT:				return 1;
	case GUILD_SUBHEADER_CG_DELETE_COMMENT:				return sizeof(DWORD);
	case GUILD_SUBHEADER_CG_REFRESH_COMMENT:			return 0;
	case GUILD_SUBHEADER_CG_CHANGE_MEMBER_GRADE:		return sizeof(DWORD) + sizeof(BYTE);
	case GUILD_SUBHEADER_CG_USE_SKILL:					return sizeof(TPacketCGGuildUseSkill);
	case GUILD_SUBHEADER_CG_CHANGE_MEMBER_GENERAL:		return sizeof(DWORD) + sizeof(BYTE);
	case GUILD_SUBHEADER_CG_GUILD_INVITE_ANSWER:		return sizeof(DWORD) + sizeof(BYTE);
	}

	return 0;
}

int CInputMain::Guild(LPCHARACTER ch, const char* data, size_t uiBytes)
{
	if (uiBytes < sizeof(TPacketCGGuild))
		return -1;

	const TPacketCGGuild* p = reinterpret_cast<const TPacketCGGuild*>(data);
	const char* c_pData = data + sizeof(TPacketCGGuild);

	uiBytes -= sizeof(TPacketCGGuild);

	const GUILD_SUBHEADER_CG SubHeader = static_cast<GUILD_SUBHEADER_CG>(p->subheader);
	const size_t SubPacketLen = GetSubPacketSize(SubHeader);

	if (uiBytes < SubPacketLen)
	{
		return -1;
	}

	CGuild* pGuild = ch->GetGuild();

	if (NULL == pGuild)
	{
		if (SubHeader != GUILD_SUBHEADER_CG_GUILD_INVITE_ANSWER)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<길드> 길드에 속해있지 않습니다."));
			return SubPacketLen;
		}
	}

	switch (SubHeader)
	{
	case GUILD_SUBHEADER_CG_DEPOSIT_MONEY:
	{
		return SubPacketLen;

		const int gold = MIN(*reinterpret_cast<const int*>(c_pData), __deposit_limit());

		if (gold < 0)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<길드> 잘못된 금액입니다."));
			return SubPacketLen;
		}

		if (ch->GetGold() < gold)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<길드> 가지고 있는 돈이 부족합니다."));
			return SubPacketLen;
		}

		pGuild->RequestDepositMoney(ch, gold);
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_WITHDRAW_MONEY:
	{
		return SubPacketLen;
#ifdef ENABLE_GUILD_YANG_ACCOUNTING_FIX
		const int gold = *reinterpret_cast<const int*>(c_pData);
#else
		const int gold = MIN(*reinterpret_cast<const int*>(c_pData), 500000);
#endif
		if (gold < 0)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<길드> 잘못된 금액입니다."));
			return SubPacketLen;
		}
#ifdef ENABLE_GUILD_YANG_ACCOUNTING_FIX
		if(ch->GetGold()+gold >= GOLD_MAX)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "쏜귑綠댐돕離댕令");
			return SubPacketLen;
		}
#endif
		pGuild->RequestWithdrawMoney(ch, gold);
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_ADD_MEMBER:
	{
		const DWORD vid = *reinterpret_cast<const DWORD*>(c_pData);
		LPCHARACTER newmember = CHARACTER_MANAGER::instance().Find(vid);

		if (!newmember)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<길드> 그러한 사람을 찾을 수 없습니다."));
			return SubPacketLen;
		}

		// @fixme145 BEGIN (+newmember ispc check)
		if (!ch->IsPC() || !newmember->IsPC())
			return SubPacketLen;
		// @fixme145 END

		pGuild->Invite(ch, newmember);
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_REMOVE_MEMBER:
	{
		if (pGuild->UnderAnyWar() != 0)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<길드> 길드전 중에는 길드원을 탈퇴시킬 수 없습니다."));
			return SubPacketLen;
		}

		const DWORD pid = *reinterpret_cast<const DWORD*>(c_pData);
		const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

		if (NULL == m)
			return -1;

		LPCHARACTER member = CHARACTER_MANAGER::instance().FindByPID(pid);

		if (member)
		{
			if (member->GetGuild() != pGuild)
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<길드> 상대방이 같은 길드가 아닙니다."));
				return SubPacketLen;
			}

			if (!pGuild->HasGradeAuth(m->grade, GUILD_AUTH_REMOVE_MEMBER))
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<길드> 길드원을 강제 탈퇴 시킬 권한이 없습니다."));
				return SubPacketLen;
			}

			member->SetQuestFlag("guild_manage.new_withdraw_time", get_global_time());
			pGuild->RequestRemoveMember(member->GetPlayerID());
		}
		else
		{
			if (!pGuild->HasGradeAuth(m->grade, GUILD_AUTH_REMOVE_MEMBER))
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<길드> 길드원을 강제 탈퇴 시킬 권한이 없습니다."));
				return SubPacketLen;
			}

			if (pGuild->RequestRemoveMember(pid))
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<길드> 길드원을 강제 탈퇴 시켰습니다."));
			else
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<길드> 그러한 사람을 찾을 수 없습니다."));
		}
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_CHANGE_GRADE_NAME:
	{
		char gradename[GUILD_GRADE_NAME_MAX_LEN + 1];
		strlcpy(gradename, c_pData + 1, sizeof(gradename));

		const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

		if (NULL == m)
			return -1;

		if (m->grade != GUILD_LEADER_GRADE)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<길드> 직위 이름을 변경할 권한이 없습니다."));
		}
		else if (*c_pData == GUILD_LEADER_GRADE)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<길드> 길드장의 직위 이름은 변경할 수 없습니다."));
		}
		else if (!check_name(gradename))
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<길드> 적합하지 않은 직위 이름 입니다."));
		}
		else
		{
			pGuild->ChangeGradeName(*c_pData, gradename);
		}
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_CHANGE_GRADE_AUTHORITY:
	{
		const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

		if (NULL == m)
			return -1;

		if (m->grade != GUILD_LEADER_GRADE)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<길드> 직위 권한을 변경할 권한이 없습니다."));
		}
		else if (*c_pData == GUILD_LEADER_GRADE)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<길드> 길드장의 권한은 변경할 수 없습니다."));
		}
		else
		{
			pGuild->ChangeGradeAuth(*c_pData, *(c_pData + 1));
		}
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_OFFER:
	{
#ifdef ENABLE_CHAR_SETTINGS
		if (ch->GetCharSettings().antiexp)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Your guild has reached the highest level"));
			return SubPacketLen;
		}
#endif
		DWORD offer = *reinterpret_cast<const DWORD*>(c_pData);

		if (pGuild->GetLevel() >= GUILD_MAX_LEVEL)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<길드> 길드가 이미 최고 레벨입니다."));
		}
		else
		{
			offer /= 100;
			offer *= 100;

			if (pGuild->OfferExp(ch, offer))
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<길드> %u의 경험치를 투자하였습니다."), offer);
#ifdef ENABLE_EXTENDED_BATTLE_PASS
				ch->UpdateExtBattlePassMissionProgress(GUILD_SPENT_EXP, offer, 0);
#endif
			}
			else
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<길드> 경험치 투자에 실패하였습니다."));
			}
		}
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_CHARGE_GSP:
	{
		const int offer = *reinterpret_cast<const int*>(c_pData);
		const int gold = offer * 100;

		if (offer < 0 || gold < offer || gold < 0 || ch->GetGold() < gold)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<길드> 돈이 부족합니다."));
			return SubPacketLen;
		}

		if (!pGuild->ChargeSP(ch, offer))
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<길드> 용신력 회복에 실패하였습니다."));
		}
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_POST_COMMENT:
	{
		const size_t length = *c_pData;

		if (length > GUILD_COMMENT_MAX_LEN)
		{
			sys_err("POST_COMMENT: %s comment too long (length: %u)", ch->GetName(), length);
			ch->GetDesc()->SetPhase(PHASE_CLOSE);
			return -1;
		}

		if (uiBytes < 1 + length)
			return -1;

		const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

		if (NULL == m)
			return -1;

		if (length && !pGuild->HasGradeAuth(m->grade, GUILD_AUTH_NOTICE) && *(c_pData + 1) == '!')
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<길드> 공지글을 작성할 권한이 없습니다."));
		}
		else
		{
			std::string str(c_pData + 1, length);
			pGuild->AddComment(ch, str);
		}

		return (1 + length);
	}

	case GUILD_SUBHEADER_CG_DELETE_COMMENT:
	{
		const DWORD comment_id = *reinterpret_cast<const DWORD*>(c_pData);

		pGuild->DeleteComment(ch, comment_id);
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_REFRESH_COMMENT:
		pGuild->RefreshComment(ch);
		return SubPacketLen;

	case GUILD_SUBHEADER_CG_CHANGE_MEMBER_GRADE:
	{
		const DWORD pid = *reinterpret_cast<const DWORD*>(c_pData);
		const BYTE grade = *(c_pData + sizeof(DWORD));
		const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

		if (NULL == m)
			return -1;

		if (m->grade != GUILD_LEADER_GRADE)
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<길드> 직위를 변경할 권한이 없습니다."));
		else if (ch->GetPlayerID() == pid)
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<길드> 길드장의 직위는 변경할 수 없습니다."));
		else if (grade == 1)
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<길드> 길드장으로 직위를 변경할 수 없습니다."));
		else
			pGuild->ChangeMemberGrade(pid, grade);
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_USE_SKILL:
	{
		const TPacketCGGuildUseSkill* p = reinterpret_cast<const TPacketCGGuildUseSkill*>(c_pData);

		pGuild->UseSkill(p->dwVnum, ch, p->dwPID);
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_CHANGE_MEMBER_GENERAL:
	{
		const DWORD pid = *reinterpret_cast<const DWORD*>(c_pData);
		const BYTE is_general = *(c_pData + sizeof(DWORD));
		const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

		if (NULL == m)
			return -1;

		if (m->grade != GUILD_LEADER_GRADE)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<길드> 장군을 지정할 권한이 없습니다."));
		}
		else
		{
			if (!pGuild->ChangeMemberGeneral(pid, is_general))
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<길드> 더이상 장수를 지정할 수 없습니다."));
			}
		}
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_GUILD_INVITE_ANSWER:
	{
		const DWORD guild_id = *reinterpret_cast<const DWORD*>(c_pData);
		const BYTE accept = *(c_pData + sizeof(DWORD));

		CGuild* g = CGuildManager::instance().FindGuild(guild_id);

		if (g)
		{
			if (accept)
				g->InviteAccept(ch);
			else
				g->InviteDeny(ch->GetPlayerID());
		}
	}
	return SubPacketLen;
	}

	return 0;
}

void CInputMain::Fishing(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGFishing* p = (TPacketCGFishing*)c_pData;
	ch->SetRotation(p->dir * 5);
	ch->fishing();
	return;
}

void CInputMain::ItemGive(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGGiveItem* p = (TPacketCGGiveItem*)c_pData;
	LPCHARACTER to_ch = CHARACTER_MANAGER::instance().Find(p->dwTargetVID);

	if (to_ch)
		ch->GiveItem(to_ch, p->ItemPos);
	else
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("아이템을 건네줄 수 없습니다."));
}

void CInputMain::Hack(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGHack* p = (TPacketCGHack*)c_pData;

	char buf[sizeof(p->szBuf)];
	strlcpy(buf, p->szBuf, sizeof(buf));

	sys_err("HACK_DETECT: %s %s", ch->GetName(), buf);

	ch->GetDesc()->SetPhase(PHASE_CLOSE);
}

int CInputMain::MyShop(LPCHARACTER ch, const char* c_pData, size_t uiBytes)
{
	TPacketCGMyShop* p = (TPacketCGMyShop*)c_pData;
	int iExtraLen = p->bCount * sizeof(TShopItemTable);

	if (uiBytes < sizeof(TPacketCGMyShop) + iExtraLen)
		return -1;

	if (ch->GetGold() >= GOLD_MAX)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("소유 돈이 20억냥을 넘어 거래를 핼수가 없습니다."));
		sys_log(0, "MyShop ==> OverFlow Gold id %u name %s ", ch->GetPlayerID(), ch->GetName());
		return (iExtraLen);
	}

	if (ch->IsStun() || ch->IsDead())
		return (iExtraLen);

	if (ch->GetExchange() || ch->IsOpenSafebox() || ch->GetShopOwner() || ch->IsCubeOpen()
#ifdef ENABLE_AURA_SYSTEM
		|| ch->isAuraOpened(true) || ch->isAuraOpened(false)
#endif
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		|| ch->IsAcceOpened()
#endif
#ifdef ENABLE_6TH_7TH_ATTR
		|| ch->Is67AttrOpen()
#endif
		)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Make sure you don't have any open windows2"));
		return (iExtraLen);
	}

	sys_log(0, "MyShop count %d", p->bCount);
	ch->OpenMyShop(p->szSign, (TShopItemTable*)(c_pData + sizeof(TPacketCGMyShop)), p->bCount);
	return (iExtraLen);
}

void CInputMain::Refine(LPCHARACTER ch, const char* c_pData)
{
	const TPacketCGRefine* p = reinterpret_cast<const TPacketCGRefine*>(c_pData);

	// if (ch->IsDead() || ch->IsStun())
		// return;
	
	if (ch->GetExchange() || ch->IsOpenSafebox() || ch->GetShopOwner() || ch->GetMyShop() || ch->IsAcceOpened() || ch->IsOpenOfflineShop()
	// ch->IsCubeOpen() || 
	// || ch->ActivateCheck()
#ifdef ENABLE_AURA_SYSTEM
		|| ch->isAuraOpened(true) || ch->isAuraOpened(false)
#endif
#ifdef ENABLE_6TH_7TH_ATTR
		|| ch->Is67AttrOpen()
#endif
		)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT ("창고,거래창등이 열린 상태에서는 개량을 할수가 없습니다"));
		ch->ClearRefineMode();
		return;
	}

	if (p->type == 255)
	{
		ch->ClearRefineMode();
		return;
	}

	if (p->pos >= INVENTORY_MAX_NUM)
	{
		ch->ClearRefineMode();
		return;
	}

	LPITEM item = ch->GetInventoryItem(p->pos);

	if (!item)
	{
		ch->ClearRefineMode();
		return;
	}
	
	ch->SetActivateTime(REFINE_CHECK_TIME);

	if (p->type == REFINE_TYPE_NORMAL)
	{
		ch->DoRefine(item);
	}
	else if (p->type == REFINE_TYPE_SCROLL || p->type == REFINE_TYPE_HYUNIRON || p->type == REFINE_TYPE_MUSIN || p->type == REFINE_TYPE_BDRAGON)
	{
		ch->DoRefineWithScroll(item);
	}
	else if (p->type == REFINE_TYPE_MONEY_ONLY)
	{
		const LPITEM item = ch->GetInventoryItem (p->pos);

		if (NULL != item)
		{
			#ifdef ENABLE_DEVILTOWER_LIMIT
			if (ENABLE_DEVILTOWER_LIMIT <= item->GetRefineSet())
			#else
			if (500 <= item->GetRefineSet())
			#endif
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Devil Tower Refining Hacker"));
			}
			else
			{
				if (ch->GetQuestFlag ("deviltower_zone.can_refine"))
				{
					if (ch->DoRefine (item, true))
					{
						ch->SetQuestFlag ("deviltower_zone.can_refine", 0);
					}
				}
				else
				{
					ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("사귀 타워 완료 보상은 한번까지 사용가능합니다."));
				}
			}
		}
	}
	ch->ClearRefineMode();
}

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
void CInputMain::Acce(LPCHARACTER pkChar, const char* c_pData)
{
	quest::PC* pPC = quest::CQuestManager::instance().GetPCForce(pkChar->GetPlayerID());
	if (pPC->IsRunning())
		return;

	TPacketAcce* sPacket = (TPacketAcce*)c_pData;
	switch (sPacket->subheader)
	{
	case ACCE_SUBHEADER_CG_CLOSE:
	{
		pkChar->CloseAcce();
	}
	break;
	case ACCE_SUBHEADER_CG_ADD:
	{
		pkChar->AddAcceMaterial(sPacket->tPos, sPacket->bPos);
	}
	break;
	case ACCE_SUBHEADER_CG_REMOVE:
	{
		pkChar->RemoveAcceMaterial(sPacket->bPos);
	}
	break;
	case ACCE_SUBHEADER_CG_REFINE:
	{
		pkChar->RefineAcceMaterials();
	}
	break;
	default:
		break;
	}
}
#endif

#ifdef ENABLE_TARGET_INFORMATION_SYSTEM
#ifndef ENABLE_TARGET_INFORMATION_RENEWAL
void CInputMain::TargetInfoLoad(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGTargetInfoLoad* p = (TPacketCGTargetInfoLoad*)c_pData;
	TPacketGCTargetInfo pInfo;
	pInfo.header = HEADER_GC_TARGET_INFO;
	static std::vector<LPITEM> s_vec_item;
	s_vec_item.clear();
	LPITEM pkInfoItem;
	LPCHARACTER m_pkChrTarget = CHARACTER_MANAGER::instance().Find(p->dwVID);

	if (!ch || !m_pkChrTarget) {
		return;
	}

	if (!ch->GetDesc()) {
		return;
	}
#ifdef ENABLE_BOT_PLAYER
	if (ch->IsBotCharacter() || m_pkChrTarget->IsBotCharacter())
		return;
#endif
	if (ITEM_MANAGER::instance().CreateDropItemVector(m_pkChrTarget, ch, s_vec_item) && (m_pkChrTarget->IsMonster() || m_pkChrTarget->IsStone()))
	{
		if (s_vec_item.size() == 0);
		else if (s_vec_item.size() == 1)
		{
			pkInfoItem = s_vec_item[0];
			pInfo.dwVID = m_pkChrTarget->GetVID();
			pInfo.race = m_pkChrTarget->GetRaceNum();
			pInfo.dwVnum = pkInfoItem->GetVnum();
			pInfo.count = pkInfoItem->GetCount();
			ch->GetDesc()->Packet(&pInfo, sizeof(TPacketGCTargetInfo));
		}
		else
		{
			int iItemIdx = s_vec_item.size() - 1;
			while (iItemIdx >= 0)
			{
				pkInfoItem = s_vec_item[iItemIdx--];
				if (!pkInfoItem)
				{
					sys_err("pkInfoItem null in vector idx %d", iItemIdx + 1);
					continue;
				}
				pInfo.dwVID = m_pkChrTarget->GetVID();
				pInfo.race = m_pkChrTarget->GetRaceNum();
				pInfo.dwVnum = pkInfoItem->GetVnum();
				pInfo.count = pkInfoItem->GetCount();
				ch->GetDesc()->Packet(&pInfo, sizeof(TPacketGCTargetInfo));
			}
		}
	}
}
#endif
#endif

#ifdef ENABLE_CUBE_RENEWAL_WORLDARD
void CInputMain::CubeRenewalSend(LPCHARACTER ch, const char* data)
{
	struct packet_send_cube_renewal* pinfo = (struct packet_send_cube_renewal*)data;
	switch (pinfo->subheader)
	{
	case CUBE_RENEWAL_SUB_HEADER_MAKE_ITEM:
	{
		int index_item = pinfo->index_item;
		int count_item = pinfo->count_item;

		Cube_Make(ch, index_item, count_item);
	}
	break;

	case CUBE_RENEWAL_SUB_HEADER_CLOSE:
	{
		Cube_close(ch);
	}
	break;
	}
}
#endif

#ifdef ENABLE_OFFLINE_SHOP
#include "new_offlineshop.h"
#include "new_offlineshop_manager.h"
template <class T>
bool CanDecode(T* p, int buffleft)
{
	return buffleft >= (int)sizeof(T);
}

template <class T>
const char* Decode(T*& pObj, const char* data, int* pbufferLeng = nullptr, int* piBufferLeft = nullptr)
{
	pObj = (T*)data;

	if (pbufferLeng)
		*pbufferLeng += sizeof(T);

	if (piBufferLeft)
		*piBufferLeft -= sizeof(T);

	return data + sizeof(T);
}

int OfflineshopPacketCreateNewShop(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGShopCreate* pack = nullptr;
	if (!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra = 0;
	data = Decode(pack, data, &iExtra, &iBufferLeft);

	offlineshop::TShopInfo& rShopInfo = pack->shop;

	if (rShopInfo.dwCount > 500)
		return -1;

	std::vector<offlineshop::TShopItemInfo> vec;
	vec.reserve(rShopInfo.dwCount);

	offlineshop::TShopItemInfo* pItem = nullptr;

	for (DWORD i = 0; i < rShopInfo.dwCount; ++i)
	{
		if (!CanDecode(pItem, iBufferLeft))
			return -1;

		data = Decode(pItem, data, &iExtra, &iBufferLeft);
		vec.push_back(*pItem);
	}

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if (!rManager.RecvShopCreateNewClientPacket(ch, rShopInfo, vec))
		// ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Storage items are not available"));
		return iExtra;
	return iExtra;
}

int OfflineshopPacketChangeShopName(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGShopChangeName* pack = nullptr;
	if (!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra = 0;
	data = Decode(pack, data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if (!rManager.RecvShopChangeNameClientPacket(ch, pack->szName))
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Input cannot be empty"));

	return iExtra;
}

#ifdef ENABLE_OFFLINESHOP_EXTEND_TIME
int OfflineshopPacketExtendTime(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGShopExtendTime* pack = nullptr;
	if(!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra=0;
	data = Decode(pack,data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if(!rManager.RecvShopExtendTimeClientPacket(ch, pack->dwTime))
		// ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Extension of stall time failed, please try again later"));
		return iExtra;
	return iExtra;
}
#endif

#ifdef ENABLE_AVERAGE_PRICE
int OfflineshopPacketAveragePrice(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	DWORD* pack = nullptr;
	if (!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra = 0;
	data = Decode(pack, data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	rManager.RecvAveragePrice(ch, *pack);

	return iExtra;
}
#endif

int OfflineshopPacketForceCloseShop(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if (!rManager.RecvShopForceCloseClientPacket(ch))
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Store closure unavailable"));

	return 0;
}

int OfflineshopPacketRequestShopList(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	rManager.RecvShopRequestListClientPacket(ch);
	return 0;
}

int OfflineshopPacketOpenShop(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGShopOpen* pack = nullptr;
	if (!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra = 0;
	data = Decode(pack, data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if (!rManager.RecvShopOpenClientPacket(ch, pack->dwOwnerID))
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("CLOSE_WINDOWS_OPENSHOP"));
	return iExtra;
}

int OfflineshopPacketOpenShowOwner(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if (!rManager.RecvShopOpenMyShopClientPacket(ch))
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Cannot open shop"));

	return 0;
}

int OfflineshopPacketBuyItem(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGShopBuyItem* pack = nullptr;
	if (!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra = 0;
	data = Decode(pack, data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if (!rManager.RecvShopBuyItemClientPacket(ch, pack->dwOwnerID, pack->dwItemID, pack->bIsSearch))
		// ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Not enough gold coins"));
		return iExtra;
	return iExtra;
}

int OfflineshopPacketAddItem(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGAddItem* pack = nullptr;
	if (!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra = 0;
	data = Decode(pack, data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if (!rManager.RecvShopAddItemClientPacket(ch, pack->pos, pack->price
#ifdef ENABLE_OFFLINE_SHOP_GRID
		, pack->display_pos
#endif
	))
		// ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Placement location not available"));
		return iExtra;
	return iExtra;
}

int OfflineshopPacketRemoveItem(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGRemoveItem* pack = nullptr;
	if (!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra = 0;
	data = Decode(pack, data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if (!rManager.RecvShopRemoveItemClientPacket(ch, pack->dwItemID))
		// ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Removing items from the store is not available"));
		return iExtra;
	return iExtra;
}

int OfflineshopPacketEditItem(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGEditItem* pack = nullptr;
	if (!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra = 0;
	data = Decode(pack, data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if (!rManager.RecvShopEditItemClientPacket(ch, pack->dwItemID, pack->price, pack->allEdit))
		// ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Editing store items is not available"));
		return iExtra;
	return iExtra;
}

int OfflineshopPacketFilterRequest(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGFilterRequest* pack = nullptr;
	if (!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra = 0;
	data = Decode(pack, data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if (!rManager.RecvShopFilterRequestClientPacket(ch, pack->filter))
		// ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Item search not available"));
		return iExtra;
	return iExtra;
}

int OfflineshopPacketOpenSafebox(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if (!rManager.RecvShopSafeboxOpenClientPacket(ch))
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Not available when opening the store"));

	return 0;
}

int OfflineshopPacketCloseBoard(LPCHARACTER ch)
{
	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	rManager.RecvCloseBoardClientPacket(ch);
	return 0;
}

int OfflineshopPacketGetItemSafebox(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGShopSafeboxGetItem* pack = nullptr;
	if (!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra = 0;
	data = Decode(pack, data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if (!rManager.RecvShopSafeboxGetItemClientPacket(ch, pack->dwItemID))
		// ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Extracting items from the stall warehouse is not available"));
		return iExtra;
	return iExtra;
}

int OfflineshopPacketGetValutesSafebox(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGShopSafeboxGetValutes *pack = nullptr;
	if (!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra = 0;
	data = Decode(pack, data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if (!rManager.RecvShopSafeboxGetValutesClientPacket(ch, pack->gold))
		// ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Extracting coins is not available"));
		return iExtra;
	return iExtra;
}

int OfflineshopPacketCloseSafebox(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if (!rManager.RecvShopSafeboxCloseClientPacket(ch))
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Closed warehouse unavailable"));

	return 0;
}


#ifdef ENABLE_SHOPS_IN_CITIES
int OfflineshopPacketClickEntity(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGShopClickEntity* pack = nullptr;
	if (!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra = 0;
	data = Decode(pack, data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	rManager.RecvShopClickEntity(ch, pack->dwShopVID);
	return iExtra;
}
#endif

int OfflineshopPacket(const char* data, LPCHARACTER ch, long iBufferLeft)
{
	if (iBufferLeft < (int)sizeof(TPacketCGNewOfflineShop))
		return -1;

	TPacketCGNewOfflineShop* pPack = nullptr;
	iBufferLeft -= sizeof(TPacketCGNewOfflineShop);
	data = Decode(pPack, data);

	switch (pPack->bSubHeader)
	{
	case offlineshop::SUBHEADER_CG_SHOP_CREATE_NEW:				return OfflineshopPacketCreateNewShop(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_CHANGE_NAME:			return OfflineshopPacketChangeShopName(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_FORCE_CLOSE:			return OfflineshopPacketForceCloseShop(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_REQUEST_SHOPLIST:		return OfflineshopPacketRequestShopList(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_OPEN:					return OfflineshopPacketOpenShop(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_OPEN_OWNER:				return OfflineshopPacketOpenShowOwner(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_BUY_ITEM:				return OfflineshopPacketBuyItem(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_ADD_ITEM:				return OfflineshopPacketAddItem(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_REMOVE_ITEM:			return OfflineshopPacketRemoveItem(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_EDIT_ITEM:				return OfflineshopPacketEditItem(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_FILTER_REQUEST:			return OfflineshopPacketFilterRequest(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_SAFEBOX_OPEN:			return OfflineshopPacketOpenSafebox(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_SAFEBOX_GET_ITEM:		return OfflineshopPacketGetItemSafebox(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_SAFEBOX_GET_VALUTES:	return OfflineshopPacketGetValutesSafebox(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_SAFEBOX_CLOSE:			return OfflineshopPacketCloseSafebox(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_CLOSE_BOARD:					return OfflineshopPacketCloseBoard(ch);
#ifdef ENABLE_SHOPS_IN_CITIES
	case offlineshop::SUBHEADER_CG_CLICK_ENTITY:				return OfflineshopPacketClickEntity(ch, data, iBufferLeft);
#endif
#ifdef ENABLE_OFFLINESHOP_EXTEND_TIME
	case offlineshop::SUBHEADER_CG_SHOP_EXTEND_TIME:			return OfflineshopPacketExtendTime(ch, data, iBufferLeft);
#endif
#ifdef ENABLE_AVERAGE_PRICE
	case offlineshop::SUBHEADER_CG_AVERAGE_PRICE:				return OfflineshopPacketAveragePrice(ch, data, iBufferLeft);
#endif

	default:
		sys_err("UNKNOWN SUBHEADER %d ", pPack->bSubHeader);
		return -1;
	}
}
#endif

#ifdef ENABLE_EXTENDED_BATTLE_PASS
int CInputMain::ReciveExtBattlePassActions(LPCHARACTER ch, const char* data, size_t uiBytes)
{
	TPacketCGExtBattlePassAction* p = (TPacketCGExtBattlePassAction*)data;

	if (uiBytes < sizeof(TPacketCGExtBattlePassAction))
		return -1;

	uiBytes -= sizeof(TPacketCGExtBattlePassAction);

	switch (p->bAction)
	{
	case 1:
		CBattlePassManager::instance().BattlePassRequestOpen(ch);
		return 0;

	case 2:
		if (get_dword_time() < ch->GetLastReciveExtBattlePassOpenRanking()) { 
			return 0;
		}
		ch->SetLastReciveExtBattlePassOpenRanking(get_dword_time() + 10000);

		for (BYTE bBattlePassType = 1; bBattlePassType <= 3; ++bBattlePassType)
		{
			BYTE bBattlePassID;
			if (bBattlePassType == 1)
				bBattlePassID = CBattlePassManager::instance().GetNormalBattlePassID();
			if (bBattlePassType == 2) {
				bBattlePassID = CBattlePassManager::instance().GetPremiumBattlePassID();
				//if (bBattlePassID != ch->GetExtBattlePassPremiumID())
				//	continue;
				if (!ch->FindAffect(AFFECT_BATTLEPASS))
					continue;
			}
			if (bBattlePassType == 3)
				bBattlePassID = CBattlePassManager::instance().GetEventBattlePassID();
#ifdef ENABLE_PLAYER2_ACCOUNT2__
			std::unique_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT player_name, battlepass_type+0, battlepass_id, UNIX_TIMESTAMP(start_time), UNIX_TIMESTAMP(end_time) FROM player2.battlepass_playerindex WHERE battlepass_type = %d and battlepass_id = %d and battlepass_completed = 1 and not player_name LIKE '[%%' ORDER BY (UNIX_TIMESTAMP(end_time)-UNIX_TIMESTAMP(start_time)) ASC LIMIT 40", bBattlePassType, bBattlePassID));
#else
			std::unique_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT player_name, battlepass_type+0, battlepass_id, UNIX_TIMESTAMP(start_time), UNIX_TIMESTAMP(end_time) FROM player.battlepass_playerindex WHERE battlepass_type = %d and battlepass_id = %d and battlepass_completed = 1 and not player_name LIKE '[%%' ORDER BY (UNIX_TIMESTAMP(end_time)-UNIX_TIMESTAMP(start_time)) ASC LIMIT 40", bBattlePassType, bBattlePassID));
#endif
			if (pMsg->uiSQLErrno)
				return 0;

			MYSQL_ROW row;

			while ((row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
			{
				TPacketGCExtBattlePassRanking pack;
				pack.bHeader = HEADER_GC_EXT_BATTLE_PASS_SEND_RANKING;
				strlcpy(pack.szPlayerName, row[0], sizeof(pack.szPlayerName));
				pack.bBattlePassType = std::atoi(row[1]);
				pack.bBattlePassID = std::atoll(row[2]);
				pack.dwStartTime = std::atoll(row[3]);
				pack.dwEndTime = std::atoll(row[4]);

				ch->GetDesc()->Packet(&pack, sizeof(pack));
			}
		}
		break;

	case 10:
		CBattlePassManager::instance().BattlePassRequestReward(ch, 1);
		return 0;

	case 11:
		CBattlePassManager::instance().BattlePassRequestReward(ch, 2);
		return 0;

	case 12:
		CBattlePassManager::instance().BattlePassRequestReward(ch, 3);
		return 0;


	default:
		break;
	}

	return 0;
}

int CInputMain::ReciveExtBattlePassPremium(LPCHARACTER ch, const char* data, size_t uiBytes)
{
	TPacketCGExtBattlePassSendPremium* p = (TPacketCGExtBattlePassSendPremium*)data;
	if (uiBytes < sizeof(TPacketCGExtBattlePassSendPremium))
		return -1;

	uiBytes -= sizeof(TPacketCGExtBattlePassSendPremium);

	if (p->premium && ch->FindAffect(AFFECT_BATTLEPASS))
	{
		ch->PointChange(POINT_BATTLE_PASS_PREMIUM_ID, CBattlePassManager::instance().GetPremiumBattlePassID());
		CBattlePassManager::instance().BattlePassRequestOpen(ch);
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Successfully activated advanced pass"));
	}

	return 0;
}
#endif

#ifdef ENABLE_6TH_7TH_ATTR
void CInputMain::Attr67(LPCHARACTER ch, const char* c_pData)
{
	const TPacketCG67Attr* p = reinterpret_cast<const TPacketCG67Attr*>(c_pData);
	
	if (ch->IsDead())
		return;

	if (ch->GetAttrInventoryItem())
		return;

	if (ch->GetExchange() || ch->IsOpenSafebox() || ch->GetShopOwner() || ch->IsCubeOpen()
#ifdef ENABLE_AURA_SYSTEM
		|| ch->isAuraOpened(true) || ch->isAuraOpened(false)
#endif
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		|| ch->IsAcceOpened()
#endif
		)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("CLOSE_WINDOWS_ERROR1"));
		return;
	}

	const LPITEM item = ch->GetInventoryItem(p->sItemPos);
	if (!item)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("There is no item."));
		return;
	}

	switch (item->GetType())
	{
	case ITEM_WEAPON:
	case ITEM_ARMOR:
	case ITEM_PENDANT:
	case ITEM_GLOVE:
		break;

	default:
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("The item type is not suitable for 6-7 attr."));
		return;
	}

	if (item->IsEquipped())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("You cannot add a bonus to an equipped item."));
		return;
	}

	if (item->IsExchanging())
		return;

	const int norm_attr_count = item->GetAttributeCount();
	const int rare_attr_count = item->Get67AttrCount();
	const int attr_set_index = item->Get67AttrIdx();

	if (attr_set_index == -1 || norm_attr_count < 5 || rare_attr_count >= 2)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("This item is not suitable for 6-7 attr."));
		return;
	}

	enum
	{
		SUCCESS_PER_MATERIAL = 100,
		MATERIAL_MAX_COUNT = 1,
		SUPPORT_MAX_COUNT = 5,
	};

	if (p->bMaterialCount > MATERIAL_MAX_COUNT || p->bSupportCount > SUPPORT_MAX_COUNT)
		return;

	DWORD dwMaterialVnum = item->Get67MaterialVnum();
	if (dwMaterialVnum == 0 || p->bMaterialCount < 1 
		|| ch->CountSpecifyItem(dwMaterialVnum) < p->bMaterialCount)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("You don't have enough material item."));
		return;
	}
	
	BYTE success = SUCCESS_PER_MATERIAL * p->bMaterialCount;
	
	if (p->sSupportPos != -1)
	{
		const LPITEM support_item = ch->GetInventoryItem(p->sSupportPos);
		if (support_item)
		{
			if (ch->CountSpecifyItem(support_item->GetVnum()) < p->bSupportCount)
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("You don't have enough support item."));
				return;
			}

			BYTE uSupportSuccess = 0;
			switch (support_item->GetVnum())
			{
			case 72064:
				uSupportSuccess = 5;
				break;
			case 72065:
				uSupportSuccess = 8;
				break;
			case 72066:
				uSupportSuccess = 10;
				break;
			case 72067:
				uSupportSuccess = 30;
				break;
			default:
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("The support item is inappropriate."));
				return;
			}

			success += uSupportSuccess * p->bSupportCount;
			ch->RemoveSpecifyItem(support_item->GetVnum(), p->bSupportCount);
		}
	}

	ch->RemoveSpecifyItem(dwMaterialVnum, p->bMaterialCount);
	
	item->RemoveFromCharacter();
	item->AddToCharacter(ch, TItemPos(ATTR_INVENTORY, 0));

	if (number(1, 100) <= success)
	{
		item->Add67Attr();
		ch->SetQuestFlag("67attr.succes", 1);
	}
	else
	{
		ch->SetQuestFlag("67attr.succes", 0);
	}	
	
	// ch->SetQuestFlag("67attr.time", get_global_time() + 21600);
	ch->SetQuestFlag("67attr.time", get_global_time() + 3);//낚섬橄昑 藤속珂쇌
	
}

void CInputMain::Attr67Close(LPCHARACTER ch, const char* c_pData)
{
	ch->Set67Attr(false);
}
#endif

int CInputMain::Analyze(LPDESC d, BYTE bHeader, const char* c_pData)
{
	LPCHARACTER ch;

	if (!(ch = d->GetCharacter()))
	{
		sys_err("no character on desc");
		d->SetPhase(PHASE_CLOSE);
		return (0);
	}

	int iExtraLen = 0;

	switch (bHeader)
	{
	case HEADER_CG_PONG:
		Pong(d);
		break;

	case HEADER_CG_TIME_SYNC:
		Handshake(d, c_pData);
		break;

	case HEADER_CG_CHAT:
		if ((iExtraLen = Chat(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;

	case HEADER_CG_WHISPER:
		if ((iExtraLen = Whisper(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;

	case HEADER_CG_MOVE:
		Move(ch, c_pData);
		break;

	case HEADER_CG_CHARACTER_POSITION:
		Position(ch, c_pData);
		break;

	case HEADER_CG_ITEM_USE:
		if (!ch->IsObserverMode())
			ItemUse(ch, c_pData);
		break;

#ifdef ENABLE_DROP_DIALOG_EXTENDED_SYSTEM
	case HEADER_CG_ITEM_DELETE:
		if (!ch->IsObserverMode())
			ItemDelete(ch, c_pData);
		break;

	case HEADER_CG_ITEM_SELL:
		if (!ch->IsObserverMode())
			ItemSell(ch, c_pData);
		break;
#else
	case HEADER_CG_ITEM_DROP:
		if (!ch->IsObserverMode())
		{
			ItemDrop(ch, c_pData);
		}
		break;

	case HEADER_CG_ITEM_DROP2:
		if (!ch->IsObserverMode())
			ItemDrop2(ch, c_pData);
		break;
#endif

	case HEADER_CG_ITEM_MOVE:
		if (!ch->IsObserverMode())
			ItemMove(ch, c_pData);
		break;

	case HEADER_CG_ITEM_PICKUP:
		if (!ch->IsObserverMode())
			ItemPickup(ch, c_pData);
		break;

	case HEADER_CG_ITEM_USE_TO_ITEM:
		if (!ch->IsObserverMode())
			ItemToItem(ch, c_pData);
		break;

	case HEADER_CG_ITEM_GIVE:
		if (!ch->IsObserverMode())
			ItemGive(ch, c_pData);
		break;

	case HEADER_CG_EXCHANGE:
		if (!ch->IsObserverMode())
			Exchange(ch, c_pData);
		break;

	case HEADER_CG_ATTACK:
	case HEADER_CG_SHOOT:
		if (!ch->IsObserverMode())
		{
			Attack(ch, bHeader, c_pData);
		}
		break;

	case HEADER_CG_USE_SKILL:
		if (!ch->IsObserverMode())
			UseSkill(ch, c_pData);
		break;

	case HEADER_CG_QUICKSLOT_ADD:
		QuickslotAdd(ch, c_pData);
		break;

	case HEADER_CG_QUICKSLOT_DEL:
		QuickslotDelete(ch, c_pData);
		break;

	case HEADER_CG_QUICKSLOT_SWAP:
		QuickslotSwap(ch, c_pData);
		break;

	case HEADER_CG_SHOP:
		if ((iExtraLen = Shop(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;

	case HEADER_CG_MESSENGER:
		if ((iExtraLen = Messenger(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;

	case HEADER_CG_ON_CLICK:
		OnClick(ch, c_pData);
		break;

	case HEADER_CG_SYNC_POSITION:
		if ((iExtraLen = SyncPosition(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;

	case HEADER_CG_ADD_FLY_TARGETING:
	case HEADER_CG_FLY_TARGETING:
		FlyTarget(ch, c_pData, bHeader);
		break;

	case HEADER_CG_SCRIPT_BUTTON:
		ScriptButton(ch, c_pData);
		break;

		// SCRIPT_SELECT_ITEM
	case HEADER_CG_SCRIPT_SELECT_ITEM:
		ScriptSelectItem(ch, c_pData);
		break;
		// END_OF_SCRIPT_SELECT_ITEM

	case HEADER_CG_SCRIPT_ANSWER:
		ScriptAnswer(ch, c_pData);
		break;

	case HEADER_CG_QUEST_INPUT_STRING:
		QuestInputString(ch, c_pData);
		break;

	case HEADER_CG_QUEST_CONFIRM:
		QuestConfirm(ch, c_pData);
		break;

	case HEADER_CG_TARGET:
		Target(ch, c_pData);
		break;

	case HEADER_CG_WARP:
		Warp(ch, c_pData);
		break;

	case HEADER_CG_SAFEBOX_CHECKIN:
		SafeboxCheckin(ch, c_pData);
		break;

	case HEADER_CG_SAFEBOX_CHECKOUT:
		SafeboxCheckout(ch, c_pData, false);
		break;

	case HEADER_CG_SAFEBOX_ITEM_MOVE:
		SafeboxItemMove(ch, c_pData);
		break;

	case HEADER_CG_MALL_CHECKOUT:
		SafeboxCheckout(ch, c_pData, true);
		break;

	case HEADER_CG_PARTY_INVITE:
		PartyInvite(ch, c_pData);
		break;

	case HEADER_CG_PARTY_REMOVE:
		PartyRemove(ch, c_pData);
		break;

	case HEADER_CG_PARTY_INVITE_ANSWER:
		PartyInviteAnswer(ch, c_pData);
		break;

	case HEADER_CG_PARTY_SET_STATE:
		PartySetState(ch, c_pData);
		break;

	case HEADER_CG_PARTY_USE_SKILL:
		PartyUseSkill(ch, c_pData);
		break;
		
	// case HEADER_CG_PARTY_USE_SKILL:
		// if (!ch || ch == NULL)
		// {
			// break;
		// }
		// if (!c_pData || c_pData == NULL)
		// {
			// break;
		// }
		// PartyUseSkill (ch, c_pData);
		// break;

	case HEADER_CG_PARTY_PARAMETER:
		PartyParameter(ch, c_pData);
		break;

	case HEADER_CG_ANSWER_MAKE_GUILD:
#ifdef ENABLE_NEWGUILDMAKE
		ch->ChatPacket(CHAT_TYPE_INFO, "<%s> 覩와빵똥쉔접곤삔묘콘綠쐐痰", __FUNCTION__);
#else
		AnswerMakeGuild(ch, c_pData);
#endif
		break;

	case HEADER_CG_GUILD:
		if ((iExtraLen = Guild(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;

	case HEADER_CG_FISHING:
		Fishing(ch, c_pData);
		break;

	case HEADER_CG_HACK:
		Hack(ch, c_pData);
		break;

	case HEADER_CG_MYSHOP:
		if ((iExtraLen = MyShop(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	case HEADER_CG_ACCE:
		Acce(ch, c_pData);
		break;
#endif

	case HEADER_CG_REFINE:
		Refine(ch, c_pData);
		break;

	case HEADER_CG_DRAGON_SOUL_REFINE:
	{
		TPacketCGDragonSoulRefine* p = reinterpret_cast <TPacketCGDragonSoulRefine*>((void*)c_pData);
		switch (p->bSubType)
		{
		case DS_SUB_HEADER_CLOSE:
			ch->DragonSoul_RefineWindow_Close();
			break;
		case DS_SUB_HEADER_DO_REFINE_GRADE:
		{
			DSManager::instance().DoRefineGrade(ch, p->ItemGrid);
		}
		break;
		case DS_SUB_HEADER_DO_REFINE_STEP:
		{
			DSManager::instance().DoRefineStep(ch, p->ItemGrid);
		}
		break;
		case DS_SUB_HEADER_DO_REFINE_STRENGTH:
		{
			DSManager::instance().DoRefineStrength(ch, p->ItemGrid);
		}
		break;
		}
	}
	break;

#ifdef ENABLE_TARGET_INFORMATION_SYSTEM
#ifndef ENABLE_TARGET_INFORMATION_RENEWAL
	case HEADER_CG_TARGET_INFO_LOAD:
		TargetInfoLoad(ch, c_pData);
		break;
#endif
#endif
#ifdef ENABLE_CUBE_RENEWAL_WORLDARD
	case HEADER_CG_CUBE_RENEWAL:
		CubeRenewalSend(ch, c_pData);
		break;
#endif
#ifdef ENABLE_OFFLINE_SHOP
	case HEADER_CG_NEW_OFFLINESHOP:
		if ((iExtraLen = OfflineshopPacket(c_pData, ch, m_iBufferLeft)) < 0)
			return -1;
		break;
#endif
#ifdef ENABLE_SWITCHBOT
	case HEADER_CG_SWITCHBOT:
		if ((iExtraLen = Switchbot(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;
#endif

#ifdef ENABLE_IN_GAME_LOG_SYSTEM
	case HEADER_CG_IN_GAME_LOG:
		if ((iExtraLen = InGameLog(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;
#endif

#ifdef ENABLE_ITEM_TRANSACTIONS
	case HEADER_CG_ITEM_TRANSACTIONS:
		if ((iExtraLen = ItemTransactions(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;
#endif

#ifdef ENABLE_PM_ALL_SEND_SYSTEM
	case HEADER_CG_BULK_WHISPER:
		BulkWhisperManager(ch, c_pData);
		break;
#endif
#ifdef ENABLE_DUNGEON_INFO
	case HEADER_CG_DUNGEON_INFO:
		if ((iExtraLen = DungeonInfo(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;
#endif

#ifdef ENABLE_SKILL_COLOR_SYSTEM
	case HEADER_CG_SKILL_COLOR:
		SetSkillColor(ch, c_pData);
		break;
#endif

#ifdef ENABLE_AURA_SYSTEM
	case HEADER_CG_AURA:
		Aura(ch, c_pData);
		break;
#endif
#ifdef ENABLE_NEW_PET_SYSTEM
	case HEADER_CG_NEW_PET:
		if ((iExtraLen = NewPet(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;
#endif

#ifdef ENABLE_EXTENDED_BATTLE_PASS
	case HEADER_CG_EXT_BATTLE_PASS_ACTION:
		if ((iExtraLen = ReciveExtBattlePassActions(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;

	case HEADER_CG_EXT_SEND_BP_PREMIUM:
		if ((iExtraLen = ReciveExtBattlePassPremium(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;
#endif

#ifdef ENABLE_LEADERSHIP_EXTENSION
	case HEADER_CG_REQUEST_PARTY_BONUS:
		if ((iExtraLen = RequestPartyBonus(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;
#endif

#ifdef ENABLE_6TH_7TH_ATTR
	case HEADER_CG_67_ATTR:
		Attr67(ch, c_pData);
		break;
		
	case HEADER_CG_CLOSE_67_ATTR:
		Attr67Close(ch, c_pData);
		break;
#endif
#ifdef ENABLE_MINI_GAME_OKEY_NORMAL
	case HEADER_CG_OKEY_CARD:
		if ((iExtraLen = MiniGameOkeyCard(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;
#endif
#ifdef ENABLE_MINI_GAME_BNW
	case HEADER_CG_MINIGAME_BNW:
		if ((iExtraLen = MinigameBnw(ch, c_pData, m_iBufferLeft))<0)
			return -1;
		break;
#endif
#ifdef ENABLE_MINI_GAME_CATCH_KING
	case HEADER_CG_MINI_GAME_CATCH_KING:
		if ((iExtraLen = CatchKing::Instance().MiniGameCatchKing(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;
#endif
	}
	return (iExtraLen);
}

int CInputDead::Analyze(LPDESC d, BYTE bHeader, const char* c_pData)
{
	LPCHARACTER ch;

	if (!(ch = d->GetCharacter()))
	{
		sys_err("no character on desc");
		return 0;
	}

	int iExtraLen = 0;

	switch (bHeader)
	{
	case HEADER_CG_PONG:
		Pong(d);
		break;

	case HEADER_CG_TIME_SYNC:
		Handshake(d, c_pData);
		break;

	case HEADER_CG_CHAT:
		if ((iExtraLen = Chat(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;

		break;

	case HEADER_CG_WHISPER:
		if ((iExtraLen = Whisper(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;

		break;

	case HEADER_CG_HACK:
		Hack(ch, c_pData);
		break;

	default:
		return (0);
	}

	return (iExtraLen);
}

#ifdef ENABLE_SWITCHBOT
int32_t CInputMain::Switchbot(LPCHARACTER ch, const char* data, size_t uiBytes)
{
	const TPacketCGSwitchbot* p = reinterpret_cast<const TPacketCGSwitchbot*>(data);

	if (uiBytes < sizeof(TPacketCGSwitchbot))
	{
		return -1;
	}

	const char* c_pData = data + sizeof(TPacketCGSwitchbot);
	uiBytes -= sizeof(TPacketCGSwitchbot);

	switch (p->subheader)
	{
	case SUBHEADER_CG_SWITCHBOT_START:
	{
		size_t extraLen = sizeof(TSwitchbotAttributeAlternativeTable) * SWITCHBOT_ALTERNATIVE_COUNT;
		if (uiBytes < extraLen)
		{
			return -1;
		}

		std::vector<TSwitchbotAttributeAlternativeTable> vec_alternatives;

		for (BYTE alternative = 0; alternative < SWITCHBOT_ALTERNATIVE_COUNT; ++alternative)
		{
			const TSwitchbotAttributeAlternativeTable* pAttr = reinterpret_cast<const TSwitchbotAttributeAlternativeTable*>(c_pData);
			c_pData += sizeof(TSwitchbotAttributeAlternativeTable);

			vec_alternatives.emplace_back(*pAttr);
		}

		CSwitchbotManager::Instance().Start(ch->GetPlayerID(), p->slot, vec_alternatives);
		return extraLen;
	}

	case SUBHEADER_CG_SWITCHBOT_STOP:
	{
		CSwitchbotManager::Instance().Stop(ch->GetPlayerID(), p->slot);
		return 0;
	}
	}

	return 0;
}
#endif

#ifdef ENABLE_IN_GAME_LOG_SYSTEM
int CInputMain::InGameLog(LPCHARACTER ch, const char* data, UINT uiBytes)
{
	if (!ch)
	{
		return -1;
	}

	if (uiBytes < sizeof(InGameLog::TPacketCGInGameLog))
	{
		return -1;
	}
	
	InGameLog::TPacketCGInGameLog* p = (InGameLog::TPacketCGInGameLog*)data;
	data += sizeof(InGameLog::TPacketCGInGameLog);
	uiBytes -= sizeof(InGameLog::TPacketCGInGameLog);

	InGameLog::InGameLogManager& rIglMgr = InGameLog::GetManager();

	return rIglMgr.RecvPacketCG(ch,p->subHeader, p->logID);
}
#endif

#ifdef ENABLE_ITEM_TRANSACTIONS
int CInputMain::ItemTransactions(LPCHARACTER ch, const char* data, int BufferLeft)
{
	if (!ch)
		return -1;

	if (!ch->CanWarp(true))
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("CLOSE_WINDOWS_ERROR1"));
		return BufferLeft;
	}

	if (BufferLeft < (int)sizeof(TItemTransactionsCGPacket))
		return -1;

	TItemTransactionsCGPacket* pack = nullptr;
	BufferLeft -= sizeof(TItemTransactionsCGPacket);
	data = Decode(pack, data);

	int extraBuffer = 0;
	std::vector<TItemTransactionsInfo> v_itemPos;
	v_itemPos.reserve(pack->itemCount);
	TItemTransactionsInfo* pInfo = nullptr;

	for (int i = 0; i < pack->itemCount; i++)
	{
		if (!CanDecode(pInfo, BufferLeft))
			return -1;

		data = Decode(pInfo, data, &extraBuffer, &BufferLeft);
		v_itemPos.push_back(*pInfo);
	}

	if (pack->transaction == 0)
	{
		ch->ItemTransactionSell(v_itemPos);
	}
	else
	{
		ch->ItemTransactionDelete(v_itemPos);
	}

	return extraBuffer;
}
#endif

#ifdef ENABLE_PM_ALL_SEND_SYSTEM
void CInputMain::BulkWhisperManager(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGBulkWhisper* f = (TPacketCGBulkWhisper*)c_pData;

	if (!ch)
		return;

	if (ch->GetGMLevel() != GM_IMPLEMENTOR)
		return;

	TPacketGGBulkWhisper p;
	p.bHeader = HEADER_GG_BULK_WHISPER;
	p.lSize = strlen(f->szText) + 1;

	TEMP_BUFFER tmpbuf;
	tmpbuf.write(&p, sizeof(p));
	tmpbuf.write(f->szText, p.lSize);

	SendBulkWhisper(f->szText);
	P2P_MANAGER::instance().Send(tmpbuf.read_peek(), tmpbuf.size());
}
#endif

#ifdef ENABLE_DUNGEON_INFO
int CInputMain::DungeonInfo(LPCHARACTER ch, const char* data, int BufferLeft)
{
	if (!ch)
		return -1;

	if (BufferLeft < (int)sizeof(TPacketDungeonInfoCG))
		return -1;

	TPacketDungeonInfoCG* pack = nullptr;
	BufferLeft -= sizeof(TPacketDungeonInfoCG);
	data = Decode(pack, data);

	int extraBuffer = 0;

	switch (pack->subHeader)
	{
		case SUB_HEADER_CG_TIME_REQUEST:
		{
			ch->SendDungeonInfoTime();
			break;
		}

		case SUB_HEADER_CG_DUNGEON_LOGIN_REQUEST:
		{
			BYTE* dungeonIdx;
			if (!CanDecode(dungeonIdx, BufferLeft))
				return -1;

			data = Decode(dungeonIdx, data, &extraBuffer, &BufferLeft);
			ch->DungeonJoinBegin(*dungeonIdx);
			break;
		}

		case SUB_HEADER_CG_DUNGEON_TIME_RESET:
		{
			BYTE* dungeonIdx;
			if (!CanDecode(dungeonIdx, BufferLeft))
				return -1;

			data = Decode(dungeonIdx, data, &extraBuffer, &BufferLeft);
			break;
		}

		default: break;
	}

	return extraBuffer;
}
#endif


#ifdef ENABLE_SKILL_COLOR_SYSTEM
void CInputMain::SetSkillColor(LPCHARACTER ch, const char* pcData)
{
	// if (!ch)
		// return;
// #ifdef ENABLE_SKILL_COLOR_ITEM_90060
	// LPITEM pRequiredItem = ch->FindSpecifyItem(90060);

	// if (!pRequiredItem || pRequiredItem->GetCount() < 1)
	// {
		// ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Need_skill_color_item90060"));
		// return;
	// }
// #endif
// #ifdef ENABLE_PREMIUM_MEMBER_SYSTEM
	// if (!ch->IsPremium())
	// {
		// ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT ("Not VIP, you cannot modify skill colors"));
		// return;
	// }
// #endif

	TPacketCGSkillColor* p = (TPacketCGSkillColor*)pcData;

	if (p->skill >= ESkillColorLength::MAX_SKILL_COUNT)
		return;

	DWORD data[ESkillColorLength::MAX_SKILL_COUNT + ESkillColorLength::MAX_BUFF_COUNT][ESkillColorLength::MAX_EFFECT_COUNT];
	memcpy(data, ch->GetSkillColor(), sizeof(data));

	data[p->skill][0] = p->col1;
	data[p->skill][1] = p->col2;
	data[p->skill][2] = p->col3;
	data[p->skill][3] = p->col4;
	data[p->skill][4] = p->col5;

	ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Skill color modification successful"));
	ch->SetSkillColor(data[0]);

	TSkillColor db_pack;
	memcpy(db_pack.dwSkillColor, data, sizeof(data));
	db_pack.player_id = ch->GetPlayerID();
	db_clientdesc->DBPacketHeader(HEADER_GD_SKILL_COLOR_SAVE, 0, sizeof(TSkillColor));
	db_clientdesc->Packet(&db_pack, sizeof(TSkillColor));
}
#endif

#ifdef ENABLE_AURA_SYSTEM
void CInputMain::Aura(LPCHARACTER pkChar, const char* c_pData)
{
	quest::PC* pPC = quest::CQuestManager::instance().GetPCForce(pkChar->GetPlayerID());
	if (pPC->IsRunning())
		return;

	TPacketAura* sPacket = (TPacketAura*)c_pData;
	switch (sPacket->subheader)
	{
	case AURA_SUBHEADER_CG_CLOSE:
	{
		pkChar->CloseAura();
	}
	break;
	case AURA_SUBHEADER_CG_ADD:
	{
		pkChar->AddAuraMaterial(sPacket->tPos, sPacket->bPos);
	}
	break;
	case AURA_SUBHEADER_CG_REMOVE:
	{
		pkChar->RemoveAuraMaterial(sPacket->bPos);
	}
	break;
	case AURA_SUBHEADER_CG_REFINE:
	{
		pkChar->RefineAuraMaterials();
	}
	break;
	default:
		break;
	}
}
#endif

#ifdef ENABLE_NEW_PET_SYSTEM
int CInputMain::NewPet(LPCHARACTER ch, const char* data, int BufferLeft)
{
	if (!ch)
		return -1;

	if (BufferLeft < (int)sizeof(TNewPetCGPacket))
		return -1;

	TNewPetCGPacket* pack = nullptr;
	BufferLeft -= sizeof(TNewPetCGPacket);
	data = Decode(pack, data);

	CNewPet* newPet = ch->GetNewPetSystem();
	if (!newPet || !newPet->IsSummon())
		return false;

	int extraBuffer = 0;

	switch (pack->subHeader)
	{
		case SUB_CG_STAR_INCREASE:
		{
			newPet->IncreaseStar();
			break;
		}

		case SUB_CG_TYPE_INCREASE:
		{
			newPet->IncreaseType();
			break;
		}

		case SUB_CG_EVOLUTION_INCREASE:
		{
			newPet->IncreaseEvolution();
			break;
		}

		case SUB_CG_BONUS_CHANGE:
		{
			newPet->BonusChange();
			break;
		}

		case SUB_CG_ACTIVATE_SKIN:
		{
			newPet->ActivateSkin();
			break;
		}

		case SUB_CG_SKIN_CHANGE:
		{
			BYTE* skinIndex;
			if (!CanDecode(skinIndex, BufferLeft))
				return -1;

			data = Decode(skinIndex, data, &extraBuffer, &BufferLeft);
			newPet->SkinChange(*skinIndex);
			break;
		}
	}

	return extraBuffer;
}
#endif

//폘痰固剛溝固
#ifdef ENABLE_LEADERSHIP_EXTENSION
int CInputMain::RequestPartyBonus(LPCHARACTER ch, const char* data, size_t uiBytes)
{
	if (1 == quest::CQuestManager::instance().GetEventFlag("disable_party_bonus"))
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Quick item splitting is currently out of use"));
		return -1;
	}

	if (ch->GetParty())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("To perform this operation, please disband the team first"));
		return 0;
	}

	TPacketCGRequestPartyBonus * p = (TPacketCGRequestPartyBonus *) data;

	if (uiBytes < sizeof(TPacketCGRequestPartyBonus))
		return -1;

	if ((ch->m_partyAddBonusPulse + 30) > get_global_time())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Please wait %d seconds and try again"),(ch->m_partyAddBonusPulse + 30) - get_global_time());
		// ch->NewChatPacket(WAIT_TO_USE_AGAIN, "%d", (ch->m_partyAddBonusPulse + 30) - get_global_time());
		return 0;
	}
	
	if (ch->GetLeadershipSkillLevel() < 20)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Command skills need to reach M1"));
		return 0;
	}
	
	ch->m_partyAddBonusPulse = get_global_time();

	float k = (float) ch->GetSkillPowerByLevel( MIN(SKILL_MAX_LEVEL, ch->GetLeadershipSkillLevel() ) )/ 100.0f;
		
	{
		bool bCompute = false;
		if(ch->GetPoint(POINT_PARTY_ATTACKER_BONUS))
		{
			ch->PointChange(POINT_PARTY_ATTACKER_BONUS, -ch->GetPoint(POINT_PARTY_ATTACKER_BONUS));
			bCompute = true;
		}
		
		if(ch->GetPoint(POINT_PARTY_TANKER_BONUS))
		{
			ch->PointChange(POINT_PARTY_TANKER_BONUS, -ch->GetPoint(POINT_PARTY_TANKER_BONUS));
			bCompute = true;
		}
			
		if(ch->GetPoint(POINT_PARTY_BUFFER_BONUS))
		{
			ch->PointChange(POINT_PARTY_BUFFER_BONUS, -ch->GetPoint(POINT_PARTY_BUFFER_BONUS));
			bCompute = true;
		}
		
		if(ch->GetPoint(POINT_PARTY_SKILL_MASTER_BONUS))
		{
			ch->PointChange(POINT_PARTY_SKILL_MASTER_BONUS, -ch->GetPoint(POINT_PARTY_SKILL_MASTER_BONUS));
			bCompute = true;
		}
	
		if(ch->GetPoint(POINT_PARTY_DEFENDER_BONUS))
		{
			ch->PointChange(POINT_PARTY_DEFENDER_BONUS, -ch->GetPoint(POINT_PARTY_DEFENDER_BONUS));
			bCompute = true;
		}
	
		if(ch->GetPoint(POINT_PARTY_HASTE_BONUS))
		{
			ch->PointChange(POINT_PARTY_HASTE_BONUS, -ch->GetPoint(POINT_PARTY_HASTE_BONUS));
			bCompute = true;
		}
	
		if(bCompute)
		{
			ch->ComputeBattlePoints();
			ch->ComputePoints();
		}
	}
	
	if(!p->bBonusID)
		return 0;
	
	bool bAdd = false;
	
	switch (p->bBonusID)
	{
		case PARTY_ROLE_ATTACKER:
			{
				int iBonus = (int) (10 + 60 * k);

				if (ch->GetPoint(POINT_PARTY_ATTACKER_BONUS) != iBonus)
				{
					bAdd = true;
					ch->PointChange(POINT_PARTY_ATTACKER_BONUS, iBonus - ch->GetPoint(POINT_PARTY_ATTACKER_BONUS));
					ch->ComputePoints();
				}
			}
			break;

		case PARTY_ROLE_TANKER:
			{
				int iBonus = (int) (50 + 1450 * k);

				if (ch->GetPoint(POINT_PARTY_TANKER_BONUS) != iBonus)
				{
					bAdd = true;
					ch->PointChange(POINT_PARTY_TANKER_BONUS, iBonus - ch->GetPoint(POINT_PARTY_TANKER_BONUS));
					ch->ComputePoints();
				}
			}
			break;

		case PARTY_ROLE_BUFFER:
			{
				int iBonus = (int) (5 + 45 * k);

				if (ch->GetPoint(POINT_PARTY_BUFFER_BONUS) != iBonus)
				{
					bAdd = true;
					ch->PointChange(POINT_PARTY_BUFFER_BONUS, iBonus - ch->GetPoint(POINT_PARTY_BUFFER_BONUS));
				}
			}
			break;

		case PARTY_ROLE_SKILL_MASTER:
			{
				int iBonus = (int) (10*k);

				if (ch->GetPoint(POINT_PARTY_SKILL_MASTER_BONUS) != iBonus)
				{
					bAdd = true;
					ch->PointChange(POINT_PARTY_SKILL_MASTER_BONUS, iBonus - ch->GetPoint(POINT_PARTY_SKILL_MASTER_BONUS));
					ch->ComputePoints();
				}
			}
			break;
		case PARTY_ROLE_HASTE:
			{
				int iBonus = (int) (1+5*k);
				if (ch->GetPoint(POINT_PARTY_HASTE_BONUS) != iBonus)
				{
					bAdd = true;
					ch->PointChange(POINT_PARTY_HASTE_BONUS, iBonus - ch->GetPoint(POINT_PARTY_HASTE_BONUS));
					ch->ComputePoints();
				}
			}
			break;
		case PARTY_ROLE_DEFENDER:
			{
				int iBonus = (int) (5+30*k);
				if (ch->GetPoint(POINT_PARTY_DEFENDER_BONUS) != iBonus)
				{
					bAdd = true;
					ch->PointChange(POINT_PARTY_DEFENDER_BONUS, iBonus - ch->GetPoint(POINT_PARTY_DEFENDER_BONUS));
					ch->ComputePoints();
				}
			}
			break;
			
		default:
			break;	
	}
	
	if(bAdd)
	{
		ch->SetQuestFlag("party.bonus_id", p->bBonusID);

		if(ch->FindAffect(AFFECT_PARTY_BONUS))
		{
			ch->RemoveAffect(AFFECT_PARTY_BONUS);
			ch->AddAffect(AFFECT_PARTY_BONUS, p->bBonusID, 0, 0, INFINITE_AFFECT_DURATION, 0, false);
		}
		else
			ch->AddAffect(AFFECT_PARTY_BONUS, p->bBonusID, 0, 0, INFINITE_AFFECT_DURATION, 0, false);
	}
	else
	{
		// No add so remove
		if(ch->FindAffect(AFFECT_PARTY_BONUS))
			ch->RemoveAffect(AFFECT_PARTY_BONUS);
	}
	
	return 0;
}
#endif

#ifdef ENABLE_MINI_GAME_OKEY_NORMAL
size_t GetMiniGameSubPacketLength(const EPacketCGMiniGameSubHeaderOkeyNormal& SubHeader)
{
	switch (SubHeader)
	{
		case SUBHEADER_CG_RUMI_START:
			return sizeof(TSubPacketCGMiniGameCardOpenClose);
		case SUBHEADER_CG_RUMI_EXIT:
			return 0;
		case SUBHEADER_CG_RUMI_DECKCARD_CLICK:
			return 0;
		case SUBHEADER_CG_RUMI_HANDCARD_CLICK:
			return sizeof(TSubPacketCGMiniGameHandCardClick);
		case SUBHEADER_CG_RUMI_FIELDCARD_CLICK:
			return sizeof(TSubPacketCGMiniGameFieldCardClick);
		case SUBHEADER_CG_RUMI_DESTROY:
			return sizeof(TSubPacketCGMiniGameDestroy);
	}

	return 0;
}

int CInputMain::MiniGameOkeyCard(LPCHARACTER ch, const char* data, size_t uiBytes)
{
	if (uiBytes < sizeof(TPacketCGMiniGameOkeyCard))
		return -1;

	const TPacketCGMiniGameOkeyCard* pinfo = reinterpret_cast<const TPacketCGMiniGameOkeyCard*>(data);
	const char* c_pData = data + sizeof(TPacketCGMiniGameOkeyCard);

	uiBytes -= sizeof(TPacketCGMiniGameOkeyCard);

	const EPacketCGMiniGameSubHeaderOkeyNormal SubHeader = static_cast<EPacketCGMiniGameSubHeaderOkeyNormal>(pinfo->bSubHeader);
	const size_t SubPacketLength = GetMiniGameSubPacketLength(SubHeader);
	if (uiBytes < SubPacketLength)
	{
		sys_err("invalid minigame subpacket length (sublen %d size %u buffer %u)", SubPacketLength, sizeof(TPacketCGMiniGameOkeyCard), uiBytes);
		return -1;
	}

	switch (SubHeader)
	{
		case SUBHEADER_CG_RUMI_START:
			{
				const TSubPacketCGMiniGameCardOpenClose* sp = reinterpret_cast<const TSubPacketCGMiniGameCardOpenClose*>(c_pData);
				ch->Cards_open(sp->bSafeMode);
			}
			return SubPacketLength;

		case SUBHEADER_CG_RUMI_EXIT:
			{
				ch->CardsEnd();
			}
			return SubPacketLength;

		case SUBHEADER_CG_RUMI_DESTROY:
			{
				const TSubPacketCGMiniGameDestroy* sp = reinterpret_cast<const TSubPacketCGMiniGameDestroy*>(c_pData);
				ch->CardsDestroy(sp->index);
			}
			return SubPacketLength;

		case SUBHEADER_CG_RUMI_DECKCARD_CLICK:
			{
				ch->Cards_pullout();
			}
			return SubPacketLength;

		case SUBHEADER_CG_RUMI_HANDCARD_CLICK:
			{
				const TSubPacketCGMiniGameHandCardClick* sp = reinterpret_cast<const TSubPacketCGMiniGameHandCardClick*>(c_pData);
				ch->CardsAccept(sp->index);
			}
			return SubPacketLength;

		case SUBHEADER_CG_RUMI_FIELDCARD_CLICK:
			{
				const TSubPacketCGMiniGameFieldCardClick* sp = reinterpret_cast<const TSubPacketCGMiniGameFieldCardClick*>(c_pData);
				ch->CardsRestore(sp->index);
			}
			return SubPacketLength;
	}

	return 0;
}
#endif
#ifdef ENABLE_MINI_GAME_BNW
int CInputMain::MinigameBnw(LPCHARACTER ch, const char* c_pData, size_t uiBytes)
{
	TPacketCGMinigameBnw* p = (TPacketCGMinigameBnw*) c_pData;
	
	if (uiBytes < sizeof(TPacketCGMinigameBnw))
		return -1;

	c_pData += sizeof(TPacketCGMinigameBnw);
	uiBytes -= sizeof(TPacketCGMinigameBnw);

	switch (p->subheader)
	{
		case MINIGAME_BNW_SUBHEADER_CG_START:
			{
				MinigameBnwStart(ch);
			}
			return 0;
		case MINIGAME_BNW_SUBHEADER_CG_SELECTED_CARD:
			{
				if (uiBytes < sizeof(BYTE))
					return -1;

				const BYTE card = *reinterpret_cast<const BYTE*>(c_pData);
				MinigameBnwSelectedCard(ch, card);
			}
			return sizeof(BYTE);

		case MINIGAME_BNW_SUBHEADER_CG_FINISHED:
			{
				MinigameBnwFinished(ch);
			}
			return 0;
	}
	return 0;
}
#endif