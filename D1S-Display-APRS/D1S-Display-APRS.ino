// D1S-Display-APRS.ino

// 04/17/2020 - update with new APRS scaling
// Vcell = 0.0025 * Byte + 2.5
// dBm = - Byte
// lux = 0.1218 * Byte^2 
// awake = 0.025 * Byte

// 06/02/18 Release
/*_____________________________________________________________________________
   Copyright(c) 2018 - 2020 Karl Berger dba IoT Kits https://w4krl.com
   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:
   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
   _____________________________________________________________________________
*/

// *******************************************************
// ******************* INCLUDES **************************
// *******************************************************
// For general sketch
#include <ESP8266WiFi.h>            // [builtin]

// for Wemos TFT 1.4 display shield
#include <Adafruit_GFX.h>           // [manager] Core graphics library
#include <Fonts/FreeSerif9pt7b.h>   //           part of GFX for APRS message text
#include <Adafruit_ST7735.h>        // [manager] Hardware-specific library
#include <SPI.h>                    // [builtin]

// Time functions by Rop Gonggrijp
// See https://github.com/ropg/ezTime
#include <ezTime.h>                       // [manager] v0.7.10 NTP & timezone 

// Your configuration file must reside in the same
// directory as this sketch
#include "APRS_config.h"

// *******************************************************
// ******************* DEFAULTS **************************
// *******************************************************
//            DO NOT CHANGE THESE DEFAULTS
const char APRS_DEVICE_NAME[] = "http://w4krl.com/iot-kits/";
const char APRS_SOFTWARE_NAME[] = "D1S-RCVR";
const char APRS_SOFTWARE_VERS[] = "1.00";
const long FRAME_INTERVAL = 5;                  // seconds to display frame

// Select an APRS-IS Tier 2 server with filter capability
// for list of servers: http://www.aprs-is.net/aprsservers.aspx
// APRS-IS.net recommendation in June 2018 is rotate.aprs2.net on port 14580
const char APRS_SERVER[] = "rotate.aprs2.net";  // world-wide pool of APRS-IS servers
const int APRS_PORT = 14580;                    // for user-defined filter feed - do not change

// *******************************************************
// ********* INSTANTIATE DEVICES *************************
// *******************************************************
// Wemos TFT 1.4 shield connections
// TFT_CS     D4  GPIO 2
// TFT_RST    -1  use -1 according to Wemos
// TFT_DC     D3  GPIO 0
// MOSI       D7
// SCK        D5
// TFT_LED    NC
// Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);
// Adafruit_ST7735 tft = Adafruit_ST7735(2,  0, -1);
// Adafruit_ST7735 tft = Adafruit_ST7735(D4, D3, 0);
#define TFT_CS     D4
#define TFT_RST    0  // you can also connect this to the Arduino reset
                      // in which case, set this #define pin to 0!
#define TFT_DC     D3

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);
WiFiClient client;    // Wi-Fi connection
Timezone myTZ;

// *******************************************************
// ******************* GLOBALS ***************************
// *******************************************************
// APRS Data Type Identifiers
// page 17 http://www.aprs.org/doc/APRS101.PDF
const char APRS_ID_POSITION_NO_TIMESTAMP = '!';
const char APRS_ID_TELEMETRY = 'T';
const char APRS_ID_WEATHER = '_';
const char APRS_ID_MESSAGE = ':';
const char APRS_ID_QUERY = '?';
const char APRS_ID_STATUS = '>';
const char APRS_ID_USER_DEF = '{';
const char APRS_ID_COMMENT = '#';
String APRSdataMessage = "";             // message text
String APRSdataWeather = "";             // weather data
String APRSdataTelemetry = "";           // telemetry data
String APRSserver = "";                  // APRS-IS server
char APRSage[9] = "";                    // time stamp for received data

// *******************************************************
// *********** WEMOS TFT 1.4 SETTINGS ********************
// *******************************************************
// display parameters for Wemos TFT 1.4 shield
const int screenW = 128;
const int screenH = 128;
const int screenW2 = screenW / 2;    // half width
const int screenH2 = screenH / 2;    // half height

// analog clock dimensions
const int clockCenterX = screenW2;
// title at top of screen is 16 pixels high so screen available
// is screenH - 16. Must add 16 back to shift center to middle
// of screen available
const int clockCenterY = (( screenH - 16 ) / 2 ) + 16;
const int clockDialRadius = 50;  // outer clock dial

