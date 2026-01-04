#include <Arduino.h>
#include <WiFi.h>
#include <esp_sntp.h>

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
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    Serial.println(buffer);
#endif
  }

  // Specialized helper for printing tm structs
  static void logTime(const char *label, const struct tm *t) {
#ifdef ENABLE_LOGGING
    if (t) {
      char timeStringBuff[64]; // Make sure the buffer is large enough
      strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", t);
      Serial.print(label);
      Serial.println(timeStringBuff);
    }
#endif
  }
};

// ============================================================================
// CONFIGURATION CONSTANTS
// ============================================================================

// Motor control pins
static const int MOTOR_ENABLE_PIN = 21;
static const int MOTOR_DIRECTION_A_PIN = 23;
static const int MOTOR_DIRECTION_B_PIN = 22;

// Timing constants (milliseconds)
static const int LOOP_DELAY_MS = 500;

// WiFi credentials
static const char *WIFI_SSID = "Alternativlos";
static const char *WIFI_PASSWORD = "Mannheim";

// NTP and timezone configuration
static const char *NTP_SERVER = "de.pool.ntp.org";
static const char *TIMEZONE_CET = "CET-1CEST,M3.5.0/02,M10.5.0/03";

// ============================================================================
// GLOBAL STATE
// ============================================================================

// Tracks the time (epoch) currently displayed on the physical clock
time_t displayedClockTime = 0;

// ============================================================================
// CLOCK DRIVER CLASS
// ============================================================================

/**
 * Class to handle the mechanical clock hardware
 */
class ClockDriver {
private:
  int _pinEnable;
  int _pinDirectionA;
  int _pinDirectionB;
  bool _direction;

public:
  ClockDriver(int enable, int directionA, int directionB) 
    : _pinEnable(enable), _pinDirectionA(directionA), _pinDirectionB(directionB), _direction(false) {}

  void begin() {
    pinMode(_pinEnable, OUTPUT);
    pinMode(_pinDirectionA, OUTPUT);
    pinMode(_pinDirectionB, OUTPUT);
    digitalWrite(_pinEnable, LOW); // Ensure motor is off
  }

  void advance() {
    _direction = !_direction;
    if (_direction) {
      Logger::log("DC >>>");
    } else {
      Logger::log("DC <<<");
    }

    digitalWrite(_pinDirectionA, _direction);
    digitalWrite(_pinDirectionB, !_direction);
    digitalWrite(_pinEnable, HIGH); // Enable the motor
  }
};

ClockDriver clockDriver(MOTOR_ENABLE_PIN, MOTOR_DIRECTION_A_PIN, MOTOR_DIRECTION_B_PIN);

// ============================================================================
// TIME INITIALIZATION
// ============================================================================

/**
 * Callback function for SNTP sync notifications
 */
void timeSyncNotificationCallback(struct timeval *tv)
{
  struct tm timeinfo;
  time_t syncTime = tv->tv_sec;
  localtime_r(&syncTime, &timeinfo);
  Logger::logTime("NTP sync completed at ", &timeinfo);
}

/**
 * Setup NTP and set local time zone
 */
void initTime(String timezone)
{
  struct tm timeinfo;

  Logger::log("Setting up time");
  
  // Configure SNTP before connecting
  sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);  // Gradual time adjustment
  sntp_set_sync_interval(3600000);            // Sync every hour (in milliseconds)
  sntp_set_time_sync_notification_cb(timeSyncNotificationCallback);
  
  // First connect to NTP server, with 0 TZ offset
  configTime(0, 0, NTP_SERVER);
  int ntpRetries = 3;
  while (ntpRetries > 0)
  {
    if (!getLocalTime(&timeinfo))
    {
      Logger::log("  Failed to obtain NTP time");
      Logger::log("  Retry attempts left: %d", ntpRetries);
      ntpRetries -= 1;
    }
    else
    {
      Logger::log("  NTP setup successful");
      Logger::log("  Setting Timezone to %s", timezone.c_str());
      // Set the timezone
      setenv("TZ", timezone.c_str(), 1);
      tzset();
      break;
    }
  }
}

// ============================================================================
// ARDUINO SETUP AND MAIN LOOP
// ============================================================================

/**
 * Setup Bodet Clock Driver
 */
// cppcheck-suppress unusedFunction
void setup()
{
  Logger::begin(57600);
  clockDriver.begin();

  // Connect to WiFi network
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
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
  initTime(TIMEZONE_CET);
  
  // Initialize the displayed clock time to current time
  time(&displayedClockTime);

  struct tm timeinfo;
  localtime_r(&displayedClockTime, &timeinfo);
  Logger::logTime("Clock initialized to: ", &timeinfo);
  Logger::log("Setup completed.");
}

/**
 * Main loop driving the minute tile flip. This loop should be run more often than once per minute.
 * Otherwise the calcuation of minutes to flip will not work.
 */
// cppcheck-suppress unusedFunction
void loop()
{
  static unsigned long lastWifiReconnectAttempt = 0;
  static bool wifiWasConnected = true; // Assume connected after setup

  // Non-blocking WiFi Reconnection Logic
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiWasConnected) {
      Logger::log("WiFi Connection lost. Clock continues on internal timer.");
      wifiWasConnected = false;
    }

    unsigned long currentMillis = millis();
    // Try to reconnect every 60 seconds, but don't block execution
    if (currentMillis - lastWifiReconnectAttempt > 60000) {
      Logger::log("Attempting background WiFi reconnect...");
      WiFi.reconnect();
      lastWifiReconnectAttempt = currentMillis;
    }
  } else {
    if (!wifiWasConnected) {
      Logger::log("WiFi reconnected.");
      wifiWasConnected = true;
      // Force an immediate NTP resync after reconnection
      Logger::log("Forcing NTP resync...");
      sntp_restart();
    }
  }

  time_t now;
  time(&now);

  // Calculate the difference in minutes between current system time and displayed time
  // difftime returns double (seconds), so we divide by 60
  // Use round() to properly handle fractional minutes (e.g., 59.98 minutes should be 60)
  double diffSeconds = difftime(now, displayedClockTime);
  int minutesDifference = (int)round(diffSeconds / 60.0);

  if (minutesDifference != 0) {
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    Logger::logTime("System time: ", &timeinfo);    
    localtime_r(&displayedClockTime, &timeinfo);
    Logger::logTime("Clock time:  ", &timeinfo);
    Logger::log("Time diff (min): %d", minutesDifference);
  }

  if (minutesDifference > 0)
  {
    // The clock is behind system time (or DST forward switch)
    // Advance the clock one minute
    clockDriver.advance();
    
    // Update the internal state: we are now 60 seconds closer to 'now'
    // We add 60 seconds to the *displayed* time, not just set it to 'now',
    // to ensures we step through every minute physically.
    displayedClockTime += 60;
  }

  delay(LOOP_DELAY_MS);
}