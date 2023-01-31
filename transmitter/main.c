/**
 * Copyright (c) 2014 - 2021, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/** @file
*
* @defgroup nrf_dev_button_radio_tx_example_main main.c
* @{
* @ingroup nrf_dev_button_radio_tx_example
*
* @brief Radio Transceiver Example Application main file.
*
* This file contains the source code for a sample application using the NRF_RADIO peripheral.
*
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "radio_config.h"
#include "nrf_gpio.h"
#include "app_timer.h"
#include "boards.h"
#include "bsp.h"
#include "nordic_common.h"

#include "nrf.h"
#include "nrf_error.h"
#include "nrf_delay.h"

#include "nrf_soc.h"
#include "nrf_fstorage.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "mem_manager.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_drv_gpiote.h"
#include "nrf_drv_twi.h"

#ifdef SOFTDEVICE_PRESENT
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_fstorage_sd.h"
#else
#include "nrf_drv_clock.h"
#include "nrf_fstorage_nvmc.h"
#endif

//FreeRTOS
//#include "FreeRTOS.h"
//#include "task.h"

//Boards Hardware Defin
#include "pin_config.h"

//Machine Learning
#include "layer_structure.h"
#include "model.h"
#include "model_struct.h"

//Peripheral
#include "mpu9250.h"
#include "bm1383aglv.h"

/**< Packet to transmit. */
static uint32_t  packet;                  
static uint32_t  ack_packet;
static const nrf_drv_twi_t m_twim = NRF_DRV_TWI_INSTANCE(0);

// global variables
volatile bool start_check_lid = 0, start_catch_baro = 0, start_send_data = 0;
volatile bool acc_timeout = 0, baro_timeout = 0;
static uint32_t m_data = 0xA0A0A0A0;
static uint32_t data;
float acc_gyro_data[6] = {0};
static int addr_offset = 0;
static uint8_t send_buff[50];


typedef union{
    float f;
    uint32_t u32;
}float_caster;

uint32_t read_addr = 0x3e000;
uint32_t write_addr = 0x3e000;
uint32_t read_data[300]; 

//'accelerometer' (MPU9250) is used to check if the lid is open. 
//'barometer'  (BM1383AGLV) is used to check if the inhaler is correctly used.
model *acc_model = NULL;
model *baro_model = NULL;
sliding_window sld;

// function prototype
static void fstorage_evt_handler(nrf_fstorage_evt_t *p_evt);

//define Timer
APP_TIMER_DEF(acc_timeout_timer);
APP_TIMER_DEF(baro_timeout_timer);

//define Flash
NRF_FSTORAGE_DEF(nrf_fstorage_t my_instance) =
    {
        .evt_handler = fstorage_evt_handler,
        .start_addr = 0x3e000,
        .end_addr = 0x3ffff,
};

static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

/**@brief Function for initializing power management.
 */
static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}

static void timers_init(void)
{
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
}

void flash_init(void)
{
    ret_code_t rc = nrf_fstorage_init(
        &my_instance,     /* Your fstorage instance, previously defined. */
        &nrf_fstorage_nvmc, /* Name of the backend. */
        NULL              /* Optional parameter, backend-dependant. */
    );
    if (rc != NRF_SUCCESS)
        NRF_LOG_INFO("%d error flash init fail!!", rc);
    
}

/**@brief   Sleep until an event is received. */
static void power_manage(void)
{
#ifdef SOFTDEVICE_PRESENT
    (void)sd_app_evt_wait();
#else
    __WFE();
#endif
}

void wait_for_flash_ready(nrf_fstorage_t const *p_fstorage)
{
    /* While fstorage is busy, sleep and wait for an event. */
    while (nrf_fstorage_is_busy(p_fstorage))
    {
        power_manage();
    }
}

static void writeToFlash(uint32_t addr, void const *src, uint32_t len)
{
    ret_code_t rc = nrf_fstorage_write(
        &my_instance, /* The instance to use. */
        addr,         /* The address in flash where to store the data. */
        src,          /* A pointer to the data. */
        len,          /* Length of the data, in bytes. */
        NULL          /* Optional parameter, backend-dependent. */
    );
    APP_ERROR_CHECK(rc);
    wait_for_flash_ready(&my_instance);
}