// Color definitions similar to resistor color code
// RGB565 format
// http://w8bh.net/pi/rgb565.py
const int BLACK     =  0x0000;
const int BROWN     =  0xA145;
const int RED       =  0xF800;     // official RED
const int ORANGERED =  0xFA20;    // my choice for RED
const int ORANGE    =  0xFD20;
const int YELLOW    =  0xFFE0;
const int GREEN     =  0x9772;    // given in table as LIGHTGREEN
const int LIME      =  0x07E0;    // adafruit choice for GREEN
const int BLUE      =  0x001F;
const int VIOLET    =  0xEC1D;
const int GRAY      =  0x8410;
const int WHITE     =  0xFFFF;
const int GOLD      =  0xFEA0;
const int SILVER    =  0xC618;
const int CYAN      =  0x07FF;
const int MAGENTA   =  0xF81F;

// *******************************************************
// ******************** SETUP ****************************
// *******************************************************
void setup() {
  Serial.begin( 115200);
  pinMode(D1, INPUT_PULLUP);   // use internal pullup resistor

  // Initializer for 1.4-in. TFT ST7735S chip, green tab
  tft.initR(INITR_144GREENTAB);

  Serial.println("IoT Kits splash");
  splashScreen();   // stays on until logon is complete

  logonToRouter();

  tft.fillScreen(BLACK);                 // clear screen
  tft.setCursor(0, 0);
  tft.setTextSize(1);
  tft.setTextColor(YELLOW);
  Serial.print("Local IP: ");            // show Wi-Fi IP
  Serial.println(WiFi.localIP());
  tft.println("Connected to:");
  tft.println( WiFi.localIP() );

  waitForSync();                          // ezTime lib
  // if timezone has not been cached in EEPROM
  // or user is asking for a new timezone
  // set the timezone
  if ( !myTZ.setCache( 0 ) || myTZ.getOlson() != tzLocation ) {
    myTZ.setLocation( tzLocation );
  }

  Serial.print("APRS logon");
  logonToAPRS();
  delay ( 5000 );                        // delay to show connection info

  tft.fillScreen(BLACK);                 // clear screen
} //setup()

// *******************************************************
// ******************** LOOP *****************************
// *******************************************************
void loop() {
  static bool blockPulse = false;
  // update frames when the second has changed
  // this is an ezTime library function
  if ( secondChanged() && !blockPulse ) {
    frameUpdate();
  }

  // continually check APRS server for incoming data
  String APRSrcvd = APRSreceiveData();

  // ignore comments and short strings (10 is arbitrary)
  if ( APRSrcvd[0] != APRS_ID_COMMENT && APRSrcvd.length() > 10 ) {
    // does stream contain weather data?
    if ( APRSrcvd.indexOf(APRS_ID_WEATHER) > 0 ) {
      if ( APRSdataWeather != APRSrcvd ) { // it has changed so update it
        APRSdataWeather = APRSrcvd;
        // record time data is received
        sprintf(APRSage, "%02d:%02d:%02d", myTZ.hour(), minute(), second());
      }
    }
    // does stream contain Telemetry?
    if ( APRSrcvd.indexOf("T#") > 0 ) {  // better than looking for 'T'
      if ( APRSdataTelemetry != APRSrcvd ) {
        APRSdataTelemetry = APRSrcvd;
      }
    }
    // does stream contain a message?
    if ( APRSrcvd.indexOf("::") > 0 ) {      // special case
      int buttonState = digitalRead(D1);
      APRSdataMessage = APRSrcvd;
      if (aprsMessageFrame() == true) {
        blockPulse = true;
        while (buttonState == HIGH) {
          buttonState = digitalRead(D1);
          delay(200);
        }
        blockPulse = false;
      }
    }
  }
} // loop()

// *******************************************************
// ************** Logon to your Wi-Fi router *************
// *******************************************************
void logonToRouter() {
  int count = 0;
  WiFi.begin( WIFI_SSID, WIFI_PASSWORD );
  while ( WiFi.status() != WL_CONNECTED ) {
    count++;
    if ( count > 100 ) {
      Serial.println("WiFi connection timeout. Rebooting.");
      ESP.reset();
    }
    delay( 100 );                     // ms delay between reports
    Serial.print(".");
  } // loop while not connected
  // WiFi is sucesfully connected
  Serial.print("\nWiFi connected to IP: ");
  Serial.println(WiFi.localIP().toString());
} // logonToRouter()

