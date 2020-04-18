// *******************************************************
// ****************** APRS_config.h **********************
// *******************************************************

// This configuration file must reside in the same Arduino
// directory as the remote display sketch

// *******************************************************
// ********************* WIFI LOGON **********************
// *******************************************************

// ENTER YOUR WI-FI SSID
// !!!!!  YOU MUST USE 2.4 GHz WiFi, NOT 5 GHz  !!!!!
const char WIFI_SSID[] = "";     // "your_wifi_ssid";

// ENTER YOUR WI-FI PASSWORD
const char WIFI_PASSWORD[] = ""; // "your_wifi_password";

// *******************************************************
// ***************** APRS CONFIG PARAMS ******************
// *******************************************************
const String APRS_MY_CALL = "";                 // Your call and ssid for display. Suggest -4
const String APRS_THEIR_CALL = "W4KRL-15";      // default - Your call and ssid of your weather station
const char APRS_PASSCODE[6] = "";               // https://apps.magicbug.co.uk/passcode/
const char APRS_FILTER[] = "b/W4KRL-*";         // default value - Change to "b-your call-*"

// *******************************************************
// ********************* TIME ZONE ***********************
// *******************************************************
// Uncomment only your location
// or add your location using time zone definitions from:
// https://en.wikipedia.org/wiki/List_of_tz_database_time_zones

// United States
String tzLocation = "America/New_York";            // Eastern
//String tzLocation = "America/Chicago";             // Central
//String tzLocation = "America/Denver";              // Mountain
//String tzLocation = "America/Los_Angeles";         // Pacific
//String tzLocation = "America/Anchorage";           // Alaska
//String tzLocation = "Pacific/Honolulu";            // Hawaii

// Canada
//String tzLocation = "America/St_Johns"             // Newfoundland
//String tzLocation = "America/Halifax";             // Atlantic
//String tzLocation = "America/Toronto";             // Eastern
//String tzLocation = "America/Winnipeg";            // Central
//String tzLocation = "America/Edmonton";            // Mountain
//String tzLocation = "America/Vancouver";           // Pacific

// Europe
//String tzLocation = "Europe/Zurich";               // Central European
//String tzLocation = "Europe/London"                // British
//String tzLocation = "Europe/Athens"                // Eastern European
//String tzLocation = "Europe/Lisbon"                // Weatern European

// Asia
//String tzLocation = "Asia/Tokyo";                  // Japan
//String tzLocation = "Asia/Shanghai"                // China

// Others
//String tzLocation = "America/PE"                   // Peru
