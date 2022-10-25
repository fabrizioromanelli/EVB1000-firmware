/*! ------------------------------------------------------------------------------------------------------------------
 * @file	DecaRanging.cpp
 * @brief	Defines the entry point for the windows application, and its main operational/display code
 *
 * @attention
 * 
 * Copyright 2008, 2013 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 */

#include "stdafx.h"
#include <winbase.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <commdlg.h>

#include "splash.h"
#include "DecaRanging.h"
#include "instance.h"
#include "deca_spinit.h"
#include "deca_spi.h"
#include "deca_vcspi.h"

#define TAG_ANCHOR_AUTO_LOG 0
#define MAX_LOADSTRING 100

// Defintions of size

#define MAIN_PANEL_HEIGHT       (150)
#define MAIN_PANEL_MIN_Y        (150-2)
#define MAIN_PANEL_MIN_WIDTH    (600)

#define DEBUG_PANEL_Y           (110)
#define DEBUG_PANEL_REF_HEIGHT  (MAIN_PANEL_MIN_Y - 4 - DEBUG_PANEL_Y)

#define LARGE_DISPLAY_DP2       0x01               // bit 0 == large display 2 decimal places, if off then 1 decimal place
#define LARGE_DISPLAY_MEAN8     0x10               // bit 4 == large display is mean of 8 (if both off == Instantaneous
#define LARGE_DISPLAY_MEAN4     0x20               // bit 5 == large display is mean of 4 (if both off == Instantanwous

// Function prototypes

// Global Variables:

HINSTANCE hInst;                                // current instance
HFONT hFont ;                                   // font handle

SPLASH mysplash;
uint32 startTime ;

TCHAR szTitle[MAX_LOADSTRING+20];               // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

HWND hWnd = 0;                                      // Main window handle

HWND hMainPanel;                                // handle to main grey panel at bottom

HWND hVer = 0 ;
HWND hTip = 0 ;

int displayDebugPanel = 0 ;
HWND hDebugPanel;                               // handle to debug panel
int debugPanelWidth = MAIN_PANEL_MIN_WIDTH ;
int debugPanelHeight = DEBUG_PANEL_REF_HEIGHT ;

int showLargeText = 0 ; // large text display of range measurements

int largeDisplayMode = LARGE_DISPLAY_MEAN8 ;    // Default display is mena of 8 with 1 decimal place

int accumulatorView = 0 ;
int accumulatorplotredraw = 1 ;
int accumulatorinstancehasnewdata = 0;
int instancehasnewdata = 0;
int rangesView = 0;
int accumulatorMagnitudesOnly = 0 ;
int accumulatorNormalise = 0 ;
int accumulatorFirstPathAlign = 1 ;

int viewFeetInches = 0 ;
int viewClockOffset = 0 ;

int logAllAcc = 0 ;
int logSPI = 0 ;

int usb2spirev = 0;
int initComplete = 0 ;                          // Wait for initialisation before polling status register
int runNotStop = 1 ;                            // Run=1, Stop = 0, for Pause / Resume functionality
int instance_mode = LISTENER ;
HWND hPauseButton  ;

HWND hStopLoggingButton = 0 ;
HWND hAccLogHistButton = 0 ;
HWND hAccLogHistButton2 = 0 ;
HWND hAccLogSaveButton = 0;
HWND hRoleComboBox;
HWND hBlinkAdvise ;                             // handle to blink advise text control
HWND hDelayTextBox;                             // handle to antenna delay text box

double antennaDelay  ;                          // This is system effect on RTD subtracted from local calculation.
double antennaDelay16 ;                         // holds antenna delay at 16 MHz PRF
double antennaDelay64 ;                         // holds antenna delay at 64 MHz PRF


UINT ll_RegAddr = 0 ;                           // low level register address
UINT ll_RegValue = 0 ;                          // low level register value

int haveSeenPause = 1 ;                         

UINT spi_Rate = DW_DEFAULT_SPI ;            // default SPI rate
UINT new_spi_Rate ;                             // temp value for update dialog

wchar_t *mode[3] = {_T("Listener"),_T("Tag"),_T("Anchor")};
wchar_t *tagmode[2] = {_T("Tag Blinking"),_T("Tag Ranging")};
wchar_t *anchID[4] = {_T("A1"),_T("A2"),_T("A3"),_T("A4")};

#define RANGING_WITH_STR_LEN    (40)
WCHAR rangingWithString[RANGING_WITH_STR_LEN+2];

#if (DECA_SUPPORT_SOUNDING==1 && DECA_KEEP_ACCUMULATOR==1)
static accBuff_t buffers[1];
#else
accBuff_t *buffers; //don't need this when DECA_SUPPORT_SOUNDING is off
#endif
// - - - - - - - - - - - - -
// Channel configuration data
//

typedef struct
{
    uint8 channel ;
    uint8 prf ;
    uint8 datarate ;
    uint8 preambleCode ;
    uint8 preambleLength ;
    uint8 pacSize ;
    uint8 nsSFD ;
	//uint16 sfdTO ;	//(plength + 1 + SFD length - PAC) //SFD timeout
    uint8 nsPreambleLength ;
    uint8 nsPreambleCode;
    uint8 nsDPScodes ;
	int sniffOn;
	int sniffOff;
} chConfig_t ;


#if (SNIFF_SELECTION == 1)
chConfig_t chConfig = { 2,              // channel
						DWT_PRF_64M,    // prf
						DWT_BR_6M8,    // datarate
                        9,             // preambleCode
                        DWT_PLEN_128,	// preambleLength
                        DWT_PAC8,		// pacSize
                        0,		// non-standard SFD
						//(2048 + 1 + 64 - 64),     // (plength + 1 + SFD length - PAC) //SFD timeout
						1,
						0,
						0,
						2,
						48
                        } ;       
#else
chConfig_t chConfig = { 2,              // channel
						DWT_PRF_64M,    // prf
						DWT_BR_110K,    // datarate
                        9,             // preambleCode
                        DWT_PLEN_1024,	// preambleLength
                        DWT_PAC32,		// pacSize
                        1,		// non-standard SFD
						//(1024 + 1 + 64 - 32),     // (plength + 1 + SFD length - PAC) //SFD timeout
						1,
						0,
						0,
						0,
						0
                        } ;  
#endif

chConfig_t tempConfig ;  // store temp changes pending "OK" or "Cancel" operations

enum {   MB_RESTART, MB_ROLE, 
            MB_ANTDELAY, MB_CEARCOUNTS, MB_PAUSE, MB_CONFIG, MB_STOPLOGGING, MB_ACCLOGHIST, MB_ACCLOGHIST2, MB_ACCLOGSAVE, MB_NUM_MOVING_BUTTONS } ;
struct {
    HWND    handle ;
    int     xoffs ;
    int     yoffs ;
} movingButtons[MB_NUM_MOVING_BUTTONS] ;

int movingPanelY = MAIN_PANEL_MIN_Y ;

#define AP_HIST_SIZE (20) //we can hold last 20 accumulators (mag, real and imag)...
int ap_index = 0;
accumPlotInfo_t ap[AP_HIST_SIZE] ;
rangesPlotInfo_t rp ;
int rp_index = 0;

// - - - - - - - - - - - - -
// User Payload Configuration data
//

typedef struct
{
    int poll_period_ms ;
	int blink_period_ms;
	int anc_resp_dly_ms ;
    int tag_resp_dly_ms;
} timingConfig_t ;

timingConfig_t timingConfig = {
    POLL_PERIOD_DEFAULT_MS,	     // tag poll period
    BLINK_PERIOD_DEFAULT_MS,	 // tag blink period
    ANC_RESP_DLY_DEFAULT_MS,     // default anchor response delay
    TAG_RESP_DLY_DEFAULT_MS	     // default tag response delay
};

timingConfig_t tempTimingConfig ;              // a copy for temp update in dialog

// - - - - - - - - - - - - -
//
// Function prototypes --  Forward declarations of functions included in this code module:
//

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    ChanConfig(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    ContFrameConfig(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    CWModeConfig(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    TagSelectAddress(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK    TimingConfig(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    RegAccess(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    Reg2File(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    File2Reg(HWND, UINT, WPARAM, LPARAM);
void                processLogEvent(void) ;
void			    Toggle_runNotStop(void);

void setlargedisplayoptions(HWND hWnd) ;
void plotlargerangetext(HWND hDrawWindow,currentRangeingInfo_t *rangeData);
int validateeditboxint(HWND hWnd, int min, int max) ;
double validateeditboxdouble(HWND hWnd, double min, double max) ;
int validatedialogboxhex(HWND hDlg, int editboxID, UINT min, UINT max, UINT *result) ;
int validatedialogboxhex64(HWND hDlg, int editboxID, UINT64 min, UINT64 max, UINT64 *result) ;
void setdialogboxhex(HWND hDlg, int editboxID, UINT value, UINT width) ;
void setdialogboxhex64(HWND hDlg, int editboxID, UINT64 value);
UINT validatedialogboxdecuint(HWND hDlg, int editboxID, UINT min, UINT max, UINT *result) ;
void setdialogboxdecuint(HWND hDlg, int editboxID, UINT value, UINT width) ;

void updateblinksseentext(void);

void setroledropdownlist(HWND hDlg, UINT8 roleIndex) ;
int getroledropdownlist(HWND hDlg) ;

void restartinstance(void);
static UINT_PTR timerID = 0 ;

extern "C" instance_data_t instance_data[NUM_INST] ;
// - - - - - - - - - - - - -
//
// Functions
//
uint32 getmstime(void)
{
    uint32 msTime ;
    _SYSTEMTIME sysTime ;

    GetLocalTime(&sysTime);
    msTime = sysTime.wMilliseconds + sysTime.wSecond*1000 + sysTime.wMinute*60*1000 + sysTime.wHour*60*60*1000 ;

    return msTime ;
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.
    MSG msg;
    HACCEL hAccelTable;

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_DECARANGING, szWindowClass, MAX_LOADSTRING);

    MyRegisterClass(hInstance);

    mysplash.Init(hInstance,IDB_SPLASH);
    mysplash.Show();

    startTime = getmstime();

#ifndef NDEBUG
    mysplash.Hide() ;  // hide splash immediately in debug build 
#endif

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DECARANGING));

	
	//SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0))
    {

        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            if (msg.hwnd == hDelayTextBox)          // if it is the delay entry text box window
            {
                if (msg.message == WM_KEYUP)        // key up, so end of keypress
                {
                    switch (msg.wParam)             // identify what key was released ?
                    {
                    case 13 : // carriage return
                    case 9  : // Tab
                    case 27 : // Escape

                        SetFocus(hWnd) ;            // set focus to main window (away from the Text box !)

                        continue ;                  // get next message don't do any more processing for this one !
                    }
                }
            }                                       // any other condition continue to process

            if (!IsDialogMessage(hWnd, &msg))
            {

/*                switch (msg.message)
                {
                case 15 : // 15 happens all the time don't know what it is
                case 275: // WM_TIMER happens all the time time ?
                case 512 : // WM_MOUSEMOVE
                case 280 : // undocumented seems to be in time with cursor flashing in edit box
                    break ;
                default:
                    printf("MSG(%i) HW(%i) W(%i) L(%i)\n",msg.message,msg.hwnd,msg.wParam,msg.lParam) ;
                }
*/

                /* Translate virtual-key messages into character messages */
                TranslateMessage(&msg);
                /* Send message to WindowProcedure */
                DispatchMessage(&msg);
            }


        //  TranslateMessage(&msg);
        //  DispatchMessage(&msg);
        }

        if (initComplete)   // if application is not paused (and initialsiation completed)
        {
            if(mysplash.SHOWING)
            {
                uint32 nowTime ;
                nowTime = getmstime();
                if ((nowTime-startTime) > 800 ) // kill splash after 800 milliseconds
                {
                    mysplash.Hide();
                }
            }
            else // splash not showing any more
            {

                if (instance_run(getmstime()))    // run it returns flag indicating status report has changed
                {
                    RECT rect ;
                    GetClientRect(hWnd,&rect) ;
                    rect.bottom = 112 ;                     // bottom of rectangle covers first four status lines only
                    if (!displayDebugPanel)                 // if debug panel is not active we have extra status info to display
                    {
                        rect.bottom = movingPanelY-1 ;  // bottom of rectangle lower down (just above main panel)
                    }
                    InvalidateRect(hWnd,&rect,0) ;          // cause redraw of status info.
                    updateblinksseentext() ;
                }

				if(instancegetrole() == TAG)
				{
					_stprintf_s(    szTitle,
									sizeof(szTitle)/sizeof(TCHAR),
									_T(" %s "),
									tagmode[1]) ;
    
					SetWindowText(hWnd,szTitle) ;
				}

				if(instancegetrole() == TAG_TDOA)
				{
					_stprintf_s(    szTitle,
									sizeof(szTitle)/sizeof(TCHAR),
									_T(" %s "),
									tagmode[0]) ;
    
					SetWindowText(hWnd,szTitle) ;
				}

                if (showLargeText)
                {
                    currentRangeingInfo_t rangeInfo;
                    instancegetcurrentrangeinginfo(&rangeInfo);
					plotlargerangetext(hDebugPanel,&rangeInfo);
                }

            }  // splash not showning

        }

        if (isinstancepaused())
        {
            if (!haveSeenPause)
            {
                haveSeenPause = 1 ;
                instanceaccumlogflush() ;
            }
        }
        else // not paused
        {
            haveSeenPause = 0 ;
        }

    } // end while message processing main loop

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DECARANGING));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCE(IDC_DECARANGING);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
}


void setLogging(int log)
{
	if(log == IDM_LOGALLACC)
	{
		logAllAcc ^= MF_CHECKED ;      // toggle   
        logSPI = 0 ;		// Force off if on
        processLogEvent() ;
	}
	else if(log == IDM_LOGSPI)
	{
		logSPI ^= MF_CHECKED ;         // toggle
		logAllAcc = 0 ;                // Force off if on
		processLogEvent() ;
	}
	else //off
	{
		logSPI = logAllAcc = 0 ;    // Force all off
        processLogEvent() ;
	}
}

// ======================================================
//
//   FUNCTION: inittestapplication  (Common initialisation stuff to initial setup and restarting tests)
//
int inittestapplication(int reset)
{
    instanceConfig_t instConfig ;
    uint32 devID ;
    int result ;
	uint8 buffer[1500];

    int handle = openspi() ;       // SPI open via Cheetah or VCP - handle used to indicate if opened sucessfully

	if (handle == DWT_ERROR)
    {
        // error opening spi
        MessageBoxA(NULL,"ERROR - Cannot open a cheetah device or connect to EVK1000 over COM port", "DecaWave Application", MB_OK | MB_ICONEXCLAMATION);
        exit(-1) ;
        // instance init could in theory fail for failure to allocate buffer memory, but this will never happen in PC environment
        // so only time handle is out of range then is if cannot talk to cheetah for some reason
	}

	usb2spirev = (handle >> 8);

	devID = instancereaddeviceid() ;

	if(DWT_DEVICE_ID != devID) 
		dwt_spicswakeup(buffer, 1500);
	
	devID = instancereaddeviceid() ;

    if ((devID & 0xFFFF0000) != 0xDECA0000)
    {
        // error seems cheetah read does not see spi of the DecaWave protoscen
        MessageBoxA(NULL,"ERROR - Failure to read ScenSor ID register, please ensure Cheetah is connected to ScenSor SPI or" 
			" if using USB connection please ensure all Cheetahs are disconnected.", "DecaWave Application", MB_OK | MB_ICONEXCLAMATION);
        exit(-1) ;
    }
    
	if (DWT_DEVICE_ID != devID)   // Means it is MP prototype
    { 
        // error seems cheetah read does not see spi of the DecaWave protoscen
        MessageBoxA(NULL,"ERROR - Unsupported Device (not MP), please consult DecaWave for advice. ", "DecaWave Application", MB_OK | MB_ICONEXCLAMATION);
        DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), NULL, About);
        exit(-1) ;
    }

    result = instance_init(buffers) ;       // reset, done as part of init

   if (result != DWT_SUCCESS)
   {
        MessageBoxA(NULL,"ERROR in initialising device", "DecaWave Application", MB_OK | MB_ICONEXCLAMATION);
        exit(-1) ;
    }

   if (initComplete == 0) // main init
   {
       instance_mode = LISTENER; //listener
       EnableMenuItem(GetMenu(hWnd),IDM_REGACCESS,MFS_DISABLED) ;  // Starting in run mode so disable the reg access menu
   }
   else
   {
        instancesetdisplayfeetinches(viewFeetInches) ;                               // enable display of feet and inches
		instancesetdisplayclockoffset(viewClockOffset) ;                              // enable display of clock offset
   }

    instancesetrole(instance_mode) ;                                                // Set this instance role

    if(instance_mode == LISTENER)
	{
		instcleartaglist();
		setLogging(0);
	}
	
	if(instance_mode == ANCHOR)
	{
		EnableMenuItem(GetMenu(hWnd),IDM_DISPCLKOFFSET,MFS_ENABLED) ;  // Anchor does Clock Offset calculation
        instancesetdisplayclockoffset(viewClockOffset) ;              // apply change
#if (TAG_ANCHOR_AUTO_LOG == 1)
		setLogging(IDM_LOGALLACC);
#endif
	}
	else
	{ 
		EnableMenuItem(GetMenu(hWnd),IDM_DISPCLKOFFSET,MFS_DISABLED) ;  // Tag/Listener does not do Clock Offset calculation
        viewClockOffset &= ~MF_CHECKED ; // uncheck if selected
        CheckMenuItem(GetMenu(hWnd),IDM_DISPCLKOFFSET,viewClockOffset) ;

        instancesetdisplayclockoffset(viewClockOffset) ;              // apply change
	}

#if (TAG_ANCHOR_AUTO_LOG == 1)
        if (instance_mode == TAG)
		    setLogging(IDM_LOGALLACC);
