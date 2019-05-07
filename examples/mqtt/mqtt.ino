#include "Quectel_LTE.h"

Quectel_MQTT LTE;

// const char *APN = "CMNET";
const char *APN = "UNINET";
const char *USERNAME = "";
const char *PASSWD = "";

/* MQTT parameters */
// const char *server = "iot.eclipse.org";
const char *server = "broker.hivemq.com";
const char *clientId = "c001";
const char *willTopic = "Heat";
const char *willMsg = "Quectecl ECxx";
const char *topic = "Heat";
uint8_t clientIdx = 0;

int16_t port = 1883;

uint8_t pubRetry = 5;

bool terminatedFlag = false;

void messageReceived(char* cb_topic, char* cb_msg) {
	Serial.print("[MQTT]incoming msg: ");
	Serial.print("Topic: "); 
  Serial.print(cb_topic);
	Serial.print(", Msg: "); 
  Serial.println(cb_msg);
}

void setup() { 
  delay(4000); 
  Serial.begin(115200);
  Serial.println("## TCP Client Demo");

  LTE.initialize();
  LTE.switchEcho(true);
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

  // Set/open/Connect
  if(!LTE.mqttSetWill(willTopic, willMsg, 1, 1, 1) || 
     !LTE.mqttSetServer(server) || 
     !LTE.mqttConnect(clientId)) 
  {
    Serial.println("### mqtt error");    
    goto terminate;
  }    
  
  // publication
  while(pubRetry--)
  {
    String msg = String(random(2000, 3000)*1.0/100.0) + " degree";
          
    if(LTE.mqttPublish(topic, msg.c_str())) {
      Serial.println("## published Topic: \"" + String(topic) + "\", Messagea: \"" + msg + "\"");	
    } else {
      Serial.println("### MQTT publish failed");
    }
    delay(1000);
  }

  // subscription
  LTE.mqttOnMessage(messageReceived);
  if(!LTE.mqttSubscribe(topic))
  {
    Serial.println("### ERROR:mqttSubscribe");
    goto terminate;
  }

  return;

terminate:
  // Terminate MQTT connection    
  if(LTE.mqttClose())
  {
    Serial.println("## Close");
  }
  else
  {
    Serial.println("### Failed to close MQTT");
  }
}

void loop() {
  LTE.mqttLoop();
}
