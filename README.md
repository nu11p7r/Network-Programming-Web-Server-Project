# 📺 High-Performance Multi-User Streaming Server

**C언어와 Windows Socket(Winsock)을 활용한 고성능 멀티유저 동영상 스트리밍 서버**입니다.

I/O Multiplexing(Select 모델)을 적용하여 단일 스레드 환경에서도 다수의 클라이언트 요청을 끊김 없이 처리하며, PostgreSQL을 연동하여 시청 기록 저장 및 이어보기 기능을 제공합니다.

## ✨ 주요 기능 (Features)

* **🚀 고속 멀티유저 스트리밍:** `select()` 기반의 비동기 소켓 처리로 다중 접속 지원
* **🎥 실시간 스트리밍 & 탐색:** HTTP Range Request 지원 (비디오 탐색바 이동 / 이어보기 지원)
* **🖼️ 썸네일 자동 생성:** FFmpeg를 연동하여 영상 추가 시 썸네일 이미지 자동 추출
* **🔐 사용자 인증:** PostgreSQL 연동 회원가입 및 로그인
* **⏱️ 이어보기 (Resume):** 시청 중단 시점 자동 저장 및 재접속 시 해당 위치부터 복원

---

## 🛠️ 개발 환경 (Tech Stack)

* **OS:** Windows 10/11 (64bit)
* **Language:** C (C99 Standard)
* **IDE:** Visual Studio 2019 / 2022
* **Database:** **PostgreSQL 18**
* **Libraries:**
  * `Winsock2` (`ws2_32.lib`)
  * `libpq` (PostgreSQL C Interface)
* **Tools:** FFmpeg (환경 변수 등록 필수)

---

## ⚙️ 설치 및 실행 방법 (Installation & Setup)

이 프로젝트를 실행하기 위해서는 데이터베이스와 컴파일 환경 설정이 필요합니다.

### 1. 필수 프로그램 설치
1. **PostgreSQL 18**: 설치 시 `Command Line Tools`와 `Development Files`를 반드시 포함해야 합니다.
2. **FFmpeg**: 설치 후 `ffmpeg.exe`가 있는 `bin` 폴더를 시스템 환경 변수 `PATH`에 등록해야 합니다.

### 2. 프로젝트 설정 (Visual Studio)

**1) 소스 코드 설정 (필수!)**
소스 코드 폴더 내의 **`DatabaseManager.h`** 파일을 열고, 아래 부분을 **본인의 PostgreSQL 설치 시 설정한 계정 정보(비밀번호 등)** 로 수정해야 합니다.

```c
#pragma once
#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

// [설정 필요] 현재 PC에서 사용 중인 userName과 password로 설정해 주세요.
// 예시: "host=localhost port=5432 dbname=ott_db user=postgres password=내비밀번호"
#define DB_CONNINFO "host=localhost port=5432 dbname=ott_db user=postgres password=root"

void DbTestConnection();
int DbRegister(const char *szUsername, const char *szPassword);
int DbLogin(const char *szUsername, const char *szPassword);
void DbSaveProgress(const char *szUsername, const char *szVideo, int iTime);
int DbLoadProgress(const char *szUsername, const char *szVideo);

#endif
```
### 3. 데이터베이스 설정 (Database Setup)
PostgreSQL 쉘(psql) 또는 pgAdmin에서 아래 쿼리를 실행하여 DB와 테이블을 생성합니다.

```sql
-- 데이터베이스 생성
CREATE DATABASE ott_db;

-- (해당 DB로 접속 후) 테이블 생성
CREATE TABLE users (
    username VARCHAR(50) PRIMARY KEY,
    password VARCHAR(50) NOT NULL
);

CREATE TABLE history (
    username VARCHAR(50),
    video_name VARCHAR(255),
    watch_time INTEGER,
    last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (username, video_name),
    FOREIGN KEY (username) REFERENCES users(username)
);
```