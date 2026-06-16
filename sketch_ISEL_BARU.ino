//utk mqtt
#include <PubSubClient.h>

//utk fuzzy
#include <Fuzzy.h>

//utk oled
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//utk wifi
#include <ESP8266WiFi.h>
//utk send email
#include <ESP_Mail_Client.h>

//---------------------setting UTK inet/email
const char* ssid = "ICX";
const char* password = "madinah10000";

//---------------------setting UTK mqtt server
//const char* mqtt_server = "broker.mqtt-dashboard.com";
const char* mqtt_server = "broker.mqtt.cool";
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

// Email settings
const char* smtp_host = "smtp.gmail.com";
const int smtp_port = 465; 
const char* email_sender = "michelliafortune99@gmail.com";
const char* email_password = "luqp cxoy mzaa lhew"; 

const char* email_recipient = "michellia.delphi@gmail.com"; // receipient email

SMTPSession smtp;
ESP_Mail_Session session;
SMTP_Message message;

//#include <SimpleDHT.h>
//-----------------------setting OLED-------
#define SCREEN_WIDTH 128 // OLED display width in pixels
#define SCREEN_HEIGHT 32 // OLED display height in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin #
#define SCREEN_ADDRESS 0x3C 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//posisi pin sensor DHT11
//const int pinDHT11 = 16;
//SimpleDHT11 dht11(pinDHT11);

#include "DHT.h"

#define DHTPIN 2     // Digital pin connected to the DHT sensor

#define DHTTYPE DHT11   

DHT dht(DHTPIN, DHTTYPE);

//------setting pin buzzer & fan motor
//BUZZER Pins
const int BUZZER_PIN = 15;

//Motor Driver Pins
const int IN1 = 13;
const int IN2 = 12;
const int EN1 = 14;

int x_chose;

int gas_value=7;  // input gas_value
float h=7 ;  // utk input humidity
float t=7 ; // utk input temperature

String input_mode = "auto";

//---------setting FUZZY-------------
//Main Setup Fuzzy
// Create Fuzzy object
Fuzzy *fuzzy = new Fuzzy();

// Fuzzy INPUTS: Gas (MQ5), Temp, Humidity
FuzzySet *gasLow = new FuzzySet(0, 400, 400, 1000);     // Low gas
FuzzySet *gasMedium = new FuzzySet(750, 2500, 2500, 5000);  // Medium gas
FuzzySet *gasHigh = new FuzzySet(4000, 6500, 6500, 10000); // High gas

FuzzySet *tempNormal = new FuzzySet(0, 10, 10, 20);         // Normal temp
FuzzySet *tempElevated = new FuzzySet(15, 25, 25, 30);      // Elevated temp
FuzzySet *tempCritical = new FuzzySet(25, 40, 50, 50);      // Critical temp

FuzzySet *humDry = new FuzzySet(0, 0, 40, 60);           // Dry humidity
FuzzySet *humModerate = new FuzzySet(50, 60, 60, 80);      // Moderate humidity
FuzzySet *humHumid = new FuzzySet(70, 80, 80, 100);         // Humid

// Fuzzy OUTPUTS: Buzzer and Fan: converts the 0–100% fuzzy output to 0–255 PWM values 
FuzzySet *buzzerOff = new FuzzySet(0, 15, 15, 30);          
FuzzySet *buzzerLow = new FuzzySet(20, 35, 35, 70);        
FuzzySet *buzzerHigh = new FuzzySet(60, 80, 80, 100);     

FuzzySet *fanOff = new FuzzySet(0, 15, 15, 30);             
FuzzySet *fanSlow = new FuzzySet(20, 35, 35, 70);          
FuzzySet *fanFast = new FuzzySet(60, 80, 80, 100);        

