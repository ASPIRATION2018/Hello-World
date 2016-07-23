/*
* Copyright (c) 2016,No Corporation
* All rights reserved.
* 
* �ļ����ƣ�NetworkInterface.h
* ժ    Ҫ������ӿ�ʵ�֣���ʵ��������·��ͨ��
* 
* ��ǰ�汾��1.0
* ��    �ߣ�ART
* ������ڣ�2016��04��07��
* ����޸����ڣ�2014��04��27��
*
*/
#ifndef __NETWORK_INTERFACE__
#define __NETWORK_INTERFACE__

class HAdapter
{
public:
	LPADAPTER lpAdapter;
	LPPACKET  lpPacket;
	int mode;

	HAdapter()
		:lpAdapter(NULL),
		lpPacket(NULL),
		mode(0)
	{}
};

class ProtocolChannel;

class NetworkInterface
{
private:
    uintptr_t	hThread;    //�����߳̾��
	//HANDLE	hThread;		
	HWND	hWnd;			//�������������ڷ�����Ϣ
	int		PacketNum;

	HAdapter adapter;

	char AdapterStat;
	
	//����Ϊ0ʱ����ʾ�˳������߳�
	int		RunStatus; 
	DWORD	LastErrCode;
	char	LastErrStr[1024];

	//��������MAC��ַ
	unsigned char srcMAC[6];
	unsigned char srcIP4[4];

	//�����б�
	std::vector<Adapter>    AdapterList;

	ProtocolChannel *p;
public:
	HANDLE hEventForRun;
	HANDLE hEventForQuit;
public:
	/*�������������*/
	NetworkInterface();
	~NetworkInterface();

	//��ȡ������Ϣ
	DWORD GetErrCode();
	char  *GetErrStr();

	//��ȡ״̬���������Ƿ��
	int   GetAdapterStat();

	//��ȡ��Ϣ���������б�MAC��IP
	int   GetAdapterList(char AdapterList[10][1024], int &AdapterNum);
	bool  GetMACAddress(LPADAPTER &lpAdapter, u_char *localMAC);
	bool  GetMACAddress(char *AdapterName, u_char *localMAC);
	bool  GetCurrentIP(u_char *MACAddress, u_char *IPV4Address);
	bool  GetCurrentIP();
	int   GetLocalIP(u_char *IP4Address);
	int   GetLocalMAC(u_char *MACAddress);

	//��ʼ�����ú���
	int   InitAdapterCommon(char *adapterName);
	int   SetRecvTimeout(int timeOut); //ms
	int   SetRecvMode(DWORD mode);
	int   SetLocalMAC();
	int   SetHandleInterface(ProtocolChannel *pc);
    int   DelHandleInterface();
	int   ReadyForCapture();

	//�ر����������ͷŽ��ջ�����
	void  PacketClose(LPADAPTER &lpAdapter, LPPACKET &lpPacket);

	//�������հ����ָ������packet
	int   SendPacket(u_char *buffer, int packetLen, int repeatNum);
	int   CapturePacket();
	void  SplitPackets(LPPACKET &lpPacket);
	

	void  StartRecvPacket();
	void  PauseRecvPacket();
    void  StopRecvPacket();

	void  PrintPacket(u_char *buf, int len, int type);

	//�����б�����
    int RefreshAdapterList();
	int GetAdapterNum();
	int GetAdapterObj(Adapter &adapter, int adapterIndex);

	//��ʼ������
	int OpenAdapter(int adapterIndex);


};

#endif