/* RGB web server with ESP8266
* analogWrite with values received from web page
*
* WiFi router that creates a fixed domain: http://rgb
* web page returns POST request with 3 RGB parameters http://192.168.1.5/
* web page inspired by https://github.com/dimsumlabs/nodemcu-httpd
*/

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>

const char *ssid = "Sivak RGB Controller";
const char *password = "12345678";
const byte DNS_PORT = 53;
const int redLED = 12;   //D6
const int greenLED = 13; //D7
const int blueLED = 15;  //D8
IPAddress apIP(192, 168, 1, 5);
DNSServer dnsServer;
ESP8266WebServer webServer(80);
bool isfade=false;
//long deltas[3] = { 5, 6, 7 };
long rgbx[3];
long rgbval;
// for reasons unknown, if value !=0, the LED doesn't light. Hmm ...
// and saturation seems to be inverted
float hue=0.0, saturation=255, value=255;//1;

/*
chosen LED SparkFun sku: COM-09264
 has Max Luminosity (RGB): (2800, 6500, 1200)mcd
 so we normalize them all to 1200 mcd -
 R  250/600  =  107/256
 G  250/950  =   67/256
 B  250/250  =  256/256
 */
long bright[3] = { 256,256, 256};
#define SIZE    255
#define DELAY    20
#define HUE_MAX  255//6.0
#define HUE_DELTA 0.3// 0.01
byte RedLight;
byte GreenLight;
byte BlueLight;

String webpage = ""
"<!DOCTYPE html><html><head><title>RGB control</title><meta name='mobile-web-app-capable' content='yes' />"
"<meta name='viewport' content='width=device-width' /></head><body style='margin: 0px; padding: 0px;'>"
"<canvas id='colorspace'></canvas></body>"
"<script type='text/javascript'>"
"(function () {"
" var canvas = document.getElementById('colorspace');"
" var ctx = canvas.getContext('2d');"
" function drawCanvas() {"
" var colours = ctx.createLinearGradient(0, 0, window.innerWidth, 0);"
" for(var i=0; i <= 360; i+=10) {"
" colours.addColorStop(i/360, 'hsl(' + i + ', 100%, 50%)');"
" }"
" ctx.fillStyle = colours;"
" ctx.fillRect(0, 0, window.innerWidth, window.innerHeight);"
" var luminance = ctx.createLinearGradient(0, 0, 0, ctx.canvas.height);"
" luminance.addColorStop(0, '#ffffff');"
" luminance.addColorStop(0.05, '#ffffff');"
" luminance.addColorStop(0.5, 'rgba(0,0,0,0)');"
" luminance.addColorStop(0.95, '#000000');"
" luminance.addColorStop(1, '#000000');"
" ctx.fillStyle = luminance;"
" ctx.fillRect(0, 0, ctx.canvas.width, ctx.canvas.height);"
" }"
" var eventLocked = false;"
" function handleEvent(clientX, clientY) {"
" if(eventLocked) {"
" return;"
" }"
" function colourCorrect(v) {"
" return Math.round(1023-(v*v)/64);"
" }"
" var data = ctx.getImageData(clientX, clientY, 1, 1).data;"
" var params = ["
" 'r=' + colourCorrect(data[0]),"
" 'g=' + colourCorrect(data[1]),"
" 'b=' + colourCorrect(data[2])"
" ].join('&');"
" var req = new XMLHttpRequest();"
" req.open('POST', '?' + params, true);"
" req.send();"
" eventLocked = true;"
" req.onreadystatechange = function() {"
" if(req.readyState == 4) {"
" eventLocked = false;"
" }"
" }"
" }"
" canvas.addEventListener('click', function(event) {"
" handleEvent(event.clientX, event.clientY, true);"
" }, false);"
" canvas.addEventListener('touchmove', function(event){"
" handleEvent(event.touches[0].clientX, event.touches[0].clientY);"
"}, false);"
" function resizeCanvas() {"
" canvas.width = window.innerWidth;"
" canvas.height = window.innerHeight;"
" drawCanvas();"
" }"
" window.addEventListener('resize', resizeCanvas, false);"
" resizeCanvas();"
" drawCanvas();"
" document.ontouchmove = function(e) {e.preventDefault()};"
" })();"
"</script></html>";

//////////////////////////////////////////////////////////////////////////////////////////////////
void handleFade()
{
  isfade=true;
}

