/****************************************************************************
 Module
   MorseService.c

 Revision
   1.0.1

 Description
   This is the first service Morse Events 

 Notes

 ****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// This module
#include "../ProjectHeaders/MorseService.h"
#include "../ProjectHeaders/MorseDecode.h"

// Hardware
#include <xc.h>
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
bool Check4Morse(void); //prototype for checker function 
void TestCalibration(void);
void CharacterizeSpace(void);
void CharacterizePulse(void);

/*---------------------------- Module Variables ---------------------------*/
enum state {
    InitMorseElements, CalWaitForRise, CalWaitForFall, EOC_WaitRise, EOC_WaitFall, DecodeWaitFall, DecodeWaitRise
};

static uint8_t MyPriority;
static uint16_t CurrentState;
static uint16_t TimeOfLastRise;
static uint16_t TimeOfLastFall;
static uint16_t LengthOfDot;
static uint16_t FirstDelta;
static uint16_t LastInputState;


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
bool InitMorseService(uint8_t Priority) {
    ES_Event_t ThisEvent;

    MyPriority = Priority;

    // When doing testing, it is useful to announce just which program
    // is running.
    clrScrn();
    puts("\rStarting Morse Service for \r");
    DB_printf("the 2nd Generation Events & Services Framework V2.4\r\n");
    DB_printf("compiled at %s on %s\n", __TIME__, __DATE__);
    DB_printf("\n\r\n");
    DB_printf("Let's get started with calibration\n\r");

    //Initialize morse line to receive stuff: 
    TRISAbits.TRISA4 = 1; //set to output 
    //Initialize button line to receive stuff: 
    TRISAbits.TRISA3 = 1; //set to output 
    //Initialize global variables: 
    CurrentState = InitMorseElements;
    FirstDelta = 0;

    InitButtonStatus();

    // post the initial transition event
    ThisEvent.EventType = ES_INIT;
    if (ES_PostToService(MyPriority, ThisEvent) == true) {
        return true;
    } else {
        return false;
    }
}

/****************************************************************************
 Function
     PostMorseService

 Parameters
     ES_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 ****************************************************************************/
