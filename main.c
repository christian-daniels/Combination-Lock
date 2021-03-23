/*
 * Project5.c
 *
 * Created: 3/18/2021 10:51:47 AM
 * 
 */ 

#include <avr/io.h>
#include <stdio.h>
#include <math.h>
#include "avr.h"
#include "lcd.h"

#define LOCKED 0
#define UNLOCKED 1
#define BLOCKED 2
#define MANAGER 3
#define RINGUNLOCKED 34
#define RINGLOCKED 1
#define LOCKBASE 255
#define BLINK 0
#define DISPLAY 1
#define BLINKTIME 5000000
#define HOLD 1000000000
#define BLOCKTIME 60 //60 seconds

#define C1 3
#define D1 5
#define E1 7
#define B2 14
#define Q 3
#define H 2
#define ROOT_A 220

// GLOBALS
int managerpassword = 1234;
int state = UNLOCKED;
int isBlinking;
typedef struct {
	int first;
	int second;
	int third;
	int fourth;
	
} Password;
Password ps;

typedef struct{
	int frequency; // These values are CODE values - switches
	int duration;
} Note;

const Note soundPrompt[] = {{E1, Q},{B2, Q}};
const Note soundSuccessUnlock[] = {{C1, Q},{E1, Q},{B2, Q},}; // plays for both Lock and Unlock
const Note soundSuccessLock[] = {{C1, Q},{E1, Q},{C1, Q},};
const Note soundFailure[] = {{B2, Q},{B2, Q},{B2, Q},};	// plays for mis inputs
const Note soundManager[] = {{C1, Q},{C1, Q},{C1, Q},}; // plays when entering manager mode
int BPM = 180;
double timer_arg = 0.00001;
	
	
double getActualFrequency(int frequencyCode){
	
	return pow(2.0, (double)frequencyCode/12) * ROOT_A;
}

double getPeriod(int frequency){
	return 1/getActualFrequency(frequency);
}
double getActualDuration(int duration){
	// avatars love is 78bpm -> 1min/78beats in a minute -> 60/78bpm = 0.769s for each quarter note
	switch (duration)
	{
		// Quarter note
		case Q:
			return (double)60/BPM /4;
		// Half note
		case H:
			return ((double)60/BPM) /2;
		
		default:
			return (double)60/BPM;
	}
	
}


int calcCycles(double duration, double period){
	return duration/period;
}
int pause;
void playNote(int frequency, int duration){
	int i;
	int k; /* number of cycles */
	double p = getPeriod(frequency);
	k = calcCycles(getActualDuration(duration), p);
	
	double Ton = round((p/2) / 0.00001);
	double Toff = Ton;
	
	//printf("FrequencyCode: %d actual frequency was %f k was %d, p was %f, Ton/Toff was %f\n", frequency, getActualFrequency(frequency),k, p, Toff);
	for (i = 0; i < k; i++){
		SET_BIT(PORTA, 0);
		avr_wait(Ton);
		CLR_BIT(PORTA, 0);
		avr_wait(Toff);
		
	}
}

void playSound(const Note *sound, int n){
	int i;
	for (i = 0; i < n; i++){
		
		playNote(sound[i].frequency, sound[i].duration);	
	}
	
}

/*
	Keypad Functions
*/
int is_pressed(int r, int c)
{
	// Set DDRC to 0 - input mode
	DDRC = 0;
	// Set all pins to N/C
	PORTC = 0;
	CLR_BIT(DDRC, r);
	SET_BIT(PORTC, r);
	SET_BIT(DDRC, c + 4);
	CLR_BIT(PORTC, c + 4);
	
	// Check if PINC is 1 or not at c
	if (!GET_BIT(PINC, r))
	{
		return 1;
	}
	
	return 0;
}

int get_key(){
	int r,c;
	for(r = 0; r < 4; r++){
		for(c = 0; c < 4; c++){
			if (is_pressed(r, c))
			{
				return 1 +  (r*4 + c);
			}
		}
	}
	return 0;
}

int retrieveNumber(int k){
	
	if (k < 4 && k > 0)
	{
		return k;
	}
	else if (k > 4 && k < 8)
	{
		return k-1;
	}
	else if (k > 8 && k < 12)
	{
		return k - 2;
	}
	else{
		return 0;
	}
}

