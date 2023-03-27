/****** Robert Li,  2023  


This starter kit: 

1. set up I2C and tested I2C and EEPROM

2. Configured RTC and RTC_Alarm and tested RTC


3 For External buttons:  
1):  The Fist External button (named extBtn1) seems can only be connected to PC1.  
	(cannot use pin PB1, for 429i-DISCO ,pb1 is used by LCD. if use this pin, always interrupt by itself
	cannot use pin PA1, used by gyro. if use this pin, never interrupt
		pd1----WILL ACT AS PC13, To trigger the RTC timestamp event)
					
2) the Second external button (extBtn2) may be conected to  PD2.  

		PA2: NOT OK. (USED BY LCD??)
		PB2: OK.
		PC2: ok, BUT sometimes (every 5 times around), press pc2 will trigger exti1, which is configured to use PC1. (is it because of using internal pull up pin config?)
		      however, press PC1 does not affect exti 2. sometimes press PC2 will also affect time stamp (PC13)
		PD2: OK,     
		PE2:  OK  (PE3, PE4 PE5 , seems has no other AF function, according to the table in the manual for discovery board)
		PF2: NOT OK. (although PF2 is used by SDRAM, it affects LCD. press it, LCD will flick and displayed chars change to garbage)
		PG2: OK
		 
*/


/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdio.h>

/** @addtogroup STM32F4xx_HAL_Examples
  * @{
  */

/** @addtogroup GPIO_EXTI
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define COLUMN(x) ((x) * (((sFONT *)BSP_LCD_GetFont())->Width))    //see font.h, for defining LINE(X)



/* Private macro -------------------------------------------------------------*/

 

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef  I2c3_Handle;

RTC_HandleTypeDef RTCHandle;
RTC_DateTypeDef RTC_DateStructure, read_RTC_DateStruct;
RTC_TimeTypeDef RTC_TimeStructure, read_RTC_TimeStruct;



HAL_StatusTypeDef Hal_status;  //HAL_ERROR, HAL_TIMEOUT, HAL_OK, of HAL_BUSY 


//memory location to write to in the device
__IO uint16_t memLocation = 0x000A; //pick any location within range
__IO uint16_t memstore = 0x0005;

uint8_t exbutton=0;
char lcd_buffer[14];

void RTC_Config(void);
void RTC_AlarmAConfig(void);

void ClearScreen(void);

void ExtBtn1_Config(void);  //for the first External button
void ExtBtn2_Config(void);

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void Error_Handler(void);

/* Private functions ---------------------------------------------------------*/
uint16_t holdstart;
uint16_t history;
uint16_t hiscount;
uint8_t adjust;
uint8_t adjuststate;
uint8_t up;
uint8_t increment;
uint8_t adjust_clean;
uint16_t changedirection;
/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  //the following variables are for testging I2C_EEPROM
	uint8_t state = 0;
	uint8_t data1 =0x67,  data2=0x68;
	uint8_t readData=0x00;
	char AA[34]= "ascjioacioencvdavadvadvasacaoinvas";
	uint8_t * bufferdata=(uint8_t *)AA;	
	int i;
	uint8_t readMatch=1;
	uint32_t EE_status;
	hiscount = 0;
	uint8_t needclear;
	BSP_LED_Init(LED4);
	BSP_LED_Init(LED3);
	/* STM32F4xx HAL library initialization:
       - Configure the Flash prefetch, instruction and Data caches
       - Configure the Systick to generate an interrupt each 1 msec
       - Set NVIC Group Priority to 4
       - Global MSP (MCU Support Package) initialization
 */
  HAL_Init();
	adjust = 0;
	adjuststate = 0;
	 /* Configure the system clock to 72 MHz */
  SystemClock_Config();
	
	HAL_InitTick(0x0000); // set systick's priority to the highest.

	//configure the USER button as exti mode. 
	BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_EXTI);   // BSP_functions in stm32f429i_discovery.c

	ExtBtn1_Config();  //for the first External button
	ExtBtn2_Config();
//Init LCD
	BSP_LCD_Init();
	//BSP_LCD_LayerDefaultInit(uint16_t LayerIndex, uint32_t FB_Address);
	BSP_LCD_LayerDefaultInit(0, LCD_FRAME_BUFFER);   //LCD_FRAME_BUFFER, defined as 0xD0000000 in _discovery_lcd.h
														// the LayerIndex may be 0 and 1. if is 2, then the LCD is dark.
	//BSP_LCD_SelectLayer(uint32_t LayerIndex);
	BSP_LCD_SelectLayer(0);
	BSP_LCD_SetLayerVisible(0, ENABLE);
	
	
	BSP_LCD_Clear(LCD_COLOR_WHITE);  //need this line, otherwise, the screen is dark	
	BSP_LCD_DisplayOn();
 
	BSP_LCD_SetFont(&Font20);  //the default font,  LCD_DEFAULT_FONT, which is defined in _lcd.h, is Font24
	
	

	
	I2C_Init(&I2c3_Handle);
	memLocation;
