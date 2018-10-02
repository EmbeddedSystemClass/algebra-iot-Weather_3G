/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
 *
 **/

#ifndef _SS_SYSTIME_H
#define _SS_SYSTIME_H

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/

#define MILLISECONDS_TO_OS_TICKS(milliseconds)    ((milliseconds)/portTICK_PERIOD_MS)
#define OS_TICKS_TO_MILLISECONDS(ticks)           ((ticks)*portTICK_PERIOD_MS)

/*------------------------- TYPE DEFINITIONS ---------------------------------*/

/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/

  
/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

#ifdef __cplusplus
}
#endif

#endif /* _TEMPLATE_H */
 
