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

bool InitButtonStatus(void);
bool Check4Button(void); 

#endif /* ServTemplate_H */

