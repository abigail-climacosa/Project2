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
#define Comparator P0_1  //for detecting the square wave

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
	XBR2     = 0x01;      // Enable UART1 on P0.0(TX1) and P0.1(RX1)
	
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
	//XBR2=0b_0000_0000;
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

void UART1_Init (unsigned long baudrate)
{
	SMOD1 = 0x0C; // no parity, 8 data bits, 1 stop bit
	SCON1 = 0x10;

	if (((SYSCLK/baudrate)/2L)/0xFFFFL < 1)
	{
		SBRL1 = 0x10000L-((SYSCLK/baudrate)/2L);
		SBCON1 |= 0x03; // set prescaler to 1
	}
	else if (((SYSCLK/baudrate)/2L)/0xFFFFL < 4)
	{
		SBRL1 = 0x10000L-(((SYSCLK/baudrate)/2L)/4L);
		SBCON1 &= ~0x03;
		SBCON1 |= 0x01; // set prescaler to 4
	}
	else if (((SYSCLK/baudrate)/2)/0xFFFFL < 12)
	{
		SBRL1 = 0x10000L-(((SYSCLK/baudrate)/2L)/12L);
		SBCON1 &= ~0x03; // set prescaler to 12
	}
	else
	{
		SBRL1 = 0x10000L-(((SYSCLK/baudrate)/2L)/48L);
		SBCON1 &= ~0x03;
		SBCON1 |= 0x02; // set prescaler to ?
	}
	SCON1 |= 0x02;    // indicate ready for TX
	SBCON1 |= 0x40;   // enable baud rate generator
}

char getchar1 (void)
{
	char c;
	while (!(SCON1 & 0x01));
	SCON1 &= ~0x01;
	SCON1&=0b_0011_1111;
	c = SBUF1;
	return (c);
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
	unsigned int j;
	for(j=ms; j!=0; j--)
	{
		Timer3us(249);
		Timer3us(249);
		Timer3us(249);
		Timer3us(250);
	}
}

	volatile int pwm1; //left wheel motor for pin 2.7
	volatile int pwm2; //left wheel motor for pin 2.6
	volatile int pwm3; //right wheel motor for pin 2.5
	volatile int pwm4; //right wheel motor for pin 2.4
    volatile int pwm1_temp;
    volatile int pwm2_temp;
    volatile int pwm3_temp;
    volatile int pwm4_temp;

void checkinstruction(float);
void car_go(void);
void stop(void);

void Timer2_ISR (void) interrupt 5
{
	TF2H = 0; // Clear Timer2 interrupt flag
	
	pwm_count++;
	if(pwm_count>100) pwm_count=0;

	LEFT1=pwm_count>pwm1?0:1;
	LEFT2=pwm_count>pwm2?0:1;  //when pwm2 > pwm1, moves forward
	RIGHT1=pwm_count>pwm3?0:1; 
	RIGHT2=pwm_count>pwm4?0:1; //when pwm4 > pwm 3, moves forward
	
}

int stopflag=0;
unsigned int turnleftflag=0;
unsigned int turnrightflag=0;



