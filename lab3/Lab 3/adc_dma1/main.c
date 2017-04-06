Hardik Patel
Alex Elkman
EEC172 Embedded Systems 
Spring 2016

//*****************************************************************************

//Standard includes
#include <stdlib.h>
#include <string.h>

// Driverlib includes
#include "hw_common_reg.h"
#include "hw_memmap.h"
#include "hw_apps_rcm.h"
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_uart.h"
#include "hw_adc.h"
#include "uart.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"
#include "systick.h"
#include "utils.h"
#include "udma.h"
#include "interrupt.h"
#include "adc.h"
#include "math.h"
#include "gpio.h"
#include "gpio_if.h"



//#include "systick.h"

// Common interface includes
#include "udma_if.h"
#include "uart_if.h"
#include "pin_mux_config.h"

#define APPLICATION_VERSION     "1.1.1"
#define APP_NAME                "ADC uDMA Example"
#define UART_PRINT              Report

//*****************************************************************************
//
// The size of the data buffer.
//
//*****************************************************************************
#define BUFFER_SIZE         1024

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************

// The destination buffers used for uDMA transfers.
volatile unsigned long g_ulPing[BUFFER_SIZE];
volatile unsigned long g_ulPong[BUFFER_SIZE];

volatile unsigned long g_ulPingCount;
volatile unsigned long g_ulPongCount;
//volatile unsigned long timerBuffer[BUFFER_SIZE];
volatile unsigned char g_ucPingflag;
volatile unsigned char g_ucPongflag;


// vector table entry
#if defined(ewarm)
    extern uVectorEntry __vector_table;
#endif

#if defined(ccs)
    extern void (* const g_pfnVectors[])(void);
#endif
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************



//*****************************************************************************
//!
//! The interrupt handler for ADC.  This interrupt will occur when a DMA
//! transfer is complete.
//!
//! \param None
//!
//! \return None
//*****************************************************************************
void ADCIntHandler(void)
{
	unsigned long ulMode;
	unsigned short Status;
	Status = ADCIntStatus(ADC_BASE, ADC_CH_3);
	ADCIntClear(ADC_BASE, ADC_CH_3, Status | ADC_DMA_DONE);

	//timer[1] = MAP_SysTickValueGet() - timer[0];
	//timer[0] = MAP_SysTickValueGet();

	ulMode = MAP_uDMAChannelModeGet(UDMA_CH17_ADC_CH3 | UDMA_PRI_SELECT);
	if (ulMode == UDMA_MODE_STOP) {
		g_ucPingflag=1;
		g_ulPingCount++;

		UDMASetupTransfer(UDMA_CH17_ADC_CH3 | UDMA_PRI_SELECT, UDMA_MODE_PINGPONG,
				BUFFER_SIZE, UDMA_SIZE_32, UDMA_ARB_1,
				(void *)(ADC_BASE + ADC_O_channel6FIFODATA), UDMA_SRC_INC_NONE,
				(void *)&(g_ulPing[0]), UDMA_DST_INC_32);
	}

	ulMode = MAP_uDMAChannelModeGet(UDMA_CH17_ADC_CH3 | UDMA_ALT_SELECT);
	if	(ulMode == UDMA_MODE_STOP) {
		g_ucPongflag=1;
		g_ulPongCount++;

		UDMASetupTransfer(UDMA_CH17_ADC_CH3 | UDMA_ALT_SELECT, UDMA_MODE_PINGPONG,
					BUFFER_SIZE, UDMA_SIZE_32, UDMA_ARB_1,
					(void *)(ADC_BASE + ADC_O_channel6FIFODATA), UDMA_SRC_INC_NONE,
					(void *)&(g_ulPong[0]), UDMA_DST_INC_32);

	}

}
//*****************************************************************************

