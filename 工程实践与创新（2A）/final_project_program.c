/*

Before code:

I was mistaken 0.5hz for 1hz, because I intended to make the led turn on for 1 sec and shut down for 1sec, 
that is 0.5 hz rather than 1hz. so all below when i metion 1hz for flashing, it is actually 0.5hz;
when i metion 2hz for flashing, it is actually 1hz.

Sang Rui, July 5th, 2020

*/


#include <stdint.h>
#include <stdbool.h>
#include "hw_memmap.h"
#include "debug.h"
#include "gpio.h"
#include "hw_i2c.h"
#include "hw_types.h"
#include "i2c.h"
#include "pin_map.h"
#include "sysctl.h"
#include "systick.h"
#include "interrupt.h"
#include "uart.h"
#include "hw_ints.h"
#include "string.h"
#include "math.h"

#define SYSTICK_FREQUENCY		1000			//1000hz
#define	I2C_FLASHTIME			500				//500mS
#define GPIO_FLASHTIME			300				//300mS
//*****************************************************************************
//
//I2C GPIO chip address and resigster define
//
//*****************************************************************************
#define TCA6424_I2CADDR 					0x22
#define PCA9557_I2CADDR						0x18

#define PCA9557_INPUT							0x00
#define	PCA9557_OUTPUT						0x01
#define PCA9557_POLINVERT					0x02
#define PCA9557_CONFIG						0x03

#define TCA6424_CONFIG_PORT0			0x0c
#define TCA6424_CONFIG_PORT1			0x0d
#define TCA6424_CONFIG_PORT2			0x0e

#define TCA6424_INPUT_PORT0				0x00
#define TCA6424_INPUT_PORT1				0x01
#define TCA6424_INPUT_PORT2				0x02

#define TCA6424_OUTPUT_PORT0			0x04
#define TCA6424_OUTPUT_PORT1			0x05
#define TCA6424_OUTPUT_PORT2			0x06

// basic functions defined
void 		Delay(uint32_t value);
void 		S800_GPIO_Init(void);
uint8_t 	I2C0_WriteByte(uint8_t DevAddr, uint8_t RegAddr, uint8_t WriteData);
uint8_t 	I2C0_ReadByte(uint8_t DevAddr, uint8_t RegAddr);
void		S800_I2C0_Init(void);
void 		S800_UART_Init(void);
void 		UART0_Handler(void);
void		SysTick_Handler(void);
void 		UARTStringPut(const char *cMessage);

//systick software counter define
volatile uint16_t   systick_1000ms_counter, systick_500ms_counter;
volatile uint8_t	systick_1000ms_state, systick_500ms_state; // typo~~~~~~~ status should be corrected to state
volatile uint8_t    result, cnt, key_value, gpio_status;

volatile  uint8_t sw; // the sw1~8 status
uint32_t ui32SysClock;
uint8_t seg7[] = {0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x77,0x7c,0x58,0x5e,0x079,0x71,0x5c, 0x40}; // 0x40 is for the "-"

// used in io ports communication
char receive_data[100];
char *str = receive_data;
uint32_t pos = 0;

// variables that defined by me
uint8_t inthour = 0;
uint8_t intminute = 0;
uint8_t intsecond = 0;
int intyear = 2020;
int intmonth = 0;
uint8_t intdate = 0;
uint8_t inthour_clk1 = 0;
uint8_t inthour_clk2 = 0;
uint8_t intminute_clk1 = 0;
uint8_t intminute_clk2 = 0;

char hour[3] = "00";
char minute[3] = "00";
char second[3] = "00";
char month[3] = "00"; 
char date[3] = "00";
char year[5] = "0000";
char hour_clk1[3] = "00";
char hour_clk2[3] = "00";
char minute_clk1[3] = "00";
char minute_clk2[3] = "00";

