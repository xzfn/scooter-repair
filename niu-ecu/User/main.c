/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2023/12/25
 * Description        : Main program body.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*
 *@Note
 *USART Print debugging routine:
 *USART1_Tx(PD6).
 *This example demonstrates the use of USART1(PD6) as a print debug port output.
 *
 */

#include "debug.h"

#include "user_common.h"
#include "rs485.h"
#include "niu_controller.h"
#include "niu_series_u.h"

/* Global typedef */

/* Global define */

/* Global Variable */


/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */

void test_rs485_send() {
    rs485_enable_send();
    uint8_t i = 0;
    while (1) {
        uart_send_byte(i++);
        delay_ms(1000);
    }
}

void test_rs485_echo_single_byte() {
    while (1) {
        rs485_enable_receive();
        uint8_t c = uart_receive_byte();

        rs485_enable_send();
        uart_send_byte(~c);
        uart_wait_transmission_complete();
    }
}

void test_rs485_echo_frame() {
    uint8_t uart_receive_buffer[5] = {0};
    while (1) {
        uint32_t first_timeout_ms = 5000;
        uint32_t idle_timeout_ms = 5;
        rs485_enable_receive();
        int result = uart_receive_frame_timeout(uart_receive_buffer, sizeof(uart_receive_buffer), first_timeout_ms, idle_timeout_ms);
        if (result > 0) {
            rs485_enable_send();
            uart_send_frame(uart_receive_buffer, result);
            rs485_enable_receive();
        }
        else {
            rs485_enable_send();
            uint8_t bad_result[4] = {'B', 'A', 'D', 0};
            bad_result[3] = (uint8_t)result;
            uart_send_frame(bad_result, 4);
            rs485_enable_receive();
        }
    }

}

void test_serial_number_crc16() {
    /*
    60.46832990646362   : ->FOC     : 20 01 01 | 03
    60.49982523918152   : <-FOC     : 20 81 10 | 4d 55 31 33 33 37 33 42 33 33 32 30 30 32 31 36
    60.67788124084473   : ->FOC     : 20 05 02 | 73 80
    60.688647747039795  : <-FOC     : 20 85 01 | ce
    */
    uint8_t serial_number[] = {0x4d, 0x55, 0x31, 0x33, 0x33, 0x37, 0x33, 0x42, 0x33, 0x33, 0x32, 0x30, 0x30, 0x32, 0x31, 0x36};
    uint16_t crc16 = calc_serial_number_crc16(serial_number, sizeof(serial_number));
    // result crc16 should be 73 80
    volatile uint8_t crc16_0 = (crc16 >> 8) & 0xFF;  // 115 0x73
    volatile uint8_t crc16_1 = crc16 & 0xFF;  // 128 0x80
    (void)crc16_0;
    (void)crc16_1;
    return;
}


int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
#if (SDI_PRINT == SDI_PR_OPEN)
    SDI_Printf_Enable();
#else
    // USART_Printf_Init(115200);
#endif
    // printf("SystemClk:%d\r\n", SystemCoreClock);
    // printf( "ChipID:%08x\r\n", DBGMCU_GetCHIPID() );
    // printf("This is printf example\r\n");

    init_tim2_timer_interrupt();
    rs485_init_half_duplex();

    // test_rs485_send();
    // test_rs485_echo_single_byte();
    // test_rs485_echo_frame();
    // test_serial_number_crc16();

    niu_main();

    while(1)
    {
    }
}