#endif

    instConfig.channelNumber = chConfig.channel ;
    instConfig.preambleCode = (chConfig.nsPreambleCode ? 0 : chConfig.preambleCode) ;
    instConfig.nsSFD = chConfig.nsSFD ;
	//instConfig.sfdTO = chConfig.sfdTO ; 
	instConfig.pacSize = chConfig.pacSize ;
    instConfig.pulseRepFreq = chConfig.prf ;  // should be one of NOMINAL_4M, NOMINAL_16M, or NOMINAL_64M

    instConfig.dataRate = chConfig.datarate ;
    instConfig.preambleLen = chConfig.preambleLength ;

    instance_config(&instConfig) ;                  // Set operating channel etc
	
    if (initComplete == 0)
	{ 
	    antennaDelay16 = instancegetantennadelay(DWT_PRF_16M);          // MP Antenna Delay at 16MHz PRF (read from OTP calibration data)
        antennaDelay64 = instancegetantennadelay(DWT_PRF_64M);          // MP Antenna Delay at 64MHz PRF (read from OTP calibration data)
        if (antennaDelay16 == 0) antennaDelay16 = DWT_PRF_16M_RFDLY;    // Set a value locally if not programmed in OTP cal data
        if (antennaDelay64 == 0) antennaDelay64 = DWT_PRF_64M_RFDLY;    // Set a value locally if not programmed in OTP cal data
	}

    if (instConfig.pulseRepFreq == DWT_PRF_64M)
    {
        antennaDelay = antennaDelay64 ;
    }
    else 
    {
        antennaDelay = antennaDelay16 ;
    }
    {
        TCHAR tempDelayString[20];
        _stprintf_s(tempDelayString,sizeof(tempDelayString)/sizeof(TCHAR),_T("%0.3f"),antennaDelay) ;
        SetWindowText(hDelayTextBox,tempDelayString) ;              // Set it into window.

    }

    instancesetantennadelays(antennaDelay) ;

    instance_set_user_timings(timingConfig.anc_resp_dly_ms, timingConfig.tag_resp_dly_ms,
                              timingConfig.blink_period_ms, timingConfig.poll_period_ms);
    instance_init_timings();

	instancesetsnifftimes(chConfig.sniffOn, chConfig.sniffOff);

    _stprintf_s(    szTitle,
                    sizeof(szTitle)/sizeof(TCHAR),
                    _T(" %s %s "),
                    mode[instance_mode], 
					rangingWithString
					) ;
    
    SetWindowText(hWnd,szTitle) ;

    //if (handle >= 2) exit(-1) ;  // Only allow 2 instances maximum
                      
    initComplete = 1 ;

    return handle ;
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance;               // Store instance handle in our global variable

   int handle ;

   // find screen width
   int cx = GetSystemMetrics(SM_CXSCREEN);
   // and height
   int cy = GetSystemMetrics(SM_CYSCREEN);

   int xs = 600 ;
   int ys = 600 ;

   int x = 32 + 1 * (xs + xs/8) ;
   int y = 64 ;

   if(cx == xs) cx = xs + 1;
   if(cy == ys) cy = ys + 1;


   if (x > (cx - xs))
   {
       x = rand() % (cx - xs) ;
       y = rand() % (cy - ys) ;
   }

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      x, y, xs, ys, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   {
       HWND hRoleGroup, hRestartButton ;
       HWND hDelayGroup ;
       HWND hClrCountsButton, hConfigureButton ;

       int panx, pany ;
       int rolx, roly ;
       int delx, dely ;

       // Retrieve a handle to the variable stock font.
       hFont = (HFONT)GetStockObject(ANSI_VAR_FONT); // DEVICE_DEFAULT_FONT, SYSTEM_FONT or ANSI_VAR_FONT, ANSI_FIXED_FONT
/*
       hFont = CreateFontW (-12, 0, 0, 0, FW_DONTCARE, false, false, false, OEM_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                DEFAULT_PITCH | FF_DONTCARE, _T("MS Shell Dlg"));  //"MS Shell Dlg"  Parameter is not being used properly !!
*/
       SendMessage(hWnd, WM_SETFONT, (WPARAM) hFont, 1) ;

       panx = 4 ; pany = MAIN_PANEL_MIN_Y ;
       rolx = 16 ; roly = 20 ;
       delx = 158 ; dely = 20 ;

       //create the panel  (second argument - class name (Button, ComboBox, Edit, ListBox, MDIClient, ScrollBar, Static,))
       hMainPanel = CreateWindowEx(0,_T("Static"),NULL, WS_VISIBLE | WS_CHILD | WS_DLGFRAME, panx, pany, MAIN_PANEL_MIN_WIDTH, 150, hWnd,(HMENU)  MAIN_PANEL_ID,NULL,NULL);
       SendMessage(hMainPanel, WM_SETFONT, (WPARAM) hFont, 1) ;

       {
       		int mode ;
            hRoleGroup = CreateWindowEx(0,_T("Button"),_T("Role"),
                                   BS_GROUPBOX|WS_VISIBLE|WS_CHILD,
                                   rolx, roly, 124, 60, hMainPanel,NULL,NULL,NULL);
            SendMessage(hRoleGroup, WM_SETFONT, (WPARAM) hFont, 1) ;
            
            //create the Role Group
            hRoleComboBox = CreateWindowEx(0,_T("ComboBox"),_T("Role"),
                                   CBS_DROPDOWNLIST|WS_VISIBLE|WS_CHILD,
                                   panx+rolx, pany+roly+20, 80, 164, hWnd,(HMENU) ROLE_ID,NULL,NULL);
            SendMessage(hRoleComboBox, WM_SETFONT, (WPARAM) hFont, 1) ;

            movingButtons[MB_ROLE].handle = hRoleComboBox ;
            movingButtons[MB_ROLE].xoffs = rolx+20 ;
            movingButtons[MB_ROLE].yoffs = roly+20 ;

            SendMessage(hRoleComboBox,CB_RESETCONTENT, 0, 0);
			SendMessage(hRoleComboBox,CB_ADDSTRING,0,(LPARAM) _T("Listener") );
            SendMessage(hRoleComboBox,CB_ADDSTRING,0,(LPARAM) _T("Tag") );
            SendMessage(hRoleComboBox,CB_ADDSTRING,0,(LPARAM) _T("Anchor") );

            instancesetrole(instance_mode) ;     // Set this instance role

			mode = instancegetrole() ;
            setroledropdownlist(hWnd, mode) ;  //modes 0,1,2 are listener, tag and anchor
       }
      
       {
            // Create text "control" on main panel to display number of tags/blinks seen
            hBlinkAdvise = CreateWindowEx(0,_T("Static"),_T(" "),
                                   WS_VISIBLE|WS_GROUP|WS_CHILD,
                                   rolx+10, roly+64, 124-6, 42, hMainPanel,NULL,NULL,NULL);
            SendMessage(hBlinkAdvise, WM_SETFONT, (WPARAM) hFont, 1) ;
       }


       //create the Delay Group
       hDelayGroup = CreateWindowEx(0,_T("Button"),_T("Antenna Delay (ns)"),
                                    BS_GROUPBOX|WS_VISIBLE|WS_CHILD,
                                    delx, dely, 127, 60, hMainPanel,NULL,NULL,NULL);
       SendMessage(hDelayGroup, WM_SETFONT, (WPARAM) hFont, 1) ;

       //create the Delay Text Box  (Sub function of hWnd window, rathar than hDelayGroup so can get at message handle proc easily)

        {
            TCHAR tempDelayString[20];
            // _stprintf(tempDelayString,_T("%0.3f"),antennaDelay) ;
            _stprintf_s(tempDelayString,sizeof(tempDelayString)/sizeof(TCHAR),_T("%0.3f"),antennaDelay) ;

            hDelayTextBox = CreateWindowEx(0,_T("Edit"),tempDelayString,
                                          BS_TEXT|WS_VISIBLE|WS_CHILD , // ES_AUTOHSCROLL
                                          panx+delx+21, pany+dely+20, 85, 20, hWnd,(HMENU) ANTENNA_DELAY_ID,NULL,NULL);
            SendMessage(hDelayTextBox, WM_SETFONT, (WPARAM) hFont, 1) ;
            //printf("Antenna Delay Text Edit box window handle is %i\n",hDelayTextBox) ;
            movingButtons[MB_ANTDELAY].handle = hDelayTextBox ;
            movingButtons[MB_ANTDELAY].xoffs = delx+21;
            movingButtons[MB_ANTDELAY].yoffs = dely+20 ;
        }

        //create the Restart Button (Sub function of hWnd window, rathar than hRoleGroup so can get at message handle proc easily)
        hRestartButton = CreateWindowEx(0,_T("Button"),_T("Restart"),
                                       BS_PUSHBUTTON|WS_VISIBLE|WS_CHILD|WS_TABSTOP,
                                       panx+310, pany+67, 75, 23, hWnd, (HMENU)RESTART_BUTTON_ID,NULL,NULL);
        SendMessage(hRestartButton, WM_SETFONT, (WPARAM) hFont, 1) ;
        movingButtons[MB_RESTART].handle = hRestartButton ;
        movingButtons[MB_RESTART].xoffs = 310 ;
        movingButtons[MB_RESTART].yoffs = 67 ;

        //create the Clear Counters Button (Sub function of hWnd window, rathar than hPanel so can get at message handle proc easily)
        hClrCountsButton = CreateWindowEx(0,_T("Button"),_T("Clear Counters"),
                                         BS_PUSHBUTTON|WS_VISIBLE|WS_CHILD|WS_TABSTOP,
                                         panx+167, pany+107, 108, 23, hWnd, (HMENU)CLEAR_COUNTS_BUTTON_ID,NULL,NULL);
        SendMessage(hClrCountsButton, WM_SETFONT, (WPARAM) hFont, 1) ;
        movingButtons[MB_CEARCOUNTS].handle = hClrCountsButton ;
        movingButtons[MB_CEARCOUNTS].xoffs = 167;
        movingButtons[MB_CEARCOUNTS].yoffs = 107 ;

        //create the Configure Button (Sub function of hWnd window, rathar than hPanel so can get at message handle proc easily)
        hConfigureButton = CreateWindowEx(0,_T("Button"),_T("Configure"),
                                         BS_PUSHBUTTON|WS_VISIBLE|WS_CHILD|WS_TABSTOP,
                                         panx+310, pany+107, 75, 23, hWnd, (HMENU)CONFIGURE_BUTTON_ID,NULL,NULL);
        SendMessage(hConfigureButton, WM_SETFONT, (WPARAM) hFont, 1) ;
        movingButtons[MB_CONFIG].handle = hConfigureButton ;
        movingButtons[MB_CONFIG].xoffs = 310;
        movingButtons[MB_CONFIG].yoffs = 107 ;

        //create the Pause Button (Sub function of hWnd window, rathar than hPanel so can get at message handle proc easily)
        hPauseButton = CreateWindowEx(0,_T("Button"),_T("Pause"),
                                     BS_PUSHBUTTON|WS_VISIBLE|WS_CHILD|WS_TABSTOP,
                                     panx+310, pany+24, 75, 23, hWnd, (HMENU)PAUSE_BUTTON_ID,NULL,NULL);
        SendMessage(hPauseButton, WM_SETFONT, (WPARAM) hFont, 1) ;
        movingButtons[MB_PAUSE].handle = hPauseButton ;
        movingButtons[MB_PAUSE].xoffs = 310;
        movingButtons[MB_PAUSE].yoffs = 24 ;

        movingButtons[MB_STOPLOGGING].handle = 0 ;      // dynamically created button not there initially
		
		movingButtons[MB_ACCLOGHIST].handle = 0 ;      // dynamically created button not there initially
		movingButtons[MB_ACCLOGHIST2].handle = 0 ;      // dynamically created button not there initially

   }

	handle = inittestapplication(1) ;                     // initialise for first time

   if(accumulatorFirstPathAlign) 
   {
	   accumulatorFirstPathAlign = MF_CHECKED ;        // set
	   CheckMenuItem(GetMenu(hWnd),IDM_ACCFPALIGN,accumulatorFirstPathAlign) ;
   }


#if (DECA_KEEP_ACCUMULATOR == 0)
    {
		HMENU hMenu = GetMenu(hWnd) ;
		EnableMenuItem(hMenu,IDM_ACCVIEW,MFS_DISABLED) ;
		EnableMenuItem(hMenu,IDM_ACCFPALIGN,MFS_DISABLED) ;
		EnableMenuItem(hMenu,IDM_ACCMAGONLY,MFS_DISABLED) ;
	}
#endif

#if ((DECA_LOG_ENABLE == 0) /*|| (DECA_ACCUM_LOG_SUPPORT==0)*/)
    {
		EnableMenuItem(GetMenu(hWnd),IDM_LOGALLACC,MFS_DISABLED) ;
	}
#endif

   setlargedisplayoptions(hWnd) ;

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

// =========================================================================

// Restart and re-configure

void restartinstance(void)
{

    logSPI = logAllAcc = 0 ;        // Force logging off
    processLogEvent() ;

    instance_close() ;                          // shut down instance, PHY, SPI close, etc.
    inittestapplication(1) ;                    // re-initialise

	usb2spirev = 0;
} // end restartinstance()

// =======================================================================================================================
// Accumulator Plotting Variables
// =======================================================================================================================

#define MAXPLOT_Y   (0x10000)
#define MINPLOT_Y   (-MAXPLOT_Y)

double plotMinX, plotMaxX, plotMinY, plotMaxY ;
double plotLimX = 0.0 ;
double xscale, yscale ;
int numPixelsX, numPixelsY ;
double normaliseScale ;

// =======================================================================================================================
// Accumulator Plotting Functions
// =======================================================================================================================


void flagaccumulatorpanelepaintneeded(void)
{
    RECT rect ;
    GetClientRect(hWnd,&rect) ;
    rect.top = DEBUG_PANEL_Y ;
    rect.bottom = DEBUG_PANEL_Y + debugPanelHeight ;
    InvalidateRect(hWnd,&rect,0) ;      // cause redraw of status info.
    accumulatorplotredraw = 1 ;
}

// -----------------------------------------------------------------------------------------------------------------------
// initialsise the plot area limits

void initaccplotsize(int sizeOfAccumulator, int good_data)
{
    // because of zoom, only want to do once... set plotLimX to zero when widow disabled so re-inits next time used

    // Also want to change if accumulator size changes e.g. because of PRF change)

    if ((plotLimX == 0.0) || ((plotLimX != sizeOfAccumulator) && good_data))                  
    {
        plotMinX = 0 ;
        plotMaxX = plotLimX = sizeOfAccumulator ;

        if (sizeOfAccumulator <= MAX_ACCUMULATOR_ELEMENTS)
        {
            plotMinY = MININT16/2 - 4000;
        	plotMaxY = MAXINT16/2 + 4000;
        }
        else
        {
            plotMinY = MININT16;
            plotMaxY = MAXINT16 ;
        }
    }
}

// -----------------------------------------------------------------------------------------------------------------------
// Compute the scaling factors

void setaccplotscale(HWND hDrawWindow)
{
    RECT rect ;

    GetClientRect(hDrawWindow,&rect) ;

    xscale = ((double)(numPixelsX = rect.right - rect.left)) / (plotMaxX - plotMinX) ;    // (Number of pixels) / ( Value Range)
    yscale = ((double)(numPixelsY = rect.bottom - rect.top)) / (plotMaxY - plotMinY) ;    // (Number of pixels) / ( Value Range)
}

// -----------------------------------------------------------------------------------------------------------------------
// X and Y Scaling functions

inline int scalexpoint(INT16 input)
{
    return (int)((((double)(input)) - plotMinX) * xscale) ;
}

inline int scaleypoint(INT16 input)
{
   return (int)(numPixelsY - ((input * normaliseScale - plotMinY) * yscale )) ;
}

inline int scaleypoint(INT32 input)
{
   return (int)(numPixelsY - ((input * normaliseScale - plotMinY) * yscale )) ;
}

inline int scalexpoint(double input)                                    // C++ overloading
{
    return (int)((((double)(input)) - plotMinX) * xscale) ;
}

inline int scaleypoint(double input)
{
   return (int)(numPixelsY - ((input * normaliseScale - plotMinY) * yscale)) ;
}

