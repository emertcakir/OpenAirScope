/**
 ******************************************************************************
 * @file    OpenAirScopeTest.h
 * @brief   OpenAirScope Hardware Test Framework - Header
 *
 * @details STM32H743-based OpenAirScope weather station hardware validation
 *          framework. Produces a PASS/FAIL result for every peripheral,
 *          reports error details, and transmits the overall test summary
 *          to a serial terminal via UART4.
 *
 * @board   OpenAirScope v1.x  (STM32H743VIT6)
 * @author  OpenAirScope Contributors
 * @date    2025
 ******************************************************************************
 */

#ifndef __OPENAIRSCOPE_TEST_H
#define __OPENAIRSCOPE_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Configuration Macros
 * -------------------------------------------------------------------------*/

/** Debug UART: UART4  (PA0=TX, PA1=RX — from main.c GPIO table) */
#define TEST_UART_INSTANCE          UART4
#define TEST_UART_BAUDRATE          115200U
#define TEST_UART_TX_PORT           GPIOA
#define TEST_UART_TX_PIN            GPIO_PIN_0
#define TEST_UART_RX_PORT           GPIOA
#define TEST_UART_RX_PIN            GPIO_PIN_1
#define TEST_UART_AF                GPIO_AF8_UART4

/** LED / GPIO test pin (main.c: PB0 configured as output) */
#define TEST_LED_PORT               GPIOB
#define TEST_LED_PIN                GPIO_PIN_0
#define TEST_LED_BLINK_DELAY_MS     150U
#define TEST_LED_BLINK_COUNT        3U

/** ADC: ADC1, channel 18 (Vbat) — sample measurement */
#define TEST_ADC_INSTANCE           ADC1
#define TEST_ADC_CHANNEL            ADC_CHANNEL_18
#define TEST_ADC_TIMEOUT_MS         200U
#define TEST_ADC_EXPECTED_MIN       100U    /**< Expected minimum raw value  */
#define TEST_ADC_EXPECTED_MAX       4095U   /**< Expected maximum raw value  */

/** DAC: DAC1 Channel-1 */
#define TEST_DAC_INSTANCE           DAC1
#define TEST_DAC_CHANNEL            DAC_CHANNEL_1
#define TEST_DAC_VALUE_12BIT        2048U   /**< ~1.65 V (Vref = 3.3 V)     */

/** I2C: I2C1  (PB6=SCL, PB7=SDA — main.c GPIO_AF4_I2C1) */
#define TEST_I2C_INSTANCE           I2C1
#define TEST_I2C_SLAVE_ADDR         0x50U   /**< Target device 7-bit address */
#define TEST_I2C_TIMEOUT_MS         50U

/** SPI: SPI1  (PG11=SCK, PB5=MOSI — main.c GPIO_AF5_SPI1) */
#define TEST_SPI_INSTANCE           SPI1
#define TEST_SPI_TX_BYTE            0x55U
#define TEST_SPI_TIMEOUT_MS         50U

/** FDCAN: FDCAN1 (PB8=RX, PB9=TX — stm32h7xx_hal_msp.c) */
#define TEST_FDCAN_INSTANCE         FDCAN1
#define TEST_FDCAN_STD_ID           0x123U
#define TEST_FDCAN_TX_TIMEOUT_MS    50U

/** Secondary UART for loopback test (USART3: PB10=TX, PB11=RX) */
#define TEST_USART_INSTANCE         USART3
#define TEST_USART_BAUDRATE         115200U
#define TEST_USART_TIMEOUT_MS       100U

/* -------------------------------------------------------------------------
 * Type Definitions
 * -------------------------------------------------------------------------*/

/** Result of a single test */
typedef enum {
    TEST_RESULT_PASS    = 0,
    TEST_RESULT_FAIL    = 1,
    TEST_RESULT_SKIP    = 2,
    TEST_RESULT_PENDING = 3
} TestResult_t;

/** Metadata structure stored for each test */
typedef struct {
    const char   *name;         /**< Test name                               */
    TestResult_t  result;       /**< Outcome: PASS / FAIL / SKIP             */
    uint32_t      durationMs;   /**< Execution time (ms)                     */
    char          detail[96];   /**< Additional info or error description     */
} TestRecord_t;

/** Summary of the entire test run */
typedef struct {
    uint8_t      total;
    uint8_t      passed;
    uint8_t      failed;
    uint8_t      skipped;
    uint32_t     totalDurationMs;
} TestSummary_t;

/* -------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------*/

/**
 * @brief  Initialises the test environment (opens UART4 debug port).
 * @retval HAL_OK on success, HAL_ERROR on initialisation failure.
 */
HAL_StatusTypeDef OpenAirScopeTest_Init(void);

/**
 * @brief  Runs all tests sequentially and prints a summary report.
 * @retval Total number of failed tests (0 = all passed).
 */
uint8_t OpenAirScopeTest_Run(void);

/**
 * @brief  Returns the summary of the last test run.
 */
TestSummary_t OpenAirScopeTest_GetSummary(void);

/* -------------------------------------------------------------------------
 * Individual Test Functions
 * -------------------------------------------------------------------------*/
TestResult_t Test_GPIO(TestRecord_t *rec);
TestResult_t Test_ADC(TestRecord_t *rec);
TestResult_t Test_DAC(TestRecord_t *rec);
TestResult_t Test_I2C(TestRecord_t *rec);
TestResult_t Test_SPI(TestRecord_t *rec);
TestResult_t Test_FDCAN(TestRecord_t *rec);
TestResult_t Test_UART(TestRecord_t *rec);

/* -------------------------------------------------------------------------
 * Peripheral Handles (extern — shared with hal_msp / main)
 * -------------------------------------------------------------------------*/
extern UART_HandleTypeDef  huart4;    /**< Debug / log UART                  */
extern ADC_HandleTypeDef   hadc1;     /**< ADC1                              */
extern DAC_HandleTypeDef   hdac1;     /**< DAC1                              */
extern I2C_HandleTypeDef   hi2c1;     /**< I2C1                              */
extern SPI_HandleTypeDef   hspi1;     /**< SPI1                              */
extern FDCAN_HandleTypeDef hfdcan1;   /**< FDCAN1 (defined in main.c)        */
extern UART_HandleTypeDef  huart3;    /**< USART3 loopback test              */

#ifdef __cplusplus
}
#endif

#endif /* __OPENAIRSCOPE_TEST_H */
