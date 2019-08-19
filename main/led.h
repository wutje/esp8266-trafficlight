/*
   SPDX-License-Identifier: GPL-3.0-or-later
*/
#ifndef LED_H
#define LED_H

enum Led {
    FRONT_GREEN   =  0,
    FRONT_ORANGE  =  8,
    FRONT_RED     =  6,

    REAR_GREEN    =  1,
    REAR_ORANGE   =  3,
    REAR_RED      =  4,
    
    PEDESTRIAN_RIGHT_RED = 2,   
    PEDESTRIAN_RIGHT_GREEN = 9,   
    
    PEDESTRIAN_LEFT_RED = 5,   
    PEDESTRIAN_LEFT_GREEN = 7,   
};

void led_init(void);
void led_all_off(void);
void led_all_on(void);
void led_set(enum Led lednr);
void led_set_mask(enum Led led_mask);
#endif
