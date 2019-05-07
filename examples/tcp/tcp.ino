
#include "Quectel_LTE.h"

Quectel_LTE LTE;

// const char *APN = "CMNET";
const char *APN = "UNINET";
const char *USERNAME = "";
const char *PASSWD = "";
const char *HOST = "arduino.cc";

int port = 80;

void setup() { 
  delay(4000); 
  Serial.begin(115200);
  Serial.println("## TCP Client Demo");

  LTE.initialize();

  Serial.println("## Waitting for module to alvie...");
  if(!LTE.isAlive(20000)) 
  {
    Serial.println("## Module not alive");
    return;
  }

  Serial.println("## Wait for network ready");
  while(!LTE.waitForNetworkRegister())
  {
    Serial.println("## Network not ready");
    return;
  }

  if(!LTE.Deactivate())
  {
    Serial.println("### Failed to deactive network");
    return;
  }

  Serial.println("## Active network");
  if(!LTE.Activate(APN, USERNAME, PASSWD))
  {
    Serial.println("### Failed to active network");
    return;
  }

  if(!LTE.getIPAddr())
  {
    Serial.println("### ERROR:getIPAddr");
    return;
  }
  Serial.print("## IP:");
  Serial.println(LTE._str_ip);

  Serial.println("## Socket open");
  if(!LTE.sockOpen(HOST, port, TCP)) 
  {
    Serial.println("### Socket open error");
    return;
  }

  if(!LTE.sockWrite(0))
  {
    Serial.println("### Socket write error");
    return;
  }

  // write request headers
  LTE.print("GET /asciilogo.txt HTTP/1.1\n");
  LTE.print("Host: "); 
  LTE.print(HOST); 
  LTE.print('\n');
  LTE.print("Accept: */*\n");
  LTE.print("User-Agent: QUECTEL_MODULE\n");
  LTE.print("Connection: Keep-Alive\n");
  LTE.print("Content-Type: application/x-www-form-urlencoded\n\n");
  LTE.print(CTRL_Z);

  while(!LTE._AtSerial.WaitForResponse("SEND OK\r\n", CMD, 10000, 1000)) 
  {
    Serial.println("### ERROR:socketWrite");
    return;
  }
  delay(2000);

  char recvData[1024] = {'\0'};
  uint16_t recvSize = 0;
  Serial.println("## Socket receive");   
  Serial.println("********** request page ************");

  while(true)
  {
    LTE._AtSerial.CleanBuffer(recvData, sizeof(recvData));
    recvSize = LTE.sockReceive(0, recvData, 1000); 
    char *p = (char *)recvData;
    while((*p++) != '\0') Serial.write(*p);    
    if(1000 > recvSize)
    if(recvSize)
    {            
      break;
    } 
  }  
  Serial.println("\r\n**************** END ***************");

  Serial.println("## Socket close");
  if(!LTE.sockClose(0))
  {
    Serial.println("### Socket close error");
  }
}

void loop() {
  /* Debug */  
  LTE._AtSerial.AT_Bypass();
}
