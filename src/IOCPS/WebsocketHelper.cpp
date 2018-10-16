#include "WebsocketHelper.h"
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <Windows.h>
#define SHA1CircularShift(bits,word) ((((word) << (bits)) & 0xFFFFFFFF) | ((word) >> (32-(bits))))

const char base[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

WebSocket::WebSocket()
{
}


WebSocket::~WebSocket()
{
}
//建立连接
bool WebSocket::WebAccept(int sock, char * buf, int len)
{
	if (!GetAcceptKey(sock, buf, len))
		return false;
	return true;
}

//获取密钥
bool WebSocket::GetAcceptKey(int sock, char * buf, int len)
{
	char *flag = "Sec-WebSocket-Key: ";

	const char * GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

	if (!buf || !len)
		return false;
	memset(m_AcceptKey, 0, sizeof(m_AcceptKey));
	char * keyBegin = strstr((char *)buf, flag);
	keyBegin += strlen(flag);
	int bufLen = strlen(buf);
	for (int i = 0; i<bufLen; i++)
	{
		if (keyBegin[i] == 0x0A || keyBegin[i] == 0x0D)
		{
			break;
		}
		m_AcceptKey[i] = keyBegin[i];
	}

	strcat(m_AcceptKey, GUID);
	char* sha1DataTemp = sha1_hash(m_AcceptKey);

	int sha1Datalen = strlen(sha1DataTemp);
	for (int i = 0; i<256; i++)
	{
		m_AcceptKey[i] = 0;
	}
	for (int i = 0; i<sha1Datalen; i += 2)
	{
		m_AcceptKey[i / 2] = htoi(sha1DataTemp, i, 2);
	}
	char * serverKey = base64_encode(m_AcceptKey, sha1Datalen / 2);
	//二次握手
	shakeHand(sock, serverKey);
	return true;
}
//二次握手
void  WebSocket::shakeHand(int connfd, char *serverKey)
{
	memset(m_ResponseHeader, 0, sizeof(m_ResponseHeader));
	if (!connfd || !serverKey)
		return;

	sprintf(m_ResponseHeader, "HTTP/1.1 101 Switching Protocols\r\n");
	sprintf(m_ResponseHeader, "%sUpgrade: websocket\r\n", m_ResponseHeader);
	sprintf(m_ResponseHeader, "%sConnection: Upgrade\r\n", m_ResponseHeader);
	sprintf(m_ResponseHeader, "%sSec-WebSocket-Accept: %s\r\n\r\n", m_ResponseHeader, serverKey);

	send(connfd, m_ResponseHeader, strlen(m_ResponseHeader), 0);
}
//接收数据
int WebSocket::WebRecv(char* buf, int bufLen)
{
	//接收数据
	memset(m_UnpackData, 0, sizeof(m_UnpackData));
	memcpy(m_UnpackData, buf, bufLen);
	memset(buf, 0, bufLen);
	// 1bit，1表示最后一帧
	char fin = (m_UnpackData[0] & 0x80) == 0x80;
	// 超过一帧暂不处理
	if (!fin || bufLen < 2 || bufLen <= 0 || (m_UnpackData[0] & 0xF) == 8)
	{
		return 0;
	}
	// 是否包含掩码
	char maskFlag = (m_UnpackData[1] & 0x80) == 0x80;
	// 不包含掩码的暂不处理
	if (!maskFlag)
	{
		return NULL;
	}
	char * payloadData = NULL;
	// 数据长度
	unsigned int payloadLen = m_UnpackData[1] & 0x7F;
	char masks[4] = { 0 };
	if (payloadLen == 126)
	{
		memcpy(masks, m_UnpackData + 4, 4);
		payloadLen = (m_UnpackData[2] & 0xFF) << 8 | (m_UnpackData[3] & 0xFF);
		payloadLen = bufLen > payloadLen ? payloadLen : bufLen;
		memset(buf, 0, payloadLen);
		memcpy(buf, m_UnpackData + 8, payloadLen);
	}
	else if (payloadLen == 127)
	{
		char temp[8] = { 0 };
		memcpy(masks, m_UnpackData + 10, 4);
		for (int i = 0; i < 8; i++)
		{
			temp[i] = m_UnpackData[9 - i];
		}
		unsigned long n = 0;
		memcpy(&n, temp, 8);
		payloadLen = bufLen > n ? n : bufLen;
		memset(buf, 0, payloadLen);
		memcpy(buf, m_UnpackData + 14, payloadLen);//toggle error(core dumped) if data is too long.
	}
	else
	{
		memcpy(masks, m_UnpackData + 2, 4);
		payloadLen = bufLen > payloadLen ? payloadLen : bufLen;
		memset(buf, 0, payloadLen);
		memcpy(buf, m_UnpackData + 6, payloadLen);
	}

	for (int i = 0; i < payloadLen; i++)
	{
		buf[i] = (char)(buf[i] ^ masks[i % 4]);
	}
	return strlen(buf);
}

//数据封包
char* WebSocket::packData(const char * message, unsigned long * len, unsigned long n)
{
	memset(m_PackData, 0, sizeof(m_PackData));
	if (n < 126)
	{
		m_PackData[0] = 0x82;
		m_PackData[1] = n;
		memcpy(m_PackData + 2, message, n);
		*len = n + 2;
	}
	else if (n < PACKDATALEN)
	{
		m_PackData[0] = 0x82;
		m_PackData[1] = 126;
		m_PackData[2] = (n >> 8 & 0xFF);
		m_PackData[3] = (n & 0xFF);
		memcpy(m_PackData + 4, message, n);
		*len = n + 4;
	}
	else
	{
		// 暂不处理超长内容
		*len = 0;
	}
	return m_PackData;
}
//发送消息
int WebSocket::WebSend(int sock, const char* buf, int bufLen)
{
	if (!sock)
		return 0;

	unsigned long n = 0;
	char * data = packData(buf, &n, bufLen);
	if (!data || n <= 0)
		return 0;
	return send(sock, data, n, 0);
}
//将大改小
int  WebSocket::tolower(int c)
{
	if (c >= 'A' && c <= 'Z')
	{
		return c + 'a' - 'A';
	}
	return c;
}
//类型转换
int  WebSocket::htoi(const char s[], int start, int len)
{
	int i;
	int n = 0;
	if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) //判断是否有前导0x或者0X
		i = 2;
	else
		i = 0;

	i += start;

	for (int j = 0; (s[i] >= '0' && s[i] <= '9')
		|| (s[i] >= 'a' && s[i] <= 'f') || (s[i] >= 'A' && s[i] <= 'F'); ++i)
	{
		if (j >= len)
		{
			break;
		}
		if (tolower(s[i]) > '9')
		{
			n = 16 * n + (10 + tolower(s[i]) - 'a');
		}
		else
		{
			n = 16 * n + (tolower(s[i]) - '0');
		}
		j++;
	}
	return n;
}
//数据编码
char *WebSocket::base64_encode(const char* data, int data_len)
{
	int RetLen = data_len / 3;
	int temp = data_len % 3;
	if (temp > 0)
	{
		RetLen += 1;
	}
	RetLen = RetLen * 4 + 1;
	char *RetData = (char *)malloc(RetLen);
	if (RetData == NULL)
	{
		printf("No enough memory.\n");
		exit(0);
	}
	memset(RetData, 0, RetLen);
	char *RetTemp = RetData;
	int tmp = 0;
	while (tmp < data_len)
	{
		temp = 0;
		int prepare = 0;
		char changed[4] = { 0 };
		while (temp < 3)
		{
			//printf("tmp = %d\n", tmp);
			if (tmp >= data_len)
			{
				break;
			}
			prepare = ((prepare << 8) | (data[tmp] & 0xFF));
			tmp++;
			temp++;
		}
		prepare = (prepare << ((3 - temp) * 8));
		//printf("before for : temp = %d, prepare = %d\n", temp, prepare);
		for (int i = 0; i < 4; i++)
		{
			if (temp < i)
			{
				changed[i] = 0x40;
			}
			else
			{
				changed[i] = (prepare >> ((3 - i) * 6)) & 0x3F;
			}
			*RetTemp = base[changed[i]];
			//printf("%.2X", changed[i]);
			RetTemp++;
		}
	}
	*RetTemp = '\0';

	return RetData;
}

