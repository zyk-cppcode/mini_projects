# OnlineOJ 代码实现步骤

> 基于 SPEC.md 第 12 节细化，按文件逐一实现。

---

## Phase 1 — 基础设施

### 1.1 `.env.example` — 环境变量模板
- 写入 SPEC.md 第 14 节全部变量，含注释说明

### 1.2 `init.sql` — 数据库初始化
- 创建数据库 `onlineoj`
- 建表 `users`、`problems`、`tags`、`problem_tags`、`test_cases`、`submissions`
- 插入种子数据：admin 用户、预设标签（如"动态规划""贪心"等）

### 1.3 `docker-compose.yml` — 服务编排
- mysql:8.0 容器（端口 3306，挂载 init.sql，persist data）
- redis:7 容器（端口 6379）
- backend 容器（端口 8080，依赖 mysql + redis）
- judge 容器（挂载 docker.sock，依赖 mysql + redis + backend）

### 1.4 `backend/CMakeLists.txt` — 后端构建
- cmake_minimum_required 3.16
- project(backend)
- 引入 cpp-httplib（FetchContent）
- 引入 nlohmann/json（FetchContent）
- 链接库：mysqlclient、hiredis、libzip、ssl、crypto
- 编译 `backend/src/` 下所有 `.cpp` 为可执行文件

### 1.5 `judge/CMakeLists.txt` — 评测机构建
- cmake_minimum_required 3.16
- project(judge)
- 链接库：hiredis、ssl、crypto、curl
- 编译 `judge/src/` 下所有 `.cpp` 为可执行文件

### 1.6 `judge/Dockerfile` — 评测机镜像
- FROM ubuntu:22.04
- 安装 docker CLI、g++、timeout 等工具
- 复制编译好的 judge 可执行文件
- CMD 启动评测机进程

---

## Phase 2 — 后端核心

### 2.1 `backend/src/model.hpp` — 数据结构定义
- 定义 struct：User、Problem、Tag、TestCase、Submission
- 定义 enum：Difficulty、SubmissionStatus
- 定义 JSON 序列化/反序列化函数（nlohmann/json）
- 定义 ErrorCode 枚举（对应 HTTP 错误响应）

### 2.2 `backend/src/db.hpp` / `db.cpp` — 数据库模块
- MySQL 连接池（固定大小，如 10 个连接）
- `init_pool()` / `get_conn()` / `release_conn()`
- 封装 `query()`、`execute()` 方法（参数化查询防注入）
- 连接保活（ping 机制）

### 2.3 `backend/src/auth.hpp` / `auth.cpp` — 认证模块
- bcrypt 密码哈希与验证
- Session 管理（内存 map：session_id → user_id）
- `register(username, password)` → 写入 users 表
- `login(username, password)` → 验证密码 → 生成 session → Set-Cookie
- `logout(session_id)` → 删除 session
- `get_user(session_id)` → 返回 User 或 null

### 2.4 `backend/src/middleware.hpp` / `middleware.cpp` — 中间件
- `auth_middleware`: 从 Cookie 提取 session_id → 解析用户 → 附加到 request context；未登录返回 401
- `admin_middleware`: 检查 user.role == 'admin'，非管理员返回 403

### 2.5 `backend/src/rate_limiter.hpp` / `rate_limiter.cpp` — 频率限制
- 每个用户维护一个滑动窗口计数器
- `try_consume(user_id)`: 检查当前窗口内提交次数 < RATE_LIMIT_PER_MIN，是则计数+1
- `refund(user_id)`: CE 时补回配额

### 2.6 `backend/src/handler_problem.hpp` / `handler_problem.cpp` — 题目处理器
- `GET /api/problems` — 分页、难度/标签筛选，关联查询
- `GET /api/problems/:id` — 返回题目详情 + 代码模板（若为 NULL 返回全局默认模板）