// -----------------------------------------------------------------------------------------------------------------------
// Main Plot Ranges history function, given window handle.
/*
void plotranges(HWND hDrawWindow, rangesPlotInfo_t *rp)
{

    PAINTSTRUCT ps;
    HDC hdc, hdcReal;
	INT16 i, count;

    INT16 ind, lastInd ;

    INT16 minIndx, maxIndx, markStep ;

    INT16 indOffset, indWrap ;
    INT pixOffset ;

    HPEN hPenKeep;
	HPEN hPenReal = CreatePen(PS_SOLID,2,RGB(255,0,0));                             // Red
    HPEN hPenImag = CreatePen(PS_SOLID,2,RGB(0,255,0));                             // Green
    //HPEN hPenMags = CreatePen(PS_SOLID,2,RGB(0,0,255));                             // Blue
	HPEN hPenMags = CreatePen(PS_SOLID,2,RGB(255,255,0));                             // Yellow
	HBRUSH hBrushTint = CreateSolidBrush(RGB(35,35,35));
    HRGN hrgn ;
    char buff[128] ;
    HBITMAP hBitmap ;

    RECT rect ;
    int x;

	if ((plotLimX == 0.0) || (plotLimX != rp->arraySize))                  
    {
        plotMinX = 0 ;
        plotMaxX = plotLimX = rp->arraySize ;

        if (rp->arraySize <= MAX_RANGE_ELEMENTS)
        {
            plotMinY = 0 ; //MININT16/2 - 4000;
        	plotMaxY = 2000 ;//MAXINT16/2 + 4000;
        }

    }   

	setaccplotscale(hDrawWindow) ;
    normaliseScale = 1.0 ;                                                          // will only scale normalise values

    GetClientRect(hDrawWindow,&rect) ;

    BeginPaint(hDrawWindow,&ps);
    hdcReal = GetDC(hDrawWindow);

    hdc = CreateCompatibleDC(NULL) ;

    hBitmap = CreateCompatibleBitmap(hdcReal,rect.right,rect.bottom);                 // assuming rectangls starts at 0,0

    SelectObject(hdc, hBitmap) ;

	SetBkColor(hdc,RGB(85,85,85)) ; 
	(HBRUSH)SelectObject(hdc,hBrushTint);                              // brush to tint rectangle


    i = 2 ;                                                                         // temp offset
    Rectangle(hdc,rect.left+i,rect.top+i,rect.right-i,rect.bottom-i);               // draw rectangle to clear the area

    i = 3 ;                                                                         // temp offset
    hrgn = CreateRectRgn(rect.left+i,rect.top+i,rect.right-i,rect.bottom-i) ;       // create a region ;

	SetBkColor(hdc,RGB(230,230,230)) ; 
    SelectClipRgn(hdc,hrgn) ;                                                       // select it as clipping area

    // Access software first path !!!!!!!!!!!!!!!!!!!!!!!!!!
    {
        pixOffset = 0 ;
        indOffset = 0 ;
        indWrap = 0 ;
    }

    // plot the X Axis

    MoveToEx(hdc,scalexpoint(plotMinX),scaleypoint((INT16)0),NULL);                 // this sets the drawing cursor
    LineTo(hdc,scalexpoint(plotMaxX),scaleypoint((INT16)0));                        // this draws the axis (with default black pen)

    minIndx = (INT16) (plotMinX) ;
    maxIndx = (INT16) (plotMaxX) ;

    {
        int size = (int)(plotMaxX - plotMinX) ;                                     // figure out size of X axis, and
        if( size > 200) markStep = 100 ;                                            // set markStep for marking off X-axis
        else if (size > 20) markStep = 10 ;
        else markStep = 1 ;
    }

	hPenKeep = (HPEN)SelectObject(hdc,hPenReal);                                    // switch on red pen (keep default for restore)  
    // plot the magnitude values
    SelectObject(hdc,hPenMags);                                                 // switch on  pen

    lastInd = MAXINT16 ;
    for (i = minIndx ; i <= maxIndx ; i++)
    {
        int x, y ;

        ind = i + indOffset ;
        if (ind < 0)            ind += indWrap ;
        else if (ind > indWrap) ind -= indWrap ;

		//j = ap->magvals[MAX_ACCUMULATOR_ELEMENTS];
        if (ind < lastInd)
        {
            MoveToEx(hdc,x=scalexpoint(ind)-pixOffset,y=scaleypoint(rp->magvals[ind]),NULL);     // this moves the drawing cursor without a line
        }
        else
        {	
            LineTo(hdc,x=scalexpoint(ind)-pixOffset,y=scaleypoint(rp->magvals[ind]));             // this draws to successive points
        }
        if ((accumulatorMagnitudesOnly) && (markStep <= 10))                        // i.r. a close up on magnitudes
        {
            // do something to indicate points
            SelectObject(hdc,hPenReal);                                                 // switch on red pen
            LineTo(hdc,x-2,y-2) ;                                                       // draw an X
            LineTo(hdc,x+2,y+2) ;
            MoveToEx(hdc,x,y,NULL) ;
            LineTo(hdc,x+2,y-2) ;
            LineTo(hdc,x-2,y+2) ;
            MoveToEx(hdc,x,y,NULL) ;
            SelectObject(hdc,hPenMags);                                                 // revert to MAG pen (blue)
        }
        lastInd = ind ;
    }

    normaliseScale = 1.0 ;                                                          // return to no normalise values

    // plot the window index as a virtical line (NB: it may not be visible)

    SelectObject(hdc,hPenKeep);                                                     // return to default pen

	
    i = 8 ;                                                                         // temp use to control Y location of text
    if (0 > (minIndx + indOffset))
    {
        x = scalexpoint((INT16)(-indOffset)) ;
        count = sprintf_s(buff,sizeof(buff),"%-4.0f", 0.0 ) ;
        TextOutA(hdc,4+x,numPixelsY/2+i,(LPCSTR)buff, count) ;                      // print min X value displayed
    }
    else
    {
        count = sprintf_s(buff,sizeof(buff),"%-4.0f", plotMinX + indOffset) ;
        TextOutA(hdc,4,numPixelsY/2+i,(LPCSTR)buff, count) ;                        // print min X value displayed
    }

    if ((indWrap) && (indWrap < (maxIndx + indOffset)))
    {
        x = scalexpoint((INT16)(indWrap - indOffset)) ;
        // printf("maxIndx(%i),indOffset(%i),indWrap(%i),  x(%i)\n",maxIndx,indOffset,indWrap,x) ;
        count = sprintf_s(buff,sizeof(buff),"%i", indWrap) ;
        TextOutA(hdc,x-20,numPixelsY/2+i,(LPCSTR)buff, count) ;                      // print max X value displayed
    }
    else
    {
        count = sprintf_s(buff,sizeof(buff),"%4.0f", plotMaxX + indOffset) ;
        TextOutA(hdc,numPixelsX-36,numPixelsY/2+i,(LPCSTR)buff, count) ;            // print max X value displayed
    }

    count = sprintf_s(buff,sizeof(buff),"%-4.0fcm", plotMaxY) ;
    TextOutA(hdc,6,4,(LPCSTR)buff, count) ;                                         // print max Y value displayed


    // markers on X axis

    SelectObject(hdc,hPenKeep);                                                     // return to default pen

    {
        int x ;
        int16 start ;

        // printf("\nStep(%i) indOffset(%i) plotMaxX(%f)\n",markStep,indOffset,plotMaxX) ;

        // start = floor(plotMinX / markStep) * markStep ;

        start = (int16) (floor((plotMinX + indOffset) / markStep) * markStep) ;

        if ((plotMinX < 1) || (plotMinX + indOffset < 1))
        {
            start = 0 ;
        }

        while ((start < plotMaxX + indOffset) || (start < plotMaxX))
        {
            MoveToEx(hdc,(x = scalexpoint(start)-pixOffset),scaleypoint(plotMinY*0.1),NULL);      // this sets the drawing cursor
            LineTo(hdc,x,scaleypoint(plotMaxY*0.1));                                    // this draws the marker line

            // printf("Start(%i),Ind(%i),X(%i)\n",start,ind, x) ;

            start+= markStep ;
                                      // print max Y value displayed
        }
    }

    {
        int x ;
        int16 start = 0 ;

        while (start < plotMaxY)
        {
            MoveToEx(hdc, 0, (x = scaleypoint(start)), NULL);      // this sets the drawing cursor
            LineTo(hdc, 10, x);                                    // this draws the marker line
			
			if((start % 200) == 0)
			{
				count = sprintf_s(buff,sizeof(buff),"%-4dcm", start) ;
				TextOutA(hdc,10,scaleypoint(start),(LPCSTR)buff, count) ;
			}
            
			start+= 100 ;
        }
    }

    // tidy up

    SelectClipRgn(hdc,NULL) ;                                                       // return to default clipping area (I hope)

    DeleteObject(hrgn) ;                                                            // delete clipping region object

    DeleteObject(hPenReal);                                                         // delete pen objects
    DeleteObject(hPenImag);
    DeleteObject(hPenMags);

    BitBlt(hdcReal,2,2,rect.right-4,rect.bottom-4,hdc,2,2,SRCCOPY) ;                // PAINT THE SCREEN !!!!

    DeleteObject(hBitmap) ;
    DeleteDC(hdc) ;

    ReleaseDC(hDrawWindow,hdcReal);

    EndPaint(hDrawWindow,&ps);

}
*/



// -----------------------------------------------------------------------------------------------------------------------
// Main Plot Accumulator function, given window handle.

void plotaccumulator(HWND hDrawWindow, accumPlotInfo_t *ap)
{

    PAINTSTRUCT ps;
    HDC hdc, hdcReal;
	INT16 i, count;
#if (DECA_KEEP_ACCUMULATOR==1)
    INT16 ind, lastInd ;
#endif
    INT16 minIndx, maxIndx, markStep ;

    INT16 indOffset, indWrap ;
    INT pixOffset ;

    HPEN hPenKeep;
    HPEN hPenReal = CreatePen(PS_SOLID,2,RGB(255,0,0));                             // Red
    HPEN hPenImag = CreatePen(PS_SOLID,2,RGB(0,255,0));                             // Green
    HPEN hPenMags = CreatePen(PS_SOLID,2,RGB(0,0,255));                             // Blue
    HPEN hPenLead = CreatePen(PS_SOLID,2,RGB(0,255,255));                           // Cyan
    //HPEN hPenHwrPeak = CreatePen(PS_SOLID,2,RGB(255,200,15));                        // Yellow
	HPEN hPenHwrLead = CreatePen(PS_SOLID,2,RGB(255,128,0));                        // Orange
    HRGN hrgn ;
    char buff[128] ;
    HBITMAP hBitmap ;
#if (DECA_BADF_ACCUMULATOR==1)
    HBRUSH hBrushKeep ;
    HBRUSH hBrushTint = CreateSolidBrush(RGB(255,255,204));                         // brush to tint background when error detected
#endif

    RECT rect ;
    int x;

    initaccplotsize(ap->accumSize,(ap->cp[0] == 'O')) ;                                       // test here determines if data is good
    setaccplotscale(hDrawWindow) ;
    normaliseScale = 1.0 ;                                                          // will only scale normalise values

    GetClientRect(hDrawWindow,&rect) ;

    BeginPaint(hDrawWindow,&ps);
    hdcReal = GetDC(hDrawWindow);

    hdc = CreateCompatibleDC(NULL) ;
    hBitmap = CreateCompatibleBitmap(hdcReal,rect.right,rect.bottom);                 // assuming rectangls starts at 0,0

    SelectObject(hdc, hBitmap) ;
#if (DECA_BADF_ACCUMULATOR==1)
    if (ap->cp[0] != 'O') // not "OKAY" so assume error
    {
        hBrushKeep = (HBRUSH)SelectObject(hdc,hBrushTint);                          // brush to tint rectangle
    }
#endif
    i = 2 ;                                                                         // temp offset
    Rectangle(hdc,rect.left+i,rect.top+i,rect.right-i,rect.bottom-i);               // draw rectangle to clear the area

    i = 3 ;                                                                         // temp offset
    hrgn = CreateRectRgn(rect.left+i,rect.top+i,rect.right-i,rect.bottom-i) ;       // create a region ;

    SelectClipRgn(hdc,hrgn) ;                                                       // select it as clipping area

    // Access software first path !!!!!!!!!!!!!!!!!!!!!!!!!!

    if (accumulatorFirstPathAlign)
    {
		pixOffset = (INT)(ap->hwrFirstPath * xscale - plotLimX * xscale * 3 / 5.0) ;        // difference between first path and 1/3 way through in pixels
		indOffset = (INT16) (ap->hwrFirstPath - (ap->accumSize * 3 / 5))  ;		
		
		indWrap = ap->accumSize ;
        // printf("offsets pix:%i ind%i\n",pixOffset,indOffset ) ;
    }
    else
    {
        pixOffset = 0 ;
        indOffset = 0 ;
        indWrap = 0 ;
    }

    // plot the X Axis

    MoveToEx(hdc,scalexpoint(plotMinX),scaleypoint((INT16)0),NULL);                 // this sets the drawing cursor
    LineTo(hdc,scalexpoint(plotMaxX),scaleypoint((INT16)0));                        // this draws the axis (with default black pen)

    minIndx = (INT16) (plotMinX) ;
    maxIndx = (INT16) (plotMaxX) ;

    {
        int size = (int)(plotMaxX - plotMinX) ;                                     // figure out size of X axis, and
        if( size > 200) markStep = 100 ;                                            // set markStep for marking off X-axis
        else if (size > 20) markStep = 10 ;
        else markStep = 1 ;
    }

    // plot the real values
    hPenKeep = (HPEN)SelectObject(hdc,hPenReal);                                    // switch on red pen (keep default for restore)
#if (DECA_KEEP_ACCUMULATOR==1)
    // draw real/imaginarry curves if enabled, or if disabled and there is no magnitudes then we need to draw sumething
	if ((!accumulatorMagnitudesOnly) || !(ap->hwrFirstPath > 0))
    {
        lastInd = MAXINT16 ;

        for (i = minIndx ; i <= maxIndx ; i++)
        {
            ind = i + indOffset ;
            if (ind < 0)            ind += indWrap ;
            else if (ind > indWrap) ind -= indWrap ;
            if (ind < lastInd)
            {
                MoveToEx(hdc,scalexpoint(ind)-pixOffset,scaleypoint(ap->accvals[ind].real),NULL);     // this moves the drawing cursor without a line
            }
            else
            {
                LineTo(hdc,scalexpoint(ind)-pixOffset,scaleypoint(ap->accvals[ind].real));             // this draws to successive points
            }
            lastInd = ind ;
        }

        // plot the imaginary values

        SelectObject(hdc,hPenImag);                                                     // switch on  pen

        lastInd = MAXINT16 ;
        for (i = minIndx ; i <= maxIndx ; i++)
        {
            ind = i + indOffset ;
            if (ind < 0)            ind += indWrap ;
            else if (ind > indWrap) ind -= indWrap ;

            if (ind < lastInd)
            {
                MoveToEx(hdc,scalexpoint(ind)-pixOffset,scaleypoint(ap->accvals[ind].imag),NULL);     // this moves the drawing cursor without a line
            }
            else
            {
                LineTo(hdc,scalexpoint(ind)-pixOffset,scaleypoint(ap->accvals[ind].imag));             // this draws to successive points
            }
            lastInd = ind ;
        }
    }

    
    // plot the magnitude values

    SelectObject(hdc,hPenMags);                                                 // switch on  pen

    lastInd = MAXINT16 ;
    for (i = minIndx ; i <= maxIndx ; i++)
    {
        int x, y ;

        ind = i + indOffset ;
        if (ind < 0)            ind += indWrap ;
        else if (ind > indWrap) ind -= indWrap ;

		//j = ap->magvals[MAX_ACCUMULATOR_ELEMENTS];
        if (ind < lastInd)
        {
            MoveToEx(hdc,x=scalexpoint(ind)-pixOffset,y=scaleypoint(ap->magvals[ind]),NULL);     // this moves the drawing cursor without a line
        }
        else
        {	
            LineTo(hdc,x=scalexpoint(ind)-pixOffset,y=scaleypoint(ap->magvals[ind]));             // this draws to successive points
        }
        if ((accumulatorMagnitudesOnly) && (markStep <= 10))                        // i.r. a close up on magnitudes
        {
            // do something to indicate points
            SelectObject(hdc,hPenReal);                                                 // switch on red pen
            LineTo(hdc,x-2,y-2) ;                                                       // draw an X
            LineTo(hdc,x+2,y+2) ;
            MoveToEx(hdc,x,y,NULL) ;
            LineTo(hdc,x+2,y-2) ;
            LineTo(hdc,x-2,y+2) ;
            MoveToEx(hdc,x,y,NULL) ;
            SelectObject(hdc,hPenMags);                                                 // revert to MAG pen (blue)
        }
        lastInd = ind ;
    }

    normaliseScale = 1.0 ;                                                          // return to no normalise values
#endif
    // plot the window index as a virtical line (NB: it may not be visible)

    SelectObject(hdc,hPenKeep);                                                     // return to default pen

	if (ap->hwrFirstPath != -1)  // magnitude and leadpath not computed
    {
        MoveToEx(hdc,(x = scalexpoint(ap->windInd)-pixOffset),scaleypoint(plotMinY/2),NULL);   // this sets the drawing cursor
        LineTo(hdc,x,scaleypoint(plotMaxY/2));                                    // this draws the leading path line

        SetBkMode(hdc, TRANSPARENT);

        // plot the first path as a virtical line if it is visible
		//only HW FP
		{
			x = scalexpoint(ap->hwrFirstPath)-pixOffset ;
			if (x > rect.right / 2)                                                     // leading path is to right of the area
			{
				count = sprintf_s(buff,sizeof(buff),"FPhw %4.3f", ap->hwrFirstPath) ;    // write text to its left
				
				TextOutA(hdc,x-102,numPixelsY-20,(LPCSTR)buff, count) ;

				//count = sprintf_s(buff,sizeof(buff),"live %4.3f", ap->liveLDE) ;    // write text to its left
				
				//TextOutA(hdc,x-102,numPixelsY-40,(LPCSTR)buff, count) ;

				/*if (numPixelsY > DEBUG_PANEL_REF_HEIGHT+40)
				{
					count = sprintf_s(buff,sizeof(buff),"FP SNR %2.1f dB", ap->fpSNR) ;
					TextOutA(hdc,x-110,numPixelsY-40,(LPCSTR)buff, count) ;
					if (numPixelsY > DEBUG_PANEL_REF_HEIGHT+80)
					{
						count = sprintf_s(buff,sizeof(buff),"FP rel %2.1f dB", ap->fpRelAtten) ;
						TextOutA(hdc,x-106,numPixelsY-60,(LPCSTR)buff, count) ;
					}
				}*/
			}
			else                                                                        // leading path is to the left of the area
			{
				count = sprintf_s(buff,sizeof(buff),"FPhw %-4.3f", ap->hwrFirstPath) ;    // write text to its right
				
				TextOutA(hdc,x+4,numPixelsY-20,(LPCSTR)buff, count) ;

				//count = sprintf_s(buff,sizeof(buff),"live %4.3f", ap->liveLDE) ;    // write text to its right
				
				//TextOutA(hdc,x+4,numPixelsY-40,(LPCSTR)buff, count) ;

				/*if (numPixelsY > DEBUG_PANEL_REF_HEIGHT+40)
				{
					count = sprintf_s(buff,sizeof(buff),"FP SNR %-2.1f dB", ap->fpSNR) ;
					TextOutA(hdc,x+4,numPixelsY-40,(LPCSTR)buff, count) ;
					if (numPixelsY > DEBUG_PANEL_REF_HEIGHT+80)
					{
						count = sprintf_s(buff,sizeof(buff),"FP rel %-2.1f dB", ap->fpRelAtten) ;
						TextOutA(hdc,x+4,numPixelsY-60,(LPCSTR)buff, count) ;
					}
				}*/
			}
		}

    }
	
	// print acc index (so we know which one we are looking at, e.g. when scrolling)
    //count = sprintf_s(buff,sizeof(buff),"ACC ind %d", ap_index) ;    // write text to its right
    //TextOutA(hdc,rect.left+50,rect.top+64,(LPCSTR)buff, count) ;

	if(ap->hwrFirstPath != -1)
	{
		// plot the Hardware supplied first path as a virtical line (NB: it may not be visible)
		SelectObject(hdc,hPenHwrLead);                                                  // return to default pen

		MoveToEx(hdc,(x = scalexpoint(ap->hwrFirstPath)-pixOffset),scaleypoint(plotMinY),NULL);   // this sets the drawing cursor
		LineTo(hdc,x,scaleypoint(plotMaxY));                                          // this draws the leading path line
		
		// plot the Hardware supplied first path as a virtical line (NB: it may not be visible)
		//SelectObject(hdc,hPenHwrPeak);                                                  // return to default pen

		//MoveToEx(hdc,(x = scalexpoint(ap->hwrPeakPath)-pixOffset),scaleypoint(plotMinY),NULL);   // this sets the drawing cursor
		//LineTo(hdc,x,scaleypoint(plotMaxY));                                          // this draws the leading path line
		
	}
    i = 8 ;                                                                         // temp use to control Y location of text
    if (0 > (minIndx + indOffset))
    {
        x = scalexpoint((INT16)(-indOffset)) ;
        count = sprintf_s(buff,sizeof(buff),"%-4.0f", 0.0 ) ;
        TextOutA(hdc,4+x,numPixelsY/2+i,(LPCSTR)buff, count) ;                      // print min X value displayed
    }
    else
    {
        count = sprintf_s(buff,sizeof(buff),"%-4.0f", plotMinX + indOffset) ;
        TextOutA(hdc,4,numPixelsY/2+i,(LPCSTR)buff, count) ;                        // print min X value displayed
    }

    if ((indWrap) && (indWrap < (maxIndx + indOffset)))
    {
        x = scalexpoint((INT16)(indWrap - indOffset)) ;
        // printf("maxIndx(%i),indOffset(%i),indWrap(%i),  x(%i)\n",maxIndx,indOffset,indWrap,x) ;
        count = sprintf_s(buff,sizeof(buff),"%i", indWrap) ;
        TextOutA(hdc,x-20,numPixelsY/2+i,(LPCSTR)buff, count) ;                      // print max X value displayed
    }
    else
    {
        count = sprintf_s(buff,sizeof(buff),"%4.0f", plotMaxX + indOffset) ;
        TextOutA(hdc,numPixelsX-36,numPixelsY/2+i,(LPCSTR)buff, count) ;            // print max X value displayed
    }

    count = sprintf_s(buff,sizeof(buff),"%-4.0f", plotMaxY) ;
    TextOutA(hdc,6,4,(LPCSTR)buff, count) ;                                         // print max Y value displayed

#if (DECA_BADF_ACCUMULATOR==1)
    TextOutA(hdc,numPixelsX-76,4,ap->cp, 7) ;                                           // Frame Error Status, only space for 7 chars
#endif
    
	if(ap->totalSNR != -99.0f)
	{
		// for both total SNR and RSL the value is not accurate when RSL > -85
		count = sprintf_s(buff,sizeof(buff),"TotSNR %c %2.1f dBm (%2.1f)",(ap->RSL> -85)?'>':'=', ap->totalSNR, ap->totalSNRavg) ;
		TextOutA(hdc,numPixelsX-190,24,(LPCSTR)buff, count) ;                           // print total SNR
	}
	
	if(ap->RSL != -99.0f)
	{
		// for both total SNR and RSL the value is not accurate when RSL > -85
		count = sprintf_s(buff,sizeof(buff),"RSL %c %2.1f dBm (%2.1f)",(ap->RSL>-85)?'>':'=', ap->RSL, ap->RSLavg) ;
		TextOutA(hdc,numPixelsX-190,44,(LPCSTR)buff, count) ;                           // print peak Path SNR
	}
    // other status / debug items
	//count = sprintf_s(buff,sizeof(buff),"Max %d",ap->maxGrowthCIR) ;
	//TextOutA(hdc,numPixelsX-142,64,(LPCSTR)buff, count) ; 

    count = sprintf_s(buff,sizeof(buff),"PSC=%i", ap->preambleSymSeen) ;
    TextOutA(hdc,6,24,(LPCSTR)buff, count) ;                                         // print preamble symbol counter

	//count = sprintf_s(buff,sizeof(buff),"Gain=%i", ap->gain) ;
    //TextOutA(hdc,6,44,(LPCSTR)buff, count) ;                                         // print gain level

#if (DECA_BADF_ACCUMULATOR==1)
    if (ap->cp[0] != 'O') // not "OKAY" so assume error
    {
        SelectObject(hdc,hBrushKeep);                                               // return to default brush
    }
#endif
    // markers on X axis

    SelectObject(hdc,hPenKeep);                                                     // return to default pen

    {
        int x ;
        int16 start ;

        // printf("\nStep(%i) indOffset(%i) plotMaxX(%f)\n",markStep,indOffset,plotMaxX) ;

        // start = floor(plotMinX / markStep) * markStep ;

        start = (int16) (floor((plotMinX + indOffset) / markStep) * markStep) ;

        if ((plotMinX < 1) || (plotMinX + indOffset < 1))
        {
            start = 0 ;
        }

        while ((start < plotMaxX + indOffset) || (start < plotMaxX))
        {
            MoveToEx(hdc,(x = scalexpoint(start)-pixOffset),scaleypoint(plotMinY*0.1),NULL);      // this sets the drawing cursor
            LineTo(hdc,x,scaleypoint(plotMaxY*0.1));                                    // this draws the marker line

            // printf("Start(%i),Ind(%i),X(%i)\n",start,ind, x) ;

            start+= markStep ;
        }
    }


    // tidy up

    SelectClipRgn(hdc,NULL) ;                                                       // return to default clipping area (I hope)

    DeleteObject(hrgn) ;                                                            // delete clipping region object
#if (DECA_BADF_ACCUMULATOR==1)
    DeleteObject(hBrushTint) ;                                                      // delete brush object
#endif
    DeleteObject(hPenReal);                                                         // delete pen objects
    DeleteObject(hPenImag);
    DeleteObject(hPenMags);
    DeleteObject(hPenLead);
    DeleteObject(hPenHwrLead);
	//DeleteObject(hPenHwrPeak);

    BitBlt(hdcReal,2,2,rect.right-4,rect.bottom-4,hdc,2,2,SRCCOPY) ;                // PAINT THE SCREEN !!!!

    DeleteObject(hBitmap) ;
    DeleteDC(hdc) ;

    ReleaseDC(hDrawWindow,hdcReal);

    EndPaint(hDrawWindow,&ps);

}


