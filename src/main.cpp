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
 * seats RST		RST			10
 * AIMEE SPI SS     SDA(SS)     5   
 * PHIL SPI SS		SDA(SS)		6	
 * NAM SPI SS		SDA(SS)		7
 * DAZ SPI SS		SDA(SS)		8
 * SANJIT SPI SS	SDA(SS)		9
 */

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>

#define UID_LENGTH				4
#define MAX_READER_ATTEMPTS		20

#define NUMBER_OF_PEOPLE		5

#define BUTTON_PIN 			4
#define AIMEE_SS_PIN		5
#define PHIL_SS_PIN 		3
#define NAM_SS_PIN 			7
#define DAZ_SS_PIN 			8
#define SANJIT_SS_PIN 		9
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

typedef enum
{
	CORRECT = 0,
	PERSON_MISSING,
	PHIL_SPILL,
	AIMEE_TRIGGERED_RACISM,
	NAM_GETS_THE_GOSS,
	NO_SEXY_DAZ
} SEATING_RESULT;

// Seat structure
typedef struct 
{
	MFRC522 reader;						// RFID reader
	uint8_t correct_uid[UID_LENGTH];	// The correct person who should be in this seat 
} Seat;

// Person structure
typedef struct 
{
	uint8_t uid[UID_LENGTH];	// UID of person
	uint8_t seat_position;		// Position of person
} Person;

// Array of all tags
uint8_t TAG_UIDS[NUMBER_OF_PEOPLE][UID_LENGTH] = {AIMEE_TAG_UID, PHIL_TAG_UID, NAM_TAG_UID, DAZ_TAG_UID, SANJIT_TAG_UID};

// Array of slave pins
const uint8_t SS_PINS[] = {AIMEE_SS_PIN, PHIL_SS_PIN, NAM_SS_PIN, DAZ_SS_PIN, SANJIT_SS_PIN};

// Array of peoples seating positions
uint8_t person_seating_position[NUMBER_OF_PEOPLE];

// Array of everyones seats 
Seat seats[NUMBER_OF_PEOPLE];   
LiquidCrystal_I2C lcd(0x27,16,2);  		// Create LCD instance. Set the LCD address to 0x27 for a 16 chars and 2 line display
const uint8_t heart_char[8] 	= {0x0,0xa,0x1f,0x1f,0xe,0x4,0x0};

void seating_result_message(SEATING_RESULT result);
void dump_byte_array(byte *buffer, byte bufferSize);
void dump_byte_array_lcd(byte *buffer, byte bufferSize);
void scroll_lcd_text_top(char* text);
void scroll_lcd_text_bot(char* text);

// Setup code. Runs once on startup
void setup() 
{	
	Serial.begin(9600);		// Initialize serial communications with the PC
	while (!Serial);		// Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)

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
	// lcd.createChar(0, heart_char);		// Make the heart character
	lcd.backlight();
	// lcd.setContrast();
	lcd.clear();
	lcd.home();
	lcd.print("Press button");
	lcd.setCursor(0,1);
	lcd.print("when ready...");
}

