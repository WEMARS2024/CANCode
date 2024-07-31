#include <Arduino.h>
#include <ESP32CAN.h>
#include <CAN_config.h>
#include <Wire.h>
#include <I2Cdev.h>

CAN_device_t CAN_cfg;               // CAN Config
unsigned long previousMillis = 0;   // will store last time a CAN Message was sent
const int interval = 20;            // interval at which to send CAN Messages (milliseconds)
const int rx_queue_size = 10;       // Receive Queue size
bool btPrintCanRx = 0;
bool btEnabled = 1;
unsigned long ulCurrentMicros;
unsigned long ulPreviousMicros;
unsigned long ul5mS = 0;

union FloatIntUnion {
    float f;
    int i;
};
union FloatIntUnion fiu;
CAN_frame_t tx_frame;
CAN_frame_t rx_frame;  // Added rx_frame to handle received frames

// IMU data placeholders
float angleX, angleY, angleZ;

void setup() {
  Serial.begin(921600); // Keep baud rate at 921600
  while(!Serial){
    ;
  }

  Serial.println("ESP32-CAN");
  CAN_cfg.tx_pin_id = GPIO_NUM_21;
  CAN_cfg.rx_pin_id = GPIO_NUM_22;
  CAN_cfg.speed = CAN_SPEED_1000KBPS;
  CAN_cfg.rx_queue = xQueueCreate(rx_queue_size, sizeof(CAN_frame_t));
  ESP32Can.CANInit(); //initialize CAN with previous settings
}

void loop() {
  ulCurrentMicros = micros();
  if((ulCurrentMicros - ulPreviousMicros) >= 500) {
    ulPreviousMicros = ulCurrentMicros;
    ul5mS = ul5mS + 5;
  }

  if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS) == pdTRUE) {
    if (rx_frame.FIR.B.FF == CAN_frame_ext) {
      // Process X Gyro Data
      if (rx_frame.MsgID == 0x040C1801) {
        uint8_t* p = (uint8_t* )&angleX;
        p[0] = rx_frame.data.u8[0];
        p[1] = rx_frame.data.u8[1];
        p[2] = rx_frame.data.u8[2];
        p[3] = rx_frame.data.u8[3];
        Serial.print("Received X: ");
        Serial.println(angleX);
      }
      // Process Y Gyro Data
      if (rx_frame.MsgID == 0x040C1841) {
        uint8_t* p = (uint8_t* )&angleY;
        p[0] = rx_frame.data.u8[0];
        p[1] = rx_frame.data.u8[1];
        p[2] = rx_frame.data.u8[2];
        p[3] = rx_frame.data.u8[3];
        Serial.print("Received Y: ");
        Serial.println(angleY);
      }
      // Process Z Gyro Data
      if (rx_frame.MsgID == 0x040C1881) {
        uint8_t* p = (uint8_t* )&angleZ;
        p[0] = rx_frame.data.u8[0];
        p[1] = rx_frame.data.u8[1];
        p[2] = rx_frame.data.u8[2];
        p[3] = rx_frame.data.u8[3];
        Serial.print("Received Z: ");
        Serial.println(angleZ);
      }
    }
  }

  if((ul5mS % 100) == 0) { //sending IMU data every 100ms
    if(!btEnabled) {  //If CAN transmission is enabled (I set it to one)
      tx_frame.FIR.B.FF = CAN_frame_ext; //Set Extended frame
      tx_frame.FIR.B.DLC = 4;

      uint8_t* p;

      // Send X Gyro data
      tx_frame.MsgID = 0x041206000A;  //X header 
      p = (uint8_t* )&angleX; //Breaks float into individual byte components and has p act as the pointer
      tx_frame.data.u8[0] = p[0];
      tx_frame.data.u8[1] = p[1];
      tx_frame.data.u8[2] = p[2];
      tx_frame.data.u8[3] = p[3];
      ESP32Can.CANWriteFrame(&tx_frame);

      // Send Y Gyro data
      tx_frame.MsgID = 0x041206010A; //Y header
      p = (uint8_t* )&angleY;
      tx_frame.data.u8[0] = p[0];
      tx_frame.data.u8[1] = p[1];
      tx_frame.data.u8[2] = p[2];
      tx_frame.data.u8[3] = p[3];
      ESP32Can.CANWriteFrame(&tx_frame);

      // Send Z Gyro data
      tx_frame.MsgID = 0x041206020A; //Z Header
      p = (uint8_t* )&angleZ;
      tx_frame.data.u8[0] = p[0];
      tx_frame.data.u8[1] = p[1];
      tx_frame.data.u8[2] = p[2];
      tx_frame.data.u8[3] = p[3];
      ESP32Can.CANWriteFrame(&tx_frame);
    }
  }
}