//Start by including the needed libraries
//Make sure that you have included these .zip libraries in your Arduino installation
#include <LiquidCrystal.h>  //Allows us to drive the LCD Display
#include "RTClib.h"         //Allows us to interface with the Real Time Clock (RTC)

RTC_DS3231 rtc;             //Define the RTC name

//Initialize the I/O Pins we will use
#define Buzzer    9
#define SwitchUp  10
#define SwitchDown  12

//Select the pins used on the LCD panel
LiquidCrystal lcd(2, 8, 4, 5, 6, 7);

//Initialize the date/time object
DateTime now;

//Initialize the variables we will use
int lastSecond = 0;           //Will be used to track screen updates
int AlarmHour = 0;            //24-Hour value of the alarm
int AlarmHourDisplay = 0;     //12-Hour Value of the alarm
int AlarmMinute = 0;          //Minute value of the alarm
int anaAlarm;                 //Raw analog pin value for alarm dial 
int mappedAlarm;              //Mapped analog value for alarm dial
int currentHour = 0;          //Hour placeholder used for handling Daylight Saving Time (DST) offsets
int DST = 0;                  //Used to Enable/Disable Daylight Saving Time (DST)
bool Alarmampm;               //Boolean for alarm AM or PM
bool AlarmSet = false;        //Boolean for whether alarm is set or not
bool ampm = true;             //Boolean for whether current time is AM or PM
bool semicol = true;          //Boolean for displaying Semi-Colon on LCD




void setup() {
  //Initialize the Serial Comms
  Serial.begin(57600);

  //Check to see if the RTC can be found
  //If it isn't available, abort
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }

  //Check if the RTC has lost power since it was last connected/setup
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    
    // The line below sets the RTC with an explicit date & time, for example to set
    // July 22, 2020 at 4pm you would call:
    // rtc.adjust(DateTime(2020, 7, 22, 16, 0, 0));
  }

  //initialize the custom characters and clear the LCD
  lcdcharinitiate();
  lcd.clear();

  //Configure I/O pin modes
  pinMode(Buzzer, OUTPUT);
  pinMode(SwitchUp, INPUT);
  pinMode(SwitchDown, INPUT);

  //Read the Dial Potentiometer value
  anaAlarm = analogRead(A1);
  
  //If the Dial potentiometer is rotated completely counter-clockwise or completely clockwise
  //AND if the 3-position rocker switch is in position "II"
  //Then adjust the time of the RTC to the time that the sketch was compiled and set it as the "Current Time"
  if(digitalRead(SwitchUp) == 1 && (anaAlarm < 150 || anaAlarm > 850))
  {
    DateTime compileDate(__DATE__, __TIME__);
    //If the Daylight Saving Time (DST) is active, adjust the RTC value to NON-DST
    if(anaAlarm < 150){//If Dial Potentiometer is rotated fully counter-clockwise, user has input DST is currently active
      DST = 1;
      if(compileDate.hour() >= 1){//If the hour is after 1AM, offset the hour backward by 1 to reference the NON-DST time
        currentHour = compileDate.hour() - 1;
      }
      else{//If the hour is between midnight and 1AM, offset the hour backward by 1 to reference NON-DST time
        currentHour = 23;
      }
      rtc.adjust(DateTime(compileDate.year(),compileDate.month(),compileDate.day(),currentHour,compileDate.minute(),compileDate.second()));
    }
    else if(anaAlarm > 850){//If Dial Potentiometer is rotated fully clockwise, user has input DST is currently inactive
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      DST = 0;
    }
  }
}


void loop()
{
  //Two core loop functions:
  //Update time is responsible for updating the time displayed on the LCD
  update_Time();

  //Update alarm is responsible for updating and sounding the alarm when it is time
  update_Alarm();
}