// =======================================================================================================================
// Display Range Information using a large font.

void plotlargerangetext(HWND hDrawWindow,currentRangeingInfo_t *rangeData)
{
    // PAINTSTRUCT ps;
    HDC hdc, hdcReal;
    char buff[128] ;
    HBITMAP hBitmap, holdBitmap  ;
    int yOffset = 0 ;
    INT16 i, cnt ;
    HRGN hrgn ;
    RECT rect ;
    double topValue ;

    hdcReal = GetDC(hDrawWindow);
    hdc = CreateCompatibleDC(NULL) ;

    GetClientRect(hDrawWindow,&rect) ;
    hBitmap = CreateCompatibleBitmap(hdcReal,rect.right,rect.bottom);               // assuming rectangls starts at 0,0
    holdBitmap = (HBITMAP) SelectObject(hdc, hBitmap) ;

    HBRUSH hBrushKeep ;
    HBRUSH hBrushTint ;

    if ((getmstime() - rangeData->lastRangeReportTime) < 5000) // Less than 5 seconds since range measuremnt 
    {
		hBrushTint = CreateSolidBrush(RGB(190,255,200));                         // background green
		SetBkColor(hdc,RGB(190,255,200)) ;      
    }
	else
	{
		hBrushTint = CreateSolidBrush(RGB(255,170,170));                         // background red
		SetBkColor(hdc,RGB(255,170,170)) ;
	} 
	
	hBrushKeep = (HBRUSH)SelectObject(hdc,hBrushTint);                              // brush to tint rectangle

    i = 2 ;                                                                         // temp offset
    Rectangle(hdc,rect.left+i,rect.top+i,rect.right-i,rect.bottom-i);               // draw rectangle to clear the area

    i = 3 ;                                                                         // temp offset
    hrgn = CreateRectRgn(rect.left+i,rect.top+i,rect.right-i,rect.bottom-i) ;       // create a region ;

    SelectClipRgn(hdc,hrgn) ;                                                       // select it as clipping area

    HFONT hLargeFont, hSmallerFont, hOldFont;

    // Retrieve a handle to the variable stock font.
    hFont = (HFONT)GetStockObject(ANSI_VAR_FONT);

    // Select the variable stock font into the specified device context.
    hOldFont = (HFONT)SelectObject(hdc, hFont) ;

    SetTextColor(hdc,RGB(0,0,0)) ;                      // default black

    // depending on largeDisplayMode we show instantaneous or mean of 4 or mean of 8 as largest item
    
    if (largeDisplayMode & LARGE_DISPLAY_MEAN8)
    {
        topValue = rangeData->mean ;                // this is the mean (generally of 8)
    }
    else if (largeDisplayMode & LARGE_DISPLAY_MEAN4)
    {
        topValue = rangeData->mean4 ;               // this is the mean of 4
    }
    else
    {
        topValue = rangeData->idist ;                // this is the instantenaous range 
    }

    if (largeDisplayMode & LARGE_DISPLAY_DP2)       // if 2 decimal palces
    {
        cnt = sprintf_s(buff,sizeof(buff),"%7.2f m   ",topValue) ;
    }
    else
    {
        cnt = sprintf_s(buff,sizeof(buff),"%7.1f m   ",topValue) ;  // else 1 decimal place
    }

    int textHeight = (rect.bottom - rect.top) / 2 ;
    int textWidth = (rect.right - rect.left) / 10 ;

    //Logical units are device dependent pixels, so this will create a handle to a logical font that is 48 pixels in height.
    //The width, when set to 0, will cause the font mapper to choose the closest matching value.
    //The font face name will be Impact.
    hLargeFont = CreateFont( textHeight,textWidth,0,0,FW_DONTCARE,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
                CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY, VARIABLE_PITCH,TEXT("Impact"));

    hSmallerFont = CreateFont( textHeight/3,0,0,0,FW_DONTCARE,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
                CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY, VARIABLE_PITCH,TEXT("Impact"));


    SelectObject(hdc, hLargeFont);
            
//    Sets the coordinates for the rectangle in which the text is to be formatted.
//    SetRect(&rect, 100,100,700,200);
//    SetTextColor(hdc, RGB(255,0,0));
//    DrawTextA(hdc, (LPCSTR)buff, -1,&rect, CT_CENTER);

    TextOutA(hdc,10,10,(LPCSTR)buff, cnt) ;
    
    // write the average range 

    int offsetX = textWidth * 2 ;
    int offsetY = textHeight + textHeight/2 ; 

    SelectObject(hdc, hSmallerFont);

    if (largeDisplayMode & (LARGE_DISPLAY_MEAN8|LARGE_DISPLAY_MEAN4))                       // if top value is a mean
    {
        cnt = sprintf_s(buff,sizeof(buff),"Instant  =   %7.2f m    ",rangeData->idist) ;    // bottom value is instantenaous range 
    }
    else // bottom value is the standard mean
    {
        cnt = sprintf_s(buff,sizeof(buff),"Mean(%i)  =   %7.2f m    ",rangeData->numAveraged,rangeData->mean) ;
    }

    TextOutA(hdc,10+offsetX,10+offsetY,(LPCSTR)buff, cnt) ;

    // tidy up

    SetTextColor(hdc,RGB(0,0,0)) ;                      // default black

    if (hOldFont != NULL)
    {
        // Restore the original font.
        SelectObject(hdc, hOldFont);
    }

    DeleteObject(hSmallerFont) ;
    DeleteObject(hLargeFont) ;

    SelectClipRgn(hdc,NULL) ;                                                       // return to default clipping area (I hope)

    DeleteObject(hrgn) ;                                                            // delete clipping region object

    if (hBrushKeep != NULL)
    {
        SelectObject(hdc,hBrushKeep);                                               // revert to old brush
    }
    DeleteObject(hBrushTint) ;                                                      // delete brush object

    BitBlt(hdcReal,2,2,rect.right-4,rect.bottom-4,hdc,2,2,SRCCOPY) ;                // PAINT THE SCREEN !!!!

    SelectObject(hdc, holdBitmap) ;     // revert to original (does this tidy somethinhg up?)
    DeleteObject(hBitmap) ;

    DeleteDC(hdc) ;

    ReleaseDC(hDrawWindow,hdcReal);

    // EndPaint(hDrawWindow,&ps);

} // emd plotlargerangetext()

void prevAccButtonDisplay()
{
	if (hAccLogHistButton == 0)  // no button
	{
		//create a Button
		hAccLogHistButton = CreateWindowEx(0,_T("Button"),_T(" << "),
									BS_PUSHBUTTON|BS_MULTILINE|WS_VISIBLE|WS_CHILD|WS_TABSTOP,
									8+400,
									movingPanelY+65 ,
									47, 25, hWnd, (HMENU)PREVACC_BUTTON_ID,NULL,NULL);
		movingButtons[MB_ACCLOGHIST].handle = hAccLogHistButton ;
		movingButtons[MB_ACCLOGHIST].xoffs = 400;
		movingButtons[MB_ACCLOGHIST].yoffs = 65 ;
		//create a Button
		hAccLogHistButton2 = CreateWindowEx(0,_T("Button"),_T(" >> "),
									BS_PUSHBUTTON|BS_MULTILINE|WS_VISIBLE|WS_CHILD|WS_TABSTOP,
									8+452,
									movingPanelY+65 ,
									47, 25, hWnd, (HMENU)NXTACC_BUTTON_ID,NULL,NULL);
		movingButtons[MB_ACCLOGHIST2].handle = hAccLogHistButton2 ;
		movingButtons[MB_ACCLOGHIST2].xoffs = 452;
		movingButtons[MB_ACCLOGHIST2].yoffs = 65 ;
		//create a Button
		hAccLogSaveButton = CreateWindowEx(0,_T("Button"),_T("Save ACC"),
									BS_PUSHBUTTON|BS_MULTILINE|WS_VISIBLE|WS_CHILD|WS_TABSTOP,
									8+400,
									movingPanelY+105 ,
									99, 25, hWnd, (HMENU)SAVEACC_BUTTON_ID,NULL,NULL);
		movingButtons[MB_ACCLOGSAVE].handle = hAccLogSaveButton ;
		movingButtons[MB_ACCLOGSAVE].xoffs = 400;
		movingButtons[MB_ACCLOGSAVE].yoffs = 105 ;

        SendMessage(hAccLogHistButton, WM_SETFONT, (WPARAM) hFont, 1) ;
        SendMessage(hAccLogHistButton2, WM_SETFONT, (WPARAM) hFont, 1) ;
        SendMessage(hAccLogSaveButton, WM_SETFONT, (WPARAM) hFont, 1) ;

	}
	else //remove button
	{
		if (hAccLogHistButton)
		{
			DestroyWindow(hAccLogHistButton) ;
			movingButtons[MB_ACCLOGHIST].handle = hAccLogHistButton = 0 ;
		}

		if (hAccLogHistButton2)
		{
			DestroyWindow(hAccLogHistButton2) ;
			movingButtons[MB_ACCLOGHIST2].handle = hAccLogHistButton2 = 0 ;
		}

		if (hAccLogSaveButton)
		{
			DestroyWindow(hAccLogSaveButton) ;
			movingButtons[MB_ACCLOGSAVE].handle = hAccLogSaveButton = 0 ;
		}
	}
}

// =========================================================================
//
//  FUNCTION: debugPanelDisplay()
//
void debugPanelDisplay(void)
{
    if (displayDebugPanel)                         // close any window that is active
    {
        DestroyWindow(hDebugPanel) ;
    }

    displayDebugPanel = accumulatorView || showLargeText || rangesView;   //(boolean OR'ing)

    if (displayDebugPanel)                          // create panel if it is needed now.
    {
            hDebugPanel = CreateWindowEx(0,_T("Static"),NULL, WS_VISIBLE | WS_CHILD | WS_DLGFRAME,
                                     4, DEBUG_PANEL_Y, debugPanelWidth, debugPanelHeight,
                                                      hWnd,NULL,NULL,NULL);
//            SendMessage(hDebugPanel, WM_SETFONT, (WPARAM)(HFONT)GetStockObject(ANSI_FIXED_FONT), 1) ;
              SendMessage(hDebugPanel, WM_SETFONT, (WPARAM)hFont, 1) ;

    }
} // end debugPanelDisplay()

// =========================================================================
//
//  FUNCTION: processAccLogEvent()
//
void processLogEvent(void)
{

    HMENU hMenu = GetMenu(hWnd) ;
    int e = 0 ;
    

    CheckMenuItem(hMenu,IDM_LOGALLACC,logAllAcc) ;
    CheckMenuItem(hMenu,IDM_LOGSPI,logSPI) ;

    if (logAllAcc) e = instanceaccumlogenable(INST_LOG_ALL) ;
    else instanceaccumlogenable(INST_LOG_OFF) ; // disable (don't take error response)

    CheckMenuItem(hMenu,IDM_LOGSPI,logSPI) ;

    if (logSPI) e += spilogenable(1) ;
    else spilogenable(0) ;                      // disable (don't take error response)

    if (e)              
    {
        MessageBoxA(NULL,"ERROR - Log file creation Error", "DecaWave Application", MB_OK | MB_ICONEXCLAMATION);
        exit(-1) ;
    }

    int defxoffs = 310+96 ;
    int defyoffs = 13 ;

    if (logAllAcc || logSPI) // logging enabled
    {
        if (hStopLoggingButton == 0)  // no button
        {
/*
            //create a Button
            hStopLoggingButton = CreateWindowEx(0,_T("Button"),_T("STOP LOGGING"),
                                     BS_PUSHBUTTON|BS_MULTILINE|WS_VISIBLE|WS_CHILD|WS_TABSTOP,
                                     4+defxoffs,
                                     movingPanelY+defyoffs ,
                                     75, 52, hWnd, (HMENU)LOGOFFACC_BUTTON_ID,NULL,NULL);
            // SendMessage(hStopLoggingButton, WM_SETFONT, (WPARAM) hFont, 1) ;
            movingButtons[MB_STOPLOGGING].handle = hStopLoggingButton ;
            movingButtons[MB_STOPLOGGING].xoffs = defxoffs;
            movingButtons[MB_STOPLOGGING].yoffs = defyoffs ;
*/



            //create a Button
            hStopLoggingButton = CreateWindowEx(0,_T("Button"),_T("STOP LOGGING"),
                                     BS_PUSHBUTTON|BS_MULTILINE|WS_VISIBLE|WS_CHILD|WS_TABSTOP,
                                     defxoffs-2,
                                     movingPanelY+defyoffs+4 ,
                                     120, 30, hWnd, (HMENU)LOGOFFACC_BUTTON_ID,NULL,NULL);
            // SendMessage(hStopLoggingButton, WM_SETFONT, (WPARAM) hFont, 1) ;
            movingButtons[MB_STOPLOGGING].handle = hStopLoggingButton ;
            movingButtons[MB_STOPLOGGING].xoffs = defxoffs-2;
            movingButtons[MB_STOPLOGGING].yoffs = defyoffs+4 ;

        }
    }
    else
    {
        if (hStopLoggingButton != 0)  // is a button
        {
            DestroyWindow(hStopLoggingButton) ;
            movingButtons[MB_STOPLOGGING].handle = hStopLoggingButton = 0 ;
        }
    }
}

void Toggle_runNotStop(void)
{
    runNotStop ^= 1 ; // toggle Run/Stop status.
#if (DECA_KEEP_ACCUMULATOR==1)	
    if(runNotStop == 0) ap_index--; //if paused
    else ap_index++;

	if(ap_index < 0) ap_index = AP_HIST_SIZE-1;
	if(ap_index >= AP_HIST_SIZE) ap_index = 0;
    prevAccButtonDisplay();
#endif
    SetWindowText(hPauseButton, runNotStop ? _T("Pause") : _T("Resume")) ;

    instance_pause(runNotStop) ;    // pause and resume the instance state machine.

	if (runNotStop) 
        EnableMenuItem(GetMenu(hWnd),IDM_REGACCESS,MFS_DISABLED) ;  // When running disable the reg access menu
    else       
        EnableMenuItem(GetMenu(hWnd),IDM_REGACCESS,MFS_ENABLED) ;   // when stopped enable the reg access menu  
}

