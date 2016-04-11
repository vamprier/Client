#include "Client.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

using namespace std;


bool Connect(Socket* localSocket,u_32* remoteIp, u_16* remotePort, u_16* initiate, u_16* receive)
{
 	if ( *initiate) //发起者
 	{
 		FILE* fp;
 		fp = fopen("c:/in.txt","rb");
 		if ( fp == NULL)
 		{
 			return false;
 		}
		// 获取文件大小 
		fseek (fp , 0 , SEEK_END);  
		long lSize = ftell (fp);  
		rewind (fp);
 		int msgSize = sizeof(MessagePackage);
		u_8* dataContent = new u_8[lSize];
		memset(dataContent,0x00,lSize);  
		u_16 lenth = fread(dataContent,1,lSize,fp);
		cout<<"sendto "<<*remoteIp<<" , "<<*remotePort<<endl;
		ClientSend(localSocket,remoteIp,remotePort,&dataContent[0],&lenth);
		_sleep(5);
 		fclose(fp);
		delete []dataContent;
 	}
 	else if( *receive) //接收者
 	{
 		time_t startTime = GetTime();
 		FILE* fp;
 		fp = fopen("c:/out.txt","ab");
 		if ( fp == NULL)
 		{
 			return false;
 		}
 		u_16 lenth = 0;
		u_8 dataContent[2048];
 		while(1)
		{
			_sleep(5);
 			bool isok = ClientRecevie(localSocket,&dataContent[0],&lenth);		
 			if ( isok)
 			{
				cout<<"strlen :"<<lenth<<endl;
 				lenth = fwrite(dataContent,lenth,1,fp);
				fclose(fp);
				return true;
 			}
 			else
 			{
 				time_t endTime = GetTime();
 				double diff = difftime(endTime, startTime);
 				if (diff >= TIME_OUT)
 				{
 					cout<<"time out"<<endl;
 					return false;
 				}
 			}			
 		}
 	}
}

static char Usage[] = 
	"Usage: Client [options] [parameter]\n"
	"options: \n"
	"-s stunServerAddr(like 10.21.5.254:10020)\n"
	"-t turnServerAddr(like 10.21.5.254:10040)\n"
	"-m messageServerAddr(like 10.21.5.254:10030)\n"
	"-p  local port(like 10004)\n"
	"-d timeout(like 4000)\n"
	"parameter: \n"
	"-i initiate\n"
	"-r receiver\n"
	"-direct using direct method\n"
	"-stun using stun method\n"
	"-turn using turn method\n";


int main( int argc, char** argv)
{
	//解析参数
	if ( argc == 1)
	{
		printf("%s",Usage);
		return -1;
	}
	u_16 localPort = 0;
	u_32 timeOut = 0;
	NetAddr stunServerAddr;
	NetAddr turnServerAddr;
	NetAddr messageServerAddr;
	u_16 defaultPort = 10000;
	u_16 initiate = 0; //发起者
	u_16 receiver = 0; //接收者
	u_16 isDirect = 0;
	u_16 isStun = 0;
	u_16 isTurn = 0;
	for ( int arg = 1; arg < argc; arg++)
	{
		if ( !strcmp(argv[arg],"-p"))//
		{
			arg++;
			if ( argc <= arg ) 
			{
				printf( "%s",Usage);
				return -1;
			}
			localPort = strtol( argv[arg], NULL, 10);
		}
		else if( !strcmp(argv[arg],"-d"))//
		{
			arg++;
			if ( argc <= arg ) 
			{
				printf( "%s",Usage);
				return -1;
			}
			timeOut = strtol( argv[arg], NULL, 10);
		}
		else if( !strcmp(argv[arg],"-s"))//
		{
			arg++;
			if ( argc <= arg ) 
			{
				printf( "%s",Usage);
				return -1;
			}
			bool ret = ParseHostName( argv[arg], &stunServerAddr.ip,&stunServerAddr.port,defaultPort);
			if ( !ret)
			{
				printf( "%s is not a valid host name \n",argv[arg]);
				printf( "%s",Usage);
				return -1;
			}
		}
		else if( !strcmp(argv[arg],"-t"))//
		{
			arg++;
			if ( argc <= arg ) 
			{
				printf( "%s",Usage);
				return -1;
			}
			bool ret = ParseHostName( argv[arg], &turnServerAddr.ip,&turnServerAddr.port,defaultPort);
			if ( !ret)
			{
				printf( "%s is not a valid host name \n",argv[arg]);
				printf( "%s",Usage);
				return -1;
			}
		}
		else if( !strcmp(argv[arg],"-m"))//
		{
			arg++;
			if ( argc <= arg ) 
			{
				printf("%s",Usage);
				return -1;
			}
			bool ret = ParseHostName( argv[arg], &messageServerAddr.ip,&messageServerAddr.port,defaultPort);
			if ( !ret)
			{
				printf( "%s is not a valid host name \n",argv[arg]);
				printf( "%s",Usage);
				return -1;
			}
		}
		else if( !strcmp(argv[arg],"-i"))//
		{
			initiate = 1;
		}
		else if( !strcmp(argv[arg],"-r"))//
		{
			receiver = 1;
		}
		else if( !strcmp(argv[arg],"-direct"))//
		{
			isDirect = 1;
		}
		else if( !strcmp(argv[arg],"-stun"))//
		{
			isStun = 1;
		}
		else if( !strcmp(argv[arg],"-turn"))//
		{
			isTurn = 1;
		}
	}
	if ( localPort == 0)
	{
		printf(" need port \n");
		printf("%s",Usage);
		return -1;
	}
	bool ret = initNetwork();
	u_32 localIp = 0;
	GetLocalIp(&localIp);

	Socket localSocket = openPort(localPort,localIp);
	if (localSocket == INVALID_SOCKET)
	{
		cout<<"socket error"<<endl;
		return false;
	}
	cout<<"localSocket "<<localSocket<<endl;
	u_32 pairFlag = 0x55AA55AA;
	bool isok = false;
	u_32 remoteIp;
	u_16 remotePort;
	
	//先尝试直接连接
	if( isDirect)
	{
		isok = DirectMethod(&messageServerAddr.ip,&messageServerAddr.port,&localSocket,localIp,localPort,pairFlag,&remoteIp,&remotePort);
	}
	else if( isStun)
	{
		//stun方式
		isok = stunMethod(&stunServerAddr.ip,&stunServerAddr.port,&messageServerAddr.ip,&messageServerAddr.port,
					              &localSocket,localIp,localPort,pairFlag,&remoteIp,&remotePort);
	}
	else if( isTurn)
	{
		//turn方式
		isok = turnMethod(&turnServerAddr.ip,&turnServerAddr.port,&messageServerAddr.ip,&messageServerAddr.port,
						              &localSocket,localIp,localPort,pairFlag,&remoteIp,&remotePort);
	}
	if ( !isok)
	{
		cout<<"Cannot Communicate"<<endl;
	}
	else
	{
		Connect(&localSocket,&remoteIp,&remotePort,&initiate,&receiver);
	}
	if (localSocket != INVALID_SOCKET)
	{
		CloseSocket(localSocket);
	}
	return 0;
}