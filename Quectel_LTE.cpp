#include <stdio.h>

#include "Quectel_LTE.h"

Quectel_LTE::Quectel_LTE() : _AtSerial(&Serial1)
{
  _TxBuf[0] = 0;
  _RxBuf[0] = 0;
  _u32ip = 0;
  _str_ip[0] = 0;
  _usedSockId[0] = 0;
  _apn[0] = 0;
  _apnUser[0] = 0;
  _apnPasswd[0] = 0;
  _operator[0] = 0;
  _longitude = 0;
  _latitude = 0;        
  _ref_longitude = 22.584322;
  _ref_latitude = 113.966678;
  _str_longitude[0] = 0;
  _str_latitude[0] = 0;
  _North_or_South[0] = 0;
  _West_or_East[0] = 0;
}

void Quectel_LTE::initialize()
{
  _AtSerial._Serial->begin(115200);  
}

/**
 * @brief turn on or turn off echo mode.
 * @param on - true to turn on, false to turn off
 * @return - true for sucess, false for failure
*/
bool Quectel_LTE::switchEcho(bool on)
{
  on ? _AtSerial.WriteCommand("AT E1\r\n") : _AtSerial.WriteCommand("AT E0\r\n");
  return _AtSerial.WaitForResponse("OK", CMD, 500);
}

/** check if module is powered on or not
 *  @returns
 *      true on success
 *      false on error
 */
bool Quectel_LTE::isAlive(uint32_t timeout)
{
    Stopwatch sw;
    sw.Restart();

    while(sw.ElapsedMilliseconds() <= timeout)
    {
        if(_AtSerial.WriteCommandAndWaitForResponse(F("AT\r\n"), "OK", CMD, 500)){
            Logln("");
            return RET_OK;
        }
        Log(".");
    }    
    
    return false;
}

/** 
 * check SIM card status
 */
bool Quectel_LTE::checkSIMStatus(void)
{
    char Buffer[32];
    int count = 0;
    _AtSerial.CleanBuffer(Buffer,32);
    while(count < 3) {
        _AtSerial.WriteCommand("AT+CPIN?\r\n");
        _AtSerial.ReadResponseUntil_EOL(Buffer, 32, "\r\nOK",CMD, DEFAULT_TIMEOUT);
        if((NULL != strstr(Buffer,"+CPIN: READY"))) {
            break;
        }
        count++;
        delay(300);
    }
    if(count == 3) {
        return false;
    }
    return  RET_OK;
}

bool Quectel_LTE::waitForNetworkRegister(uint32_t timeout)
{
    Stopwatch sw;
    sw.Restart(); 
    
    // Check CS Registration
    while(true)
    {
        if(_AtSerial.WriteCommandAndWaitForResponse("AT+CEREG?\r\n", "+CEREG: 0,1", CMD, 500) || // Home network
        _AtSerial.WriteCommandAndWaitForResponse("AT+CEREG?\r\n", "+CEREG: 0,5", CMD, 500)) // Roaming
        {
            break;
        }        
        if (sw.ElapsedMilliseconds() >= (unsigned long)timeout) return RET_ERR;
        Log(".");
    }

    sw.Restart();
    // Check PS Registration
    while(true)
    {
        if(_AtSerial.WriteCommandAndWaitForResponse("AT+CGREG?\r\n", "+CGREG: 0,1", CMD, 500) || // Home network
        _AtSerial.WriteCommandAndWaitForResponse("AT+CGREG?\r\n", "+CGREG: 0,5", CMD, 500)) // Roaming
        {
            break;
        }        
        if (sw.ElapsedMilliseconds() >= (unsigned long)timeout) return RET_ERR;
        Log(".");
    }
  
  return RET_OK;
}

bool Quectel_LTE::Activate(const char* APN, const char* userName, const char* password, long waitForRegistTimeout)
{   
  _AtSerial.CleanBuffer(_TxBuf, sizeof(_TxBuf));
  sprintf(_TxBuf, "AT+QICSGP=1,1,\"%s\",\"%s\",\"%s\",1\r\n", APN, userName, password);
  if(!_AtSerial.WriteCommandAndWaitForResponse(_TxBuf, "OK", CMD, 5000))
  {
      return RET_ERR;
  }
  if(!waitForNetworkRegister(waitForRegistTimeout)) return RET_ERR;
  _AtSerial.WriteCommand("AT+QIACT=1\r\n");
  delay(200);

	return RET_OK;
}

bool Quectel_LTE::Deactivate()
{
    if (!_AtSerial.WriteCommandAndWaitForResponse("AT+QIDEACT=1\r\n", "OK", CMD, 500)) return RET_ERR;

	return RET_OK;
}

/**
 * Get IP address
 */
bool Quectel_LTE::getIPAddr(void)
{
    // AT+QIACT?
    // +QIACT: 1,1,1,"10.72.134.66"

    char *p;
    uint8_t i = 0;

    // Get IP address
    _AtSerial.CleanBuffer(_RxBuf, sizeof(_RxBuf));
    _AtSerial.WriteCommand("AT+QIACT?\r\n");
    if(!_AtSerial.ReadResponseUntil_EOL(_RxBuf, sizeof(_RxBuf), "+QIACT:", CMD, 500))
    {
      return RET_ERR;
    }
    
    if(NULL != (p = strstr(_RxBuf, "+QIACT:")))
    {       
        p = strtok(_RxBuf, ",");  // +QIACT: 1,1,1,"10.72.134.66"
        p = strtok(NULL, ",");  // 1,1,"10.72.134.66"
        p = strtok(NULL, ",");  // 1,"10.72.134.66"
        p = strtok(NULL, ",");  // "10.72.134.66"
        p += 1;
    }
    else
    {
        Log_error("AT+QIACT?");
        return RET_ERR;
    }

    memset(_str_ip, '\0',sizeof(_str_ip));
    while((*(p+i) != '\"') && (*(p+i) != '\0'))
    { 
      _str_ip[i] = *(p+i);
      i++;
    }
    _u32ip = str_to_u32ip(_str_ip);

    return RET_OK;
}

