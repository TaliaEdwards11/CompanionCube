#include <ESP8266WiFi.h>
#include <ESP8266WebServerSecure.h>
#include <ESP8266mDNS.h>
#include <HTTPSRedirect.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

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

// your Google Script Deployment ID 
const char *DeploymentID = "AKfycbwl9tOCRD_QhEDtShY2NenBS56sPgvzNIyYT5S9E6fli_b92YbCbyg45643l47_9STR";

// Google Sheets information (no need to change this)
const char* host1 = "script.google.com";
const int httpsPort = 443;
String url = String("/macros/s/") + DeploymentID + "/exec";
HTTPSRedirect* client1 = nullptr;
unsigned long period = 300000;

// will be used for the body of the POST request (do not change)
String payload_base =  "{\"colour\": ";
String payload = "";

// Initialize variables for WiFi connection
String wifiSSID =  "";     
String pass = "";
int retry = 0;
bool isWifiConnected = false;

// Initialize variables for the creation of an access portal and server
IPAddress local_IP(192,168,4,22);
IPAddress subnet(255,255,255,0);
BearSSL::ESP8266WebServerSecure server(443);
bool isFailed = false;
bool isServerOn = false;
unsigned long soft_access_portal_timeout = 600000; 

//Declaring variables for keeping track of time that has passed
unsigned long startMillis;
unsigned long currentMillis; 

// Declaring a struct
struct wifiConfig{
    byte isSet;
    char ssid[50];
    char pass[70]; 
 } savedConfig;

// Initializing certificate and private key to set up a TLS connection 
// between the server and the client
// the client receives the public certificate to encrypt messages
// the server decrypts them using the secret private key below
static const char serverCert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIEODCCAyCgAwIBAgIUV8gg4z8eDF0LWWF8/NAKYLGcyYMwDQYJKoZIhvcNAQEL
BQAwgaQxCzAJBgNVBAYTAkNBMRAwDgYDVQQIDAdPbnRhcmlvMQ8wDQYDVQQHDAZP
dHRhd2ExFjAUBgNVBAoMDUNvbXBhbmlvbkN1YmUxFjAUBgNVBAsMDUNvbXBhbmlv
bkN1YmUxGDAWBgNVBAMMD0NvbXBhbmlvbkN1YmVDQTEoMCYGCSqGSIb3DQEJARYZ
dGFsaWEubC5lZHdhcmRzQGdtYWlsLmNvbTAeFw0yNDA5MjQwMDE4MzFaFw0yOTA5
MjQwMDE4MzFaMIGiMQswCQYDVQQGEwJDQTEQMA4GA1UECAwHT250YXJpbzEPMA0G
A1UEBwwGT3R0YXdhMRYwFAYDVQQKDA1Db21wYW5pb25DdWJlMRYwFAYDVQQLDA1D
b21wYW5pb25DdWJlMRYwFAYDVQQDDA1Db21wYW5pb25DdWJlMSgwJgYJKoZIhvcN
AQkBFhl0YWxpYS5sLmVkd2FyZHNAZ21haWwuY29tMIIBIjANBgkqhkiG9w0BAQEF
AAOCAQ8AMIIBCgKCAQEA3o0Edb9q+CMkNKF+E5orJ+jfm8PSBiGnD+GBOY2Uia42
3FXaysDRZhZLM/V59i2Qf0Wfm8wzfq2FLNTOWpEIPRTQkh+vkkrnAVy2zUg9nowp
LmUaTr5ZTAJKkLQu25WbNx3OutLJktX6C+yPB+XpgKOt0KQAJDtHJkyepxR2PRoK
Vrp4jO5ibKzPY1ofoRQbZf+wazNcmt+8nTwNQMarZv5V2IkezxYRso2/72LFExBl
xYrHMjC2bc0PJCB2H92V+VcReWdUCnoweP+wiGplNbfeGMR/vJdiP/EIQwleIKWO
2fwTdVDSa6qkkcm+IQZ7Yk02/B17qafcsbYnp2uAkQIDAQABo2IwYDAfBgNVHSME
GDAWgBRBzQzi9x/4Y4uBBKTs69BDKlTgTTAJBgNVHRMEAjAAMAsGA1UdDwQEAwIE
8DAlBgNVHREEHjAcghRjb21wYW5pb24tY3ViZS5sb2NhbIcEwKgEFjANBgkqhkiG
9w0BAQsFAAOCAQEAei+VwCjOLaDT7lRo+1fwmdSLxIzm08GhBxa1rz3XT/zAZ9h8
pjIHyaxbu/gVzpioySf/x4y4GbmMADaHlID1unGQsvs3gvgRmDwuhRdF366FK3SY
QDlX96ijYR6y4DEvjQ1EldubM1v/GqjH75QpsLUtvGQ0gd0j6x5hGnvzS5WQ8VfN
YeFYP3JoeQFN87U8nhULgTf6eODmI64OA27Mnki0zEaxEN9XjCzC2/tYeR60pOpr
5Y6RwghOqA+J/tn5DWVmiWtQG1PV7JomWaixnzysuxIXaZ4CVNbj6MuQjRtWjStK
UTJcsXhABhzmp1WxT7RWpNeFnkdaP/EG1RTYAg==
-----END CERTIFICATE-----
)EOF";

