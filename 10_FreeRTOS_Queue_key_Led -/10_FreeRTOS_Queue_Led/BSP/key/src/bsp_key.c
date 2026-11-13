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
 * @author Jack | R&D Dept. | EternalChip ��оǶ��ʽ
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
 * @return KEY_OK: the key has been pressed
 *         KEY_ERRORTIMEOUT: the key has been timeout
 * */
key_status_t key_scan(key_press_status_t * key_value)
{
    
    uint32_t counter = 0;
    key_press_status_t key_status_value = KEY_NOT_PRESSED;
    
    while (counter < 1000){
        //2.���������PA0)�ĵ�ƽΪ�ͣ���˵�������������ˡ�
        //2.1  ������������£����Ͷ�Ӧ����Ϣ����
        if(HAL_GPIO_ReadPin(Key_GPIO_Port, Key_Pin) == GPIO_PIN_RESET)
        {
            key_status_value = KEY_PRESSED;
            *key_value = key_status_value;
            return KEY_OK;
        }
        //2.2 �������û�б�����
        counter++;
    }
    *key_value = key_status_value;
    
    return KEY_ERRORTIMEOUT;//ʼ��û�а��������£����س�ʱ
}

/**
 * @brief this function will check the key status between long press and short press
 * 
 * Steps:
 *  1.doing the specific operations
 *  
 * @param[in] void 
 * 
 * @return KEY_OK: the key has been pressed
 *         KEY_ERRORTIMEOUT: the key has been timeout
           KEY_SHORT_PRESSED: key has been pressed with short time
           KEY_LONG_PRESSED : key has been pressed with long time
 * */
 key_status_t key_scan_short_long_press(key_press_status_t* key_value,
                                            uint32_t short_press_time)
{
    key_press_status_t key_value_temp = KEY_NOT_PRESSED;
		uint32_t counter_tick=0;
    key_status_t ret_key_status=KEY_OK;
	
    ret_key_status = key_scan(&key_value_temp);
	
    if(KEY_OK==ret_key_status){
        if(key_value_temp==KEY_PRESSED){
            counter_tick = HAL_GetTick();//get the timestamp
            while(HAL_GetTick()<counter_tick+short_press_time)
            ;
            ret_key_status = key_scan(&key_value_temp);
            if(key_value_temp==KEY_NOT_PRESSED){
                * key_value= KEY_SHORT_PRESSED;
                return KEY_OK;
            }
            else{
                * key_value=KEY_LONG_PRESSED;
								while(KEY_OK==key_scan(&key_value_temp));
                return KEY_OK;
            }

        }
    }
    return ret_key_status;
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
    osDelay(1);
  }
  /* USER CODE END 5 */
}