/**
 * Get operator name
*/
bool Quectel_LTE::getOperator()
{
    // AT+COPS?
    // +COPS: 0,0,"CHN-UNICOM",7

    char *p;

    _AtSerial.CleanBuffer(_RxBuf, sizeof(_RxBuf));
    _AtSerial.WriteCommand("AT+COPS?\r\n");
    _AtSerial.ReadResponseUntil(_RxBuf, sizeof(_RxBuf), "OK", CMD, 5000);
    debugPrintLn(_RxBuf);  

    if(NULL != (p = strstr(_RxBuf, "+COPS:")))
    {
        if(1 == sscanf(p, "+COPS: %*d,%*d,\"%[^\"]\",%*d", _operator))
        {
            // debugPrint("Operator: ");
            // debugPrintLn(_operator);
        }
    }
    else
    {
        Log_error("+COPS failed");
        return RET_ERR;
    }

    return RET_OK;
}

/** 
 * Parse IP string
 * @return ip in hex
 */
uint32_t Quectel_LTE::str_to_u32ip(char* str)
{
    uint32_t ip = 0;
    char *p = (char*)str;
    
    for(int i = 0; i < 4; i++) {
        ip |= atoi(p);
        p = strchr(p, '.');
        if (p == NULL) {
            break;
        }
        if(i < 3) ip <<= 8;
        p++;
    }
    return ip;
}

/** get Signal Strength from SIM900 (see AT command: AT+CSQ) as integer
*  @param
*  @returns
*      true on success
*      false on error
*/
bool Quectel_LTE::getSignalStrength(int *buffer)
{
    //AT+CSQ                        --> 6 + CR = 10
    //+CSQ: <rssi>,<ber>            --> CRLF + 5 + CRLF = 9                     
    //OK                            --> CRLF + 2 + CRLF =  6

    byte i = 0;
    char Buffer[26];
    char *p, *s;
    char buffers[4];

    _AtSerial.FlushSerial();
    _AtSerial.CleanBuffer(Buffer, 26);
    _AtSerial.WriteCommand("AT+CSQ\r");    
    _AtSerial.ReadResponseUntil_EOL(Buffer, 26, "\r\nOK",CMD, 1000);
    if (NULL != (s = strstr(Buffer, "+CSQ:"))) {
        s = strstr((char *)(s), " ");
        s = s + 1;  //We are in the first phone number character 
        p = strstr((char *)(s), ","); //p is last character """
        if (NULL != s) {
            i = 0;
            while (s < p) {
                buffers[i++] = *(s++);
            }
            buffers[i] = '\0';
        }
        *buffer = atoi(buffers);
        return  RET_OK;
    }
    return false;
}

/**
 * Set host server by ip/domain and port, then connect to the server 
 * @param sockid is local socket id
 * @param host is remote server ip or domain
 * @param port is remote server port
 * @return  RET_OK for success, false for failure
*/
bool Quectel_LTE::sockOpen(const char *host, int port, Socket_type connectType )
{
  if(host == NULL || host[0] == '\0') return false;
  if(port < 0 || 65535 < port) return false;

  const char* typeStr;
  switch(connectType)
  {
    case TCP:
      typeStr = "TCP";
      break;
    case UDP:
      typeStr = "UDP";
      break;
    
    default: 
      return false;
  }

  _AtSerial.CleanBuffer(_RxBuf, sizeof(_RxBuf));  
  //TODO - if socketId 0 has been used, then deactive PDP then re-open socket
  _AtSerial.WriteCommand("AT+QISTATE?\r\n");
  _AtSerial.ReadResponseUntil_EOL((char*)_RxBuf, sizeof(_RxBuf), "\r\nOK", CMD, 1000);
  if(NULL != strstr(_RxBuf, "+QISTATE:")) return false;
  
  _AtSerial.CleanBuffer(_TxBuf, sizeof(_TxBuf));
  if(connectType == TCP) { sprintf(_TxBuf, "AT+QIOPEN=1,0,\"%s\",\"%s\",%d\r\n", typeStr, host, port); }
  else if(connectType == UDP) { sprintf(_TxBuf, "AT+QIOPEN=1,0,\"%s\",\"%s\",%d\r\n", typeStr, host, port); }
  else { return false; }   
  debugPrintLn(_TxBuf);
  
  uint8_t retry = 3;
  while(retry--)
  {
    if(!_AtSerial.WriteCommandAndWaitForResponse(_TxBuf, "+QIOPEN: 0,0", CMD, 10000, 1000)) //wait 5000ms for the connection result "+QIOPEN: 0,0"
    {
        debugPrintLn("ERROR:QIOPEN");
    }  
    else
    {
      break;
    }    
  }
  
  return  RET_OK;
}

/**
 * Close connected socket
 * 
 * @param sockid socket id will be closed
 * @return  RET_OK for success, false for failure.
*/

