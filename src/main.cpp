#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

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
	Serial.print("String: ");
	Serial.println(dhtData.temperature);
	Serial.print("String: ");
	Serial.println(dhtData.temperature);
}

void setup()
{
	Serial.begin(115200);

	Serial.print("MAC deste dispositivo: ");
	Serial.println(WiFi.macAddress());

	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);

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
