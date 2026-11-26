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

#include "bsp_key_irq.h"

#define FALING_TYPE 0
#define RISING_TYPE 1

osThreadId_t key_TaskHandle;
QueueHandle_t inter_key_queue  =  NULL;
QueueHandle_t       key_queue  =  NULL;

const osThreadAttr_t key_Task_attributes = {
  .name = "key_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};


/**
 * @brief Instantiates the bsp_led_handler_t target.
 * 
 * Steps:
 *  1.doing the specific operations
 *  
 * @param[in] void 
 * 
 * @return KEY_OK            : the key has been pressed
 *         KEY_ERRORTIMEOUT  : the key has been released
 * */
key_status_t key_scan(key_press_status_t * key_value)
{
    key_press_status_t key_status_value = KEY_NOT_PRESSED;
    
    if(HAL_GPIO_ReadPin(Key_GPIO_Port, Key_Pin) == GPIO_PIN_RESET)
    {
        key_status_value = KEY_PRESSED;
        *key_value = key_status_value;
        return KEY_OK;
    }

    *key_value = key_status_value;
    
    return KEY_ERRORTIMEOUT;//始终没有按键被按下，返回超时
}

/**
 * @brief this function will check the key status between long press and 
 *        shrot press.
 * 
 * Steps:
 *  1.doing the specific operations
 *  
 * @param[in] void 
 * 
 * @return KEY_SHORT_PRESSED : key has been pressed with short time.
 *         KEY_LONG_PRESSED  : key has been pressed with long time.
 * 
 * */
key_status_t key_scan_short_long_press(key_press_status_t *  key_value, 
                                       uint32_t      short_press_time)
{
    /**Variables */
    key_status_t       ret_key_status =          KEY_OK;    /* return value  */
    key_press_status_t key_value_temp = KEY_NOT_PRESSED;    /* temporary     */
    uint32_t             counter_tick =               0;    /* counter       */
    /**Variables */

    // 1. check if the key is pressed
    ret_key_status = key_scan(&key_value_temp);
    // TBD : critical section


    // 1.1 if the key is pressed, then check if it is short pressed
    if ( KEY_OK == ret_key_status )
    {
        if ( KEY_PRESSED == key_value_temp )
        {
            // get the timestamp of the first key press
            counter_tick = HAL_GetTick(); 

            // 1.1.1 check the short press time
            while( HAL_GetTick() <  counter_tick + short_press_time )
                ;
            
            // 1.1.2 now could check if the user pressed the key 
            ret_key_status = key_scan(&key_value_temp);
            
            if ( KEY_NOT_PRESSED == key_value_temp )
            {
                // 1.1.3 the key is pressed with short press
                *key_value = KEY_SHORT_PRESSED;
                return KEY_OK;
            } 
            else 
            {
                // 1.2 the key is pressed with long press
                *key_value = KEY_LONG_PRESSED;
                
                // 1.2.1 keep focuing on the status of key to avoid the short
                // press
                while( KEY_OK == key_scan(&key_value_temp) ) // key is pressed
                    ;
                
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
  /**     Variables (in task stack)            */
//  key_status_t  ket_ret                     =              KEY_OK;
  //key_press_status_t key_statues            =     KEY_NOT_PRESSED;
	key_press_event_t  key_press_event        =
	{
	 .edge_type= RASING,
   .trigger_tick =0
	};
	uint32_t           event_index            =                   0;
	uint32_t           first_trigger_tick     =                   0;
  uint32_t           short_press_time       =       SHORT_PRESS_TIME;
	/**     Variables (in task stack)            */
	
	/**     Variables (in OS heap)               */
  key_queue       = xQueueCreate( 10, sizeof( key_press_status_t ) );
	
	inter_key_queue = xQueueCreate( 10, sizeof( key_press_event_t ) );
	/**     Variables (in OS heap)               */

	// check if the queue has been created successfully
  if (NULL == key_queue || NULL ==inter_key_queue)
  {
      printf("key_queue created failed \r\n");
			return;
  } 
  else
      printf("key_queue created successfully \r\n");
  for(;;)
  {
		osDelay(100);
		printf("key_task_func running  at [%d] tick\r\n",
																		  HAL_GetTick());
		if( pdTRUE == xQueueReceive(                  inter_key_queue, 
                                              &(key_press_event ), 
                                              ( TickType_t ) 0 ) )
		{
			printf("inter_key_queue receive key event"
						 " at [%d] tick\r\n",   HAL_GetTick());
			if( RASING==key_press_event.edge_type&&
				  0     ==               event_index)
			{
				printf("key RASING fetched!,error\r\n");
			}
			if( FAILING==key_press_event.edge_type&&
				   0     ==            event_index )
			{
				printf("key FAILING fetched!\r\n");
				event_index +=1;
				first_trigger_tick=key_press_event.trigger_tick;
			}
			if( RASING==key_press_event.edge_type&&
				  1     ==               event_index)
			{
				printf("key RASING after the falling\r\n");
				if(key_press_event.trigger_tick-first_trigger_tick<10)
				{
					continue;
				}
				if((key_press_event.trigger_tick-first_trigger_tick)<short_press_time){
					key_press_status_t key_result = KEY_SHORT_PRESSED;
					if(pdTRUE == xQueueSendToFront(key_queue,&key_result,0))
					{
						printf("key_result send short press successfully at [%d] tick\r\n",
																										HAL_GetTick());
						event_index=0;
					}
					else
					{
						printf("key_result send failed at [%d] tick\r\n",
																							HAL_GetTick());
					}										
				}
				if((key_press_event.trigger_tick-first_trigger_tick)>short_press_time)
					{
						key_press_status_t key_result = KEY_LONG_PRESSED;
						if(pdTRUE == xQueueSendToFront(key_queue,&key_result,0))
					{
						printf("key_result send long press successfully at [%d] tick\r\n",
																										HAL_GetTick());
						event_index=0;
					}
					else
					{
						printf("key_result send failed at [%d] tick\r\n",
																							HAL_GetTick());
					}
			}
  }
  /* USER CODE END 5 */
}
}}
KEY_CALLBACK
{
	
	static uint32_t irq_type =  FALING_TYPE ;
	BaseType_t xHigherPrioritTaskWoken;
		if(FALING_TYPE==irq_type) {
		key_press_event_t key_press_event1=
	  {
		 .edge_type     =FAILING,
		 .trigger_tick  =HAL_GetTick()
    };
			if(pdTRUE == xQueueSendToFrontFromISR(inter_key_queue,&key_press_event1,&xHigherPrioritTaskWoken))
			{
				printf("key_result send faling_event successfully at [%d] tick\r\n",
																										HAL_GetTick());
		  }
		else
			{
				printf("key_result send failed at [%d] tick\r\n",
																							HAL_GetTick());
			}		
			irq_type=RISING_TYPE;
			
			GPIO_InitTypeDef GPIO_InitStruct = {0};
			GPIO_InitStruct.Pin = Key_Pin;
			GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
			GPIO_InitStruct.Pull = GPIO_PULLUP;
			HAL_GPIO_Init(Key_GPIO_Port, &GPIO_InitStruct);
}
		else if(RISING_TYPE==irq_type){
			key_press_event_t key_press_event2=
	{
		 .edge_type     =RASING,
		 .trigger_tick  =HAL_GetTick()
	};
			if(pdTRUE == xQueueSendToFrontFromISR(inter_key_queue,&key_press_event2,&xHigherPrioritTaskWoken))
			{
				printf("key_result send rasing_event successfully at [%d] tick\r\n",
																										HAL_GetTick());
		  }
		else
			{
				printf("key_result send failed at [%d] tick\r\n",
																							HAL_GetTick());}
				irq_type=FALING_TYPE;
			GPIO_InitTypeDef GPIO_InitStruct = {0};
			GPIO_InitStruct.Pin = Key_Pin;
			GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
			GPIO_InitStruct.Pull = GPIO_PULLUP;
			HAL_GPIO_Init(Key_GPIO_Port, &GPIO_InitStruct);
		}
}
