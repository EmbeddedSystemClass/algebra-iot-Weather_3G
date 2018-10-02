/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
 *
 **/

/*------------------------- INCLUDED FILES ************************************/
#include "ssLed.h"
#include "FreeRTOS.h"
#include "bsp.h"

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/

/*------------------------- TYPE DEFINITIONS ---------------------------------*/

/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PRIVATE VARIABLES --------------------------------*/

/*------------------------- PRIVATE FUNCTION PROTOTYPES ----------------------*/

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/


void ssLedRgbColourSet(uint32_t rgb)
{
  uint32_t r, g, b;
  r = (rgb & 0xFF0000) >> 16;
  g = (rgb & 0x00FF00) >> 8;
  b = (rgb & 0x0000FF);
  BSP_Led_Pwm_Control(BSP_RGB_LED_RED, (r * 100)/255);
  BSP_Led_Pwm_Control(BSP_RGB_LED_GREEN, (g* 100)/255);
  BSP_Led_Pwm_Control(BSP_RGB_LED_BLUE, (b * 100)/255);
}

void ssLedHsiColourSet(led_hsi_t *hsi)
{
  uint32_t rgb;
  
  rgb = ssLedHsi2Rgb(hsi);
  ssLedRgbColourSet(rgb);
}


uint32_t ssLedHsi2Rgb(led_hsi_t const *hsi)
{
  float H, S, I, R, G, B;
  uint32_t rgb;
  
  H = (float)hsi->h/360.;
  S = (float)hsi->s/100.0;
  I = (float)hsi->i/100.0;
  
  if (I==0.0) {
    // black image
    R = G = B = 0;
  }
  else {
    if (S==0.0) {
      // grayscale image
      R = G = B = I;
    }
    else {
      double domainOffset = 0.0;
      if (H<1.0/6.0) {  // red domain; green acending
        domainOffset = H;
        R = I;
        B = I * (1-S);
        G = B + (I-B)*domainOffset*6;
      }
      else {
        if (H<2.0/6) {  // yellow domain; red acending
          domainOffset = H - 1.0/6.0;
          G = I;
          B = I * (1-S);
          R = G - (I-B)*domainOffset*6;
        }
        else {
          if (H<3.0/6) {  // green domain; blue descending
            domainOffset = H-2.0/6;
            G = I;
            R = I * (1-S);
            B = R + (I-R)*domainOffset * 6;
          }
          else {
            if (H<4.0/6) {  // cyan domain, green acsending
              domainOffset = H - 3.0/6;
              B = I;
              R = I * (1-S);
              G = B - (I-R) * domainOffset * 6;
            }
            else {
              if (H<5.0/6) {  // blue domain, red ascending
                domainOffset = H - 4.0/6;
                B = I;
                G = I * (1-S);
                R = G + (I-G) * domainOffset * 6;
              }
              else {  // magenta domain, blue descending
                domainOffset = H - 5.0/6;
                R = I;
                G = I * (1-S);
                B = R - (I-G) * domainOffset * 6;
              }
            }
          }
        }
      }
    }
  }
  
  rgb = ((uint32_t)((uint8_t)(R*255)) << 16) +
        ((uint32_t)((uint8_t)(G*255)) << 8) +
        ((uint32_t)((uint8_t)(B*255)));
  
  return rgb;
}

/*------------------------- PRIVATE FUNCTION DEFINITIONS ---------------------*/


#ifdef __cplusplus
}
#endif



