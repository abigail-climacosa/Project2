$NOLIST
$MODLP52
$LIST

; Reset vector
org 0x0000
    ljmp MainProgram

; External interrupt 0 vector (not used in this code)
org 0x0003
	reti

; Timer/Counter 0 overflow interrupt vector
org 0x000B
	ljmp Timer0_ISR

; External interrupt 1 vector (not used in this code)
org 0x0013
	reti

; Timer/Counter 1 overflow interrupt vector (not used in this code)
org 0x001B
	reti

; Serial port receive/transmit interrupt vector (not used in this code)
org 0x0023 
	reti
	
; Timer/Counter 2 overflow interrupt vector
org 0x002B
	ljmp Timer2_ISR   

CLK  EQU 22118400
BAUD equ 115200
TIMER0_RATE   EQU 4096     ;-->changes pitch of wave  2048Hz squarewave (peak amplitude of CEM-1203 speaker)
TIMER0_RELOAD EQU ((65536-(CLK/TIMER0_RATE)))
T1LOAD equ (0x100-(CLK/(16*BAUD)))
TIMER2_RATE	equ 1000 ;rate of 1 second
TIMER2_RELOAD equ ((65536-(CLK/TIMER2_RATE)))
TIMER0_ADDRESS	equ 089H
	
; Variables	
DSEG at 30H
Result: 		ds 2
x:				ds 4
y:				ds 4
bcd:			ds 5
Count1ms: 		ds 2 ; Used to determine when half second has passed
Minutes:  		ds 1 ;
Seconds:  		ds 1 ;
Soak_Timer:		ds 1 ; counter for the soak 
Check_Temp:		ds 2 ; checks temperature at 60 seconds

;-----------------------------------------------------------------------------------------------------------------------------------
soak_temp: ds 2
soak_time: ds 2
reflow_temp: ds 2
reflow_time: ds 2
;-----------------------------------------------------------------------------------------------------------------------------------
;**********************************
;		PULSE WIDTH MODULATION
pwm:	ds 1
pwm_counter:	ds 1
ten_second_decrementer: ds 1
;**********************************
cseg

State:  	  	  db 'St:', 0
Time:	  	  	  db 'Time:  :', 0
Temp:			  db 'Temp:', 0

RF_Time:	  	  db 'RF:    s', 0
SK_Time:	  	  db 'SK:', 0
RF_Temp:	  	  db 'RFT:', 0
SK_Temp:	  	  db 'SKT:', 0	



; Flags
BSEG
mf: 			dbit 1
half_seconds_flag: dbit 1 ; Set to one in the ISR every time 500 ms had passed
soak_flag: 		dbit 1
reflow_start: 	dbit 1
ramp_to_soak_done: dbit 1
soak_done_flag: dbit 1
reflow_peak: 	dbit 1
open_door: 		dbit 1
take_PCB: 		dbit 1
reflow_done:	dbit 1
start_button_flag: dbit 1
state_begin: dbit 1
state0flag: dbit 1 ; *************NEW*******************
state1flag: dbit 1 ; *************NEW*******************
;**********************************
;		PULSE WIDTH MODULATION
seconds_flag2: dbit 1
;**********************************
;		SOAK TIMER/COUNTER 
seconds_flag3: dbit 1

;**********************************


cseg
; These 'equ' must match the wiring between the microcontroller and the LCD
LCD_RS equ P1.2
LCD_RW equ P1.3
LCD_E  equ P1.4
LCD_D4 equ P3.2
LCD_D5 equ P3.3
LCD_D6 equ P3.4
LCD_D7 equ P3.5

; These 'equ' must match the wiring between the microcontroller and ADC
CE_ADC EQU P2.0
MY_MOSI EQU P2.1
MY_MISO EQU P2.2
MY_SCLK EQU P2.3

SOUND_OUT EQU P3.7

LEFT		  equ p2.5 ;Hours
MIDDLE		  equ p2.6 ;Minutes