static void readFromFlash(uint32_t addr, void *dst, uint32_t len)
{
    ret_code_t rc = nrf_fstorage_read(
        &my_instance, /* The instance to use. */
        addr,         /* The address in flash where to read data from. */
        dst,          /* A buffer to copy the data into. */
        len           /* Lenght of the data, in bytes. */
    );
    APP_ERROR_CHECK(rc);
}

static void eraseFlash(uint32_t n_pages)
{
    // for nRF52840, 4096 bytes/page
    ret_code_t rc = nrf_fstorage_erase(
        &my_instance,           /* The instance to use. */
        my_instance.start_addr, /* The address of the flash pages to erase. */
        n_pages,                /* The number of pages to erase. */
        NULL                    /* Optional parameter, backend-dependent. */
    );
    APP_ERROR_CHECK(rc);
}

// flash related functions
static void fstorage_evt_handler(nrf_fstorage_evt_t *p_evt)
{
    if (p_evt->result != NRF_SUCCESS)
    {
        NRF_LOG_INFO("--> Event received: ERROR while executing an fstorage operation.");
        return;
    }

    switch (p_evt->id)
    {
    case NRF_FSTORAGE_EVT_WRITE_RESULT:
    {
        /*
        NRF_LOG_INFO("--> Event received: wrote %d bytes at address 0x%x.",
                     p_evt->len, p_evt->addr);
        */
        //NRF_LOG_INFO("Write to addr 0x%x",p_evt->addr);
    }
    break;

    case NRF_FSTORAGE_EVT_ERASE_RESULT:
    {
        NRF_LOG_INFO("--> Event received: erased %d page from address 0x%x.",
                     p_evt->len, p_evt->addr);
    }
    break;

    default:
        break;
    }
}
void twi_init(void)
{
    ret_code_t err_code;

    const nrf_drv_twi_config_t twi_config = {
        .scl = SCL_PIN,
        .sda = SDA_PIN,
        .frequency = NRF_DRV_TWI_FREQ_400K,
        .interrupt_priority = APP_IRQ_PRIORITY_LOW,
        .clear_bus_init = false};

    err_code = nrf_drv_twi_init(&m_twim, &twi_config, NULL, NULL);
    APP_ERROR_CHECK(err_code);

    nrf_drv_twi_enable(&m_twim);
}

// timer handlers
static void acc_timeout_handler(void *p_context)
{
    NRF_LOG_INFO("acc_timeout_handler");
    acc_timeout = 1;
}

static void baro_timeout_handler(void *p_context)
{
    NRF_LOG_INFO("baro_timeout_handler");
    baro_timeout = 1;
    start_catch_baro = 0;
}

static void create_timers()
{
    ret_code_t err_code;
    // this timer is used to check if the lid is open, when timeout occurred, stop testing.
    err_code = app_timer_create(&acc_timeout_timer,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                acc_timeout_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&baro_timeout_timer,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                baro_timeout_handler);
    APP_ERROR_CHECK(err_code);


    // err_code = app_timer_create(&m_rtc_timer,
    //                             APP_TIMER_MODE_REPEATED,
    //                             rtc_handler);
    // APP_ERROR_CHECK(err_code);
}

void in_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    uint8_t rc = 1;
    uint16_t length;

    //Voltage High to Low
    if (action == NRF_GPIOTE_POLARITY_HITOLO)
    {
        switch (pin)
        {
            //start catch BM1383 value
        case DEF_BUTTON_1:
            start_catch_baro = 1;
            app_timer_start(baro_timeout_timer, APP_TIMER_TICKS(10000), NULL);
            break;
            //store data in flash 
        case DEF_BUTTON_2:
            //read from flash and send
            start_send_data = 1;
            break;
  
        default:
            NRF_LOG_INFO("Something wrong...");
            break;
        }
    }
    //Voltage Low to High
    else if (action == NRF_GPIOTE_POLARITY_LOTOHI)
    {
        switch (pin)
        {
          case ACCEL_INT_PIN:
              if (!start_check_lid)
              {
                  mpu9250_reset(&m_twim);
                  mpu9250_total_init(&m_twim);
                  app_timer_start(acc_timeout_timer, APP_TIMER_TICKS(10000), NULL);
                  start_check_lid = 1;
                  NRF_LOG_INFO("In ACCEL_INT_PIN");
              }
              break;
          default:
              break;
        }
    }
}

