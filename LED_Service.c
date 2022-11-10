/****************************************************************************
 Module
   LED_Service.c

 Revision
   1.0.1

 Description
   This is the LED service for lab 4 part 3 

 Notes

/*----------------------------- Include Files -----------------------------*/
// This module
#include "LEDService.h"
#include "DM_Display.h"
#include "PIC32_SPI_HAL.h"


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


/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
// add a deferral queue for up to 3 pending deferrals +1 to allow for overhead
static ES_Event_t DeferralQueue[3 + 1];

enum state {
    InitLED, Ready4Char, Updating
};
static uint16_t CurrentState;

/*------------------------------ Module Code ------------------------------*/

/****************************************************************************
 Function
     InitLEDService

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
bool InitLEDService(uint8_t Priority) {

    ES_Event_t ThisEvent;

    MyPriority = Priority;

    SPISetup_BasicConfig(SPI_SPI1);

    //set leader

    SPISetup_SetLeader(SPI_SPI1, SPI_SMP_MID);


    SPISetup_MapSSOutput(SPI_SPI1, SPI_RPA0);

    SPISetup_MapSDOutput(SPI_SPI1, SPI_RPA1);

    //set clock idle bit

    SPISetup_SetClockIdleState(SPI_SPI1, SPI_CLK_HI);

    SPISetup_SetActiveEdge(SPI_SPI1, SPI_SECOND_EDGE);


    //set transfer width

    SPISetup_SetXferWidth(SPI_SPI1, SPI_16BIT);


    //set bit time

    SPISetup_SetBitTime(SPI_SPI1, 100000);

    //set enhanced buffer

    SPISetEnhancedBuffer(SPI_SPI1, true);


    //enable SPI

    SPISetup_EnableSPI(SPI_SPI1);

    // initialize deferral queue

    ES_InitDeferralQueueWith(DeferralQueue, ARRAY_SIZE(DeferralQueue));
    CurrentState = InitLED;

    ThisEvent.EventType = ES_INIT;
    if (ES_PostToService(MyPriority, ThisEvent) == true) {
        return true;
    } else {
        return false;
    }
}

/****************************************************************************
 Function
     PostLED

 Parameters
     ES_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 ****************************************************************************/
bool PostLED(ES_Event_t ThisEvent) {
    return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunLEDService

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes

 Author
   J. Edward Carryer, 01/15/12, 15:23
 ****************************************************************************/
ES_Event_t RunLEDService(ES_Event_t ThisEvent) {
    ES_Event_t ReturnEvent;
    ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

    uint16_t NextState = CurrentState;

    switch (CurrentState) {
        case InitLED: // announce
        {
            if (ThisEvent.EventType == ES_INIT) {
                while (false == DM_TakeInitDisplayStep()) {
                }
                NextState = Ready4Char;
            }

        }
            break;
        case Ready4Char:
        {

            if (ThisEvent.EventType == ES_NEW_KEY) {
                DM_ScrollDisplayBuffer(4);
                DM_AddChar2DisplayBuffer(ThisEvent.EventParam);
                NextState = Updating;
                struct ES_Event ThisEvent;
                ThisEvent.EventType = UPDATE_ROW;
                PostLED(ThisEvent);
            }

        }
            break;
        case Updating:
        {
            if (ThisEvent.EventType == ES_NEW_KEY) { //if we get a new key here add to deferral queue
                ES_DeferEvent(DeferralQueue, ThisEvent);
            }
            if (true == DM_TakeDisplayUpdateStep()) {
                NextState = Ready4Char;
                ES_RecallEvents(MyPriority, DeferralQueue);
                struct ES_Event ThisEvent;
                ThisEvent.EventType = FINISHED_UPDATE;
                PostLED(ThisEvent);
            } else {
                NextState = Updating;
                struct ES_Event ThisEvent;
                ThisEvent.EventType = UPDATE_ROW;
                PostLED(ThisEvent);
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

/***************************************************************************
 private functions
 ***************************************************************************/
