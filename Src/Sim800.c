


/* Private includes */
/* USER CODE BEGIN Includes */

#include "Sim800.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "timer.h"
#include "LCD.h"

/* USER CODE END Includes */

/* Private define */
/* USER CODE BEGIN Includes */

#define LCD_DEBUG

/* USER CODE END Includes */

/* Private variables */
/* USER CODE BEGIN PV */

uint8_t reciveSms[] = "AT+CNMI=1,2,0,0,0";
uint8_t networkReigistration[] = "AT+CREG?\r";
uint8_t signalCheck[] = "AT+CSQ\r";
uint8_t deleteReadMessage[] = "AT+CMGDA=\"DEL READ\"\r";
uint8_t deleteAllMessage[] = "AT+CMGDA=\"DEL ALL\"\r";
uint8_t atCmgf[] = "AT+CMGF?\r";
uint8_t textMode[] = "AT+CMGF=?\r";
uint8_t atTextModesave[] = "AT&W\r";
uint8_t messageText[100] = {0};
uint8_t messageNumber[15] = {0};
uint8_t addresLen = 0;
uint8_t text = 0;
uint8_t messageIndex[2] = {0};
uint32_t gsmTaskTimeout = 0;

Sms sms;
extern uint8_t rxData[1000];
extern uint16_t rxDataIndex;

char signal[5];
char tempstr[100];
char display[17];
char networkNumber[1];
char *location;
char *enterFinder;
char *commaFinder;
char display[17];

typedef enum
{
	GSM_TASK_STATE_START_1
	, GSM_TASK_STATE_START_2
	, GSM_TASK_STATE_WAITING_ANTENA_1
	, GSM_TASK_STATE_WAITING_ANTENA_2
	, GSM_TASK_STATE_SEND_MESSAGE_ENTER_NUMBER
	, GSM_TASK_STATE_SEND_MESSAGE_SEND_TEXT
	, GSM_TASK_STATE_IDLE
	, GSM_TASK_STATE_TEXT_MODE_CHECK_1
	, GSM_TASK_STATE_TEXT_MODE_CHECK_2
	, GSM_TASK_STATE_TEXT_MODE_SET
	, GSM_TASK_STATE_TEXT_MODE_SAVE
	, GSM_TASK_STATE_DELETE_READ_MESSAGE
	, GSM_TASK_STATE_SIGNAL_CHECK
	, GSM_TASK_STATE_SIGNAL_CHECK_2
	, GSM_TASK_STATE_DELETE_ALL_MESSSAGE_1
	, GSM_TASK_STATE_DELETE_ALL_MESSSAGE_2
	, GSM_TASK_STATE_READ_MESSAGE
	, GSM_TASK_STATE_CHECK_NEW_MESSAGE
}GSMTaskState;

GSMTaskState gsmTakState = GSM_TASK_STATE_START_1;

/* USER CODE END PV */


/* Private functions prototypes */
/* USER CODE BEGIN PV */


void gsmStart(void);
void resetBuffer(void);

/* USER CODE END PV */



/* Main */
/* USER CODE BEGIN PV */