//*********************Testing I2C EEPROM------------------
	/*
	LCD_DisplayString(4, 2, (uint8_t *)"MT2TA4 LAB 3");
	LCD_DisplayString(6, 0, (uint8_t *)"Testing I2C & EEPROM....");
		
	HAL_Delay(2000);   //display for 1 second
		
	//BSP_LCD_Clear(LCD_COLOR_WHITE);
	BSP_LCD_ClearStringLine(4);
	HAL_Delay(5);   //have trouble to clear line 6 if there is no Delay!!??
	BSP_LCD_ClearStringLine(6);
	HAL_Delay(5);
	BSP_LCD_ClearStringLine(7);
	


	EE_status=I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation , data1);
	if (EE_status==HAL_OK)
			LCD_DisplayString(0, 0, (uint8_t *)"w data1 in EE OK");
	else
			LCD_DisplayString(0, 0, (uint8_t *)"w data1 failed");

	EE_status=I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation+1 , data2);
	if (EE_status==HAL_OK)
			LCD_DisplayString(1, 0, (uint8_t *)"w data2 in EE OK");
	else
			LCD_DisplayString(1, 0, (uint8_t *)"w data2 failed");
	

	
	
	readData=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation);
	if (data1 == readData) {
		LCD_DisplayString(3, 0, (uint8_t *)"r data1 OK:");
			LCD_DisplayInt(3, 13, readData);
	}else{
			LCD_DisplayString(3, 0, (uint8_t *)"data1 mismatch");
	}	
	
	
	readData=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation+1);
	if (data2 == readData) {
		LCD_DisplayString(4, 0, (uint8_t *)"r data2 OK:");
			LCD_DisplayInt(4, 13, readData);
	}else{
			LCD_DisplayString(4, 0, (uint8_t *)"data2 mismatch");
	}	
	
	

	EE_status=I2C_BufferWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation, bufferdata, 34);
	if (EE_status==HAL_OK)
			LCD_DisplayString(6, 0, (uint8_t *)"w EE buffer OK");
	else
			LCD_DisplayString(6, 0, (uint8_t *)"w buffer failed");

	for (i=0;i<=33;i++) { 
			readData=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation+i);
			HAL_Delay(50);   // just for display effect. for EEPROM read, do not need dalay
						//BUT if here delay longer time, the following display will hve trouble. ???
			//LCD_DisplayInt(8, i, (uint8_t)EE_status);
	
			BSP_LCD_DisplayChar(COLUMN(i%16),LINE(8+ 2*(int)(i/16)), (char) readData);	
			BSP_LCD_DisplayChar(COLUMN(i%16),LINE(9+ 2*(int)(i/16)),  bufferdata[i]);
			if (bufferdata[i]!=readData)
					readMatch=0;
	}

	if (readMatch==0)
				LCD_DisplayString(15, 0, (uint8_t *)"rd buffer mismatch");
	else 
				LCD_DisplayString(15, 0, (uint8_t *)"rd EE buffer OK");

	HAL_Delay(2000);  //display for 4 seconds
  */
	
	adjust_clean = 0;
	//configure real-time clock
	RTC_Config();
		
	RTC_AlarmAConfig();
	history = 0;
	/*
 //test realtime clock	
    BSP_LCD_Clear(LCD_COLOR_WHITE);
		HAL_Delay(10);   
		//otherwise, the following line will have trouble to display. 1. while cannot delay too long.??? 
		LCD_DisplayString(1, 0, (uint8_t *)"Testing RTC...");
	
	
		RTC_DateStructure.Month=3;
		RTC_DateStructure.WeekDay=1;   //what will happen if the date/weekday is not correct?
		RTC_DateStructure.Date=6;
		RTC_DateStructure.Year=23; //2012???  how about 1912?
		
		HAL_RTC_SetDate(&RTCHandle, &RTC_DateStructure, RTC_FORMAT_BIN);
	
		RTC_TimeStructure.Hours=23;
		RTC_TimeStructure.Minutes=20;   
		RTC_TimeStructure.Seconds=11;
		
		HAL_RTC_SetTime(&RTCHandle, &RTC_TimeStructure, RTC_FORMAT_BIN);
	
		
		//Read from RTC
		HAL_RTC_GetTime(&RTCHandle, &read_RTC_TimeStruct, RTC_FORMAT_BIN);
		HAL_RTC_GetDate(&RTCHandle, &read_RTC_DateStruct, RTC_FORMAT_BIN); 
		//NOTE from the UM1725 : You must call HAL_RTC_GetDate() after HAL_RTC_GetTime() to unlock the values in the higher order 
		//calendar shadow register ....		
	
		LCD_DisplayString(3,0, (uint8_t *) "HH:MM:SS");
	
		LCD_DisplayInt(4,0,read_RTC_TimeStruct.Hours);
		LCD_DisplayInt(4,3,read_RTC_TimeStruct.Minutes);
		LCD_DisplayInt(4,6,read_RTC_TimeStruct.Seconds);

		LCD_DisplayString(6,0, (uint8_t *) "WD:DD:MM:YY");
	
		LCD_DisplayInt(7,0, read_RTC_DateStruct.WeekDay);
		LCD_DisplayInt(7,3, read_RTC_DateStruct.Date);
		LCD_DisplayInt(7,6, read_RTC_DateStruct.Month);
		LCD_DisplayInt(7,9, read_RTC_DateStruct.Year);
 
	*/
	uint8_t adjust_Hours;
	uint8_t adjust_Minutes;
	uint8_t adjust_Seconds;
	uint8_t adjust_WeekDay;
	uint8_t adjust_Date;
	uint8_t adjust_Month;
	uint8_t adjust_Year;
	exbutton = 0;
	int monthnum[7] = {1,3,5,7,8,10,12}; //Month With 31 Days
	
	int val=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memstore);
	LCD_DisplayInt(14,0,val);
	
	
	
	//printf("%d",read_RTC_TimeStruct.Hours);
	/* Infinite loop */