;---------------- START/KILL BUTTON -------------------;
START_BUTTON equ p1.1
RESET equ RST
;---------------- START/KILL BUTTON END -------------------;
Stop_message:  db 'Program Halted', 0
Check_message: db 'Error: No Sensor', 0
End_message:   db 'Reflow Finished', 0
Open_message:  db 'Open door', 0

;-----------------------------------------------------------------------------------------------------------------------------------------
; Adjustment of reflow controller parameters using pushbuttons
SOAK_TEMP_BUTTON equ P0.0
SOAK_TIME_BUTTON equ P0.2
REFLOW_TEMP_BUTTON equ P0.4
REFLOW_TIME_BUTTON equ P0.6
;-----------------------------------------------------------------------------------------------------------------------------------------
;**********************************
;		PULSE WIDTH MODULATION
PULSE_WIDTH_OUT equ P2.4
;**********************************

$NOLIST
$include(LCD_4bit_1.inc) ; A library of LCD related functions and utility macros
$include(math32.inc) ; A library of math functions and utility macros
$include(Change_States_Beep.inc) ;Contains comparisons for switching states
$LIST

;---------------------------------;
; Routine to initialize the ISR   ;
; for timer 0                     ;
;---------------------------------;
Timer0_Init:
	mov a, TMOD
	anl a, #0xf0 ; Clear the bits for timer 0
	orl a, #0x01 ; Configure timer 0 as 16-timer
	mov TMOD, a
	mov TH0, #high(TIMER0_RELOAD)
	mov TL0, #low(TIMER0_RELOAD)
	; Enable the timer and interrupts
    setb ET0  ; Enable timer 0 interrupt
    setb TR0  ; Start timer 0
	ret

;---------------------------------;
; ISR for timer 0.  Set to execute;
; every 1/4096Hz to generate a    ;
; 2048 Hz square wave at pin P3.7 ;
;---------------------------------;
Timer0_ISR:
	;clr TF0  ; According to the data sheet this is done for us already.
	; In mode 1 we need to reload the timer.
	clr TR0
	mov TH0, #high(TIMER0_RELOAD)
	mov TL0, #low(TIMER0_RELOAD)
	setb TR0
	cpl SOUND_OUT ; Connect speaker to P3.7!
	reti

;---------------------------------;
; Routine to initialize the ISR   ;
; for timer 2                     ;
;---------------------------------;
Timer2_Init:
	mov T2CON, #0 ; Stop timer/counter.  Autoreload mode.
	mov RCAP2H, #high(TIMER2_RELOAD)
	mov RCAP2L, #low(TIMER2_RELOAD)
	; Init One millisecond interrupt counter.  It is a 16-bit variable made with two 8-bit parts
	clr a
	mov Count1ms+0, a
	mov Count1ms+1, a
	; Enable the timer and interrupts
    setb ET2  ; Enable timer 2 interrupt
    setb TR2  ; Enable timer 2
	ret

;---------------------------------;
; ISR for timer 2                 ;
;---------------------------------;
Timer2_ISR:
	clr TF2  ; Timer 2 doesn't clear TF2 automatically. Do it in ISR
	cpl P3.6 ; To check the interrupt rate with oscilloscope. It must be precisely a 1 ms pulse.
	; The two registers used in the ISR must be saved in the stack
	push acc
	push psw
	; Increment the 16-bit one mili second counter
	inc Count1ms+0    ; Increment the low 8-bits first
	mov a, Count1ms+0 ; If the low 8-bits overflow, then increment high 8-bits
	jnz Inc_Done
	inc Count1ms+1

Inc_Done:
	; Check if half second has passed
	mov a, Count1ms+0
	cjne a, #low(1000), Timer2_ISR_done ; Warning: this instruction changes the carry flag!
	mov a, Count1ms+1
	cjne a, #high(1000), Timer2_ISR_done
	
	; 500 milliseconds have passed.  Set a flag so the main program knows
	setb half_seconds_flag ; Let the main program know half second had passed
	;cpl TR0 ; Enable/disable timer/counter 0. This line creates a beep-silence-beep-silence sound.
	; Reset to zero the milli-seconds counter, it is a 16-bit variable
