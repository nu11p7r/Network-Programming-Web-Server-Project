#include "Server.h"
#include "DatabaseManager.h"

// 로그 함수 구현
void LogInfo(const char *szTag, const char *szFormat, ...)
{
	time_t tNow = time(NULL);
	struct tm *pTime = localtime(&tNow);
	printf("[%02d:%02d:%02d] [%-7s] ", pTime->tm_hour, pTime->tm_min, pTime->tm_sec, szTag);

	va_list pArgs;
	va_start(pArgs, szFormat);
	vprintf(szFormat, pArgs);
	va_end(pArgs);
	printf("\n");
}

void PrintServerIp(int iPort)
{
	char szHostname[256];
	if(gethostname(szHostname, sizeof(szHostname)) == 0)
	{
		struct hostent *pHost = gethostbyname(szHostname);
		if(pHost)
		{
			struct in_addr **ppAddrList = (struct in_addr **)pHost->h_addr_list;
			for(int i = 0; ppAddrList[i] != NULL; i++)
			{
				LogInfo("SERVER", "내 IP 주소: %s", inet_ntoa(*ppAddrList[i]));
			}
		}
	}
	LogInfo("SERVER", "포트 번호 : %d", iPort);
}

// 헬퍼 함수
void UrlDecode(char *szDst, const char *szSrc)
{
	char cA, cB;
	while(*szSrc)
	{
		if((*szSrc == '%') && ((cA = szSrc[1]) && (cB = szSrc[2])) && (isxdigit(cA) && isxdigit(cB)))
		{
			if(cA >= 'a')
			{
				cA -= 'a' - 'A';
			}

			if(cA >= 'A')
			{
				cA -= ('A' - 10);
			}
			else
			{
				cA -= '0';
			}

			if(cB >= 'a')
			{
				cB -= 'a' - 'A';
			}

			if(cB >= 'A')
			{
				cB -= ('A' - 10);
			}
			else
			{
				cB -= '0';
			}

			*szDst++ = 16 * cA + cB;
			szSrc += 3;
		}
		else if(*szSrc == '+')
		{
			*szDst++ = ' ';
			szSrc++;
		}
		else
		{
			*szDst++ = *szSrc++;
		}
	}
	*szDst = '\0';
}

const char *GetContentType(const char *szPath)
{
	const char *szLastDot = strrchr(szPath, '.');
	if(szLastDot)
	{
		if(strcmp(szLastDot, ".css") == 0)
		{
			return "text/css";
		}
		if(strcmp(szLastDot, ".html") == 0)
		{
			return "text/html";
		}
		if(strcmp(szLastDot, ".jpg") == 0)
		{
			return "image/jpeg";
		}
		if(strcmp(szLastDot, ".mp4") == 0)
		{
			return "video/mp4";
		}
	}
	return "application/octet-stream";
}

// 썸네일 생성 (media -> thumbs)
void GenerateThumbnailIfNotExist(const char *szVideoName)
{
	char szThumbPath[512];
	sprintf(szThumbPath, "%s%s.jpg", DIR_THUMB, szVideoName);

	if(_access(szThumbPath, 0) == 0)
	{
		return;
	}

	LogInfo("SYSTEM", "썸네일 생성: %s", szVideoName);
	char szCmd[1024];
	sprintf(szCmd, "ffmpeg -y -i \"%s%s\" -ss 00:00:01 -vframes 1 \"%s\" > nul 2>&1", DIR_MEDIA, szVideoName, szThumbPath);
	system(szCmd);
}

// 소켓 함수들
SOCKET CreateSocket(const char *szHost, const char *szPort)
{
	struct addrinfo sHints;
	memset(&sHints, 0, sizeof(sHints));
	sHints.ai_family = AF_INET;
	sHints.ai_socktype = SOCK_STREAM;
	sHints.ai_flags = AI_PASSIVE;

	struct addrinfo *pBindAddress;
	getaddrinfo(szHost, szPort, &sHints, &pBindAddress);

	SOCKET hSocketListen;
	hSocketListen = socket(pBindAddress->ai_family, pBindAddress->ai_socktype, pBindAddress->ai_protocol);

	if(!ISVALIDSOCKET(hSocketListen))
	{
		exit(1);
	}

	int iYes = 1;
	setsockopt(hSocketListen, SOL_SOCKET, SO_REUSEADDR, (char *)&iYes, sizeof(iYes));

	if(bind(hSocketListen, pBindAddress->ai_addr, pBindAddress->ai_addrlen))
	{
		exit(1);
	}

	freeaddrinfo(pBindAddress);

	if(listen(hSocketListen, 10) < 0)
	{
		exit(1);
	}

	return hSocketListen;
}