/*
	Display Functions
*/
void displayState(int time){
	lcd_clr();
	
	// First line displays Lock title and state
	char line1[17];
	if (state == UNLOCKED){
		sprintf(line1, "UCI LOCK      %c%c", RINGUNLOCKED, RINGLOCKED);
	}
	else if(state == BLOCKED){
		sprintf(line1, "BLOCKED       %c%c", RINGLOCKED, RINGLOCKED);
	}
	else{
		sprintf(line1, "UCI LOCK      %c%c", RINGLOCKED, RINGLOCKED);
	}
	
	
	lcd_pos(0,0);
	lcd_puts(line1);
	
	char line2[17];
	
	if (isBlinking == BLINK)
	{
		sprintf(line2, "              %c%c", LOCKBASE, LOCKBASE);
	}
	else{
		if (state == UNLOCKED){
			sprintf(line2, "SET W/ A      %c%c", LOCKBASE, LOCKBASE);
			
		}
		else if(state == BLOCKED){
			sprintf(line2, "FOR: %02d       %c%c",time,LOCKBASE, LOCKBASE);
		}
		else{
			sprintf(line2, "UNLOCK W/ A   %c%c", LOCKBASE, LOCKBASE);
		}
		
	}
	
	
	lcd_pos(1,0);
	lcd_puts(line2);
}

void displayCreatePassword(int count, const Password *ps){
	lcd_clr();
	
	// First line displays Lock title and state
	char line1[17];
	
	sprintf(line1, "SET COMBO     %c%c", RINGUNLOCKED, RINGLOCKED);
	
	lcd_pos(0,0);
	lcd_puts(line1);
	
	
	
	char line2[17];
	switch(count){
		case 0:
		sprintf(line2, "____          %c%c", LOCKBASE, LOCKBASE);
		break;
		case 1:
		sprintf(line2, "%d___          %c%c", ps->first, LOCKBASE, LOCKBASE);
		break;
		case 2:
		sprintf(line2, "%d%d__          %c%c",ps->first, ps->second, LOCKBASE, LOCKBASE);
		break;
		case 3:
		sprintf(line2, "%d%d%d_          %c%c",ps->first, ps->second, ps->third, LOCKBASE, LOCKBASE);
		break;
		case 4:
		sprintf(line2, "%d%d%d%d          %c%c",ps->first,ps->second, ps->third, ps->fourth, LOCKBASE, LOCKBASE);
		break;
		case 5:
		sprintf(line2, "%d%d%d%d          %c%c",ps->first,ps->second, ps->third, ps->fourth, LOCKBASE, LOCKBASE);
	}
	
	
	lcd_pos(1,0);
	lcd_puts(line2);
}

void displayGuessPassword(int count, const Password *ps){
	lcd_clr();
	
	// First line displays Lock title and state
	char line1[17];
	
	sprintf(line1, "ENTER COMBO   %c%c", RINGLOCKED, RINGLOCKED);
	
	lcd_pos(0,0);
	lcd_puts(line1);
	
	
	
	char line2[17];
	switch(count){
		case 0:
		sprintf(line2, "____          %c%c", LOCKBASE, LOCKBASE);
		break;
		case 1:
		sprintf(line2, "*___          %c%c", LOCKBASE, LOCKBASE);
		break;
		case 2:
		sprintf(line2, "**__          %c%c", LOCKBASE, LOCKBASE);
		break;
		case 3:
		sprintf(line2, "***_          %c%c", LOCKBASE, LOCKBASE);
		break;
		case 4:
		sprintf(line2, "****          %c%c", LOCKBASE, LOCKBASE);
		break;
		
	}
	
	
	lcd_pos(1,0);
	lcd_puts(line2);
}

void displayValidation(const Password *ps){
	lcd_clr();
	
	// First line displays Lock title and state
	char line1[17];
	if (state == UNLOCKED){
		sprintf(line1, "PASS: %d%d%d%d", ps->first, ps->second, ps->third, ps->fourth);
	}
	else{
		sprintf(line1, "PASS: ****");
	}
	
	lcd_pos(0,0);
	lcd_puts(line1);
	
	char line2[17];
	
	sprintf(line2, "ADV: A REDO: B");
	lcd_pos(1,0);
	lcd_puts(line2);
}

