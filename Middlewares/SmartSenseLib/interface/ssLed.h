/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
 *
 **/

#ifndef _SS_LED_H
#define _SS_LED_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/

#define SS_LED_OK   0
#define SS_LED_ERR  1

/*------------------------- TYPE DEFINITIONS ---------------------------------*/
typedef struct
{
  uint8_t r;
  uint8_t g;
  uint8_t b;
} led_rgb_t;

typedef struct
{
  uint16_t h;
  uint8_t s;
  uint8_t i;
} led_hsi_t;

/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/
uint32_t ssLedHsi2Rgb(led_hsi_t const *hsi);
void ssLedHsiColourSet(led_hsi_t *hsi);
void ssLedRgbColourSet(uint32_t rgb);

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/


#ifdef __cplusplus
}
#endif

#endif /* _SS_LED_H */

