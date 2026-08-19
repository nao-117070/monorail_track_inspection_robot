#!/bin/bash

echo "=========================================="
echo "  モノレールロボット システム起動中..."
echo "=========================================="

# ROS 2の環境設定
source /opt/ros/jazzy/setup.bash
source ~/monorail_track_inspection_robot/install/setup.bash

# 1. Web画面配信サーバーをバックグラウンド起動(&)
echo "[1/3] Webサーバー(ポート8000)を起動します"
cd ~/monorail_track_inspection_robot/web
python3 -m http.server 8000 &
HTTP_PID=$!

# 2. rosbridge(WebSocket)をバックグラウンド起動(&)
echo "[2/3] rosbridgeサーバー(ポート9090)を起動します"
ros2 launch rosbridge_server rosbridge_websocket_launch.xml > /dev/null 2>&1 &
BRIDGE_PID=$!
sleep 2  # 起動完了まで2秒待機

# 3. モーターUDP通信ノードをバックグラウンド起動(&)
echo "[3/3] モーター通信ノードを起動します"
ros2 run motor_udp_node udp_publisher_exec &
UDP_PID=$!

echo "=========================================="
echo "  起動完了！ iPadからアクセスしてください"
echo "  (終了するにはここで Ctrl + C を押します)"
echo "=========================================="

# Ctrl+C が押されたら、裏で動いている3つのプログラムをまとめて終了する設定
trap "echo -e '\nシステムを終了しています...'; kill $HTTP_PID $BRIDGE_PID $UDP_PID; exit" INT

# プログラムが終了しないように待機
wait
