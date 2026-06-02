# OnlineOJ — 完整产品规格书

> 版本: v1.0 (MVP Phase)
> 最后更新: 2026-06-02

---

## 1. 项目概述

### 1.1 一句话定位
一个小型公有在线判题系统（Online Judge），面向约 20 人用户群体，核心流程为：用户提交 C++ 代码 → 后端接收 → 评测机在 Docker 沙箱中编译执行 → 返回判题结果。

### 1.2 核心价值
- 低成本、易部署、无需复杂运维
- C++ 标准判题体验（AC/WA/TLE/MLE/CE）
- 完整的前后端分离 + 独立评测机架构

### 1.3 业务目标
| 指标 | 目标值 |
|------|--------|
| 并发用户数 | ≤ 20 人 |
| 用户提交流量 | < 10 QPS |
| 评测延迟（P95） | < 5s |
| 支持题目数 | < 200 道 |
| 单题测试数据量 | ≤ 50 组，每组 ≤ 10MB |

### 1.4 非目标（MVP 不做）
- 多语言支持（除 C++ 外）
- Special Judge / 交互题
- 竞赛/比赛模式
- 讨论区 / 题解区
- Kubernetes / 自动伸缩

---

## 2. 架构总览

### 2.1 系统架构图（文字版）

```
┌─────────────┐      HTTP (REST)       ┌──────────────┐     Redis Queue     ┌──────────────┐
│   浏览器     │ ◄──────────────────────►│  后端 (C++)  │ ◄──────────────────► │  评测机 (C++) │
│ (HTML+CSS+JS)│    Session/Cookie      │  cpp-httplib │   SubmitJob/Result   │  + Docker CLI│
└─────────────┘                         └──────┬───────┘                      └──────┬───────┘
                                               │                                     │
                                               ▼                                     ▼
                                        ┌──────────┐                          ┌──────────────┐
                                        │  MySQL    │                          │ Docker 容器   │
                                        │          │                          │ --rm --cpus=1 │
                                        │- users   │                          │ --mem=256m    │
                                        │- problems│                          │ 编译/运行      │
                                        │- testdata│                          └──────────────┘
                                        │- submissions│
                                        │- tags/difficulty│
                                        └──────────┘
```

### 2.2 组件职责

| 组件 | 语言 | 职责 |
|------|------|------|
| **前端** | HTML + CSS + JS (+ Pico.css CDN) | 所有 UI 页面、前端路由、API 调用 |
| **后端** | C++ (cpp-httplib) | REST API、认证授权、CRUD、频率限制、队列投递 |
| **评测机** | C++ | 从 Redis 队列取任务，Docker 沙箱编译运行，结果上报 |
| **Redis** | — | 消息队列（SubmitJob / Result），解耦后端与评测机 |
| **MySQL** | — | 持久化存储：用户、题目、测试数据、提交记录 |
| **Docker** | — | 评测机用来启动隔离容器编译和运行用户代码 |

### 2.3 部署拓扑（Docker Compose）

```
┌─────────────────────────────────────────────────┐
│                  docker-compose                  │
│                                                  │
│  ┌──────────┐ ┌──────────────┐ ┌──────────────┐ │
│  │ backend  │ │    judge     │ │ judge-2 ...  │ │
│  │  :8080   │ │(Docker sock) │ │  (可扩展)     │ │
│  └──────────┘ └──────────────┘ └──────────────┘ │
│  ┌──────────┐ ┌──────────┐                      │
│  │  redis   │ │  mysql   │                      │
│  │  :6379   │ │  :3306   │                      │
│  └──────────┘ └──────────┘                      │
└─────────────────────────────────────────────────┘
```

---

## 3. 技术选型

