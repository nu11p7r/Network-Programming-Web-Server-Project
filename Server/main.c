#include "Server.h"
#include "DatabaseManager.h"

// 전역 클라이언트 헤드
static struct SClientInfo *g_pClients = 0;

int main()
{
	WSADATA sWsaData;
	if(WSAStartup(MAKEWORD(2, 2), &sWsaData))
	{
		return 1;
	}

	SOCKET hServer = CreateSocket(0, "8080");
	char szCurrentDir[1024];
	_getcwd(szCurrentDir, sizeof(szCurrentDir));

	printf("\n");
	LogInfo("SYSTEM", "========================================");
	LogInfo("SYSTEM", "  STREAMING SERVER");
	LogInfo("SYSTEM", "========================================");
	LogInfo("SYSTEM", "실행 경로: %s", szCurrentDir);
	LogInfo("SYSTEM", "리소스: %s, %s, %s", DIR_WEB, DIR_MEDIA, DIR_THUMB);
	PrintServerIp(8080);

	DbTestConnection();

	while(1)
	{
		fd_set sReads = WaitOnClients(hServer, g_pClients);
		if(FD_ISSET(hServer, &sReads))
		{
			struct SClientInfo *pClient = GetClient(-1, &g_pClients);
			pClient->hSocket = accept(hServer, (struct sockaddr *)&(pClient->sAddress), &(pClient->iAddressLength));
			LogInfo("NET", ">>> 접속: %s", GetClientAddress(pClient));
		}

		struct SClientInfo *pClient = g_pClients;
		while(pClient)
		{
			struct SClientInfo *pNext = pClient->pNext;
			if(FD_ISSET(pClient->hSocket, &sReads))
			{
				if(MAX_REQUEST_SIZE == pClient->iReceived)
				{
					Send400(pClient, &g_pClients);
					pClient = pNext;
					continue;
				}

				int iRecv = recv(pClient->hSocket, pClient->szRequest + pClient->iReceived, MAX_REQUEST_SIZE - pClient->iReceived, 0);
				if(iRecv < 1)
				{
					DropClient(pClient, &g_pClients);
				}
				else
				{
					pClient->iReceived += iRecv;
					pClient->szRequest[pClient->iReceived] = 0;
					char *szQ = strstr(pClient->szRequest, "\r\n\r\n");
					if(szQ)
					{
						*szQ = 0;
						if(strncmp("GET /", pClient->szRequest, 5))
						{
							Send400(pClient, &g_pClients);
						}
						else
						{
							char *szPath = pClient->szRequest + 4;
							char *szEndPath = strstr(szPath, " ");
							if(!szEndPath)
							{
								Send400(pClient, &g_pClients);
							}
							else
							{
								*szEndPath = 0;
								if(strncmp(szPath, "/api/", 5) == 0)
								{
									HandleApi(pClient, szPath, &g_pClients);
								}
								else
								{
									ServeResource(pClient, szPath, &g_pClients);
								}
							}
						}
					}
				}
			}
			pClient = pNext;
		}
	}

	CLOSESOCKET(hServer);
	WSACleanup();
	return 0;
}