#pragma once

#include <Arduino.h>

#include "Stopwatch.h"
#include "AtSerial.h"


#define MAX_TXBUF_LEN 200 
#define MAX_RXBUF_LEN 4095

#ifndef MQTT_MAX_PACKET_SIZE
#define MQTT_MAX_PACKET_SIZE 512
#endif

#define STR_AT	"AT"
#define STR_OK	"OK"
#define STR_ERR	"ERROR"

#define CTRL_Z '\x1A'
#define IP_FORMAT "%d.%d.%d.%d"

#define U32IP_TO_TUPLE(x) (uint8_t)(((x) >> 24) & 0xFF), \
                       (uint8_t)(((x) >> 16) & 0xFF), \
                       (uint8_t)(((x) >> 8) & 0xFF), \
                       (uint8_t)(((x) >> 0) & 0xFF)

#define TUPLE_TO_U32IP(a1, a2, a3, a4) ((((uint32_t)a1) << 24) | \
                                     (((uint32_t)a2) << 16) | \
                                      (((uint32_t)a3) << 8) | \
                                      (((uint32_t)a4) << 0))

enum Socket_type {
    CLOSED = 0,
    TCP    = 6,
    UDP    = 17
};

enum CheckState {
    RET_OK = true,
    RET_ERR = false,
};

typedef enum {
    GGA = 0,
    RMC,
    GSV,
    GSA,
    VTG,
    GNS
} NMEA_type;

enum mqtt_protocol_ver
{
    MQTTv3 = 3,
    MQTTv4 = 4,
};

typedef void (*MQTTClientCallback)(char* topic, char* payload);

class Quectel_LTE
{    
    public:
        char _TxBuf[MAX_TXBUF_LEN];
        char _RxBuf[MAX_RXBUF_LEN];
        uint32_t _u32ip;
		char _str_ip[20];
        bool _usedSockId[7];
        char _apn[32];
		char _apnUser[32];
		char _apnPasswd[32];
        char _operator[32];        

        AtSerial _AtSerial;

        Quectel_LTE();        
        void initialize();
        uint32_t str_to_u32ip(char* str);
        
        void print(char data);
		void print(const char* cmd);		

/************************** Common AT command **************************/        
        bool switchEcho(bool on);
        bool checkSIMStatus(void);
        bool isAlive(uint32_t timeout = 5000);
        bool getSignalStrength(int *buffer);        
/************************** Network startup **************************/                    
        bool waitForNetworkRegister(uint32_t timeout = 120000);
        bool Activate(const char* APN, const char* userName, const char* password, long waitForRegistTimeout = 120000);
        bool Deactivate();          
        bool getIPAddr(void);
		bool getOperator(void); 
        // bool ping(char * server);               
/************************** Socket TCP UDP **************************/
        bool sockOpen(const char *host, int port, Socket_type connectType);
        bool sockClose(int sockid);
        bool sockWrite(int sockid);
        bool sockWrite(uint8_t sockid, char *data, uint16_t dataSize);
        bool sockWrite(uint8_t sockid, char *data);        
        uint16_t sockReceive(uint8_t sockid, char *data, uint16_t dataSize, uint32_t timeoutMs=500);
/************************** HTTP(S) **************************/
        bool httpSetUrl(const char *url, uint8_t timeoutSec = 80);
        uint32_t httpGet(const char *url, char *data, uint16_t dataSize, uint32_t timeout = 60000);
        uint32_t httpPost(const char *url, char *data, uint16_t dataSize, uint32_t timeout = 60000);
/************************** MQTT **************************/        
        
/************************** GNSS **************************/
        bool gpsOn(void);
        bool gpsOff(void);
        bool gpsLocRawData(char *rawData, uint16_t dataSize);
};

class Quectel_MQTT : public Quectel_LTE
{
    public:
        uint16_t _keepAlive;
        char * _clientName;
        char * _password;
        char * _clientId;
        uint8_t _clientIdx;
                       
        Quectel_MQTT();
        bool mqttSetVersion(uint8_t ver = MQTTv3);
        bool mqttSetServer(const char * server, uint16_t port);
        bool mqttSetServer(const char * server);
        bool mqttClose(uint8_t clientIdx = 0);
        bool mqttSetTimeout(uint8_t pkt_timeout, uint8_t retry_times, uint8_t timeout_notice);
        bool mqttClearSession(uint8_t clean);
        bool mqttSetKeepAlive(uint16_t seconds);
        // will
        bool mqttSetWill(const char *topic, const char *msg, uint8_t flag, uint8_t qos, uint8_t retain);
        bool mqttSetWill(const char *topic, const char *msg);
        bool mqttClearWill();
        // MQTT control command
        bool mqttConnect(const char *client, const char *username, const char *passwd);
        bool mqttConnect(const char *client);        
        bool mqttDisconnect(uint8_t clientidx);
        bool mqttDisconnect(void);
        bool mqttPublish(const char *topic, const char *msg, uint8_t qos, uint8_t retain);
        bool mqttPublish(const char *topic, const char *msg);
        bool mqttSubscribe(const char * topic, uint8_t qos = 0);
        bool mqttUnSubscribe(const char * topic);
        void mqttOnMessage(MQTTClientCallback cb);        
        void mqttLoop(void);

    private:
        MQTTClientCallback callback;      
};

