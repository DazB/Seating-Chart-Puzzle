/**
 * A, P, N, D, S
 * 
 * -----------------------------------------------------------------------------------------
 * Signal          	Device Pin 	Arduino Uno Pin  		   			           
 * -----------------------------------------------------------------------------------------
 * SPI SCK     		SCK         13 (Yellow)
 * SPI MISO		    MISO        12 (Green)
 * SPI MOSI    		MOSI        11 (Orange)
 *
 * SEATS RST		RST			10
 * AIMEE SPI SS     SDA(SS)     0
 * PHIL SPI SS		SDA(SS)		1	
 * NAM SPI SS		SDA(SS)		2
 * DAZ SPI SS		SDA(SS)		3
 * SANJIT SPI SS	SDA(SS)		4
 * 
 * BUTTON IN		BUTTON		9
 * BUTTON +			VCC			3v3
 * 
 * LCD + 			VCC			5v
 * LCD SCL			SCL			A5
 * LCD SDA			SDA			A4
 * 
 */

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>

#define UID_LENGTH				4
#define MAX_READER_ATTEMPTS		20

#define NUMBER_OF_PEOPLE		5

#define BUTTON_PIN 			9
#define AIMEE_SS_PIN		0
#define PHIL_SS_PIN 		1
#define NAM_SS_PIN 			2
#define DAZ_SS_PIN 			3
#define SANJIT_SS_PIN 		4
#define RST_PIN 			10

#define AIMEE 			0
#define PHIL 			1
#define NAM 			2
#define DAZ 		 	3
#define SANJIT	 	 	4

#define AIMEE_TAG_UID 	 {0x49, 0xB2, 0x90, 0x6E}
#define PHIL_TAG_UID  	 {0x96, 0x70, 0x4D, 0xF9}
#define NAM_TAG_UID 	 {0xD9, 0x13, 0x8E, 0x6E}
#define DAZ_TAG_UID  	 {0x49, 0x87, 0x94, 0x6D}
#define SANJIT_TAG_UID 	 {0xA6, 0xA8, 0x6F, 0xF9}

#define SCROLL_SPEED	300

typedef enum
{
	CORRECT = 0,
	PERSON_MISSING,
	PHIL_SPILL,
	AIMEE_TRIGGERED_RACISM,
	NAM_GETS_THE_GOSS,
	NO_SEXY_DAZ,
	WRONG
} SEATING_RESULT;

// Seat structure
typedef struct 
{
	MFRC522 reader;						// RFID reader
	uint8_t correct_uid[UID_LENGTH];	// The correct person who should be in this seat 
} Seat;

// Array of all tags
uint8_t TAG_UIDS[NUMBER_OF_PEOPLE][UID_LENGTH] = {AIMEE_TAG_UID, PHIL_TAG_UID, NAM_TAG_UID, DAZ_TAG_UID, SANJIT_TAG_UID};

// Array of slave pins
uint8_t SS_PINS[] = {AIMEE_SS_PIN, PHIL_SS_PIN, NAM_SS_PIN, DAZ_SS_PIN, SANJIT_SS_PIN};

// Array of peoples seating positions. This is used in our comparisons to check if they're right
uint8_t person_seating_position[NUMBER_OF_PEOPLE];

// Array of everyones seats 
Seat seats[NUMBER_OF_PEOPLE];   

// LCD instance. Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27,16,2);  		

// Heart <3
uint8_t heart_char[8] 	= {0x0,0xa,0x1f,0x1f,0xe,0x4,0x0};

// Prototype function headers
void seating_result_message(SEATING_RESULT result);
void scroll_lcd_text_top(char* text);

// Setup code. Runs once on startup
void setup() 
{	
	pinMode(BUTTON_PIN, INPUT);	// Set up button

	for (uint8_t seat = 0; seat < NUMBER_OF_PEOPLE; seat++)
	{
		// Set up slave pins
		pinMode(SS_PINS[seat], OUTPUT);
		digitalWrite(SS_PINS[seat], HIGH);
	
		// Initalise correct UID's for their seats
		memcpy(seats[seat].correct_uid, TAG_UIDS[seat], UID_LENGTH);
	}
	
	SPI.begin();				// Init SPI bus

	lcd.init();                      	// Initialize the lcd 
	lcd.createChar(0, heart_char);		// Make the heart character
	lcd.backlight();
	lcd.clear();
	lcd.home();
	lcd.print("Press button to");
	lcd.setCursor(0,1);
	lcd.print("assign seats...");
}

