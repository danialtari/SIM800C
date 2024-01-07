
#ifndef INC_SIM800_H_
#define INC_SIM800_H_

/*
Sim800 library
*/

#include "gpio.h"


typedef struct
{
	uint8_t existMessage;
	char phoneNumber[15];
	char smsDate[9];
	char smsTime[9];
	char smsText[100];
}Sms;

typedef enum
{
	GSM_STATUS_OK
	, GSM_STATUS_BUSY
	, GSM_STATUS_SIGNAL_ERROR
}GSMStatus;



void gsmTask(void);
GSMStatus gsmSendMessage(uint8_t* text, uint8_t* number);
GSMStatus gsmReciveMessage(void);


#endif /* INC_SIM800_H_ */
