/*
* Copyright (c) 2016,No Corporation
* All rights reserved.
* 
* �ļ����ƣ�NetworkInterface.cpp
* ժ    Ҫ������ӿ�ʵ�֣���ʵ��������·��ͨ��
* 
* ��ǰ�汾��1.0
* ��    �ߣ�ART
* ������ڣ�2016��04��07��
* ����޸����ڣ�2014��04��27��
*
*/
#include "stdafx.h"
#include <time.h>
#include <stdio.h>
#include <WinSock2.h>
#include <Iphlpapi.h>
#include <iostream>
//-------------------------------------------------------------------------
DWORD WINAPI ThreadProc (PVOID pParam)
{
	NetworkInterface *p = (NetworkInterface *)pParam;
	p->CapturePacket();
	return 0;
}

void ThreadExe(void *pParam)
{
    NetworkInterface *p = (NetworkInterface *)pParam;
	p->CapturePacket();
    _endthread();
}
//-------------------------------------------------------------------------
//���캯��
NetworkInterface::NetworkInterface()
{
	//��ʼ���������������
	hEventForRun = 0;
	hEventForQuit = 0;  //����
	hThread = 0;
	hWnd = 0;

	PacketNum = 0;
	
	RunStatus  = 1;
	LastErrCode = 0;
	LastErrStr[0] = 0;

	//MAC��ʼ��Ϊ00:00:00:00:00:00
	ZeroMemory(srcMAC, 6);
	ZeroMemory(srcIP4, 4);
}

//��������
NetworkInterface::~NetworkInterface()
{
	if(AdapterStat == 1)
		PacketClose(adapter.lpAdapter, adapter.lpPacket);
	//ResetEvent(hEventForQuit);
    RunStatus = 0; 
    //Sleep(500);
	//WaitForSingleObject(hEventForQuit, INFINITE);
}

//��ȡ�������
DWORD NetworkInterface::GetErrCode()
{
	return LastErrCode;
}

//��ȡ������Ϣ
char *NetworkInterface::GetErrStr()
{
	return LastErrStr;
}

int NetworkInterface::GetAdapterStat()
{
	return AdapterStat;
}

//��ȡ������������������
int NetworkInterface::GetAdapterList(char AdapterList[][1024], int &AdapterNum)
{
	//ascii strings
	char AdapterName[8192]; // string that contains a list of the network adapters
	char *temp,*temp1;

	ULONG AdapterLength;

	int i = 0;	

	AdapterLength = sizeof(AdapterName);

	if (PacketGetAdapterNames(AdapterName,&AdapterLength) == FALSE)
	{
		strcpy_s(LastErrStr, "Unable to retrieve the list of the adapters!");
		LastErrCode = GetLastError();
		return -1;
	}

	temp  = AdapterName;
	temp1 = AdapterName;

	while ( (*temp != '\0') || (*(temp-1) != '\0') )
	{
		if (*temp == '\0') 
		{
			memcpy(AdapterList[i], temp1, temp - temp1 + 1);
			temp1 = temp + 1;
			i++;
		}
		temp++;
	}
		  
	AdapterNum = i;	//��ȡ��������
	return 0;
}

//ͨ���򿪵������������ȡMAC
bool NetworkInterface::GetMACAddress(LPADAPTER &lpAdapter, u_char *localMAC)
{
	BOOLEAN		Status;
	PPACKET_OID_DATA  OidData;
	
	//����һ�������������MAC��ַ
	OidData = (PPACKET_OID_DATA)malloc(6 + sizeof(PACKET_OID_DATA));
	if (OidData == NULL) 
	{
		LastErrCode = 3;
		strcpy_s(LastErrStr, "error allocating memory!");
		return false;
	}

	//ͨ����ѯNIC����ȡ��������MAC��ַ
	//OID_802_3_PERMANENT_ADDRESS �������ַ 
	//OID_802_3_CURRENT_ADDRESS   ��mac��ַ 
	OidData->Oid = OID_802_3_CURRENT_ADDRESS;
	OidData->Length = 6;

	//��0�����һ���ڴ�����
	ZeroMemory(OidData->Data, 6);
	
	Status = PacketRequest(lpAdapter, FALSE, OidData);
	if(Status)
	{
		for(int i = 0; i < 6; ++i)
			localMAC[i] = (OidData->Data)[i];
	}
	else
	{
		LastErrCode = 4;
		strcpy_s(LastErrStr, "error retrieving the MAC address of the adapter!");
		free(OidData);
		return false;
	}

	free(OidData);
	return true;
}