void WebSocket::SHA1Reset(SHA1Context * context)
{
	context->Length_Low = 0;
	context->Length_High = 0;
	context->Message_Block_Index = 0;

	context->Message_Digest[0] = 0x67452301;
	context->Message_Digest[1] = 0xEFCDAB89;
	context->Message_Digest[2] = 0x98BADCFE;
	context->Message_Digest[3] = 0x10325476;
	context->Message_Digest[4] = 0xC3D2E1F0;

	context->Computed = 0;
	context->Corrupted = 0;
}

int WebSocket::SHA1Result(SHA1Context * context)
{
	if (context->Corrupted)
		return 0;

	if (!context->Computed) {
		SHA1PadMessage(context);
		context->Computed = 1;
	}
	return 1;
}

void WebSocket::SHA1Input(SHA1Context * context, const char *message_array, unsigned int length)
{
	if (!length) return;

	if (context->Computed || context->Corrupted) {
		context->Corrupted = 1;
		return;
	}

	while (length-- && !context->Corrupted) {
		context->Message_Block[context->Message_Block_Index++] = (*message_array & 0xFF);

		context->Length_Low += 8;

		context->Length_Low &= 0xFFFFFFFF;
		if (context->Length_Low == 0) {
			context->Length_High++;
			context->Length_High &= 0xFFFFFFFF;
			if (context->Length_High == 0) context->Corrupted = 1;
		}

		if (context->Message_Block_Index == 64) {
			SHA1ProcessMessageBlock(context);
		}
		message_array++;
	}
}