//Check if the displayed time needs to be updated
void update_Time()
{
  //Take the current time, and check if it is a new second
  //If it is, then display the new time with thedisplayfunction()
  now = rtc.now();
  //Load the RTC hour value into the currentHour placeholder and adjust for Daylight Saving Time being Enabled/Disabled
  if(DST == 1){//If DST is active
    if(now.hour() <= 22){//If it is before 11PM, add 1 Hour to the time
      currentHour = now.hour() + 1;
    }
    else{//If it is after 11PM, add 1 Hour to the time w/o overrunning the bound
      currentHour = 0;
    }
  }
  else{//If DST is inactive, then the RTC time does not need to be offset
    currentHour = now.hour();
  }
  
  if(now.second() != lastSecond)
  {
    //Pass the current hour, minute, and second to thedisplayfunction()
    thedisplayfunction(currentHour, now.minute(), now.second());
  }
  //Set the last second to the value currently displayed
  lastSecond = now.second();
}



void update_Alarm()
{
  //Check if the switch is in the "Alarm On" position
  if(digitalRead(SwitchUp) == 1)
  {
    //Check if an alarm has been set since the Arduino booted up
    if(AlarmSet == true)
    {
      //Set the LCD cursor position to the first line, and 19th position
      lcd.setCursor(19,0);
      //Write '*' to that position to signify the alarm is "ON"
      lcd.print("*");

      //Check if it is time to activate the alarm, give a 2-second window for the alarm to activate
      if(currentHour==AlarmHour && now.minute()==AlarmMinute && now.second()>=0 && now.second()<=2)
      {
        //While the Alarm switch is in the "ON" position, sound buzzer
        while(digitalRead(SwitchUp)==1)
        {
          //You can customize the tone frequency, and pulse duration by adjusting tone() below
          //In tone(x, y, z), x is the buzzer output pin, y is the frequency of the sound, z if the duration in milliseconds
          tone(Buzzer, 1000, 500);
          delay(100);
          tone(Buzzer, 2000, 500);
          delay(100);
          //Update the time displayed so the display does not appear frozen
          update_Time();
        }
      }
    }
    else
    {
      //Display "No Alarm Set"
      lcd.clear();
      //Set the LCD cursor to the second line, 4th position
      lcd.setCursor(4,1);
      //Write 'No Alarm Set' beginning at that location
      lcd.print("No Alarm Set");
      //Display the message for 1-second
      delay(1000);
      //Clear the display to prepare for the next time update
      lcd.clear();
    }
  }
  else if(digitalRead(SwitchDown) == 1)//Else, if switch is in "Set Alarm" position
  {
    //Clear the LCD, and set the cursor position like before, and print 'Set Alarm for:'
    lcd.clear();
    lcd.setCursor(3,1);
    lcd.print("Set Alarm for:");
    //While the switch is in the "Set Alarm" position, read the analog value from the dial, and update the time to set the alarm for
    while(digitalRead(SwitchDown) == 1)
    {
      //Read the dial value
      anaAlarm = analogRead(A1);

      //If the alarm is in the range to enable/disable DST (Daylight Saving Time), then display the appropriate message and Enable or Disable it below
      if(anaAlarm < 150){
        //Enable DST
        DST = 1;
        lcd.clear();
        lcd.setCursor(2,1);
        lcd.print("Switch to 'O' to");
        lcd.setCursor(5,2);
        lcd.print("ENABLE DST");
      }
      else if(anaAlarm > 850){
        //Disable DST
        DST = 0;
        lcd.clear();
        lcd.setCursor(2,1);
        lcd.print("Switch to 'O' to");
        lcd.setCursor(4,2);
        lcd.print("DISABLE DST");
      }

      //If the value is in the alarm setting range, process the internal code
      if(anaAlarm >= 150 and anaAlarm <= 850)
      {
        if(anaAlarm < 200)
        {
          anaAlarm = 200;
        }
        else if(anaAlarm > 800)
        {
          anaAlarm = 800;
        }
        
        lcd.clear();
        lcd.setCursor(3,1);
        lcd.print("Set Alarm for:");
        //Map the value to 24 hours divided into 15-minute intervals
        mappedAlarm = map(anaAlarm, 200, 800, 0, 95);
        //Calculate the minute value of the mappedAlarm
        //Multiply by 15, since alarm will be set in 15 minute increments, then take the modulo of that to get the minutes value
        AlarmMinute = (mappedAlarm * 15) % 60;
        //Calculate the hour value of the mappedAlarm
        //Remove the Minute offset from the mapped value to get the hour, divide by 4 to convert from 15 minute increments to hours
        AlarmHour = ((mappedAlarm - (AlarmMinute / 15)) / 4);
        //If the alarm hour is after 12:XX pm, shift the displayed time to 1 pm instead of 13:XX
        //and display appropriate 'AM' or 'PM' Value
        if(AlarmHour > 12)
        {
          AlarmHourDisplay = AlarmHour - 12;
          Alarmampm = false;
        }
        else if(AlarmHour == 12)
        {
          AlarmHourDisplay = AlarmHour;
          Alarmampm = false;
        }
        else if(AlarmHour == 0)
        {
          AlarmHourDisplay = 12;
          Alarmampm = true;
        }
        else
        {
          AlarmHourDisplay = AlarmHour;
          Alarmampm = true;
        }
  
        //Set cursor, and print leading '0' if alarm value is less than 10
        //This keeps the numbers in the same position instead of shifting around
        lcd.setCursor(6,2);
        if(AlarmHourDisplay < 10)
        {
          lcd.print("0");
        }
        //Print the Alarm hour value
        lcd.print(AlarmHourDisplay);
        lcd.print(":");
        //Print the same leading '0' for the minutes value if less than 10 
        if(AlarmMinute < 10)
        {
          lcd.print("0");
        }
        //Print the Alarm minute value and the appropriate 'AM' or 'PM'
        lcd.print(AlarmMinute);
        lcd.print(" ");
        if(Alarmampm == true)
        {
          lcd.print("AM");
        }
        else{
          lcd.print("PM");
        }
      }

      
      delay(100);
    }
    
    //After alarm has been set, display 'Alarm set for:'
    lcd.clear();
    lcd.setCursor(3,1);
    lcd.print("Alarm set for:");
    lcd.setCursor(6,2);
    //Print a leading '0' if less than 10 and display the hour value of the alarm
    if(AlarmHour < 10)
    {
      lcd.print("0");
    }
    lcd.print(AlarmHourDisplay);
    lcd.print(":");
    //Print a leading '0' if less than 10 and display the minute value of the alarm
    if(AlarmMinute < 10)
    {
      lcd.print("0");
    }
    lcd.print(AlarmMinute);
    lcd.print(" ");
    //Print he appropriate 'AM' or 'PM'
    if(Alarmampm == true)
    {
      lcd.print("AM");
    }
    else{
      lcd.print("PM");
    }
    //Display the time the alarm is set, for four seconds then clear the LCD
    delay(4000);
    lcd.clear();
    AlarmSet = true;
    //Update the current time to be displayed
    update_Time();
  }
  else
  {   
    //Set cursor to line 1, position 19, and ensure nothing is printed there
    //This clears the '*' that could be printed there without clearing the screen
    lcd.setCursor(19,0);
    lcd.print(" ");
  }
}