void InitAdcDma( void )
{
	unsigned short Status;

	UDMAInit();

	MAP_uDMAChannelAssign(UDMA_CH17_ADC_CH3);

	UDMASetupTransfer(UDMA_CH17_ADC_CH3 | UDMA_PRI_SELECT, UDMA_MODE_PINGPONG,
			BUFFER_SIZE, UDMA_SIZE_32, UDMA_ARB_1,
			(void *)(ADC_BASE + ADC_O_channel6FIFODATA), UDMA_SRC_INC_NONE,
			(void *)&(g_ulPing[0]), UDMA_DST_INC_32);

	UDMASetupTransfer(UDMA_CH17_ADC_CH3 | UDMA_ALT_SELECT, UDMA_MODE_PINGPONG,
			BUFFER_SIZE, UDMA_SIZE_32, UDMA_ARB_1,
			(void *)(ADC_BASE + ADC_O_channel6FIFODATA), UDMA_SRC_INC_NONE,
			(void *)&(g_ulPong[0]), UDMA_DST_INC_32);

	g_ulPingCount=0;
	g_ulPongCount=0;
	g_ucPingflag=0;
	g_ucPongflag=0;
	ADCDMAEnable(ADC_BASE, ADC_CH_3);
	ADCIntRegister(ADC_BASE, ADC_CH_3, ADCIntHandler);
	Status = ADCIntStatus(ADC_BASE, ADC_CH_3);
	ADCIntClear(ADC_BASE, ADC_CH_3, Status | ADC_DMA_DONE);
	ADCIntEnable(ADC_BASE, ADC_CH_3, ADC_DMA_DONE);
	ADCEnable(ADC_BASE);
	ADCChannelEnable(ADC_BASE, ADC_CH_3);
}

//*****************************************************************************
//
//! Application startup display on UART
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void
DisplayBanner()
{
    UART_PRINT("\n\n\n\r");
    UART_PRINT("\t\t   *******************************************\n\r");
    UART_PRINT("\t\t        CC3200 %s Application        \n\r", APP_NAME);
    UART_PRINT("\t\t   *******************************************\n\r");
    UART_PRINT("\n\n\n\r");

}


float roundToHundredths(float x){
      x *=10000;
       return floor(x) / 10000;
}

//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
  //
  // Set vector table base
  //
#if defined(ccs)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
  //
  // Enable Processor
  //
  MAP_IntMasterEnable();
  MAP_IntEnable(FAULT_SYSTICK);

  PRCMCC3200MCUInit();
}