void setup() {
  //Serial.begin(115200);
  Serial.begin(9600);
  Serial.println(F("DHTxx test!"));

  dht.begin();

  //utk input gas
  pinMode(A0, INPUT);

  //utk input pin motor fan
  pinMode(IN1, OUTPUT);  // output pin 
  pinMode(IN2, OUTPUT);  // output pin
  pinMode(EN1, OUTPUT);   //PWM Pin

  //utk input pin motor Buzzer
  pinMode(BUZZER_PIN, OUTPUT);   // Buzzer PWM Pin

  
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

 // setup putaran motor
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  // buka koneksi wifi-----------------------------------------------
        // Connect to WiFi
      WiFi.begin(ssid, password);
      Serial.print("Connecting to WiFi");
      while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(200);
      }
      Serial.println("\nConnected!");

      // Configure email
      //ESP_Mail_Session session;
      session.server.host_name = smtp_host;
      session.server.port = smtp_port;
      session.login.email = email_sender;
      session.login.password = email_password;
      session.login.user_domain = "";

      // Create the message
      //SMTP_Message message;
      message.sender.name = "ESP8266";
      message.sender.email = email_sender;
      message.subject = "Important Notification";
      message.addRecipient("Recipient", email_recipient);

      //-------------------------------------------------------

  // Set up Fuzzy Inputs and Outputs
  setupFuzzy();

  //pilihan, spy oled bisa berganti2 tampilan (0: utk hasil sensor, 1: utk hasil fuzzy logic)
  x_chose = 1;

  // setup for mqtt
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

}

void loop() {
  // put your main code here, to run repeatedly:
    // read without samples.
    // read gas
  //READ MQ5 (Gas)
if (input_mode == "auto") {
    gas_value = analogRead(A0);
    h = dht.readHumidity();
  // Read temperature as Celsius (the default)
    t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
      Serial.println(F("Failed to read from DHT sensor!"));
      return;
    }
}
/*
  int gas_value = analogRead(A0);
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
*/

  // Read temperature as Fahrenheit (isFahrenheit = true)
  //float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).

  /*
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  */

/*
  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);
*/

/*
  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.print(f);
  Serial.print(F("°F  Heat index: "));
  Serial.print(hic);
  Serial.print(F("°C "));
  Serial.print(hif);
  Serial.println(F("°F"));
*/

  //utk PWM motor
  //int Motor_Speed = map(Speed_Value, 0,1023, 0,255);
  //int Motor_Speed = 100;
  //analogWrite(EN1, Motor_Speed); //PWM Signal to control the speed of motor. (0 - 255)

  //utk PWM Buzzer
  //int Buzzer_Intensity = 0;
  //analogWrite(BUZZER_PIN, Buzzer_Intensity); //PWM Signal to control the speed of motor. (0 - 255)

 
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

/*
    if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    Serial.print("Read DHT11 failed, err="); Serial.print(SimpleDHTErrCode(err));
    Serial.print(","); Serial.println(SimpleDHTErrDuration(err)); delay(1000);
    return;
  }
*/

