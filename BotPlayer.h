#pragma once

#include <memory>
#include <ctime>
#include <unordered_map>
#include <string>
#include <vector>

struct TBotCharacterInfo
{
	std::string name;
	uint8_t job;
	uint8_t level;
	int32_t alignment;
	int32_t mountVnum;

	uint32_t itemWeapon;
	uint32_t itemArmor;
	uint32_t itemHair;
	uint32_t itemNeck;
	uint32_t itemEar;
	uint32_t itemFoots;
	uint32_t itemWrist;
	uint32_t itemShield;
	uint32_t itemHead;
	uint32_t itemAura;
	uint32_t itemAcce;

	TBotCharacterInfo()
		: name(""), job(0), level(0), alignment(0), mountVnum(0),
		itemWeapon(0), itemArmor(0), itemHair(0),
		itemNeck(0), itemEar(0), itemFoots(0),
		itemWrist(0), itemShield(0), itemHead(0),
		itemAura(0), itemAcce(0)
	{}
};

class BotCharacter
{
public:
	BotCharacter() : m_botCharacter(nullptr), m_botCharacterEvent(nullptr) {}
	~BotCharacter() = default;

	void SetBotCharacter(LPCHARACTER botCharacter) { m_botCharacter = botCharacter; }
	LPCHARACTER GetBotCharacter() const { return m_botCharacter; }
	// void SetStartEvent(LPCHARACTER botCharacter);
	void SetStartEvent(LPCHARACTER botCharacter, long startX, long startY);
	void SetStartChatEvent(LPCHARACTER botCharacter);

public:
	LPCHARACTER m_botCharacter;
	LPEVENT m_botCharacterEvent;
	LPEVENT m_botCharacterChatEvent;
};

class CBotCharacterManager : public singleton<CBotCharacterManager>
{
public:
	CBotCharacterManager() = default;
	~CBotCharacterManager() = default;

	void Initialize();
	void InitializeBotNames();		// 从 bot_name.txt 文件中上传机器人名称
	void InitializeChatMessages();	// 从 bot_player_chat.txt 加载聊天消息
	void Reload();					// 重新加载机器人文件。
	void BotSpawn(LPCHARACTER ch, int32_t spawn_count, int8_t empire_id = 0);
	
	// 基于帝国的机器人生成功能
	void BotSpawnShinsoo(LPCHARACTER ch, int32_t spawn_count);  // 盛唐国
	void BotSpawnChunjo(LPCHARACTER ch, int32_t spawn_count);   // 秦皇国
	void BotSpawnJinno(LPCHARACTER ch, int32_t spawn_count);    // 汉武国

	void EquipItem(LPCHARACTER ch);
	void EquipItemAttributes(LPCHARACTER ch);

	void BotFullRemove();
	void BotCharacterRemove(const char* c_szName);

	uint32_t BotCharacterCount() const { return static_cast<uint32_t>(m_botCharacters.size()); }


	void NoticePacket(LPCHARACTER ch, const char* szNotice);
	bool IsBotCharacter(const char* c_szName) const;
	LPCHARACTER GetBotCharacter(const char* c_szName) const;
	
	const std::vector<std::string>& GetChatMessages() const { return m_chatMessages; }

	//机器人名称 -> 语言代码
	// BYTE GetBotLanguage(const char* c_szName) const;
	
private:
	std::unordered_map<std::string, std::unique_ptr<TBotCharacterInfo>> m_mapBotCharacterInfo;
	std::unordered_map<std::string, std::unique_ptr<BotCharacter>> m_botCharacters;
	std::vector<std::string> m_botNames;		// 机器人名称列表
	std::vector<std::string> m_chatMessages;	// 聊天消息列表
	
	// 机器人字符语法（机器人名称 -> 语言代码
	std::unordered_map<std::string, BYTE> m_botLanguages;
public:
	// 轻型存储：机器人武器数量缓存(代替任务标记)
	void SetBotWeaponVnum(const char* name, int vnum) { if (name && *name) m_botWeaponVnum[name] = vnum; }
	int GetBotWeaponVnum(const char* name) const {
		if (!name || !*name) return 0;
		auto it = m_botWeaponVnum.find(name);
		return it != m_botWeaponVnum.end() ? it->second : 0;
	}
	void EraseBotWeaponVnum(const char* name) { if (name && *name) m_botWeaponVnum.erase(name); }

private:
	std::unordered_map<std::string, int> m_botWeaponVnum;
};
