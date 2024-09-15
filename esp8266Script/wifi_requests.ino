#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <HTTPSRedirect.h>

// Note that the pins are reversed. In the hardware, the light is connected to the NODEMCU ESP8266 PIN D2
// However, in the Arduino IDE this becomes pin 4 
// For the button pin the same thing occurs, it is connected to D4 but must say 2. 
#define LIGHT_PIN 4
#define BUTTON_PIN 2

// Parameters:
// 1 = num of pixels in the circle (16 in this case)
// 2 = the number of the pin - reversed from harware (see comment above for the define light pin
// 3 = pixel type flags added together:
//                  NEO_KHZ800  800 KHz bitstream 
//                  NEO_GRB     for GRB bitstream 
Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, LIGHT_PIN, NEO_GRB + NEO_KHZ800);

// indicates the colour selected 
// each colour is mapped to a number
int colour;

// simple flag to check when the button has been pressed and remember for future use
int pressed;

//current state of the button or switched (pressed or not pressed)
int currentButtonState = 0;  

// CUSTOMIZE THIS SECTION
// --------------------------------------------------
// your ssid and wifi passwords 
const char *wifi_ssid =  "your WIFI name";     
const char *pass =  "your WIFI password";
// your Google Script Deployment ID 
const char *DeploymentID = "deployment id from the current google script deployment";
// --------------------------------------------------

// Google Sheets information (no need to change this)
const char* host1 = "script.google.com";
const int httpsPort = 443;
String url = String("/macros/s/") + DeploymentID + "/exec";
HTTPSRedirect* client1 = nullptr;

// setup for timer till next GET request to see if the colour has changed
unsigned long startMillis;
unsigned long currentMillis; 
unsigned long period = 300000; // this is in milliseconds, it equals 5 min

// will be used for the body of the POST request (do not change)
String payload_base =  "{\"colour\": ";
String payload = "";

void setup() 
{
  Serial.begin(9600);
  delay(20);
               
  Serial.println("Connecting to the WIFI:  ");
  Serial.println(wifi_ssid); 
 
  WiFi.begin(wifi_ssid, pass); 
      
  // while the WIFI is not connected wait (blocking code)
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("._.");
  }
     
  Serial.println("WiFi Connected :)"); 
      
  // create a TLS connection so that the client can communicate with the server/host
  client1 = new HTTPSRedirect(httpsPort);
  client1->setInsecure();
  client1->setPrintResponseBody(true);

  // the body of the request is json format
  client1->setContentTypeHeader("application/json");
  
  Serial.print("Connecting to ");
  Serial.println(host1);

  // Attempt connection 5 times
  bool flag = false;
    for (int i=0; i<5; i++){ 
      int retval = client1->connect(host1, httpsPort);
      if (retval == 1){
        flag = true;
        Serial.println("Connected");
        break;
      } else{
        Serial.println("Connection failed. Retrying...");
      }
    }
        
    if (!flag){
      Serial.print("Could not connect to server: ");
      Serial.println(host1);
        return;
    }

    // delete the HTTPSRedirect client
    delete client1;    
    client1 = nullptr; 

    // initialize lights to pink (pink = 7)
    strip.begin();
    strip.setBrightness(50); //adjust brightness here
    strip.show(); // Initialize all pixels to 'off'
    colour = 7;
    pressed = 1;
      
    // initialize the button or switch
    pinMode(BUTTON_PIN, INPUT);
    startMillis= millis();
    currentMillis = startMillis;
}
 

