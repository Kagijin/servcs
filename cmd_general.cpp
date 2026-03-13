#include "stdafx.h"
#ifdef __FreeBSD__
#include <md5.h>
#else
#include "../../libs/libthecore/include/xmd5.h"
#endif

#include "utils.h"
#include "config.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "motion.h"
#include "packet.h"
#include "affect.h"
#include "pvp.h"
#include "start_position.h"
#include "party.h"
#include "guild_manager.h"
#include "p2p.h"
#include "dungeon.h"
#include "messenger_manager.h"
#include "war_map.h"
#include "questmanager.h"
#include "item_manager.h"
#include "mob_manager.h"
#include "dev_log.h"
#include "item.h"
#include "arena.h"
#include "buffer_manager.h"
#include "unique_item.h"

#include "../../common/VnumHelper.h"
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
#include "MountSystem.h"
#endif
#ifdef ENABLE_PET_COSTUME_SYSTEM
#include "PetSystem.h"
#endif
#ifdef ENABLE_GLOBAL_RANKING
#include "statistics_rank.h"
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

#ifdef ENABLE_GOLDTIGER_BETTING
#include <string>
#include <algorithm>
#include <cctype>
// input_main.cpp 中使用的函数
extern void interpret_command(LPCHARACTER ch, const char* argument, size_t len);

namespace
{
	struct SQuizQA { const char* q; const char* a; };

	// 问题/答案在源文件中（您提供的那个源文件）。
	static const SQuizQA s_quizPool[] =
	{
		{ "谜语：像个蛋，不是蛋，说它圆，不太圆，说它没有它又有。（打一数字）", "0" },
		{ "谜语：一本书，天天看，看了一篇撕一篇。一年到头多少天，小书撕下多少篇？（打一物）", "日历" },
		{ "谜语：510 （猜数字成语）", "一五一十" },
		{ "谜语：小小花儿爬篱笆，张开嘴巴不说话，红紫白蓝样样有，个个都像小喇叭。（打一植物）", "牵牛花" },
		{ "谜语：一个小姑娘，生在水中央，身穿粉红衫，坐在绿船上。打一植物", "荷花" },
		{ "谜语：身穿绿衣裳，肚里水汪汪，生的子儿多，个个黑脸膛。打一水果", "西瓜" },
		{ "谜语：有个妈妈真奇怪，肚上有个皮口袋，不放萝卜不放菜，里边放个小乖乖。打一动物", "袋鼠" },
		{ "谜语：一物生得真奇怪，腰里长出胡子来，拔掉胡子剥开看，露出牙齿一排排。(打一植物)", "玉米" },
		{ "谜语：一个圆桶桶，一头开了洞。中秋佳节时常见，小小蜡烛在当中。", "灯笼" },
		{ "谜语：小小姑娘黑又黑，秋天走了春天回，带着一把小剪刀，半天空里飞呀飞。（打一动物）", "燕子" },
		{ "谜语：四个人搬木头，猜一字", "杰" },
		{ "谜语：一加一，打一字", "王" },
		{ "谜语：小姑娘，夜纳凉。带灯笼，闪闪亮。（打一动物名）", "萤火虫" },
		{ "谜语：耳朵长，尾巴短。只吃菜，不吃饭。（打一动物名）", "兔子" },
		{ "谜语：天热爬上树梢，总爱大喊大叫，明明啥也不懂，偏说知道知道。（打一动物）", "知了" },
		{ "谜语：一只狗，四个口，猜一字", "器" },
		{ "谜语：个儿不算大，帮着人看家，身子用铁打，辫子门上挂（打一用具）", "锁" },
		{ "谜语：兄弟几个人，各进一道门，那个进错了，看了笑死人。（打一物）", "扣子" },
		{ "谜语：一根小棍儿，顶个圆粒儿，小孩儿玩它，容易出事儿。（打一日常用品）", "火柴" },
		{ "谜语：身体细长，兄弟成双，光爱吃菜，不爱喝汤。（打一日常用具）", "筷子" },
		{ "题目：《倚天2》共有几个职业？", "八个" },
		{ "题目：帮派基地中，精炼炉可用于冶炼什么？", "宝石" },
		{ "题目：游戏中恢复生命需使用哪种道具？", "血瓶" },
		{ "题目：游戏中动态格斗依赖哪种3D引擎？", "SDVIRS" },
		{ "日常猜谜：小小身子圆，肚里藏火焰，点燃冒青烟，做饭离不了。", "煤气灶" },
		{ "日常猜谜：有柄没有盖，有嘴不会叫，倒进水来喝，口渴离不了。", "水壶" },
		{ "日常猜谜：身穿银白衣，肚里装寒气，夏天放水果，保鲜又凉意。", "冰箱"},
		{ "日常猜谜：小小四方块，能装书和袋，出门背身上，轻便又实在。", "书包" },
		{ "日常猜谜：有腿不会走，有面不会吼，吃饭放上面，碗筷都能有。", "桌子" },
		{ "日常猜谜：弯弯两只脚，天天身上套，走路不用跑，保护小赤脚。", "鞋子" },
		{ "日常猜谜：薄薄一张膜，贴在手机上，防刮又防摔，保护小屏幕。", "钢化膜" },
		{ "日常猜谜：小小玻璃瓶，装着油和精，炒菜滴几滴，香味满厨房。", "香油" },
		{ "日常猜谜：身子细又长，头上尖又亮，裁纸又剪纸，本领真正强。", "剪刀" },
		{ "日常猜谜：方方一个框，里面亮堂堂，洗脸放东西，整洁又大方。", "洗脸盆" },
		{ "日常猜谜：小小一根绳，天天系在颈，上面挂钥匙，丢了可不行。", "钥匙绳" },
		{ "日常猜谜：浑身毛茸茸，睡觉抱怀中，冬天暖烘烘，孤单不怕冷。", "玩偶" },
		{ "日常猜谜：小小一个盘，能装菜和饭，吃饭离不了，干净又好看。", "盘子" },
		{ "日常猜谜：有刃没有柄，切菜又切肉，用时要小心，别伤手和指。", "菜刀" },
		{ "日常猜谜：小小塑料盒，能装汤和饭，上班带身上，加热就能吃。", "饭盒" },
		{ "日常猜谜：身子圆又滑，常在水里耍，洗澡用得上，搓泥全靠它。", "沐浴球" },
		{ "日常猜谜：小小一张卡，上面印着画，刷卡能消费，出门不用带钞。", "银行卡" },
		{ "日常猜谜：有门又有窗，能关又能敞，挡风又挡雨，保护我的房。", "窗户" },
		{ "日常猜谜：小小一盏灯，挂在天花板，夜里开起来，照亮整个间。", "吊灯" },
		{ "日常猜谜：身子扁又平，能装纸和瓶，出门带身上，杂物全装清。", "手提袋" },
		{ "日常猜谜：小小一支管，里面装着液，刷牙挤一点，洁齿又护龈。", "牙膏" },
		{ "日常猜谜：方方一个凳，能坐不能动，吃饭和休息，天天都要用。", "凳子" },
		{ "日常猜谜：小小一个钟，挂在墙壁中，滴答滴答响，提醒我时钟。", "挂钟" },
		{ "日常猜谜：身穿彩色衣，肚里藏着笔，画画又写字，孩子都喜欢。", "彩笔" },
		{ "日常猜谜：小小一个包，装着化妆品，出门补个妆，美丽又大方。", "化妆包" },
		{ "日常猜谜：身子细又软，能弯又能卷，扎头发用它，好看又方便。", "皮筋" },
		{ "日常猜谜：小小一个刷，刷毛密密麻，刷碗又刷盘，干净顶呱呱。", "洗碗刷" },
		{ "日常猜谜：有面又有底，能装水和米，做饭和煮汤，天天都要你。", "锅" },
		{ "日常猜谜：小小一个瓶，装着花露水，夏天喷一喷，蚊子远离我。", "花露水" },
		{ "日常猜谜：身子方又正，里面藏着镜，照脸又照影，整理仪容行。", "镜子" },
		{ "日常猜谜：小小一根针，穿着一根线，缝补衣和衫，本领大无边。", "缝衣针" },
		{ "日常猜谜：薄薄一块皂，能洗衫和袍，沾水搓一搓，泡沫满屋飘。", "肥皂" },
		{ "日常猜谜：小小一个盒，装着指甲刀，剪甲又修边，干净又好看。", "指甲盒" },
		{ "日常猜谜：身子长又直，站在墙角里，能挂衣和帽，节省好空间。", "衣架" },
		{ "日常猜谜：小小一个杯，能装茶和水，渴了喝一口，清爽又暖胃。", "水杯" },
		{ "日常猜谜：身穿黑衣裳，肚里装着光，按下小按钮，黑夜变明亮。", "台灯" },
		{ "日常猜谜：小小一个夹，能夹纸和画，整齐又牢固，不易被吹刮。", "夹子" },
		{ "日常猜谜：身子圆又硬，表面光又亮，放在桌子上，能压纸和账。", "镇纸" },
		{ "日常猜谜：小小一个勺，能装汤和料，吃饭和喝汤，天天都需要。", "勺子" },
		{ "日常猜谜：有盖又有底，能装糖和米，防潮又防虫，干净又整齐。", "米桶" },
		{ "日常猜谜：小小一个扇，能扇风和凉，夏天握在手，解暑又清爽。", "扇子" },
		{ "日常猜谜：身子软又轻，能铺又能拎，出门垫屁股，干净又舒心。", "坐垫" },
		{ "日常猜谜：小小一个笔，能画又能记，不用装墨水，写错能擦去。", "白板笔" },
		{ "日常猜谜：身穿透明衣，肚里装着蜜，喝上一小口，甜到心眼里。", "蜂蜜" },
		{ "日常猜谜：小小一个网，能装菜和粮，买菜带身上，方便又环保。", "菜网袋" },
		{ "日常猜谜：身子细又长，能吹又能唱，孩子吹一吹，声音响当当。", "口哨" },
		{ "日常猜谜：小小一个垫，铺在鞋子里，吸汗又透气，舒服又护脚。", "鞋垫" },
		{ "日常猜谜：方方一个本，能贴又能写，记录小秘密，留住好回忆。", "笔记本" },
		{ "日常猜谜：小小一个器，能削果和皮，用时转一转，果皮全脱离。", "削皮刀" },
		{ "日常猜谜：小小两片瓦，左右不分家，喉咙擦一擦，清爽祛烟味。", "口香糖"},
		{ "日常猜谜：身子细又短，头上带个碗，下雨淋不着，遮阳顶呱呱。", "雨伞" },
		{ "日常猜谜：方方一块板，面上有格线，写字又演算，学习离不了。", "练习本" },
		{ "日常猜谜：小小金属圈，套在手指间，见证爱与缘，美观又纪念。", "戒指" },
		{ "日常猜谜：弯弯像月牙，齿齿挨挨扎，吃饭先动口，嚼碎五谷粮。", "牙齿" },
		{ "日常猜谜：小小一个筒，装着纸层层，擦汗又擦手，脏了扔纸篓。", "抽纸" },
		{ "日常猜谜：身子滑又亮，肚里藏墨香，写字先蘸水，字迹清又朗。", "毛笔" },
		{ "日常猜谜：方方一块布，吸水本领强，洗脸擦手脚，柔软又舒爽。", "毛巾" },
		{ "日常猜谜：小小一个轮，推着满街跑，装菜又装米，买菜离不了。", "购物车" },
		{ "日常猜谜：细细一根杆，头上绑个网，打球挥一挥，球儿跑远方。", "羽毛球拍" },
		{ "日常猜谜：小小玻璃罐，装着盐和糖，炒菜撒一点，味道喷喷香。", "调料罐" },
		{ "日常猜谜：身子扁又宽，能把电视看，按按小按钮，节目随你选。", "遥控器" },
		{ "日常猜谜：小小一个刷，刷毛软又滑，梳头顺发丝，发型美如画。", "梳子" },
		{ "日常猜谜：方方一个箱，里面藏衣裳，冬天装棉袄，夏天放短装。", "衣柜" },
		{ "日常猜谜：小小一个盘，能把热气散，饭菜放上面，凉得快又欢。", "菜盘" },
		{ "日常猜谜：细细一根线，通电亮闪闪，夜里走黑路，不怕看不见。", "手电筒" },
		{ "日常猜谜：小小一个皂，洗澡离不了，搓出泡泡多，浑身清爽了。", "沐浴露" },
		{ "日常猜谜：身子圆又胖，肚里装着浆，写字涂一涂，错字全盖掉。", "修正液" },
		{ "日常猜谜：小小一个架，能把杯子架，桌上摆一排，整齐又不洒。", "杯架" },
		{ "日常猜谜：弯弯一个钩，能把重物勾，干活离不了，省力又顺手。", "钩子" },
		{ "日常猜谜：小小一本书，里面藏知识，翻页看一看，学问涨一点。", "课外书" },
		{ "日常猜谜：方方一个垫，铺在桌子面，防刮又防烫，桌面保新鲜。", "桌垫" },
		{ "日常猜谜：小小一个瓶，装着洗发精，洗头搓一搓，头发香喷喷。", "洗发水" },
		{ "日常猜谜：身子细又硬，能把木头钉，敲敲小脑袋，钻进木头里。", "钉子" },
		{ "日常猜谜：小小一个锤，敲钉不怕累，木头钉牢固，干活不怕费。", "锤子" },
		{ "日常猜谜：方方一个盒，里面装笔墨，钢笔和橡皮，学习全装着。", "文具盒" },
		{ "日常猜谜：小小一个筛，眼儿密密排，淘米又筛面，杂物全筛开。", "筛子" },
		{ "日常猜谜：身子像小船，漂在水里面，划船用双桨，湖面上游玩。", "小船" },
		{ "日常猜谜：小小一个瓢，能把水来舀，浇花又舀米，用处真不少。", "水瓢" },
		{ "日常猜谜：细细一根绳，能把东西捆，绑箱又绑包，牢固不松身。", "绳子" },
		{ "日常猜谜：小小一个镜，揣在口袋中，出门照一照，仪容更整齐。", "小镜子" },
		{ "日常猜谜：方方一个板，能把衣服展，烫衣来回推，褶皱全不见。", "熨衣板" },
		{ "日常猜谜：小小一个斗，能把米来量，做饭称分量，不多也不少。", "量斗" },
		{ "日常猜谜：身子像云朵，柔软又蓬松，擦桌又擦窗，灰尘全扫光。", "抹布" },
		{ "日常猜谜：小小一个夹，能把衣服夹，晾衣挂阳台，不怕风吹落。", "衣夹" },
		{ "日常猜谜：方方一个柜，里面藏水杯，茶杯和茶壶，摆放整整齐。", "茶柜" },
		{ "日常猜谜：小小一个炉，能把火烧熟，煮茶又煮水，方便又快速。", "烧水壶" },
		{ "日常猜谜：身子细又长，能把缝来量，画画描线条，笔直又漂亮。", "直尺" },
		{ "日常猜谜：小小一个圆，能把圈来画，转一转一圈，圆圈圆又圆。", "圆规" },
		{ "日常猜谜：方方一个盘，能把饺子装，过年吃饺子，团圆喜洋洋。", "饺子盘" },
		{ "日常猜谜：小小一个铲，炒菜离不了，翻菜又盛菜，顺手又灵巧。", "锅铲" },
		{ "日常猜谜：身子像剪刀，能把指甲剪，指甲修短短，干净又卫生。", "指甲刀" },
		{ "日常猜谜：小小一个刷，能把鞋子刷，鞋子刷干净，走路笑哈哈。", "鞋刷" },
		{ "日常猜谜：方方一个盒，能把首饰装，耳环和项链，整齐不慌张。", "首饰盒" },
		{ "日常猜谜：小小一个瓶，装着风油精，头晕擦一点，清凉又清醒。", "风油精" },
		{ "日常猜谜：身子细又尖，能把字来点，按按小按钮，字迹印纸面。", "圆珠笔" },
		{ "日常猜谜：小小一个袋，能把垃圾装，脏物全装里，环境更漂亮。", "垃圾袋" },
		{ "日常猜谜：方方一个框，能把照片装，摆在桌上面，回忆心中藏。", "相框" },
		{ "日常猜谜：小小一个扇，挂在墙上面，通电转一转，凉风满屋传。", "电风扇" },
		{ "日常猜谜：身子像宝塔，节节往上爬，开水泡一泡，茶香飘万家。", "茶叶" },
		{ "日常猜谜：小小一个勺，能把药来舀，吃药按分量，治病效果好。", "药勺" },
		{ "日常猜谜：方方一个垫，铺在椅子面，久坐不腰疼，柔软又舒坦。", "椅垫" },
		{ "日常猜谜：小小一个笼，能把包子蒸，蒸出香喷喷，早餐吃不停。", "蒸笼" },
		{ "日常猜谜：身子滑又凉，能把脸来霜，补水又保湿，皮肤水汪汪。", "面霜" },
		{ "日常猜谜：小小一个刷，能把牙齿刷，天天刷一刷，牙齿白花花。", "牙刷" },
		{ "日常猜谜：方方一个盒，能把纸巾装，桌面摆一个，用着真方便。", "纸巾盒" },
		{ "日常猜谜：小小一个塔，层层都是纱，洗脸擦一擦，吸水顶呱呱。", "洗脸巾" },
		{ "日常猜谜：身子细又长，能把面来擀，擀出薄面皮，包饺子真香。", "擀面杖" },
		{ "日常猜谜：小小一个盆，能把衣服洗，搓搓又揉揉，衣服干净净。", "洗衣盆" },
		{ "谜题：马踏春归。请在聊天栏输入 ", "过年" },
		{ "谜题：金马迎祥。请在聊天栏输入 ", "福到" },
		{ "谜题：红纸书吉。请在聊天栏输入 ", "春联" },
		{ "谜题：竹爆辞旧。请在聊天栏输入 ", "鞭炮" },
		{ "谜题：门贴红符。请在聊天栏输入 ", "福字" },
		{ "谜题：马跃新岁。请在聊天栏输入 ", "吉祥" },
		{ "谜题：庭前悬彩。请在聊天栏输入 ", "灯笼" },
		{ "谜题：拜年赠封。请在聊天栏输入 ", "红包" },
		{ "谜题：除夕煮圆。请在聊天栏输入 ", "饺子" },
		{ "谜题：剪纸贴窗。请在聊天栏输入 ", "窗花" },
		{ "谜题：马开鸿运。请在聊天栏输入 ", "开运" },
		{ "谜题：焰耀夜空。请在聊天栏输入 ", "烟花" },
		{ "谜题：迎神接财。请在聊天栏输入 ", "财神" },
		{ "谜题：瑞马护宅。请在聊天栏输入 ", "平安" },
		{ "谜题：新年焕裳。请在聊天栏输入 ", "新衣" },
		{ "谜题：福满厅堂。请在聊天栏输入 ", "满堂" },
		{ "谜题：马行顺途。请在聊天栏输入 ", "顺意" },
		{ "谜题：元宵猜趣。请在聊天栏输入 ", "闹春" },
		{ "谜题：岁启丙午。请在聊天栏输入 ", "马年" },
		{ "谜题：登门贺岁。请在聊天栏输入 ", "拜年" },
		{ "谜题：金马纳福。请在聊天栏输入 ", "纳祥" },
		{ "谜题：阖家围宴。请在聊天栏输入 ", "团圆" },
		{ "谜题：马踏祥云。请在聊天栏输入 ", "如意" },
		{ "谜题：福字倒贴。请在聊天栏输入 ", "福到" },
		{ "谜题：逛园赏灯。请在聊天栏输入 ", "庙会" },
		{ "谜题：马报佳音。请在聊天栏输入 ", "喜信" },
		{ "谜题：新桃换符。请在聊天栏输入 ", "迎春" },
		{ "谜题：压岁赐钱。请在聊天栏输入 ", "红包" },
		{ "谜题：马驰前程。请在聊天栏输入 ", "高升" },
		{ "谜题：除夕守更。请在聊天栏输入 ", "守岁" },
		{ "谜题：糖瓜祭灶。请在聊天栏输入 ", "小年" },
		{ "谜题：瑞马临门。请在聊天栏输入 ", "纳福" },
		{ "谜题：春满人间。请在聊天栏输入 ", "新春" },
		{ "谜题：马年大吉。请在聊天栏输入 ", "大利" },
		{ "谜题：庭前结彩。请在聊天栏输入 ", "挂彩" },
		{ "谜题：玉碗盛甜。请在聊天栏输入 ", "元宵" },
		{ "谜题：马腾盛世。请在聊天栏输入 ", "太平" },
		{ "谜题：开门迎喜。请在聊天栏输入 ", "喜至" },
		{ "谜题：红烛贺春。请在聊天栏输入 ", "吉庆" },
		{ "谜题：马伴春来。请在聊天栏输入 ", "春到" },
		{ "谜题：贴画迎年。请在聊天栏输入 ", "年画" },
		{ "谜题：壶煮甜圆。请在聊天栏输入 ", "汤圆" },
		{ "谜题：金马送福。请在聊天栏输入 ", "贺岁" },
		{ "谜题：走亲访友。请在聊天栏输入 ", "串门" },
		{ "谜题：马跃年丰。请在聊天栏输入 ", "丰收" },
		{ "谜题：福运亨通。请在聊天栏输入 ", "顺达" },
		{ "谜题：除夕燃灯。请在聊天栏输入 ", "守岁" },
		{ "谜题：瑞气盈门。请在聊天栏输入 ", "迎春" },
		{ "谜题：马踏吉途。请在聊天栏输入 ", "好运" },
		{ "谜题：阖家安康。请在聊天栏输入 ", "平安" },
		{ "题目：春节爆竹禁忌放哪里？", "阳台"},
		{ "题目：春节鞭炮离啥远点。", "易燃物" },
		{ "题目：烟花剩芯咋处理。", "浇冷水" },
		{ "题目：电暖能盖被吗。", "不能" },
		{ "题目：离家断啥源。", "电源" },
		{ "题目：春节燃气漏先关啥。", "阀门" },
		{ "题目：春节煮饺别忘啥。", "看火" },
		{ "题目：春节楼道忌堆啥。", "杂物" },
		{ "题目：春节红包放哪安全。", "贴身" },
		{ "题目：春节出门锁啥门。", "入户门" },
		{ "题目：春节酒驾能开车吗。", "不能" },
		{ "题目：春节逛庙会看啥。", "人流" },
		{ "题目：插座忌啥插。", "超负荷" },
		{ "题目：炭火放哪烧。", "室外" },
		{ "题目：剩菜咋保存。", "冷藏" },
		{ "题目：春节放鞭看啥天。", "大风天" },
		{ "题目：春节儿童燃炮要啥。", "陪同" },
		{ "题目：电器坏了找啥。", "专业人" },
		{ "题目：燃气漏别开啥。", "电灯" },
		{ "题目：走亲忌啥行。", "横穿路" },
		{ "题目：车窗离车前关啥。", "车窗" },
		{ "题目：年货别堆哪。", "灶台" },
		{ "题目：春节烟花能拿手里放吗。", "不能" },
		{ "题目：热水袋能充电过夜吗。", "不能" },
		{ "题目：楼道门能锁吗。", "不能" },
		{ "题目：吃酒别碰啥。", "明火" },
		{ "题目：电梯困人咋做。", "按警铃" },
		{ "题目：陌生链接点吗。", "不点" },
		{ "题目：燃气漏要开啥。", "窗户" },
		{ "题目：春节放烟花看啥标。", "合格证" },
		{ "题目：电动车能上楼充吗。", "不能" },
		{ "题目：剩菜吃前要啥。", "加热" },
		{ "题目：春节出门关啥阀。", "燃气阀" },
		{ "题目：春节玩雪别碰啥。", "电线" },
		{ "题目：春节红包别露啥。", "现金" },
		{ "题目：春节燃炮用啥点。", "香头" },
		{ "题目：春节浴霸能长时间开吗。", "不能" },
		{ "题目：春节陌生敲门开吗。", "不开" },
		{ "题目：春节开车别啥行。", "超速" },
		{ "题目：春节烟花渣咋处理。", "及时清" },
		{ "题目：春节饮水机忘关会啥。", "干烧" },
		{ "题目：春节逛市别丢啥。", "孩子" },
		{ "题目：春节电池别扔哪。", "火旁" },
		{ "题目：春节火锅离啥远。", "桌布" },
		{ "题目：春节停车别停哪。", "消防道" },
		{ "题目：春节冻肉别用啥解。", "热水" },
		{ "题目：春节鞭炮能放车内吗。", "不能" },
		{ "题目：春节洗手后碰啥擦干。", "开关" },
		{ "题目：春节出行备啥。", "口罩" },
		{ "题目：玩游戏忘啥。", "吃饭"},
		{ "题目：游戏瘾犯了想啥。", "开黑" },
		{ "题目：玩游戏熬到啥。", "通宵" },
		{ "题目：打游戏顾不上啥。", "工作" },
		{ "题目：玩游戏不离啥。", "手机" },
		{ "题目：打游戏废啥。", "时间" },
		{ "题目：玩游戏忽略啥。", "家人" },
		{ "题目：打游戏久了伤啥。", "眼睛" },
		{ "题目：玩游戏不愿啥。", "出门" },
		{ "题目：打游戏会啥坐。", "久坐" },
		{ "题目：玩游戏忘接啥。", "电话" },
		{ "题目：打游戏缺啥觉。", "睡眠" },
		{ "题目：玩游戏推啥约。", "饭局" },
		{ "题目：打游戏迷啥局。", "战局" },
		{ "题目：玩游戏常啥时。", "深夜" },
		{ "题目：打游戏乱啥费。", "钱财" },
		{ "题目：玩游戏走神啥。", "上课" },
		{ "题目：打游戏不啥洗。", "洗脸" },
		{ "题目：玩游戏盼啥假。", "周末" },
		{ "题目：打游戏抢啥位。", "C 位" },
		{ "题目：玩游戏啥不动。", "久坐" },
		{ "题目：打游戏忘啥事。", "家务" },
		{ "题目：玩游戏靠啥充。", "氪金" },
		{ "题目：打游戏失啥神。", "精神" },
		{ "题目：玩游戏拒啥聊。", "闲聊" },
		{ "猜人名：火眼金睛，大闹天宫是谁。", "孙悟空"},
		{ "猜人名：嬛嬛娘娘，孙俪饰演是谁。", "甄嬛" },
		{ "猜人名：江左梅郎，琅琊榜首是谁。", "梅长苏" },
		{ "猜人名：功夫之王，龙叔本名是谁。", "成龙" },
		{ "猜人名：千年蛇妖，水漫金山是谁。", "白素贞" },
		{ "猜人名：无厘头王，大话西游是谁。", "周星驰" },
		{ "猜人名：草船借箭，三国智圣是谁。", "诸葛亮" },
		{ "猜人名：花千骨主，颖宝本名是谁。", "赵丽颖" },
		{ "猜人名：盗墓小哥，沉默寡言是谁。", "张起灵" },
		{ "猜人名：仙剑灵儿，刘亦菲饰是谁。", "赵灵儿" },
		{ "猜人名：战狼冷锋，铁血硬汉是谁。", "吴京" },
		{ "猜人名：黛玉葬花，红楼佳人是谁。", "林黛玉" },
		{ "猜人名：晴川扮演者，杨幂饰演是谁。", "洛晴川" },
		{ "猜人名：陈情令羡，云梦少年是谁。", "魏无羡" },
		{ "猜人名：脚踏风火，哪吒闹海是谁。", "哪吒" },
		{ "猜人名：泰囧主演，傻根本名是谁。", "王宝强" },
		{ "猜人名：还珠格格，小燕子是哪位。", "赵薇" },
		{ "猜人名：功夫熊猫，阿宝配音是谁。", "黄渤" },
		{ "猜人名：漫威铁男，斯塔克是谁。", "钢铁侠" },
		{ "猜人名：李焕英女儿，贾玲饰演是谁。", "贾晓玲" },
		{ "猜人名：三生三世，白浅上神是谁。", "白浅" },
		{ "猜人名：唐探秦风，昊然饰演是谁。", "秦风" },
		{ "猜人名：景阳冈打虎，水浒行者是谁。", "武松" },
		{ "猜人名：蓝氏含光，雅正端方是谁。", "蓝忘机" },
		{ "猜人名：葫芦娃兄，大娃力大是谁。", "大娃" },
		{ "猜人名：酷盖弟弟，蓝忘机饰是谁。", "王一博" },
		{ "猜人名：甄嬛传帝，雍正皇帝是谁。", "胤禛" },
		{ "猜人名：天蓬元帅，错投猪胎是谁。", "猪八戒" },
		{ "猜人名：三生凤九，新疆女星是谁。", "迪丽热巴" },
		{ "猜人名：过五关斩六将，红脸武圣是谁。", "关羽" },
		{ "猜人名：流浪地球，刘培强饰是谁。", "刘培强" },
		{ "猜人名：四大名捕，冷面无情是谁。", "无情" },
		{ "猜人名：歌神称号，吻别金曲是谁。", "张学友" },
		{ "猜人名：楚乔传主，巾帼英雄是谁。", "楚乔" },
		{ "猜人名：丽颖搭档，宇文玥饰是谁。", "林更新" },
		{ "猜人名：衔玉而生，红楼公子是谁。", "贾宝玉" },
		{ "猜人名：庆余年主，张若昀饰是谁。", "范闲" },
		{ "猜人名：天后称号，红豆金曲是谁。", "王菲" },
		{ "猜人名：亮剑团长，铁血硬汉是谁。", "李云龙" },
		{ "猜人名：漫威美队，盾牌在手是谁。", "史蒂夫" },
		{ "猜人名：容嬷嬷饰演，经典配角是谁。", "李明启" },
		{ "猜人名：还珠格格，五阿哥永琪是谁。", "永琪" },
		{ "猜人名：延禧攻略，魏璎珞饰是谁。", "魏璎珞" },
		{ "猜人名：封神榜中，二郎神君是谁。", "杨戬" },
		{ "猜人名：西游唐僧，金蝉子转世是谁。", "玄奘" },
		{ "猜人名：夏洛特烦恼，沈腾饰演是谁。", "夏洛" },
		{ "猜人名：白蛇传中，许仙官人是谁。", "叶童" },
		{ "猜人名：琅琊榜中，靖王殿下是谁。", "萧景琰" },
		{ "猜人名：伪装者明台，胡歌饰演是谁。", "明台" },
		{ "题目：悟空出世的仙石，坐落何处。", "花果山"},
		{ "题目：悟空拜师学艺，祖师赐名叫什么。", "孙悟空" },
		{ "题目：悟空的兵器，东海龙宫所借什么？。", "金箍棒" },
		{ "题目：悟空闹天宫后，被压在哪座山。", "五行山" },
		{ "题目：唐僧为悟空取的混名。", "行者" },
		{ "题目：唐僧西天取经，出发地是哪。", "长安" },
		{ "题目：收服悟空的法宝，观音所赠是什么。", "紧箍咒" },
		{ "题目：唐僧收的第二个徒弟是谁。", "猪八戒" },
		{ "题目：八戒的原身，天庭何职。", "天蓬帅" },
		{ "题目：八戒的兵器，九齿钉耙。", "九齿耙" },
		{ "题目：唐僧收的第三个徒弟。", "沙和尚" },
		{ "题目：沙僧的原身，天庭何职。", "卷帘将" },
		{ "题目：沙僧的兵器，降妖宝杖。", "降妖杖" },
		{ "题目：取经团队的坐骑，白龙化身。", "白龙马" },
		{ "题目：白龙马原是，西海龙王之子。", "三太子" },
		{ "题目：观音赐唐僧的取经信物。", "锦斓袈裟" },
		{ "题目：悟空火眼金睛，炼于何处。", "八卦炉" },
		{ "题目：悟空的筋斗云，一翻多少里。", "十万八千里" },
		{ "题目：悟空大闹天宫，偷吃的仙品。", "蟠桃" },
		{ "题目：悟空偷吃的，太上老君丹药。", "金丹" },
		{ "题目：收服八戒的地点，高老庄。", "高老庄" },
		{ "题目：收服沙僧的地点，流沙河。", "流沙河" },
		{ "题目：悟空三打，白骨精所变人形。", "白骨精" },
		{ "题目：唐僧因白骨精，赶走悟空的原因。", "误会" },
		{ "题目：悟空三借，什么扇过火焰山。", "芭蕉扇" },
		{ "题目：芭蕉扇的主人，牛魔王之妻是谁。", "铁扇公主" },
		{ "题目：红孩儿的法宝，能喷三昧真火。", "火尖枪" },
		{ "题目：收服红孩儿的神仙。", "观音" },
		{ "题目：红孩儿被观音封为，善财童子。", "善财童" },
		{ "题目：真假美猴王中，假悟空的真身。", "六耳猕猴" },
		{ "题目：分辨真假悟空的，西天佛祖。", "如来" },
		{ "题目：唐僧师徒过的，满是河水的险地。", "通天河" },
		{ "题目：通天河的妖怪，灵感大王。", "灵感王" },
		{ "题目：悟空大闹地府，勾掉的簿子。", "生死簿" },
		{ "题目：悟空的第一个师父。", "菩提祖师" },
		{ "题目：天庭封悟空的，有名无实之职。", "弼马温" },
		{ "题目：悟空自封的称号。", "齐天大圣" },
		{ "题目：取经成功后，悟空被封为何。", "斗战胜佛" },
		{ "题目：取经成功后，唐僧被封为何。", "旃檀功德佛" },
		{ "题目：取经成功后，八戒被封为何。", "净坛使者" },
		{ "题目：取经成功后，沙僧被封为何。", "金身罗汉" },
		{ "题目：白龙马取经后，化龙归海封为何。", "八部天龙" },
		{ "题目：五庄观中，悟空推倒的果树。", "人参果树" },
		{ "题目：五庄观的主人，镇元大仙。", "镇元子" },
		{ "题目：悟空三打白骨精，唐僧念的咒语。", "紧箍咒" },
		{ "题目：西天取经，共经历多少难。", "八十一难" },
		{ "题目：取经的最终目的地。", "西天" },
		{ "题目：如来佛祖所在的灵山。", "灵山" },
		{ "题目：打游戏熬啥夜。", "彻夜" },
		{ "题目：玩游戏盯啥屏。", "屏幕" },
		{ "题目：打游戏喊啥词。", "冲啊" },
		{ "题目：玩游戏忘啥点。", "饭点" },
		{ "题目：打游戏费啥神。", "精力" },
		{ "题目：玩游戏不啥动。", "走动" },
		{ "题目：打游戏盼啥赢。", "胜利" },
		{ "题目：玩游戏刷啥级。", "升级" },
		{ "题目：打游戏啥不住。", "手痒" },
		{ "题目：玩游戏忘啥课。", "网课" },
		{ "题目：打游戏喝啥水。", "凉水" },
		{ "题目：玩游戏堆啥衣。", "脏衣" },
		{ "题目：打游戏求啥胜。", "完胜" },
		{ "题目：玩游戏啥三餐。", "凑合" },
		{ "题目：打游戏摸啥机。", "手机" },
		{ "题目：玩游戏逃啥班。", "上班" },
		{ "题目：打游戏缺啥动。", "运动" },
		{ "题目：打游戏啥眼睛。", "揉眼" },
		{ "题目：玩游戏关啥音。", "消息" },
		{ "题目：打游戏盼啥友。", "队友" },
		{ "题目：玩游戏啥腰背。", "酸困" },
		{ "题目：打游戏守啥机。", "手机" },
		{ "题目：玩游戏啥吃饭。", "边玩" },
		{ "题目：小酒窝，K 歌之王。谁演唱的？", "林俊杰"},
		{ "题目：倒带，日不落，谁演唱的？。", "蔡依林" },
		{ "题目：成都，消愁。谁演唱的？", "赵雷" },
		{ "题目：孤勇者，小幸运。谁演唱的？", "陈奕迅" },
		{ "题目：卡路里，大碗宽面。谁演唱的？", "杨超越" },
		{ "题目：漠河舞厅，独舞出圈。谁演唱的？", "柳爽" },
		{ "题目：学不会，江南。谁演唱的？", "林俊杰" },
		{ "题目：情人，迷迭香。谁演唱的？", "周杰伦" },
		{ "题目：字字句句，实力派女嗓。谁演唱的？", "张碧晨" },
		{ "题目：野狼 disco，宝石老舅。谁演唱的？", "董宝石" },
		{ "题目：稻香，七里香。谁演唱的？", "周杰伦" },
		{ "题目：左手指月，高音封神。谁演唱的？", "萨顶顶" },
		{ "题目：网红校长，挖呀挖呀挖。谁演唱的？", "黄老师" },
		{ "题目：如愿，人世间。谁演唱的？", "王菲" },
		{ "题目：科目三，魔性舞蹈出圈。谁演唱的？", "姜涛" },
		{ "题目：心墙，淋雨一直走。谁演唱的？", "张韶涵" },
		{ "题目：月老掉线，dj 版爆红。谁演唱的？", "王不醒" },
		{ "题目：隐形的翅膀，欧若拉。谁演唱的？", "张韶涵" },
		{ "题目：网红小李，挖野菜名场面。谁演唱的？", "李梦华" },
		{ "题目：花海，说好不哭。谁演唱的？", "周杰伦" },
		{ "题目：笼，消失的她主题曲。谁演唱的？", "张碧晨" },
		{ "题目：小杨哥，疯狂带货出圈。谁演唱的？", "张庆杨" },
		{ "题目：年轮，凉凉。谁演唱的？", "杨宗纬" },
		{ "题目：挖呀挖，幼儿园黄老师。谁演唱的？", "黄静美" },
		{ "题目：体面，说散就散。谁演唱的？", "于文文" },
		{ "题目：嘴哥，健身带货搭档小杨哥。谁演唱的？", "徐不徐" },
		{ "题目：勇气，情歌天后。谁演唱的？", "梁静茹" },
		{ "题目：罗刹海市，刀郎新歌。谁演唱的？", "刀郎" },
		{ "题目：七叔，燕无歇，踏山河。谁演唱的？", "叶泽浩" },
		{ "题目：彩虹，我的歌声里。谁演唱的？", "曲婉婷" },
		{ "题目：秀才，中老年网红顶流。谁演唱的？", "秀才" },
		{ "题目：小幸运，我的少女时代。谁演唱的？", "田馥甄" },
		{ "题目：大风吹，路灯下的小姑娘。谁演唱的？", "刘惜君" },
		{ "题目：宸荨樱桃，网红舞蹈博主。谁演唱的？", "蔡梓彤" },
		{ "题目：永不失联的爱，谁演唱的？", "单依纯" },
		{ "题目：倪海杉，户外直播打赏出圈。谁演唱的？", "倪海杉" },
		{ "题目：光年之外，邓紫棋。谁演唱的？", "邓紫棋" },
		{ "题目：早安隆回，袁树雄。谁演唱的？", "袁树雄" },
		{ "题目：疯狂小杨嫂，小杨哥妻子。谁演唱的？", "陈璐" },
		{ "题目：易燃易爆炸，山海。谁演唱的？", "华晨宇" },
		{ "题目：姐就是女王，自信放光芒。谁演唱的？", "王莎莎" },
		{ "题目：鹿哈，网红翻版鹿晗。谁演唱的？", "凌达乐" },
		{ "题目：听海，剪爱，张惠妹。谁演唱的？", "张惠妹" },
		{ "题目：科目三，广西网红舞蹈。谁演唱的？", "阿斌" },
		{ "题目：雪落下的声音，延禧攻略。谁演唱的？", "陆虎" },
		{ "题目：聂小雨，网红机车女神。谁演唱的？", "聂小雨" },
		{ "题目：星辰大海，黄霄雲。谁演唱的？", "黄霄雲" },
		{ "题目：多余的温柔，网红神曲。谁演唱的？", "小曼" },
		{ "题目：潘周聃，学霸出圈名场面。谁演唱的？", "潘周聃" },
		{ "谜题：耳朵长，尾巴短，爱吃萝卜菜。", "兔子"},
		{ "谜题：头戴红冠，身穿白袍，清晨报晓。", "公鸡" },
		{ "谜题：小时蝌蚪，长大穿绿袍，呱呱叫。", "青蛙" },
		{ "谜题：身上梅花印，头上长小树。", "梅花鹿" },
		{ "谜题：鼻子像钩子，耳朵像扇子。", "大象" },
		{ "谜题：白天睡大觉，晚上空中飞。", "蝙蝠" },
		{ "谜题：身穿花衣裳，春天把歌唱。", "蝴蝶" },
		{ "谜题：红红脸，圆又圆，咬口脆又甜。", "苹果" },
		{ "谜题：身穿绿衣裳，肚里水汪汪。", "西瓜" },
		{ "谜题：小小金坛子，装满金饺子。", "橘子" },
		{ "谜题：兄弟七八个，围着柱子坐。", "大蒜" },
		{ "谜题：水里生，水里长，身穿粉红衫。", "荷花" },
		{ "谜题：小时青，老来黄，敲敲打打响。", "竹子" },
		{ "谜题：方方正正，有门没窗，里面冰凉。", "冰箱" },
		{ "谜题：有面没有口，有脚没有手。", "桌子" },
		{ "谜题：远看像座山，上边水直流。", "雨伞" },
		{ "谜题：头戴玻璃帽，肚里装灯泡。", "手电筒" },
		{ "谜题：白嫩小宝宝，洗澡吹泡泡。", "香皂" },
		{ "谜题：一根小棍儿，顶个圆粒儿。", "火柴" },
		{ "谜题：两个小口袋，天天随身带。", "袜子" },
		{ "谜题：小小诸葛亮，独坐中军帐。", "蜘蛛" },
		{ "谜题：看不见，摸不着，吹得树枝摇。", "风" },
		{ "谜题：千条线，万条线，落入水中不见。", "雨" },
		{ "谜题：小白花，天上飘，像白糖，像鹅毛。", "雪" },
		{ "谜题：夜晚天上挂，圆圆像银盘。", "月亮" },
		{ "谜题：白天出现，晚上不见，照亮大地。", "太阳" },
		{ "谜题：彩色拱桥，雨后挂天边。", "彩虹" },
		{ "谜题：小小伞兵，随风满天飞。", "蒲公英" },
		{ "谜题：有个好朋友，天天跟我走。", "影子" },
		{ "谜题：颜色白如雪，身子硬如铁。", "碗" },
		{ "猜人名：孤勇者，演唱者是谁？", "陈奕迅"},
		{ "猜人名：七里香，演唱者是谁？", "周杰伦" },
		{ "猜人名：小幸运，演唱者是谁？", "田馥甄" },
		{ "猜人名：体面，演唱者是谁？", "于文文" },
		{ "猜人名：起风了，演唱者是谁？", "买辣椒也用券" },
		{ "猜人名：年少有为，演唱者是谁？", "李荣浩" },
		{ "猜人名：演员，演唱者是谁？", "薛之谦" },
		{ "猜人名：告白气球，演唱者是谁？", "周杰伦" },
		{ "猜人名：消愁，演唱者是谁？", "毛不易" },
		{ "猜人名：江南，演唱者是谁？", "林俊杰" },
		{ "猜人名：泡沫，演唱者是谁？", "邓紫棋" },
		{ "猜人名：光年之外，演唱者是谁？", "邓紫棋" },
		{ "猜人名：可惜没如果，演唱者是谁？", "林俊杰" },
		{ "猜人名：追光者，演唱者是谁？", "岑宁儿" },
		{ "猜人名：成都，演唱者是谁？", "赵雷" },
		{ "猜人名：刚好遇见你，演唱者是谁？", "李玉刚" },
		{ "猜人名：我们不一样，演唱者是谁？", "大壮" },
		{ "猜人名：纸短情长，演唱者是谁？", "烟把儿乐队" },
		{ "猜人名：沙漠骆驼，演唱者是谁？", "展展与罗罗" },
		{ "猜人名：绿色，演唱者是谁？", "陈雪凝" },
		{ "猜人名：桥边姑娘，演唱者是谁？", "海伦" },
		{ "猜人名：世间美好与你环环相扣，演唱者是谁？", "柏松" },
		{ "猜人名：芒种，演唱者是谁？", "音阙诗听" },
		{ "猜人名：火红的萨日朗，演唱者是谁？", "乌兰托娅" },
		{ "猜人名：可可托海的牧羊人，演唱者是谁？", "王琪" },
		{ "猜人名：白月光与朱砂痣，演唱者是谁？", "大籽" },
		{ "猜人名：大风吹，演唱者是谁？", "刘惜君、王赫野" },
		{ "猜人名：星辰大海，演唱者是谁？", "黄霄雲" },
		{ "猜人名：千千万万，演唱者是谁？", "深海鱼子酱" },
		{ "猜人名：浪子闲话，演唱者是谁？", "花僮" },
		{ "猜人名：踏山河，演唱者是谁？", "七叔" },
		{ "猜人名：燕无歇，演唱者是谁？", "七叔" },
		{ "猜人名：忘川彼岸，演唱者是谁？", "零一九零贰" },
		{ "猜人名：下山，演唱者是谁？", "要不要买菜" },
		{ "猜人名：少年，演唱者是谁？", "梦然" },
		{ "猜人名：万有引力，演唱者是谁？", "汪苏泷" },
		{ "猜人名：不潮不用花钱，演唱者是谁？", "林俊杰" },
		{ "猜人名：那些年，演唱者是谁？", "胡夏" },
		{ "猜人名：小酒窝，演唱者是谁？", "林俊杰、蔡卓妍" },
		{ "猜人名：因为爱情，演唱者是谁？", "陈奕迅、王菲" },
		{ "猜人名：十年，演唱者是谁？", "陈奕迅" },
		{ "猜人名：后来，演唱者是谁？", "刘若英" },
		{ "猜人名：遇见，演唱者是谁？", "孙燕姿" },
		{ "猜人名：永不失联的爱，演唱者是谁？", "周兴哲" },
		{ "猜人名：这世界那么多人，演唱者是谁？", "莫文蔚" }
	};