while (1)
  {
	if (adjust ==0){ //Not in adjust Mode
		if (history == 0){ //Show Date Mode
			LCD_DisplayString(1, 1, (uint8_t *)"Real Time Clock");
			HAL_RTC_GetTime(&RTCHandle, &read_RTC_TimeStruct, RTC_FORMAT_BIN); //Get the time and Date
			HAL_RTC_GetDate(&RTCHandle, &read_RTC_DateStruct, RTC_FORMAT_BIN); 
			LCD_DisplayString(3,0, (uint8_t *) "HH:MM:SS");
			LCD_DisplayInt(4,0,read_RTC_TimeStruct.Hours); //Show the Time
			LCD_DisplayInt(4,3,read_RTC_TimeStruct.Minutes);
			LCD_DisplayInt(4,6,read_RTC_TimeStruct.Seconds);
			if (read_RTC_TimeStruct.Seconds>50){
				needclear = 1;
			}
			if(read_RTC_TimeStruct.Seconds==0&&needclear==1){ //Clear the Time if time display from 2 number to 1 number
				BSP_LCD_ClearStringLine(4);
				needclear = 0;
			}
			/*LCD_DisplayString(6,0, (uint8_t *) "WD:DD:MM:YY");
			LCD_DisplayInt(7,0, read_RTC_DateStruct.WeekDay);
			LCD_DisplayInt(7,3, read_RTC_DateStruct.Date);
			LCD_DisplayInt(7,6, read_RTC_DateStruct.Month);
			LCD_DisplayInt(7,9, read_RTC_DateStruct.Year);*/
			//BSP_LCD_ClearStringLine(9);
			if (history==1){ //Clear screen for enter history mode
				BSP_LCD_Clear(LCD_COLOR_WHITE);
			}
		}	
		else if(history ==1){//in history mode
			LCD_DisplayString(5,2, (uint8_t *) "History");
			/*if (hiscount==0){}
			else if (hiscount==1){
				LCD_DisplayString(7,8, (uint8_t *) "HH:MM:SS");
				uint8_t readData1=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation-3); //Read history From EEPROM and Print on Screen
				uint8_t readData2=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation-2);
				uint8_t readData3=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation-1);
				LCD_DisplayInt(9,6,1);
				LCD_DisplayInt(9,8,readData1);
				LCD_DisplayInt(9,11,readData2);
				LCD_DisplayInt(9,14,readData3);
		//ClearScreen();
			}
			else{*/
				uint8_t readData1=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation-3);
				uint8_t readData2=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation-2);
				uint8_t readData3=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation-1);
				LCD_DisplayString(7,8, (uint8_t *) "HH:MM:SS");
				LCD_DisplayInt(9,6,1);
				LCD_DisplayInt(9,8,readData1);
				LCD_DisplayInt(9,11,readData2);
				LCD_DisplayInt(9,14,readData3);
				uint8_t readData4=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation-6);
				uint8_t readData5=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation-5);
				uint8_t readData6=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation-4);
				LCD_DisplayInt(11,6,2);
				LCD_DisplayInt(11,8,readData4);
				LCD_DisplayInt(11,11,readData5);
				LCD_DisplayInt(11,14,readData6);
			if (history==0){
				BSP_LCD_Clear(LCD_COLOR_WHITE);
			}
		}
  }
	if(adjust == 1){ //Adjust Value Mode
		if(adjust_clean==1){
			BSP_LCD_Clear(LCD_COLOR_WHITE); //Clean 
			adjust_clean=0;
			adjust_Hours = read_RTC_TimeStruct.Hours; //Record all the Time and Date Now
			adjust_Minutes = read_RTC_TimeStruct.Minutes;
			adjust_Seconds = read_RTC_TimeStruct.Seconds;
			adjust_WeekDay = read_RTC_DateStruct.WeekDay;
			adjust_Date = read_RTC_DateStruct.Date;
			adjust_Month = read_RTC_DateStruct.Month;
			adjust_Year = read_RTC_DateStruct.Year;
		}
		LCD_DisplayString(1,0, (uint8_t *) "Adjust Clock");
		LCD_DisplayString(3,0, (uint8_t *) "HH:MM:SS");
		LCD_DisplayInt(4,0,adjust_Hours);
		LCD_DisplayInt(4,3,adjust_Minutes);
		LCD_DisplayInt(4,6,adjust_Seconds);
		LCD_DisplayString(6,0, (uint8_t *) "WD:DD:MM:YY");
		LCD_DisplayInt(7,0, adjust_WeekDay);
		LCD_DisplayInt(7,3, adjust_Date);
		LCD_DisplayInt(7,6, adjust_Month);
		LCD_DisplayInt(7,9, adjust_Year);
		switch(adjuststate){ //Adjust Each Value (Seconds.....Year)
			case 0:{
				if(up==1){
					adjust_Hours=adjust_Hours+1;
					if (adjust_Hours==24){
						adjust_Hours = 0;
						BSP_LCD_ClearStringLine(4);
					}
					if (adjust_Hours>24){
						adjust_Hours = 24;
					}
					up=0;
				}
				if(up==2){
					adjust_Hours=adjust_Hours-1;
					if (adjust_Hours==9){
						BSP_LCD_ClearStringLine(4);
					}
					if (adjust_Hours>24){
						adjust_Hours = 23;
					}
					up=0;
				}
				else{
					LCD_DisplayString(5,0, (uint8_t *) "^");
				}
				if (exbutton==1){
					adjuststate=1;
					exbutton=0;
					BSP_LCD_ClearStringLine(5);
					BSP_LCD_ClearStringLine(8);
				}
				
				break;
			}
			case 1:{
				if(up==1){
					adjust_Minutes=adjust_Minutes+1;
					if (adjust_Minutes==60){
						adjust_Minutes = 0;
						BSP_LCD_ClearStringLine(4);
					}
					up=0;
				}
				if(up==2){
					adjust_Minutes=adjust_Minutes-1;
					if (adjust_Minutes==9){
						BSP_LCD_ClearStringLine(4);
					}
					if (adjust_Minutes>60){
						adjust_Minutes = 59;
					}
					up=0;
				}
				else{
				LCD_DisplayString(5,3, (uint8_t *) "^");
				}
				if (exbutton==1){
					adjuststate=2;
					exbutton=0;
					BSP_LCD_ClearStringLine(5);
					BSP_LCD_ClearStringLine(8);
				}
				
				break;
			}
			case 2:{
				if(up==1){
					adjust_Seconds=adjust_Seconds+1;
					if (adjust_Seconds==60){
						adjust_Seconds = 0;
						BSP_LCD_ClearStringLine(4);
					}
					up=0;
				}
				if(up==2){
					adjust_Seconds=adjust_Seconds-1;
					if (adjust_Seconds==9){
						BSP_LCD_ClearStringLine(4);
					}
					if (adjust_Seconds>60){
						adjust_Seconds = 59;
					}
					up=0;
				}
				else{
				LCD_DisplayString(5,6, (uint8_t *) "^");
				}
				if (exbutton==1){
					adjuststate=3;
					exbutton=0;
					BSP_LCD_ClearStringLine(5);
					BSP_LCD_ClearStringLine(8);
				}
				break;
			}
			case 3:{
				if(up==1){
					adjust_Year=adjust_Year+1;
					if (adjust_Year==100){
						adjust_Year = 0;
						BSP_LCD_ClearStringLine(7);
					}
					up=0;
				}
				if(up==2){
					adjust_Year=adjust_Year-1;
					if (adjust_Year==9){
						BSP_LCD_ClearStringLine(7);
					}
					if (adjust_Year>100){
						adjust_Year = 99;
					}
					up=0;
				}
				else{
					LCD_DisplayString(8,9, (uint8_t *) "^");
				}	
				if (exbutton==1){
					adjuststate=4;
					exbutton=0;
					BSP_LCD_ClearStringLine(5);
					BSP_LCD_ClearStringLine(8);
				}
				break;
			}
			case 4:{
				if(up==1){
					adjust_Month=adjust_Month+1;
					if (adjust_Month==13){
						adjust_Month = 1;
						BSP_LCD_ClearStringLine(7);
					}
					up = 0;
				}
				if(up==2){
					adjust_Month=adjust_Month-1;
					if (adjust_Month==9){
						BSP_LCD_ClearStringLine(7);
					}
					if (adjust_Month==0){
						adjust_Month = 12;
					}
					up=0;
				}
				else{
					LCD_DisplayString(8,6, (uint8_t *) "^");
				}
				if (exbutton==1){
					adjuststate=5;
					exbutton=0;
					BSP_LCD_ClearStringLine(5);
					BSP_LCD_ClearStringLine(8);
				}
				break;
			}case 5:{
				if(up==1){
					adjust_Date=adjust_Date+1;
					if (adjust_Year%4==0&& adjust_Year%100!=0){
							for(int i =0;i<7;i++){
								if (monthnum[i]==adjust_Month){
									if (adjust_Date==32){
										adjust_Date = 1;
										BSP_LCD_ClearStringLine(7);
									}
									break;
								}
							}
							if (adjust_Month==2){
								if (adjust_Date==29){
										adjust_Date = 1;
									BSP_LCD_ClearStringLine(7);
									}
							}
							else{
								if (adjust_Date==31){
										adjust_Date = 1;
									BSP_LCD_ClearStringLine(7);
									}
							}
					}
					else{
						for(int i =0;i<7;i++){
								if (monthnum[i]==adjust_Month){
									if (adjust_Date==32){
										adjust_Date = 1;
										BSP_LCD_ClearStringLine(7);
									}
									break;
								}
							}
							if (adjust_Month==2){
								if (adjust_Date==28){
										adjust_Date = 1;
									BSP_LCD_ClearStringLine(7);
									}
							}
							else{
								if (adjust_Date==31){
										adjust_Date = 1;
									BSP_LCD_ClearStringLine(7);
									}
							}
					}
					/*if (adjust_Date==30){
						adjust_Date = 1;
					}*/
					up = 0;
				}
				if(up==2){
					adjust_Date=adjust_Date-1;
					if (adjust_Year%4==0&& adjust_Year%100!=0){
							for(int i =0;i<7;i++){
								if (monthnum[i]==adjust_Month){
									if (adjust_Date==9){
										BSP_LCD_ClearStringLine(7);
									}
									if (adjust_Date==0){
										adjust_Date = 31;
										BSP_LCD_ClearStringLine(7);
									}
									break;
								}
							}
							if (adjust_Month==2){
								if (adjust_Date==9){
										BSP_LCD_ClearStringLine(7);
									}
								if (adjust_Date==0){
										adjust_Date = 28;
									BSP_LCD_ClearStringLine(7);
									}
							}
							else{
								if (adjust_Date==9){
										BSP_LCD_ClearStringLine(7);
									}
								if (adjust_Date==0){
										adjust_Date = 30;
									BSP_LCD_ClearStringLine(7);
									}
							}
					}
					else{
						for(int i =0;i<7;i++){
								if (monthnum[i]==adjust_Month){
									if (adjust_Date==9){
										BSP_LCD_ClearStringLine(7);
									}
									if (adjust_Date==0){
										adjust_Date = 31;
										BSP_LCD_ClearStringLine(7);
									}
									break;
								}
							}
							if (adjust_Month==2){
								if (adjust_Date==9){
										BSP_LCD_ClearStringLine(7);
									}
								if (adjust_Date==0){
										adjust_Date =27;
									BSP_LCD_ClearStringLine(7);
									}
							}
							else{
								if (adjust_Date==0){
									if (adjust_Date==9){
										BSP_LCD_ClearStringLine(7);
									}
										adjust_Date = 30;
									BSP_LCD_ClearStringLine(7);
									}
							}
					}
					/*if (adjust_Date==30){
						adjust_Date = 1;
					}*/
					up = 0;
				}
				else{
					LCD_DisplayString(8,3, (uint8_t *) "^");
				}
				if (exbutton==1){
					adjuststate=6;
					exbutton=0;
					BSP_LCD_ClearStringLine(5);
					BSP_LCD_ClearStringLine(8);
				}
				break;
			}case 6:{
				if(up==1){
					adjust_WeekDay=adjust_WeekDay+1;
					if (adjust_WeekDay==8){
						adjust_WeekDay = 1;
					}
					up = 0;
				}
				if(up==2){
					adjust_WeekDay=adjust_WeekDay-1;
					if (adjust_WeekDay==0){
						adjust_WeekDay = 7;
					}
					up = 0;
				}
				else{
					LCD_DisplayString(8,0, (uint8_t *) "^");
				}
				if (exbutton==1){
					adjuststate=7;
					exbutton=0;
					BSP_LCD_ClearStringLine(5);
					BSP_LCD_ClearStringLine(8);
				}
				break;
			}
			case 7:{
				BSP_LCD_Clear(LCD_COLOR_WHITE);
				RTC_DateStructure.Month=adjust_Month; //Reset RTC to time and date we set
				RTC_DateStructure.WeekDay=adjust_WeekDay; 
				RTC_DateStructure.Date=adjust_Date;
				RTC_DateStructure.Year=adjust_Year;
				HAL_RTC_SetDate(&RTCHandle, &RTC_DateStructure, RTC_FORMAT_BIN);
				RTC_TimeStructure.Hours=adjust_Hours;
				RTC_TimeStructure.Minutes=adjust_Minutes;   
				RTC_TimeStructure.Seconds=adjust_Seconds;
				HAL_RTC_SetTime(&RTCHandle, &RTC_TimeStructure, RTC_FORMAT_BIN);
				adjuststate = 0;
				adjust = 0;
				break;
			}
			}
		}
	}	//end of while


	}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 

	* 					Oscillator											=HSE
	*    				HSE frequencey 										=8,000,000   (8MHz)
	*      ----However, if the project is created by uVision, the default HSE_VALUE is 25MHz. thereore, need to define HSE_VALUE
	*						PLL Source											=HSE
  *            PLL_M                          = 4
  *            PLL_N                          = 72
  *            PLL_P                          = 2
  *            PLL_Q                          = 3
  *        --->therefore, PLLCLK =8MHz X N/M/P=72MHz   
	*            System Clock source            = PLL (HSE)
  *        --> SYSCLK(Hz)                     = 72,000,000
  *            AHB Prescaler                  = 1
	*        --> HCLK(Hz)                       = 72 MHz
  *            APB1 Prescaler                 = 2
	*        --> PCLK1=36MHz,  -->since TIM2, TIM3, TIM4 TIM5...are on APB1, thiese TIMs CLK is 36X2=72MHz
							 	
  *            APB2 Prescaler                 = 1
	*        --> PCLK1=72MHz 

  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 5
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();
  
  /* The voltage scaling allows optimizing the power consumption when the device is 
     clocked below the maximum system frequency, to update the voltage scaling value 
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  
  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 3;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /* Activate the Over-Drive mode */
  HAL_PWREx_EnableOverDrive();
 
  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;  
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}