int monthdate[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; // index 0~11

// functions defined by me
void optimize(void);
void display_id(void);
void display_time(void);
void display_date(void);
void display_clk_one(void);
void display_clk_two(void);
void display_clk_one_sethour(void);
void display_clk_two_sethour(void);
void display_time_sethour(void);
void display_clk_one_setminute(void);
void display_clk_two_setminute(void);
void display_time_setminute(void);
void display_setmonth(void);
void display_setdate(void);
void flash_time_onehz(void);
void flash_time_twohz(void);

// initial clock state and time display state
bool state_clk1 = 0; 
bool state_clk2 = 0;
bool state_24 = 0; // 1 for 12hours

uint8_t pre = 0x0ff; 
int sw_one_cnt = 0; // record how many times sw1 being pressed

// used in led flashing
bool one_sec = 0;
bool half_sec = 0;

volatile uint8_t working_pattern = 0x0fe;

int time_cnt_15sec = 0;

// press usr-sw1 to shutdown display
bool shutdown_disp = 0;
uint32_t read_key = 1;
uint32_t pre_read_key = 1;

int main(void)
{

	// set initial time
	intmonth = 7;
	intdate = 7;
	inthour = 13;
	intminute = 13;
	intsecond = 50;
	inthour_clk1 = 13;
	inthour_clk2 = 18;
	intminute_clk1 = 14;
	intminute_clk2 = 14;

	/**********   basic initialize   **********/
	ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_16MHZ |SYSCTL_OSC_INT | SYSCTL_USE_PLL |SYSCTL_CFG_VCO_480), 20000000);
	
    SysTickPeriodSet(ui32SysClock/1000); // set sysTickPeriod 1ms
	SysTickEnable();
	SysTickIntEnable();
    IntMasterEnable();		
	
	S800_GPIO_Init();
	S800_I2C0_Init();
	S800_UART_Init();
	
	IntEnable(INT_UART0);
	UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
	IntMasterEnable();

	// PWM_Init();
	
	/*
	// set priority
	uint32_t ui32SysClock,ui32IntPriorityGroup,ui32IntPriorityMask;
	uint32_t ui32IntPrioritySystick,ui32IntPriorityUart0;
	ui32IntPriorityMask				= IntPriorityMaskGet();
	IntPriorityGroupingSet(3);														//Set all priority to pre-emtption priority
	
    IntPrioritySet(INT_UART0,0x0E0);													//Set INT_UART0 to lowest priority
    IntPrioritySet(FAULT_SYSTICK,0x00);									//Set INT_SYSTICK to highest priority
	
	ui32IntPriorityGroup			= IntPriorityGroupingGet();
	ui32IntPriorityUart0			= IntPriorityGet(INT_UART0);
	ui32IntPrioritySystick		= IntPriorityGet(FAULT_SYSTICK);
	*/


	result = I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_OUTPUT_PORT2, 0); // clear all the segment to avoid the sticking image
	
	// display student id
	display_id();

	while(1)
	{
		if (systick_1000ms_state == 1)
		{
			one_sec = !one_sec;

			intsecond++;
			time_cnt_15sec++;
			optimize();
			systick_1000ms_state = 0;
		}

		if (systick_500ms_state == 1)
		{
			half_sec = !half_sec;
			systick_500ms_state = 0;
		}
		

		// one key shut down display or turn on display
		read_key = GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_0);
		if (pre_read_key != read_key)
		{
			pre_read_key = read_key;
			if (!read_key)
			{
				shutdown_disp = !shutdown_disp;
			}
		}
		if (shutdown_disp)
		{
			I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_OUTPUT_PORT2, 0);
		}


		/************                                   whether bell ring                             *****************/
		// press any sw to disable clock ringing
		while (inthour == inthour_clk1 && intminute == intminute_clk1 && state_clk1)
		{
			if(shutdown_disp) 
			{
				shutdown_disp = 0;
			}

			if (systick_1000ms_state == 1)
			{
				one_sec = !one_sec;

				intsecond++;
				time_cnt_15sec++;
				optimize();
				systick_1000ms_state = 0;
			}
			if (systick_500ms_state == 1)
			{
				half_sec = !half_sec;
				systick_500ms_state = 0;
			}

			// ring the bell for 1hz
			GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_5, GPIO_PIN_5);	

			// flash time display for 1hz
			flash_time_onehz();

			// flash led 1 for 1hz
			if (one_sec)
			{
				I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT, 0x0fe);
			}
			else
			{
				I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT, 0x0ff);
			}

			// if pressed, disable clock one 
			sw = I2C0_ReadByte(TCA6424_I2CADDR, TCA6424_INPUT_PORT0);
			if(sw != 0x0ff)
			{
				state_clk1 = 0;
			}
		}
		while (inthour == inthour_clk2 && intminute == intminute_clk2 && state_clk2)
		{
			if(shutdown_disp) 
			{
				shutdown_disp = 0;
			}

			if (systick_1000ms_state == 1)
			{
				one_sec = !one_sec;

				intsecond++;
				time_cnt_15sec++;
				optimize();
				systick_1000ms_state = 0;
			}
			if (systick_500ms_state == 1)
			{
				half_sec = !half_sec;
				systick_500ms_state = 0;
			}

			// ring the bell for 2hz
			GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_5, GPIO_PIN_5);

			// flash time display for 2hz
			flash_time_twohz();

			// flash led 2 for 2hz
			if (half_sec)
			{
				I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT, 0x0fd);
			}
			else
			{
				I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT, 0x0ff);
			}

			// if pressed, disable clock two
			sw = I2C0_ReadByte(TCA6424_I2CADDR, TCA6424_INPUT_PORT0);
			if(sw != 0x0ff)
			{
				state_clk2 = 0;
			}
		}

		GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_5, 0);
		sw = I2C0_ReadByte(TCA6424_I2CADDR, TCA6424_INPUT_PORT0);
		// make sure pressed and different from previous press
		if (pre != sw) 
		{
			pre = sw;
			
			// sw 1 pressed
			if (sw == 0x0fe)
			{
				sw_one_cnt++;
				if(sw_one_cnt == 6)
				{
					sw_one_cnt = 0;
				}
			}

			// sw2 pressed:set hours
			else if(sw == 0x0fd)
			{
				if(sw_one_cnt == 2) // set clock , turn on led 1(archived below outside big if)
				{
					inthour_clk1++;
					optimize();
				}
				else if(sw_one_cnt == 3) // set clock 2, turn on led 2
				{
					inthour_clk2++;
					optimize();

				}
				else if(sw_one_cnt == 4) // set current time
				{
					inthour++;
					optimize();
				}
			}

			// sw3 pressed:set minutes
			else if(sw == 0x0fb)
			{
				if(sw_one_cnt == 2) // set clock 1
				{
					intminute_clk1++;
					optimize();
				}
				else if(sw_one_cnt == 3) // set clock 2
				{
					intminute_clk2++;
					optimize();

				}
				else if(sw_one_cnt == 4) // set current time
				{
					intminute++;
					optimize();
				}
			}

			// sw4 pressed:set month
			else if(sw == 0x0f7)
			{
				intmonth++;
				optimize();
			}

			// sw5 pressed:set date
			else if(sw == 0x0ef)
			{
				intdate++;
				optimize();
			}

			// sw6 pressed:enabel/disabele clock 1
			else if(sw == 0x0df)
			{
				state_clk1 = !state_clk1;
				sw = 0x0fe;
			}

			// sw7 pressed::enabel/disabele clock 2
			else if(sw == 0x0bf)
			{
				state_clk2 = !state_clk2;
				sw = 0x0fe;
			}

			// sw8 pressed:set display pattern
			else if(sw == 0x7f)
			{
				state_24 = !state_24;
				sw = 0x0fe;
			}
		} // the end of big if to count the press times and set variables respectively


		/*********************************   show segment and led according to the variables:   ********************************/
		// find the correct working_pattern
		if(sw != 0x0ff)
		{
			working_pattern = pre;
			time_cnt_15sec = 0;
		}

		// if not pressed for 15sec, go to display time
		if(time_cnt_15sec == 15)
		{
			working_pattern == 0x0fe;
			sw_one_cnt = 0;
		}

		// display according to working pattern and sw_one_cnt
		if(working_pattern == 0x0fe && !shutdown_disp)
		{
			// I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT, 0x0ff); // shut down all leds
			switch (sw_one_cnt)
			{
				case 0:
					display_time();
					break;
				case 1:
					display_date();
					break;
				case 2:
					display_clk_one();
					break;
				case 3:
					display_clk_two();
					break;
				case 4:
					display_time();
					break;
				case 5:
					display_date();
			}
		}
		else if(working_pattern == 0x0fd && !shutdown_disp)
		{
			if(sw_one_cnt == 2)
			{
				display_clk_one_sethour();
				// turn on led1 
				I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT, 0x0fe);	
			}
			else if(sw_one_cnt == 3)
			{
				display_clk_two_sethour();
				// turn on led2
				I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT, 0x0fd);
			}
			else if(sw_one_cnt == 4)
			{
				display_time_sethour();
				// flash led3 for 1hz
				if (one_sec)
				{
					I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT, 0x0fb);
				}
				else
				{
					I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT, 0x0ff);
				}
			}
		}
		else if(working_pattern == 0x0fb && !shutdown_disp)
		{
			if(sw_one_cnt == 2)
			{
				display_clk_one_setminute();
				// turn on led1 
				I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT, 0x0fe);
			}
			else if(sw_one_cnt == 3)
			{
				display_clk_two_setminute();
				// turn on led2
				I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT, 0x0fd);
			}
			else if(sw_one_cnt == 4)
			{
				display_time_setminute();
				// flash led3 for 1hz
				if (one_sec)
				{
					I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT, 0x0fb);
				}
				else
				{
					I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT, 0x0ff);
				}
			}
		}
		else if(working_pattern == 0x0f7 && !shutdown_disp)
		{
			display_setmonth();
			// flash led4
			if (one_sec)
			{
				I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT, 0x0f7);
			}
			else
			{
				I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT, 0x0ff);
			}
		}
		else if(working_pattern == 0x0ef && !shutdown_disp)
		{
			display_setdate();
			// flash led4
			if (one_sec)
			{
				I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT, 0x0f7);
			}
			else
			{
				I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT, 0x0ff);
			}
		}
		else
		{
			if(!shutdown_disp)
				display_time();
		}

		// ???????,?????led??,???????????? 0xXX ????????if????????????,???????????????
		// ?????????……??
		// flash led1 for 1hz, according to "state_clk1"
		if (state_clk1 && !shutdown_disp)
		{
			if (one_sec)
			{
				I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT, 0x0fe);
			}
			else
			{
				I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT, 0x0ff);
			}
		}
		else
		{
			I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT, 0x0ff);
		}
		
		// flash led2 for 1hz, according to "state_clk2"
		if (state_clk2 && !shutdown_disp)
		{
			if (one_sec)
			{
				I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT, 0x0fd);
			}
			else
			{
				I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT, 0x0ff);
			}
		}
		else
		{
			I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT, 0x0ff);
		}


		// turn led on or shut down, according to "state_24"
		// turn on led5,indicating PM
		if (state_24 && inthour>=12 && !shutdown_disp)
		{
			I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT, 0x0ef);
		}


		/***********     UART part, serial port     *****************/
		if (pos != 0)
		{
			if (!strncmp(str, "GETDATE", 7))
			{
				// show date on serial port
				UARTStringPut("DATE");
				UARTStringPut(year);
				UARTCharPutNonBlocking(UART0_BASE, ':');
				UARTStringPut(month);
				UARTCharPutNonBlocking(UART0_BASE, ':');
				UARTStringPut(date);
			}
			else if (!strncmp(str, "GETTIME", 7))
			{
				// show timw on serial port
				UARTStringPut("TIME");
				UARTStringPut(hour);
				UARTCharPutNonBlocking(UART0_BASE, ':');
				UARTStringPut(minute);
				UARTCharPutNonBlocking(UART0_BASE, ':');
				UARTStringPut(second);
			}
			else if (!strncmp(str, "SETDATE", 7))
			{
				// set date by serial port
				intyear = 1000 * (str[7] - '0') + 100 * (str[8] - '0') + 10 * (str[9] - '0') + (str[10] - '0');
				intmonth = 10 * (str[12] - '0') + (str[13] - '0');
				intdate = 10 * (str[15] - '0') + (str[16] - '0');
				optimize();
				// show date on serial port
				UARTStringPut("DATE");
				UARTStringPut(year);
				UARTCharPutNonBlocking(UART0_BASE, ':');
				UARTStringPut(month);
				UARTCharPutNonBlocking(UART0_BASE, ':');
				UARTStringPut(date);
			}
			else if (!strncmp(str, "SETTIME", 7))
			{
				// set time by serial port
				inthour = 10 * (str[7] - '0') + str[8] - '0';
				intminute = 10 * (str[10] - '0') + str[11] - '0';
				intsecond = 10 * (str[13] - '0') + str[14] - '0';
				optimize();
				// show timw on serial port
				UARTStringPut("TIME");
				UARTStringPut(hour);
				UARTCharPutNonBlocking(UART0_BASE, ':');
				UARTStringPut(minute);
				UARTCharPutNonBlocking(UART0_BASE, ':');
				UARTStringPut(second);
			}
			else if (!strncmp(str, "SETALM1", 7))
			{
				// set alarm 1 by serial port
				inthour_clk1 = 10 * (str[7] - '0') + str[8] - '0';
				intminute_clk1 = 10 * (str[10] - '0') + str[11] - '0';
				optimize();
				// show timw on serial port
				UARTStringPut("ALARM1 TIME");
				UARTStringPut(hour_clk1);
				UARTCharPutNonBlocking(UART0_BASE, ':');
				UARTStringPut(minute_clk1);
			}
			else if (!strncmp(str, "SETALM2", 7))
			{
				// set alarm 2 by serial port
				inthour_clk2 = 10 * (str[7] - '0') + str[8] - '0';
				intminute_clk2 = 10 * (str[10] - '0') + str[11] - '0';
				optimize();
				// show timw on serial port
				UARTStringPut("ALARM2 TIME");
				UARTStringPut(hour_clk2);
				UARTCharPutNonBlocking(UART0_BASE, ':');
				UARTStringPut(minute_clk2);
			}
			else if (!strncmp(str, "RESET", 5))
			{
				// reset whole program
				/******************************************                 to be done                 **************************************/
			}
			else 
			{
				UARTStringPut("Illegal!\r\n");
			}

			pos = 0;
			receive_data[0] = '\0';
		}




	} // the end of while()



} // the end of the main function