void gpio_init(void)
{
    nrf_drv_gpiote_init();
    nrf_drv_gpiote_in_config_t in_config  = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(false); // Set high-to-low event
    nrf_drv_gpiote_in_config_t in_config1 = NRFX_GPIOTE_CONFIG_IN_SENSE_LOTOHI(true);
    nrfx_gpiote_out_config_t out_config = NRFX_GPIOTE_CONFIG_OUT_SIMPLE(NRF_GPIOTE_INITIAL_VALUE_HIGH);
    in_config.pull  = NRF_GPIO_PIN_PULLUP;       // gpio initial pull-up
    in_config1.pull = NRF_GPIO_PIN_PULLUP;

    nrf_drv_gpiote_in_init(DEF_BUTTON_1, &in_config, in_pin_handler); // initial gpio pin
    nrf_drv_gpiote_in_event_enable(DEF_BUTTON_1, true);               // enable gpio pin interrupt
    nrf_drv_gpiote_in_init(DEF_BUTTON_2, &in_config, in_pin_handler); // initial gpio pin
    nrf_drv_gpiote_in_event_enable(DEF_BUTTON_2, true);               // enable gpio pin interrupt
    nrf_drv_gpiote_in_init(ACCEL_INT_PIN, &in_config1, in_pin_handler);
    nrf_drv_gpiote_in_event_enable(ACCEL_INT_PIN, true);
   
    nrf_drv_gpiote_out_init(GPIO_BM1383_OUTPUT_PIN, &out_config);
}

/**@brief Function for sending packet.
 */


static uint32_t read_ack_packet()
{
    uint32_t result = 0;

    NRF_RADIO->EVENTS_READY = 0U;
    // Enable radio and wait for ready
    NRF_RADIO->TASKS_RXEN = 1U;

    while (NRF_RADIO->EVENTS_READY == 0U)
    {
        // wait
    }
    NRF_RADIO->EVENTS_END = 0U;
    // Start listening and wait for address received event
    NRF_RADIO->TASKS_START = 1U;

    // Wait for end of packet or buttons state changed
    while (NRF_RADIO->EVENTS_END == 0U)
    {
        // wait
    }

    if (NRF_RADIO->CRCSTATUS == 1U)
    {
        result = packet;
    }

    NRF_RADIO->EVENTS_DISABLED = 0U;
    // Disable radio
    NRF_RADIO->TASKS_DISABLE = 1U;

    while (NRF_RADIO->EVENTS_DISABLED == 0U)
    {
        // wait
    }
    return result;
}

void send_packet()
{
    // send the packet:
    NRF_RADIO->EVENTS_READY = 0U;
    NRF_RADIO->TASKS_TXEN   = 1;

    while (NRF_RADIO->EVENTS_READY == 0U)
    {
        // wait
    }
    NRF_RADIO->EVENTS_END  = 0U;
    NRF_RADIO->TASKS_START = 1U;

    while (NRF_RADIO->EVENTS_END == 0U)
    {
        // wait
    }

    NRF_LOG_INFO("packet sent");
    //APP_ERROR_CHECK(err_code);

    NRF_RADIO->EVENTS_DISABLED = 0U;
    // Disable radio
    NRF_RADIO->TASKS_DISABLE = 1U;

    while (NRF_RADIO->EVENTS_DISABLED == 0U)
    {
        // wait
    }
}

/**@brief Function for initialization oscillators.
 */
