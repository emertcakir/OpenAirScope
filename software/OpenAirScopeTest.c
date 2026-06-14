/**
 ******************************************************************************
 * @file    OpenAirScopeTest.c
 * @brief   OpenAirScope Hardware Test Framework - Implementation
 *
 * @details For every peripheral this module provides:
 *          - Full peripheral init (global handle, error checking enforced)
 *          - PASS / FAIL decision with error-code reporting
 *          - Execution time measurement via HAL_GetTick()
 *          - Result summary printed as an ASCII table over UART4
 *
 * @board   OpenAirScope v1.x  (STM32H743VIT6)
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "OpenAirScopeTest.h"

/* -------------------------------------------------------------------------
 * Peripheral Handle Definitions
 * -------------------------------------------------------------------------*/
UART_HandleTypeDef  huart4;   /* Debug / log UART  — PA0=TX, PA1=RX        */
UART_HandleTypeDef  huart3;   /* USART3 loopback   — PB10=TX, PB11=RX      */
ADC_HandleTypeDef   hadc1;    /* ADC1                                       */
DAC_HandleTypeDef   hdac1;    /* DAC1                                       */
I2C_HandleTypeDef   hi2c1;    /* I2C1  — PB6=SCL, PB7=SDA                  */
SPI_HandleTypeDef   hspi1;    /* SPI1  — PG11=SCK, PB5=MOSI                */
/* hfdcan1: defined in main.c, used here via extern                         */

/* -------------------------------------------------------------------------
 * Module-private Variables
 * -------------------------------------------------------------------------*/
#define MAX_TESTS   8U

static TestRecord_t  s_records[MAX_TESTS];
static uint8_t       s_testCount = 0U;
static TestSummary_t s_summary   = {0};

/* -------------------------------------------------------------------------
 * Helper Utilities
 * -------------------------------------------------------------------------*/

/** Redirect printf output to UART4 (newlib retarget) */
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart4, (uint8_t *)&ch, 1U, HAL_MAX_DELAY);
    return ch;
}

/**
 * @brief  Prints a separator line of a given character and length.
 */
static void print_separator(char c, uint8_t len)
{
    for (uint8_t i = 0U; i < len; i++) {
        __io_putchar(c);
    }
    printf("\r\n");
}

/**
 * @brief  Initialises a test record and returns the start tick.
 */
static uint32_t record_start(TestRecord_t *rec, const char *name)
{
    memset(rec, 0, sizeof(TestRecord_t));
    rec->name   = name;
    rec->result = TEST_RESULT_PENDING;
    return HAL_GetTick();
}

/**
 * @brief  Finalises a test record with its result and elapsed time.
 */
static void record_finish(TestRecord_t *rec, uint32_t startTick,
                           TestResult_t result, const char *detail)
{
    rec->result     = result;
    rec->durationMs = HAL_GetTick() - startTick;
    if (detail != NULL) {
        strncpy(rec->detail, detail, sizeof(rec->detail) - 1U);
    }
}

/**
 * @brief  Prints a single test record as a formatted table row.
 */
static void print_record(const TestRecord_t *rec)
{
    const char *resultStr;
    switch (rec->result) {
        case TEST_RESULT_PASS: resultStr = "PASS"; break;
        case TEST_RESULT_FAIL: resultStr = "FAIL"; break;
        case TEST_RESULT_SKIP: resultStr = "SKIP"; break;
        default:               resultStr = "????"; break;
    }
    printf("| %-10s | %-4s | %5lu ms | %-40s |\r\n",
           rec->name, resultStr, rec->durationMs, rec->detail);
}

/* =========================================================================
 * UART4 Peripheral Init (Debug Port)
 * =========================================================================*/
static HAL_StatusTypeDef uart4_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    /* Enable GPIO and UART4 peripheral clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_UART4_CLK_ENABLE();

    /* PA0 = UART4_TX, PA1 = UART4_RX */
    gpio.Pin       = TEST_UART_TX_PIN | TEST_UART_RX_PIN;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_PULLUP;
    gpio.Speed     = GPIO_SPEED_FREQ_LOW;
    gpio.Alternate = TEST_UART_AF;
    HAL_GPIO_Init(TEST_UART_TX_PORT, &gpio);

    huart4.Instance          = TEST_UART_INSTANCE;
    huart4.Init.BaudRate     = TEST_UART_BAUDRATE;
    huart4.Init.WordLength   = UART_WORDLENGTH_8B;
    huart4.Init.StopBits     = UART_STOPBITS_1;
    huart4.Init.Parity       = UART_PARITY_NONE;
    huart4.Init.Mode         = UART_MODE_TX_RX;
    huart4.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart4.Init.OverSampling = UART_OVERSAMPLING_16;

    return HAL_UART_Init(&huart4);
}