// deal with the over range problem
void optimize(void)
{
	while (intsecond > 59)
	{
		intsecond -= 60;
		intminute++;
	}
	while (intminute > 59)
	{
		intminute -= 60;
		inthour++;
	}
	while (inthour > 23)
	{
		inthour -= 24;
		intdate++;
	}
	while (intdate > monthdate[intmonth - 1])
	{
		intdate -= monthdate[intmonth - 1];
		intmonth++;
	}
	while(intmonth > 12)
	{
		intmonth = intmonth - 14; // ??????????????????? why -12 wont work but -14 works well??????
		intyear++;
	}
	while(intminute_clk1 > 59)
	{
		intminute_clk1 -= 60;
		inthour_clk1++;
	}
	while(inthour_clk1 > 23)
	{
		inthour_clk1 -= 24;
	}
	while(intminute_clk2 > 59)
	{
		intminute_clk2 -= 60;
		inthour_clk2++;
	}
	while(inthour_clk2 > 23)
	{
		inthour_clk2 -= 24;
	}

	// char, 0 represent for tens digit, 1 for units digit
	// used in uart and usb port
	second[1] = intsecond % 10 + '0';
	second[0] = (int)(floor(intsecond / 10)) + '0';
	minute[1] = intminute % 10 + '0';
	minute[0] = (int)(floor(intminute / 10)) + '0';
	hour[1] = inthour % 10 + '0';
	hour[0] = (int)(floor(inthour / 10)) + '0';
	date[1] = intdate % 10 + '0';
	date[0] = (int)(floor(intdate / 10)) + '0';
	month[1] = intmonth % 10 + '0';
	month[0] = (int)(floor(intmonth / 10)) + '0';
	hour_clk1[1] = inthour_clk1 % 10 + '0';
	hour_clk1[0] = (int)(floor(inthour_clk1 / 10)) + '0';
	hour_clk2[1] = inthour_clk2 % 10 + '0';
	hour_clk2[0] = (int)(floor(inthour_clk2 / 10)) + '0';
	minute_clk1[1] = intminute_clk1 % 10 + '0';
	minute_clk1[0] = (int)(floor(intminute_clk1 / 10)) + '0';
	minute_clk2[1] = intminute_clk2 % 10 + '0';
	minute_clk2[0] = (int)(floor(intminute_clk2 / 10)) + '0';
	year[0] = (int)(floor(intyear / 1000)) + '0';
	year[1] = (int)(floor((intyear % 1000) / 100)) + '0';
	year[2] = (int)(floor((intyear % 100) / 10)) + '0';
	year[3] = (int)(floor(intyear % 10)) + '0';
}