| 维度 | 选型 | 理由 |
|------|------|------|
| 后端框架 | **cpp-httplib** (header-only) | 零依赖、高性能、符合用户要求 |
| 数据库 | **MySQL 8.0** | 关系型，稳定成熟，小团队够用 |
| 消息队列 | **Redis 7** (BRPOP/LPUSH 模式) | 轻量，不需要额外部署 RabbitMQ/Kafka |
| 前端 CSS | **Pico.css** (CDN) | 无构建步骤，语义化 class，暗色主题内置 |
| 沙箱 | **Docker** (--rm --cpus=1 --memory=256m --network=none --pids-limit=50 --cap-drop=ALL) | 每个提交用临时容器编译+运行，挂载代码目录，结束后自动销毁 |
| 部署编排 | **Docker Compose** | 一键起停全部服务 |
| 认证 | **Session + Cookie** | 传统方案，cpp-httplib 易于实现 |
| 前端轮询 | **fetch + setInterval** | 简单可靠，20 人规模无需 WebSocket |
| ZIP 解压 | **libzip** (C 库) | 后端解析测试数据 ZIP，无需额外解压工具 |
| 静态资源 | **cpp-httplib 内建 serve** | 后端直接 serve `frontend/` 目录，无需 Nginx |

---

## 4. 功能清单

### 4.1 用户系统
- [ ] 注册（用户名 + 密码）
- [ ] 登录（Session + Cookie）
- [ ] 登出
- [ ] 管理员角色（admin flag），管理员可管理题目
- [ ] 密码 bcrypt 哈希存储

### 4.2 题目管理（管理员）
- [ ] 创建/编辑/删除题目
- [ ] 题目字段：标题、描述（纯文本，MVP）、难度（简单/中等/困难）、标签（预设列表多选）
- [ ] 上传测试数据 ZIP（约定格式：`1.in` / `1.out` / `2.in` / `2.out` ...）
- [ ] 标签管理（预设标签列表，管理员可增删）
- [ ] 全局 C++ 代码模板（系统级默认模板，所有题共用：`#include <iostream> using namespace std; int main() { ... }`），题目可单独覆盖该模板

### 4.3 提交评测（用户）
- [ ] 用户选择题目，在 textarea 中编写/粘贴 C++ 代码
- [ ] 提交 → 后端创建 Submission 记录 → 推入 Redis 队列
- [ ] 评测结果：AC / WA / TLE / MLE / CE / SE（System Error）
- [ ] 每个用户每分钟最多 N 次提交（N 可配，默认 10）
- [ ] 同一道题同一用户同一时刻只允许一个 Pending 提交：新提交若已有 Pending，直接拒绝并提示"已有待评测提交，请等待完成"
- [ ] 测试遇到第一个 WA/TLE 即停止（快速失败）
- [ ] 编译错误（CE）不占用频率限制配额：提交时后端先扣除配额，CE 结果上报后自动补回该次配额
- [ ] 前端轮询提交结果（每 500ms），最多轮询 30s 后超时提示

### 4.4 结果展示
- [ ] 显示最终判定：AC / WA / TLE / MLE / CE / SE
- [ ] WA 时显示：第几个测试点失败 + 该测试点的输入内容 + 期望输出 + 实际输出
- [ ] AC 时显示：通过测试点数 / 总测试点数，总耗时，峰值内存
- [ ] CE 时显示：g++ 编译器完整错误输出
- [ ] TLE/MLE 时显示：触发超时/超内存的那个测试点信息

### 4.5 排行榜与统计
- [ ] 排行榜：按通过题数降序，同分按通过率降序，支持按难度/标签筛选
- [ ] 个人统计页：已通过题目数、总提交数、通过率、按难度的通过分布

### 4.6 代码编辑器（二期优先迭代）
- [ ] 集成 Monaco Editor（CDN）实现语法高亮
- [ ] 全局 C++ 代码模板预填

---

## 5. 数据库 Schema

### `users`
| 列 | 类型 | 说明 |
|----|------|------|
| id | INT AUTO_INCREMENT PK | |
| username | VARCHAR(50) UNIQUE | |
| password_hash | VARCHAR(255) | bcrypt |
| role | ENUM('user','admin') | 默认 'user' |
| created_at | DATETIME | |

### `problems`
| 列 | 类型 | 说明 |
|----|------|------|
| id | INT AUTO_INCREMENT PK | |
| title | VARCHAR(200) | |
| description | TEXT | 题目描述（纯文本） |
| difficulty | ENUM('easy','medium','hard') | |
| code_template | TEXT | C++ 模板代码（可为 NULL，NULL 时使用系统全局默认模板） |
| time_limit_ms | INT | 默认 2000 |
| memory_limit_kb | INT | 默认 262144 (256MB) |
| author_id | INT FK → users.id | |
| created_at | DATETIME | |
| updated_at | DATETIME | |