static const char serverKey[] PROGMEM =  R"EOF(
-----BEGIN PRIVATE KEY-----
MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQDejQR1v2r4IyQ0
oX4Tmisn6N+bw9IGIacP4YE5jZSJrjbcVdrKwNFmFksz9Xn2LZB/RZ+bzDN+rYUs
1M5akQg9FNCSH6+SSucBXLbNSD2ejCkuZRpOvllMAkqQtC7blZs3Hc660smS1foL
7I8H5emAo63QpAAkO0cmTJ6nFHY9GgpWuniM7mJsrM9jWh+hFBtl/7BrM1ya37yd
PA1Axqtm/lXYiR7PFhGyjb/vYsUTEGXFiscyMLZtzQ8kIHYf3ZX5VxF5Z1QKejB4
/7CIamU1t94YxH+8l2I/8QhDCV4gpY7Z/BN1UNJrqqSRyb4hBntiTTb8HXupp9yx
tiena4CRAgMBAAECggEBAMpOGIrNGfk9QLFTSF+bWrWp3HMlXCtUtiAalzTxPeC2
bIp4xS5gfzm09GHkOR0PKHZ5nPCsfPcf15r8TCVKB9o2rK7YfOqYikvTsx0DGXih
4uGY6IRCRrxwrMXD9YCJDBLbVsy/xptjRi1GunKsjknxAJVUMAi/lhr7tZEeaal5
64qtwhFanZmGfjejUpIb7rVxbTVC+CDhM7dsqE9iUaT81SSRzI38EmCvrvzem1+D
lVDFBvm3sakmxC6ki/8fttxQerikL7dnEkr0GW4pFtHPz1pp40AspQtqbaXom8RO
8fm2sifClmouPLm34VABQFmnq3FdE3q12kmytm/+L/0CgYEA8BUTQIajCogQgA+c
FpSRKLTTWzj5Tt8EUdWD77bmjrfNpCxvm/xu8i1zQPo+47j03osmnfpsCvzswhV0
DIaX41QrRqy0bZyiDAYTJ+68CZRzCy8ZI/Symk+0StDzJFL+F/rWOhkuS9mrFv9p
9S1qp81Yww7X0wW+9EuFkbeyBacCgYEA7U5hQ1jTa4Efpc5H0RpfLxp8bHmWiMTn
xv5Kczszi2TEQHI1lV6+jKRC9F6Jd5LtpzA9cfgLKX0RzERc2DUljAXPvU1ID0Za
1fiGB2BttCr9A6ZKLB8S41Px+6dERUoz4kd91k6XbGBLjmoI3geF+n4XBLEdXY/r
EbsMi5g0/wcCgYBqH1bAOgNNv3FTuhKr5IX5sVyPwTJqi6gUKEZGqtllEpgizsWS
9NVx1WdRvIwaCwKqcGXkhPJHNR/Odr8yXjg7c1QhzGuA3DDBEdpb12pk7aqgsfPx
8dMU5NB0FcK0Nr88iFMzoTfWrWO0KbiDeMzhWaK3rhL2o16SC+QB0motlwKBgAMG
A6E7CX6OJ5wSM8ILRvkvqjQrnGpjr9xOMx6iu3ZkM0fq7JnZmi3qjdvVNIUsfxCp
Qa8pDONbb5WdrV0n2DGfhVURHgDr0Y52WybO6Lbp8U1RmhJT1wbEdAnTyL/lQSH5
7TWEzNMZqvzGhxRlHiTh5GMv6oLdqY4RRwikSNCTAoGAYfDTLRlMZIuoXRgdZTls
qi7U+rNPn28i8IfWKlMkY4Rb7U6+tCx82+YPnHOgqqbeceKqMmcMuJ4FI3uibxb/
N5Rza6w8rz9RmnxDdYyN6m7TW/K1wQXR/ceL56SEv+gdXaAoNstavkwkI7hd8vAg
2XaHXKJLQyino+UFyfTFMtA=
-----END PRIVATE KEY-----
)EOF";


// Initializing the HTML page to retrieve the WiFi configuration
char* config_html = "<!DOCTYPE HTML><html><head>"
  "<title>Wifi Config</title>"
  "<style>"
    "table, th, td {"
    "border: 1px solid;"
  "}"
    "body {"
      "background-color: #FAF9F6;"
    "}"
    
    "input[type=submit] {"
  "background-color: #25547a;"
  "border: none;"
  "margin: 3px;"
  "text-align: center;"
  "color: white;"
  "padding: 2px;"
  "cursor: pointer;"
