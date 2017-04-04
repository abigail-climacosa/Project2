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
	if (Count >  0)
	{ 
		Count = 0;
		if (action == 0){
			ToggleLED(); // toggle the state of the LED
		}
		else if (action != 0){ 
			action--; 
			GPIOA_ODR &= (0b00000000);
		}
	}   
}

void delay(int dly)
{
	while( dly--);
}

void SysInit(void)
{
	// Set up output port bit for blinking LED
	RCC_AHBENR |= 0x00020000;  // peripheral clock enable for port A 
	GPIOA_MODER |= 0b00000001; // Make pin PA0 and PA1 output
	GPIOB_MODER |= 0b00000000; // Make all B pins inputs
	
	// Set up timer
	RCC_APB2ENR |= BIT11; // turn on clock for timer1
	TIM1_ARR = 250;      // reload counter with 8000 at each overflow (equiv to 1ms)
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
	int bamboozling_baud_bit = 640;
	SysInit();
	while(1)
	{    
		if (GPIOA_IDR & BIT1) { // button 1
			delay(10000);
			if (GPIOA_IDR & BIT1){
				while (GPIOA_IDR & BIT1);
				action = bamboozling_baud_bit;
			}
		}
		if (GPIOA_IDR & BIT2) { // button 2
			delay(10000);
			if (GPIOA_IDR & BIT2){
				while (GPIOA_IDR & BIT2);
				 action = bamboozling_baud_bit*2;
			}
		}
		if (GPIOA_IDR & BIT3) { // button 5
			delay(10000);
			if (GPIOA_IDR & BIT3){
				while (GPIOA_IDR & BIT3);
				action = bamboozling_baud_bit*5;
			}
		}
		if (GPIOA_IDR & BIT4) { // button 6
			delay(10000);
			if (GPIOA_IDR & BIT4){
				while (GPIOA_IDR & BIT4);
				action = bamboozling_baud_bit*6;
			}
		}
		if (GPIOA_IDR & BIT5) { // button 3
			delay(10000);
			if (GPIOA_IDR & BIT5){
				while (GPIOA_IDR & BIT5);
				action = bamboozling_baud_bit*3;
				}
		}
		if (GPIOA_IDR & BIT6) { // button 7
			delay(10000);
			if (GPIOA_IDR & BIT6){
				while (GPIOA_IDR & BIT6);
				action = bamboozling_baud_bit*7;
			}
		}
		if (GPIOA_IDR & BIT7) { // button 4
			delay(10000);
			if (GPIOA_IDR & BIT7){
				while (GPIOA_IDR & BIT7);
				action = bamboozling_baud_bit*4;
			}
		}
	/*	if (GPIOA_IDR & BIT1) {
			delay(10000);
			if (GPIOA_IDR & BIT1){
				while (GPIOA_IDR & BIT1);
				action = 4000; // 2 button 1
			}
		}
		if (GPIOA_IDR & BIT2) {
			delay(10000);
			if (GPIOA_IDR & BIT2){
				while (GPIOA_IDR & BIT2);
				action = 5000; // 4 button 2
			}
		}
		if (GPIOA_IDR & BIT3) {
			delay(10000);
			if (GPIOA_IDR & BIT3){
				while (GPIOA_IDR & BIT3);
				action = 8000; //10 button 5
			}
		}
		if (GPIOA_IDR & BIT4) {
			delay(10000);
			if (GPIOA_IDR & BIT4){
				while (GPIOA_IDR & BIT4);
				action = 9000; // 12 button 6
			}
		}
		if (GPIOA_IDR & BIT5) {
			delay(10000);
			if (GPIOA_IDR & BIT5){
				while (GPIOA_IDR & BIT5);
				action = 6000; //6 button 3
				}
		}
		if (GPIOA_IDR & BIT6) {
			delay(10000);
			if (GPIOA_IDR & BIT6){
				while (GPIOA_IDR & BIT6);
				action = 10000; // 14 button 7
			}
		}
		if (GPIOA_IDR & BIT7) {
			delay(10000);
			if (GPIOA_IDR & BIT7){
				while (GPIOA_IDR & BIT7);
				action = 7000; //8 button 4
			}
		}*/
			
	
	}
	return 0;
}
