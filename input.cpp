#include "stdafx.h"
#include <sstream>

#include "desc.h"
#include "desc_manager.h"
#include "char.h"
#include "buffer_manager.h"
#include "config.h"
#include "profiler.h"
#include "p2p.h"

#include "db.h"
#include "questmanager.h"
#include "fishing.h"
#include "priv_manager.h"
#include "dev_log.h"
#include "../../common/Controls.h"
#include "desc_client.h"
#include "utils.h"

// #define ENABLE_UNKNOWN_PACKT_TEST	//开启数据包捕捉错误

bool IsEmptyAdminPage()
{
	return g_stAdminPageIP.empty();
}

/* 遍历IP列表，判断是否为管理员页面IP */
bool IsAdminPage(const char* ip) 
{
	for (size_t n = 0; n < g_stAdminPageIP.size(); ++n)
	{
		if (g_stAdminPageIP[n] == ip)
			return 1;
	}
	return 0;
}

 /* 清空管理员IP列表 */
void ClearAdminPages() 
{
	for (size_t n = 0; n < g_stAdminPageIP.size(); ++n)
		g_stAdminPageIP[n].clear();

	g_stAdminPageIP.clear();
}

CInputProcessor::CInputProcessor() : m_pPacketInfo(NULL), m_iBufferLeft(0)
{
	if (!m_pPacketInfo)
		// 绑定客户端->服务器（CG）的数据包协议定义
		BindPacketInfo(&m_packetInfoCG); 
}

void CInputProcessor::BindPacketInfo(CPacketInfo* pPacketInfo)
{
	// 关联数据包协议解析规则
	m_pPacketInfo = pPacketInfo;
}


#ifdef ENABLE_PM_ALL_SEND_SYSTEM
// 函数对象：封装给单个客户端发送批量私聊包的逻辑
struct SWhisperPacketFunc
{
	const char* c_pszText;

	SWhisperPacketFunc(const char* text) :
		c_pszText(text)
	{
	}

	void operator () (LPDESC d)
	{
		if (!d || !d->GetCharacter())
			return;

		struct packet_bulk_whisper bulk_whisper_pack;
		bulk_whisper_pack.header = HEADER_GC_BULK_WHISPER;
		bulk_whisper_pack.size = sizeof(struct packet_bulk_whisper) + strlen(c_pszText);

		TEMP_BUFFER buf;
		buf.write(&bulk_whisper_pack, sizeof(struct packet_bulk_whisper));
		buf.write(c_pszText, strlen(c_pszText));

		d->Packet(buf.read_peek(), buf.size());
	}
};

// 遍历所有在线客户端，调用SWhisperPacketFunc发送私聊包
void CInputProcessor::SendBulkWhisper(const char* c_pszText)
{
	const DESC_MANAGER::DESC_SET& f = DESC_MANAGER::instance().GetClientSet();
	std::for_each(f.begin(), f.end(), SWhisperPacketFunc(c_pszText));
}
#endif

bool CInputProcessor::Process(LPDESC lpDesc, const void* c_pvOrig, int iBytes, int& r_iBytesProceed)
{
	const char* c_pData = (const char*)c_pvOrig;

	BYTE	bLastHeader = 0;
	int		iLastPacketLen = 0;
	int		iPacketLen;

	if (!m_pPacketInfo)
	{
		sys_err("No packet info has been binded to");
		return true;
	}

	// 循环解析缓冲区中的所有完整数据包
	for (m_iBufferLeft = iBytes; m_iBufferLeft > 0;)
	{
		// 获取数据包头部（1字节）
		BYTE bHeader = (BYTE) * (c_pData);
		const char* c_pszName;
#ifdef ENABLE_UNKNOWN_PACKT_TEST
		if (m_pPacketInfo->Get(bHeader, &iPacketLen, &c_pszName))
			sys_err("FormatMy GET Packet -> Header %d Name %s", bHeader, c_pszName);
#endif
		// 未知数据包头部处理：记录错误并关闭连接
		if (bHeader == 0)
			iPacketLen = 1;
		else if (!m_pPacketInfo->Get(bHeader, &iPacketLen, &c_pszName))
		{
			sys_err("UNKNOWN HEADER: %d, LAST HEADER: %d(%d), REMAIN BYTES: %d, fd: %d",
				bHeader, bLastHeader, iLastPacketLen, m_iBufferLeft, lpDesc->GetSocket());
			lpDesc->SetPhase(PHASE_CLOSE);
			return true;
		}
		// 数据包不完整：等待后续数据
		if (m_iBufferLeft < iPacketLen)
			return true;
		
		// 解析并处理完整数据包
		if (bHeader)
		{
			m_pPacketInfo->Start();
			// 核心解析逻辑
			int iExtraPacketSize = Analyze(lpDesc, bHeader, c_pData);

			if (iExtraPacketSize < 0)
				return true;

			iPacketLen += iExtraPacketSize;
			// 记录数据包日志
			lpDesc->Log("%s %d", c_pszName, iPacketLen);
			m_pPacketInfo->End();
		}
		// 移动缓冲区指针，更新已处理字节数
		c_pData += iPacketLen;
		m_iBufferLeft -= iPacketLen;
		r_iBytesProceed += iPacketLen;

		iLastPacketLen = iPacketLen;
		bLastHeader = bHeader;
		// 处理器类型不匹配：终止解析
		if (GetType() != lpDesc->GetInputProcessor()->GetType())
			return false;
	}

	return true;
}
// 处理客户端心跳包，标记连接存活
void CInputProcessor::Pong(LPDESC d)
{
	d->SetPong(true);
}