	static bool        s_quizActive = false;
	static std::string s_question;
	static std::string s_answerNorm;
	static DWORD       s_gmPid = 0;
	static DWORD       s_rewardVnum = 90065;

	static std::string TrimCopy(const std::string& s)
	{
		size_t b = 0;
		while (b < s.size() && std::isspace((unsigned char)s[b])) b++;
		size_t e = s.size();
		while (e > b && std::isspace((unsigned char)s[e - 1])) e--;
		return s.substr(b, e - b);
	}

	static std::string Normalize(const std::string& s)
	{
		std::string t = TrimCopy(s);
		std::string out;
		out.reserve(t.size());

		bool prevSpace = false;
		for (unsigned char c : t)
		{
			if (std::isspace(c))
			{
				if (!prevSpace) { out.push_back(' '); prevSpace = true; }
			}
			else
			{
				out.push_back((char)std::tolower(c));
				prevSpace = false;
			}
		}
		return TrimCopy(out);
	}

	static LPCHARACTER FindGM()
	{
		if (!s_gmPid)
			return nullptr;
		return CHARACTER_MANAGER::instance().FindByPID(s_gmPid);
	}

	//会触发 GM 命令，在屏幕上打印类似“b”（大通知）的内容。
	static void BigNotice(const char* msg)
	{
		LPCHARACTER gm = FindGM();
		if (gm && gm->GetDesc())
		{
			std::string cmd = "big_notice ";
			cmd += msg;
			interpret_command(gm, cmd.c_str(), cmd.size());
			return;
		}

		// GM bulunamazsa fallback
		BroadcastNotice(msg);
	}

	static void AnnounceQuestion()
	{
		BigNotice("[竞猜活动] 已经开始了！");
		BigNotice("[竞猜活动] 最快回答正确谜题的人将获得“竞猜礼盒”奖励！");
		BigNotice("");
		{
			char tmp[512];
			snprintf(tmp, sizeof(tmp), "%s", s_question.c_str());
			BigNotice(tmp);
		}
		BigNotice("请在聊天窗口回答即可,(答题中聊天不可用)");
	}

	static void StartRandom(LPCHARACTER gm)
	{
		const int poolSize = (int)(sizeof(s_quizPool) / sizeof(s_quizPool[0]));
		if (poolSize <= 0)
			return;

		const int idx = number(0, poolSize - 1);

		s_quizActive = true;
		s_gmPid = (gm ? gm->GetPlayerID() : 0);
		s_question = s_quizPool[idx].q;
		s_answerNorm = Normalize(s_quizPool[idx].a);

		AnnounceQuestion();
	}

	static void EndWithWinner(LPCHARACTER winner)
	{
		s_quizActive = false; //聊天窗口自动重新打开（input_main 检查）

		if (!winner)
			return;

		winner->AutoGiveItem(s_rewardVnum, 1);

		const TItemTable* pItem = ITEM_MANAGER::instance().GetTable(s_rewardVnum);
		char msg[512];
		if (pItem)
			snprintf(msg, sizeof(msg), "[竞猜活动] 本轮获胜者: %s! 获得奖励: %s 已发放到背包.", winner->GetName(), pItem->szLocaleName);
		else
			snprintf(msg, sizeof(msg), "[竞猜活动] 本轮获胜者: %s! 奖品物品: %u 已发放到背包.", winner->GetName(), (unsigned)s_rewardVnum);

		BigNotice(msg);
	}
}

// input_main.cpp 在此处调用
bool Quiz_IsActive()
{
	return s_quizActive;
}

// 当测验处于活动状态时，input_main.cpp 会将所有消息发送到这里。
void Quiz_CheckAnswer(LPCHARACTER ch, const char* msg)
{
	if (!s_quizActive)
		return;

	if (!ch || !ch->IsPC())
		return;

	if (!msg || !*msg)
		return;

	const std::string ans = Normalize(msg);
	if (ans.empty())
		return;

	if (ans == s_answerNorm)
		EndWithWinner(ch);
}

