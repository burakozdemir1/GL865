/*
 * 
 *
 *  Created on: Sep 4, 2024
 *      Author: burak
 */




#include "GL865.h"
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

UART_HandleTypeDef *GL865_Serial;
GPIO_TypeDef *GL865_POWER_EN_PORT;
uint16_t GL865_POWER_EN_PIN;

uint8_t GL865_RX_BUFFER[GL865_RX_BUFFER_SIZE];
uint16_t GL865_RX_BUFFER_INDEX = 0;

bool volatile GL865_LOCK_RX_BUFFER = false;

int Len(char *gelen)
{
  int i;
  for (i = 0; gelen[i] != 0; i++)
    ; // string'in son karakterini görene kadar i degiskeni artacaktir.
  return i;
}

int const_char_len(const char *mStr)
{
  int Size = 0;
  while (mStr[Size] != '\0')
    Size++;
  return Size;
}

uint32_t GetNetworkTime()
{
  bool mResult = GL865_Communicate("AT+CCLK?\r", 1000, "OK");
  char mData[64];

  //printf("CCLK MSG LEN = %i\n", GL865_RX_BUFFER_INDEX);

  char *ptr;
  int begin, end;

  ptr = strchr((char *)GL865_RX_BUFFER, '"');
  if (ptr == NULL)
  {
    //printf("Character not found\n");
    begin = 0;
  }
  begin = ptr - (char *)GL865_RX_BUFFER;

  ptr = strchr(ptr + 1, '"');
  if (ptr == NULL)
  {
    //printf("Character not found\n");
    end = 0;
  }
  end = ptr - (char *)GL865_RX_BUFFER;


  if (begin != 0 && end != 0)
  {

    memcpy((uint8_t *)mData, GL865_RX_BUFFER + begin, end - begin + 1);
    //printf("Network Time Str = %s\n", mData);

    int year, month, day, hour, minute, second;
    struct tm tm;
    time_t t_of_day;

    if (sscanf(mData, "\"%d/%d/%d,%d:%d:%d+12\"", &year, &month, &day, &hour, &minute, &second) >= 2)
    {
      tm.tm_sec = second;
      tm.tm_min = minute;
      tm.tm_hour = hour;
      tm.tm_mday = day;
      tm.tm_mon = month - 1;
      tm.tm_year = year + 2000 - 1900;
      int mErrChk = mktime(&tm);
      if (mErrChk == -1)
      {
        printf("Unable to make using mktime\n");
      }
      else
      {
        t_of_day = mktime(&tm);
        //printf("UnixTime = %i\n", (uint32_t)t_of_day);
      }
    }
    else
    {
      printf("Network Time ScanF Error !");
    }

    return (uint32_t)t_of_day;
  }
  else
  {
    printf("Network Time Error \n");
    return 0;
  }
}

bool CheckNetworkRegistiration()
{
  GL865_Communicate("AT+CREG?\r", 250, "OK");

  char *found = strstr((char *)GL865_RX_BUFFER, "+CREG: 0,1");
  if (found != NULL)
  {
    return true;
  }
  else
  {
    return false;
  }
}



bool CLM920_CheckNetworkRegistiration()
{
  GL865_Communicate("AT+CREG?\r", 250, "OK");

  char *found = strstr((char *)GL865_RX_BUFFER, "+CREG: 3,1,");
	char *found2 = strstr((char *)GL865_RX_BUFFER, "+CREG: 0,1");
	char *found3 = strstr((char *)GL865_RX_BUFFER, "+CREG: 0,11");
  if ((found != NULL || found2 != NULL) && found3 == NULL)
  {
    return true;
  }
  else
  {
    return false;
  }
}



bool CheckInternetConnection()
{

  bool mResult = false;
  for (int i = 0; i < 5; i++)
  {
    GL865_Communicate("AT#SGACT?\r", 250, "OK");

    char *found = strstr((char *)GL865_RX_BUFFER, "#SGACT: 1,1");
    if (found != NULL)
    {
      mResult = true;
      return true;
    }
    vTaskDelay(1000);
  }
  return mResult;
}