void display_id(void)
{
	systick_1000ms_state = 0;
	systick_1000ms_counter = 1000;
	while(systick_1000ms_state != 1)
	{ 
		switch (systick_1000ms_counter % 8) // from left to right, is low bit to high bit
		{
			case 0:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[2]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x01);					//write port 2		 the most left segment
				break;
			case 1:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[1]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x02);					//write port 2		the second segnment from left
				break;
			case 2:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[9]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x04);					//write port 2		 the 3rd segnment  from left
				break;
			case 3:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[1]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x08);					//write port 2		the 4th segment from left
				break;
			case 4:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[1]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x10);					//write port 2		the 5th segment from left
				break;
			case 5:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[0]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x20);					//write port 2		
				break;
			case 6:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[8]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x40);			
				break;
			case 7:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[5]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x80);					
				break;
		}
	}
}


void display_time(void)
{
	uint8_t inthour_disp;
	inthour_disp = inthour;

	if(state_24 && inthour >= 12)
	{
		inthour_disp -= 12;
		//I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT, 0x0ef);
	}

	switch (systick_1000ms_counter % 7) // from left to right, is low bit to high bit
	{
		case 0:
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(inthour_disp / 10))]);						//write port 1 				
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x01);					//write port 2		 the most left segment
			break;
		case 1:
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[inthour_disp % 10]);						//write port 1 				
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x02);					//write port 2		the second segnment from left
			break;
		case 2:
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[17]);						//write port 1 				
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x24);					//write port 2		 the 3rd segnment and the 6th segment from left
			break;
		case 3:
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(intminute / 10))]);						//write port 1 				
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x08);					//write port 2		the 4th segment from left
			break;
		case 4:
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[intminute % 10]);						//write port 1 				
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x10);					//write port 2		the 5th segment from left
			break;
		case 5:
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(intsecond / 10))]);						//write port 1 				
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x40);					//write port 2		
			break;
		case 6:
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[intsecond % 10]);						//write port 1 				
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x80);					//write port 2 the right most segment	
			break;
	}
}


