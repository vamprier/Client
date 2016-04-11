////////////////////////////////////////////////////////////
//公司名称：西安影子科技有限公司
//产品名称：思扣快信
//版 本 号：1.0.0.0
//文 件 名：Client.cpp
//开发人员：赵娟
//日    期：2015-11-27
//更新人员：
//更新日期：
//文件说明：turn服务器端
////////////////////////////////////////////////////////////
#include "Client.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

using namespace std;

RequestMessagePackage rmsg; //请求包
RequestMessagePackage emsg; //交换包
MessagePackage msg; //数据包

//===========================================
//CreateRequestData函数说明
//函数功能：生成请求数据包
//参数：    localAddr：本地ip地址和端口号		
//函数返回：无 
//===========================================
void CreateRequestData( NetAddr localAddr, u_32* pairingFlag)
{
	rmsg.headFlag = PACKAGE_HEADER_FLAG;
	rmsg.dataType = REQUEST_TYPE;
	rmsg.messageContent.command = REQUEST_COMMAND;
	rmsg.messageContent.pairingFlag = *pairingFlag;
	rmsg.messageContent.localAddr = localAddr;
	memset(&rmsg.messageContent.localNatAddr,0x00,sizeof(NetAddr));
	memset(&rmsg.messageContent.remoteAddr,0x00,sizeof(NetAddr));
	memset(&rmsg.messageContent.remoteNatAddr,0x00,sizeof(NetAddr));
	rmsg.tailFlag = PACKAGE_TAIL_FLAG;
}

//===========================================
//CreateData函数说明
//函数功能：生成数据包
//参数：    无
//函数返回：无 
//===========================================
void CreateData( u_8* sendData, u_16 realLenth, u_16 dataLenth, u_16 totalNumber, u_16 dataNumber)
{
	msg.headFlag = rmsg.headFlag;
	msg.dataType = TRANSFORM_TYPE;
	msg.realMessageSize = realLenth;
	msg.messageSize = dataLenth;
	msg.totalNumber = totalNumber;
	msg.messageNumber = dataNumber;
	msg.messageContent.localAddr = rmsg.messageContent.localAddr;
	msg.messageContent.localNatAddr = rmsg.messageContent.localNatAddr;
	msg.messageContent.remoteAddr = rmsg.messageContent.remoteAddr;
	msg.messageContent.remoteNatAddr = rmsg.messageContent.remoteNatAddr;
	memcpy(msg.messageContent.dataContent,sendData,dataLenth);
	msg.tailFlag = rmsg.tailFlag;
}

//===========================================
//SendUdpMessage函数说明
//函数功能：给目标主机发送消息
//参数说明：LocalSocket：发送信息的本地端口Socket
//          msgStr：发送的字符串
//          msgLen：消息长度
//          destIp：目标主机ip
//          destPort：目标主机端口号
//          sendNum：尝试发送消息的次数，默认为3次
//函数返回：true：消息发送成功
//          false：消息发送失败
//===========================================
bool 
SendUdpMessage(Socket* LocalSocket, char* msgStr, int* msgLen, u_32* destIp,u_16* destPort)
{
	bool isok = sendMessage(*LocalSocket,msgStr,*msgLen,*destIp,*destPort);//发送数据
	if ( !isok)//第一次发送失败后，再次尝试发送sendNum次
	{
		for (int i=0;i<TRY_SEND_NUMBER;i++)
		{
			_sleep(5);
			isok = sendMessage(*LocalSocket,msgStr,*msgLen,*destIp,*destPort);//发送数据
			if (isok == true)
			{
				return isok;
			}
		}//end for
	}//end if
	return isok;
}

//===========================================
//communication函数说明
//函数功能：ServerAddr：服务器ip地址和端口号
//参数：    localSocket：本地socket连接
//			msg：数据包
//			timeOut：超时时间
//函数返回：成功或者失败 
//===========================================
bool communication( u_32* serverIp,u_16* serverPort, Socket* localSocket)
{
	int msgSize = sizeof(RequestMessagePackage);
	bool isok = SendUdpMessage(localSocket,(char*)(&rmsg),&msgSize,serverIp,serverPort);
	if ( isok)//消息发送成功
	{
		_sleep(5);
		NetAddr from;
		time_t startTime = GetTime();
		while(1)
		{
			int msgSize = sizeof(RequestMessagePackage);
			isok = getMessage(*localSocket,(char*)(&rmsg),&msgSize,&from.ip,&from.port);
			if ( isok)//消息接收成功
			{
				//判断包头包尾是否正确
				if ( !CheckRequestMessage(&rmsg))
				{
					cout<<"message type error"<<endl;
					return false;
				}
				else //
				{
					return true;
				}
			}
			else//消息接收失败，判断是否超时
			{
				time_t endTime = GetTime();
				double diff = difftime( endTime, startTime);
				if ( diff >= TIME_OUT)
				{
					cout<<"socket timeout"<<endl;
					return false;
				}
			}
		}
	} //end if
	else //消息发送失败
	{
		cout<<"false"<<endl;
		return false;
	}
}