// *******************************************************
// **************** FRAME UPDATE *************************
// *******************************************************
void frameUpdate() {
  const int maxFrames = 4;              // number of display frames
  static boolean firstTime = true;      // update frame background first time only
  static int frame = second() % FRAME_INTERVAL;

  if ( second() % FRAME_INTERVAL == 0 ) {
    // erase screen each time a new frame is displayed
    tft.fillScreen(BLACK);
    firstTime = true;
    frame++;
    if ( frame > maxFrames - 1 ) {
      frame = 0;
    }
  }

  // choose frame to display
  switch ( frame ) {
    case 0:
      analogClockFrame( firstTime );
      firstTime = false;
      break;
    case 1:
      digitalClockFrame( firstTime );
      firstTime = false;
      break;
    case 2:
      aprsWXFrame( firstTime );
      firstTime = false;
      break;
    case 3:
      aprsTelemetryFrame( firstTime );
      firstTime = false;
      break;
  }
} // frameUpdate()

// *******************************************************
// **************** SPLASH SCREEN ************************
// *******************************************************
void splashScreen() {
  tft.fillScreen(BLUE);
  tft.setTextSize(2);
  tft.setTextColor(YELLOW);
  int topLine = 19;
  displayCenter( "D1S-APRS", screenW2, topLine,      2 );
  displayCenter( "Remote",  screenW2, topLine + 20, 2 );
  displayCenter( "Display", screenW2, topLine + 40, 2 );
  displayCenter( "IoT Kits", screenW2, topLine + 60, 2 );
  displayCenter( "by W4KRL", screenW2, topLine + 80, 2 );
  for (int i = 0; i < 4; i++) {
    tft.drawRoundRect(12 - 3 * i, 12 - 3 * i, screenW - 12, screenH - 12, 8, YELLOW);
  }
} // splashScreen()

// *******************************************************
// ************* logon to APRS server ********************
// *******************************************************
void logonToAPRS() {
  // See http://www.aprs-is.net/Connecting.aspx
  String dataString = "";
  if ( client.connect( APRS_SERVER, APRS_PORT ) == true ) {
    Serial.println( "APRS connected." );
    dataString += "user ";
    dataString += APRS_MY_CALL;
    dataString += " pass ";
    dataString += APRS_PASSCODE;
    dataString += " vers ";
    dataString += APRS_SOFTWARE_NAME;
    dataString += " ";
    dataString += APRS_SOFTWARE_VERS;
    dataString += " filter ";
    dataString += APRS_FILTER;
    client.println( dataString );     // send to APRS-IS
    Serial.println( dataString );     // print to serial monitor

    if ( APRSverified() == true ) {
      Serial.println( "APRS Login ok." );
      tft.setTextSize(2);
      tft.setTextColor( GREEN );
      displayCenter( "APRS-IS",  screenW2, 30, 2 );
      displayCenter( APRS_MY_CALL, screenW2, 50, 2 );
      displayCenter( APRSserver, screenW2, 90, 2 );
    } else {
      Serial.println( "APRS Login failed." );
    } // if APRSverified
  } // if connect
} // logonToAPRS()

// *******************************************************
// ********** Verify logon to APRS-IS server *************
// *******************************************************
// returns server ID in global String APRSserver
boolean APRSverified() {
  boolean flag = false;
  unsigned long timeBegin = millis();
  while ( true ) {
    String srvrResponse = APRSreceiveData();
    if ( srvrResponse.indexOf("verified") > 0 ) {
      if ( srvrResponse.indexOf("unverified") == -1 ) {
        // "unverfied" not found, read APRS_IS server name
        flag = true;
        APRSserver = srvrResponse.substring(srvrResponse.indexOf("server") + 7);
        APRSserver.trim();  // remove trailing spaces
        break;
      }
    }
  }
  return flag;
} // APRSverified()

