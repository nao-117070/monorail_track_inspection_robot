#include <Arduino.h>
#include <NativeEthernet.h>
#include <NativeEthernetUdp.h>
#include <Metro.h>

// ---------- ネットワーク設定 ----------
byte mac[] = { 0x04, 0xE9, 0xE5, 0x00, 0x00, 0x01 };
IPAddress ip(192, 168, 1, 10);        // Teensy IP
IPAddress pc_ip(192, 168, 1, 20);     // ミニPC IP
unsigned int localPort = 8888;
EthernetUDP Udp;

int16_t baseTargetRpm = 0; // ミニPCから送られてくる基準目標速度
Metro statusTimer(100);    // ★0.1秒(100ms)周期

void setup() {
  Serial.begin(115200);
  Ethernet.begin(mac, ip);
  Udp.begin(localPort);
  
  delay(1000);
  Serial.println("==========================================");
  Serial.println("  Teensy UDP Test Node Started (Debug Log)");
  Serial.println("==========================================");
}

void loop() {
  // 1. ミニPCからのUDP命令を受信 (ミニPC -> Teensy)
  int packetSize = Udp.parsePacket();
  if (packetSize > 0) {
    char packetBuffer[64];
    int len = Udp.read(packetBuffer, sizeof(packetBuffer) - 1);
    if (len > 0) {
      packetBuffer[len] = '\0';
      baseTargetRpm = (int16_t)atoi(packetBuffer); // 受信した数字を更新
      
      // ★ 受信ログをシリアル出力
      Serial.print("[RX (受信)] PCからの回転指示: ");
      Serial.print(packetBuffer);
      Serial.print(" -> 適用RPM: ");
      Serial.println(baseTargetRpm);
    }
  }

  // 2. 0.1秒(100ms)ごとにミニPCへデータ送信 (Teensy -> ミニPC)
  if (statusTimer.check()) {
    char sendBuffer[256];
    
    // ダミーデータ: 経過時間, 角度(10,20,30,40度固定), 実RPM(受信したbaseTargetRpmをそのままセット)
    sprintf(sendBuffer, "%lu,10.0,20.0,30.0,40.0,%d,%d,%d,%d",
            millis(),
            baseTargetRpm, baseTargetRpm, baseTargetRpm, baseTargetRpm);

    Udp.beginPacket(pc_ip, 8888);
    Udp.write(sendBuffer);
    Udp.endPacket();

    // ★ 送信ログをシリアル出力
    Serial.print("[TX (送信)] PCへパケット送信: ");
    Serial.println(sendBuffer);
  }
}