//*****************************************************************************
//
//! Main function for uDMA Application 
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void main() {
	unsigned int uiIndex;
	unsigned char Inchar;

    //
    // Initailizing the board
    //
    BoardInit();
    //
    // Muxing for Enabling UART_TX and UART_RX and ADC Channel 3 (pin 60)
    //
    PinMuxConfig();
    GPIO_IF_LedConfigure(LED1|LED2|LED3);
    GPIO_IF_LedOff(MCU_ALL_LED_IND);
    //
    // Initialising the Terminal.
    //
    InitTerm();

    //
    // Display Welcome Message
    //
    DisplayBanner();
    UART_PRINT("Make sure analog signal in range 0 - 1.4V\n\r");
    UART_PRINT("Simple test program for Ping-Pong DMA;\r\n");
    UART_PRINT("Displays just a small portion of buffers\r\n");
    UART_PRINT("Press any key to start\r\n");
    UART_PRINT("Press 'q' key to stop\r\n");

    while(MAP_UARTCharsAvail(UARTA0_BASE) == false)  { ; }
    Inchar = MAP_UARTCharGetNonBlocking(UARTA0_BASE);
    int sin =1;
    InitAdcDma();
    int i;
    //MAP_SysTickPeriodSet(16777216);
    //MAP_SysTickEnable();
    Inchar = ' ';
    do {

    	if (g_ucPingflag) {
    		g_ucPingflag=0;
    		UART_PRINT("Ping: \r\n");
    		uiIndex=0;
    		while (uiIndex < 24) {
    			UART_PRINT("%f\t",(((float)((g_ulPing[uiIndex] >> 2 ) & 0x0FFF))*1.467)/4096);
    			uiIndex++;
    			if ((uiIndex % 4)==0)
    				UART_PRINT("\n\r");
    		}

    	/*	if(square){
    			int tempOld = 0;
    			int flag = 0;
				int count = 0;
				int temp = 0;
				for(i=0; i<1024; i++){
					if(flag == 1)
						count++;
					else if (flag == 2)
						break;
					temp = (int)((((((float)((g_ulPing[i] >> 2 ) & 0x0FFF))*1.467)/4096)) + 0.5);
					if(temp == tempOld)
						flag++;
					if((((((float)((g_ulPing[i] >> 2 ) & 0x0FFF))*1.467)/4096)) > .6)
						tempOld = 1;
					if((((((float)((g_ulPing[i] >> 2 ) & 0x0FFF))*1.467)/4096)) < .6)
						tempOld = 0;

				}

				UART_PRINT("Frequency is: %d\n\r", 62500/count);
			}*/

    		int highFlag = 0;
    		int lowFlag = 0;
    		if(sin){
    			int A=0, B=0, C=0;
    			for(i=0; i<1024; i++){

    				if(((((((float)((g_ulPing[i] >> 2 ) & 0x0FFF))*1.467)/4096)) > .99) && B==0)
    			    					highFlag = 1;
    				else if(((((((float)((g_ulPing[i] >> 2 ) & 0x0FFF))*1.467)/4096)) < .99) && A==0)
    				    				lowFlag = 1;

					if(highFlag ==1 && lowFlag ==1){
						if(((((((float)((g_ulPing[i] >> 2 ) & 0x0FFF))*1.467)/4096)) > .99) && B==0)
							A++;
						else if(((((((float)((g_ulPing[i] >> 2 ) & 0x0FFF))*1.467)/4096)) < .99) && A!=0)
							B++;
						else if(((((((float)((g_ulPing[i] >> 2 ) & 0x0FFF))*1.467)/4096)) > .99) && B!=0){
							break;
						}
					}
    			}
    			UART_PRINT("A is: %d \n\r ", A);
    			if(A<3){
    				C = B+A;
    			    GPIO_IF_LedOff(MCU_ALL_LED_IND);
    				UART_PRINT("Frequency is: %d  and wave is: ", 62500/C, A);
    				UART_PRINT("TRIANGLE\n\r" );
    				GPIO_IF_LedOn(MCU_ORANGE_LED_GPIO);

    			}
    			else if(A>=3 && A<11){
    				C = B+A;
    			    GPIO_IF_LedOff(MCU_ALL_LED_IND);
    				UART_PRINT("Frequency is: %d  and wave is: ", 62500/C, A);
    				UART_PRINT("SINE \n\r" );
    				GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);
    			}
    			else{
					C = B+A;
				    GPIO_IF_LedOff(MCU_ALL_LED_IND);
					UART_PRINT("Frequency is: %d  and wave is: ", 62500/C, A);
    				UART_PRINT("SQUARE \n\r" );
    				GPIO_IF_LedOn(MCU_RED_LED_GPIO);
    			}

    		}



    	}

    	if (g_ucPongflag) {
    		g_ucPongflag=0;
    		UART_PRINT("Pong: \r\n");
    		uiIndex=0;
    		while (uiIndex < 24) {
    			UART_PRINT("%f\t",(((float)((g_ulPong[uiIndex] >> 2 ) & 0x0FFF))*1.467)/4096);
    			uiIndex++;
    			if ((uiIndex % 4)==0)
    				UART_PRINT("\n\r");
    		}
    		//UART_PRINT("Spot 500: %f\t\n\r",(((float)((g_ulPong[500] >> 2 ) & 0x0FFF))*1.467)/4096);
    	}
        if (MAP_UARTCharsAvail(UARTA0_BASE))  {
        	Inchar = MAP_UARTCharGetNonBlocking(UARTA0_BASE);
        }
   } while (Inchar != 'q');
    UART_PRINT("Program stopped\r\n");
	ADCDMADisable(ADC_BASE, ADC_CH_3);
	ADCChannelDisable(ADC_BASE, ADC_CH_3);
}


//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************