//-----------------------fuzzy operation------------------

  // Fuzzify inputs
  fuzzy->setInput(1, gas_value);
  fuzzy->setInput(2, t);
  fuzzy->setInput(3, h);

  // Run fuzzy inference
  fuzzy->fuzzify();

  // Defuzzify outputs
  float buzzerOutput = fuzzy->defuzzify(1);
  float fanOutput = fuzzy->defuzzify(2);

  // Actuate outputs (PWM signals)
  //analogWrite(BUZZER_PIN, map(buzzerOutput, 0, 100, 0, 255));
  analogWrite(EN1, map(fanOutput, 0, 100, 0, 255));
  // analogWrite(BUZZER_PIN, map(buzzerOutput, 0, 100, 0, 255));
  // Debugging output

  int fz_buzzer_container = map(buzzerOutput, 0, 100, 0, 255);
  int fz_fan_container = map(fanOutput, 0, 100, 0, 255);
  //---utk di email, jika nilai buzzer mulai naik lbh dr 60 (skitar 25% dr PWM)
  if (fz_buzzer_container > 60) 
  {
      // bunyikan buzzer ktika PWM diatas 60  
      analogWrite(BUZZER_PIN, map(buzzerOutput, 0, 100, 0, 255));
      // HTML content
      String htmlMsg = "<div style=\"color:#2f4468;\"><h1>Gas Alert!</h1><p>TThe Gas Has leaked, Pls secure the tubes immediately from ESP8266!</p></div>";
      message.html.content = htmlMsg.c_str();
      message.html.charSet = "utf-8";
      message.html.transfer_encoding = Content_Transfer_Encoding::enc_qp;
      
      // Text content (alternative for non-HTML clients)
      message.text.content = "Alert!! Gas Leak Detected\nThis is a plain text version.";
      message.text.charSet = "utf-8";
      message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

      // Connect to SMTP server
      if (!smtp.connect(&session)) {
        Serial.println("Connection error: " + smtp.errorReason());
        return;
      }

      // Send email
      if (!MailClient.sendMail(&smtp, &message)) {
        Serial.println("Error sending email: " + smtp.errorReason());
      } else {
        Serial.println("Email sent successfully!");
      }

  } else {
     analogWrite(BUZZER_PIN, 0);
  }

  /*

  Serial.print("Gas: "); Serial.print(gas_value);
  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.print(f);
  Serial.print(F("°F  Heat index: "));
  Serial.print("Gas(ppm): ");
  Serial.print(gas_value);
  */

  Serial.print(" | Gas: "); Serial.print(gas_value);
  Serial.print(" | Temp: "); Serial.print(t);
  Serial.print(" | Hum: "); Serial.print(h);
  Serial.print(" | Buzzer: "); Serial.print(buzzerOutput);
  Serial.print(" | Fan: "); Serial.print(fanOutput);
  Serial.print(" | Buzzer(pwm): "); Serial.print(map(buzzerOutput, 0, 100, 0, 255));
  Serial.print(" | Fan(pwm): "); Serial.println(map(fanOutput, 0, 100, 0, 255));

//---------------------------------------------------------
if (x_chose==0)
  {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0,0);

      display.print("Temperature:");
      //display.print((int)temperature);
      display.print(t);
      display.print("°C ");

      display.setCursor(0,10);
      display.print("Humidity:");
      display.print(h);
      display.print("%");

      //display.print(" %");

      display.setCursor(0,20);
      display.print("Gas(ppm):");
      display.print(gas_value);
      //display.print(" %");
    
      display.display();
      //Serial.print("Sample OK: ");
      //Serial.print((int)temperature); Serial.print(" *C, "); 
      //Serial.print((int)humidity); Serial.println(" H");

      // DHT11 sampling rate is 1HZ.      
      x_chose = 1;
     // delay(3500);

  } 
    else
  {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0,0);

      //int fz_buzzer_container = map(buzzerOutput, 0, 100, 0, 255);
      display.print("Buz|pwm:");
      //display.print((int)temperature);
      display.print(buzzerOutput);
      display.print("|");
      display.print(fz_buzzer_container);

      display.setCursor(0,10);
      display.print("Fan|pwm::");
      display.print(fanOutput);
      display.print("|");
      display.print(fz_fan_container);

      display.setCursor(0,20);
      display.print("Michellia DI Pravda");

      display.display();
      //Serial.print("Sample OK: ");
      //Serial.print((int)temperature); Serial.print(" *C, "); 
      //Serial.print((int)humidity); Serial.println(" H");
    
      //-------------
      // DHT11 sampling rate is 1HZ.
      
      x_chose = 0;
      //delay(3500);

  }

    //-----------------------------utk mqtt--------------------
      if (!client.connected()) {
        reconnect();
      }
      client.loop();
      //gas_value
      String response1 = "TEMP:" + String(t) + "C, HUM:" + String(h) + "%, GAS:" + String(gas_value);
      client.publish("outTopic1122", response1.c_str());
      //ini bisa dibuatkan image svg utk checking readyness dr service mqtt ESP8266
      //client.publish("outTopic1122", "inTopic1122 and inTopic1133--> Ready for Request");
      // client.publish("outTopic1133", "inTopic1133-Ready for Request");
      delay(5000);
    //-----------------------------utk mqtt--------------------

      /*
      unsigned long now = millis();
      if (now - lastMsg > 2000) {
        lastMsg = now;
        ++value;
        snprintf (msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
        Serial.print("Publish message: ");
        Serial.println(msg);
        client.publish("ich008009OUT", msg);
      }
      */
      //client.subscribe("req_ich99"); 
      //delay(3500);

       //-------------
       /*
      gas_value = 0;
      t = 0;
      h = 0;
      */
      buzzerOutput = 0;
      fanOutput = 0;
      

}

