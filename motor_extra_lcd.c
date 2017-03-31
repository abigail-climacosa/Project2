//  square.c: Uses timer 2 interrupt to generate a square wave in pin
//  P2.0 and a 75% duty cycle wave in pin P2.1
//  Copyright (c) 2010-2015 Jesus Calvino-Fraga
//  ~C51~

#include <C8051F38x.h>
#include <stdlib.h>
#include <stdio.h>

#define SYSCLK    48000000L // SYSCLK frequency in Hz
#define BAUDRATE  115200L   // Baud rate of UART in bps

#define LEFT1 P2_7
#define LEFT2 P2_6
#define RIGHT1 P2_5
#define RIGHT2 P2_4
<<<<<<< HEAD
#define START_STOP P2_7   // 1 to start, 0 to s
=======
//#define START_STOP P2_2
>>>>>>> 53cbcf637283af631adaaf5db69e256520366ac6

char buffer[33]; // for turning int into string for LCD
volatile unsigned char pwm_count=0;

char _c51_external_startup (void)
{
	PCA0MD&=(~0x40) ;    // DISABLE WDT: clear Watchdog Enable bit
	VDM0CN=0x80; // enable VDD monitor
	RSTSRC=0x02|0x04; // Enable reset on missing clock detector and VDD

	// CLKSEL&=0b_1111_1000; // Not needed because CLKSEL==0 after reset
	#if (SYSCLK == 12000000L)
		//CLKSEL|=0b_0000_0000;  // SYSCLK derived from the Internal High-Frequency Oscillator / 4 
	#elif (SYSCLK == 24000000L)
		CLKSEL|=0b_0000_0010; // SYSCLK derived from the Internal High-Frequency Oscillator / 2.
	#elif (SYSCLK == 48000000L)
		CLKSEL|=0b_0000_0011; // SYSCLK derived from the Internal High-Frequency Oscillator / 1.
	#else
		#error SYSCLK must be either 12000000L, 24000000L, or 48000000L
	#endif
	OSCICN |= 0x03; // Configure internal oscillator for its maximum frequency

	// Configure UART0
	SCON0 = 0x10; 
#if (SYSCLK/BAUDRATE/2L/256L < 1)
	TH1 = 0x10000-((SYSCLK/BAUDRATE)/2L);
	CKCON &= ~0x0B;                  // T1M = 1; SCA1:0 = xx
	CKCON |=  0x08;
#elif (SYSCLK/BAUDRATE/2L/256L < 4)
	TH1 = 0x10000-(SYSCLK/BAUDRATE/2L/4L);
	CKCON &= ~0x0B; // T1M = 0; SCA1:0 = 01                  
	CKCON |=  0x01;
#elif (SYSCLK/BAUDRATE/2L/256L < 12)
	TH1 = 0x10000-(SYSCLK/BAUDRATE/2L/12L);
	CKCON &= ~0x0B; // T1M = 0; SCA1:0 = 00
#else
	TH1 = 0x10000-(SYSCLK/BAUDRATE/2/48);
	CKCON &= ~0x0B; // T1M = 0; SCA1:0 = 10
	CKCON |=  0x02;
#endif
	TL1 = TH1;      // Init Timer1
	TMOD &= ~0xf0;  // TMOD: timer 1 in 8-bit autoreload
	TMOD |=  0x20;                       
	TR1 = 1; // START Timer1
	TI = 1;  // Indicate TX0 ready
	
	// Configure the pins used for square output
	P2MDOUT|=0b_0000_0011;
	P0MDOUT |= 0x10; // Enable UTX as push-pull output
	XBR0     = 0x01; // Enable UART on P0.4(TX) and P0.5(RX)                     
	XBR1     = 0x40; // Enable crossbar and weak pull-ups

	// Initialize timer 2 for periodic interrupts
	TMR2CN=0x00;   // Stop Timer2; Clear TF2;
	CKCON|=0b_0001_0000;
	TMR2RL=(-(SYSCLK/(2*48))/(100L)); // Initialize reload value
	TMR2=0xffff;   // Set to reload immediately
	ET2=1;         // Enable Timer2 interrupts
	TR2=1;         // Start Timer2

	EA=1; // Enable interrupts
	
	return 0;
}

void PORT_Init (void)
{
	P0MDOUT |= 0x10; // Enable UART TX as push-pull output
	XBR0=0b_0000_0001; // Enable UART on P0.4(TX) and P0.5(RX)                    
	XBR1=0b_0101_0000; // Enable crossbar.  Enable T0 input.
	XBR2=0b_0000_0000;
}

void SYSCLK_Init (void)
{
	// CLKSEL&=0b_1111_1000; // Not needed because CLKSEL==0 after reset
#if (SYSCLK == 12000000L)
	//CLKSEL|=0b_0000_0000;  // SYSCLK derived from the Internal High-Frequency Oscillator / 4 
#elif (SYSCLK == 24000000L)
	CLKSEL|=0b_0000_0010; // SYSCLK derived from the Internal High-Frequency Oscillator / 2.
#elif (SYSCLK == 48000000L)
	CLKSEL|=0b_0000_0011; // SYSCLK derived from the Internal High-Frequency Oscillator / 1.
#else
	#error SYSCLK must be either 12000000L, 24000000L, or 48000000L
#endif
	OSCICN |= 0x03;   // Configure internal oscillator for its maximum frequency
	RSTSRC  = 0x04;   // Enable missing clock detector
}


