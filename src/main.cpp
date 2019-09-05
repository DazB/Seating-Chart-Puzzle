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

#define DELAY_TEXT_TIME 	1000
#define UID_LENGTH			4
#define MAX_READER_ATTEMPTS		10

#define NUMBER_OF_SEATS		5
#define BUTTON_PIN 			4
#define AIMEE_SS_PIN		5
#define PHIL_SS_PIN 		3
#define NAM_SS_PIN 			7
#define DAZ_SS_PIN 			8
#define SANJIT_SS_PIN 		9
#define RST_PIN 			10

#define AIMEE_SEAT 			0
#define PHIL_SEAT 			1
#define NAM_SEAT 			2
#define DAZ_SEAT 		 	3
#define SANJIT_SEAT 	 	4

const uint8_t AIMEE_TAG_UID[] 	= {0x49, 0xB2, 0x90, 0x6E};
const uint8_t PHIL_TAG_UID[] 	= {0x96, 0x70, 0x4D, 0xF9};
const uint8_t NAM_TAG_UID []	= {0xD9, 0x13, 0x8E, 0x6E};
const uint8_t DAZ_TAG_UID[]  	= {0x49, 0x87, 0x94, 0x6D};
const uint8_t SANJIT_TAG_UID[] 	= {0xA6, 0xA8, 0x6F, 0xF9};

const uint8_t SS_PINS[] = {AIMEE_SS_PIN, PHIL_SS_PIN, NAM_SS_PIN, DAZ_SS_PIN, SANJIT_SS_PIN};

typedef enum
{
	CORRECT = 0,
	PERSON_MISSING,
} SEATING_RESULT;

// Define Seat structure
typedef struct 
{
	MFRC522 reader;
	uint8_t correct_uid[UID_LENGTH];
	char* name;
} Seat;


Seat seats[NUMBER_OF_SEATS];   // Create array of all seats

LiquidCrystal_I2C lcd(0x27,16,2);  		// Create LCD instance. Set the LCD address to 0x27 for a 16 chars and 2 line display
const uint8_t heart_char[8] 	= {0x0,0xa,0x1f,0x1f,0xe,0x4,0x0};

void dump_byte_array(byte *buffer, byte bufferSize);
void dump_byte_array_lcd(byte *buffer, byte bufferSize);
void scroll_lcd_text_top(char* text);
void scroll_lcd_text_bot(char* text);

void setup() 
{	
	Serial.begin(9600);		// Initialize serial communications with the PC
	while (!Serial);		// Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)

	pinMode(BUTTON_PIN, INPUT);	// Set up button

	for (uint8_t seat = 0; seat < NUMBER_OF_SEATS; seat++)
	{
		pinMode(SS_PINS[seat], OUTPUT);
		digitalWrite(SS_PINS[seat], HIGH);
	}

	seats[AIMEE_SEAT].name 	= "Aimee";
	memcpy(seats[AIMEE_SEAT].correct_uid, AIMEE_TAG_UID, UID_LENGTH);
	seats[PHIL_SEAT].name	= "Phil";
	memcpy(seats[PHIL_SEAT].correct_uid, PHIL_TAG_UID, UID_LENGTH);
	seats[NAM_SEAT].name 	= "Nam";
	memcpy(seats[NAM_SEAT].correct_uid, NAM_TAG_UID, UID_LENGTH);
	seats[DAZ_SEAT].name 	= "Sexy Daz";
	memcpy(seats[DAZ_SEAT].correct_uid, DAZ_TAG_UID, UID_LENGTH);
	seats[SANJIT_SEAT].name = "Sanjo";
	memcpy(seats[SANJIT_SEAT].correct_uid, SANJIT_TAG_UID, UID_LENGTH);
	
	SPI.begin();				// Init SPI bus

	lcd.init();                      	// Initialize the lcd 
	// lcd.createChar(0, heart_char);		// Make the heart character
	lcd.backlight();
	lcd.clear();
	lcd.home();
	lcd.print("Press button");
	lcd.setCursor(0,1);
	lcd.print("when ready...");
}

void loop() 
{
	// If the button has been pushed
	if (digitalRead(BUTTON_PIN) == HIGH)
	{
		SEATING_RESULT seating_result; 	// Result of guess

		lcd.clear();
		lcd.home();
		lcd.print("Wait...");

  		for (uint8_t seat = 0; seat < NUMBER_OF_SEATS; seat++) 
		{
			Serial.print("On reader ");
			Serial.print(seat);
			Serial.println();
			
			seats[seat].reader.PCD_Init(SS_PINS[seat], RST_PIN);

			uint8_t retry_counter = 0;
			while (retry_counter < MAX_READER_ATTEMPTS)
			{
				if (seats[seat].reader.PICC_IsNewCardPresent() && seats[seat].reader.PICC_ReadCardSerial())
				{
					break;
				}
				
				Serial.print("Retry \n");
				retry_counter++;
				
				seats[seat].reader.PCD_Init(SS_PINS[seat], RST_PIN);
				delay(10);
			}

			if (retry_counter == MAX_READER_ATTEMPTS)
			{
				Serial.print("Fail :( \n");
				seating_result = PERSON_MISSING;
				break;
			}
			Serial.print("Success :D \n");
			seating_result = CORRECT;



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

			
		} 

		switch (seating_result)
		{
		case CORRECT:
			lcd.clear();
			lcd.home();
			scroll_lcd_text_top("Well fuckin done. You got it. Code word is \"CHICKS\"");
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
			break;

		default:
			break;
		}
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

		delay(250);
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

		delay(250);
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