ACMD(do_quiz_start)
{
	if (!ch || !ch->IsGM())
		return;

	if (s_quizActive)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "[竞猜活动] 目前处于激活中,想结束请使用 /quiz_stop");
		return;
	}

	StartRandom(ch);
	ch->ChatPacket(CHAT_TYPE_INFO, "[竞猜活动] 活动期间聊天功能将关闭.");
}

ACMD(do_quiz_stop)
{
	if (!ch || !ch->IsGM())
		return;

	if (!s_quizActive)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "[竞猜活动] 目前已处于关闭中.");
		return;
	}

	s_quizActive = false;
	s_gmPid = ch->GetPlayerID();
	BigNotice("竞猜活动已结束了.");
	ch->ChatPacket(CHAT_TYPE_INFO, "竞猜活动已结束了.");
}
#endif

ACMD(do_user_horse_ride)
{
	if (ch->IsObserverMode())
		return;

	if (ch->IsDead() || ch->IsStun())
		return;

	if (ch->IsHorseRiding() == false)
	{
		if (ch->GetMountVnum())
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("捞固 呕巴阑 捞侩吝涝聪促."));
			return;
		}

		if (ch->GetHorse() == NULL)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("富阑 刚历 家券秦林技夸."));
			return;
		}

		ch->StartRiding();
	}
	else
	{
		ch->StopRiding();
	}
}

ACMD(do_user_horse_back)
{
	if (ch->GetHorse() != NULL)
	{
		ch->HorseSummon(false);
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("富阑 倒妨焊陈嚼聪促."));
	}
	else if (ch->IsHorseRiding() == true)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("富俊辑 刚历 郴妨具 钦聪促."));
	}
	else
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("富阑 刚历 家券秦林技夸."));
	}
}

ACMD(do_user_horse_feed)
{
	if (ch->GetMyShop())
		return;

	if (ch->GetHorse() == NULL)
	{
		if (ch->IsHorseRiding() == false)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("富阑 刚历 家券秦林技夸."));
		}
		else
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("富阑 藕 惑怕俊辑绰 冈捞甫 临 荐 绝嚼聪促."));
		}
		return;
	}

	DWORD dwFood = ch->GetHorseGrade() + 50054 - 1;

	if (ch->CountSpecifyItem(dwFood) > 0)
	{
		ch->RemoveSpecifyItem(dwFood, 1);
		ch->FeedHorse();
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("富俊霸 %s%s 林菌嚼聪促."),ITEM_MANAGER::instance().GetTable (dwFood)->szLocaleName,"");
	}
	else
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("%s 酒捞袍捞 鞘夸钦聪促"), ITEM_MANAGER::instance().GetTable (dwFood)->szLocaleName);
	}
}

#define MAX_REASON_LEN		128

EVENTINFO(TimedEventInfo)
{
	DynamicCharacterPtr ch;
	int		subcmd;
	int         	left_second;
	char		szReason[MAX_REASON_LEN];

	TimedEventInfo()
		: ch()
		, subcmd(0)
		, left_second(0)
	{
		::memset(szReason, 0, MAX_REASON_LEN);
	}
};

struct SendDisconnectFunc
{
	void operator () (LPDESC d)
	{
		if (d->GetCharacter())
		{
#ifdef FLUSH_AT_SHUTDOWN
			d->GetCharacter()->SaveReal();
			DWORD pid = d->GetCharacter()->GetPlayerID();
			db_clientdesc->DBPacketHeader(HEADER_GD_FLUSH_CACHE, 0, sizeof(DWORD));
			db_clientdesc->Packet(&pid, sizeof(DWORD));
#endif
			if (d->GetCharacter()->GetGMLevel() == GM_PLAYER)
				d->GetCharacter()->ChatPacket(CHAT_TYPE_COMMAND, "quit Shutdown(SendDisconnectFunc)");
		}
	}
};

struct DisconnectFunc
{
	void operator () (LPDESC d)
	{
		if (d->GetType() == DESC_TYPE_CONNECTOR)
			return;

		if (d->IsPhase(PHASE_P2P))
			return;

		if (d->GetCharacter())
			d->GetCharacter()->Disconnect("Shutdown(DisconnectFunc)");

		d->SetPhase(PHASE_CLOSE);
	}
};

EVENTINFO(shutdown_event_data)
{
	int seconds;

	shutdown_event_data()
		: seconds(0)
	{
	}
};

EVENTFUNC(shutdown_event)
{
	shutdown_event_data* info = dynamic_cast<shutdown_event_data*>(event->info);

	if (info == NULL)
	{
		sys_err("shutdown_event> <Factor> Null pointer");
		return 0;
	}

	int* pSec = &(info->seconds);

	if (*pSec < 0)
	{
		sys_log(0, "shutdown_event sec %d", *pSec);

		if (-- * pSec == -10)
		{
			const DESC_MANAGER::DESC_SET& c_set_desc = DESC_MANAGER::instance().GetClientSet();
			std::for_each(c_set_desc.begin(), c_set_desc.end(), DisconnectFunc());
			return passes_per_sec;
		}
		else if (*pSec < -10)
			return 0;

		return passes_per_sec;
	}
	else if (*pSec == 0)
	{
		const DESC_MANAGER::DESC_SET& c_set_desc = DESC_MANAGER::instance().GetClientSet();
		std::for_each(c_set_desc.begin(), c_set_desc.end(), SendDisconnectFunc());
		g_bNoMoreClient = true;
		--* pSec;
		return passes_per_sec;
	}
	else
	{
		char buf[64];
		snprintf (buf, sizeof (buf), LC_TEXT ("妓促款捞 %d檬 巢疽嚼聪促."), *pSec);
		SendNotice(buf, true);

		--* pSec;
		return passes_per_sec;
	}
}

void Shutdown(int iSec)
{
	if (g_bNoMoreClient)
	{
		thecore_shutdown();
		return;
	}

	CWarMapManager::instance().OnShutdown();

	char buf[64];
	snprintf(buf, sizeof(buf), LC_TEXT("%d檬 饶 霸烙捞 妓促款 邓聪促."), iSec);
	SendNotice(buf,true);

	shutdown_event_data* info = AllocEventInfo<shutdown_event_data>();
	info->seconds = iSec;

	event_create(shutdown_event, info, 1);
}

ACMD(do_shutdown)
{
	if (NULL == ch)
	{
		sys_err("Accept shutdown command from %s.", ch->GetName());
	}

	TPacketGGShutdown p;
	p.bHeader = HEADER_GG_SHUTDOWN;
	P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGShutdown));
	Shutdown(10);
}

EVENTFUNC(timed_event)
{
	TimedEventInfo* info = dynamic_cast<TimedEventInfo*>(event->info);

	if (info == NULL)
	{
		sys_err("timed_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER	ch = info->ch;
	if (ch == NULL) { // <Factor>
		return 0;
	}
	LPDESC d = ch->GetDesc();

#ifdef ENABLE_DIZCIYA_GOTTEN
	if (1 < 2)
#else
	if (info->left_second <= 0)
#endif
	{
		ch->m_pkTimedEvent = NULL;

		switch (info->subcmd)
		{
		case SCMD_LOGOUT:
		case SCMD_QUIT:
		case SCMD_PHASE_SELECT:
		{
			TPacketNeedLoginLogInfo acc_info;
			acc_info.dwPlayerID = ch->GetDesc()->GetAccountTable().id;
			db_clientdesc->DBPacket(HEADER_GD_VALID_LOGOUT, 0, &acc_info, sizeof(acc_info));
		}
		break;
		}

		switch (info->subcmd)
		{
		case SCMD_LOGOUT:
			if (d)
				d->SetPhase(PHASE_CLOSE);
			break;
#ifdef ENABLE_FIX_CONNETCION_STATUS
		case SCMD_QUIT:
			ch->ChatPacket(CHAT_TYPE_COMMAND, "quit");
			if (d)
				d->DelayedDisconnect(3);
			break;
#else
		case SCMD_QUIT:
			ch->ChatPacket(CHAT_TYPE_COMMAND, "quit");
			break;
#endif
		case SCMD_PHASE_SELECT:
		{
			ch->Disconnect("timed_event - SCMD_PHASE_SELECT");

			if (d)
			{
				d->SetPhase(PHASE_SELECT);
			}
		}
		break;
		}

		return 0;
	}
	else
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("%d檬 巢疽嚼聪促."), info->left_second);
		--info->left_second;
	}

	return PASSES_PER_SEC(1);
}

ACMD(do_cmd)
{
	if (ch->m_pkTimedEvent)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("秒家 登菌嚼聪促."));
		event_cancel(&ch->m_pkTimedEvent);
		return;
	}

	switch (subcmd)
	{
		case SCMD_LOGOUT:
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("肺弊牢 拳搁栏肺 倒酒 癌聪促. 泪矫父 扁促府技夸."));
			break;

		case SCMD_QUIT:
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("霸烙阑 辆丰 钦聪促. 泪矫父 扁促府技夸."));
			break;

		case SCMD_PHASE_SELECT:
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("某腐磐甫 傈券 钦聪促. 泪矫父 扁促府技夸."));
			break;
	}

	if (!ch->CanWarp() && (!ch->GetWarMap() || ch->GetWarMap()->GetType() == GUILD_WAR_TYPE_FLAG))
	{
		return;
	}

	switch (subcmd)
	{
	case SCMD_LOGOUT:
	case SCMD_QUIT:
	case SCMD_PHASE_SELECT:
	{
		TimedEventInfo* info = AllocEventInfo<TimedEventInfo>();

		{
			if (ch->IsPosition(POS_FIGHTING))
				info->left_second = 10;
			else
				info->left_second = 3;
		}

		info->ch = ch;
		info->subcmd = subcmd;
		strlcpy(info->szReason, argument, sizeof(info->szReason));

		ch->m_pkTimedEvent = event_create(timed_event, info, 1);
	}
	break;
	}
}

ACMD(do_mount)
{
}

ACMD(do_fishing)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	ch->SetRotation(atof(arg1));
	ch->fishing();
}

ACMD(do_console)
{
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ConsoleEnable");
}

ACMD(do_restart)
{
	if (false == ch->IsDead())
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "CloseRestartWindow");
		ch->StartRecoveryEvent();
		return;
	}

	if (NULL == ch->m_pkDeadEvent)
		return;

	int iTimeToDead = (event_time(ch->m_pkDeadEvent) / passes_per_sec);

	if (subcmd != SCMD_RESTART_TOWN && (!ch->GetWarMap() || ch->GetWarMap()->GetType() == GUILD_WAR_TYPE_FLAG))
	{
#define eFRS_HERESEC	170
		if (iTimeToDead > eFRS_HERESEC)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("酒流 犁矫累 且 荐 绝嚼聪促. (%d檬 巢澜)"), iTimeToDead - eFRS_HERESEC);
			return;
		}
	}

	//PREVENT_HACK

	if (subcmd == SCMD_RESTART_TOWN)
	{
		if (!ch->CanWarp(false, true))
		{
			if ((!ch->GetWarMap() || ch->GetWarMap()->GetType() == GUILD_WAR_TYPE_FLAG))
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("酒流 犁矫累 且 荐 绝嚼聪促. (%d檬 巢澜)"), iTimeToDead - (180 - g_WarplLimitTime));
				return;
			}
		}

#define eFRS_TOWNSEC	173
		if (iTimeToDead > eFRS_TOWNSEC)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("酒流 付阑俊辑 犁矫累 且 荐 绝嚼聪促. (%d 檬 巢澜)"), iTimeToDead - eFRS_TOWNSEC);
			return;
		}
	}
	//END_PREVENT_HACK

	ch->ChatPacket(CHAT_TYPE_COMMAND, "CloseRestartWindow");

	ch->GetDesc()->SetPhase(PHASE_GAME);
	ch->SetPosition(POS_STANDING);
	ch->StartRecoveryEvent();

	if (ch->GetDungeon())
		ch->GetDungeon()->UseRevive(ch);

	if (ch->GetWarMap() && !ch->IsObserverMode())
	{
		CWarMap* pMap = ch->GetWarMap();
		DWORD dwGuildOpponent = pMap ? pMap->GetGuildOpponent(ch) : 0;

		if (dwGuildOpponent)
		{
			switch (subcmd)
			{
				//帮会地图村落复活
				case SCMD_RESTART_TOWN:
					PIXEL_POSITION pos;

					if (CWarMapManager::instance().GetStartPosition(ch->GetMapIndex(), ch->GetGuild()->GetID() < dwGuildOpponent ? 0 : 1, pos))
						ch->Show(ch->GetMapIndex(), pos.x, pos.y);
					else
						ch->ExitToSavedLocation();

					ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
					ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
					ch->ReviveInvisible(5);
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
					ch->CheckMount();
#endif
#ifdef ENABLE_PET_COSTUME_SYSTEM
					ch->CheckPet();
#endif
					break;
				//帮会地图原地复活
				case SCMD_RESTART_HERE:
					ch->RestartAtSamePos();
					ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
					ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
					ch->ReviveInvisible(5);
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
					ch->CheckMount();
#endif
#ifdef ENABLE_PET_COSTUME_SYSTEM
					ch->CheckPet();
#endif
				break;
			}

			return;
		}
	}
	switch (subcmd)
	{
	//村落复活
	case SCMD_RESTART_TOWN:
		PIXEL_POSITION pos;

		if (SECTREE_MANAGER::instance().GetRecallPositionByEmpire(ch->GetMapIndex(), ch->GetEmpire(), pos))
			ch->WarpSet(pos.x, pos.y);
		else
			ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));

#ifdef ENABLE_POINT_HP_SYSTEM__
			ch->PointChange (POINT_HP, ch->GetMaxHP() - ch->GetHP());
			ch->PointChange (POINT_SP, ch->GetMaxSP() - ch->GetSP());
#else
			ch->PointChange (POINT_HP, 50 - ch->GetHP());
#endif
		ch->DeathPenalty(1);
		break;
	//原地复活
	case SCMD_RESTART_HERE:
		ch->RestartAtSamePos();

#ifdef ENABLE_POINT_HP_SYSTEM__
		ch->PointChange (POINT_HP, ch->GetMaxHP() - ch->GetHP());
		ch->PointChange (POINT_SP, ch->GetMaxSP() - ch->GetSP());
#else
		ch->PointChange(POINT_HP, 50 - ch->GetHP());
#endif
		ch->DeathPenalty(0);
		ch->ReviveInvisible(5);
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
		ch->CheckMount();
#endif
#ifdef ENABLE_PET_COSTUME_SYSTEM
		ch->CheckPet();
#endif
		break;
	}
}

#ifdef ENABLE_RESTART_INSTANT
ACMD(do_restart_now)
{
	if (false == ch->IsDead())
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "CloseRestartWindow");
		ch->StartRecoveryEvent();
		return;
	}

	if (ch->GetMapIndex() == 202 || ch->GetMapIndex() == 211 || ch->GetMapIndex() == 241)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Immediate resurrection is prohibited on the battlefield."));
		return;
	}

	if (ch->GetGold() < 100000)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Immediate resurrection requires 100000 yuan"));
		return;
	}

	if (NULL == ch->m_pkDeadEvent)
		return;

	if (!ch->GetWarMap() || ch->GetWarMap()->GetType() == GUILD_WAR_TYPE_FLAG)
	{
		if (!test_server)
		{
			if (ch->CanWarp())
			{
				if ((!ch->GetWarMap() || ch->GetWarMap()->GetType() == GUILD_WAR_TYPE_FLAG))
				{
					ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Cannot be resurrected when using trading and other windows"));
					return;
				}
			}
		}

		ch->ChatPacket(CHAT_TYPE_COMMAND, "CloseRestartWindow");
		ch->GetDesc()->SetPhase(PHASE_GAME);
		ch->SetPosition(POS_STANDING);
		ch->StartRecoveryEvent();
		ch->RestartAtSamePos();
		ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
		ch->DeathPenalty(0);
		ch->ReviveInvisible(5);
		ch->PointChange(POINT_GOLD, -100000, false);
	}
}
#endif

#define MAX_STAT g_iStatusPointSetMaxValue

ACMD(do_stat_reset)
{
	ch->PointChange(POINT_STAT_RESET_COUNT, 12 - ch->GetPoint(POINT_STAT_RESET_COUNT));
}

ACMD(do_stat_minus)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (ch->IsPolymorphed())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("敌癌 吝俊绰 瓷仿阑 棵副 荐 绝嚼聪促."));
		return;
	}

	if (ch->GetPoint(POINT_STAT_RESET_COUNT) <= 0)
		return;

	if (!strcmp(arg1, "st"))
	{
		if (ch->GetRealPoint(POINT_ST) <= JobInitialPoints[ch->GetJob()].st)
			return;

		ch->SetRealPoint(POINT_ST, ch->GetRealPoint(POINT_ST) - 1);
		ch->SetPoint(POINT_ST, ch->GetPoint(POINT_ST) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_ST, 0);
	}
	else if (!strcmp(arg1, "dx"))
	{
		if (ch->GetRealPoint(POINT_DX) <= JobInitialPoints[ch->GetJob()].dx)
			return;

		ch->SetRealPoint(POINT_DX, ch->GetRealPoint(POINT_DX) - 1);
		ch->SetPoint(POINT_DX, ch->GetPoint(POINT_DX) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_DX, 0);
	}
	else if (!strcmp(arg1, "ht"))
	{
		if (ch->GetRealPoint(POINT_HT) <= JobInitialPoints[ch->GetJob()].ht)
			return;

		ch->SetRealPoint(POINT_HT, ch->GetRealPoint(POINT_HT) - 1);
		ch->SetPoint(POINT_HT, ch->GetPoint(POINT_HT) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_HT, 0);
		ch->PointChange(POINT_MAX_HP, 0);
	}
	else if (!strcmp(arg1, "iq"))
	{
		if (ch->GetRealPoint(POINT_IQ) <= JobInitialPoints[ch->GetJob()].iq)
			return;

		ch->SetRealPoint(POINT_IQ, ch->GetRealPoint(POINT_IQ) - 1);
		ch->SetPoint(POINT_IQ, ch->GetPoint(POINT_IQ) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_IQ, 0);
		ch->PointChange(POINT_MAX_SP, 0);
	}
	else
		return;

	ch->PointChange(POINT_STAT, +1);
	ch->PointChange(POINT_STAT_RESET_COUNT, -1);
	ch->ComputePoints();
}

ACMD(do_stat)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
		return;

	if (ch->IsPolymorphed())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("敌癌 吝俊绰 瓷仿阑 棵副 荐 绝嚼聪促."));
		return;
	}

	if (ch->WindowOpenCheck() || ch->ActivateCheck())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("CLOSE_WINDOWS_ERROR"));
		return;
	}

	if (ch->GetPoint(POINT_STAT) <= 0)
		return;

	BYTE idx = 0;

	if (!strcmp(arg1, "st"))
		idx = POINT_ST;
	else if (!strcmp(arg1, "dx"))
		idx = POINT_DX;
	else if (!strcmp(arg1, "ht"))
		idx = POINT_HT;
	else if (!strcmp(arg1, "iq"))
		idx = POINT_IQ;
	else
		return;

	if (ch->GetRealPoint(idx) >= MAX_STAT)
		return;

	int point_incr = 1;
	int deline = MAX_STAT - ch->GetRealPoint(idx);
	str_to_number(point_incr, arg2);
	point_incr = MIN(point_incr, deline); //@Check the real point to add (check ch->GetRealPoint(idx) + point_incr >= MAX_STAT)

	if (point_incr <= 0)
		return;
	else if (point_incr > ch->GetPoint(POINT_STAT))
		point_incr = ch->GetPoint(POINT_STAT);

	ch->SetRealPoint(idx, ch->GetRealPoint(idx) + point_incr);
	ch->SetPoint(idx, ch->GetPoint(idx) + point_incr);
	ch->ComputePoints();
	ch->PointChange(idx, 0);

	if (idx == POINT_IQ)
	{
		ch->PointChange(POINT_MAX_HP, 0);
	}
	else if (idx == POINT_HT)
	{
		ch->PointChange(POINT_MAX_SP, 0);
	}

	ch->PointChange(POINT_STAT, -point_incr);
	ch->ComputePoints();
}

ACMD(do_pvp)
{
#ifdef TOURNAMENT_PVP_SYSTEM
	if (CTournamentPvP::instance().IsTournamentMap(ch, TOURNAMENT_BLOCK_DUEL))
		return;
#endif
	if (ch->GetArena() != NULL || CArenaManager::instance().IsArenaMap(ch->GetMapIndex()) == true)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("措访厘俊辑 荤侩窍角 荐 绝嚼聪促."));
		return;
	}

	if (ch->GetMapIndex() == 113) // @fixme212
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("OX map cannot be used for PK."));
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	DWORD vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER pkVictim = CHARACTER_MANAGER::instance().Find(vid);

	if (!pkVictim)
		return;

	if (pkVictim->IsNPC())
		return;

	if (pkVictim->GetArena() != NULL)
	{
		pkVictim->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("惑措规捞 措访吝涝聪促."));
		return;
	}

	CPVPManager::instance().Insert(ch, pkVictim);
}

ACMD(do_guildskillup)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (!ch->GetGuild())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<辨靛> 辨靛俊 加秦乐瘤 臼嚼聪促."));
		return;
	}

	CGuild* g = ch->GetGuild();
	TGuildMember* gm = g->GetMember(ch->GetPlayerID());
	if (gm->grade == GUILD_LEADER_GRADE)
	{
		DWORD vnum = 0;
		str_to_number(vnum, arg1);
		g->SkillLevelUp(vnum);
	}
	else
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<辨靛> 辨靛 胶懦 饭骇阑 函版且 鼻茄捞 绝嚼聪促."));
	}
}

ACMD(do_skillup)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vnum = 0;
	str_to_number(vnum, arg1);

	if (true == ch->CanUseSkill(vnum))
	{
		ch->SkillLevelUp(vnum);
	}
	else
	{
		switch (vnum)
		{
		case SKILL_HORSE_WILDATTACK:
		case SKILL_HORSE_CHARGE:
		case SKILL_HORSE_ESCAPE:
		case SKILL_HORSE_WILDATTACK_RANGE:

		case SKILL_7_A_ANTI_TANHWAN:
		case SKILL_7_B_ANTI_AMSEOP:
		case SKILL_7_C_ANTI_SWAERYUNG:
		case SKILL_7_D_ANTI_YONGBI:

		case SKILL_8_A_ANTI_GIGONGCHAM:
		case SKILL_8_B_ANTI_YEONSA:
		case SKILL_8_C_ANTI_MAHWAN:
		case SKILL_8_D_ANTI_BYEURAK:

		case SKILL_ADD_HP:
		case SKILL_RESIST_PENETRATE:
#ifdef ENABLE_PASSIVE_SKILLS
		case SKILL_PASSIVE_MONSTER:
		case SKILL_PASSIVE_STONE:
		case SKILL_PASSIVE_BERSERKER:
		case SKILL_PASSIVE_HUMAN:
		case SKILL_PASSIVE_SKILL_COOLDOWN:
		case SKILL_PASSIVE_DRAGON_HEARTH:
		case SKILL_PASSIVE_HERBOLOGY:
		case SKILL_PASSIVE_UPGRADING:
		case SKILL_PASSIVE_FISHING:
#endif

#ifdef ENABLE_BUFFI_SYSTEM
		case SKILL_BUFFI_1:
		case SKILL_BUFFI_2:
		case SKILL_BUFFI_3:
		case SKILL_BUFFI_4:
		case SKILL_BUFFI_5:
#endif
			ch->SkillLevelUp(vnum);
			break;
		}
	}
}

ACMD(do_safebox_close)
{
	ch->CloseSafebox();
}

ACMD(do_safebox_password)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (ch->GetMapIndex() == 112 || ch->GetMapIndex() == 113 || ch->GetMapIndex() == 26 || ch->GetMapIndex() == 110 || ch->GetMapIndex() == 202)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("The battlefield map cannot open the warehouse."));//当前地图不能开启仓库
		return;
	}

	if (ch->WindowOpenCheck() || ch->ActivateCheck())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("CLOSE_WINDOWS_ERROR1"));
		return;
	}
	ch->ReqSafeboxLoad(arg1);
}

ACMD(do_safebox_change_password)
{
	return;

	char arg1[256];
	char arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || strlen(arg1) > 6)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<芒绊> 肋给等 鞠龋甫 涝仿窍继嚼聪促."));
		return;
	}

	if (!*arg2 || strlen(arg2) > 6)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<芒绊> 肋给等 鞠龋甫 涝仿窍继嚼聪促."));
		return;
	}

	TSafeboxChangePasswordPacket p;

	p.dwID = ch->GetDesc()->GetAccountTable().id;
	strlcpy(p.szOldPassword, arg1, sizeof(p.szOldPassword));
	strlcpy(p.szNewPassword, arg2, sizeof(p.szNewPassword));

	db_clientdesc->DBPacket(HEADER_GD_SAFEBOX_CHANGE_PASSWORD, ch->GetDesc()->GetHandle(), &p, sizeof(p));
}