/**
  * @brief  Configures EXTI Line0 (connected to PA0 pin) in interrupt mode
  * @param  None
  * @retval None
  */



/**
 * Use this function to configure the GPIO to handle input from
 * external pushbuttons and configure them so that you will handle
 * them through external interrupts.
 */
void ExtBtn1_Config(void)     // for GPIO C pin 1
// can only use PA0, PB0... to PA4, PB4 .... because only  only  EXTI0, ...EXTI4,on which the 
	//mentioned pins are mapped to, are connected INDIVIDUALLY to NVIC. the others are grouped! 
		//see stm32f4xx.h, there is EXTI0_IRQn...EXTI4_IRQn, EXTI15_10_IRQn defined
{
  GPIO_InitTypeDef   GPIO_InitStructure;

  /* Enable GPIOB clock */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  
  /* Configure PA0 pin as input floating */
  GPIO_InitStructure.Mode =  GPIO_MODE_IT_FALLING;
  GPIO_InitStructure.Pull =GPIO_PULLUP;
  GPIO_InitStructure.Pin = GPIO_PIN_1;
	GPIO_InitStructure.Speed=GPIO_SPEED_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

	//__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_1);   //is defined the same as the __HAL_GPIO_EXTI_CLEAR_FLAG(GPIO_PIN_1); ---check the hal_gpio.h
	//__HAL_GPIO_EXTI_CLEAR_FLAG(GPIO_PIN_1);// after moving the chunk of code in the GPIO_EXTI callback from _it.c (before these chunks are in _it.c)
																					//the program "freezed" when start, suspect there is a interupt pending bit there. Clearing it solve the problem.
  /* Enable and set EXTI Line0 Interrupt to the lowest priority */
  HAL_NVIC_SetPriority(EXTI1_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);
}

