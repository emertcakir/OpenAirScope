#ifndef __OPENAIRSCOPE_TEST_H
#define __OPENAIRSCOPE_TEST_H

#include "stm32h7xx_hal.h"
#include <stdio.h>

// UART handle (UART4 will be used, PD0 = TX, PD1 = RX)
extern UART_HandleTypeDef huart4;

// Test functions
void OpenAirScopeTest_Init(void);
void OpenAirScopeTest_Run(void);

// Individual test functions
void Test_GPIO(void);
void Test_ADC(void);
void Test_DAC(void);
void Test_I2C(void);
void Test_SPI(void);
void Test_CAN(void);
void Test_UART(void);

#endif /* __OPENAIRSCOPE_TEST_H */
