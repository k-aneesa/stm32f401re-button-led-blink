/*
 * btnled_exled_demo.c
 *
 *  Created on: Sep 4, 2026
 *      Author: ANEESA
 */
#include <stdint.h>

#include "stm32f401RE.h"
#include"stm32f401RE_gpio.h"

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif

 /*main.c
 *
 *Task 1: Onboard user button (B1 / PC13) toggles onboard user LED (LD2 / PA5).
 * Task 2: Two external LEDs on PA6 and PA7 blink alternately, 250ms ON / 250ms
 *         OFF each (500ms shared period), 180 degrees out of phase.
 * nly driver APIs (GPIO_PeriClockControl, GPIO_Init, GPIO_WritetoOutputPin,
 * GPIO_ReadFromInputPin, GPIO_ToggleOutputPin) are used for GPIO setup and
 * I/O -- no direct register access anywhere in this file.
 */
//#include "stm32f401RE_gpio.h"

/* ---------------------------------------------------------------------
 * Pin definitions
 * ---------------------------------------------------------------------
 * LD2 (onboard user LED)    -> GPIOA Pin 5  (standard on all Nucleo-64 boards)
 * B1  (onboard user button) -> GPIOC Pin 13 (standard on all Nucleo-64 boards,
 *                               active-LOW: pressed = pulled to GND)
 * EXT_LED1 -> GPIOA Pin 6
 * EXT_LED2 -> GPIOA Pin 7
 */

#define LED_ONBOARD_PORT     GPIOA
#define LED_ONBOARD_PIN      GPIO_PinNo_5

#define BUTTON_PORT          GPIOC
#define BUTTON_PIN           GPIO_PinNo_13

#define EXT_LED1_PORT        GPIOA
#define EXT_LED1_PIN         GPIO_PinNo_6

#define EXT_LED2_PORT        GPIOA
#define EXT_LED2_PIN         GPIO_PinNo_7

void delay_ms(uint32_t ms)
{
	for(uint32_t i=0;i< ms*4000; i++);

}

void delay_debounce(void)
{
	for(uint32_t i=0;i< 50000; i++);

}

static void LED_Onboard_Init(void)
{
	GPIO_Handle_t ledHandle={0};

	ledHandle.pGPIOx = LED_ONBOARD_PORT  ;
	ledHandle.GPIO_PinConfig.GPIO_PinNumber = LED_ONBOARD_PIN;
	ledHandle.GPIO_PinConfig.GPIO_PinMode   = GPIO_MODE_OUT;
	ledHandle.GPIO_PinConfig.GPIO_PinSpeed  = GPIO_SPEED_LOW;
	ledHandle.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
	ledHandle.GPIO_PinConfig.GPIO_PinPuPd	= GPIO_NO_PUPD;

	GPIO_PeriClockControl(LED_ONBOARD_PORT, ENABLE);
	GPIO_Init(&ledHandle);

	/*Start from the LED off so the toggle logic starts from a  known state*/
	GPIO_WritetoOutputPin(LED_ONBOARD_PORT, LED_ONBOARD_PIN, GPIO_PIN_RESET);
}
static void Button_Init(void)
{
    GPIO_Handle_t btnHandle = {0};

    btnHandle.pGPIOx = BUTTON_PORT;
    btnHandle.GPIO_PinConfig.GPIO_PinNumber = BUTTON_PIN;
    btnHandle.GPIO_PinConfig.GPIO_PinMode   = GPIO_MODE_IN;
    btnHandle.GPIO_PinConfig.GPIO_PinSpeed  = GPIO_SPEED_LOW;

    /* GPIO_NO_PU == internal pull-UP enabled this MUST be
     * enabled here or the pin will float and reads will be unreliable. */
    btnHandle.GPIO_PinConfig.GPIO_PinPuPd   = GPIO_NO_PU;

    GPIO_PeriClockControl(BUTTON_PORT, ENABLE);
    GPIO_Init(&btnHandle);

}