// ==============================================================================
// set large display options
void setlargedisplayoptions(HWND hWnd)
{
    HMENU hMenu = GetMenu(hWnd) ;

    CheckMenuItem(hMenu,IDM_LARGEINST,0) ;              // Turn all 3 ticks off
    CheckMenuItem(hMenu,IDM_LARGEAVE4,0) ;
    CheckMenuItem(hMenu,IDM_LARGEAVE8,0) ;
    
    switch (largeDisplayMode & (LARGE_DISPLAY_MEAN8|LARGE_DISPLAY_MEAN4))
    {
    case 0                   : CheckMenuItem(hMenu,IDM_LARGEINST,MF_CHECKED) ; break ;    // Set appriopriate tick  
    case LARGE_DISPLAY_MEAN8 : CheckMenuItem(hMenu,IDM_LARGEAVE8,MF_CHECKED) ; break ;  
    case LARGE_DISPLAY_MEAN4 : CheckMenuItem(hMenu,IDM_LARGEAVE4,MF_CHECKED) ; break ;   

    case LARGE_DISPLAY_MEAN8|LARGE_DISPLAY_MEAN4: 
    default :
        // Error case both set !!!
        largeDisplayMode &= ~ LARGE_DISPLAY_MEAN4 ;         // turn off mean of 4
        CheckMenuItem(hMenu,IDM_LARGEAVE8,MF_CHECKED) ;     // set tick for mean of 8
        break ;  
    }

    CheckMenuItem(hMenu,IDM_LARGE2DP,(largeDisplayMode&LARGE_DISPLAY_DP2)?MF_CHECKED:0) ;

} // end setlargedisplayoptions()

// =========================================================================
//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;

    PAINTSTRUCT ps;
    HDC hdc;

/*
    switch (message)
    {
    case WM_PAINT :     //  15 0x000F
    case WM_TIMER:      // 275 0x0113
    case WM_MOUSEMOVE : // 512 0x0200
    case WM_NCHITTEST : // 132 0x0084  mouse moving also
    case WM_SETCURSOR : //  32 0x0020
        break;
    default:
        printf ("hWnd %i, msg %04X, wparam %08X, lparam %08X\n", hWnd,  message,  wParam,  lParam) ;
        break;
    }
*/
	//if(WM_TIMER != message)
		//printf("message %x\n", message);

    switch (message)
    {
        case WM_COMMAND:

            // don't process commands if splash screen is showing
            if (mysplash.SHOWING) return DefWindowProc(hWnd, message, wParam, lParam);

            wmId    = LOWORD(wParam);
            wmEvent = HIWORD(wParam);

        //printf("WM_COMMAND %i %i %i\n", wmId, wmEvent, lParam) ;

        // Parse the menu selections:
        switch (wmId)
        {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;

            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            
			case IDM_CONTFRAME:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_CONTFRAME), hWnd, ContFrameConfig);
                break;
			
			case IDM_CWMODE:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_CWMODE), hWnd, CWModeConfig);
                break;

            case IDM_CHANSETUP:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_CHANSETUPDIALOG), hWnd, ChanConfig);
                break;

            case IDM_TIMINGSETUP:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_TIMINGDIALOG), hWnd, TimingConfig);
                break;

            case IDM_REGACCESS:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_REGACCESSDIALOG), hWnd, RegAccess);
                break;

			case IDM_ACCFPALIGN:
                {
                    accumulatorFirstPathAlign ^= MF_CHECKED ; // toggle
                    HMENU hMenu = GetMenu(hWnd) ;
                    CheckMenuItem(hMenu,IDM_ACCFPALIGN,accumulatorFirstPathAlign) ;
                    if (accumulatorView)
                    {
                        flagaccumulatorpanelepaintneeded() ;
                    }
                }
                break;

            case IDM_ACCMAGONLY:
                {
                    accumulatorMagnitudesOnly ^= MF_CHECKED ; // toggle
                    HMENU hMenu = GetMenu(hWnd) ;
					plotLimX = 0; //to reinitialise the graph axes
                    CheckMenuItem(hMenu,IDM_ACCMAGONLY,accumulatorMagnitudesOnly) ;
                    if (accumulatorView)
                    {
                        flagaccumulatorpanelepaintneeded() ;
                    }
                }
                break;


            case IDM_DISPFEETINCHES:
                {
                    viewFeetInches ^= MF_CHECKED ; // toggle
                    HMENU hMenu = GetMenu(hWnd) ;
                    CheckMenuItem(hMenu,IDM_DISPFEETINCHES,viewFeetInches) ;

                    instancesetdisplayfeetinches(viewFeetInches) ;              // apply change

                }
                break;

			case IDM_DISPCLKOFFSET:
                {
                    viewClockOffset ^= MF_CHECKED ; // toggle
                    HMENU hMenu = GetMenu(hWnd) ;
                    CheckMenuItem(hMenu,IDM_DISPCLKOFFSET,viewClockOffset) ;

                    instancesetdisplayclockoffset(viewClockOffset) ;              // apply change

                }
                break;

            case IDM_SOFTRESET:
                instancedevicesoftreset();              // do soft reset, and automatically
                restartinstance() ;                     // restart (generally needed anyway)
                break;

            case IDM_LARGETEXTVIEW:
                {
                    showLargeText ^= MF_CHECKED ; // toggle
                    HMENU hMenu = GetMenu(hWnd) ;
                    CheckMenuItem(hMenu,IDM_LARGETEXTVIEW,showLargeText) ;

                    if (accumulatorView)
                    {
                       CheckMenuItem(hMenu,IDM_ACCVIEW,0) ;
                       accumulatorView = 0 ;
                       plotLimX = 0.0 ;
                    }

                    /*if (rangesView)
                    {
                       rangesView = 0 ;
                       CheckMenuItem(hMenu,IDM_RNGVIEW,rangesView) ;
                    }*/

                    debugPanelDisplay() ; // turn display panel on/off

                }
                break ;

            case  IDM_LARGEINST:
                {
                    largeDisplayMode &= ~(LARGE_DISPLAY_MEAN8|LARGE_DISPLAY_MEAN4) ; 
                    setlargedisplayoptions(hWnd) ;
                }
                break;

            case  IDM_LARGEAVE4:
                {
                    largeDisplayMode &= ~(LARGE_DISPLAY_MEAN8|LARGE_DISPLAY_MEAN4) ;    // Clear bits
                    largeDisplayMode |= LARGE_DISPLAY_MEAN4 ;                           // Set Average of 4
                    setlargedisplayoptions(hWnd) ;
                }
                break;

            case  IDM_LARGEAVE8:
                {
                    largeDisplayMode &= ~(LARGE_DISPLAY_MEAN8|LARGE_DISPLAY_MEAN4) ;    // Clear bits
                    largeDisplayMode |= LARGE_DISPLAY_MEAN8 ;                           // Set Average of 8
                    setlargedisplayoptions(hWnd) ;
                }
                break;

            case IDM_LARGE2DP:
                {
                    largeDisplayMode ^= LARGE_DISPLAY_DP2 ; // toggle
                    setlargedisplayoptions(hWnd) ;
                }
                break;


            /*case IDM_RNGVIEW:
                {
                    rangesView ^= MF_CHECKED ; // toggle
                    HMENU hMenu = GetMenu(hWnd) ;
                    CheckMenuItem(hMenu,IDM_RNGVIEW,rangesView) ;
                    
					if (accumulatorView)
                    {
                       accumulatorView = 0 ;
                       CheckMenuItem(hMenu,IDM_ACCVIEW,showLargeText) ;
                    }

                    if (showLargeText)
                    {
                       showLargeText = 0 ;
                       CheckMenuItem(hMenu,IDM_LARGETEXTVIEW,showLargeText) ;
                    }

                    debugPanelDisplay() ; // turn display panel on/off
               }
                break;
				*/
            case IDM_ACCVIEW:
                {
                    accumulatorView ^= MF_CHECKED ; // toggle
                    HMENU hMenu = GetMenu(hWnd) ;
                    CheckMenuItem(hMenu,IDM_ACCVIEW,accumulatorView) ;

                    if (showLargeText)
                    {
                       showLargeText = 0 ;
                       CheckMenuItem(hMenu,IDM_LARGETEXTVIEW,showLargeText) ;
                    }

                    /*if (rangesView)
                    {
                       rangesView = 0 ;
                       CheckMenuItem(hMenu,IDM_RNGVIEW,rangesView) ;
                    }*/

					if (!accumulatorView)
                    {
                        plotLimX = 0.0 ;                                          // Acc View so flag need init plot limits again next time enabled
                    }
                    else
                    {
                        flagaccumulatorpanelepaintneeded() ;
                    }

                    debugPanelDisplay() ; // turn display panel on/off
               }
                break;

            case IDM_LOGALLACC:
					setLogging(IDM_LOGALLACC);
                    break ;

            case IDM_LOGSPI:
                    setLogging(IDM_LOGSPI);
                    break ;

			case PREVACC_BUTTON_ID:
                if(accumulatorView) //if paused
				{
					--ap_index;
					if(ap_index <= 0) ap_index = (AP_HIST_SIZE - 1);

					if (ap[ap_index].accumSize != 0)
					{
						plotaccumulator(hDebugPanel, &ap[ap_index]) ;	
					}
				}
				SetFocus(hWnd) ;                        // set focus to main window (away from the button) to allow key accelerators to work
                break ;

            case NXTACC_BUTTON_ID:
                    if(accumulatorView) //if paused
					{
						ap_index++;
						if(ap_index >= AP_HIST_SIZE) ap_index = 0;

						if (ap[ap_index].accumSize != 0)
						{
							plotaccumulator(hDebugPanel, &ap[ap_index]) ;	
						}
					}
					SetFocus(hWnd) ;                        // set focus to main window (away from the button) to allow key accelerators to work
                break ;

            case SAVEACC_BUTTON_ID:
                    if(accumulatorView) //if paused
					{
                        accumPlotInfo_t *apt = &ap[ap_index];
#if (DECA_KEEP_ACCUMULATOR==1)	
						int i;
#endif
						if(instanceaccumlogenable(INST_LOG_SINGLE) == 0)
						{
							FILE *logfile = instanceaccumloghandle(); 
							if(apt != NULL)
							{
							
                                //save header
                                fprintf(logfile,"\nAccum idx %i\n",ap_index) ;
                                //RX OK srt(018B A0713A00) WInd(0120), HLP(0000.0000), PSC(1009), SLP(0121.6194), SLPN(0120.3299), SLPS(0000.0000), RC(018B 66F8ACEB), Ofs(   61 in 7927104), FoM(00), TotSNR(05.6796), PeakSNR(-4.0383), fpSNR(30.1184), fpReLev(-00.7807), DIAG0(DEADDEAD), DIAG1(80452E5F), DIAG2(DE016400)
								fprintf(logfile,"\nRX %c%c srt(%04X %08X) ", apt->cp[0], apt->cp[1], 0,0) ;
								fprintf(logfile,"WInd(%04i), HLP(%09.4f), PSC(%04i), SLP(%09.4f), RC(%04X %08X)",
									apt->windInd, apt->hwrFirstPath, apt->preambleSymSeen, 0,0) ;
                                fprintf(logfile,", Ofs(%5i in %-5i), FoM(%02X)", 0,0,0) ;
                                fprintf(logfile,", TotSNR(%07.4f), RSL(%07.4f), fpSNR(%07.4f), fpReLev(%08.4f)",
                                    apt->totalSNR, apt->RSL, apt->fpSNR, apt->fpRelAtten) ;
                                
                                //save ACC data
								fprintf(logfile,"\nAccum Len %i\n",apt->accumSize) ;
#if (DECA_KEEP_ACCUMULATOR==1)						
								for (i = 0 ; i < apt->accumSize ; i++)
									fprintf(logfile,"%i, %i\n",apt->accvals[i].real, apt->accvals[i].imag) ;
#else
								fprintf(logfile,"\nDECA_KEEP_ACCUMULATOR is OFF!\n") ;
#endif
								//save FP data
								fprintf(logfile,"\nFP ind %4.3f, FPhw %4.3f (%1.3f)\n", 0, apt->hwrFirstPath, 0) ;  
								fprintf(logfile,"FP SNR %2.1f dB\n", apt->fpSNR) ;
								fprintf(logfile,"FP rel %2.1f dB\n", apt->fpRelAtten) ;

								fprintf(logfile,"TotSNR %c %2.1f dB\n",(apt->RSL> -85)?'>':'=', apt->totalSNR) ;
								fprintf(logfile,"PeakSNR %c %2.1f dB\n",(apt->RSL> -85)?'>':'=', apt->RSL) ;
								fprintf(logfile,"PSC=%i\n", apt->preambleSymSeen) ;

								fflush(logfile) ;
								
							}
							else
							{
								fprintf(logfile,"\nAccum has no data!!\n") ;
							}

							instanceaccumlogenable(INST_LOG_OFF);
						}


					}
					SetFocus(hWnd) ;                        // set focus to main window (away from the button) to allow key accelerators to work
                break ;

            case LOGOFFACC_BUTTON_ID:
				setLogging(0);
                break ;

            case CLEAR_COUNTS_BUTTON_ID:
				memset(rp.magvals, 0, MAX_RANGE_ELEMENTS);
				rp_index = 0;
                instanceclearcounts() ;
                SetFocus(hWnd) ;                        // set focus to main window (away from the button) to allow key accelerators to work
                break;

            case CONFIGURE_BUTTON_ID:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_CHANSETUPDIALOG), hWnd, ChanConfig);
                break;

            case PAUSE_BUTTON_ID:
				Toggle_runNotStop();
                SetFocus(hWnd) ;                        // set focus to main window (away from the button) to allow key accelerators to work
                break;

            case RESTART_BUTTON_ID:
				memset(rp.magvals, 0, MAX_RANGE_ELEMENTS);
				rp_index = 0;
                restartinstance() ;
                SetFocus(hWnd) ;                        // set focus to main window (away from the button) to allow key accelerators to work
                break;

            case ROLE_ID :

                if (wmEvent == 9)
                {
                    int new_instance_role = getroledropdownlist(hWnd) ;

                    switch (new_instance_role) 
                    {
                    case LISTENER :
                        instance_mode = new_instance_role ;
						swprintf(rangingWithString,RANGING_WITH_STR_LEN,_T(" ")) ;
                        break ;
                    case TAG :
                        instance_mode = new_instance_role ;
                        break ;
                    case ANCHOR:
                        if (instgettaglistlength() != 0)
                        {
                            DialogBox(hInst, MAKEINTRESOURCE(IDD_TAGSELECTDIALOG), hWnd, TagSelectAddress);
                            instance_mode = new_instance_role ;
                        }
                        else    // no blinks received 
                        {
                            instance_mode = LISTENER ;                     // force back to listener
                            setroledropdownlist(hWnd, instance_mode) ;     // and set role drop down list
                            MessageBoxA(NULL,"ERROR - Can't set anchor mode, no tag blinks received", "DecaWave Application", MB_OK | MB_ICONEXCLAMATION);
                        }
                        break ;
                    } // end switch on new role

                    restartinstance() ;

                    SetFocus(hWnd) ;
                }
                break;

            case ANTENNA_DELAY_ID:
                {
                    if (wmEvent == EN_KILLFOCUS)
                    {
                        antennaDelay = validateeditboxdouble(hDelayTextBox, 0.0, 10000.0) ;
                        //printf("Antenna Delay %f\n",antennaDelay) ;
                        instancesetantennadelays(antennaDelay) ;
                        if (chConfig.prf  == DWT_PRF_64M)
                        {
                            antennaDelay64 = antennaDelay ;
                        }
                        else 
                        {
                            antennaDelay16 = antennaDelay ;
                        }
                    }
                break;
                }
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);

        } // end switch (wmId)
        break; // end case WM_COMMAND

    case WM_SIZE:
        {
            // main window size changed - resize and move panel window.
            RECT rect ;
            int i, newY, newWidth ;

            GetWindowRect(hWnd,&rect) ;
            newY = rect.bottom - rect.top - 54 - MAIN_PANEL_HEIGHT - 4 ;
            newWidth = rect.right - rect.left - 22 ;
            if (newY < MAIN_PANEL_MIN_Y) newY = MAIN_PANEL_MIN_Y ;
            if (newWidth < MAIN_PANEL_MIN_WIDTH) newWidth = MAIN_PANEL_MIN_WIDTH ;
            SetWindowPos(hMainPanel,NULL,4,newY,newWidth,MAIN_PANEL_HEIGHT, SWP_NOZORDER) ;
            for ( i = 0 ; i < MB_NUM_MOVING_BUTTONS ; i++ )
            {
                if (movingButtons[i].handle != 0)
                {
                    SetWindowPos(movingButtons[i].handle,NULL,4+movingButtons[i].xoffs,newY+movingButtons[i].yoffs,0,0, SWP_NOZORDER | SWP_NOSIZE) ;
                }
            }
            //  resize debug panel
            debugPanelWidth = newWidth ;
            debugPanelHeight = newY - 4 - DEBUG_PANEL_Y ;
            if (displayDebugPanel)
            {
                SetWindowPos(hDebugPanel,NULL,4,DEBUG_PANEL_Y,debugPanelWidth, debugPanelHeight, SWP_NOZORDER) ;
                if (accumulatorView)
                {
                    flagaccumulatorpanelepaintneeded() ;
                }
            }
            movingPanelY = newY ;
        }
        break ;

    case WM_MOUSEWHEEL:
	//case WM_RBUTTONDOWN:
	//case WM_LBUTTONDOWN:
        if (accumulatorView)
        {
            int fwKeys = GET_KEYSTATE_WPARAM(wParam);                   // e.j control key is MK_CONTROL
            int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
            int yPos = lParam >> 16;                                    // screen co-ordinates
            int xPos = lParam & 0xFFFF ;
            RECT rect ;
            double xeqv ;

            double change = (plotMaxY - plotMinY) * (plotMaxX - plotMinX)  ;
			/*
			if(message == WM_RBUTTONDOWN)
				zDelta = -120;
			if(message == WM_LBUTTONDOWN)
				zDelta = 120;
			*/
            GetWindowRect(hDebugPanel,&rect) ;

            if (( (xPos > rect.left) && (xPos < rect.right) &&
                 (yPos > rect.top) && (yPos < rect.bottom)    )         // mouse is over graph...
				 /*||
				 (message == WM_LBUTTONDOWN) || (message == WM_RBUTTONDOWN)*/)
            {
                if (fwKeys & MK_CONTROL)                                    // control key held so zoom/unzoom Y
                {
                    if (zDelta > 0)
                    {
                        plotMaxY = floor(plotMaxY / 2.0) ;
                        if (plotMaxY < 64.0) plotMaxY = 64.0 ;
                        plotMinY = (-plotMaxY) ;
                    }
                    else
                    {
                        plotMaxY = plotMaxY * 2.0 ;
                        if (plotMaxY >= MAXPLOT_Y)
                        {
                            plotMaxY = (double)(MAXPLOT_Y) ;
                            plotMinY = (double)(MINPLOT_Y) ;
                        }
                        else
                        {
                            plotMinY = (-plotMaxY) ;
                        }
                    }
                }
                else                                                        // zoom/unzoom X as normal
                {

                     xPos -= rect.left ;                                     // convert to panel Coordinate

                     // printf("\nxPos(%i)", xPos) ;

                    if (zDelta > 0)
                    {
                        xeqv = (((double)xPos) / xscale) + plotMinX ;

                        // printf(" xeqv(%f) xscale(%f)", xeqv, xscale) ;

                        plotMinX = floor(plotMinX + 0.25 * (xeqv - plotMinX)) ;
                        plotMaxX =  ceil(plotMaxX - 0.25 * (plotMaxX - xeqv)) ;

                        // printf(" plotMinX(%f) plotMaxX(%f)", plotMinX, plotMaxX) ;

                    }
                    else
                    {
                        xeqv = (plotMaxX - plotMinX) * 0.125 ;
                        plotMinX = floor(plotMinX - xeqv) ;
                        if (plotMinX < 0) plotMinX = 0 ;
                        plotMaxX = ceil(plotMaxX + xeqv) ;
                        if (plotMaxX > plotLimX) plotMaxX = plotLimX ;
                    }
                }

				if(accumulatorMagnitudesOnly) plotMinY = -500;
            }


            if (change != ((plotMaxY - plotMinY) * (plotMaxX - plotMinX)))    // something changed
            {
                setaccplotscale(hDebugPanel) ;
                flagaccumulatorpanelepaintneeded() ;
            }
        }

        break ;


    case WM_PAINT:

        hdc = BeginPaint(hWnd, &ps);

        {
            int i ;
            statusReport_t * s ;

             // TODO: Add any drawing code here...
            if (timerID == 0) timerID = SetTimer(hWnd,1,1,NULL) ;

            s = getstatusreport() ;
            for (i = 0 ; i < NUM_BASIC_STAT_LINES ; i++)
            {
                // SelectObject(hdc,hFont);  // Select the thinner font
                TextOutA( hdc, 4 /* X */, 4 + i*26/* Y */, (LPCSTR) &(s->scr[i][0]), STAT_LINE_LENGTH);
            }
            i = 105 ;
            MoveToEx(hdc,4,i,NULL) ;
            LineTo(hdc,MAIN_PANEL_MIN_WIDTH,i) ;

            if (!displayDebugPanel) // if debug panel is not active we have extra status info to display
            {
                for (i = NUM_BASIC_STAT_LINES ; i < NUM_STAT_LINES ; i++)
                {
                    // SelectObject(hdc,hFont);  // Select the thinner font
                    TextOutA( hdc, 4 /* X */, 4 + i*28/* Y */, (LPCSTR) &(s->scr[i][0]), STAT_LINE_LENGTH);
                }

                // Block to display listener log information
                {
                    // Want to figure out size of display area available.
                    int start_y =  4 + (NUM_STAT_LINES * 28) ;
                    int last_y = movingPanelY ;
                    int num_lines = (last_y - start_y + 11) / 20 ; 
                    if (num_lines > NUM_MSGBUFF_LINES) num_lines = NUM_MSGBUFF_LINES ;
                    int index = (s->lmsg_last_index - num_lines + 1) & NUM_MSGBUFF_MODULO_MASK ;
                    for (i = 0 ; i < num_lines ; i++)
                    {
					    TextOutA( hdc, 
                                  4 /* X */, 
                                  4 + (i*20) + (NUM_STAT_LINES * 28)/* Y */, 
                                  (LPCSTR) &(s->lmsg_scrbuf[index][0]), STAT_LINE_LONG_LENGTH);
                        index = (index + 1) & NUM_MSGBUFF_MODULO_MASK ;
                    }
                }
            }
            else
            {
                // lets try to put start of payload line at end of long term average line
                TextOutA( hdc, 4 + MAIN_PANEL_MIN_WIDTH /* X */, 4 + 3*26/* Y */, (LPCSTR) &(s->scr[NUM_BASIC_STAT_LINES][0]), STAT_LINE_LENGTH);
            }
        }

        EndPaint(hWnd, &ps);

		accumulatorinstancehasnewdata = instancenewplotdata();

        if (accumulatorView && (accumulatorinstancehasnewdata || accumulatorplotredraw))
        {
			if(accumulatorinstancehasnewdata)
			{
				const char *cp = instancegetaccumulatordata(&ap[ap_index]) ;     // instance fills in size and points to its accumulator data.
			
				memcpy(&(ap[ap_index]).cp, cp, 7);
			}

			if (ap[ap_index].accumSize != 0)
			{
				plotaccumulator(hDebugPanel, &ap[ap_index]) ;
				
				if(accumulatorinstancehasnewdata) ap_index++;
				if(ap_index >= AP_HIST_SIZE) ap_index = 0;
			}

			accumulatorplotredraw = 0 ;

        }

		/*instancehasnewdata = instancenewrangedata();

		if (rangesView && instancehasnewdata)
		{
			rp.magvals[rp_index++] = (int) (instancenewrange()*100);
			
			if(rp_index >= MAX_RANGE_ELEMENTS)
			{
				rp_index = 0;

				rp.arraySize = MAX_RANGE_ELEMENTS;
			}
			else
			{
				if(rp.arraySize < MAX_RANGE_ELEMENTS) rp.arraySize = rp_index;
			}

			plotranges(hDebugPanel,&rp);
		}*/
        break;

    case WM_DESTROY:
        instance_close() ;
		usb2spirev = 0;
        PostQuitMessage(0);
        break;