void display_date(void)
{
	switch (systick_1000ms_counter % 8) // from left to right, is low bit to high bit
	{
		case 0:
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(intyear / 1000))]);						//write port 1 				
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x01);					//write port 2		 the most left segment
			break;
		case 1:
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(intyear%1000)/100]);						//write port 1 				
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x02);					//write port 2		the second segnment from left
			break;
		case 2:
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(intyear%100)/10]);						//write port 1 				
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x04);					//write port 2		 the 3rd segnment from left
			break;
		case 3:
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[intyear%10]);						//write port 1 				
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x08);					//write port 2		the 4th segment from left
			break;
		case 4:
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(intmonth / 10))]);						//write port 1 				
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x10);					//write port 2		the 5th segment from left
			break;
		case 5:
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[intmonth % 10]);						//write port 1 				
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x20);					//write port 2		 the 3rd segnment from left
			break;
		case 6:
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(intdate / 10))]);						//write port 1 				
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x40);					//write port 2		the 4th segment from left
			break;
		case 7:
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[intdate % 10]);						//write port 1 				
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x80);					//write port 2		the 5th segment from left
			break;
	}
}


void display_clk_one(void)
{
	uint8_t inthour_disp;
	inthour_disp = inthour_clk1;

	switch (systick_1000ms_counter % 5) // from left to right, is low bit to high bit
	{
		case 0:
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(inthour_disp / 10))]);						//write port 1 				
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x01);					//write port 2		 the most left segment
			break;
		case 1:
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[inthour_disp % 10]);						//write port 1 				
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x02);					//write port 2		the second segnment from left
			break;
		case 2:
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[17]);						//write port 1 				
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x04);					//write port 2		 the 3rd segnment from left
			break;
		case 3:
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(intminute_clk1 / 10))]);						//write port 1 				
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x08);					//write port 2		the 4th segment from left
			break;
		case 4:
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[intminute_clk1 % 10]);						//write port 1 				
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x10);					//write port 2		the 5th segment from left
			break;
	}
}


void display_clk_two(void)
{
	uint8_t inthour_disp;
	inthour_disp = inthour_clk2;

	switch (systick_1000ms_counter % 5) // from left to right, is low bit to high bit
	{
		case 0:
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(inthour_disp / 10))]);						//write port 1 				
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x01);					//write port 2		 the most left segment
			break;
		case 1:
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[inthour_disp % 10]);						//write port 1 				
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x02);					//write port 2		the second segnment from left
			break;
		case 2:
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[17]);						//write port 1 				
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x04);					//write port 2		 the 3rd segnment from left
			break;
		case 3:
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(intminute_clk2 / 10))]);						//write port 1 				
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x08);					//write port 2		the 4th segment from left
			break;
		case 4:
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[intminute_clk2 % 10]);						//write port 1 				
			result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x10);					//write port 2		the 5th segment from left
			break;
	}
}


void display_clk_one_sethour(void)
{
	uint8_t inthour_disp;
	inthour_disp = inthour_clk1;

	if (one_sec)
	{
		switch (systick_1000ms_counter % 5) // from left to right, is low bit to high bit
		{
			case 0:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(inthour_disp / 10))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x01);					//write port 2		 the most left segment
				break;
			case 1:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[inthour_disp % 10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x02);					//write port 2		the second segnment from left
				break;
			case 2:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[17]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x04);					//write port 2		 the 3rd segnment from left
				break;
			case 3:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(intminute_clk1 / 10))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x08);					//write port 2		the 4th segment from left
				break;
			case 4:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[intminute_clk1 % 10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x10);					//write port 2		the 5th segment from left
				break;
		}
	}
	else
	{
		switch (systick_1000ms_counter % 3) // from left to right, is low bit to high bit
		{
			case 0:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[17]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x04);					//write port 2		 the 3rd segnment from left
				break;
			case 1:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(intminute_clk1 / 10))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x08);					//write port 2		the 4th segment from left
				break;
			case 2:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[intminute_clk1 % 10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x10);					//write port 2		the 5th segment from left
				break;
		}
	}
}


void display_clk_two_sethour(void)
{
	uint8_t inthour_disp;
	inthour_disp = inthour_clk2;

	if (one_sec)
	{
		switch (systick_1000ms_counter % 5) // from left to right, is low bit to high bit
		{
			case 0:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(inthour_disp / 10))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x01);					//write port 2		 the most left segment
				break;
			case 1:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[inthour_disp % 10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x02);					//write port 2		the second segnment from left
				break;
			case 2:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[17]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x04);					//write port 2		 the 3rd segnment from left
				break;
			case 3:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(intminute_clk2 / 10))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x08);					//write port 2		the 4th segment from left
				break;
			case 4:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[intminute_clk2 % 10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x10);					//write port 2		the 5th segment from left
				break;
		}
	}
	else
	{
		switch (systick_1000ms_counter % 3) // from left to right, is low bit to high bit
		{
			case 0:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[17]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x04);					//write port 2		 the 3rd segnment from left
				break;
			case 1:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(intminute_clk2 / 10))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x08);					//write port 2		the 4th segment from left
				break;
			case 2:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[intminute_clk2 % 10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x10);					//write port 2		the 5th segment from left
				break;
		}
	}
}