struct SClientInfo *GetClient(SOCKET hSocket, struct SClientInfo **ppHead)
{
	struct SClientInfo *pCi = *ppHead;
	while(pCi)
	{
		if(pCi->hSocket == hSocket)
		{
			break;
		}
		pCi = pCi->pNext;
	}

	if(pCi)
	{
		return pCi;
	}

	struct SClientInfo *pN = (struct SClientInfo *)calloc(1, sizeof(struct SClientInfo));
	if(!pN)
	{
		exit(1);
	}

	pN->iAddressLength = sizeof(pN->sAddress);
	pN->pNext = *ppHead;
	*ppHead = pN;

	return pN;
}

void DropClient(struct SClientInfo *pClient, struct SClientInfo **ppHead)
{
	CLOSESOCKET(pClient->hSocket);
	struct SClientInfo **ppP = ppHead;
	while(*ppP)
	{
		if(*ppP == pClient)
		{
			*ppP = pClient->pNext;
			free(pClient);
			return;
		}
		ppP = &(*ppP)->pNext;
	}
	exit(1);
}

const char *GetClientAddress(struct SClientInfo *pCi)
{
	static char szAddressBuffer[100];
	getnameinfo((struct sockaddr *)&pCi->sAddress, pCi->iAddressLength, szAddressBuffer, sizeof(szAddressBuffer), 0, 0, NI_NUMERICHOST);
	return szAddressBuffer;
}

fd_set WaitOnClients(SOCKET hServer, struct SClientInfo *pHead)
{
	fd_set sReads;
	FD_ZERO(&sReads);
	FD_SET(hServer, &sReads);

	SOCKET hMaxSocket = hServer;
	struct SClientInfo *pCi = pHead;
	while(pCi)
	{
		FD_SET(pCi->hSocket, &sReads);
		if(pCi->hSocket > hMaxSocket)
		{
			hMaxSocket = pCi->hSocket;
		}
		pCi = pCi->pNext;
	}

	struct timeval sTimeout;
	sTimeout.tv_sec = 0;
	sTimeout.tv_usec = 100000;

	if(select(hMaxSocket + 1, &sReads, 0, 0, &sTimeout) < 0)
	{
		exit(1);
	}
	return sReads;
}

void Send400(struct SClientInfo *pClient, struct SClientInfo **ppHead)
{
	const char *szMsg = "HTTP/1.1 400 Bad Request\r\nConnection: close\r\nContent-Length: 11\r\n\r\nBad Request";
	send(pClient->hSocket, szMsg, (int)strlen(szMsg), 0);
	DropClient(pClient, ppHead);
}

void Send404(struct SClientInfo *pClient, struct SClientInfo **ppHead)
{
	const char *szMsg = "HTTP/1.1 404 Not Found\r\nConnection: close\r\nContent-Length: 9\r\n\r\nNot Found";
	send(pClient->hSocket, szMsg, (int)strlen(szMsg), 0);
	DropClient(pClient, ppHead);
}