//ͨ�����������ֻ�ȡMAC��ַ
bool NetworkInterface::GetMACAddress(char *adapterName, u_char *localMAC)
{
	LPADAPTER lpAdapter = PacketOpenAdapter(adapterName);
	if (!adapter.lpAdapter || (adapter.lpAdapter->hFile == INVALID_HANDLE_VALUE))
	{
		LastErrCode = GetLastError();
		strcpy_s(LastErrStr, "Unable to open the adapter!");
		return false;
	}
	GetMACAddress(lpAdapter, localMAC);
	PacketCloseAdapter(lpAdapter);
	return true;

}

//����ָ��MAC��ַ���IP��ַ
bool NetworkInterface::GetCurrentIP(u_char *MACAddress, u_char *IPV4Address)
{
	int flg = 0;
	PIP_ADAPTER_INFO pIpAdapterInfo = new IP_ADAPTER_INFO();
	unsigned long stSize = sizeof(IP_ADAPTER_INFO);

	int nRel = GetAdaptersInfo(pIpAdapterInfo,&stSize);
	if (ERROR_BUFFER_OVERFLOW==nRel)
	{
		//����������ص���ERROR_BUFFER_OVERFLOW
		//��˵��GetAdaptersInfo�������ݵ��ڴ�ռ䲻��,ͬʱ�䴫��stSize,��ʾ��Ҫ�Ŀռ��С
		delete pIpAdapterInfo;
		pIpAdapterInfo = (PIP_ADAPTER_INFO)new BYTE[stSize];
		nRel=GetAdaptersInfo(pIpAdapterInfo,&stSize); 
	}
	if (ERROR_SUCCESS==nRel)
	{
		PIP_ADAPTER_INFO tpIpAdapterInfo = pIpAdapterInfo;
		while (tpIpAdapterInfo)
		{
			if(memcmp(MACAddress, tpIpAdapterInfo->Address, 6) == 0)
			{
				flg = 1;
				char *pIpAddrString =(tpIpAdapterInfo->IpAddressList.IpAddress.String);
				int i = 0, off = 0;
				while(pIpAddrString[off] != 0)
				{
					if(pIpAddrString[off] == '.')
					{
						i++;
						off++;
						continue;
					}
					IPV4Address[i] = IPV4Address[i] * 10 + pIpAddrString[off] - '0';
					off++;
				}
			}
			tpIpAdapterInfo = tpIpAdapterInfo->Next;
		}
	}
	if(pIpAdapterInfo)
	{
		delete[] pIpAdapterInfo;
	}
	if(flg == 1)
		return true;
	else
		return false;
}

//�����ѻ�ȡ��MAC��ַ����IP
bool NetworkInterface::GetCurrentIP()
{
	if(GetCurrentIP(srcMAC, srcIP4))
		return true;
	else
		return false;
}

//��ȡ�Ѵ�������IP��ַ
int NetworkInterface::GetLocalIP(u_char *IP4Address)
{
    u_char IP4[4] = {192,168,1,18};
    memcpy(IP4Address, IP4, 4);
    //memcpy(IP4Address, srcIP4, 4);
	return 0;
}

//��ȡ�Ѵ�������MAC��ַ
int NetworkInterface::GetLocalMAC(u_char *MACAddress)
{
	memcpy(MACAddress, srcMAC, 6);
	return 0;
}

//��ʼ���������������ͳ�ʼ�����ṹ
int NetworkInterface::InitAdapterCommon(char *adapterName)
{
	//����������
	adapter.lpAdapter = PacketOpenAdapter(adapterName);
	if (!adapter.lpAdapter || (adapter.lpAdapter->hFile == INVALID_HANDLE_VALUE))
	{
		LastErrCode = GetLastError();
		strcpy_s(LastErrStr, "Unable to open the adapter!");
		return -1;
	}
	//���䲢��ʼ��һ�����ṹ�������ڽ������ݰ�
	if ((adapter.lpPacket = PacketAllocatePacket()) == NULL)
	{
		LastErrCode = GetLastError();
		strcpy_s(LastErrStr, "Error: failed to allocate the LPPACKET structure.");
		return (-1);
	}
	//�˴�����Ķ�̬�ڴ��� PacketClose()���ͷ�
	char *buffer = new char[256000];	//256KB
	PacketInitPacket(adapter.lpPacket,(char*)buffer,256000);

	//������������512K�Ļ�����
	if (PacketSetBuff(adapter.lpAdapter, 512000) == FALSE)
	{
		LastErrCode = GetLastError();
		strcpy_s(LastErrStr, "Unable to set the kernel buffer!");
		return -1;
	}
	AdapterStat = 1;
	return 0;
}

//���ý���Packet��ʱ
int NetworkInterface::SetRecvTimeout(int timeOut) //ms
{
	if (PacketSetReadTimeout(adapter.lpAdapter, timeOut) == FALSE)
	{
		LastErrCode = GetLastError();
		strcpy_s(LastErrStr, "Warning: unable to set the read tiemout!");
		return -1;
	}
	return 0;
}