//===========================================
//HandShakeProcess函数说明
//函数功能：握手过程
//参数说明：handMessageSocket：消息结构体
//          sendMsg：返回接收到的信息
//          destIp：目标主机的Ip
//          destPort：目标主机的端口
//          timeOut：超时时间
//          sendNum：
//函数返回：true：函数执行成功
//          false：函数执行失败
//===========================================
bool 
HandShakeProcess( Socket* localSocket, u_32* remoteIp,u_16* remotePort)
{
	//通过向STUN服务器请求STUN服务的端口向另外一端（Peer）的NAT设备IP及Port发送握手请求数据包
	int msgSize = sizeof(RequestMessagePackage);
	bool isok = SendUdpMessage( localSocket,(char*)(&rmsg),&msgSize,remoteIp,remotePort);
	time_t startTime = GetTime();
	//握手主消息循环，接收消息
	while(1)
	{	
		int msgSize = sizeof(RequestMessagePackage);
		NetAddr from;
		isok = getMessage(*localSocket,(char*)(&rmsg),&msgSize,&from.ip,&from.port);		
		if ( isok && CheckRequestMessage(&rmsg))
		{
			if ( rmsg.messageContent.command == REQUEST_COMMAND)//收到握手请求
			{
				cout<<"receive HandshakeType type is "<<REQUEST_COMMAND<<endl;
				memcpy(&rmsg,&emsg,msgSize);
				rmsg.messageContent.command = RESPOND_COMMAND;
				_sleep(5);
				bool ret = SendUdpMessage(localSocket,(char*)(&rmsg),&msgSize,&from.ip,&from.port);
				if (ret != true)
				{
					return false;
				}
				continue;
			}
			if ( rmsg.messageContent.command == RESPOND_COMMAND)//收到握手响应
			{
				cout<<"receive HandshakeType is "<<RESPOND_COMMAND<<endl;
				memcpy(&rmsg,&emsg,msgSize);
				rmsg.messageContent.command = SUCCESS_COMMAND;
				_sleep(5);
				bool ret = SendUdpMessage(localSocket,(char*)(&rmsg),&msgSize,&from.ip,&from.port);
				return true;
			} 
			if ( rmsg.messageContent.command == SUCCESS_COMMAND)//收到握手成功
			{
				cout<<"receive HandshakeType is "<<SUCCESS_COMMAND<<endl;
				memcpy(&rmsg,&emsg,msgSize);
				return true;
			}
		}
		else // 超时判断
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
	return true;
}

//===========================================
//DirectMethod函数说明
//函数功能：turn转发方式通讯
//参数：    messageServerIp：消息服务器ip地址
//			messageServerPort：消息服务器端口号
//			localSocket：本地socket连接
//			localIp：本地ip地址
//			locaPort：本地端口号
//			pairFlag：配对码
//			remoteIp：返回的对端的ip地址
//			remotePort：返回的对端的端口号
//函数返回：成功或者失败
//===========================================
bool DirectMethod( u_32* messageServerIp, u_16* messageServerPort, Socket* localSocket,
	              u_32 localIp, u_16 locaPort, u_32 pairFlag, u_32* remoteIp, u_16* remotePort)
{
	//发送消息给message服务器
	NetAddr localAddr;
	localAddr.ip = localIp;
	localAddr.port = locaPort;
	CreateRequestData( localAddr, &pairFlag);
	rmsg.dataType = EXCHANG_TYPE;
	bool isok = communication( messageServerIp, messageServerPort, localSocket);
	if (isok)
	{
		isok = ( rmsg.dataType == EXCHANG_TYPE);
	}
	//握手过程
	if ( isok)
	{
		memcpy(&emsg,&rmsg,sizeof(RequestMessagePackage));
		isok = HandShakeProcess(localSocket,&(rmsg.messageContent.remoteAddr.ip),&(rmsg.messageContent.remoteAddr.port));
	}
	if ( isok)
	{
		*remoteIp = rmsg.messageContent.remoteAddr.ip;
		*remotePort = rmsg.messageContent.remoteAddr.port;
	}
	cout<<isok<<endl;
	return isok;
}

//===========================================
//stunMethod函数说明
//函数功能：turn转发方式通讯
//参数：    stunServerIp：stun服务器ip地址
//			stunServerPort：stun服务器端口号
//			messageServerIp：消息服务器ip地址
//			messageServerPort：消息服务器端口号
//			localSocket：本地socket连接
//			localIp：本地ip地址
//			locaPort：本地端口号
//			pairFlag：配对码
//			remoteIp：返回的对端的ip地址
//			remotePort：返回的对端的端口号
//函数返回：成功或者失败
//===========================================
bool stunMethod(u_32* stunServerIp, u_16* stunServerPort,u_32* messageServerIp, u_16* messageServerPort, Socket* localSocket, 
	            u_32 localIp,u_16 locaPort,u_32 pairFlag,u_32* remoteIp, u_16* remotePort)
{
	//发送消息给stun服务器
	NetAddr localAddr;
	localAddr.ip = localIp;
	localAddr.port = locaPort;
	CreateRequestData(localAddr,&pairFlag);
	bool isok = communication( stunServerIp, stunServerPort, localSocket);
	if ( isok)
	{
		isok = ( rmsg.dataType == REQUEST_TYPE);	
	}
	cout<<rmsg.messageContent.localAddr.ip<<" "<<rmsg.messageContent.localAddr.port<<endl;
	cout<<rmsg.messageContent.localNatAddr.ip<<" "<<rmsg.messageContent.localNatAddr.port<<endl;
	//发送消息给message服务器
	if ( isok)
	{
		rmsg.dataType = EXCHANG_TYPE;
		isok = communication( messageServerIp, messageServerPort, localSocket);
	}
	if (isok)
	{
		isok = ( rmsg.dataType == EXCHANG_TYPE);		
	}

	//握手过程
	if ( isok)
	{		
		memcpy(&emsg,&rmsg,sizeof(RequestMessagePackage));
		isok = HandShakeProcess( localSocket,&(rmsg.messageContent.remoteNatAddr.ip),&(rmsg.messageContent.remoteNatAddr.port));
	}
	cout<<rmsg.messageContent.localAddr.ip<<" "<<rmsg.messageContent.localAddr.port<<endl;
	cout<<rmsg.messageContent.localNatAddr.ip<<" "<<rmsg.messageContent.localNatAddr.port<<endl;
	cout<<rmsg.messageContent.remoteAddr.ip<<" "<<rmsg.messageContent.remoteAddr.port<<endl;
	cout<<rmsg.messageContent.remoteNatAddr.ip<<" "<<rmsg.messageContent.remoteNatAddr.port<<endl;
	if ( isok)
	{
		*remoteIp = rmsg.messageContent.remoteNatAddr.ip;
		*remotePort = rmsg.messageContent.remoteNatAddr.port;
	}
	cout<<isok<<endl;
	return isok;
}

//===========================================
//turnMethod函数说明
//函数功能：turn转发方式通讯
//参数：    turnServerIp：turn服务器ip地址
//			turnServerPort：turn服务器端口号
//			messageServerIp：消息服务器ip地址
//			messageServerPort：消息服务器端口号
//			localSocket：本地socket连接
//			localIp：本地ip地址
//			locaPort：本地端口号
//			pairFlag：配对码
//			remoteIp：返回的对端的ip地址
//			remotePort：返回的对端的端口号
//函数返回：成功或者失败
//===========================================
bool turnMethod( u_32* turnServerIp, u_16* turnServerPort, u_32* messageServerIp, u_16* messageServerPort, Socket* localSocket, 
	             u_32 localIp, u_16 locaPort, u_32 pairFlag, u_32* remoteIp, u_16* remotePort)
{
	//发送消息给turn服务器
	NetAddr localAddr;
	localAddr.ip = localIp;
	localAddr.port = locaPort;
	CreateRequestData( localAddr, &pairFlag);
	bool isok = communication( turnServerIp, turnServerPort, localSocket);
	if ( isok)
	{
		isok = ( rmsg.dataType == REQUEST_TYPE);		
	}
	//发送消息给message服务器
	if ( isok)
	{
		rmsg.dataType = EXCHANG_TYPE;
		isok = communication( messageServerIp, messageServerPort, localSocket);
	}
	if ( isok)
	{
		isok = ( rmsg.dataType == EXCHANG_TYPE);
	}
	//握手过程
	if ( isok)
	{
		memcpy(&emsg,&rmsg,sizeof(RequestMessagePackage));
		isok = HandShakeProcess( localSocket, turnServerIp, turnServerPort);
	}
	cout<<rmsg.messageContent.localAddr.ip<<" "<<rmsg.messageContent.localAddr.port<<endl;
	cout<<rmsg.messageContent.localNatAddr.ip<<" "<<rmsg.messageContent.localNatAddr.port<<endl;
	cout<<rmsg.messageContent.remoteAddr.ip<<" "<<rmsg.messageContent.remoteAddr.port<<endl;
	cout<<rmsg.messageContent.remoteNatAddr.ip<<" "<<rmsg.messageContent.remoteNatAddr.port<<endl;
	if( isok)
	{
		*remoteIp = *turnServerIp;
		*remotePort = *turnServerPort;
	}
	cout<<isok<<endl;
	return isok;
}

//===========================================
//ClientSend函数说明
//函数功能：客户端发送数据
//参数：    localSocket：本地socket连接
//			remoteIp：对端ip
//			remotePort：对端port
//			sendData：发送的数据
//			dataLenth：数据长度
//函数返回：成功或者失败
//===========================================
bool ClientSend( Socket* localSocket, u_32* remoteIp, u_16* remotePort, u_8* sendData,u_16* dataLenth)
{
	if ( *dataLenth <= DATA_TOTAL_LENGTH)
	{
		CreateData(sendData,*dataLenth,*dataLenth,1,0);
		int msgSize = sizeof(msg);
		return SendUdpMessage( localSocket,(char*)(&msg),&msgSize,remoteIp,remotePort);
	}
	else
	{
		int lSize = *dataLenth;
		int n = lSize/DATA_TOTAL_LENGTH;
		int yu = lSize%DATA_TOTAL_LENGTH;
		int totalNumber = 0;
		if ( yu == 0)
		{
			totalNumber = n;
		}
		else
		{
			totalNumber = n+1;
		}
		bool isok = true;
		int okSize = 0;
		for (int i=0;i<n;i++)
		{
			CreateData(&sendData[i*DATA_TOTAL_LENGTH],*dataLenth,DATA_TOTAL_LENGTH,totalNumber,i+1);
			int msgSize = sizeof(msg);
			isok = SendUdpMessage( localSocket,(char*)(&msg),&msgSize,remoteIp,remotePort);
			if ( isok) // 发送成功的次数
			{
				okSize++;
			}
		}
		if ( yu != 0)
		{
			CreateData(&sendData[(totalNumber-1)*DATA_TOTAL_LENGTH],*dataLenth,yu,totalNumber,totalNumber);
			int msgSize = sizeof(msg);
			isok = SendUdpMessage( localSocket,(char*)(&msg),&msgSize,remoteIp,remotePort);
			if ( isok) // 发送成功的次数
			{
				okSize++;
			}
		}		
		if ( okSize == 0)
		{
			return false;
		}
		return true;
	}
}

//===========================================
//ClientRecevie函数说明
//函数功能：客户端接收数据
//参数：    localSocket：本地socket连接
//			recevieBuffer：接收到的数据
//			bufferLenth：接收到的数据长度
//函数返回：成功或者失败
//===========================================
bool ClientRecevie( Socket* localSocket,u_8* recevieBuffer,u_16* bufferLenth)
{
	NetAddr from;
	int msgSize = sizeof(msg);
	time_t startTime = GetTime();
	bool isok = getMessage(*localSocket,(char*)(&msg),&msgSize,&from.ip,&from.port);
	if ( isok)
	{
		if ( !CheckMessage(&msg))
		{
			return false;
		}
		else
		{
			if ( msg.totalNumber == 1)
			{
				memcpy(recevieBuffer,msg.messageContent.dataContent,msg.messageSize);
				*bufferLenth = msg.messageSize;
				return true;
			}
			else
			{
				int number = msg.messageNumber;
				memcpy(&recevieBuffer[(number-1)*DATA_TOTAL_LENGTH],msg.messageContent.dataContent,msg.messageSize);
				*bufferLenth = msg.realMessageSize;
				int okNumber = 1;
				int totalNumber = msg.totalNumber;
				while(1)
				{
					_sleep(5);
					int msgSize = sizeof(msg);
					bool istrue = getMessage(*localSocket,(char*)(&msg),&msgSize,&from.ip,&from.port);
					if ( istrue)
					{
						if ( !CheckMessage(&msg))
						{
							continue;
						}
						else
						{
							okNumber++;
							int number = msg.messageNumber;
							memcpy(&recevieBuffer[(number-1)*DATA_TOTAL_LENGTH],msg.messageContent.dataContent,msg.messageSize);
							if ( okNumber >= totalNumber)
							{
								return true;
							}
						}
						
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
	}
}



