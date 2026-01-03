#include <Arduino.h>
#include <WiFi.h>

// Mode configuration - uncomment to run in bare bone mode
// will advance the tile every 0.5 sec.
//#define BARE_BONE_MODE

// Log configuration - comment out to disable logging
#define ENABLE_LOGGING

class Logger {
public:
  static void begin(unsigned long baud) {
#ifdef ENABLE_LOGGING
    Serial.begin(baud);
#endif
  }

  static void print(const char *msg) {
#ifdef ENABLE_LOGGING
    Serial.print(msg);
#endif
  }

  static void print(IPAddress ip) {
#ifdef ENABLE_LOGGING
    Serial.print(ip);
#endif
  }

  // Formatting logger (printf style) that adds a newline
  static void log(const char *format, ...) {
#ifdef ENABLE_LOGGING
    va_list args;
    va_start(args, format);
    Serial.printf(format, args);
    va_end(args);
    Serial.println();
#endif
  }

  // Specialized helper for printing tm structs
  static void logTime(const char *label, const struct tm *t) {
#ifdef ENABLE_LOGGING
    if (t) {
      char timeStringBuff[64]; // Make sure the buffer is large enough
      strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S zone %Z %z", t);
      Serial.print(label);
      Serial.println(timeStringBuff);
    }
#endif
  }
};

// Board parameters
static const int enable_pin = 21;
static const int input1_pin = 23;
static const int input2_pin = 22;

// Duration to keep the motor enabled in milliseconds
#define PULSE_DURATION_MS 500
#define LOOP_DELAY_MS 500

// WiFi parameters
static const char *ssid = "WLAN SSID";
static const char *password = "secret";

// NTP parameters
static const char *ntp_pool = "de.pool.ntp.org";
static const char *cet_tz = "CET-1CEST,M3.5.0/02,M10.5.0/03";

/** Global varibales */
bool setup_completed = false;
// keeps the last time (epoch) assumed to be displayed on clock
time_t last_displayed_time = 0;

/**
 * Class to handle the mechanical clock hardware
 */
class ClockDriver {
private:
  int _pin_enable;
  int _pin_in1;
  int _pin_in2;
  int _pulse_ms;
  bool _direction;

public:
  ClockDriver(int en, int in1, int in2, int pulse_ms) 
    : _pin_enable(en), _pin_in1(in1), _pin_in2(in2), _pulse_ms(pulse_ms), _direction(false) {}

  void begin() {
    pinMode(_pin_enable, OUTPUT);
    pinMode(_pin_in1, OUTPUT);
    pinMode(_pin_in2, OUTPUT);
    digitalWrite(_pin_enable, LOW); // Ensure motor is off
  }

  void advance() {
    _direction = !_direction;
    if (_direction) {
      Logger::log("DC >>>");
    } else {
      Logger::log("DC <<<");
    }

    digitalWrite(_pin_in1, _direction);
    digitalWrite(_pin_in2, !_direction);
    digitalWrite(_pin_enable, HIGH); // Enable the motor
    delay(_pulse_ms);
    digitalWrite(_pin_enable, LOW);  // Disable the motor
  }
};

ClockDriver clockDriver(enable_pin, input1_pin, input2_pin, PULSE_DURATION_MS);

/**
 * Setup NTP and set local time zone
 */
void initTime(String timezone)
{
  struct tm timeinfo;

  Logger::log("Setting up time");
  // First connect to NTP server, with 0 TZ offset
  configTime(0, 0, ntp_pool);
  int ntp_retries = 3;
  while (ntp_retries > 0)
  {
    if (!getLocalTime(&timeinfo))
    {
      Logger::log("  Failed to obtain time");
      Logger::log("  Retry attempts left: %d", ntp_retries);
      ntp_retries -= 1;
    }
    else
    {
      Logger::log("  Got time from NTP");
      Logger::log("  Setting Timezone to %s", timezone.c_str());
      // Set the timezone
      setenv("TZ", timezone.c_str(), 1);
      tzset();
      break;
    }
  }
}