void main (void)
{	
	char c;
	float x=0.0;
	float deltaV;
	volatile float V[4];
	float Period;
	int flag=0;  //for intersection
	int flagcomp=0;
//	stopflag=1;
//	float comparator_counter;
	int count=0;
	int start_timer_flag = 0;
	int turn_180_flag = 0;
	int back_flag = 1;
	int a = 0;

	//PORT_Init();     // Initialize Port I/O
	SYSCLK_Init ();  // Initialize Oscillator
	XBR2 |= 0b0000_0001; //pin 0.1
	UART1_Init(50);
	UART0_Init();    // Initialize UART0
	TIMER0_Init();

	
	InitADC();
	InitPinADC(2, 0); // Configure P2.0 as analog input ---  middle inductor
	InitPinADC(2, 1); // Configure P2.1 as analog input ---  right inductor
	InitPinADC(2, 2); // Configure P2.2 as analog input ---  left inductor
	InitPinADC(1,7); // Configure P1.7 as analog input -- back inductor
	TF0 = 0;
	

	

		
	while(1)
	{
		
	    V[0]=Volts_at_Pin(LQFP32_MUX_P2_0); //middle inductor
   		V[1]=Volts_at_Pin(LQFP32_MUX_P2_2); //left inductor
        V[2]=Volts_at_Pin(LQFP32_MUX_P2_1); //right inductor
        V[3]=Volts_at_Pin(LQFP32_MUX_P1_7); //back inductor
		// UART1
		c = getchar1();
		a = (int) c;
		checkinstruction(a);
		printf("a = %d\n", c);
        
		
		//printf("Vmiddle=%5.3f, Vleft=%5.3f, Vright=%5.3f, Vback= %5.3f, delta =%5.3f\r",V[0], V[1], V[2], V[3], deltaV);
		//-------------Backwards CODE--------//
		//---------------------------------//
		if(V[3]<1){
			if(V[0]>1.6){
			pwm3=70;
			pwm4=0;
			pwm1=0;
			pwm2=10;
			waitms(50);
			}
			if(V[0]>0.5 && V[0]<1.6){//turn left
			pwm1=70;
			pwm2=0;
			pwm3=0;
			pwm4=10;
			waitms(50);
		
			}
		}
		//-------------Backwards Code Done--------//
		
		//-------------STOP CODE--------//
		//---------------------------------//
        if(stopflag==-1){
        pwm1=0;
        pwm2=0;
        pwm3=0;
        pwm4=0;
        //start_timer_flag = 0;
        }
		//-------------STOP CODE--------//
		//---------------------------------//
       	
       
		if(V[1] > V[2]){      //left wheel voltage > right wheel voltage
   		deltaV = V[1] - V[2];
   		}
   		else if(V[2] > V[1]){ //right wheel voltage > left wheel voltage
   		deltaV = V[2] - V[1];
   		}
   		else{                 //right wheel voltage = left wheel voltage
   		deltaV = 0.0;
   		}
		
		/*if(V[0] >= 2.2  && stopflag==0 && turnrightflag == 1 ){  //detecting intersection, usually V[0] = 0, 
				pwm1= 0;
	            pwm2= 50;
                pwm3= 50;
                pwm4= 0;
                waitms(500);
                while(deltaV>0.2){
                    //printf("Do we get here?\r");
                    V[0]=Volts_at_Pin(LQFP32_MUX_P2_0); //middle inductor
   					V[1]=Volts_at_Pin(LQFP32_MUX_P2_2); //left inductor
        			V[2]=Volts_at_Pin(LQFP32_MUX_P2_1); //right inductor
        			if(V[1] > V[2]){      //left wheel voltage > right wheel voltage
   						deltaV = V[1] - V[2];
   					}
   					else if(V[2] > V[1]){ //right wheel voltage > left wheel voltage
   						deltaV = V[2] - V[1];
   					}
   					else{                 //right wheel voltage = left wheel voltage
   						deltaV = 0.0;
   					}
   					
    	           	pwm1= 0;
		           	pwm2= 100;
    	           	pwm3= 100;
    	           	pwm4= 0;
	            }	
	            //car_go();
	            pwm1 = 0;
	            pwm2 = 100;
	            pwm3 = 0;
	            pwm4 = 100;
	            waitms(300);

        	}*/
        
        	
        /*
			
		if(V[0] >= 2.2  && stopflag==0 && turnleftflag==1 ){  //detecting intersection, usually V[0] = 0, 
				pwm1= 80;
	            pwm2= 0;
                pwm3= 0;
                pwm4= 80;
                waitms(500);
				while(deltaV>0.25){
				//	printf("Do we get here? %5.3f\r", deltaV);
                    V[0]=Volts_at_Pin(LQFP32_MUX_P2_0); //middle inductor
   					V[1]=Volts_at_Pin(LQFP32_MUX_P2_2); //left inductor
        			V[2]=Volts_at_Pin(LQFP32_MUX_P2_1); //right inductor
        			if(V[1] > V[2]){      //left wheel voltage > right wheel voltage
   						deltaV = V[1] - V[2];
   					}
   					else if(V[2] > V[1]){ //right wheel voltage > left wheel voltage
   						deltaV = V[2] - V[1];
   					}
   					else{                 //right wheel voltage = left wheel voltage
   						deltaV = 0.0;
   					}
   					
    	           	pwm1= 100;
		           	pwm2= 0;
    	           	pwm3= 0;
    	           	pwm4= 100;
	            }	
	            //car_go();
	            pwm1 = 0;
	            pwm2 = 100;
	            pwm3 = 0;
	            pwm4 = 100;
	            waitms(300);
			
			}
		*/
		
	
		// turn 180
		//Left wheel turns forward, right wheel turns backward
	
		
		/*
		while (V[0] > 0.17 && turn_180==1) {
			pwm1 = 100;
			pwm2 = 0;
			pwm3 = 0;
			pwm4 = 100;
		}
		
		//Go forward
		
		pwm1 = 100;
		pwm2 = 0;
		pwm3 = 100;
		pwm4 = 0;
		
		waitms(5000);
		
		turn_180 = 0;
	*/
	
	
	
	

			
		// pwm1 left wheel motor for pin 2.7
 		// pwm2 left wheel motor for pin 2.6
		// pwm3 right wheel motor for pin 2.5
		// pwm4 right wheel motor for pin 2.4	
        
        if(V[2]/V[1] == 1 && stopflag==0 && back_flag==0){ //when right wheel = left wheel
        	pwm1 = 0;  
        	pwm2 = 100;  
        	pwm3 = 0;  
        	pwm4 = 100;
        } 
        	
        if(V[1]>V[2]  && stopflag==0 && back_flag==0){ //right wheel is closer than left wheel, decrease right pwm Vleft>Vright
        	pwm1 = 0;    // set left wheel at highest speed
        	pwm2 = 100; 
        	if(deltaV >= 0.5){
        		x = 0.0;
        		pwm4 = 100.0 - x;
        		pwm3 = 0.0;
        	}   
        	if(deltaV >= 0.4 && deltaV < 0.5){
        		x = 100.0*(deltaV/0.6);
        		pwm3 = 100.0 - x;
        		pwm4 = 0.0;
        	}
        	if(deltaV < 0.4 && deltaV > 0.1) {
        		x = (100.0*(deltaV/0.6))/1.5; 
        		pwm3 = 100.0 - x;
        		pwm4 = 0.0;
        	}
        	if(deltaV <= 0.1){
        		x = 0.0;
        		pwm3 = 100.0 - x;
        		pwm4 = 0.0;
        	}
        //	pwm4 = 0;
        //	pwm3 = abs(40.0 - x); 
    	}
        	
        if(V[1]<V[2]  && stopflag==0  && back_flag==0){ //left wheel is closer, decrease left pwm.... Vleft<Vright
        	pwm3 = 0;  //set right wheel at max speed
        	pwm4 = 100;
        	if(deltaV >= 0.5){
        		//x = 0.0;
        		pwm2 = 0;
        		pwm1 = 100;
        	}
        	if(deltaV >= 0.3 && deltaV < 0.5){
        		x = 100.0*(deltaV/0.6);
        		pwm2 = 0;
        		pwm1 = 100 - x;
        	}
        	if(deltaV < 0.3 && deltaV > 0.1) {
        		x = (100.0*(deltaV/0.6))/1.5;
        		pwm1 = 0;
        		pwm2 = 100 - x;
        	}
        	if(deltaV <= 0.1){
        		//x = 0.0;
        		pwm1 = 0;
        		pwm2 = 100;
        	}
   	
        }
        
        
     
        if(start_timer_flag == 0 && Comparator == 0){
        	TH0=0; 
			TL0=0; // Reset the timer
			TR0=1; // Stop timer 0
			start_timer_flag = 1;
        }
		
		if (Comparator==0){ // Wait for the signal to be zero	
			flagcomp=1;
			if(TF0 == 1){
				TF0 = 0;
				count++;
			}
		} // Start timing, when comparator == 0
		
		if(Comparator==1 && flagcomp==1){
			TR0=0;
			// Stop timer 0, comparator == 1
			Period=(count*65536.0+TH0*256.0+TL0);
			printf("period = %5.3f\r\n", Period);
			checkinstruction(Period);
		//	Period = 0.0;
			flagcomp = 0;
		}
		

     /*
        if (Comparator == 0 && flag == 0){ //detects signal sent from guide wire
			TR0 = 1;
			TF0 = 0;
			printf("Timer 0 is on\n");
			TH0 = 0;
			TL0 = 0;
			flag = 1;
			count = 0;
        }
       
        
		if(Comparator == 1 && flag == 1){
			TR0 = 0;
			flag = 0;
			comparator_counter = (TH0*256.0+TL0);
			printf("comparator counter = %5.3f\r\n", comparator_counter);
			printf("count = %d\n",count);
		}
		
		if(TF0 == 1 && flag == 1){
		printf("Timer overflows,timer 0 is turned off.\n");
		TR0 = 0;
		TF0 = 0;
		count++;
		}*/
		
		// Turn 180
		
      	if(turn_180_flag==1){

				pwm1 = 100;
				pwm2 = 0;
				pwm3 = 0;
				pwm4 = 100;
				waitms(1200);
				turn_180_flag=0;
		}
		
		// Backwards
		
		/*
		pwm1 "right" wheel
 		pwm2 "right" wheel 
		pwm3 "left" wheel 
		pwm4 "left" wheel 
		
		pwm 1 & 3 = "forward"
		pwm 2 & 4 = "backward"
		
		V[0]middle inductor
   		V[1]"right" inductor 
      	V[2]"left" inductor
		*/
		
		printf("Vmiddle=%5.3f, Vright=%5.3f, Vleft=%5.3f, delta =%5.3f\r",V[0], V[1], V[2], deltaV);
		 
		// Go "forward"
		/*
		 if(V[0]>1.5){
			pwm1=0;
			pwm2=0;
			pwm3=40;
			pwm4=0;
			printf("1\n");
		 }
		 */
		 // if middle inductor voltage is close to zero, move forward
		if(V[0]<0.6){
			pwm1=40;
			pwm2=0;
			pwm3=40;
			pwm4=0;
			printf("forward\n");
			}
			
		// adjusts car position until the middle inductor value is close to zero	
		 if(V[0]>0.6 && V[0]<1.9){
			 
			printf("adjustment"); 
			 
			// adjust, turn to left
			// right inductor > left inductor
			if (V[1] > V[2]) {
				pwm1=20;
				pwm2=0;
				pwm3=0;
				pwm4=0;
				waitms(100);
				pwm1 = 0;
				pwm2 = 0;
				pwm3 = 5;
				pwm4 = 0;
			}
			// adjust, turn right
			// right inductor < left inductor
			if (V[1] < V[2]) {
				pwm1 = 0;
				pwm2 = 0;
				pwm3 = 20;
				pwm4 = 0;
				waitms(100);
				pwm1 = 5;
				pwm2 = 0;
				pwm3 = 0;
				pwm4 = 0;
				
			}
				
		 }	
		// turn 
		if (V[0] > 1.9) && (deltaV > 1.0 ) {
			printf("turning");
			 //turn left
			 if (V[1] > V[2]) {
				 pwm1 = 20;
				 pwm2 = 0;
				 pwm3 = 0;
				 pwm4 = 0;
			 }
			 // turn right
			 if (V[1] < V[2]) {
				pwm1 = 0;
				pwm2 = 0;
				pwm3 = 20;
				pwm4 = 0;		 
			}
		}
		
		/*if(V[2]/V[1] == 1 && stopflag==0  && back_flag==1){ //when right wheel = left wheel
        	pwm1 = 40; 
        	pwm2 = 0;  
       		pwm3 = 40;
        	pwm4 = 0;  
       	 } 
        	
        // V"right">V"left"
        // Turn left	
        if(V[1]>V[2]  && stopflag==0 && back_flag==1){ //right wheel is closer than left wheel, decrease right pwm Vleft>Vright
        	pwm1 = 40; 
        	pwm2 = 0;    // set right wheel speed
        	if(deltaV >= 0.5){
        		x = 0.0;
        		pwm3 = 0.0;
        		pwm4 = 40.0 - x;
        	}   
        	if(deltaV >= 0.4 && deltaV < 0.5){
        		x = 100.0*(deltaV/0.6);
        		pwm3 = 0.0;
        		pwm4 = 40.0 - x;
       
        	}
        	if(deltaV < 0.4 && deltaV > 0.1) {
        		x = (100.0*(deltaV/0.6))/1.5; 
        		pwm3 = 40.0 - x;
        		pwm4 = 0.0;
        	}
        	if(deltaV <= 0.1){
        		x = 0.0;
        		pwm3 = 40.0 - x;
        		pwm4 = 0.0;
        	}
         
    	}
        
    	// V"right"<V"left"
    	//Turn Right
        if(V[1]<V[2]  && stopflag==0 && back_flag==1){ 
        	pwm3 = 40;
        	pwm4 = 0;  //set "left" wheel at max speed
        	// turn "right"
        	if(deltaV >= 0.5){
        		//x = 0.0;
        		pwm1 = 0;
        		pwm2 = 40;
        	}
        	// adjust "right" wheel speed to correct car
        	if(deltaV >= 0.3 && deltaV < 0.5){
        		x = 100.0*(deltaV/0.6);
        		pwm1 = 0;
        		pwm2 = 40 - x;
        	}
        	if(deltaV < 0.3 && deltaV > 0.1) {
        		x = (100.0*(deltaV/0.6))/1.5;
        		pwm1 = 40 - x;
        		pwm2 = 0;
        	}
        	// go "forward"
        	if(deltaV <= 0.1){
        		//x = 0.0;
        		pwm1 = 40;
        		pwm2 = 0;
        	}
        }*/
       
    }
   
}