void display_time_sethour(void)
{
	uint8_t inthour_disp;
	inthour_disp = inthour;

	if(state_24 && inthour >= 12)
	{
		inthour_disp -= 12;
	}

	if (one_sec)
	{
		switch (systick_1000ms_counter % 5) // from left to right, is low bit to high bit
		{
			case 0:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(inthour_disp / 10))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x01);					//write port 2		 the most left segment
				break;
			case 1:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[inthour_disp % 10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x02);					//write port 2		the second segnment from left
				break;
			case 2:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[17]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x04);					//write port 2		 the 3rd segnment from left
				break;
			case 3:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(intminute / 10))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x08);					//write port 2		the 4th segment from left
				break;
			case 4:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[intminute % 10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x10);					//write port 2		the 5th segment from left
				break;
		}
	}
	else
	{
		switch (systick_1000ms_counter % 3) // from left to right, is low bit to high bit
		{
			case 0:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[17]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x04);					//write port 2		 the 3rd segnment from left
				break;
			case 1:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(intminute / 10))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x08);					//write port 2		the 4th segment from left
				break;
			case 2:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[intminute % 10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x10);					//write port 2		the 5th segment from left
				break;
		}
	}
}


void display_clk_one_setminute(void)
{
	uint8_t inthour_disp;
	inthour_disp = inthour_clk1;

	if (one_sec)
	{
		switch (systick_1000ms_counter % 5) // from left to right, is low bit to high bit
		{
			case 0:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(inthour_disp / 10))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x01);					//write port 2		 the most left segment
				break;
			case 1:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[inthour_disp % 10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x02);					//write port 2		the second segnment from left
				break;
			case 2:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[17]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x04);					//write port 2		 the 3rd segnment from left
				break;
			case 3:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(intminute_clk1 / 10))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x08);					//write port 2		the 4th segment from left
				break;
			case 4:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[intminute_clk1 % 10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x10);					//write port 2		the 5th segment from left
				break;
		}
	}
	else
	{
		switch (systick_1000ms_counter % 3) // from left to right, is low bit to high bit
		{
			case 0:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(inthour_disp / 10))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x01);					//write port 2		 the most left segment
				break;
			case 1:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[inthour_disp % 10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x02);					//write port 2		the second segnment from left
				break;
			case 2:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[17]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x04);					//write port 2		 the 3rd segnment from left
				break;
		}
	}
}


void display_clk_two_setminute(void)
{
	uint8_t inthour_disp;
	inthour_disp = inthour_clk2;

	if (one_sec)
	{
		switch (systick_1000ms_counter % 5) // from left to right, is low bit to high bit
		{
			case 0:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(inthour_disp / 10))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x01);					//write port 2		 the most left segment
				break;
			case 1:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[inthour_disp % 10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x02);					//write port 2		the second segnment from left
				break;
			case 2:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[17]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x04);					//write port 2		 the 3rd segnment from left
				break;
			case 3:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(intminute_clk2 / 10))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x08);					//write port 2		the 4th segment from left
				break;
			case 4:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[intminute_clk2 % 10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x10);					//write port 2		the 5th segment from left
				break;
		}
	}
	else
	{
		switch (systick_1000ms_counter % 3) // from left to right, is low bit to high bit
		{
			case 0:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(inthour_disp / 10))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x01);					//write port 2		 the most left segment
				break;
			case 1:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[inthour_disp % 10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x02);					//write port 2		the second segnment from left
				break;
			case 2:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[17]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x04);					//write port 2		 the 3rd segnment from left
				break;
		}
	}
}


void display_time_setminute(void)
{
	uint8_t inthour_disp;
	inthour_disp = inthour;

	if(state_24 && inthour >= 12)
	{
		inthour_disp -= 12;
	}

	if (one_sec)
	{
		switch (systick_1000ms_counter % 5) // from left to right, is low bit to high bit
		{
			case 0:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(inthour_disp / 10))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x01);					//write port 2		 the most left segment
				break;
			case 1:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[inthour_disp % 10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x02);					//write port 2		the second segnment from left
				break;
			case 2:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[17]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x04);					//write port 2		 the 3rd segnment from left
				break;
			case 3:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(intminute / 10))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x08);					//write port 2		the 4th segment from left
				break;
			case 4:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[intminute % 10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x10);					//write port 2		the 5th segment from left
				break;
		}
	}
	else
	{
		switch (systick_1000ms_counter % 3) // from left to right, is low bit to high bit
		{
			case 0:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(inthour_disp / 10))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x01);					//write port 2		 the most left segment
				break;
			case 1:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[inthour_disp % 10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x02);					//write port 2		the second segnment from left
				break;
			case 2:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[17]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x04);					//write port 2		 the 3rd segnment from left
				break;
		}
	}
}