void displaySuccess(){
	lcd_clr();
	
	// First line displays Lock title and state
	char line1[17];
	if (state == MANAGER){
		sprintf(line1, "MANAGER CODE  %c%c", RINGUNLOCKED, RINGLOCKED);
	}
	else{
		sprintf(line1, "SUCCESS!      %c%c", RINGUNLOCKED, RINGLOCKED);
	}
	
	lcd_pos(0,0);
	lcd_puts(line1);
	
	char line2[17];
	if (state == MANAGER){
		sprintf(line2, "ENTERED       %c%c", LOCKBASE, LOCKBASE);
	}
	else{
		sprintf(line2, "UNLOCKING     %c%c", LOCKBASE, LOCKBASE);
	}
		
	lcd_pos(1,0);
	lcd_puts(line2);
	if (state == MANAGER){
		avr_wait(HOLD);
		sprintf(line1, "REMOVED PASS   %c%c", RINGUNLOCKED, RINGLOCKED);
		lcd_pos(0,0);
		lcd_puts(line1);
		sprintf(line2, "+ UNLOCKING    %c%c", LOCKBASE, LOCKBASE);
		lcd_pos(1,0);
		lcd_puts(line2);
		avr_wait(HOLD);
	}
}

void displayFailure(int tries){
	lcd_clr();
	
	// First line displays Lock title and state
	char line1[17];
	
		sprintf(line1, "FAILURE!      %c%c", RINGLOCKED, RINGLOCKED);
	lcd_pos(0,0);
	lcd_puts(line1);
	
	char line2[17];
	if (tries == 3){
		sprintf(line2, "BLOCKING      %c%c", LOCKBASE, LOCKBASE);
	}
	else{
		sprintf(line2, "%d TRIES LEFT  %c%c",tries, LOCKBASE, LOCKBASE);
	}
	lcd_pos(1,0);
	lcd_puts(line2);
}

void displayManager(){
	lcd_clr();
	
	// First line displays Lock title and state
	char line1[17];
	sprintf(line1, "ENTERING MANAGER");
	lcd_pos(0,0);
	lcd_puts(line1);
	
	char line2[17];
	sprintf(line2, "      MODE      ");
	lcd_pos(1,0);
	lcd_puts(line2);
	
}
// DEBUG function
void displayBothPasswords(const Password *ps, const Password *entered){
	lcd_clr();
	

	char line1[17];
	
	sprintf(line1, "PASS:%d%d%d%d",ps->first, ps->second, ps->third, ps->fourth);
	lcd_pos(0,0);
	lcd_puts(line1);
	
	char line2[17];
	
	sprintf(line2, "ENTERED: %d%d%d%d   ", entered->first, entered->second, entered->third, entered->fourth);
	
	lcd_pos(1,0);
	lcd_puts(line2);
}

/*
	Password Functions
*/
void initPassObj(Password *ps){
	ps->fourth = 0;
	ps->first = 0;
	ps->second = 0;
	ps->third = 0;
}

int userIsSure(){
	while(1){
		avr_wait(200);
		int a = get_key();
		if (a == 4){
			// user is sure
			return 1;
		}
		if(a == 8){
			// User is not sure
			return 0;
		}
	}
}

void createPassword(){
	
	int count = 0;
	Password newPassword;
	initPassObj(&newPassword);
	// Play Sound Prompt
	displayCreatePassword(count, &newPassword);
	playSound(soundPrompt, 2);
	while (1)
	{
		avr_wait(BLINKTIME);
		
		if (count == 4){
			// Ask user if they are sure
			
			// Display instructions
			displayValidation(&newPassword);
			//Read input
			if (userIsSure())
			{
				// Exit out of loop and start LOCK state
				break;
			}
			else{
				// Reset end condition set count to 0
				count = 0;
				initPassObj(&newPassword);
				// Notify user to enter a new password
			}
		}
		
		int k = get_key();
		if (((k > 0 && k < 4) || (k > 4 && k < 8) || (k > 8 && k < 12)) && count < 4){
			int converted = retrieveNumber(k);
			count++;
			switch (count)
			{
				case 1:
				newPassword.first = converted;
				break;
				case 2:
				newPassword.second = converted;
				break;
				case 3:
				newPassword.third = converted;
				break;
				case 4:
				newPassword.fourth = converted;
				break;
			}
			
			
			
		}
		displayCreatePassword(count, &newPassword);
		//displayState(count);
	}
	
	ps.first = newPassword.first;
	ps.second = newPassword.second;
	ps.third = newPassword.third;
	ps.fourth = newPassword.fourth;
}

