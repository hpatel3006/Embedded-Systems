//Hardik Patel
//Alex Elkman
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
// Simplelink includes
#include "simplelink.h"
#include "stdio.h"
#include "gpio_if.h"
#include "common.h"
#include "stdio.h"


#define MAX_URI_SIZE 128
#define URI_SIZE MAX_URI_SIZE + 1


#define APPLICATION_NAME        "SSL"
#define APPLICATION_VERSION     "1.2.6"

//#define SERVER_NAME                "AHMAIFS2X4J4Y.iot.us-west-2.amazonaws.com"
#define SERVER_NAME    			    "A3MIY1Q2V62MAH.iot.us-east-1.amazonaws.com"
#define GOOGLE_DST_PORT             8443

#define SL_SSL_CA_CERT "/cert/rootca.der"
#define SL_SSL_PRIVATE "/cert/private.der"
#define SL_SSL_CLIENT  "/cert/client.der"


//NEED TO UPDATE THIS FOR IT TO WORK!
#define DATE                23    /* Current Date */
#define MONTH               5     /* Month 1-12 */
#define YEAR                2016  /* Current year */
#define HOUR                21    /* Time - hours */
#define MINUTE              04    /* Time - minutes */
#define SECOND              0     /* Time - seconds */

#define POSTHEADER "POST /things/Schnauzer/shadow HTTP/1.1\nGET /things/Schnauzer/shadow"
//#define HOSTHEADER "Host: AHMAIFS2X4J4Y.iot.us-west-2.amazonaws.com\r\n"
#define HOSTHEADER "Host: https://A3MIY1Q2V62MAH.iot.us-east-1.amazonaws.com\r\n"
//#define AUTHHEADER "Authorization: SharedAccessSignature sr=swiftsoftware-ns.servicebus.windows.net&sig=6sIkgCiaNbK9R0XEpsKJcQ2Clv8MUMVdQfEVQP09WkM%3d&se=1733661915&skn=EventHubPublisher\r\n"
#define CHEADER "Connection: Keep-Alive\r\n"
#define CTHEADER "Content-Type: application/json; charset=utf-8\r\n"
#define CLHEADER1 "Content-Length: "
#define CLHEADER2 "\r\n\r\n"
//#define DATA1 "{\"MessageType\":\"CC3200 Sensor\",\"Temp\":"
//#define DATA2 ",\"Humidity\":50,\"Location\":\"YourLocation\",\"Room\":\"YourRoom\",\"Info\":\"Sent from CC3200 LaunchPad\"}"
#define DATA2 ",\"Humidity\":50,\"Location\":\"YourLocation\",\"Room\":\"YourRoom\",\"Info\":\"Sent from CC3200 LaunchPad\"}"

