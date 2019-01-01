// This #include statement was automatically added by the Particle IDE.
#include <Adafruit_DHT.h>

// This #include statement was automatically added by the Particle IDE.
#include <ThingSpeak.h>

// replace by your product ID
PRODUCT_ID(7790);
// each time you upload to the console
PRODUCT_VERSION(10);

// Define system mode and multithreading allowed
SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);

// Set DHT parameters
#define DHTTYPE DHT22
#define DHT_5V_PIN D1
#define DHT_SENSOR_PIN D2
#define DHT_GROUND_PIN D4

// Define variables
int led = D4;                   // LED power
float rainfall = 0;             // Rainfall total per period measured, reset on publish
int temperature;                // Temperature (C)
int humidity;                   // Humidity (%)
int rain_sensor = D3;           // Set rain gauge input to pin D3
int rain_power = D6;            // Set rain gauge power to pin D6
int current_charge;             // Current battery charge level
int min_op_charge = 20;         // Minimum "allowable" charge before going into DEEP_SLEEP mode for one hour
float current_voltage;          // Current battery voltage
int normal = 1;                 // Normal operating mode indication
int safe = 2;                   // Low power mode operating indication
int ota = 3;                    // Weekly OTA mode operating indication
int safe_time = 3;              // Sets low power mode time in hours    

// Power management
void charge_manager();

// Start up instructions
void startup_procedure();

// Runs the main program
void program();

// Pause for updates and external instuctions
void listen_and_learn();

// Cellular and cloud connection function
void make_connection();

// Sensor reads
void sensor_package();

// Publish to Particle cloud
void particle_package();

// Publish to Thingspeak cloud
void thingspeak_package();

// Safe mode
void safe_mode();

// DHT sensor
DHT dht(DHT_SENSOR_PIN, DHTTYPE);

/* Thingspeak */
TCPClient client;
unsigned long myChannelNumber = 543729;
const char * myWriteAPIKey = "xxxxxxxxxxxxxx";

void setup() {

// Define pins for the DHT22 sensor
pinMode(DHT_5V_PIN, OUTPUT);
pinMode(DHT_GROUND_PIN, OUTPUT);
digitalWrite(DHT_GROUND_PIN, LOW);

// Give power to rainsensor
digitalWrite(D6, HIGH);

// Makes connection and publishes current status at startup
    startup_procedure();
}

// Make initial connection
void startup_procedure()
{
    digitalWrite(D4, HIGH);
    FuelGauge fuel;
    current_charge = fuel.getSoC();
    make_connection();
    Particle.publish("boot", String("Device boot,") + " " + String(current_charge) + String("% battery charge."));
        sensor_package();
        particle_package();
        thingspeak_package();
    delay(5000);
    Cellular.off();
}

// Manages power levels
void charge_manager()
{
    FuelGauge fuel;
    current_charge = fuel.getSoC();
    current_voltage = fuel.getVCell();

    if(current_charge < min_op_charge)
    {
        safe_mode();
    }
}

//Safe mode
void safe_mode()
{
    Particle.publish("safe mode", String("Entering safe mode for 3 hours.") + " " + String(current_charge) + "% battery remaining.");
    
    ThingSpeak.begin(client);
            ThingSpeak.setField(1, (float)temperature);
            ThingSpeak.setField(2, (float)humidity);
            ThingSpeak.setField(3, (float)rainfall);
            ThingSpeak.setField(4, (float)current_charge);
            ThingSpeak.setField(5, (float)current_voltage);
            ThingSpeak.setField(6, (float)safe);
    ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    
    delay(5000);
    
    System.sleep(SLEEP_MODE_DEEP, 60 * 60 * safe_time);
}

// Executes primary functions
void program()
{
    if (Time.minute() == 0 && current_charge >= min_op_charge)
    {
        make_connection();
        sensor_package();
        thingspeak_package();
        particle_package();
        Cellular.off();
        delay(30000);
    }
}

// Set an extended connection every thursday at 13:40UTC to receive updates
void listen_and_learn()
{
    if (Time.weekday() == 5 && Time.hour() == 21 && Time.minute() == 40 && current_charge >= min_op_charge)
    {
        make_connection();
        Particle.publish("check in", String("Checking in for 10 minutes."));

        ThingSpeak.begin(client);
            ThingSpeak.setField(1, (float)temperature);
            ThingSpeak.setField(2, (float)humidity);
            ThingSpeak.setField(3, (float)rainfall);
            ThingSpeak.setField(4, (float)current_charge);
            ThingSpeak.setField(5, (float)current_voltage);
            ThingSpeak.setField(6, (float)ota);

        delay(600000);

        Particle.publish("goodbye", String("See you next week!"));

        Cellular.off();
        
        // **Not sure why this is here. Might be able to remove after testing.
        delay(30000);
    }
}

// Reads rain gauge
float get_rainfall()
{
    if (digitalRead(rain_sensor) == HIGH)
          {
          rainfall = rainfall + 0.8;
          }
    delay(200);

    return rainfall;

    delay(1000);
}

// Make connection to cellular network, then Particle.io cloud
void make_connection()
{
    Cellular.on();
    Cellular.connect();
    waitUntil(Cellular.ready);
    Particle.process();
    Particle.connect();
    waitUntil(Particle.connected);
}

// Reads Temp/Humidity sensor
void sensor_package()
{
    // Give power to the DHT22 sensor
    digitalWrite(DHT_5V_PIN, HIGH);

    // Wait for the sensor to stabilize
    delay(1000);
    
    // Start DHT22 sensor
    dht.begin();
    
    // Humidity measurement
    temperature = dht.getTempCelcius();
    
    // Humidity measurement
    humidity = dht.getHumidity();
    
    // Power down the DHT22 sensor
    digitalWrite(DHT_5V_PIN, LOW);
}

// Publishes data to Particle.io cloud
void particle_package()
{
    Particle.publish("data package", String(temperature) + "," + String(humidity) + "," + String(rainfall) + "," + String(current_charge) + "," + String(current_voltage));
    
    // Allows time for transmission to complete
    delay(5000);
    
    // Resets rainfall count
    rainfall = 0;
}

// Publishes data to Thingspeak.com cloud
void thingspeak_package()
{
    ThingSpeak.begin(client);
    
    // **Might not be necessary. Possibly remove after testing.
    delay(1000);

    ThingSpeak.setField(1, (float)temperature);
    ThingSpeak.setField(2, (float)humidity);
    ThingSpeak.setField(3, (float)rainfall);
    ThingSpeak.setField(4, (float)current_charge);
    ThingSpeak.setField(5, (float)current_voltage);
    ThingSpeak.setField(6, (float)normal);
    
    ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    
    // Allows time for transmission to complete
    delay(5000);
}

void loop()
{
    // Check battery status
    charge_manager();
    
    // Weekly check in for updates
    listen_and_learn();
    
    // Runs main program package
    program();
    
    // Runs raingauge sensor program
    rainfall = rainfall + get_rainfall();
}