### `problem_tags`
| 列 | 类型 | 说明 |
|----|------|------|
| problem_id | INT FK | |
| tag_id | INT FK | |

### `tags`
| 列 | 类型 | 说明 |
|----|------|------|
| id | INT AUTO_INCREMENT PK | |
| name | VARCHAR(50) UNIQUE | 如 “动态规划” |

### `test_cases`
| 列 | 类型 | 说明 |
|----|------|------|
| id | INT AUTO_INCREMENT PK | |
| problem_id | INT FK | |
| case_number | INT | 测试点序号 1..N |
| input_data | MEDIUMTEXT | 输入内容 |
| expected_output | MEDIUMTEXT | 期望输出 |
| created_at | DATETIME | |

### `submissions`
| 列 | 类型 | 说明 |
|----|------|------|
| id | INT AUTO_INCREMENT PK | |
| user_id | INT FK | |
| problem_id | INT FK | |
| code | MEDIUMTEXT | 用户提交的源代码 |
| status | ENUM('pending','compiling','running','accepted','wrong_answer','time_limit_exceeded','memory_limit_exceeded','compilation_error','system_error') | |
| failed_case | INT | 失败测试点序号（WA/TLE时） |
| time_used_ms | INT | 总耗时（ms） |
| memory_used_kb | INT | 峰值内存（KB） |
| passed_cases | INT | 通过的测试点数 |
| total_cases | INT | 总测试点数 |
| compile_error | TEXT | 编译错误信息 |
| detail_json | JSON | 额外详情 |
| submitted_at | DATETIME | |
| judged_at | DATETIME | |

---

## 6. API 设计

### 6.1 认证
| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| POST | `/api/register` | 注册 | 否 |
| POST | `/api/login` | 登录 | 否 |
| POST | `/api/logout` | 登出 | 是 |
| GET | `/api/session` | 获取当前用户信息 | 是 |

### 6.2 题目（用户侧）
| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| GET | `/api/problems` | 题目列表（分页、难度/标签筛选） | 是 |
| GET | `/api/problems/:id` | 题目详情（含代码模板） | 是 |

### 6.3 题目管理（管理员）
| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| POST | `/api/admin/problems` | 创建题目 | Admin |
| PUT | `/api/admin/problems/:id` | 更新题目 | Admin |
| DELETE | `/api/admin/problems/:id` | 删除题目 | Admin |
| POST | `/api/admin/problems/:id/testdata` | 上传测试数据 ZIP | Admin |
| GET | `/api/problems/:id/testdata` | 获取测试数据（仅评测机调用） | Token |

### 6.4 提交与评测
| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| POST | `/api/submit` | 提交代码 | 是 |
| GET | `/api/submissions/:id` | 查询提交结果（前端轮询） | 是 |
| GET | `/api/submissions` | 用户提交历史列表 | 是 |
| PUT | `/api/submissions/:id/result` | 评测机上报结果 | Token |

### 6.5 排行榜与统计
| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| GET | `/api/leaderboard` | 排行榜（支持 ?difficulty=&tag=） | 是 |
| GET | `/api/statistics` | 当前用户统计 | 是 |

### 6.6 通用响应格式

**成功响应**：
```json
{ "data": { ... } }
```

**错误响应**：
```json
{ "error": "错误描述信息", "code": "ERROR_CODE" }
```

| HTTP 状态码 | code | 说明 |
|:---|:---|:---|
| 400 | `BAD_REQUEST` | 请求参数错误 |
| 401 | `UNAUTHORIZED` | 未登录 |
| 403 | `FORBIDDEN` | 权限不足 |
| 404 | `NOT_FOUND` | 资源不存在 |
| 409 | `CONFLICT` | 冲突（如同题已有 Pending 提交） |
| 429 | `RATE_LIMITED` | 频率限制 |
| 500 | `INTERNAL_ERROR` | 服务器内部错误 |

---