void clock_initialization()
{
    /* Start 16 MHz crystal oscillator */
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART    = 1;

    /* Wait for the external oscillator to start up */
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0)
    {
        // Do nothing.
    }

    /* Start low frequency crystal oscillator for app_timer(used by bsp)*/
    NRF_CLOCK->LFCLKSRC            = (CLOCK_LFCLKSRC_SRC_Xtal << CLOCK_LFCLKSRC_SRC_Pos);
    NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_LFCLKSTART    = 1;

    while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0)
    {
        // Do nothing.
    }
}

static float inline standardize(float ori_value, float std_value, int scaler)
{
    return (ori_value - std_value) / scaler;
}

static int inline getMaxIndex(float *array, int size)
{
    float max = array[0];
    int maxId = 0;
    for (int i = 1; i < size; i++)
    {
        if (max < array[i])
        {
            max = array[i];
            maxId = i;
        }
    }
    return maxId;
}

static void initModelParams(void)
{
    // 'accelerometer' model(MPU9250) is used to check if the lid is open, and 'barometer' model(BM1383AGLV) is used to check if the inhaler is correctly used.
    acc_model = (model *)calloc(1, sizeof(model));
    acc_model->input = (float *)calloc(152 * 6, sizeof(float));
    acc_model->inst = load_acc_model();
    acc_model->input_dim[0] = 152;
    acc_model->input_dim[1] = 6;

    baro_model = (model *)calloc(1, sizeof(model));
    baro_model->input = (float *)calloc(150 * 2, sizeof(float));
    baro_model->inst = load_inhaler_model();
    baro_model->input_dim[0] = 150;
    baro_model->input_dim[1] = 2;

    sld.stride = 10;
}

static void checkLidOpen(void)
{
    float *current_ptr = acc_model->input;
    /////////// check if time out ///////////
    for (int i = 0; !acc_timeout; i++)
    {
        mpu9250_accel_three_axis_raw(&m_twim, acc_gyro_data + 0, acc_gyro_data + 1, acc_gyro_data + 2);
        mpu9250_gyro_three_axis(&m_twim, acc_gyro_data + 3, acc_gyro_data + 4, acc_gyro_data + 5); // gyroscope data has been prprocessed! (divided by 250)

        // for (int ii = 0; ii < 6; ii++)
        //     printf("%2d: %.6f ", ii, acc_gyro_data[ii]);
        // printf("\r\n");
        // nrf_delay_ms(5);


        // Implement 'SLIDING WINDOW' technique......
        // I think this is enough...... :> (by young boy)
        memcpy(current_ptr,
               acc_gyro_data,
               sizeof(acc_gyro_data));
        current_ptr += acc_model->input_dim[1];

        if ((i + 1) == acc_model->input_dim[0]) // if collect enough data
        {
            acc_model->output = acc_predict(acc_model->input, acc_model->inst);
            //      SCHEMATIC DIAGRAM
            //     ******|\data\*******
            // ==> *******\data\|******
            //            (copy)
            // float tmp[142 * 6] = {0};
            // memcpy(tmp, acc_model->input + 10 * 6, 142 * 6);
            // memcpy(acc_model->input, tmp, 142 * 6);
            memmove(acc_model->input,
                    acc_model->input + sld.stride * acc_model->input_dim[1],
                    (acc_model->input_dim[0] - sld.stride) * acc_model->input_dim[1]);
            
            NRF_LOG_INFO("predict: " NRF_LOG_FLOAT_MARKER "and memmove!", NRF_LOG_FLOAT(acc_model->output[1]));
            i = acc_model->input_dim[0] - sld.stride - 1;
            current_ptr =
                acc_model->input + (acc_model->input_dim[0] - sld.stride) * acc_model->input_dim[1];

            if (acc_model->output[1] >= 0.6)
            {
                start_check_lid = 0;
                start_catch_baro = 1;
                nrf_free(acc_model->output);

                // turn on BM1383!
                nrf_drv_gpiote_out_set(GPIO_BM1383_OUTPUT_PIN); // power for BM1383AGLV
                BM1383AGLV_init(&m_twim, BM1383AGLV_DEVICE_ADDRESS);
                break;
            }
            nrf_free(acc_model->output);
        }
    }
    /////////// check if time out end ///////////
    start_check_lid = 0;
    NRF_LOG_INFO("Don't need to check timeout!\r\n");
}