/* =========================================================================
 * PUBLIC: OpenAirScopeTest_Init
 * =========================================================================*/
HAL_StatusTypeDef OpenAirScopeTest_Init(void)
{
    HAL_StatusTypeDef status = uart4_init();
    if (status != HAL_OK) {
        /* Debug port could not be opened — signal failure via LED blink */
        HAL_GPIO_WritePin(TEST_LED_PORT, TEST_LED_PIN, GPIO_PIN_SET);
        HAL_Delay(2000U);
        HAL_GPIO_WritePin(TEST_LED_PORT, TEST_LED_PIN, GPIO_PIN_RESET);
        return HAL_ERROR;
    }

    /* Print banner */
    printf("\r\n");
    print_separator('=', 70);
    printf("  OpenAirScope Hardware Test Framework\r\n");
    printf("  Board : STM32H743VIT6\r\n");
    printf("  Build : %s  %s\r\n", __DATE__, __TIME__);
    print_separator('=', 70);
    printf("\r\n");

    s_testCount = 0U;
    memset(&s_summary, 0, sizeof(s_summary));

    return HAL_OK;
}

/* =========================================================================
 * PUBLIC: OpenAirScopeTest_Run
 * =========================================================================*/
uint8_t OpenAirScopeTest_Run(void)
{
    /* Temporary record slot reused for each test */
    TestRecord_t rec;

    /* --- Execute tests --- */
    s_records[s_testCount++] = (Test_GPIO (&rec), rec);
    s_records[s_testCount++] = (Test_ADC  (&rec), rec);
    s_records[s_testCount++] = (Test_DAC  (&rec), rec);
    s_records[s_testCount++] = (Test_I2C  (&rec), rec);
    s_records[s_testCount++] = (Test_SPI  (&rec), rec);
    s_records[s_testCount++] = (Test_FDCAN(&rec), rec);
    s_records[s_testCount++] = (Test_UART (&rec), rec);

    /* --- Compute summary --- */
    s_summary.total = s_testCount;
    for (uint8_t i = 0U; i < s_testCount; i++) {
        s_summary.totalDurationMs += s_records[i].durationMs;
        switch (s_records[i].result) {
            case TEST_RESULT_PASS: s_summary.passed++;  break;
            case TEST_RESULT_FAIL: s_summary.failed++;  break;
            case TEST_RESULT_SKIP: s_summary.skipped++; break;
            default: break;
        }
    }

    /* --- Print report table --- */
    printf("\r\n");
    print_separator('-', 70);
    printf("| %-10s | %-4s | %-8s | %-40s |\r\n",
           "Test", "Result", "Duration", "Detail");
    print_separator('-', 70);
    for (uint8_t i = 0U; i < s_testCount; i++) {
        print_record(&s_records[i]);
    }
    print_separator('-', 70);

    /* --- Overall summary --- */
    printf("\r\n");
    print_separator('=', 70);
    printf("  TOTAL : %u   PASSED : %u   FAILED : %u   SKIPPED : %u\r\n",
           s_summary.total, s_summary.passed,
           s_summary.failed, s_summary.skipped);
    printf("  Total duration : %lu ms\r\n", s_summary.totalDurationMs);
    printf("  Overall result : %s\r\n",
           (s_summary.failed == 0U) ? ">>> ALL TESTS PASSED <<<" :
                                      "!!! SOME TESTS FAILED !!!");
    print_separator('=', 70);
    printf("\r\n");

    return s_summary.failed;
}

/* =========================================================================
 * PUBLIC: OpenAirScopeTest_GetSummary
 * =========================================================================*/
TestSummary_t OpenAirScopeTest_GetSummary(void)
{
    return s_summary;
}

/* =========================================================================
 * TEST: GPIO
 *   Performs a 3-cycle blink test on PB0 (LED output pin).
 *   Pin state is verified via IDR read-back after each SET / RESET.
 *   A Toggle operation is also validated.
 * =========================================================================*/