void callback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (unsigned int i = 0; i < length; i++) {
      message += (char)payload[i];
    }

    Serial.print("Message received on [");
    Serial.print(topic);
    Serial.print("]: ");
    Serial.println(message);

  //client.publish("outTopic1122", "XXX");

    if (String(topic) == "inTopic1122_xgas") {
        input_mode = "manual";
        gas_value = message.toInt();
    }

    if (String(topic) == "inTopic1122_xhumi") {
        input_mode = "manual";
        h = message.toFloat();
    }

    if (String(topic) == "inTopic1122_xtemp") {
        input_mode = "manual";
        t = message.toFloat();
    }

    if (String(topic) == "inTopic1122") {

      if (message == "auto") {
         input_mode = "auto";
      }

      if (message == "manual") {
         input_mode = "manual";
      }
      // digitalWrite(LED_PIN, HIGH);
      // Serial.println("LED turned ON");
     
    }

}

void reconnect() {
  
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic1122", "hello sensors");
   
      //client.publish("outTopic1133", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic1122_xgas"); // input nilai gas (manual)
      client.subscribe("inTopic1122_xhumi"); // input nilai humidity (manual)
      client.subscribe("inTopic1122_xtemp"); // input nilai temperatur (manual)
      client.subscribe("inTopic1122"); // input 'manual' or 'auto' (default:auto)
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 4 seconds");
      // Wait 5 seconds before retrying
      delay(4000);
    }
  }

}

// Configure fuzzy membership functions and rules
void setupFuzzy() {
  // Gas Input (MQ5)
  FuzzyInput *gas = new FuzzyInput(1);
  gas->addFuzzySet(gasLow);
  gas->addFuzzySet(gasMedium);
  gas->addFuzzySet(gasHigh);
  fuzzy->addFuzzyInput(gas);

  // Temperature Input (DHT11)
  FuzzyInput *temp = new FuzzyInput(2);
  temp->addFuzzySet(tempNormal);
  temp->addFuzzySet(tempElevated);
  temp->addFuzzySet(tempCritical);
  fuzzy->addFuzzyInput(temp);

  // Humidity Input (DHT11)
  FuzzyInput *hum = new FuzzyInput(3);
  hum->addFuzzySet(humDry);
  hum->addFuzzySet(humModerate);
  hum->addFuzzySet(humHumid);
  fuzzy->addFuzzyInput(hum);

  // Buzzer Output
  FuzzyOutput *buzzer = new FuzzyOutput(1);
  buzzer->addFuzzySet(buzzerOff);
  buzzer->addFuzzySet(buzzerLow);
  buzzer->addFuzzySet(buzzerHigh);
  fuzzy->addFuzzyOutput(buzzer);

  // Fan Output
  FuzzyOutput *fan = new FuzzyOutput(2);
  fan->addFuzzySet(fanOff);
  fan->addFuzzySet(fanSlow);
  fan->addFuzzySet(fanFast);
  fuzzy->addFuzzyOutput(fan);

  // Add rules (from your design)
  addRules();
}

