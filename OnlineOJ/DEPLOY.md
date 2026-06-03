# OnlineOJ 部署文档

## 系统要求

| 组件 | 最低版本 |
|------|----------|
| 操作系统 | Ubuntu 20.04+ / CentOS 8+ |
| Docker | 24.0+ (含 docker compose v2) |
| MySQL | 8.0 |
| Redis | 7.0 |
| 编译工具 | g++ 11+, cmake 3.16+, make |

**可选（生产环境推荐）：**
- Docker 镜像 `gcc:13`（评测机沙箱编译，无镜像时自动回退到本地 g++）
- `python3`（支持 Python 语言提交）

---

## 快速部署（5 分钟）

### 1. 克隆项目

```bash
git clone <repo-url> OnlineOJ
cd OnlineOJ
```

### 2. 配置环境变量

```bash
cp .env.example .env
vim .env   # 修改密码等敏感配置
```

关键配置项：

```bash
MYSQL_HOST=mysql          # Docker Compose 用 mysql；本地运行用 127.0.0.1
MYSQL_PORT=3306
MYSQL_USER=oj
MYSQL_PASSWORD=change_me  # 务必修改
MYSQL_DATABASE=onlineoj
MYSQL_ROOT_PASSWORD=root_change_me
REDIS_HOST=redis
REDIS_PORT=6379
BACKEND_PORT=8080
SESSION_SECRET=请生成随机字符串
JUDGE_API_TOKEN=请生成随机Token
RATE_LIMIT_PER_MIN=10
JUDGE_BACKEND_URL=http://backend:8080
```

### 3. 启动全部服务

**方式 A：Docker Compose（推荐）**

```bash
# 首次需构建镜像（3-5分钟）
docker compose build

# 启动
docker compose up -d

# 查看状态
docker compose ps

# 查看日志
docker compose logs -f
```

访问 `http://<服务器IP>:8080`

**方式 B：本地运行（开发调试）**

```bash
# 1. 启动 MySQL 和 Redis
docker run -d --name oj-mysql --restart unless-stopped \
  -e MYSQL_ROOT_PASSWORD=root_change_me \
  -e MYSQL_USER=oj -e MYSQL_PASSWORD=change_me \
  -e MYSQL_DATABASE=onlineoj \
  -p 3307:3306 \
  -v $(pwd)/init.sql:/docker-entrypoint-initdb.d/init.sql \
  mysql:8.0

docker run -d --name oj-redis --restart unless-stopped \
  -p 6379:6379 redis:7-alpine

# 2. 编译
cd backend/build && cmake .. && make -j$(nproc)
cd ../../judge/build && cmake .. && make -j$(nproc)
cd ../..

# 3. 启动
export MYSQL_HOST=127.0.0.1 MYSQL_PORT=3307 MYSQL_USER=oj MYSQL_PASSWORD=change_me
export MYSQL_DATABASE=onlineoj REDIS_HOST=127.0.0.1 REDIS_PORT=6379
export BACKEND_PORT=8080 SESSION_SECRET=your_secret JUDGE_API_TOKEN=static_token_for_judge
export JUDGE_BACKEND_URL=http://127.0.0.1:8080

nohup ./backend/build/backend > backend.log 2>&1 &
nohup ./judge/build/judge > judge.log 2>&1 &
```

**方式 C：一键脚本**

```bash
bash start.sh start    # 启动所有服务
bash start.sh status   # 查看状态
bash start.sh stop     # 停止
```

---

## 初始化管理员

1. 访问 `http://localhost:8080/register.html` 注册账号
2. 进入 MySQL 提升为管理员：

```sql
UPDATE users SET role='admin' WHERE username='你的用户名';
```

---

## 使用流程

### 管理员
1. 登录后进入「管理后台」
2. 新建题目 → 填写标题/描述/难度/语言(C++/Python) → 上传测试数据 ZIP
3. 测试数据格式：`1.in` + `1.out`、`2.in` + `2.out` ...

### 用户
1. 注册 → 登录
2. 浏览题目列表，筛选难度/标签
3. 进入题目 → 编写代码 → 提交
4. 查看评测结果（AC/WA/TLE/CE）
5. 查看排行榜和个人统计

---

## 项目结构