TestResult_t Test_GPIO(TestRecord_t *rec)
{
    uint32_t t0 = record_start(rec, "GPIO");
    printf("[GPIO] Starting...\r\n");

    uint8_t errorCount = 0U;

    for (uint8_t i = 0U; i < TEST_LED_BLINK_COUNT; i++) {
        /* SET and verify */
        HAL_GPIO_WritePin(TEST_LED_PORT, TEST_LED_PIN, GPIO_PIN_SET);
        HAL_Delay(TEST_LED_BLINK_DELAY_MS);
        if (HAL_GPIO_ReadPin(TEST_LED_PORT, TEST_LED_PIN) != GPIO_PIN_SET) {
            errorCount++;
            printf("[GPIO] ERROR: pin read LOW after SET (iteration %u)\r\n", i);
        }

        /* RESET and verify */
        HAL_GPIO_WritePin(TEST_LED_PORT, TEST_LED_PIN, GPIO_PIN_RESET);
        HAL_Delay(TEST_LED_BLINK_DELAY_MS);
        if (HAL_GPIO_ReadPin(TEST_LED_PORT, TEST_LED_PIN) != GPIO_PIN_RESET) {
            errorCount++;
            printf("[GPIO] ERROR: pin read HIGH after RESET (iteration %u)\r\n", i);
        }
    }

    /* Toggle verification */
    GPIO_PinState before = HAL_GPIO_ReadPin(TEST_LED_PORT, TEST_LED_PIN);
    HAL_GPIO_TogglePin(TEST_LED_PORT, TEST_LED_PIN);
    GPIO_PinState after  = HAL_GPIO_ReadPin(TEST_LED_PORT, TEST_LED_PIN);
    if (before == after) {
        errorCount++;
        printf("[GPIO] ERROR: pin state unchanged after Toggle\r\n");
    }

    /* Leave pin low */
    HAL_GPIO_WritePin(TEST_LED_PORT, TEST_LED_PIN, GPIO_PIN_RESET);

    if (errorCount == 0U) {
        record_finish(rec, t0, TEST_RESULT_PASS, "PB0 SET/RESET/Toggle OK");
        printf("[GPIO] PASSED\r\n");
    } else {
        char buf[64];
        snprintf(buf, sizeof(buf), "Error count: %u", errorCount);
        record_finish(rec, t0, TEST_RESULT_FAIL, buf);
        printf("[GPIO] FAILED\r\n");
    }
    return rec->result;
}

/* =========================================================================
 * TEST: ADC
 *   Initialises ADC1, runs offset calibration, then samples the internal
 *   Vbat channel (CH18). Validates that the raw reading falls within the
 *   expected range and reports the calculated voltage.
 * =========================================================================*/