// Print the stylized current time to the LCD screen
void thedisplayfunction(int theHour, int theMinute, int theSecond)
{
  //Since this function is called roughly once per second, toggle the semicolon display
  //This has the effect of having the semicolons blink on and off every other second
  semicol=!semicol;

  //If the hour is greater than 12 (1pm or later), shift the value back to display in 12-hour mode
  //and set the appropriate 'AM' or 'PM' value
  if(theHour > 12)
  {
    theHour = theHour - 12;
    ampm = false;
  }
  else if(theHour == 12)
  {
    ampm = false;
  }
  else if(theHour == 0)
  {
    theHour = 12;
    ampm = true;
  }
  else
  {
    ampm = true;
  }

  //Call the numberprinter() function to print the stylized characters for each digit of the current time
  //In numberprinter(x, y), x is the value to be written (0-9), and y is the start position of the number on the LCD
  //Print the minute ones-place value
  numberprinter(theMinute % 10,13);
  //Print the minute tens-place value
  numberprinter(theMinute / 10,9);
  //Print the hour ones-place value
  numberprinter(theHour % 10,5);
  //Print the hour tens-place value
  numberprinter(theHour / 10,1);

  //In secondprinter(x, y), x is the value to be written (0-9), and y is the start position of the number on the LCD
  //Print the second tens-place value
  secondprinter(theSecond / 10 ,17);
  //Print the second ones-place value
  secondprinter(theSecond % 10 ,18);

  //Display the appropriate 'AM' or 'PM' value by calling lcdam() or lcdpm()
  //In lcdxm(y), x is the A or P in am or pm, and y is the start position of the print
  if(ampm)
  {
    lcdam(17);
  }
  else
  {
    lcdpm(17);
  }

  //Check to see if you should display the semicolon this second
  //Then pass the Semicolon start locations to the lcdnumbersemicolon() or lcdnumbersemicolonoff() function
  if(semicol)
  {
    lcdnumbersemicolon(8);
    lcdnumbersemicolon(16);
  }
  else
  {
    lcdnumbersemicolonoff(8);
    lcdnumbersemicolonoff(16);
  }
}


