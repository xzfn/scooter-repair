
#include "rs485.h"

#include "debug.h"

#include "user_common.h"


void rs485_init_half_duplex() {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_USART1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // UART pin remap USART1_RM = 10 partial mapping TX-PD6 RX-PD5 (default is TX-PD5 RX-PD6)
    GPIO_PinRemapConfig(GPIO_PartialRemap2_USART1, ENABLE);

    GPIO_InitTypeDef gpio_struct = {0};

    // GPIO UART TX/RX use PD6
    gpio_struct.GPIO_Pin = GPIO_Pin_6;
    gpio_struct.GPIO_Speed = GPIO_Speed_2MHz;
    gpio_struct.GPIO_Mode = GPIO_Mode_AF_OD;  // half-duplex need open drain + external pull-up resistor
    GPIO_Init(GPIOD, &gpio_struct);

    // GPIO RS485 direction DE/RE# use PA2
    gpio_struct.GPIO_Pin = GPIO_Pin_2;
    gpio_struct.GPIO_Speed = GPIO_Speed_2MHz;
    gpio_struct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &gpio_struct);


    USART_InitTypeDef uart_struct = {0};
    // UART init 8E1. 8 bits data, even parity, 1 stop bit
    uart_struct.USART_BaudRate = 9600;
    uart_struct.USART_WordLength = USART_WordLength_9b;  // with parity need 8 + 1, the actual data byte is still 8 bit
    uart_struct.USART_StopBits = USART_StopBits_1;
    uart_struct.USART_Parity = USART_Parity_Even;
    uart_struct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    uart_struct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &uart_struct);

    // UART half-duplex mode enable
    USART_HalfDuplexCmd(USART1, ENABLE);

    // Enable UART
    USART_Cmd(USART1, ENABLE);

    // Disable after init
    rs485_disable();
}

void rs485_direction_send() {
    GPIO_WriteBit(GPIOA, GPIO_Pin_2, Bit_SET);
}

void rs485_direction_receive() {
    GPIO_WriteBit(GPIOA, GPIO_Pin_2, Bit_RESET);
}

void rs485_disable() {
    uint32_t tempreg = USART1->CTLR1;
    tempreg &= ~(USART_Mode_Tx | USART_Mode_Rx);
    USART1->CTLR1 = tempreg;

    rs485_direction_receive();
}

void rs485_enable_send() {
    rs485_direction_send();

    uint32_t tempreg = USART1->CTLR1;
    tempreg &= ~USART_Mode_Rx;
    tempreg |= USART_Mode_Tx;
    USART1->CTLR1 = tempreg;
}

void rs485_enable_receive() {
    uint32_t tempreg = USART1->CTLR1;
    tempreg &= ~USART_Mode_Tx;
    tempreg |= USART_Mode_Rx;
    USART1->CTLR1 = tempreg;

    rs485_direction_receive();

    // clear receive buffer
    if (uart_receive_has_data()) {
        USART_ReceiveData(USART1);
    }
}

void uart_send_byte(uint8_t c) {
    while (!USART_GetFlagStatus(USART1, USART_FLAG_TXE)) {}  // wait TXE
    USART_SendData(USART1, c);
}

void uart_wait_transmission_complete() {
    while (!USART_GetFlagStatus(USART1, USART_FLAG_TC)) {}  // wait TC
}

void uart_send_frame(const uint8_t *buffer, uint8_t buffer_length) {
    for (uint8_t i = 0; i < buffer_length; ++i) {
        uart_send_byte(buffer[i]);
    }
    uart_wait_transmission_complete();
}

uint8_t uart_receive_byte() {
    while (!USART_GetFlagStatus(USART1, USART_FLAG_RXNE)) {}  // wait RXNE
    return USART_ReceiveData(USART1);
}

bool uart_receive_has_data() {
    return USART_GetFlagStatus(USART1, USART_FLAG_RXNE);
}

bool uart_receive_byte_timeout(uint8_t *out_byte, uint32_t timeout_ms) {
    uint32_t start = get_ms();
    while (get_ms() - start < timeout_ms) {
        if (uart_receive_has_data()) {
            *out_byte = USART_ReceiveData(USART1);
            return true;
        }
    }
    return false;
}

int uart_receive_frame_timeout(uint8_t *out_buffer, uint8_t out_buffer_length, uint32_t first_timeout_ms, uint32_t idle_timeout_ms) {
    uint8_t c = 0;
    bool success = false;

    if (out_buffer_length <= 0) {
        uart_wait_line_idle(idle_timeout_ms);  // wait line idle before return
        return -1;  // bad buffer length
    }

    uint8_t index = 0;

    // receive first byte use timeout first_timeout_ms (receive until first byte arrived, if not abort)
    success = uart_receive_byte_timeout(&c, first_timeout_ms);
    if (success) {
        out_buffer[index++] = c;
    }
    else {
        return -2;  // first byte timeout
    }

    // receive until last byte timeout idle_timeout_ms (receive until idle)
    while (1) {
        success = uart_receive_byte_timeout(&c, idle_timeout_ms);
        if (success) {
            if (index >= out_buffer_length) {
                uart_wait_line_idle(idle_timeout_ms);  // wait line idle before return
                return -3;  // buffer too small
            }
            else {
                out_buffer[index++] = c;
            }
        }
        else {
            // receive end
            return index;  // current index is total received length
        }
    }

    return -4;  // unreachable
}

void uart_wait_line_idle(uint32_t idle_timeout_ms) {
    uint8_t c = 0;
    bool success = false;

    while (1) {
        success = uart_receive_byte_timeout(&c, idle_timeout_ms);
        if (success) {
            // still getting data, continue wait
            ;
        }
        else {
            // idle timeout, line idle
            return;
        }
    }
}