void UART0_Init (void)
{
	SCON0 = 0x10;
   
#if (SYSCLK/BAUDRATE/2L/256L < 1)
	TH1 = 0x10000-((SYSCLK/BAUDRATE)/2L);
	CKCON &= ~0x0B;                  // T1M = 1; SCA1:0 = xx
	CKCON |=  0x08;
#elif (SYSCLK/BAUDRATE/2L/256L < 4)
	TH1 = 0x10000-(SYSCLK/BAUDRATE/2L/4L);
	CKCON &= ~0x0B; // T1M = 0; SCA1:0 = 01                  
	CKCON |=  0x01;
#elif (SYSCLK/BAUDRATE/2L/256L < 12)
	TH1 = 0x10000-(SYSCLK/BAUDRATE/2L/12L);
	CKCON &= ~0x0B; // T1M = 0; SCA1:0 = 00
#else
	TH1 = 0x10000-(SYSCLK/BAUDRATE/2/48);
	CKCON &= ~0x0B; // T1M = 0; SCA1:0 = 10
	CKCON |=  0x02;
#endif
	TL1 = TH1;      // Init Timer1
	TMOD &= ~0xf0;  // TMOD: timer 1 in 8-bit autoreload
	TMOD |=  0x20;                       
	TR1 = 1; // START Timer1
	TI = 1;  // Indicate TX0 ready
}

 void TIMER0_Init(void)
{
	TMOD&=0b_1111_0000; // Set the bits of Timer/Counter 0 to zero
	TMOD|=0b_0000_0001; // Timer/Counter 0 used as a 16-bit timer
	TR0=0; // Stop Timer/Counter 0
}

void Timer3us(unsigned char us)
{
	unsigned char i;               // usec counter
	
	// The input for Timer 3 is selected as SYSCLK by setting T3ML (bit 6) of CKCON:
	CKCON|=0b_0100_0000;
	
	TMR3RL = (-(SYSCLK)/1000000L); // Set Timer3 to overflow in 1us.
	TMR3 = TMR3RL;                 // Initialize Timer3 for first overflow
	
	TMR3CN = 0x04;                 // Sart Timer3 and clear overflow flag
	for (i = 0; i < us; i++)       // Count <us> overflows
	{
		while (!(TMR3CN & 0x80));  // Wait for overflow
		TMR3CN &= ~(0x80);         // Clear overflow indicator
	}
	TMR3CN = 0 ;                   // Stop Timer3 and clear overflow flag
}

void InitADC (void)
{
<<<<<<< HEAD
=======
	// Init ADC
	ADC0CF = 0xF8; // SAR clock = 31, Right-justified result
	ADC0CN = 0b_1000_0000; // AD0EN=1, AD0TM=0
  	REF0CN = 0b_0000_1000; //Select VDD as the voltage reference for the converter
}

void InitPinADC (unsigned char portno, unsigned char pinno)
{
	unsigned char mask;
	
	mask=1<<pinno;
	
	switch (portno)
	{
		case 0:
			P0MDIN &= (~mask); // Set pin as analog input
			P0SKIP |= mask; // Skip Crossbar decoding for this pin
		break;
		case 1:
			P1MDIN &= (~mask); // Set pin as analog input
			P1SKIP |= mask; // Skip Crossbar decoding for this pin
		break;
		case 2:
			P2MDIN &= (~mask); // Set pin as analog input
			P2SKIP |= mask; // Skip Crossbar decoding for this pin
		break;
		case 3:
			P3MDIN &= (~mask); // Set pin as analog input
			P3SKIP |= mask; // Skip Crossbar decoding for this pin
		break;
		default:
		break;
	}
}

unsigned int ADC_at_Pin(unsigned char pin)
{
	AMX0P = pin;             // Select positive input from pin
	AMX0N = LQFP32_MUX_GND;  // GND is negative input (Single-ended Mode)
	// Dummy conversion first to select new pin
	AD0BUSY=1;
	while (AD0BUSY); // Wait for dummy conversion to finish
	// Convert voltage at the pin
	AD0BUSY = 1;
	while (AD0BUSY); // Wait for conversion to complete
	return (ADC0L+(ADC0H*0x100));
}

float Volts_at_Pin(unsigned char pin)
{
	 return ((ADC_at_Pin(pin)*3.30)/1024.0);
}
void waitms (unsigned int ms)
{
>>>>>>> 53cbcf637283af631adaaf5db69e256520366ac6
	unsigned int j;
	for(j=ms; j!=0; j--)
	{
		Timer3us(249);
		Timer3us(249);
		Timer3us(249);
		Timer3us(250);
	}
}

	volatile int pwm1; //left wheel motor for pin 2.6
	volatile int pwm2; //left wheel motor for pin 2.5
	volatile int pwm1_stop;
	volatile int pwm2_stop;
	volatile int pwm1_temp;
	volatile int pwm2_temp;
	volatile int stop_flag;
	volatile int pwm3;
	volatile int pwm4;

