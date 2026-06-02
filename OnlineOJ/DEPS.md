# Ubuntu 22 依赖安装说明

> 根据 SPEC.md 整理，适用于本机 Ubuntu 22.04 开发环境。

---

## 1. 系统依赖（apt 安装）

```bash
sudo apt update && sudo apt install -y \
  build-essential       # g++、make 等编译工具链 \
  cmake                 # C++ 项目构建 \
  libmysqlclient-dev    # MySQL 客户端库（C++ 连接 MySQL 8.0） \
  libzip-dev            # ZIP 解压库（解析测试数据 ZIP） \
  libhiredis-dev        # Redis C 客户端库（消息队列消费） \
  libssl-dev            # OpenSSL 开发库（bcrypt 密码哈希 / HTTPS） \
  pkg-config            # 编译时查找库依赖
```

---

## 2. Docker 相关

```bash
# 安装 Docker Engine
sudo apt install -y docker.io

# 安装 Docker Compose（独立插件）
sudo apt install -y docker-compose-v2

# 将当前用户加入 docker 组（免 sudo 运行 docker）
sudo usermod -aG docker $USER

# 重新登录后生效，或执行：
newgrp docker
```

**Docker 用途**：
- 评测机通过 Docker CLI 启动隔离容器编译运行用户 C++ 代码
- Docker Compose 一键编排全部服务（MySQL、Redis、backend、judge）

---

## 3. 第三方 C++ 库

以下库为 header-only 或需手动获取：

| 库 | 用途 | 获取方式 |
|----|------|----------|
| **cpp-httplib** | HTTP 服务器框架（后端 REST API） | header-only，放入 `backend/` 目录或通过 CMake FetchContent 下载 |
| **nlohmann/json** | JSON 解析/序列化 | header-only，通过 CMake FetchContent 或手动下载 |
| **bcrypt** (Openwall) | 密码 bcrypt 哈希 | 编译进项目，OPENBSD_ORIGINAL 风格，CMake 中链接 `-lcrypt` 或使用 OpenSSL 实现 |

---

## 4. 容器内服务（无需本机安装）

以下由 Docker Compose 自动管理，本机不需要额外安装：

| 服务 | 用途 | 说明 |
|------|------|------|
| **MySQL 8.0** | 持久化存储 | docker-compose 启动 mysql:8.0 容器 |
| **Redis 7** | 消息队列 (SubmitJob/Result) | docker-compose 启动 redis:7 容器 |
| **gcc:13** | C++ 编译运行镜像 | 评测机评测时按需拉取 |

---

## 5. 一键安装脚本

```bash
#!/bin/bash
set -e

# 系统库
sudo apt update
sudo apt install -y build-essential cmake libmysqlclient-dev libzip-dev libhiredis-dev libssl-dev pkg-config

# Docker
sudo apt install -y docker.io docker-compose-v2
sudo usermod -aG docker $USER

echo "安装完成。请重新登录使 docker 组生效。"
```

---

## 6. 验证安装

```bash
# 验证编译工具
g++ --version
cmake --version

# 验证库文件
pkg-config --libs mysqlclient
pkg-config --libs libzip
pkg-config --libs hiredis
pkg-config --libs openssl

# 验证 Docker
docker --version
docker compose version
```
