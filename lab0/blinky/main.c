//Alex Elkman, Hardik Patel
//EEC 172 
//Spring 2016

//! \addtogroup blinky
//! @{

// Standard includes
#include <stdio.h>

// Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "interrupt.h"
#include "hw_apps_rcm.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"
#include "prcm.h"
#include "gpio.h"
#include "utils.h"

// Common interface includes
#include "gpio_if.h"
#include "pin_mux_config.h"
#define APPLICATION_VERSION     "1.1.1"
#if defined(ccs)
	extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
	extern uVectorEntry __vector_table;
#endif

// Global switch variables for error checking
int sw2=0;
int sw3=0;

//initialize board function
static void BoardInit(void)
	{
	/* In case of TI-RTOS vector table is initialize by OS itself */
	#ifndef USE_TIRTOS
		
	// Set vector table base
	#if defined(ccs)
		MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
	#endif
	#if defined(ewarm)
		MAP_IntVTableBaseSet((unsigned long)&__vector_table);
	#endif
	#endif
		
		// Enable Processor
		MAP_IntMasterEnable();
		MAP_IntEnable(FAULT_SYSTICK);

		PRCMCC3200MCUInit();
	}

//gets signals from switches sets to global variables for error checking
void getSwitches()		
{
    sw2 = GPIOPinRead(GPIOA2_BASE, 0x40);
    sw3 = GPIOPinRead(GPIOA1_BASE, 0x20);
}

int
main()
{
	//start the board and set the pin_mux_config
	BoardInit();
	PinMuxConfig();
	
	//start with LEDs off, and pin18 off
	GPIO_IF_LedConfigure(LED1|LED2|LED3);
	GPIO_IF_LedOff(MCU_ALL_LED_IND);
	GPIOPinWrite(GPIOA3_BASE, 0x10, 0x00); //0xFF for on, 0x00 for off
	
	//define states 
	typedef enum {s0, s1, s2, s3, s4} states;
    states state = s0;
    
	//loop through state machine until state 4 is reached
	while(state != s4){

		//state machine definition follow Moore Diagram (included)
		switch(state){

			case s0:	//PRIMARY (RESET) STATE
				//turns on Green LED
				GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);
				
				//if both switches are OFF, will stay in state 0
				if(sw2 == 0 && sw3 == 0){
					state = s0; break; }
				
				//if sw2 is pressed ONLY, then go to s1
				else if(sw3 == 0 && sw2 != 0){
					state = s1; break; }
				
				//any other switch assignments will result in ERROR state
				else {state = s4; break;}
				
				//check switches for the next state
				getSwitches();
				break;

			case s1:
				//turn off GREEN and on YELLOW LEDS
				GPIO_IF_LedOff(MCU_GREEN_LED_GPIO);
				GPIO_IF_LedOn(MCU_ORANGE_LED_GPIO);

				//if sw2 is ON, stay in state 1
				if(sw3 == 0 && sw2 != 0)
					state = s1;
				
				//if both switches are pressed, proceed to state 2
				else if(sw2 != 0 && sw3 != 0)
					state = s2;
				
				//any other switch assignment will result in ERROR state
				else
					state = s4;
				
				//check switches for the next state
				getSwitches();
				break;

			case s2:
				//turn on GREEN LED
				GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);
				
				//if both switches are still pressed, stay in state 2
				if(sw2 != 0 && sw3 != 0)
					state = s2;
				
				//if switch 3 is pressed only, proceed to SUCCESS state 3
				else if(sw3 != 0 && sw2 == 0)
					state = s3;
				
				//any other switch configuration results in ERROR state
				else
					state = s4;
				
				//check switches for next state
				getSwitches();
				break;

			case s3:	//SUCCESS STATE
				//turn on the RED LED
				GPIO_IF_LedOn(MCU_RED_LED_GPIO);
				
				//toggle pin18 on/off once we have reached state 3, we'll stay in this loop forever
				while(s3){
					MAP_UtilsDelay(1000000);
					GPIOPinWrite(GPIOA3_BASE, 0x10, 0x00);
					MAP_UtilsDelay(1000000);
					GPIOPinWrite(GPIOA3_BASE, 0x10, 0xFF);
				}
				break;

			case s4:	//ERROR STATE
				//turn on only the RED LED, all others will be turned off
				GPIO_IF_LedOff(MCU_GREEN_LED_GPIO);
				GPIO_IF_LedOff(MCU_ORANGE_LED_GPIO);
				GPIO_IF_LedOn(MCU_RED_LED_GPIO);
				
				//we'll stay in this state indefinitely
				state = s4;
				break;
		
			//error checking
			default:
				printf("ERROR");
				break;
    	}
    }
    return 0;
}
