#pragma once

#include <Arduino.h>

#include "Stopwatch.h"
#include "board_config.h"

#define DEFAULT_TIMEOUT           (1000)   //miliseconds
#define DEFAULT_INCHAR_TIMEOUT    (500)   //miliseconds

#if defined (ARDUINO_ARCH_SAMD)
	#define _debugSerial Serial
#elif defined (ARDUINO_ARCH_AVR)
	#define _debugSerial Serial
#elif defined(ARDUINO_ARCH_SEEED_STM32F4)
	#define _debugSerial SerialUSB
#else
	#error "Architecture not matched"
#endif

#ifdef _debugSerial
	#define Log(...)     (_debugSerial.print(__VA_ARGS__))
	#define Logln(...)     (_debugSerial.println(__VA_ARGS__))
	#define Log_info(...)     (_debugSerial.print("[INFO]"), _debugSerial.println(__VA_ARGS__))
	#define Log_error(...)     (_debugSerial.print("[ERROR]"), _debugSerial.println(__VA_ARGS__))
	#define Log_prolog_in(...)       (_debugSerial.print("[IN]"), _debugSerial.println(__VA_ARGS__))
	#define Log_prolog_out(...)      (_debugSerial.print("[OUT]"), _debugSerial.println(__VA_ARGS__))
#else
	#define Log(...)
	#define Logln(...)
	#define Log_info(...)
	#define Log_error(...)
	#define Log_prolog_in(...)
	#define Log_prolog_out(...)
#endif

#if(UART_DEBUG == 1)
	#define writeProlog()			_debugSerial.print("[AT] ")	
	#define debugPrintLn(...) _debugSerial.println(__VA_ARGS__)
	#define debugPrint(...)  _debugSerial.print(__VA_ARGS__)
	#define debugPrintIn(...)     _debugSerial.print("[IN]"),_debugSerial.println(__VA_ARGS__)
	#define debugPrintOut(...)     _debugSerial.print("[OUT]"),_debugSerial.println(__VA_ARGS__)
#else
	#define writeProlog()
	#define debugPrintLn(...)
	#define debugPrint(...)
	#define debugPrintIn(...)
	#define debugPrintOut(...)
#endif

enum DataType {
	CMD     = 0,
	DATA    = 1,
};

class AtSerial
{
	public:
		

		HardwareSerial* _Serial;


		AtSerial(HardwareSerial* serial);
		bool Available();
		bool WaitForAvailable(Stopwatch* sw, uint32_t timeout) const;
		void FlushSerial();
		// bool ReadLine(char *buffer, int count, uint16_t timeout = DEFAULT_TIMEOUT);
		bool ReadResponseUntil(char *buffer, int count, const char *pattern, DataType type, uint16_t timeout = DEFAULT_TIMEOUT);
		bool ReadResponseUntil_EOL(char *buffer, int count, const char *pattern, DataType type, uint16_t timeout);
		// bool ReadResponse(char *buffer, uint16_t count, DataType type, uint32_t timeout = DEFAULT_TIMEOUT, uint32_t inchar_timeout = DEFAULT_INCHAR_TIMEOUT);
		void CleanBuffer(char* buffer, uint16_t count);
		void WriteCommand(char data);
		void WriteCommand(const char* cmd);
		void WriteCommand(char *data, uint16_t dataSize);
		void WriteCommand(const __FlashStringHelper* cmd);
		void WriteEndMark(void);
		bool WaitForResponse(const char* resp, DataType type, uint32_t timeout = DEFAULT_TIMEOUT, uint32_t inchar_timeout = DEFAULT_INCHAR_TIMEOUT);
		bool WriteCommandAndWaitForResponse(const char* cmd, const char *resp, DataType type, uint32_t timeout = DEFAULT_TIMEOUT*5, uint32_t inchar_timeout = DEFAULT_INCHAR_TIMEOUT);
		bool WriteCommandAndWaitForResponse(const __FlashStringHelper* cmd, const char *resp, DataType type, uint32_t timeout = DEFAULT_TIMEOUT, uint32_t inchar_timeout = DEFAULT_INCHAR_TIMEOUT);
		void AT_Bypass(void);
};
