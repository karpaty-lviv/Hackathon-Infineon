/***************************************************************************//**
* \file IR.h
* \version 2.0
*
*  This file provides constants and parameter values for the UART component.
*
********************************************************************************
* \copyright
* Copyright 2016-2017, Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#if !defined(IR_CY_SCB_UART_PDL_H)
#define IR_CY_SCB_UART_PDL_H

#include "cyfitter.h"
#include "scb/cy_scb_uart.h"

#if defined(__cplusplus)
extern "C" {
#endif

/***************************************
*   Initial Parameter Constants
****************************************/

#define IR_DIRECTION  (1U)
#define IR_ENABLE_RTS (0U)
#define IR_ENABLE_CTS (0U)

/* UART direction enum */
#define IR_RX    (0x1U)
#define IR_TX    (0x2U)

#define IR_ENABLE_RX  (0UL != (IR_DIRECTION & IR_RX))
#define IR_ENABLE_TX  (0UL != (IR_DIRECTION & IR_TX))


/***************************************
*        Function Prototypes
***************************************/
/**
* \addtogroup group_general
* @{
*/
/* Component specific functions. */
void IR_Start(void);

/* Basic functions */
__STATIC_INLINE cy_en_scb_uart_status_t IR_Init(cy_stc_scb_uart_config_t const *config);
__STATIC_INLINE void IR_DeInit(void);
__STATIC_INLINE void IR_Enable(void);
__STATIC_INLINE void IR_Disable(void);

/* Register callback. */
__STATIC_INLINE void IR_RegisterCallback(cy_cb_scb_uart_handle_events_t callback);

/* Configuration change. */
#if (IR_ENABLE_CTS)
__STATIC_INLINE void IR_EnableCts(void);
__STATIC_INLINE void IR_DisableCts(void);
#endif /* (IR_ENABLE_CTS) */

#if (IR_ENABLE_RTS)
__STATIC_INLINE void     IR_SetRtsFifoLevel(uint32_t level);
__STATIC_INLINE uint32_t IR_GetRtsFifoLevel(void);
#endif /* (IR_ENABLE_RTS) */

__STATIC_INLINE void IR_EnableSkipStart(void);
__STATIC_INLINE void IR_DisableSkipStart(void);

#if (IR_ENABLE_RX)
/* Low level: Receive direction. */
__STATIC_INLINE uint32_t IR_Get(void);
__STATIC_INLINE uint32_t IR_GetArray(void *buffer, uint32_t size);
__STATIC_INLINE void     IR_GetArrayBlocking(void *buffer, uint32_t size);
__STATIC_INLINE uint32_t IR_GetRxFifoStatus(void);
__STATIC_INLINE void     IR_ClearRxFifoStatus(uint32_t clearMask);
__STATIC_INLINE uint32_t IR_GetNumInRxFifo(void);
__STATIC_INLINE void     IR_ClearRxFifo(void);
#endif /* (IR_ENABLE_RX) */

#if (IR_ENABLE_TX)
/* Low level: Transmit direction. */
__STATIC_INLINE uint32_t IR_Put(uint32_t data);
__STATIC_INLINE uint32_t IR_PutArray(void *buffer, uint32_t size);
__STATIC_INLINE void     IR_PutArrayBlocking(void *buffer, uint32_t size);
__STATIC_INLINE void     IR_PutString(char_t const string[]);
__STATIC_INLINE void     IR_SendBreakBlocking(uint32_t breakWidth);
__STATIC_INLINE uint32_t IR_GetTxFifoStatus(void);
__STATIC_INLINE void     IR_ClearTxFifoStatus(uint32_t clearMask);
__STATIC_INLINE uint32_t IR_GetNumInTxFifo(void);
__STATIC_INLINE bool     IR_IsTxComplete(void);
__STATIC_INLINE void     IR_ClearTxFifo(void);
#endif /* (IR_ENABLE_TX) */

#if (IR_ENABLE_RX)
/* High level: Ring buffer functions. */
__STATIC_INLINE void     IR_StartRingBuffer(void *buffer, uint32_t size);
__STATIC_INLINE void     IR_StopRingBuffer(void);
__STATIC_INLINE void     IR_ClearRingBuffer(void);
__STATIC_INLINE uint32_t IR_GetNumInRingBuffer(void);