void CInputProcessor::Handshake(LPDESC d, const char* c_pData)
{
	// 验证客户端握手数据（dwHandshake），完成后切换连接阶段(PHASE_AUTH/PHASE_LOGIN)
	TPacketCGHandshake* p = (TPacketCGHandshake*)c_pData;

	if (d->GetHandshake() != p->dwHandshake)
	{
		/* 握手失败，关闭连接 */
		sys_err("Invalid Handshake on %d", d->GetSocket());
		d->SetPhase(PHASE_CLOSE);
	}
	else
	{
		/* 握手成功，切换到认证/登录阶段 */
		if (d->IsPhase(PHASE_HANDSHAKE))
		{
			if (d->HandshakeProcess(p->dwTime, p->lDelta, false))
			{
#ifdef _IMPROVED_PACKET_ENCRYPTION_
				d->SendKeyAgreement();
#else
				if (g_bAuthServer)
					d->SetPhase(PHASE_AUTH);
				else
					d->SetPhase(PHASE_LOGIN);
#endif // #ifdef _IMPROVED_PACKET_ENCRYPTION_
			}
		}
		else
			d->HandshakeProcess(p->dwTime, p->lDelta, true);
	}
}

void LoginFailure(LPDESC d, const char* c_pszStatus)
{
	if (!d)
		return;

	TPacketGCLoginFailure failurePacket;

	failurePacket.header = HEADER_GC_LOGIN_FAILURE;
	strlcpy(failurePacket.szStatus, c_pszStatus, sizeof(failurePacket.szStatus));

	d->Packet(&failurePacket, sizeof(failurePacket));
}
/* 绑定握手阶段的数据包协议 */
CInputHandshake::CInputHandshake()
{
	CPacketInfoCG* pkPacketInfo = M2_NEW CPacketInfoCG;
	m_pMainPacketInfo = m_pPacketInfo;
	BindPacketInfo(pkPacketInfo);
}
 /* 释放协议对象 */
CInputHandshake::~CInputHandshake()
{
	if (NULL != m_pPacketInfo)
	{
		M2_DELETE(m_pPacketInfo);
		m_pPacketInfo = NULL;
	}
}
std::vector<TPlayerTable> g_vec_save;

// BLOCK_CHAT
ACMD(do_block_chat);
// END_OF_BLOCK_CHAT

int CInputHandshake::Analyze(LPDESC d, BYTE bHeader, const char* c_pData)
{
	/* 只处理握手阶段允许的数据包，其他包直接记录错误 */
	// if (bHeader == 10)
		// return 0;

	/* 新增修复微测试 */
	if (bHeader == HEADER_CG_SCRIPT_SELECT_ITEM)
		return 0;
	if (bHeader == HEADER_CG_DRAGON_SOUL_REFINE)
		return 0;
	
	// if (bHeader == HEADER_CG_FISHING)
		// return 0;
	// if (bHeader == HEADER_CG_ANSWER_MAKE_GUILD)
		// return 0;

	/* 公会徽章服务器登录处理 */
	if (bHeader == HEADER_CG_MARK_LOGIN)
	{
		if (!guild_mark_server)
		{
			sys_err("Guild Mark login requested but i'm not a mark server!");
			d->SetPhase(PHASE_CLOSE);
			return 0;
		}

		d->SetPhase(PHASE_LOGIN);
		return 0;
	}
	/* 频道状态检查 */
	else if (bHeader == HEADER_CG_STATE_CHECKER)
	{
		if (d->isChannelStatusRequested()) 
		{
			return 0;
		}
		d->SetChannelStatusRequested(true);
		db_clientdesc->DBPacket(HEADER_GD_REQUEST_CHANNELSTATUS, d->GetHandle(), NULL, 0);
	}
	else if (bHeader == HEADER_CG_PONG)
		Pong(d);
	else if (bHeader == HEADER_CG_HANDSHAKE)
		Handshake(d, c_pData);
#ifdef _IMPROVED_PACKET_ENCRYPTION_
	/* 增强加密的密钥协商 */
	else if (bHeader == HEADER_CG_KEY_AGREEMENT)
	{
		d->SendKeyAgreementCompleted();
		d->ProcessOutput();

		TPacketKeyAgreement* p = (TPacketKeyAgreement*)c_pData;
		if (!d->IsCipherPrepared())
		{
			sys_err("Cipher isn't prepared. %s maybe a Hacker.", inet_ntoa(d->GetAddr().sin_addr));
			d->DelayedDisconnect(5);
			return 0;
		}
		if (d->FinishHandshake(p->wAgreedLength, p->data, p->wDataLength))
		{
			if (g_bAuthServer)
			{
				d->SetPhase(PHASE_AUTH);
			}
			else
			{
				d->SetPhase(PHASE_LOGIN);
			}
		}
		else
		{
			d->SetPhase(PHASE_CLOSE);
		}
	}
#endif // _IMPROVED_PACKET_ENCRYPTION_
	else
		/* 握手阶段收到非法包，记录错误 */
		// sys_err("Handshake phase does not handle packet %d (fd %d)", bHeader, d->GetSocket());
		sys_err("HANDSHAKE phase does not handle packet %d (fd %d) (source %s:%u)", bHeader, d->GetSocket(), d->GetHostName(), d->GetPort());

	return 0;
}
