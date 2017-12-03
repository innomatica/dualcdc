/**
    \file
    \author <a href="http://www.innomatic.ca">innomatic</a>
    \brief  Dual COM port CDC device
    
    A USB CDC device with two COM ports, each connected to its own UART 
    port. Settings of the UARTs are fixed with 115Kb, N81.
    
 */
#include "project.h"
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>  

#define USBUART_BUFFER_SIZE     (64u)
#define DBG_PRINT               0

void USB_Task(void);
void CDC_RxTask(void);
void UART_RxTask(void);

/** Check USB configuration and initialize CDC interface
 */
void USB_Task(void)
{
    // check if USB configuration is changed
    if (USB_IsConfigurationChanged())
    {
        // initialize IN endpoints when device is configured
        if (USB_GetConfiguration())
        {
            //enable OUT endpoint to receive data from host
            USB_CDC_Init();
        }
    }
}

/** Read UART Rx buffer and transfer data to CDC interface
 */
void UART_RxTask(void)
{
    static uint8 port_no = 0;    
    uint16 count, index = 0;
    uint8 buffer[USBUART_BUFFER_SIZE];   
    
    if(port_no == 0)
    {
        // check UART0 rx data
        count = UART0_GetRxBufferSize();
        // rx buffer not empty
        if(count)
        {
            // read all data
            while(count)
            {
                buffer[index++] = UART0_GetByte();
                count--;
            }
            // select CDC port
            USB_SetComPort(0);
            // wait until the CDC is ready
            while(!USB_CDCIsReady());
            // send data
            USB_PutData(buffer, index);
        }
    }
    else
    {
        // check UART1 rx data
        count = UART1_GetRxBufferSize();
        // rx buffer not empty
        if(count)
        {
            // read all data
            while(count)
            {
                buffer[index++] = UART1_GetByte();
                count--;
            }
            // select CDC port
            USB_SetComPort(1);
            // wait until the CDC is ready
            while(!USB_CDCIsReady());
            // send data
            USB_PutData(buffer, index);
        }        
    }
    
    // switch port
    port_no = (port_no + 1) % 2;   
}

/** Check CDC Rx buffer and transfer data to UART
 */
void CDC_RxTask(void)
{
    static uint8 port_no = 0;
    uint16 count;
    uint8 buffer[USBUART_BUFFER_SIZE];    
    
    // choose port
    USB_SetComPort(port_no);
 
    // check if the device is configured
    if (USB_GetConfiguration())
    {
        // check for CDC rx data
        if (USB_DataIsReady())
        {
            // read received data
            count = USB_GetAll(buffer);

            if (count)
            {
                // send data to corresponding UART
                if(port_no == 0)
                {
                    UART0_PutArray(buffer, count);
                }
                else
                {
                    UART1_PutArray(buffer, count);                    
                }
            }
        }
    }    
    
    // switch COM port
    port_no = (port_no + 1) % 2;
}


int main(void)
{
    CyGlobalIntEnable;

    // intialize UART module
    UART0_Start();
    UART1_Start();
    
    // initialize USB module
    USB_Start(0, USB_3V_OPERATION);  
    
    for(;;)
    {
        // handles USB configuration change
        USB_Task();
        // handles USB CDC rx input
        CDC_RxTask();
        // handles UART rx input
        UART_RxTask();
    }
}
