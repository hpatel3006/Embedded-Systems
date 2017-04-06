//*****************************************************************************
//
// Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/
//
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions
//  are met:
//
//    Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

//*****************************************************************************
//
// Application Name     - Timer Count Capture
// Application Overview - This application showcases Timer's count capture 
//                        feature to measure frequency of an external signal.
// Application Details  -
// http://processors.wiki.ti.com/index.php/CC32xx_Timer_Count_Capture_Application
// or
// docs\examples\CC32xx_Timer_Count_Capture_Application.pdf
//
//*****************************************************************************

// Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "interrupt.h"
#include "prcm.h"
#include "gpio.h"
#include "utils.h"
#include "uart.h"
#include "timer.h"
#include "rom.h"
#include "rom_map.h"
#include "pin.h"
#include "math.h"
#include "string.h"
#include "spi.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1351.h"
#include "glcdfont.h"
// Common interface includes
#include "uart_if.h"
#include "pinmux.h"
#include "string.h"


#define APPLICATION_VERSION     "1.1.1"
#define APP_NAME        "Timer Count Capture"


#define MASTER 			1
//#define SLAVE			0
#define BUTTON_ONE		66339000
#define BUTTON_TWO		3423416
#define BUTTON_THREE	5520056
#define BUTTON_FOUR		7616696
#define BUTTON_FIVE		9713336
#define BUTTON_SIX		11809976
#define BUTTON_SEVEN	13906616
#define BUTTON_EIGHT	16003256
#define BUTTON_NINE		18099896
#define BUTTON_ZERO		20196536
#define BUTTON_LAST		30728880
#define BUTTON_MUTE		41162936

#define SPI_IF_BIT_RATE  100000
//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
static unsigned long g_ulSamples[2];
static char messageBuffer[100];
static char number[100];
static char lastPressed;
static char tempString[1];
static int keyBuffer[13] = {0};
static int value = 1;
static int i = 0;
//static int index = 0;
static int delta = 0;
#if defined(ccs) || defined(gcc)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************