;**********************************
;		PULSE WIDTH MODULATION
	setb seconds_flag2
;**********************************
;		SOAK TIMER/COUNTER
	setb seconds_flag3
;**********************************
	clr a
	mov Count1ms+0, a
	mov Count1ms+1, a
	; Increment the BCD counter
	mov a, Seconds
	; If seconds < 59, increment
	cjne a, #0x59, Timer2_ISR_Increment_Seconds
Timer2_ISR_Increment_Minutes:
	mov a, Minutes
	add a, #0x01
	da a
	mov Minutes, a
Reset_Seconds:
	mov a, Seconds
	mov a, #0x00
	sjmp Timer2_ISR_da
Timer2_ISR_Increment_Seconds:
	add a, #0x01 ; Adding the 10-complement of -1 is like subtracting 1.
Timer2_ISR_da:
	da a ; Decimal adjust instruction.  Check datasheet for more details!
	mov Seconds, a
Timer2_ISR_done:
	pop psw
	pop acc
	reti

cseg
INIT_SPI:
	setb MY_MISO ; Make MISO an input pin
	clr MY_SCLK ; For mode (0,0) SCLK is zero
	ret
DO_SPI_G:
	push acc
	mov R1, #0 ; Received byte stored in R1
	mov R2, #8 ; Loop counter (8-bits)
DO_SPI_G_LOOP:
	mov a, R0 ; Byte to write is in R0
	rlc a ; Carry flag has bit to write
	mov R0, a
	mov MY_MOSI, c
	setb MY_SCLK ; Transmit
	mov c, MY_MISO ; Read received bit
	mov a, R1 ; Save received bit in R1
	rlc a
	mov R1, a
	clr MY_SCLK
	djnz R2, DO_SPI_G_LOOP
	pop acc
	ret

; Configure the serial port and baud rate using timer 1
InitSerialPort:
    ; Since the reset button bounces, we need to wait a bit before
    ; sending messages, or risk displaying gibberish!
    mov R1, #222
    mov R0, #166
    djnz R0, $   ; 3 cycles->3*45.21123ns*166=22.51519us
    djnz R1, $-4 ; 22.51519us*222=4.998ms
    ; Now we can safely proceed with the configuration
	clr	TR1
	anl	TMOD, #0x0f
	orl	TMOD, #0x20
	orl	PCON,#0x80
	mov	TH1,#T1LOAD
	mov	TL1,#T1LOAD
	setb TR1
	mov	SCON,#0x52
    ret

; Send a character using the serial port
putchar:
    jnb TI, putchar
    clr TI
    mov SBUF, a
    ret

; Send a constant-zero-terminated string using the serial port
SendString:
    clr A
    movc A, @A+DPTR
    jz SendStringDone
    lcall putchar
    inc DPTR
    sjmp SendString
SendStringDone:
    ret
	
	
Display_10_digit_BCD:
    Set_Cursor(2, 7)
    Display_BCD(bcd+4)
    Display_BCD(bcd+3)
    Display_BCD(bcd+2)
    Display_BCD(bcd+1)
    Display_BCD(bcd+0) 
 	ret

;                     1234567890123456    <- This helps determine the location of the counter
Initial_Message:  db '    xx:xx:xx  xx  '    

;-----------------------------------------------------------------------------------------------------------------------------------------
InitParameters:
	mov soak_temp, 		#0x50
	mov soak_temp+1,	#0x1			; default 150 degrees
	mov soak_time,		#0x80
	mov soak_time+1,	#0x00			; default 80 seconds
	mov reflow_temp, 	#0x17
	mov reflow_temp+1,	#0x2			; default 217 degrees
	mov reflow_time, 	#0x60
	mov reflow_time+1,	#0x00			; default 60 seconds
	ret
;-----------------------------------------------------------------------------------------------------------------------------------------
	; Incrementing Hex Variables with hundreds and ones
