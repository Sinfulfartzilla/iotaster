#include <Servo.h>
#include <WiFi.h>

typedef struct node
{
  int timez;
  struct node* link;
} STACK_NODE;
typedef struct
{
  int count;
  struct node* top;
} STACK;


Servo vexmotor;
TaskHandle_t Task1;
TaskHandle_t Task2;
bool powerstate = false;  //this is whether or not the toaster is on or off
int pos = 0;  //this is used for the vex mc29 motor controller value

int usetime;
bool remember = false;

// Replace with your network credentials
const char* ssid     = "Pretty Fly for a Wi-Fi";
const char* password = "livelycartoon963";

int timerthing;
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


bool push(STACK* pStack, int time2)
{
  STACK_NODE* pNew;
  bool success;

  pNew = (STACK_NODE*)malloc(sizeof (STACK_NODE));
  if (!pNew)
    success = false;
  else
  {
    pNew->timez = time2;

    success = true;
  }
  return success;
}

int pop(STACK* pStack)
{
  STACK_NODE* pDlt;
  bool success;
  printf("\n");
  if (pStack->top)
  {
    success = true;
    usetime = pStack->top->timez;
    pDlt = pStack->top;
    pStack->top = (pStack->top)->link;
    pStack->count--;
    free (pDlt);
  }
  else
    success = false;
  return usetime;
}



//Task1code: toaster controller
void Task1code( void * pvParameters ) {
  STACK* pStack;
  pStack = (STACK*)malloc(sizeof(STACK));
  if (!pStack){
    printf("error allocating stack"); 
    exit(101);
  }
  pStack->top = NULL;
  pStack->count = 0;

  bool updatedata;


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
      if (remember == false) {
        usetime += 1;
      } else {
        usetime -= 1;
      }

      if (usetime == -1) {
        powerstate = false;
      }

      Serial.println(usetime);
      updatedata = false;
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
      if (updatedata == true)
      {
        push(pStack, usetime);
      }
      Serial.println(usetime);
      //Serial.println("this is what is stored in the stack");
      //Serial.println(pop(pStack));


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
              if (header.indexOf("GET /toast/on") >= 0) {
                Serial.println("power state is not set to on");
                powerstate = true;
              } else if (header.indexOf("GET /toast/off") >= 0) {
                Serial.println("powerstate is now set to off");
                powerstate = false;
              }  else if (header.indexOf("GET /replay/on") >= 0) {
                remember = true;
              } else if (header.indexOf("GET /replay/off") >= 0) {
                remember = false;
              }

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
              if (powerstate == false) {
                client.println("<p><a href=\"/toast/on\"><button class=\"button button2\">OFF</button></a></p>");
              } else {
                client.println("<p><a href=\"/toast/off\"><button class=\"button \">ON</button></a></p>");
              }
              if (powerstate == false) {
                client.println("<p><a href=\"/replay/on\"><button class=\"button\">repeat</button></a></p>");
              } else {
                client.println("<p><a href=\"/replay/off\"><button class=\"button button2\">STOP!</button></a></p>");
              }
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