void gsmTask(void)
{
	switch(gsmTakState)
	{
	case GSM_TASK_STATE_START_1:
		HAL_GPIO_WritePin(SIM_P_SW_GPIO_Port, SIM_P_SW_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(SIM_EN_GPIO_Port, SIM_EN_Pin, GPIO_PIN_SET);
        gsmTaskTimeout = timerStartTimeout();
        gsmTakState = GSM_TASK_STATE_START_2;
		break;

	case GSM_TASK_STATE_START_2:
		 if(timerCheckTimeout(gsmTaskTimeout, 1300))
		 {
			 HAL_GPIO_WritePin(SIM_EN_GPIO_Port, SIM_EN_Pin, GPIO_PIN_RESET);
			 LCD_Clear();
#ifdef LCD_DEBUG
		     LCD_Puts(0, 0, "Waiting for anten" );
#endif
			 gsmTakState = GSM_TASK_STATE_WAITING_ANTENA_1;
		 }
		break;

	case GSM_TASK_STATE_WAITING_ANTENA_1:
		gsmTaskTimeout = timerStartTimeout();
		resetBuffer();
		HAL_UART_Transmit(&huart3, networkReigistration, strlen((char*)networkReigistration), 100);
		gsmTakState = GSM_TASK_STATE_WAITING_ANTENA_2;

		break;

	case GSM_TASK_STATE_WAITING_ANTENA_2:
		if(timerCheckTimeout(gsmTaskTimeout, 500))
		{
			location = strstr((char*)rxData, "+CREG: ");
			if(location != 0)
			{
				strncpy(networkNumber, location + 9, 1);
				if(strchr(location, '5'))
				{
#ifdef LCD_DEBUG
					LCD_Clear();
					LCD_Puts(0, 0, "Ready ...");
					memset(location, 0, 1000);
#endif //LCD_DEBUG
					gsmTakState = GSM_TASK_STATE_TEXT_MODE_CHECK_1;
				}
				else
				{
					gsmTakState = GSM_TASK_STATE_WAITING_ANTENA_1;
				}
			}
			else
			{
				gsmTakState = GSM_TASK_STATE_WAITING_ANTENA_1;
			}

		}
		break;

	case GSM_TASK_STATE_TEXT_MODE_CHECK_1:
		resetBuffer();
		HAL_UART_Transmit(&huart3, atCmgf, strlen((char*)atCmgf), 100);
		gsmTakState = GSM_TASK_STATE_TEXT_MODE_CHECK_2;
		gsmTaskTimeout = timerStartTimeout();
		//LCD_Clear();
		break;

	case GSM_TASK_STATE_TEXT_MODE_CHECK_2:
		if(timerCheckTimeout(gsmTaskTimeout, 200))
		{
			if(strstr((char*)rxData, "+CMGF: 0"))
			{
#ifdef LCD_DEBUG
				LCD_Puts(0, 1, "PDU mode.");
#endif //LCD_DEBUG
				gsmTakState = GSM_TASK_STATE_TEXT_MODE_SET;
			}
			if(strstr((char*)rxData, "+CMGF: 1"))
			{
#ifdef LCD_DEBUG
				LCD_Puts(0, 1, "Text mode");
#endif //LCD_DEBUG
				gsmTakState = GSM_TASK_STATE_DELETE_ALL_MESSSAGE_1;
			}
			else
			{
#ifdef LCD_DEBUG
				LCD_Puts(0, 1, "Error");
#endif //LCD_DEBUG
				gsmTakState = GSM_TASK_STATE_IDLE;
			}
		}
		break;

	case GSM_TASK_STATE_TEXT_MODE_SET:
		resetBuffer();
		HAL_UART_Transmit(&huart3, textMode, strlen((char*)textMode), 100);
		gsmTakState = GSM_TASK_STATE_TEXT_MODE_SAVE;
		gsmTaskTimeout = timerStartTimeout();
		break;

	case GSM_TASK_STATE_TEXT_MODE_SAVE:
		if(timerCheckTimeout(gsmTaskTimeout, 200))
		{
			if(strstr((char*)rxData, "OK"))
			{
				HAL_UART_Transmit(&huart3, atTextModesave, strlen((char*)atTextModesave), 100);
#ifdef LCD_DEBUG
				LCD_Clear();
				LCD_Puts(0, 1, "New mode saved");
#endif //LCD_DEBUG
				gsmTakState = GSM_TASK_STATE_IDLE;
			}
			else
			{
#ifdef LCD_DEBUG
				LCD_Clear();
				LCD_Puts(0, 1, "Not changed");
#endif //LCD_DEBUG
				gsmTakState = GSM_TASK_STATE_IDLE;
			}
		}
		break;

	case GSM_TASK_STATE_SEND_MESSAGE_ENTER_NUMBER:
		sprintf(tempstr, "AT+CMGS=\"%s\"\r", (char*)messageNumber);
		resetBuffer();
		HAL_UART_Transmit(&huart3, (uint8_t*)tempstr, strlen(tempstr), 100);
		gsmTakState = GSM_TASK_STATE_SEND_MESSAGE_SEND_TEXT;
		break;

	case GSM_TASK_STATE_SEND_MESSAGE_SEND_TEXT:
		if(strchr((char*)rxData, '>'))
		{
			HAL_UART_Transmit(&huart3, messageText, strlen((char*)messageText), 100);
			memset(messageText, 0, 100);
			gsmTakState = GSM_TASK_STATE_IDLE;
		}
		break;

	case GSM_TASK_STATE_DELETE_ALL_MESSSAGE_1:
		resetBuffer();
		HAL_UART_Transmit(&huart3, deleteAllMessage, strlen((char*)deleteAllMessage), 100);
		gsmTakState = GSM_TASK_STATE_DELETE_ALL_MESSSAGE_2;
		break;

	case GSM_TASK_STATE_DELETE_ALL_MESSSAGE_2:
		if(strstr((char*)rxData, "OK"))
		{
#ifdef LCD_DEBUG
			LCD_Clear();
			LCD_Puts(0, 0, "Message deleted");
			gsmTakState = GSM_TASK_STATE_IDLE;
#endif //LCD_DEBUG
		}
		if(strstr((char*)rxData, "ERROR"))
		{
#ifdef LCD_DEBUG
			LCD_Clear();
			LCD_Puts(0, 0, "Message delete error !");
			gsmTakState = GSM_TASK_STATE_IDLE;
#endif //LCD_DEBUG
		}

		break;

	case GSM_TASK_STATE_DELETE_READ_MESSAGE:
		resetBuffer();
		HAL_UART_Transmit(&huart3, deleteReadMessage, strlen((char*)deleteReadMessage), 100);
		if(strstr((char*)rxData, "OK"))
		{
#ifdef LCD_DEBUG
			LCD_Clear();
			LCD_Puts(0, 1, "Message deleted");
#endif //LCD_DEBUG
		}
		if(strstr((char*)rxData, "ERROR"))
		{
#ifdef LCD_DEBUG
			LCD_Clear();
			LCD_Puts(0, 1, "Message delete error !");
#endif //LCD_DEBUG
		}
		gsmTakState = GSM_TASK_STATE_IDLE;

		break;

	case GSM_TASK_STATE_IDLE:

		break;

	case GSM_TASK_STATE_SIGNAL_CHECK:
		resetBuffer();
		HAL_UART_Transmit(&huart3, signalCheck, strlen((char*)signalCheck), 100);
		gsmTakState = GSM_TASK_STATE_SIGNAL_CHECK_2;

		break;
	case GSM_TASK_STATE_SIGNAL_CHECK_2:
		location = strstr((char*)rxData, "CSQ: ");
		if(location != 0)
		{
			if(strchr(location ,0xd))
			{
				strncpy(signal, (char*)rxData + 6, 5);
				sprintf(display, "Signal:%s", signal);
#ifdef LCD_DEBUG
				LCD_Clear();
				LCD_Puts(0, 0, display);
#endif //LCD_DEBUG
				memset(location, 0, 1000);
				memset(display, 0, 1000);
				gsmTakState = GSM_TASK_STATE_IDLE;
			}
		}
		break;
	case GSM_TASK_STATE_CHECK_NEW_MESSAGE:
		location = strstr((char*)rxData, "+CMTI: \"SM\",");
		if(location != 0)
		{

			location = strchr(location, ',');
			enterFinder = strchr(location, '\r');
			if(enterFinder != 0)
			{
				addresLen = enterFinder - location;
				strncpy((char*)messageIndex, location + 1, addresLen);
				text = atoi((char*)messageIndex);
				sprintf((char*)messageText, "AT+CMGR=%d\r", text);
				resetBuffer();
				HAL_UART_Transmit(&huart3, messageText, strlen((char*)messageText), 100);

				gsmTakState = GSM_TASK_STATE_READ_MESSAGE;
			}
		}
		break;

	case GSM_TASK_STATE_READ_MESSAGE:
		location = strstr((char*)rxData, "UNREAD");
		if(location != 0)
		{
			location = strchr(location, ',');
			enterFinder = strchr(location, '\r');
			enterFinder = strchr(enterFinder + 1, '\r');
			if(enterFinder != 0)
			{
				commaFinder = strchr(location + 1, ',');
				addresLen = commaFinder - location - 3;
				strncpy(sms.phoneNumber, location + 2, addresLen);		//phone number
#ifdef LCD_DEBUG
				LCD_Clear();
				LCD_Puts(0, 0, sms.phoneNumber);
#endif //LCD_DEBUG
				memset(display, 0, 17);

				location = strchr(location, ',');
				location = strchr(location, ',');
				strncpy(sms.smsDate, location + 1, 8);					//sms date
				memset(sms.smsDate, 0, 17);

				location = strchr(location, ',');
				strncpy(sms.smsTime, location + 1, 8);                  //sms time
				memset(sms.smsTime, 0, 17);

				location = strchr(location, '\n');
				enterFinder = strchr(location, '\r');
				addresLen = enterFinder - location;
				strncpy(sms.smsText, location + 1, addresLen);			//sms text
#ifdef LCD_DEBUG
				LCD_Puts(0, 1, sms.smsText);
#endif //LCD_DEBUG
				memset(display, 0, 17);
				gsmTakState = GSM_TASK_STATE_IDLE;
			}
		}

		break;


	}
}

/* USER CODE END PV */


/* Functions code */

/* USER CODE BEGIN PV */

void resetBuffer(void)
{
	memset(rxData, 0, 1000);
	rxDataIndex = 0;
}


GSMStatus gsmReciveMessage(void)
{
	if(gsmTakState == GSM_TASK_STATE_IDLE)
	{
		gsmTakState = GSM_TASK_STATE_CHECK_NEW_MESSAGE;
	}
	return GSM_STATUS_BUSY;
}

GSMStatus gsmSendMessage(uint8_t* text, uint8_t* number)
{
	if(gsmTakState == GSM_TASK_STATE_IDLE)
	{
		strcpy((char*)messageText, (char*)text);
		messageText[strlen((char*)messageText)] = 26; // 26 == ascii code of "ctr z".
		strcpy((char*)messageNumber, (char*)number);
		gsmTakState = GSM_TASK_STATE_SEND_MESSAGE_ENTER_NUMBER;
		return GSM_STATUS_OK;

	}
	return GSM_STATUS_BUSY;
}

/* USER CODE END PV */