// Main loop
void loop() 
{
	// If the button has been pushed
	if (digitalRead(BUTTON_PIN) == HIGH)
	{
		// THE BUTTON HAS BEEN PRESSED. A GUESS HAS BEEN MADE

		lcd.clear();
		lcd.home();
		lcd.print("Wait...");

		// Go through every seat
  		for (uint8_t seat = 0; seat < NUMBER_OF_PEOPLE; seat++) 
		{
			Serial.print("On reader ");
			Serial.print(seat);
			Serial.println();
			
			// Cheap chinese fuckin reader needs a hard reset every time for any kind of reliability 
			seats[seat].reader.PCD_Init(SS_PINS[seat], RST_PIN);

			// First, we go through every reader to see if there is actually a tag on each one
			// We retry this a few times to make sure
			uint8_t retry_counter = 0;
			while (retry_counter < MAX_READER_ATTEMPTS)
			{
				// If we can read a card
				if (seats[seat].reader.PICC_IsNewCardPresent() && seats[seat].reader.PICC_ReadCardSerial())
				{
					// Can read and have now read. Break out of while loop
					break;
				}
				
				// Reinitialize and retry
				Serial.print("Retry \n");
				retry_counter++;
				seats[seat].reader.PCD_Init(SS_PINS[seat], RST_PIN);
				delay(10);
			}

			// If we've maxed out attempts
			if (retry_counter == MAX_READER_ATTEMPTS)
			{
				// There's someone missing
				Serial.print("Someone missing :( \n");
				// Show message and start again
				seating_result_message(PERSON_MISSING);
				return;
			}
		}

		// Check all seats again and get everyones seating position (there is a disgusting amount of for loops in this code)
  		for (uint8_t seat = 0; seat < NUMBER_OF_PEOPLE; seat++) 
		{
			// O(n2) bbbbbby
			for (uint8_t person = 0; person < NUMBER_OF_PEOPLE; person++)
			{
				// If we have found the identity of the person sitting here
				if (memcmp(seats[seat].reader.uid.uidByte, TAG_UIDS[person], UID_LENGTH) == 0)
				{
					// Save the persons position
					person_seating_position[person] = seat;
				}
			}
		}
		
		/************************************
		 * Check Phil Spill
		 ***********************************/
		// If Phil is sitting far left (like his politics)
		if (person_seating_position[PHIL] == 0)
		{
			// If Sanjit is on his right
			if (person_seating_position[PHIL] == (person_seating_position[SANJIT] - 1))
			{
				seating_result_message(PHIL_SPILL);
				return;
			}
		}
		// Else if Phil is sitting far right (like his personality)
		else if (person_seating_position[PHIL] == (NUMBER_OF_PEOPLE - 1))
		{
			// If Sanjit is on his left
			if (person_seating_position[PHIL] == (person_seating_position[SANJIT] + 1))
			{
				seating_result_message(PHIL_SPILL);
				return;
			}	
		}
		// Phil isn't on the ends of the table, lets see if Sanjo is next to him
		else if ((person_seating_position[PHIL] == (person_seating_position[SANJIT] - 1)) ||
			person_seating_position[PHIL] == (person_seating_position[SANJIT] + 1))
		{
			seating_result_message(PHIL_SPILL);
			return;
		}


		/***********************************
		 * Aimee #triggered
		 ***********************************/
		// If Aimee is sitting far left (like her politics)
		if (person_seating_position[AIMEE] == 0)
		{
			// If Sanjit or Daz are on her right
			if ((person_seating_position[AIMEE] == (person_seating_position[SANJIT] - 1)) ||
			(person_seating_position[AIMEE] == (person_seating_position[DAZ] - 1)))
			{
				seating_result_message(AIMEE_TRIGGERED_RACISM);
				return;
			}
		}
		// Else if Aimee is sitting far right (like her fiance)
		else if (person_seating_position[AIMEE] == (NUMBER_OF_PEOPLE - 1))
		{
			// If Sanjit or Daz are on her left
			if ((person_seating_position[AIMEE] == (person_seating_position[SANJIT] + 1)) ||
			(person_seating_position[AIMEE] == (person_seating_position[DAZ] + 1)))
			{
				seating_result_message(AIMEE_TRIGGERED_RACISM);
				return;
			}	
		}
		// Aimee isn't on the ends of the table, lets see if Sanjo or Dazzy boi are next to her
		else if ((person_seating_position[AIMEE] == (person_seating_position[SANJIT] - 1)) ||
			(person_seating_position[AIMEE] == (person_seating_position[DAZ] - 1)) ||
			(person_seating_position[AIMEE] == (person_seating_position[SANJIT] + 1)) ||
			(person_seating_position[AIMEE] == (person_seating_position[DAZ] + 1)))
		{
			seating_result_message(AIMEE_TRIGGERED_RACISM);
			return;
		}

		/***********************************
		 * Nam get's the goss
		 ***********************************/
		// If Nam is sitting far left (the politics joke is old now)
		if (person_seating_position[NAM] == 0)
		{
			// If Sanjit or Aimee are on her right
			if ((person_seating_position[NAM] == (person_seating_position[SANJIT] - 1)) ||
			(person_seating_position[NAM] == (person_seating_position[AIMEE] - 1)))
			{
				seating_result_message(NAM_GETS_THE_GOSS);
				return;
			}
		}
		// Else if Nam is sitting far right
		else if (person_seating_position[NAM] == (NUMBER_OF_PEOPLE - 1))
		{
			// If Sanjit or Aimee are on her left
			if ((person_seating_position[NAM] == (person_seating_position[SANJIT] + 1)) ||
			(person_seating_position[NAM] == (person_seating_position[AIMEE] + 1)))
			{
				seating_result_message(NAM_GETS_THE_GOSS);
				return;
			}	
		}
		// Nam isn't on the ends of the table, lets see if Sanjo or Aims are next to her
		else if ((person_seating_position[NAM] == (person_seating_position[SANJIT] - 1)) ||
			(person_seating_position[NAM] == (person_seating_position[AIMEE] - 1)) ||
			(person_seating_position[NAM] == (person_seating_position[SANJIT] + 1)) ||
			(person_seating_position[NAM] == (person_seating_position[AIMEE] + 1)))
		{
			seating_result_message(NAM_GETS_THE_GOSS);
			return;
		}

		/***********************************
		 * Daz no look good
		 ***********************************/
		if (person_seating_position[NAM] > person_seating_position[DAZ])
		{
			seating_result_message(NO_SEXY_DAZ);
			return;
		}

		/***********************************
		 * Are they correct???????
		 ***********************************/
		// Let's make sure they're right
  		for (uint8_t seat = 0; seat < NUMBER_OF_PEOPLE; seat++) 
		{
			// Compare the read uid with the one that is meant to be on the seat
			if (memcmp(seats[seat].reader.uid.uidByte, seats[seat].correct_uid, UID_LENGTH) != 0)
			{
				// They are different, which means they are wrong.
				// This shouldn't be the case cos we should have caught it above, but just in case. 
				seating_result_message(WRONG);
				return; 
			}
		}

		// The bastards got it
		// Show winning message. This will loop forever, so will require a hard reset to play again
		seating_result_message(CORRECT);
		lcd.clear();
		lcd.home();
		lcd.print("Codeword: CHICKS");
		// LOOP FOREVER
		// Do a little heart dance on the bottom
		while(1)
		{
			lcd.setCursor(0,1);
			for (uint8_t i = 0; i < 8; i++)
			{
				lcd.write(0);
				lcd.print(" ");
			}
			delay(1000);
			lcd.setCursor(0,1);
			for (uint8_t i = 0; i < 8; i++)
			{
				lcd.print(" ");
				lcd.write(0);
			}
			delay(1000);
		}
	}
}