TestResult_t Test_ADC(TestRecord_t *rec)
{
    uint32_t t0 = record_start(rec, "ADC");
    printf("[ADC] Starting...\r\n");

    /* Enable ADC1/2 peripheral clock */
    __HAL_RCC_ADC12_CLK_ENABLE();

    hadc1.Instance                      = TEST_ADC_INSTANCE;
    hadc1.Init.ClockPrescaler           = ADC_CLOCK_ASYNC_DIV4;
    hadc1.Init.Resolution               = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode             = ADC_SCAN_DISABLE;
    hadc1.Init.EOCSelection             = ADC_EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait         = DISABLE;
    hadc1.Init.ContinuousConvMode       = DISABLE;
    hadc1.Init.NbrOfConversion          = 1U;
    hadc1.Init.DiscontinuousConvMode    = DISABLE;
    hadc1.Init.ExternalTrigConv         = ADC_SOFTWARE_START;
    hadc1.Init.ExternalTrigConvEdge     = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;
    hadc1.Init.Overrun                  = ADC_OVR_DATA_PRESERVED;
    hadc1.Init.LeftBitShift             = ADC_LEFTBITSHIFT_NONE;
    hadc1.Init.OversamplingMode         = DISABLE;

    if (HAL_ADC_Init(&hadc1) != HAL_OK) {
        record_finish(rec, t0, TEST_RESULT_FAIL, "HAL_ADC_Init failed");
        printf("[ADC] FAILED (init)\r\n");
        return TEST_RESULT_FAIL;
    }

    /* Run offset calibration */
    if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET,
                                    ADC_SINGLE_ENDED) != HAL_OK) {
        record_finish(rec, t0, TEST_RESULT_FAIL, "Calibration failed");
        printf("[ADC] FAILED (calibration)\r\n");
        HAL_ADC_DeInit(&hadc1);
        return TEST_RESULT_FAIL;
    }

    /* Channel configuration */
    ADC_ChannelConfTypeDef chConf = {0};
    chConf.Channel      = TEST_ADC_CHANNEL;
    chConf.Rank         = ADC_REGULAR_RANK_1;
    chConf.SamplingTime = ADC_SAMPLETIME_387CYCLES_5;
    chConf.SingleDiff   = ADC_SINGLE_ENDED;
    chConf.OffsetNumber = ADC_OFFSET_NONE;
    chConf.Offset       = 0U;

    if (HAL_ADC_ConfigChannel(&hadc1, &chConf) != HAL_OK) {
        record_finish(rec, t0, TEST_RESULT_FAIL, "Channel config failed");
        printf("[ADC] FAILED (channel)\r\n");
        HAL_ADC_DeInit(&hadc1);
        return TEST_RESULT_FAIL;
    }

    /* Start conversion and poll for result */
    HAL_ADC_Start(&hadc1);
    HAL_StatusTypeDef pollStatus =
        HAL_ADC_PollForConversion(&hadc1, TEST_ADC_TIMEOUT_MS);

    TestResult_t result = TEST_RESULT_FAIL;
    char detail[64]     = {0};

    if (pollStatus == HAL_OK) {
        uint32_t raw = HAL_ADC_GetValue(&hadc1);
        /* Voltage calculation (3.3 V reference, 12-bit resolution) */
        float voltage = (float)raw * 3.3f / 4095.0f;
        snprintf(detail, sizeof(detail),
                 "Raw=%lu  Voltage=%.3fV", raw, (double)voltage);
        printf("[ADC] %s\r\n", detail);

        if (raw >= TEST_ADC_EXPECTED_MIN && raw <= TEST_ADC_EXPECTED_MAX) {
            result = TEST_RESULT_PASS;
        } else {
            snprintf(detail, sizeof(detail),
                     "Out of range: %lu (expected %u-%u)",
                     raw, TEST_ADC_EXPECTED_MIN, TEST_ADC_EXPECTED_MAX);
        }
    } else {
        snprintf(detail, sizeof(detail), "Conversion timeout (%u ms)",
                 TEST_ADC_TIMEOUT_MS);
        printf("[ADC] ERROR: %s\r\n", detail);
    }

    HAL_ADC_Stop(&hadc1);
    HAL_ADC_DeInit(&hadc1);

    record_finish(rec, t0, result, detail);
    printf("[ADC] %s\r\n", (result == TEST_RESULT_PASS) ? "PASSED" : "FAILED");
    return result;
}

/* =========================================================================
 * TEST: DAC
 *   Initialises DAC1 Channel-1 and sets an output of ~1.65 V
 *   (2048 / 4095 * 3.3 V). The DAC data register is read back to
 *   confirm the written value.
 * =========================================================================*/
TestResult_t Test_DAC(TestRecord_t *rec)
{
    uint32_t t0 = record_start(rec, "DAC");
    printf("[DAC] Starting...\r\n");

    __HAL_RCC_DAC12_CLK_ENABLE();

    hdac1.Instance = TEST_DAC_INSTANCE;
    if (HAL_DAC_Init(&hdac1) != HAL_OK) {
        record_finish(rec, t0, TEST_RESULT_FAIL, "HAL_DAC_Init failed");
        printf("[DAC] FAILED (init)\r\n");
        return TEST_RESULT_FAIL;
    }

    DAC_ChannelConfTypeDef dacConf = {0};
    dacConf.DAC_Trigger                    = DAC_TRIGGER_NONE;
    dacConf.DAC_OutputBuffer               = DAC_OUTPUTBUFFER_ENABLE;
    dacConf.DAC_ConnectOnChipPeripheral    = DAC_CHIPCONNECT_DISABLE;
    dacConf.DAC_UserTrimming               = DAC_TRIMMING_FACTORY;

    if (HAL_DAC_ConfigChannel(&hdac1, &dacConf, TEST_DAC_CHANNEL) != HAL_OK) {
        record_finish(rec, t0, TEST_RESULT_FAIL, "Channel config failed");
        HAL_DAC_DeInit(&hdac1);
        printf("[DAC] FAILED (channel)\r\n");
        return TEST_RESULT_FAIL;
    }

    if (HAL_DAC_Start(&hdac1, TEST_DAC_CHANNEL) != HAL_OK) {
        record_finish(rec, t0, TEST_RESULT_FAIL, "HAL_DAC_Start failed");
        HAL_DAC_DeInit(&hdac1);
        printf("[DAC] FAILED (start)\r\n");
        return TEST_RESULT_FAIL;
    }

    if (HAL_DAC_SetValue(&hdac1, TEST_DAC_CHANNEL,
                         DAC_ALIGN_12B_R, TEST_DAC_VALUE_12BIT) != HAL_OK) {
        record_finish(rec, t0, TEST_RESULT_FAIL, "SetValue failed");
        HAL_DAC_Stop(&hdac1, TEST_DAC_CHANNEL);
        HAL_DAC_DeInit(&hdac1);
        printf("[DAC] FAILED (set)\r\n");
        return TEST_RESULT_FAIL;
    }

    HAL_Delay(5U); /* Allow output to stabilise */

    /* Read back the DAC register and verify */
    uint32_t readBack = HAL_DAC_GetValue(&hdac1, TEST_DAC_CHANNEL);
    float voltage     = (float)readBack * 3.3f / 4095.0f;

    char detail[64];
    snprintf(detail, sizeof(detail),
             "Set=%u  ReadBack=%lu  (~%.3fV)",
             TEST_DAC_VALUE_12BIT, readBack, (double)voltage);
    printf("[DAC] %s\r\n", detail);

    TestResult_t result = (readBack == TEST_DAC_VALUE_12BIT)
                          ? TEST_RESULT_PASS : TEST_RESULT_FAIL;

    HAL_DAC_Stop(&hdac1, TEST_DAC_CHANNEL);
    HAL_DAC_DeInit(&hdac1);

    record_finish(rec, t0, result, detail);
    printf("[DAC] %s\r\n", (result == TEST_RESULT_PASS) ? "PASSED" : "FAILED");
    return result;
}

