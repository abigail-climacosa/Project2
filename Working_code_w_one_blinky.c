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
#define LED P1_0
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
		CLfKSEL|=0b_0000_0010; // SYSCLK derived from the Internal High-Frequency Oscillator / 2.
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

/*void PORT_Init (void)
{
	P0MDOUT |= 0x10; // Enable UART TX as push-pull output
	XBR0=0b_0000_0001; // Enable UART on P0.4(TX) and P0.5(RX)                    
	XBR1=0b_0101_0000; // Enable crossbar.  Enable T0 input.
	//XBR2=0b_0000_0000;
}*/

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

void checkinstruction(int);
void car_go(void);
void stop(void);
void blinky(void);

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
void longdelay (void)
{
	unsigned int i, j;
	for(j=0; j<3; j++)
		for(i=0; i<0x8000; i++);
}


unsigned int turnleftflag=0;
unsigned int turnrightflag=0;
int stopflag=0;
int back_flag=0;
int turn_180_flag=0;
float deltaV;
volatile float V[4];
int ab;


void main (void)
{	
	char c;
	float x=0.0;
//	float counterstop=0;
//	float Period;
	int flag=0;  //for intersection
	int flagcomp=0;
//	stopflag=1;
//	float comparator_counter;
	int count=0;
	int start_timer_flag = 0;
	int a=0;
	//int back_flag = 0;


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
	InitPinADC(1, 7); // Configure P1.7 as analog input -- back inductor
	
	//TF0 = 0;
	LED=1;
	while(1)
	{
		
	    V[0]=Volts_at_Pin(LQFP32_MUX_P2_0); //middle inductor
   		V[1]=Volts_at_Pin(LQFP32_MUX_P2_2); //right inductor           
        V[2]=Volts_at_Pin(LQFP32_MUX_P2_1); //left inductor
        V[3]=Volts_at_Pin(LQFP32_MUX_P1_7); //back inductor
		//printf("Vmiddle=%5.3f, Vright=%5.3f, Vleft=%5.3f, Vback= %5.3f, delta =%5.3f\r",V[0], V[1], V[2], V[3], deltaV);
		
		
		printf("LED = %d\r", LED);
		
		
		if(V[1] > V[2]){      //left wheel voltage > right wheel voltage
   		deltaV = V[1] - V[2];
   		}
   		else if(V[2] > V[1]){ //right wheel voltage > left wheel voltage
   		deltaV = V[2] - V[1];
   		}
   		else{                 //right wheel voltage = left wheel voltage
   		deltaV = 0.0;
   		}
		// UART1
		if(Comparator == 0 && deltaV <= 0.07 && V[0]< 0.9){
		c = getchar1();
		a = (int) c;
			if(a==-1 || a==-2 || a==-4 || a==-8 || a==-16){
			printf("a = %d\n", c);
			checkinstruction(a);
			}
		}
	
       // printf("Vmiddle=%5.3f, Vright=%5.3f, Vleft=%5.3f, Vback= %5.3f, delta =%5.3f\r",V[0], V[1], V[2], V[3], deltaV);
		
		
		
		//--------------------------FORWARD CODE -----------------//
		//--------------------------------------------------------//
		
		/*
		  pwm1 left wheel motor for pin 2.7
 		  pwm2 left wheel motor for pin 2.6
		  pwm3 right wheel motor for pin 2.5
		  pwm4 right wheel motor for pin 2.4
		*/	
        if(turnleftflag==-1){
        	if(LED==1)
        	{
        		LED=0;
        	}
        	else
        	{
        		LED=1;
        	}
        	waitms(20);
        } 
        
        
        if(V[2]/V[1] == 1 && stopflag==0 && back_flag==0){ //when right wheel = left wheel
        	pwm1 = 0;  
        	pwm2 = 50;  
        	pwm3 = 0;  
        	pwm4 = 50;
        } 
        	
       if(V[1]>V[2]  && stopflag==0 && back_flag==0){ //right wheel is closer than left wheel, decrease right pwm Vleft>Vright
        	pwm1 = 0;    // set left wheel at highest speed
        	pwm2 = 60; 
        	if(deltaV >= 0.5){
        		x = 0.0;
        		pwm4 = 50.0 - x;
        		pwm3 = 0.0;
        	}   
        	if(deltaV >= 0.4 && deltaV < 0.5){
        		x = 50.0*(deltaV/0.6);
        		pwm3 = 50.0 - x;
        		pwm4 = 0.0;
        	}
        	if(deltaV < 0.4 && deltaV > 0.1) {
        		x = (50.0*(deltaV/0.6))/1.5; 
        		pwm3 = 50.0 - x;
        		pwm4 = 0.0;
        	}
        	if(deltaV <= 0.1){
        		x = 0.0;
        		pwm3 = 30.0 - x;
        		pwm4 = 0.0;
        	}
    	}
        	
        if(V[1]<V[2]  && stopflag==0  && back_flag==0){ //left wheel is closer, decrease left pwm.... Vleft<Vright
        	pwm3 = 0;  //set right wheel at max speed
        	pwm4 = 60;
        	if(deltaV >= 0.5){
        		pwm2 = 0;
        		pwm1 = 50;
        	}
        	if(deltaV >= 0.3 && deltaV < 0.5){
        		x = 50.0*(deltaV/0.6);
        		pwm2 = 0;
        		pwm1 = 50.0 - x;
        	}
        	if(deltaV < 0.3 && deltaV >= 0.1) {
        		x = (50.0*(deltaV/0.6))/1.5;
        		pwm1 = 0;
        		pwm2 = 50.0 - x;
        	}
        	if(deltaV <= 0.1){
        		pwm1 = 0;
        		pwm2 = 30;
        	}
   	
        }
       /* if(V[1]>V[2]  && stopflag==0 && back_flag==0){ //right wheel is closer than left wheel, decrease right pwm Vleft>Vright
        	pwm1 = 0;    // set left wheel at highest speed
        	pwm2 = 70; 
        	if(V[3]<1.6){
        	pwm4=50;
        	pwm3=10;
        	}
    	}
        if(V[1]<V[2]  && stopflag==0  && back_flag==0){ //left wheel is closer, decrease left pwm.... Vleft<Vright
        	pwm3 = 0;  //set right wheel at max speed
        	pwm4 = 70;
        	if(V[3]<1.6){
        	pwm1=50;

        	}
   	
        }*/
        //-------------FORWARD CODE END--------//
		//------------------------------------//
	
	
		//-------------STOP CODE--------//
		//---------------------------------//
        if(stopflag==-1){
			pwm1=0;
			pwm2=0;
			pwm3=0;
			pwm4=0;
        }
		//-------------STOP CODE END--------//
		//---------------------------------//
       	
       

		//-------------BACKWARDS CODE--------//
		//---------------------------------//
		if(V[3]<1 && back_flag==-1){
			pwm3=50;
			pwm4=0;
			pwm1=50;
			pwm2=0;
				if(V[0]>1.6){
					pwm3=50;
					pwm4=0;
					pwm1=0;
					pwm2=10;
					waitms(50);
				}
				if(V[0]>0.5 && V[0]<1.6){//turn left
					pwm1=50;
					pwm2=0;
					pwm3=0;
					pwm4=10;
					waitms(50);
				}
	//	printf("TESTTTTT\r");
		}
		//-------------BACKWARD END---------------//
		//----------------------------------------//
		
		
		//------------- RIGHT TURN CODE ----------//
		//----------------------------------------//
		
		if(V[0] >= 1.84  && stopflag==0 && turnrightflag == -1 ){  //detecting intersection, usually V[0] = 0, 
				turnrightflag=0;
				//LED=0;
			//	blinky();
				pwm1= 0;
	            pwm2= 50; //60 for usb
                pwm3= 50; //60 for usb
                pwm4= 0;
                waitms(1100); //900ms for usb 1100 for battery
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
		           	pwm2= 50;
    	           	pwm3= 50;
    	           	pwm4= 0;
	            }	
	            //car_go();
	            pwm1 = 0;
	            pwm2 = 60;
	            pwm3 = 0;
	            pwm4 = 60;
	            waitms(500);//500 for battery 350 with usb
	            LED=1;
	          // counterstop++;
	          //turnrightflag=0;
	           
			// printf("stopflag = %d,\nturnleftflag = %d,\nturnrightflag = %d,\nbackflag = %d\n180 Flag= %d\ndeltaV= %5.3fV[0]=%5.3f\n", stopflag, turnleftflag, turnrightflag, back_flag, turn_180_flag,deltaV,V[0]);
        	// printf("Vmiddle=%5.3f, Vleft=%5.3f, Vright=%5.3f, Vback= %5.3f, delta =%5.3f\n\n\n",V[0], V[1], V[2], V[3], deltaV);
		
        }
       	//------------- RIGHT TURN CODE END ----------//
		//--------------------------------------------//
        
        	
       //------------- LEFT TURN CODE ---------------//
	   //--------------------------------------------//
	
			
		if(V[0] >= 1.84  && stopflag==0 && turnleftflag==-1){  //detecting intersection, usually V[0] = 0, //2.2 for USB,1.7 battery
				turnleftflag=0;
			//	LED=0;
				pwm1= 60;
	            pwm2= 0;
                pwm3= 0;
                pwm4= 60;
                waitms(700);
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
   					
    	           	pwm1= 50;
		           	pwm2= 0;
    	           	pwm3= 0;
    	           	pwm4= 50;
	            }	
	            //car_go();
	            pwm1 = 0;
	            pwm2 = 45;
	            pwm3 = 0;
	            pwm4 = 45;
	            waitms(250);
	            //turnleftflag=0;
	            LED=1;
	           // counterstop++;
	             // printf("stopflag = %d,\nturnleftflag = %d,\nturnrightflag = %d,\nbackflag = %d\n180 Flag= %d\ndeltaV= %5.3fV[0]=%5.3f\n", stopflag, turnleftflag, turnrightflag, back_flag, turn_180_flag,deltaV,V[0]);
        	 printf("Vmiddle=%5.3f, Vleft=%5.3f, Vright=%5.3f, Vback= %5.3f, delta =%5.3f\n\n\n",V[0], V[1], V[2], V[3], deltaV);
        	// LED=1;
		}
		//------------- LEFT TURN CODE END ------------//
	    //-----------------------------------------//
		
	
		//------------- TURN 180 CODE ------------//
	    //-----------------------------------------// 
		// Left wheel turns forward, right wheel turns backward
      	if(turn_180_flag==-1){

				pwm1 = 80;
				pwm2 = 0;
				pwm3 = 0;
				pwm4 = 80;
				waitms(450);
				turn_180_flag=0;
		}
		//------------- TURN 180 CODE END ------------//
	    //-----------------------------------------// 
	    
    }
   
}



