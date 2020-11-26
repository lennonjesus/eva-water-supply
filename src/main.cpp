#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "WiFi.h"

#include "credentials.h" // ATTENTION: look at credentials.sample.h file

#define uS_TO_S_FACTOR 1000000 // conv. de microsegundos para segundos
#define TIME_TO_SLEEP 1800 // tempo de hibernacao em s

#define ENA 2
#define IN1 18
#define IN2 19
#define WATER_LVL 4

WiFiClient client;

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

Adafruit_MQTT_Subscribe soilMoistureSub = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/potOneSoilMoisture", MQTT_QOS_1);

TaskHandle_t IrrigacaoTaskInstance;

bool hasWater = false;
bool isMotorOn = false;
int soilMoisture = 100;

void readWaterLevel();
void checkWifi();
void connectWiFi();
void initMQTT();
void connectMqtt();
void soilMoistureCallback(char *data, uint16_t len);

void IrrigacaoTask(void *args) {

  Serial.println((String) __func__ + " rodando em " + (String) xPortGetCoreID());

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  for (;;) {

    Serial.println("Blah");

    readWaterLevel();

    if (hasWater && soilMoisture <= 30) {

      if (!isMotorOn) {
        Serial.println("Ligando bomba");
        
        digitalWrite(ENA, HIGH);
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, HIGH);

        isMotorOn = true;

      }

      Serial.println("Irrigando...");

      delay(3000);

      return;

    }

    if (isMotorOn) {
      Serial.println("Desligando bomba");

      digitalWrite(ENA, LOW);
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      
      isMotorOn = false;

      delay(1000);
    }

    delay(1000 * 5 * 1);
  }
  
}

void setup() {
  
  Serial.begin(115200);

  Serial.println((String) __func__ + " rodando em " + (String) xPortGetCoreID());

  // esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  pinMode(WATER_LVL, INPUT);

  xTaskCreatePinnedToCore(IrrigacaoTask, 
                    "IrrigacaoTask", 
                    2048, 
                    &IrrigacaoTaskInstance, 
                    3, 
                    NULL, 
                    PRO_CPU_NUM);

  connectWiFi();
  initMQTT();
  delay(1000);

}

void loop() {
  checkWifi();
  connectMqtt();
  mqtt.processPackets(5000);

  delay(4000);
}

void readWaterLevel() {

  hasWater = !digitalRead(WATER_LVL);

  if (hasWater) {
    Serial.println("Tem agua");
  } else {
    Serial.println("Nao tem agua");
  }
}

void initMQTT() {
  soilMoistureSub.setCallback(soilMoistureCallback);
  mqtt.subscribe(&soilMoistureSub);
}

void soilMoistureCallback(char *data, uint16_t len) {
  Serial.println(data);
  soilMoisture = atoi(data);
  Serial.print("Umidade do solo: "); 
  Serial.println(soilMoisture);
}

void connectMqtt() {
  int8_t ret;
 
  if (mqtt.connected()) {
    return;
  }
 
  Serial.println("Conectando-se ao broker mqtt...");
 
  uint8_t num_tentativas = 5;
  
  while ((ret = mqtt.connect()) != 0) {
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Falha ao se conectar. Tentando se reconectar em 5 segundos.");
    mqtt.disconnect();
    delay(5000);
    num_tentativas--;
    if (num_tentativas == 0) {
      Serial.println("Seu ESP será resetado.");
      ESP.restart();
    }
  }
 
  Serial.println("Conectado ao broker com sucesso.");
}

void checkWifi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(" Conexão não estabelecida. Reiniciando...");
    ESP.restart();
  } 
}

void connectWiFi() {

  delay(100);
  
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Conectando em ");
  Serial.print(WIFI_SSID);

  int timeout = 0;

  while (WiFi.status() != WL_CONNECTED && ++timeout <= 10) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    Serial.print("."); 
  }

  checkWifi();

  Serial.println("");
  Serial.print("Conectado em ");
  Serial.print(WIFI_SSID);
  Serial.print(" com o IP ");
  Serial.println(WiFi.localIP());
}