/**@brief   Helper function to obtain the last address on the last page of the on-chip flash that
 *          can be used to write user data.
 */
static uint32_t nrf5_flash_end_addr_get()
{
    uint32_t const bootloader_addr = BOOTLOADER_ADDRESS;
    uint32_t const page_sz         = NRF_FICR->CODEPAGESIZE;
    uint32_t const code_sz         = NRF_FICR->CODESIZE;

    return (bootloader_addr != 0xFFFFFFFF ?
            bootloader_addr : (code_sz * page_sz));
}

static void send_barodata(void)
{  
    
    uint32_t received = 0;
    for(int i = 0 ; i < 300 ; i++){
        read_addr += 4;
        if (read_addr == write_addr){
            start_send_data = 0;
            break;
        }
        readFromFlash(read_addr, read_data+i, 4);
        NRF_LOG_INFO("0x%x",read_data[i]);

        
        packet = read_data[i];
        send_packet();
        ack_packet = read_ack_packet();
        NRF_LOG_INFO("ack :%d",ack_packet);
        

        NRF_LOG_FLUSH();
        nrf_delay_ms(50);
    }
    NRF_LOG_INFO("send_data complete");
    NRF_LOG_FLUSH();
    

}

static void catch_barodata(void)
{
    bool  first_time = true;
    float first_pressure = 0, first_temperature = 0;
    float pressure = 0, temperature = 0;

    //Erase flash first;
    eraseFlash(1);
    float_caster float_temperature;
    float_caster float_pressure;

    
    float *current_ptr = baro_model->input;
    for (int i = 0; !baro_timeout; i++)
    {
        
        ret_code_t rc = BM1383AGLV_getval(&m_twim, &pressure, &temperature);
        if (rc != NRF_SUCCESS)
        {
            NRF_LOG_INFO("read BM1383 error!\r\n");
        }
        
        if (first_time)
        {
            first_pressure = pressure;
            first_temperature = temperature;
            first_time = false;
        }

        //write to flash
        //float_pressure.u32 = i;
        float_pressure.f = pressure;
        float_temperature.f = temperature; 
        writeToFlash(write_addr, &float_pressure.u32, 4);
        write_addr+=4;
        NRF_LOG_INFO("pressure: " NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(pressure));


        //Feed to Model
        //standardize
        pressure = standardize(pressure, first_pressure, 40);
        temperature = standardize(temperature, first_temperature, 5);
        
        //copy to baro_model_input
        memcpy(current_ptr, &pressure, sizeof(pressure));
        memcpy(current_ptr + 1, &temperature, sizeof(temperature));
        current_ptr += baro_model->input_dim[1];
        
        // if collect enough data
        if ((i + 1) == baro_model->input_dim[0]) 
        {
            baro_model->output = inhaler_predict(baro_model->input, baro_model->inst);
            // SCHEMATIC DIAGRAM
            // ******|\data\*******
            memmove(baro_model->input,
                    baro_model->input + sld.stride * baro_model->input_dim[1],
                    (baro_model->input_dim[0] - sld.stride) * baro_model->input_dim[1]);

            i = baro_model->input_dim[0] - sld.stride - 1;
            current_ptr = baro_model->input +
                          (baro_model->input_dim[0] - sld.stride) * baro_model->input_dim[1];

            int maxId = getMaxIndex(baro_model->output, 4);
            //NRF_LOG_INFO("maxId: %d / value: "NRF_LOG_FLOAT_MARKER "\r\n", maxId, NRF_LOG_FLOAT(baro_model->output[maxId]));

            /*
            float_caster myfloat;
            myfloat.f = baro_model->output[maxId];
            packet = myfloat.u32;
            
            NRF_LOG_INFO("packet %x",packet);
            send_packet();
            */
             
            switch (maxId)
            {
            case 0:
                sprintf(send_buff, "normal!! %f", baro_model->output[maxId]);
                break;
            case 1:
                sprintf(send_buff, "strong!! %f", baro_model->output[maxId]);
                break;
            case 2:
                sprintf(send_buff, "weak!! %f", baro_model->output[maxId]);
                break;
            case 3:
                sprintf(send_buff, "ex_bf_in!! %f", baro_model->output[maxId]);
                break;
            default:
                break;
            }
            
            //NRF_LOG_INFO("strlen: %d sendBuff: %s\r\n", strlen(send_buff), send_buff);
            nrf_free(baro_model->output);
            NRF_LOG_FLUSH();
            nrf_delay_ms(10);
      
        }
    }
    
    
}