//Function to print the second value to the LCD screen
void secondprinter(int num, int pos)
{
  //Set the cursor to the x-position that has been passed to the function, and the second line
  lcd.setCursor(pos, 1);

  //Determine which decimal value (0-9) to write, and write that value to the LCD screen using a signed-byte encoding value
  //You can check the ASCII chart availble on this products page to see different encoding values and how the LCD will interpret them
  if(num == 0)
  {
    lcd.write(byte(48));
  }
  if(num == 1)
  {
    lcd.write(byte(49));
  }
  if(num == 2)
  {
    lcd.write(byte(50));
  }
  if(num == 3)
  {
    lcd.write(byte(51));
  }
  if(num == 4)
  {
    lcd.write(byte(52));
  }
  if(num == 5)
  {
    lcd.write(byte(53));
  }
  if(num == 6)
  {
    lcd.write(byte(54));
  }
  if(num == 7)
  {
    lcd.write(byte(55));
  }
  if(num == 8)
  {
    lcd.write(byte(56));
  }
  if(num == 9)
  {
    lcd.write(byte(57));
  }
}


//Function to find the needed stylized characer and add it to the needed position
void numberprinter(int num , int pos)
{
  //Determine which stylized number needs to be printed, and call that function
  //lcdnumberx(y) where x is '0-9' and y is the position passed to the function
  if(num == 0)
  {
    //This is a toggle to determine whether to print a leading zero or not
    //It is currently set to print a leading zero for minutes, but not hours
    if(pos == 1)
    {
      //This prints an empty space at the stylized number's location
      lcdempty(pos); 
    }
    else
    {
      lcdnumber0(pos);
    }
  }
  if(num == 1)
  {
    lcdnumber1(pos);
  }
  if(num == 2)
  {
    lcdnumber2(pos);
  }
  if(num == 3)
  {
    lcdnumber3(pos);
  }
  if(num == 4)
    lcdnumber4(pos);
  {
  }
  if(num == 5)
  {
    lcdnumber5(pos);
  }
  if(num == 6)
  {
    lcdnumber6(pos);
  }
  if(num == 7)
  {
    lcdnumber7(pos);
  }
  if(num == 8)
  {
    lcdnumber8(pos);
  }
  if(num == 9)
  {
    lcdnumber9(pos);
  }
}