/* High level: Receive direction functions. */
__STATIC_INLINE cy_en_scb_uart_status_t IR_Receive(void *buffer, uint32_t size);
__STATIC_INLINE void     IR_AbortReceive(void);
__STATIC_INLINE uint32_t IR_GetReceiveStatus(void);
__STATIC_INLINE uint32_t IR_GetNumReceived(void);
#endif /* (IR_ENABLE_RX) */

#if (IR_ENABLE_TX)
/* High level: Transmit direction functions. */
__STATIC_INLINE cy_en_scb_uart_status_t IR_Transmit(void *buffer, uint32_t size);
__STATIC_INLINE void     IR_AbortTransmit(void);
__STATIC_INLINE uint32_t IR_GetTransmitStatus(void);
__STATIC_INLINE uint32_t IR_GetNumLeftToTransmit(void);
#endif /* (IR_ENABLE_TX) */

/* Interrupt handler */
__STATIC_INLINE void IR_Interrupt(void);
/** @} group_general */


/***************************************
*    Variables with External Linkage
***************************************/
/**
* \addtogroup group_globals
* @{
*/
extern uint8_t IR_initVar;
extern cy_stc_scb_uart_config_t const IR_config;
extern cy_stc_scb_uart_context_t IR_context;
/** @} group_globals */


/***************************************
*         Preprocessor Macros
***************************************/
/**
* \addtogroup group_macros
* @{
*/
/** The pointer to the base address of the hardware */
#define IR_HW     ((CySCB_Type *) IR_SCB__HW)
/** @} group_macros */


/***************************************
*    In-line Function Implementation
***************************************/

/*******************************************************************************
* Function Name: IR_Init
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_Init() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE cy_en_scb_uart_status_t IR_Init(cy_stc_scb_uart_config_t const *config)
{
   return Cy_SCB_UART_Init(IR_HW, config, &IR_context);
}


/*******************************************************************************
* Function Name: IR_DeInit
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_DeInit() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE void IR_DeInit(void)
{
    Cy_SCB_UART_DeInit(IR_HW);
}


/*******************************************************************************
* Function Name: IR_Enable
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_Enable() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE void IR_Enable(void)
{
    Cy_SCB_UART_Enable(IR_HW);
}


/*******************************************************************************
* Function Name: IR_Disable
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_Disable() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE void IR_Disable(void)
{
    Cy_SCB_UART_Disable(IR_HW, &IR_context);
}


/*******************************************************************************
* Function Name: IR_RegisterCallback
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_RegisterCallback() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE void IR_RegisterCallback(cy_cb_scb_uart_handle_events_t callback)
{
    Cy_SCB_UART_RegisterCallback(IR_HW, callback, &IR_context);
}


#if (IR_ENABLE_CTS)
/*******************************************************************************
* Function Name: IR_EnableCts
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_EnableCts() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE void IR_EnableCts(void)
{
    Cy_SCB_UART_EnableCts(IR_HW);
}


/*******************************************************************************
* Function Name: Cy_SCB_UART_DisableCts
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_DisableCts() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE void IR_DisableCts(void)
{
    Cy_SCB_UART_DisableCts(IR_HW);
}
#endif /* (IR_ENABLE_CTS) */


#if (IR_ENABLE_RTS)
/*******************************************************************************
* Function Name: IR_SetRtsFifoLevel
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_SetRtsFifoLevel() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE void IR_SetRtsFifoLevel(uint32_t level)
{
    Cy_SCB_UART_SetRtsFifoLevel(IR_HW, level);
}


/*******************************************************************************
* Function Name: IR_GetRtsFifoLevel
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_GetRtsFifoLevel() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE uint32_t IR_GetRtsFifoLevel(void)
{
    return Cy_SCB_UART_GetRtsFifoLevel(IR_HW);
}
#endif /* (IR_ENABLE_RTS) */