bool CLM920_CheckInternetConnection()
{

  bool mResult = false;
  for (int i = 0; i < 5; i++)
  {
    GL865_Communicate("AT+QIPACT?\r", 250, "OK");

    char *found = strstr((char *)GL865_RX_BUFFER, "+QIPACT:1,1");
    if (found != NULL)
    {
      mResult = true;
      return true;
    }
    vTaskDelay(1000);
  }
  return mResult;
}

bool GL865_Communicate(const char *TXData, uint64_t Timeout, const char *WaitingResponse)
{
  for (int i = 0; i < GL865_RX_BUFFER_SIZE; i++)
  {
    GL865_RX_BUFFER[i] = 0x00;
  }

	uint8_t *mTXData = (uint8_t *)TXData;
  GL865_RX_BUFFER_INDEX = 0;
  HAL_UART_Transmit(GL865_Serial, mTXData, const_char_len(TXData), 500);
  uint64_t WaitUntil = xTaskGetTickCount() + Timeout;
  vTaskDelay(25);
  while (xTaskGetTickCount() < WaitUntil)
  {
    char *found = strstr((char*)GL865_RX_BUFFER, WaitingResponse);
		
    if (found != NULL)                                              /* strstr returns NULL if item not found */
    {
      return true;
    }
  }
  return false;
}

const char *GL865_GetIMEI()
{

  bool mResult = GL865_Communicate("AT+CGSN\r", 1000, "OK");
  char mData[16];

  printf("IMEI MSG LEN = %i\n", GL865_RX_BUFFER_INDEX);
  if (mResult)
  {
    memcpy((uint8_t *)mData, GL865_RX_BUFFER + 10, 15);
  }

  std::string mDataResult(mData);

#ifdef DEBUG_GSM
  printf("IMEI = %s\n", mDataResult.c_str());
#endif
  return mDataResult.c_str();
}

const char *GL865_GetIP()
{
  return "127.0.0.1";

  bool mResult = GL865_Communicate("AT+CIFSR\r", 1000, "OK");
  char mData[16];
  if (mResult)
  {
    memcpy((uint8_t *)mData, GL865_RX_BUFFER + 10, GL865_RX_BUFFER_INDEX - 11);
  }
  std::string mDataResult(mData);

#ifdef DEBUG_GSM
  printf("IP = %s\n", mDataResult.c_str());
#endif
  return mDataResult.c_str();
}