void ExtBtn2_Config(void){  //**********PD2.***********

	//
	GPIO_InitTypeDef   GPIO_InitStructure;

  /* Enable GPIOB clock */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  
  /* Configure PA0 pin as input floating */
  GPIO_InitStructure.Mode =  GPIO_MODE_IT_FALLING;
  GPIO_InitStructure.Pull =GPIO_PULLUP;
  GPIO_InitStructure.Pin = GPIO_PIN_2;
	GPIO_InitStructure.Speed=GPIO_SPEED_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStructure);

	//__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_2);   //is defined the same as the __HAL_GPIO_EXTI_CLEAR_FLAG(GPIO_PIN_1); ---check the hal_gpio.h
	//__HAL_GPIO_EXTI_CLEAR_FLAG(GPIO_PIN_2);// after moving the chunk of code in the GPIO_EXTI callback from _it.c (before these chunks are in _it.c)
																					//the program "freezed" when start, suspect there is a interupt pending bit there. Clearing it solve the problem.
  /* Enable and set EXTI Line0 Interrupt to the lowest priority */
	
	
  // Enable and set EXTI Line0 Interrupt to the lowest priority 
  HAL_NVIC_SetPriority(EXTI2_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);
}


void RTC_Config(void) {
	RTC_TimeTypeDef RTC_TimeStructure;
	RTC_DateTypeDef RTC_DateStructure;
	
	/****************
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	RTC_AlarmTypeDef RTC_AlarmStructure;
	****************/
	//1: Enable the RTC domain access (enable wirte access to the RTC )
			//1.1: Enable the Power Controller (PWR) APB1 interface clock:
        __HAL_RCC_PWR_CLK_ENABLE(); 
			//1.2:  Enable access to RTC domain 
				HAL_PWR_EnableBkUpAccess();
			//1.3: Select the RTC clock source
				__HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_LSI);  //RCC_RTCCLKSOURCE_LSI is defined in hal_rcc.h
	       // according to P9 of AN3371 Application Note, LSI's accuracy is not suitable for RTC application!!!! 
					//can not use LSE!!!---LSE is not available, at leaset not available for stm32f407 board.
				//****"Without parts at X3, C16, C27, and removing SB15 and SB16, the LSE is not going to tick or come ready"*****.
			//1.4: Enable RTC Clock
			__HAL_RCC_RTC_ENABLE();   //enable RTC
			
	
			//1.5  Enable LSI
			__HAL_RCC_LSI_ENABLE();   //need to enable the LSI !!!
																//defined in _rcc.c
			while (__HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY)==RESET) {}    //defind in rcc.c
	
			// for the above steps, please see the CubeHal UM1725, p616, section "Backup Domain Access" 	
				
				
				
	//2.  Configure the RTC Prescaler (Asynchronous and Synchronous) and RTC hour 
        
				RTCHandle.Instance = RTC;
				RTCHandle.Init.HourFormat = RTC_HOURFORMAT_24;
				//RTC time base frequency =LSE/((AsynchPreDiv+1)*(SynchPreDiv+1))=1Hz
				//see the AN3371 Application Note: if LSE=32.768K, PreDiv_A=127, Prediv_S=255
				//    if LSI=32K, PreDiv_A=127, Prediv_S=249
				//also in the note: LSI accuracy is not suitable for calendar application!!!!!! 
				RTCHandle.Init.AsynchPrediv = 127; //if using LSE: Asyn=127, Asyn=255: 
				RTCHandle.Init.SynchPrediv = 249;  //if using LSI(32Khz): Asyn=127, Asyn=249: 
				// but the UM1725 says: to set the Asyn Prescaler a higher value can mimimize power comsumption
				
				RTCHandle.Init.OutPut = RTC_OUTPUT_DISABLE;
				RTCHandle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
				RTCHandle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
				
				//HAL_RTC_Init(); 
				if(HAL_RTC_Init(&RTCHandle) != HAL_OK)
				{
						LCD_DisplayString(1, 0, (uint8_t *)"RTC Init Error!");
				}
	
	//3. init the time and date
				RTC_DateStructure.Year = 23;
				RTC_DateStructure.Month = RTC_MONTH_MARCH;
				RTC_DateStructure.Date = 8; //if use RTC_FORMAT_BCD, NEED TO SET IT AS 0x18 for the 18th.
				RTC_DateStructure.WeekDay = RTC_WEEKDAY_WEDNESDAY; //???  if the real weekday is not correct for the given date, still set as 
																												//what is specified here.
				
				if(HAL_RTC_SetDate(&RTCHandle,&RTC_DateStructure,RTC_FORMAT_BIN) != HAL_OK)   //BIN format is better 
															//before, must set in BCD format and read in BIN format!!
				{
					LCD_DisplayString(2, 0, (uint8_t *)"Date Init Error!");
				} 
  
  
				RTC_TimeStructure.Hours = 15;  
				RTC_TimeStructure.Minutes = 20; //if use RTC_FORMAT_BCD, NEED TO SET IT AS 0x19
				RTC_TimeStructure.Seconds = 00;
				RTC_TimeStructure.TimeFormat = RTC_HOURFORMAT12_AM;
				RTC_TimeStructure.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
				RTC_TimeStructure.StoreOperation = RTC_STOREOPERATION_RESET;//?????/
				
				if(HAL_RTC_SetTime(&RTCHandle,&RTC_TimeStructure,RTC_FORMAT_BIN) != HAL_OK)   //BIN format is better
																																					//before, must set in BCD format and read in BIN format!!
				{
					LCD_DisplayString(3, 0, (uint8_t *)"TIME Init Error!");
				}
  
			//Writes a data in a RTC Backup data Register0   --why need this line?
			//HAL_RTCEx_BKUPWrite(&RTCHandle,RTC_BKP_DR0,0x32F2);   

	/*
			//The RTC Resynchronization mode is write protected, use the
			//__HAL_RTC_WRITEPROTECTION_DISABLE() befor calling this function.
			__HAL_RTC_WRITEPROTECTION_DISABLE(&RTCHandle);
			//wait for RTC APB registers synchronisation
			HAL_RTC_WaitForSynchro(&RTCHandle);
			__HAL_RTC_WRITEPROTECTION_ENABLE(&RTCHandle);				
	 */
				
				
			__HAL_RTC_TAMPER1_DISABLE(&RTCHandle);
			__HAL_RTC_TAMPER2_DISABLE(&RTCHandle);	
				//Optionally, a tamper event can cause a timestamp to be recorded. ---P802 of RM0090
				//Timestamp on tamper event
				//With TAMPTS set to ‘1 , any tamper event causes a timestamp to occur. In this case, either
				//the TSF bit or the TSOVF bit are set in RTC_ISR, in the same manner as if a normal
				//timestamp event occurs. The affected tamper flag register (TAMP1F, TAMP2F) is set at the
				//same time that TSF or TSOVF is set. ---P802, about Tamper detection
				//-------that is why need to disable this two tamper interrupts. Before disable these two, when program start, there is always a timestamp interrupt.
				//----also, these two disable function can not be put in the TSConfig().---put there will make  the program freezed when start. the possible reason is
				//-----one the RTC is configured, changing the control register again need to lock and unlock RTC and disable write protection.---See Alarm disable/Enable 
				//---function.
				
			HAL_RTC_WaitForSynchro(&RTCHandle);	
			//To read the calendar through the shadow registers after Calendar initialization,
			//		calendar update or after wake-up from low power modes the software must first clear
			//the RSF flag. The software must then wait until it is set again before reading the
			//calendar, which means that the calendar registers have been correctly copied into the
			//RTC_TR and RTC_DR shadow registers.The HAL_RTC_WaitForSynchro() function
			//implements the above software sequence (RSF clear and RSF check).	
}