void Timer2_ISR (void) interrupt 5
{
	TF2H = 0; // Clear Timer2 interrupt flag
	
	pwm_count++;
	if(pwm_count>100) pwm_count=0;

	LEFT1=pwm_count>pwm1?0:1;
	LEFT2=pwm_count>pwm2?0:1;
	RIGHT1=pwm_count>pwm3?0:1;
	RIGHT2=pwm_count>pwm4?0:1;
	
}

void main (void)
{	
	float deltaV;
	float x=0.0;
	volatile float V[4];
	int flag=1;
	PORT_Init();     // Initialize Port I/O
	SYSCLK_Init ();  // Initialize Oscillator
	UART0_Init();    // Initialize UART0
	TIMER0_Init();

	
	InitADC();
	InitPinADC(2, 0); // Configure P2.0 as analog input ---  middle inductor
	InitPinADC(2, 1); // Configure P2.1 as analog input ---  left inductor
	InitPinADC(2, 2); // Configure P2.2 as analog input ---  right inductor
	pwm1_stop = 50;
	pwm2_stop = 50;
	
	stop_flag = 1;
	
/*	printf("\x1b[2J"); // Clear screen using ANSI escape sequence.
	printf("\rEnter pwm 1 value for left signal:");
	scanf("%d\n", &pwm1);// smaller than pwm2 goes forward 
	printf("\nEnter pwm 2 value for left signal:");
	scanf("%d\n", &pwm2);
	printf("\nEnter pwm 3 value for right signal:");
	scanf("%d\n", &pwm3);//smaller value goes forward
	printf("\nEnter pwm 4 value for right signal:");
	scanf("%d\n", &pwm4);
	pwm1_temp = pwm1;
	pwm2_temp = pwm2;
<<<<<<< HEAD
=======
	*/
>>>>>>> 53cbcf637283af631adaaf5db69e256520366ac6
	
	while(1)
	{
		
	    V[0]=Volts_at_Pin(LQFP32_MUX_P2_0); //middle inductor
        V[2]=Volts_at_Pin(LQFP32_MUX_P2_1); //left inductor
   		V[1]=Volts_at_Pin(LQFP32_MUX_P2_2); //right inductor
   		if(V[1] > V[2]){
   		deltaV = V[1] - V[2];
   		}
   		else if(V[2] > V[1]){
   		deltaV = V[2] - V[1];
   		}
   		else{
   		deltaV = 0.0;
   		}
   		//deltaV = abs(V[1] - V[2]);
   		//waitms(500);
   		printf("Vmiddle=%5.3f, Vleft=%5.3f, Vright=%5.3f, delta =%5.3f\r",V[0], V[1], V[2], deltaV);
        
		while(V[0] >= 2.2){
			pwm1 = 100;
        	pwm2 = 100;
        	pwm3 = 100;
        	pwm4 = 100;
        //	if(deltaV<=1){
        //	flag = 0;
        //	pwm1 = 100;
        //	pwm2 = 100;
        //	pwm3 = 100;
        //	pwm4 = 100;
        //	}
			}
			
        	/*pwm1 = 100;
        	pwm2 = 100;
        	pwm3 = 100;
        	pwm4 = 100;*/
        
        if(V[2]/V[1] == 1 ){
        	pwm1 = 0;  //0
        	pwm2 = 100;  //100
        	pwm3 = 0;  //0
        	pwm4 = 100;} //100
        	
        if(V[1]>V[2]){ //right wheel is closer than left wheel, decrease right pwm
        	pwm1 = 0;    // set left wheel at highest speed
        	pwm2 = 100; 
        	if(deltaV >= 0.6){
        	x = 0.0;
        	}
<<<<<<< HEAD
      	}
        }
         
=======
        	if(deltaV >= 0.5 && deltaV < 6){
        	x = 100.0*(deltaV/0.6);
        	}
        	if(deltaV < 0.5) {
        	x = (100.0*(deltaV/0.6))/1.5; 
        	}
        	if(deltaV <= 0.1){
        	x = 0.0;
        	}
        	pwm3 = 0;
        	pwm4 = 100.0 - x; //100
        	}
        	
        if(V[1]<V[2]){ //left wheel is closer, decrease left pwm
        	pwm3 = 0;
        	pwm4 = 100;
        	if(deltaV >= 0.6){
        	x = 0.0;
        	}
        	if(deltaV >= 0.5 && deltaV < 0.6){
        	x = 100.0*(deltaV/0.6);
        	}
        	if(deltaV < 0.5) {
        	x = (100.0*(deltaV/0.6))/1.5;
        	}
        	if(deltaV <= 0.1){
        	x = 0.0;
        	}
        	pwm1 = 0;
        	pwm2 = 100.0 - x; //100
        	} 
        	
        
>>>>>>> 53cbcf637283af631adaaf5db69e256520366ac6
    }
   
}
