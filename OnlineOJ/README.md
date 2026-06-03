# OnlineOJ — 小型在线判题系统

基于 C++ 的轻量级在线判题系统，支持 C++ 代码提交、Docker 沙箱编译运行、自动评测。

## 快速启动

### 前置依赖

- **Docker** 与 **Docker Compose**（用于容器化部署）
- 或本地安装：`g++`, `cmake`, `MySQL 8.0`, `Redis 7`, `libmysqlclient-dev`, `libhiredis-dev`, `libzip-dev`, `libssl-dev`

### 一键部署（Docker Compose）

```bash
cp .env.example .env    # 编辑密码等配置
docker compose up -d     # 启动 MySQL + Redis + Backend + Judge
```

访问 `http://localhost:8080`

### 本地开发运行

```bash
# 1. 启动 MySQL 和 Redis
docker run -d --name oj-mysql -p 3307:3306 \
  -e MYSQL_ROOT_PASSWORD=root_change_me \
  -e MYSQL_USER=oj -e MYSQL_PASSWORD=change_me \
  -e MYSQL_DATABASE=onlineoj \
  -v $(pwd)/init.sql:/docker-entrypoint-initdb.d/init.sql \
  mysql:8.0

docker run -d --name oj-redis -p 6379:6379 redis:7-alpine

# 2. 构建
cd backend/build && cmake .. && make -j$(nproc)
cd ../../judge/build && cmake .. && make -j$(nproc)

# 3. 启动（使用 localhost 地址）
export MYSQL_HOST=127.0.0.1 MYSQL_PORT=3307 MYSQL_USER=oj MYSQL_PASSWORD=change_me
export MYSQL_DATABASE=onlineoj REDIS_HOST=127.0.0.1 REDIS_PORT=6379
export BACKEND_PORT=8080 SESSION_SECRET=secret123 JUDGE_API_TOKEN=static_token_for_judge
export JUDGE_BACKEND_URL=http://127.0.0.1:8080

./backend/build/backend &
./judge/build/judge &
```

## 默认管理员

启动后访问 `http://localhost:8080/register.html` 注册账号，然后在数据库中手动提升为管理员：

```sql
UPDATE users SET role='admin' WHERE username='你的用户名';
```

## 系统架构

```
浏览器 (HTML/CSS/JS) → 后端 (cpp-httplib, C++) → Redis 队列 → 评测机 (C++)
                           ↓                                    ↓
                        MySQL 8.0                          Docker 沙箱
                                                      (g++ 编译 + 运行)
```

## 功能

- 用户注册/登录/登出 (Session + Cookie)
- 题目列表、搜索、难度/标签筛选
- C++ 代码提交与自动评测
- 评测结果：AC / WA / TLE / MLE / CE / SE
- WA 时显示失败测试点的输入/期望输出/实际输出
- 排行榜（按通过数 + 通过率排序，支持难度/标签筛选）
- 个人统计
- 管理后台：题目 CRUD、测试数据 ZIP 上传、标签管理
- 频率限制、pending 冲突检测（同题同用户同时只允许一个待评测提交）
- CE 不占用频率限制配额

## 目录结构

```
OnlineOJ/
├── backend/          # 后端 (C++ + cpp-httplib)
│   ├── src/          # 源码
│   └── build/        # 构建产物
├── judge/            # 评测机 (C++)
│   ├── src/          # 源码
│   └── build/        # 构建产物
├── frontend/         # 前端 (HTML/CSS/JS + Pico.css)
│   ├── js/           # api.js, auth.js, components.js, utils.js
│   ├── admin/        # 管理后台页面
│   └── ...
├── docker-compose.yml
├── init.sql          # 数据库初始化 (DDL + 种子数据)
├── .env.example      # 环境变量模板
└── SPEC.md           # 产品规格书
```

## 环境变量

| 变量 | 说明 | 默认值 |
|------|------|--------|
| `MYSQL_HOST` | MySQL 主机 | localhost |
| `MYSQL_PORT` | MySQL 端口 | 3306 |
| `MYSQL_USER` | MySQL 用户 | root |
| `MYSQL_PASSWORD` | MySQL 密码 | — |
| `MYSQL_DATABASE` | 数据库名 | onlineoj |
| `REDIS_HOST` | Redis 主机 | localhost |
| `REDIS_PORT` | Redis 端口 | 6379 |
| `BACKEND_PORT` | 后端监听端口 | 8080 |
| `SESSION_SECRET` | Session 密钥 | — |
| `JUDGE_API_TOKEN` | 评测机认证 Token | — |
| `RATE_LIMIT_PER_MIN` | 每用户每分钟最大提交数 | 10 |
| `JUDGE_BACKEND_URL` | 后端地址（评测机用） | http://localhost:8080 |

## 测试数据格式

上传 ZIP 文件，包含成对的 `.in` / `.out` 文件：

```
testdata.zip
├── 1.in
├── 1.out
├── 2.in
├── 2.out
└── ...
```

## 技术栈

- **后端**: C++17, cpp-httplib, MySQL Connector C, hiredis, libzip, OpenSSL
- **评测机**: C++17, Docker CLI, hiredis, cpp-httplib (HTTP 客户端)
- **前端**: HTML5, CSS3, Vanilla JS, Pico.css (CDN)
- **数据库**: MySQL 8.0
- **消息队列**: Redis 7 (BRPOP/LPUSH)
- **沙箱**: Docker (gcc:13 镜像, --network=none, --pids-limit=50)