// *******************************************************
// ************ RECEIVE APRS-IS DATA *********************
// *******************************************************
String APRSreceiveData() {
  // add a timeout function
  const  unsigned int maxSize = 500;  // what is largest packet???
  char rcvBuffer[maxSize] = "";
  if ( client.available() > 0 ) {
    int i = 0;
    while ( i < maxSize ) {
      char charRcvd = client.read();
      rcvBuffer[i] = charRcvd;
      i++; // increment index
      if ( charRcvd == '\n' ) {
        // entire line received
        break;
      }
    }
    rcvBuffer[i] = '\0'; // add null marker at end to finish string
    Serial.println(rcvBuffer);
  }
  return rcvBuffer;
} //APRSreceiveData()

// *******************************************************
// **************** SEND APRS ACK ************************
// *******************************************************
void APRSsendACK( String recipient, String msgID ) {
  String dataString = APRS_MY_CALL;
  dataString += ">APRS,TCPIP*:";
  dataString += APRS_ID_MESSAGE;
  dataString += APRSpadCall(recipient); // pad to 9 characters
  dataString += APRS_ID_MESSAGE;
  dataString += "ack";
  dataString += msgID;
  client.println( dataString );     // send to APRS-IS
  Serial.println( dataString );     // print to serial port
} // APRsendACK()

// *******************************************************
// ******** Format callsign for APRS responses ***********
// *******************************************************
// max length with SSID is 9, for ex.: WA3YST-13
// this pads a short call with spaces to the right
// Example:  "W0V-1^^^^" Note: ^ indicates space
String APRSpadCall( String callSign ) {
  int len = callSign.length();
  String dataString = callSign; // initialize dataString with callsign
  for ( int i = len; i < 9; i++ ) {
    dataString += " ";
  }
  return dataString;
}  // APRSpadCall()

// *******************************************************
// *************** DISPLAY FRAMES ************************
// *******************************************************