bool Quectel_LTE::sockClose(int sockid)
{
  // Only socketId 0 were used, so we need to close socketId 0
  _AtSerial.WriteCommand("AT+QICLOSE=0\r\n");
  _AtSerial.WriteCommand((uint8_t)sockid);
  _AtSerial.WriteCommand("\r\n");
  if(!_AtSerial.WaitForResponse("OK", CMD, 10000, 5000))
  {
    return false;
  }
  
  return  RET_OK;
}


bool Quectel_LTE::sockWrite(int sockid)
{
  if(sockid > 6) return false;
  char txBuf[32] = {'\0'};
  sprintf(txBuf, "AT+QISEND=%d\r\n", sockid);
  debugPrint(txBuf);
  
  if(!_AtSerial.WriteCommandAndWaitForResponse(txBuf, ">", CMD, 1000, 1000)) return false;
  
  return  RET_OK;
}

/**
 * Write data to opened socket
 * 
 * @param sockid socket id created before 
 * @param data data array will be sent to remote
 * @param dataSize
 * @return  RET_OK for success, false for failure.
*/
bool Quectel_LTE::sockWrite(uint8_t sockid, char *data, uint16_t dataSize)
{
  if(sockid > 6) return false;
  if(data == NULL || data[0] == '\0' || dataSize <= 0) return false;

  char txBuf[32] = {'\0'};

  sprintf(txBuf, "AT+QISEND=%d,%d\r\n", sockid, dataSize);
  if(!_AtSerial.WriteCommandAndWaitForResponse(txBuf, ">", CMD, 500)) return false;
  _AtSerial.WriteCommand(data, dataSize);
  if(!_AtSerial.WaitForResponse("SEND OK\r\n", CMD, 10000)) return false;
  // if(!_AtSerial.WaitForResponse("+QIURC: \"recv\",0", CMD, 10000)) return false; // Some server won't send back messages

  return  RET_OK;
}

bool Quectel_LTE::sockWrite(uint8_t sockid, char *data)
{
  return sockWrite(sockid, data, strlen(data));
}

uint16_t Quectel_LTE::sockReceive(uint8_t sockid, char *data, uint16_t dataSize, uint32_t timeoutMs)
{
  char *p;
  int rxSize = 0;  

  _AtSerial.CleanBuffer(_TxBuf, sizeof(_TxBuf));
  _AtSerial.CleanBuffer(_RxBuf, sizeof(_RxBuf));

  sprintf(_TxBuf, "AT+QIRD=%d,%d\r\n", sockid, dataSize);
  _AtSerial.WriteCommand(_TxBuf);  
  _AtSerial.ReadResponseUntil_EOL(_RxBuf, dataSize + 32, "\r\nOK", CMD, timeoutMs);  // 32 bytes is for URC info  
  
  // Debug print
  // Logln(_RxBuf);

  if(NULL == (p = strstr(_RxBuf, "+QIRD:"))) 
  {
    return 0;
  }
  
  p += 6;
  rxSize = atoi(p);
  if(rxSize < 0)
  {
    Logln("Can not parse +QIRD:");
    return 0;
  }
  
  //goto the next line to pass the bytes number
  while(true)
  {
    if(*p == '\r') 
    {
      p++;
      break;
    }
    if(*p == '\0') return 0;
    p++;
  }

  memcpy(data, p, rxSize);
  data[rxSize] = '\0';

  // Debug print
  // Log("\r\n>> rxSize: ");
  // Logln(rxSize);
  
  return rxSize; 
}

/**
 * @brief set url for HTTP(S) connect
 * @param url - use to set HTTP(S) url
*/
bool Quectel_LTE::httpSetUrl(const char *url, uint8_t timeoutSec)
{  
  if(url == NULL || strlen(url) < 1) return false;  
  //CHeck if url has https or HTTPS
  if (strncmp(url, "https:", 6) == 0) 
  {    
    Log_info("Config HTTPS");

    if (!_AtSerial.WriteCommandAndWaitForResponse("AT+QHTTPCFG=\"sslctxid\",1\r\n", "OK", CMD, 500)) return false;
    if (!_AtSerial.WriteCommandAndWaitForResponse("AT+QSSLCFG=\"sslversion\",1,4\r\n", "OK", CMD, 500)) return false;
    if (!_AtSerial.WriteCommandAndWaitForResponse("AT+QSSLCFG=\"ciphersuite\",1,0XFFFF\r\n", "OK", CMD, 500)) return false;
    if (!_AtSerial.WriteCommandAndWaitForResponse("AT+QSSLCFG=\"seclevel\",1,0\r\n", "OK", CMD, 500)) return false;
  }

  // if (!_AtSerial.WriteCommandAndWaitForResponse("AT+QHTTPCFG=\"requestheader\",0\r\n", "OK", CMD, 500)) return false;
  
  _AtSerial.CleanBuffer(_TxBuf, sizeof(_TxBuf));
  sprintf(_TxBuf, "AT+QHTTPURL=%d,%d\r\n", strlen(url), timeoutSec);
  // debug print
  debugPrintLn(_TxBuf);  
  if(!_AtSerial.WriteCommandAndWaitForResponse(_TxBuf, "CONNECT", CMD, 10000, 1000))
  {  
    Log_error("QHTTPURL");
    return false;
  }

  _AtSerial.WriteCommand(url);
  if(!_AtSerial.WaitForResponse("OK", CMD, 10000, 5000)) 
  {
    Log_error(url);
    return false;
  }

  return  RET_OK;
}