void loop() {
  // read the current state of the button or switch
  currentButtonState = digitalRead(BUTTON_PIN);

  // Check if the button is currently pressed. 
  // If it is pressed, the currentButtonState is LOW
  if (currentButtonState == LOW) {
    // press detected
    pressed = 0;
    // indicate that the button press has been detected by flashing green
    // user can now release the button
    strip.fill(strip.Color(0, 255, 0), 0, 15); // Green all at once
    delay(500);
    colorWipe(strip.Color(0, 0, 0 ), 20); 
  } else {
    // if the button is no longer pressed but a press was detected -> change the colour
      if(pressed == 0){
        // increment the colour to change it
        colour = colour + 1;
        // if the colour is more than the max value of 10 reset to 1
        if( colour == 11 ){
           colour = 1;
           period = 300000;
        } else if( colour == 10 ){
          // 10 is a power saving mode (lights off and less get requests)
          period = 300000 * 2;
        } else{
          period = 300000;
        }
        Serial.print(colour);
     
        // do a POST request to write the new colour into the google sheet
        writeData(0);
    
    }
    pressed = 1;
    
    //read data from google sheet if 5 min has passed
    currentMillis = millis();
    if((currentMillis -  startMillis) > period){
      readData();
      startMillis = currentMillis;
    }
    if(startMillis > currentMillis){
      startMillis = currentMillis;
    }
    
    // change the colour depending on the obtained value
     switch (colour) {
    case 1:  
      colorWipe(strip.Color(255, 0, 0), 100); // red 
      break;
    case 2:  
      colorWipe(strip.Color(0, 255, 0), 100); // Green
      break;
    case 3: 
      colorWipe(strip.Color(86, 157, 117 ), 100); //white 
      break;
    case 4: 
      colorWipe(strip.Color(100, 50, 0), 100); // orange
      break;
    case 5:  
      colorWipe(strip.Color(0, 0, 255 ), 100); //blue
      break;
    case 6:  
      colorWipe(strip.Color(50, 0, 200 ), 100); //purple
      break;
    case 7:  
      colorWipe(strip.Color(150,0,100), 100); //pink
      break;
    case 8:  
      rainbow(16); 
      break;
    case 9:  
      rainbowCycle(16); 
      break;
    case 10:
      // power saving mode 
      colorWipe(strip.Color(0, 0, 0 ), 100); // no colour 
      break;
    }
  }

 delete client1;    
 client1 = nullptr; 
  
  
}


// Taken from the neopixel documentation and examples
// Fill the dots one after the other with a color 
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
  delay(2000);
}


// Taken from the neopixel documentation and examples
void rainbow(uint8_t wait) {
  uint16_t i, j;
 
  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Taken from the neopixel documentation and examples
// distributed rainbow 
void rainbowCycle(uint8_t wait) {
  int touch = 0;
  uint16_t i, j;
 
  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Taken from the neopixel documentation and examples
// Input a value 0 to 255 to get a color value.
// .
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

// GET request
void readData(){
  if( WiFi.status() != WL_CONNECTED){
      reconnectWifi();
  }
  connectClient();
  // read data from the Google Sheet
  Serial.println("Initiating GET request -> ");

  if(client1->GET(url, host1)){ 

      String current_colour_value = client1->getResponseBody();
      
      // retrieve body (colour number)
      Serial.print(current_colour_value.toInt());
      colour = current_colour_value.toInt();
  }
  else{
    // do stuff here if publish was not successful
    Serial.println("Error - did not connect");
  }
  if ( colour == 10 ){
        // 10 is a power saving mode (lights off and less get requests)
        period = 300000 * 2;
  }
  delete client1;    
  client1 = nullptr; 
}

// POST request
void writeData(int num){
  if( WiFi.status() != WL_CONNECTED){
      reconnectWifi();
  }
  connectClient();
  // write data to the Google Sheet
  Serial.println("Initiating POST request ->");
  
  // complete the payload with the updated colour value
  payload = payload_base  + colour + "}";
 
  if(client1->POST(url, host1, payload)){ 
    Serial.println(colour);
  }
  else{
    Serial.println("Error - did not connect");
    // retry once
    if (num != 1){
      writeData(1);
    }
  }
 delete client1;    
 client1 = nullptr; 
}

// make sure the WIFI connection has not been lost
void reconnectWifi(){
  
     WiFi.begin(wifi_ssid, pass); 
     Serial.print("Reconnecting to WIFI");
     while (WiFi.status() != WL_CONNECTED){
            delay(500);
            Serial.print("._.");
     }
     Serial.print("Done :)");   
}

void connectClient(){
    client1 = new HTTPSRedirect(httpsPort);
    client1->setInsecure();
    client1->setPrintResponseBody(true);
    client1->setContentTypeHeader("application/json");
  
  if (client1 != nullptr){
    if (!client1->connected()){
      client1->connect(host1, httpsPort);
    }
  }else{
    Serial.println("Error creating a client :( ");
  }
}