// *******************************************************
// ************* ANALOG CLOCK FRAME **********************
// *******************************************************
void analogClockFrame( boolean firstRender ) {
  // scale dimensions from dial radius
  int numeralR = clockDialRadius - 6 ;
  int outerTickR = numeralR - 6;
  int innerTickR = outerTickR - 6;
  int minHand = innerTickR;       // longest hand
  int secHand = innerTickR;       // use different color
  int hourHand = 2 * minHand / 3; // hour hand is 2/3 minute hand length
  int x2, x3, x4, y2, y3, y4;     // various coordinates

  // draw clock face first time only to speed up graphics
  if ( firstRender == true ) {
    // display time zone and static dial elements
    tft.drawRoundRect( 0, 0, screenW, screenH, 8, ORANGERED );
    tft.setTextSize( 2 );
    tft.setCursor( 4, 4 );
    tft.setTextColor( WHITE, BLACK );
    tft.print( myTZ.getTimezoneName() );  // local time zone abbreviation
    tft.drawCircle( clockCenterX, clockCenterY, 2, YELLOW );  // hub
    tft.drawCircle( clockCenterX, clockCenterY, clockDialRadius, WHITE );  // dial edge
    tft.setTextSize( 1 );  // for dial numerals
    tft.setTextColor( GREEN, BLACK );
    // add hour ticks & numerals
    for ( int numeral = 1; numeral < 13; numeral++ ) {
      // Begin at 30Â° and stop at 360Â° (noon)
      float angle = 30 * numeral;          // convert hour to angle in degrees
      angle = angle / 57.29577951; // Convert degrees to radians
      x2 = ( clockCenterX + ( sin( angle ) * outerTickR ));
      y2 = ( clockCenterY - ( cos( angle ) * outerTickR ));
      x3 = ( clockCenterX + ( sin( angle ) * innerTickR ));
      y3 = ( clockCenterY - ( cos( angle ) * innerTickR ));
      tft.drawLine( x2, y2, x3, y3, YELLOW );      // tick line
      // place numeral a little beyond tick line
      x4 = ( clockCenterX + ( sin(angle) * numeralR ));
      y4 = ( clockCenterY - ( cos(angle) * numeralR ));
      tft.setCursor( x4 - 2, y4 - 4 );  // minus third character width, half height
      if ( numeral == 12 ) tft.setCursor( x4 - 5, y4 - 4 );
      tft.print( numeral );
    }
  } // if firstRender

  // display second hand
  float angle = second() * 6;         // each second advances 6 degrees
  angle = angle  / 57.29577951;       // Convert degrees to radians
  static float oldSecond = angle;
  // erase previous second hand
  x3 = ( clockCenterX + ( sin( oldSecond ) * secHand ));
  y3 = ( clockCenterY - ( cos( oldSecond ) * secHand ));
  tft.drawLine( clockCenterX, clockCenterY, x3, y3, BLACK );
  oldSecond = angle;  // save current angle for erase next time
  // draw new second hand
  x3 = ( clockCenterX + ( sin( angle ) * secHand ));
  y3 = ( clockCenterY - ( cos( angle ) * secHand ));
  tft.drawLine( clockCenterX, clockCenterY, x3, y3, ORANGERED );

  // display minute hand
  angle = minute() * 6;        // each minute advances 6 degrees
  angle = angle / 57.29577951; // Convert degrees to radians
  static float oldMinute = angle;
  // erase old minute hand
  if ( angle != oldMinute || firstRender ) {
    x3 = ( clockCenterX + ( sin( oldMinute ) * minHand ));
    y3 = ( clockCenterY - ( cos( oldMinute ) * minHand ));
    tft.drawLine( clockCenterX, clockCenterY, x3, y3, BLACK );
    oldMinute = angle;
  }
  // draw new minute hand
  x3 = ( clockCenterX + ( sin(angle ) * minHand ));
  y3 = ( clockCenterY - ( cos(angle ) * minHand ));
  tft.drawLine( clockCenterX, clockCenterY, x3, y3, GREEN );

  // display hour hand
  int dialHour = myTZ.hour();
  if ( dialHour > 13 ) dialHour = dialHour - 12;
  // 30 degree increments + adjust for minutes
  angle = myTZ.hour() * 30 + int(( minute() / 12 ) * 6 );
  angle = ( angle / 57.29577951 ); //Convert degrees to radians
  static float oldHour = angle;
  if ( angle != oldHour || firstRender ) {
    x3 = ( clockCenterX + ( sin( oldHour ) * hourHand ));
    y3 = ( clockCenterY - ( cos( oldHour ) * hourHand ));
    tft.drawLine( clockCenterX, clockCenterY, x3, y3, BLACK );
    oldHour = angle;
  }
  // draw new hour hand
  x3 = ( clockCenterX + ( sin(angle ) * hourHand ));
  y3 = ( clockCenterY - ( cos(angle ) * hourHand ));
  tft.drawLine( clockCenterX, clockCenterY, x3, y3, YELLOW );
} // analogClockFrame()

// *******************************************************
// ************* DIGITAL CLOCK FRAME *********************
// *******************************************************
void digitalClockFrame( boolean firstRender ) {
  int topLine = 15;                // sets location of top line

  tft.setTextSize(2);
  if ( firstRender ) {
    // print labels
    tft.drawRoundRect(0, 0, screenW, screenH, 8, WHITE);
    tft.setTextColor(ORANGERED, BLACK);
    displayCenter( "UTC", screenW2, topLine, 2 );
    tft.setTextColor( GREEN, BLACK );
    // local time zone
    displayCenter( myTZ.getTimezoneName(), screenW2, topLine + 60, 2 );
  }
  // display times
  tft.setTextColor( ORANGERED, BLACK );
  tft.setCursor(17, topLine + 20);
  tft.print( UTC.dateTime( "H~:i~:s"));

  tft.setTextColor( GREEN, BLACK );
  tft.setCursor( 17, topLine + 80 );
  tft.print( myTZ.dateTime( "H~:i~:s" ));
} // digitalClockFrame()