## 7. 评测流程详解

评测机通过 Docker socket 挂载（`-v /var/run/docker.sock:/var/run/docker.sock`）复用宿主机 Docker daemon，不构建中间镜像。编译和运行都直接在 `gcc:13` 基础容器中通过卷挂载完成。

```
1. 评测机 BRPOP redis:submit_queue → {submission_id, code, problem_id, time_limit, memory_limit}
2. 评测机 GET /api/problems/{problem_id}/testdata → [ {case_number, input, expected_output}, ... ]
3. 将用户代码写入临时工作目录（如 /tmp/judge/{submission_id}/code.cpp）
4. 编译阶段：
   docker run --rm --cpus=1 --memory=256m \
     -v /tmp/judge/{submission_id}:/app \
     gcc:13 g++ -O2 /app/code.cpp -o /app/solution 2>&1
   若编译失败 → 上报 CE → 结束
5. 运行阶段（逐个测试点）：
   for each test_case:
     echo input_data | docker run --rm --cpus=1 --memory={memory_limit} --network=none \
       --pids-limit=50 -i \
       -v /tmp/judge/{submission_id}:/app \
       gcc:13 timeout {time_limit}s /app/solution
     根据容器退出码和 OOM 状态判断：
       - 退出码 0 → stdout 对比（忽略行末空格，归一化 \r\n→\n）
       - 退出码 124（timeout 超时）→ TLE
       - 退出码 137 且 Docker OOMKilled=true → MLE
       - 其他非 0 退出码 → RE / SE
     stdout 比对：
       - 失败 → 上报 WA（含 failed_case=当前序号，input、expected、actual）→ 结束
       - 成功 → 继续下一组
6. 全部通过 → 上报 AC（含 passed_cases=total, time_used, memory_used）
```

### 输出比对算法（伪代码）
```
function compare(actual, expected):
    actual_lines = actual.strip_trailing_spaces_on_each_line()
    expected_lines = expected.strip_trailing_spaces_on_each_line()
    return normalize_newlines(actual_lines) == normalize_newlines(expected_lines)
``` 

---

## 8. 前端页面清单

### 8.1 页面列表
| 页面 | 路径 | 功能 |
|------|------|------|
| 登录 | `/login.html` | 用户名+密码登录 |
| 注册 | `/register.html` | 用户名+密码注册 |
| 题目列表 | `/index.html` (或 `/`) | 题目列表、搜索、难度/标签筛选、分页 |
| 题目详情 | `/problem.html?id=N` | 题目描述、代码输入区、提交按钮、提交结果面板 |
| 提交记录 | `/submissions.html` | 当前用户所有提交历史列表，可跳转到详情 |
| 提交详情 | `/submission.html?id=N` | 单次提交详情（代码、结果、测试点信息） |
| 排行榜 | `/leaderboard.html` | 排行榜，可筛选难度/标签 |
| 个人统计 | `/statistics.html` | 个人题目通过统计 |
| 管理后台 | `/admin/problems.html` | 题目 CRUD 界面、测试数据上传 |
| 管理后台-标签 | `/admin/tags.html` | 标签管理 |

### 8.2 前端技术约定
- 所有 JS 使用原生 `fetch` API
- 认证通过 Cookie 自动携带（无需前端手动管理 token）
- 页面共用头部导航栏（根据登录状态/角色动态渲染）
- 使用 Pico.css CDN (`<link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/@picocss/pico@2/css/pico.min.css">`)

---

## 9. 非功能需求

### 9.1 安全
- [ ] 密码 bcrypt 加盐哈希存储
- [ ] Session ID 使用 `crypto/random` 生成，HttpOnly Cookie
- [ ] 评测机 API 使用静态 Token 认证（环境变量注入）
- [ ] SQL 注入防护：全部使用参数化查询（预处理语句）
- [ ] Docker 容器：`--network=none` 禁止网络访问
- [ ] Docker 容器：`--pids-limit=50` 防止 fork 炸弹
- [ ] 用户代码大小限制：最大 64KB
- [ ] XSS 防护：题目描述渲染前做 HTML 转义