/*
    case WM_LBUTTONDOWN:
        {
            int x, y ;
            char str[256] ;
            x = LOWORD(lParam);
            y = HIWORD(lParam);
            sprintf_s(str,256,"Co-ordinates are\nX=%i and Y=%i",x,y);
            MessageBoxA(hWnd, str, "Left Button Clicked", MB_OK);
        }
        break;
*/

    case WM_TIMER:
        {
            timerID = SetTimer(hWnd,timerID,1,NULL) ;
        }
        break ;

    default:
            return DefWindowProc(hWnd, message, wParam, lParam);

    } // end switch (message)
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;

    case WM_MOUSEMOVE:
        {
            int xPos = LOWORD(lParam);
            int yPos = HIWORD(lParam);

            //printf("X=%i Y=%i\n",xPos, yPos) ;

            if ((xPos > 147) && (xPos < 252) &&
                (yPos > 19) && (yPos < 44) )
            {
                if (hVer == 0)
                {
                    WCHAR wTempBuff[256] ;
                    size_t retVal ;
                    char * ds = "Build: " __DATE__  ", " __TIME__ ;
                    mbstowcs_s(&retVal,wTempBuff,sizeof(wTempBuff)/sizeof(WCHAR),ds,254) ;
                    hVer = CreateWindowEx(0,_T("Static"),NULL, WS_VISIBLE | WS_CHILD | WS_DLGFRAME, 12, 100, 154, 22, hDlg,NULL,NULL,NULL);
                    SendMessage(hVer, WM_SETFONT, (WPARAM) hFont, 1) ;
                    SetWindowText(hVer,wTempBuff) ;
                }
            }
            else
            {
                if (hVer != 0)
                {
                    DestroyWindow(hVer) ;
                    hVer = 0 ;
                }

            }

            if ((xPos > 23) && (xPos < 38) &&
                (yPos > 34) && (yPos < 50) )
            {
                if (hTip == 0)
                {
                    hTip = CreateWindowEx(0,_T("Static"),NULL, WS_VISIBLE | WS_CHILD | WS_DLGFRAME, 14, 88, 152, 50, hDlg,NULL,NULL,NULL);
                    SendMessage(hTip, WM_SETFONT, (WPARAM) hFont, 1) ;
                    SetWindowText(hTip,_T("\x48\x65\x6cl\x6f \x66\x72\x6f\x6d \x42\x69l\x6c\x79 \x61\x6e\x64 \x5ao\x72\x61\x6e\x2e.\x2e  \x57\x65\'\x72\x65 \x72") _T("e\x73\x70on\x73ib\x6c") _T("e f\x6fr \x74hi\x73 a\x70pl\x69") _T("ca\x74io\x6e.")) ;
                }
            }
            else
            {
                if (hTip != 0)
                {
                    DestroyWindow(hTip) ;
                    hTip = 0 ;
                }

            }
        }
        break ;

    }
    return (INT_PTR)FALSE;
}

void setroledropdownlist(HWND hDlg, UINT8 roleIndex)
{
    HWND dropdownlist = GetDlgItem(hDlg,ROLE_ID);
    int listLength =  SendMessage(dropdownlist,CB_GETCOUNT,0,0 );

    for (int i = 0 ; i < listLength ; i++ )
    {
		if (i==roleIndex)
		{	
			SendMessage(dropdownlist,CB_SETCURSEL,i,0) ;  // set the selection
			return ;
		}
    }
    printf("\nSTRANGE - no match to list?\n") ;
    return ; // not found make no change - this probably will not happen !
}

int getroledropdownlist(HWND hDlg)
{
    HWND dropdownlist = GetDlgItem(hDlg,ROLE_ID);
    int sel =  SendMessage(dropdownlist,CB_GETCURSEL,0,0 );
    
    setroledropdownlist(hDlg, sel) ;

    return sel ;
}

//
// Function to update text on main panel to display number of tags/blinks seen
//
#define BLINK_SEEN_ADVISE_STRING_LEN  126
void updateblinksseentext(void)
{
    WCHAR wTempString[BLINK_SEEN_ADVISE_STRING_LEN+2];

    uint8 ntags = instgettaglistlength() ;
    uint32 nblinks = instgetblinkrxcount() ;
    char *tags = (ntags == 1 ? "" : "s") ;
    char *blinks = (nblinks == 1 ? "" : "s") ;

    if (LISTENER == instance_mode)
    {
        swprintf(wTempString,BLINK_SEEN_ADVISE_STRING_LEN+1,
             _T("RX total %u blink%s\n      from %u tag%s"),nblinks,blinks,ntags,tags) ;
    }
    else
    {
        swprintf(wTempString,BLINK_SEEN_ADVISE_STRING_LEN+1,_T(" ")) ; // no text 
    }

    SetWindowText(hBlinkAdvise,wTempString) ; // and write it

} // end updateblinksseentext() ;




// ==============================================================================
// set channel dropdown selection to match supplied channel number
void setdropdownchannel(HWND dropdownlist,UINT8 ch)
{
    int listLength =  SendMessage(dropdownlist,CB_GETCOUNT,0,0 );
    TCHAR str[32];                                          // temp string buffer for dropdown list entry

    for (int i = 0 ; i < listLength ; i++ )
    {
        SendMessage(dropdownlist,CB_GETLBTEXT,i, (LPARAM) str );
        if (_ttoi(str) == ch)
        {
            SendMessage(dropdownlist,CB_SETCURSEL,i,0) ;  // set the
            return ;
        }
    }
    printf("\nSTRANGE - no match to channel ?\n") ;
    return ; // not found make no change - this probably will not happen !
}

// channel number indicated by current dropdown selection
INT8 getdropdownchannel(HWND hDlg)
{
    TCHAR str[32];                                          // temp string buffer for dropdown list entry
    HWND dropdownlist = GetDlgItem(hDlg,IDC_CHANSELECT);
    int sel =  SendMessage(dropdownlist,CB_GETCURSEL,0,0 );
    if (sel == CB_ERR)
    {
        setdropdownchannel(dropdownlist,1) ;
        return 1 ;
    }
    SendMessage(dropdownlist,CB_GETLBTEXT,sel, (LPARAM) str );
    return _ttoi(str) ;
}


// ==============================================================================
// set dropdown attenuation selection to match supplied attenuaton
void setdropdownattenuation(HWND dropdownlist,UINT8 attn)
{
    // quick solution attn parameter is direct index
    SendMessage(dropdownlist,CB_SETCURSEL,attn,0) ;  // select the list item
}

// get attenuation indicated by current dropdown selection
INT8 getdropdownattenuation(HWND hDlg)
{
    TCHAR str[32];                                          // temp string buffer for dropdown list entry
    int i ;

    HWND dropdownlist = GetDlgItem(hDlg,IDC_TXATTSELECT);
    int sel =  SendMessage(dropdownlist,CB_GETCURSEL,0,0 );
    if (sel == CB_ERR)
    {
        setdropdownchannel(dropdownlist,1) ;
        return 1 ;
    }
    SendMessage(dropdownlist,CB_GETLBTEXT,sel, (LPARAM) str );

    // first elements of string is "xx)" decimal index beginning at 01

    i = (str[0] - '0') * 10 ;
    i += str[1] - '0' - 1 ;

    return (i) ;
}

#define NUM_NS_IDS 5
const short   nsID[NUM_NS_IDS] = { /*ID_CONF_PREAM32,*/ ID_CONF_PREAM64,  ID_CONF_PREAM128, ID_CONF_PREAM256,   ID_CONF_PREAM512, ID_CONF_PREAM2048 } ;
const short snapID[NUM_NS_IDS] = { ID_CONF_PREAM64, ID_CONF_PREAM1024, ID_CONF_PREAM1024, ID_CONF_PREAM1024, ID_CONF_PREAM4096 } ;
const short plength[NUM_NS_IDS]= {              64,              1024,              1024,              1024,              4096 } ;
const uint8 plength_dwt[NUM_NS_IDS]= {     DWT_PLEN_64,     DWT_PLEN_1024,     DWT_PLEN_1024,     DWT_PLEN_1024,	 DWT_PLEN_4096 } ;

// enable/disable i.e. grey out or otherwise remove ability to select non-standard preamble lengths, and snap to next highest length if ns one was ser
void allownspreambles(HWND hDlg,int allow)
{
    HWND hwnd ;
    int i ;

    if (allow)
    {
        for (i = 0 ; i < NUM_NS_IDS ; i++)
        {
            hwnd = GetDlgItem(hDlg,nsID[i]);                        // get handle to button
            EnableWindow(hwnd,1) ;                                  // enable (black it in)
        }
    }
    else
    {
        for (i = 0 ; i < NUM_NS_IDS ; i++)
        {
            hwnd = GetDlgItem(hDlg,nsID[i]);                        // get handle to button
            if (SendMessage(hwnd,BM_GETCHECK,0,0) == BST_CHECKED)   // if the button is checked
            {
                SendMessage(hwnd,BM_SETCHECK,BST_UNCHECKED,0) ;     // uncheck it
                SendMessage(GetDlgItem(hDlg,snapID[i]),BM_SETCHECK,BST_CHECKED,0) ;  // Snap to next highest standard (using table)
                //tempConfig.preambleLength = plength[i] ;            // and make sure temp config also agrees (via table)
				tempConfig.preambleLength = plength_dwt[i] ;
            }
            EnableWindow(hwnd,0) ;                                  // disable (grey it out)
        }
    }
};


void setradioprf(HWND hDlg,int pv)
{
    HWND prfbutton ;

    switch (pv)
    {
    case DWT_PRF_64M:
        prfbutton = GetDlgItem(hDlg,ID_CONF_PRF64MHZ);
        SendMessage(prfbutton,BM_SETCHECK,BST_CHECKED,0) ;
        break ;
    case DWT_PRF_16M:
    default:
        prfbutton = GetDlgItem(hDlg,ID_CONF_PRF16MHZ);
        SendMessage(prfbutton,BM_SETCHECK,BST_CHECKED,0) ;
        break ;
    }
}

int readradioprf(HWND hDlg)
{
    HWND prfbutton ;

    prfbutton = GetDlgItem(hDlg,ID_CONF_PRF64MHZ);
    if (BST_CHECKED == SendMessage(prfbutton,BM_GETCHECK,0,0)) return DWT_PRF_64M ;

    return DWT_PRF_16M ;
}


void setradiodatarate(HWND hDlg,int rate,int onOrOff)
{
    HWND button ;
    int id ;
    int reqState ;

    if (onOrOff) reqState = BST_CHECKED ;
    else reqState = BST_UNCHECKED ;

    switch (rate)
    {
    case DWT_BR_6M8:  id = ID_CONF_RATE6M81;  break ;            // 6.81 Mbits/s
    case DWT_BR_850K:  id = ID_CONF_RATE850K;  break ;            // 850 kbits/s
    case DWT_BR_110K:                                             // 110 kbits/s
    default: id = ID_CONF_RATE110K;  break ;
    }
    button = GetDlgItem(hDlg,id);
    SendMessage(button,BM_SETCHECK,reqState,0) ;
}

void setradiopreamblelength(HWND hDlg,int plen,int onOrOff)
{
    HWND button ;
    int id ;
    int reqState ;

    if (onOrOff) reqState = BST_CHECKED ;
    else reqState = BST_UNCHECKED ;

    switch (plen)
    {
    case DWT_PLEN_4096:  id = ID_CONF_PREAM4096;  break ;
    case DWT_PLEN_2048:  id = ID_CONF_PREAM2048;  break ;
    case DWT_PLEN_1536:  id = ID_CONF_PREAM1536;  break ;
    case DWT_PLEN_1024:  id = ID_CONF_PREAM1024;  break ;
    case DWT_PLEN_512:   id = ID_CONF_PREAM512;  break ;
    case DWT_PLEN_256:   id = ID_CONF_PREAM256;  break ;
    case DWT_PLEN_128:   id = ID_CONF_PREAM128;  break ;
    //case 32:    id = ID_CONF_PREAM32;  break ;
    //case 16:    id = ID_CONF_PREAM16;  break ;
    case DWT_PLEN_64:
    default:    id = ID_CONF_PREAM64;  break ;
    }
    button = GetDlgItem(hDlg,id);
    SendMessage(button,BM_SETCHECK,reqState,0) ;
}