// *******************************************************
// ************* APRS WEATHER FRAME **********************
// *******************************************************
void aprsWXFrame( boolean firstRender ) {
  // see APRS Protocol pg 65
  static boolean beenDisplayed = false;
  int index = 0;
  char tempF[4] = "";
  char humid[3] = "";
  char barom[6] = "";
  // print static labels the first time
  if ( firstRender ) {
    int headerY = 40;
    int radius = 8;
    // top panel
    tft.fillRoundRect( 0, 0, screenW, 2 * radius, radius, YELLOW );
    tft.fillRect( 0, radius, screenW, headerY - radius, YELLOW );
    tft.fillRect( 0, headerY, screenW, screenH - headerY - radius, BLUE );
    tft.fillRoundRect( 0, screenH - 2 * radius, screenW, 2 * radius, radius, BLUE );

    tft.setTextSize(2);
    tft.setTextColor( BLUE, YELLOW );
    displayCenter( "Weather", screenW2, 3, 2 );
    displayCenter( APRS_THEIR_CALL, screenW2, 23, 2 );
  }

  if ( APRSdataWeather.indexOf( APRS_ID_WEATHER ) > 0 ) {
    // remove "waiting" message when weather data is found
    if ( !beenDisplayed ) {
      tft.setTextColor( BLUE, BLUE );
      displayCenter( "Waiting on", screenW2, 50, 2 );
      displayCenter( "Data", screenW2, 70, 2 );
    }
    // temperature
    index = APRSdataWeather.indexOf( 't' );
    for ( int i = 0; i < 3; i++ ) {
      tempF[i] = APRSdataWeather[index + 1 + i];
    }
    int itempF = atoi( tempF );
    int itempC = ( itempF - 32 ) * 5 / 9;

    // humidity
    index = APRSdataWeather.indexOf( 'h', index + 3 );
    for (int i = 0; i < 2; i++) {
      humid[i] = APRSdataWeather[index + 1 + i];
    }
    int iHumid = atoi( humid );

    // barometric pressure
    index = APRSdataWeather.indexOf( 'b', index + 2 );
    for ( int i = 0; i < 5; i++ ) {
      barom[i] = APRSdataWeather[index + 1 + i];
    }
    float Press = atof( barom ) / 10;

    String dispTempF = String( itempF ) + " F ";
    String dispTempC = String( itempC ) + " C ";
    String dispHumid = String( iHumid ) + " % ";
    String dispPress = String( Press, 1) + " mb";

    tft.setTextColor(YELLOW, BLUE);
    // left edge must clear border screenW - 3
    displayFlushRight( dispTempF, 125,  45, 2 );
    displayFlushRight( dispTempC, 125,  65, 2 );
    displayFlushRight( dispHumid, 125,  85, 2 );
    displayFlushRight( dispPress, 125, 105, 2 );

    beenDisplayed = true;
  } else {
    tft.setTextColor( YELLOW, BLUE );
    displayCenter( "Waiting on", screenW2, 50, 2 );
    displayCenter( "Data", screenW2, 70, 2 );
  }
} // aprsWXFrame()

// *******************************************************
// ************ APRS TELEMETRY FRAME *********************
// *******************************************************
// hard coded to match telemetry used in D1M-WX1 Solar Weather station
void aprsTelemetryFrame( boolean firstRender ) {
  // see APRS Protocol pg 68
  static boolean beenDisplayed = false;
  int index = 0;
  char vCell[4] = "";
  char lux[4] = "";
  char rssi[4] = "";
  tft.setTextSize(2);
  if ( firstRender ) {
    int headerY = 40;
    int radius = 8;
    tft.fillRoundRect( 0, 0, screenW, 2 * radius, radius, BLUE );
    tft.fillRect( 0, radius, screenW, headerY - radius, BLUE );
    tft.fillRect( 0, headerY, screenW, screenH - headerY - radius, YELLOW );
    tft.fillRoundRect( 0, screenH - 2 * radius, screenW, 2 * radius, radius, YELLOW );

    tft.setTextColor( YELLOW, BLUE );
    displayCenter( "Telemetry", screenW2, 3, 2 );
    displayCenter( APRS_THEIR_CALL, screenW2, 23, 2 );
  }

  index = APRSdataTelemetry.indexOf( "T#" );
  if ( index > 0 ) {
    // telemetry data is found
    if ( !beenDisplayed ) {
      tft.setTextColor( YELLOW, YELLOW );  // erase waiting text
      displayCenter( "Waiting on", screenW2, 50, 2 );
      displayCenter( "Data", screenW2, 70, 2 );
    }
    index = APRSdataTelemetry.indexOf(',', index + 3);  // comma at end of sequence number
    for (int i = 0; i < 3; i++) {
      vCell[i] = APRSdataTelemetry[index + 1 + i];
    }
    float ivCell = atof( vCell ) * 0.0025 + 2.5;  // known scaling factor

    index = APRSdataTelemetry.indexOf(',', index + 2);
    for ( int i = 0; i < 3; i++ ) {
      rssi[i] = APRSdataTelemetry[index + 1 + i];
    }
    int irssi = - atoi( rssi );  // known scaling factor (-1)

    index = APRSdataTelemetry.indexOf(',', index + 2);
    for ( int i = 0; i < 3; i++ ) {
      lux[i] = APRSdataTelemetry[index + 1 + i];
    }
    long ilux = atoi(lux);
    ilux = 0.1281 * ilux * ilux;  // known scaling factor = square of byte rcvd
    tft.setTextColor( BLUE, YELLOW );
    // align right side of values and left side of units
    String dispVdc = String( ivCell ) + " Vdc";
    String dispdBm = String( irssi ) + " dBm";
    String dispLux = String( ilux ) + " lux";

    displayFlushRight( dispVdc, 125, 45, 2 );
    displayFlushRight( dispdBm, 125, 65, 2 );
    displayFlushRight( dispLux, 125, 85, 2 );

    // show time data was received
    tft.setTextSize( 1 );
    tft.setTextColor( ORANGERED );
    char str[16];
    strcpy(str, "Rcvd @ ");
    strcat(str, APRSage);
    displayCenter( str, screenW2, 108, 1 );

    beenDisplayed = true;
  } else {
    tft.setTextColor( BLUE, YELLOW );
    displayCenter( "Waiting on", screenW2, 50, 2 );
    displayCenter( "Data", screenW2, 70, 2 );
  }
} // aprsTelemetryFrame()

