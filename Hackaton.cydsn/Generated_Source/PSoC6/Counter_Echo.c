/*******************************************************************************
* File Name: Counter_Echo.c
* Version 1.0
*
* Description:
*  This file provides the source code to the API for the Counter_Echo
*  component
*
********************************************************************************
* Copyright 2016-2017, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#include "Counter_Echo.h"

/** Indicates whether or not the Counter_Echo has been initialized. 
*  The variable is initialized to 0 and set to 1 the first time 
*  Counter_Echo_Start() is called. This allows the Component to 
*  restart without reinitialization after the first call to 
*  the Counter_Echo_Start() routine.
*/
uint8_t Counter_Echo_initVar = 0U;

/** The instance-specific configuration structure. This should be used in the 
*  associated Counter_Echo_Init() function.
*/ 
cy_stc_tcpwm_counter_config_t const Counter_Echo_config =
{
        .period = 32768UL,
        .clockPrescaler = 0UL,
        .runMode = 0UL,
        .countDirection = 0UL,
        .compareOrCapture = 2UL,
        .compare0 = 16384UL,
        .compare1 = 16384UL,
        .enableCompareSwap = false,
        .interruptSources = 0UL,
        .captureInputMode = 2UL,
        .captureInput = CY_TCPWM_INPUT_CREATOR,
        .reloadInputMode = 3UL,
        .reloadInput = CY_TCPWM_INPUT_CREATOR,
        .startInputMode = 3UL,
        .startInput = CY_TCPWM_INPUT_CREATOR,
        .stopInputMode = 3UL,
        .stopInput = CY_TCPWM_INPUT_CREATOR,
        .countInputMode = 3UL,
        .countInput = CY_TCPWM_INPUT_CREATOR,
};


/*******************************************************************************
* Function Name: Counter_Echo_Start
****************************************************************************//**
*
*  Calls the Counter_Echo_Init() when called the first time and enables 
*  the Counter_Echo. For subsequent calls the configuration is left 
*  unchanged and the component is just enabled.
*
* \globalvars
*  \ref Counter_Echo_initVar
*
*******************************************************************************/
void Counter_Echo_Start(void)
{
    if (0U == Counter_Echo_initVar)
    {
        (void)Cy_TCPWM_Counter_Init(Counter_Echo_HW, Counter_Echo_CNT_NUM, &Counter_Echo_config); 

        Counter_Echo_initVar = 1U;
    }

    Cy_TCPWM_Enable_Multiple(Counter_Echo_HW, Counter_Echo_CNT_MASK);
    
    #if (Counter_Echo_INPUT_DISABLED == 7UL)
        Cy_TCPWM_TriggerStart(Counter_Echo_HW, Counter_Echo_CNT_MASK);
    #endif /* (Counter_Echo_INPUT_DISABLED == 7UL) */    
}


/* [] END OF FILE */