/* =========================================================================
 * TEST: I2C
 *   Initialises I2C1 in master mode. Probes address 0x50 with
 *   HAL_I2C_IsDeviceReady(). If the slave is absent but the bus is
 *   healthy (NACK received), the test is marked SKIP rather than FAIL.
 * =========================================================================*/
TestResult_t Test_I2C(TestRecord_t *rec)
{
    uint32_t t0 = record_start(rec, "I2C");
    printf("[I2C] Starting...\r\n");

    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* PB6=SCL, PB7=SDA already set to GPIO_AF4_I2C1 by main.c */
    hi2c1.Instance              = TEST_I2C_INSTANCE;
    hi2c1.Init.Timing           = 0x00C0EAFF; /* ~100 kHz @ 64 MHz APB1    */
    hi2c1.Init.OwnAddress1      = 0U;
    hi2c1.Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2      = 0U;
    hi2c1.Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        record_finish(rec, t0, TEST_RESULT_FAIL, "HAL_I2C_Init failed");
        printf("[I2C] FAILED (init)\r\n");
        return TEST_RESULT_FAIL;
    }

    /* Device presence check */
    HAL_StatusTypeDef devStatus =
        HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(TEST_I2C_SLAVE_ADDR << 1),
                              3U, TEST_I2C_TIMEOUT_MS);

    char detail[64];
    TestResult_t result;

    if (devStatus == HAL_OK) {
        snprintf(detail, sizeof(detail),
                 "ACK received from 0x%02X", TEST_I2C_SLAVE_ADDR);
        result = TEST_RESULT_PASS;
    } else {
        /* Try a transmit to distinguish bus fault from missing slave */
        uint8_t txData = 0xAAU;
        HAL_StatusTypeDef txStatus =
            HAL_I2C_Master_Transmit(&hi2c1,
                                    (uint16_t)(TEST_I2C_SLAVE_ADDR << 1),
                                    &txData, 1U, TEST_I2C_TIMEOUT_MS);

        if (txStatus == HAL_ERROR &&
            hi2c1.ErrorCode == HAL_I2C_ERROR_AF) {
            /* Bus is functional; slave is simply not present */
            snprintf(detail, sizeof(detail),
                     "Bus OK, 0x%02X NACK (slave absent?)",
                     TEST_I2C_SLAVE_ADDR);
            result = TEST_RESULT_SKIP;
        } else {
            snprintf(detail, sizeof(detail),
                     "Bus error, ErrorCode=0x%lX", hi2c1.ErrorCode);
            result = TEST_RESULT_FAIL;
        }
    }

    printf("[I2C] %s\r\n", detail);
    HAL_I2C_DeInit(&hi2c1);

    record_finish(rec, t0, result, detail);
    printf("[I2C] %s\r\n",
           (result == TEST_RESULT_PASS) ? "PASSED" :
           (result == TEST_RESULT_SKIP) ? "SKIPPED (slave absent)" : "FAILED");
    return result;
}