void checkinstruction(float Period){
	if(Period > 400000.00 && Period < 550000.00){//button1
	   stopflag=~stopflag;
	   printf("stopflag = %d\n", stopflag);
	}
	if(Period > 580000.00 && Period < 630000.00){ //button2
		turnleftflag  = 1;
	}
	if(Period > 700000.00 && Period < 780000.00){//button3
		turnrightflag = 1;
	}
	/*if(Period > 840000.00 && Period < 930000.00){//button4
	//
	}
	if(Period > 950000.00 && Period < 1000000.00){//button5
	  //instruction
	}
	if(Period > 1100000.00 && Period < 1200000.00){ //button6
	//instruction
	}
	if(Period>1200000.00){ //button 7
	//instruction
	}*/
	
	//Period = 0.0;
	
	return;
}

void car_go (void){

	printf("Test\r");
	/*pwm1_temp = pwm1;
    pwm2_temp = pwm2;
    pwm3_temp = pwm3;
    pwm4_temp = pwm4;
    */
    
    pwm1= 100;
    pwm2= 0;
    pwm3= 100;
    pwm4= 0;
    //stopflag=1;
    
    return;
}


void stop (void){
    
    pwm1_temp = pwm1;
    pwm2_temp = pwm2;
    pwm3_temp = pwm3;
    pwm4_temp = pwm4;
    
    pwm1 = 0;
    pwm2 = 0;
    pwm3 = 0;
    pwm4 = 0;
   return;
    
}

void backward (void){
    
    pwm1_temp = pwm1;
    pwm2_temp = pwm2;
    pwm3_temp = pwm3;
    pwm4_temp = pwm4;
    
    pwm1 = pwm2_temp;
    pwm2 = pwm1_temp;
    pwm3 = pwm4_temp;
    pwm4 = pwm3_temp;
    
}






void save_pwm_temp (void) {
	pwm1_temp = pwm1;
    pwm2_temp = pwm2;
    pwm3_temp = pwm3;
    pwm4_temp = pwm4;
}