void addRules() {
// GAS Group----------TEMP Group-----Humidity Group
// GAS LOW-----
  //Rule1: Gas(Low) + Temp(Normal) + Humidity(Dry) --> Buzzer(low) + Fan(slow)=================>  
//BE
//---------------------------------------------------------------------
  FuzzyRuleAntecedent *input_gasLow_tempNormal = new FuzzyRuleAntecedent();
  input_gasLow_tempNormal->joinWithAND(gasLow, tempNormal);

  FuzzyRuleAntecedent *input_gasLow_tempCritical = new FuzzyRuleAntecedent();
  input_gasLow_tempCritical->joinWithAND(gasLow, tempCritical);

  FuzzyRuleAntecedent *input_gasLow_tempElevated = new FuzzyRuleAntecedent();
  input_gasLow_tempElevated->joinWithAND(gasLow, tempElevated);

//---------------------------------------------------------------------
  FuzzyRuleAntecedent *input_gasMedium_tempNormal = new FuzzyRuleAntecedent();
  input_gasMedium_tempNormal->joinWithAND(gasMedium, tempNormal);

  FuzzyRuleAntecedent *input_gasMedium_tempElevated = new FuzzyRuleAntecedent();
  input_gasMedium_tempElevated->joinWithAND(gasMedium, tempElevated);

  FuzzyRuleAntecedent *input_gasMedium_tempCritical = new FuzzyRuleAntecedent();
  input_gasMedium_tempCritical->joinWithAND(gasMedium, tempCritical);

//---------------------------------------------------------------------
  FuzzyRuleAntecedent *input_gasHigh_tempNormal = new FuzzyRuleAntecedent();
  input_gasHigh_tempNormal->joinWithAND(gasHigh, tempNormal);

  FuzzyRuleAntecedent *input_gasHigh_tempElevated = new FuzzyRuleAntecedent();
  input_gasHigh_tempElevated->joinWithAND(gasHigh, tempElevated);

  FuzzyRuleAntecedent *input_gasHigh_tempCritical = new FuzzyRuleAntecedent();
  input_gasHigh_tempCritical->joinWithAND(gasHigh, tempCritical);

//---------------------------------------------------------------------
  FuzzyRuleAntecedent *input_humDry = new FuzzyRuleAntecedent();
  input_humDry->joinSingle(humDry);

  FuzzyRuleAntecedent *input_humModerate = new FuzzyRuleAntecedent();
  input_humModerate->joinSingle(humModerate);

  FuzzyRuleAntecedent *input_humHumid = new FuzzyRuleAntecedent();
  input_humHumid->joinSingle(humHumid);

  //Combine Type-------type1/AD(13)
  
//------------------------------------------------------------------------------------------------

  FuzzyRuleAntecedent *input_combine_gasLow_tempNormal_humDry = new FuzzyRuleAntecedent();
  input_combine_gasLow_tempNormal_humDry->joinWithAND(input_gasLow_tempNormal, input_humDry);

  FuzzyRuleAntecedent *input_combine_gasLow_tempNormal_humModerate = new FuzzyRuleAntecedent();
  input_combine_gasLow_tempNormal_humModerate->joinWithAND(input_gasLow_tempNormal, input_humModerate);

  FuzzyRuleAntecedent *input_combine_gasLow_tempNormal_humHumid = new FuzzyRuleAntecedent();
  input_combine_gasLow_tempNormal_humHumid->joinWithAND(input_gasLow_tempNormal, input_humHumid);

  FuzzyRuleAntecedent *input_buzzerOff_fanOff_1 = new FuzzyRuleAntecedent();
  input_buzzerOff_fanOff_1->joinWithOR(input_combine_gasLow_tempNormal_humDry, input_combine_gasLow_tempNormal_humModerate);

  FuzzyRuleAntecedent *input_buzzerOff_fanOff_2 = new FuzzyRuleAntecedent();
  input_buzzerOff_fanOff_2->joinWithOR(input_buzzerOff_fanOff_1, input_combine_gasLow_tempNormal_humHumid);

//------------------------------------------------------------------------------------------------

  FuzzyRuleAntecedent *input_combine_gasLow_tempElevated_humModerate = new FuzzyRuleAntecedent();
  input_combine_gasLow_tempElevated_humModerate->joinWithAND(input_gasLow_tempElevated, input_humModerate);

  FuzzyRuleAntecedent *input_combine_gasLow_tempElevated_humHumid = new FuzzyRuleAntecedent();
  input_combine_gasLow_tempElevated_humHumid->joinWithAND(input_gasLow_tempElevated, input_humHumid);

  FuzzyRuleAntecedent *input_buzzerOff_fanOff_3 = new FuzzyRuleAntecedent();
  input_buzzerOff_fanOff_3->joinWithOR(input_combine_gasLow_tempElevated_humModerate, input_combine_gasLow_tempElevated_humHumid);

  FuzzyRuleAntecedent *input_buzzerOff_fanOff_4 = new FuzzyRuleAntecedent();
  input_buzzerOff_fanOff_4->joinWithOR(input_buzzerOff_fanOff_3, input_buzzerOff_fanOff_2);

  //------------------------------------------------------------------------------------------------

  FuzzyRuleAntecedent *input_combine_gasLow_tempCritical_humModerate = new FuzzyRuleAntecedent();
  input_combine_gasLow_tempCritical_humModerate->joinWithAND(input_gasLow_tempCritical, input_humModerate);

  FuzzyRuleAntecedent *input_combine_gasLow_tempCritical_humHumid = new FuzzyRuleAntecedent();
  input_combine_gasLow_tempCritical_humHumid->joinWithAND(input_gasLow_tempCritical, input_humHumid);

  FuzzyRuleAntecedent *input_buzzerOff_fanOff_5 = new FuzzyRuleAntecedent();
  input_buzzerOff_fanOff_5->joinWithOR(input_combine_gasLow_tempCritical_humModerate, input_combine_gasLow_tempCritical_humHumid);

  FuzzyRuleAntecedent *input_buzzerOff_fanOff_6 = new FuzzyRuleAntecedent();
  input_buzzerOff_fanOff_6->joinWithOR(input_buzzerOff_fanOff_5, input_buzzerOff_fanOff_4);

//------------------------------------------------------------------------------------------------

  FuzzyRuleAntecedent *input_combine_gasMedium_tempNormal_humModerate = new FuzzyRuleAntecedent();
  input_combine_gasMedium_tempNormal_humModerate->joinWithAND(input_gasMedium_tempNormal, input_humModerate);

  FuzzyRuleAntecedent *input_combine_gasMedium_tempNormal_humHumid = new FuzzyRuleAntecedent();
  input_combine_gasMedium_tempNormal_humHumid->joinWithAND(input_gasMedium_tempNormal, input_humHumid);

  FuzzyRuleAntecedent *input_buzzerOff_fanOff_7 = new FuzzyRuleAntecedent();
  input_buzzerOff_fanOff_7->joinWithOR(input_combine_gasMedium_tempNormal_humModerate, input_combine_gasMedium_tempNormal_humHumid);

  FuzzyRuleAntecedent *input_buzzerOff_fanOff_8 = new FuzzyRuleAntecedent();
  input_buzzerOff_fanOff_8->joinWithOR(input_buzzerOff_fanOff_7, input_buzzerOff_fanOff_6);

//------------------------------------------------------------------------------------------------

  FuzzyRuleAntecedent *input_combine_gasMedium_tempElevated_humModerate = new FuzzyRuleAntecedent();
  input_combine_gasMedium_tempElevated_humModerate->joinWithAND(input_gasMedium_tempNormal, input_humModerate);

  FuzzyRuleAntecedent *input_combine_gasMedium_tempElevated_humHumid = new FuzzyRuleAntecedent();
  input_combine_gasMedium_tempElevated_humHumid->joinWithAND(input_gasMedium_tempNormal, input_humHumid);

  FuzzyRuleAntecedent *input_buzzerOff_fanOff_9 = new FuzzyRuleAntecedent();
  input_buzzerOff_fanOff_9->joinWithOR(input_combine_gasMedium_tempElevated_humModerate, input_combine_gasMedium_tempElevated_humHumid);

  FuzzyRuleAntecedent *input_buzzerOff_fanOff_10 = new FuzzyRuleAntecedent();
  input_buzzerOff_fanOff_10->joinWithOR(input_buzzerOff_fanOff_9, input_buzzerOff_fanOff_8);

//------------------------------------------------------------------------------------------------

  FuzzyRuleAntecedent *input_combine_gasHigh_tempNormal_humHumid = new FuzzyRuleAntecedent();
  input_combine_gasHigh_tempNormal_humHumid->joinWithAND(input_gasMedium_tempNormal, input_humHumid);

  FuzzyRuleAntecedent *input_buzzerOff_fanOff_11 = new FuzzyRuleAntecedent();
  input_buzzerOff_fanOff_11->joinWithOR(input_combine_gasHigh_tempNormal_humHumid, input_buzzerOff_fanOff_10);

  FuzzyRuleAntecedent *input_combine_gasHigh_tempElevated_humHumid = new FuzzyRuleAntecedent();
  input_combine_gasHigh_tempElevated_humHumid->joinWithAND(input_gasMedium_tempElevated, input_humHumid);

  FuzzyRuleAntecedent *input_buzzerOff_fanOff_12 = new FuzzyRuleAntecedent();
  input_buzzerOff_fanOff_12->joinWithOR(input_combine_gasHigh_tempElevated_humHumid, input_buzzerOff_fanOff_11);
//------------------------------------------------------------------------------------------------
  FuzzyRuleConsequent *output_type1 = new FuzzyRuleConsequent();
  output_type1->addOutput(buzzerOff);
  output_type1->addOutput(fanOff);

  FuzzyRule *fuzzyRule1 = new FuzzyRule(1, input_buzzerOff_fanOff_12, output_type1);
  fuzzy->addFuzzyRule(fuzzyRule1);
//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------

//Combine Type-------type1/BE(9)
//------------------------------------------------------------------------------------------------

  FuzzyRuleAntecedent *input_combine_gasLow_tempElevated_humDry = new FuzzyRuleAntecedent();
  input_combine_gasLow_tempElevated_humDry->joinWithAND(input_gasLow_tempElevated, input_humDry);

  FuzzyRuleAntecedent *input_combine_gasLow_tempCritical_humDry = new FuzzyRuleAntecedent();
  input_combine_gasLow_tempCritical_humDry->joinWithAND(input_gasLow_tempCritical, input_humDry);

  FuzzyRuleAntecedent *input_buzzerLow_fanSlow_1 = new FuzzyRuleAntecedent();
  input_buzzerLow_fanSlow_1->joinWithOR(input_combine_gasLow_tempElevated_humDry, input_combine_gasLow_tempCritical_humDry);

//------------------------------------------------------------------------------------------------

  FuzzyRuleAntecedent *input_combine_gasMedium_tempNormal_humDry = new FuzzyRuleAntecedent();
  input_combine_gasMedium_tempNormal_humDry->joinWithAND(input_gasMedium_tempNormal, input_humDry);

  FuzzyRuleAntecedent *input_combine_gasMedium_tempElevated_humDry = new FuzzyRuleAntecedent();
  input_combine_gasMedium_tempElevated_humDry->joinWithAND(input_gasMedium_tempElevated, input_humDry);

  FuzzyRuleAntecedent *input_buzzerLow_fanSlow_2 = new FuzzyRuleAntecedent();
  input_buzzerLow_fanSlow_2->joinWithOR(input_combine_gasMedium_tempNormal_humDry, input_combine_gasMedium_tempElevated_humDry);

  FuzzyRuleAntecedent *input_buzzerLow_fanSlow_3 = new FuzzyRuleAntecedent();
  input_buzzerLow_fanSlow_3->joinWithOR(input_buzzerLow_fanSlow_2, input_buzzerLow_fanSlow_1);

//---------------------

  FuzzyRuleAntecedent *input_combine_gasHigh_tempNormal_humModerate = new FuzzyRuleAntecedent();
  input_combine_gasHigh_tempNormal_humModerate->joinWithAND(input_gasHigh_tempNormal, input_humModerate);

  FuzzyRuleAntecedent *input_combine_gasHigh_tempElevated_humModerate = new FuzzyRuleAntecedent();
  input_combine_gasHigh_tempElevated_humModerate->joinWithAND(input_gasHigh_tempElevated, input_humModerate);

  FuzzyRuleAntecedent *input_buzzerLow_fanSlow_4 = new FuzzyRuleAntecedent();
  input_buzzerLow_fanSlow_4->joinWithOR(input_combine_gasHigh_tempNormal_humModerate, input_combine_gasHigh_tempElevated_humModerate);

  FuzzyRuleAntecedent *input_buzzerLow_fanSlow_5 = new FuzzyRuleAntecedent();
  input_buzzerLow_fanSlow_5->joinWithOR(input_buzzerLow_fanSlow_4, input_buzzerLow_fanSlow_3);

//---------------------

  FuzzyRuleAntecedent *input_combine_gasHigh_tempCritical_humHumid = new FuzzyRuleAntecedent();
  input_combine_gasHigh_tempCritical_humHumid->joinWithAND(input_gasHigh_tempCritical, input_humHumid);

  FuzzyRuleAntecedent *input_buzzerLow_fanSlow_6 = new FuzzyRuleAntecedent();
  input_buzzerLow_fanSlow_6->joinWithOR(input_combine_gasHigh_tempCritical_humHumid, input_buzzerLow_fanSlow_5);

//------------------------------------------------------------------------------------------------
  FuzzyRuleConsequent *output_type2 = new FuzzyRuleConsequent();
  output_type2->addOutput(buzzerLow);
  output_type2->addOutput(fanSlow);

  FuzzyRule *fuzzyRule2 = new FuzzyRule(2, input_buzzerLow_fanSlow_6, output_type2);
  fuzzy->addFuzzyRule(fuzzyRule2);

//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------

  //Combine Type-------type1/CF(5)

  FuzzyRuleAntecedent *input_combine_gasMedium_tempCritical_humDry = new FuzzyRuleAntecedent();
  input_combine_gasMedium_tempCritical_humDry->joinWithAND(input_gasMedium_tempCritical, input_humDry);

  FuzzyRuleAntecedent *input_combine_gasHigh_tempNormal_humDry = new FuzzyRuleAntecedent();
  input_combine_gasHigh_tempNormal_humDry->joinWithAND(input_gasHigh_tempNormal, input_humDry);

  FuzzyRuleAntecedent *input_buzzerHigh_fanFast_1 = new FuzzyRuleAntecedent();
  input_buzzerHigh_fanFast_1->joinWithOR(input_combine_gasMedium_tempCritical_humDry, input_combine_gasHigh_tempNormal_humDry);

//---------------------
  FuzzyRuleAntecedent *input_combine_gasHigh_tempElevated_humDry = new FuzzyRuleAntecedent();
  input_combine_gasHigh_tempElevated_humDry->joinWithAND(input_gasHigh_tempElevated, input_humDry);

  FuzzyRuleAntecedent *input_combine_gasHigh_tempCritical_humDry = new FuzzyRuleAntecedent();
  input_combine_gasHigh_tempCritical_humDry->joinWithAND(input_gasHigh_tempCritical, input_humDry);

  FuzzyRuleAntecedent *input_buzzerHigh_fanFast_2 = new FuzzyRuleAntecedent();
  input_buzzerHigh_fanFast_2->joinWithOR(input_combine_gasHigh_tempCritical_humDry, input_combine_gasHigh_tempElevated_humDry);

  FuzzyRuleAntecedent *input_buzzerHigh_fanFast_3 = new FuzzyRuleAntecedent();
  input_buzzerHigh_fanFast_3->joinWithOR(input_buzzerHigh_fanFast_2, input_buzzerHigh_fanFast_1);

//---------------------

  FuzzyRuleAntecedent *input_combine_gasHigh_tempCritical_humModerate = new FuzzyRuleAntecedent();
  input_combine_gasHigh_tempCritical_humModerate->joinWithAND(input_gasHigh_tempCritical, input_humModerate);

  FuzzyRuleAntecedent *input_buzzerHigh_fanFast_4 = new FuzzyRuleAntecedent();
  input_buzzerHigh_fanFast_4->joinWithOR(input_combine_gasHigh_tempCritical_humModerate, input_buzzerHigh_fanFast_3);

//---------------------

  FuzzyRuleConsequent *output_type3 = new FuzzyRuleConsequent();
  output_type2->addOutput(buzzerHigh);
  output_type2->addOutput(fanFast);

  FuzzyRule *fuzzyRule3 = new FuzzyRule(3, input_buzzerHigh_fanFast_4, output_type3);
  fuzzy->addFuzzyRule(fuzzyRule3);

//------------------------------------------------------------------------------------------------
  }


 