ACMD(do_mall_password)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (ch->GetMapIndex() == 112 || ch->GetMapIndex() == 113 || ch->GetMapIndex() == 26 || ch->GetMapIndex() == 110 || ch->GetMapIndex() == 202)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("The battlefield map cannot open the mall."));//战场地图不能开启仓库
		return;
	}

	if (!*arg1 || strlen(arg1) > 6)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<芒绊> 肋给等 鞠龋甫 涝仿窍继嚼聪促."));//"<仓库> 密码输入错误 ";
		return;
	}

	int iPulse = thecore_pulse();

	if (ch->GetMall())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<芒绊> 芒绊啊 捞固 凯妨乐嚼聪促."));//"<仓库> 仓库已打开 ";
		return;
	}

	if (iPulse - ch->GetMallLoadTime() < passes_per_sec * 10)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<芒绊> 芒绊甫 摧篮瘤 10檬 救俊绰 凯 荐 绝嚼聪促."));//"<仓库> 关闭仓库后10秒内不能再次打开仓库 ";
		return;
	}

	ch->SetMallLoadTime(iPulse);

	TSafeboxLoadPacket p;
	p.dwID = ch->GetDesc()->GetAccountTable().id;
	strlcpy(p.szLogin, ch->GetDesc()->GetAccountTable().login, sizeof(p.szLogin));
	strlcpy(p.szPassword, arg1, sizeof(p.szPassword));

	db_clientdesc->DBPacket(HEADER_GD_MALL_LOAD, ch->GetDesc()->GetHandle(), &p, sizeof(p));
}

ACMD(do_mall_close)
{
	if (ch->GetMall())
	{
		ch->SetMallLoadTime(thecore_pulse());
		ch->CloseMall();
		ch->Save();
	}
}

ACMD(do_ungroup)
{
	if (!ch->GetParty())
		return;

	if (!CPartyManager::instance().IsEnablePCParty())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<颇萍> 辑滚 巩力肺 颇萍 包访 贸府甫 且 荐 绝嚼聪促."));
		return;
	}

	if (ch->GetDungeon())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<颇萍> 带傈 救俊辑绰 颇萍俊辑 唱哎 荐 绝嚼聪促."));
		return;
	}

	LPPARTY pParty = ch->GetParty();

	if (pParty->GetMemberCount() == 2)
	{
		// party disband
		CPartyManager::instance().DeleteParty(pParty);
	}
	else
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<颇萍> 颇萍俊辑 唱啊继嚼聪促."));
		pParty->Quit(ch->GetPlayerID());
	}
}

ACMD(do_close_shop)
{
	if (ch->GetMyShop())
	{
		ch->CloseMyShop();
		return;
	}
}

ACMD(do_set_walk_mode)
{
	ch->SetNowWalking(true);
	ch->SetWalking(true);
}

ACMD(do_set_run_mode)
{
	ch->SetNowWalking(false);
	ch->SetWalking(false);
}

ACMD(do_war)
{
	CGuild* g = ch->GetGuild();

	if (!g)
		return;

	if (g->UnderAnyWar())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<辨靛> 捞固 促弗 傈里俊 曼傈 吝 涝聪促."));
		return;
	}

	char arg1[256], arg2[256];
	DWORD type = GUILD_WAR_TYPE_FIELD; //fixme102 base int modded uint
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
		return;

	if (*arg2)
	{
		str_to_number(type, arg2);

		if (type < 0)// @fixme20
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<error> can't declare war with type less than zero."));
			return;
		}

		if (type >= GUILD_WAR_TYPE_MAX_NUM)
			type = GUILD_WAR_TYPE_FIELD;
	}

	DWORD gm_pid = g->GetMasterPID();

	if (gm_pid != ch->GetPlayerID())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<辨靛> 辨靛傈俊 措茄 鼻茄捞 绝嚼聪促."));
		return;
	}

	CGuild* opp_g = CGuildManager::instance().FindGuildByName(arg1);

	if (!opp_g)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<辨靛> 弊繁 辨靛啊 绝嚼聪促."));
		return;
	}

	switch (g->GetGuildWarState(opp_g->GetID()))
	{
	case GUILD_WAR_NONE:
	{
		if (opp_g->UnderAnyWar())
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<辨靛> 惑措规 辨靛啊 捞固 傈里 吝 涝聪促."));
			return;
		}

		int iWarPrice = KOR_aGuildWarInfo[type].iWarPrice;

		if (g->GetGuildMoney() < iWarPrice)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<辨靛> 傈厚啊 何练窍咯 辨靛傈阑 且 荐 绝嚼聪促."));
			return;
		}

		if (opp_g->GetGuildMoney() < iWarPrice)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<辨靛> 惑措规 辨靛狼 傈厚啊 何练窍咯 辨靛傈阑 且 荐 绝嚼聪促."));
			return;
		}
	}
	break;

	case GUILD_WAR_SEND_DECLARE:
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("捞固 急傈器绊 吝牢 辨靛涝聪促."));
		return;
	}
	break;

	case GUILD_WAR_RECV_DECLARE:
	{
		if (opp_g->UnderAnyWar())
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<辨靛> 惑措规 辨靛啊 捞固 傈里 吝 涝聪促."));
			g->RequestRefuseWar(opp_g->GetID());
			return;
		}
	}
	break;

	case GUILD_WAR_RESERVE:
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<辨靛> 捞固 傈里捞 抗距等 辨靛 涝聪促."));
		return;
	}
	break;

	case GUILD_WAR_END:
		return;

	default:
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<辨靛> 捞固 傈里 吝牢 辨靛涝聪促."));
		g->RequestRefuseWar(opp_g->GetID());
		return;
	}

	if (!g->CanStartWar(type))
	{
		if (g->GetLadderPoint() == 0)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<辨靛> 饭歹 痢荐啊 葛磊扼辑 辨靛傈阑 且 荐 绝嚼聪促."));
		}
		else if (g->GetMemberCount() < GUILD_WAR_MIN_MEMBER_COUNT)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<辨靛> 辨靛傈阑 窍扁 困秦急 弥家茄 %d疙捞 乐绢具 钦聪促."), GUILD_WAR_MIN_MEMBER_COUNT);
		}
		return;
	}

	if (!opp_g->CanStartWar(GUILD_WAR_TYPE_FIELD))
	{
		if (opp_g->GetLadderPoint() == 0)
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<辨靛> 惑措规 辨靛狼 饭歹 痢荐啊 葛磊扼辑 辨靛傈阑 且 荐 绝嚼聪促."));
		else if (opp_g->GetMemberCount() < GUILD_WAR_MIN_MEMBER_COUNT)
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<辨靛> 惑措规 辨靛狼 辨靛盔 荐啊 何练窍咯 辨靛傈阑 且 荐 绝嚼聪促."));
		return;
	}

	do
	{
		if (g->GetMasterCharacter() != NULL)
			break;

		CCI* pCCI = P2P_MANAGER::instance().FindByPID(g->GetMasterPID());

		if (pCCI != NULL)
			break;

		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<辨靛> 惑措规 辨靛狼 辨靛厘捞 立加吝捞 酒凑聪促."));
		g->RequestRefuseWar(opp_g->GetID());
		return;
	} while (false);

	do
	{
		if (opp_g->GetMasterCharacter() != NULL)
			break;

		CCI* pCCI = P2P_MANAGER::instance().FindByPID(opp_g->GetMasterPID());

		if (pCCI != NULL)
			break;

		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<辨靛> 惑措规 辨靛狼 辨靛厘捞 立加吝捞 酒凑聪促."));
		g->RequestRefuseWar(opp_g->GetID());
		return;
	} while (false);

	g->RequestDeclareWar(opp_g->GetID(), type);
}

ACMD(do_nowar)
{
	CGuild* g = ch->GetGuild();
	if (!g)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD gm_pid = g->GetMasterPID();

	if (gm_pid != ch->GetPlayerID())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<辨靛> 辨靛傈俊 措茄 鼻茄捞 绝嚼聪促."));
		return;
	}

	CGuild* opp_g = CGuildManager::instance().FindGuildByName(arg1);

	if (!opp_g)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("<辨靛> 弊繁 辨靛啊 绝嚼聪促."));
		return;
	}

	g->RequestRefuseWar(opp_g->GetID());
}

ACMD(do_pkmode)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	BYTE mode = 0;
	str_to_number(mode, arg1);

	if (mode == PK_MODE_PROTECT)
		return;

	if (ch->GetLevel() < PK_PROTECT_LEVEL && mode != 0)
		return;
#ifdef TOURNAMENT_PVP_SYSTEM
	if (ch->GetMapIndex() == TOURNAMENT_MAP_INDEX)
		return;

	if (mode == PK_MODE_TEAM_A || mode == PK_MODE_TEAM_B)
		return;
#endif
	ch->SetPKMode(mode);
}

ACMD(do_messenger_auth)
{
	if (ch->GetArena())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("措访厘俊辑 荤侩窍角 荐 绝嚼聪促."));
		return;
	}

	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
		return;

	char answer = LOWER(*arg1);
	// @fixme130 AuthToAdd void -> bool
	bool bIsDenied = answer != 'y';
	bool bIsAdded = MessengerManager::instance().AuthToAdd(ch->GetName(), arg2, bIsDenied); // DENY
	if (bIsAdded && bIsDenied)
	{
		LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg2);

		if (tch)
			tch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("%s 丛栏肺 何磐 模备 殿废阑 芭何 寸沁嚼聪促."), ch->GetName());
	}
}

ACMD(do_setblockmode)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		BYTE flag = 0;
		str_to_number(flag, arg1);
		ch->SetBlockMode(flag);
	}
}

ACMD(do_unmount)
{

#ifdef ENABLE_MOUNT_SKIN
#ifdef ENABLE_HIDE_COSTUME
	if (ch->GetWear(WEAR_MOUNT_SKIN) && !ch->IsMountSkinHidden())
#else
	if (ch->GetWear(WEAR_MOUNT_SKIN))
#endif
	{
		CMountSystem* mountSystem = ch->GetMountSystem();
		LPITEM mount = ch->GetWear(WEAR_MOUNT_SKIN);
		DWORD mobVnum = 0;

		if (!mountSystem && !mount)
			return;

		if (mount->GetValue(1) != 0)
			mobVnum = mount->GetValue(1);

		if (ch->GetMountVnum())
		{
			if (mountSystem->CountSummoned() == 0)
			{
				mountSystem->Unmount(mobVnum);
			}
		}
		return;
	}
#endif

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
#ifdef ENABLE_MOUNT_SKIN
	else if (ch->GetWear(WEAR_COSTUME_MOUNT))
#else
	if (ch->GetWear(WEAR_COSTUME_MOUNT))
#endif
	{
		CMountSystem* mountSystem = ch->GetMountSystem();
		LPITEM mount = ch->GetWear(WEAR_COSTUME_MOUNT);
		DWORD mobVnum = 0;

		if (!mountSystem && !mount)
			return;

		if (mount->GetValue(1) != 0)
			mobVnum = mount->GetValue(1);

		if (ch->GetMountVnum())
		{
			if (mountSystem->CountSummoned() == 0)
			{
				mountSystem->Unmount(mobVnum);
			}
		}
		return;
	}
#endif

	LPITEM item = ch->GetWear(WEAR_UNIQUE1);
	LPITEM item2 = ch->GetWear(WEAR_UNIQUE2);
	LPITEM item3 = ch->GetWear(WEAR_COSTUME_MOUNT);

	if (item && item->IsRideItem())
		ch->UnequipItem(item);

	if (item2 && item2->IsRideItem())
		ch->UnequipItem(item2);

	if (item3 && item3->IsRideItem())
		ch->UnequipItem(item3);

	if (ch->IsHorseRiding())
	{
		ch->StopRiding();
	}
}

ACMD(do_observer_exit)
{
	if (ch->IsObserverMode())
	{
#ifdef TOURNAMENT_PVP_SYSTEM
		if (CTournamentPvP::instance().IsTournamentMap(ch, TOURNAMENT_BLOCK_EXIT_OBSERVER_MODE))
			return;
#endif
		if (ch->GetWarMap())
			ch->SetWarMap(NULL);

		if (ch->GetArena() != NULL || ch->GetArenaObserverMode() == true)
		{
			ch->SetArenaObserverMode(false);

			if (ch->GetArena() != NULL)
				ch->GetArena()->RemoveObserver(ch->GetPlayerID());

			ch->SetArena(NULL);
			ch->WarpSet(ARENA_RETURN_POINT_X(ch->GetEmpire()), ARENA_RETURN_POINT_Y(ch->GetEmpire()));
		}
		else
		{
			ch->ExitToSavedLocation();
		}
		ch->SetObserverMode(false);
	}
}

ACMD(do_view_equip)
{
	if (!ch)
		return;
	// if (ch->GetGMLevel() <= GM_PLAYER)
		// return;
#if defined(ENABLE_MAP_195_ALIGNMENT)	
	if (ch->GetMapIndex() ==195)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("ALIGNMENT_MAP195_NO_equip"));
		return;
	}
#endif
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		DWORD vid = 0;
		str_to_number(vid, arg1);
		LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

		if (!tch)
			return;

#ifdef ENABLE_BOT_PLAYER
		if (tch->IsBotCharacter())
		{
			tch->SendEquipment(ch);
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("BOT_PLAYER_NO_equip"));
			return;
		}
#endif

		if (!tch->IsPC())
			return;

		tch->SendEquipment(ch);
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("The window has been opened."));
	}
}

ACMD(do_party_request)
{
	if (ch->GetArena())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("措访厘俊辑 荤侩窍角 荐 绝嚼聪促."));
		return;
	}

	if (ch->GetParty())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("捞固 颇萍俊 加秦 乐栏骨肺 啊涝脚没阑 且 荐 绝嚼聪促."));
		return;
	}
#ifdef TOURNAMENT_PVP_SYSTEM
	if (CTournamentPvP::instance().IsTournamentMap(ch, TOURNAMENT_BLOCK_PARTY))
		return;
#endif
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

	if (tch)
		if (!ch->RequestToParty(tch))
			ch->ChatPacket(CHAT_TYPE_COMMAND, "PartyRequestDenied");
}

ACMD(do_party_request_accept)
{
#ifdef TOURNAMENT_PVP_SYSTEM
	if (CTournamentPvP::instance().IsTournamentMap(ch, TOURNAMENT_BLOCK_PARTY))
		return;
#endif
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

	if (tch)
		ch->AcceptToParty(tch);
}

ACMD(do_party_request_deny)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

	if (tch)
		ch->DenyToParty(tch);
}

// LUA_ADD_GOTO_INFO
struct GotoInfo
{
	std::string 	st_name;

	BYTE 	empire;
	int 	mapIndex;
	DWORD 	x, y;

	GotoInfo()
	{
		st_name = "";
		empire = 0;
		mapIndex = 0;

		x = 0;
		y = 0;
	}

	GotoInfo(const GotoInfo& c_src)
	{
		__copy__(c_src);
	}

	void operator = (const GotoInfo& c_src)
	{
		__copy__(c_src);
	}

	void __copy__(const GotoInfo& c_src)
	{
		st_name = c_src.st_name;
		empire = c_src.empire;
		mapIndex = c_src.mapIndex;

		x = c_src.x;
		y = c_src.y;
	}
};

ACMD(do_inventory)
{
	int	index = 0;
	int	count = 1;

	char arg1[256];
	char arg2[256];

	LPITEM	item;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: inventory <start_index> <count>");
		return;
	}

	if (!*arg2)
	{
		index = 0;
		str_to_number(count, arg1);
	}
	else
	{
		str_to_number(index, arg1); index = MIN(index, INVENTORY_MAX_NUM);
		str_to_number(count, arg2); count = MIN(count, INVENTORY_MAX_NUM);
	}

	for (int i = 0; i < count; ++i)
	{
		if (index >= INVENTORY_MAX_NUM)
			break;

		item = ch->GetInventoryItem(index);

		ch->ChatPacket(CHAT_TYPE_INFO, "inventory [%d] = %s",
			index, item ? item->GetName() : "<NONE>");
		++index;
	}
}

#ifdef ENABLE_CUBE_RENEWAL_WORLDARD
ACMD(do_cube)
{
	const char* line;
	char arg1[256], arg2[256], arg3[256];
	line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	one_argument(line, arg3, sizeof(arg3));

	if (0 == arg1[0])
	{
		return;
	}

	switch (LOWER(arg1[0]))
	{
	case 'o':	// open
		Cube_open(ch);
		break;

	default:
		return;
	}
}
#endif

ACMD(do_in_game_mall)
{
	if (LC_IsEurope() == true)
	{
		char country_code[3];

		switch (LC_GetLocalType())
		{
		case LC_GERMANY:	country_code[0] = 'd'; country_code[1] = 'e'; country_code[2] = '\0'; break;
		case LC_FRANCE:		country_code[0] = 'f'; country_code[1] = 'r'; country_code[2] = '\0'; break;
		case LC_ITALY:		country_code[0] = 'i'; country_code[1] = 't'; country_code[2] = '\0'; break;
		case LC_SPAIN:		country_code[0] = 'e'; country_code[1] = 's'; country_code[2] = '\0'; break;
		case LC_UK:			country_code[0] = 'e'; country_code[1] = 'n'; country_code[2] = '\0'; break;
		case LC_TURKEY:		country_code[0] = 't'; country_code[1] = 'r'; country_code[2] = '\0'; break;
		case LC_POLAND:		country_code[0] = 'p'; country_code[1] = 'l'; country_code[2] = '\0'; break;
		case LC_PORTUGAL:	country_code[0] = 'p'; country_code[1] = 't'; country_code[2] = '\0'; break;
		case LC_GREEK:		country_code[0] = 'g'; country_code[1] = 'r'; country_code[2] = '\0'; break;
		case LC_RUSSIA:		country_code[0] = 'r'; country_code[1] = 'u'; country_code[2] = '\0'; break;
		case LC_DENMARK:	country_code[0] = 'd'; country_code[1] = 'k'; country_code[2] = '\0'; break;
		case LC_BULGARIA:	country_code[0] = 'b'; country_code[1] = 'g'; country_code[2] = '\0'; break;
		case LC_CROATIA:	country_code[0] = 'h'; country_code[1] = 'r'; country_code[2] = '\0'; break;
		case LC_MEXICO:		country_code[0] = 'm'; country_code[1] = 'x'; country_code[2] = '\0'; break;
		case LC_ARABIA:		country_code[0] = 'a'; country_code[1] = 'e'; country_code[2] = '\0'; break;
		case LC_CZECH:		country_code[0] = 'c'; country_code[1] = 'z'; country_code[2] = '\0'; break;
		case LC_ROMANIA:	country_code[0] = 'r'; country_code[1] = 'o'; country_code[2] = '\0'; break;
		case LC_HUNGARY:	country_code[0] = 'h'; country_code[1] = 'u'; country_code[2] = '\0'; break;
		case LC_NETHERLANDS: country_code[0] = 'n'; country_code[1] = 'l'; country_code[2] = '\0'; break;
		case LC_USA:		country_code[0] = 'u'; country_code[1] = 's'; country_code[2] = '\0'; break;
		case LC_CANADA:	country_code[0] = 'c'; country_code[1] = 'a'; country_code[2] = '\0'; break;
		default:
			break;
		}

		char buf[512 + 1];
		char sas[33];
		MD5_CTX ctx;
		const char sas_key[] = "GF9001";

		snprintf(buf, sizeof(buf), "%u%u%s", ch->GetPlayerID(), ch->GetAID(), sas_key);

		MD5Init(&ctx);
		MD5Update(&ctx, (const unsigned char*)buf, strlen(buf));
#ifdef __FreeBSD__
		MD5End(&ctx, sas);
#else
		static const char hex[] = "0123456789abcdef";
		unsigned char digest[16];
		MD5Final(digest, &ctx);
		int i;
		for (i = 0; i < 16; ++i) {
			sas[i + i] = hex[digest[i] >> 4];
			sas[i + i + 1] = hex[digest[i] & 0x0f];
		}
		sas[i + i] = '\0';
#endif

		snprintf(buf, sizeof(buf), "mall http://%s/ishop?pid=%u&c=%s&sid=%d&sas=%s",
			g_strWebMallURL.c_str(), ch->GetPlayerID(), country_code, g_server_id, sas);

		ch->ChatPacket(CHAT_TYPE_COMMAND, buf);
	}
}

ACMD(do_dice)
{
	char arg1[256], arg2[256];
	int start = 1, end = 100;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (*arg1 && *arg2)
	{
		start = atoi(arg1);
		end = atoi(arg2);
	}
	else if (*arg1 && !*arg2)
	{
		start = 1;
		end = atoi(arg1);
	}

	end = MAX(start, end);
	start = MIN(start, end);

	int n = number(start, end);

#ifdef ENABLE_DICE_SYSTEM
	if (ch->GetParty())
		ch->GetParty()->ChatPacketToAllMember (CHAT_TYPE_DICE_INFO, LC_TEXT ("%s丛捞 林荤困甫 奔妨 %d啊 唱吭嚼聪促. (%d-%d)"), ch->GetName(), n, start, end);
	else
		ch->ChatPacket (CHAT_TYPE_DICE_INFO, LC_TEXT ("寸脚捞 林荤困甫 奔妨 %d啊 唱吭嚼聪促. (%d-%d)"), n, start, end);
#else
	if (ch->GetParty())
		ch->GetParty()->ChatPacketToAllMember (CHAT_TYPE_INFO, LC_TEXT ("%s丛捞 林荤困甫 奔妨 %d啊 唱吭嚼聪促. (%d-%d)"), ch->GetName(), n, start, end);
	else
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("寸脚捞 林荤困甫 奔妨 %d啊 唱吭嚼聪促. (%d-%d)"), n, start, end);
#endif
}

#ifdef ENABLE_NEWSTUFF
ACMD(do_click_safebox)
{
	if ((ch->GetGMLevel() <= GM_PLAYER) && (ch->GetDungeon() || ch->GetWarMap()))
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("You cannot open the safebox in dungeon or at war."));
		return;
	}

	ch->SetSafeboxOpenPosition();
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ShowMeSafeboxPassword");
}
ACMD(do_force_logout)
{
	LPDESC pDesc = DESC_MANAGER::instance().FindByCharacterName(ch->GetName());
	if (!pDesc)
		return;
	pDesc->DelayedDisconnect(0);
}
#endif

ACMD(do_click_mall)
{
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ShowMeMallPassword");
}

ACMD(do_ride)
{
	if (ch->IsDead() || ch->IsStun())
		return;

#ifdef ENABLE_COSTUME_RELATED_FIXES
	LPITEM armor = ch->GetWear(WEAR_BODY);
	int vnumList[4] = { 12000, 12001, 12002, 12003 };

	for (int i = 0; i < _countof(vnumList); i++)
	{
		if (armor && armor->GetVnum() == vnumList[i])
		{
			if (ch->GetEmptyInventory(armor->GetSize()) == -1)
				return;
			else
				ch->UnequipItem(armor);
		}
	}
#endif

	if (ch->GetMapIndex() == 113) 
	{
		return;
	}

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	if (ch->IsPolymorphed() == true) 
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("You cannot perform this action while polymorped."));
		return;
	}

	if (ch->GetWear(WEAR_BODY)) 
	{
		LPITEM armor = ch->GetWear(WEAR_BODY);

		if (armor && (armor->GetVnum() >= 11901 && armor->GetVnum() <= 11904)) {
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Wearing wedding dresses, one cannot ride horses"));
			return;
		}
	}

#ifdef ENABLE_MOUNT_SKIN
#ifdef ENABLE_HIDE_COSTUME
	if (ch->GetWear(WEAR_MOUNT_SKIN) && !ch->IsMountSkinHidden())
#else
	if (ch->GetWear(WEAR_MOUNT_SKIN))
#endif
	{
		CMountSystem* mountSystem = ch->GetMountSystem();
		LPITEM mount = ch->GetWear(WEAR_MOUNT_SKIN);
		DWORD mobVnum = 0;
	
		if (!mountSystem && !mount)
			return;

		if (mount->GetValue(1) != 0)
			mobVnum = mount->GetValue(1);

		if (ch->GetMountVnum())
		{
			if (mountSystem->CountSummoned() == 0)
				mountSystem->Unmount(mobVnum);
		}
		else
		{
			if (mountSystem->CountSummoned() == 1)
				mountSystem->Mount(mobVnum, mount);
		}

		return;
	}
#endif

#ifdef ENABLE_MOUNT_SKIN
	else if (ch->GetWear(WEAR_COSTUME_MOUNT))
#else
	if (ch->GetWear(WEAR_COSTUME_MOUNT))
#endif
	{
		CMountSystem* mountSystem = ch->GetMountSystem();
		LPITEM mount = ch->GetWear(WEAR_COSTUME_MOUNT);
		DWORD mobVnum = 0;

		if (!mountSystem && !mount)
			return;

		if (mount->GetValue(1) != 0)
			mobVnum = mount->GetValue(1);

		if (ch->GetMountVnum())
		{
			if (mountSystem->CountSummoned() == 0)
				mountSystem->Unmount(mobVnum);
		}
		else
		{
			if (mountSystem->CountSummoned() == 1)
			{
				mountSystem->Mount(mobVnum, mount);
			}
		}

		return;
	}
