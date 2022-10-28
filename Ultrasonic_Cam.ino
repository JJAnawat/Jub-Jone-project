#include <TridentTD_LineNotify.h>
#include <FS.h>
#include <ESP8266HTTPClient.h>

//WiFi
#define SSID        "HW_Wifi"
#define PASSWORD    "horwangwifi"

//IP Camera 
String IPCAM_IP  =  "172.23.38.24:36649";
String IPCAM_USERNAME = "admin";
String IPCAM_PASSWORD = "Horwang2565";

//Line Token  
//#define LINE_TOKEN "osGCbOUBe9ED1S0FeROy9SftBiqOp9WITJIETp5Lwr9" //for group
#define LINE_TOKEN "YqDEvvYZiWmTI0fVJROVYI32II0pxlGlSHskmYNFA5o" //for 1on1 chat

//Ultrasonic
#define trigPin 12
#define echoPin 14
#define SOUND_VELOCITY 0.034
long duration;
float distanceCm;

//Ip cam on?
bool ipCameraEnabled =false;

void downloadAndSaveFile(String fileName, String  url){
  
  HTTPClient http;

  Serial.println("[HTTP] begin...\n");
  Serial.println(fileName);
  Serial.println(url);
  http.begin(url);
  
  Serial.printf("[HTTP] GET...\n", url.c_str());
  // start connection and send HTTP header
  int httpCode = http.GET();
  if(httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);
      Serial.printf("[FILE] open file for writing %d\n", fileName.c_str());
      
      File file = SPIFFS.open(fileName, "w");

      // file found at server
      if(httpCode == HTTP_CODE_OK) {

          // get lenght of document (is -1 when Server sends no Content-Length header)
          int len = http.getSize();

          // create buffer for read
          uint8_t buff[128] = { 0 };

          // get tcp stream
          WiFiClient * stream = http.getStreamPtr();

          // read all data from server
          while(http.connected() && (len > 0 || len == -1)) {
              // get available data size
              size_t size = stream->available();
              if(size) {
                  // read up to 128 byte
                  int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
                  // write it to Serial
                  //Serial.write(buff, c);
                  file.write(buff, c);
                  if(len > 0) {
                      len -= c;
                  }
              }
              delay(1);
          }
          Serial.println();
          Serial.println("[HTTP] connection closed or file end.\n");
          Serial.println("[FILE] closing file\n");
          file.close();   
      }
  }
  http.end();
}

void sendLineNotify(){
  
  LINE.setToken(LINE_TOKEN);
  LINE.notify("Intruder detected");

  if(ipCameraEnabled){
    downloadAndSaveFile("/snapshot.jpg","http://"+ IPCAM_IP 
    +"/snapshot.cgi?user="+ IPCAM_USERNAME+"&pwd="+ IPCAM_PASSWORD);
    //listFiles();  
    LINE.notifyPicture("Camera snapshot", SPIFFS, "/snapshot.jpg");

  }
}

//List files in SPIFFS (For debugging)
void listFiles(void) {
  Serial.println();
  Serial.println("SPIFFS files found:");

  Dir dir = SPIFFS.openDir("/"); // Root directory
  String  line = "=====================================";

  Serial.println(line);
  Serial.println("  File name               Size");
  Serial.println(line);

  while (dir.next()) {
    String fileName = dir.fileName();
    Serial.print(fileName);
    int spaces = 25 - fileName.length(); // Tabulate nicely
    while (spaces--) Serial.print(" ");
    File f = dir.openFile("r");
    Serial.print(f.size()); Serial.println(" bytes");
  }

  Serial.println(line);
  Serial.println();
  delay(1000);
}

void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  
  WiFi.begin(SSID, PASSWORD);
  Serial.printf("WiFi connecting to %s\n",  SSID);
  while(WiFi.status() != WL_CONNECTED) { Serial.print("."); delay(400); }
  Serial.printf("\nWiFi connected\n");
  
  delay(1000);

  if(ipCameraEnabled){
    //Initialize File System
    if(SPIFFS.begin()){
      Serial.println("SPIFFS Initialize....ok");
    }else{
      Serial.println("SPIFFS Initialization...failed");
    }
   
    //Format File System
    if(SPIFFS.format()){
      Serial.println("File System Formated");
    }else{
      Serial.println("File System Formatting Error");
    }
  }

  Serial.println("-- Intruder Detection READY --");

  LINE.setToken(LINE_TOKEN);
  LINE.notify("Intruder Detector Ready");
}

void loop() {
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  
  // Calculate the distance
  distanceCm = duration * SOUND_VELOCITY/2;
  
  if(distanceCm<50&&distanceCm>10){
    Serial.println("Intruder Detected");
    Serial.print("Distance(CM) : ");
    Serial.println(distanceCm);
    sendLineNotify();
    delay(1000);
  }
  delay(100);
}