void display_setmonth(void)
{
	if (one_sec)
	{
		switch (systick_1000ms_counter % 8) // from left to right, is low bit to high bit
		{
			case 0:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(intyear / 1000))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x01);					//write port 2		 the most left segment
				break;
			case 1:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(intyear%1000)/100]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x02);					//write port 2		the second segnment from left
				break;
			case 2:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(intyear%100)/10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x04);					//write port 2		 the 3rd segnment from left
				break;
			case 3:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[intyear%10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x08);					//write port 2		the 4th segment from left
				break;
			case 4:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(intmonth / 10))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x10);					//write port 2		the 5th segment from left
				break;
			case 5:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[intmonth % 10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x20);					//write port 2		 the 3rd segnment from left
				break;
			case 6:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(intdate / 10))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x40);					//write port 2		the 4th segment from left
				break;
			case 7:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[intdate % 10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x80);					//write port 2		the 5th segment from left
				break;
		}
	}
	else
	{
		switch (systick_1000ms_counter % 6) // from left to right, is low bit to high bit
		{
			case 0:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(intyear / 1000))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x01);					//write port 2		 the most left segment
				break;
			case 1:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(intyear%1000)/100]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x02);					//write port 2		the second segnment from left
				break;
			case 2:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(intyear%100)/10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x04);					//write port 2		 the 3rd segnment from left
				break;
			case 3:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[intyear%10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x08);					//write port 2		the 4th segment from left
				break;
			case 4:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(intdate / 10))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x40);					//write port 2		the 4th segment from left
				break;
			case 5:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[intdate % 10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x80);					//write port 2		the 5th segment from left
				break;
		}
	}
}


void display_setdate(void)
{
	if (one_sec)
	{
		switch (systick_1000ms_counter % 8) // from left to right, is low bit to high bit
		{
			case 0:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(intyear / 1000))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x01);					//write port 2		 the most left segment
				break;
			case 1:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(intyear%1000)/100]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x02);					//write port 2		the second segnment from left
				break;
			case 2:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(intyear%100)/10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x04);					//write port 2		 the 3rd segnment from left
				break;
			case 3:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[intyear%10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x08);					//write port 2		the 4th segment from left
				break;
			case 4:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(intmonth / 10))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x10);					//write port 2		the 5th segment from left
				break;
			case 5:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[intmonth % 10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x20);					//write port 2		 the 3rd segnment from left
				break;
			case 6:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(intdate / 10))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x40);					//write port 2		the 4th segment from left
				break;
			case 7:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[intdate % 10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x80);					//write port 2		the 5th segment from left
				break;
		}
	}
	else
	{
		switch (systick_1000ms_counter % 6) // from left to right, is low bit to high bit
		{
			case 0:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(intyear / 1000))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x01);					//write port 2		 the most left segment
				break;
			case 1:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(intyear%1000)/100]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x02);					//write port 2		the second segnment from left
				break;
			case 2:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(intyear%100)/10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x04);					//write port 2		 the 3rd segnment from left
				break;
			case 3:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[intyear%10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x08);					//write port 2		the 4th segment from left
				break;
			case 4:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(intmonth / 10))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x10);					//write port 2		the 5th segment from left
				break;
			case 5:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[intmonth % 10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x20);					//write port 2		 the 3rd segnment from left
				break;
		}
	}
}


void flash_time_onehz(void)
{
	uint8_t inthour_disp;
	inthour_disp = inthour;

	if(state_24 && inthour >= 12)
	{
		inthour_disp -= 12;
		//I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT, 0x0ef);
	}

	if(one_sec)
	{
		switch (systick_1000ms_counter % 7) // from left to right, is low bit to high bit
		{
			case 0:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(inthour_disp / 10))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x01);					//write port 2		 the most left segment
				break;
			case 1:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[inthour_disp % 10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x02);					//write port 2		the second segnment from left
				break;
			case 2:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[17]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x24);					//write port 2		 the 3rd segnment and the 6th segment from left
				break;
			case 3:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(intminute / 10))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x08);					//write port 2		the 4th segment from left
				break;
			case 4:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[intminute % 10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x10);					//write port 2		the 5th segment from left
				break;
			case 5:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(intsecond / 10))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x40);					//write port 2		
				break;
			case 6:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[intsecond % 10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x80);					//write port 2 the right most segment	
				break;
		}
	}
	else
	{
		I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
	}
}


void flash_time_twohz(void)
{
	uint8_t inthour_disp;
	inthour_disp = inthour;

	if(state_24 && inthour >= 12)
	{
		inthour_disp -= 12;
		//I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT, 0x0ef);
	}

	if(half_sec)
	{
		switch (systick_1000ms_counter % 7) // from left to right, is low bit to high bit
		{
			case 0:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(inthour_disp / 10))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x01);					//write port 2		 the most left segment
				break;
			case 1:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[inthour_disp % 10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x02);					//write port 2		the second segnment from left
				break;
			case 2:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[17]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x24);					//write port 2		 the 3rd segnment and the 6th segment from left
				break;
			case 3:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(intminute / 10))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x08);					//write port 2		the 4th segment from left
				break;
			case 4:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[intminute % 10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x10);					//write port 2		the 5th segment from left
				break;
			case 5:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[(uint8_t)(floor(intsecond / 10))]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x40);					//write port 2		
				break;
			case 6:
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[intsecond % 10]);						//write port 1 				
				result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x80);					//write port 2 the right most segment	
				break;
		}
	}
	else
	{
		I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0);
	}
}


// void PWM_Init(void)
// {
//     SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
//  	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);

//   	GPIOPinConfigure(GPIO_PK5_M0PWM7);