#endif

	if (ch->IsHorseRiding())
	{
		ch->StopRiding();
		return;
	}

	if (ch->GetHorse() != NULL)
	{
		ch->StartRiding();
		return;
	}

	for (UINT i = 0; i < INVENTORY_MAX_NUM; ++i)
	{
		LPITEM item = ch->GetInventoryItem(i);
		if (NULL == item)
			continue;

		if (item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_MOUNT) {
			ch->UseItem(TItemPos(INVENTORY, i));
			return;
		}
	}

	ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("富阑 刚历 家券秦林技夸."));
}

#ifdef ENABLE_CHANNEL_SWITCH_SYSTEM
ACMD(do_change_channel)
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Usage: channel <new channel>"));
		return;
	}

	short channel;
	str_to_number(channel, arg1);

	if (channel < 0 || channel > 6)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Please enter a valid channel."));
		return;
	}

	if (channel == g_bChannel)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("You are already on channel %d."), g_bChannel);
		return;
	}

	if (g_bChannel == 99)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("The map you are at is cross-channel, changing won't have any effect."));
		return;
	}

	if (ch->GetDungeon())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("You cannot change channel while in a dungeon."));
		return;
	}

	if (ch->IsObserverMode())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("You must wait a bit to perform this action."));
		return;
	}

	if (CWarMapManager::instance().IsWarMap(ch->GetMapIndex()))
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("You cannot change channel in war map."));
		return;
	}

	TPacketChangeChannel p;
	p.channel = channel;
	p.lMapIndex = ch->GetMapIndex();

	db_clientdesc->DBPacket(HEADER_GD_FIND_CHANNEL, ch->GetDesc()->GetHandle(), &p, sizeof(p));
}
#endif

#ifdef ENABLE_AFFECT_BUFF_REMOVE
ACMD(do_remove_buff)
{
	if (!ch)
		return;
	
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (ch->WindowOpenCheck() || ch->ActivateCheck())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("CLOSE_WINDOWS_ERROR"));
		return;
	}

	int affect = 0;
	str_to_number(affect, arg1);

	if (affect == 66)
		return;

	CAffect* pAffect = ch->FindAffect(affect);

	if (pAffect)
	{
		if ((ch->m_potionUseTime + 3) > get_global_time())
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Please wait %d seconds and try again"),(ch->m_potionUseTime + 3) - get_global_time());
			// ch->NewChatPacket(WAIT_TO_USE_AGAIN, "%d",(ch->m_potionUseTime + 3) - get_global_time());
			return;
		}

#ifdef ENABLE_AUTO_PICK_UP
		else if (affect == AFFECT_AUTO_PICK_UP)
		{
			if (pAffect->lSPCost == 0)
			{
				pAffect->lSPCost = 1;
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Auto-pickup enabled"));
				ch->SetAutoPickUp(true);
			}
			else
			{
				pAffect->lSPCost = 0;
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Auto-pickup disabled"));
				ch->SetAutoPickUp(false);
			}
			ch->m_potionUseTime = get_global_time();
		}
#endif
		else
		{
			ch->RemoveAffect(affect);
		}
	}
		
}
#endif

#ifdef ENABLE_SPLIT_ITEMS_FAST
ACMD(do_split_selected_item)
{
	if (!ch) 
		return;

	if (quest::CQuestManager::instance().GetEventFlag("split_item") == 1)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Quick item splitting is currently out of use"));
		return;
	}

	const char* line;
	char arg1[256], arg2[256], arg3[256];

	line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	one_argument(line, arg3, sizeof(arg3));

	if (!*arg1 || !*arg2 || !*arg3)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("The input command is invalid"));
		return;
	}

	if (ch->WindowOpenCheck() || ch->ActivateCheck())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("CLOSE_WINDOWS_ERROR1"));
		return;
	}

#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	MAX_COUNT count = 0;
#endif
	WORD cell = 0;
	WORD destCell = 0;

	str_to_number(cell, arg1);
	str_to_number(count, arg2);
	str_to_number(destCell, arg3);

	LPITEM item = ch->GetInventoryItem(cell);
	if (item != NULL)
	{
#ifdef ENABLE_REFRESH_CONTROL
		ch->RefreshControl(REFRESH_INVENTORY, false);
#endif
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
		MAX_COUNT itemCount = item->GetCount();
#endif
		while (itemCount > 0)
		{
			if (count > itemCount)
				count = itemCount;

			int iEmptyPosition = ch->GetEmptyInventoryFromIndex(destCell, item->GetSize());
			if (iEmptyPosition == -1)
				break;

			itemCount -= count;
			ch->MoveItem(TItemPos(INVENTORY, cell), TItemPos(INVENTORY, iEmptyPosition), count
#ifdef ENABLE_HIGHLIGHT_SYSTEM
				, true
#endif
			);
		}
#ifdef ENABLE_REFRESH_CONTROL
		ch->RefreshControl(REFRESH_INVENTORY, true, true);
#endif
	}
}

#ifdef ENABLE_SPECIAL_STORAGE
ACMD(do_split_storage_item)
{
	if (!ch) 
		return;

	if (quest::CQuestManager::instance().GetEventFlag("special_split_item") == 1)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Quick item splitting is currently out of use"));
		return;
	}

	char arg1[256], arg2[256], arg3[256], arg4[256];
	two_arguments(two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3), arg4, sizeof(arg4));

	if (!*arg1 || !*arg2 || !*arg3 || !*arg4)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("The input command is invalid"));
		return;
	}

	if (ch->WindowOpenCheck() || ch->ActivateCheck())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("CLOSE_WINDOWS_ERROR1"));
		return;
	}

#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	MAX_COUNT count = 0;
#endif
	WORD cell = 0;
	WORD destCell = 0;
	BYTE windowtype = 0;

	str_to_number(cell, arg1);
	str_to_number(count, arg2);
	str_to_number(destCell, arg3);
	str_to_number(windowtype, arg4);

	LPITEM item = ch->WindowTypeGetItem(cell, windowtype);
	if (item != NULL)
	{
#ifdef ENABLE_REFRESH_CONTROL
		ch->RefreshControl(REFRESH_INVENTORY, false);
#endif
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
		uint16_t itemCount = item->GetCount();
#endif
		while (itemCount > 0)
		{
			if (count > itemCount)
				count = itemCount;

			int iEmptyPosition = ch->GetEmptyInventoryFromIndex(destCell, item->GetSize(), windowtype);
			if (iEmptyPosition == -1)
				break;

			itemCount -= count;
			ch->MoveItem(TItemPos(windowtype, cell), TItemPos(windowtype, iEmptyPosition), count
#ifdef ENABLE_HIGHLIGHT_SYSTEM
				, true
#endif
			);
		}
#ifdef ENABLE_REFRESH_CONTROL
		ch->RefreshControl(REFRESH_INVENTORY, true, true);
#endif
	}
}
#endif

#ifdef ENABLE_SORT_INVENTORIES
ACMD(do_sort_items)
{
	if (!ch)
		return;

	if (ch->WindowOpenCheck() || ch->ActivateCheck())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("CLOSE_WINDOWS_ERROR1"));
		return;
	}

	if ((ch->m_sortLastUsed + 30) > get_global_time())
	{
		// ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Please wait %d seconds and try again"),(ch->m_sortLastUsed + 30) - get_global_time());
		ch->NewChatPacket(WAIT_TO_USE_AGAIN, "%d", (ch->m_sortLastUsed + 30) - get_global_time());
		return;
	}

	ch->m_sortLastUsed = get_global_time();
#ifdef ENABLE_REFRESH_CONTROL
	ch->RefreshControl(REFRESH_INVENTORY, false);
#endif
	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
	{
		LPITEM item = ch->GetInventoryItem(i);

		if (!item)
			continue;

		if (item->isLocked())
			continue;

		if (item->GetCount() == g_bItemCountLimit)
			continue;

		if (item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
		{
			for (int j = i; j < INVENTORY_MAX_NUM; ++j)
			{
				LPITEM item2 = ch->GetInventoryItem(j);

				if (!item2)
					continue;

				if (item2->isLocked())
					continue;

				if (item2->GetVnum() == item->GetVnum())
				{
					bool bStopSockets = false;

					for (int k = 0; k < ITEM_SOCKET_MAX_NUM; ++k)
					{
						if (item2->GetSocket(k) != item->GetSocket(k))
						{
							bStopSockets = true;
							break;
						}
					}

					if (bStopSockets)
						continue;

#ifdef ENABLE_EXTENDED_COUNT_ITEMS
					MAX_COUNT bAddCount = MIN(g_bItemCountLimit - item->GetCount(), item2->GetCount());
#else
					BYTE bAddCount = MIN(g_bItemCountLimit - item->GetCount(), item2->GetCount());
#endif

					item->SetCount(item->GetCount() + bAddCount);
					item2->SetCount(item2->GetCount() - bAddCount);
					continue;
				}
			}
		}
	}
#ifdef ENABLE_REFRESH_CONTROL
	ch->RefreshControl(REFRESH_INVENTORY, true,true);
#endif
}
ACMD(do_sort_special_storage)
{

	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;


	if ((ch->m_sortLastUsed + 30) > get_global_time())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Please wait %d seconds and try again"),(ch->m_sortLastUsed + 30) - get_global_time());
		// ch->NewChatPacket(WAIT_TO_USE_AGAIN, "%d", (ch->m_sortLastUsed + 30) - get_global_time());
		return;
	}

	if (ch->WindowOpenCheck() || ch->ActivateCheck())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("CLOSE_WINDOWS_ERROR1"));
		return;
	}

	BYTE windowtype = 0;
	str_to_number(windowtype, arg1);
	ch->m_sortLastUsed = get_global_time();
#ifdef ENABLE_REFRESH_CONTROL
	ch->RefreshControl(REFRESH_INVENTORY, false);
#endif
#ifdef ENABLE_SPECIAL_STORAGE
	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
#else
	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
#endif
	{
		LPITEM item = ch->WindowTypeGetItem(i,windowtype);

		if (!item)
			continue;

		if (item->isLocked())
			continue;

		if (item->GetCount() == g_bItemCountLimit)
			continue;

		if (item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
		{
#ifdef ENABLE_SPECIAL_STORAGE
			for (int j = i; j < SPECIAL_INVENTORY_MAX_NUM; ++j)
#else
			for (int j = i; j < INVENTORY_MAX_NUM; ++j)
#endif
			{
				LPITEM item2 = ch->WindowTypeGetItem(j, windowtype);

				if (!item2)
					continue;

				if (item2->isLocked())
					continue;

				if (item2->GetVnum() == item->GetVnum())
				{
					bool bStopSockets = false;

					for (int k = 0; k < ITEM_SOCKET_MAX_NUM; ++k)
					{
						if (item2->GetSocket(k) != item->GetSocket(k))
						{
							bStopSockets = true;
							break;
						}
					}

					if (bStopSockets)
						continue;

#ifdef ENABLE_EXTENDED_COUNT_ITEMS
					MAX_COUNT bAddCount = MIN(g_bItemCountLimit - item->GetCount(), item2->GetCount());
#else
					BYTE bAddCount = MIN(g_bItemCountLimit - item->GetCount(), item2->GetCount());
#endif

					item->SetCount(item->GetCount() + bAddCount);
					item2->SetCount(item2->GetCount() - bAddCount);

					continue;
				}
			}
		}
	}
#ifdef ENABLE_REFRESH_CONTROL
	ch->RefreshControl(REFRESH_INVENTORY, true, true);
#endif
}
#endif

#endif

#ifdef ENABLE_GLOBAL_RANKING
ACMD(do_request_ranking_list)
{
	if (!ch)
		return;
	
	CStatisticsRanking::Instance().RequestRankingList(ch);
}
#endif

#ifdef ENABLE_CHAR_SETTINGS
ACMD(do_SetCharSettings)
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;
	BYTE settingtype = 0;
	str_to_number(settingtype, arg1);

	switch (settingtype)
	{
		case SETTINGS_ANTI_EXP:
		{
			if (ch->GetCharSettings().antiexp)
			{
				ch->SetSettingsAntiExp(false);
			}
			else
			{
				ch->SetSettingsAntiExp(true);
			}
			break;
		}
#ifdef ENABLE_HIDE_COSTUME_SYSTEM
		case SETTINGS_HIDE_COSTUME_HAIR:
		{
			ch->SetHideCostumeState(SETTINGS_HIDE_COSTUME_HAIR, ch->IsCostumeHairHidden() ? 0 : 1);
			ch->UpdatePacket();
			break;
		}
		case SETTINGS_HIDE_COSTUME_BODY:
		{
			ch->SetHideCostumeState(SETTINGS_HIDE_COSTUME_BODY, ch->IsCostumeBodyHidden() ? 0 : 1);
			ch->UpdatePacket();
			break;
		}
		case SETTINGS_HIDE_COSTUME_WEAPON:
		{
			ch->SetHideCostumeState(SETTINGS_HIDE_COSTUME_WEAPON, ch->IsCostumeWeaponHidden() ? 0 : 1);
			ch->UpdatePacket();
			break;
		}
		case SETTINGS_HIDE_COSTUME_ACCE:
		{
			ch->SetHideCostumeState(SETTINGS_HIDE_COSTUME_ACCE, ch->IsCostumeAcceHidden() ? 0 : 1);
			ch->UpdatePacket();
			break;
		}
		case SETTINGS_HIDE_COSTUME_AURA:
		{
			ch->SetHideCostumeState(SETTINGS_HIDE_COSTUME_AURA, ch->IsCostumeAuraHidden() ? 0 : 1);
			ch->UpdatePacket();
			break;
		}
		case SETTINGS_HIDE_COSTUME_CROWN:
		{
			ch->SetHideCostumeState(SETTINGS_HIDE_COSTUME_CROWN, ch->IsCrownHidden() ? 0 : 1);
			ch->UpdatePacket();
			break;
		}
#endif
#ifdef ENABLE_STOP_CHAT
		case SETTINGS_STOP_CHAT:
		{
			if ((ch->m_stopChatCooldown + 10) > get_global_time())
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Please wait %d seconds and try again"),(ch->m_stopChatCooldown + 10) - get_global_time());
				return;
			}

			if (ch->GetStopChatState() == 1)
			{
				ch->SetStopChatState(0);
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Chatsettingschanged"));
				return;
			}
			else
			{
				ch->SetStopChatState(1);
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Chatsettingschanged_NO"));
				return;
			}
			// ch->SetStopChatState(ch->GetStopChatState() ? 0 : 1);
			// ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Chat settings changed"));

			ch->m_stopChatCooldown = get_global_time();
			break;
		}
#endif
		default: return;
	}
	ch->SendCharSettingsPacket(settingtype);
}
#endif


#ifdef ENABLE_DISTANCE_NPC
#include "shop.h"
#include "shop_manager.h"
ACMD(do_distance_npc)
{
	if (quest::CQuestManager::instance().GetEventFlag("distance_shop_disable") == 1)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("split_item shopping is currently suspended"));
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1) return;

	DWORD vnum = 0;
	str_to_number(vnum, arg1);

	if (!ch->IsPC()) return;

	if (ch->WindowOpenCheck() || ch->ActivateCheck())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("CLOSE_WINDOWS_ERROR1"));
		return;
	}

	if (ch->GetDungeon())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("You cannot open the storage in a dungeon or battleground"));
		return;
	}

	LPSHOP shop = CShopManager::instance().Get(vnum);
	if (!shop) return;

	ch->SetShopOwner(ch);
	shop->AddGuest(ch, 0, false);
}
#endif

#ifdef ENABLE_FARM_BLOCK
ACMD(do_set_farm_block)
{
	if (!ch)
		return;

	const int pulse = thecore_pulse();
	const int limit_time = 10 * passes_per_sec;

	if ((pulse - ch->GetFarmBlockSetTime()) < limit_time)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Please wait 10 seconds to try"));
		return;
	}
	CHwidManager::instance().SetFarmBlockGD(ch);
}
#endif

#ifdef ENABLE_PASSIVE_SKILLS
ACMD(do_passiveread)
{
	if (!ch)
		return;

	if (!ch->GetDesc())
		return;

	if (!ch->CanWarp())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Can't do that now."));
		return;
	}

	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	if (!*arg1 || !*arg2)
		return;

	int skillIdx = atoi(arg2);
	bool allRead = atoi(arg1);

	ch->PassiveSkillUpgrade(skillIdx, allRead);
}
#endif

#ifdef ENABLE_FAST_SOUL_STONE_SYSTEM
EVENTINFO(SoulStoneEvent)
{
	LPCHARACTER	ch;
	long skillindex;
	bool sage;
	bool align;

	SoulStoneEvent()
		: ch(nullptr)
		, skillindex(0)
		, sage(false)
		, align(false)
	{
	}
};

EVENTFUNC(soulStone_Event)
{
	SoulStoneEvent* info = dynamic_cast<SoulStoneEvent*>(event->info);

	if (info == nullptr)
	{
		sys_err("soulStone_Event> <Factor> nullptr pointer");
		return 0;
	}

	LPCHARACTER ch = info->ch;

	if (!ch)
		return 0;

	bool sage = info->sage;
	bool alignment = info->align;
	long skillindex = info->skillindex;


	int soulStoneVnum = sage ? 24000 : 50513;
	int ruhVnum = sage ? 24013 : 71001;
	int munzeviVnum = sage ? 24014 : 71094;

	if (!ch->CountSpecifyItemText(soulStoneVnum, 1))
		return 0;
	if (!ch->CountSpecifyItemText(ruhVnum, 1))
		return 0;
	if (!ch->CountSpecifyItemText(munzeviVnum, 1))
		return 0;

	int zenCount = 50;
	int alignmentDownPoint = 0;
	if (!alignment)
	{
		int alignmentPoint = ch->GetAlignment() / 10;
		zenCount = MIN(9, ch->GetSkillLevel(skillindex) - (sage ? 40 : 30));

		if (alignmentPoint > 0)
		{
			if (alignmentPoint > zenCount * 500)
			{
				alignmentDownPoint = (zenCount * 500) * (-10);
				zenCount = 0;
			}
			else
			{
				alignmentDownPoint = alignmentPoint * (-10);
				zenCount -= alignmentPoint / 500;
			}
		}
	}

	if (zenCount > 0 && !ch->CountSpecifyItemText(70102, zenCount))
		return 0;

	if (sage)
	{
		if (!ch->LearnSageMasterSkill(skillindex))
			return 0;
	}
	else
	{
		if (!ch->LearnGrandMasterSkill(skillindex))
			return 0;
	}

	ch->RemoveSpecifyItem(munzeviVnum, 1);
	ch->RemoveSpecifyItem(ruhVnum, 1);
	ch->RemoveSpecifyItem(soulStoneVnum, 1);
	ch->RemoveSpecifyItem(70102, zenCount);

	if (alignmentDownPoint != 0)
		ch->UpdateAlignment(alignmentDownPoint);

#ifdef ENABLE_BUFFI_SYSTEM
	if (skillindex >= SKILL_BUFFI_1 && skillindex <= SKILL_BUFFI_5)
	{
		if (ch->GetBuffiSystem() && ch->GetBuffiSystem()->IsSummon())
		{
			ch->GetBuffiSystem()->UpdateSkillLevel(skillindex);
		}
	}
#endif
	return 1;
}

ACMD(do_ruhoku)
{
	char arg1[256], arg2[256], arg3[256], arg4[256];

	four_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2), arg3, sizeof(arg3), arg4, sizeof(arg4));

	if (!*arg1 || !*arg2 || !*arg3 || !*arg4)
		return;

	bool allRead = false;
	bool sage = false;
	bool alignment = false;
	long skillindex = 0;

	str_to_number(allRead, arg1);
	str_to_number(sage, arg2);
	str_to_number(skillindex, arg3);
	str_to_number(alignment, arg4);

	CSkillProto* pkSk = CSkillManager::instance().Get(skillindex);
	if (!pkSk)
		return;

	if (!ch)
		return;

	if (!ch->CanWarp())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Can't do that now."));
		return;
	}

	if (ch->GetSkillGroup() == 0)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("You cannot read skill books if you have not joined a sect"));
		return;
	}

	if (ch->GetSkillLevel(skillindex) >= (sage ? 50 : 40))
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("This skill book can no longer increase the skill level"));
		return;
	}

	if (allRead == 1)
	{
		int soulStoneVnum = sage ? 24000 : 50513;

		int ruhVnum = sage ? 24013 : 71001;
		int munzeviVnum = sage ? 24014 : 71094;

		if (!ch->CountSpecifyItemText(soulStoneVnum, 1))
			return;
		if (!ch->CountSpecifyItemText(ruhVnum, 1))
			return;
		if (!ch->CountSpecifyItemText(munzeviVnum, 1))
			return;

		int zenCount = 50;
		int alignmentDownPoint = 0;
		if (!alignment)
		{
			int alignmentPoint = ch->GetAlignment() / 10;
			zenCount = MIN(9, ch->GetSkillLevel(skillindex) - (sage ? 40 : 30));

			if (alignmentPoint > 0)
			{
				if (alignmentPoint > zenCount * 500)
				{
					alignmentDownPoint = (zenCount * 500) * (-10);
					zenCount = 0;
				}
				else
				{
					alignmentDownPoint = alignmentPoint * (-10);
					zenCount -= alignmentPoint / 500;
				}
			}
		}

		if (zenCount > 0 && !ch->CountSpecifyItemText(70102, zenCount))
			return;
			
		if (sage)
		{
			if (!ch->LearnSageMasterSkill(skillindex))
				return;
		}
		else
		{
			if (!ch->LearnGrandMasterSkill(skillindex))
				return;
		}

		ch->RemoveSpecifyItem(ruhVnum, 1);
		ch->RemoveSpecifyItem(munzeviVnum, 1);
		ch->RemoveSpecifyItem(soulStoneVnum, 1);
		ch->RemoveSpecifyItem(70102, zenCount);

		if (alignmentDownPoint != 0)
			ch->UpdateAlignment(alignmentDownPoint);
#ifdef ENABLE_BUFFI_SYSTEM
		if (skillindex >= SKILL_BUFFI_1 && skillindex <= SKILL_BUFFI_5)
		{
			if (ch->GetBuffiSystem() && ch->GetBuffiSystem()->IsSummon())
			{
				ch->GetBuffiSystem()->UpdateSkillLevel(skillindex);
			}
		}
#endif
	}

	else
	{
		LPEVENT fastEvent = ch->GetFastReadEvent();
		if (fastEvent)
			event_cancel(&fastEvent);

		SoulStoneEvent* info = AllocEventInfo<SoulStoneEvent>();
		info->ch = ch;
		info->skillindex = skillindex;
		info->sage = sage;
		info->align = alignment;
		ch->SetFastReadEvent(event_create(soulStone_Event, info, PASSES_PER_SEC(1)));
	}
	return;
}
#endif


#ifdef ENABLE_EVENT_MANAGER
ACMD(do_event_manager)
{
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);

	if (vecArgs.size() < 2)
	{
		return;
	}
	else if (vecArgs[1] == "info")
	{
		CHARACTER_MANAGER::Instance().SendDataPlayer(ch);
	}
	else if (vecArgs[1] == "remove")
	{
		if (!ch->IsGM())
			return;

		if (vecArgs.size() < 3)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("put the event index!!"));
			return; 
		}

		BYTE removeIndex = 0;
		str_to_number(removeIndex, vecArgs[2].c_str());

		if(CHARACTER_MANAGER::Instance().CloseEventManuel(removeIndex))
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("successfuly remove!"));
		}
		else
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("dont has any event!"));
		}
	}
	else if (vecArgs[1] == "update")
	{
		if (!ch->IsGM())
			return;

		const BYTE subHeader = EVENT_MANAGER_UPDATE;
		db_clientdesc->DBPacket(HEADER_GD_EVENT_MANAGER, 0, &subHeader, sizeof(BYTE));
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("successfully update!"));
	}
}
#endif