void RTC_AlarmAConfig(void)
{
	RTC_AlarmTypeDef RTC_Alarm_Structure;

	RTC_Alarm_Structure.Alarm = RTC_ALARM_A;
  RTC_Alarm_Structure.AlarmMask = RTC_ALARMMASK_ALL;
				// See reference manual. especially 
				//p11-12 of AN3371 Application Note.
				// this mask mean alarm occurs every second.
				//if MaskAll, the other 3 fieds of the AlarmStructure do not need to be initiated
				//the other three fieds are: RTC_AlarmTime(for when to occur), 
				//RTC_AlarmDateWeekDaySel (to use RTC_AlarmDateWeekDaySel_Date or RTC_AlarmDateWeekDaySel_WeekDay
				//RTC_AlarmDateWeekDay (0-31, or RTC_Weekday_Monday, RTC_Weekday_Tuesday...., depends on the value of AlarmDateWeekDaySel)	
	//RTC_Alarm_Structure.AlarmDateWeekDay = RTC_WEEKDAY_MONDAY;
  //RTC_Alarm_Structure.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
  //RTC_Alarm_Structure.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
		   //RTC_ALARMSUBSECONDMASK_ALL --> All Alarm SS fields are masked. 
        //There is no comparison on sub seconds for Alarm 
			
  //RTC_Alarm_Structure.AlarmTime.Hours = 0x02;
  //RTC_Alarm_Structure.AlarmTime.Minutes = 0x20;
  //RTC_Alarm_Structure.AlarmTime.Seconds = 0x30;
  //RTC_Alarm_Structure.AlarmTime.SubSeconds = 0x56;
  
  if(HAL_RTC_SetAlarm_IT(&RTCHandle,&RTC_Alarm_Structure,RTC_FORMAT_BCD) != HAL_OK)
  {
			LCD_DisplayString(4, 0, (uint8_t *)"Alarm setup Error!");
  }
  
	//Enable the RTC Alarm interrupt
//	__HAL_RTC_ALARM_ENABLE_IT(&RTCHandle,RTC_IT_ALRA);   //already in function HAL_RTC_SetAlarm_IT()
	
	//Enable the RTC ALARMA peripheral.
//	__HAL_RTC_ALARMA_ENABLE(&RTCHandle);  //already in function HAL_RTC_SetAlarm_IT()
	
	__HAL_RTC_ALARM_CLEAR_FLAG(&RTCHandle, RTC_FLAG_ALRAF); //need it? !!!!, without it, sometimes(SOMETIMES, when first time to use the alarm interrupt)
																			//the interrupt handler will not work!!! 		
	

	
		//need to set/enable the NVIC for RTC_Alarm_IRQn!!!!
	HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);   
	HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 0x00, 0);  //not important
	
				
	
}