//Initialize the custom charcters to be loaded into CGRAM
//Note, you can use up to 8 custom characters (C0 - C7)
//If you attempt to use more than 8, the screen will overwrite the oldest character in its CGRAM
void lcdcharinitiate()
{
  //The LCD screen is a 20x4 grid of locations, where each location is made up of an 8 pixel high by 5 pixel wide grid
  //For each byte array, there are 8 Hex values written to the the array which corresponds to the 8 horizantal lines for each location
  //For C0[8], the first 0x1F, writes a HEX value to the first line of that location which in binary is '11111'
  //This tells the LCD to turn on all 5 pixels for that line when that character is called
  //0x00 passes a HEX value that is '00000' in binary, and turns all pixels in that line off
  //0x0E passes a HEX value that is '01110' in binary, and turns on the middle 3 pixels for that line
  //Feel free to experiment with different configurations by changing the values after the 'x' in each HEX value. Use an online HEX to Binary converter
  //to find different values to display. 
  
  //Note, the below values create the stylized characters that make this clock display correctly.
  byte C0[8] = {0x1F,0x1F,0x1F,0x00,0x00,0x00,0x00,0x00};
  byte C1[8] = {0x1F,0x1F,0x1F,0x00,0x00,0x1F,0x1F,0x1F};
  byte C2[8] = {0x00,0x00,0x00,0x00,0x00,0x1F,0x1F,0x1F};
  byte C3[8] = {0x00,0x00,0x0E,0x0A,0x0A,0x0E,0x00,0x00};
  byte C4[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x1F,0x1F};
  byte C5[8] = {0x1F,0x1F,0x1F,0x1F,0x1F,0x00,0x00,0x00};
  byte C6[8] = {0x00,0x00,0x00,0x1F,0x1F,0x1F,0x1F,0x1F};
  byte C7[8] = {0x1F,0x1F,0x00,0x00,0x00,0x00,0x00,0x00};

  //Create the characters and load them into the CGRAM on the LCD
  //We will write (byte(x)) to the LCD where x is 0-7 below, to display our custom characters
  lcd.createChar(0 , C0);
  lcd.createChar(1 , C1);
  lcd.createChar(2 , C2);
  lcd.createChar(3 , C3);
  lcd.createChar(4 , C4);
  lcd.createChar(5 , C5);
  lcd.createChar(6 , C6);
  lcd.createChar(7 , C7);

  //Initialize the LCD itself with the loaded values
  //If your custom characters don't display correctly, reset the Arduino, and it will reload them into the CGRAM of the LCD
  //It may take a couple resets to get the CGRAM loaded correctly
  lcd.begin(20 , 4);

}