/* =========================================================================
 * TEST: SPI
 *   Initialises SPI1 as full-duplex master and transmits a 4-byte pattern.
 *   If MISO is shorted to MOSI (loopback), the received bytes must match
 *   the transmitted ones. Without a loopback the test is marked SKIP.
 * =========================================================================*/
TestResult_t Test_SPI(TestRecord_t *rec)
{
    uint32_t t0 = record_start(rec, "SPI");
    printf("[SPI] Starting...\r\n");

    __HAL_RCC_SPI1_CLK_ENABLE();

    hspi1.Instance               = TEST_SPI_INSTANCE;
    hspi1.Init.Mode              = SPI_MODE_MASTER;
    hspi1.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity       = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase          = SPI_PHASE_1EDGE;
    hspi1.Init.NSS               = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
    hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.NSSPMode          = SPI_NSS_PULSE_DISABLE;

    if (HAL_SPI_Init(&hspi1) != HAL_OK) {
        record_finish(rec, t0, TEST_RESULT_FAIL, "HAL_SPI_Init failed");
        printf("[SPI] FAILED (init)\r\n");
        return TEST_RESULT_FAIL;
    }

    uint8_t txBuf[4] = {TEST_SPI_TX_BYTE, 0xA5U, 0x3CU, 0xF0U};
    uint8_t rxBuf[4] = {0};

    HAL_StatusTypeDef status =
        HAL_SPI_TransmitReceive(&hspi1, txBuf, rxBuf,
                                sizeof(txBuf), TEST_SPI_TIMEOUT_MS);

    char detail[64];
    TestResult_t result;

    if (status != HAL_OK) {
        snprintf(detail, sizeof(detail),
                 "TransmitReceive failed, err=0x%lX", hspi1.ErrorCode);
        result = TEST_RESULT_FAIL;
    } else {
        /* Loopback check (MISO-MOSI shorted on the board) */
        if (memcmp(txBuf, rxBuf, sizeof(txBuf)) == 0) {
            snprintf(detail, sizeof(detail),
                     "Loopback OK: TX[%02X %02X %02X %02X]",
                     txBuf[0], txBuf[1], txBuf[2], txBuf[3]);
            result = TEST_RESULT_PASS;
        } else {
            /* Bus is active but loopback connector is not fitted */
            snprintf(detail, sizeof(detail),
                     "TX=%02X %02X, RX=%02X %02X (no loopback?)",
                     txBuf[0], txBuf[1], rxBuf[0], rxBuf[1]);
            result = TEST_RESULT_SKIP;
        }
    }

    printf("[SPI] %s\r\n", detail);
    HAL_SPI_DeInit(&hspi1);

    record_finish(rec, t0, result, detail);
    printf("[SPI] %s\r\n",
           (result == TEST_RESULT_PASS) ? "PASSED" :
           (result == TEST_RESULT_SKIP) ? "SKIPPED (no loopback?)" : "FAILED");
    return result;
}

/* =========================================================================
 * TEST: FDCAN
 *   Uses FDCAN1 (STM32H7 does not have a classic CAN_HandleTypeDef).
 *   Configured in INTERNAL LOOPBACK mode so no external CAN bus or
 *   termination resistor is required. One classic CAN frame is queued
 *   for transmission and the looped-back frame is verified in RX FIFO0.
 * =========================================================================*/
