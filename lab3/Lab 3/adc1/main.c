Hardik Patel
Alex Elkman
EEC172 Embedded Systems 
Spring 2016

// Standard includes
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

// Driverlib includes
#include "utils.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "hw_types.h"
#include "hw_adc.h"
#include "hw_ints.h"
#include "hw_gprcm.h"
#include "rom.h"
#include "rom_map.h"
#include "interrupt.h"
#include "prcm.h"
#include "uart.h"
#include "pin_mux_config.h"
#include "pin.h"
#include "adc.h"
#include "spi.h"
#include "uart_if.h"

#define USER_INPUT 
#define UART_PRINT         Report
#define FOREVER            1
#define APP_NAME           "ADC Reference"
#define NO_OF_SAMPLES 		64
#define SPI_IF_BIT_RATE  100000
unsigned long pulAdcSamples[4096];

//*****************************************************************************
//                      GLOBAL VARIABLES
//*****************************************************************************
#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif

/****************************************************************************/
/*                      LOCAL FUNCTION PROTOTYPES                           */
/****************************************************************************/
static void BoardInit(void);
static void DisplayBanner(char * AppName);

//*****************************************************************************
//
//! Application startup display on UART
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
static void
DisplayBanner(char * AppName)
{
    Report("\n\n\n\r");
    Report("\t\t *************************************************\n\r");
    Report("\t\t       CC3200 %s Application       \n\r", AppName);
    Report("\t\t *************************************************\n\r");
    Report("\n\n\n\r");
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
//! main - calls Crypt function after populating either from pre- defined vector 
//! or from User
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void 
main()
{
	float temp;
    unsigned int  uiIndex=0;
    unsigned long ulSample;
    unsigned long temp2;
    unsigned long temp3;
    //
    // Initialize Board configurations
    //
    BoardInit();

    //adc
    // Configuring UART for Receiving input and displaying output
    // 1. PinMux setting
    // 2. Initialize UART
    // 3. Displaying Banner
    //
    PinMuxConfig();
    InitTerm();
    DisplayBanner(APP_NAME);

        //
        // Initialize Array index for multiple execution
        //
        uiIndex=0;
      

        //
        // Configure ADC timer which is used to timestamp the ADC data samples
        //
 //       MAP_ADCTimerConfig(ADC_BASE,2^17);
        MAP_PRCMPeripheralClkEnable(PRCM_GSPI,PRCM_RUN_MODE_CLK);
      	MAP_PRCMPeripheralReset(PRCM_GSPI);
      	MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
      					SPI_IF_BIT_RATE,SPI_MODE_MASTER,SPI_SUB_MODE_0,
      					(SPI_SW_CTRL_CS |
      					SPI_4PIN_MODE |
      					SPI_TURBO_OFF |
      					SPI_CS_ACTIVELOW |
      					SPI_WL_16));
      	MAP_SPIEnable(GSPI_BASE);
      	MAP_SPICSEnable(GSPI_BASE);
        //
        // Enable ADC timer which is used to timestamp the ADC data samples
        //
//        MAP_ADCTimerEnable(ADC_BASE);

        //
        // Enable ADC module
        //
        MAP_ADCEnable(ADC_BASE);

        //
        // Enable ADC channel
        //






        //
        // Print out ADC samples
        //
while(1){
	  MAP_ADCChannelEnable(ADC_BASE, ADC_CH_3);
    while(uiIndex < NO_OF_SAMPLES + 4)
    {
        if(MAP_ADCFIFOLvlGet(ADC_BASE, ADC_CH_3))
        {
            ulSample = MAP_ADCFIFORead(ADC_BASE, ADC_CH_3);
            pulAdcSamples[uiIndex++] = ulSample;
        }


    }

    MAP_ADCChannelDisable(ADC_BASE, ADC_CH_3);

    uiIndex = 0;

        while(uiIndex < NO_OF_SAMPLES)
        {
        	temp = (((float)((pulAdcSamples[4+uiIndex] >> 2 ) & 0xFFFF))*1.4)/4096+temp;
            UART_PRINT("%f\t",(((float)((pulAdcSamples[4+uiIndex] >> 2 ) & 0xFFFF))*1.4)/4096);
            uiIndex++;

            if ((uiIndex % 4)==0)
            	UART_PRINT("\n\r");
        }
        uiIndex = 0;
        temp = temp / NO_OF_SAMPLES;
        UART_PRINT("\n\r%f", temp);
      //  SPIDataPut(GSPI_BASE, temp);
        temp2=(unsigned long)3;
        //	SPIDataPutNonBlocking(GSPI_BASE, pulAdcSamples);
        //	SPIDataGet(GSPI_BASE ,temp3);
        int intVal = (temp/3.3)*255; //roughly 50 for this application
        UART_PRINT("\n\rinVal: %d\n\r", intVal);
    	MAP_SPICSEnable(GSPI_BASE);
    	intVal = intVal + 3840;   //3840 is the DAC command for loading and updating the DAC registers
        SPIDataPut(GSPI_BASE, (unsigned long)intVal);   //sends command byte 3 to load both DAC input
        SPIDataGet(GSPI_BASE, temp3);
       // SPIDataPut(GSPI_BASE, intVal);
        //SPIDataGet(GSPI_BASE ,temp3);
        MAP_SPICSDisable(GSPI_BASE);
	}
}
//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
