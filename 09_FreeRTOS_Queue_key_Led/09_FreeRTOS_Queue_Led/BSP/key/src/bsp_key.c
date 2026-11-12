/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file key.c
 * 
 * @par dependencies 
 * - bsp_key.h
 * - stdio.h
 * - stdint.h
 * 
 * @author Jack | R&D Dept. | EternalChip 立芯嵌入式
 * 
 * @brief Provide the HAL APIs of Key and corresponding opetions.
 * 
 * Processing flow:
 * 
 * call directly.
 * 
 * @version V1.0 2024-10-30
 *
 * @note 1 tab == 4 spaces!
 * 
 *****************************************************************************/

#include "bsp_key.h"
osThreadId_t key_TaskHandle;

QueueHandle_t key_queue;

const osThreadAttr_t key_Task_attributes = {
  .name = "key_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};


/**
 * @brief Instantiates the bsp_led_handler_t target.
 * 
 * Steps:
 *  1.doing the specific operations
 *  
 * @param[in] void 
 * 
 * @return led_handler_status_t : Status of the function.
 * 
 * */
key_status_t key_scan(key_press_status_t * key_value)
{
    
    uint32_t counter = 0;
    key_press_status_t key_status_value = KEY_NOT_PRESSED;
    
    while (counter < 1000){
        //2.如果按键（PA0)的电平为低，则说明按键被按下了。
        //2.1  如果按键被按下，则发送对应的消息队列
        if(HAL_GPIO_ReadPin(Key_GPIO_Port, Key_Pin) == GPIO_PIN_RESET)
        {
            key_status_value = KEY_PRESSED;
            *key_value = key_status_value;
            return KEY_OK;
        }
        //2.2 如果按键没有被按下
        counter++;
    }
    *key_value = key_status_value;
    
    return KEY_ERRORTIMEOUT;//始终没有按键被按下，返回超时
}

/**
 * @brief key_task_func for thread to run.
 * 
 * Steps:
 *  1.doing the specific operations
 *  
 * @param[in] void 
 * 
 * @return led_handler_status_t : Status of the function.
 * 
 * */
void key_task_func(void *argument)
{
    
  key_status_t  ket_ret          =          KEY_OK;
  key_press_status_t key_statues = KEY_NOT_PRESSED;
  key_queue = xQueueCreate( 10, sizeof( uint32_t ) );
  uint32_t counter_tick = 0;
  if (NULL == key_queue )
  {
      printf("key_queue created failed \r\n");
  } 
  else
  {
      printf("key_queue created successfully \r\n");
  }
  for(;;)
  {
    counter_tick++;
    //printf("Hellow Key thread\r\n");
    
    ket_ret = key_scan(&key_statues);
    
    if( KEY_OK == ket_ret)
    {
        if ( KEY_PRESSED == key_statues )
        {
            printf("Key_Pressed\r\n");
            if ( pdTRUE == xQueueSendToFront(key_queue,&counter_tick,0))
            {
                printf("send successfully\r\n");
            }
        }
    }
    if( KEY_OK != ket_ret)
    {
        printf("Key_not_Pressed\r\n");
    }        
    osDelay(100);
  }
  /* USER CODE END 5 */
}

