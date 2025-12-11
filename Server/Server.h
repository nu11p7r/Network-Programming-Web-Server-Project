#pragma once
#ifndef SERVER_H
#define SERVER_H

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <direct.h>
#include <io.h>
#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#pragma comment(lib, "ws2_32.lib")

#define MAX_REQUEST_SIZE 2047
#define MAX_JSON_SIZE 1024 * 50

#define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
#define CLOSESOCKET(s) closesocket(s)

#define DIR_WEB   "./www/"
#define DIR_MEDIA "./media/"
#define DIR_THUMB "./thumbs/"

struct SClientInfo
{
	socklen_t iAddressLength;
	struct sockaddr_storage sAddress;
	SOCKET hSocket;
	char szRequest[MAX_REQUEST_SIZE + 1];
	int iReceived;
	struct SClientInfo *pNext;
};

// 로그 함수
void LogInfo(const char *szTag, const char *szFormat, ...);
void PrintServerIp(int iPort);

// 소켓 및 클라이언트 관리
SOCKET CreateSocket(const char *szHost, const char *szPort);
struct SClientInfo *GetClient(SOCKET hSocket, struct SClientInfo **ppHead);
void DropClient(struct SClientInfo *pClient, struct SClientInfo **ppHead);
const char *GetClientAddress(struct SClientInfo *pCi);
fd_set WaitOnClients(SOCKET hServer, struct SClientInfo *pHead);

// 요청 처리
void Send400(struct SClientInfo *pClient, struct SClientInfo **ppHead);
void Send404(struct SClientInfo *pClient, struct SClientInfo **ppHead);
void HandleApi(struct SClientInfo *pClient, const char *szPath, struct SClientInfo **ppHead);
void ServeResource(struct SClientInfo *pClient, const char *szPath, struct SClientInfo **ppHead);

#endif