```
OnlineOJ/
├── backend/                # 后端 (C++17 + cpp-httplib)
│   ├── src/                # 源码
│   │   ├── main.cpp        # 入口
│   │   ├── server.cpp      # 路由注册 + 静态文件服务
│   │   ├── db.cpp          # MySQL 连接池
│   │   ├── auth.cpp        # 用户认证 (SHA-256 + Salt)
│   │   ├── handler_*.cpp   # API 处理器
│   │   ├── rate_limiter.cpp # 频率限制
│   │   └── middleware.cpp  # 权限中间件
│   └── build/              # CMake 构建目录
├── judge/                  # 评测机 (C++17)
│   ├── src/
│   │   ├── main.cpp        # 入口 + 评测主循环
│   │   ├── compiler.cpp    # 编译 (Docker 沙箱 / 原生)
│   │   ├── runner.cpp      # 运行测试点
│   │   ├── comparator.cpp  # 输出比对
│   │   ├── reporter.cpp    # 结果上报 (3次重试)
│   │   └── queue.cpp       # Redis 队列消费
│   └── build/
├── frontend/               # 前端 (原生 HTML/CSS/JS + Monaco Editor)
│   ├── index.html          # 题目列表
│   ├── problem.html        # 题目详情 + 代码编辑器
│   ├── login.html          # 登录
│   ├── register.html       # 注册
│   ├── leaderboard.html    # 排行榜
│   ├── statistics.html     # 个人统计
│   ├── submissions.html    # 提交历史
│   ├── submission.html     # 提交详情
│   ├── admin/              # 管理后台
│   │   ├── problems.html   # 题目管理
│   │   └── tags.html       # 标签管理
│   ├── js/                 # JavaScript
│   │   ├── api.js          # fetch 封装
│   │   ├── auth.js         # 认证状态管理
│   │   ├── components.js   # 侧边栏导航 / Toast / 分页
│   │   └── utils.js        # HTML转义 / 状态文本 / 日期格式化
│   └── css/
│       └── custom.css      # 完整 UI 样式 (浅色高级主题)
├── docker-compose.yml      # 容器编排
├── init.sql                # 数据库建表 DDL
├── .env.example            # 环境变量模板
├── start.sh                # 一键启动脚本
└── restart.sh              # 重新启动脚本
```

---

## API 接口

### 认证
| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/register` | 注册 |
| POST | `/api/login` | 登录 (返回 Cookie) |
| POST | `/api/logout` | 登出 |
| GET | `/api/session` | 当前用户信息 |

### 题目
| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/problems` | 题目列表 (?page=&difficulty=&tag=) |
| GET | `/api/problems/:id` | 题目详情 |

### 提交
| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/submit` | 提交代码 {problem_id, code, language} |
| GET | `/api/submissions` | 提交历史 (?page=) |
| GET | `/api/submissions/:id` | 提交详情 |

### 管理员
| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/admin/problems` | 创建题目 |
| PUT | `/api/admin/problems/:id` | 更新题目 |
| DELETE | `/api/admin/problems/:id` | 删除题目 |
| POST | `/api/admin/problems/:id/testdata` | 上传测试数据 ZIP |
| GET/POST/DELETE | `/api/admin/tags` | 标签管理 |

### 排行榜
| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/leaderboard` | 排行榜 (?difficulty=&tag=) |
| GET | `/api/statistics` | 个人统计 |

---

## 评测结果

| 状态码 | 含义 |
|--------|------|
| `pending` | 等待评测 |
| `compiling` | 编译中 |
| `running` | 运行中 |
| `accepted` | 通过 (AC) |
| `wrong_answer` | 答案错误 (WA) |
| `time_limit_exceeded` | 超时 (TLE) |
| `memory_limit_exceeded` | 超内存 (MLE) |
| `compilation_error` | 编译错误 (CE) |
| `system_error` | 系统错误 (SE) |

---

## 测试数据格式

ZIP 文件，成对的 `.in` / `.out`：

```
aplusb_testdata.zip
├── 1.in
├── 1.out
├── 2.in
├── 2.out
└── 3.in
└── 3.out
```

- 文件名中的数字对应测试点序号
- `.out` 也可用 `.ans` 后缀
- 输出比对忽略行末空格，归一化 `\r\n` → `\n`

---

## 安全特性

- 密码 SHA-256 + Salt 哈希存储
- Session Cookie (HttpOnly, SameSite)
- 评测机 API Token 认证
- SQL 参数化查询 (db_escape)
- Docker 沙箱：`--network=none` `--pids-limit=50` `--cap-drop=ALL`
- 代码长度限制 64KB
- 频率限制 10次/分钟/用户
- 非管理员越权拦截 403

---

## 故障排查

| 问题 | 检查 |
|------|------|
| 页面打不开 | `docker compose ps` / `ss -tlnp \| grep 8080` |
| 提交后一直是 pending | 评测机是否启动？`ps aux \| grep judge` |
| 系统错误 0/0 | 题目是否上传了测试数据？ |
| 编译错误 | 代码语法是否正确？点击「错误信息」查看详情 |
| Redis 连接失败 | `docker exec oj-redis redis-cli ping` |
| MySQL 连接失败 | `docker exec oj-mysql mysqladmin ping` |

---

## 技术栈

| 层 | 技术 |
|----|------|
| 后端 | C++17 · cpp-httplib · MySQL Connector C · hiredis · libzip · OpenSSL |
| 评测机 | C++17 · Docker CLI · hiredis |
| 前端 | HTML5 · CSS3 · Vanilla JS · Monaco Editor (CDN) |
| 数据库 | MySQL 8.0 |
| 消息队列 | Redis 7 (BRPOP/LPUSH) |
| 容器化 | Docker · Docker Compose |
