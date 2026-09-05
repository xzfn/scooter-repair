
#include "user_common.h"

#include "debug.h"


volatile uint32_t g_ms;

uint32_t get_ms() {
    return g_ms;
}

void init_tim2_timer_interrupt() {
    TIM_TimeBaseInitTypeDef tim_init_struct = {0};

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    tim_init_struct.TIM_Period = 1000 - 1;
    tim_init_struct.TIM_Prescaler = SystemCoreClock / 1000000 - 1;
    tim_init_struct.TIM_ClockDivision = TIM_CKD_DIV1;
    tim_init_struct.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &tim_init_struct);

    TIM_ARRPreloadConfig(TIM2, ENABLE);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    NVIC_SetPriority(TIM2_IRQn, 0x80);
    NVIC_EnableIRQ(TIM2_IRQn);

    TIM_Cmd(TIM2, ENABLE);
}

void delay_ms(uint32_t ms) {
    uint32_t start = get_ms();
    while (get_ms() - start < ms) {}  // wait
    return;
}

__attribute__((interrupt("WCH-Interrupt-fast")))
void TIM2_IRQHandler(void) {
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        ++g_ms;
    }
}
