#include <Arduino.h>
#include <ESP32CAN.h>
#include <CAN_config.h>
#include "MyWEBserver.h"


struct [[gnu::packed]] RobotState {
  uint64_t matchTimeSeconds : 8;
  uint64_t matchNumber : 10;
  uint64_t replayNumber : 6;
  uint64_t redAlliance : 1;
  uint64_t enabled : 1;
  uint64_t autonomous : 1;
  uint64_t testMode : 1;
  uint64_t systemWatchdog : 1;
  uint64_t tournamentType : 3;
  uint64_t timeOfDay_yr : 6;
  uint64_t timeOfDay_month : 4;
  uint64_t timeOfDay_day : 5;
  uint64_t timeOfDay_sec : 6;
  uint64_t timeOfDay_min : 6;
  uint64_t timeOfDay_hr : 5;
};

RobotState strucRobotState; 
CAN_device_t CAN_cfg;               // CAN Config
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

unsigned char ubDeviceConnected[20];
int iMotorCurrent;
float fMotorSpeed[20];
float fMotorCurrent[20];

unsigned char ubDeviceIndex;
unsigned char ubDeviceCount;

extern String CAN_message;



CAN_frame_t tx_frame;
CAN_frame_t rx_frame;
char bMotorSpeed[6][4];

unsigned long ulCurrentMicros;
unsigned long ulPreviousMicros;
unsigned long ul5mS = 0;

void setup() {
  Serial.begin(921600);
  Serial.println("ESP32-CAN");
  CAN_cfg.speed = CAN_SPEED_1000KBPS;
  CAN_cfg.tx_pin_id = GPIO_NUM_21;
  CAN_cfg.rx_pin_id = GPIO_NUM_22;
  CAN_cfg.rx_queue = xQueueCreate(rx_queue_size, sizeof(CAN_frame_t));
  // Init CAN Module
  
  strucRobotState.timeOfDay_hr = 0;
  strucRobotState.timeOfDay_min = 0;
  strucRobotState.timeOfDay_sec = 0;
  strucRobotState.timeOfDay_day = 0;
  strucRobotState.timeOfDay_month = 0;
  strucRobotState.timeOfDay_yr = 0;
  strucRobotState.tournamentType = 0;
  strucRobotState.systemWatchdog = 0;
  strucRobotState.testMode = 0;
  strucRobotState.autonomous = 0;
  strucRobotState.enabled = 0;
  strucRobotState.redAlliance = 0;
  strucRobotState.replayNumber = 0;
  strucRobotState.matchNumber = 0;
  strucRobotState.matchTimeSeconds = 0;
  for(int MotorID = 0; MotorID < 6;MotorID++)
  {
    bMotorSpeed[MotorID][0] = 0;
    bMotorSpeed[MotorID][1] = 0;
    bMotorSpeed[MotorID][2] = 0;
    bMotorSpeed[MotorID][3] = 0;
  }
  ESP32Can.CANInit();
  
  setupWEbServer();
  
}


