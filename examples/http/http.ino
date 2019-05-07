
#include "Quectel_LTE.h"

Quectel_LTE LTE;

#define DATA_SIZE   300

// const char *APN = "CMNET";
const char *APN = "UNINET";
const char *USERNAME = "";
const char *PASSWD = "";
const char *url = "https://httpbin.org/get";


char recvData[DATA_SIZE];

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
  
  uint32_t size = LTE.httpGet(url, recvData, DATA_SIZE, 10000);     
  if(0 == size)
  {
    Serial.println("### ERROR:httpGet");
    return;
  }  
  Serial.print("\r\n**************** Get content ");
  Serial.print(size);
  Serial.println(" bytes *******************");
  for(uint32_t i = 0; recvData[i] != '\0';i++)
  {
    Serial.print(recvData[i]);
  } 
  Serial.println("\r\n**************** END ************************************");
}

void loop() {
  /* Debug */
  LTE._AtSerial.AT_Bypass(); 
}