/**
 * @brief input url and try request HTTPGET method.
 * @param url - url.
 * @param data - data buffer to store content from request.
 * @param dataSize - data buffer size to store the content.
 * @param timeout - timeout for QHTTPGET QHTTPREAD.
 * @return contentSize - content size of the requested data.
*/
uint32_t Quectel_LTE::httpGet(const char *url, char *data, uint16_t dataSize, uint32_t timeout)
{    
  char rxBuf[64] = {'\0'};
  char *p;
  uint16_t contentSize = 0; 
  
  if(!httpSetUrl(url, 80)) return 0;
  delay(1000);

  _AtSerial.WriteCommand("AT+QHTTPGET=80\r\n");
  if(!_AtSerial.ReadResponseUntil_EOL((char *)rxBuf, sizeof(rxBuf), "+QHTTPGET:", CMD, timeout)) // wait for "+QHTTPGET:"
  {
    Log_error("QHTTPGET");
    return 0; 
  }

  if(NULL != (p = strstr(rxBuf, "+QHTTPGET:"))) 
  {
    // +QHTTPGET: 0,200,2263
    p = strtok(p, ",");
    p = strtok(NULL, ",");
    p = strtok(NULL, ",");
    if(NULL != p)
    {
      contentSize = atoi(p);      
    }

    if(contentSize == 0)
    {
      return 0;
    }

    _AtSerial.CleanBuffer(_RxBuf, sizeof(_RxBuf));
    _AtSerial.WriteCommand("AT+QHTTPREAD=80\r\n");
    if(!_AtSerial.ReadResponseUntil_EOL(_RxBuf, MAX_RXBUF_LEN, "+QHTTPREAD: 0", CMD, timeout)) // wait for "++QHTTPREAD:"
    {
      Log_error("QHTTPREAD");
      return 0; 
    }

    // Copy content
    p = strstr(_RxBuf, "CONNECT");
    p += strlen("CONNECT\r\n");
    if(dataSize < contentSize) memcpy(data, p, dataSize);   
    else memcpy(data, p, contentSize);   
  }
 
  return contentSize;
}


// uint32_t Quectel_LTE::httpPost(const char *url, char *data, uint16_t dataSize, uint32_t timeout = 60000)
// {
//   char *p;
//   uint16_t contentSize = 0; 
  
//   if(!httpSetUrl(url, 80)) return 0;
//   delay(1000);

//   _AtSerial.CleanBuffer(_TxBuf, 64);
//   _AtSerial.CleanBuffer(_RxBuf, 64);
//   sprintf(_TxBuf, "AT+QHTTPPOST=%d,80,80\r\n", dataSize);
//   if(!_AtSerial.WriteCommandAndWaitForResponse(_TxBuf, "CONNECT", CMD, 10000)) return 0;
//   _AtSerial.WriteCommand(data);
//   if(!_AtSerial.ReadResponseUntil_EOL(_RxBuf, sizeof(rxBuf), "OK\r\n\r\n+QHTTPPOST:", CMD, timeout))
//   {
//     Log_error("QHTTPPOST");
//     return 0; 
//   }

//   if(NULL != (p = strstr(rxBuf, "+QHTTPPOST:"))) 
//   {
//     // +QHTTPPOST: 0,200,2263
//     p = strtok(p, ",");
//     p = strtok(NULL, ",");
//     p = strtok(NULL, ",");
//     if(NULL != p)
//     {
//       contentSize = atoi(p);      
//     }

//     if(contentSize == 0)
//     {
//       return 0;
//     }

//     _AtSerial.CleanBuffer(_RxBuf, sizeof(_RxBuf));
//     _AtSerial.WriteCommand("AT+QHTTPREAD=80\r\n");
//     if(!_AtSerial.ReadResponseUntil_EOL(_RxBuf, MAX_RXBUF_LEN, "+QHTTPREAD: 0", CMD, timeout)) // wait for "++QHTTPREAD:"
//     {
//       Log_error("QHTTPREAD");
//       return 0; 
//     }

//     // Copy content
//     p = strstr(_RxBuf, "CONNECT");
//     p += strlen("CONNECT\r\n");
//     if(dataSize < contentSize) memcpy(data, p, dataSize);   
//     else memcpy(data, p, contentSize);   
//   }
 
//   return contentSize;
// }

bool Quectel_LTE::close_GNSS()
{
  int errCounts = 0;

  while(!_AtSerial.WriteCommandAndWaitForResponse("AT+QGNSSC?\n\r", "+QGNSSC: 0", CMD, 2000)) {
      errCounts ++;
      if(errCounts > 100){
        return false;
      }
      _AtSerial.WriteCommandAndWaitForResponse("AT+QGNSSC=0\n\r", "OK", CMD, 2000);
      delay(1000);
  }

  return  RET_OK;
}

/**
 * Aquire GPS sentence
 */
bool Quectel_LTE::dataFlowMode(void)
{
    // Make sure that "#define UART_DEBUG" is uncomment.
    _AtSerial.WriteCommand("AT+QGPSLOC?\r\n");
    return _AtSerial.WaitForResponse("OK", CMD, 2000, DEFAULT_INCHAR_TIMEOUT);
}

bool Quectel_LTE::open_GNSS(void)
{
  int errCounts = 0;

  while(!_AtSerial.WriteCommandAndWaitForResponse("AT+QGPS?\r\n", "+QGPS: 1", CMD, 2000)){
      errCounts ++;
      if(errCounts > 5){
        return false;
      }
      _AtSerial.WriteCommandAndWaitForResponse("AT+QGPS=1\r\n", "OK", CMD, 2000);
      delay(1000);
  }

  return  RET_OK;
}

/** 
 * Get coordinate infomation
 */
