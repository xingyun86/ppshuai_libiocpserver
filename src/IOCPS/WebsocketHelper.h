#pragma once

//#include <MacroDefine.h>

#define RESPONSELEN                     512                  //握手返回
#define ACCEPTKEYLEN                    512                  //连接密钥
#define PACKDATALEN                     1024                 //封包数据
#define ACCEPTDATALEN                   1024                 //建立连接
#define UNPACKDATA                      1024                 //解包数据

typedef struct SHA1Context {

	unsigned Message_Digest[5];

	unsigned Length_Low;

	unsigned Length_High;

	unsigned char Message_Block[64];

	int Message_Block_Index;

	int Computed;

	int Corrupted;

} SHA1Context;

class WebSocket
{
private:
	char                             m_ResponseHeader[RESPONSELEN];        //握手返回
	char                             m_AcceptKey[ACCEPTKEYLEN];            //连接密钥
	char                             m_PackData[PACKDATALEN];              //封包数据
	char                             m_AcceptData[ACCEPTDATALEN];          //建立连接
	char                             m_UnpackData[UNPACKDATA];             //解包数据
public:
	WebSocket();
	~WebSocket();
public:
	//建立连接
	bool WebAccept(int sock, char * buf, int len);
	//发送消息
	int WebSend(int sock, const char* buf, int bufLen);
	//接收消息
	int WebRecv(char* buf, int bufLen);

private:
	//连接密钥
	bool  GetAcceptKey(int sock, char * buf, int len);
	//二次握手
	void  shakeHand(int connfd, char *serverKey);
	//数据封包
	char* packData(const char * message, unsigned long * len, unsigned long n);

private:
	//将大改小
	int  tolower(int c);
	//类型转换
	int  htoi(const char s[], int start, int len);
private:
	//数据编码
	char *base64_encode(const char* data, int data_len);
private:
	//初始化SHA1Context
	void SHA1Reset(SHA1Context *);
	//数据验证
	int  SHA1Result(SHA1Context *);
	//提取数据
	void SHA1Input(SHA1Context *, const char *, unsigned int);
	//编码算法
	void SHA1ProcessMessageBlock(SHA1Context *);
	//编码算法
	void SHA1PadMessage(SHA1Context *);
	//算法入口
	char * sha1_hash(const char *source);

};
