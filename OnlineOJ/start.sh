#!/bin/bash
# OnlineOJ 本地开发启动脚本
# 用法: bash start.sh [start|stop|status]

set -e
BASE_DIR="$(cd "$(dirname "$0")" && pwd)"

case "${1:-start}" in
  start)
    echo "=== Starting MySQL container ==="
    docker rm -f oj-mysql 2>/dev/null || true
    docker run -d --name oj-mysql --restart unless-stopped \
      -e MYSQL_ROOT_PASSWORD=root_change_me \
      -e MYSQL_USER=oj \
      -e MYSQL_PASSWORD=change_me \
      -e MYSQL_DATABASE=onlineoj \
      -p 3307:3306 \
      -v "$BASE_DIR/init.sql:/docker-entrypoint-initdb.d/init.sql" \
      mysql:8.0

    echo "=== Starting Redis container ==="
    docker rm -f oj-redis 2>/dev/null || true
    docker run -d --name oj-redis --restart unless-stopped \
      -p 6379:6379 redis:7-alpine

    echo "=== Waiting for services to be ready ==="
    sleep 5
    until docker exec oj-mysql mysqladmin ping -h localhost -u root -proot_change_me --silent 2>/dev/null; do
      echo "  Waiting for MySQL..."
      sleep 2
    done
    until docker exec oj-redis redis-cli ping 2>/dev/null | grep -q PONG; do
      echo "  Waiting for Redis..."
      sleep 1
    done
    echo "  Services ready."

    echo "=== Building (if needed) ==="
    cd "$BASE_DIR/backend/build" && cmake .. > /dev/null 2>&1 && make -j$(nproc) > /dev/null 2>&1
    cd "$BASE_DIR/judge/build" && cmake .. > /dev/null 2>&1 && make -j$(nproc) > /dev/null 2>&1
    cd "$BASE_DIR"

    echo "=== Starting Backend (port 8080) ==="
    pkill -f "backend/build/backend" 2>/dev/null || true
    nohup env MYSQL_HOST=127.0.0.1 MYSQL_PORT=3307 MYSQL_USER=oj MYSQL_PASSWORD=change_me \
      MYSQL_DATABASE=onlineoj REDIS_HOST=127.0.0.1 REDIS_PORT=6379 \
      BACKEND_PORT=8080 SESSION_SECRET=dev_secret JUDGE_API_TOKEN=static_token_for_judge \
      RATE_LIMIT_PER_MIN=10 \
      "$BASE_DIR/backend/build/backend" > /tmp/oj-backend.log 2>&1 &
    sleep 2

    echo "=== Starting Judge ==="
    pkill -f "judge/build/judge" 2>/dev/null || true
    mkdir -p /tmp/judge
    nohup env REDIS_HOST=127.0.0.1 REDIS_PORT=6379 \
      JUDGE_BACKEND_URL=http://127.0.0.1:8080 JUDGE_API_TOKEN=static_token_for_judge \
      "$BASE_DIR/judge/build/judge" > /tmp/oj-judge.log 2>&1 &
    sleep 1

    echo ""
    echo "=== OnlineOJ Started ==="
    echo "  Frontend:  http://localhost:8080"
    echo "  Backend:   http://localhost:8080"
    echo "  MySQL:     localhost:3307"
    echo "  Redis:     localhost:6379"
    echo ""
    echo "  Logs: tail -f /tmp/oj-backend.log /tmp/oj-judge.log"
    echo "  Stop:  bash start.sh stop"
    ;;

  stop)
    echo "=== Stopping ==="
    pkill -f "backend/build/backend" 2>/dev/null || true
    pkill -f "judge/build/judge" 2>/dev/null || true
    docker stop oj-mysql oj-redis 2>/dev/null || true
    docker rm oj-mysql oj-redis 2>/dev/null || true
    echo "  All services stopped."
    ;;

  status)
    echo "=== Service Status ==="
    echo -n "  Backend: "; pgrep -f "backend/build/backend" > /dev/null && echo "RUNNING" || echo "STOPPED"
    echo -n "  Judge:   "; pgrep -f "judge/build/judge" > /dev/null && echo "RUNNING" || echo "STOPPED"
    echo -n "  MySQL:   "; docker exec oj-mysql mysqladmin ping --silent 2>/dev/null && echo "RUNNING" || echo "STOPPED"
    echo -n "  Redis:   "; docker exec oj-redis redis-cli ping 2>/dev/null | grep -q PONG && echo "RUNNING" || echo "STOPPED"
    ;;

  *)
    echo "Usage: bash start.sh [start|stop|status]"
    exit 1
    ;;
esac
