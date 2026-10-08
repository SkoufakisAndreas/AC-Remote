/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdint.h> 
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
  ON,
  OFF
} state_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MODE_AUTO 0b0000
#define MODE_COOL 0b0001
#define MODE_DRY  0b0010
#define MODE_HEAT 0b0011
#define MODE_FAN  0b0100
#define MODE_OFF 0b0111
#define FAN_AUTO 0b0000
#define FAN_1 0b0100
#define FAN_2 0b0110
#define FAN_3 0b1000
#define FAN_4 0b1010
#define FAN_5 0b1100
#define BYTE_1 0b11110010
#define BYTE_2 0b00001101
#define BYTE_3 0b00000011
#define BYTE_4 0b11111100
#define BYTE_5 0b00000001
#define BYTE_8_NO_PURE 0b00000000
#define BYTE_8_PURE 0b00010000
#define BUFF_SIZE 9
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim11;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
volatile uint8_t rx_char;
volatile uint8_t transmitting = 0;
volatile uint16_t index = 0;
volatile uint8_t send_ack = 0;
volatile uint16_t tmr_intervals[294];
volatile uint8_t repeat_flag = 1;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM11_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
void create_instruction(uint8_t rx, uint8_t *buff);
void xor_cheksum(uint8_t *buff, int size);
void interrupt_intervals(uint16_t *intervals, uint8_t *command, int length);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
    uint8_t inital_msg[50];
    int msg_length;
    uint8_t command[9] = {0};
    state_t state = OFF; //Initialize with state off
    uint8_t on_flag = 0;
		
    
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM3_Init();
  MX_TIM11_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
	HAL_UART_Receive_IT(&huart2, (uint8_t*)&rx_char, 1);
	
  msg_length = sprintf(inital_msg, "System Initialized.... \r\n");
  HAL_UART_Transmit(&huart2, inital_msg, msg_length, 10);

  
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    
  
   //Check states
   if(state == OFF){
    //We fist need to open the AC
    if(rx_char != 'B') {
			__NOP();
		};
    if(rx_char == 'B'){
      on_flag = 1;
      state = ON; //Update the state
			
    }
   }

   if(state == ON) on_flag = 1;

   //START PROCESSING IF FLAG IS ON
   if(on_flag == 1 && transmitting == 0 && repeat_flag == 0){
          //Raise repeat flag to not re-transmit the message 
          repeat_flag = 1;

          //Use create instruction to create the bitstring
          create_instruction(rx_char, command);

          //Create the cheksum (last byte)
          xor_cheksum(command, BUFF_SIZE);

          //Fill the array with the intervals between each interrupt
          interrupt_intervals(tmr_intervals, command, BUFF_SIZE);

          //Also check if the command was off to drop the on flag
          if(rx_char == 'A') {
            on_flag = 0;
            state = OFF;
          }
          //Set step to zero and set transmitting to one so we start transmitting
          transmitting = 1;
          index = 1;
          //Iitialize the first interrupt duration and start the timer(TIM11)
          __HAL_TIM_SET_AUTORELOAD(&htim11, tmr_intervals[0]);
          HAL_TIM_Base_Start_IT(&htim11);
          //Start PWM
          HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
        
          //From now on we leave the interrupt service routine and the callback to run the pwm
   }
   
   if(send_ack){
       send_ack = 0;
       
		
       // send ack msg with \n in the end so readline from py script can recognize the end of the message
       uint8_t ack_msg[3] = {rx_char, '\r', '\n'};
       HAL_UART_Transmit(&huart2, ack_msg, 3, 10); 
		 
   }
   
   __WFI();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 420;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 140;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief TIM11 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM11_Init(void)
{

  /* USER CODE BEGIN TIM11_Init 0 */

  /* USER CODE END TIM11_Init 0 */

  /* USER CODE BEGIN TIM11_Init 1 */

  /* USER CODE END TIM11_Init 1 */
  htim11.Instance = TIM11;
  htim11.Init.Prescaler = 0;
  htim11.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim11.Init.Period = 65535;
  htim11.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim11.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim11) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM11_Init 2 */

  /* USER CODE END TIM11_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

//Receive UART callback
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
  //Check if intrp was fired from our uart channel
  if(huart -> Instance == USART2){
     if(rx_char == 'R' || rx_char == 'r') {
       NVIC_SystemReset(); //Reset if we receive r or R
     }
     repeat_flag = 0; //Drop the flag so we can initiate the transmittion
     HAL_UART_Receive_IT(&huart2, (uint8_t*)&rx_char, 1);
  }
}

/*COMMAND CONSTRUCT FUNCTION  (EXEPT SETTEPM)*/
void create_instruction(uint8_t rx, uint8_t *buff){
   
    uint8_t temp; //For tempup and tempdown
    uint8_t mode; //Capture mode when needed
    uint8_t fan_vol; //Capture fan volume when needed

    buff[0] = BYTE_1;
    buff[1] = BYTE_2;
    buff[2] = BYTE_3;
    buff[3] = BYTE_4;
    buff[4] = BYTE_5;
    
    //OFF CASE
    if(rx == 'A'){
      buff[6] = MODE_OFF;
    }
    //ON(CASE) Just to make it easier on will always be at 22/cool/fan_2/no pure
    if(rx == 'B'){
      //Just write the temp
      buff[5] = 0b01010000;
      //Load the FAN_VOL and left shift because FAN settings are the 4 MSBs
      buff[6] = FAN_2;
      buff[6] = buff[6] << 4;
      //load mode in the lowest 4 bits
      buff[6] = buff[6] | MODE_COOL;
      buff[7] = BYTE_8_NO_PURE;
    }
    //TEMPUP CASE
    if(rx == 'C'){
      //Save current mode
      mode = buff[5] & 0b00001111; 
      //4 rigth shift to take the temp
      temp = buff[5] >> 4; 
      //Still temp has 5 bits and the MSB is always 1 in the range we want located on LSB of BYTE 4
      temp = temp | 0b00010000;
      temp = temp + 1;
      //Reconstruct the byte
      buff[5] = temp << 5; //(5 because MSB needs to go to the previous byte . Its already there)
      buff [5] = buff[5] | mode;
    }
    //TEMPDOWN CASE
    if(rx == 'D'){
      //Save current mode
      mode = buff[5] & 0b00001111; 
      //4 rigth shift to take the temp
      temp = buff[5] >> 4; 
      //Still temp has 5 bits and the MSB is always 1 in the range we want located on LSB of BYTE 4
      temp = temp | 0b00010000;
      temp = temp - 1;
      //Reconstruct the byte
      buff[5] = temp << 5; //(5 because MSB needs to go to the previous byte . Its already there)
      buff [5] = buff[5] | mode;
    }
    //MODE SET 
    if(rx == 'E' || rx == 'F' || rx == 'G' || rx == 'H' || rx == 'I'){
      //Erase current mode
      buff[6] = buff[6] & 0b11110000;
      //load new mode
         switch(rx){
          case 'E': mode = MODE_AUTO; break;
          case 'F': mode = MODE_COOL; break;
          case 'G': mode = MODE_DRY; break;
          case 'H': mode = MODE_HEAT; break;
          case 'I': mode = MODE_FAN; break;
         }
         //Replace the mode 
      buff[6] = buff[6] | mode;
    }
    //FAN VOLUME SET
    if(rx == 'J' || rx == 'K' || rx == 'L' || rx == 'M' || rx == 'N' || rx == 'O'){

      //Save mode 
      mode = buff[6] & 0b00001111;
      //Load new fan vol 
      switch(rx){
          case 'J': fan_vol = FAN_AUTO; break;
          case 'K': fan_vol = FAN_1; break;
          case 'L': fan_vol = FAN_2; break;
          case 'M': fan_vol = FAN_3; break;
          case 'N': fan_vol = FAN_4; break;
          case 'O': fan_vol = FAN_5; break;
         }
         //Replace the fan vol
         buff[6] = fan_vol << 4;
         //Reload mode
         buff[6] = buff[6] | mode;
    }
    //PURE
    if(rx == 'P'){
      //Set bit 5 of byte 8 to 0
       buff[7] = buff[7] | 0b00010000;
    }
    //NO PURE
    if(rx == 'Q') buff[7] = buff[7] & 0b11101111;
   //CREATE THE LAST BYTE 
    xor_cheksum(buff, BUFF_SIZE);
}
/*CREATE THE LAST BYTE (CHEKSUM) XORING ALL THE OTHER BYTES*/
void xor_cheksum(uint8_t *buff, int size){

  uint8_t xor_result = buff[0];
  //Xor all the bytes
  for(int i = 1; i< size - 1; i++){

       xor_result = buff[i] ^ xor_result;
  }
  //Assign the result to BYTE 9
  buff[8] = xor_result;
}

