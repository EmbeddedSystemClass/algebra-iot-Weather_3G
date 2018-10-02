/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
 *
 **/

#ifndef _SS_SUPERVISION_H
#define _SS_SUPERVISION_H

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/
#define SS_SUPERVISION_TASK_CPID  0x06
/*------------------------- TYPE DEFINITIONS ---------------------------------*/

/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/
void ssSupervisionInit(void);
void ssSupervisionReboot(void);

#ifdef __cplusplus
}
#endif

#endif /* _SS_SUPERVISION_H */
 