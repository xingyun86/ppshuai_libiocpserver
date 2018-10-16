#pragma once

//#include <MacroDefine.h>

#define RESPONSELEN                     512                  //���ַ���
#define ACCEPTKEYLEN                    512                  //������Կ
#define PACKDATALEN                     1024                 //�������
#define ACCEPTDATALEN                   1024                 //��������
#define UNPACKDATA                      1024                 //�������

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
	char                             m_ResponseHeader[RESPONSELEN];        //���ַ���
	char                             m_AcceptKey[ACCEPTKEYLEN];            //������Կ
	char                             m_PackData[PACKDATALEN];              //�������
	char                             m_AcceptData[ACCEPTDATALEN];          //��������
	char                             m_UnpackData[UNPACKDATA];             //�������
public:
	WebSocket();
	~WebSocket();
public:
	//��������
	bool WebAccept(int sock, char * buf, int len);
	//������Ϣ
	int WebSend(int sock, const char* buf, int bufLen);
	//������Ϣ
	int WebRecv(char* buf, int bufLen);

private:
	//������Կ
	bool  GetAcceptKey(int sock, char * buf, int len);
	//��������
	void  shakeHand(int connfd, char *serverKey);
	//���ݷ��
	char* packData(const char * message, unsigned long * len, unsigned long n);

private:
	//�����С
	int  tolower(int c);
	//����ת��
	int  htoi(const char s[], int start, int len);
private:
	//���ݱ���
	char *base64_encode(const char* data, int data_len);
private:
	//��ʼ��SHA1Context
	void SHA1Reset(SHA1Context *);
	//������֤
	int  SHA1Result(SHA1Context *);
	//��ȡ����
	void SHA1Input(SHA1Context *, const char *, unsigned int);
	//�����㷨
	void SHA1ProcessMessageBlock(SHA1Context *);
	//�����㷨
	void SHA1PadMessage(SHA1Context *);
	//�㷨���
	char * sha1_hash(const char *source);

};