//*****************************************************************************
//
//! Timer interrupt handler
//
//*****************************************************************************
static void TimerIntHandler()
{

	//
    // Clear the interrupt
    //
	g_ulSamples[1] = MAP_TimerValueGet(TIMERA1_BASE,TIMER_A);
	TimerLoadSet(TIMERA1_BASE, TIMER_A,0xffff);
	//Report("%d", value);

    //
    // Get the samples and compute the frequency
    //
	//number[i] =
    g_ulSamples[0] = g_ulSamples[1];

   // Report("sample 0: %d\n\r", g_ulSamples[0]);
    //Report("sample 1: %d\n\r", g_ulSamples[1]);

    delta = g_ulSamples[1];
    if(delta < 54000)
    	value = 0;
    else
    	value = 1;
    if(i==0)
    	value = 0;
	number[i] = (char)value;
	i++;
    //if(delta > 6000)
    //	delta = 65535/2 - delta;
    //Report("delta: %d\n\r", delta);
	MAP_TimerIntClear(TIMERA1_BASE,TIMER_CAPA_EVENT);


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
DisplayBanner(char * AppName)
{

    Report("\n\n\n\r");
    Report("\t\t *************************************************\n\r");
    Report("\t\t\t  CC3200 %s Application       \n\r", AppName);
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
void
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
  //
  // Set vector table base
  //
#if defined(ccs) || defined(gcc)
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


void delay(unsigned long ulCount){
	int i;

  do{
    ulCount--;
		for (i=0; i< 63000; i++) ;
	}while(ulCount);
}

//*****************************************************************************
//
//! Main  Function
//
//*****************************************************************************
int main()
{
	int z = 0;
	int y = 0;
	int k = 0;
	int sum = 0;
	int temp = 0;
    BoardInit();
    PinMuxConfig();
    InitTerm();
    DisplayBanner(APP_NAME);
    MAP_PRCMPeripheralClkEnable(PRCM_GSPI,PRCM_RUN_MODE_CLK);
	MAP_PRCMPeripheralReset(PRCM_GSPI);
	MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
					SPI_IF_BIT_RATE,SPI_MODE_MASTER,SPI_SUB_MODE_3,
					(SPI_SW_CTRL_CS |
					SPI_4PIN_MODE |
					SPI_TURBO_OFF |
					SPI_CS_ACTIVELOW |
					SPI_WL_8));
	MAP_SPIEnable(GSPI_BASE);
	MAP_SPICSEnable(GSPI_BASE);
	Adafruit_Init();
	fillScreen(BLACK);
	MAP_PinConfigSet(PIN_02,PIN_TYPE_STD_PU,PIN_STRENGTH_6MA);
    MAP_TimerIntRegister(TIMERA1_BASE,TIMER_A,TimerIntHandler);
    MAP_TimerConfigure(TIMERA1_BASE, (TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_TIME));
    MAP_TimerControlEvent(TIMERA1_BASE,TIMER_A,TIMER_EVENT_NEG_EDGE);
    MAP_TimerLoadSet(TIMERA1_BASE,TIMER_A,0xffff);
    MAP_TimerIntEnable(TIMERA1_BASE,TIMER_CAPA_EVENT);
    MAP_TimerEnable(TIMERA1_BASE,TIMER_A);

    //Enable and set up the UARTA1
    MAP_UARTEnable(UARTA1_BASE);
    MAP_UARTFIFOEnable(UARTA1_BASE);
   // MAP_UARTIntRegister(UARTA1_BASE,UARTIntHandler);
    //MAP_UARTIntEnable(UARTA1_BASE,UART_INT_DMARX);
    MAP_UARTConfigSetExpClk(UARTA1_BASE,MAP_PRCMPeripheralClockGet(PRCM_UARTA1),
                             UART_BAUD_RATE,
                             (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));

    while(1){
		while(i<26){
			//if(i == 26)
				//break;
		}
		Report("\n\n\r");
		i=0;
		Report("You pressed: ");

		for(k = 0; k<25; k++){
			sum += (int)number[k]*pow(2, (k+1));
			Report("%d", (int)number[k]);
		}
		Report("\n\rUnique ID: %d", sum);
		index++; //increment index
		Report("\n\rYou pressed: ");
		switch(sum){

			case(BUTTON_ONE):
				keyBuffer[1]+=1;
				Report("1");
				lastPressed = '1';
				index-=1;
				break;

			case(BUTTON_TWO):
				Report("2");
				if(keyBuffer[2] == 3){
					keyBuffer[2] = 0;
					delKey("C");
				}
				if(lastPressed != '2')
					keyBuffer[2] = 0;
				if(keyBuffer[2]==0){
					Outstr("A");
				}
				else if(keyBuffer[2]==1){
					delKey("A");
					Outstr("B");
				}
				else if(keyBuffer[2]==2){
					delKey("B");
					Outstr("C");
				}
				lastPressed = '2';
				keyBuffer[2]+=1;
				break;

			case(BUTTON_THREE):
				Report("3");
				if(keyBuffer[3] == 3){
					keyBuffer[3] = 0;
					delKey("F");
				}
				if(lastPressed != '3')
					keyBuffer[3] = 0;
				if(keyBuffer[3]==0){
					Outstr("D");
				}
				else if(keyBuffer[3]==1){
					delKey("D");
					Outstr("E");
				}
				else if(keyBuffer[3]==2){
					delKey("E");
					Outstr("F");
				}
				lastPressed = '3';
				keyBuffer[3]+=1;
				break;
			case(BUTTON_FOUR):

				Report("4");

				if(keyBuffer[4] == 3){
					keyBuffer[4] = 0;
					delKey("I");
				}

				if(keyBuffer[4] == 3)
					keyBuffer[4] = 0;

				if(lastPressed != '4')
					keyBuffer[4] = 0;

				if(keyBuffer[4]==0){
					Outstr("G");
				}

				else if(keyBuffer[4]==1){
					delKey("G");
					Outstr("H");
				}
				else if(keyBuffer[4]==2){
					delKey("H");
					Outstr("I");
				}

				lastPressed = '4';
				keyBuffer[4]+=1;
				break;

			case(BUTTON_FIVE):
				Report("5");
				if(keyBuffer[5] == 3){
					keyBuffer[5] = 0;
					delKey("L");
				}

				if(lastPressed != '5')
					keyBuffer[5] = 0;
				if(keyBuffer[5]==0){
					Outstr("J");
				}
				else if(keyBuffer[5]==1){
					delKey("J");
					Outstr("K");
				}
				else if(keyBuffer[5]==2){
					delKey("K");
					Outstr("L");
				}
				lastPressed = '5';
				keyBuffer[5]+=1;
				break;
			case(BUTTON_SIX):
				Report("6");
				if(keyBuffer[6] == 3){
					keyBuffer[6] = 0;
					delKey("O");
				}

				if(lastPressed != '6')
					keyBuffer[6] = 0;
				if(keyBuffer[6]==0){
					Outstr("M");
				}
				else if(keyBuffer[6]==1){
					delKey("M");
					Outstr("N");
				}
				else if(keyBuffer[6]==2){
					delKey("N");
					Outstr("O");
				}
				lastPressed = '6';
				keyBuffer[6]+=1;
				break;
			case(BUTTON_SEVEN):

				Report("7");

				if(keyBuffer[7] == 4){
					keyBuffer[7] = 0;
					delKey("S");
				}

				if(lastPressed != '7')
					keyBuffer[7] = 0;

				if(keyBuffer[7]==0){
					Outstr("P");
				}

				else if(keyBuffer[7]==1){
					delKey("P");
					Outstr("Q");
				}
				else if(keyBuffer[7]==2){
					delKey("Q");
					Outstr("R");
				}
				else if(keyBuffer[7]==3){
					delKey("R");
					Outstr("S");
				}

				lastPressed = '7';
				keyBuffer[7]+=1;
				break;

			case(BUTTON_EIGHT):

				Report("8");

				if(keyBuffer[8] == 3){
					keyBuffer[8] = 0;
					delKey("V");
				}

				if(lastPressed != '8')
					keyBuffer[8] = 0;

				if(keyBuffer[8]==0){
					Outstr("T");
				}

				else if(keyBuffer[8]==1){
					delKey("T");
					Outstr("U");
				}
				else if(keyBuffer[8]==2){
					delKey("U");
					Outstr("V");
				}

				lastPressed = '8';
				keyBuffer[8]+=1;
				break;
			case(BUTTON_NINE):

				Report("9");

				if(keyBuffer[9] == 4){
					keyBuffer[9] = 0;
					delKey("Z");
				}

				if(lastPressed != '9')
					keyBuffer[9] = 0;

				if(keyBuffer[9]==0){
					Outstr("W");
				}

				else if(keyBuffer[9]==1){
					delKey("W");
					Outstr("X");
				}
				else if(keyBuffer[9]==2){
					delKey("X");
					Outstr("Y");
				}
				else if(keyBuffer[9]==3){
					delKey("Y");
					Outstr("Z");
				}
				lastPressed = '9';
				keyBuffer[9]+=1;
				break;

			case(BUTTON_ZERO):
				Report("0");
				Outstr(" ");
				lastPressed = '0';
				break;
			case(BUTTON_LAST):
				if(index>1)		//since delKey will decrement index, and we want it to stay > 1
					index = index - 1;
				//Report("\n\rmessageBuffer is: %c\n\r", messageBuffer[index-1]);
				tempString[0] = messageBuffer[index-1];
				messageBuffer[index-1] = NULL;
				Report("\n\rLAST");
				delKey(tempString);
				lastPressed = 'l';
				break;
			case(BUTTON_MUTE):
				Report("MUTE");

				//Running the ENTER command for the MASTER device

					Report("\n\rMaster Entered!\n\r");
					//Put each character from messageBuffer into the TX FIFO (for UART1)
					for(z=0; z<(index-1); z++){
						if(MASTER)
						UARTCharPut(UARTA1_BASE, messageBuffer[z]);
						else{
						tempString[0] = ((char)UARTCharGet(UARTA1_BASE));
						Outstr2(tempString);
						Report("\n\rtempString: %s\n\r", tempString);
						}

					}

				/*else{
				//	delay(100);
					for(z=0; z<index; z++){
						tempString[0] = ((char)UARTCharGetNonBlocking(UARTA1_BASE));
						Outstr2(tempString[0]);
						Report("\n\rSlave Entered!\n\rTemp String : %s\n\r", tempString);
					}*/

				//Outstr(" MUTE ");
				lastPressed = 'm';
				index-=1;
				break;
			default:
				Report("Unknown code %d", sum);
				//Outstr(" Unknown code ");
				//lastPressed = 'u';
				index-=1;
				break;
		}
		if(lastPressed == 'l' || lastPressed == 'm' || lastPressed == '1') {}
		else if (lastPressed == '0')
			messageBuffer[index-1] = ' ';
		else{
			temp = 65 + (lastPressed-48-2)*3 + keyBuffer[(int)lastPressed-48] - 1;	//returns ascii value for the char you selected aelkman
			if(lastPressed == '8' || lastPressed == '9')
				temp += 1;
			messageBuffer[index-1] = (char)temp;
		}
		//else
		//	int temp = 65 + (lastPressed-48-2)*3 + keyBuffer[(int)lastPressed-48] - 1;
		//int temp = lastPressed;
		Report("\n\rtemp: %d, index: %d", temp, index);
		sum=0;
		Report("\n\rMessege Buffer: ");
		for(y=0;y<100;y++){
			Report("%c", messageBuffer[y]);

		}
	}



}