HAL_StatusTypeDef  RTC_AlarmA_IT_Disable(RTC_HandleTypeDef *hrtc) 
{ 
 	// Process Locked  
	__HAL_LOCK(hrtc);
  
  hrtc->State = HAL_RTC_STATE_BUSY;
  
  // Disable the write protection for RTC registers 
  __HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);
  
  // __HAL_RTC_ALARMA_DISABLE(hrtc);
    
   // In case of interrupt mode is used, the interrupt source must disabled 
   __HAL_RTC_ALARM_DISABLE_IT(hrtc, RTC_IT_ALRA);


 // Enable the write protection for RTC registers 
  __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);
  
  hrtc->State = HAL_RTC_STATE_READY; 
  
  // Process Unlocked 
  __HAL_UNLOCK(hrtc);  
}


HAL_StatusTypeDef  RTC_AlarmA_IT_Enable(RTC_HandleTypeDef *hrtc) 
{	
	// Process Locked  
	__HAL_LOCK(hrtc);	
  hrtc->State = HAL_RTC_STATE_BUSY;
  
  // Disable the write protection for RTC registers 
  __HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);
  
  // __HAL_RTC_ALARMA_ENABLE(hrtc);
    
   // In case of interrupt mode is used, the interrupt source must disabled 
   __HAL_RTC_ALARM_ENABLE_IT(hrtc, RTC_IT_ALRA);


 // Enable the write protection for RTC registers 
  __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);
  
  hrtc->State = HAL_RTC_STATE_READY; 
  
  // Process Unlocked 
  __HAL_UNLOCK(hrtc);  

}