#ifdef ENABLE_ITEMSHOP
ACMD(do_ishop)
{
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);

	if (vecArgs.size() < 2)
	{
		return;
	}
	else if (vecArgs[1] == "data")
	{
		if (ch->GetProtectTime("itemshop.load") == 1)
			return;
		ch->SetProtectTime("itemshop.load", 1);

		if (vecArgs.size() < 3)
			return;

		int updateTime = 0;
		str_to_number(updateTime, vecArgs[2].c_str());
		CHARACTER_MANAGER::Instance().LoadItemShopData(ch, CHARACTER_MANAGER::Instance().GetItemShopUpdateTime() != updateTime);
	}
	else if (vecArgs[1] == "log")
	{
		if (ch->GetProtectTime("itemshop.log") == 1)
			return;
	
		ch->SetProtectTime("itemshop.log", 1);

		CHARACTER_MANAGER::Instance().LoadItemShopLog(ch);
	}
	else if (vecArgs[1] == "buy")
	{
		if (vecArgs.size() < 4)
			return;

		int itemID = 0;
		str_to_number(itemID, vecArgs[2].c_str());
		int itemCount = 0;
		str_to_number(itemCount, vecArgs[3].c_str());
		CHARACTER_MANAGER::Instance().LoadItemShopBuy(ch, itemID, itemCount);
	}	
	else if (vecArgs[1] == "u_buy")
	{
		if (vecArgs.size() < 4)
			return;

		int itemID = 0;
		str_to_number(itemID, vecArgs[2].c_str());
		int itemCount = 0;
		str_to_number(itemCount, vecArgs[3].c_str());
		CHARACTER_MANAGER::Instance().LoadItemShopBuy(ch, itemID, itemCount, true);
	}
	else if (vecArgs[1] == "wheel")
	{
		if (vecArgs.size() < 3)
		{
			return;
		}
		else if (vecArgs[2] == "start")
		{
			if (vecArgs.size() < 4)
				return;

			BYTE ticketType = 0;

			str_to_number(ticketType, vecArgs[3].c_str());
			if (ticketType > 1)
				return;

			else if (ch->GetProtectTime("WheelWorking") != 0)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The lottery has started!"));
				return;
			}
			if (ticketType == 0)
			{
				if (ch->CountSpecifyItem(wheelSpinTicketVnum) - wheelSpinWithTicketPrice < 0)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your tickets are not enough!"));
					return;
				}
				ch->RemoveSpecifyItem(wheelSpinTicketVnum, wheelSpinWithTicketPrice);
			}
			else if (ticketType == 1)
			{
				long long dragonCoin = ch->GetDragonCoin();

				if(dragonCoin - wheelDCPrice < 0)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Not enough points"));
					return;
				}
		
				ch->SetDragonCoin(dragonCoin- wheelDCPrice);

				ch->ChatPacket(CHAT_TYPE_COMMAND, "SetDragonCoin %lld", dragonCoin - wheelDCPrice);

				BYTE subIndex = ITEMSHOP_LOG_ADD;

				DWORD accountID = ch->GetDesc()->GetAccountTable().id;
				char playerName[CHARACTER_NAME_MAX_LEN+1];
				char ipAdress[16];

				strlcpy(playerName,ch->GetName(),sizeof(playerName));
				strlcpy(ipAdress,ch->GetDesc()->GetHostName(),sizeof(ipAdress));
				db_clientdesc->DBPacketHeader(HEADER_GD_ITEMSHOP, ch->GetDesc()->GetHandle(), sizeof(BYTE)+sizeof(DWORD)+sizeof(playerName)+sizeof(ipAdress));
				db_clientdesc->Packet(&subIndex, sizeof(BYTE));
				db_clientdesc->Packet(&accountID, sizeof(DWORD));
				db_clientdesc->Packet(&playerName, sizeof(playerName));
				db_clientdesc->Packet(&ipAdress, sizeof(ipAdress));

				if (ch->GetProtectTime("itemshop.log") == 1)
				{
					char       timeText[21];
					time_t     now = time(0);
					struct tm  tstruct = *localtime(&now);
					strftime(timeText, sizeof(timeText), "%Y-%m-%d %X", &tstruct);
					ch->ChatPacket(CHAT_TYPE_COMMAND, "ItemShopAppendLog %s %d %s %s 1 1 %u", timeText, time(0), playerName, ipAdress, wheelDCPrice);
				}
			}
			//倚天2梦天下
			std::vector<std::pair<long, long>> m_important_item = {
				 {200300,1},	//轮盘票

			};
				std::map<std::pair<long, long>, int> m_normal_item = {
				 {{27003,200},50},	//血瓶(大)
				 {{27006,200},50},	//魔法瓶(大)
				 {{71085,1},30},	//追加物品属性秘笈
				 {{71084,1},30},	//物品属性转换秘笈
				 {{71001,1},30},	//主眼秘笈
				 {{71094,1},30},	//先人的教训
				 {{71095,1},30},	//通行证明书

				 {{71027,1},30},	//龙神的生命
				 {{71028,1},30},	//龙神的攻击
				 {{71030,1},30},	//龙神的防御
				 {{27987,1},15},	//珍珠蚌
				 {{70039,1},4},		//高手指南针
				 {{90032,1},3},		//发型礼盒
				 {{70043,1},5},		//夺宝手套

				 {{80019,1},5},	//点卷(500)
				 {{80020,1},3},	//点卷(1000)
				 {{80021,1},2},	//点卷(2000)
				 {{80022,1},1},	//点卷(5000)
				 {{34083,1},1},	//紫色飞龙召唤牌
			};
			
			//丐世英雄和华夏服设置
			// std::vector<std::pair<long, long>> m_important_item = {
				 // {200300,1},	//轮盘票
				 // {200300,2},	//轮盘票
			// };
				// std::map<std::pair<long, long>, int> m_normal_item = {
				 // {{200018,2},30},	//被动技能书宝箱
				 // {{200018,3},20},	//被动技能书宝箱
				 // {{71084,10},65},	//物品属性转换秘笈
				 // {{71084,20},50},	//物品属性转换秘笈
				 // {{71084,50},50},	//物品属性转换秘笈
				 // {{27987,1},50},	//珍珠蚌
				 // {{27987,2},40},	//珍珠蚌
				 // {{71094,2},50},	//先人的教训
				 // {{71094,5},40},	//先人的教训
				 // {{71001,2},50},	//主眼秘笈
				 // {{71001,5},40},	//主眼秘笈
				 // {{71095,1},40},	//通信证明书
				 // {{71030,1},40},	//龙神的防御
				 // {{71027,1},40},	//龙神的生命
				 // {{71028,1},40},	//龙神的攻击
				 // {{71044,1},40},	//双倍伤害丸
				 // {{25040,1},40},	//永恒铸件
				 // {{70043,1},30},	//夺宝手套
				 // {{34086,1}, 5},	//灰色巨龙召唤牌
			// };
			
			//龙驹怀旧服设置
			// std::vector<std::pair<long, long>> m_important_item = {
				 // {71032,1},	//龙神的祝福书
				 // {70039,1},	//高手指南针
			// };
				// std::map<std::pair<long, long>, int> m_normal_item = {
				 // {{71084,5},80},	//物品属性转换秘笈
				 // {{71084,10},60},	//物品属性转换秘笈
				 // {{71094,2},50},	//先人的教训
				 // {{71094,5},40},	//先人的教训
				 // {{71001,2},50},	//主眼秘笈
				 // {{71001,5},40},	//主眼秘笈
				 // {{70024,1},80},	//祝福之珠
				 // {{71030,1},60},	//神龙药水
				 // {{71027,1},60},	//神龙药水
				 // {{71028,1},60},	//神龙药水
				 // {{71045,1},60},	//无视防御丸
				 // {{71044,1},60},	//双倍无视丸
				 // {{200018,2},20},	//被动技能书宝箱
				 // {{200018,5},20},	//被动技能书宝箱
				 // {{25040,2},80},	//永恒铸件
				 // {{25040,5},70},	//永恒铸件
				 // {{71107,1},40},	//仙桃
				 // {{24010,3},25},	//统帅技能书
				 // {{24011,3},25},	//采矿技能书
				 // {{24012,3},25},	//钓鱼技能书 
				 // {{70043,1},35},	//夺宝手套
				 // {{27987,2},80},	//珍珠蚌
				 // {{72726,1},15},	//火龙的祝福(7天)
				 // {{72730,1},15},	//水龙的祝福(7天)
			// };
			//伐天之战设置
			// std::vector<std::pair<long, long>> m_important_item = {
				 // {71032,1},	//龙神的祝福书
				 // {70039,1},	//高手指南针
			// };
				// std::map<std::pair<long, long>, int> m_normal_item = {
				 // {{71084,5},80},	//物品属性转换秘笈
				 // {{71084,10},60},	//物品属性转换秘笈
				 // {{71094,2},50},	//先人的教训
				 // {{71094,5},40},	//先人的教训
				 // {{71001,2},50},	//主眼秘笈
				 // {{71001,5},40},	//主眼秘笈
				 // {{70024,1},80},	//祝福之珠
				 // {{71030,1},60},	//神龙药水
				 // {{71027,1},60},	//神龙药水
				 // {{71028,1},60},	//神龙药水
				 // {{71045,1},60},	//无视防御丸
				 // {{71044,1},60},	//双倍无视丸
				 // {{200018,2},20},	//被动技能书宝箱
				 // {{200018,5},20},	//被动技能书宝箱
				 // {{34081,1},5},		//蛟龙召唤牌
				 // {{34048,1},5},		//双凤召唤牌
				 // {{25040,2},80},	//永恒铸件
				 // {{25040,5},70},	//永恒铸件
				 // {{71107,1},40},	//仙桃
				 // {{200500,1},40},	//真亡灵塔门票
				 // {{200501,1},40},	//蜘蛛巢穴门票
				 // {{200505,1},40},	//天意洞窟门票 
				 // {{70043,1},35},	//夺宝手套
				 // {{27987,2},80},	//珍珠蚌
				 // {{72726,1},15},	//火龙的祝福(7天)
				 // {{72730,1},15},	//水龙的祝福(7天)
			// };
			
			std::vector<std::pair<long, long>> m_send_items;
			if (m_important_item.size())
			{
				int random = number(0,m_important_item.size()-1);
				m_send_items.emplace_back(m_important_item[random].first, m_important_item[random].second);
			}

			while (true)
			{
				for (auto it = m_normal_item.begin(); it != m_normal_item.end(); ++it)
				{
					int randomEx = number(0, 4);
					if (randomEx == 4)
					{
						int random = number(0, 100);
						if (it->second >= random)
						{
							auto itFind = std::find(m_send_items.begin(), m_send_items.end(), it->first);
							if (itFind == m_send_items.end())
							{
								m_send_items.emplace_back(it->first.first, it->first.second);
								if (m_send_items.size() >= 10)
									break;
							}
						}
					}
				}
				if (m_send_items.size() >= 10)
					break;
			}

			std::string cmd_wheel = "";

			if (m_send_items.size())
			{
				for (auto it = m_send_items.begin(); it != m_send_items.end(); ++it)
				{
					cmd_wheel += std::to_string(it->first);
					cmd_wheel += "|";
					cmd_wheel += std::to_string(it->second);
					cmd_wheel += "#";
				}
			}

			int luckyWheel = number(0, 9);
			if (luckyWheel == 0)
				if (number(0, 1) == 0)
					luckyWheel = number(0, 9);

			ch->SetProtectTime("WheelLuckyIndex", luckyWheel);
			ch->SetProtectTime("WheelLuckyItemVnum", m_send_items[luckyWheel].first);
			ch->SetProtectTime("WheelLuckyItemCount", m_send_items[luckyWheel].second);

			ch->SetProtectTime("WheelWorking", 1);

			ch->ChatPacket(CHAT_TYPE_COMMAND, "SetWheelItemData %s", cmd_wheel.c_str());
			ch->ChatPacket(CHAT_TYPE_COMMAND, "OnSetWhell %d", luckyWheel);
		}
		else if (vecArgs[2] == "done")
		{
			if (ch->GetProtectTime("WheelWorking") == 0)
				return;

			ch->AutoGiveItem(ch->GetProtectTime("WheelLuckyItemVnum"), ch->GetProtectTime("WheelLuckyItemCount"));
			ch->ChatPacket(CHAT_TYPE_COMMAND, "GetWheelGiftData %d %d", ch->GetProtectTime("WheelLuckyItemVnum"), ch->GetProtectTime("WheelLuckyItemCount"));
			ch->SetProtectTime("WheelLuckyIndex", 0);
			ch->SetProtectTime("WheelLuckyItemVnum", 0);
			ch->SetProtectTime("WheelLuckyItemCount", 0);
			ch->SetProtectTime("WheelWorking", 0);
		}

	}
}
#endif

#ifdef ENABLE_NAMING_SCROLL
ACMD(do_namingscroll)
{
	if (!ch)
		return;

	if (!ch->GetDesc())
		return;

	if (!ch->CanWarp())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Can't send now"));
		return;
	}

	const char* line;
	char arg1[256], arg2[256], arg3[256];

	line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	one_argument(line, arg3, sizeof(arg3));

	if (!*arg1 || !*arg2 || !*arg3) // arg 1 = pos in inv, arg 2 = scrolltype, arg3 = name
		return;

	BYTE scrollType = 0;

	int bCell = 0;
	str_to_number(bCell, arg1); // for checking the item in the inventory

	LPITEM namingScroll = ch->GetInventoryItem(bCell);

	if (!namingScroll)
		return;

	str_to_number(scrollType, arg2);

	for (int i = 0; i < strlen(arg3); ++i)
	{
		if (arg3[i] == '%' ||
			arg3[i] == '/' ||
			arg3[i] == '>' ||
			arg3[i] == '|' ||
			arg3[i] == ';' ||
			arg3[i] == ':' ||
			arg3[i] == '}' ||
			arg3[i] == '{' ||
			arg3[i] == '[' ||
			arg3[i] == ']' ||
			arg3[i] == '#' ||
			arg3[i] == '@' ||
			arg3[i] == '^' ||
			arg3[i] == '&' ||
			arg3[i] == '\\')
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("The name entered is invalid"));
			return;
		}
	}

	if (scrollType == MOUNT_NAME_NUM+1)
	{
		if (ch->IsRiding())
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("You need to take back the mount to change the mount name"));
			return;
		}

		if (namingScroll->GetVnum() != 24050 && namingScroll->GetVnum() != 24051 && namingScroll->GetVnum() != 24052 && namingScroll->GetVnum() != 24053 && namingScroll->GetVnum() != 64000)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Insufficient materials"));
			return;
		}

		CAffect* mountAffect = ch->FindAffect(AFFECT_NAMING_SCROLL_MOUNT);
		if (mountAffect)
		{
			if (mountAffect->lApplyValue == namingScroll->GetValue(NAMING_SCROLL_BONUS_RATE_VALUE))
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("You have achieved this effect"));
				return;
			}
			ch->RemoveAffect(AFFECT_NAMING_SCROLL_MOUNT);
		}

		ch->AddAffect(AFFECT_NAMING_SCROLL_MOUNT, 0, namingScroll->GetValue(NAMING_SCROLL_BONUS_RATE_VALUE),
			0, namingScroll->GetValue(NAMING_SCROLL_TIME_VALUE), 0, false);

		LPITEM wearMount = ch->GetWear(WEAR_COSTUME_MOUNT);
		if (wearMount)
		{
			if (ch->GetWear(WEAR_MOUNT_SKIN))
				wearMount = ch->GetWear(WEAR_MOUNT_SKIN);

			ch->MountUnsummon(wearMount);
		}

		char szName[CHARACTER_NAME_MAX_LEN + 1];
		DBManager::instance().EscapeString(szName, sizeof(szName), arg3, strlen(arg3));

		DBManager::instance().DirectQuery("UPDATE player SET naming_scroll_mount = '%s' WHERE id = %u", szName, ch->GetPlayerID()); // setting name for playerid cuz we want only one player have this shit
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("The mount has been named successfully!"));
		ITEM_MANAGER::Instance().RemoveItem(namingScroll);
		ch->NamingScrollNameClear(MOUNT_NAME_NUM);
		ch->MountSummon(wearMount);
		ch->ComputePoints();
	}
	else if (scrollType == PET_NAME_NUM+1)
	{

		if (namingScroll->GetVnum() != 24054 && namingScroll->GetVnum() != 24055 && namingScroll->GetVnum() != 24056 && namingScroll->GetVnum() != 24057 && namingScroll->GetVnum() != 64001)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Insufficient materials"));
			return;
		}

		CAffect* mountAffect = ch->FindAffect(AFFECT_NAMING_SCROLL_PET);
		if (mountAffect)
		{
			if (mountAffect->lApplyValue == namingScroll->GetValue(NAMING_SCROLL_BONUS_RATE_VALUE))
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("You have achieved this effect"));
				return;
			}
			ch->RemoveAffect(AFFECT_NAMING_SCROLL_PET);
		}

		ch->AddAffect(AFFECT_NAMING_SCROLL_PET, 0, namingScroll->GetValue(NAMING_SCROLL_BONUS_RATE_VALUE),
			0, namingScroll->GetValue(NAMING_SCROLL_TIME_VALUE), 0, false);

		LPITEM wearPet = ch->GetWear(WEAR_PET);
		if (wearPet)
		{
			if (ch->GetWear(WEAR_PET_SKIN))
				wearPet = ch->GetWear(WEAR_PET_SKIN);

			ch->PetUnsummon(wearPet);
		}

		char szName[CHARACTER_NAME_MAX_LEN + 1];
		DBManager::instance().EscapeString(szName, sizeof(szName), arg3, strlen(arg3));
		DBManager::instance().DirectQuery("UPDATE player SET naming_scroll_pet = '%s' WHERE id = %u", szName, ch->GetPlayerID()); // setting name for playerid cuz we want only one player have this shit
		
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Pet naming has been successful!"));
		ITEM_MANAGER::Instance().RemoveItem(namingScroll);
		ch->NamingScrollNameClear(PET_NAME_NUM);
		ch->PetSummon(wearPet);
		ch->ComputePoints();
	}
#ifdef ENABLE_BUFFI_SYSTEM
	else if (scrollType == BUFFI_NAME_NUM + 1)
	{

		if (namingScroll->GetVnum() != 24058 && namingScroll->GetVnum() != 24059 && namingScroll->GetVnum() != 24060 && namingScroll->GetVnum() != 24061 && namingScroll->GetVnum() != 64002)
			return;

		CAffect* mountAffect = ch->FindAffect(AFFECT_NAMING_SCROLL_BUFFI);
		if (mountAffect)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("You have achieved this effect"));
			return;
		}

		ch->AddAffect(AFFECT_NAMING_SCROLL_BUFFI, 0, 0,
			0, namingScroll->GetValue(NAMING_SCROLL_TIME_VALUE), 0, false);

		if (ch->GetBuffiSystem() && ch->GetBuffiSystem()->IsSummon())
		{
			ch->GetBuffiSystem()->UnSummon();
			//ch->GetBuffiSystem()->Summon();//FIX BUG BUFFI NONAME
		}

		char szName[CHARACTER_NAME_MAX_LEN + 1];
		DBManager::instance().EscapeString(szName, sizeof(szName), arg3, strlen(arg3));
		DBManager::instance().DirectQuery("UPDATE player SET naming_scroll_buffi = '%s' WHERE id = %u", szName, ch->GetPlayerID()); // setting name for playerid cuz we want only one player have this shit

		ITEM_MANAGER::Instance().RemoveItem(namingScroll);
		ch->NamingScrollNameClear(BUFFI_NAME_NUM);
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Renaming successful"));
		// ch->GetBuffiSystem()->Summon();//FIX BUG BUFFI NONAME
	}
#endif
}
#endif

#ifdef ENABLE_SPECIAL_STORAGE
ACMD(do_transfer_inv_storage)
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD dwPos = 0;

	str_to_number(dwPos, arg1);

	if ((ch->GetExchange() || ch->IsOpenSafebox() || ch->GetShopOwner()) || ch->IsCubeOpen() || ch->IsAcceOpened() || ch->IsOpenOfflineShop()
#ifdef ENABLE_AURA_SYSTEM
		|| ch->isAuraOpened(true) || ch->isAuraOpened(false)
#endif
#ifdef ENABLE_6TH_7TH_ATTR
		|| ch->Is67AttrOpen()
#endif
		)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("促弗 芭贰吝(芒绊,背券,惑痢)俊绰 俺牢惑痢阑 荤侩且 荐 绝嚼聪促."));
			return;
		}

	if (dwPos >= 0 && dwPos <= INVENTORY_MAX_NUM)
	{
		LPITEM item = ch->GetInventoryItem(dwPos);

		if (item)
		{
			if (item->IsUpgradeItem() || item->IsBook() || item->IsStone() || item->IsChest())
			{
				BYTE window_type = ch->VnumGetWindowType(item->GetVnum());
				int iEmptyPos = ch->WindowTypeToGetEmpty(window_type, item);

				if (iEmptyPos != -1)
					ch->MoveItem(TItemPos(INVENTORY, item->GetCell()), TItemPos(window_type, iEmptyPos), item->GetCount()
#ifdef ENABLE_HIGHLIGHT_SYSTEM
						, true
#endif
					);
				else
					ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Insufficient storage space for categories"));
			}
		}
	}
}

ACMD(do_transfer_storage_inv)
{
	if (!ch)
		return;

	char arg1[256];
	char arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
		return;

	DWORD dwPos = 0;
	BYTE window_type = 0;

	str_to_number(dwPos, arg1);
	str_to_number(window_type, arg2);

	if ((ch->GetExchange() || ch->IsOpenSafebox() || ch->GetShopOwner()) || ch->IsCubeOpen() || ch->IsAcceOpened() || ch->IsOpenOfflineShop()
#ifdef ENABLE_AURA_SYSTEM
		|| ch->isAuraOpened(true) || ch->isAuraOpened(false)
#endif
#ifdef ENABLE_6TH_7TH_ATTR
		|| ch->Is67AttrOpen()
#endif
		)
		{

			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("促弗 芭贰吝(芒绊,背券,惑痢)俊绰 俺牢惑痢阑 荤侩且 荐 绝嚼聪促."));
			return;
		}

	if (dwPos >= 0 && dwPos <= INVENTORY_MAX_NUM && window_type >= UPGRADE_INVENTORY && window_type <= CHEST_INVENTORY)
	{
		LPITEM item = ch->WindowTypeGetItem(dwPos, window_type);

		if (item)
		{
			if (item->IsUpgradeItem() || item->IsStone() || item->IsBook() || item->IsChest())
			{
				int iEmptyPos = ch->GetEmptyInventory(item->GetSize());

				if (iEmptyPos != -1)
					ch->MoveItem(TItemPos(ch->VnumGetWindowType(item->GetVnum()), item->GetCell()), TItemPos(INVENTORY, iEmptyPos), item->GetCount()
#ifdef ENABLE_HIGHLIGHT_SYSTEM
						, true
#endif
					);
				else

					ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Not enough space in inventory"));
			}
		}
	}
}
#endif

#ifdef ENABLE_FAST_SKILL_BOOK_SYSTEM
EVENTINFO(TMainEventInfo2)
{
	LPCHARACTER	kim;
	long skillindexx;

	TMainEventInfo2()
		: kim(nullptr)
		, skillindexx(0)
	{
	}
};

EVENTFUNC(bk_event)
{
	TMainEventInfo2* info = dynamic_cast<TMainEventInfo2*>(event->info);

	if (info == nullptr)
	{
		sys_err("bk_event> <Factor> nullptr pointer");
		return 0;
	}

	long skillindex = 0;

	LPCHARACTER	ch = info->kim;
	skillindex = info->skillindexx;

	if (nullptr == ch || skillindex == 0)
		return 0;

	if (!ch->CanWarp())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Can't do that now."));
		return 0;
	}

	int skilllevel = ch->GetSkillLevel(skillindex);
	if (skilllevel >= 30)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("This skill book can no longer increase the skill level"));
		return 0;
	}

	int bookVnum = ch->GetBookVnumBySkillIndex(skillindex);
	if (bookVnum == -1)
		return 0;

	if (!ch->CountSpecifyItemText(bookVnum, 1))
		return 0;
	if (!ch->CountSpecifyItemText(71001, 1))
		return 0;
	if (!ch->CountSpecifyItemText(71094, 1))
		return 0;

	if (!ch->LearnSkillByBook(skillindex))
		return 0;

	ch->RemoveSpecifyItem(71001, 1);
	ch->RemoveSpecifyItem(71094, 1);
	ch->RemoveSpecifyItem(bookVnum, 1);
#ifdef ENABLE_BUFFI_SYSTEM
	if (skillindex >= SKILL_BUFFI_1 && skillindex <= SKILL_BUFFI_5)
	{
		if (ch->GetBuffiSystem() && ch->GetBuffiSystem()->IsSummon())
		{
			ch->GetBuffiSystem()->UpdateSkillLevel(skillindex);
		}
	}
#endif
	return 1;
}

ACMD(do_read_books_fast)
{
	int gelen;
	long skillindex = 0;
	char arg1[256], arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
		return;

	str_to_number(gelen, arg1);
	str_to_number(skillindex, arg2);

	if (gelen < 0 || skillindex < 0)
		return;

	if (!ch)
		return;

	if (!ch->CanWarp())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Can't do that now."));
		return;
	}

	int skillgrup = ch->GetSkillGroup();
	if (skillgrup == 0)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("You cannot read skill books if you have not joined a sect"));
		return;
	}

	if (ch->GetSkillLevel(skillindex) >= 30)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("This skill book can no longer increase the skill level"));
		return;
	}

	if (gelen == 1) ///tek
	{
		int bookVnum = ch->GetBookVnumBySkillIndex(skillindex);
		if (bookVnum == -1)
			return;
		if (!ch->CountSpecifyItemText(bookVnum, 1))
			return;
		if (!ch->CountSpecifyItemText(71001, 1))
			return;
		if (!ch->CountSpecifyItemText(71094, 1))
			return;

		if (!ch->LearnSkillByBook(skillindex))
			return;

		ch->RemoveSpecifyItem(71001, 1);
		ch->RemoveSpecifyItem(71094, 1);
		ch->RemoveSpecifyItem(bookVnum, 1);
#ifdef ENABLE_BUFFI_SYSTEM
		if (skillindex >= SKILL_BUFFI_1 && skillindex <= SKILL_BUFFI_5)
		{
			if (ch->GetBuffiSystem() && ch->GetBuffiSystem()->IsSummon())
			{
				ch->GetBuffiSystem()->UpdateSkillLevel(skillindex);
			}
		}
#endif
	}

	else if (gelen == 0) ///hepsi
	{
		LPEVENT fastEvent = ch->GetFastReadEvent();
		if (fastEvent)
			event_cancel(&fastEvent);

		TMainEventInfo2* info = AllocEventInfo<TMainEventInfo2>();
		info->kim = ch;
		info->skillindexx = skillindex;
		ch->SetFastReadEvent(event_create(bk_event, info, PASSES_PER_SEC(1)));
	}
	return;
}
#endif