//  	GPIOPinTypePWM(GPIO_PORTK_BASE, GPIO_PIN_5);
//  	PWMGenConfigure(PWM0_BASE, PWM_GEN_3, PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC); 
//  	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_3, 8000); 
//  	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, 8000 / 4);
//  	PWMOutputState(PWM0_BASE, PWM_OUT_7_BIT, true);
//  	PWMGenEnable(PWM0_BASE, PWM_GEN_3);
// }


void SysTick_Handler(void)
{
	if (systick_1000ms_counter	!= 0)
	{
		systick_1000ms_counter--;
	}
	else
	{
		systick_1000ms_counter	= 1000;
		systick_1000ms_state 	= 1;
	}
	
	if (systick_500ms_counter	!= 0)
	{
		systick_500ms_counter--;
	}
	else
	{
		systick_500ms_counter	= 500;
		systick_500ms_state 	= 1;
	}
}


void UART0_Handler(void)
{
    int32_t uart0_int_status;
    uart0_int_status 		= UARTIntStatus(UART0_BASE, true);
    UARTIntClear(UART0_BASE, uart0_int_status);

	// get receive_data
	pos = 0;
	while (UARTCharsAvail(UART0_BASE))
	{
		while(UARTCharsAvail(UART0_BASE))
		{
			receive_data[pos++] = (char)UARTCharGetNonBlocking(UART0_BASE);
		}
		Delay(600); // after few trials, 600 is perfect for this implement
	}

	receive_data[pos] = '\0';
	str = receive_data;	
}


void Delay(uint32_t value)
{
	uint32_t ui32Loop;
	for(ui32Loop = 0; ui32Loop < value; ui32Loop++){};
}


void UARTStringPut(const char *cMessage)
{
	while(*cMessage!='\0' && UARTSpaceAvail(UART0_BASE))
		UARTCharPut(UART0_BASE,*(cMessage++));
}


void S800_UART_Init(void)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);						//Enable PortA
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));			//Wait for the GPIO moduleA ready

	GPIOPinConfigure(GPIO_PA0_U0RX);												// Set GPIO A0 and A1 as UART pins.
    GPIOPinConfigure(GPIO_PA1_U0TX);    			

    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

	// Configure the UART for 115,200, 8-N-1 operation.
    UARTConfigSetExpClk(UART0_BASE, ui32SysClock,115200,(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |UART_CONFIG_PAR_NONE));
	//UARTStringPut((uint8_t *)"\r\nHello, world!\r\n");
}


void S800_GPIO_Init(void)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);						//Enable PortF
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));			//Wait for the GPIO moduleF ready
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);						//Enable PortJ	
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ));			//Wait for the GPIO moduleJ ready	
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);						//Enable PortN	
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION));			//Wait for the GPIO moduleN ready		
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);						// enable beeeeeeeeeeee
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOK));
	
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0);			//Set PF0 as Output pin
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1);			//Set PN0 as Output pin
    GPIOPinTypeGPIOOutput(GPIO_PORTK_BASE, GPIO_PIN_5);

	GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE,GPIO_PIN_0 | GPIO_PIN_1);//Set the PJ0,PJ1 as input pin
	GPIOPadConfigSet(GPIO_PORTJ_BASE,GPIO_PIN_0 | GPIO_PIN_1,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);
}


void S800_I2C0_Init(void)
{
	uint8_t result;
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	GPIOPinConfigure(GPIO_PB2_I2C0SCL);
    GPIOPinConfigure(GPIO_PB3_I2C0SDA);
    GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
    GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);

	I2CMasterInitExpClk(I2C0_BASE,ui32SysClock, true);										//config I2C0 400k
	I2CMasterEnable(I2C0_BASE);	

	result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_CONFIG_PORT0,0x0ff);		//config port 0 as input
	result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_CONFIG_PORT1,0x0);			//config port 1 as output
	result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_CONFIG_PORT2,0x0);			//config port 2 as output 

	result = I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_CONFIG,0x00);					//config port as output
	result = I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT,0x0ff);				//turn off the LED1-8
	
}


uint8_t I2C0_WriteByte(uint8_t DevAddr, uint8_t RegAddr, uint8_t WriteData)
{
	uint8_t rop;
	while(I2CMasterBusy(I2C0_BASE)){};
	I2CMasterSlaveAddrSet(I2C0_BASE, DevAddr, false);
	I2CMasterDataPut(I2C0_BASE, RegAddr);
	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);
	while(I2CMasterBusy(I2C0_BASE)){};
	rop = (uint8_t)I2CMasterErr(I2C0_BASE);

	I2CMasterDataPut(I2C0_BASE, WriteData);
	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
	while(I2CMasterBusy(I2C0_BASE)){};

	rop = (uint8_t)I2CMasterErr(I2C0_BASE);
	return rop;
}


uint8_t I2C0_ReadByte(uint8_t DevAddr, uint8_t RegAddr)
{
	uint8_t value,rop;
	while(I2CMasterBusy(I2C0_BASE)){};	
	I2CMasterSlaveAddrSet(I2C0_BASE, DevAddr, false);
	I2CMasterDataPut(I2C0_BASE, RegAddr);
//	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);		
	I2CMasterControl(I2C0_BASE,I2C_MASTER_CMD_SINGLE_SEND);
	while(I2CMasterBusBusy(I2C0_BASE));
	rop = (uint8_t)I2CMasterErr(I2C0_BASE);
	Delay(10);
	//receive data
	I2CMasterSlaveAddrSet(I2C0_BASE, DevAddr, true);
	I2CMasterControl(I2C0_BASE,I2C_MASTER_CMD_SINGLE_RECEIVE);
	while(I2CMasterBusBusy(I2C0_BASE));
	value=I2CMasterDataGet(I2C0_BASE);
		Delay(10);
	return value;
}

















