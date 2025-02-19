/**
   * Copyright (c) 2015, Realsil Semiconductor Corporation. All rights reserved.
    *
     */
#ifndef _BOARD_H_
#define _BOARD_H_


#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include "ameba_soc.h"

#define platform_debug DBG_8195A

//trace_uart
//========================================
/* CONFIG */
//#define TRACE_UART_DEFAULT 
#define TRACE_UART_TX_IRQ
//#define TRACE_UART_TX_WHILE
//#define TRACE_UART_DMA
/* TRACE UART DEFAULT USE PA18 PA19 UART0
   (BEBCAUSE OF THE BT LOG IS PA16)*/
#define TRACE_UART_BAUDRATE  1500000

#ifdef TRACE_UART_DEFAULT


#define TRACE_UART_DEV    UART0_DEV
#define TRACE_UART_IRQ    UART0_IRQ
#define TRACE_UART_TX     _PA_18
//#define TRACE_UART_RX     _PA_19

#ifdef TRACE_UART_DMA
#define TRACE_UART_INDEX        0
#define TRACEUART_DMA_PRIO      12
#endif

#else
#define TRACE_UART_DEV    UART3_DEV
#define TRACE_UART_IRQ    UARTLP_IRQ
#define TRACE_UART_TX     _PA_26
//#define TRACE_UART_RX     _PA_25   //km0 not support dma
#endif


#ifdef TRACE_UART_TX_IRQ
#define TRACE_COUNT             16   //ONE IRQ send DATA LEN
#define TRACEUART_IRQ_PRIO      12
#endif


//==hci_uart============================
//====trace_task========
#define TRACE_TASK_PRIO  3


//hci_rtk============

#define hci_board_debug DBG_8195A
#define BT_DEFAUT_LMP_SUBVER   0x8721

#define HCI_START_IQK
#define HCI_WRITE_IQK
#ifdef CONFIG_MP_INCLUDED
#define BT_MP_MODE
#define HCI_MP_BRIDGE   1
#endif



//=board.h====

#define MERGE_PATCH_ADDRESS_OTA1  0x080F8000
#define MERGE_PATCH_ADDRESS_OTA2  0x081F8000
#define MERGE_PATCH_SWITCH_ADDR   0x08003028
#define MERGE_PATCH_SWITCH_SINGLE 0xAAAAAAAA

#define CALI_IQK_RF_STEP0   0x4000
#define CALI_IQK_RF_STEP1   0x0180
#define CALI_IQK_RF_STEP2   0x3800
#define CALI_IQK_RF_STEP3F   0x0400



#define LEFUSE(x)  (x-0x190)

#define EFUSE_SW_USE_LOGIC_EFUSE     BIT0
#define EFUSE_SW_BT_FW_LOG           BIT1
#define EFUSE_SW_RSVD                BIT2
#define EFUSE_SW_IQK_HCI_OUT         BIT3
#define EFUSE_SW                     BIT4
#define EFUSE_SW_TRACE_SWITCH        BIT5
#define EFUSE_SW_DRIVER_DEBUG_LOG    BIT6
#define EFUSE_SW                     BIT7
#define CHECK_SW(x)                  (hci_tp_lgc_efuse[LEFUSE(0x1A1)] & x)

#if 0
uint8_t rltk_wlan_is_mp();
#endif


#ifdef __cplusplus
}
#endif

#endif

