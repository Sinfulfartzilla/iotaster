#include <Servo.h>
#include <WiFi.h>

Servo vexmotor;
TaskHandle_t Task1;
TaskHandle_t Task2;
bool powerstate = false;
int pos = 0;

// Replace with your network credentials
const char* ssid     = "Pretty Fly for a Wi-Fi";
const char* password = "livelycartoon963";

// LED pins
const int led1 = 2;

// Variable to store the HTTP request
String header;

// Set web server port number to 80
WiFiServer server(80);

void setup() {
  vexmotor.attach(13);
  Serial.begin(115200);
  pinMode(led1, OUTPUT); //the annoying blue led located on the pcb

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();

  //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore( //task 1 is the controller to control the toaster
    Task1code,   /* Task function. */
    "Task1",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &Task1,      /* Task handle to keep track of created task */
    0);          /* pin task to core 0 */
  delay(500);

  //create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore( //task 2 in the program controller
    Task2code,   /* Task function. */
    "Task2",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &Task2,      /* Task handle to keep track of created task */
    1);          /* pin task to core 1 */
  delay(500);
}

//Task1code: toaster controller
void Task1code( void * pvParameters ) {
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());
  for (;;) {
    if (powerstate == true) { //toaster on code
      if (digitalRead(14) != 1)
      {
        pos = 0; //bring down
      } else {
        pos = 170; //stop motor
      }
      delay(15);
    }
    if (powerstate == false) {  //toaster off code
      if (digitalRead(15) != 0)
      {
        pos = 60; //bring up
        digitalWrite(led1, LOW);
      } else {
        pos = 0; //stop motor
        digitalWrite(led1, HIGH);
      }
      delay(15);
    }
    vexmotor.write(pos);  //send the value to the motor controller
  }
}
//Task2code: blinks an LED every 700 ms
void Task2code( void * pvParameters ) {
  Serial.print("Task2 running on core ");
  Serial.println(xPortGetCoreID());

  for (;;) {
    WiFiClient client = server.available();   // Listen for incoming clients

    if (client) {                             // If a new client connects,
      Serial.println("New Client.");          // print a message out in the serial port
      String currentLine = "";                // make a String to hold incoming data from the client
      while (client.connected()) {            // loop while the client's connected
        if (client.available()) {             // if there's bytes to read from the client,
          char c = client.read();             // read a byte, then
          Serial.write(c);                    // print it out the serial monitor
          header += c;
          if (c == '\n') {                    // if the byte is a newline character
            // if the current line is blank, you got two newline characters in a row.
            // that's the end of the client HTTP request, so send a response:
            if (currentLine.length() == 0) {
              // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
              // and a content-type so the client knows what's coming, then a blank line:
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println("Connection: close");
              client.println();

              // turns the GPIOs on and off
              if (header.indexOf("GET /26/on") >= 0) {
                Serial.println("power state is not set to on");
                powerstate = true;
              } else if (header.indexOf("GET /26/off") >= 0) {
                Serial.println("powerstate is now set to off");
                powerstate = false;
              }// else if (header.indexOf("GET /27/on") >= 0) {
              //                Serial.println("GPIO 27 on");
              ////                output27State = "on";
              ////                digitalWrite(output27, HIGH);
              //              } else if (header.indexOf("GET /27/off") >= 0) {
              //                Serial.println("GPIO 27 off");
              ////                output27State = "off";
              ////                digitalWrite(output27, LOW);
              //              }

              // Display the HTML web page
              client.println("<!DOCTYPE html><html>");
              client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
              client.println("<link rel=\"icon\" href=\"data:,\">");
              // CSS to style the on/off buttons
              client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
              client.println(".button { background-color: #4CAF50; border: none; color: pink; padding: 16px 40px;");
              client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
              client.println(".button2 {background-color: #555555;}</style></head>");

              // Web Page Heading
              client.println("<body><h1>Toast Control:</h1>");

              // Display current state, and ON/OFF buttons for GPIO 26
              //client.println("<p>GPIO 26 - State " + output26State + "</p>");
              //+++client.println("<p>GPIO 26 - State " + powerstate + "</p>");
              // If the output26State is off, it displays the ON button
              if (powerstate == false) {
                client.println("<p><a href=\"/26/on\"><button class=\"button\">ON</button></a></p>");
              } else {
                client.println("<p><a href=\"/26/off\"><button class=\"button button2\">OFF</button></a></p>");
              }
              //sample portion to base new parts of code off of
              //              // Display current state, and ON/OFF buttons for GPIO 27
              //              client.println("<p>GPIO 27 - State " + output27State + "</p>");
              //              // If the output27State is off, it displays the ON button
              //              if (output27State == "off") {
              //                client.println("<p><a href=\"/27/on\"><button class=\"button\">ON</button></a></p>");
              //              } else {
              //                client.println("<p><a href=\"/27/off\"><button class=\"button button2\">OFF</button></a></p>");
              //              }
              client.println("</body></html>");

              // The HTTP response ends with another blank line
              client.println();
              // Break out of the while loop
              break;
            } else { // if you got a newline, then clear currentLine
              currentLine = "";
            }
          } else if (c != '\r') {  // if you got anything else but a carriage return character,
            currentLine += c;      // add it to the end of the currentLine
          }
        }
      }
      // Clear the header variable
      header = "";
      // Close the connection
      client.stop();
      Serial.println("Client disconnected.");
      Serial.println("");
    }
  }
}
void loop() {}