// Application specific status/error codes
typedef enum{
    // Choosing -0x7D0 to avoid overlap w/ host-driver's error codes
    LAN_CONNECTION_FAILED = -0x7D0,
    INTERNET_CONNECTION_FAILED = LAN_CONNECTION_FAILED - 1,
    DEVICE_NOT_IN_STATION_MODE = INTERNET_CONNECTION_FAILED - 1,

    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;

typedef struct
{
   /* time */
   unsigned long tm_sec;
   unsigned long tm_min;
   unsigned long tm_hour;
   /* date */
   unsigned long tm_day;
   unsigned long tm_mon;
   unsigned long tm_year;
   unsigned long tm_week_day; //not required
   unsigned long tm_year_day; //not required
   unsigned long reserved[3];
}SlDateTime;
#define APP_NAME        "Timer Count Capture"


#define MASTER 			0
//These are the decimal values that corrospont to the binary data from the buttons as decoded bt our inturrupt handler
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
static char messageBuffer[100]; //Keeps track of the messege in a global variable
static char number[100]; //Keeps track the binary strings of the buttons
static char lastPressed; //Keeps track of the last digit passes. 
static char tempString[1]; // Is used for the UARTCharGet functions to store the character in a temporary variable befor we display it on the OLED
static int keyBuffer[13] = {0}; // An array that helps with the implementation of the multi-tap aspect of the program.
static int value = 1; // Generates the binary value
static int i = 0;
//static int index = 0;
static int delta = 0;
volatile unsigned long  g_ulStatus = 0;//SimpleLink Status
unsigned long  g_ulPingPacketsRecv = 0; //Number of Ping Packets received
unsigned long  g_ulGatewayIP = 0; //Network Gateway IP address
unsigned char  g_ucConnectionSSID[SSID_LEN_MAX+1]; //Connection SSID
unsigned char  g_ucConnectionBSSID[BSSID_LEN_MAX]; //Connection BSSID
signed char    *g_Host = SERVER_NAME;
SlDateTime g_time;
#if defined(ccs) || defined(gcc)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif
char *security_file_list[] = {"/cert/private.der", "/cert/client.der", "/cert/rootCA.der"};
//local prototype
static long WlanConnect();
static int set_time();
static void BoardInit(void);
static long InitializeAppVariables();
static int tls_connect();
static int connectToAccessPoint();
static int http_post(int);
char* DATA1 = "{\"state\": {\n\r\"desired\" : {\n\r\"message\" : \"DfongHelloWorldCC3200\"\n\r}}}\n\r\n\r";

//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************

void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent)
{
    if(!pWlanEvent)
    {
        return;
    }

    switch(pWlanEvent->Event)
    {
        case SL_WLAN_CONNECT_EVENT:
        {
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);

            //
            // Information about the connected AP (like name, MAC etc) will be
            // available in 'slWlanConnectAsyncResponse_t'.
            // Applications can use it if required
            //
            //  slWlanConnectAsyncResponse_t *pEventData = NULL;
            // pEventData = &pWlanEvent->EventData.STAandP2PModeWlanConnected;
            //

            // Copy new connection SSID and BSSID to global parameters
            memcpy(g_ucConnectionSSID,pWlanEvent->EventData.
                   STAandP2PModeWlanConnected.ssid_name,
                   pWlanEvent->EventData.STAandP2PModeWlanConnected.ssid_len);
            memcpy(g_ucConnectionBSSID,
                   pWlanEvent->EventData.STAandP2PModeWlanConnected.bssid,
                   SL_BSSID_LENGTH);

            UART_PRINT("[WLAN EVENT] STA Connected to the AP: %s , "
                       "BSSID: %x:%x:%x:%x:%x:%x\n\r",
                       g_ucConnectionSSID,g_ucConnectionBSSID[0],
                       g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                       g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                       g_ucConnectionBSSID[5]);
        }
        break;

        case SL_WLAN_DISCONNECT_EVENT:
        {
            slWlanConnectAsyncResponse_t*  pEventData = NULL;

            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);

            pEventData = &pWlanEvent->EventData.STAandP2PModeDisconnected;

            // If the user has initiated 'Disconnect' request,
            //'reason_code' is SL_USER_INITIATED_DISCONNECTION
            if(SL_USER_INITIATED_DISCONNECTION == pEventData->reason_code)
            {
                UART_PRINT("[WLAN EVENT]Device disconnected from the AP: %s,"
                    "BSSID: %x:%x:%x:%x:%x:%x on application's request \n\r",
                           g_ucConnectionSSID,g_ucConnectionBSSID[0],
                           g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                           g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                           g_ucConnectionBSSID[5]);
            }
            else
            {
                UART_PRINT("[WLAN ERROR]Device disconnected from the AP AP: %s, "
                           "BSSID: %x:%x:%x:%x:%x:%x on an ERROR..!! \n\r",
                           g_ucConnectionSSID,g_ucConnectionBSSID[0],
                           g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                           g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                           g_ucConnectionBSSID[5]);
            }
            memset(g_ucConnectionSSID,0,sizeof(g_ucConnectionSSID));
            memset(g_ucConnectionBSSID,0,sizeof(g_ucConnectionBSSID));
        }
        break;

        default:
        {
            UART_PRINT("[WLAN EVENT] Unexpected event [0x%x]\n\r",
                       pWlanEvent->Event);
        }
        break;
    }
}

