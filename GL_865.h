#ifndef GL865_H
#define GL865_H


#include "stm32f4xx_hal.h"
#include <time.h>
#include <sstream>


#include <string>
#include <string.h>
#include "stdio.h"

#include <vector>
#include "cmsis_os.h"
#define DEBUG_GSM

#define GL865_RX_BUFFER_SIZE 8192

//Added CLM920 - 4G Options. ClassName should be Changed !!!!

using std::vector;

extern UART_HandleTypeDef *GL865_Serial;
extern GPIO_TypeDef *GL865_POWER_EN_PORT;
extern uint16_t GL865_POWER_EN_PIN;

extern volatile bool GL865_LOCK_RX_BUFFER;
extern uint8_t GL865_RX_BUFFER[GL865_RX_BUFFER_SIZE];
extern uint16_t GL865_RX_BUFFER_INDEX;

void ModemHardReset();

bool GL865_Communicate(const char *, uint64_t, const char *);
const char * GL865_GetIMEI();
const char * GL865_GetIP();
bool GL865Connect(std::string, std::string, std::string);
bool CheckInternetConnection();
bool CheckNetworkRegistiration();

uint32_t GetNetworkTime();
const char* HTTPRequest(const char*,const char *,const char*, const char*, int *, int);


//CLM920 Functions ...
void CLM920_ModemHardReset();
bool CLM920_Connect(std::string, std::string);
bool CLM920_CheckNetworkRegistiration();
bool CLM920_CheckInternetConnection();
const char* CLM920_HTTPRequest(const char*,const char *,const char*, const char*, int *);
bool CLM920_TCPLogRequest(const char *, const char *, const char *);
void RemoveCLM920Messages();
void RemoveCR();
void RemoveLF();

int GetCLM920_SignalQuality();
#endif