void interrupt_intervals(uint16_t *intervals, uint8_t *command, int length){
   
  int repeat_offset = (length*2*8) + 4;
  //SET the header intervals
  intervals[0] = 4300;
  intervals[1] = 4300;
  intervals[ repeat_offset] = 4300;
  intervals[ repeat_offset + 1] = 4300;
  
  //Set the rest of the intervals
  for(int i = 0; i < length; i++){
    for(int j = 0; j < 8; j++){
      //If bit is one 580-1570
      if(((command[i] >> (7 - j)) & 0x01) == 0x01){
            intervals[2 + (8*i + j)*2] = 580;
            intervals[3 + (8*i + j)*2] = 1570;
            //Set the intervals for the repeated sequence
             intervals[2 + (8*i + j)*2  + repeat_offset] = 580;
            intervals[3 + (8*i + j)*2 + repeat_offset] = 1570;
      }
      //If bit is zero 580-490
      if(((command[i] >> (7 - j)) & 0x01) == 0x00){
            intervals[2 + (8*i + j)*2] = 580;
            intervals[3 + (8*i + j)*2] = 490;
            //Set the intervals for the repeated sequence
             intervals[2 + (8*i + j)*2  + repeat_offset] = 580;
            intervals[3 + (8*i + j)*2 + repeat_offset] = 490;
      }
    }
  }
  //Set the repeat pulse intervals
  intervals[repeat_offset - 2] = 580;
  intervals[repeat_offset - 1] = 7400; 
}
//Change apporeach and enter those in tim11calback sthn isr toy dhladh

//TIM11 (timer) Callback
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){

  if(htim -> Instance == TIM11){
    //When the entire message has been transmitted
    if(index >= 294){
      //Stop transmiting
      transmitting = 0;
      //Stop the timer 
      HAL_TIM_Base_Stop_IT(&htim11);
      //Reset index
      index = 0;
      //Set the send ack
      send_ack = 1;
      //Just for safety stop PWM
      HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_2);
    }
    //TRANSMITTING...
    else{
      //Update the TIM11 interrupt interval
      __HAL_TIM_SET_AUTORELOAD(&htim11, tmr_intervals[index]);
      //Evenpulses on
      if(index % 2 == 0 ){
        //We have even so pulse is high start pwm
        HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
      }
      //ODD pulses off
      if(index % 2 !=0 ){
        //We have even so pulse is high start pwm
        HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_2);
      }
    }
     index++;
  }
}



/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
