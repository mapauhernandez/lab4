/****************************************************************************
 Module
   TestHarnessService0.c

 Revision
   1.0.1

 Description
   This is the first service Morse Events 

 Notes

 ****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// This module
#include "../ProjectHeaders/MorseService.h"

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

/*---------------------------- Module Variables ---------------------------*/
enum state {
    InitMorseElements, CalWaitForRise, CalWaitForFall, EOC_WaitRise
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
    DB_printf("Wait for Falling or Rising Morse Events\n\r");

    //Initialize morse line to receive stuff: 
    TRISAbits.TRISA4 = 1; //set to output 

    //Initialize global variables: 
    CurrentState = InitMorseElements;
    FirstDelta = 0;

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
                DB_printf("Got initialized!\n");
                NextState = CalWaitForRise;
            }
        }
            break;
        case CalWaitForRise: // announce
        {
            if (ThisEvent.EventType == MORSE_RISE) {
                TimeOfLastRise = ThisEvent.EventParam;
                DB_printf("RISE, at %d\n", TimeOfLastRise);

                NextState = CalWaitForFall;
            }
            if (ThisEvent.EventType == CALIBRATION_COMPLETE) {
                DB_printf("Length of Dot: %d\n", LengthOfDot);
                NextState = EOC_WaitRise;
            }
        }
            break;
        case CalWaitForFall: // announce
        {
            if (ThisEvent.EventType == MORSE_FALL) {
                TimeOfLastFall = ThisEvent.EventParam;
                DB_printf("FALL, at %d\n", TimeOfLastFall);

                NextState = CalWaitForRise;
                TestCalibration();
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
        DB_printf("First Delta: %d \n", FirstDelta);
        DB_printf("Second Delta: %d \n", SecondDelta);

        if (100.0 * FirstDelta / SecondDelta <= 33.33) {
            LengthOfDot = FirstDelta;
            struct ES_Event ThisEvent;
            ThisEvent.EventType = CALIBRATION_COMPLETE;
            PostMorseService(ThisEvent);

        } else if (100.0 * FirstDelta / SecondDelta > 300) {
            LengthOfDot = SecondDelta;
            struct ES_Event ThisEvent;
            ThisEvent.EventType = CALIBRATION_COMPLETE;
            PostMorseService(ThisEvent);
        } else {
            FirstDelta = SecondDelta;
        }
    }

}
