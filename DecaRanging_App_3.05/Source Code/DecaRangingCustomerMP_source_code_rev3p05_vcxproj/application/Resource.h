//{{NO_DEPENDENCIES}}
// Microsoft Visual C++ generated include file.
// Used by DecaRanging.rc
//
/*! ----------------------------------------------------------------------------
 * @file	resourse.h 
 * @brief	Manually edited file to add definitions for DecaRanging application's menus, buttons and dialog boxes
 *		    
 *
 * @attention
 *
 * Copyright 2013 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 */

#define IDS_APP_TITLE			103

#define IDR_MAINFRAME			128

#define IDD_DECARANGING_DIALOG	102
#define IDD_ABOUTBOX			103
#define IDM_ABOUT				104
#define IDM_EXIT				105
#define IDI_DECARANGING			107
#define IDI_SMALL				108
#define IDC_DECARANGING			109
#define IDM_REGACCESS           110
#define IDD_CONTFRAME			111
#define IDD_REGACCESSDIALOG     112
#define IDM_ACCVIEW             113
#define IDM_CHANSETUP           114
#define IDD_CHANSETUPDIALOG     115
#define IDC_CHANSELECT			116
#define IDM_LARGETEXTVIEW       117
#define IDM_LOGALLACC           118
#define IDM_RNGVIEW				119
#define IDM_TIMINGSETUP        120    
#define IDD_TIMINGDIALOG       121
#define IDM_ACCFPALIGN          122
#define IDM_SOFTRESET           123
#define IDM_CONTFRAME			124
#define IDM_ACCMAGONLY          125
#define IDB_SPLASH              126
#define IDM_DISPFEETINCHES      128
#define IDM_DISPCLKOFFSET		129
#define IDM_CWMODE				130
#define IDD_CWMODE				131
#define IDB_FOLDER              132
//#define IDM_xxxxx             134
//#define IDD_xxxxxDIALOG       135
#define IDM_LOGSPI              136
#define IDM_UCODELOAD			138
#define IDD_UCODELOADDIALOG		139
#define IDM_ACCVIEWGF           140

#define IDM_LARGEINST           144
#define IDM_LARGEAVE4           145
#define IDM_LARGEAVE8           146
#define IDM_LARGE2DP            147

#define IDD_TAGSELECTDIALOG		148
#define IDC_TAGSELECT		    149

#define BV_BASE                 (156)          
#define RESTART_BUTTON_ID       (BV_BASE+0)
#define CLEAR_COUNTS_BUTTON_ID  (BV_BASE+1)
#define CONFIGURE_BUTTON_ID     (BV_BASE+2)
#define PAUSE_BUTTON_ID         (BV_BASE+3)
#define ANTENNA_DELAY_ID        (BV_BASE+4)
#define LOGOFFACC_BUTTON_ID     (BV_BASE+5)
#define PREVACC_BUTTON_ID		(BV_BASE+6)
#define NXTACC_BUTTON_ID		(BV_BASE+7)
#define SAVEACC_BUTTON_ID		(BV_BASE+8)
#define ROLE_ID                 (BV_BASE+10)
#define MAIN_PANEL_ID           (BV_BASE+11)

#define BVBASE1                 200          
#define ID_REGREAD              (BVBASE1+0)
#define ID_REGWRITE             (BVBASE1+1)
#define ID_REG_ADDRBOX          (BVBASE1+2)
#define ID_REG_ADDRESS          (BVBASE1+3)
#define ID_REG_VALBOX           (BVBASE1+4)
#define ID_REG_VALUE            (BVBASE1+5)

#define ID_FRD_REGID_BOX        (BVBASE1+6)
#define ID_FRD_REGID            (BVBASE1+7)
#define ID_FRD_BLKS_BOX         (BVBASE1+8)
#define ID_FRD_BLKS             (BVBASE1+9)
#define ID_FRD_NBLK_BOX         (BVBASE1+10)
#define ID_FRD_NBLK             (BVBASE1+11)
#define ID_FRD_NAME_BOX         (BVBASE1+12)
#define ID_FRD_NAME             (BVBASE1+13)
#define ID_FRD_FILESEL          (BVBASE1+14)

