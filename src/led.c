/**
 * @file led.c
 * @author Honbo (hehongbo918@gmail.com)
 * @brief LED_BLINK_TIME: led blink period when stboot start
 * @version 1.0
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <stm32h7xx_hal.h>
#include <stdio.h>
#include "bsp.h"

static int stick;

void led_init(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOC_CLK_ENABLE();
/* PC13 for LED */
    gpio.Pin = GPIO_PIN_13;
    gpio.Pin = GPIO_PIN_13;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &gpio);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, 0);

    pr_info("led: gpioc-13 as triggered led");

    stick = HAL_GetTick();
}

void led_timer_handler(void)
{
    if (HAL_GetTick() - stick > LED_BLINK_TIME) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        stick = HAL_GetTick();
    }
}
