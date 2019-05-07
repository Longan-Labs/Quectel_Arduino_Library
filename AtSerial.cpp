#include "AtSerial.h"

AtSerial::AtSerial(HardwareSerial* serial) : _Serial(serial)
{
	_Serial = serial;
}

bool AtSerial::Available()
{
	return _Serial->available();
}

bool AtSerial::WaitForAvailable(Stopwatch* sw, uint32_t timeout) const
{
	while(!_Serial->available())
	{
		if(timeout > 0 && sw != NULL && sw->ElapsedMilliseconds() >= timeout)
		{
			return false;
		}
	}
	return true;
}

void AtSerial::FlushSerial()
{
	while(_Serial->available() > 0){
			_Serial->read();
	}
}

// bool AtSerial::ReadLine(char *buffer, int count, uint16_t timeout)
// {
// 	uint16_t i = 0;

// 	Stopwatch sw;
// 	sw.Restart();
	
// 	while(1) {
// 			while (_Serial->available()) {
// 					char c = _Serial->read();
// 					buffer[i++] = c;
// 					if( (i >= count) || ('\n' == c) || ('\0' == c) ) break;
// 			}
// 			if(i >= count)break;
// 			if (sw.ElapsedMilliseconds() >= timeout) {
// 					break;
// 			}
// 	}
// 	return true;
// }

bool AtSerial::ReadResponseUntil_EOL(char *buffer, int count, const char *pattern, DataType type, uint16_t timeout)
{
	uint16_t i = 0;
	uint8_t sum = 0;
	uint8_t len = strlen(pattern);
	bool matched = false;
	bool result = false;
	char inchr;
	

	Stopwatch sw;
	sw.Restart();
	
	while(true) 
	{
			if(_Serial->available()) 
			{
					inchr = _Serial->read();
					debugPrint(inchr); // debug			
					buffer[i++] = inchr;
					if(!matched)
					{
						sum = (inchr == pattern[sum]) ? sum+1 : 0;
						if(sum == len) matched = true;
					} 
					else if(inchr == '\n') 
					{
						buffer[count - 1] = '\0';
						result = true;
						break;					
					}
			}
			if(i >= (count -1) ) 
			{
				buffer[count - 1] = '\0';
				break;
			}
			if (sw.ElapsedMilliseconds() >= timeout) 
			{
					break;
			}
	}
	//If is a CMD, we will finish to read buffer.
	if(type == CMD) FlushSerial();	

	
	return result;
}

bool AtSerial::ReadResponseUntil(char *buffer, int count, const char *pattern, DataType type, uint16_t timeout)
{
	uint16_t i = 0;
	uint8_t sum = 0;
	uint8_t len = strlen(pattern);
	bool result = false;
	char inchr;
	
	Stopwatch sw;
	sw.Restart();
	
	while(1) {
			if(_Serial->available()) {
					inchr = _Serial->read();
					debugPrint(inchr);  //debug
					buffer[i++] = inchr;
					sum = (inchr == pattern[sum]) ? sum+1 : 0;
					if(sum == len)
					{
						result = true;
						break;
					}
			}
			if(i >= (count -1) ) break;
			if (sw.ElapsedMilliseconds() >= timeout) {
					break;
			}
	}
	//If is a CMD, we will finish to read buffer.
	if(type == CMD) FlushSerial();
	buffer[count - 1] = '\0';

	return result;
}

// bool AtSerial::ReadResponse(char *buffer, uint16_t count, DataType type, uint32_t timeout, uint32_t inchar_timeout)
// {
// 	uint16_t i = 0;
	
// 	Stopwatch sw;
// 	Stopwatch sw_inchar;
// 	sw.Restart();

// 	while(1) {
// 			if(_Serial->available()) {
// 				sw_inchar.Restart();
// 				char c = _Serial->read();
// 				debugPrint(c);
// 				buffer[i++] = c;
// 				if(i >= count) break;
// 			}
// 			if(i >= count) break;
// 			if (sw.ElapsedMilliseconds() >= timeout) {					
// 					return 0;
// 			}

// 			//If inchar Timeout => break. So we can return sooner from this function.
// 			if (sw_inchar.ElapsedMilliseconds() >= inchar_timeout) {
// 					break;
// 			}
// 	}
// 	// buffer[count - 1] = '\0';
// 	//If is a CMD, we will finish to read buffer.
// 	if(type == CMD) FlushSerial();

// 	return (uint16_t)(i - 1);
// }

void  AtSerial::CleanBuffer(char* buffer, uint16_t count)
{
	for(int i=0; i < count; i++) {
			buffer[i] = '\0';
	}
}

void AtSerial::WriteCommand(char data)
{
	_Serial->write(data);
}	

void  AtSerial::WriteCommand(const char* cmd)
{
	for(uint16_t i=0; i<strlen(cmd); i++)
	{
			WriteCommand(cmd[i]);
	}	
}

void AtSerial::WriteCommand(char *data, uint16_t dataSize)
{
	for(uint16_t i = 0; i < dataSize; i++)
	{
		_Serial->write(data[i]);
	}	
}	

void  AtSerial::WriteCommand(const __FlashStringHelper* cmd)
{
	int i = 0;
  const char *ptr = (const char *) cmd;
  while (pgm_read_byte(ptr + i) != 0x00) 
	{
    WriteCommand(pgm_read_byte(ptr + i++));
  }
}

void AtSerial::WriteEndMark(void)
{
	WriteCommand((char)26);
}

bool AtSerial::WaitForResponse(const char* resp, DataType type, uint32_t timeout, uint32_t inchar_timeout)
{
	int len = strlen(resp);
	int sum = 0;
	
	Stopwatch sw;
	Stopwatch sw_inchar;

	sw.Restart();
	sw_inchar.Restart();

	while(1) {
			if(_Serial->available()) {
					sw_inchar.Restart();
					char c = _Serial->read();
					debugPrint(c);
					sum = (c==resp[sum]) ? sum+1 : 0;
					if(sum == len)break;
			}
			if (sw.ElapsedMilliseconds() >= timeout) {
					return false;
			}
			//If inchar Timeout => return FALSE. So we can return sooner from this function.
			if (sw_inchar.ElapsedMilliseconds() >= inchar_timeout) {
					return false;
			}
	}
	debugPrintLn();
	//If is a CMD, we will finish to read buffer.
	if(type == CMD) FlushSerial();

	return true;
}

bool AtSerial::WriteCommandAndWaitForResponse(const char* cmd, const char *resp, DataType type, uint32_t timeout, uint32_t inchar_timeout)
{
	WriteCommand(cmd);
  return WaitForResponse(resp, type, timeout, inchar_timeout);
}

bool AtSerial::WriteCommandAndWaitForResponse(const __FlashStringHelper* cmd, const char *resp, DataType type, uint32_t timeout, uint32_t inchar_timeout)
{
	WriteCommand(cmd);
  return WaitForResponse(resp, type, timeout, inchar_timeout);
}

void AtSerial::AT_Bypass(void)
{
	if(_Serial->available() > 0)
	{
		_debugSerial.write(_Serial->read());
	}
	if(_debugSerial.available() > 0)
	{
		_Serial->write(_debugSerial.read());
	}
}