int guessPassword(){
	Password enteredPassword;
	initPassObj(&enteredPassword);
	
	int count = 0;
	while (1)
	{
		avr_wait(BLINKTIME);
		
		// Code concerned with ensuring asking User if they entered the correct information
		if (count == 4){
			
			// Display instructions
			displayValidation(&enteredPassword);
			//Read input
			if (userIsSure())
			{
				// Exit out of loop and start LOCK state
				break;
			}
			else{
				// Reset end condition set count to 0
				count = 0;
				initPassObj(&enteredPassword);
				// Notify user to enter a new password
			}
		}
		
		// Code concerned with getting and saving user input
		int k = get_key();
		if (((k > 0 && k < 4) || (k > 4 && k < 8) || (k > 8 && k < 12)) && count < 4){
			int converted = retrieveNumber(k);
			count++;
			switch (count)
			{
				case 1:
				enteredPassword.first = converted;
				break;
				case 2:
				enteredPassword.second = converted;
				break;
				case 3:
				enteredPassword.third = converted;
				break;
				case 4:
				enteredPassword.fourth = converted;
				break;
			}
			
			
			
		}
		
		//avr_wait(BLINKTIME);
		displayGuessPassword(count, &enteredPassword);
		
	}
	
	// debug
	//displayBothPasswords(&ps, &enteredPassword);
	//Check if password is manager password
	if ((enteredPassword.first == 1) && (enteredPassword.second == 2) && (enteredPassword.third == 3) && (enteredPassword.fourth == 4)){
		return MANAGER;
	}
	
	if ((ps.first != enteredPassword.first) || (ps.second != enteredPassword.second) || (ps.third != enteredPassword.third) || (ps.fourth != enteredPassword.fourth))
	{
		return LOCKED;
	}
	return UNLOCKED;
	
}

int main(void)
{
	SET_BIT(DDRA, 0);
	lcd_init();
	
	initPassObj(&ps);
	isBlinking = DISPLAY;
	while (1)
	{
		
		displayState(0);
		avr_wait(BLINKTIME);
		
		int a = get_key();
		if (a == 4 && state == UNLOCKED){
			
			// Create a password
			createPassword(&ps);
			playSound(soundSuccessLock, 3);
			state = LOCKED;
		}
		else if(a == 4 && state == LOCKED){
			// Attempting to Unlock - Enter password	
			playSound(soundPrompt, 2);
			int tries = 0;	
			while(1){
			
			if (state == BLOCKED){
				for(int i = BLOCKTIME; i > 0; --i){
					int k = get_key();
					displayState(i);
					avr_wait(HOLD);
					if (k == 16)
					{
						// Enter manager mode
						// Display manager mode switch
						playSound(soundManager, 3);
						displayManager();
						avr_wait(HOLD);
						// Retrieve password
						int m = guessPassword(&ps);
						if (m == MANAGER){
							state = MANAGER;
							break;
						}
					}
				}
				if (state != MANAGER)
				{
					state = LOCKED;
				}
				else{
					// Display override
					displaySuccess();
					// play unlocked sound
					playSound(soundSuccessUnlock, 3);
					state = UNLOCKED;
					
					break;
				}
				
			}
			else{
				
				int k = guessPassword(&ps);
				if (k == UNLOCKED){
					// User got Correct password
					
					// Display Success
					displaySuccess();
					avr_wait(HOLD);
					// Play Sound
					playSound(soundSuccessUnlock, 3);
					// Change state
					state = UNLOCKED;
					break;
				}
				else{
					// User got Incorrect password
					// Track attempts to determine BLOCK
					tries++;
					// Display Failure - show number of tries
					displayFailure(3-tries);
					// Play Sound
					playSound(soundFailure, tries);
					avr_wait(HOLD);
					
					
					
				}
				
			}
				// if user gets password incorrect 3 times - wait for 60 seconds
				if (tries == 3)
				{
					tries = 0;
					state = BLOCKED;
				}
				
			}
			
			
		}
		isBlinking = BLINK;
		displayState(0);
		avr_wait(BLINKTIME);
		isBlinking = DISPLAY;
	}
}
