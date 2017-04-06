//Alex Elkman
//Hardik Patel
//EEC 172 Embedded Systems
//Spring 2016

#include <string.h>
#include <stdlib.h>

#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "hw_ints.h"
#include "spi.h"
#include "rom.h"
#include "gpio.h"
#include "rom_map.h"
#include "utils.h"
#include "prcm.h"
#include "uart.h"

#include "i2c_if.h"
#include "interrupt.h"


#include "uart_if.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1351.h"
#include "glcdfont.h"
#include "pin_mux_config.h"



extern int cursor_x;
extern int cursor_y;

float p = 3.1415926;

char xarray[] = "readreg 0x18 0x3 1";
char yarray[] = "readreg 0x18 0x5 1";
int xaxis, yaxis;


#define APPLICATION_VERSION     "1.1.1"

// MASTER_MODE = 1 : Application in master mode
// MASTER_MODE = 0 : Application in slave mode
#define MASTER_MODE      1

#define SPI_IF_BIT_RATE  100000
#define TR_BUFF_SIZE     100

#define MASTER_MSG       "This is CC3200 SPI Master Application\n\r"
#define SLAVE_MSG        "This is CC3200 SPI Slave Application\n\r"


#define APPLICATION_VERSION     "1.1.1"
#define APP_NAME                "I2C Demo"
#define UART_PRINT              Report
#define FOREVER                 1
#define CONSOLE                 UARTA0_BASE
#define FAILURE                 -1
#define SUCCESS                 0
#define RETERR_IF_TRUE(condition) {if(condition) return FAILURE;}
#define RET_IF_ERR(Func)          {int iRetVal = (Func); \
                                   if (SUCCESS != iRetVal) \
                                     return  iRetVal;}



#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif

static void BoardInit(void)
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

void delay(unsigned long ulCount){
	int i;

  do{
    ulCount--;
		for (i=0; i< 65535; i++) ;
	}while(ulCount);
}

int ProcessReadRegCommand(char *pcInpString)
{
    unsigned char ucDevAddr, ucRegOffset, ucRdLen;
    unsigned char aucRdDataBuf[256];
    char *pcErrPtr;
    char *temp = pcInpString;
    //
    // Get the device address
    //
    pcInpString = strtok(NULL, " ");
    //RETERR_IF_TRUE(pcInpString == NULL);
  //  ucDevAddr = (unsigned char)strtoul(pcInpString+2, &pcErrPtr, 16);
    //
    // Get the register offset address
    //
    pcInpString = strtok(NULL, " ");
   // RETERR_IF_TRUE(pcInpString == NULL);
    ucRegOffset = (unsigned char)strtoul(pcInpString+2, &pcErrPtr, 16);
   // temp = ucDevAddr+ucRegOffset;
    //
    // Get the length of data to be read
    //
    pcInpString = strtok(NULL, " ");
   // RETERR_IF_TRUE(pcInpString == NULL);
    ucRdLen = (unsigned char)strtoul(pcInpString, &pcErrPtr, 10);
    //RETERR_IF_TRUE(ucLen > sizeof(aucDataBuf));
	
	//checks the values of the data at the following register values, saves to global variables
   if(xarray[15] == temp[15]){
	   ucRegOffset = 0x5;}
   if(yarray[15] == temp[15]){
	   ucRegOffset = 0x3;}
	   ucDevAddr = 0x18;
    //
    // Write the register address to be read from.
    // Stop bit implicitly assumed to be 0.
    //
   // RET_IF_ERR(I2C_IF_Write(ucDevAddr,&ucRegOffset,1,0));
    I2C_IF_Write(ucDevAddr,&ucRegOffset,1,0);
    //
    // Read the specified length of data
    //
    //RET_IF_ERR(I2C_IF_Read(ucDevAddr, &aucRdDataBuf[0], ucRdLen));
    I2C_IF_Read(ucDevAddr, &aucRdDataBuf[0], 1);


    if(ucRegOffset == 0x5)
    	xaxis = (int)aucRdDataBuf[0];
    if(ucRegOffset == 0x3)
    	yaxis = (int)aucRdDataBuf[0];



}
int ChangeCircle1(int a, int b)
{
		if (a + b > 123)
			a = 123;
		else if (a + b < 4)
			a = 4;
		else
			a = (a + b)%128;
		return a;

}


void main()
{
    // Initialize Board configurations
    BoardInit();

    // Muxing UART and SPI lines.
    PinMuxConfig();

    // Enable the SPI module clock
    MAP_PRCMPeripheralClkEnable(PRCM_GSPI,PRCM_RUN_MODE_CLK);

    // Reset the peripheral
    MAP_PRCMPeripheralReset(PRCM_GSPI);

        // Configure SPI interface
     MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                         SPI_IF_BIT_RATE,SPI_MODE_MASTER,SPI_SUB_MODE_3,
                         (SPI_SW_CTRL_CS |
                         SPI_4PIN_MODE |
                         SPI_TURBO_OFF |
                         SPI_CS_ACTIVELOW |
                         SPI_WL_8));

        // Enable SPI for communication
        MAP_SPIEnable(GSPI_BASE);
        MAP_SPICSEnable(GSPI_BASE);
        I2CMasterEnable(I2CA0_BASE);
        I2CSlaveEnable(I2CA0_BASE);
        Adafruit_Init();



        fillScreen(BLACK);
        int cx = 64, cy = 64;
        drawCircle(cx, cy, 4, BLACK);
        int x2, y2, x1, y1, x3, y3;
        while(1)
        {
        				ProcessReadRegCommand(&xarray);
        	        	ProcessReadRegCommand(&yarray);


					if (xaxis > 64)
						xaxis = (xaxis - 255)/4;

					if (yaxis > 64)
						yaxis = (yaxis - 255)/4;

					x1 = cx;
					y1 = cy;
					drawCircle(x1, y1, 4, BLACK);

					//x3 = ChangeCircle1(cx, xaxis);
					//y3 = ChangeCircle1(cy, yaxis);

					if (cx + xaxis < 4 )
						cx = 4;
					else if (cx + xaxis >123)
						cx = 123;
					else
						cx = ((cx + xaxis) % 128);


					if (cy + yaxis <4)
						cy = 4;
					else if (cy + yaxis > 123)
						cy = 123;
					else
						cy = ((cy + yaxis)) % 128;


					drawCircle(cx, cy, 4, BLUE);
        }

}



