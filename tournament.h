#ifdef TOURNAMENT_PVP_SYSTEM

#define ENABLE_KILL_COUNTS_FOR_EACH_PLAYER //为每个玩家启用杀戮计数 
// #define ENABLE_EXTRA_LIVES_TOURNAMENT //启用额外生命锦标赛 

#ifdef ENABLE_KILL_COUNTS_FOR_EACH_PLAYER
	#define FLAG_KILL_COUNT	"tournament.kill_count"
#endif

#ifdef ENABLE_EXTRA_LIVES_TOURNAMENT	
	#define FLAG_EXTRA_LIVES	"tournament.is_extra_lives"
	#define FLAG_USED_COUNT_EXTRA_LIVES	"tournament.used_extra_lives"//默认死亡次数
#endif

#define FLAG_OBSERVER	"tournament.is_observer_mode"
enum ETournamentT
{
	TEAM_MEMBERS_A = 1,	//锦标赛 A队标识
	TEAM_MEMBERS_B = 2,	//锦标赛 B队标识

	TOURNAMENT_STATE_OPEN = 1,	//状态：开放报名
	TOURNAMENT_STATE_CLOSE = 2,	//状态：关闭报名
	TOURNAMENT_STATE_FIGHT = 3,	//状态：战斗中
	TOURNAMENT_STATE_FINISHED = 0,//状态：已结束

	TOURNAMENT_DURATING_FIGHT = 30 * 60, //战斗持续时间（30 分钟）
	
	TOURNAMENT_MIN_MEMBERS = 2, // 锦标赛最低参与人数（2 人）
	
	TOURNAMENT_MAX_LIVES  = 10, //玩家默认复活次数（6 次）
	
	TOURNAMENT_NEED_CHANNEL = 99, //锦标赛专用地图通道（99）

	TOURNAMENT_LAST_LIFE	= 1,
	TOURNAMENT_MAP_INDEX = 241, //锦标赛专用地图编号（241）
	TOURNAMENT_ITEM_BOX = 84201,//奖励配置
	
#ifdef ENABLE_EXTRA_LIVES_TOURNAMENT	//是否开启额外道具加生命次数
	TOURNAMENT_EXTRA_LIVES = TOURNAMENT_MAX_LIVES + 1, // 
	TOURNAMENT_ITEM_EXTRA_LIVES = 84202,
	TOURNAMENT_CAN_USED_MAX_EXTRA_LIVES = 10, //额外生命道具最大使用次数（10 次）
#endif

	TOURNAMENT_NO_MEMBERS = 0,
	TOURNAMENT_BLOCK_DUEL = 0,
	TOURNAMENT_BLOCK_PARTY = 1,
	TOURNAMENT_BLOCK_RING_MARRIAGE = 2,
	TOURNAMENT_BLOCK_POLY = 3,
	TOURNAMENT_BLOCK_CHANGE_PKMODE = 4,
	TOURNAMENT_BLOCK_MOUNT = 5,
	TOURNAMENT_BLOCK_HORSE = 6,
	TOURNAMENT_BLOCK_EXIT_OBSERVER_MODE = 7,
	
	TOURNAMENT_PASSER_NOTICE = 10,
};

class CTournamentPvP : public singleton<CTournamentPvP>
{
	private :
		std::vector<DWORD> m_listForbidden;	
		std::map<DWORD, DWORD> m_map_team_a;
		std::map<DWORD, DWORD> m_map_team_b;
		std::map<DWORD, DWORD> m_map_lives;	

		LPEVENT m_pkTournamentUpdateEvent;

	public :
		bool		Initialize();
		void		Destroy();

		void		ClearRegisters();
		bool		IsRegister(LPCHARACTER ch);
		void		Respawn(LPCHARACTER ch);
		void		TeleportMembers(std::map<DWORD, DWORD> m_map_teams, DWORD index);
	
		void		ClearSTDMap();
		bool CanFinish();
		
		void		OnKill(LPCHARACTER pkKiller, LPCHARACTER pkDead);
		void		PushBack(std::vector<DWORD>* m_vecCacheParticipants);
		void		ReadFileItems();
		
		void  		StartTournamentUpdateEvent();
		void		StopTournamentUpdateEvent();

		int StartFight();
		
		int NeedMinParticipants();

		void		GetTeamWinner();
		void		GiveReward(LPCHARACTER ch);
		void		OnDisconnect(LPCHARACTER ch);
		void		OnLogin(LPCHARACTER ch);
		void		SendNoticeLine(const char * format, ...);
		int			GetIndexTeam(DWORD dwPID);
		
#ifdef ENABLE_EXTRA_LIVES_TOURNAMENT
		bool		GetExistExtraLives(LPCHARACTER ch);
		int			GetUsedCountExtraLives(LPCHARACTER ch);
		void		SetUsedCountExtraLives(LPCHARACTER ch, int val);
#endif
		
		void		Warp(LPCHARACTER ch);
		bool		CheckingStart();

		std::string	ConvertTeamToString(DWORD teamIndex);
		
		int			GetAttackMode(int teamIndex);
		void		PrepareAnnouncement();
		
		bool		IsTournamentMap(LPCHARACTER ch, int key);
		
		bool		TransferByCategory();
		
		bool		file_is_empty(std::ifstream& file);
		int			GetStatus();
		void		SetStatus(int iValue);

#ifdef ENABLE_KILL_COUNTS_FOR_EACH_PLAYER
		void		InsertPlayerKillLogs(LPCHARACTER ch);
#endif
		int			GetMyLives(DWORD dwPID) { return m_map_lives[dwPID]; }
		
		void		DestroyAll();
		
		bool		CanUseItem(LPCHARACTER ch, LPITEM item);

		int			GetParticipants();

		DWORD		GetMembersTeamA() { return m_map_team_a.size(); }
		DWORD		GetMembersTeamB() { return m_map_team_b.size(); }

		bool		IsLimitedItem(LPCHARACTER ch, DWORD dwVnum);
		
		void		AppendLives(LPCHARACTER ch);
		bool		RemoveLives(LPCHARACTER ch);
		void		Register(DWORD dwPID);
};
#endif