bool Quectel_LTE::getCoordinate(void)
{
  int tmp = 0;
  char *p = NULL;
  uint8_t str_len = 0;
  char buffer[128];

  _AtSerial.CleanBuffer(buffer, 128);
  _AtSerial.WriteCommand("AT+QGPSLOC?\r\n");
  if(!_AtSerial.ReadResponseUntil_EOL(buffer, 128, "\r\nOK", CMD, 2000)) return false; 
  // +QGPSLOC: 084757.700,2235.0272N,11357.9730E,1.6,40.0,3,171.43,0.0,0.0,290617,10    
  if(NULL == (p = strstr(buffer, "+QGPSLOC:"))) return false;  
  p += 10;      
  p = strtok(buffer, ","); // time
  p = strtok(NULL, ",");  // _latitude
  sprintf(_str_latitude, "%s", p);
  _latitude = strtod(p, NULL);
  tmp = (int)(_latitude / 100);
  _latitude = (double)(tmp + (_latitude - tmp*100)/60.0);

  // Get North and South status
  str_len = strlen(p);
  if ((*(p+str_len-1) != 'N') && (*(p+str_len-1) != 'S')){
    _North_or_South[0] = '0';
    _North_or_South[1] = '\0';
  } else {
    _North_or_South[0] = *(p+str_len-1);
    _North_or_South[1] = '\0';
  }

  p = strtok(NULL, ",");  // _longitude
  sprintf(_str_longitude, "%s", p);
  _longitude = strtod(p, NULL);

  // Get West and East status
  str_len = strlen(p);
  if ((*(p+str_len-1) != 'W') && (*(p+str_len-1) != 'E')){
    _West_or_East[0] = '0';        
    _West_or_East[1] = '\0';
  } else {
    _West_or_East[0] = *(p+str_len-1);
    _West_or_East[1] = '\0';
  }

  tmp = (int)(_longitude / 100);
  _longitude = (double)(tmp + (_longitude - tmp*100)/60.0);

  // if(_North_or_South[0] == 'S'){
  //     // _latitude = 0.0 - _latitude;
  // } else if(_North_or_South[0] = 'N'){
  //     _latitude = 0.0 - _latitude;
  // }

  // if(_West_or_East[0] == 'W'){
  //     // _longitude = 0.0 - _longitude;
  // } else if(_West_or_East[0] = 'E'){
  //     _longitude = 0.0 - _longitude;
  // }

  doubleToString(_longitude, _latitude);

  return  RET_OK;
}

/**
 * Convert double coordinate data to string
 */
void Quectel_LTE::doubleToString(double _longitude, double _latitude)
{
  int u8_lon = (int)_longitude;
  int u8_lat = (int)_latitude;
  uint32_t u32_lon = (_longitude - u8_lon)*1000000;
  uint32_t u32_lat = (_latitude - u8_lat)*1000000;

  sprintf(_str_longitude, "%d.%lu", u8_lon, u32_lon);
  sprintf(_str_latitude, "%d.%lu", u8_lat, u32_lat);
}

/**
 * Set outpu sentences in NMEA mode
*/
bool Quectel_LTE::enable_NMEA_mode()
{
  if(!_AtSerial.WriteCommandAndWaitForResponse("AT+QGPSCFG=\"nmeasrc\",1\r\n", "OK", CMD, 2000)){
        return false;
  }
  return  RET_OK;
}

/**
 * Disable NMEA mode
*/
bool Quectel_LTE::disable_NMEA_mode()
{
  if(!_AtSerial.WriteCommandAndWaitForResponse("AT+QGPSCFG=\"nmeasrc\",0\r\n", "OK", CMD, 2000)){
        return false;
  }
  return  RET_OK;
}

/**
 *  Request NMEA data and save the responce sentence
*/
bool Quectel_LTE::NMEA_read_and_save(const char *type, char *data)
{
  char rxBuf[192] = {'\0'};  
  char *p = NULL;
  uint16_t i = 0;

  _AtSerial.CleanBuffer(rxBuf, sizeof(rxBuf)); 
  _AtSerial.WriteCommand("AT+QGPSGNMEA=");
  _AtSerial.WriteCommand("\"");
  _AtSerial.WriteCommand(type);
  _AtSerial.WriteCommand("\"\r\n");
                                                                                                                                                                                          
  _AtSerial.ReadResponseUntil(rxBuf, sizeof(rxBuf), STR_OK, CMD, 1000);  // Save response data
  // Serial.print("##DEBUG _AtSerial.ReadResponseUntil: ");
  // Serial.println(rxBuf);
  if(NULL == (p = strstr(rxBuf, "+QGPSGNMEA:")))
  {
    return false;
  }
  p += 12;
  while((*(p) != '\n') && (*(p) != '\0')) {  // If receive "+QGPSGNMEA:", than keep saving the NMEA sentence 
    data[i++] = *(p++);
  }
  data[i] = '\0';
  // Serial.print("##DEBUG data: ");
  // Serial.println(data);
  return  RET_OK;

} 

/**
 * Read NMEA data
*/
bool Quectel_LTE::read_NMEA(NMEA_type type, char *save_buff)
{
  switch(type){
    case GGA:                
      NMEA_read_and_save("GGA", save_buff);
      break;
    case RMC:
      NMEA_read_and_save("RMC", save_buff);
      break;
    case GSV:
      // NMEA_read_and_save("GSV", save_buff); // Delete GSV aquirement, too much content to be saved, 
      break;
    case GSA:
      NMEA_read_and_save("GSA", save_buff);
      break;
    case VTG:
      NMEA_read_and_save("VTG", save_buff);
      break;
    case GNS:
      NMEA_read_and_save("GNS", save_buff);  // GNS sentence didn't show anything.
      break;    

    default:
      break;
  }

  return  RET_OK;
}