void WebSocket::SHA1ProcessMessageBlock(SHA1Context * context)
{
	const unsigned K[] = { 0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6 };
	int         t;
	unsigned    temp;
	unsigned    W[80];
	unsigned    A, B, C, D, E;

	for (t = 0; t < 16; t++) {
		W[t] = ((unsigned)context->Message_Block[t * 4]) << 24;
		W[t] |= ((unsigned)context->Message_Block[t * 4 + 1]) << 16;
		W[t] |= ((unsigned)context->Message_Block[t * 4 + 2]) << 8;
		W[t] |= ((unsigned)context->Message_Block[t * 4 + 3]);
	}

	for (t = 16; t < 80; t++)  W[t] = SHA1CircularShift(1, W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16]);

	A = context->Message_Digest[0];
	B = context->Message_Digest[1];
	C = context->Message_Digest[2];
	D = context->Message_Digest[3];
	E = context->Message_Digest[4];

	for (t = 0; t < 20; t++) {
		temp = SHA1CircularShift(5, A) + ((B & C) | ((~B) & D)) + E + W[t] + K[0];
		temp &= 0xFFFFFFFF;
		E = D;
		D = C;
		C = SHA1CircularShift(30, B);
		B = A;
		A = temp;
	}
	for (t = 20; t < 40; t++) {
		temp = SHA1CircularShift(5, A) + (B ^ C ^ D) + E + W[t] + K[1];
		temp &= 0xFFFFFFFF;
		E = D;
		D = C;
		C = SHA1CircularShift(30, B);
		B = A;
		A = temp;
	}
	for (t = 40; t < 60; t++) {
		temp = SHA1CircularShift(5, A) + ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
		temp &= 0xFFFFFFFF;
		E = D;
		D = C;
		C = SHA1CircularShift(30, B);
		B = A;
		A = temp;
	}
	for (t = 60; t < 80; t++) {
		temp = SHA1CircularShift(5, A) + (B ^ C ^ D) + E + W[t] + K[3];
		temp &= 0xFFFFFFFF;
		E = D;
		D = C;
		C = SHA1CircularShift(30, B);
		B = A;
		A = temp;
	}
	context->Message_Digest[0] = (context->Message_Digest[0] + A) & 0xFFFFFFFF;
	context->Message_Digest[1] = (context->Message_Digest[1] + B) & 0xFFFFFFFF;
	context->Message_Digest[2] = (context->Message_Digest[2] + C) & 0xFFFFFFFF;
	context->Message_Digest[3] = (context->Message_Digest[3] + D) & 0xFFFFFFFF;
	context->Message_Digest[4] = (context->Message_Digest[4] + E) & 0xFFFFFFFF;
	context->Message_Block_Index = 0;
}

void WebSocket::SHA1PadMessage(SHA1Context * context)
{
	if (context->Message_Block_Index > 55) {
		context->Message_Block[context->Message_Block_Index++] = 0x80;
		while (context->Message_Block_Index < 64)  context->Message_Block[context->Message_Block_Index++] = 0;
		SHA1ProcessMessageBlock(context);
		while (context->Message_Block_Index < 56) context->Message_Block[context->Message_Block_Index++] = 0;
	}
	else {
		context->Message_Block[context->Message_Block_Index++] = 0x80;
		while (context->Message_Block_Index < 56) context->Message_Block[context->Message_Block_Index++] = 0;
	}
	context->Message_Block[56] = (context->Length_High >> 24) & 0xFF;
	context->Message_Block[57] = (context->Length_High >> 16) & 0xFF;
	context->Message_Block[58] = (context->Length_High >> 8) & 0xFF;
	context->Message_Block[59] = (context->Length_High) & 0xFF;
	context->Message_Block[60] = (context->Length_Low >> 24) & 0xFF;
	context->Message_Block[61] = (context->Length_Low >> 16) & 0xFF;
	context->Message_Block[62] = (context->Length_Low >> 8) & 0xFF;
	context->Message_Block[63] = (context->Length_Low) & 0xFF;

	SHA1ProcessMessageBlock(context);
}

char * WebSocket::sha1_hash(const char *source)
{
	SHA1Context sha;
	char *buf;//[128];

	SHA1Reset(&sha);
	SHA1Input(&sha, source, strlen(source));

	if (!SHA1Result(&sha))
	{
		printf("SHA1 ERROR: Could not compute message digest");
		return NULL;
	}
	else
	{
		buf = (char *)malloc(128);
		memset(buf, 0, sizeof(buf));
		sprintf(buf, "%08X%08X%08X%08X%08X", sha.Message_Digest[0], sha.Message_Digest[1],
			sha.Message_Digest[2], sha.Message_Digest[3], sha.Message_Digest[4]);
		return buf;
	}
	return NULL;
}
