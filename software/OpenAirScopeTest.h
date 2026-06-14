/**
 ******************************************************************************
 * @file    OpenAirScopeTest.h
 * @brief   OpenAirScope Hardware Test Framework - Header
 *
 * @details STM32H743 tabanlı OpenAirScope hava istasyonu donanım doğrulama
 *          çerçevesi. Her periferik için PASS/FAIL sonucu üretir, hata
 *          detaylarını raporlar ve toplam test özetini UART4 üzerinden seri
 *          porta iletir.
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
 * Konfigürasyon Makroları
 * -------------------------------------------------------------------------*/

/** Debug UART: UART4  (PA0=TX, PA1=RX  — main.c GPIO tablosundan) */
#define TEST_UART_INSTANCE          UART4
#define TEST_UART_BAUDRATE          115200U
#define TEST_UART_TX_PORT           GPIOA
#define TEST_UART_TX_PIN            GPIO_PIN_0
#define TEST_UART_RX_PORT           GPIOA
#define TEST_UART_RX_PIN            GPIO_PIN_1
#define TEST_UART_AF                GPIO_AF8_UART4

/** LED / GPIO test pini (main.c: PB0 output) */
#define TEST_LED_PORT               GPIOB
#define TEST_LED_PIN                GPIO_PIN_0
#define TEST_LED_BLINK_DELAY_MS     150U
#define TEST_LED_BLINK_COUNT        3U

/** ADC: ADC1, kanal 18 (Vbat) — örnek ölçüm */
#define TEST_ADC_INSTANCE           ADC1
#define TEST_ADC_CHANNEL            ADC_CHANNEL_18
#define TEST_ADC_TIMEOUT_MS         200U
#define TEST_ADC_EXPECTED_MIN       100U    /**< Beklenen min. ham değer   */
#define TEST_ADC_EXPECTED_MAX       4095U   /**< Beklenen maks. ham değer  */

/** DAC: DAC1 Kanal-1 */
#define TEST_DAC_INSTANCE           DAC1
#define TEST_DAC_CHANNEL            DAC_CHANNEL_1
#define TEST_DAC_VALUE_12BIT        2048U   /**< ~1.65 V (Vref=3.3 V)     */

/** I2C: I2C1  (PB6=SCL, PB7=SDA — main.c GPIO_AF4_I2C1) */
#define TEST_I2C_INSTANCE           I2C1
#define TEST_I2C_SLAVE_ADDR         0x50U   /**< Hedef cihaz 7-bit adresi */
#define TEST_I2C_TIMEOUT_MS         50U

/** SPI: SPI1  (PG11=SCK, PB5=MOSI — main.c GPIO_AF5_SPI1) */
#define TEST_SPI_INSTANCE           SPI1
#define TEST_SPI_TX_BYTE            0x55U
#define TEST_SPI_TIMEOUT_MS         50U

/** FDCAN: FDCAN1 (PB8=RX, PB9=TX — stm32h7xx_hal_msp.c) */
#define TEST_FDCAN_INSTANCE         FDCAN1
#define TEST_FDCAN_STD_ID           0x123U
#define TEST_FDCAN_TX_TIMEOUT_MS    50U

/** UART loopback testi için ek UART (USART3: PB10=TX, PB11=RX) */
#define TEST_USART_INSTANCE         USART3
#define TEST_USART_BAUDRATE         115200U
#define TEST_USART_TIMEOUT_MS       100U

/* -------------------------------------------------------------------------
 * Tip Tanımları
 * -------------------------------------------------------------------------*/

/** Tek bir testin sonucu */
typedef enum {
    TEST_RESULT_PASS    = 0,
    TEST_RESULT_FAIL    = 1,
    TEST_RESULT_SKIP    = 2,
    TEST_RESULT_PENDING = 3
} TestResult_t;

/** Her test için tutulan meta veri yapısı */
typedef struct {
    const char   *name;         /**< Test adı                              */
    TestResult_t  result;       /**< Sonuç: PASS / FAIL / SKIP             */
    uint32_t      durationMs;   /**< Süre (ms)                             */
    char          detail[96];   /**< Ek bilgi veya hata açıklaması         */
} TestRecord_t;

/** Tüm test çalıştırmasının özeti */
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
 * @brief  Test ortamını başlatır (UART4 debug portu açılır).
 * @retval HAL_OK başarıysa, HAL_ERROR başlatma hatası.
 */
HAL_StatusTypeDef OpenAirScopeTest_Init(void);

/**
 * @brief  Tüm testleri sırayla çalıştırır ve özet raporu basar.
 * @retval Toplam başarısız test sayısı (0 = tümü geçti).
 */
uint8_t OpenAirScopeTest_Run(void);

/**
 * @brief  Son test çalıştırmasının özetini döndürür.
 */
TestSummary_t OpenAirScopeTest_GetSummary(void);

/* -------------------------------------------------------------------------
 * Bireysel Test Fonksiyonları
 * -------------------------------------------------------------------------*/
TestResult_t Test_GPIO(TestRecord_t *rec);
TestResult_t Test_ADC(TestRecord_t *rec);
TestResult_t Test_DAC(TestRecord_t *rec);
TestResult_t Test_I2C(TestRecord_t *rec);
TestResult_t Test_SPI(TestRecord_t *rec);
TestResult_t Test_FDCAN(TestRecord_t *rec);
TestResult_t Test_UART(TestRecord_t *rec);

/* -------------------------------------------------------------------------
 * Peripheral Handle'ları (extern — hal_msp / main ile paylaşım)
 * -------------------------------------------------------------------------*/
extern UART_HandleTypeDef  huart4;    /**< Debug / log UART               */
extern ADC_HandleTypeDef   hadc1;     /**< ADC1                           */
extern DAC_HandleTypeDef   hdac1;     /**< DAC1                           */
extern I2C_HandleTypeDef   hi2c1;     /**< I2C1                           */
extern SPI_HandleTypeDef   hspi1;     /**< SPI1                           */
extern FDCAN_HandleTypeDef hfdcan1;   /**< FDCAN1 (main.c'den)            */
extern UART_HandleTypeDef  huart3;    /**< USART3 loopback testi          */

#ifdef __cplusplus
}
#endif

#endif /* __OPENAIRSCOPE_TEST_H */