//Draw the stylized LCD characters on the LCD
//Below are lcdnumberx(y) functions for x-values '0-9', and where 'y' is the start location of the character that we passed
//to the function when we called it
//See lcdbnumber0 for an example of how these functions work
void lcdnumber0(int startposition)
{   
    //Each character is 3 locations wide on the LCD, so using the startposition, we can write values to each of the 3x4 locations for each stylized character
    //This function creates a stylized '0' that is 3 LCD locations wide, by 4 LCD locations high
    
    //Start by setting the cursor to the top-left corner of the character
    lcd.setCursor(startposition+0,0);
    //Write 255 to the LCD, which corresponds to all pixels for that location being turned on
    lcd.write(byte(255));
    //Move the cursor to the next position on the same line
    lcd.setCursor(startposition+1,0);
    //Write 0 to the LCD, this corresponds to the first value in CGRAM from our custom characters (C0)
    lcd.write(byte(0));
    //Move the cursor to the next position on the same line
    lcd.setCursor(startposition+2,0);
    //Write 255 to the LCD to turn all pixels for that location on
    lcd.write(byte(255));
    //Move the cursor to the next line, and back to the starting X position
    lcd.setCursor(startposition+0,1);
    //Write 255 to the LCD
    lcd.write(byte(255));
    //Move the cursor to the next location
    lcd.setCursor(startposition+1,1);
    //Write 32 to the LCD, see the chart on this products page to see what this value corresponds to
    //Tip: 32 in binary is 00011111, so this is printing a blank space
    lcd.write(byte(32));
    //Move the cursor to the next location
    lcd.setCursor(startposition+2,1);
    //Write 255 to the LCD
    lcd.write(byte(255));
    //Move the cursor to the next line, and reset the X position
    lcd.setCursor(startposition+0,2);
    //Write 255 to the LCD
    lcd.write(byte(255));
    //Move the cursor to the next location
    lcd.setCursor(startposition+1,2);
    //Write 32 to the LCD
    lcd.write(byte(32));
    //Move the cursor to the next location
    lcd.setCursor(startposition+2,2);
    //Write 255 to the LCD
    lcd.write(byte(255));
    //Move the cursor to the next line, and reset the X position
    lcd.setCursor(startposition+0,3);
    //Write 255 to the LCD
    lcd.write(byte(255));
    //Move the cursor to the next location
    lcd.setCursor(startposition+1,3);
    //Write 2 to the LCD, this corresponds to the third value in CGRAM from our custom characters (C2)
    lcd.write(byte(2));
    //Move the cursor to the next location
    lcd.setCursor(startposition+2,3);
    //Write 255 to the LCD
    lcd.write(byte(255));
}
void lcdnumber1(int startposition)
{
    lcd.setCursor(startposition+0,0);
    lcd.write(byte(0));
    lcd.setCursor(startposition+1,0);
    lcd.write(byte(255));
    lcd.setCursor(startposition+2,0);
    lcd.write(byte(32));
    lcd.setCursor(startposition+0,1);
    lcd.write(byte(32));
    lcd.setCursor(startposition+1,1);
    lcd.write(byte(255));
    lcd.setCursor(startposition+2,1);
    lcd.write(byte(32));
    lcd.setCursor(startposition+0,2);
    lcd.write(byte(32));
    lcd.setCursor(startposition+1,2);
    lcd.write(byte(255));
    lcd.setCursor(startposition+2,2);
    lcd.write(byte(32));
    lcd.setCursor(startposition+0,3);
    lcd.write(byte(2));
    lcd.setCursor(startposition+1,3);
    lcd.write(byte(255));
    lcd.setCursor(startposition+2,3);
    lcd.write(byte(2));
}
void lcdnumber2(int startposition)
{
    lcd.setCursor(startposition+0,0);
    lcd.write(byte(5));
    lcd.setCursor(startposition+1,0);
    lcd.write(byte(0));
    lcd.setCursor(startposition+2,0);
    lcd.write(byte(255));
    lcd.setCursor(startposition+0,1);
    lcd.write(byte(2));
    lcd.setCursor(startposition+1,1);
    lcd.write(byte(2));
    lcd.setCursor(startposition+2,1);
    lcd.write(byte(255));
    lcd.setCursor(startposition+0,2);
    lcd.write(byte(255));
    lcd.setCursor(startposition+1,2);
    lcd.write(byte(0));
    lcd.setCursor(startposition+2,2);
    lcd.write(byte(0));
    lcd.setCursor(startposition+0,3);
    lcd.write(byte(255));
    lcd.setCursor(startposition+1,3);
    lcd.write(byte(2));
    lcd.setCursor(startposition+2,3);
    lcd.write(byte(6));
}
void lcdnumber3(int startposition)
{
    lcd.setCursor(startposition+0,0);
    lcd.write(byte(5));
    lcd.setCursor(startposition+1,0);
    lcd.write(byte(0));
    lcd.setCursor(startposition+2,0);
    lcd.write(byte(255));
    lcd.setCursor(startposition+0,1);
    lcd.write(byte(32));
    lcd.setCursor(startposition+1,1);
    lcd.write(byte(2));
    lcd.setCursor(startposition+2,1);
    lcd.write(byte(255));
    lcd.setCursor(startposition+0,2);
    lcd.write(byte(32));
    lcd.setCursor(startposition+1,2);
    lcd.write(byte(0));
    lcd.setCursor(startposition+2,2);
    lcd.write(byte(255));
    lcd.setCursor(startposition+0,3);
    lcd.write(byte(6));
    lcd.setCursor(startposition+1,3);
    lcd.write(byte(2));
    lcd.setCursor(startposition+2,3);
    lcd.write(byte(255));
}
void lcdnumber4(int startposition)
{
    lcd.setCursor(startposition+0,0);
    lcd.write(byte(255));
    lcd.setCursor(startposition+1,0);
    lcd.write(byte(32));
    lcd.setCursor(startposition+2,0);
    lcd.write(byte(255));
    lcd.setCursor(startposition+0,1);
    lcd.write(byte(255));
    lcd.setCursor(startposition+1,1);
    lcd.write(byte(2));
    lcd.setCursor(startposition+2,1);
    lcd.write(byte(255));
    lcd.setCursor(startposition+0,2);
    lcd.write(byte(32));
    lcd.setCursor(startposition+1,2);
    lcd.write(byte(32));
    lcd.setCursor(startposition+2,2);
    lcd.write(byte(255));
    lcd.setCursor(startposition+0,3);
    lcd.write(byte(32));
    lcd.setCursor(startposition+1,3);
    lcd.write(byte(32));
    lcd.setCursor(startposition+2,3);
    lcd.write(byte(255));
}
void lcdnumber5(int startposition)
{
    lcd.setCursor(startposition+0,0);
    lcd.write(byte(255));
    lcd.setCursor(startposition+1,0);
    lcd.write(byte(0));
    lcd.setCursor(startposition+2,0);
    lcd.write(byte(5));
    lcd.setCursor(startposition+0,1);
    lcd.write(byte(255));
    lcd.setCursor(startposition+1,1);
    lcd.write(byte(2));
    lcd.setCursor(startposition+2,1);
    lcd.write(byte(2));
    lcd.setCursor(startposition+0,2);
    lcd.write(byte(32));
    lcd.setCursor(startposition+1,2);
    lcd.write(byte(32));
    lcd.setCursor(startposition+2,2);
    lcd.write(byte(255));
    lcd.setCursor(startposition+0,3);
    lcd.write(byte(6));
    lcd.setCursor(startposition+1,3);
    lcd.write(byte(2));
    lcd.setCursor(startposition+2,3);
    lcd.write(byte(255));
}
void lcdnumber6(int startposition)
{
    lcd.setCursor(startposition+0,0);
    lcd.write(byte(255));
    lcd.setCursor(startposition+1,0);
    lcd.write(byte(0));
    lcd.setCursor(startposition+2,0);
    lcd.write(byte(5));
    lcd.setCursor(startposition+0,1);
    lcd.write(byte(255));
    lcd.setCursor(startposition+1,1);
    lcd.write(byte(2));
    lcd.setCursor(startposition+2,1);
    lcd.write(byte(2));
    lcd.setCursor(startposition+0,2);
    lcd.write(byte(255));
    lcd.setCursor(startposition+1,2);
    lcd.write(byte(32));
    lcd.setCursor(startposition+2,2);
    lcd.write(byte(255));
    lcd.setCursor(startposition+0,3);
    lcd.write(byte(255));
    lcd.setCursor(startposition+1,3);
    lcd.write(byte(2));
    lcd.setCursor(startposition+2,3);
    lcd.write(byte(255));
}
void lcdnumber7(int startposition)
{
    lcd.setCursor(startposition+0,0);
    lcd.write(byte(5));
    lcd.setCursor(startposition+1,0);
    lcd.write(byte(0));
    lcd.setCursor(startposition+2,0);
    lcd.write(byte(255));
    lcd.setCursor(startposition+0,1);
    lcd.write(byte(32));
    lcd.setCursor(startposition+1,1);
    lcd.write(byte(4));
    lcd.setCursor(startposition+2,1);
    lcd.write(byte(255));
    lcd.setCursor(startposition+0,2);
    lcd.write(byte(32));
    lcd.setCursor(startposition+1,2);
    lcd.write(byte(7));
    lcd.setCursor(startposition+2,2);
    lcd.write(byte(255));
    lcd.setCursor(startposition+0,3);
    lcd.write(byte(32));
    lcd.setCursor(startposition+1,3);
    lcd.write(byte(32));
    lcd.setCursor(startposition+2,3);
    lcd.write(byte(255));
}
void lcdnumber8(int startposition)
{
    lcd.setCursor(startposition+0,0);
    lcd.write(byte(255));
    lcd.setCursor(startposition+1,0);
    lcd.write(byte(0));
    lcd.setCursor(startposition+2,0);
    lcd.write(byte(255));
    lcd.setCursor(startposition+0,1);
    lcd.write(byte(255));
    lcd.setCursor(startposition+1,1);
    lcd.write(byte(4));
    lcd.setCursor(startposition+2,1);
    lcd.write(byte(255));
    lcd.setCursor(startposition+0,2);
    lcd.write(byte(255));
    lcd.setCursor(startposition+1,2);
    lcd.write(byte(7));
    lcd.setCursor(startposition+2,2);
    lcd.write(byte(255));
    lcd.setCursor(startposition+0,3);
    lcd.write(byte(255));
    lcd.setCursor(startposition+1,3);
    lcd.write(byte(2));
    lcd.setCursor(startposition+2,3);
    lcd.write(byte(255));
}
void lcdnumber9(int startposition)
{
    lcd.setCursor(startposition+0,0);
    lcd.write(byte(255));
    lcd.setCursor(startposition+1,0);
    lcd.write(byte(0));
    lcd.setCursor(startposition+2,0);
    lcd.write(byte(255));
    lcd.setCursor(startposition+0,1);
    lcd.write(byte(255));
    lcd.setCursor(startposition+1,1);
    lcd.write(byte(2));
    lcd.setCursor(startposition+2,1);
    lcd.write(byte(255));
    lcd.setCursor(startposition+0,2);
    lcd.write(byte(32));
    lcd.setCursor(startposition+1,2);
    lcd.write(byte(32));
    lcd.setCursor(startposition+2,2);
    lcd.write(byte(255));
    lcd.setCursor(startposition+0,3);
    lcd.write(byte(6));
    lcd.setCursor(startposition+1,3);
    lcd.write(byte(2));
    lcd.setCursor(startposition+2,3);
    lcd.write(byte(255));
}