//*****************************************************************************
//
//! \brief This function handles network events such as IP acquisition, IP
//!           leased, IP released etc.
//!
//! \param[in]  pNetAppEvent - Pointer to NetApp Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
    if(!pNetAppEvent)
    {
        return;
    }

    switch(pNetAppEvent->Event)
    {
        case SL_NETAPP_IPV4_IPACQUIRED_EVENT:
        {
            SlIpV4AcquiredAsync_t *pEventData = NULL;

            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);

            //Ip Acquired Event Data
            pEventData = &pNetAppEvent->EventData.ipAcquiredV4;

            //Gateway IP address
            g_ulGatewayIP = pEventData->gateway;

            UART_PRINT("[NETAPP EVENT] IP Acquired: IP=%d.%d.%d.%d , "
                       "Gateway=%d.%d.%d.%d\n\r",
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,3),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,2),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,1),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,0),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,3),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,2),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,1),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,0));
        }
        break;

        default:
        {
            UART_PRINT("[NETAPP EVENT] Unexpected event [0x%x] \n\r",
                       pNetAppEvent->Event);
        }
        break;
    }
}


//*****************************************************************************
//
//! \brief This function handles HTTP server events
//!
//! \param[in]  pServerEvent - Contains the relevant event information
//! \param[in]    pServerResponse - Should be filled by the user with the
//!                                      relevant response information
//!
//! \return None
//!
//****************************************************************************
void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pHttpEvent,
                                  SlHttpServerResponse_t *pHttpResponse)
{
    // Unused in this application
}

//*****************************************************************************
//
//! \brief This function handles General Events
//!
//! \param[in]     pDevEvent - Pointer to General Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent)
{
    if(!pDevEvent)
    {
        return;
    }

    //
    // Most of the general errors are not FATAL are are to be handled
    // appropriately by the application
    //
    UART_PRINT("[GENERAL EVENT] - ID=[%d] Sender=[%d]\n\n",
               pDevEvent->EventData.deviceEvent.status,
               pDevEvent->EventData.deviceEvent.sender);
}


//*****************************************************************************
//
//! This function handles socket events indication
//!
//! \param[in]      pSock - Pointer to Socket Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{
    if(!pSock)
    {
        return;
    }

    switch( pSock->Event )
    {
        case SL_SOCKET_TX_FAILED_EVENT:
            switch( pSock->socketAsyncEvent.SockTxFailData.status)
            {
                case SL_ECLOSE:
                    UART_PRINT("[SOCK ERROR] - close socket (%d) operation "
                                "failed to transmit all queued packets\n\n",
                                    pSock->socketAsyncEvent.SockTxFailData.sd);
                    break;
                default:
                    UART_PRINT("[SOCK ERROR] - TX FAILED  :  socket %d , reason "
                                "(%d) \n\n",
                                pSock->socketAsyncEvent.SockTxFailData.sd, pSock->socketAsyncEvent.SockTxFailData.status);
                  break;
            }
            break;

        default:
        	UART_PRINT("[SOCK EVENT] - Unexpected Event [%x0x]\n\n",pSock->Event);
          break;
    }

}


//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- End
//*****************************************************************************


//*****************************************************************************
//
//! \brief This function initializes the application variables
//!
//! \param    0 on success else error code
//!
//! \return None
//!
//*****************************************************************************
static long InitializeAppVariables()
{
    g_ulStatus = 0;
    g_ulGatewayIP = 0;
    g_Host = SERVER_NAME;
    memset(g_ucConnectionSSID,0,sizeof(g_ucConnectionSSID));
    memset(g_ucConnectionBSSID,0,sizeof(g_ucConnectionBSSID));
    return SUCCESS;
}


