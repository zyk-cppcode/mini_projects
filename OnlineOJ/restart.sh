#!/bin/bash
cd /home/zyk/mini_projects/OnlineOJ

# Kill any existing
pkill -f "backend/build/backend" 2>/dev/null
pkill -f "judge/build/judge" 2>/dev/null
docker exec oj-redis redis-cli DEL submit_queue 2>/dev/null
docker exec oj-mysql mysql -u oj -pchange_me onlineoj -e "DELETE FROM submissions WHERE status IN ('pending','compiling','running');" 2>/dev/null

sleep 1

# Start backend
MYSQL_HOST=127.0.0.1 MYSQL_PORT=3307 MYSQL_USER=oj MYSQL_PASSWORD=change_me \
MYSQL_DATABASE=onlineoj REDIS_HOST=127.0.0.1 REDIS_PORT=6379 \
BACKEND_PORT=8080 SESSION_SECRET=dev_secret JUDGE_API_TOKEN=static_token_for_judge \
RATE_LIMIT_PER_MIN=10 \
setsid ./backend/build/backend </dev/null >/tmp/backend.log 2>&1 &

BACKEND_PID=$!
echo "Backend PID: $BACKEND_PID"

sleep 2

# Start judge  
REDIS_HOST=127.0.0.1 REDIS_PORT=6379 JUDGE_BACKEND_URL=http://127.0.0.1:8080 \
JUDGE_API_TOKEN=static_token_for_judge \
setsid ./judge/build/judge </dev/null >/tmp/judge.log 2>&1 &

JUDGE_PID=$!
echo "Judge PID: $JUDGE_PID"

sleep 2
echo "---"
tail -2 /tmp/backend.log
tail -2 /tmp/judge.log
