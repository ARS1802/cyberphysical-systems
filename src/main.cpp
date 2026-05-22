#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

uint8_t broadcastAddress[] = {0xEC, 0x64, 0xC9, 0x86, 0x1D, 0xE1};

struct DHTData
{
	char temperature[65];
	char humidity[65];
};
DHTData dhtData;

bool aceso = true;
void piscar()
{
	aceso = !aceso;
	digitalWrite(LED_BUILTIN, aceso);
}

void onDataSend(const uint8_t *mac_addr, esp_now_send_status_t status)
{
	Serial.print("Status do envio: ");

	if (status == ESP_NOW_SEND_SUCCESS)
	{
		Serial.println("OK");
	}
	else
	{
		Serial.println("Falha no envio");
	}
}

void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len)
{
	memcpy(&dhtData, incomingData, sizeof(dhtData));
	if (sizeof(dhtData) > 0)
	{
		piscar();
	}
	Serial.println();
	Serial.print("Bytes recebidos: ");
	Serial.println(len);
	Serial.print("String: ");
	Serial.println(dhtData.temperature);
	Serial.print("String: ");
	Serial.println(dhtData.temperature);

	esp_err_t result = esp_now_send(
		 broadcastAddress,
		 (uint8_t *)&dhtData,
		 sizeof(dhtData));

	if (result == ESP_OK)
	{
		Serial.println("Enviado com sucesso");
	}
	else
	{
		Serial.println("Erro ao enviar");
	}
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
	//=============================================================
	esp_now_register_send_cb(onDataSend);

	esp_now_peer_info_t peerInfo = {};
	memcpy(peerInfo.peer_addr, broadcastAddress, 6);

	peerInfo.channel = 0;
	peerInfo.encrypt = false;

	if (esp_now_add_peer(&peerInfo) != ESP_OK)
	{
		Serial.println("Erro ao adicionar peer");
		return;
	}
}

void loop()
{
}
