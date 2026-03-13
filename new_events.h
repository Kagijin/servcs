#include "../../common/service.h"

#ifdef __NEW_EVENTS__
	#include "../../common/length.h"
	#include "../../common/item_length.h"
	#include "../../common/tables.h"
	#ifdef __KINGDOMS_WAR__
		namespace KingdomsWar
		{
			extern char const * MSG[];
			extern std::string EVENT_STAT, EVENT_MIN_LV, EVENT_MAX_LV, REWARD_VNUM, REWARD_COUNT, REWARD_B_VNUM, REWARD_B_COUNT;
			extern BYTE IS_UNACCPETABLE_ITEM(DWORD dwVnum);
			
			enum eEventSetup
			{
				MAP_INDEX = 211, //地图索引
				MAP_CHANNEL = 99, //通道
				DEAD_LIMIT = 50, //允许死亡次数
				LIMIT_WIN_KILLS = 200, //各国杀敌数
				MAX_TIME_LIMIT = 60 * 40, //战斗时限
			};
			

			enum eEventFlags
			{
				END,
				START
			};
			
			typedef struct sPartecipants
			{
				int	kill, killed;
				std::string	account_name;
			} tPartecipants;
			
			struct sReward
			{
				DWORD	r_dwItemVnum, r_dwItemCount, b_dwItemVnum, b_dwItemCount;
				DWORD	dwMinLevel, dwMaxLevel;
			};
		}
		
		class CMgrKingdomsWar : public singleton<CMgrKingdomsWar>
		{
			public:
				KingdomsWar::sReward	reward;
			
			public:
				CMgrKingdomsWar();
				virtual	~CMgrKingdomsWar();
				void	SetKill(BYTE bEmpire) {kills[bEmpire - 1] += 1;}
				void	SetKillForce(BYTE bEmpire, int iAmount) {kills[bEmpire - 1] = iAmount;}
				int		GetKillsScore(BYTE bEmpire) const {return kills[bEmpire - 1];}
				bool	Start();
				bool	Stop();
				bool	Config(DWORD dwMinLv, DWORD dwMaxLv, DWORD r_dwItemVnum, DWORD r_dwItemCount, DWORD t_dwItemVnum, DWORD t_dwItemCount);
				void	Register(LPCHARACTER pChar);
				void	Unregister(LPCHARACTER pChar);
				void	DeclareWinner();
				void	OnKill(DWORD dwPlayerID1, BYTE bEmpire1, DWORD dwPlayerID2, BYTE bEmpire2);
			
			private:
				bool	bWinner;
				DWORD	dwTimeRemained;
				int		kills[EMPIRE_MAX_NUM - 1];
				std::map<DWORD, KingdomsWar::tPartecipants>	mPartecipants1, mPartecipants2, mPartecipants3;
			
			protected:
				LPEVENT	e_Limit;
		};
	#endif
#endif