void checkinstruction(int a){
	/*if(a!=-1 || a!=-2 || a!=-4 || a!=-8 || a!=-16){
	stopflag=0;
	turnleftflag=0;
	turnrightflag=0;
	turn_180_flag=0;
	back_flag=0;
	return;
	}*/
	if(a == -1){//button1 stop/start
	   	stopflag=~stopflag;
	   	turnleftflag=0;
		turnrightflag=0;
		turn_180_flag=0;
		back_flag=0;

	   	printf("stopflag = %d,\nturnleftflag = %d,\nturnrightflag = %d,\nbackflag = %d\n180 Flag= %d\ndeltaV= %5.3fV[0]=%5.3f\n", stopflag, turnleftflag, turnrightflag, back_flag, turn_180_flag,deltaV,V[0]);
        printf("Vmiddle=%5.3f, Vleft=%5.3f, Vright=%5.3f, Vback= %5.3f, delta =%5.3f\n\n\n",V[0], V[1], V[2], V[3], deltaV);
		}
	
	if(a == -2 && turnrightflag==0 && back_flag==0 && turn_180_flag==0){ //button2 left turn
		stopflag=0;
		turnleftflag=-1;
		printf("stopflag = %d,\nturnleftflag = %d,\nturnrightflag = %d,\nbackflag = %d\n180 Flag= %d\ndeltaV= %5.3fV[0]=%5.3f\n", stopflag, turnleftflag, turnrightflag, back_flag, turn_180_flag,deltaV,V[0]);
       	printf("Vmiddle=%5.3f, Vleft=%5.3f, Vright=%5.3f, Vback= %5.3f, delta =%5.3f\n\n\n",V[0], V[1], V[2], V[3], deltaV);
		printf("turnleftflag = %d\n", turnleftflag);
		//LED=0;
	}
	
	if(a == -4 && turnleftflag==0 && back_flag==0 && turn_180_flag==0){//button3 right turn
		stopflag=0;
		turnrightflag=-1;
		printf("stopflag = %d,\nturnleftflag = %d,\nturnrightflag = %d,\nbackflag = %d\n180 Flag= %d\ndeltaV= %5.3fV[0]=%5.3f\n", stopflag, turnleftflag, turnrightflag, back_flag, turn_180_flag,deltaV,V[0]);
        printf("Vmiddle=%5.3f, Vleft=%5.3f, Vright=%5.3f, Vback= %5.3f, delta =%5.3f\n\n\n",V[0], V[1], V[2], V[3], deltaV);
		printf("turnrightflag = %d\n", turnrightflag);
	}
	
	if(a == -8 && turnrightflag==0 && turnleftflag==0 && turn_180_flag==0){//button4 backward
		stopflag=0;
		back_flag=-1;
		printf("stopflag = %d,\nturnleftflag = %d,\nturnrightflag = %d,\nbackflag = %d\n180 Flag= %d\ndeltaV= %5.3fV[0]=%5.3f\n", stopflag, turnleftflag, turnrightflag, back_flag, turn_180_flag,deltaV,V[0]);
        printf("Vmiddle=%5.3f, Vleft=%5.3f, Vright=%5.3f, Vback= %5.3f, delta =%5.3f\n\n\n",V[0], V[1], V[2], V[3], deltaV);
		printf("back_flag = %d\n", back_flag);
	}
	
	if(a == -16 && turnrightflag==0 && turnleftflag ==0 && back_flag==0){//button5 180 turn
	  stopflag=0;
	  turn_180_flag=-1;
	  	printf("stopflag = %d,\nturnleftflag = %d,\nturnrightflag = %d,\nbackflag = %d\n180 Flag= %d\ndeltaV= %5.3fV[0]=%5.3f\n", stopflag, turnleftflag, turnrightflag, back_flag, turn_180_flag,deltaV,V[0]);
        printf("Vmiddle=%5.3f, Vleft=%5.3f, Vright=%5.3f, Vback= %5.3f, delta =%5.3f\n\n\n",V[0], V[1], V[2], V[3], deltaV);
		printf("180 Flag = %d\n", turn_180_flag);
	}
	
	if(a==0){
	turnleftflag=0;
	turnrightflag=0;
	turn_180_flag=0;
	back_flag=0;
	stopflag=0;
	}
	
	if(a == -32){ //button6
	turnleftflag=0;
	turnrightflag=0;
	turn_180_flag=0;
	back_flag=0;
	stopflag=0;
	}
	if(a == -64){ //button 7
	turnleftflag=0;
	turnrightflag=0;
	turn_180_flag=0;
	back_flag=0;
	stopflag=0;
	}
	
	return;
}

void blinky(void){
	PCA0MD&=(~0x40) ; // DISABLE WDT: clear Watchdog Enable bit
	ab=0;
    //Enable the Port I/O Crossbar
    P1SKIP|=0x04;  //skip LED pin in crossbar assignments
    XBR1=0x40;     //enable Crossbar
    P1MDOUT|=0x08; //make LED pin output push-pull
    P1MDIN|=0x08;  //make LED pin input mode digital
    
	while(ab<100)
	{
		P1_0=1;        //Led on
		longdelay();
		P1_0=0;        //Led off
		longdelay();
		ab++;
	}
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

