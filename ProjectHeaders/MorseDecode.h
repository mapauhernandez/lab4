/****************************************************************************

  Header file for Morse Service
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef MorseDecode_H
#define MorseDecode_H

#include <stdint.h>
#include <stdbool.h>

#include "ES_Events.h"
#include "ES_Port.h"                // needed for definition of REENTRANT
// Public Function Prototypes

bool InitMorseDecode(uint8_t Priority);
bool PostMorseDecode(ES_Event_t ThisEvent);
ES_Event_t RunMorseDecode(ES_Event_t ThisEvent);


#endif /* ServTemplate_H */