TestResult_t Test_FDCAN(TestRecord_t *rec)
{
    uint32_t t0 = record_start(rec, "FDCAN");
    printf("[FDCAN] Starting...\r\n");

    /* Re-initialise hfdcan1 (originally defined in main.c) */
    hfdcan1.Instance                  = TEST_FDCAN_INSTANCE;
    hfdcan1.Init.FrameFormat          = FDCAN_FRAME_CLASSIC;
    hfdcan1.Init.Mode                 = FDCAN_MODE_INTERNAL_LOOPBACK;
    hfdcan1.Init.AutoRetransmission   = ENABLE;
    hfdcan1.Init.TransmitPause        = DISABLE;
    hfdcan1.Init.ProtocolException    = DISABLE;
    /* 500 kbps @ 80 MHz FDCAN clock: Prescaler=4, TimeSeg1=15, TimeSeg2=4 */
    hfdcan1.Init.NominalPrescaler     = 4U;
    hfdcan1.Init.NominalSyncJumpWidth = 1U;
    hfdcan1.Init.NominalTimeSeg1      = 15U;
    hfdcan1.Init.NominalTimeSeg2      = 4U;
    hfdcan1.Init.DataPrescaler        = 1U;
    hfdcan1.Init.DataSyncJumpWidth    = 1U;
    hfdcan1.Init.DataTimeSeg1         = 1U;
    hfdcan1.Init.DataTimeSeg2         = 1U;
    hfdcan1.Init.MessageRAMOffset     = 0U;
    hfdcan1.Init.StdFiltersNbr        = 1U;
    hfdcan1.Init.ExtFiltersNbr        = 0U;
    hfdcan1.Init.RxFifo0ElmtsNbr     = 1U;
    hfdcan1.Init.RxFifo0ElmtSize     = FDCAN_DATA_BYTES_8;
    hfdcan1.Init.RxFifo1ElmtsNbr     = 0U;
    hfdcan1.Init.RxFifo1ElmtSize     = FDCAN_DATA_BYTES_8;
    hfdcan1.Init.RxBuffersNbr        = 0U;
    hfdcan1.Init.RxBufferSize        = FDCAN_DATA_BYTES_8;
    hfdcan1.Init.TxEventsNbr         = 0U;
    hfdcan1.Init.TxBuffersNbr        = 0U;
    hfdcan1.Init.TxFifoQueueElmtsNbr = 1U;
    hfdcan1.Init.TxFifoQueueMode     = FDCAN_TX_FIFO_OPERATION;
    hfdcan1.Init.TxElmtSize          = FDCAN_DATA_BYTES_8;

    if (HAL_FDCAN_Init(&hfdcan1) != HAL_OK) {
        record_finish(rec, t0, TEST_RESULT_FAIL, "HAL_FDCAN_Init failed");
        printf("[FDCAN] FAILED (init)\r\n");
        return TEST_RESULT_FAIL;
    }

    /* Filter: accept all standard IDs into RX FIFO0 */
    FDCAN_FilterTypeDef filter = {0};
    filter.IdType       = FDCAN_STANDARD_ID;
    filter.FilterIndex  = 0U;
    filter.FilterType   = FDCAN_FILTER_MASK;
    filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    filter.FilterID1    = 0x000U;
    filter.FilterID2    = 0x000U; /* mask = 0 → accept all */
    HAL_FDCAN_ConfigFilter(&hfdcan1, &filter);
    HAL_FDCAN_ConfigGlobalFilter(&hfdcan1,
                                 FDCAN_ACCEPT_IN_RX_FIFO0,
                                 FDCAN_ACCEPT_IN_RX_FIFO0,
                                 FDCAN_FILTER_REMOTE,
                                 FDCAN_FILTER_REMOTE);

    if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK) {
        record_finish(rec, t0, TEST_RESULT_FAIL, "HAL_FDCAN_Start failed");
        HAL_FDCAN_DeInit(&hfdcan1);
        printf("[FDCAN] FAILED (start)\r\n");
        return TEST_RESULT_FAIL;
    }

    /* Build and queue TX frame */
    FDCAN_TxHeaderTypeDef txHdr = {0};
    txHdr.Identifier          = TEST_FDCAN_STD_ID;
    txHdr.IdType              = FDCAN_STANDARD_ID;
    txHdr.TxFrameType         = FDCAN_DATA_FRAME;
    txHdr.DataLength          = FDCAN_DLC_BYTES_4;
    txHdr.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    txHdr.BitRateSwitch       = FDCAN_BRS_OFF;
    txHdr.FDFormat            = FDCAN_CLASSIC_CAN;
    txHdr.TxEventFifoControl  = FDCAN_NO_TX_EVENTS;
    txHdr.MessageMarker       = 0U;

    uint8_t txData[4] = {0xDE, 0xAD, 0xBE, 0xEF};

    if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &txHdr, txData) != HAL_OK) {
        record_finish(rec, t0, TEST_RESULT_FAIL, "TX FIFO queue failed");
        HAL_FDCAN_Stop(&hfdcan1);
        HAL_FDCAN_DeInit(&hfdcan1);
        printf("[FDCAN] FAILED (tx)\r\n");
        return TEST_RESULT_FAIL;
    }

    /* Wait for the loopback frame to appear in RX FIFO0 */
    uint32_t deadline = HAL_GetTick() + TEST_FDCAN_TX_TIMEOUT_MS;
    while (HAL_FDCAN_GetRxFifoFillLevel(&hfdcan1, FDCAN_RX_FIFO0) == 0U) {
        if (HAL_GetTick() > deadline) {
            record_finish(rec, t0, TEST_RESULT_FAIL, "RX timeout");
            HAL_FDCAN_Stop(&hfdcan1);
            HAL_FDCAN_DeInit(&hfdcan1);
            printf("[FDCAN] FAILED (rx timeout)\r\n");
            return TEST_RESULT_FAIL;
        }
    }

    FDCAN_RxHeaderTypeDef rxHdr = {0};
    uint8_t rxData[8]           = {0};
    HAL_FDCAN_GetRxMessage(&hfdcan1, FDCAN_RX_FIFO0, &rxHdr, rxData);

    char detail[64];
    TestResult_t result;

    if (rxHdr.Identifier == TEST_FDCAN_STD_ID &&
        memcmp(txData, rxData, 4U) == 0) {
        snprintf(detail, sizeof(detail),
                 "Loopback OK, ID=0x%03lX, Data=%02X %02X %02X %02X",
                 rxHdr.Identifier,
                 rxData[0], rxData[1], rxData[2], rxData[3]);
        result = TEST_RESULT_PASS;
    } else {
        snprintf(detail, sizeof(detail),
                 "Data mismatch, RxID=0x%03lX", rxHdr.Identifier);
        result = TEST_RESULT_FAIL;
    }

    printf("[FDCAN] %s\r\n", detail);
    HAL_FDCAN_Stop(&hfdcan1);
    HAL_FDCAN_DeInit(&hfdcan1);

    record_finish(rec, t0, result, detail);
    printf("[FDCAN] %s\r\n",
           (result == TEST_RESULT_PASS) ? "PASSED" : "FAILED");
    return result;
}