### 9.2 可靠性
- [ ] 评测机崩溃：Redis BRPOP 超时 + 后端兜底超时（30s），标记为 SE
- [ ] 数据库连接池保活
- [ ] 评测机上报结果失败重试 3 次

### 9.3 可扩展性（预留但不实现）
- [ ] `judger_interface.hpp`：抽象基类 `class IJudger { virtual void judge(...) = 0; }`
- [ ] 判题策略预留：通过工厂模式支持多种判题器
- [ ] 语言预留：`IRunner` 抽象，未来可添加 `PythonRunner`、`JavaRunner`
- [ ] 评测机可水平扩展（docker-compose 中加 `judge` 实例即可）

### 9.4 性能
- [ ] 数据库查询加合理索引
- [ ] 测试数据一次拉取全量（非逐条查），减少网络往返
- [ ] Docker 基础镜像（gcc:13）提前拉取至宿主机，避免评测时下载延迟

---

## 10. 项目目录结构

```
OnlineOJ/
├── SPEC.md                    # 本文件
├── docker-compose.yml         # 一键部署编排
├── .env.example               # 环境变量模板
├── init.sql                   # MySQL 初始化 DDL + 种子数据
├── backend/
│   ├── CMakeLists.txt
│   ├── src/
│   │   ├── main.cpp           # 入口
│   │   ├── server.hpp/cpp     # HTTP 服务器启动与路由注册
│   │   ├── db.hpp/cpp         # MySQL 连接池、查询封装
│   │   ├── auth.hpp/cpp       # 注册/登录/登出/Session 管理
│   │   ├── middleware.hpp/cpp  # Auth/Admin 中间件
│   │   ├── handler_problem.hpp/cpp
│   │   ├── handler_submit.hpp/cpp
│   │   ├── handler_leaderboard.hpp/cpp
│   │   ├── handler_admin.hpp/cpp
│   │   ├── rate_limiter.hpp/cpp
│   │   └── model.hpp          # 数据结构定义
│   └── ...
├── judge/
│   ├── CMakeLists.txt
│   ├── Dockerfile             # 评测机镜像（含 Docker CLI + gcc 基础环境）
│   ├── src/
│   │   ├── main.cpp
│   │   ├── queue.hpp/cpp      # Redis 队列消费
│   │   ├── compiler.hpp/cpp   # Docker 编译封装
│   │   ├── runner.hpp/cpp     # Docker 运行封装
│   │   ├── comparator.hpp/cpp # 输出比对
│   │   ├── reporter.hpp/cpp   # 结果上报
│   │   └── interface.hpp      # IJudger/IRunner 抽象接口（预留）
│   └── ...
└── frontend/
    ├── index.html             # 题目列表
    ├── login.html
    ├── register.html
    ├── problem.html           # 题目详情 + 代码提交
    ├── submission.html
    ├── submissions.html
    ├── leaderboard.html
    ├── statistics.html
    ├── admin/
    │   ├── problems.html
    │   └── tags.html
    ├── css/
    │   └── custom.css         # 少量自定义覆盖 Pico.css
    └── js/
        ├── api.js             # fetch 封装 + 统一错误处理
        ├── auth.js            # 登录状态管理
        ├── components.js      # 公用 UI 组件（navbar 等）
        └── utils.js           # 工具函数
```

---

## 11. 验收标准

### 11.1 核心流程验收
- [ ] 用户可注册、登录、登出
- [ ] 管理员可创建题目、上传测试数据 ZIP
- [ ] 用户提交 C++ 代码，评测机返回正确结果（AC/WA/TLE/MLE/CE）
- [ ] WA 时能看到具体失败测试点的输入/期望输出/实际输出
- [ ] 输出比对忽略行末空格，归一化换行符
- [ ] 遇到第一个 WA 即停止后续测试
- [ ] CE 不占用频率限制
- [ ] 同一题同一用户同时只能有一个 Pending 提交（重复提交被拒绝并提示等待）

### 11.2 安全验收
- [ ] 密码 bcrypt 哈希存储（数据库中不可见到明文密码）
- [ ] 非管理员无法访问 `/api/admin/*` 接口
- [ ] Docker 容器内 curl/wget 无法访问外网（--network=none）
- [ ] 超长/恶意代码不会导致评测机崩溃