//���ý���Packetģʽ
int NetworkInterface::SetRecvMode(DWORD mode)
{
	if (PacketSetHwFilter(adapter.lpAdapter, mode) == FALSE)
	{
		LastErrCode = GetLastError();
		strcpy_s(LastErrStr, "Warning: unable to set promiscuous mode!");
		return -1;
	}
    return 0;
}

//ȡ���Ѵ�������MAC��ַ
int NetworkInterface::SetLocalMAC()
{
	if( !GetMACAddress(adapter.lpAdapter, srcMAC) )
	{
		LastErrCode = 0x00;
		strcpy_s(LastErrStr, "Failed to Set the MAC Address!");
		return -1;
	}
	return 0;
}

//����Packet�ⲿ����ӿ�
int NetworkInterface::SetHandleInterface(ProtocolChannel *pc)
{
	p = pc;
	return 0;
}

int NetworkInterface::DelHandleInterface()
{
    //p = NULL;
    return 0;
}

//�����¼����߳�
int NetworkInterface::ReadyForCapture()
{
	hEventForRun =  CreateEvent (NULL, TRUE, FALSE, NULL);
	hEventForQuit = CreateEvent (NULL, TRUE, FALSE, NULL);
	//hThread = CreateThread(NULL, 0, ThreadProc, this, 0, NULL);
    hThread = _beginthread(ThreadExe, 0, this);
	RunStatus = 1;
	return 0;
}

//�ر���������
void NetworkInterface::PacketClose(LPADAPTER &lpAdapter, LPPACKET &lpPacket)
{
	//free a PACKET structure
	delete (lpPacket->Buffer);
	lpPacket->Length = 0;
	PacketFreePacket(lpPacket);
	
	//�ر��������˳�
	PacketCloseAdapter(lpAdapter);
}


//ץ��
int NetworkInterface::CapturePacket()
{
	while(1)
	{
		if(!RunStatus)
			break;
		WaitForSingleObject(hEventForRun, INFINITE);
		if (PacketReceivePacket(adapter.lpAdapter, adapter.lpPacket, TRUE) == FALSE)
		{
			LastErrCode = GetLastError();
			strcpy_s(LastErrStr, "Error: PacketReceivePacket failed");
			return (-1);
		}

        if(!RunStatus)
			break;
		/*�ڴ˴��������ݰ�������*/
		SplitPackets(adapter.lpPacket);
	}
	SetEvent(hEventForQuit);
	return 0;
}

//ץ��������,����������Packet
void NetworkInterface::SplitPackets(LPPACKET &lpPacket)
{
	ULONG   ulBytesReceived;
	char	*pChar;
	char	*buf;
	u_int   off = 0;

	//caplenΪ��������ݳ��ȣ�datalenΪԭʼ���ݳ���
	//caplen����С��datalen
	u_int   caplen, datalen;	
	struct  bpf_hdr *hdr;
	
	//�յ����ֽ���
	ulBytesReceived = lpPacket->ulBytesReceived;

	//�������׵�ַ
	buf = (char*)lpPacket->Buffer;

	//��ʼ��ƫ����Ϊ0
	off = 0;
	
	while (off < ulBytesReceived)
	{	
		hdr = (struct bpf_hdr *)(buf + off);
		datalen = hdr->bh_datalen;
		caplen  = hdr->bh_caplen;

		//У�鲶��İ��Ƿ�����,���ڲ������İ���������
		if(caplen != datalen)
			break;
		off += hdr->bh_hdrlen;

		//�������ݶ��׵�ַ
		pChar = (char*)(buf + off);

		//������һ�������׵�ַ�����ܲ����ڣ�
		off = Packet_WORDALIGN(off + caplen);

		//�����Լ���Packet������Ӧ
		if(memcmp((pChar + 6), srcMAC, 6) == 0)
			continue;
		/*�ڴ˴�����ÿ�����ݰ����׵�ַΪpChar,����Ϊcaplen*/
		//�ڴ˴������ⲿ�����������ݰ�
		//PrintPacket((u_char*)pChar, caplen, 0);
        if(p != NULL)
		    p->HandlePacket((u_char*)pChar, caplen);	
	}
}

