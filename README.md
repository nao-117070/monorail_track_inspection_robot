# モノレール軌道点検ロボット (Monorail Track Inspection Robot)

iPadのWebブラウザからミニPC (ROS 2) を経由し、Teensy 4.1で制御された4輪モノレールロボットを遠隔操作・モニタリングするシステムです。

---

## 🏗 システム構成

```text
[iPad (Webブラウザ)]
   │ 
   ├── HTTP (Port 8000)      : 操作画面取得
   └── WebSocket (Port 9090) : ROS 2トピック双方向通信 (rosbridge)
   ▼
[ミニPC (ROS 2 Jazzy)]
   │
   └── UDP通信 (Port 8888)   : 双方向通信 (0.1s周期データ取得 / RPM指令)
   ▼
[Teensy 4.1 (マイコン)]
   │
   └── CAN通信 / ADC         : モーター4輪駆動 (M3508) & ポテンショメータ読み取り

起動手順
1.ロボットシステムの一括起動
ミニPCのターミナルで以下を実行します。

cd ~/monorail_track_inspection_robot
./start_robot.sh

2.i Padからの操作
2.1. iPadをアクセスポイントに接続します。
2.2. ブラウザで http://192.168.1.20:8000 にアクセスします。
2.3. 画面上のボタン(前進・停止・後退)で遠隔操作を行います。

ログの保存とCSV変換
データ録画(ros2 bag)

source /opt/ros/jazzy/setup.bash
cd ~/monorail_track_inspection_robot/ros_logs
ros2 bag record /motor_status

CSVデータへの変換(Excel用)

# ターミナル1: 変換スクリプト起動
python3 bag_to_csv.py

# ターミナル2: ログ再生
ros2 bag play <対象のrosbagフォルダ>