bool GL865Connect(std::string SimGPIO, std::string SimPin, std::string SimAPN)
{
  printf("Connect Routine Start !\n");
  std::string mTempCmd = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";

  vTaskDelay(500);
  GL865_Communicate("AT+CMEE=1\r", 1000, "OK");
  vTaskDelay(500);

  if (GL865_Communicate("ATZ\r", 250, "OK"))
  {

#ifdef DEBUG_GSM
    printf("CMD = ATZ! OK \n");
#endif
    vTaskDelay(500);
    GL865_Communicate("AT+CMEE=1\r", 1000, "OK");
    vTaskDelay(500);

    mTempCmd = "AT#GPIO=" + SimGPIO + "\r";
    if (GL865_Communicate(mTempCmd.c_str(), 500, "OK"))
    {

#ifdef DEBUG_GSM
      printf("CMD = AT#GPIO! OK \n");
#endif
      if (GL865_Communicate("AT#SIMDET=0\r", 500, "OK"))
      {

#ifdef DEBUG_GSM
        printf("CMD = AT#SIMDET=0! OK \n");
#endif
        uint64_t WaitUntil = xTaskGetTickCount() + 7000;

        while (xTaskGetTickCount() < WaitUntil)
        {
          vTaskDelay(500);
        }

        if (GL865_Communicate("AT#SIMDET=1\r", 500, "OK"))
        {

#ifdef DEBUG_GSM
          printf("CMD = AT#SIMDET=1 OK\n");
#endif

          WaitUntil = xTaskGetTickCount() + 4000;

          while (xTaskGetTickCount() < WaitUntil)
          {
            vTaskDelay(500);
          }

          if (GL865_Communicate("AT+CSIM=0\r", 500, "OK"))
          {

#ifdef DEBUG_GSM
            printf("CMD = AT+CSIM=0 OK \n");
#endif

            mTempCmd = "AT+CPIN=" + SimPin + "\r\n";
						//mTempCmd = "AT\r\n";

            if (GL865_Communicate(mTempCmd.c_str(), 1000, "OK"))
            {

#ifdef DEBUG_GSM
              printf("CMD = AT+CPIN=xxxx OK \n");
#endif

              if (GL865_Communicate("AT&K0\r", 1000, "OK"))
              {
#ifdef DEBUG_GSM
                printf("CMD = AT&K0 OK \n");
#endif
                if (GL865_Communicate("AT#SLED=2\r", 1000, "OK"))
                {
#ifdef DEBUG_GSM
                  printf("CMD = AT#SLED=2 OK \n");
#endif
                  if (GL865_Communicate("AT&W0\r", 1000, "OK"))
                  {
#ifdef DEBUG_GSM
                    printf("CMD = AT&W0 OK \n");
#endif

                    if (GL865_Communicate("AT#GAUTH=0\r", 5000, "OK"))
										//if (1==1)
                    {
#ifdef DEBUG_GSM
                      printf("CMD = AT#GAUTH=0 OK \n");
#endif

                      bool NetworkRegistiration;

                      for (int i = 0; i < 30; i++)
                      {
                        NetworkRegistiration = CheckNetworkRegistiration();
                        if (NetworkRegistiration == true)
                        {
                          break;
                        }
                        vTaskDelay(1000);
                      }

                      if (NetworkRegistiration)
                      {

#ifdef DEBUG_GSM
                        printf("Registered to Network !\n");
#endif

                        mTempCmd = "AT+CGDCONT=1,\"IP\",\"" + SimAPN + "\"\r";
                        //printf("mCMD = %s\n", mCMD.c_str());

                        if (GL865_Communicate(mTempCmd.c_str(), 1000, "OK"))
                        {
#ifdef DEBUG_GSM
                          printf("CMD = AT+CGDCONT OK \n");

#endif

                          vTaskDelay(2000);

                          GL865_Communicate("AT#SGACT?\r", 1000, "OK");
                          vTaskDelay(1000);
                          bool mInternetResult;
                          for (int i = 0; i < 10; i++)
                          {
#ifdef DEBUG_GSM
                            printf("CMD =AT#SGACT Try No[%i] ... \n", i);
#endif
                            mInternetResult = GL865_Communicate("AT#SGACT=1,1\r", 10000, "OK");

                            if (mInternetResult)
                            {
#ifdef DEBUG_GSM
                              printf("CMD =AT#SGACT Try No[%i] OK \n", i);
#endif
                              break;
                            }
                            else
                            {
#ifdef DEBUG_GSM
                              printf("CMD =AT#SGACT Try No[%i] ERROR \n", i);
#endif
                            }
                          }

                          CheckInternetConnection();

#ifdef DEBUG_GSM
                          if (mInternetResult)
                          {
                            printf("Connected To Internet !\n");
                          }
                          else
                          {
                            printf("Internet Connection Error !\n");
                          }

#endif
                          return mInternetResult;
                        }
                        else
                        {

#ifdef DEBUG_GSM
                          printf("CMD = AT+CGDCONT ERROR \n");
#endif
                        }
                      }
                      else
                      {
#ifdef DEBUG_GSM
                        printf("Network Registiration Failed ! \n");
#endif
                      }
                    }
                    else
                    {
#ifdef DEBUG_GSM
                      printf("CMD = AT#GAUTH=0 ERROR \n");
#endif
                    }
                  }
                  else
                  {
#ifdef DEBUG_GSM
                    printf("CMD = AT&W0 ERROR \n");
#endif
                  }
                }
                else
                {
#ifdef DEBUG_GSM
                  printf("CMD = AT#SLED=2 ERROR \n");
#endif
                }
              }
              else
              {
#ifdef DEBUG_GSM
                printf("CMD = AT&K0 ERROR \n");
#endif
              }
            }
            else
            {
#ifdef DEBUG_GSM
              printf("CMD = AT+CPIN=xxxx ERROR \n");
#endif
            }
          }
          else
          {
#ifdef DEBUG_GSM
            printf("CMD = AT+CSIM=0 ERROR \n");
#endif
          }
        }
        else
        {
#ifdef DEBUG_GSM
          printf("CMD = AT#SIMDET=1 ERROR \n");
#endif
        }
      }
      else
      {

#ifdef DEBUG_GSM
        printf("CMD = AT#SIMDET=0! ERROR \n");
#endif
      }
    }
    else
    {

#ifdef DEBUG_GSM
      printf("CMD = AT#GPIO ERROR \n");
#endif
    }
  }
  else
  {

#ifdef DEBUG_GSM
    printf("CMD = ATZ! ERROR \n");
#endif
  }

  printf("Connect Routine End !\n");

  return false;
}