bool PostMorseService(ES_Event_t ThisEvent) {
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
ES_Event_t RunMorseService(ES_Event_t ThisEvent) {
    ES_Event_t ReturnEvent;
    ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

    uint16_t NextState = CurrentState;
    switch (CurrentState) {
        case InitMorseElements:
        {
            if (ThisEvent.EventType == ES_INIT) {
                NextState = CalWaitForRise;
                DB_printf("Calibrating....\n");

            }
        }
            break;
        case CalWaitForRise:
        {
            if (ThisEvent.EventType == MORSE_RISE) {
                TimeOfLastRise = ThisEvent.EventParam;

                NextState = CalWaitForFall;
            }
            if (ThisEvent.EventType == CALIBRATION_COMPLETE) {
                DB_printf("Length of dot: %d\n", LengthOfDot);
                NextState = EOC_WaitRise;
            }
        }
            break;
        case CalWaitForFall:
        {
            if (ThisEvent.EventType == MORSE_FALL) {
                TimeOfLastFall = ThisEvent.EventParam-5;
                NextState = CalWaitForRise;
                TestCalibration();
            }
        }
            break;
        case EOC_WaitRise:
        {
            if (ThisEvent.EventType == MORSE_RISE) {
                TimeOfLastRise = ThisEvent.EventParam;
                NextState = EOC_WaitFall;
                CharacterizeSpace();
            }
            if (ThisEvent.EventType == BUTTON_DOWN) {
                DB_printf("\nRecalibrating....\n");

                NextState = CalWaitForRise;
                FirstDelta = 0;
            }
        }
            break;
        case EOC_WaitFall:
        {
            if (ThisEvent.EventType == MORSE_FALL) {
                TimeOfLastFall = ThisEvent.EventParam-5 ;
                NextState = EOC_WaitRise;
            }
            if (ThisEvent.EventType == BUTTON_DOWN) {
                DB_printf("\nRecalibrating....\n");
                NextState = CalWaitForRise;
                FirstDelta = 0;
            }
            if (ThisEvent.EventType == EOC_DETECTED || ThisEvent.EventType == EOW_DETECTED) {
                NextState = DecodeWaitFall;
            }
        }
            break;
        case DecodeWaitRise:
        {
            if (ThisEvent.EventType == MORSE_RISE) {
                TimeOfLastRise = ThisEvent.EventParam;
                NextState = DecodeWaitFall;
                CharacterizeSpace();
            }
            if (ThisEvent.EventType == BUTTON_DOWN) {
                DB_printf("\nRecalibrating....\n");
                NextState = CalWaitForRise;
                FirstDelta = 0;
            }
        }
            break;

        case DecodeWaitFall:
        {
            if (ThisEvent.EventType == MORSE_FALL) {
                TimeOfLastFall = ThisEvent.EventParam -5 ;
                NextState = DecodeWaitRise;
                CharacterizePulse();
            }
            if (ThisEvent.EventType == BUTTON_DOWN) {
                DB_printf("\nRecalibrating....\n");
                NextState = CalWaitForRise;
                FirstDelta = 0;
            }
        }
            break;

        default:
        {
        }
            break;
    }
    CurrentState = NextState;

    return ReturnEvent;
}

/*------------------------------ Helper Functions ------------------------------*/

bool Check4Morse(void) {
    uint16_t CurrentInputState = PORTAbits.RA4;
    bool returnVal = false;
    if (CurrentInputState != LastInputState) {
        if (CurrentInputState == 0) {
            struct ES_Event ThisEvent;
            ThisEvent.EventType = MORSE_FALL;
            ThisEvent.EventParam = ES_Timer_GetTime();
            PostMorseService(ThisEvent);
            returnVal = true;
        }
        if (CurrentInputState == 1) {
            struct ES_Event ThisEvent;
            ThisEvent.EventType = MORSE_RISE;
            ThisEvent.EventParam = ES_Timer_GetTime();
            PostMorseService(ThisEvent);
            returnVal = true;
        }
    }
    LastInputState = CurrentInputState;
    return returnVal;
}

void TestCalibration(void) {
    uint16_t SecondDelta;
    if (FirstDelta == 0) {
        FirstDelta = TimeOfLastFall - TimeOfLastRise;
    } else {
        SecondDelta = TimeOfLastFall - TimeOfLastRise;
        uint16_t diff = 100.0 * FirstDelta / SecondDelta;
        //DB_printf("diff: %d\n", diff);
        if (diff <= 34) {
            LengthOfDot = FirstDelta;
            struct ES_Event ThisEvent;
            ThisEvent.EventType = CALIBRATION_COMPLETE;
            PostMorseService(ThisEvent);
        } else if (diff > 300) {
            LengthOfDot = SecondDelta;
            struct ES_Event ThisEvent;
            ThisEvent.EventType = CALIBRATION_COMPLETE;
            PostMorseService(ThisEvent);
        } else {
            FirstDelta = SecondDelta;
        }
    }

}

void CharacterizeSpace(void) {

    uint16_t LastInterval = TimeOfLastRise - TimeOfLastFall;
    struct ES_Event Event2Post;

    if ((LastInterval <= LengthOfDot - 10) || (LastInterval >= (LengthOfDot + 10))) {//if last interval ok for dot  
        if ((LastInterval >= 3 * (LengthOfDot - 10)) && (LastInterval <= 3 * (LengthOfDot + 10))) { //if last interval ok for charcter space 

            struct ES_Event ThisEvent;
            ThisEvent.EventType = EOC_DETECTED;
            PostMorseService(ThisEvent);
            PostMorseDecode(ThisEvent);

        } else {
            if ((LastInterval >= 7 * (LengthOfDot - 10)) && (LastInterval <= 7 * (LengthOfDot + 10))) { //if last interval ok for word space 
                struct ES_Event ThisEvent;
                ThisEvent.EventType = EOW_DETECTED;
                PostMorseService(ThisEvent);
                PostMorseDecode(ThisEvent);

            } else {
                struct ES_Event ThisEvent;
                ThisEvent.EventType = BAD_SPACE;
                PostMorseService(ThisEvent);
                PostMorseDecode(ThisEvent);

            }
        }
    }
}

void CharacterizePulse(void) {
    uint16_t LastPulseWidth = TimeOfLastFall - TimeOfLastRise;
    struct ES_Event Event2Post;
    if ((LastPulseWidth >= LengthOfDot - 10) && (LastPulseWidth <= (LengthOfDot + 10))) {//if last interval ok for dot  
        struct ES_Event ThisEvent;
        ThisEvent.EventType = DOT_DETECTED;
        PostMorseDecode(ThisEvent);

    } else {
        if ((LastPulseWidth >= 3 * (LengthOfDot - 10)) && (LastPulseWidth <= 3 * (LengthOfDot + 10))) { //if last interval ok for dash  
            struct ES_Event ThisEvent;
            ThisEvent.EventType = DASH_DETECTED;
            PostMorseDecode(ThisEvent);

        } else {
            struct ES_Event ThisEvent;
            ThisEvent.EventType = BAD_PULSE;
            PostMorseService(ThisEvent);
            PostMorseDecode(ThisEvent);

        }
    }
}