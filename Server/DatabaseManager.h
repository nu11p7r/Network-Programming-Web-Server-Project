#pragma once
#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

// 데이터베이스 연결 인포 현재 PC에서 사용중인 userName과 password로 설정 필요
// "host=localhost port=5432 dbname=ott_db user=user1234 password=pass1234"
#define DB_CONNINFO "host=localhost port=5432 dbname=ott_db user=postgres password=root"

void DbTestConnection();
int DbRegister(const char *szUsername, const char *szPassword);
int DbLogin(const char *szUsername, const char *szPassword);
void DbSaveProgress(const char *szUsername, const char *szVideo, int iTime);
int DbLoadProgress(const char *szUsername, const char *szVideo);

#endif