#if (PAC_SELECTION == 1)
void setradiopreacqcorsize(HWND hDlg,int pacsz,int onOrOff)
{
    HWND button ;
    int id, i ;
    int reqState ;

	//uncheck all
	for(i=0; i<4; i++)
	{
		SendMessage(GetDlgItem(hDlg,ID_CONF_PAC_8_SYM+i),BM_SETCHECK,BST_UNCHECKED,0) ;
	}

	//then check the requested one
    if (onOrOff) reqState = BST_CHECKED ;
    else reqState = BST_UNCHECKED ;

    switch (pacsz)
    {
    case DWT_PAC64:  id = ID_CONF_PAC_64_SYM;  break ;
    case DWT_PAC32:  id = ID_CONF_PAC_32_SYM;  break ;
    case DWT_PAC16:  id = ID_CONF_PAC_16_SYM;  break ;
    case DWT_PAC8 :
    default:              id = ID_CONF_PAC_8_SYM;  break ;
    }
    button = GetDlgItem(hDlg,id);
    SendMessage(button,BM_SETCHECK,reqState,0) ;
}
#endif

 void settickbox(HWND hDlg,int id ,int on)
 {
    HWND button = GetDlgItem(hDlg,id);
    SendMessage(button,BM_SETCHECK,(on ? BST_CHECKED : BST_UNCHECKED),0) ;
 }

 UINT8 gettickbox(HWND hDlg,int id)
 {
    HWND button = GetDlgItem(hDlg,id);
    return (UINT8)(SendMessage(button,BM_GETCHECK,0,0)) ;
 }

void composecodeselectdropdown(HWND hDlg,chConfig_t *config)
{
    HWND hCtrl ;
    int listLength ;
    TCHAR str[64];                                          // temp string buffer for dropdown list entry

    hCtrl = GetDlgItem(hDlg,IDC_PCODSELECT);

    // First empty current list

    do {
        listLength = SendMessage(hCtrl,CB_DELETESTRING,0,0);
    } while (listLength > CB_OKAY) ;

    // Now now re-populate it

    if (config->nsPreambleCode)
    {
        SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T("Non-Standard") );
        config->preambleCode = 0 ;             // 0 signifies this choice
        SendMessage(hCtrl,CB_SETCURSEL,0,0) ;
        return ;
    }

    if (config->prf == DWT_PRF_64M)           // see IEEE Std 802.15.4a-2007, Table 39e
    {
        // use 127 bit codes
        switch(config->channel)
        {
        case 1:
        case 2:
        case 3:
        case 5:
        case 6:
        case 8:
        case 9:
        case 10:
            // Codes allowed are 9, 10, 11, 12
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T(" 9") );
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T("10") );
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T("11") );
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T("12") );
            break;
        case 4:
        case 7:
        case 11:
            // codes allowed are 17, 18, 19 and 20
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T("17") );
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T("18") );
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T("19") );
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T("20") );
            // Also for compatabibility with narrow band implementations can select 1,2,3,4,5, and 6.
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T(" 9 - for interworking") );
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T("10 - for interworking") );
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T("11 - for interworking") );
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T("12 - for interworking") );
            break ;
        default:
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T("UNKNOWN CHANNEL ???") );
            SendMessage(hCtrl,CB_SETCURSEL,0,0) ;
            return ;
        } // end switch
        if (config->nsDPScodes)
        {
            // codes allowed are 13 to 16 and 21 to 24 available for DPS.
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T("13 - for DPS") );
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T("14 - for DPS") );
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T("15 - for DPS") );
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T("16 - for DPS") );
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T("21 - for DPS") );
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T("22 - for DPS") );
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T("23 - for DPS") );
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T("24 - for DPS") );
        }
    }
    else   // see IEEE Std 802.15.4a-2007, Table 39d
    {
        // use 31 bit codes
        switch(config->channel)
        {
        case 1:
        case 8:
            // Codes allowed are 1,2
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T(" 1 ") );
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T(" 2 ") );
            break;
        case 2:
        case 5:
        case 9:
            // codes allowed are 3,4
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T(" 3 ") );
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T(" 4 ") );
            break ;
        case 3:
        case 6:
        case 10:
            // codes allowed are 5,6
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T(" 5 ") );
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T(" 6 ") );
            break ;
        case 4:
        case 7:
        case 11:
            // codes allowed are 7,8
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T(" 7 ") );
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T(" 8 ") );
            // Also for compatabibility with narrow band implementations can select 1,2,3,4,5, and 6.
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T(" 1 - for ch 1,8 interworking") );
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T(" 2 - for ch 1,8 interworking") );
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T(" 3 - for ch 2,5,9 interworking") );
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T(" 4 - for ch 2,5,9 interworking") );
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T(" 5 - for ch 4,7,11 interworking") );
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T(" 6 - for ch 4,7,11 interworking") );
            break ;
        default:
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T("UNKNOWN CHANNEL ???") );
            config->preambleCode = -1 ;             // not a valid code.
            SendMessage(hCtrl,CB_SETCURSEL,0,0) ;
            return ;
        } // end switch
    } // end else 31 bit codes

    // now see if were have a selection we can keep

    listLength =  SendMessage(hCtrl,CB_GETCOUNT,0,0 );

    for (int i = 0 ; i < listLength ; i++ )
    {
        SendMessage(hCtrl,CB_GETLBTEXT,i, (LPARAM) str );
        if (_ttoi(str) == config->preambleCode)
        {
            SendMessage(hCtrl,CB_SETCURSEL,i,0) ;               // match to configured code, select that entry
            return ;
        }
    }

    // no match if we get here

    SendMessage(hCtrl,CB_SETCURSEL,0,0) ;                           // just select the first one
    SendMessage(hCtrl,CB_GETLBTEXT,0,(LPARAM)str );                 // get the string
    config->preambleCode = _ttoi(str) ;                             // set preamble code value

} // end composecodeselectdropdown()

// get preamble code indicated by current dropdown selection
INT8 getdropdownpreamblecode(HWND hDlg)
{
    TCHAR str[64];                                          // temp string buffer for dropdown list entry
    HWND hCtrl = GetDlgItem(hDlg,IDC_PCODSELECT);
    int sel =  SendMessage(hCtrl,CB_GETCURSEL,0,0 );
    if (sel == CB_ERR)
    {
        return -1 ;   // something is wrong !
    }
    SendMessage(hCtrl,CB_GETLBTEXT,sel, (LPARAM) str );
    return _ttoi(str) ;
}

INT_PTR CALLBACK TagSelectAddress(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent ;

    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        {   
			WCHAR tempString[18];
			HWND hCtrl = GetDlgItem(hDlg,IDC_TAGSELECT);
			int i, len;
			uint8* taglist ;
			len= instgettaglistlen();
			
			SendMessage(hCtrl,CB_RESETCONTENT, 0, 0);
			for(i=0; i < len; i++)
			{
				uint32 low32=0, hi32=0;
				taglist = instgettaglist(i);
				memcpy(&low32, &taglist[0], 4);
				memcpy(&hi32, &taglist[4], 4);
				swprintf(tempString,17,_T("%08X%08X"),hi32,low32) ;  
				SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) tempString);
			}
			SendMessage(hCtrl,CB_SETCURSEL, 0, 0) ;
        }
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        // Parse the menu selections:

        // printf("\n ID %i, EV %i",wmId,wmEvent) ;

        switch (wmId)
        {
        case IDOK :
            {
                int len;
			    HWND dropdownlist = GetDlgItem(hDlg,IDC_TAGSELECT);
			    int sel =  SendMessage(dropdownlist,CB_GETCURSEL,0,0 ); // sel is index in drop down list
			    uint8* taglist ;
				len= instgettaglistlen();
				uint32 low32=0, hi32=0;
				instsettagtorangewith(sel); // tell instance (index) of tags selected 
				taglist = instgettaglist(sel);
				memcpy(&low32, &taglist[0], 4);
				memcpy(&hi32, &taglist[4], 4);
			    swprintf(rangingWithString,RANGING_WITH_STR_LEN,_T("ranging with %08X%08X"),hi32,low32) ;
                EndDialog(hDlg, LOWORD(wParam));
            }
            return (INT_PTR)TRUE;

        case IDCANCEL :
            {
			instsettagtorangewith(TAG_LIST_SIZE);                   // tell instance no tag selected
			swprintf(rangingWithString,RANGING_WITH_STR_LEN,_T("ranging has NO SELECTED TAG !!")) ;
            EndDialog(hDlg, LOWORD(wParam));
            }
            return (INT_PTR)TRUE;

        } // end switch
    }
    return (INT_PTR)FALSE;
}


// ---------------------------------------------------------------
// constrain (grey out) the 6.8Mbps Data Rate depending on PRF
//
void constraindatarate(HWND hDlg, chConfig_t *config)
{
    HWND button6M8 = GetDlgItem(hDlg,ID_CONF_RATE6M81) ;

    if (config->prf == DWT_PRF_64M)
    {
        EnableWindow(button6M8,1) ;                                 // Blacken in the 6.8 Mbp/s button
    }
    else
    {
        if (config->datarate == DWT_BR_6M8)                    // if current data rate is 6.8 Mbps
        {
            setradiodatarate(hDlg,config->datarate,0) ;             // Turn off the 6M8 button
            config->datarate = DWT_BR_850K ;                    // then snap data rate to 850 kbps
            setradiodatarate(hDlg,config->datarate,1) ;             // and turn on the new button
        }
        EnableWindow(button6M8,0) ;                                 // Gray out the 6.8 Mbp/s button
    }

} // end constraindatarate()


// ==============================================================================


// Message handler for Continuous Wave Mode Configuration Box.
INT_PTR CALLBACK CWModeConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent ;

    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        {

		}
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        // Parse the menu selections:

        // printf("\n ID %i, EV %i",wmId,wmEvent) ;

        switch (wmId)
        {
        case IDOK :
			{
				//in Listener mode the device will be in RX - so turn it off
				dwt_forcetrxoff();

				//measure the frequency
				//Spectrum Analyser set:
				//FREQ to be channel default e.g. 3.9936 GHz for channel 2
				//SPAN to 10MHz
				//PEAK SEARCH
				instance_startcwmode(chConfig.channel);
			}
            break;
        case IDCANCEL :
			restartinstance() ;            //stop the tx frame mode and restart !!!
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        } // end switch
    }
    return (INT_PTR)FALSE;
}

// Message handler for Continuous Frame Transmission Configuration Box.
INT_PTR CALLBACK ContFrameConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent ;

    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        {

		}
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        // Parse the menu selections:

        // printf("\n ID %i, EV %i",wmId,wmEvent) ;

        switch (wmId)
        {
		case IDHELP :
				DialogBox(hInst, MAKEINTRESOURCE(IDD_REGACCESSDIALOG), hWnd, RegAccess);
			break;
        case IDOK :
			{
				//in Listener mode the device will be in RX - so turn it off
				dwt_forcetrxoff();
				// the value here 0x1000 gives a period of 32.82 µs
				//this is setting 0x1000 as frame period (125MHz clock cycles) (time from Tx en - to next - Tx en)
				instance_starttxtest(0x1000);
			}
            break;
        case IDCANCEL :
			restartinstance() ;            //stop the tx frame mode and restart !!!
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        } // end switch
    }
    return (INT_PTR)FALSE;
}

// ==============================================================================

// Message handler for Channel Configuration Box.
INT_PTR CALLBACK ChanConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent ;
    HWND hCtrl ;

    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        {

            tempConfig = chConfig ;     // copy config to temp for mods pending "OK" or "Cancel" operations

            /* Modes supported by the Prototype: Channels 1, 4, 5 and 7.
                                                 All preamble codes giving complex channels per standard.
                                                 Data Rates: 110 Kbits/s, 850 Kbits/s and 6.8 Mbits/s    */

            hCtrl = GetDlgItem(hDlg,IDC_CHANSELECT);

//            EnableWindow(hCtrl,0) ; //don't allow channel selection for Demo SW

            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T(" 1 - Centre 3.5 GHz (500 MHz)") );  // (LPARAM) (LPCTSTR)
            
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T(" 2 - Centre 4.0 GHz (500 MHz)") );

            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T(" 3 - Centre 4.5 GHz (500 MHz)") );

            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T(" 4 - Centre 4.0 GHz (900 MHz)") );
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T(" 5 - Centre 6.5 GHz (500 MHz)") );
            // SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T(" 6 - Centre 7.0 GHz (500 MHz)") );
            SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T(" 7 - Centre 6.5 GHz (900 MHz)") );
            // SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T(" 8 - Centre 7.5 GHz (500 MHz)") );
            // SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T(" 9 - Centre 8.0 GHz (500 MHz)") );
            // SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T("10 - Centre 8.5 GHz (500 MHz)") );
            // SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM) _T("11 - Centre 8.0 GHz (1.3 GHz)") );

            setdropdownchannel(hCtrl,tempConfig.channel) ;

            setradioprf(hDlg,tempConfig.prf) ;
            setradiodatarate(hDlg,tempConfig.datarate,1) ;
            setradiopreamblelength(hDlg,tempConfig.preambleLength,1) ;
#if (PAC_SELECTION == 1)
			setradiopreacqcorsize(hDlg,tempConfig.pacSize,1) ;
#endif
            allownspreambles(hDlg,tempConfig.nsPreambleLength) ;

            composecodeselectdropdown(hDlg,&tempConfig) ;       // compose and select the appriopriate preamble code
            
            settickbox(hDlg,ID_CONF_NSTDSFD,tempConfig.nsSFD) ;
            //constraindatarate(hDlg, &tempConfig) ;

            /*
             Notes:  110 datarate implies long SFD, others imply short SFD
            */
#if (SNIFF_SELECTION == 1)
			setdialogboxdecuint(hDlg, ID_SNIFF_ON, tempConfig.sniffOn, 5) ;
			setdialogboxdecuint(hDlg, ID_SNIFF_OFF, tempConfig.sniffOff, 5) ;
#endif
        }
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        // Parse the menu selections:

        // printf("\n ID %i, EV %i",wmId,wmEvent) ;

        switch (wmId)
        {
        case IDOK :
            chConfig = tempConfig ;        // copy temp config onto actual configuration
            restartinstance() ;            // Apply configuration and restart !!!
            // fall into end dialog function
        case IDCANCEL :
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;

        case IDC_CHANSELECT :      // channel selection might have changed
            if (wmEvent == CBN_SELCHANGE)
            {
                tempConfig.channel = getdropdownchannel(hDlg) ;
                composecodeselectdropdown(hDlg,&tempConfig) ;       // compose and select the appriopriate preamble code
            }
            // channel selection determines what PRF's and preamble codes are available
            break;
           
        case ID_CONF_PRF16MHZ :
        case ID_CONF_PRF64MHZ :                                                 // if we get activity on the PRF radio buttons....
            {
                int val ;
                if (tempConfig.prf != (val = readradioprf(hDlg)))               // see if the selection has actually changed
                {
                    tempConfig.prf = val ;                                      // update to new value
                    composecodeselectdropdown(hDlg,&tempConfig) ;               // compose and select the appriopriate preamble code
                   // constraindatarate(hDlg, &tempConfig) ;
                }
            }
            break ;

        case ID_CONF_RATE110K :
            tempConfig.datarate = DWT_BR_110K;                              // here this means 110 Kbits/s

            // Snap PLEN to 2048 or 4096 if non-standard preambles are not allowed

            setradiopreamblelength(hDlg,tempConfig.preambleLength,0) ;          // clear current PLEN radio button
            if (tempConfig.nsPreambleLength) 
			{
				tempConfig.preambleLength = DWT_PLEN_1024; //set to be the default value
				tempConfig.pacSize = DWT_PAC32 ;
			}
            else 
			{
				tempConfig.preambleLength = DWT_PLEN_1024; //set to be the default value
				tempConfig.pacSize = DWT_PAC32 ;
			}
            setradiopreamblelength(hDlg,tempConfig.preambleLength,1) ;          // draw new PLEN radio button
#if (PAC_SELECTION == 1)
			setradiopreacqcorsize(hDlg,tempConfig.pacSize,1) ;
#endif
            break ;

        case ID_CONF_RATE850K :
            tempConfig.datarate = DWT_BR_850K;                  // here this means 850 Kbits/s,
            break ;

        case ID_CONF_RATE6M81 :
            tempConfig.datarate = DWT_BR_6M8;                  // here this means 6.81 Mbits/s,

			//by default the standard SFD is used
			tempConfig.nsSFD = 0;

            settickbox(hDlg,ID_CONF_NSTDSFD,tempConfig.nsSFD) ;

            break ;

        /*case ID_CONF_PREAM16 :
            tempConfig.preambleLength = DWT_PLEN_16; //16 ;
            break ;

        case ID_CONF_PREAM32 :
            tempConfig.preambleLength = DWT_PLEN_32; //32 ;
            break ;
			*/
        case ID_CONF_PREAM64 :
            tempConfig.preambleLength = DWT_PLEN_64; //64 ;
			tempConfig.pacSize = DWT_PAC8 ;
            break ;

        case ID_CONF_PREAM128 :
            tempConfig.preambleLength = DWT_PLEN_128; //128 ;
			tempConfig.pacSize = DWT_PAC8 ;
            break ;

        case ID_CONF_PREAM256 :
            tempConfig.preambleLength = DWT_PLEN_256; //256 ;
			tempConfig.pacSize = DWT_PAC16 ;
            break ;

        case ID_CONF_PREAM512 :
            tempConfig.preambleLength = DWT_PLEN_512; //512 ;
			tempConfig.pacSize = DWT_PAC16 ;
            break ;

        case ID_CONF_PREAM1024 :
            tempConfig.preambleLength = DWT_PLEN_1024; //1024 ;
			tempConfig.pacSize = DWT_PAC32 ;
            break ;

        case ID_CONF_PREAM1536 :
            tempConfig.preambleLength = DWT_PLEN_1536; //1536 ;
			tempConfig.pacSize = DWT_PAC64 ;
            break ;

        case ID_CONF_PREAM2048 :
            tempConfig.preambleLength = DWT_PLEN_2048; //2048 ;
			tempConfig.pacSize = DWT_PAC64 ;
            break ;

        case ID_CONF_PREAM4096 :
            tempConfig.preambleLength = DWT_PLEN_4096; //4096 ;
			tempConfig.pacSize = DWT_PAC64 ;
            break ;
#if (PAC_SELECTION == 1)
        case ID_CONF_PAC_8_SYM :
			tempConfig.pacSize = DWT_PAC8 ;
            break;

        case ID_CONF_PAC_16_SYM :
            tempConfig.pacSize = DWT_PAC16 ;
            break;

        case ID_CONF_PAC_32_SYM :
            tempConfig.pacSize = DWT_PAC32 ;
            break;

        case ID_CONF_PAC_64_SYM :
            tempConfig.pacSize = DWT_PAC64 ;
            break; 
#endif
        case IDC_PCODSELECT :
            if (wmEvent == CBN_SELCHANGE)
            {
                // printf("\n preamble code selection") ;
                tempConfig.preambleCode = getdropdownpreamblecode(hDlg) ;
                // printf(" is %i", tempConfig.preambleCode ) ;
            }
            break;

        case ID_CONF_NSTDSFD :
            tempConfig.nsSFD = gettickbox(hDlg,ID_CONF_NSTDSFD) ;
            break;

		//case ID_CONF_SNIFFON :
            //tempConfig.nsSFD = gettickbox(hDlg,ID_CONF_NSTDSFD) ;
            //break;
		case ID_SNIFF_ON:
		case ID_SNIFF_OFF:
			if (wmEvent == EN_KILLFOCUS)
			{
				int on = validateeditboxint(GetDlgItem(hDlg,ID_SNIFF_ON), 0, 15) ; //max is 15 PACs
                setdialogboxdecuint(hDlg, ID_SNIFF_ON, on, 5) ;
				
				int off = validateeditboxint(GetDlgItem(hDlg,ID_SNIFF_OFF), 0, 8000) ;
                setdialogboxdecuint(hDlg, ID_SNIFF_OFF, off, 5) ;
				
				tempConfig.sniffOn = on;
				tempConfig.sniffOff = off;

			}
			break;
        } // end switch
    }
    return (INT_PTR)FALSE;
}