void loop() {
    

  //Serial.println(CAN_message);
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
     case 's':
     {
       tx_frame.FIR.B.FF = CAN_frame_ext;
       tx_frame.MsgID = 0x00000000;
       tx_frame.FIR.B.DLC = 0;
    
       if (ESP32Can.CANWriteFrame(&tx_frame) == -1)
       {
         CANBUS_Loss_Error = 1;
       }   
       cIncomingByte = 0;
      break;
     }
     case 'h':
     case 'H':
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
     case 'e':
     {
       btEnabled = 1;
       btH_Enabled = 0;
       //strucRobotState.enabled = 1;
      //bTEstByte = 0x12;
       cIncomingByte = 0;
       break;
     }
     case '1':
     {
       ubDeviceNumberToPrint = 1;
       cIncomingByte = 0;
       break;
     }
     case '2':
     {
       ubDeviceNumberToPrint = 2;
       cIncomingByte = 0;
       break;
     }
     case '3':
     {
       ubDeviceNumberToPrint = 
       31;
       cIncomingByte = 0;
       break;
     }
     case '4':
     {
       ubDeviceNumberToPrint = 4;
       cIncomingByte = 0;
       break;
     }
     case '5':
     {
       ubDeviceNumberToPrint = 5;
       cIncomingByte = 0;
       break;
     }
     case '6':
     {
       ubDeviceNumberToPrint = 6;
       cIncomingByte = 0;
       break;
     }
     case 'a':
     case 'A':
     {
       ubDeviceNumberToPrint = 99;
       cIncomingByte = 0;
       break;
     }
     case 'P':
     case 'p':
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
     case 'd':
     {
       //strucRobotState.enabled = 0;
       btEnabled = 0;
       btH_Enabled = 1;
       cIncomingByte = 0;
       break;
     } 
     case 'G':
     case 'g':
     {
       
      cIncomingByte = 0;
       break;
     } 

    }

  }


  // Receive next CAN frame from queue
  if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS) == pdTRUE) {
   // Serial.printf("frame           :%x\n",rx_frame.MsgID);
   ubDeviceType = (unsigned char)((rx_frame.MsgID &   0B00011111000000000000000000000000)>>24); //5-bit value
   ubManufacturer = (unsigned char)((rx_frame.MsgID & 0B00000000111111110000000000000000)>>16); //8-bit value
   ubAPIClass = (unsigned char)((rx_frame.MsgID &     0B00000000000000001111110000000000)>>10); //6-bit 
   ubAPIIndex = (unsigned char)((rx_frame.MsgID &     0B00000000000000000000001111000000)>>6);  //4-bit 
   ubDeviceNumber = (unsigned char)(rx_frame.MsgID &  0B00000000000000000000000000111111);  //6-bit

   if(btPrintCanRx) 
   {
    if((ubDeviceNumber == ubDeviceNumberToPrint) || (ubDeviceNumberToPrint == 99))
    {
      printf("%i\t0x%08X\t0x%02X\t0x%02X\t0x%02X\t0x%02X\t0x%02X\tDLC %d\t",micros(), rx_frame.MsgID,ubDeviceType,ubManufacturer,ubAPIClass,ubAPIIndex,ubDeviceNumber, rx_frame.FIR.B.DLC);
      for (int i = 0; i < rx_frame.FIR.B.DLC; i++)
      {
        printf("0x%02X\t", rx_frame.data.u8[i]);
      }
      printf("\n");
    }
   }
   
   if((ubDeviceType == 0x02) && (ubAPIClass == 0x06) && (ubAPIIndex == 0x01))  //assuming ther eare 6 motor drivers addressed 1 to 6
   {
     ubDeviceConnected[ubDeviceNumber-1] = ubDeviceNumber;
     fMotorSpeed[ubDeviceNumber-1] = (float)((tx_frame.data.u8[3]<<24) & (tx_frame.data.u8[2]<<16) & (tx_frame.data.u8[1] <<8) & (tx_frame.data.u8[0]));
     iMotorCurrent = (tx_frame.data.u8[6] <<8) & (tx_frame.data.u8[7]);
     fMotorCurrent[ubDeviceNumber-1] = (iMotorCurrent / 4095) * 80;
     
     CAN_message = "";
     for(ubDeviceIndex = 0;ubDeviceIndex <= 14;ubDeviceIndex++)
      {
        if((ubDeviceConnected[ubDeviceIndex] == 0) || (ubDeviceConnected[ubDeviceIndex] > 6))
        {
          CAN_message += "0;0;0;";
        }
        else
        {
          CAN_message += String(ubDeviceConnected[ubDeviceIndex]) + ";" + String(fMotorSpeed[ubDeviceIndex]) + ";" + String(fMotorCurrent[ubDeviceIndex])+ ";";
        }
      }
      CAN_message += "END";
     
   }
  }

  //web button control
  if((ul5mS % 250) == 0) //read and clear buttons
  { 
    ucWorkingButtonState = 9;
    loopWEBServerButtonresponce();
    if(ucWorkingButtonState != 9)
    {
       btH_Enabled = 0;
       btEnabled = 1;
    }
    else
    {
       btH_Enabled = 1;
       btEnabled = 0;
    }
     
  }


   //Send CAN Message
  
  if((ul5mS % 20) == 0) //tx universal CAN heartbeat every 20mS
  {
    if(btH_Enabled)
    {
      tx_frame.FIR.B.FF = CAN_frame_ext;
      tx_frame.MsgID = 0x000502C0;
      tx_frame.FIR.B.DLC = 1;
      
        // Copy the RobotState structure to the CAN frame data array
      //memcpy(tx_frame.data.u8, &strucRobotState, sizeof(RobotState));
      
      tx_frame.data.u8[1] = 1;
      
      ESP32Can.CANWriteFrame(&tx_frame);
    
   // Serial.printf("data: 0x%02X\n",tx_frame.data.u8[4]);
    } 
  }

  if((ul5mS % 10) == 0) //tx  CAN heartbeat run every 10mS
  {
    if(btEnabled)
    {
      tx_frame.FIR.B.FF = CAN_frame_ext;
      tx_frame.MsgID = 0x02052C80;  // 0x02 0x05 0x0B 0x02 0x00  data 0x80 6th byte
      tx_frame.FIR.B.DLC = 8;
      
       
      tx_frame.data.u8[0] = 0x7E;
      tx_frame.data.u8[1] = 0x00;
      tx_frame.data.u8[2] = 0x00;
      tx_frame.data.u8[3] = 0x00;
      tx_frame.data.u8[4] = 0x00;
      tx_frame.data.u8[5] = 0x00;
      tx_frame.data.u8[6] = 0x80;
      tx_frame.data.u8[7] = 0x00;
      
      ESP32Can.CANWriteFrame(&tx_frame);
    
   // Serial.printf("data: 0x%02X\n",tx_frame.data.u8[4]);
    } 
  }

  if((ul5mS % 30) == 0) //tx universal CAN heartbeat every 20mS
  {
    //iMotorSpeed = 0x3DCCCCCD;
    if(btEnabled)//strucRobotState.enabled)
    {
      
      //motor 1
       tx_frame.MsgID = 0x02050081;
       tx_frame.FIR.B.DLC = 8;
         // Copy the RobotState structure to the CAN frame data array
      // memcpy(tx_frame.data.u8, &iMotorSpeed55, 4);
       //Serial.printf("tx_frame.data.u8: 0x%02x,0x%02x,0x%02x,0x%02x\n",tx_frame.data.u8[0],tx_frame.data.u8[1],tx_frame.data.u8[2],tx_frame.data.u8[3]);
       tx_frame.data.u8[0] = bMotorSpeed[0][0];
       tx_frame.data.u8[1] = bMotorSpeed[0][1];
       tx_frame.data.u8[2] = bMotorSpeed[0][2];
       if((ucWorkingButtonState == 1) ||(ucWorkingButtonState == 2)) ///forward and Left
       {
        tx_frame.data.u8[3] = bMotorSpeed[0][3] & 0x80;
       }
       else
       {
        tx_frame.data.u8[3] = bMotorSpeed[0][3];
       }
       

       tx_frame.data.u8[4] = 0x00;
       tx_frame.data.u8[5] = 0x00;
       tx_frame.data.u8[6] = 0x00;
       tx_frame.data.u8[7] = 0x00;
      
  
      ESP32Can.CANWriteFrame(&tx_frame);

        tx_frame.MsgID = 0x02050082;  // motor 2
       tx_frame.FIR.B.DLC = 8;
         // Copy the RobotState structure to the CAN frame data array
      // memcpy(tx_frame.data.u8, &iMotorSpeed55, 4);
       //Serial.printf("tx_frame.data.u8: 0x%02x,0x%02x,0x%02x,0x%02x\n",tx_frame.data.u8[0],tx_frame.data.u8[1],tx_frame.data.u8[2],tx_frame.data.u8[3]);
       tx_frame.data.u8[0] = bMotorSpeed[1][0];
       tx_frame.data.u8[1] = bMotorSpeed[1][1];
       tx_frame.data.u8[2] = bMotorSpeed[1][2];
       if((ucWorkingButtonState == 1) ||(ucWorkingButtonState == 2)) ///forward and Left
       {
        tx_frame.data.u8[3] = bMotorSpeed[1][3] & 0x80;
       }
       else
       {
        tx_frame.data.u8[3] = bMotorSpeed[1][3];
       }
       
       tx_frame.data.u8[4] = 0x00;
       tx_frame.data.u8[5] = 0x00;
       tx_frame.data.u8[6] = 0x00;
       tx_frame.data.u8[7] = 0x00;
      
  
      ESP32Can.CANWriteFrame(&tx_frame);

        tx_frame.MsgID = 0x02050083;  //motor 3
       tx_frame.FIR.B.DLC = 8;
         // Copy the RobotState structure to the CAN frame data array
      // memcpy(tx_frame.data.u8, &iMotorSpeed55, 4);
       //Serial.printf("tx_frame.data.u8: 0x%02x,0x%02x,0x%02x,0x%02x\n",tx_frame.data.u8[0],tx_frame.data.u8[1],tx_frame.data.u8[2],tx_frame.data.u8[3]);
       tx_frame.data.u8[0] = bMotorSpeed[2][0];
       tx_frame.data.u8[1] = bMotorSpeed[2][1];
       tx_frame.data.u8[2] = bMotorSpeed[2][2];
       if((ucWorkingButtonState == 1) ||(ucWorkingButtonState == 2)) ///forward and Left
       {
        tx_frame.data.u8[3] = bMotorSpeed[2][3] & 0x80;
       }
       else
       {
        tx_frame.data.u8[3] = bMotorSpeed[2][3];
       }
       
       tx_frame.data.u8[4] = 0x00;
       tx_frame.data.u8[5] = 0x00;
       tx_frame.data.u8[6] = 0x00;
       tx_frame.data.u8[7] = 0x00;
      
  
      ESP32Can.CANWriteFrame(&tx_frame);

      //Serial.println(tx_frame.data.u8[4tx_frame.FIR.B.FF = CAN_frame_ext;
       tx_frame.MsgID = 0x02050084;  //motor 4
       tx_frame.FIR.B.DLC = 8;
         // Copy the RobotState structure to the CAN frame data array
      // memcpy(tx_frame.data.u8, &iMotorSpeed55, 4);
       //Serial.printf("tx_frame.data.u8: 0x%02x,0x%02x,0x%02x,0x%02x\n",tx_frame.data.u8[0],tx_frame.data.u8[1],tx_frame.data.u8[2],tx_frame.data.u8[3]);
       tx_frame.data.u8[0] = bMotorSpeed[3][0];
       tx_frame.data.u8[1] = bMotorSpeed[3][1];
       tx_frame.data.u8[2] = bMotorSpeed[3][2];
       if((ucWorkingButtonState == 1) ||(ucWorkingButtonState == 2)) ///forward and Left
       {
        tx_frame.data.u8[3] = bMotorSpeed[3][3] & 0x80;
       }
       else
       {
        tx_frame.data.u8[3] = bMotorSpeed[3][3];
       }
       
       tx_frame.data.u8[4] = 0x00;
       tx_frame.data.u8[5] = 0x00;
       tx_frame.data.u8[6] = 0x00;
       tx_frame.data.u8[7] = 0x00;
      
  
      ESP32Can.CANWriteFrame(&tx_frame);

      //Serial.println(tx_frame.data.u8[4tx_frame.FIR.B.FF = CAN_frame_ext;
       tx_frame.MsgID = 0x02050085; //motor 5
       tx_frame.FIR.B.DLC = 8;
         // Copy the RobotState structure to the CAN frame data array
      // memcpy(tx_frame.data.u8, &iMotorSpeed55, 4);
       //Serial.printf("tx_frame.data.u8: 0x%02x,0x%02x,0x%02x,0x%02x\n",tx_frame.data.u8[0],tx_frame.data.u8[1],tx_frame.data.u8[2],tx_frame.data.u8[3]);
       tx_frame.data.u8[0] = bMotorSpeed[4][0];
       tx_frame.data.u8[1] = bMotorSpeed[4][1];
       tx_frame.data.u8[2] = bMotorSpeed[4][2];
       if((ucWorkingButtonState == 1) ||(ucWorkingButtonState == 2)) ///forward and Left
       {
        tx_frame.data.u8[3] = bMotorSpeed[4][3] & 0x80;
       }
       else
       {
        tx_frame.data.u8[3] = bMotorSpeed[4][3];
       }
       
       tx_frame.data.u8[4] = 0x00;
       tx_frame.data.u8[5] = 0x00;
       tx_frame.data.u8[6] = 0x00;
       tx_frame.data.u8[7] = 0x00;
      
  
      ESP32Can.CANWriteFrame(&tx_frame);

      //Serial.println(tx_frame.data.u8[4tx_frame.FIR.B.FF = CAN_frame_ext;
       tx_frame.MsgID = 0x02050086;  //motor 6
       tx_frame.FIR.B.DLC = 8;
         // Copy the RobotState structure to the CAN frame data array
      // memcpy(tx_frame.data.u8, &iMotorSpeed55, 4);
       //Serial.printf("tx_frame.data.u8: 0x%02x,0x%02x,0x%02x,0x%02x\n",tx_frame.data.u8[0],tx_frame.data.u8[1],tx_frame.data.u8[2],tx_frame.data.u8[3]);
       tx_frame.data.u8[0] = bMotorSpeed[5][0];
       tx_frame.data.u8[1] = bMotorSpeed[5][1];
       tx_frame.data.u8[2] = bMotorSpeed[5][2];
       if((ucWorkingButtonState == 1) ||(ucWorkingButtonState == 2)) ///forward and Left
       {
        tx_frame.data.u8[3] = bMotorSpeed[5][3] & 0x80;
       }
       else
       {
        tx_frame.data.u8[3] = bMotorSpeed[5][3];
       }
       
       tx_frame.data.u8[4] = 0x00;
       tx_frame.data.u8[5] = 0x00;
       tx_frame.data.u8[6] = 0x00;
       tx_frame.data.u8[7] = 0x00;
      
  
      ESP32Can.CANWriteFrame(&tx_frame);





    }
  }

}
