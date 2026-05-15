#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

const char *LOG_FILE_PATH = "/log.txt";
const uint8_t LOG_QUEUE_LENGTH = 10;

typedef struct struct_message
{
  char id[33];
  char message[100];

} struct_message;

typedef struct log_entry
{
  uint32_t receivedAtMs;
  uint16_t bytesReceived;
  char senderMac[18];
  char id[33];
  char message[100];
} log_entry;

QueueHandle_t logQueue = nullptr;
bool littleFsReady = false;
volatile uint32_t droppedMessages = 0;

bool aceso = true;
void piscar()
{
  aceso = !aceso;
  digitalWrite(LED_BUILTIN, aceso);
}

void formatMacAddress(const uint8_t *mac, char *buffer, size_t bufferSize)
{
  snprintf(buffer, bufferSize, "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void printSavedLog()
{
  if (!littleFsReady)
  {
    return;
  }

  File logFile = LittleFS.open(LOG_FILE_PATH, FILE_READ);
  if (!logFile || logFile.isDirectory())
  {
    Serial.println("Nenhum log persistido encontrado em /log.txt.");
    return;
  }

  Serial.println();
  Serial.println("Conteudo persistido em /log.txt:");
  Serial.println("--------------------------------");
  while (logFile.available())
  {
    Serial.write(logFile.read());
  }

  Serial.println();
  Serial.println("--------------------------------");
  logFile.close();
}

void appendLogEntry(const log_entry &entry)
{
  if (!littleFsReady)
  {
    Serial.println("LittleFS indisponivel. Log nao foi salvo.");
    return;
  }

  File logFile = LittleFS.open(LOG_FILE_PATH, FILE_APPEND);
  if (!logFile)
  {
    Serial.println("Erro ao abrir /log.txt para gravacao.");
    return;
  }

  logFile.printf("[%lu ms] mac=%s bytes=%u id=%s mensagem=%s\n",
                 static_cast<unsigned long>(entry.receivedAtMs),
                 entry.senderMac,
                 entry.bytesReceived,
                 entry.id,
                 entry.message);
  logFile.close();
}

void enqueueReceivedData(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  struct_message receivedMessage = {};
  const size_t bytesToCopy = min(static_cast<size_t>(len), sizeof(receivedMessage));
  memcpy(&receivedMessage, incomingData, bytesToCopy);
  receivedMessage.id[sizeof(receivedMessage.id) - 1] = '\0';
  receivedMessage.message[sizeof(receivedMessage.message) - 1] = '\0';

  log_entry entry = {};
  entry.receivedAtMs = millis();
  entry.bytesReceived = static_cast<uint16_t>(len);
  formatMacAddress(mac, entry.senderMac, sizeof(entry.senderMac));
  strncpy(entry.id, receivedMessage.id, sizeof(entry.id) - 1);
  strncpy(entry.message, receivedMessage.message, sizeof(entry.message) - 1);

  if (logQueue == nullptr || xQueueSend(logQueue, &entry, 0) != pdTRUE)
  {
    droppedMessages++;
  }
}

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataRecv(const esp_now_recv_info_t *recvInfo, const uint8_t *incomingData, int len)
{
  enqueueReceivedData(recvInfo->src_addr, incomingData, len);
}
#else
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  enqueueReceivedData(mac, incomingData, len);
}
#endif

void processLogQueue()
{
  log_entry entry = {};
  while (logQueue != nullptr && xQueueReceive(logQueue, &entry, 0) == pdTRUE)
  {
    piscar();

    Serial.println();
    Serial.print("Bytes recebidos: ");
    Serial.println(entry.bytesReceived);
    Serial.print("MAC remetente: ");
    Serial.println(entry.senderMac);
    Serial.print("ID: ");
    Serial.println(entry.id);
    Serial.print("String: ");
    Serial.println(entry.message);

    appendLogEntry(entry);
  }
}

void warnDroppedMessages()
{
  static uint32_t lastDroppedMessages = 0;
  if (droppedMessages != lastDroppedMessages)
  {
    lastDroppedMessages = droppedMessages;
    Serial.print("Mensagens perdidas por fila cheia: ");
    Serial.println(lastDroppedMessages);
  }
}

void setupLittleFS()
{
  littleFsReady = LittleFS.begin(true);
  if (!littleFsReady)
  {
    Serial.println("Erro ao montar LittleFS.");
    return;
  }

  Serial.println("LittleFS montado com sucesso.");
  printSavedLog();
}

void setupEspNow()
{
  WiFi.mode(WIFI_STA);

  Serial.print("MAC deste dispositivo: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Erro ao inicializar o ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
}

void setup()
{
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  setupLittleFS();

  logQueue = xQueueCreate(LOG_QUEUE_LENGTH, sizeof(log_entry));
  if (logQueue == nullptr)
  {
    Serial.println("Erro ao criar fila de logs.");
  }

  setupEspNow();
}

void loop()
{
  processLogQueue();
  warnDroppedMessages();
  delay(10);
}