static void ExtLeds_Init(void)
{
    GPIO_Handle_t led1Handle = {0};
    GPIO_Handle_t led2Handle = {0};

    /* PA6 */
    led1Handle.pGPIOx = EXT_LED1_PORT;
    led1Handle.GPIO_PinConfig.GPIO_PinNumber = EXT_LED1_PIN;
    led1Handle.GPIO_PinConfig.GPIO_PinMode   = GPIO_MODE_OUT;
    led1Handle.GPIO_PinConfig.GPIO_PinSpeed  = GPIO_SPEED_LOW;
    led1Handle.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
    led1Handle.GPIO_PinConfig.GPIO_PinPuPd   = GPIO_NO_PUPD;

    GPIO_PeriClockControl(EXT_LED1_PORT, ENABLE);
    GPIO_Init(&led1Handle);

    /*PA7 same port as PA6, but initiialised via
     *  its own handle in case the pins are moved to
     *  different ports later*/
    led2Handle.pGPIOx = EXT_LED2_PORT;
    led2Handle.GPIO_PinConfig.GPIO_PinNumber = EXT_LED2_PIN;
    led2Handle.GPIO_PinConfig.GPIO_PinMode   = GPIO_MODE_OUT;
    led2Handle.GPIO_PinConfig.GPIO_PinSpeed  = GPIO_SPEED_LOW;
    led2Handle.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
    led2Handle.GPIO_PinConfig.GPIO_PinPuPd   = GPIO_NO_PUPD;

    GPIO_PeriClockControl(EXT_LED2_PORT, ENABLE);
    GPIO_Init(&led2Handle);

    /* Start both external LEDs off */
     GPIO_WritetoOutputPin(EXT_LED1_PORT, EXT_LED1_PIN, GPIO_PIN_RESET);
     GPIO_WritetoOutputPin(EXT_LED2_PORT, EXT_LED2_PIN, GPIO_PIN_RESET);

}

/*Task 1: Poll the buttton once per main loop iteration  debounce,
 * and toggle the onboard LED on a clean press then release.*/

static void Handle_Button_Toggle(void)
{
    if (GPIO_ReadFromInputPin(BUTTON_PORT, BUTTON_PIN) == GPIO_PIN_RESET)
    {
        /* Button reads pressed (active-low). Wait out mechanical bounce,
         * then confirm it is still pressed before acting. */
        delay_debounce();

     if (GPIO_ReadFromInputPin(BUTTON_PORT, BUTTON_PIN) == GPIO_PIN_RESET)
     {
         /* Confirmed press: toggle the LED exactly once. */
         GPIO_ToggleOutputPin(LED_ONBOARD_PORT, LED_ONBOARD_PIN);

         /* Block until the button is released (debounced again). This
          * is what makes "single press -> single toggle" reliable: we
          * don't re-enter this branch until the user has physically
          * let go, so one press cannot be read as many presses even if
          * held down. */
            while (GPIO_ReadFromInputPin(BUTTON_PORT, BUTTON_PIN) == GPIO_PIN_RESET);


            delay_debounce();
        }
    }
}


/*Task 2: Alternate the two external LEDs, 250ms each, out of phase*/

static void Blink_External_Leds_Step(void)
{
    /* LED1 on, LED2 off */
    GPIO_WritetoOutputPin(EXT_LED1_PORT, EXT_LED1_PIN, GPIO_PIN_SET);
    GPIO_WritetoOutputPin(EXT_LED2_PORT, EXT_LED2_PIN, GPIO_PIN_RESET);
    delay_ms(250);

    /* LED1 off, LED2 on */
    GPIO_WritetoOutputPin(EXT_LED1_PORT, EXT_LED1_PIN, GPIO_PIN_RESET);
    GPIO_WritetoOutputPin(EXT_LED2_PORT, EXT_LED2_PIN, GPIO_PIN_SET);
    delay_ms(250);
}

int main(void)
{
	LED_Onboard_Init();
	Button_Init();
	ExtLeds_Init();
	while(1)
	{
		Handle_Button_Toggle(); // Task1

		Blink_External_Leds_Step(); // Task2
	}
	return 0;
}

