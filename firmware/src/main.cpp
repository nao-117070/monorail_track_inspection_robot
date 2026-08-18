#include <Arduino.h>
#include "DriveController.h"
#include "RK39A1A00007.h"
#include <IntervalTimer.h>
#include <Metro.h>

const GainParams speedLoopGain = {3, 0.3, 0.5};
const GainParams positionLoopGain = {0.03, 0.0, 0.0};
const double controlIntervalMs = 1.0;
const int controllerNodeId = 1;
const int motorCount = 4; // 制御するモータ数

const int potPin = A0;             // ここにポテンショメータを接続します
const int potAnalogMax = 4095;     // Teensy 4.1 の ADC 解像度
const float potAngleScale = 360.0f / potAnalogMax;
const int canBusBaud = 500000;     // 必要に応じてボーレートを変更

BusLink busBridge(controllerNodeId);
DriveController driveControl(busBridge, controlIntervalMs);
RK39A1A00007Sensor angleSensor(busBridge, 0x300);
IntervalTimer updateTimer;
Metro statusTimer(500);

void setup() {
  Serial.begin(115200);
  delay(100);
  busBridge.initBus(canBusBaud);
  angleSensor.begin();

  pinMode(potPin, INPUT);

  for (int id = 1; id <= motorCount; ++id) {
    driveControl.configureMotorType(id, DRIVE_M3508);
    driveControl.setSpeedGain(id, speedLoopGain);
    driveControl.setPositionGain(id, positionLoopGain);
    driveControl.enableTorque(id, true);
  }
  updateTimer.begin(DriveController::interruptHandler, controlIntervalMs * 1000);
}

void loop() {
  const int potRaw = analogRead(potPin);
  const float potVoltageAngle = potRaw * potAngleScale;
  const uint8_t potSensorId = 1; // 確認したいポテンショメータID

  if (statusTimer.check()) {
    Serial.print("AnalogPot A0:");
    Serial.print(potRaw);
    Serial.print(" -> ");
    Serial.print(potVoltageAngle, 1);
    Serial.println(" deg");

    Serial.print("CanPotID "); Serial.print(potSensorId);
    if (angleSensor.readAngle(potSensorId)) {
      Serial.print(" ANG:"); Serial.println(angleSensor.getAngleDegrees(potSensorId), 2);
    } else {
      Serial.println(" ANG:N/A");
    }
  }
}