Increment_HEX_number:
	clr a
	mov a, R0
	cjne a, #0x99, Increment_only_ones
	mov R0, #0x00
	mov a, R1
	add a, #0x1
	da	a
	mov R1, a
	ret
Increment_only_ones:
	add a, #0x1
	da	a
	mov R0, a
	ret
;-----------------------------------------------------------------------------------------------------------------------------------------
;	PARAMETER INCREMENTS, RESETS AND WINDOWS
Adjust_soak_temp:
	clr a
	mov a, soak_temp+1  
	cjne a, #0x1, Increment_soak_temp		; Max 170 degrees Celsius
	mov a, soak_temp
	cjne a, #0x70, Increment_soak_temp
	mov soak_temp, #0x40					; Min 140 degrees Celsius
	mov soak_temp+1, #0x1
	ljmp Adjust_Parameters
Increment_soak_temp:	
	mov R0, soak_temp
	mov R1, soak_temp+1
	lcall Increment_HEX_number
	mov soak_temp, R0
	mov soak_temp+1, R1
	ljmp Adjust_Parameters
Adjust_soak_time:
	clr a
	mov a, soak_time+1  
	cjne a, #0x1, Increment_soak_time		; Max 120 seconds
	mov a, soak_time
	cjne a, #0x20, Increment_soak_time
	mov soak_time, #0x60					; Min 60 seconds
	mov soak_time+1, #0x00
	ljmp Adjust_Parameters
Increment_soak_time:	
	mov R0, soak_time
	mov R1, soak_time+1
	lcall Increment_HEX_number
	mov soak_time, R0
	mov soak_time+1, R1
	ljmp Adjust_Parameters
Adjust_reflow_temp:
	clr a
	mov a, reflow_temp+1  
	cjne a, #0x2, Increment_reflow_temp		; Max 249 degrees Celsius
	mov a, reflow_temp
	cjne a, #0x49, Increment_reflow_temp
	mov reflow_temp, #0x00					; Min 200 degrees Celsius 
	ljmp Adjust_Parameters
Increment_reflow_temp:	
	mov R0, reflow_temp
	mov R1, reflow_temp+1
	lcall Increment_HEX_number
	mov reflow_temp, R0
	mov reflow_temp+1, R1
	ljmp Adjust_Parameters
Adjust_reflow_time:
	clr a
	mov a, reflow_time+1  
	cjne a, #0x0, Increment_reflow_time		; Max 60 seconds
	mov a, reflow_time
	cjne a, #0x60, Increment_reflow_time
	mov reflow_time, #0x30					; Min 30 seconds 
	ljmp Adjust_Parameters
Increment_reflow_time:	
	mov R0, reflow_time
	mov R1, reflow_time+1
	lcall Increment_HEX_number
	mov reflow_time, R0
	mov reflow_time+1, R1
	ljmp Adjust_Parameters

;--------------------------------------------------------------------------------------------------------------------------------------------------
;	BOOSTERS (for setting parameters)
Adjust_soak_temp_booster:
	ljmp Adjust_soak_temp
Adjust_soak_time_booster:
	ljmp Adjust_soak_time
Adjust_reflow_time_booster:
	ljmp Adjust_reflow_time
Adjust_reflow_temp_booster:
	ljmp Adjust_reflow_temp
;------------------------------------------------------------------------
;******************************************
;		PULSE WIDTH MODULATION
Decrement_counter:
	dec a
	mov ten_second_decrementer, a
	ljmp Dec_return
Decrement_pwm_counter:
	setb PULSE_WIDTH_OUT
	dec a
	mov pwm_counter, a
	ljmp PWM_return
Change_counter:
	clr seconds_flag2
	mov a, pwm_counter						; first, checks pwm decrementer counter
	cjne a, #0, Decrement_pwm_counter
	clr PULSE_WIDTH_OUT
PWM_return:
	mov a, ten_second_decrementer			; second, checks ten-second decrementer
	cjne a, #0, Decrement_counter
	mov ten_second_decrementer, #9
	mov pwm_counter, pwm