### 2.7 `backend/src/handler_submit.hpp` / `handler_submit.cpp` — 提交处理器
- `POST /api/submit` — 校验代码长度 ≤ 64KB → 检查同题 Pending → 频率限制检查 → 创建 Submission → LPUSH Redis submit_queue → 返回 submission_id
- `GET /api/submissions/:id` — 返回单次提交结果（前端轮询）
- `GET /api/submissions` — 用户提交历史列表（分页）
- `PUT /api/submissions/:id/result` — 评测机上报结果（Token 认证），更新 submission 记录，CE 时补回配额

### 2.8 `backend/src/handler_admin.hpp` / `handler_admin.cpp` — 管理处理器
- `POST /api/admin/problems` — 创建题目
- `PUT /api/admin/problems/:id` — 更新题目
- `DELETE /api/admin/problems/:id` — 删除题目
- `POST /api/admin/problems/:id/testdata` — 接收 ZIP 文件 → libzip 解压 → 逐对解析 `1.in/1.out` → 写入 test_cases 表
- `GET /api/problems/:id/testdata` — 返回全部测试数据（评测机调用，Token 认证）

### 2.9 `backend/src/handler_leaderboard.hpp` / `handler_leaderboard.cpp` — 排行榜处理器
- `GET /api/leaderboard` — 按通过题数降序、通过率降序，支持难度/标签筛选
- `GET /api/statistics` — 当前用户统计（通过题数、总提交、通过率、按难度分布）

### 2.10 `backend/src/server.hpp` / `server.cpp` — HTTP 服务器
- 初始化 httplib::Server，设置端口 BACKEND_PORT
- 注册所有路由 + 对应中间件
- 注册静态文件服务：`frontend/` 目录
- 注册中间件：Session Cookie 解析 + CORS 头

### 2.11 `backend/src/main.cpp` — 入口
- 加载环境变量
- 初始化数据库连接池
- 初始化 Redis 连接
- 初始化 Session 管理
- 启动 HTTP 服务器（server.run()）

---

## Phase 3 — 评测机

### 3.1 `judge/src/main.cpp` — 入口
- 加载环境变量（JUDGE_API_TOKEN, JUDGE_BACKEND_URL, REDIS_HOST, REDIS_PORT）
- 初始化 Redis 连接
- 定义并发数（JUDGE_MAX_TASKS=3）
- 循环：BRPOP submit_queue → 多线程/协程处理

### 3.2 `judge/src/queue.hpp` / `queue.cpp` — Redis 队列消费
- `connect_redis(host, port)`
- `fetch_task()`: BRPOP submit_queue → 解析 JSON → 返回 SubmitJob {submission_id, code, problem_id, time_limit, memory_limit}

### 3.3 `judge/src/compiler.hpp` / `compiler.cpp` — Docker 编译封装
- `compile(submission_id, code)`:
  - 创建临时目录 `/tmp/judge/{submission_id}/`
  - 写入 code.cpp
  - `docker run --rm --cpus=1 --memory=256m --network=none -v /tmp/judge/{submission_id}:/app gcc:13 g++ -O2 /app/code.cpp -o /app/solution 2>/tmp/compile_error.txt`
  - 返回 {success: bool, error_msg: string}

### 3.4 `judge/src/runner.hpp` / `runner.cpp` — Docker 运行封装
- `run_single_test(submission_id, input_data, time_limit, memory_limit)`:
  - `echo input_data | docker run --rm --cpus=1 --memory={memory_limit}k --network=none --pids-limit=50 --cap-drop=ALL -i -v /tmp/judge/{submission_id}:/app gcc:13 timeout {time_limit}s /app/solution`
  - 获取 stdout、退出码、OOM 状态
  - 返回 {exit_code, stdout, oom_killed}

### 3.5 `judge/src/comparator.hpp` / `comparator.cpp` — 输出比对
- `compare(actual, expected)`:
  - 每行去除行末空格
  - 统一换行符（\r\n → \n）
  - 比较是否完全相等