/*******************************************************************************
* Function Name: IR_EnableSkipStart
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_EnableSkipStart() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE void IR_EnableSkipStart(void)
{
    Cy_SCB_UART_EnableSkipStart(IR_HW);
}


/*******************************************************************************
* Function Name: IR_DisableSkipStart
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_DisableSkipStart() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE void IR_DisableSkipStart(void)
{
    Cy_SCB_UART_DisableSkipStart(IR_HW);
}


#if (IR_ENABLE_RX)
/*******************************************************************************
* Function Name: IR_Get
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_Get() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE uint32_t IR_Get(void)
{
    return Cy_SCB_UART_Get(IR_HW);
}


/*******************************************************************************
* Function Name: IR_GetArray
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_GetArray() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE uint32_t IR_GetArray(void *buffer, uint32_t size)
{
    return Cy_SCB_UART_GetArray(IR_HW, buffer, size);
}


/*******************************************************************************
* Function Name: IR_GetArrayBlocking
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_GetArrayBlocking() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE void IR_GetArrayBlocking(void *buffer, uint32_t size)
{
    Cy_SCB_UART_GetArrayBlocking(IR_HW, buffer, size);
}


/*******************************************************************************
* Function Name: IR_GetRxFifoStatus
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_GetRxFifoStatus() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE uint32_t IR_GetRxFifoStatus(void)
{
    return Cy_SCB_UART_GetRxFifoStatus(IR_HW);
}


/*******************************************************************************
* Function Name: IR_ClearRxFifoStatus
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_ClearRxFifoStatus() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE void IR_ClearRxFifoStatus(uint32_t clearMask)
{
    Cy_SCB_UART_ClearRxFifoStatus(IR_HW, clearMask);
}


/*******************************************************************************
* Function Name: IR_GetNumInRxFifo
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_GetNumInRxFifo() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE uint32_t IR_GetNumInRxFifo(void)
{
    return Cy_SCB_UART_GetNumInRxFifo(IR_HW);
}


/*******************************************************************************
* Function Name: IR_ClearRxFifo
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_ClearRxFifo() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE void IR_ClearRxFifo(void)
{
    Cy_SCB_UART_ClearRxFifo(IR_HW);
}
#endif /* (IR_ENABLE_RX) */


#if (IR_ENABLE_TX)
/*******************************************************************************
* Function Name: IR_Put
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_Put() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE uint32_t IR_Put(uint32_t data)
{
    return Cy_SCB_UART_Put(IR_HW,data);
}


/*******************************************************************************
* Function Name: IR_PutArray
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_PutArray() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE uint32_t IR_PutArray(void *buffer, uint32_t size)
{
    return Cy_SCB_UART_PutArray(IR_HW, buffer, size);
}


/*******************************************************************************
* Function Name: IR_PutArrayBlocking
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_PutArrayBlocking() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE void IR_PutArrayBlocking(void *buffer, uint32_t size)
{
    Cy_SCB_UART_PutArrayBlocking(IR_HW, buffer, size);
}


/*******************************************************************************
* Function Name: IR_PutString
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_PutString() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE void IR_PutString(char_t const string[])
{
    Cy_SCB_UART_PutString(IR_HW, string);
}


/*******************************************************************************
* Function Name: IR_SendBreakBlocking
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_SendBreakBlocking() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE void IR_SendBreakBlocking(uint32_t breakWidth)
{
    Cy_SCB_UART_SendBreakBlocking(IR_HW, breakWidth);
}


/*******************************************************************************
* Function Name: IR_GetTxFifoStatus
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_GetTxFifoStatus() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE uint32_t IR_GetTxFifoStatus(void)
{
    return Cy_SCB_UART_GetTxFifoStatus(IR_HW);
}


/*******************************************************************************
* Function Name: IR_ClearTxFifoStatus
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_ClearTxFifoStatus() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE void IR_ClearTxFifoStatus(uint32_t clearMask)
{
    Cy_SCB_UART_ClearTxFifoStatus(IR_HW, clearMask);
}


/*******************************************************************************
* Function Name: IR_GetNumInTxFifo
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_GetNumInTxFifo() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE uint32_t IR_GetNumInTxFifo(void)
{
    return Cy_SCB_UART_GetNumInTxFifo(IR_HW);
}


/*******************************************************************************
* Function Name: IR_IsTxComplete
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_IsTxComplete() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE bool IR_IsTxComplete(void)
{
    return Cy_SCB_UART_IsTxComplete(IR_HW);
}


/*******************************************************************************
* Function Name: IR_ClearTxFifo
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_ClearTxFifo() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE void IR_ClearTxFifo(void)
{
    Cy_SCB_UART_ClearTxFifo(IR_HW);
}
#endif /* (IR_ENABLE_TX) */


