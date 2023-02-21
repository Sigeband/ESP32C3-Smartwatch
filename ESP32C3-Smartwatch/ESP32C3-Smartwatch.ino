
#include <Arduino.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <WiFiManager.h>


#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif


String openWeatherMapApiKey = "your-api-key";
String city = "Cityname&units=metric&"; //replace Cityname and units with your prefered.
String countryCode = "your-country-code";  //replace 2- digit country code
String jsonBuffer;

// Variables to store the time
byte hours = 0;
byte minutes = 0;
byte seconds = 0;

// Constants for the button pins
const int PIN_BUTTON_MODE = D0;
const int PIN_BUTTON_SET = D1;
const int WEATHERPIN = D2;

const int BUTTON_MODE_DEBOUNCE_TIME = 250;
const int BUTTON_SET_DEBOUNCE_TIME = 10;

const int MODE_SHOW_TIME = 0;
const int MODE_SET_SECONDS = 3;
const int MODE_SET_MINUTES = 2;
const int MODE_SET_HOURS = 1;

// Variables for the button state
// We are using the internal pull-up resistors via INPUT_PULLUP, so
// press is LOW and not pressed is HIGH
unsigned long elapsedButtonModeMillis = 0;
unsigned long previousButtonModeMillis = 0;

unsigned long elapsedButtonSetMillis = 0;
unsigned long previousButtonSetMillis = 0;

// Char array for the time being showed on the display
char timeString[9];

// Variables to store the time
unsigned long currentMillis = 0;

// Int is enough to store the elapsed time
int elapsedTimeUpdateMillis = 0;
unsigned long previousTimeUpdateMillis = 0;

float percentageOfSecondElapsed = 0;

byte currentMode = MODE_SHOW_TIME;



// A complete list of all displays is available at: https://github.com/olikraus/u8g2/wiki/u8g2setupcpp
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

void setup(void) {

  Serial.begin(115200);

    // Configure the pins of the buttons with the internal PULLUP resistor
  pinMode(PIN_BUTTON_MODE, INPUT_PULLUP);
  pinMode(PIN_BUTTON_SET, INPUT_PULLUP);
  pinMode(WEATHERPIN, INPUT_PULLUP);

 
  bool res;

  WiFiManager wm; //start WifiManager
  res = wm.autoConnect("AutoConnectAP","password"); //first parameter is name of access point, second is the password
  
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
 

  u8g2.setFont(u8g2_font_logisoso28_tf); //set font
  u8g2.begin();
}

void loop(void) {

  // millis() itself takes 1.812 micro seconds that is 0.001812 milli seconds
  // https://arduino.stackexchange.com/questions/113/is-it-possible-to-find-the-time-taken-by-millis
  currentMillis = millis();

  if (digitalRead(WEATHERPIN) == LOW) { // if WEATHERPIN is low, execute function: updateweather
    updateWeather();
  }

  
  if (digitalRead(PIN_BUTTON_MODE) == LOW) {// if PIN_BUTTON_MODE is low, execute function: buttonModeHandler
    buttonModeHandler();
  }

  if (digitalRead(PIN_BUTTON_SET) == LOW) {// if PIN_BUTTON_SET is low, execute function: buttonSetHandler
    buttonSetHandler();
  }


  checkTime();

  if (currentMode == MODE_SHOW_TIME) {
    increaseSeconds();
  } else {
    previousTimeUpdateMillis = currentMillis;
  }

  drawScreen();

}

void checkTime() {
  // Check if a minutes has been elapsed
  if (seconds > 59) {
    seconds = 0;
    minutes++;
  }

  // Check if an hour has been elapsed
  if (minutes > 59) {
    minutes = 0;
    hours++;
  }

  // Check if a day has been elapsed
  if (hours > 23) {
    hours = 0;
  }
}
  
