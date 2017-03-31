#include "stm32f05xxx.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

void ToggleLED(void);
volatile int Count = 0;
volatile int action = 0; // if not zero, any button has been pressed


// Interrupt service routines are the same as normal
// subroutines (or C funtions) in Cortex-M microcontrollers.
// The following should happen at a rate of 1kHz.
// The following function is associated with the TIM1 interrupt 
// via the interrupt vector table defined in startup.s
void Timer1ISR(void) 
{
	TIM1_SR &= ~BIT0; // clear update interrupt flag
	Count++;
	if (Count > 10)
	{ 
		Count = 0;
		if (action == 0){
			ToggleLED(); // toggle the state of the LED every second
		}
		else if (action != 0){
			action--;
		}
	}   
}

void SysInit(void)
{
	// Set up output port bit for blinking LED
	RCC_AHBENR |= 0x00020000;  // peripheral clock enable for port A 
	GPIOA_MODER |= 0b00000001; // Make pin PA0 and PA1 output
	GPIOB_MODER |= 0b00000000; // Make all B pins inputs
	
	// Set up timer
	RCC_APB2ENR |= BIT11; // turn on clock for timer1
	TIM1_ARR = 8;      // reload counter with 8000 at each overflow (equiv to 1ms)    CHANGED TO 800
	ISER |= BIT13;        // enable timer interrupts in the NVIC
	TIM1_CR1 |= BIT4;     // Downcounting
	TIM1_CR1 |= BIT0;     // enable counting
	TIM1_DIER |= BIT0;    // enable update event (reload event) interrupt  
	enable_interrupts();
}

void ToggleLED(void) 
{   
	GPIOA_ODR ^= BIT0; // Toggle PA0
	
}

int main(void)
{	
	int stop_flag = false;
	SysInit();
	while(1)
	{    
		if (GPIOA_IDR & BIT1) {
		
			while (GPIOA_IDR & BIT1);
	
			if (stop_flag == 0)
				stop_flag = 1;
			if (stop_flag == 1)
				stop_flag = 0;
			

			switch (stop_flag) {
				case 0:
					action = 1;
					break;
				case 1:
					action = 8;
					break;
			}
		}
		if (GPIOA_IDR & BIT2) {
			while (GPIOA_IDR & BIT2);
			action = 2;
		}
		if (GPIOA_IDR & BIT3) {
			while (GPIOA_IDR & BIT3);
			action = 5;
		}
		if (GPIOA_IDR & BIT4) {
			while (GPIOA_IDR & BIT4);
			action = 6;
		}
		if (GPIOA_IDR & BIT5) {
			while (GPIOA_IDR & BIT5);
			action = 3;
		}
		if (GPIOA_IDR & BIT6) {
			while (GPIOA_IDR & BIT6);
			action = 7;
		}
		if (GPIOA_IDR & BIT7) {
			while (GPIOA_IDR & BIT7);
			action = 4;
		}
			
	
	}
	return 0;
}