"}"
"input[type=text] {"
  "margin: 8px 0;"
"}"
    "</style>"
  "</head><body>"
  "<h1>Companion Cube WiFi Configuration Form</h1><hr>" 
  "<form id=\"wifiForm\" action=\"/connect\" method=\"post\">"
    "<label for=\"ssid\">SSID:   </label>"
    "<input type=\"text\" name=\"ssid\"><br>"
    "<label for=\"password\">Password:   </label>"
    "<input type=\"password\" name=\"password\"><br>"
    "<input type=\"submit\" value=\"Submit\">"
  "</form><br><br>"
  "<table>"
  "<tr>"
    "<th style=\"background-color: #E5E4E2\">Cube Colour</th>"
    "<th style=\"background-color: #E5E4E2\">Meaning</th>"
  "</tr>"
  "<tr>"
    "<td  style=\"color:orange\">ORANGE</td>"
    "<td>Attempting to Connect. Wait.</td>"
  "</tr>"
  "<tr>"
    "<td  style=\"color:green\">GREEN</td>"
    "<td>Successful Connection. No other actions needed.</td>"
  "</tr>"
  "<tr>"
    "<td style=\"color:red\">RED</td>"
    "<td>Failed to connect.<br>" 
    "</td>"
  "</tr>"
  "<tr>"
    "<td  style=\"color:#FF1493\">PINK</td>"
    "<td>Failed to connect 2 times. Unplug and try again.</td>"
  "</tr>"
"</table>"
"</body></html>";


// Declaring the functions
void readData();
void writeData(int num);
void connectClient();
void reconnectWifi();
bool initialConnectionToGoogleSheets();
uint32_t Wheel(byte WheelPos);
void rainbowCycle(uint8_t wait);
void rainbow(uint8_t wait);
void colorWipe(uint32_t c, uint8_t wait);
void disconnect_softAP();
void handle_NotFound();
void connectClient();
void reconnectWifi();
void connect_wifi();
void get_wifi_config();
void displayPage();
void updateServer();


void setup() {
  // It is necessary to set up WIFI
  Serial.begin(115200);
  delay(20);
  // Initialize the EEPROM 
  // the EEPROM is emulated, the ESP8266 will use Flash Memory
  EEPROM.begin(500);
  
  // retrieve saved wifi configuration from memory
  // If they are not in memory, each byte is set as 255 (11111111)
  // store them in the savedConfig struct
  EEPROM.get(0, savedConfig); 
 
  // initialize light to off
  strip.begin();
  // adjust brightness here
  strip.setBrightness(50); 
  // Initialize all pixels to 'off'
  strip.show(); 
  colorWipe(strip.Color(0, 0, 0 ), 20);
      
  // initialize the button or switch
  pinMode(BUTTON_PIN, INPUT);
      
  // check if the values have been set before
  if(savedConfig.isSet == 0){
    // look at previously saved values
    Serial.print( "\n Data was restored from EEPROM\r\n");
    Serial.print("\r\n ssid="); Serial.print(savedConfig.ssid);
    Serial.print( "\r\n --------\r\n");
    
    wifiSSID = savedConfig.ssid;
    pass = savedConfig.pass;
    connect_wifi();
    retry = 0;
  } 
  
  if (!isWifiConnected){
    // the old WiFi credentials are wrong or not set
    // ask the user for new values using an access portal
    Serial.print("Setting soft-AP ... ");
  
    startMillis = millis();
    currentMillis = startMillis;

    Serial.println(WiFi.softAPConfig(local_IP, local_IP, subnet) ? "Ready" : "Failed!");
    Serial.println(WiFi.softAP("Companion Cube", "apple_pine_windOw2") ? "Ready" : "Failed!");
  
    Serial.print("Soft-AP IP address = "); 
    Serial.println(WiFi.softAPIP());

    // display webpage to client 
    server.on("/", displayPage);
    server.on("/connect", get_wifi_config);
      
    server.onNotFound(handle_NotFound);
    server.getServer().setRSACert(new BearSSL::X509List(serverCert), new   BearSSL::PrivateKey(serverKey));
    server.begin();
    if (MDNS.begin("companion-cube")) {
      Serial.println("MDNS responder started");
    }
    Serial.println("HTTP server started");
    isServerOn = true;
    
    }
    // WiFi did connect
    else{
      if(initialConnectionToGoogleSheets()){
        colour = 2;
        pressed = 0;
      }else{
        exit(0);
      }
    }       
}