void ModemHardReset()
{
#ifdef DEBUG_GSM
  //printf("Modem HardReset - Power Off \n");
#endif
  HAL_GPIO_WritePin(GL865_POWER_EN_PORT, GL865_POWER_EN_PIN, GPIO_PIN_RESET);
  vTaskDelay(750);
#ifdef DEBUG_GSM
  //printf("Modem HardReset - Power ON \n");
#endif
  HAL_GPIO_WritePin(GL865_POWER_EN_PORT, GL865_POWER_EN_PIN, GPIO_PIN_SET);
  vTaskDelay(5000);
}

const char *HTTPRequest(const char *SERVER, const char *SERVER_PORT, const char *API, const char *Data, int *result, int ModemType)
{
	
	if(ModemType == 2) {
	
		return CLM920_HTTPRequest(SERVER, SERVER_PORT, API, Data, result);
		
	} else {
	
	#ifdef DEBUG_GSM
		printf("HTTPRequest Begin \nSocket Open\n");

	#endif

		std::string mTemp = "";
		mTemp += "AT#SD=1,0,";
		mTemp += SERVER_PORT;
		mTemp += ",\"";
		mTemp += SERVER;
		mTemp += "\",0,0\r";

	#ifdef DEBUG_GSM
		printf("Connecting URL = %s\n", mTemp.c_str());
	#endif
		bool ConnectServer = GL865_Communicate(mTemp.c_str(), 25000, "CONNECT");

		if (ConnectServer)
		{

			char mContentLength[256];
			snprintf(mContentLength, sizeof(mContentLength), "%i", const_char_len(Data));
			mTemp = "";
			mTemp += "POST ";
			mTemp += API;
			mTemp += " HTTP/1.1\r\n";
			mTemp += "Host: ";
			mTemp += SERVER;
			mTemp += ":";
			mTemp += SERVER_PORT;
			mTemp += "\r\n";
			mTemp += "Content-Type: application/json\r\n";
			mTemp += "Content-Length: ";
			mTemp += mContentLength;
			mTemp += "\r\n";
			mTemp += "Cache-Control: no-cache\r\n";
			mTemp += "\r\n";
			mTemp += Data;

	#ifdef DEBUG_GSM
			printf("POST DATA = %s\n", mTemp.c_str());
	#endif

			*result = 0;
			GL865_Communicate(mTemp.c_str(), 45000, "NO CARRIER\r\n");
		}
		else
		{
			*result = -1;
		}

		mTemp.clear();

		char *ptr;
		int index;

		ptr = strchr((char *)GL865_RX_BUFFER, '{');
		if (ptr == NULL)
		{
			printf("Character not found\n");
			index = 0;
		}

		index = ptr - (char *)GL865_RX_BUFFER;

		//printf("The index is %d\n", index);

		if (GL865_RX_BUFFER_INDEX > 14)
		{
			//REMOVE NO CARRIER
			for (int i = GL865_RX_BUFFER_INDEX - 14; i < GL865_RX_BUFFER_SIZE; i++)
			{
				GL865_RX_BUFFER[i] = 0x00;
			}
		}

		if (GL865_RX_BUFFER[GL865_RX_BUFFER_INDEX + 1] == 0x00)
		{

			return (const char *)GL865_RX_BUFFER + index;
		}
		else
		{
			return "{\"Status\":\"0\"}";
		}
	}
}