// Main loop
void loop() 
{
	// If the button has been pushed
	if (digitalRead(BUTTON_PIN) == HIGH)
	{
		// THE BUTTON HAS BEEN PRESSED. A GUESS HAS BEEN MADE

		SEATING_RESULT seating_result = CORRECT; 	// Result of guess

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

		// Now we've looked at every seat and everyone is here, lets check if they're right
  		for (uint8_t seat = 0; seat < NUMBER_OF_PEOPLE; seat++) 
		{
			// Compare the read uid with the one that is meant to be on the seat
			if (memcmp(seats[seat].reader.uid.uidByte, seats[seat].correct_uid, UID_LENGTH) != 0)
			{
				// They are different, which means they are wrong. SAD
				seating_result = PERSON_MISSING; // I know a person isn't missing, but this acts as a flag for now
				break; // break from for loop
			}
		}

		// If the bastards got it
		if (seating_result == CORRECT)
		{
			// Show winning message. This will loop forever, so will require a hard reset to play again
			seating_result_message(CORRECT);
		}

		// So we know they're wrong. Let's figure out how wrong they are

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

		Serial.print("phil pos: ");
		Serial.print(person_seating_position[PHIL]);
		Serial.println();
		Serial.print("sanjo pos: ");
		Serial.print(person_seating_position[SANJIT]);
		Serial.println();
		
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


    		// // Look for new cards
			// if ((seats[seat].reader.PICC_IsNewCardPresent()) && (seats[seat].reader.PICC_ReadCardSerial())) 
			// {
			// 	Serial.print("seat ");
			// 	Serial.print(seat);
			// 	// Show some details of the PICC (that is: the tag/card)
			// 	Serial.print(": Card UID: " );
			// 	dump_byte_array(seats[seat].reader.uid.uidByte, seats[seat].reader.uid.size);
			// 	Serial.println();
			// 	Serial.print("size: ");
			// 	Serial.print(seats[seat].reader.uid.size, HEX);
			// 	Serial.println();
			// }
			
			// delay(10);

		lcd.print("End");

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
			scroll_lcd_text_top("Well fuckin done. You got it. Code word is \"CHICKS\"");
			while(1);	// LOOP FOREVER!!!!!!!!!!!!!!!
			break;
		
		case PERSON_MISSING:
			lcd.clear();
			lcd.home();
			lcd.setCursor(0,1);
			lcd.print("RUDE RUDE RUDE RUDE RUDE RUDE RUDE RUDE ");
			scroll_lcd_text_top("There's someone bloody missing!!   That's a little RUDE of you don't you think?");
			delay(1000);
			lcd.clear();
			lcd.home();
			lcd.print("Try again!");
			delay(1000);
			lcd.clear();
			lcd.home();
			lcd.print("Press button");
			lcd.setCursor(0,1);
			lcd.print("when ready...");
			break;

		case PHIL_SPILL:
			lcd.clear();
			lcd.home();
			lcd.setCursor(0,1);
			lcd.print("SPILL SPILL SPILL SPILL SPILL SPILL SPIL");
			scroll_lcd_text_top("Great. Now Phil has spilled all his wine on Sanjo. THE ELBOWS ARE LOOSE NOW TOO OH MY GOD");
			delay(1000);
			lcd.clear();
			lcd.home();
			lcd.print("Try again!");
			delay(1000);
			lcd.clear();
			lcd.home();
			lcd.print("Press button");
			lcd.setCursor(0,1);
			lcd.print("when ready...");		
			break;

		default:
			break;
		}
}

/**
 * Compares uid arrays to see if they are equal
 * @returns bool true if equal, false if not equal
 */
bool check_uid_same(byte* read_uid, byte* correct_uid)
{
	for (uint8_t i; i < UID_LENGTH; i++)
	{
		if (read_uid[i] != correct_uid[i])
		{
			return false;
		}
	}
	return true;
}

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

		delay(225);
	}
}

void scroll_lcd_text_bot(char* text)
{
	int string_length = strlen(text);
	uint8_t lcd_position = 16;

	lcd.setCursor(lcd_position, 1);  // Set the cursor to column 15, line 1

	for (uint8_t text_position = 0; text_position < string_length; text_position++)
	{
		lcd.print(text[text_position]);
		lcd.scrollDisplayLeft(); // Scrolls the contents of the display one space to the left
		lcd_position++;

		if (lcd_position == 40)
		{
			lcd_position = 0;
			lcd.setCursor(lcd_position, 1);  // Set the cursor to column 1, line 1
		}

		delay(225);
	}
}

/**
 * Helper routine to dump a byte array as hex values to Serial.
 */
void dump_byte_array(byte *buffer, byte bufferSize) 
{
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void dump_byte_array_lcd(byte *buffer, byte bufferSize)
{
  for (byte i = 0; i < bufferSize; i++) {
    lcd.print(buffer[i] < 0x10 ? " 0" : " ");
    lcd.print(buffer[i], HEX);
  }
}

