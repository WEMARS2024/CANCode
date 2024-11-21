#include <Arduino.h>
#include <driver/twai.h>
#include <Wire.h>
#include <I2Cdev.h>
//#include "MyWEBserver.h"

unsigned long previousMillis = 0;   // will store last time a CAN Message was send
const int interval = 20;          // interval at which send CAN Messages (milliseconds)
const int rx_queue_size = 10;       // Receive Queue size
bool btPrintCanRx = 0;
bool btEnabled = 0;
bool btH_Enabled = 0;
bool CANBUS_Loss_Error = 0;
bool CAN_Found = 0;
bool NegateMotorSpeed;
unsigned char bTEstByte = 0;
char cIncomingByte = 0; // for incoming serial data
unsigned char ubDeviceType;
unsigned char ubManufacturer;
unsigned char ubAPIClass;
unsigned char ubAPIIndex;
unsigned char ubDeviceNumber;
unsigned char ubDeviceNumberToPrint = 99;
bool receivedX = false, receivedY = false, receivedZ = false, receivedLat = false, receivedLng = false, receivedSpeed = false;
int joyStickInputs[5];
unsigned char ubDeviceConnected[20];
int iMotorCurrent;
float fMotorSpeed[20];
float fMotorCurrent[20];
float angleX, angleY, angleZ, speed;
float baseX, baseY, wristX, forearmY, claw;
float Lwheel1C, Lwheel2C, Lwheel3C, Rwheel1C, Rwheel2C, Rwheel3C;
double latitude, longitude;


unsigned char ubDeviceIndex;
unsigned char ubDeviceCount;
unsigned char ubPrintCounter;
unsigned char ubUSB_Rx_Buffer[10];
float fUSB_Rx_Speed;

union FloatIntUnion {
    float f;
    int i;
};
union FloatIntUnion fiu;

bool btIsWEB = 0;

//extern String CAN_message;  //used for web page

char bMotorSpeed[6][4];

//Set Timing Variables
unsigned long ulCurrentMicros;
unsigned long ulPreviousMicros;
unsigned long ul5mS = 0;

//Twai CAN Setup
const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_1MBITS(); // Use 1 MBPS timing configuration
const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL(); // Accept all filter config
const twai_general_config_t g_config = {
    .mode = TWAI_MODE_NORMAL,
    .tx_io = GPIO_NUM_21, // Set GPIO 21 for CAN TX
    .rx_io = GPIO_NUM_22, // Set GPIO 22 for CAN RX
    .clkout_io = TWAI_IO_UNUSED,
    .bus_off_io = TWAI_IO_UNUSED,
    .tx_queue_len = 10,
    .rx_queue_len = 10,
    .alerts_enabled = TWAI_ALERT_NONE,
    .clkout_divider = 0
};

twai_message_t tx_msg;
twai_message_t rx_msg;
 

void setup() {
  Serial.begin(921600);
  Serial.println("ESP32-CAN");
  //Initialize Twai Can
  twai_driver_install(&g_config, &t_config, &f_config);
  twai_start();


  for(int MotorID = 0; MotorID < 6;MotorID++)
  {
    bMotorSpeed[MotorID][0] = 0;
    bMotorSpeed[MotorID][1] = 0;
    bMotorSpeed[MotorID][2] = 0;
    bMotorSpeed[MotorID][3] = 0;
  }

  //setupWEbServer();

  pinMode(2, OUTPUT);
  
}