void CLM920_ModemHardReset()
{
#ifdef DEBUG_GSM
  //printf("Modem HardReset - Power Off \n");
#endif
  HAL_GPIO_WritePin(GL865_POWER_EN_PORT, GL865_POWER_EN_PIN, GPIO_PIN_RESET);
  vTaskDelay(750);
#ifdef DEBUG_GSM
  //printf("Modem HardReset - Power ON \n");
#endif
  HAL_GPIO_WritePin(GL865_POWER_EN_PORT, GL865_POWER_EN_PIN, GPIO_PIN_SET);
  vTaskDelay(1000);

	vTaskDelay(8000);
}


bool CLM920_Connect(std::string SimPin, std::string SimAPN)
{
  printf("Connect Routine Start !\n");
	
  std::string mTempCmd = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";


	GL865_Communicate("AT+HOSCFG=1,1\r", 1000, "OK");
	vTaskDelay(2000);
  GL865_Communicate("AT+CFUN?\r", 1000, "OK");
  vTaskDelay(100);

  if (GL865_Communicate("ATZ\r", 250, "OK"))
  {

#ifdef DEBUG_GSM
    printf("CMD = ATZ! OK \n");
#endif
    vTaskDelay(100);	


		mTempCmd = "AT+CPIN=" + SimPin + "\r\n";
		//mTempCmd = "AT\r\n";

		if (GL865_Communicate(mTempCmd.c_str(), 1000, "OK"))
		{

#ifdef DEBUG_GSM
			printf("CMD = AT+CPIN=xxxx OK \n");
#endif

			vTaskDelay(100);



			bool NetworkRegistiration;

			for (int i = 0; i < 60; i++)
			{
				NetworkRegistiration = CLM920_CheckNetworkRegistiration();
				if (NetworkRegistiration == true)
				{
					break;
				}
				vTaskDelay(500);
			}

			if (NetworkRegistiration)
			{

#ifdef DEBUG_GSM
				printf("Registered to Network !\n");
#endif

				mTempCmd = "AT+CGDCONT=1,\"IP\",\"" + SimAPN + "\"\r";
				//printf("mCMD = %s\n", mCMD.c_str());

				if (GL865_Communicate(mTempCmd.c_str(), 1000, "OK"))
				{
#ifdef DEBUG_GSM
					printf("CMD = AT+CGDCONT OK \n");

#endif

					
					GL865_Communicate("AT+CGATT=1\r", 5000, "OK");
					GL865_Communicate("AT+CGACT=1,1\r", 5000, "OK");
					
					vTaskDelay(100);

					GL865_Communicate("AT+CGDCONT?\r", 1000, "OK");
					vTaskDelay(100);
					bool mInternetResult;
					for (int i = 0; i < 10; i++)
					{
#ifdef DEBUG_GSM
						printf("CMD =AT#SGACT Try No[%i] ... \n", i);
#endif
					
						GL865_Communicate("AT+CGDCONT?\r", 1000, "OK");
						
						
						mInternetResult = GL865_Communicate("AT+QIPACT=1\r", 10000, "OK");

						if (mInternetResult)
						{
#ifdef DEBUG_GSM
							printf("CMD =AT#SGACT Try No[%i] OK \n", i);
#endif
							break;
						}
						else
						{
#ifdef DEBUG_GSM
							printf("CMD =AT#SGACT Try No[%i] ERROR \n", i);
#endif
						}
					}

					CLM920_CheckInternetConnection();

#ifdef DEBUG_GSM
					if (mInternetResult)
					{
						printf("Connected To Internet !\n");
					}
					else
					{
						printf("Internet Connection Error !\n");
					}

#endif
					return mInternetResult;
				}
				else
				{

#ifdef DEBUG_GSM
					printf("CMD = AT+CGDCONT ERROR \n");
#endif
				}
			}
			else
			{
#ifdef DEBUG_GSM
				printf("Network Registiration Failed ! \n");
#endif
			}
		} else {
#ifdef DEBUG_GSM
printf("CMD = AT+CPIN=xxxx ERROR \n");
#endif
		}
	} else {
#ifdef DEBUG_GSM
printf("CMD = ATZ ERROR \n");
#endif
	}



  printf("Connect Routine End !\n");

  return false;
}


