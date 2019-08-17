/*
 * BinClock.c
 * Jarrod Olivier
 * Modified for EEE3095S/3096S by Keegan Crankshaw
 * August 2019
 * 
 * <LCKMAT002>
 * 16 August 2019
*/

#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdio.h> //For printf functions
#include <stdlib.h> // For system functions
#include <signal.h>

#include "BinClock.h"
#include "CurrentTime.h"
#include<unistd.h>

//Global variables
int hours, mins, secs;
long lastInterruptTime = 0; //Used for button debounce
int RTC; //Holds the RTC instance

int HH,MM,SS,h;

//Array for writing to LEDS
int hoursLED[12][4];
int minsLED[60][7];


void initGPIO(void){
	/* 
	 * Sets GPIO using wiringPi pins. see pinout.xyz for specific wiringPi pins
	 * You can also use "gpio readall" in the command line to get the pins
	 * Note: wiringPi does not use GPIO or board pin numbers (unless specifically set to that mode)
	 */

	 printf("Setting up\n");
	 wiringPiSetup(); //This is the default mode. If you want to change pinouts, be aware
	
	RTC = wiringPiI2CSetup(RTCAddr); //Set up the RTC
	
	for(int i=0; i < sizeof(LEDS)/sizeof(LEDS[0]); i++){
	    pinMode(LEDS[i], OUTPUT); 		
	}
	
	//Set Up the Seconds LED for PWM
	//Write your logic here
	

	
	//Set up the Buttons
	for(int j=0; j < sizeof(BTNS)/sizeof(BTNS[0]); j++){
		pinMode(BTNS[j], INPUT);
		//Changed to pull down
		pullUpDnControl(BTNS[j], PUD_DOWN);
	}
	
	//Attach interrupts to Buttons
	//Write your logic here

	wiringPiISR (BTNS[0], INT_EDGE_RISING,  &hourInc );
	wiringPiISR (BTNS[1], INT_EDGE_RISING,  &minInc) ;
	
	printf("BTNS done\n");
	printf("Setup done\n");
}


/*
 * The main function
 * This function is called, and calls all relevant functions we've written
 */
int main(void){
	
	if (signal(SIGINT, sig_handler) == SIG_ERR);

	//printf("Setting up\n");
	//wiringPiSetup(); //This is the default mode. If you want to change pinouts, be aware
	wiringPiSetup();

	initGPIO();

	//Setup of Binary Arrays
	for (int z = 0; z < 12; ++z) {
    hoursLED[z][0] = GET_BIT(z, 3);
    hoursLED[z][1] = GET_BIT(z, 2);
    hoursLED[z][2] = GET_BIT(z, 1);
    hoursLED[z][3] = GET_BIT(z, 0);
	}

	for (int z = 0; z < 60; ++z) {
    minsLED[z][0] = GET_BIT(z, 6);
    minsLED[z][1] = GET_BIT(z, 5);
    minsLED[z][2] = GET_BIT(z, 4);
	minsLED[z][3] = GET_BIT(z, 3);
    minsLED[z][4] = GET_BIT(z, 2);
	minsLED[z][5] = GET_BIT(z, 1);
	minsLED[z][6] = GET_BIT(z, 0);
	}

	// Test Binsary Array
	// for (int i = 0; i < 60; ++i, puts(""))
    //     for (int j = 0; j < 7; ++j)
    //         printf("%d", minsLED[i][j]);
	
	//Start counting
	wiringPiI2CWriteReg8(RTC, SEC, 0b10000001);

	// wiringPiI2CWriteReg8(RTC, HOUR, binaryConverter(10));
	// wiringPiI2CWriteReg8(RTC, MIN, binaryConverter(55));
	// wiringPiI2CWriteReg8(RTC, SEC, binaryConverter(30));

	toggleTime();
	
	
	// Repeat this until we shut down
	for (;;){
		//Fetch the time from the RTC
		//Write your logic here

		hours= getHoursRTC() ;
		mins= getMinsRCTC() ;
		secs= getSecsRTC();

		//Function calls to toggle LEDs
		//Write your logic here
		
		// Print out the time we have stored on our RTC
		printf("The current time is: %02d:%02d:%02d\n", hours, mins, secs);
		lightHours(hours);
		lightMins(mins);

		//int secs= wiringPiI2CReadReg8 (RTC, SEC) ;
		//printf("%d\n",secs);

		//using a delay to make our program "less CPU hungry"
		delay(1000); //milliseconds
	}
	return 0;
}

void sig_handler(int signo)
{
  if (signo == SIGINT)
  {	wiringPiI2CWriteReg8(RTC, HOUR, 0);
	wiringPiI2CWriteReg8(RTC, MIN, 0);
	wiringPiI2CWriteReg8(RTC, SEC, 0);

	for(int z=0; z < sizeof(LEDS)/sizeof(LEDS); z++){
		digitalWrite (LEDS[z],0);
	}

    printf("\nGPIO Cleared!");

	exit(0);
	}    
}

int getHoursRTC(){
	int hours= bcdConverter(wiringPiI2CReadReg8 (RTC, HOUR)) ;
	return hours;
}