//This is a modified function like the lcdnumberx() functions above
//lcdempty() writes a blank value to each of the locations within a number
void lcdempty(int startposition)
{
    lcd.setCursor(startposition+0,0);
    lcd.write(byte(32));
    lcd.setCursor(startposition+1,0);
    lcd.write(byte(32));
    lcd.setCursor(startposition+2,0);
    lcd.write(byte(32));
    lcd.setCursor(startposition+0,1);
    lcd.write(byte(32));
    lcd.setCursor(startposition+1,1);
    lcd.write(byte(32));
    lcd.setCursor(startposition+2,1);
    lcd.write(byte(32));
    lcd.setCursor(startposition+0,2);
    lcd.write(byte(32));
    lcd.setCursor(startposition+1,2);
    lcd.write(byte(32));
    lcd.setCursor(startposition+2,2);
    lcd.write(byte(32));
    lcd.setCursor(startposition+0,3);
    lcd.write(byte(32));
    lcd.setCursor(startposition+1,3);
    lcd.write(byte(32));
    lcd.setCursor(startposition+2,3);
    lcd.write(byte(32));
}


//Write the custom character C3 to the locations for the Semicolons to appear
void lcdnumbersemicolon(int startposition)
{
  lcd.setCursor(startposition+0,1);
  lcd.write(byte(3));
  lcd.setCursor(startposition+0,2);
  lcd.write(byte(3));
}


//Write blank spaces to the location that the Semicolons were previously
void lcdnumbersemicolonoff(int startposition)
{
  lcd.setCursor(startposition+0,1);
  lcd.write(byte(32));
  lcd.setCursor(startposition+0,2);
  lcd.write(byte(32));
}


//Write an uppercase 'AM' to the location passed to the function
void lcdam(int startposition)
{
  lcd.setCursor(startposition+0,2);
  lcd.write(byte(65));
  lcd.setCursor(startposition+1,2);
  lcd.write(byte(77));
}


//Write an uppercase 'PM' to the location passed to the function
void lcdpm(int startposition)
{
  lcd.setCursor(startposition+0,2);
  lcd.write(byte(80));
  lcd.setCursor(startposition+1,2);
  lcd.write(byte(77));
}
