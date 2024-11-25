/**
 * @file led.c
 * @author Honbo (hehongbo918@gmail.com)
 * @brief blink `LED_BLINK_TIMES` times when stboot start
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
        GPIO_InitTypeDef gpio_cfg = {0};

        __HAL_RCC_GPIOC_CLK_ENABLE();

/* PC13 for LED */
        gpio_cfg.Pin = GPIO_PIN_13;
        gpio_cfg.Mode = GPIO_MODE_OUTPUT_PP;
        gpio_cfg.Pull = GPIO_PULLUP;
        gpio_cfg.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOC, &gpio_cfg);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, 1);

        pr_info("led: led enabled");

        stick = HAL_GetTick();
}

void led_timer_handler(void)
{
        int etick = HAL_GetTick();

        if (etick - stick > LED_BLINK_TIME) {
                HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
                stick = etick;
        }
}