void updateWeather() {
 // Send an HTTP GET request to get the json data

    if(WiFi.status()== WL_CONNECTED){
      String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey;
                           //https://api.openweathermap.org/data/2.5/weather?q=buxtehude&units=metric&DE&APPID=yourauthkey
      jsonBuffer = httpGETRequest(serverPath.c_str());
      Serial.println(jsonBuffer);
      JSONVar myObject = JSON.parse(jsonBuffer);
  
      // JSON.typeof(jsonVar) can be used to get the type of the var
      if (JSON.typeof(myObject) == "undefined") {
        Serial.println("Parsing input failed!");
        return;
      }
    
      /*
      If you uncomment this, you will get all the weatherdata neatly printed in the serial monitor.

      Serial.print("JSON object = ");
      Serial.println(myObject);
      Serial.print("Temperature: ");
      Serial.println(myObject["main"]["temp"]);
      Serial.print("Pressure: ");
      Serial.println(myObject["main"]["pressure"]);
      Serial.print("Humidity: ");
      Serial.println(myObject["main"]["humidity"]);
      Serial.print("Wind Speed: ");
      Serial.println(myObject["wind"]["speed"]);
      */

    }
    else {
      Serial.println("WiFi Disconnected");
    }
    
  }


void buttonModeHandler() {
  elapsedButtonModeMillis = currentMillis - previousButtonModeMillis;
  if (elapsedButtonModeMillis > BUTTON_MODE_DEBOUNCE_TIME) {
    Serial.println("Mode Handler");
    previousButtonModeMillis = currentMillis;
    currentMode++;

    if (currentMode > 3) {
      currentMode = 0;
    }

  }
}


void buttonSetHandler() {
  elapsedButtonSetMillis = currentMillis - previousButtonSetMillis;
  Serial.println(elapsedButtonSetMillis);
  if (elapsedButtonSetMillis > BUTTON_SET_DEBOUNCE_TIME) {
    Serial.println("Set Handler");
    previousButtonSetMillis = currentMillis;

    if (currentMode == MODE_SET_SECONDS) {
      seconds = 0;
    }
    if (currentMode == MODE_SET_MINUTES) {
      minutes++;
    }
    if (currentMode == MODE_SET_HOURS) {
      hours++;
    }
  }
}

void increaseSeconds() {
  elapsedTimeUpdateMillis = currentMillis - previousTimeUpdateMillis;

  // Check if 1000ms, 1 second, has been elapsed
  if (elapsedTimeUpdateMillis > 1000) {
    seconds++;

    // It might be possible that more than 1000ms has been elapsed e.g. 1200ms 
    // Then there are already 200ms elapsed of the next second. We need to
    // substract these on the "last time". So the next second will be updated 200ms earlier. 
    // This reduces the amount of time drift.
    previousTimeUpdateMillis = currentMillis - (elapsedTimeUpdateMillis - 1000);
  }
}

void drawScreen() {
  u8g2.firstPage();
  u8g2.setFont(u8g2_font_logisoso28_tf);

  do {

    if (currentMode != MODE_SHOW_TIME) {
      u8g2.drawTriangle((currentMode - 1) * 43 + 5, 0, currentMode * 43 - 5, 0, (currentMode - 1) * 43 + 21, 5);
    }

    //drawAnimation();
    drawTime();

  } while (u8g2.nextPage());
}

void drawTime() {
  u8g2.setFont(u8g2_font_logisoso28_tf); //set font to big one for the clock

  // Found at https://forum.arduino.cc/index.php?topic=371117.0
  // sprintf_P uses the Program Memory instead of RAM, more info at http://gammon.com.au/progmem
  // Here we format the minutes and seconds with a leading zero: e.g. 01, 02, 03 etc.
  sprintf_P(timeString, PSTR("%2d:%02d:%02d"), hours, minutes, seconds);

  // Draw the timeString
  u8g2.drawStr(0, 45, timeString); //draw the time on the display
       
      JSONVar myObject = JSON.parse(jsonBuffer);

     u8g2.setCursor(0, 10); //start at the beginning
     u8g2.setFont(u8g2_font_luRS08_te); //set font to a small one for the temperature
     u8g2.println(myObject["main"]["temp"]); //print the temperature out of the main file.
     u8g2.setCursor(32, 10); //set cursor right next to the end of the temperature. the problem with this is that if the temperature is 1-9 its too far and if its 10+ then its right next to the last digit.
     u8g2.println("C"); // print C for celsius.

}


String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
    
  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}