/*
static void vTaskCode1()
{
  for(;;){
    NRF_LOG_INFO("%u",(uint8_t)uxTaskGetStackHighWaterMark(NULL));
    NRF_LOG_INFO("HI");
    size_t xFreeStackSpace = xPortGetFreeHeapSize();
    NRF_LOG_INFO("xFreeStackSpace %u",xFreeStackSpace);
    NRF_LOG_FLUSH();
    vTaskDelay(500);
  }
} 

static void vTaskCode2()
{
  for(;;){
    NRF_LOG_INFO("%u",(uint8_t)uxTaskGetStackHighWaterMark(NULL));
    NRF_LOG_INFO("Ho");
    NRF_LOG_FLUSH();
    vTaskDelay(2000);
  } 
}
*/

/**
 * @brief Function for application main entry.
 * @return 0. int return type required by ANSI/ISO standard.
 */

int main(void)
{
    uint32_t err_code = NRF_SUCCESS;
    clock_initialization();
    log_init();
    timers_init();
    
    power_management_init();
    twi_init();
    gpio_init();
    
    nrf_mem_init();
    flash_init();
    
    nrf_delay_ms(10);
    NRF_LOG_INFO("Setup stage finished!");
    
    initModelParams();
    create_timers();
    mpu9250_accel_init_wom_mode(&m_twim);
    mpu9250_total_init(&m_twim);
    NRF_LOG_INFO("Inhaler project start!");

    //Power for BM1383AGLV
    nrf_drv_gpiote_out_set(GPIO_BM1383_OUTPUT_PIN); 
    BM1383AGLV_init(&m_twim, BM1383AGLV_DEVICE_ADDRESS);
    
    /*
    err_code = bsp_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS, bsp_evt_handler);
    APP_ERROR_CHECK(err_code);
    */
    /*
    eraseFlash(1);
    writeToFlash(0x3e000, &m_data, 4);
    readFromFlash(0x3e000, &data, 4);
    NRF_LOG_INFO("0x%x",data);
    */
    // Set radio configuration parameters
    radio_configure();
    // Set payload pointer
    NRF_RADIO->PACKETPTR = (uint32_t)&packet;

    //err_code = bsp_indication_set(BSP_INDICATE_USER_STATE_OFF);
    NRF_LOG_INFO("Radio transmitter example started.");
    NRF_LOG_INFO("Press Any Button");
    APP_ERROR_CHECK(err_code);
    
    uint32_t address = nrf5_flash_end_addr_get();
    NRF_LOG_INFO("address = %x",address);
    NRF_LOG_FLUSH();

    
    //xTaskCreate(vTaskCode1 ,"TASK1" ,501 ,NULL ,6 ,NULL);
    //xTaskCreate(vTaskCode2 ,"TASK2" ,256 ,NULL ,6 ,NULL);
    //vTaskStartScheduler();
    
    while (true)
    {
        //if(start_check_lid)
        //    checkLidOpen();

        if(start_catch_baro)
            catch_barodata();
        
        if(start_send_data){
            send_barodata();
        }
            
        /*
        if (packet != 0)
        {
            send_packet();
            NRF_LOG_INFO("The contents of the package was %u", (unsigned int)packet);
            packet = 0;
        }
        */
        NRF_LOG_FLUSH();
        __WFE();          //Enter Low Power state until an event occurs.
    }
    
}


/**
 *@}
 **/