void LCD_DisplayString(uint16_t LineNumber, uint16_t ColumnNumber, uint8_t *ptr)
{  
  //here the LineNumber and the ColumnNumber are NOT  pixel numbers!!!
		while (*ptr!=NULL)
    {
				BSP_LCD_DisplayChar(COLUMN(ColumnNumber),LINE(LineNumber), *ptr); //new version of this function need Xpos first. so COLUMN() is the first para.
				ColumnNumber++;
			 //to avoid wrapping on the same line and replacing chars 
				if ((ColumnNumber+1)*(((sFONT *)BSP_LCD_GetFont())->Width)>=BSP_LCD_GetXSize() ){
					ColumnNumber=0;
					LineNumber++;
				}
					
				ptr++;
		}
}

void LCD_DisplayInt(uint16_t LineNumber, uint16_t ColumnNumber, int Number)
{  
  //here the LineNumber and the ColumnNumber are NOT  pixel numbers!!!
		char lcd_buffer[15];
		sprintf(lcd_buffer,"%d",Number);
	
		LCD_DisplayString(LineNumber, ColumnNumber, (uint8_t *) lcd_buffer);
}

void LCD_DisplayFloat(uint16_t LineNumber, uint16_t ColumnNumber, float Number, int DigitAfterDecimalPoint)
{  
  //here the LineNumber and the ColumnNumber are NOT  pixel numbers!!!
		char lcd_buffer[15];
		
		sprintf(lcd_buffer,"%.*f",DigitAfterDecimalPoint, Number);  //6 digits after decimal point, this is also the default setting for Keil uVision 4.74 environment.
	
		LCD_DisplayString(LineNumber, ColumnNumber, (uint8_t *) lcd_buffer);
}



/**
  * @brief EXTI line detection callbacks
  * @param GPIO_Pin: Specifies the pins connected EXTI line
  * @retval None
  */


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	
  if(GPIO_Pin == KEY_BUTTON_PIN)  //GPIO_PIN_0
  {
		if (history==0&&adjust==0){
			holdstart = HAL_GetTick(); 
			
			while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0)){ // while button is pressed
				RTC_AlarmA_IT_Disable(&RTCHandle);
				if((HAL_GetTick()- holdstart) > 500){ // if held more than 0.5s
					HAL_RTC_GetTime(&RTCHandle, &read_RTC_TimeStruct, RTC_FORMAT_BIN); //Show the Date
					HAL_RTC_GetDate(&RTCHandle, &read_RTC_DateStruct, RTC_FORMAT_BIN); 
					LCD_DisplayString(3,0, (uint8_t *) "HH:MM:SS");
					LCD_DisplayInt(4,0,read_RTC_TimeStruct.Hours);
					LCD_DisplayInt(4,3,read_RTC_TimeStruct.Minutes);
					LCD_DisplayInt(4,6,read_RTC_TimeStruct.Seconds);
					LCD_DisplayString(6,0, (uint8_t *) "WD:DD:MM:YY");
					LCD_DisplayInt(7,0, read_RTC_DateStruct.WeekDay);
					LCD_DisplayInt(7,3, read_RTC_DateStruct.Date);
					LCD_DisplayInt(7,6, read_RTC_DateStruct.Month);
					LCD_DisplayInt(7,9, read_RTC_DateStruct.Year);
				}
			}
			BSP_LCD_ClearStringLine(6); //Clear the Date if release Button from holding
			BSP_LCD_ClearStringLine(7);
			if ((HAL_GetTick()- holdstart) < 500){ //CLick the user button
					HAL_RTC_GetTime(&RTCHandle, &read_RTC_TimeStruct, RTC_FORMAT_BIN); //Print the time recorded and store it in EEPROM
					LCD_DisplayString(9,0, (uint8_t *) "Record: HH:MM:SS");
					LCD_DisplayInt(10,8,read_RTC_TimeStruct.Hours);
					LCD_DisplayInt(10,11,read_RTC_TimeStruct.Minutes);
					LCD_DisplayInt(10,14,read_RTC_TimeStruct.Seconds);
					I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation,read_RTC_TimeStruct.Hours);
					I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation+1,read_RTC_TimeStruct.Minutes);
					I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation+2,read_RTC_TimeStruct.Seconds);
					memLocation = memLocation+3;
					hiscount++;
					I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memstore,memLocation);
				}
			
		}
		else if(history==0 && adjust==1){ //if in adjust mode
			up = 2; //value decrease one 
		}
  }
	
	
	if(GPIO_Pin == GPIO_PIN_1)
  {
		if (history ==0 && adjust ==0){//in show time mode
		history = 1;//switch to history mod
		}
		else if (history==1){ //if in history mode
	  history = 0;//exit to show time mode
		}
		else if(history==0 && adjust==1){ // if in adjust mode
				up = 1;//value increase one
			}
	}  //end of PIN_1

	if(GPIO_Pin == GPIO_PIN_2)
  {
		if (adjust == 1){
			exbutton = 1;//Switch Value to change
		}
		if (adjust==0&&history==0){
			adjust=1;//Change to adjust mode
			adjuststate=0;//Change the first value(Seconds)
			adjust_clean = 1;//Clean the show time state screen
		}
		
	} //end of if PIN_2	APB
	
	
}





void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
	
	//RTC_TimeShow();
}




/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  /* Turn LED4 on */
  BSP_LED_On(LED4);
  while(1)
  {
  }
}

void ClearScreen(void){
	for (int i=1;i<8;i++){
		BSP_LCD_ClearStringLine(i);
	}
	HAL_Delay(100);
}


#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
