#include <ESP8266WiFi.h>
#include "bme280.h"
#include <Wire.h>
#include "ssd1306.h"
#include <stdio.h>
#include "sgp40.h"


// set any useful name and password here for productive use.
const char *ssid = "electronstogo";
const char *password = "anypassword";




void setup()
{
	delay(10);

	// init arduino i2c framework class.
  Wire.begin();
}


void loop()
{
	// some utility helping variables.
	unsigned int loop_counter = 0;
	char char_buffer[20];
	String string_buffer;
	
	
	// value that keeps if already an initial connection was done.
	bool do_initial_connect = true;
	// value that keeps if the client is connected to the AP server.
	bool connected = false;
	// value counts time after connection attempt. when it is 0 there was no recent connection try. 
	unsigned char connection_attempt_timer = 0;


	// Use WiFiClient class to create TCP connections
	WiFiClient client;
  // this is the default AP server IP.
	const char* host = "192.168.4.1";
  // http port
	const int httpPort = 80;

  
	// SGP40 sensor class variable to read SGP40 measurements.
	SGP40Sensor sgp40_sensor;

	// Initiate a bme280 sensor class variable.
	BME280Sensor bme280_sensor;

	// Initiate a SSD1306 OLED display class variable.
	SSD1306 ssd1306(0x3C);


  

	while(true)
	{
		ssd1306.clear_oled_buffer();
		
		
		// print ID on OLED display
		string_buffer = "#1  ";
		if(connected)
		{
			string_buffer += "ACTIVE";
		}
		else
		{
			string_buffer += "DISCON";
		}
		
		string_buffer.toCharArray(char_buffer, 20);
		ssd1306.draw_string(0, 55, char_buffer);


		// get measurements from BME280 sensor.
		int32_t temperature;
		uint32_t pressure, humidity;
		bme280_sensor.do_humidity_temperature_pressure_measurement(&temperature, &pressure, &humidity);


		// print BME280 measurements on OLED display.
		string_buffer = String((float)humidity / 1000.0, 1);
		string_buffer += "% ";
		string_buffer += String((float)temperature / 100.0, 1);
		string_buffer += "$C"; 
		string_buffer.toCharArray(char_buffer, 20);
		ssd1306.draw_string(0, 15, char_buffer);




		// get the VOC index from SGP40.
		unsigned int voc_index;
		if(!sgp40_sensor.get_voc_index((float)temperature / 100.0, (float)humidity / 1000.0, &voc_index))
		{
			// write any standard number to recognize error for OLED output.
			voc_index = 1234;
		}


		// print measured value index on the OLED display.
		string_buffer = "VOC: ";
		
		if(voc_index == 1234)
		{
			string_buffer += "ERR";
		}
		else
		{
			string_buffer += String(voc_index);
		}
		
		string_buffer.toCharArray(char_buffer, 20);
		ssd1306.draw_string(0, 35, char_buffer);
		
		ssd1306.flush_oled_buffer();


		
		
		// handle wifi connection with access point server.
		// if the connection attempt timer is > 0, we still wait for a conntection to succeed. 
		if(connection_attempt_timer > 0)
		{
			if(WiFi.status() == WL_CONNECTED)
			{
				connection_attempt_timer = 0;
				connected = true;
			}
			else
			{
				connection_attempt_timer++;
			}
			
			// after a while without conntection success we give up here.
			if(connection_attempt_timer > 25)
			{
				connection_attempt_timer = 0;
				WiFi.persistent(true);
				connected = false;
			}
			
			
		} // if there is no active connection to the AP, we try to connect after specific time intervals.
		else if(!(loop_counter % 180) && WiFi.status() != WL_CONNECTED)
		{
			if(do_initial_connect)
			{
				// Explicitly set the ESP8266 to be a WiFi-client
				WiFi.mode(WIFI_STA);
				WiFi.begin(ssid, password);
				do_initial_connect = false;
			}
			else
			{
				WiFi.reconnect();
			}
			
			connection_attempt_timer = 1;
		}


		// send data
		if(!(loop_counter % 5) && client.connect(host, httpPort))
		{
			String url = "/data/";
			url += "?ID=1";
			url += "&temperature=";
			url += String(temperature/100);
			url += "&humidity=";
			url += String(humidity/1000);
			url += "&pressure=";
			url += String(pressure);
			url += "&voc_index=";
			url += String(voc_index);

			// This will send the request to the server
			client.print(String("GET ") + url + " HTTP/1.1\r\n" +
			"Host: " + host + "\r\n" +
			"Connection: close\r\n\r\n");

			client.stop();
		}


		delay(1000);
		loop_counter++;
	}
}
