
/*----------------------------- Include Files -----------------------------*/
// This module
#include "DisplayLineService.h"
#include "LEDService.h"

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
#include "DM_Display.h"
#include "PIC32_SPI_HAL.h"

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
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
 */

static void InitLED(void);
static void BlinkLED(void);
#ifdef TEST_INT_POST
static void InitTMR2(void);
static void StartTMR2(void);
#endif
/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
// add a deferral queue for up to 3 pending deferrals +1 to allow for overhead
static ES_Event_t DeferralQueue[3 + 1];
static int counter = 0;

/*------------------------------ Module Code ------------------------------*/

/****************************************************************************
 Function
     InitTestHarnessService0

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any
     other required initialization for this service
 Notes

 Author
     J. Edward Carryer, 01/16/12, 10:00
 ****************************************************************************/
bool InitDisplayLineService(uint8_t Priority) {
    ES_Event_t ThisEvent;

    MyPriority = Priority;
}

/****************************************************************************
 Function
     PostTestHarnessService0

 Parameters
     ES_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 ****************************************************************************/
bool PostDisplayLineService(ES_Event_t ThisEvent) {
    return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunTestHarnessService0

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes
 ****************************************************************************/
ES_Event_t RunDisplayLineService(ES_Event_t ThisEvent) {
    ES_Event_t ReturnEvent;
    ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
    static char DeferredChar = '1';
    char* line = "Hi my name is Mapau and I love embedded programming";

#ifdef _INCLUDE_BYTE_DEBUG_
    _HW_ByteDebug_SetValueWithStrobe(ENTER_RUN);
#endif  
    switch (ThisEvent.EventType) {
        case SEND_CHAR: // re-start timer & announce
        {
            ES_Timer_InitTimer(SERVICE0_TIMER, ONE_SEC);

            char cur_char = line[counter];
            ThisEvent.EventType = ES_LED_EVENT;
            ThisEvent.EventParam = cur_char;
            PostPostLEDService(ThisEvent);
            counter++;
            if (line[counter] == '\0') {
                ThisEvent.EventType = FINISHED_STRING;
                PostPostLEDService(ThisEvent);
            }
        }
            break;
        default:
        {
        }
            break;
    }

    return ReturnEvent;
}

/***************************************************************************
 private functions
 ***************************************************************************/
#define LED LATBbits.LATB6

static void InitLED(void) {
    LED = 0; //start with it off
    TRISBbits.TRISB6 = 0; // set RB6 as an output
}

static void BlinkLED(void) {
    // toggle state of LED
    LED = ~LED;
}


// for testing posting from interrupts.
// Intializes TMR2 to gerenate an interrupt at 100ms

static void InitTMR2(void) {
    // turn timer off
    T2CONbits.ON = 0;
    // Use internal peripheral clock
    T2CONbits.TCS = 0;
    // setup for 16 bit mode
    T2CONbits.T32 = 0;
    // set prescale to 1:1
    T2CONbits.TCKPS = 0;
    // load period value
    PR2 = 2000 - 1; // creates a 100ms period with a 20MHz peripheral clock
    // set priority
    IPC2bits.T2IP = 2;
    // clear interrupt flag
    IFS0bits.T2IF = 0;
    // enable the timer interrupt
    IEC0bits.T2IE = 1;
}

// Clears and Starts TMR2

static void StartTMR2(void) {
    // clear timer
    TMR2 = 0;
    // start timer
    //LATBbits.LATB14 = 0;
    T2CONbits.ON = 1;
}