// 목록 생성
void GetVideoListJson(char *szBuffer)
{
	WIN32_FIND_DATAA sFindData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	char szSearchPath[512];
	sprintf(szSearchPath, "%s*.mp4", DIR_MEDIA);

	strcpy(szBuffer, "[");
	hFind = FindFirstFileA(szSearchPath, &sFindData);
	int iCount = 0;
	if(hFind != INVALID_HANDLE_VALUE)
	{
		int iFirst = 1;
		do
		{
			if(!(sFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				GenerateThumbnailIfNotExist(sFindData.cFileName);
				if(!iFirst)
				{
					strcat(szBuffer, ",");
				}

				char szItem[512];
				sprintf(szItem, "{\"name\":\"%s\"}", sFindData.cFileName);
				strcat(szBuffer, szItem);
				iFirst = 0;
				iCount++;
			}
		} while(FindNextFileA(hFind, &sFindData) != 0);
		FindClose(hFind);
	}
	strcat(szBuffer, "]");
	LogInfo("FILE", "목록 스캔: %d개 (from %s)", iCount, DIR_MEDIA);
}

void HandleApi(struct SClientInfo *pClient, const char *szPath, struct SClientInfo **ppHead)
{
	char *szResponseBody = (char *)malloc(MAX_JSON_SIZE);
	if(!szResponseBody)
	{
		return;
	}

	char szId[50] = { 0 }, szPw[50] = { 0 };

	LogInfo("API", "요청: %s (IP: %s)", szPath, GetClientAddress(pClient));

	if(strstr(szPath, "/api/signup"))
	{
		char *pId = strstr(szPath, "id=");
		char *pPw = strstr(szPath, "pw=");
		if(pId && pPw)
		{
			sscanf(pId, "id=%[^&]", szId);
			sscanf(pPw, "pw=%s", szPw);
		}

		if(strlen(szId) > 0 && strlen(szPw) > 0)
		{
			if(DbRegister(szId, szPw))
			{
				sprintf(szResponseBody, "{\"status\":\"success\",\"msg\":\"Registered\"}");
			}
			else
			{
				sprintf(szResponseBody, "{\"status\":\"fail\",\"msg\":\"ID Exists\"}");
			}
		}
		else
		{
			sprintf(szResponseBody, "{\"status\":\"fail\",\"msg\":\"Invalid Input\"}");
		}
	}
	else if(strstr(szPath, "/api/login"))
	{
		char *pId = strstr(szPath, "id=");
		char *pPw = strstr(szPath, "pw=");
		if(pId && pPw)
		{
			sscanf(pId, "id=%[^&]", szId);
			sscanf(pPw, "pw=%s", szPw);
		}

		if(strlen(szId) > 0 && strlen(szPw) > 0 && DbLogin(szId, szPw))
		{
			sprintf(szResponseBody, "{\"status\":\"success\",\"msg\":\"Login OK\"}");
		}
		else
		{
			sprintf(szResponseBody, "{\"status\":\"fail\",\"msg\":\"Check DB\"}");
		}
	}
	else if(strstr(szPath, "/api/save"))
	{
		char szU[50] = { 0 }, szV[256] = { 0 }, szT[20] = { 0 };
		char *pU = strstr(szPath, "u=");
		char *pV = strstr(szPath, "v=");
		char *pT = strstr(szPath, "t=");

		if(pU && pV && pT)
		{
			sscanf(pU, "u=%[^&]", szU);
			sscanf(pV, "v=%[^&]", szV);
			sscanf(pT, "t=%s", szT);
			DbSaveProgress(szU, szV, atoi(szT));
		}
		sprintf(szResponseBody, "{\"status\":\"saved\"}");
	}
	else if(strstr(szPath, "/api/load"))
	{
		char szU[50] = { 0 }, szV[256] = { 0 };
		char *pU = strstr(szPath, "u=");
		char *pV = strstr(szPath, "v=");
		int iSavedTime = 0;

		if(pU && pV)
		{
			sscanf(pU, "u=%[^&]", szU);
			sscanf(pV, "v=%s", szV);
			iSavedTime = DbLoadProgress(szU, szV);
		}
		sprintf(szResponseBody, "{\"status\":\"success\",\"time\":%d}", iSavedTime);
	}
	else if(strstr(szPath, "/api/list"))
	{
		GetVideoListJson(szResponseBody);
	}
	else
	{
		free(szResponseBody);
		Send404(pClient, ppHead);
		return;
	}

	char szHeader[512];
	sprintf(szHeader, "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: %d\r\nConnection: close\r\n\r\n", (int)strlen(szResponseBody));
	send(pClient->hSocket, szHeader, (int)strlen(szHeader), 0);
	send(pClient->hSocket, szResponseBody, (int)strlen(szResponseBody), 0);
	free(szResponseBody);
	DropClient(pClient, ppHead);
}

void ServeVideoStream(struct SClientInfo *pClient, const char *szRawPath, struct SClientInfo **ppHead)
{
	char szFullPath[1024];
	char szDecodedName[256];
	UrlDecode(szDecodedName, szRawPath);

	sprintf(szFullPath, "%s%s", DIR_MEDIA, szDecodedName);

	FILE *pFile = fopen(szFullPath, "rb");
	if(!pFile)
	{
		Send404(pClient, ppHead);
		return;
	}

	fseek(pFile, 0L, SEEK_END);
	long lFileSize = ftell(pFile);
	long lStart = 0;
	long lEnd = lFileSize - 1;
	char *szRangeHeader = strstr(pClient->szRequest, "Range: bytes=");

	if(szRangeHeader)
	{
		sscanf(szRangeHeader, "Range: bytes=%ld-", &lStart);
		long lChunkSize = 1024 * 1024 * 2;
		lEnd = lStart + lChunkSize;
		if(lEnd >= lFileSize)
		{
			lEnd = lFileSize - 1;
		}
	}

	long lContentLength = lEnd - lStart + 1;
	fseek(pFile, lStart, SEEK_SET);

	LogInfo("STREAM", "전송: %s (%ld~%ld)", szDecodedName, lStart, lEnd);
	char szHeader[512];

	if(szRangeHeader)
	{
		sprintf(szHeader, "HTTP/1.1 206 Partial Content\r\nContent-Type: video/mp4\r\nContent-Range: bytes %ld-%ld/%ld\r\nContent-Length: %ld\r\nAccept-Ranges: bytes\r\nConnection: close\r\n\r\n", lStart, lEnd, lFileSize, lContentLength);
	}
	else
	{
		sprintf(szHeader, "HTTP/1.1 200 OK\r\nContent-Type: video/mp4\r\nContent-Length: %ld\r\nAccept-Ranges: bytes\r\nConnection: close\r\n\r\n", lFileSize);
	}
	send(pClient->hSocket, szHeader, (int)strlen(szHeader), 0);

	char szBuffer[8192];
	long lTotalSent = 0;
	while(lTotalSent < lContentLength)
	{
		long lToRead = sizeof(szBuffer);
		if(lTotalSent + lToRead > lContentLength)
		{
			lToRead = lContentLength - lTotalSent;
		}

		int iRead = fread(szBuffer, 1, lToRead, pFile);
		if(iRead <= 0)
		{
			break;
		}

		send(pClient->hSocket, szBuffer, iRead, 0);
		lTotalSent += iRead;
	}
	fclose(pFile);
	DropClient(pClient, ppHead);
}

void ServeResource(struct SClientInfo *pClient, const char *szPath, struct SClientInfo **ppHead)
{
	if(strcmp(szPath, "/") == 0)
	{
		szPath = "index.html";
	}

	if(strstr(szPath, ".."))
	{
		Send404(pClient, ppHead);
		return;
	}

	char szRawPath[256];
	// ★ 요청하신 수정 부분: if-else 모두 중괄호 처리
	if(szPath[0] == '/')
	{
		strcpy(szRawPath, szPath + 1);
	}
	else
	{
		strcpy(szRawPath, szPath);
	}

	char szDecodedPath[256];
	UrlDecode(szDecodedPath, szRawPath);

	LogInfo("REQ", "파일: %s", szDecodedPath);

	if(strstr(szDecodedPath, ".mp4") && !strstr(szDecodedPath, ".jpg"))
	{
		ServeVideoStream(pClient, szDecodedPath, ppHead);
		return;
	}

	char szFullPath[512];
	if(strstr(szDecodedPath, ".jpg"))
	{
		sprintf(szFullPath, "%s%s", DIR_THUMB, strrchr(szDecodedPath, '/') ? strrchr(szDecodedPath, '/') + 1 : szDecodedPath);
	}
	else
	{
		sprintf(szFullPath, "%s%s", DIR_WEB, szDecodedPath);
	}

	FILE *pFile = fopen(szFullPath, "rb");
	if(!pFile)
	{
		if(!strstr(szFullPath, ".jpg"))
		{
			LogInfo("ERROR", "파일없음: %s", szFullPath);
		}
		Send404(pClient, ppHead);
		return;
	}

	fseek(pFile, 0L, SEEK_END);
	size_t lCl = ftell(pFile);
	rewind(pFile);

	const char *szCt = GetContentType(szDecodedPath);
	char szBuffer[4096];
	sprintf(szBuffer, "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: %lu\r\nContent-Type: %s\r\n\r\n", (unsigned long)lCl, szCt);
	send(pClient->hSocket, szBuffer, (int)strlen(szBuffer), 0);

	int iRead;
	while((iRead = fread(szBuffer, 1, sizeof(szBuffer), pFile)) > 0)
	{
		send(pClient->hSocket, szBuffer, iRead, 0);
	}
	fclose(pFile);
	DropClient(pClient, ppHead);
}