// *******************************************************
// ************ APRS MESSAGE FRAME ***********************
// *******************************************************
boolean aprsMessageFrame() {
  // see APRS Protocol pg 71
  // a message can be from 0 to 67 characters
  boolean messageDisplayed = false;
  int index = APRSdataMessage.indexOf(APRS_ID_STATUS);
  String APRSsender = APRSdataMessage.substring(0, index);
  index = APRSdataMessage.indexOf("::");  // special case
  String APRSrcvr = APRSdataMessage.substring(index + 2, index + 11);
  String APRSmessage = APRSdataMessage.substring(index + 12);

  boolean telemFlag = false;
  if (APRSmessage.indexOf("PARM") != -1) telemFlag = true;
  if (APRSmessage.indexOf("UNIT") != -1) telemFlag = true;
  if (APRSmessage.indexOf("EQNS") != -1) telemFlag = true;
  if (APRSmessage.indexOf("BITS") != -1) telemFlag = true;

  if ( telemFlag == false) {
    tft.fillRect( 0, 0, screenW, 27, ORANGERED );
    tft.fillRect( 0, 28, screenW, screenH - 28, BLACK );
    tft.setTextColor( YELLOW, ORANGERED );
    tft.setTextSize( 1 );
    tft.setCursor( 0, 0 );
    tft.print( "Msg from:" );
    tft.setTextSize( 2 );
    tft.setCursor( 0, 10 );
    tft.print( APRSsender );
    tft.setTextSize( 1 );
    tft.setCursor( 0, 35);
    tft.setTextColor( YELLOW, BLACK );
    tft.setFont( &FreeSerif9pt7b );  // looks nice and permits longer text
    tft.print( APRSmessage );
    messageDisplayed = true;
    index = APRSdataMessage.indexOf(APRS_ID_USER_DEF);  // device should acknowledge message
    if ( index > 0 ) {
      String APRSmsgID = APRSdataMessage.substring(index + 1);
      APRSsendACK( APRSsender, APRSmsgID );
    }
  }
  tft.setFont();                 // return to standard font
  return messageDisplayed;
} // aprsMessageFrame()

// *******************************************************
// *********** DISPLAY TEXT FORMATTING *******************
// *******************************************************
void displayFlushRight( String text, int column, int line, int size ) {
  int numChars = text.length();
  int widthText = size * ( 6 * numChars - 1 );
  tft.setCursor( column - widthText, line );
  tft.print( text );
} // displayFlushRight()

void displayCenter( String text, int column, int line, int size ) {
  int numChars = text.length();
  int widthText = size * ( 6 * numChars - 1 );
  tft.setCursor( column - widthText / 2, line );
  tft.print( text );
} // displayCenter()