#if (IR_ENABLE_RX)
/*******************************************************************************
* Function Name: IR_StartRingBuffer
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_StartRingBuffer() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE void IR_StartRingBuffer(void *buffer, uint32_t size)
{
    Cy_SCB_UART_StartRingBuffer(IR_HW, buffer, size, &IR_context);
}


/*******************************************************************************
* Function Name: IR_StopRingBuffer
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_StopRingBuffer() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE void IR_StopRingBuffer(void)
{
    Cy_SCB_UART_StopRingBuffer(IR_HW, &IR_context);
}


/*******************************************************************************
* Function Name: IR_ClearRingBuffer
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_ClearRingBuffer() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE void IR_ClearRingBuffer(void)
{
    Cy_SCB_UART_ClearRingBuffer(IR_HW, &IR_context);
}


/*******************************************************************************
* Function Name: IR_GetNumInRingBuffer
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_GetNumInRingBuffer() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE uint32_t IR_GetNumInRingBuffer(void)
{
    return Cy_SCB_UART_GetNumInRingBuffer(IR_HW, &IR_context);
}


/*******************************************************************************
* Function Name: IR_Receive
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_Receive() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE cy_en_scb_uart_status_t IR_Receive(void *buffer, uint32_t size)
{
    return Cy_SCB_UART_Receive(IR_HW, buffer, size, &IR_context);
}


/*******************************************************************************
* Function Name: IR_GetReceiveStatus
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_GetReceiveStatus() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE uint32_t IR_GetReceiveStatus(void)
{
    return Cy_SCB_UART_GetReceiveStatus(IR_HW, &IR_context);
}


/*******************************************************************************
* Function Name: IR_AbortReceive
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_AbortReceive() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE void IR_AbortReceive(void)
{
    Cy_SCB_UART_AbortReceive(IR_HW, &IR_context);
}


/*******************************************************************************
* Function Name: IR_GetNumReceived
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_GetNumReceived() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE uint32_t IR_GetNumReceived(void)
{
    return Cy_SCB_UART_GetNumReceived(IR_HW, &IR_context);
}
#endif /* (IR_ENABLE_RX) */


#if (IR_ENABLE_TX)
/*******************************************************************************
* Function Name: IR_Transmit
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_Transmit() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE cy_en_scb_uart_status_t IR_Transmit(void *buffer, uint32_t size)
{
    return Cy_SCB_UART_Transmit(IR_HW, buffer, size, &IR_context);
}


/*******************************************************************************
* Function Name: IR_GetTransmitStatus
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_GetTransmitStatus() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE uint32_t IR_GetTransmitStatus(void)
{
    return Cy_SCB_UART_GetTransmitStatus(IR_HW, &IR_context);
}


/*******************************************************************************
* Function Name: IR_AbortTransmit
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_AbortTransmit() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE void IR_AbortTransmit(void)
{
    Cy_SCB_UART_AbortTransmit(IR_HW, &IR_context);
}


/*******************************************************************************
* Function Name: IR_GetNumLeftToTransmit
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_GetNumLeftToTransmit() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE uint32_t IR_GetNumLeftToTransmit(void)
{
    return Cy_SCB_UART_GetNumLeftToTransmit(IR_HW, &IR_context);
}
#endif /* (IR_ENABLE_TX) */


/*******************************************************************************
* Function Name: IR_Interrupt
****************************************************************************//**
*
* Invokes the Cy_SCB_UART_Interrupt() PDL driver function.
*
*******************************************************************************/
__STATIC_INLINE void IR_Interrupt(void)
{
    Cy_SCB_UART_Interrupt(IR_HW, &IR_context);
}

#if defined(__cplusplus)
}
#endif

#endif /* IR_CY_SCB_UART_PDL_H */


/* [] END OF FILE */