/**
 * Read NMEA GSV sentence
 * GSV sentence gonna be 6 lines, that's too much content to save as other NMEA data.
 * save_buff should be 512 Bytes size at least. 
*/
bool Quectel_LTE::read_NMEA_GSV(char *data)
{
  char rxBuf[512] = {'\0'};
  char *p;
  uint16_t i = 0;

  _AtSerial.CleanBuffer(rxBuf, sizeof(rxBuf));                                                                                                                                                                                                                  
  _AtSerial.WriteCommand("AT+QGPSGNMEA=\"GSV\"\r\n");  // Send command
  _AtSerial.ReadResponseUntil((char *)rxBuf, 512, STR_OK, CMD, 1000);  // Save response data 

  if(NULL == (p = strstr(rxBuf, "+QGPSGNMEA:")))
  {
    return false;
  }

  while(NULL != (p = strstr(p, "+QGPSGNMEA:")))
  {    
    p += strlen("+QGPSGNMEA:");       
    while((*(p) != '\n') && (*(p) != '\0')) {  // If receive "+QGPSGNMEA:", than keep saving the NMEA sentence 
      data[i++] = *(p++);
    }
    // memcpy((uint8_t *)data, p, strlen(p));
  }
  
  // Serial.print("##DEBUG save_buff: ");
  // Serial.println(save_buff);
  return  RET_OK;

} 

void Quectel_LTE::print(char data)
{
  _AtSerial.WriteCommand(data);
}

void Quectel_LTE::print(const char* cmd)
{
  _AtSerial.WriteCommand(cmd);
}

Quectel_MQTT::Quectel_MQTT() : _clientIdx(0)
{
}

/**
 * @brief set mqtt protocol ver
 * @param ver - select MQTTv3 or MQTTv4
 * @return - setting result, true for success, false for failure
*/
bool Quectel_MQTT::mqttSetVersion(uint8_t ver)
{

  _AtSerial.CleanBuffer(_TxBuf, sizeof(_TxBuf));
  sprintf(_TxBuf, "AT+QMTCFG=\"version\",%d,%d\r\n", _clientIdx, ver);
  if(!_AtSerial.WriteCommandAndWaitForResponse(_TxBuf, "OK", CMD, 500))
  {  
    Log_error(_TxBuf);
    return false;
  }
  
  return true;
}


bool Quectel_MQTT::mqttSetWill(const char *topic, const char *msg, uint8_t flag, uint8_t qos, uint8_t retain)
{
  _AtSerial.CleanBuffer(_TxBuf, sizeof(_TxBuf));
  sprintf(_TxBuf, "AT+QMTCFG=\"will\",%d,%d,%d,%d,\"%s\",\"%s\"\r\n", _clientIdx, flag, qos, retain, topic, msg);
  if(!_AtSerial.WriteCommandAndWaitForResponse(_TxBuf, "OK", CMD, 500))
  {  
    Log_error(_TxBuf);
    return false;
  }
  
  return true;
}

bool Quectel_MQTT::mqttSetWill(const char *topic, const char *msg)
{
  return mqttSetWill(topic, msg, 0, 0, 0);
}

bool Quectel_MQTT::mqttClearWill()
{
  _AtSerial.CleanBuffer(_TxBuf, sizeof(_TxBuf));
  sprintf(_TxBuf, "AT+QMTCFG=\"will\",%d,0,0,0,\"\",\"\"\r\n", _clientIdx);
  if(!_AtSerial.WriteCommandAndWaitForResponse(_TxBuf, "OK", CMD, 500))
  {  
    Log_error(_TxBuf);
    return false;
  }
  
  return true;
}

bool Quectel_MQTT::mqttSetServer(const char *server, uint16_t port)
{    
  char *p;
  int retCode = -1;

  _AtSerial.CleanBuffer(_TxBuf, sizeof(_TxBuf));
  sprintf(_TxBuf, "AT+QMTCFG=\"recv/mode\",%d,0,1\r\n", _clientIdx);
  // debug print
  debugPrintLn(_TxBuf);  
  if(!_AtSerial.WriteCommandAndWaitForResponse(_TxBuf, "OK", CMD, 500))
  {  
    Log_error("QMTCFG");
    return false;
  }
  
  _AtSerial.CleanBuffer(_TxBuf, sizeof(_TxBuf));
  sprintf(_TxBuf, "AT+QMTOPEN=%d,\"%s\",%d\r\n", _clientIdx, server, port);  // AT+QMTOPEN=0,"test.mosquitto.org",1883
  // debug print    
  debugPrintLn(_TxBuf);

  _AtSerial.CleanBuffer(_RxBuf, 64);
  _AtSerial.WriteCommand(_TxBuf);
  if(!_AtSerial.ReadResponseUntil_EOL(_RxBuf, 64, "OK\r\n\r\n+QMTOPEN:", CMD, 10000))
  {  
    Log_error("mqtt open");
    return false;
  }

  if(NULL == (p = strstr(_RxBuf, "+QMTOPEN:"))) return false;
  if(NULL == (p = strtok(p, ","))) return false;
  if(NULL == (p = strtok(p, ","))) return false;
  retCode = atoi(p);
  if(retCode != 0) return false;  // open mqtt server failed
  

  return true;

}

bool Quectel_MQTT::mqttSetServer(const char *server)
{    
  return mqttSetServer(server, 1883);
}

