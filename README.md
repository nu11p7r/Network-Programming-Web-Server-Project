📺 High-Performance Multi-User Streaming Server
C언어와 Windows Socket(Winsock)을 활용한 고성능 멀티유저 동영상 스트리밍 서버입니다. I/O Multiplexing(Select 모델)을 적용하여 단일 스레드 환경에서도 다수의 클라이언트 요청을 끊김 없이 처리하며, PostgreSQL을 연동하여 시청 기록 저장 및 이어보기 기능을 제공합니다.

✨ 주요 기능 (Features)
🚀 고속 멀티유저 스트리밍: select() 기반의 비동기 소켓 처리로 다중 접속 지원

🎥 실시간 스트리밍 & 탐색: HTTP Range Request 지원 (비디오 탐색바 이동 가능)

🖼️ 썸네일 자동 생성: FFmpeg를 연동하여 영상 추가 시 썸네일 자동 추출

🔐 사용자 인증: PostgreSQL 연동 회원가입 및 로그인 (Salt 처리 없음, 학습용)

⏱️ 이어보기 (Resume): 시청 중단 시점 자동 저장 및 재접속 시 복원

🛠️ 개발 환경 (Tech Stack)
OS: Windows 10/11 (64bit)

Language: C (C99 Standard)

IDE: Visual Studio 2019 / 2022

Database: PostgreSQL 13+

Libraries:

Winsock2 (ws2_32.lib)

libpq (PostgreSQL C Interface)

Tools: FFmpeg (환경 변수 설정 필수)

⚙️ 설치 및 실행 방법 (Installation & Setup)
이 프로젝트를 실행하기 위해서는 데이터베이스와 컴파일 환경 설정이 필요합니다.

1. 필수 프로그램 설치
PostgreSQL: 설치 시 Command Line Tools와 Development Files를 반드시 포함해야 합니다.

FFmpeg: 설치 후 ffmpeg.exe가 있는 bin 폴더를 시스템 환경 변수 PATH에 등록해야 합니다. (CMD에서 ffmpeg -version 입력 시 정보가 나와야 함)

2. 데이터베이스 설정 (Database Setup)
PostgreSQL 쉘(psql) 또는 pgAdmin에서 아래 쿼리를 실행하여 DB와 테이블을 생성합니다.

SQL

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
3. 프로젝트 설정 (Visual Studio)
소스 코드 설정 (DatabaseManager.h)

본인의 PostgreSQL 계정 정보에 맞춰 연결 문자열을 수정하세요.

C

#define DB_CONNINFO "host=localhost port=5432 dbname=ott_db user=postgres password=본인비번"
라이브러리 경로 설정 (중요!)

프로젝트 속성 -> C/C++ -> 일반 -> 추가 포함 디렉터리에 PostgreSQL의 include 폴더 추가

예: C:\Program Files\PostgreSQL\14\include

프로젝트 속성 -> 링커 -> 일반 -> 추가 라이브러리 디렉터리에 PostgreSQL의 lib 폴더 추가

예: C:\Program Files\PostgreSQL\14\lib

(libpq.lib는 소스 코드 내 #pragma comment로 연결되어 있으므로 경로만 잡아주면 됩니다.)

4. 폴더 구조 생성
실행 파일(main.exe)이 생성되는 위치(보통 x64/Debug 또는 프로젝트 루트)에 아래 폴더들이 존재해야 합니다.

root/

www/ : 웹 프론트엔드 파일 (index.html, style.css 등)

media/ : 스트리밍할 .mp4 파일들 위치

thumbs/ : 빈 폴더 (썸네일이 이곳에 생성됨)

🚀 실행 (Usage)
Visual Studio에서 프로젝트를 빌드(Ctrl + Shift + B)합니다.

libpq.dll 등 필요한 DLL이 없다면 PostgreSQL bin 폴더에서 복사해 옵니다.

서버를 실행합니다.

Bash

[SYSTEM] STREAMING SERVER
[SYSTEM] 실행 경로: ...
[SUCCESS] DB 연결 성공!
브라우저를 열고 http://localhost:8080 으로 접속합니다.

📂 디렉토리 구조 (Directory Structure)
📦 StreamServer
 ┣ 📂 media          # (생성 필요) MP4 영상 원본
 ┣ 📂 thumbs         # (생성 필요) 자동 생성된 썸네일
 ┣ 📂 www            # (생성 필요) 웹 클라이언트 리소스
 ┣ 📜 main.c         # 엔트리 포인트 및 소켓 루프
 ┣ 📜 Server.c       # HTTP 요청 처리 및 스트리밍 로직
 ┣ 📜 DatabaseManager.c # PostgreSQL 연동 로직
 ┗ 📜 README.md      # 프로젝트 설명서