#define ID_REG_OFFSBOX          (BVBASE1+15)
#define ID_REG_OFFSET           (BVBASE1+16)

#define BVBASE2                 200             // it is okay for ID to overlap with other dialogs above

#define ID_CONF_CHANBOX         (BVBASE2+6)

#define ID_CONF_PRFBOX          (BVBASE2+7)
#define ID_CONF_PRF16MHZ        (BVBASE2+8)
#define ID_CONF_PRF64MHZ        (BVBASE2+9)

#define ID_CONF_RATEBOX         (BVBASE2+11)
#define ID_CONF_RATE110K        (BVBASE2+12)
#define ID_CONF_RATE850K        (BVBASE2+13)
#define ID_CONF_RATE6M81        (BVBASE2+14)

#define ID_CONF_PREAMBOX        (BVBASE2+15)
//#define ID_CONF_PREAM16         (BVBASE2+16)
//#define ID_CONF_PREAM32         (BVBASE2+17)
#define ID_CONF_PREAM64         (BVBASE2+18)
#define ID_CONF_PREAM128        (BVBASE2+29)
#define ID_CONF_PREAM256        (BVBASE2+20)
#define ID_CONF_PREAM512        (BVBASE2+21)
#define ID_CONF_PREAM1024       (BVBASE2+22)
#define ID_CONF_PREAM2048       (BVBASE2+23)
#define ID_CONF_PREAM4096       (BVBASE2+24)
#define ID_CONF_PREAM1536		(BVBASE2+25)

#define ID_CONF_PCODBOX         (BVBASE2+31)
#define IDC_PCODSELECT          (BVBASE2+32)

#define ID_CONF_TXATTBOX        (BVBASE2+38)
#define IDC_TXATTSELECT         (BVBASE2+39)

#define ID_CONF_MISCBOX         (BVBASE2+40)
#define ID_CONF_NSTDSFD         (BVBASE2+41)

#define PAC_SELECTION			(0) //enable this to enable user PAC configuration
#define SNIFF_SELECTION			(0) //enable SNIFF mode configuration

#define ID_CONF_PAC_SIZE_BOX    (BVBASE2+43)
#define ID_CONF_PAC_8_SYM       (BVBASE2+44)
#define ID_CONF_PAC_16_SYM      (BVBASE2+45)
#define ID_CONF_PAC_32_SYM      (BVBASE2+46)
#define ID_CONF_PAC_64_SYM      (BVBASE2+47)

#define ID_CONF_SNIFF_SIZE_BOX  (BVBASE2+48)
#define ID_SNIFF_ON				(BVBASE2+50)
#define ID_SNIFF_OFF			(BVBASE2+51)

//#define BVBASE2                 260           -  this base causes problems- Maybe numbers have to stay less than 256 ?
#define BVBASE3                 200             // it is okay for ID of Payload configure items to overlap with other dialogs above
#define ID_A1POLL				(BVBASE3+11)
#define ID_A2POLL				(BVBASE3+12)
#define ID_A3POLL				(BVBASE3+13)
#define ID_A4POLL				(BVBASE3+14)
#define ID_PL_TAADDRBOX			(BVBASE3+15)
#define ID_TAG_POLL_PERIOD		(BVBASE3+16)
#define ID_ANC_RESP_DLY			(BVBASE3+18)
#define ID_TAG_BLINK_PERIOD		(BVBASE3+19)
#define ID_TAG_RESP_DLY	        (BVBASE3+20) 

#define IDC_MYICON              2
#ifndef IDC_STATIC
#define IDC_STATIC              -1
#endif
// Next default values for new objects
//
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS

#define _APS_NO_MFC                 130
#define _APS_NEXT_RESOURCE_VALUE    129
#define _APS_NEXT_COMMAND_VALUE     32771
#define _APS_NEXT_CONTROL_VALUE     1000
#define _APS_NEXT_SYMED_VALUE       110
#endif
#endif