Dec_return:
	ljmp Returning
Check_half_seconds:							; uses half-seconds flag(2) to determine if counters should change
	jb seconds_flag2, Change_counter	; if not, return
	ljmp Returning
;******************************************

MainProgram:
		mov SP, #7FH
		mov PMOD, #0
		lcall InitSerialPort
		lcall INIT_SPI		
		
		;Set LCD to 4 bit
		lcall LCD_4BIT
		
		Set_Cursor(1,1)
		Send_Constant_String(#Initial_Message)    
 
		setb half_seconds_flag
		clr PULSE_WIDTH_OUT
;**********************************
;		PULSE WIDTH MODULATION
	mov ten_second_decrementer, #9 		; for half-second flags
	mov pwm, #4
	clr seconds_flag2
;**********************************
		mov Seconds, #0x00
		mov Minutes, #0x00
		mov Soak_Timer, #0x00

		clr TR0 ;clear Timer 0 Interrupt Bit
		clr SOUND_OUT
		lcall clear_flags
		
;------------------------------------------------------------------------------------------------------------------------------------------
;	ADJUSTABLE PARAMETER CHANGES, before "forever" loop and before clock starts
	lcall InitParameters
Adjust_Parameters:
	jb SOAK_TEMP_BUTTON, Soak_time_check
	wait_milli_seconds(#50)
	jb SOAK_TEMP_BUTTON, Soak_time_check
	jnb SOAK_TEMP_BUTTON, $
	ljmp Adjust_soak_temp_booster						; Soak temp button pressed -- adjust it
Soak_time_check:
	jb SOAK_TIME_BUTTON, Reflow_time_check
	wait_milli_seconds(#50)
	jb SOAK_TIME_BUTTON, Reflow_time_check
	jnb SOAK_TIME_BUTTON, $
	ljmp Adjust_soak_time_booster						; Soak time button pressed -- adjust it
Reflow_time_check:
	jb REFLOW_TEMP_BUTTON, Reflow_temp_check
	wait_milli_seconds(#50)
	jb REFLOW_TEMP_BUTTON, Reflow_temp_check
	jnb REFLOW_TEMP_BUTTON, $
	ljmp Adjust_reflow_temp_booster						; Reflow temp button pressed -- adjust it
Reflow_temp_check:
	jb REFLOW_TIME_BUTTON, No_Parameter_Adjustment
	wait_milli_seconds(#50)
	jb REFLOW_TIME_BUTTON, No_Parameter_Adjustment
	jnb REFLOW_TIME_BUTTON, $
	ljmp Adjust_reflow_time_booster						; Reflow time button pressed -- adjust it
	
No_Parameter_Adjustment:
	clr start_button_flag
	
;	*******Display parameters here if you want to check them before starting program*******
	
;------------------------------------
	lcall varDisplay
;------------------------------------
	
	Wait_Milli_Seconds(#50)	
	jnb START_BUTTON, Final_Start_Loop
	ljmp Adjust_Parameters
	
Final_Start_Loop:
;	setb state0flag ; *************NEW*******************
;	jb state0flag, Begin_reflow ; *************NEW*******************
	setb start_button_flag
	jbc start_button_flag, Begin_reflow
	ljmp Adjust_Parameters
	
Begin_reflow:
	lcall Timer2_Init		; Start clock
	lcall Timer0_Init		; Enable Timer 0
	
	writecommand(#0x01)     ; Clears the screen!
	mov r2, #2
	wait_milli_seconds(#250)
	setb half_seconds_flag
	mov Seconds, #0x00
	mov Minutes, #0x00
	mov Soak_Timer, #0x00
	mov Check_Temp, #0x50
	mov ten_second_decrementer, #9
	mov pwm_counter, #10
	lcall Display
    setb EA					; Enable all interupts
	

	
;------------------------------------------------------------------------------------------------------------------------------------------
 		
		
		
Forever:		
	clr TR0
	clr CE_ADC
	mov R0, #00000001B ; Start bit:1
	lcall DO_SPI_G
	mov R0, #10000000B ; Single ended, read channel 0
	lcall DO_SPI_G
	mov a, R1 ; R1 contains bits 8 and 9
	anl a, #00000011B ; We need only the two least significant bits
	mov Result+1, a ; Save result high.
	mov x+1, a
	
	mov R0, #55H ; It doesn't matter what we transmit...
	lcall DO_SPI_G
	mov Result, R1 ; R1 contains bits 0 to 7. Save result low.
	mov x, R1
	mov x+2, #0
	mov x+3, #0
	setb CE_ADC
	Wait_Milli_Seconds(#250)		;Delay
	Wait_Milli_Seconds(#250)
	Wait_Milli_Seconds(#250)
	Wait_Milli_Seconds(#250)
	lcall Convert_Result		;convert result to BCD


;**************************************
;		PULSE WIDTH MODULATION
	ljmp Check_half_seconds	
Returning:
;**************************************
;		CHECK TEMPERATURE AT 60SEC 
	mov a, Minutes
	cjne a, #0x01, display_temp
	mov a, Seconds
	cjne a, #0x00, display_temp
	clr mf
	mov x+0, bcd+0
	mov x+1, bcd+1
	Load_y(Check_Temp)
	lcall x_gteq_y
	;compare_var(Check_Temp+0, Check_Temp+1, x_gteq_y)
	jnb mf, Error_Loop
	
	; Display temperature to LCD
display_temp:
  	Set_Cursor(2,6)
	Display_BCD(bcd+1)
	Set_Cursor(2,8)
	Display_BCD(bcd)
	mov a, bcd
	lcall begin_state
	jb take_PCB, Jump_End_Reflow
	ljmp loop_a

Jump_End_Reflow:
	ljmp End_Reflow
	
;CheckStartButton:
;	jnb START_BUTTON, Error_Loop
;	Wait_Milli_Seconds(#50)	
;	ret
	
Error_Loop:
	writecommand(#0x01)
	mov r2, #2
	Wait_Milli_Seconds(#50)
	Set_Cursor(1,1)
    Send_Constant_String(#Stop_message)
	Set_Cursor(2,1)
	Send_Constant_String(#Check_message)
	ljmp floop
	
Kill_Loop:
	writecommand(#0x01)
	mov r2, #2
	Wait_Milli_Seconds(#250)
	Set_Cursor(1,1)
    Send_Constant_String(#Stop_message)
	ljmp floop
	
floop:
	ljmp floop

Finish_Kill:
	jnb START_BUTTON, ProgHalted
	sjmp Finish_Kill
	
ProgHalted:
	writecommand(#0x01)
	mov r2, #2        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
 	Wait_Milli_Seconds(#250)
	Set_Cursor(1,1)
	Send_Constant_String(#Stop_message)
	ljmp floop	
	
End_Reflow:
	writecommand(#0x01)
	mov r2, #2     
 	Wait_Milli_Seconds(#250)
	Set_Cursor(1,1)
	Send_Constant_String(#End_message)
	ljmp floop	
			
Convert_Result:
	 Load_y(5042000)
	 lcall mul32
	 Load_y(1023)
	 lcall div32
	 Load_y(221)
	 lcall div32
	 Load_y(41)
	 lcall div32
	 lcall hex2bcd
	 Send_BCD(bcd+1)
	 Send_BCD(bcd)
	 mov a, #'\n'
	 lcall putchar
	 mov a, #'\r'
	 lcall putchar	
	 ret
	 
Return_Forever:
	ret
	
loop_a:
	jnb half_seconds_flag, Jump_Forever
loop_b:
    clr half_seconds_flag ; We clear this flag in the main loop, but it is set in the ISR for timer 2
	Set_Cursor(1, 9)     ; the place in the LCD where we want the BCD counter value
	Display_BCD(Seconds) ; This macro is also in 'LCD_4bit.inc'
	Set_Cursor(1,6)
	Display_BCD(Minutes)   
Jump_Forever:
	ljmp Forever
	
END