// Message handler for low-level register access box.
INT_PTR CALLBACK RegAccess(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent, changed, failure ;
    UINT offset ;

    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:

        setdialogboxhex(hDlg, ID_REG_ADDRESS, ll_RegAddr, 2) ;  // display last accessed address if any, 2 characters
        setdialogboxhex(hDlg, ID_REG_OFFSET,offset=0,2) ;       // Always start with offset of zero 
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        // Parse the menu selections:

        SetWindowText(hDlg,_T("Low-Level Register Access")) ;

        switch (wmId)
        {
        case IDOK :
            {
                HWND hwndtmp = GetFocus();
                if(GetDlgItem(hDlg,IDOK) == hwndtmp)
                {
                    // OKAY Finished Key
                    EndDialog(hDlg, LOWORD(wParam));
                    return (INT_PTR)TRUE;
                }
                // Enter key was hit
                break ;         // ignore enter key
            }

        case IDCANCEL :
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;

        case ID_REGREAD :
            changed = validatedialogboxhex(hDlg, ID_REG_ADDRESS, 0, 0x3F,&ll_RegAddr) +
                      validatedialogboxhex(hDlg, ID_REG_OFFSET, 0, 0x7FFF,&offset) ;

            if (changed)
            {
                SetDlgItemText(hDlg,ID_REG_VALUE,_T(" ")) ;             // clear read register value
                break ;                                                 // wait for a 2nd click to do read
            }

            failure = instancelowlevelreadreg(ll_RegAddr,offset,&ll_RegValue) ;    // Read the register value
            if (failure)
            {
                SetDlgItemText(hDlg,ID_REG_VALUE,_T("RD_FAIL!")) ;              // indicate if it fails
            }
            else
            {
                setdialogboxhex(hDlg, ID_REG_VALUE, ll_RegValue, 8) ;           // display result, 8 characters
            }

            break;
        case ID_REGWRITE :

            changed = validatedialogboxhex(hDlg, ID_REG_ADDRESS, 0, 0x3F,&ll_RegAddr) +
                      validatedialogboxhex(hDlg, ID_REG_OFFSET, 0, 0x7FFF,&offset) +
                      validatedialogboxhex(hDlg, ID_REG_VALUE, 0, 0xFFFFFFFF,&ll_RegValue) ;

            

            if (changed)
            {
                SetWindowText(hDlg,_T("Confirm modified value - click WRITE again")) ;
                break ;                                        // wait for a 2nd click if changed
            }

            failure = instancelowlevelwritereg(ll_RegAddr,offset,ll_RegValue) ;      // Write the register value
            if (failure)
            {
                SetDlgItemText(hDlg,ID_REG_VALUE,_T("WR_FAIL!")) ;                // indicate if it fails
            }

            break;

        } // end switch
    }
    return (INT_PTR)FALSE;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// Convert single char to a hexidecimal digit or retunn -1 if not a hex char
int convhexdigit(int ch)
{
    ch = toupper(ch) ;
    if ((ch >= '0') && (ch <= '9')) return ch - '0' ;
    if ((ch >= 'A') && (ch <= 'F')) return ch - 'A' + 10 ;
    return -1 ;
} // end of convhexdigit()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// Read hex byte from file ignoring nonHex digits, return -1 on eof
static int readHexByte(FILE *f)
{
    int byte ;
    int nibble ;
    int filechar ;

    for (;;)                                                        // search forever for hex nibble
    {
        if (EOF == (filechar = fgetc(f))) return -1 ;               // return if end of file encountered
        if (-1 != (nibble = convhexdigit(filechar)))                // found 1st nibble
        {
            byte = nibble << 4 ;                                    // put into high part of byte
        
            // next char should be second nibble of hex byte

            if (EOF == (filechar = fgetc(f))) return -1 ;           // error next char is end of file !!! ???
            if (-1 == (nibble = convhexdigit(filechar))) return -1; // error next char isn't hex digit !! ???

            byte += nibble ;                                        // complete byte
            return byte ;                                           // finished
        }
    }
} // end of readHexByte()

// -------------------------------------------------------------------------------------------------------------------
//
// Message handler for configuration of payload address and user data
//
// -------------------------------------------------------------------------------------------------------------------
//
INT_PTR CALLBACK TimingConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent ;

    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        tempTimingConfig = timingConfig;                         // temp copy of configurations

		setdialogboxdecuint(hDlg, ID_TAG_BLINK_PERIOD, tempTimingConfig.blink_period_ms, 5) ;
		setdialogboxdecuint(hDlg, ID_TAG_POLL_PERIOD, tempTimingConfig.poll_period_ms, 5) ;
		setdialogboxdecuint(hDlg, ID_ANC_RESP_DLY, tempTimingConfig.anc_resp_dly_ms, 5) ;
        setdialogboxdecuint(hDlg, ID_TAG_RESP_DLY, tempTimingConfig.tag_resp_dly_ms, 5);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);

        switch (wmId)
        {
        case IDOK :
            {
				int tagDelay, tagBlinkDelay, anc_resp_dly, tag_resp_dly;

				tagDelay = validateeditboxint(GetDlgItem(hDlg, ID_TAG_POLL_PERIOD), 0, 60000) ;
				tagBlinkDelay = validateeditboxint(GetDlgItem(hDlg, ID_TAG_BLINK_PERIOD), 0, 60000) ;
                anc_resp_dly = validateeditboxint(GetDlgItem(hDlg, ID_ANC_RESP_DLY), 30, 1000) ;
                tag_resp_dly = validateeditboxint(GetDlgItem(hDlg, ID_TAG_RESP_DLY), 30, 1000);
                setdialogboxdecuint(hDlg, ID_ANC_RESP_DLY, anc_resp_dly, 5) ;
                setdialogboxdecuint(hDlg, ID_TAG_RESP_DLY, tag_resp_dly, 5);

                tempTimingConfig.blink_period_ms = tagBlinkDelay;
                tempTimingConfig.poll_period_ms = tagDelay;
                tempTimingConfig.anc_resp_dly_ms = anc_resp_dly;
                tempTimingConfig.tag_resp_dly_ms = tag_resp_dly;
                
                if(GetDlgItem(hDlg,IDOK) == GetFocus())                         // OKAY Finished Key
                {
                    timingConfig = tempTimingConfig;                         // copy temp configurations back to real ones

                    instance_set_user_timings(timingConfig.anc_resp_dly_ms, timingConfig.tag_resp_dly_ms,
                                              timingConfig.blink_period_ms, timingConfig.poll_period_ms);
                    instance_init_timings();

                    EndDialog(hDlg, LOWORD(wParam));
                    return (INT_PTR)TRUE;
                }
                // Enter key was hit
                break ;         // ignore enter key
            }

        case IDCANCEL :
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        } // end switch
    }
    return (INT_PTR)FALSE;

} // end TimingConfig()


// -------------------------------------------------------------------------------------------------------------------
//
// Function to validate (and correct) input state of a Hex number text entry box withing a dialog box
//
// get the hex string and convert to int, compare with min/max and snap, write back a valid hex string uppercase.
// Compare with original (case insensitive) and if not same update string and wait for another read button press
// Return value read, and indicatation whether the validation had to significantly change the user entered value
//
// Returns 0 if all went well or 1 if it had to correct the entry
//
#define MAX_X_LENGTH 8
#define MAX_X_LENGTH_64 16

int validatedialogboxhex(HWND hDlg, int editboxID, UINT min, UINT max, UINT *result)
{
    int n, d, i;
    UINT v ;
    int change = 0 ;
    WCHAR tempString[MAX_X_LENGTH+2];
    int width = (int)(log((double) max) / log(16.0)) + 1 ;                          // width of number string
    if (min>max) min=max ;                                               // keeping sane

    n = GetDlgItemText(hDlg,editboxID,tempString,MAX_X_LENGTH+1) ;  // +1 to allow for string termination

    v = 0 ;

    for (i = 0 ; i < n ; i++ )
    {
        if (isxdigit(d = tempString[i]))
        {
//          int t ;
          v = (v << 4) + ( (d <= '9') ? (d - _T('0') ) : (toupper(d) - _T('A') + 10)) ;
          /*  v = (v << 4) ;
            if (d <= _T('9'))
                d = d - _T('0') + 9 ;
            else
                d = toupper(d) - _T('A') ;
            v += d ; */
        }
        else change = 1 ;
    }

    if (v < min) { v = min ; change = 1 ; }
    else if (v > max) { v = max ; change = 1 ; }

    swprintf(tempString,MAX_X_LENGTH+1,_T("%0*X"),width,v) ;        // format the value with leading zeros if necessary
    SetDlgItemText(hDlg,editboxID,tempString) ;                     // and write back

    *result = v ;                                               // read value

    return change ;                                             // was it changed significantly from user input

} // end validatedialogboxhex()

int validatedialogboxhex64(HWND hDlg, int editboxID, UINT64 min, UINT64 max, UINT64 *result)
{
    int n, d, i;
    UINT64 v ;
    int change = 0 ;
    WCHAR tempString[MAX_X_LENGTH_64+2];
    int width = (int)(log((double) max) / log(16.0)) + 1 ;                          // width of number string
    if (min>max) min=max ;                                               // keeping sane

    n = GetDlgItemText(hDlg,editboxID,tempString,MAX_X_LENGTH_64+2) ;  // +1 to allow for string termination

    v = 0 ;

    for (i = 0 ; i < n ; i++ )
    {
        if (isxdigit(d = tempString[i]))
        {
//          int t ;
          v = (v << 4) + ( (d <= '9') ? (d - _T('0') ) : (toupper(d) - _T('A') + 10)) ;
          /*  v = (v << 4) ;
            if (d <= _T('9'))
                d = d - _T('0') + 9 ;
            else
                d = toupper(d) - _T('A') ;
            v += d ; */
        }
        else change = 1 ;
    }

    if (v < min) { v = min ; change = 1 ; }
    else if (v > max) { v = max ; change = 1 ; }

    swprintf(tempString,MAX_X_LENGTH_64+2,_T("%0*X"),width,v) ;        // format the value with leading zeros if necessary
    SetDlgItemText(hDlg,editboxID,tempString) ;                     // and write back

    *result = v ;                                               // read value

    return change ;                                             // was it changed significantly from user input

} // end validatedialogboxhex()

// -------------------------------------------------------------------------------------------------------------------
//
// Function to set a Hex number text value into a text entry box withing a dialog box
//
void setdialogboxhex(HWND hDlg, int editboxID, UINT value, UINT width)
{
    WCHAR tempString[MAX_X_LENGTH+2];

    swprintf(tempString,MAX_X_LENGTH+1,_T("%0*X"),width,value) ;                // format the value with leading zeros
    SetDlgItemText(hDlg,editboxID,tempString) ;                                 // and write it

} // end setdialogboxhex() ;

void setdialogboxhex64(HWND hDlg, int editboxID, UINT64 value)
{
    WCHAR tempString[MAX_X_LENGTH_64+2];
    uint32 a = value >> 32;
    uint32 b = (uint32) value;

    swprintf(tempString,MAX_X_LENGTH_64+1,_T("%6X%08X"),a,b) ;  // format the value with leading zeros
    SetDlgItemText(hDlg,editboxID,tempString) ;                                 // and write it

} // end setdialogboxhex() ;

#define MAX_DECINT_LENGTH 10

UINT validatedialogboxdecuint(HWND hDlg, int editboxID, UINT min, UINT max, UINT *result)
{
    int n, d, i;
    UINT v ;
    int change = 0 ;
    WCHAR tempString[MAX_DECINT_LENGTH+2];
    int width = (int)(log((double) max) / log(10.0)) + 1 ;              // width of number string
    if (min>max) min=max ;                                              // keeping sane

    n = GetDlgItemText(hDlg,editboxID,tempString,MAX_DECINT_LENGTH+1) ; // +1 to allow for string termination

/*    for ( ; n > 1 ; n--) 
    {
      // d = tempString[n-1] ;
      //  if (32 != d) break ;            // ignore trailing spaces
      if (32 != (d = tempString[n-1])) break ;            // ignore trailing spaces
    }
*/
    for ( ; n > 1 ; n--) 
    {
        if (32 != tempString[n-1]) break ;            // ignore trailing spaces
    }
    
    v = 0 ;

    for (i = 0 ; i < n ; i++ )
    {
        if (iswdigit(d = tempString[i]))
        {
          v = (v * 10) + (d - _T('0')) ;
        }
        else change = 1 ;
    }

    if (v < min) { v = min ; change = 1 ; }
    else if (v > max) { v = max ; change = 1 ; }

    //swprintf(tempString,MAX_DECINT_LENGTH+1,_T("%-*u"),width,v) ;    // format the value
    swprintf(tempString,MAX_DECINT_LENGTH+1,_T("%u"),v) ;           // format the value
    SetDlgItemText(hDlg,editboxID,tempString) ;                     // and write it back

    *result = v ;                                                   // read value

    return change ;                                                 // was it changed significantly from user input

} // end validatedialogboxdecuint()



// -------------------------------------------------------------------------------------------------------------------
//
// Function to set a decimal integer text value into a text entry box withing a dialog box
//
void setdialogboxdecuint(HWND hDlg, int editboxID, UINT value, UINT width)
{
    WCHAR tempString[MAX_DECINT_LENGTH+2];

//    swprintf(tempString,MAX_DECINT_LENGTH+1,_T("%-*u"),width,value) ;        // format the value
    swprintf(tempString,MAX_DECINT_LENGTH+1,_T("%u"),value) ;        // format the value
    // swprintf(tempString,MAX_DECINT_LENGTH+1,_T("%u"),value) ;        // format the value
    SetDlgItemText(hDlg,editboxID,tempString) ;                             // and write it

} // end setdialogboxdecuint() ;


// -------------------------------------------------------------------------------------------------------------------
//
// Function to validate input/state of a single line text edit box for specifying a floating point value
//
// Format accepted includes digits 0 to 9 and decimal point.  (Could probably also accept '+', '-' and 'e' since I expect _tstof() would accept them)
//

#define MAX_LENGTH 20

double validateeditboxdouble(HWND hWnd, double min, double max)
{
    WCHAR inbuffer[MAX_LENGTH] ;
    WCHAR outbuffer[MAX_LENGTH] ;
    WCHAR c ;

    double val ;

    int got, i, j, notgotdot = 1 ;

    got = GetWindowText(hWnd, inbuffer, MAX_LENGTH-1) ;     // read edit box text (assuming hindow is an edit box)

    for (i = 0, j = 0 ; i < got ; i++)                      // scan string and remove all non digits or decimal point
    {
        c =  inbuffer[i] ;
        if (_istdigit(c))                                   // char is digit
        {
            outbuffer[j++] = c ;                            // copy it to output
        }
        else if (notgotdot &&  (c == _T('.')))               // char is '.' (first one only)
        {
            outbuffer[j++] = c ;                            // copy it to output
            notgotdot = 0 ;                                 // remember we got it
        }

    } // end for

    outbuffer[j] = _T( '\0' ) ;                             // end the string

    SetWindowText(hWnd,outbuffer) ;                         // Set it back into window.

    val = _tstof(outbuffer) ;                               // find the float

    //printf("value read = %f ", val) ;                       // debug

    if (val < min) val = min ;
    if (val > max) val = max ;

    //printf("value returned = %f \n", val) ;                 // debug

    return val ;

} // end validateeditboxdouble()

int validateeditboxint(HWND hWnd, int min, int max)
{
    WCHAR inbuffer[MAX_LENGTH] ;
    WCHAR outbuffer[MAX_LENGTH] ;
    WCHAR c ;

    int val ;

    int got, i, j, notgotdot = 1 ;

    got = GetWindowText(hWnd, inbuffer, MAX_LENGTH-1) ;     // read edit box text (assuming hindow is an edit box)

    for (i = 0, j = 0 ; i < got ; i++)                      // scan string and remove all non digits or decimal point
    {
        c =  inbuffer[i] ;
        if (_istdigit(c))                                   // char is digit
        {
            outbuffer[j++] = c ;                            // copy it to output
        }
        else if (notgotdot &&  (c == _T('.')))               // char is '.' (first one only)
        {
            outbuffer[j++] = c ;                            // copy it to output
            notgotdot = 0 ;                                 // remember we got it
        }

    } // end for

    outbuffer[j] = _T( '\0' ) ;                             // end the string

    SetWindowText(hWnd,outbuffer) ;                         // Set it back into window.

    val = _tstoi(outbuffer) ;                               // find the float

    //printf("value read = %f ", val) ;                       // debug

    if (val < min) val = min ;
    if (val > max) val = max ;

    //printf("value returned = %f \n", val) ;                 // debug

    return val ;
}// end validateeditboxint()

// -------------------------------------------------------------------------------------------------------------------
//
// Function pop-up a window to report a debug event has occurred and OK to continue or Exit to leave.
//
extern "C" void debugEventReport(int event_number,int cannot_continue) 
{
    // single threaded code so we will stick here when called from anywhere!

    char msgBuff[256] ;
    int r ;

    sprintf_s(msgBuff,256,
            "ERROR - Debug Event %i found%s\n",
            event_number,
            (cannot_continue ? ", cannot continue, click OK to Exit."
                             : ", click Cancel to continue or OK to Exit.")
            ) ;

    if (cannot_continue)
    {
        MessageBoxA(NULL,msgBuff, "DecaWave Application", MB_OK | MB_ICONERROR);
        exit(-1) ;
    }
    else
    {
        r = MessageBoxA(NULL,msgBuff, "DecaWave Application", MB_OKCANCEL | MB_ICONEXCLAMATION);
        
        if ( r != IDCANCEL) exit(-1) ;      // fall through and return to caller if cancel selected
    }
}