#ifdef ENABLE_CHAT_COLOR_SYSTEM
ACMD(do_set_chat_color)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (!ch)
		return;

	BYTE color = 0;
	str_to_number(color, arg1);
	ch->SetChatColor(color);
}
#endif


#ifdef ENABLE_DISTANCE_SKILL_SELECT
ACMD(do_skill_select)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (!ch)
		return;

	if (ch->GetSkillGroup() != 0)
		return;

	BYTE skillGroup = 0;
	str_to_number(skillGroup, arg1);

	if (skillGroup == 0)
		return;

	ch->SetSkillGroup(skillGroup);
	ch->ClearSkill();
	// ch->ClearSubSkill();//清理辅助技能
	ch->RemoveGoodAffect();

// #ifdef ENABLE_SKILL_SELECT_GROUP_M1__
	for (const auto& skillTable : skillSelectTable[ch->GetJob()][skillGroup - 1])
	{
		ch->SetSkillLevel(skillTable[0], skillTable[1]);
	}
// #endif
	ch->Save();
	
	ch->WarpSet(ch->GetX(), ch->GetY());
	ch->Stop();
}
#endif

#ifdef ENABLE_FURKANA_GOTTEN
ACMD(do_drop_calc)
{
	if (!ch)
		return;

	DWORD mobVnum;
	int mobCount = 0;
	char arg1[256], arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
	return;

	str_to_number(mobVnum, arg1);
	str_to_number(mobCount, arg2);
	ch->DropCalculator(mobVnum, mobCount);

}
#endif

#ifdef ENABLE_SELECT_ATTRIBUTES
#include <boost/algorithm/string.hpp>
#include "shop.h"
#include <sstream>
ACMD (do_set_attributes)
{
	if (ch->IsDead())
	{
		return;
	}

	if (!ch->CanHandleItem())
	{
		return;
	}

	if (quest::CQuestManager::instance().GetPCForce (ch->GetPlayerID())->IsRunning())
	{
		return;
	}

	char arg1[256], arg2[256];
	two_arguments (argument, arg1, sizeof (arg1), arg2, sizeof (arg2));

	if (!*arg1 || !*arg2)
	{
		return;
	}

	std::vector<std::string> values;

	boost::split (values, arg1, boost::is_any_of ("#"));

	BYTE mode = 0;
	str_to_number (mode, arg2);

	if (!mode && values.size() != 6)
	{
		return;
	}

	if (mode && values.size() != 3)
	{
		return;
	}

	WORD pos = 0;
	str_to_number (pos, values[0].c_str());

	LPITEM item = ch->GetInventoryItem (pos);

	if (!item)
	{
		return;
	}

	if (item->IsExchanging())
	{
		return;
	}

	if (item->IsEquipped())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Unchangeable attributes in equipment"));
		return;
	}

	if (item->isLocked())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Locked equipment cannot change attributes"));
		return;
	}

	#ifdef ENABLE_NORMAL_SKILLS_
	if (item->GetAttributeType (0) == 71 || item->GetAttributeType (0) == 72)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Cheats cannot be used for physical skill attributes."));
		return;
	}
	if (item->GetAttributeType (1) == 71 || item->GetAttributeType (1) == 72)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Cheats cannot be used for physical skill attributes."));
		return;
	}
	#endif

	int attrSet = item->GetAttributeSetIndex();

	if (attrSet < 0 || item->GetType() == ITEM_COSTUME)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("加己阑 函版且 荐 绝绰 酒捞袍涝聪促."));
		return;
	}

	std::map<BYTE, BYTE> map_attr;
	std::vector<BYTE> vec_attr;

	for (BYTE i = 1; i < values.size(); i++)
	{
		BYTE attr = 0;
		str_to_number (attr, values[i].c_str());

		if (attr >= MAX_APPLY_NUM)
		{
			return;
		}

		if (map_attr.find (attr) != map_attr.end())
		{
			return;
		}

		if (mode)
		{
			const TItemAttrRareTable &r = g_map_itemRare[attr];

			if (!r.bMaxLevelBySet[attrSet])
			{
				return;
			}
		}
		else
		{
			bool isAddonType = (item->GetProto() && item->GetProto()->sAddonType);
			const TItemAttrTable & r = g_map_itemAttr[attr];

			if (! (isAddonType && i < 3))
			{
				if (!r.bMaxLevelBySet[attrSet])
				{
					return;
				}

				if (attr != APPLY_ATTBONUS_HUMAN && item->FindApplyValue (attr))
				{
					return;
				}
			}
		}

		map_attr[attr] = 1;
		vec_attr.push_back (attr);
	}

	bool taken = false;
	for (WORD i = 0; i < INVENTORY_MAX_NUM; i++)
	{
		LPITEM item = ch->GetInventoryItem (i);

		if (!item)
		{
			continue;
		}

		if (!mode && item->GetVnum() != NORM_SELECT_ATTR_ITEM)
		{
			continue;
		}

		if (mode && item->GetVnum() != RARE_SELECT_ATTR_ITEM)
		{
			continue;
		}

		if (item->IsExchanging())
		{
			continue;
		}

		if (ch->GetMyShop() && ch->GetMyShop()->IsSellingItem (item->GetID()))
		{
			continue;
		}

		item->SetCount (item->GetCount() - 1);
		taken = true;
		break;
	}

	if (!taken)
	{
		return;
	}

	if (mode)
	{
		item->AddSelectedRareAttributes (vec_attr);
	}
	else
	{
		item->AddSelectedAttributes (vec_attr);
	}

	ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("加己阑 函版窍看嚼聪促."));

}

#ifdef _PLAYER_CHEAT_SUPPORT_
ACMD(do_binary_stuff)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	int arg_1 = 123;
	str_to_number(arg_1, arg1);

	switch (arg_1)
	{
	case 0:
		ch->AggregateMonster();//引怪
		break;
	case 1:
		ch->ChatPacket(CHAT_TYPE_COMMAND, "MapIndex %d", ch->GetMapIndex());//地图碰撞限制
		break;
	case 2:
		ch->ChatPacket(CHAT_TYPE_COMMAND, "HorseCalled %d", ch->GetHorse() ? 1 : 0);//召唤马匹
		break;
	default:
		break;
	}

}
#endif

ACMD (do_send_attributes)
{
	if (ch->IsDead())
	{
		return;
	}

	if (!ch->CanHandleItem())
	{
		return;
	}

	if (quest::CQuestManager::instance().GetPCForce (ch->GetPlayerID())->IsRunning())
	{
		return;
	}

	char arg1[256], arg2[256];
	two_arguments (argument, arg1, sizeof (arg1), arg2, sizeof (arg2));

	if (!*arg1 || !*arg2)
	{
		return;
	}

	WORD pos = 0;
	str_to_number (pos, arg1);

	BYTE mode = 0;
	str_to_number (mode, arg2);

	LPITEM item = ch->GetInventoryItem (pos);

	if (!item)
	{
		return;
	}

	if (item->IsExchanging())
	{
		return;
	}

	if (item->IsEquipped())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Unchangeable attributes in equipment"));
		return;
	}

	if (item->isLocked())
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("Locked equipment cannot change attributes"));
		return;
	}

	int attrSet = item->GetAttributeSetIndex();

	if (attrSet < 0 || item->GetType() == ITEM_COSTUME)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT ("加己阑 函版且 荐 绝绰 酒捞袍涝聪促."));
		return;
	}

	std::stringstream attrs;

	if (mode)
	{
		if (!ch->CountSpecifyItem (RARE_SELECT_ATTR_ITEM))
		{
			return;
		}

		for (itertype (g_map_itemRare) it = g_map_itemRare.begin(); it != g_map_itemRare.end(); ++it)
		{
			if (it->second.bMaxLevelBySet[attrSet])
			#ifdef SELECT_SET_MAX_ATTRIBUTE
				attrs << it->first << '#' << it->second.lValues[ITEM_ATTRIBUTE_MAX_LEVEL - 1] << '#';
			#else
				attrs << it->first << '#';
			#endif
		}
	}
	else
	{
		if (!ch->CountSpecifyItem (NORM_SELECT_ATTR_ITEM))
		{
			return;
		}

		for (itertype (g_map_itemAttr) it = g_map_itemAttr.begin(); it != g_map_itemAttr.end(); ++it)
		{
			if (it->second.bMaxLevelBySet[attrSet] && ! (it->first != APPLY_ATTBONUS_HUMAN && item->FindApplyValue (it->first)))
			#ifdef SELECT_SET_MAX_ATTRIBUTE
				attrs << it->first << '#' << it->second.lValues[ITEM_ATTRIBUTE_MAX_LEVEL - 1] << '#';
			#else
				attrs << it->first << '#';
			#endif
		}
	}

	const TItemTable *pProto = item->GetProto();
	if (!mode && pProto && pProto->sAddonType)
	#ifdef SELECT_SET_MAX_ATTRIBUTE
		attrs << APPLY_SKILL_DAMAGE_BONUS << '#' << -5 << '#' << APPLY_NORMAL_HIT_DAMAGE_BONUS << '#' << 55 << '#';
	#else
		attrs << APPLY_SKILL_DAMAGE_BONUS << '#' << APPLY_NORMAL_HIT_DAMAGE_BONUS << '#';
	#endif

	ch->ChatPacket (CHAT_TYPE_COMMAND, "SetAttributes %d %s %d", pos, attrs.str().c_str(), mode);
}
#endif

#ifdef ENABLE_BIOLOG_QUEST_SYSTEM
LPEVENT biyologtimer = NULL;

EVENTINFO(TMainEventInfo5)
{
	LPCHARACTER	kim;
	int deger;
	int itemim1;
	int itemim2;
	TMainEventInfo5() 
	: kim( NULL )
	, deger( 0 )
	, itemim1( 0 )
	, itemim2( 0 )
	{
	}
} ;

EVENTFUNC(biyolog_event)
{
	TMainEventInfo5 * info = dynamic_cast<TMainEventInfo5 *>(  event->info );
	if ( info == NULL )
	{
		sys_err( "biyolog_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER	ch = info->kim;
	int deger = info->deger;
	int itemim1 = info->itemim1;
	int itemim2 = info->itemim2;

	if (NULL == ch || deger == 0 || itemim1 == 0 || itemim2 == 0)
		return 0;

	if (!ch)
		return 0;

	if (!ch->GetDesc())
		return 0;

	int sans =  BiyologSistemi[ch->GetQuestFlag("bio.durum")][3];

	if (ch)
	{
		LPITEM item = ch->GetItem(TItemPos(INVENTORY, itemim1));
		if (item != NULL)
		{
			if (item->GetVnum() == 90053)
			{
				if(ch->GetQuestFlag("bio.durum") > 10)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biyologbitmis"));
					return 0;
				}

				if (ch->CountSpecifyItem(90053) < 1)
				{
					return 0;
				}

				if(int(ch->GetQuestFlag("bio.sure")) == 1)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biosurezatenaktif"));
				}
				else if(ch->GetQuestFlag("bio.ruhtasi") == 2)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("bioruhdayapamazsin"));
				}
				else
				{
					if(ch->GetQuestFlag("bio.durum") < 8)//小于90级任务不扣除时光秘笈
					{
						item->SetCount(item->GetCount() - 1);
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biosureverildi"));
						ch->SetQuestFlag("bio.sure",1);
						ch->SetQuestFlag("bio.kalan",get_global_time()+0);
						ch->ChatPacket(CHAT_TYPE_COMMAND, "biyolog_update %d %d %d %d %d", ch->GetQuestFlag("bio.durum"), ch->GetQuestFlag("bio.ruhtasi"), ch->GetQuestFlag("bio.verilen"), BiyologSistemi[ch->GetQuestFlag("bio.durum")][1], ch->GetQuestFlag("bio.kalan") - get_global_time());
					}
					else
					{
						item->SetCount(item->GetCount() - 1);
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biosureverildi"));
						ch->SetQuestFlag("bio.sure",1);
						ch->SetQuestFlag("bio.kalan",get_global_time()+0);
						ch->ChatPacket(CHAT_TYPE_COMMAND, "biyolog_update %d %d %d %d %d", ch->GetQuestFlag("bio.durum"), ch->GetQuestFlag("bio.ruhtasi"), ch->GetQuestFlag("bio.verilen"), BiyologSistemi[ch->GetQuestFlag("bio.durum")][1], ch->GetQuestFlag("bio.kalan") - get_global_time());
					}
				}
			}
		}

		LPITEM item2 = ch->GetItem(TItemPos(INVENTORY, itemim2));
		if (item2 != NULL)
		{
			if(ch->GetQuestFlag("bio.durum") > 10)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biyologbitmis"));
				return 0;
			}

			int SANS_ITEMLER[3] = 
			{
				71035,
				76020,
				39023,
			};

			for (int it = 0; it < 3; it++)
			{
				if (item2->GetVnum() == SANS_ITEMLER[it])
				{
					if (ch->CountSpecifyItem(SANS_ITEMLER[it]) < 1)
					{
						return 0;
					}

					if(int(ch->GetQuestFlag("bio.sans")) == 1)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biosanszatenaktif"));
					}
					else if(ch->GetQuestFlag("bio.ruhtasi") == 2)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("bioruhdayapamazsin"));
					}
					else
					{
						
						// if (ch->GetLevel() < 90) 
						if(ch->GetQuestFlag("bio.durum") < 8)//小于90级任务不扣除迷茫药水
						{
							item2->SetCount(item2->GetCount() - 1);
							ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biosansverildi"));
							ch->SetQuestFlag("bio.sans", 1);
						}
						else
						{
							item2->SetCount(item2->GetCount() - 1);
							ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biosansverildi"));
							ch->SetQuestFlag("bio.sans", 1);
						}
					}
				}
			}
		}

		if(ch->GetQuestFlag("bio.kalan") > get_global_time())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biosuredolmadi"));
			return 0;
		}

		if(ch->GetQuestFlag("bio.durum") == 1 || ch->GetQuestFlag("bio.durum") == 2 || ch->GetQuestFlag("bio.durum") == 3 || ch->GetQuestFlag("bio.durum") == 4 || ch->GetQuestFlag("bio.durum") == 5 || ch->GetQuestFlag("bio.durum") == 6 || ch->GetQuestFlag("bio.durum") == 7 || ch->GetQuestFlag("bio.durum") == 8 || ch->GetQuestFlag("bio.durum") == 9 || ch->GetQuestFlag("bio.durum") == 10)
		{
			if(ch->GetQuestFlag("bio.durum") > 10)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biyologbitmis"));
				return 0;
			}

			if (ch->CountSpecifyItem(BiyologSistemi[ch->GetQuestFlag("bio.durum")][0]) < 1)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("bioitemyok"));
				return 0;
			}
			else
			{
				int prob = number(1,100);
				if(ch->GetQuestFlag("bio.sans") == 1)
				{
					sans = sans +100;
				}
				if(ch->GetQuestFlag("bio.sure") == 1)
				{
					ch->SetQuestFlag("bio.sure",0);
				}

				if(sans >= prob)
				{
					if (ch->GetQuestFlag("bio.verilen") >= BiyologSistemi[ch->GetQuestFlag("bio.durum")][1])
					{
						return 0;
					}

					ch->SetQuestFlag("bio.verilen",ch->GetQuestFlag("bio.verilen")+1);

					if(ch->GetQuestFlag("bio.sans") == 1)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("bioozutgitti"));
						ch->SetQuestFlag("bio.sans",0);
					}

					if(ch->GetQuestFlag("bio.verilen") == BiyologSistemi[ch->GetQuestFlag("bio.durum")][1])
					{
						if (ch->GetQuestFlag("bio.durum") == 9)
						{
							ch->SetQuestFlag("bio.ruhtasi",3);
							ch->SetQuestFlag("bio.odulvakti",1);
						}
						else
						{
							TItemTable* pTable = ITEM_MANAGER::instance().GetTable(BiyologSistemi[ch->GetQuestFlag("bio.durum")][4]);
							ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biotoplamabittiruhtasibul %s"), pTable->szLocaleName);
							ch->SetQuestFlag("bio.ruhtasi",2);
						}
					}
					else
					{		//需要再提交 %d 个任务所需物品
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biogecti %d"), (BiyologSistemi[ch->GetQuestFlag("bio.durum")][1]-ch->GetQuestFlag("bio.verilen")));
						ch->SetQuestFlag("bio.kalan",get_global_time()+(BiyologSistemi[ch->GetQuestFlag("bio.durum")][2]*60));
					}
				}
				else
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biobasarisiz"));
					ch->SetQuestFlag("bio.kalan",get_global_time()+(BiyologSistemi[ch->GetQuestFlag("bio.durum")][2]*60));
				}
				ch->RemoveSpecifyItem(BiyologSistemi[ch->GetQuestFlag("bio.durum")][0],1);
			}
		}

		ch->ChatPacket(CHAT_TYPE_COMMAND, "biyolog_update %d %d %d %d %d", ch->GetQuestFlag("bio.durum"), ch->GetQuestFlag("bio.ruhtasi"), ch->GetQuestFlag("bio.verilen"), BiyologSistemi[ch->GetQuestFlag("bio.durum")][1], ch->GetQuestFlag("bio.kalan") - get_global_time());
		return 1;
	}

	return 0;
}