bool CLM920_TCPLogRequest(const char *SERVER, const char *SERVER_PORT, const char *Data) {
	
#ifdef DEBUG_GSM
  printf("CLM920_TCPLogRequest Begin \nSocket Open\n");
#endif
	
	
	std::string mTemp = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
  mTemp = "AT+QIPOPEN=1,1,\"TCP\",\"";
  mTemp += SERVER;
  mTemp += "\",";
  mTemp += SERVER_PORT;
  mTemp += ",,1\r\n";

#ifdef DEBUG_GSM
  printf("Connecting URL = %s\n", mTemp.c_str());
#endif
	
	bool ConnectServer = GL865_Communicate(mTemp.c_str(), 3000, "+QIPOPEN: 1,0");
	
	if (ConnectServer)
  {	
		char mContentLength[256];
		snprintf(mContentLength, sizeof(mContentLength), "%i", const_char_len(Data));
		mTemp = Data;
		bool ConnectResult = GL865_Communicate("AT+QIPSEND=1\r\n", 3000, ">");
		
		
		GL865_Communicate(mTemp.c_str(), 5000, "+QIPCLOSEURC: 1");
		GL865_Communicate("AT+QIPCLOSE=1\r\n", 500, "OK");
		
	} else {
		
		GL865_Communicate("AT+QIPCLOSE=1\r\n", 500, "OK");
		return false;
	}

}


const char *CLM920_HTTPRequest(const char *SERVER, const char *SERVER_PORT, const char *API, const char *Data, int *result)
{
	
	
	char mPacket[1024] = {0x00};
	sprintf(mPacket, "AT+QIPOPEN=1,1,\"TCP\",\"%s\",%s,,1\r\n",SERVER,SERVER_PORT);
	printf("mPacket = %s\n", mPacket);

  bool ConnectServer = GL865_Communicate(mPacket, 2500, "+QIPOPEN: 1,0");
  if (ConnectServer)
  {		
    char mContentLength[1024];
    snprintf(mContentLength, sizeof(mContentLength), "%i", const_char_len(Data));
   
		bool ConnectResult = GL865_Communicate("AT+QIPSEND=1\r\n", 30000, ">");
		
	
		char mPacket[1024] = {0x00};
		char mEndChar = 0x1A;
		
		sprintf(mPacket, "POST %s HTTP/1.1\r\nHost: %s:%s\r\nContent-Type: application/json\r\nContent-Length: %s\r\nCache-Control: no-cache\r\n\r\n%s%c",
		API, SERVER, SERVER_PORT, mContentLength, Data, mEndChar);

		// printf("FreeHeap GL865 = %i\n", xPortGetFreeHeapSize());
		//vTaskDelay(1000);
#ifdef DEBUG_GSM
    printf("POST DATA = %s\n", mPacket);
#endif

    *result = 0;
    GL865_Communicate(mPacket, 30000, "+QIPCLOSEURC: 1");
		vTaskDelay(50);
  }
  else
  {
		
		GL865_Communicate("AT+QIPCLOSE=1\r\n", 1000, "OK");
    *result = -1;
  }

	
  char *ptr;
  int index;
	

  ptr = strchr((char *)GL865_RX_BUFFER, '{');
  if (ptr == NULL)
  {
    printf("Character not found\n");
    index = 0;
  }

  index = ptr - (char *)GL865_RX_BUFFER;

  //printf("The index is %d\n", index);

  if (GL865_RX_BUFFER_INDEX > 21)
  {
    //REMOVE NO CARRIER
    for (int i = GL865_RX_BUFFER_INDEX - 21; i < GL865_RX_BUFFER_SIZE; i++)
    {
      GL865_RX_BUFFER[i] = 0x00;
    }
		
		//REMOVE CLM920 INFORMATION MESSAGES..
		RemoveCLM920Messages();
		RemoveLF();
		RemoveCR();
  }

  if (GL865_RX_BUFFER[GL865_RX_BUFFER_INDEX + 1] == 0x00)
  {
		
		ptr = strchr((char *)GL865_RX_BUFFER, '{');
		if (ptr == NULL)
		{
			printf("Character not found\n");
			index = 0;
		}

		index = ptr - (char *)GL865_RX_BUFFER;
		const char *mResult = (const char *)GL865_RX_BUFFER + index;
		printf("%s\n", mResult);
    return mResult;
  }
  else
  {
		GL865_Communicate("AT+QIPCLOSE=1\r\n", 1000, "OK");
    return "{\"Status\":\"0\"}";
  }
}