### 3.6 `judge/src/reporter.hpp` / `reporter.cpp` — 结果上报
- `report_result(submission_id, result)`:
  - HTTP PUT → `{backend_url}/api/submissions/{submission_id}/result`
  - Header: `Authorization: Bearer {judge_api_token}`
  - 请求体：status、passed_cases、total_cases、time_used_ms、memory_used_kb、failed_case、compile_error 等
  - 失败重试 3 次（指数退避）

### 3.7 `judge/src/interface.hpp` — 抽象接口（预留，不实现）
- `class IJudger { virtual void judge(...) = 0; }`
- `class IRunner { virtual RunResult run(...) = 0; }`

---

## Phase 4 — 前端

### 4.1 `frontend/js/api.js` — API 封装
- `api.get(path)`, `api.post(path, body)`, `api.put(path, body)`
- 统一处理 HTTP 错误（显示 toast/alert）
- 统一解析 `{ data: ... }` / `{ error: ..., code: ... }`

### 4.2 `frontend/js/auth.js` — 登录状态管理
- `checkAuth()`: GET /api/session → 获取当前用户
- `login(username, password)`: POST /api/login
- `register(username, password)`: POST /api/register
- `logout()`: POST /api/logout
- 全局变量 `currentUser` + `isAdmin`

### 4.3 `frontend/js/components.js` — 公共 UI 组件
- `renderNavbar()`: 根据登录状态/角色渲染导航栏
- `renderPagination(page, total)`: 分页组件
- `showToast(msg, type)`: 提示消息

### 4.4 `frontend/js/utils.js` — 工具函数
- `htmlEscape(str)`: HTML 转义防 XSS
- `formatDate(datetime)`: 日期格式化
- `statusLabel(status)`: 提交状态中文映射
- `statusColor(status)`: 状态对应颜色

### 4.5 `frontend/css/custom.css` — 自定义样式
- 状态颜色类（.ac-green, .wa-red 等）
- 代码展示区样式（等宽字体、滚动、行号）
- 少量覆盖 Pico.css

### 4.6 各 HTML 页面（按复杂度递增）
1. **`login.html`** — 登录表单 → api.login() → 跳转首页
2. **`register.html`** — 注册表单 → api.register() → 跳转登录
3. **`index.html`** — 题目列表：分页 + 难度下拉 + 标签多选 → 点击跳转 /problem.html?id=N
4. **`problem.html`** — 题目描述 + textarea 代码区（预填模板）+ 提交按钮 → 轮询结果面板（AC/WA/TLE/MLE/CE 详情）
5. **`submissions.html`** — 当前用户提交历史列表（分页表）
6. **`submission.html`** — 单次提交详情（代码高亮 + 测试点结果 + 错误信息）
7. **`leaderboard.html`** — 排行榜表格 + 难度/标签筛选
8. **`statistics.html`** — 个人统计数据展示
9. **`admin/problems.html`** — 题目 CRUD 表格 + 新建/编辑弹窗 + ZIP 上传
10. **`admin/tags.html`** — 标签列表增删

---

## Phase 5 — 联调与验收

1. `docker-compose up -d` 一键启动
2. 注册用户 → 登录
3. 管理员创建题目 + 上传测试数据 ZIP
4. 用户提交 C++ 代码
5. 验证 AC / WA / TLE / MLE / CE 各状态
6. 验证频率限制、同题 Pending 拒绝、CE 补回配额
7. 验证排行榜排序、筛选
8. 验证安全：越权访问 403、XSS 转义、容器无网络
9. 验证异常：停止评测机后提交超时 30s 变 SE

---

## 实现顺序建议

```
Phase 1 (基础设施)
  → Phase 2 后端 model.hpp → db → auth → middleware → handler_problem
  → handler_submit → handler_admin → handler_leaderboard → server → main
  → Phase 3 评测机 queue → compiler → runner → comparator → reporter → main
  → Phase 4 前端 js/api → js/auth → js/components → login/register
  → index → problem → submissions → submission → leaderboard → statistics → admin
  → Phase 5 联调验收
```