int first = 1;
void loop() {
  // keep the WiFi Access protal open for 10 minutes then close 
  while ((currentMillis - startMillis) < soft_access_portal_timeout && !isWifiConnected && !isFailed && isServerOn){
    connect_wifi();
    MDNS.update();
    server.handleClient();
    delay(200);
    currentMillis = millis();
  }
  
  if(isServerOn && first==1){
    if(!isWifiConnected && (currentMillis - startMillis) < soft_access_portal_timeout){
      disconnect_softAP();
    }
    initialConnectionToGoogleSheets();
    first =0;
  }
  
  // read the current state of the button or switch
  currentButtonState = digitalRead(BUTTON_PIN);

  // Check if the button is currently pressed. 
  // If it is pressed, the currentButtonState is LOW
  if (currentButtonState == LOW) {
    // press detected
    pressed = 1;
    // indicate that the button press has been detected by flashing green
    // user can now release the button
    strip.fill(strip.Color(0, 255, 0), 0, 15); // Green all at once
    delay(500);
     colorWipe(strip.Color(0, 0, 0 ), 20); 
  ;
  } else {
    // if the button is no longer pressed but a press was detected -> change the colour
      if(pressed == 1){
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
    pressed = 0;
    
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
      //rainbow(16);
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

void displayPage(){
  server.send(200, "text/html", config_html); 
}



void get_wifi_config()
{
  
  // get the input for the WiFi  
  wifiSSID =server.arg("ssid");
  pass = server.arg("password");

  delay(200);
  MDNS.update();
  server.handleClient();

  server.sendHeader("Location","/");        // Add a header to respond with a new location for the browser to go to the home page again
  server.send(303);
}

void updateServer(){
  if(isServerOn){
    MDNS.update();
    server.handleClient();
  }
}

void connect_wifi(){
  
   if (wifiSSID == "" || pass == ""){
      return;
   }
   updateServer();
   
  unsigned long startMillisOfFunction = millis();
  currentMillis = startMillisOfFunction;
  
   colorWipe(strip.Color(100, 50, 0), 100); // orange
  
  Serial.println("Variables Received");

  Serial.println("Connecting to the WIFI:  ");
  Serial.println(wifiSSID); 
  
  updateServer();
  
  WiFi.begin(wifiSSID, pass); 
  
  // while the WIFI is not connected wait
  while ((WiFi.status() != WL_CONNECTED) && ((currentMillis - startMillisOfFunction) < 15000)) {
    currentMillis = millis();
    delay(500);
    updateServer();
    Serial.print("._.");
  }
  if (WiFi.status() == WL_CONNECTED){
    Serial.println("\nWe have connected to the WiFi!");
    isWifiConnected = true;
    Serial.println("WiFi Connected :)"); 
    colorWipe(strip.Color(0, 255, 0), 100); // Green all at once

    
    // if we connected to the WiFi because of the data from the server 
    if(isServerOn){
      Serial.println("Create char arrays"); 
      wifiSSID.toCharArray(savedConfig.ssid, 50);
     pass.toCharArray(savedConfig.pass, 70);
      savedConfig.isSet = 0;
      Serial.println("PUT IN EEPROM"); 
      EEPROM.put(0, savedConfig);  
      EEPROM.commit();
    }
    if(isServerOn){
      disconnect_softAP();
    }
    } else{
      WiFi.disconnect();

      // 2 chances to get the password right
      if (retry >= 1){
        Serial.println("WiFi Connection Failed :(");
        colorWipe(strip.Color(150,0,100), 100); //pink
        isFailed = true;
        wifiSSID= "";
        pass = "";
        if(isServerOn){
          disconnect_softAP();
        }
        exit(1);
    } else{
      
      updateServer();

      retry = retry + 1;
      wifiSSID= "";
      pass = "";
      colorWipe(strip.Color(255, 0, 0), 100); // red
      // TODO 
      //TODO store in storage + refresh for the status in the page 
      // have an inprogress variable so that variables are not submitted more than once (form resubmits every refresh or does it?)
      

    }
    
  }
}


void disconnect_softAP(){
   // shut down the access portal
  server.stop();
  if (!(WiFi.softAPdisconnect(true))){
    delay(200);
    // try one more time and then stop completely
    if (!(WiFi.softAPdisconnect(true))){
      Serial.println("\nCould not close the soft access portal Companion Cube.\nShutting down...");
      exit(1);
    }
  }
  Serial.println("\nThe Companion Cube access portal is now closed.");
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
// The colours are a transition r - g - b - back to r.
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


bool initialConnectionToGoogleSheets(){
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
            return true;
          } else
            Serial.println("Connection failed. Retrying...");
        }
        
      if (!flag){
        Serial.print("Could not connect to server: ");
        Serial.println(host1);
        return false;
      }

      // delete the HTTPSRedirect client
      delete client1;    
      client1 = nullptr; 
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
  payload = payload_base + colour + "}";
 
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
  
     WiFi.begin(wifiSSID, pass); 
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


void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
};