//*****************************************************************************
//! \brief This function puts the device in its default state. It:
//!           - Set the mode to STATION
//!           - Configures connection policy to Auto and AutoSmartConfig
//!           - Deletes all the stored profiles
//!           - Enables DHCP
//!           - Disables Scan policy
//!           - Sets Tx power to maximum
//!           - Sets power policy to normal
//!           - Unregister mDNS services
//!           - Remove all filters
//!
//! \param   none
//! \return  On success, zero is returned. On error, negative is returned
//*****************************************************************************
static long ConfigureSimpleLinkToDefaultState()
{
    SlVersionFull   ver = {0};
    _WlanRxFilterOperationCommandBuff_t  RxFilterIdMask = {0};

    unsigned char ucVal = 1;
    unsigned char ucConfigOpt = 0;
    unsigned char ucConfigLen = 0;
    unsigned char ucPower = 0;

    long lRetVal = -1;
    long lMode = -1;

    lMode = sl_Start(0, 0, 0);
    ASSERT_ON_ERROR(lMode);

    // If the device is not in station-mode, try configuring it in station-mode
    if (ROLE_STA != lMode)
    {
        if (ROLE_AP == lMode)
        {
            // If the device is in AP mode, we need to wait for this event
            // before doing anything
            while(!IS_IP_ACQUIRED(g_ulStatus))
            {
#ifndef SL_PLATFORM_MULTI_THREADED
              _SlNonOsMainLoopTask();
#endif
            }
        }

        // Switch to STA role and restart
        lRetVal = sl_WlanSetMode(ROLE_STA);
        ASSERT_ON_ERROR(lRetVal);

        lRetVal = sl_Stop(0xFF);
        ASSERT_ON_ERROR(lRetVal);

        lRetVal = sl_Start(0, 0, 0);
        ASSERT_ON_ERROR(lRetVal);

        // Check if the device is in station again
        if (ROLE_STA != lRetVal)
        {
            // We don't want to proceed if the device is not coming up in STA-mode
            return DEVICE_NOT_IN_STATION_MODE;
        }
    }

    // Get the device's version-information
    ucConfigOpt = SL_DEVICE_GENERAL_VERSION;
    ucConfigLen = sizeof(ver);
    lRetVal = sl_DevGet(SL_DEVICE_GENERAL_CONFIGURATION, &ucConfigOpt,
                                &ucConfigLen, (unsigned char *)(&ver));
    ASSERT_ON_ERROR(lRetVal);

    UART_PRINT("Host Driver Version: %s\n\r",SL_DRIVER_VERSION);
    UART_PRINT("Build Version %d.%d.%d.%d.31.%d.%d.%d.%d.%d.%d.%d.%d\n\r",
    ver.NwpVersion[0],ver.NwpVersion[1],ver.NwpVersion[2],ver.NwpVersion[3],
    ver.ChipFwAndPhyVersion.FwVersion[0],ver.ChipFwAndPhyVersion.FwVersion[1],
    ver.ChipFwAndPhyVersion.FwVersion[2],ver.ChipFwAndPhyVersion.FwVersion[3],
    ver.ChipFwAndPhyVersion.PhyVersion[0],ver.ChipFwAndPhyVersion.PhyVersion[1],
    ver.ChipFwAndPhyVersion.PhyVersion[2],ver.ChipFwAndPhyVersion.PhyVersion[3]);

    // Set connection policy to Auto + SmartConfig
    //      (Device's default connection policy)
    lRetVal = sl_WlanPolicySet(SL_POLICY_CONNECTION,
                                SL_CONNECTION_POLICY(1, 0, 0, 0, 1), NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Remove all profiles
    lRetVal = sl_WlanProfileDel(0xFF);
    ASSERT_ON_ERROR(lRetVal);



    //
    // Device in station-mode. Disconnect previous connection if any
    // The function returns 0 if 'Disconnected done', negative number if already
    // disconnected Wait for 'disconnection' event if 0 is returned, Ignore
    // other return-codes
    //
    lRetVal = sl_WlanDisconnect();
    if(0 == lRetVal)
    {
        // Wait
        while(IS_CONNECTED(g_ulStatus))
        {
#ifndef SL_PLATFORM_MULTI_THREADED
              _SlNonOsMainLoopTask();
#endif
        }
    }

    // Enable DHCP client
    lRetVal = sl_NetCfgSet(SL_IPV4_STA_P2P_CL_DHCP_ENABLE,1,1,&ucVal);
    ASSERT_ON_ERROR(lRetVal);

    // Disable scan
    ucConfigOpt = SL_SCAN_POLICY(0);
    lRetVal = sl_WlanPolicySet(SL_POLICY_SCAN , ucConfigOpt, NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Set Tx power level for station mode
    // Number between 0-15, as dB offset from max power - 0 will set max power
    ucPower = 0;
    lRetVal = sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID,
            WLAN_GENERAL_PARAM_OPT_STA_TX_POWER, 1, (unsigned char *)&ucPower);
    ASSERT_ON_ERROR(lRetVal);

    // Set PM policy to normal
    lRetVal = sl_WlanPolicySet(SL_POLICY_PM , SL_NORMAL_POLICY, NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Unregister mDNS services
    lRetVal = sl_NetAppMDNSUnRegisterService(0, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Remove  all 64 filters (8*8)
    memset(RxFilterIdMask.FilterIdMask, 0xFF, 8);
    lRetVal = sl_WlanRxFilterSet(SL_REMOVE_RX_FILTER, (_u8 *)&RxFilterIdMask,
                       sizeof(_WlanRxFilterOperationCommandBuff_t));
    ASSERT_ON_ERROR(lRetVal);

    lRetVal = sl_Stop(SL_STOP_TIMEOUT);
    ASSERT_ON_ERROR(lRetVal);

    InitializeAppVariables();

    return lRetVal; // Success
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


//****************************************************************************
//
//! \brief Connecting to a WLAN Accesspoint
//!
//!  This function connects to the required AP (SSID_NAME) with Security
//!  parameters specified in te form of macros at the top of this file
//!
//! \param  None
//!
//! \return  0 on success else error code
//!
//! \warning    If the WLAN connection fails or we don't aquire an IP
//!            address, It will be stuck in this function forever.
//
//****************************************************************************
static long WlanConnect()
{
    SlSecParams_t secParams = {0};
    long lRetVal = 0;

    secParams.Key = SECURITY_KEY;
    secParams.KeyLen = strlen(SECURITY_KEY);
    secParams.Type = SECURITY_TYPE;

    lRetVal = sl_WlanConnect(SSID_NAME, strlen(SSID_NAME), 0, &secParams, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Wait for WLAN Event
    while((!IS_CONNECTED(g_ulStatus)) || (!IS_IP_ACQUIRED(g_ulStatus)))
    {
        // Toggle LEDs to Indicate Connection Progress
        _SlNonOsMainLoopTask();
        GPIO_IF_LedOff(MCU_IP_ALLOC_IND);
        MAP_UtilsDelay(800000);
        _SlNonOsMainLoopTask();
        GPIO_IF_LedOn(MCU_IP_ALLOC_IND);
        MAP_UtilsDelay(800000);
    }

    return SUCCESS;

}

//*****************************************************************************
//
//! This function updates the date and time of CC3200.
//!
//! \param None
//!
//! \return
//!     0 for success, negative otherwise
//!
//*****************************************************************************

static int set_time()
{
    long retVal;

    g_time.tm_day = DATE;
    g_time.tm_mon = MONTH;
    g_time.tm_year = YEAR;
    g_time.tm_sec = HOUR;
    g_time.tm_hour = MINUTE;
    g_time.tm_min = SECOND;

    retVal = sl_DevSet(SL_DEVICE_GENERAL_CONFIGURATION,
                          SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME,
                          sizeof(SlDateTime),(unsigned char *)(&g_time));

    ASSERT_ON_ERROR(retVal);
    return SUCCESS;
}

//*****************************************************************************
//
//! This function demonstrates how certificate can be used with SSL.
//! The procedure includes the following steps:
//! 1) connect to an open AP
//! 2) get the server name via a DNS request
//! 3) define all socket options and point to the CA certificate
//! 4) connect to the server via TCP
//!
//! \param None
//!
//! \return  0 on success else error code
//! \return  LED1 is turned solid in case of success
//!    LED2 is turned solid in case of failure
//!
//*****************************************************************************
static int tls_connect()
{
    SlSockAddrIn_t    Addr;
    int    iAddrSize;
    unsigned char    ucMethod = SL_SO_SEC_METHOD_TLSV1_2;
    unsigned int uiIP,uiCipher = SL_SEC_MASK_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA;
    long lRetVal = -1;
    int iSockID;

    lRetVal = sl_NetAppDnsGetHostByName(g_Host, strlen((const char *)g_Host),
                                    (unsigned long*)&uiIP, SL_AF_INET);

    if(lRetVal < 0)
    {
        UART_PRINT("Device couldn't retrive the host name \n\r");
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }

    Addr.sin_family = SL_AF_INET;
    Addr.sin_port = sl_Htons(GOOGLE_DST_PORT);
    Addr.sin_addr.s_addr = sl_Htonl(uiIP);
    iAddrSize = sizeof(SlSockAddrIn_t);
    //
    // opens a secure socket
    //
    iSockID = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, SL_SEC_SOCKET);
    if( iSockID < 0 )
    {
        UART_PRINT("Device unable to create secure socket \n\r");
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }

    //
    // configure the socket as TLS1.2
    //
    lRetVal = sl_SetSockOpt(iSockID, SL_SOL_SOCKET, SL_SO_SECMETHOD, &ucMethod,\
                               sizeof(ucMethod));
    if(lRetVal < 0)
    {
        UART_PRINT("Device couldn't set socket options \n\r");
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }
    //
    //configure the socket as ECDHE RSA WITH AES256 CBC SHA
    //
    lRetVal = sl_SetSockOpt(iSockID, SL_SOL_SOCKET, SL_SO_SECURE_MASK, &uiCipher,\
                           sizeof(uiCipher));
    if(lRetVal < 0)
    {
        UART_PRINT("Device couldn't set socket options \n\r");
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }

    //
    //configure the socket with CA certificate - for server verification
    //
    lRetVal = sl_SetSockOpt(iSockID, SL_SOL_SOCKET, \
                           SL_SO_SECURE_FILES_CA_FILE_NAME, \
						   SL_SSL_CA_CERT, \
                           strlen(SL_SSL_CA_CERT));

    if(lRetVal < 0)
    {
        UART_PRINT("Device couldn't set socket options \n\r");
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }

    //configure the socket with Client Certificate - for server verification
    //
    lRetVal = sl_SetSockOpt(iSockID, SL_SOL_SOCKET, \
    			SL_SO_SECURE_FILES_CERTIFICATE_FILE_NAME, \
									SL_SSL_CLIENT, \
                           strlen(SL_SSL_CLIENT));

    if(lRetVal < 0)
    {
        UART_PRINT("Device couldn't set socket options \n\r");
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }

    //configure the socket with Private Key - for server verification
    //
    lRetVal = sl_SetSockOpt(iSockID, SL_SOL_SOCKET, \
    		SL_SO_SECURE_FILES_PRIVATE_KEY_FILE_NAME, \
			SL_SSL_PRIVATE, \
                           strlen(SL_SSL_PRIVATE));

    if(lRetVal < 0)
    {
        UART_PRINT("Device couldn't set socket options \n\r");
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }


    /* connect to the peer device - Google server */
    lRetVal = sl_Connect(iSockID, ( SlSockAddr_t *)&Addr, iAddrSize);

    if(lRetVal < 0)
    {
        UART_PRINT("Device couldn't connect to AWS server \n\r");
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }
    else{
    	UART_PRINT("Device has connected to the website:");
    	UART_PRINT(SERVER_NAME);
    	UART_PRINT("\n\r");
    }

    GPIO_IF_LedOff(MCU_RED_LED_GPIO);
    GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);
    return iSockID;
}

int connectToAccessPoint(){
	long lRetVal = -1;
    GPIO_IF_LedConfigure(LED1|LED3);

    GPIO_IF_LedOff(MCU_RED_LED_GPIO);
    GPIO_IF_LedOff(MCU_GREEN_LED_GPIO);

    lRetVal = InitializeAppVariables();
    ASSERT_ON_ERROR(lRetVal);

    //
    // Following function configure the device to default state by cleaning
    // the persistent settings stored in NVMEM (viz. connection profiles &
    // policies, power policy etc)
    //
    // Applications may choose to skip this step if the developer is sure
    // that the device is in its default state at start of applicaton
    //
    // Note that all profiles and persistent settings that were done on the
    // device will be lost
    //
    lRetVal = ConfigureSimpleLinkToDefaultState();
    if(lRetVal < 0)
    {
      if (DEVICE_NOT_IN_STATION_MODE == lRetVal)
          UART_PRINT("Failed to configure the device in its default state \n\r");

      return lRetVal;
    }

    UART_PRINT("Device is configured in default state \n\r");

    CLR_STATUS_BIT_ALL(g_ulStatus);

    ///
    // Assumption is that the device is configured in station mode already
    // and it is in its default state
    //
    lRetVal = sl_Start(0, 0, 0);
    if (lRetVal < 0 || ROLE_STA != lRetVal)
    {
        UART_PRINT("Failed to start the device \n\r");
        return lRetVal;
    }

    UART_PRINT("Device started as STATION \n\r");

    //
    //Connecting to WLAN AP
    //
    lRetVal = WlanConnect();
    if(lRetVal < 0)
    {
        UART_PRINT("Failed to establish connection w/ an AP \n\r");
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }

    UART_PRINT("Connection established w/ AP and IP is aquired \n\r");
    return 0;
}


static int http_post(int iTLSSockID){
    char acSendBuff[512];
    char acRecvbuff[1460];
    char cCLLength[200];
    char* pcBufHeaders;
    int lRetVal = 0;

	pcBufHeaders = acSendBuff;
	strcpy(pcBufHeaders, POSTHEADER);
	pcBufHeaders += strlen(POSTHEADER);
	strcpy(pcBufHeaders, " HTTP/1.1\r\n");
	pcBufHeaders += strlen(" HTTP/1.1\r\n");
	strcpy(pcBufHeaders, HOSTHEADER);
	pcBufHeaders += strlen(HOSTHEADER);
	strcpy(pcBufHeaders, CHEADER);
	pcBufHeaders += strlen(CHEADER);
	strcpy(pcBufHeaders, "\r\n\r\n");

	int dataLength = strlen(DATA1);

	strcpy(pcBufHeaders, CTHEADER);
	pcBufHeaders += strlen(CTHEADER);
	strcpy(pcBufHeaders, CLHEADER1);

	pcBufHeaders += strlen(CLHEADER1);
	sprintf(cCLLength, "%d", dataLength);

	strcpy(pcBufHeaders, cCLLength);
	pcBufHeaders += strlen(cCLLength);
	strcpy(pcBufHeaders, CLHEADER2);
	pcBufHeaders += strlen(CLHEADER2);

	strcpy(pcBufHeaders, DATA1);
	pcBufHeaders += strlen(DATA1);

	int testDataLength = strlen(pcBufHeaders);

	//
	// Send the packet to the server */
	//
	lRetVal = sl_Send(iTLSSockID, acSendBuff, strlen(acSendBuff), 0);
	if(lRetVal < 0)
	{
		UART_PRINT("POST failed. Error Number: %i\n\r",lRetVal);
    	sl_Close(iTLSSockID);
    	GPIO_IF_LedOn(MCU_RED_LED_GPIO);
    	return lRetVal;
	}
	lRetVal = sl_Recv(iTLSSockID, &acRecvbuff[0], sizeof(acRecvbuff), 0);
	if(lRetVal < 0)
	{
		UART_PRINT("Received failed. Error Number: %i\n\r",lRetVal);
	    //sl_Close(iSSLSockID);
	    GPIO_IF_LedOn(MCU_RED_LED_GPIO);
	       return lRetVal;
	}
	else
	{
		acRecvbuff[lRetVal+1] = '\0';
		UART_PRINT(acRecvbuff);
		UART_PRINT("\n\r\n\r");
	}

	return 0;
}

//*****************************************************************************
//
//! Timer interrupt handler
//
//*****************************************************************************
static void TimerIntHandler()
{


	g_ulSamples[1] = MAP_TimerValueGet(TIMERA2_BASE,TIMER_A);
	TimerLoadSet(TIMERA2_BASE, TIMER_A,0xffff);

    g_ulSamples[0] = g_ulSamples[1];
    delta = g_ulSamples[1];
	//Delta is used to differentiate betwwn a 0 pulse width and a 1 pulse width. 
    if(delta < 54000)
    	value = 0;
    else
    	value = 1;
    if(i==0)
    	value = 0;
	number[i] = (char)value; //Storing a binary value in an array. 
	i++;
	MAP_TimerIntClear(TIMERA2_BASE,TIMER_CAPA_EVENT); // Clearing interrupts


}
//Functions not modified by us
void
DisplayBanner(char * AppName)
{

    Report("\n\n\n\r");
    Report("\t\t *************************************************\n\r");
    Report("\t\t\t  CC3200 %s Application       \n\r", AppName);
    Report("\t\t *************************************************\n\r");
    Report("\n\n\n\r");
}


////////////////////////////////////////
//Generates a small delay
void delay(unsigned long ulCount){
	int i;

  do{
    ulCount--;
		for (i=0; i< 63000; i++) ;
	}while(ulCount);
}

int main()
{
	//Local variables used for different purposes. 
	int y = 0;
	int k = 0;
	int sum = 0;
	int temp = 0;

	 char  str2 [1000];

	char* boba = "{\"state\": {\n\r\"desired\" : {\n\r\"message\" : \"";
	char* boba2 = "\"\n\r}}}\n\r\n\r";
	strcpy(str2, boba);
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
//	Adafruit_Init();
	MAP_PinConfigSet(PIN_04,PIN_TYPE_STD_PU,PIN_STRENGTH_6MA);
    MAP_TimerIntRegister(TIMERA2_BASE,TIMER_A,TimerIntHandler);
    MAP_TimerConfigure(TIMERA2_BASE, (TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_TIME));
	 // We are calling interrupts at the positice edges of the input signal
    MAP_TimerControlEvent(TIMERA2_BASE,TIMER_A,TIMER_EVENT_NEG_EDGE);
	//Starting the timer at 0xffff
    MAP_TimerLoadSet(TIMERA2_BASE,TIMER_A,0xffff);
    MAP_TimerIntEnable(TIMERA2_BASE,TIMER_CAPA_EVENT);
    MAP_TimerEnable(TIMERA2_BASE,TIMER_A);

    while(1){
		// i is the nummerof bits generated by our remote. Hence, we run this loop in 26 times 
		while(i<26){
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
		//This switch statement is where all the multitap aspect is implemented. Also it includes our enter button (MUTE) and delete buttons (LAST).
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
			//We print black over the last pressed as a delete implementation
				if(index>1)		//since delKey will decrement index, and we want it to stay > 1
					index = index - 1;
				tempString[0] = messageBuffer[index-1];
				messageBuffer[index-1] = NULL;
				Report("\n\rLAST");
				delKey(tempString);
				lastPressed = 'l';
				break;
			case(BUTTON_MUTE):
				Report("MUTE");
				//DATA1 = "{\"state\": {\n\r\"desired\" : {\n\r\"message\" : \"a\"\n\r}}}\n\r\n\r";
				//DATA1 = ("{\"state\": {\n\r\"desired\" : {\n\r\"message\" : \"%c\"\n\r}}}\n\r\n\r", messageBuffer[3]);
				//Running the ENTER command for the MASTER device
					Report("\n\rMaster Entered!\n\r");
					//Put each character from messageBuffer into the TX FIFO (for UART1)
					/*for(z=0; z<(index-1); z++){
						if(MASTER)
							UARTCharPut(UARTA1_BASE, messageBuffer[z]);
						else{
							tempString[0] = ((char)UARTCharGet(UARTA1_BASE));
							Outstr2(tempString);
							Report("\n\rtempString: %s\n\r", tempString);
						}
					} Commented out UART transmission because we are using AWS*/
					strcat(str2, messageBuffer);
					strcat(str2, boba2);
					DATA1 = str2;
					//Restful API main()
				    long lRetVal = -1;
				    //
				    // Initialize board configuration
				    //
				    BoardInit();

				    PinMuxConfig();

				    InitTerm();
				    //Connect the CC3200 to the local access point
				    lRetVal = connectToAccessPoint();
				    //Set time so that encryption can be used
				    lRetVal = set_time();
				    if(lRetVal < 0)
				    {
				        UART_PRINT("Unable to set time in the device");
				        LOOP_FOREVER();
				    }
				    //Connect to the website with TLS encryption
				    lRetVal = tls_connect();
				    if(lRetVal < 0)
				    {
				        ERR_PRINT(lRetVal);
				    }
				    http_post(lRetVal);

				    sl_Stop(SL_STOP_TIMEOUT);
				lastPressed = 'm';
				index-=1;
				break;
			default:
				Report("Unknown code %d", sum);
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
		Report("\n\rtemp: %d, index: %d", temp, index);
		sum=0;
		Report("\n\rMessege Buffer: ");
		for(y=0;y<100;y++){
			Report("%c", messageBuffer[y]);
		}
	}
}