bool Quectel_MQTT::mqttClose(uint8_t clientId)
{
  int retCode = -2;
  char *p;

  _AtSerial.CleanBuffer(_TxBuf, sizeof(_TxBuf));
  sprintf(_TxBuf, "AT+QMTCLOSE=%d\r\n", clientId);
  // debug print
  debugPrintIn(_TxBuf);  

  // get return code
  _AtSerial.CleanBuffer(_RxBuf, 64);
  _AtSerial.WriteCommand(_TxBuf);
  if(!_AtSerial.ReadResponseUntil_EOL(_RxBuf, 64, "OK\r\n\r\n+QMTCLOSE:", CMD, 3000))
  {  
    Log_error("mqtt close");
    return false;
  }

  if(NULL == (p = strstr(_RxBuf, "+QMTCLOSE"))) return false;
  if(NULL == (p = strtok(p, ","))) return false;
  retCode = atoi(p);
  if(retCode != 0) return false;  // open mqtt server failed

  return true;
}

bool Quectel_MQTT::mqttSetTimeout(uint8_t pkt_timeout, uint8_t retry_times, uint8_t timeout_notice)
{
  _AtSerial.CleanBuffer(_TxBuf, sizeof(_TxBuf));
  sprintf(_TxBuf, "AT+QMTCFG=\"timeout\",%d,%d,%d,%d\r\n", _clientIdx, pkt_timeout, retry_times, timeout_notice);
  if(!_AtSerial.WriteCommandAndWaitForResponse(_TxBuf, "OK", CMD, 500))
  {  
    Log_error(_TxBuf);
    return false;
  }
  
  return true;
}

/**
 * @brief set clean session
 * @param - 0 The server must store the subscriptions of the client after it disconnects.
 *          1 The server must discard any previously maintained information about the client and treat the connection as “clean”.
 * @return - setting result true for success, false for failure.
*/
bool Quectel_MQTT::mqttClearSession(uint8_t clean)
{
  _AtSerial.CleanBuffer(_TxBuf, sizeof(_TxBuf));
  sprintf(_TxBuf, "AT+QMTCFG=\"session\",%d,%d\r\n", _clientIdx, clean);
  if(!_AtSerial.WriteCommandAndWaitForResponse(_TxBuf, "OK", CMD, 500))
  {  
    Log_error(_TxBuf);
    return false;
  }
  
  return true;
}

bool Quectel_MQTT::mqttSetKeepAlive(uint16_t seconds)
{
  _AtSerial.CleanBuffer(_TxBuf, sizeof(_TxBuf));
  sprintf(_TxBuf, "AT+QMTCFG=\"keepalive\",%d,%d\r\n", _clientIdx, seconds);
  if(!_AtSerial.WriteCommandAndWaitForResponse(_TxBuf, "OK", CMD, 500))
  {  
    Log_error(_TxBuf);
    return false;
  }
  
  return true;
}

bool Quectel_MQTT::mqttConnect(const char *clientId, const char *username, const char *passwd)
{ 
  char *p;
  int retCode = -1;

  _AtSerial.CleanBuffer(_TxBuf, sizeof(_TxBuf));
  if(username != NULL && passwd != NULL)
  {
    sprintf(_TxBuf, "AT+QMTCONN=%d,\"%s\",\"%s\",\"%s\"\r\n", _clientIdx, clientId, username, passwd);
  }
  else
  {
    sprintf(_TxBuf, "AT+QMTCONN=%d,\"%s\"\r\n", _clientIdx, clientId);
  }
  // debug print
  debugPrintIn(_TxBuf);

  // get return code
  _AtSerial.CleanBuffer(_RxBuf, 64);
  _AtSerial.WriteCommand(_TxBuf);
  if(!_AtSerial.ReadResponseUntil_EOL(_RxBuf, 64, "OK\r\n\r\n+QMTCONN:", CMD, 10000))
  {  
    Log_error(_RxBuf);
    return false;
  }
  debugPrintOut(_RxBuf);

  if(NULL == (p = strstr(_RxBuf, "+QMTCONN"))) return false;
  if(NULL == (p = strtok(_RxBuf, ","))) return false;
  if(NULL == (p = strtok(NULL, ","))) return false;
  retCode = atoi(p);
  if(retCode != 0) return false;  // open mqtt server failed

  return true;
}

bool Quectel_MQTT::mqttConnect(const char *clientId)
{
  return mqttConnect(clientId, NULL, NULL);
}


bool Quectel_MQTT::mqttDisconnect(uint8_t clientidx)
{
  char *p;
  int retCode = -1;

  _AtSerial.CleanBuffer(_TxBuf, 32);
  sprintf(_TxBuf, "AT+QMTDISC=%d\r\n", clientidx);
  // debug print
  debugPrintIn(_TxBuf);  

  // get return code
  _AtSerial.CleanBuffer(_RxBuf, 64);
  _AtSerial.WriteCommand(_TxBuf);
  if(!_AtSerial.ReadResponseUntil_EOL(_RxBuf, 64, "OK\r\n\r\n+QMTDISC:", CMD, 5000))
  {  
    Log_error(_TxBuf);
    return false;
  }
  debugPrintOut(_RxBuf);

  if(NULL == (p = strstr(_RxBuf, "+QMTPUBEX"))) return false;
  if(NULL == (p = strtok(p, ","))) return false;
  if(NULL == (p = strtok(NULL, ","))) return false;
  retCode = atoi(p);
  if(retCode != 0) return false;  // open mqtt server failed

  return true;
}