/**
 * Shows a message based on the result of a guess on the LCD
 */
void seating_result_message(SEATING_RESULT result)
{
		switch (result)
		{
		case CORRECT:
			lcd.clear();
			lcd.home();
			lcd.setCursor(0,1);
			lcd.print("YAY YAY YAY YAY YAY YAY YAY YAY YAY ");
			scroll_lcd_text_top("Well bloody done. You've got it. Everyone is happily seated :D");
			delay(1000);
			break;
		
		case PERSON_MISSING:
			lcd.clear();
			lcd.home();
			lcd.setCursor(0,1);
			lcd.print("RUDE RUDE RUDE RUDE RUDE RUDE RUDE RUDE ");
			scroll_lcd_text_top("There's someone bloody missing!! That's a little RUDE of you don't you think?");
			delay(1000);
			lcd.clear();
			lcd.home();
			lcd.print("Try again!");
			delay(1000);
			lcd.clear();
			lcd.home();
			lcd.print("Press button to");
			lcd.setCursor(0,1);
			lcd.print("assign seats...");
			break;

		case PHIL_SPILL:
			lcd.clear();
			lcd.home();
			lcd.setCursor(0,1);
			lcd.print("SPILL SPILL SPILL SPILL SPILL SPILL SPIL");
			scroll_lcd_text_top("Great. Phil has spilled all his wine on Sanjit. THE ELBOWS ARE LOOSE NOW TOO OH MY GOD");
			delay(1000);
			lcd.clear();
			lcd.home();
			lcd.print("Try again!");
			delay(1000);
			lcd.clear();
			lcd.home();
			lcd.print("Press button to");
			lcd.setCursor(0,1);
			lcd.print("assign seats...");
			break;

		case AIMEE_TRIGGERED_RACISM:
			lcd.clear();
			lcd.home();
			lcd.setCursor(0,1);
			lcd.print("CASUAL RACISM CASUAL RACISM CASUAL RACIS");
			scroll_lcd_text_top("Oh no. Aimee is not happy because someone next to her said the n-word");
			delay(1000);
			lcd.clear();
			lcd.home();
			lcd.print("Try again!");
			delay(1000);
			lcd.clear();
			lcd.home();
			lcd.print("Press button to");
			lcd.setCursor(0,1);
			lcd.print("assign seats...");
			break;
			
		case NAM_GETS_THE_GOSS:
			lcd.clear();
			lcd.home();
			lcd.setCursor(0,1);
			lcd.print("DAZ GOSS DAZ GOSS DAZ GOSS DAZ GOSS DAZ ");
			scroll_lcd_text_top("One of you bastards told Nam about my bard character in DnD. Now she's not talking to me");
			delay(1000);
			lcd.clear();
			lcd.home();
			lcd.print("Try again!");
			delay(1000);
			lcd.clear();
			lcd.home();
			lcd.print("Press button to");
			lcd.setCursor(0,1);
			lcd.print("assign seats...");
			break;

		case NO_SEXY_DAZ:
			lcd.clear();
			lcd.home();
			lcd.setCursor(0,1);
			lcd.print("DAZ NOT HOT DAZ NOT HOT DAZ NOT HOT ");
			scroll_lcd_text_top("Nam just looked at me from the left for the first time and she's disgusted");
			delay(1000);
			lcd.clear();
			lcd.home();
			lcd.print("Try again!");
			delay(1000);
			lcd.clear();
			lcd.home();
			lcd.print("Press button to");
			lcd.setCursor(0,1);
			lcd.print("assign seats...");
			break;

		default:
			lcd.clear();
			lcd.home();
			scroll_lcd_text_top("Something fucked has happened. You should never get here, but fuck it I'll put something here anyway");
			break;
		}
}

/**
 * Scrolls display while writting text on top line 
 */
void scroll_lcd_text_top(char* text)
{
	int string_length = strlen(text);
	uint8_t lcd_position = 16;

	lcd.setCursor(lcd_position, 0);  // Set the cursor to column 15, line 0

	for (uint8_t text_position = 0; text_position < string_length; text_position++)
	{
		lcd.print(text[text_position]);
		lcd.scrollDisplayLeft(); // Scrolls the contents of the display one space to the left
		lcd_position++;

		if (lcd_position == 40)
		{
			lcd_position = 0;
			lcd.setCursor(lcd_position, 0);  // Set the cursor to column 1, line 0
		}

		delay(SCROLL_SPEED);
	}
}

