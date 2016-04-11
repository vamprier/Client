////////////////////////////////////////////////////////////
//公司名称：西安影子科技有限公司
//产品名称：思扣快信
//版 本 号：1.0.0.0
//文 件 名：Client.h
//开发人员：赵娟
//日    期：2015-11-27
//更新人员：
//更新日期：
//文件说明：turn服务器端
////////////////////////////////////////////////////////////

#ifndef CLIENT_H
#define CLIENT_H

#include "typedef.h"

//直连方式
bool DirectMethod( u_32* messageServerIp, u_16* messageServerPort, Socket* localSocket,
	u_32 localIp, u_16 locaPort, u_32 pairFlag, u_32* remoteIp, u_16* remotePort);
//stun方式
bool stunMethod(u_32* stunServerIp, u_16* stunServerPort,u_32* messageServerIp, u_16* messageServerPort, Socket* localSocket, 
	u_32 localIp,u_16 locaPort,u_32 pairFlag,u_32* remoteIp, u_16* remotePort);
//turn方式
bool turnMethod( u_32* turnServerIp, u_16* turnServerPort, u_32* messageServerIp, u_16* messageServerPort, Socket* localSocket, 
	u_32 localIp, u_16 locaPort, u_32 pairFlag, u_32* remoteIp, u_16* remotePort);
//客户端发送
bool ClientSend( Socket* localSocket, u_32* remoteIp, u_16* remotePort, u_8* sendData,u_16* dataLenth);
//客户端接收
bool ClientRecevie( Socket* localSocket,u_8* recevieBuffer,u_16* bufferLenth);


#endif