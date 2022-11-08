/****************************************************************************

  Header file for Morse Service
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef MorseService_H
#define MorseService_H_H

#include <stdint.h>
#include <stdbool.h>

#include "ES_Events.h"
#include "ES_Port.h"                // needed for definition of REENTRANT
// Public Function Prototypes

bool InitMorseService(uint8_t Priority);
bool PostMorseService(ES_Event_t ThisEvent);
ES_Event_t RunMorseService(ES_Event_t ThisEvent);

bool Check4Morse(void) ;


#endif /* ServTemplate_H */