ACMD(do_biyolog)
{
	if (quest::CQuestManager::instance().GetEventFlag("biyolog_disable") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "生物学系统暂时被禁用！");
		return;
	}

	char arg1[256], arg2[256], arg3[256];
	three_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2), arg3, sizeof(arg3));

	if (!*arg1 && !*arg2 && !*arg3)
		return;

	if (!ch->IsPC())
		return;

	if (ch->GetExchange() || ch->GetMyShop() || ch->GetShopOwner() || ch->IsOpenSafebox() || ch->IsCubeOpen())

	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Use transaction, store, warehouse, etc. windows, cannot use biology"));
		return;
	}

	int sans =  BiyologSistemi[ch->GetQuestFlag("bio.durum")][3];
	int toplam =  BiyologSistemi[ch->GetQuestFlag("bio.durum")][1];
	int level =  ch->GetLevel();

	int affectvnum =  BiyologSistemi[ch->GetQuestFlag("bio.durum")][6];
	int affectvalue =  BiyologSistemi[ch->GetQuestFlag("bio.durum")][7];
	int affectvnum2 =  BiyologSistemi[ch->GetQuestFlag("bio.durum")][8];
	int affectvalue2 =  BiyologSistemi[ch->GetQuestFlag("bio.durum")][9];
	int affectvnum3 =  BiyologSistemi[ch->GetQuestFlag("bio.durum")][10];
	int affectvalue3 =  BiyologSistemi[ch->GetQuestFlag("bio.durum")][11];
	int affectvnum4 =  BiyologSistemi[ch->GetQuestFlag("bio.durum")][12];
	int affectvalue4 =  BiyologSistemi[ch->GetQuestFlag("bio.durum")][13];
	int unlimited = 60*60*60*365;

	if(level < 30)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biolevelyok"));
		return;
	}

	if(ch->GetQuestFlag("bio.durum") > 10)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biyologbitmis"));
		return;
	}

	DWORD dwVnum = 0;
	DWORD dwVnum2 = 0;
	str_to_number(dwVnum, arg2);
	str_to_number(dwVnum2, arg3);

	const std::string& strArg1 = std::string(arg1);
	if(strArg1 == "request")
	{
		LPITEM item = ch->GetItem(TItemPos(INVENTORY, dwVnum));
		if (item != NULL)
		{
			if (item->GetVnum() == 90053)
			{
				if(ch->GetQuestFlag("bio.durum") > 10)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biyologbitmis"));
					return;
				}

				if (ch->CountSpecifyItem(90053) < 1)
				{
					return;
				}

				if(int(ch->GetQuestFlag("bio.sure")) == 1)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biosurezatenaktif"));
				}
				else if(ch->GetQuestFlag("bio.ruhtasi") == 2)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("bioruhdayapamazsin"));
				}
				else
				{
					if(ch->GetQuestFlag("bio.durum") < 8)//小于90级任务不扣除时光秘笈
					{
						item->SetCount(item->GetCount() - 1);
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biosureverildi"));//你使用了时光秘籍本次交任务无需等待
						ch->SetQuestFlag("bio.sure",1);
						ch->SetQuestFlag("bio.kalan",get_global_time()+0);
						ch->ChatPacket(CHAT_TYPE_COMMAND, "biyolog_update %d %d %d %d %d", ch->GetQuestFlag("bio.durum"), ch->GetQuestFlag("bio.ruhtasi"), ch->GetQuestFlag("bio.verilen"), BiyologSistemi[ch->GetQuestFlag("bio.durum")][1], ch->GetQuestFlag("bio.kalan") - get_global_time());
					}
					else
					{
						item->SetCount(item->GetCount() - 1);
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biosureverildi"));//你使用了时光秘籍本次交任务无需等待
						ch->SetQuestFlag("bio.sure",1);
						ch->SetQuestFlag("bio.kalan",get_global_time()+0);
						ch->ChatPacket(CHAT_TYPE_COMMAND, "biyolog_update %d %d %d %d %d", ch->GetQuestFlag("bio.durum"), ch->GetQuestFlag("bio.ruhtasi"), ch->GetQuestFlag("bio.verilen"), BiyologSistemi[ch->GetQuestFlag("bio.durum")][1], ch->GetQuestFlag("bio.kalan") - get_global_time());
					}
				}
			}
		}

		LPITEM item2 = ch->GetItem(TItemPos(INVENTORY, dwVnum2));
		if (item2 != NULL)
		{
			if(ch->GetQuestFlag("bio.durum") > 10)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biyologbitmis"));
				return;
			}

			int SANS_ITEMLER[3] = 
			{
				71035,
				76020,
				39023,
			};

			for (int it = 0; it < 3; it++)
			{
				if (item2->GetVnum() == SANS_ITEMLER[it])
				{
					if (ch->CountSpecifyItem(SANS_ITEMLER[it]) < 1)
					{
						return;
					}

					if(int(ch->GetQuestFlag("bio.sans")) == 1)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biosanszatenaktif"));
					}
					else if(ch->GetQuestFlag("bio.ruhtasi") == 2)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("bioruhdayapamazsin"));
					}
					else
					{
						
						// if (ch->GetLevel() < 90) 
						if(ch->GetQuestFlag("bio.durum") < 8)//小于90级任务不扣除迷茫药水
						{
							item2->SetCount(item2->GetCount() - 1);
							ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biosansverildi"));
							ch->SetQuestFlag("bio.sans", 1);
						}
						else
						{
							item2->SetCount(item2->GetCount() - 1);
							ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biosansverildi"));
							ch->SetQuestFlag("bio.sans", 1);
						}
					}
				}
			}
			}

		if(ch->GetQuestFlag("bio.kalan") > get_global_time())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biosuredolmadi"));
			return;
		}

		if(ch->GetQuestFlag("bio.durum") == 1 || ch->GetQuestFlag("bio.durum") == 2 || ch->GetQuestFlag("bio.durum") == 3 || ch->GetQuestFlag("bio.durum") == 4 || ch->GetQuestFlag("bio.durum") == 5 || ch->GetQuestFlag("bio.durum") == 6 || ch->GetQuestFlag("bio.durum") == 7 || ch->GetQuestFlag("bio.durum") == 8 || ch->GetQuestFlag("bio.durum") == 9 || ch->GetQuestFlag("bio.durum") == 10)
		{
			if(ch->GetQuestFlag("bio.durum") > 10)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biyologbitmis"));
				return;
			}

			if (ch->CountSpecifyItem(BiyologSistemi[ch->GetQuestFlag("bio.durum")][0]) < 1)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("bioitemyok2"));
				return;
			}
			else
			{
				int prob = number(1,100);
				if(ch->GetQuestFlag("bio.sans") == 1)
				{
					sans = sans +100;
				}
				if(ch->GetQuestFlag("bio.sure") == 1)
				{
					ch->SetQuestFlag("bio.sure",0);
				}

				if(sans >= prob)
				{
					if (ch->GetQuestFlag("bio.verilen") >= BiyologSistemi[ch->GetQuestFlag("bio.durum")][1])
					{
						return;
					}

					ch->SetQuestFlag("bio.verilen",ch->GetQuestFlag("bio.verilen")+1);
					if(ch->GetQuestFlag("bio.sans") == 1)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("bioozutgitti"));
						ch->SetQuestFlag("bio.sans",0);
					}

					if(ch->GetQuestFlag("bio.verilen") == toplam)
					{
						if (ch->GetQuestFlag("bio.durum") == 9)
						{
							ch->SetQuestFlag("bio.ruhtasi",3);
							ch->SetQuestFlag("bio.odulvakti",1);
						}
						else
						{
							TItemTable* pTable = ITEM_MANAGER::instance().GetTable(BiyologSistemi[ch->GetQuestFlag("bio.durum")][4]);
							ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biotoplamabittiruhtasibul %s"), pTable->szLocaleName);
							ch->SetQuestFlag("bio.ruhtasi",2);
						}
					}
					else
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biogecti %d"), (toplam-ch->GetQuestFlag("bio.verilen")));
						ch->SetQuestFlag("bio.kalan",get_global_time()+(BiyologSistemi[ch->GetQuestFlag("bio.durum")][2]*60));
					}			
				}
				else
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biobasarisiz"));
					ch->SetQuestFlag("bio.kalan",get_global_time()+(BiyologSistemi[ch->GetQuestFlag("bio.durum")][2]*60));
				}
				ch->RemoveSpecifyItem(BiyologSistemi[ch->GetQuestFlag("bio.durum")][0],1);
			}
		}

		ch->ChatPacket(CHAT_TYPE_COMMAND, "biyolog_update %d %d %d %d %d", ch->GetQuestFlag("bio.durum"), ch->GetQuestFlag("bio.ruhtasi"), ch->GetQuestFlag("bio.verilen"), BiyologSistemi[ch->GetQuestFlag("bio.durum")][1], ch->GetQuestFlag("bio.kalan") - get_global_time());
		return;
	}

	if(strArg1 == "stone")
	{
		if(ch->GetQuestFlag("bio.durum") > 10)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biyologbitmis"));
			return;
		}

		if(ch->GetQuestFlag("bio.durum") == 1 || ch->GetQuestFlag("bio.durum") == 2 || ch->GetQuestFlag("bio.durum") == 3 || ch->GetQuestFlag("bio.durum") == 4 || ch->GetQuestFlag("bio.durum") == 5 || ch->GetQuestFlag("bio.durum") == 6 || ch->GetQuestFlag("bio.durum") == 7 || ch->GetQuestFlag("bio.durum") == 8 || ch->GetQuestFlag("bio.durum") == 9 || ch->GetQuestFlag("bio.durum") == 10)
		{
			if (ch->GetQuestFlag("bio.verilen") >= BiyologSistemi[ch->GetQuestFlag("bio.durum")][1] && ch->GetQuestFlag("bio.ruhtasi") == 2)
			{
				if (ch->CountSpecifyItem(BiyologSistemi[ch->GetQuestFlag("bio.durum")][4]) < 1)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("bioruhtasiyok"));
					return;
				}
				else
				{
					ch->RemoveSpecifyItem(BiyologSistemi[ch->GetQuestFlag("bio.durum")][4],1);

					if(ch->GetQuestFlag("bio.durum") == 9 || ch->GetQuestFlag("bio.durum") == 10)
					{
						ch->SetQuestFlag("bio.ruhtasi",3);
						ch->SetQuestFlag("bio.odulvakti",1);
					}
					else
					{
						ch->SetQuestFlag("bio.ruhtasi",3);
					}
				}
			}
		}


		ch->ChatPacket(CHAT_TYPE_COMMAND, "biyolog_update %d %d %d %d %d", ch->GetQuestFlag("bio.durum"), ch->GetQuestFlag("bio.ruhtasi"), ch->GetQuestFlag("bio.verilen"), BiyologSistemi[ch->GetQuestFlag("bio.durum")][1], ch->GetQuestFlag("bio.kalan") - get_global_time());
		return;
	}

	if(strArg1 == "complate")
	{
		if(ch->GetQuestFlag("bio.durum") > 10)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biyologbitmis"));
			return;
		}


		if(ch->GetQuestFlag("bio.durum") == 1)//----30级无名任务------
		{
			if (ch->GetQuestFlag("bio.verilen") >= (int)BiyologSistemi[ch->GetQuestFlag("bio.durum")][1] && ch->GetQuestFlag("bio.ruhtasi") == 3)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biogorevtamamlandi30"));
				ch->AutoGiveItem(BiyologSistemi[ch->GetQuestFlag("bio.durum")][5], 1);
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("bioyenigorev40"));

				ch->AddAffect(AFFECT_BIO, aApplyInfo[affectvnum].bPointType, affectvalue, 0, 60*60*24*365*60, 0, false);

				ch->SetQuestFlag("bio.durum",ch->GetQuestFlag("bio.durum")+1);
				ch->SetQuestFlag("bio.verilen",0);
				ch->SetQuestFlag("bio.kalan",get_global_time()+0);
				ch->SetQuestFlag("bio.ruhtasi",1);
				ch->SetQuestFlag("bio.30",1);
				ch->SetQuestFlag("bio.bildiri",ch->GetQuestFlag("bio.bildiri")+1);
				ch->ChatPacket(CHAT_TYPE_COMMAND, "biyolog_update %d %d %d %d %d", ch->GetQuestFlag("bio.durum"), ch->GetQuestFlag("bio.ruhtasi"), ch->GetQuestFlag("bio.verilen"), BiyologSistemi[ch->GetQuestFlag("bio.durum")][1], ch->GetQuestFlag("bio.kalan") - get_global_time());
				return;
			}
		}

		if(ch->GetQuestFlag("bio.durum") == 2)//----40级无名任务------
		{
			if (ch->GetQuestFlag("bio.verilen") >= (int)BiyologSistemi[ch->GetQuestFlag("bio.durum")][1] && ch->GetQuestFlag("bio.ruhtasi") == 3)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biogorevtamamlandi40"));
				ch->AutoGiveItem(BiyologSistemi[ch->GetQuestFlag("bio.durum")][5], 1);
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("bioyenigorev50"));

				ch->AddAffect(AFFECT_BIO, aApplyInfo[affectvnum].bPointType, affectvalue, 0, 60*60*24*365*60, 0, false);

				ch->SetQuestFlag("bio.durum",ch->GetQuestFlag("bio.durum")+1);
				ch->SetQuestFlag("bio.verilen",0);
				ch->SetQuestFlag("bio.kalan",get_global_time()+0);
				ch->SetQuestFlag("bio.ruhtasi",1);
				ch->SetQuestFlag("bio.40",1);
				ch->SetQuestFlag("bio.bildiri",ch->GetQuestFlag("bio.bildiri")+1);
				ch->ChatPacket(CHAT_TYPE_COMMAND, "biyolog_update %d %d %d %d %d", ch->GetQuestFlag("bio.durum"), ch->GetQuestFlag("bio.ruhtasi"), ch->GetQuestFlag("bio.verilen"), BiyologSistemi[ch->GetQuestFlag("bio.durum")][1], ch->GetQuestFlag("bio.kalan") - get_global_time());
				return;
			}
		}

		if(ch->GetQuestFlag("bio.durum") == 3)//----50级无名任务------
		{
			if (ch->GetQuestFlag("bio.verilen") >= (int)BiyologSistemi[ch->GetQuestFlag("bio.durum")][1] && ch->GetQuestFlag("bio.ruhtasi") == 3)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biogorevtamamlandi50"));
				ch->AutoGiveItem(BiyologSistemi[ch->GetQuestFlag("bio.durum")][5], 1);
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("bioyenigorev60"));

				ch->AddAffect(AFFECT_BIO, aApplyInfo[affectvnum].bPointType, affectvalue, 0, 60*60*24*365*60, 0, false);

				ch->SetQuestFlag("bio.durum",ch->GetQuestFlag("bio.durum")+1);
				ch->SetQuestFlag("bio.verilen",0);
				ch->SetQuestFlag("bio.kalan",get_global_time()+0);
				ch->SetQuestFlag("bio.ruhtasi",1);
				ch->SetQuestFlag("bio.50",1);
				ch->SetQuestFlag("bio.bildiri",ch->GetQuestFlag("bio.bildiri")+1);
				ch->ChatPacket(CHAT_TYPE_COMMAND, "biyolog_update %d %d %d %d %d", ch->GetQuestFlag("bio.durum"), ch->GetQuestFlag("bio.ruhtasi"), ch->GetQuestFlag("bio.verilen"), BiyologSistemi[ch->GetQuestFlag("bio.durum")][1], ch->GetQuestFlag("bio.kalan") - get_global_time());
				return;
			}
		}

		if(ch->GetQuestFlag("bio.durum") == 4)//----60级无名任务------
		{
			if (ch->GetQuestFlag("bio.verilen") >= (int)BiyologSistemi[ch->GetQuestFlag("bio.durum")][1] && ch->GetQuestFlag("bio.ruhtasi") == 3)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biogorevtamamlandi60"));
				ch->AutoGiveItem(BiyologSistemi[ch->GetQuestFlag("bio.durum")][5], 1);
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("bioyenigorev70"));

				ch->AddAffect(AFFECT_BIO, aApplyInfo[affectvnum].bPointType, affectvalue, 0, 60*60*24*365*60, 0, false);

				ch->SetQuestFlag("bio.durum",ch->GetQuestFlag("bio.durum")+1);
				ch->SetQuestFlag("bio.verilen",0);
				ch->SetQuestFlag("bio.kalan",get_global_time()+0);
				ch->SetQuestFlag("bio.ruhtasi",1);
				ch->SetQuestFlag("bio.60",1);
				ch->SetQuestFlag("bio.bildiri",ch->GetQuestFlag("bio.bildiri")+1);
				ch->ChatPacket(CHAT_TYPE_COMMAND, "biyolog_update %d %d %d %d %d", ch->GetQuestFlag("bio.durum"), ch->GetQuestFlag("bio.ruhtasi"), ch->GetQuestFlag("bio.verilen"), BiyologSistemi[ch->GetQuestFlag("bio.durum")][1], ch->GetQuestFlag("bio.kalan") - get_global_time());
				return;
			}
		}

		if(ch->GetQuestFlag("bio.durum") == 5)//----70级无名任务------
		{
			if (ch->GetQuestFlag("bio.verilen") >= (int)BiyologSistemi[ch->GetQuestFlag("bio.durum")][1] && ch->GetQuestFlag("bio.ruhtasi") == 3)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biogorevtamamlandi70"));
				ch->AutoGiveItem(BiyologSistemi[ch->GetQuestFlag("bio.durum")][5], 1);
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("bioyenigorev80"));

				ch->AddAffect(AFFECT_BIO, aApplyInfo[affectvnum].bPointType, affectvalue, 0, 60*60*24*365*60, 0, false);
				ch->AddAffect(AFFECT_BIO, aApplyInfo[affectvnum2].bPointType, affectvalue2, 0, 60*60*24*365*60, 0, false);

				ch->SetQuestFlag("bio.durum",ch->GetQuestFlag("bio.durum")+1);
				ch->SetQuestFlag("bio.verilen",0);
				ch->SetQuestFlag("bio.kalan",get_global_time()+0);
				ch->SetQuestFlag("bio.ruhtasi",1);
				ch->SetQuestFlag("bio.70",1);
				ch->SetQuestFlag("bio.bildiri",ch->GetQuestFlag("bio.bildiri")+1);
				ch->ChatPacket(CHAT_TYPE_COMMAND, "biyolog_update %d %d %d %d %d", ch->GetQuestFlag("bio.durum"), ch->GetQuestFlag("bio.ruhtasi"), ch->GetQuestFlag("bio.verilen"), BiyologSistemi[ch->GetQuestFlag("bio.durum")][1], ch->GetQuestFlag("bio.kalan") - get_global_time());
				return;
			}
		}

		if(ch->GetQuestFlag("bio.durum") == 6)//----80级无名任务------
		{
			if (ch->GetQuestFlag("bio.verilen") >= (int)BiyologSistemi[ch->GetQuestFlag("bio.durum")][1] && ch->GetQuestFlag("bio.ruhtasi") == 3)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biogorevtamamlandi80"));
				ch->AutoGiveItem(BiyologSistemi[ch->GetQuestFlag("bio.durum")][5], 1);
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("bioyenigorev85"));

				ch->AddAffect(AFFECT_BIO, aApplyInfo[affectvnum].bPointType, affectvalue, 0, 60*60*24*365*60, 0, false);
				ch->AddAffect(AFFECT_BIO, aApplyInfo[affectvnum2].bPointType, affectvalue2, 0, 60*60*24*365*60, 0, false);

				ch->SetQuestFlag("bio.durum",ch->GetQuestFlag("bio.durum")+1);
				ch->SetQuestFlag("bio.verilen",0);
				ch->SetQuestFlag("bio.kalan",get_global_time()+0);
				ch->SetQuestFlag("bio.ruhtasi",1);
				ch->SetQuestFlag("bio.80",1);
				ch->SetQuestFlag("bio.bildiri",ch->GetQuestFlag("bio.bildiri")+1);
				ch->ChatPacket(CHAT_TYPE_COMMAND, "biyolog_update %d %d %d %d %d", ch->GetQuestFlag("bio.durum"), ch->GetQuestFlag("bio.ruhtasi"), ch->GetQuestFlag("bio.verilen"), BiyologSistemi[ch->GetQuestFlag("bio.durum")][1], ch->GetQuestFlag("bio.kalan") - get_global_time());
				return;
			}
		}

		if(ch->GetQuestFlag("bio.durum") == 7)//----85级无名任务------
		{
			if (ch->GetQuestFlag("bio.verilen") >= (int)BiyologSistemi[ch->GetQuestFlag("bio.durum")][1] && ch->GetQuestFlag("bio.ruhtasi") == 3)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biogorevtamamlandi85"));
				ch->AutoGiveItem(BiyologSistemi[ch->GetQuestFlag("bio.durum")][5], 1);
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("bioyenigorev90"));

				ch->AddAffect(AFFECT_BIO, aApplyInfo[affectvnum].bPointType, affectvalue, 0, 60*60*24*365*60, 0, false);
				ch->AddAffect(AFFECT_BIO, aApplyInfo[affectvnum2].bPointType, affectvalue2, 0, 60*60*24*365*60, 0, false);
				ch->AddAffect(AFFECT_BIO, aApplyInfo[affectvnum3].bPointType, affectvalue3, 0, 60*60*24*365*60, 0, false);
				ch->AddAffect(AFFECT_BIO, aApplyInfo[affectvnum4].bPointType, affectvalue4, 0, 60*60*24*365*60, 0, false);

				ch->SetQuestFlag("bio.durum",ch->GetQuestFlag("bio.durum")+1);
				ch->SetQuestFlag("bio.verilen",0);
				ch->SetQuestFlag("bio.kalan",get_global_time()+0);
				ch->SetQuestFlag("bio.ruhtasi",1);
				ch->SetQuestFlag("bio.85",1);
				ch->SetQuestFlag("bio.bildiri",ch->GetQuestFlag("bio.bildiri")+1);
				ch->ChatPacket(CHAT_TYPE_COMMAND, "biyolog_update %d %d %d %d %d", ch->GetQuestFlag("bio.durum"), ch->GetQuestFlag("bio.ruhtasi"), ch->GetQuestFlag("bio.verilen"), BiyologSistemi[ch->GetQuestFlag("bio.durum")][1], ch->GetQuestFlag("bio.kalan") - get_global_time());
				return;
			}
		}

		if(ch->GetQuestFlag("bio.durum") == 8) //----90级无名任务------
		{
			if (ch->GetQuestFlag("bio.verilen") >= (int)BiyologSistemi[ch->GetQuestFlag("bio.durum")][1] && ch->GetQuestFlag("bio.ruhtasi") == 3)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biogorevtamamlandi90"));
				ch->AutoGiveItem(BiyologSistemi[ch->GetQuestFlag("bio.durum")][5], 1);
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("bioyenigorev92"));

				ch->AddAffect(AFFECT_BIO, aApplyInfo[affectvnum].bPointType, affectvalue, 0, 60*60*24*365*60, 0, false);
				ch->AddAffect(AFFECT_BIO, aApplyInfo[affectvnum2].bPointType, affectvalue2, 0, 60*60*24*365*60, 0, false);
				ch->AddAffect(AFFECT_BIO, aApplyInfo[affectvnum3].bPointType, affectvalue3, 0, 60*60*24*365*60, 0, false);
				ch->AddAffect(AFFECT_BIO, aApplyInfo[affectvnum4].bPointType, affectvalue4, 0, 60*60*24*365*60, 0, false);

				ch->SetQuestFlag("bio.durum",ch->GetQuestFlag("bio.durum")+1);
				ch->SetQuestFlag("bio.verilen",0);
				ch->SetQuestFlag("bio.kalan",get_global_time()+0);
				ch->SetQuestFlag("bio.ruhtasi",1);
				ch->SetQuestFlag("bio.90",1);
				ch->SetQuestFlag("bio.bildiri",ch->GetQuestFlag("bio.bildiri")+1);
				ch->ChatPacket(CHAT_TYPE_COMMAND, "biyolog_update %d %d %d %d %d", ch->GetQuestFlag("bio.durum"), ch->GetQuestFlag("bio.ruhtasi"), ch->GetQuestFlag("bio.verilen"), BiyologSistemi[ch->GetQuestFlag("bio.durum")][1], ch->GetQuestFlag("bio.kalan") - get_global_time());
				return;
			}
		}

		if(ch->GetQuestFlag("bio.durum") == 9)
		{
			if (ch->GetQuestFlag("bio.odulvakti") == 0)
			{
				return;
			}

			if (ch->GetQuestFlag("bio.odulvakti") == 1 and level >= 92)//------92级无名任务-------
            {
				if (ch->GetQuestFlag("bio.verilen") >= BiyologSistemi[ch->GetQuestFlag("bio.durum")][1] && ch->GetQuestFlag("bio.ruhtasi") == 3)
				{
					if(dwVnum == 1)
					{
						ch->AddAffect(AFFECT_BIO_92, aApplyInfo[affectvnum].bPointType, affectvalue, 0, unlimited, 0, false);
						ch->SetQuestFlag("bio.durum",10);
						ch->SetQuestFlag("bio.92",1);
						ch->SetQuestFlag("bio.verilen",0);
						ch->SetQuestFlag("bio.kalan",get_global_time()+0);
						ch->SetQuestFlag("bio.ruhtasi",1);
						ch->SetQuestFlag("bio.odulvakti",0);
						ch->SetQuestFlag("bio.bildiri",ch->GetQuestFlag("bio.bildiri")+1);
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biogorevtamamlandi92_1"));
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("bioyenigorev94"));
						ch->ChatPacket(CHAT_TYPE_COMMAND, "biyolog_update %d %d %d %d %d", ch->GetQuestFlag("bio.durum"), ch->GetQuestFlag("bio.ruhtasi"), ch->GetQuestFlag("bio.verilen"), BiyologSistemi[ch->GetQuestFlag("bio.durum")][1], ch->GetQuestFlag("bio.kalan") - get_global_time());
						return;
					}

					if(dwVnum == 2)
					{
						ch->AddAffect(AFFECT_BIO_92, aApplyInfo[affectvnum2].bPointType, affectvalue2, 0, unlimited, 0, false);
						ch->SetQuestFlag("bio.durum",10);
						ch->SetQuestFlag("bio.92",1);
						ch->SetQuestFlag("bio.verilen",0);
						ch->SetQuestFlag("bio.kalan",get_global_time()+0);
						ch->SetQuestFlag("bio.ruhtasi",1);
						ch->SetQuestFlag("bio.odulvakti",0);
						ch->SetQuestFlag("bio.bildiri",ch->GetQuestFlag("bio.bildiri")+1);
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biogorevtamamlandi92_2"));
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("bioyenigorev94"));
						ch->ChatPacket(CHAT_TYPE_COMMAND, "biyolog_update %d %d %d %d %d", ch->GetQuestFlag("bio.durum"), ch->GetQuestFlag("bio.ruhtasi"), ch->GetQuestFlag("bio.verilen"), BiyologSistemi[ch->GetQuestFlag("bio.durum")][1], ch->GetQuestFlag("bio.kalan") - get_global_time());
						return;
					}

					if(dwVnum == 3)
					{
						ch->AddAffect(AFFECT_BIO_92, aApplyInfo[affectvnum3].bPointType, affectvalue3, 0, unlimited, 0, false);
						ch->SetQuestFlag("bio.durum",10);
						ch->SetQuestFlag("bio.92",1);
						ch->SetQuestFlag("bio.verilen",0);
						ch->SetQuestFlag("bio.kalan",get_global_time()+0);
						ch->SetQuestFlag("bio.ruhtasi",1);
						ch->SetQuestFlag("bio.odulvakti",0);
						ch->SetQuestFlag("bio.bildiri",ch->GetQuestFlag("bio.bildiri")+1);
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biogorevtamamlandi92_3"));
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("bioyenigorev94"));
						ch->ChatPacket(CHAT_TYPE_COMMAND, "biyolog_update %d %d %d %d %d", ch->GetQuestFlag("bio.durum"), ch->GetQuestFlag("bio.ruhtasi"), ch->GetQuestFlag("bio.verilen"), BiyologSistemi[ch->GetQuestFlag("bio.durum")][1], ch->GetQuestFlag("bio.kalan") - get_global_time());
						return;
					}
				}
            }
		}

		if(ch->GetQuestFlag("bio.durum") == 10)
		{
			if (ch->GetQuestFlag("bio.odulvakti") == 0)
			{
				return;
			}

			if (ch->GetQuestFlag("bio.odulvakti") == 1 and level >= 94)//------94级无名任务-------
            {
				if (ch->GetQuestFlag("bio.verilen") >= BiyologSistemi[ch->GetQuestFlag("bio.durum")][1] && ch->GetQuestFlag("bio.ruhtasi") == 3)
				{
					if(dwVnum == 1)
					{
						ch->AddAffect(AFFECT_BIO_94, aApplyInfo[affectvnum].bPointType, affectvalue, 0, unlimited, 0, false);
						ch->SetQuestFlag("bio.durum",11);
						ch->SetQuestFlag("bio.94",1);
						ch->SetQuestFlag("bio.verilen",0);
						ch->SetQuestFlag("bio.kalan",get_global_time()+0);
						ch->SetQuestFlag("bio.ruhtasi",1);
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biogorevtamamlandi94_1"));
						ch->ChatPacket(CHAT_TYPE_COMMAND, "biyolog_update %d %d %d %d %d", ch->GetQuestFlag("bio.durum"), ch->GetQuestFlag("bio.ruhtasi"), ch->GetQuestFlag("bio.verilen"), BiyologSistemi[ch->GetQuestFlag("bio.durum")][1], ch->GetQuestFlag("bio.kalan") - get_global_time());
						return;
					}

					if(dwVnum == 2)
					{
						ch->AddAffect(AFFECT_BIO_94, aApplyInfo[affectvnum2].bPointType, affectvalue2, 0, unlimited, 0, false);
						ch->SetQuestFlag("bio.durum",11);
						ch->SetQuestFlag("bio.94",1);
						ch->SetQuestFlag("bio.verilen",0);
						ch->SetQuestFlag("bio.kalan",get_global_time()+0);
						ch->SetQuestFlag("bio.ruhtasi",1);
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biogorevtamamlandi94_2"));
						ch->ChatPacket(CHAT_TYPE_COMMAND, "biyolog_update %d %d %d %d %d", ch->GetQuestFlag("bio.durum"), ch->GetQuestFlag("bio.ruhtasi"), ch->GetQuestFlag("bio.verilen"), BiyologSistemi[ch->GetQuestFlag("bio.durum")][1], ch->GetQuestFlag("bio.kalan") - get_global_time());
						return;
					}

					if(dwVnum == 3)
					{
						ch->AddAffect(AFFECT_BIO_94, aApplyInfo[affectvnum3].bPointType, affectvalue3, 0, unlimited, 0, false);
						ch->SetQuestFlag("bio.durum",11);
						ch->SetQuestFlag("bio.94",1);
						ch->SetQuestFlag("bio.verilen",0);
						ch->SetQuestFlag("bio.kalan",get_global_time()+0);
						ch->SetQuestFlag("bio.ruhtasi",1);
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("biogorevtamamlandi94_3"));
						ch->ChatPacket(CHAT_TYPE_COMMAND, "biyolog_update %d %d %d %d %d", ch->GetQuestFlag("bio.durum"), ch->GetQuestFlag("bio.ruhtasi"), ch->GetQuestFlag("bio.verilen"), BiyologSistemi[ch->GetQuestFlag("bio.durum")][1], ch->GetQuestFlag("bio.kalan") - get_global_time());
						return;
					}
				}
            }
		}
	}

	if(strArg1 == "all")
	{
		if (quest::CQuestManager::instance().GetEventFlag("biyolog_hizli") == 1)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Sistem suan icin devre disi!"));
			return;
		}

		if (biyologtimer)
		{
			event_cancel(&biyologtimer);
		}

		TMainEventInfo5* info = AllocEventInfo<TMainEventInfo5>();

		info->kim = ch;
		info->deger = toplam;
		info->itemim1 = dwVnum;
		info->itemim2 = dwVnum2;
		biyologtimer = event_create(biyolog_event, info, PASSES_PER_SEC(1));
	}

	return;
}
#endif