int getMinsRCTC(){
	int mins= bcdConverter(wiringPiI2CReadReg8 (RTC, MIN)) ;
	return mins;

}

int getSecsRTC(){
	int secs= bcdConverter(wiringPiI2CReadReg8 (RTC, SEC)) ;
	return secs;
}



/*
 * Performs bitwise operations to extract BCD
 */

int bcdConverter(int BCD){
	int ones = BCD & 0b00001111;
	int tens = ((BCD & (0b01110000))>>4);
	return ones + tens*10;
}

int binaryConverter(int bin){
	int ones =bin%10 ;
	int tens = ((bin-ones)/10)<<4;
	return ((ones | tens) | 0b10000000);
}

/*
 * Change the hour format to 12 hours
 */
int hFormat(int hours){
	/*formats to 12h*/
	if (hours >= 24){
		hours = 0;
	}
	else if (hours > 12){
		hours -= 12;
	}
	return (int)hours;
}

/*
 * Turns on corresponding LED's for hours
 */
void lightHours(int units){
	int h=hFormat(units);
	for(int z=0; z < sizeof(hoursLEDs)/sizeof(hoursLEDs[0]); z++){
		digitalWrite (hoursLEDs[z],hoursLED[h][z]);
	}
}

/*
 * Turn on the Minute LEDs
 */
void lightMins(int units){
	for(int z=0; z < sizeof(minLEDs)/sizeof(minLEDs[0]); z++){
		digitalWrite (minLEDs[z],minsLED[units][z]);
	}
}

/*
 * PWM on the Seconds LED
 * The LED should have 60 brightness levels
 * The LED should be "off" at 0 seconds, and fully bright at 59 seconds
 */
void secPWM(int units){
	// Write your logic here
}

/*
 * hexCompensation
 * This function may not be necessary if you use bit-shifting rather than decimal checking for writing out time values
 */
int hexCompensation(int units){
	/*Convert HEX or BCD value to DEC where 0x45 == 0d45 
	  This was created as the lighXXX functions which determine what GPIO pin to set HIGH/LOW
	  perform operations which work in base10 and not base16 (incorrect logic) 
	*/
	int unitsU = units%0x10;

	if (units >= 0x50){
		units = 50 + unitsU;
	}
	else if (units >= 0x40){
		units = 40 + unitsU;
	}
	else if (units >= 0x30){
		units = 30 + unitsU;
	}
	else if (units >= 0x20){
		units = 20 + unitsU;
	}
	else if (units >= 0x10){
		units = 10 + unitsU;
	}
	return units;
}


/*
 * decCompensation
 * This function "undoes" hexCompensation in order to write the correct base 16 value through I2C
 */
int decCompensation(int units){
	int unitsU = units%10;

	if (units >= 50){
		units = 0x50 + unitsU;
	}
	else if (units >= 40){
		units = 0x40 + unitsU;
	}
	else if (units >= 30){
		units = 0x30 + unitsU;
	}
	else if (units >= 20){
		units = 0x20 + unitsU;
	}
	else if (units >= 10){
		units = 0x10 + unitsU;
	}
	return units;
}


/*
 * hourInc
 * Fetch the hour value off the RTC, increase it by 1, and write back
 * Be sure to cater for there only being 23 hours in a day
 * Software Debouncing should be used
 */
void hourInc(void){
	//Debounce
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>200){
		printf("Interrupt 1 triggered, %x\n", hours);
		
		hours = getHoursRTC();
		hours++;

		if (hours>23) hours=0;

		wiringPiI2CWriteReg8(RTC, HOUR, binaryConverter(hours));

		lightHours(hours);

	}
	lastInterruptTime = interruptTime;
}

/* 
 * minInc
 * Fetch the minute value off the RTC, increase it by 1, and write back
 * Be sure to cater for there only being 60 minutes in an hour
 * Software Debouncing should be used
 */
void minInc(void){
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>200){
		printf("Interrupt 2 triggered, %x\n", mins);
		
		hours = getHoursRTC();
		mins = getMinsRCTC();

		mins++;

		if (mins>59) {
			mins++;
			if (hours>23) hours=0;
			mins =0;
		}

		wiringPiI2CWriteReg8(RTC, HOUR, binaryConverter(hours));
		wiringPiI2CWriteReg8(RTC, MIN, binaryConverter(mins));

		lightMins(mins);

		
	}
	lastInterruptTime = interruptTime;
}

//This interrupt will fetch current time from another script and write it to the clock registers
//This functions will toggle a flag that is checked in main
void toggleTime(void){
	HH = getHours();
	MM = getMins();
	SS = getSecs();

	//HH = hFormat(HH);
	HH = decCompensation(HH);
	wiringPiI2CWriteReg8(RTC, HOUR, HH);

	MM = decCompensation(MM);
	wiringPiI2CWriteReg8(RTC, MIN, MM);

	SS = decCompensation(SS);
	wiringPiI2CWriteReg8(RTC, SEC, 0b10000000+SS);


}