//����
int NetworkInterface::SendPacket(u_char *buffer, int packetLen, int repeatNum)
{
	
	LPPACKET   lpPacket;

	memcpy((buffer + 6), srcMAC, 6);
	//���䲢��ʼ��һ�����ṹ�������ڷ������ݰ�
	if ( (lpPacket = PacketAllocatePacket()) == NULL )
	{
		LastErrCode = GetLastError();
		strcpy_s(LastErrStr, "Error: failed to allocate the LPPACKET structure.");
		return (-1);
	}

	PacketInitPacket(lpPacket, buffer, packetLen);

	if (PacketSetNumWrites(adapter.lpAdapter, repeatNum) == FALSE)
	{
		LastErrCode = GetLastError();
		strcpy_s(LastErrStr, "Unable to send more than one packet in a single write!");
		PacketFreePacket(lpPacket);
		return -1;
	}
	
	if(PacketSendPacket(adapter.lpAdapter, lpPacket, TRUE) == FALSE)
	{
		LastErrCode = GetLastError();
		strcpy_s(LastErrStr, "Error sending the packets!");
		PacketFreePacket(lpPacket);
		return -1;
	}
	PacketFreePacket(lpPacket);
	//�����͵����ݰ�ת�浽�ļ���
	//PrintPacket(buffer, packetLen, 1);
	return 0;
}

//��ʼץ��
void NetworkInterface::StartRecvPacket()
{
	SetEvent(hEventForRun);
}

//��ͣץ��
void NetworkInterface::PauseRecvPacket()
{
	ResetEvent(hEventForRun);
}

//ֹͣץ��
void NetworkInterface::StopRecvPacket()
{
    ResetEvent(hEventForQuit);
    RunStatus = 0; 
	WaitForSingleObject(hEventForQuit, INFINITE);
}

//�����ݰ����浽��־��
void  NetworkInterface::PrintPacket(u_char *buf, int len, int type)
{
	int	i, j, ulLines, ulen, tlen = len;
	u_char	*pChar = buf, *pLine, *base = buf;
	time_t timer;
	tm *ltm;
	if(type == 1)
		freopen("SendLog.txt", "at", stdout);
	else 
		freopen("RecvLog.txt", "at", stdout);

	timer = time(NULL);
	ltm = localtime(&timer);
	printf("Time: %d-%d-%d %d:%d:%d , Length: %d\n", ltm->tm_year + 1900, ltm->tm_mon, ltm->tm_mday, 
		ltm->tm_hour, ltm->tm_min, ltm->tm_sec, len);
	//ÿ��16�����ݣ�����ܹ������Ŷ�����
	ulLines = (tlen + 15) / 16;
		
	for ( i = 0; i < ulLines; i++ )
	{
		pLine = pChar;
		printf( "%08lx : ", pChar-base );
		ulen = tlen;
		ulen = ( ulen > 16 ) ? 16 : ulen;
		tlen -= ulen;
		for ( j = 0; j< ulen; j++ )
			printf( "%02x ", *(BYTE *)pChar++ );

		if ( ulen < 16 )
			printf( "%*s", (16 - ulen) * 3, " " );

		pChar = pLine;

		for ( j = 0; j < ulen; j++, pChar++ )
			printf( "%c", isprint( (u_char)*pChar ) ? *pChar : '.' );

		printf( "\n" );
	} 
	printf( "\n" );
	fclose(stdout);
}

//�����б�����
int NetworkInterface::RefreshAdapterList()
{
	char AdapterListStr[10][1024];
	int AdapterNum = 10;
	
	GetAdapterList(AdapterListStr, AdapterNum);

	//���ԭ�е�������Ϣ
	for(int i = 0; i < AdapterList.size(); ++i)
		FreeAdapter(&(AdapterList[i]));
	AdapterList.clear();
	Adapter adapter;
	for(int i = 0; i < AdapterNum; ++i)
	{
		adapter.AdapterName = new char[strlen(AdapterListStr[i]) + 2];
		strcpy(adapter.AdapterName, AdapterListStr[i]);
		AdapterList.push_back(adapter);
	}
	return 0;
}

//��ȡ����������Ŀ
int NetworkInterface::GetAdapterNum()
{
	return AdapterList.size();
}

//��ȡָ��������Ϣ
int NetworkInterface::GetAdapterObj(Adapter &adapter, int adapterIndex)
{
	if(adapterIndex < 0 || adapterIndex >= AdapterList.size())
		return -1;
	adapter = AdapterList[adapterIndex];
	return 0;
}

//��ָ��������packet�ⲿ�������������趨
int NetworkInterface::OpenAdapter(int adapterIndex)
{
	if(adapterIndex < 0 || adapterIndex >= AdapterList.size())
	{
		return -1;
	}
	if(GetAdapterStat() == 1)		//��������Ѵ����ټ�������
		return 1;
	InitAdapterCommon(AdapterList[adapterIndex].AdapterName);
	SetRecvTimeout(20);
	SetRecvMode(NDIS_PACKET_TYPE_DIRECTED);
	SetLocalMAC();
	GetCurrentIP();
	ReadyForCapture();
	return 0;
}