/****************************************************************************
 Module
   ButtonModule.c

 Revision
   1.0.1

 Description
 Button Module to check for button events
 ****************************************************************************/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_Events.h"
#include "ES_PostList.h"
#include "ES_ServiceHeaders.h"
#include "ES_Port.h"
#include "EventCheckers.h"
/****************************************************************************
 Function
   Check4Button
 Parameters
   None
 Returns
   bool: true if button state changes 
 Description
 checks for calibration button presses 
 ****************************************************************************/
//Global Variables: 
static uint16_t LastButtonState;

/****************************************************************************/

void InitButtonStatus(void){ 
    LastButtonState = PORTAbits.RA3;
    return; 
}

bool Check4Button(void) {
    bool ReturnVal = false;
    uint16_t CurrentButtonState = PORTAbits.RA3;

    if (CurrentButtonState != LastButtonState) {
        ReturnVal = true;
        if (CurrentButtonState == 1) {
            struct ES_Event ThisEvent;
            ThisEvent.EventType = BUTTON_DOWN;
            PostMorseService(ThisEvent);
        }
        LastButtonState = CurrentButtonState;
    }
    return ReturnVal;
}

