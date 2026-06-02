# OnlineOJ — 功能总结

> 一个小型在线判题系统，仿 LeetCode 核心流程，面向约 20 人使用。

---

## 一句话概述

用户注册登录 → 浏览题目 → 编写 C++ 代码 → 提交 → 后端推送到 Redis 队列 → 评测机在 Docker 沙箱中编译执行 → 返回 AC/WA/TLE/MLE/CE 结果 → 排行榜排名。

---

## 核心功能一览

### 1. 用户系统
- 注册 / 登录 / 登出（Session + Cookie）
- 两种角色：普通用户、管理员
- 密码 bcrypt 加密存储

### 2. 题库管理（管理员）
- 创建/编辑/删除题目
- 题目含：标题、描述、难度（简单/中等/困难）、标签（动态规划/贪心/图论…）
- 上传测试数据 ZIP（内部为 `1.in` `1.out` `2.in` `2.out` …）
- 管理全局标签列表

### 3. 提交与评测（用户）
- 在线编写/粘贴 C++ 代码，提交
- 评测流程：编译 → 逐一运行测试点 → 输出比对
- 六种结果：**AC**（通过）/ **WA**（答案错误）/ **TLE**（超时）/ **MLE**（超内存）/ **CE**（编译错误）/ **SE**（系统错误）
- WA 时显示具体失败测试点的输入、期望输出、实际输出
- 遇到第一个失败即停止（快速失败），节约资源
- 编译错误（CE）不占提交频率额度
- 同一题同时只能有一个进行中的提交

### 4. 安全隔离
- 每次提交启动临时 Docker 容器（`--cpus=1 --memory=256m`），运行完即销毁
- 容器禁止网络访问、限制进程数
- 代码最大 64KB，防恶意提交

### 5. 排行榜与统计
- 按通过题数降序排列，同分按通过率排
- 支持按难度、标签筛选
- 个人统计：已通过题数、总提交数、通过率、按难度分布

### 6. 反滥用
- 每人每分钟最多 10 次提交（可配）
- 评测机故障时自动标记 System Error

---

## 技术架构

| 层 | 技术 |
|----|------|
| 前端 | HTML + CSS + JS + Pico.css CDN |
| 后端 | C++ (cpp-httplib) |
| 数据库 | MySQL 8.0 |
| 消息队列 | Redis 7 |
| 沙箱 | Docker |
| 部署 | Docker Compose 一键编排 |

---

## 前端页面（10 个）

| 页面 | 功能 |
|------|------|
| 登录/注册 | 账号密码认证 |
| 题目列表 | 浏览、搜索、按难度/标签筛选、分页 |
| 题目详情 | 查看描述 + 编写代码 + 提交 + 实时查看判题结果 |
| 提交记录 | 查看自己所有提交历史 |
| 提交详情 | 单次提交的代码、结果、测试点详情 |
| 排行榜 | 通过数排名，可按难度/标签筛选 |
| 个人统计 | 通过题数、提交数、通过率 |
| 管理后台 | 题目增删改、测试数据上传 |
| 标签管理 | 预设标签增删 |

---

## API 端点（20+）

```
认证：  POST /api/register | /login | /logout
       GET  /api/session

题目：  GET  /api/problems (列表+筛选)
       GET  /api/problems/:id (详情)

管理：  POST   /api/admin/problems
       PUT    /api/admin/problems/:id
       DELETE /api/admin/problems/:id
       POST   /api/admin/problems/:id/testdata (ZIP上传)
       GET    /api/problems/:id/testdata (评测机拉取)

提交：  POST /api/submit
       GET  /api/submissions/:id (轮询结果)
       GET  /api/submissions (历史)
       PUT  /api/submissions/:id/result (评测机上报)

排行：  GET /api/leaderboard
       GET /api/statistics
```

---

## 后续扩展计划（二期）

1. 集成 Monaco Editor（代码高亮 + 在线 IDE）
2. 支持 Java / Python 等多语言
3. Special Judge（自定义判题脚本）
4. 竞赛模式（限时比赛 + 实时排名）