void RemoveLF() {
	char StartNeedle[32] = "\r";
	char *StartStr;
	int StartIndex = 0;
	
	StartStr = strstr((char *)GL865_RX_BUFFER, StartNeedle);
	
	while(StartStr != NULL) {
    
    StartStr = StartStr + 1;
    StartIndex = StartStr - (char *)GL865_RX_BUFFER;
		
		//printf("StartIndex = %i\n", StartIndex);
    
    for(int i = StartIndex-1; i <= GL865_RX_BUFFER_INDEX; i++) {
        GL865_RX_BUFFER[i] = GL865_RX_BUFFER[i + 1];
    }
    
    StartStr = strstr((char *)GL865_RX_BUFFER, StartNeedle);
	}
}

void RemoveCR() {
	char StartNeedle[32] = "\n";
	char *StartStr;
	int StartIndex = 0;
	
	StartStr = strstr((char *)GL865_RX_BUFFER, StartNeedle);
	
	while(StartStr != NULL) {
    
    StartStr = StartStr + 1;
    StartIndex = StartStr - (char *)GL865_RX_BUFFER;
    
    for(int i = StartIndex-1; i <= GL865_RX_BUFFER_INDEX; i++) {
        GL865_RX_BUFFER[i] = GL865_RX_BUFFER[i + 1];
    }
    
    StartStr = strstr((char *)GL865_RX_BUFFER, StartNeedle);
	}
}

void RemoveCLM920Messages() {
	

	char StartNeedle[32] = "RECV FROM";
	char EndNeedle[10] = "\r\n";
	char *StartStr;
	char *EndStr;
	int StartIndex = 0;
	int EndIndex = 0;
	
	StartStr = strstr((char *)GL865_RX_BUFFER, StartNeedle);
	
	while(StartStr != NULL) {
    
    StartStr = StartStr + 1;
    StartIndex = StartStr - (char *)GL865_RX_BUFFER;
    EndStr = strstr(StartStr,EndNeedle) + 1;
    EndIndex = EndStr - (char *)StartStr;
		
    
    for(int i = StartIndex-1; i <= GL865_RX_BUFFER_INDEX; i++) {
        GL865_RX_BUFFER[i] = GL865_RX_BUFFER[i + EndIndex+1];
    }
    
    StartStr = strstr((char *)GL865_RX_BUFFER, StartNeedle);
	}

}

int GetCLM920_SignalQuality() {
	
	bool mResult = GL865_Communicate("AT+CSQ\r", 1000, "OK");	
	char mData[3];

  if (mResult)
  {
    memcpy((uint8_t *)mData, GL865_RX_BUFFER + 15, 3);
		
		if(mData[1] == ',') {
			mData[1] = 0x00;
		}
		mData[2] = 0x00;

		int num = (int)(atoi(mData) * 3.22);
		return  num;
  }	
}