void handleRoot() {
  isfade=false;
// Serial.println("handle root..");
String red = webServer.arg(0); // read RGB arguments
String green = webServer.arg(1);
String blue = webServer.arg(2);

if((red != "") && (green != "") && (blue != ""))
{
  analogWrite(redLED, 1023 - red.toInt());
  analogWrite(greenLED, 1023 - green.toInt());
  analogWrite(blueLED, 1023 - blue.toInt());
}


Serial.print("Red: ");
Serial.println(red.toInt()); 
Serial.print("Green: ");
Serial.println(green.toInt()); 
Serial.print("Blue: ");
Serial.println(blue.toInt()); 
Serial.println();

webServer.send(200, "text/html", webpage);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {

pinMode(redLED, OUTPUT);
pinMode(greenLED, OUTPUT);
pinMode(blueLED, OUTPUT);

analogWrite(redLED, 0);
analogWrite(greenLED, 0);
analogWrite(blueLED, 0);

delay(1000);
Serial.begin(115200);
Serial.println();

WiFi.mode(WIFI_AP);
WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
WiFi.softAP(ssid);

// if DNSServer is started with "*" for domain name, it will reply with provided IP to all DNS request
dnsServer.start(DNS_PORT, "rgb", apIP);

webServer.on("/", handleRoot);
webServer.on("/fade", handleFade);
webServer.begin();

testRGB();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void loop() {


  if (isfade)
  {
   hue += HUE_DELTA;
  if (hue > HUE_MAX) {
    hue=0.0;
  }
setLedColorHSV(hue, saturation, value);
  rgbx[0]=RedLight;
  rgbx[1]=GreenLight;
  rgbx[2]=BlueLight;
  /*
  rgbval=HSV_to_RGB(hue, saturation, value);
  rgbx[0] = (rgbval & 0x00FF0000) >> 16; // there must be better ways
  rgbx[1] = (rgbval & 0x0000FF00) >> 8;
  rgbx[2] = rgbval & 0x000000FF;*/
  for (int k=0; k<3; k++) { // for all three colours
   if (k==0) analogWrite(redLED, rgbx[0] * bright[0]/256); // R set
   if (k==1) analogWrite(greenLED, rgbx[1] * bright[1]/256); // G set
   if (k==2) analogWrite(blueLED, rgbx[2] * bright[2]/256); // B set
    //analogWrite(R_PIN + k, rgbx[k] * bright[k]/256);
  }
  
  delay(DELAY);
  }

dnsServer.processNextRequest();
webServer.handleClient();

}
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

void testRGB() { // fade in and out of Red, Green, Blue

analogWrite(redLED, 0); // R off
analogWrite(greenLED, 0); // G off
analogWrite(blueLED, 0); // B off

fade(redLED); // R
fade(greenLED); // G
fade(blueLED); // B
}

//////////////////////////////////////////////////////////////////////////////////////////////////



void setLedColorHSV(byte h, byte s, byte v) {
  // this is the algorithm to convert from RGB to HSV
  h = (h * 192) / 256;  // 0..191
  unsigned int i = h / 32;   // We want a value of 0 thru 5
  unsigned int f = (h % 32) * 8;   // 'fractional' part of 'i' 0..248 in jumps

  unsigned int sInv = 255 - s;  // 0 -> 0xff, 0xff -> 0
  unsigned int fInv = 255 - f;  // 0 -> 0xff, 0xff -> 0
  byte pv = v * sInv / 256;  // pv will be in range 0 - 255
  byte qv = v * (256 - s * f / 256) / 256;
  byte tv = v * (256 - s * fInv / 256) / 256;

  switch (i) {
  case 0:
    RedLight = v;
    GreenLight = tv;
    BlueLight = pv;
    break;
  case 1:
    RedLight = qv;
    GreenLight = v;
    BlueLight = pv;
    break;
  case 2:
    RedLight = pv;
    GreenLight = v;
    BlueLight = tv;
    break;
  case 3:
    RedLight = pv;
    GreenLight = qv;
    BlueLight = v;
    break;
  case 4:
    RedLight = tv;
    GreenLight = pv;
    BlueLight = v;
    break;
  case 5:
    RedLight = v;
    GreenLight = pv;
    BlueLight = qv;
    break;
  }
}

void fade(int pin) {

for (int u = 0; u < 1024; u++) {
analogWrite(pin, u);
delay(1);
}
for (int u = 0; u < 1024; u++) {
analogWrite(pin, 1023 - u);
delay(1);
}
}
