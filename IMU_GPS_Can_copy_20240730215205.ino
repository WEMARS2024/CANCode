#include <Arduino.h>
#include <driver/twai.h>
#include <Wire.h>
#include <I2Cdev.h>
#include "MPU6050_6Axis_MotionApps20.h"
#include <MPU6050.h>
#include <TinyGPS++.h>

// IMU SETUP
const int pinSDA = 13;
const int pinSCL = 14;
const int gpsRX = 19;
const int gpsTX = 23;

MPU6050 mpu;
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);

bool dmpReady = false;
uint8_t devStatus;
uint8_t packetSize;
uint8_t fifoCount;
uint8_t fifoBuffer[64];
Quaternion q;
VectorFloat gravity;
float ypr[3];
bool btEnabled = 1;

// CAN SETUP
const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_1MBITS();
const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
const twai_general_config_t g_config = {
    .mode = TWAI_MODE_NORMAL,
    .tx_io = GPIO_NUM_48, // GPIO 48 for CAN TX
    .rx_io = GPIO_NUM_47, // GPIO 47 for CAN RX
    .clkout_io = TWAI_IO_UNUSED,
    .bus_off_io = TWAI_IO_UNUSED,
    .tx_queue_len = 10,
    .rx_queue_len = 10,
    .alerts_enabled = TWAI_ALERT_NONE,
    .clkout_divider = 0
};

twai_message_t tx_msg;  // Define the CAN frame variable

void setup() {
  // Serial setup
  Serial.begin(921600);
  twai_driver_install(&g_config, &t_config, &f_config); // Initialize CAN driver
  twai_start(); // Start CAN driver

  // IMU SETUP
  Wire.begin(pinSDA, pinSCL); // Join I2C bus with custom SDA and SCL pins
  Wire.setClock(400000); // 400kHz I2C clock
  mpu.initialize();
  mpu.setDLPFMode(1); // Set DLPF mode to 1 (184 Hz bandwidth)
  mpu.setRate(9); // Sample rate divider set to 9 for 100Hz sample rate
  Serial.println(F("Initializing DMP..."));
  devStatus = mpu.dmpInitialize();

  // Min sensitivity gyro Offets, can be better calibrated in future
  mpu.setXGyroOffset(220);
  mpu.setYGyroOffset(76);
  mpu.setZGyroOffset(-85);
  mpu.setZAccelOffset(1788);

  if (devStatus == 0) {
    mpu.CalibrateGyro(6);
    mpu.PrintActiveOffsets();

    Serial.println(F("Enabling DMP..."));
    mpu.setDMPEnabled(true);

    Serial.println(F("DMP ready!"));
    dmpReady = true;

    packetSize = mpu.dmpGetFIFOPacketSize();
  } else {
    Serial.print(F("DMP Initialization failed (code "));
    Serial.print(devStatus);
    Serial.println(F(")"));
    while (1); // Stay here if DMP initialization fails
  }
  //IMU SETUP


  //GPS SETUP
  gpsSerial.begin(9600, SERIAL_8N1, 23, 19);
  

  Serial.println("Setup Complete");
}


void loop() {
  unsigned long ulCurrentMicros = micros();
  static unsigned long ulPreviousMicros = 0;
  static unsigned long ul5mS = 0;

  if ((ulCurrentMicros - ulPreviousMicros) >= 500) {
    ulPreviousMicros = ulCurrentMicros;
    ul5mS = ul5mS + 5;
  }

  // IMU CAN MESSAGING LOOP
  if (!dmpReady) {
    Serial.println("DMP not ready");
    return;
  }

  // Check for new data
  fifoCount = mpu.getFIFOCount();

  if (fifoCount == 1024) {
    mpu.resetFIFO();
    Serial.println("FIFO overflow!"); //If this appears, need to read packets faster for no overflow
  } else if (fifoCount >= packetSize) {
    // Read a packet from FIFO
    mpu.getFIFOBytes(fifoBuffer, packetSize);

    mpu.dmpGetQuaternion(&q, fifoBuffer);
    mpu.dmpGetGravity(&gravity, &q);
    mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);

    float angleX = ypr[1] * 180 / M_PI;
    float angleY = ypr[2] * 180 / M_PI;
    float angleZ = ypr[0] * 180 / M_PI;

    if ((ul5mS % 100) == 0) { //sending IMU data every 100ms
      if (btEnabled) {  //If CAN transmission is enabled (I set it to one)

        uint8_t* p;


        // Send X Gyro data
        tx_msg.identifier = 0x040C1801;  // X header
        tx_msg.extd = 1;  // Extended frame
        tx_msg.data_length_code = 4;
        p = (uint8_t*)&angleX; // Breaks float into individual byte components and has p act as the pointer
        tx_msg.data[0] = p[0];
        tx_msg.data[1] = p[1];
        tx_msg.data[2] = p[2];
        tx_msg.data[3] = p[3];
        if(twai_transmit(&tx_msg, 3 * portTICK_PERIOD_MS) == ESP_OK){
          Serial.println("transmitted x succesfully");
        }



        // Send Y Gyro data
        tx_msg.identifier = 0x040C1841; // Y header   0x
        p = (uint8_t*)&angleY;
        tx_msg.data[0] = p[0];
        tx_msg.data[1] = p[1];
        tx_msg.data[2] = p[2];
        tx_msg.data[3] = p[3];
        twai_transmit(&tx_msg, 3 * portTICK_PERIOD_MS);

        // Send Z Gyro data
        tx_msg.identifier = 0x040C1881; // Z Header
        p = (uint8_t*)&angleZ;
        tx_msg.data[0] = p[0];
        tx_msg.data[1] = p[1];
        tx_msg.data[2] = p[2];
        tx_msg.data[3] = p[3];
        twai_transmit(&tx_msg, 3 * portTICK_PERIOD_MS);
      }
    }
  }
  // IMU CAN MESSAGING LOOP

  //GPS CAN LOOP

  if (!btEnabled) {
    while (gpsSerial.available()) {
      gps.encode(gpsSerial.read());
      if (gps.location.isUpdated()) {
        Serial.print("Latitude: ");
        Serial.println(gps.location.lat(), 6);

        Serial.print("Longitude: ");
        Serial.println(gps.location.lng(), 6);

        Serial.print("Satellites: ");
        Serial.println(gps.satellites.value());
      }
    }


  }


}
