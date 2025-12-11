#include "Server.h"
#include "DatabaseManager.h"
#include <libpq-fe.h>

#pragma comment(lib, "libpq.lib")

void DbTestConnection()
{
	LogInfo("DB", "연결 시도... (%s)", DB_CONNINFO);
	PGconn *pConn = PQconnectdb(DB_CONNINFO);
	PQsetClientEncoding(pConn, "EUC_KR");

	if(PQstatus(pConn) != CONNECTION_OK)
	{
		LogInfo("ERROR", "DB 연결 실패! : %s", PQerrorMessage(pConn));
	}
	else
	{
		LogInfo("SUCCESS", "DB 연결 성공! (PostgreSQL)");
	}
	PQfinish(pConn);
}

int DbRegister(const char *szUsername, const char *szPassword)
{
	PGconn *pConn = PQconnectdb(DB_CONNINFO);
	PQsetClientEncoding(pConn, "EUC_KR");

	if(PQstatus(pConn) != CONNECTION_OK)
	{
		PQfinish(pConn);
		return -1;
	}

	char szQuery[512];
	sprintf(szQuery, "INSERT INTO users (username, password) VALUES ('%s', '%s')", szUsername, szPassword);
	PGresult *pRes = PQexec(pConn, szQuery);
	int iSuccess = (PQresultStatus(pRes) == PGRES_COMMAND_OK);

	if(iSuccess)
	{
		LogInfo("DB", "가입 완료: %s", szUsername);
	}
	else
	{
		LogInfo("DB FAIL", "가입 실패: %s", PQerrorMessage(pConn));
	}

	PQclear(pRes);
	PQfinish(pConn);
	return iSuccess;
}

int DbLogin(const char *szUsername, const char *szPassword)
{
	PGconn *pConn = PQconnectdb(DB_CONNINFO);
	PQsetClientEncoding(pConn, "EUC_KR");

	if(PQstatus(pConn) != CONNECTION_OK)
	{
		PQfinish(pConn);
		return 0;
	}

	char szQuery[512];
	sprintf(szQuery, "SELECT count(*) FROM users WHERE username='%s' AND password='%s'", szUsername, szPassword);
	PGresult *pRes = PQexec(pConn, szQuery);
	int iSuccess = 0;

	if(PQresultStatus(pRes) == PGRES_TUPLES_OK)
	{
		if(atoi(PQgetvalue(pRes, 0, 0)) > 0)
		{
			iSuccess = 1;
		}
	}

	if(iSuccess)
	{
		LogInfo("AUTH", "로그인 성공: %s", szUsername);
	}
	else
	{
		LogInfo("AUTH", "로그인 실패: %s", szUsername);
	}

	PQclear(pRes);
	PQfinish(pConn);
	return iSuccess;
}

void DbSaveProgress(const char *szUsername, const char *szVideo, int iTime)
{
	PGconn *pConn = PQconnectdb(DB_CONNINFO);
	PQsetClientEncoding(pConn, "EUC_KR");

	if(PQstatus(pConn) != CONNECTION_OK)
	{
		PQfinish(pConn);
		return;
	}

	char szQuery[1024];
	sprintf(szQuery, "INSERT INTO history (username, video_name, watch_time) VALUES ('%s', '%s', %d) ON CONFLICT (username, video_name) DO UPDATE SET watch_time = %d, last_updated = CURRENT_TIMESTAMP", szUsername, szVideo, iTime, iTime);
	PGresult *pRes = PQexec(pConn, szQuery);

	if(PQresultStatus(pRes) != PGRES_COMMAND_OK)
	{
		LogInfo("DB ERROR", "저장 실패: %s", PQerrorMessage(pConn));
	}
	PQclear(pRes);
	PQfinish(pConn);
}

int DbLoadProgress(const char *szUsername, const char *szVideo)
{
	PGconn *pConn = PQconnectdb(DB_CONNINFO);
	PQsetClientEncoding(pConn, "EUC_KR");

	if(PQstatus(pConn) != CONNECTION_OK)
	{
		PQfinish(pConn);
		return 0;
	}

	char szQuery[512];
	sprintf(szQuery, "SELECT watch_time FROM history WHERE username='%s' AND video_name='%s'", szUsername, szVideo);
	PGresult *pRes = PQexec(pConn, szQuery);
	int iTime = 0;

	if(PQresultStatus(pRes) == PGRES_TUPLES_OK && PQntuples(pRes) > 0)
	{
		iTime = atoi(PQgetvalue(pRes, 0, 0));
		LogInfo("DB LOAD", "기록 로드: %s -> %d초", szUsername, iTime);
	}
	PQclear(pRes);
	PQfinish(pConn);
	return iTime;
}