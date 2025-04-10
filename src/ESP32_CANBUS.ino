
// requires https://github.com/collin80/esp32_can and https://github.com/collin80/can_common

#include <esp32_can.h>
#include <esp_now.h>
#include <WiFi.h>
#include <cmath>
#include <iostream>

// uint8_t receiver_mac[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };  // Your screen MAC Address

// TaskHandle_t send_speed_handle = NULL;

// int send_speed_interval = 100;  // 10 sends per second

// struct can_struct {
//   uint8_t speed_mph;
//   uint16_t speed_rpm;
// };

// can_struct CANMessage;

// void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
//   Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Send Success" : "Send Failed");
// }

// void send_speed(void *parameter) {
//   while (true) {
//     esp_err_t result = esp_now_send(receiver_mac, (uint8_t *)&CANMessage, sizeof(CANMessage));

//     if (result == ESP_OK) {
//       Serial.println("Sent via ESP-NOW");
//     } else {
//       Serial.print("Error sending message: ");
//       Serial.println(result);
//     }

//     vTaskDelay(send_speed_interval / portTICK_PERIOD_MS);
//   }
// }
static unsigned long lastPrintTime = 0; 
static unsigned long lastFristRunTime = 0; 
uint16_t global_rpm = 0;
uint16_t global_map = 0;
uint16_t global_tps = 0;
uint16_t global_fuel_pressure = 0;
float global_afr = 0.0;
uint16_t global_vss = 0;
float global_voltage = 0.0;
uint16_t global_clt = 0;
uint16_t global_iat = 0;
boolean first_run = true;
void setup()
{
	Serial.begin(115200);
	Serial.println("Serial ON!");

	// Set device as a Wi-Fi Station
	// WiFi.mode(WIFI_STA);
	// Serial.println("Wifi initialized");
	// if (esp_now_init() != ESP_OK) {
	//   Serial.println("ESP-NOW Init Failed");
	//   return;
	// }
	// esp_now_register_send_cb(OnDataSent);

	// Serial.println("ESP32-NOW initialized");
	CAN0.setCANPins(GPIO_NUM_16, GPIO_NUM_17); // see important note above
	// CAN0.begin(500000);						   // 500Kbps
	if (CAN0.begin(500000))
	{
		Serial.println("CAN driver initialized successfully.");
	}
	else
	{
		Serial.println("Failed to initialize CAN driver.");
	}
	delay(5000);

	// Filter untuk ID tertentu berdasarkan dokumen Haltech
	CAN0.watchFor(0x360); // RPM, MAP, TPS
	CAN0.watchFor(0x361); // Fuel Pressure
	CAN0.watchFor(0x368); // AFR 01
	CAN0.watchFor(0x370); // VSS
	CAN0.watchFor(0x372); // Voltage
	CAN0.watchFor(0x3E0); // CLT
}

void loop()
{
	unsigned long currentTime = millis(); // Waktu saat ini

	if (CAN0.available())
	{ // Periksa apakah ada pesan di buffer
		if (!first_run)
		{
			CAN_FRAME can_message;
			if (CAN0.read(can_message))
			{
				// Proses data berdasarkan ID
				switch (can_message.id)
				{
				case 0x360:
				{																			   // RPM, MAP, TPS
					global_rpm = (can_message.data.byte[0] << 8) | can_message.data.byte[1];   // Byte 0-1
					uint16_t map = (can_message.data.byte[2] << 8) | can_message.data.byte[3]; // Byte 2-3
					uint16_t tps = (can_message.data.byte[4] << 8) | can_message.data.byte[5]; // Byte 4-5
					global_map = map / 10.0;												   // Konversi ke kPa
					global_tps = tps / 10.0;												   // Konversi ke kPa
					break;
				}
				case 0x361:
				{																						 // Fuel Pressure
					uint16_t fuel_pressure = (can_message.data.byte[0] << 8) | can_message.data.byte[1]; // Byte 0-1
					global_fuel_pressure = fuel_pressure / 10 - 101.3;
					break;
				}
				case 0x368:
				{																				   // AFR 01
					uint16_t afr_raw = (can_message.data.byte[0] << 8) | can_message.data.byte[1]; // Byte 0-1
					float lambda = afr_raw / 1000.0;
					global_afr = lambda * 14.7;
					break;
				}
				case 0x370:
				{																			   // VSS
					uint16_t vss = (can_message.data.byte[0] << 8) | can_message.data.byte[1]; // Byte 0-1
					global_vss = vss / 10.0;												   // Konversi ke km/h
					break;
				}
				case 0x372:
				{																				   // Voltage
					uint16_t voltage = (can_message.data.byte[0] << 8) | can_message.data.byte[1]; // Byte 0-1
					global_voltage = voltage / 10.0;											   // Konversi ke volt
					break;
				}
				case 0x3E0:
				{																				   // CLT
					uint16_t clt_raw = (can_message.data.byte[0] << 8) | can_message.data.byte[1]; // Byte 0-1
					uint16_t iat_raw = (can_message.data.byte[2] << 8) | can_message.data.byte[3]; // Byte 0-1
					float clt_k = clt_raw / 10.0;
					float iat_k = iat_raw / 10.0;

					global_clt = clt_k - 273.15;
					global_iat = iat_k - 273.15;
					break;
				}
				default:
					break;
				}
			}
		}
		else
		{
			if (currentTime - lastFristRunTime >= 5000)

				first_run = false;
		}
	}
	if (currentTime - lastPrintTime >= 1000)
	{ // Interval 1000ms
		Serial.print("RPM: ");
		Serial.print(global_rpm);
		Serial.print(" MAP: ");
		Serial.print(global_map);
		Serial.print(" kPa TPS: ");
		Serial.print(global_tps);
		Serial.print(" % Fuel Pressure: ");
		Serial.print(global_fuel_pressure);
		Serial.print(" kPa AFR: ");
		Serial.print(global_afr, 2);
		Serial.print(" VSS: ");
		Serial.print(global_vss);
		Serial.print(" km/h Voltage: ");
		Serial.print(global_voltage, 2);
		Serial.print(" V CLT: ");
		Serial.print(global_clt);
		Serial.print(" °C IAT: ");
		Serial.print(global_iat);
		Serial.println(" °C");

		lastPrintTime = currentTime; // Update waktu terakhir print
	}
	yield();   // Beri kesempatan untuk tugas lain
	delay(20); // 50Hz = 1000ms / 50 = 20ms
}