### 11.3 功能验收
- [ ] 排行榜按通过数 + 通过率排序
- [ ] 排行榜可筛选难度/标签
- [ ] 提交历史可正常分页浏览
- [ ] 个人统计页面数据正确

### 11.4 运维验收
- [ ] `docker-compose up -d` 一键启动全部服务
- [ ] 评测机宕机后新提交在 30s 内标记为 SE
- [ ] MySQL 数据持久化（重启不丢数据）

---

## 12. TODO 开发清单

### Phase 1 — 基础设施 (预计 3-5 天)
- [ ] `docker-compose.yml` 编写（MySQL + Redis + backend + judge）
- [ ] `init.sql` DDL 全部建表语句
- [ ] `backend/` CMakeLists.txt + cpp-httplib 引入
- [ ] `judge/` CMakeLists.txt + Dockerfile 基础镜像
- [ ] `.env.example` 环境变量模板

### Phase 2 — 后端核心 (预计 5-7 天)
- [ ] 数据库连接池模块
- [ ] 用户注册/登录/登出 + Session 管理
- [ ] 权限中间件（Auth + Admin）
- [ ] 题目 CRUD API（管理员）
- [ ] 测试数据上传与解析（ZIP）
- [ ] 提交 API + 频率限制
- [ ] Redis 队列投递与轮询
- [ ] 排行榜 API
- [ ] 统计 API

### Phase 3 — 评测机 (预计 3-5 天)
- [ ] Redis 队列消费
- [ ] Docker 编译封装
- [ ] Docker 运行封装（单测试点）
- [ ] 输出比对器
- [ ] 结果上报
- [ ] 错误处理与重试

### Phase 4 — 前端 (预计 5-7 天)
- [ ] 公共模块（api.js, auth.js, components.js, custom.css）
- [ ] 登录/注册页面
- [ ] 题目列表页面（含筛选/分页）
- [ ] 题目详情 + 代码提交页面（含结果面板）
- [ ] 提交记录 / 提交详情页面
- [ ] 排行榜页面
- [ ] 个人统计页面
- [ ] 管理后台（题目管理 + 标签管理）

### Phase 5 — 联调与验收 (预计 2-3 天)
- [ ] 全流程测试（注册→创建题目→提交→判题→排行榜）
- [ ] 安全验收（越权/注入/沙箱逃逸测试）
- [ ] 异常场景验收（评测机宕机、Docker 不可用、超大数据）
- [ ] README 编写

### Phase 6 — 二期扩展（按需）
- [ ] Monaco Editor 集成
- [ ] 多语言支持（Java / Python）
- [ ] Special Judge
- [ ] 竞赛模式

---

## 13. 风险与权衡

| 风险 | 影响 | 缓解措施 |
|------|------|----------|
| C++ 用户代码安全 | 沙箱逃逸 | `--network=none` `--pids-limit=50` `--cap-drop=ALL` |
| Redis 单点故障 | 无法提交 | MVP 接受；后期可加 Redis Sentinel |
| Session 内存存储（无持久化） | 重启后所有用户需重新登录 | MVP 接受；后期改 Redis Session Store |
| cpp-httplib 无内置 ORM | 需手写 SQL | 使用 MySQL Connector C++ 或 libmysqlclient |

---

## 14. 环境变量

```bash
# MySQL
MYSQL_HOST=mysql
MYSQL_PORT=3306
MYSQL_USER=oj
MYSQL_PASSWORD=change_me
MYSQL_DATABASE=onlineoj

# Redis
REDIS_HOST=redis
REDIS_PORT=6379

# Backend
BACKEND_PORT=8080
SESSION_SECRET=your_random_secret_string
JUDGE_API_TOKEN=static_token_for_judge
RATE_LIMIT_PER_MIN=10

# Judge
JUDGE_BACKEND_URL=http://backend:8080
JUDGE_API_TOKEN=static_token_for_judge
JUDGE_MAX_TASKS=3
```

---

> **签名确认**
> 以上规格完整描述了 MVP 阶段所有功能和约束，开发过程中如有变更需同步更新本文件并标注变更理由。