void loop() {

  digitalWrite(2, HIGH);
  delay(500);
  digitalWrite(2, LOW);

  ulCurrentMicros = micros();
  if((ulCurrentMicros - ulPreviousMicros) >= 500)
  {
    ulPreviousMicros = ulCurrentMicros;
    ul5mS = ul5mS + 5;
  }


  while (Serial.available() > 0) 
  {
    cIncomingByte = Serial.read();
    //Serial.printf("USB RX: ,%i\n",cIncomingByte);
    switch(cIncomingByte)
    {
     case 'S':
     case 's':   //emergency stop all
     {
       tx_msg.extd = 1;
       tx_msg.identifier = 0x00000000;
       tx_msg.data_length_code = 0;
    
       if (twai_transmit(&tx_msg, pdMS_TO_TICKS(3)) != ESP_OK)
       {
         CANBUS_Loss_Error = 1;
       }   
       cIncomingByte = 0;
      break;
     }
     case 'h':
     case 'H':  //heart beat can with generic heart beat
     {
       btH_Enabled ^= 1;
       if(btH_Enabled)
       {
        Serial.println("Heart ON");
       }
       else
       {
        Serial.println("Heart OFF");
       }
       
       cIncomingByte = 0;
       break;
     }
     case 'E':
     case 'e':    //enable can 
     {
       btEnabled = 1;
       btH_Enabled = 0;
      
       cIncomingByte = 0;
       break;
     }
     case '1'://printe can id 1
     {
       ubDeviceNumberToPrint = 1;
       cIncomingByte = 0;
       break;
     }
     case '2': //print can id 2
     {
       ubDeviceNumberToPrint = 2;
       cIncomingByte = 0;
       break;
     }
     case '3'://print can id 3
     {
       ubDeviceNumberToPrint = 
       31;
       cIncomingByte = 0;
       break;
     }
     case '4':// print can id 4
     {
       ubDeviceNumberToPrint = 4;
       cIncomingByte = 0;
       break;
     }
     case '5': //print can id 5
     {
       ubDeviceNumberToPrint = 5;
       cIncomingByte = 0;
       break;
     }
     case '6'://print can id 6
     {
       ubDeviceNumberToPrint = 6;
       cIncomingByte = 0;
       break;
     }
     case 'a':
     case 'A':     //print all
     {
       ubDeviceNumberToPrint = 99;
       cIncomingByte = 0;
       break;
     }
     case 'P':
     case 'p':  //turn on/off printing 
     {
       
       btPrintCanRx ^= 1;
       if(btPrintCanRx)
       {
        Serial.println("Print ON");
       }
       else
       {
        Serial.println("Print OFF");
       }
       break;
     }
     case 'D':
     case 'd':   //disable motors
     {
    
       btEnabled = 0;
       btH_Enabled = 1;
       cIncomingByte = 0;
       break;
     } 
     case 'M':
     case 'm':   //motor drive command
     {
      
       btEnabled = 1;
       btH_Enabled = 0;
       btIsWEB - 0;
       cIncomingByte = Serial.read();
      if((cIncomingByte == 'L') || (cIncomingByte == 'l'))  //all left motors
          {
          cIncomingByte = Serial.read();
          if(cIncomingByte == ',')
          {
            ubUSB_Rx_Buffer[0] = Serial.read(); //"+/-"
            ubUSB_Rx_Buffer[1] = Serial.read(); //"1 or 0"
            ubUSB_Rx_Buffer[2] = Serial.read(); //"."
            ubUSB_Rx_Buffer[3] = Serial.read(); //"0 - 9"
            ubUSB_Rx_Buffer[4] = Serial.read(); //"0 - 9"
            ubUSB_Rx_Buffer[5] = 0;

            fiu.f = fUSB_Rx_Speed;
            bMotorSpeed[0][0] = (fiu.i & 0x000000FF);
            bMotorSpeed[0][1] = (fiu.i & 0x0000FF00) >> 8;
            bMotorSpeed[0][2] = (fiu.i & 0x00FF0000) >> 16;
            bMotorSpeed[0][3] = (fiu.i & 0xFF000000) >> 24;

            bMotorSpeed[1][0] = (fiu.i & 0x000000FF);
            bMotorSpeed[1][1] = (fiu.i & 0x0000FF00) >> 8;
            bMotorSpeed[1][2] = (fiu.i & 0x00FF0000) >> 16;
            bMotorSpeed[1][3] = (fiu.i & 0xFF000000) >> 24;

            bMotorSpeed[2][0] = (fiu.i & 0x000000FF);
            bMotorSpeed[2][1] = (fiu.i & 0x0000FF00) >> 8;
            bMotorSpeed[2][2] = (fiu.i & 0x00FF0000) >> 16;
            bMotorSpeed[2][3] = (fiu.i & 0xFF000000) >> 24;
            //Serial.printf("bMotorSpeed: 0x%02x 0x%02x 0x%02x, 0x%02x\n",bMotorSpeed[0][0],bMotorSpeed[0][1],bMotorSpeed[0][2],bMotorSpeed[0][3]);
          }
        }
        if((cIncomingByte == 'R') || (cIncomingByte == 'r'))  //all right motors
          {
          cIncomingByte = Serial.read();
          if(cIncomingByte == ',')
          {
            ubUSB_Rx_Buffer[0] = Serial.read(); //"+/-"
            ubUSB_Rx_Buffer[1] = Serial.read(); //"1 or 0"
            ubUSB_Rx_Buffer[2] = Serial.read(); //"."
            ubUSB_Rx_Buffer[3] = Serial.read(); //"0 - 9"
            ubUSB_Rx_Buffer[4] = Serial.read(); //"0 - 9"
            ubUSB_Rx_Buffer[5] = 0;
            Serial.printf("USB RX: %s\n",ubUSB_Rx_Buffer);
            fUSB_Rx_Speed =  atof((char*)ubUSB_Rx_Buffer);
            fiu.f = fUSB_Rx_Speed;
            bMotorSpeed[3][0] = (fiu.i & 0x000000FF);
            bMotorSpeed[3][1] = (fiu.i & 0x0000FF00) >> 8;
            bMotorSpeed[3][2] = (fiu.i & 0x00FF0000) >> 16;
            bMotorSpeed[3][3] = (fiu.i & 0xFF000000) >> 24;

            bMotorSpeed[4][0] = (fiu.i & 0x000000FF);
            bMotorSpeed[4][1] = (fiu.i & 0x0000FF00) >> 8;
            bMotorSpeed[4][2] = (fiu.i & 0x00FF0000) >> 16;
            bMotorSpeed[4][3] = (fiu.i & 0xFF000000) >> 24;

            bMotorSpeed[5][0] = (fiu.i & 0x000000FF);
            bMotorSpeed[5][1] = (fiu.i & 0x0000FF00) >> 8;
            bMotorSpeed[5][2] = (fiu.i & 0x00FF0000) >> 16;
            bMotorSpeed[5][3] = (fiu.i & 0xFF000000) >> 24;
            //Serial.printf("bMotorSpeed: 0x%02x 0x%02x 0x%02x, 0x%02x\n",bMotorSpeed[0][0],bMotorSpeed[0][1],bMotorSpeed[0][2],bMotorSpeed[0][3]);
          }
        }
       cIncomingByte = 0;
       break;
     } 
     case 'X':
     case 'x': 
     {

      btEnabled = 1;
      btH_Enabled = 0;

      joyStickInputs[0] = Serial.read();
      joyStickInputs[1] = Serial.read();
      joyStickInputs[2] = Serial.read();
      joyStickInputs[3] = Serial.read();
      joyStickInputs[4] = Serial.read();
      joyStickInputs[5] = Serial.read();

      cIncomingByte = 0;
      break;
     }
     

    }

  }


  // Receive next CAN frame from queue
  if (twai_receive(&rx_msg, pdMS_TO_TICKS(3)) == ESP_OK) {
   // Serial.printf("frame           :%x\n",rx_frame.MsgID);
   ubDeviceType = (unsigned char)((rx_msg.identifier &   0B00011111000000000000000000000000)>>24); //5-bit value
   ubManufacturer = (unsigned char)((rx_msg.identifier & 0B00000000111111110000000000000000)>>16); //8-bit value
   ubAPIClass = (unsigned char)((rx_msg.identifier &     0B00000000000000001111110000000000)>>10); //6-bit 
   ubAPIIndex = (unsigned char)((rx_msg.identifier &     0B00000000000000000000001111000000)>>6);  //4-bit 
   ubDeviceNumber = (unsigned char)(rx_msg.identifier &  0B00000000000000000000000000111111);  //6-bit

   if (btPrintCanRx) {  // Check if CAN RX data should be printed
        if ((ubDeviceNumber == ubDeviceNumberToPrint) || (ubDeviceNumberToPrint == 99)) {
            printf("%i\t0x%08X\t0x%02X\t0x%02X\t0x%02X\t0x%02X\t0x%02X\tDLC %d\t", micros(), rx_msg.identifier, ubDeviceType, ubManufacturer, ubAPIClass, ubAPIIndex, ubDeviceNumber, rx_msg.data_length_code);
            for (int i = 0; i < rx_msg.data_length_code; i++) {
                printf("0x%02X\t", rx_msg.data[i]);
            }
            printf("\n");
        }
    }
   
   if((ubDeviceType == 0x02) && (ubAPIClass == 0x06) && (ubAPIIndex == 0x01))  //assuming there are 6 motor drivers addressed 1 to 6
   {
     ubDeviceConnected[ubDeviceNumber-1] = ubDeviceNumber;
     fMotorSpeed[ubDeviceNumber-1] = (float)((rx_msg.data[3]<<24) & (rx_msg.data[2]<<16) & (rx_msg.data[1] <<8) & (rx_msg.data[0]));
     iMotorCurrent = (rx_msg.data[6] <<8) & (rx_msg.data[7]);
     fMotorCurrent[ubDeviceNumber-1] = (iMotorCurrent / 4095) * 80;
     
    // CAN_message = "";
     for(ubDeviceIndex = 0;ubDeviceIndex <= 14;ubDeviceIndex++)
      {
        if((ubDeviceConnected[ubDeviceIndex] == 0) || (ubDeviceConnected[ubDeviceIndex] > 6))
        {
         // CAN_message += "0;0;0;";
        }
        else
        {
        //  CAN_message += String(ubDeviceConnected[ubDeviceIndex]) + ";" + String(fMotorSpeed[ubDeviceIndex]) + ";" + String(fMotorCurrent[ubDeviceIndex])+ ";";
        }
      }
     // CAN_message += "END";
     
   }

  
    rx_ID = "MessageID: 0x" + String(rx_msg.identifier, HEX)

    Serial.println(rx_ID)

    Serial.print("Data Length: ");
    Serial.println(rx_msg.data_length_code);

    Serial.print("Data: ");
    for (int i = 0; i < rx_msg.data_length_code; i++) {
      Serial.print("0x");
      Serial.print(rx_msg.data[i], HEX);
      Serial.print(" ");
    }
    Serial.println(); 

   if (rx_msg.identifier == 0x040C1801) {
        uint8_t *p = (uint8_t *)&angleX;
        p[0] = rx_msg.data[0];
        p[1] = rx_msg.data[1];
        p[2] = rx_msg.data[2];
        p[3] = rx_msg.data[3];
        receivedX = true;
      }

   if (rx_msg.identifier == 0x040C1841) {
        uint8_t *p = (uint8_t *)&angleY;
        p[0] = rx_msg.data[0];
        p[1] = rx_msg.data[1];
        p[2] = rx_msg.data[2];
        p[3] = rx_msg.data[3];
        receivedY = true;
      }

   if (rx_msg.identifier == 0x040C1881) {
        uint8_t *p = (uint8_t *)&angleZ;
        p[0] = rx_msg.data[0];
        p[1] = rx_msg.data[1];
        p[2] = rx_msg.data[2];
        p[3] = rx_msg.data[3];
        receivedZ = true;
      }

   if (rx_msg.identifier == 0x050C1801) {
        uint8_t *p = (uint8_t *)&latitude;
        p[0] = rx_msg.data[0];
        p[1] = rx_msg.data[1];
        p[2] = rx_msg.data[2];
        p[3] = rx_msg.data[3];
        p[4] = rx_msg.data[4];
        p[5] = rx_msg.data[5];
        p[6] = rx_msg.data[6];
        p[7] = rx_msg.data[7];
        receivedLat = true;
      }

   if (rx_msg.identifier == 0x050C1841) {
        uint8_t *p = (uint8_t *)&longitude;
        p[0] = rx_msg.data[0];
        p[1] = rx_msg.data[1];
        p[2] = rx_msg.data[2];
        p[3] = rx_msg.data[3];
        p[4] = rx_msg.data[4];
        p[5] = rx_msg.data[5];
        p[6] = rx_msg.data[6];
        p[7] = rx_msg.data[7];
        receivedLng = true;
      }

  if (rx_msg.identifier == 0x050C1881) {
        uint8_t *p = (uint8_t *)&speed;
        p[0] = rx_msg.data[0];
        p[1] = rx_msg.data[1];
        p[2] = rx_msg.data[2];
        p[3] = rx_msg.data[3];
        receivedSpeed = true;
      }

  //LOOK INTO THIS ALTERNATIVE METHOD, WILL MAKE CODE LOOK NICER!
  //if (rx_msg.identifier == 0x050C1881) {
    //memcpy(&speed, rx_msg.data, sizeof(float));  // Copy 4 bytes directly into the speed variable
    //receivedSpeed = true;  // Mark that speed has been received
  //}
  
  

   
   

   if (receivedX && receivedY && receivedZ && receivedLat && receivedLng && receivedSpeed == true){
        Serial.print(angleX);
        Serial.print(", ");
        Serial.print(angleY);
        Serial.print(", ");
        Serial.print(angleZ);
        Serial.print(", ");
        Serial.print(latitude);
        Serial.print(", ");
        Serial.print(longitude);
        Serial.print(", ");
        Serial.println(speed);
      }

  }

  //web button control
  // if((ul5mS % 250) == 0) //read and clear buttons
  // { 
  //   ucWorkingButtonState = 9;
  //   loopWEBServerButtonresponce();
  //   if(btIsWEB)
  //   {
  //     if(ucWorkingButtonState != 9)
  //     {
  //       btH_Enabled = 0;
  //       btEnabled = 1;
  //     }
  //     else
  //     {
  //       btH_Enabled = 1;
  //       btEnabled = 0;
  //     }
  //   }
     
  // }


   //Send CAN Message
  
  if((ul5mS % 20) == 0) //tx universal CAN heartbeat every 20mS
  {
    if(btH_Enabled){
    
      tx_msg.extd = 1;
      tx_msg.identifier = 0x000502C0;
      tx_msg.data_length_code = 1; 
      tx_msg.data[0] = 1;

      twai_transmit(&tx_msg, pdMS_TO_TICKS(3));    
   // Serial.printf("data: 0x%02X\n",tx_frame.data.u8[4]);
    } 
  }
 
  if((ul5mS % 10) == 0) //tx  CAN heartbeat run every 10mS
  {
    if(btEnabled){
    
      tx_msg.extd = 1;
      tx_msg.identifier = 0x02052C80;  // 0x02 0x05 0x0B 0x02 0x00  data 0x80 6th byte
      tx_msg.data_length_code = 8;
      
       
      tx_msg.data[0] = 0x7E;
      tx_msg.data[1] = 0x00;
      tx_msg.data[2] = 0x00;
      tx_msg.data[3] = 0x00;
      tx_msg.data[4] = 0x00;
      tx_msg.data[5] = 0x00;
      tx_msg.data[6] = 0x80;
      tx_msg.data[7] = 0x00;
      
      twai_transmit(&tx_msg, pdMS_TO_TICKS(3));
 
   // Serial.printf("data: 0x%02X\n",tx_frame.data.u8[4]);
    } 
  }

  if((ul5mS % 30) == 0) //tx motor speeds 30mS
  {
    
    if(btEnabled)
    {
      
  //motor 1
       tx_msg.extd = 1;
       tx_msg.identifier = 0x02050081;    
       tx_msg.data_length_code = 8;
              
       tx_msg.data[0] = bMotorSpeed[0][0];
       tx_msg.data[1] = bMotorSpeed[0][1];
       tx_msg.data[2] = bMotorSpeed[0][2];
       tx_msg.data[3] = bMotorSpeed[0][3];
       tx_msg.data[4] = 0x00;
       tx_msg.data[5] = 0x00;
       tx_msg.data[6] = 0x00;
       tx_msg.data[7] = 0x00;
        
      twai_transmit(&tx_msg, pdMS_TO_TICKS(3));
//Motor 2
       tx_msg.extd = 1;
       tx_msg.identifier = 0x02050082;  // motor 2
       tx_msg.data_length_code = 8;
              
       tx_msg.data[0] = bMotorSpeed[1][0];
       tx_msg.data[1] = bMotorSpeed[1][1];
       tx_msg.data[2] = bMotorSpeed[1][2];
       tx_msg.data[3] = bMotorSpeed[1][3];
       tx_msg.data[4] = 0x00;
       tx_msg.data[5] = 0x00;
       tx_msg.data[6] = 0x00;
       tx_msg.data[7] = 0x00;
        
      twai_transmit(&tx_msg, pdMS_TO_TICKS(3));

//motor 3
       tx_msg.extd = 1;
       tx_msg.identifier = 0x02050083;  //motor 3
       tx_msg.data_length_code = 8;
              
       tx_msg.data[0] = bMotorSpeed[2][0];
       tx_msg.data[1] = bMotorSpeed[2][1];
       tx_msg.data[2] = bMotorSpeed[2][2];
       tx_msg.data[3] = bMotorSpeed[2][3];
       tx_msg.data[4] = 0x00;
       tx_msg.data[5] = 0x00;
       tx_msg.data[6] = 0x00;
       tx_msg.data[7] = 0x00;
        
      twai_transmit(&tx_msg, pdMS_TO_TICKS(3));

//motor 4
       tx_msg.extd = 1;
       tx_msg.identifier = 0x02050084;  //motor 4
       tx_msg.data_length_code = 8;
              
       tx_msg.data[0] = bMotorSpeed[3][0];
       tx_msg.data[1] = bMotorSpeed[3][1];
       tx_msg.data[2] = bMotorSpeed[3][2];
       tx_msg.data[3] = bMotorSpeed[3][3];
       tx_msg.data[4] = 0x00;
       tx_msg.data[5] = 0x00;
       tx_msg.data[6] = 0x00;
       tx_msg.data[7] = 0x00;
        
      twai_transmit(&tx_msg, pdMS_TO_TICKS(3));

//motor 5
       tx_msg.extd = 1;
       tx_msg.identifier = 0x02050085; //motor 5
       tx_msg.data_length_code = 8;
              
       tx_msg.data[0] = bMotorSpeed[4][0];
       tx_msg.data[1] = bMotorSpeed[4][1];
       tx_msg.data[2] = bMotorSpeed[4][2];
       tx_msg.data[3] = bMotorSpeed[4][3];
       tx_msg.data[4] = 0x00;
       tx_msg.data[5] = 0x00;
       tx_msg.data[6] = 0x00;
       tx_msg.data[7] = 0x00;
        
      twai_transmit(&tx_msg, pdMS_TO_TICKS(3));

//motor 6
       tx_msg.extd = 1;
       tx_msg.identifier = 0x02050086;  //motor 6
       tx_msg.data_length_code = 8;
              
       tx_msg.data[0] = bMotorSpeed[5][0];
       tx_msg.data[1] = bMotorSpeed[5][1];
       tx_msg.data[2] = bMotorSpeed[5][2];
       tx_msg.data[3] = bMotorSpeed[5][3];
       tx_msg.data[4] = 0x00;
       tx_msg.data[5] = 0x00;
       tx_msg.data[6] = 0x00;
       tx_msg.data[7] = 0x00;
        
      twai_transmit(&tx_msg, pdMS_TO_TICKS(3));

    }
  }


  if ((ul5mS % 500) == 0) {

    if(btEnabled){
      tx_msg.extd = 1;
      tx_msg.identifier = 0x0C0C1801;  // CAN ID for joystick inputs
      tx_msg.data_length_code = 6;     // Data Length Code is 6

      // Set joystick data explicitly
      tx_msg.data[0] = joyStickInputs[0];
      tx_msg.data[1] = joyStickInputs[1];
      tx_msg.data[2] = joyStickInputs[2];
      tx_msg.data[3] = joyStickInputs[3];
      tx_msg.data[4] = joyStickInputs[4];
      tx_msg.data[5] = joyStickInputs[5];

      twai_transmit(&tx_msg, pdMS_TO_TICKS(3));

      //if (rx_msg.identifier = 0x01234567) //Current 1 Reading.
      //uint8_t *p = (uint8_t *)&Lwheel1C;
      //p[0] = rx_msg.data[0];
      //p[1] = rx_msg.data[1];
      //p[2] = rx_msg.data[2];
      //p[3] = rx_msg.data[3];
      //}
    }
  }
}