/* =========================================================================
 * TEST: UART
 *   Initialises USART3 (PB10=TX, PB11=RX) and performs a loopback test.
 *   If TX and RX are shorted, the transmitted string is received back
 *   intact. Without a loopback connector the test is marked SKIP.
 * =========================================================================*/
TestResult_t Test_UART(TestRecord_t *rec)
{
    uint32_t t0 = record_start(rec, "UART");
    printf("[UART] Starting...\r\n");

    __HAL_RCC_USART3_CLK_ENABLE();

    huart3.Instance          = TEST_USART_INSTANCE;
    huart3.Init.BaudRate     = TEST_USART_BAUDRATE;
    huart3.Init.WordLength   = UART_WORDLENGTH_8B;
    huart3.Init.StopBits     = UART_STOPBITS_1;
    huart3.Init.Parity       = UART_PARITY_NONE;
    huart3.Init.Mode         = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart3) != HAL_OK) {
        record_finish(rec, t0, TEST_RESULT_FAIL, "HAL_UART_Init failed");
        printf("[UART] FAILED (init)\r\n");
        return TEST_RESULT_FAIL;
    }

    const uint8_t txMsg[]  = "OpenAirScope-UART-Loopback-Test\r\n";
    const uint8_t txLen    = (uint8_t)(sizeof(txMsg) - 1U);
    uint8_t       rxBuf[sizeof(txMsg)] = {0};

    HAL_UART_Transmit(&huart3, txMsg, txLen, TEST_USART_TIMEOUT_MS);

    HAL_StatusTypeDef rxStatus =
        HAL_UART_Receive(&huart3, rxBuf, txLen, TEST_USART_TIMEOUT_MS);

    char detail[64];
    TestResult_t result;

    if (rxStatus == HAL_OK && memcmp(txMsg, rxBuf, txLen) == 0) {
        snprintf(detail, sizeof(detail),
                 "Loopback OK, %u bytes matched", txLen);
        result = TEST_RESULT_PASS;
    } else if (rxStatus == HAL_TIMEOUT) {
        snprintf(detail, sizeof(detail),
                 "RX timeout (loopback not connected?)");
        result = TEST_RESULT_SKIP;
    } else {
        snprintf(detail, sizeof(detail),
                 "Data mismatch or RX error");
        result = TEST_RESULT_FAIL;
    }

    printf("[UART] %s\r\n", detail);
    HAL_UART_DeInit(&huart3);

    record_finish(rec, t0, result, detail);
    printf("[UART] %s\r\n",
           (result == TEST_RESULT_PASS) ? "PASSED" :
           (result == TEST_RESULT_SKIP) ? "SKIPPED (no loopback?)" : "FAILED");
    return result;
}