bool Quectel_MQTT::mqttDisconnect(void)
{
  return mqttDisconnect(_clientIdx);
}

bool Quectel_MQTT::mqttPublish(const char *topic, const char *msg, uint8_t qos, uint8_t retain)
{
  uint16_t len = strlen(msg);
  char *p;
  int retCode = -1;

  _AtSerial.CleanBuffer(_TxBuf, 64);
  sprintf(_TxBuf, "AT+QMTPUBEX=%d,%d,%d,%d,\"%s\",%d\r\n", _clientIdx, 0, qos, retain, topic, len);
  // debug print
  debugPrintIn(_TxBuf);  
  if(!_AtSerial.WriteCommandAndWaitForResponse(_TxBuf, ">", CMD, 5000))
  {  
    Log_error("QMTPUBEX");
    return false;
  }

  debugPrintIn(msg);  

  // get return code
  _AtSerial.CleanBuffer(_RxBuf, 64);
  _AtSerial.WriteCommand(msg);
  if(!_AtSerial.ReadResponseUntil_EOL(_RxBuf, 64, "OK\r\n\r\n+QMTPUBEX:", CMD, 5000))
  {  
    Log_error(_TxBuf);
    return false;
  }
  debugPrintOut(_RxBuf);

  if(NULL == (p = strstr(_RxBuf, "+QMTPUBEX"))) return false;
  if(NULL == (p = strtok(p, ","))) return false;
  if(NULL == (p = strtok(NULL, ","))) return false;
  retCode = atoi(p);
  if(retCode != 0) return false;  // open mqtt server failed

  return true;
}

bool Quectel_MQTT::mqttPublish(const char *topic, const char *msg)
{
  return mqttPublish(topic, msg, 0, 0);
}

bool Quectel_MQTT::mqttSubscribe(const char * topic, uint8_t qos)
{
  char *p;
  int result = -1;

  _AtSerial.CleanBuffer(_TxBuf, sizeof(_TxBuf));
  sprintf(_TxBuf, "AT+QMTSUB=%d,%d,\"%s\",%d\r\n", _clientIdx, 0, topic, qos);
  // debug print
  debugPrintIn(_TxBuf);  
  // get return code
  _AtSerial.CleanBuffer(_RxBuf, 64);
  _AtSerial.WriteCommand(_TxBuf);
  if(!_AtSerial.ReadResponseUntil_EOL(_RxBuf, 64, "OK\r\n\r\n+QMTSUB:", CMD, 5000))
  {  
    Log_error(_TxBuf);
    return false;
  }

  if(NULL == (p = strstr(_RxBuf, "+QMTSUB:"))) return false;
  if(NULL == (p = strtok(p, ","))) return false;
  if(NULL == (p = strtok(NULL, ","))) return false;
  if(NULL == (p = strtok(NULL, ","))) return false;
  result = atoi(p);
  if(result != 0) return false;  // open mqtt server failed

  return true;
}

bool Quectel_MQTT::mqttUnSubscribe(const char * topic)
{
  char *p;
  int result = -1;

  _AtSerial.CleanBuffer(_TxBuf, sizeof(_TxBuf));
  sprintf(_TxBuf, "AT+QMTUNS=%d,%d,\"%s\"\r\n", _clientIdx, 0, topic);
  // debug print
  debugPrintIn(_TxBuf);  
  // get return code
  _AtSerial.CleanBuffer(_RxBuf, 64);
  _AtSerial.WriteCommand(_TxBuf);
  if(!_AtSerial.ReadResponseUntil_EOL(_RxBuf, 64, "OK\r\n\r\n+QMTUNS:", CMD, 5000))
  {  
    Log_error(_TxBuf);
    return false;
  }

  if(NULL == (p = strstr(_RxBuf, "+QMTUNS:"))) return false;
  if(NULL == (p = strtok(p, ","))) return false;
  if(NULL == (p = strtok(NULL, ","))) return false;
  if(NULL == (p = strtok(NULL, ","))) return false;
  result = atoi(p);
  if(result != 0) return false;  // open mqtt server failed

  return true;
}

void Quectel_MQTT::mqttOnMessage(MQTTClientCallback cb)
{
  callback = cb;
}

void Quectel_MQTT::mqttLoop(void)
{
  // Is there a packet?
	char *p;

	if (_AtSerial.Available() > 0) 
	{
		char mqtt_packets[256] = {'\0'};		
		char topic[MQTT_MAX_PACKET_SIZE] = {'\0'};
		char msg[MQTT_MAX_PACKET_SIZE] = {'\0'};

		if(!_AtSerial.ReadResponseUntil_EOL(mqtt_packets, sizeof(mqtt_packets), "+QMTRECV:", CMD, 500)) return;

    if(NULL == (p = strstr(mqtt_packets, "+QMTRECV:"))) return;
    if(NULL == (p = strtok(p, ","))) return;
    if(NULL == (p = strtok(NULL, ","))) return;
    if(NULL == (p = strtok(NULL, ","))) return;
    if(strlen(p) < MQTT_MAX_PACKET_SIZE) memcpy(topic, p, strlen(p)); // Read topic
    else memcpy(topic, p, MQTT_MAX_PACKET_SIZE - 1); // Read topic
    if(NULL == (p = strtok(NULL, ","))) return;            
    if(strlen(p) < MQTT_MAX_PACKET_SIZE) memcpy(msg, p, strlen(p)); // Read payload
    else memcpy(msg, p, MQTT_MAX_PACKET_SIZE - 1); // Read payload

    if(NULL != callback)
    {
      callback((char*)topic, (char*)msg);
    }						
	}
}