#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#define TX 17
#define RX 16

struct DHTData
{
	char temperature[65];
	char humidity[65];
};
DHTData dhtData;

void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len)
{
	memcpy(&dhtData, incomingData, sizeof(dhtData));
	Serial.println();
	Serial.print("Bytes recebidos: ");
	Serial.println(len);
	Serial.print("Temperatura: ");
	Serial.println(dhtData.temperature);
	Serial.print("Umidade: ");
	Serial.println(dhtData.humidity);

	Serial2.print(dhtData.temperature);
	Serial2.print(",");
	Serial2.println(dhtData.humidity);
}

void setup()
{
	Serial.begin(115200);
	Serial2.begin(115200, SERIAL_8N1, RX, TX);

	Serial.print("MAC deste dispositivo: ");
	Serial.println(WiFi.macAddress());

	WiFi.mode(WIFI_STA);

	if (esp_now_init() != 0)
	{
		Serial.println("Erro ao inicializar o ESP-NOW");
		return;
	}

	esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
}

void loop()
{
}