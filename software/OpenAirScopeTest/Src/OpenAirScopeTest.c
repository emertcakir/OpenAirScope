#include "OpenAirScopeTest.h"

// UART4 handle
UART_HandleTypeDef huart4;

// Redirect printf output to UART4
int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart4, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

// Initialize test environment
void OpenAirScopeTest_Init(void) {
    // UART4 init (PD0 = TX, PD1 = RX)
    huart4.Instance = UART4;
    huart4.Init.BaudRate = 115200;
    huart4.Init.WordLength = UART_WORDLENGTH_8B;
    huart4.Init.StopBits = UART_STOPBITS_1;
    huart4.Init.Parity = UART_PARITY_NONE;
    huart4.Init.Mode = UART_MODE_TX_RX;
    huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart4.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart4) != HAL_OK) {
        Error_Handler();
    }

    printf("\r\n===== OpenAirScopeTest Started =====\r\n");
}

// Run all tests
void OpenAirScopeTest_Run(void) {
    Test_GPIO();
    Test_ADC();
    Test_DAC();
    Test_I2C();
    Test_SPI();
    Test_CAN();
    Test_UART();

    printf("===== All tests completed =====\r\n");
}

// ---------------- TEST FUNCTIONS -----------------

// GPIO test
void Test_GPIO(void) {
    printf("[GPIO] Test started...\r\n");
    // Example: Toggle LED pin
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
    HAL_Delay(200);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
    HAL_Delay(200);
    printf("[GPIO] Success\r\n");
}

// ADC test
void Test_ADC(void) {
    printf("[ADC] Test started...\r\n");
    // Example: Sample from ADC1
    ADC_HandleTypeDef hadc1;
    ADC_ChannelConfTypeDef sConfig = {0};

    hadc1.Instance = ADC1;
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
        uint32_t value = HAL_ADC_GetValue(&hadc1);
        printf("[ADC] Measured value: %lu\r\n", value);
    }
    HAL_ADC_Stop(&hadc1);
}

// DAC test
void Test_DAC(void) {
    printf("[DAC] Test started...\r\n");
    DAC_HandleTypeDef hdac1;

    hdac1.Instance = DAC1;
    HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);
    HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 2048); // ~1.65V output
    printf("[DAC] Channel 1 output set to ~1.65V\r\n");
}

// I2C test
void Test_I2C(void) {
    printf("[I2C] Test started...\r\n");
    I2C_HandleTypeDef hi2c1;
    hi2c1.Instance = I2C1;
    uint8_t testData = 0xAA;
    HAL_I2C_Master_Transmit(&hi2c1, 0x50 << 1, &testData, 1, 100);
    printf("[I2C] Data sent to address 0x50\r\n");
}

// SPI test
void Test_SPI(void) {
    printf("[SPI] Test started...\r\n");
    SPI_HandleTypeDef hspi1;
    hspi1.Instance = SPI1;
    uint8_t txData = 0x55;
    uint8_t rxData = 0;
    HAL_SPI_TransmitReceive(&hspi1, &txData, &rxData, 1, 100);
    printf("[SPI] Sent: 0x%02X, Received: 0x%02X\r\n", txData, rxData);
}

// CAN test
void Test_CAN(void) {
    printf("[CAN] Test started...\r\n");
    CAN_HandleTypeDef hcan1;
    hcan1.Instance = CAN1;
    CAN_TxHeaderTypeDef txHeader;
    uint32_t txMailbox;
    uint8_t txData[2] = {0x12, 0x34};

    txHeader.StdId = 0x123;
    txHeader.DLC = 2;
    txHeader.IDE = CAN_ID_STD;
    txHeader.RTR = CAN_RTR_DATA;

    if (HAL_CAN_AddTxMessage(&hcan1, &txHeader, txData, &txMailbox) == HAL_OK) {
        printf("[CAN] Message sent (ID: 0x123)\r\n");
    }
}

// UART test
void Test_UART(void) {
    printf("[UART] Test started...\r\n");
    uint8_t txMsg[] = "UART4 loopback test\r\n";
    HAL_UART_Transmit(&huart4, txMsg, sizeof(txMsg)-1, 100);
    printf("[UART] Data transmitted\r\n");
}
