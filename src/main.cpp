#include <Arduino.h>
#include <WiFi.h>

// Mode configuration - uncomment to run in bare bone mode
// will advance the tile every 2.5 sec.
//#define BARE_BONE_MODE

// Log configuration - comment out to get rid of any serial out
#define DEBUG_ENABLED
#ifdef DEBUG_ENABLED
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINT(x) Serial.print(x)
#else
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINT(x)
#endif

// Board parameters
static const int enable_pin = 21;
static const int input1_pin = 23;
static const int input2_pin = 22;

// WiFi parameters
static const char *ssid = "WLAN SSID";
static const char *password = "secret";

// NTP parameters
static const char *ntp_pool = "de.pool.ntp.org";
static const char *cet_tz = "CET-1CEST,M3.5.0/02,M10.5.0/03";

/** Global varibales */
bool setup_completed = false;
// used to alternate DC flow
bool direction = false;
// Keeps the number of flips to perform on the minute tile to adjust for DST changes.
// Values can be positive, 0, or negativ.
// It is assumed that the main loop is performed at a higher frequency than a minute (e.g. once every 500ms)
// Positive values indicate the number of minutes to advance the clock.
// A 0 indicates that the clock does not need to get changed.
// Negative values indicate that the clock must be paused for -n minutes.
// Positive values larger than one and negative values can be used to perform summer/winter time adjustment.
int dst_adjustment = 0;
// keeps the last time assumed to be displayed on clock
struct tm clock_time;

/**
 * Setup NTP and set local time zone
 */
void initTime(String timezone)
{
  struct tm timeinfo;

  DEBUG_PRINTLN("Setting up time");
  // First connect to NTP server, with 0 TZ offset
  configTime(0, 0, ntp_pool);
  int ntp_retries = 3;
  while (ntp_retries > 0)
  {
    if (!getLocalTime(&timeinfo))
    {
#ifdef DEBUG_ENABLED
      Serial.println("  Failed to obtain time");
      Serial.print("  Retry attempts left: ");
      Serial.println(ntp_retries);
#endif
      ntp_retries -= 1;
    }
    else
    {
#ifdef DEBUG_ENABLED
      Serial.println("  Got time from NTP");
      Serial.printf("  Setting Timezone to %s\n", timezone.c_str());
#endif
      // Set the timezone
      setenv("TZ", timezone.c_str(), 1);
      tzset();
      break;
    }
  }
}

// Function to alternate the direction of current to drive the clock motor
void advanceMinuteTile()
{
  direction = !direction;
#ifdef DEBUG_ENABLED
  if (direction)
  {
    Serial.println("DC >>>");
  }
  else
  {
    Serial.println("DC <<<");
  }
#endif
  digitalWrite(input1_pin, direction);
  digitalWrite(input2_pin, !direction);
  digitalWrite(enable_pin, HIGH); // Enable the motor
}

/**
 * Setup Bodet Clock Driver
 */
// cppcheck-suppress unusedFunction
void setup()
{
  Serial.begin(115200);
  // Initializte the GPIO pins as output and set voltage
  pinMode(enable_pin, OUTPUT);
  pinMode(input1_pin, OUTPUT);
  pinMode(input2_pin, OUTPUT);
  //better without? advanceMinuteTile();
  // just plugin in clock a few seconds after the minute switched to what is shown on the clock.

#ifdef BARE_BONE_MODE
  return;
#endif

  // Connect to WiFi network
  WiFi.begin(ssid, password);
  DEBUG_PRINT("Connecting to WiFi.");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(200);
    DEBUG_PRINT(".");
  }
  DEBUG_PRINTLN("");
  DEBUG_PRINTLN("Connected to WiFi.");
  DEBUG_PRINT("IP Address: ");
  DEBUG_PRINTLN(WiFi.localIP());

  // Configure NTP and clock time
  initTime(cet_tz);
  if (!getLocalTime(&clock_time))
  {
    DEBUG_PRINTLN("Failed to obtain clock time");
    return;
  }

#ifdef DEBUG_ENABLED
  Serial.print("Clock time is: ");
  Serial.println(&clock_time, "%A, %B %d %Y %H:%M zone %Z %z ");
  Serial.println("Setup completed.");
#endif

  setup_completed = true;
}

/**
* Loop for bare bone testing mode.
*/
void bare_bone_loop()
{
  DEBUG_PRINT("Advance Tile in bare bone mode.");
  advanceMinuteTile();
  delay(500);
}

/**
 * Main loop driving the minute tile flip. This loop should be run more often than once per minute.
 * Otherwise the calcuation of minutes to flip will not work.
 */
void main_loop()
{
  // check if time advanced by a minute
  bool advanced_by_minute = false;
  struct tm system_time;
  if (!getLocalTime(&system_time))
  {
    DEBUG_PRINTLN("Failed to obtain new system time. Using last clock time.");
    system_time = clock_time;
  }
  if (system_time.tm_min != clock_time.tm_min)
  {
    advanced_by_minute = true;
  }

  // check for DST change
  if (system_time.tm_isdst && !clock_time.tm_isdst)
  {
    // switch to summer time (DST / CEST)
    dst_adjustment += 60;
  }
  else if (!system_time.tm_isdst && clock_time.tm_isdst)
  {
    // switch to winter time (non-DST / CET)
    dst_adjustment -= 60;
  }

  // advance the clock per minute or to catch up after switch to DST
  if (advanced_by_minute)
  {
#ifdef DEBUG_ENABLED
    Serial.print("System time: ");
    Serial.println(&system_time, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
    Serial.print("Clock time : ");
    Serial.println(&clock_time, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
    Serial.print("Advance minute: ");
    Serial.print(advanced_by_minute);
    Serial.print("DST adjustments: ");
    Serial.println(dst_adjustment);
#endif
    // set clock time to system time
    clock_time = system_time;

    if (dst_adjustment < 0)
    {
      // clock time is ahead of system time (DST to normal), do not flip minute tile.
      dst_adjustment += 1;
#ifdef DEBUG_ENABLED
      Serial.print(" > DST adjustment: *not* advancing minute, left: ");
      Serial.println(dst_adjustment);
#endif
    }
    else
    {
      // clock time is in sync (normal) or behind system time, flip minute tile.
      advanceMinuteTile();
      // do not reduce dst_adjustment as this is a regular minute
    }
  }
  else if (dst_adjustment > 0)
  {
    // if clock time is running behind (DST change), catch up
    advanceMinuteTile();
    dst_adjustment -= 1;
#ifdef DEBUG_ENABLED
    Serial.print(" > DST adjustment: advancing minute, delta left: ");
    Serial.println(dst_adjustment);
#endif
  }

  // 500ms delay is the result of tests with the clock.
  // This results in tiles quickly for testing or during DST switch.
  delay(500);
}

// cppcheck-suppress unusedFunction
void loop()
{
  // To allow testing without WiFi and/or NTP
  if (!setup_completed)
  {
    bare_bone_loop();
    return;
  }
  else {
    main_loop();
  }  
}