/**
 * Setup Bodet Clock Driver
 */
// cppcheck-suppress unusedFunction
void setup()
{
  Logger::begin(115200);
  clockDriver.begin();

#ifdef BARE_BONE_MODE
  return;
#endif

  // Connect to WiFi network
  WiFi.begin(ssid, password);
  Logger::print("Connecting to WiFi.");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(200);
    Logger::print(".");
  }
  Logger::log(""); // Newline
  Logger::log("Connected to WiFi.");
  Logger::print("IP Address: ");
  Logger::print(WiFi.localIP());
  Logger::log(""); // Newline

  // Configure NTP and clock time
  initTime(cet_tz);
  
  // Initialize the last displayed time to current time
  time(&last_displayed_time);

  struct tm timeinfo;
  localtime_r(&last_displayed_time, &timeinfo);
  Logger::logTime("Clock initialized to", &timeinfo);
  Logger::log("Setup completed.");

  setup_completed = true;
}

/**
 * Main loop driving the minute tile flip. This loop should be run more often than once per minute.
 * Otherwise the calcuation of minutes to flip will not work.
 */
void main_loop()
{
  static unsigned long last_wifi_reconnect_attempt = 0;
  static bool wifi_was_connected = true; // Assume connected after setup

  // Non-blocking WiFi Reconnection Logic
  if (WiFi.status() != WL_CONNECTED) {
    if (wifi_was_connected) {
      Logger::log("WiFi Connection lost. Clock continues on internal timer.");
      wifi_was_connected = false;
    }

    unsigned long now_ms = millis();
    // Try to reconnect every 60 seconds, but don't block execution
    if (now_ms - last_wifi_reconnect_attempt > 60000) {
      Logger::log("Attempting background WiFi reconnect...");
      WiFi.reconnect();
      last_wifi_reconnect_attempt = now_ms;
    }
  } else {
    if (!wifi_was_connected) {
      Logger::log("WiFi reconnected.");
      wifi_was_connected = true;
      // Note: Standard ESP32 SNTP service will automatically resume syncing
    }
  }

  time_t now;
  time(&now);

  // Calculate the difference in minutes between current system time and displayed time
  // difftime returns double (seconds), so we divide by 60
  double diff_seconds = difftime(now, last_displayed_time);
  int diff_minutes = (int)(diff_seconds / 60);

  if (diff_minutes != 0) {
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    Logger::logTime("System time", &timeinfo);
    
    localtime_r(&last_displayed_time, &timeinfo);
    Logger::logTime("Clock time ", &timeinfo);
    
    Logger::log("Time diff (min): %d", diff_minutes);
  }

  if (diff_minutes > 0)
  {
    // The clock is behind system time (or DST forward switch)
    // Advance the clock one minute
    clockDriver.advance();
    
    // Update the internal state: we are now 60 seconds closer to 'now'
    // We add 60 seconds to the *displayed* time, not just set it to 'now',
    // to ensures we step through every minute physically.
    last_displayed_time += 60;
  }
  else if (diff_minutes < 0)
  {
    // The clock is ahead of system time (e.g. DST backward switch)
    // We do nothing and wait for time to catch up.
    // last_displayed_time remains the same.
    Logger::log("Clock ahead - Waiting for time to catch up...");
  }

  // delay to allow loop to breathe and motor to rest between rapid flips
  delay(LOOP_DELAY_MS);
}

// cppcheck-suppress unusedFunction
void loop()
{
  // To allow testing without WiFi and/or NTP
  if (!setup_completed)
  {
    // Testing mode: advances the clock tile periodically 
    // without any time synchronization logic.
    Logger::log("Advance Tile in test mode.");
    clockDriver.advance();
    delay(LOOP_DELAY_MS);
  }
  else {
    main_loop();
  }  
}
