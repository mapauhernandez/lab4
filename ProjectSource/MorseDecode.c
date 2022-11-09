/****************************************************************************
 Module
   MorseDecode.c

 Revision
   1.0.1

 Description
   This is the first service Morse Events 

 Notes

 ****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// This module
#include "../ProjectHeaders/MorseDecode.h"
#include "../ProjectHeaders/LEDService.h"

// Hardware
#include <xc.h>
#include <string.h>
//#include <proc/p32mx170f256b.h>

// Event & Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_DeferRecall.h"
#include "ES_Port.h"
#include "terminal.h"
#include "dbprintf.h"
#include "ButtonModule.h"

/*----------------------------- Module Defines ----------------------------*/
// these times assume a 10.000mS/tick timing
#define ONE_SEC 1000
#define HALF_SEC (ONE_SEC / 2)
#define TWO_SEC (ONE_SEC * 2)
#define FIVE_SEC (ONE_SEC * 5)

#define ENTER_POST     ((MyPriority<<3)|0)
#define ENTER_RUN      ((MyPriority<<3)|1)
#define ENTER_TIMEOUT  ((MyPriority<<3)|2)

#define TEST_INT_POST
/*---------------------------- Module Functions ---------------------------*/
char DecodeMorseString(void);

/*---------------------------- Module Variables ---------------------------*/
enum state {
    InitMorseElements, CalWaitForRise, CalWaitForFall, EOC_WaitRise, EOC_WaitFall, DecodeWaitFall, DecodeWaitRise
};

static uint8_t MyPriority;

static char MorseString[8];
static char legalChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890?.,:'-/()\"= !$&+;@_";
static char morseCode[][8] = {".-", "-...", "-.-.", "-..", ".", "..-.", "--.",
    "....", "..", ".---", "-.-", ".-..", "--", "-.", "---",
    ".--.", "--.-", ".-.", "...", "-", "..-", "...-",
    ".--", "-..-", "-.--", "--..", ".----", "..---",
    "...--", "....-", ".....", "-....", "--...", "---..",
    "----.", "-----", "..--..", ".-.-.-", "--..--",
    "---...", ".----.", "-....-", "-..-.", "-.--.-",
    "-.--.-", ".-..-.", "-...-", "-.-.--", "...-..-",
    ".-...", ".-.-.", "-.-.-.", ".--.-.", "..--.-"};

/*------------------------------ Module Code ------------------------------*/

/****************************************************************************
 Function
     InitMorseService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any
     other required initialization for this service
 ****************************************************************************/
bool InitMorseDecode(uint8_t Priority) {
    MyPriority = Priority;
    MorseString[0] = '\0';

}

/****************************************************************************
 Function
     PostMorseDecode

 Parameters
     ES_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 ****************************************************************************/
bool PostMorseDecode(ES_Event_t ThisEvent) {
    return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunMorseService

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 ****************************************************************************/
ES_Event_t RunMorseDecode(ES_Event_t ThisEvent) {
    ES_Event_t ReturnEvent;
    ReturnEvent.EventType = ES_NO_EVENT;

    switch (ThisEvent.EventType) {
        case DOT_DETECTED:
        {
            uint16_t index = strlen(MorseString);
            if (index < 6) { //if the dot fits
                MorseString[index] = '.';
                MorseString[index + 1] = '\0';

            } else {
                ReturnEvent.EventType = ES_ERROR;
                ReturnEvent.EventParam = DOT_DETECTED;
            }
        }
            break;
        case DASH_DETECTED:
        {
            uint16_t index = strlen(MorseString);
            if (index < 6) { //if the dot fits
                MorseString[index] = '-';
                MorseString[index + 1] = '\0';

            } else {
                ReturnEvent.EventType = ES_ERROR;
                ReturnEvent.EventParam = DASH_DETECTED;
            }
        }
            break;
        case EOC_DETECTED:
        {
            char cur = DecodeMorseString();
            struct ES_Event ThisEvent;
            ThisEvent.EventType = ES_NEW_KEY;
            ThisEvent.EventParam = cur;

            PostLED(ThisEvent);
            DB_printf("%c", cur);
            MorseString[0] = '\0';
        }
            break;
        case EOW_DETECTED:
        {
            char cur = DecodeMorseString();
            struct ES_Event ThisEvent;
            ThisEvent.EventType = ES_NEW_KEY;
            ThisEvent.EventParam = cur;
            PostLED(ThisEvent);

            ThisEvent.EventType = ES_NEW_KEY;
            ThisEvent.EventParam = ' ';
            PostLED(ThisEvent);
            DB_printf("%c ", cur);

            MorseString[0] = '\0';
        }
            break;
        case BAD_SPACE:
        {
            MorseString[0] = '\0';
        }
            break;
        case BAD_PULSE:
        {
            MorseString[0] = '\0';
        }
            break;
        case BUTTON_DOWN:
        {
            MorseString[0] = '\0';
        }
            break;
        default:
        {
        }
            break;
    }

    return ReturnEvent;
}

/*------------------------------ Helper Functions ------------------------------*/

char DecodeMorseString(void) {
    ///DB_printf("String: %c%c%c%c", MorseString[0], MorseString[1], MorseString[2], MorseString[3]);
    for (int i = 0; i < 56; i++) {
        if ((strcmp((const char*) MorseString, (const char *) morseCode[i])) == 0) {
            return legalChars[i];
        }
    }
    return '~';
}
