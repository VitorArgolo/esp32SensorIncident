#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

const char* loginEndpoint = "https://api-incidents-1.onrender.com/login";
const char* incidentsEndpoint = "https://api-incidents-1.onrender.com/incidents";
const char* username = "admin";
const char* password = "admin";

const int geophoneSensorPin = 32;  

String jwtToken;
String lastState = "No occurrence";

void setup() {
  Serial.begin(115200);
  WiFi.begin("usuario", "senha"); // wifi dados
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  pinMode(geophoneSensorPin, INPUT);

  getJwtToken();
}

void loop() {
  int sensorValue = analogRead(geophoneSensorPin);

  Serial.print("Analog value: ");
  Serial.println(sensorValue);

  String currentState = getVibrationLevel(sensorValue);

  if (currentState != lastState) {
    Serial.print("State changed: ");
    Serial.println(currentState);
    handleIncident(currentState);
    lastState = currentState;
  }

  delay(5000);  
}

String getVibrationLevel(int sensorValue) {
  if (sensorValue < 200) {
    return "No occurrence";
  } else if (sensorValue < 400) {
    return "Low";
  } else if (sensorValue < 600) {
    return "Medium";
  } else {
    return "High";
  }
}

void getJwtToken() {
  HTTPClient http;

  http.begin(loginEndpoint);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<200> doc;
  doc["username"] = username;
  doc["password"] = password;
  String requestBody;
  serializeJson(doc, requestBody);

  int httpResponseCode = http.POST(requestBody);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Response:");
    Serial.println(response);

    DynamicJsonDocument jsonDoc(200);
    deserializeJson(jsonDoc, response);
    jwtToken = jsonDoc["token"].as<String>();

    Serial.println("JWT Token:");
    Serial.println(jwtToken);
  } else {
    Serial.print("Error on HTTP request: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

void handleIncident(String vibrationLevel) {
  int incidentId = checkForIncidentWithId1();

  if (incidentId == 1) {
    updateIncident(vibrationLevel, incidentId);
  } else {
    createIncident(vibrationLevel);
  }
}

int checkForIncidentWithId1() {
  HTTPClient http;

  http.begin(incidentsEndpoint);
  http.addHeader("Authorization", jwtToken);

  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Response:");
    Serial.println(response);

    DynamicJsonDocument jsonDoc(1024);
    deserializeJson(jsonDoc, response);

    for (JsonObject incident : jsonDoc.as<JsonArray>()) {
      if (incident["id"].as<int>() == 1) {
        Serial.println("Incident with id 1 found.");
        return 1;
      }
    }
  } else {
    Serial.print("Error on HTTP request: ");
    Serial.println(httpResponseCode);
  }

  http.end();
  return -1;  
}

void createIncident(String vibrationLevel) {
  HTTPClient http;

  http.begin(incidentsEndpoint);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization",  jwtToken);

  
  StaticJsonDocument<200> doc;
  doc["title"] = "Bloco 1 - Geophone Sensor";
  doc["description"] = "Vibration level detected";
  doc["severity"] = getSeverity(vibrationLevel);
  doc["leak"] = (vibrationLevel != "No occurrence");
  String requestBody;
  serializeJson(doc, requestBody);

  Serial.println("JSON request body:");
  Serial.println(requestBody);

  int httpResponseCode = http.POST(requestBody);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Response:");
    Serial.println(response);
  } else {
    Serial.print("Error on HTTP request: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

void updateIncident(String vibrationLevel, int incidentId) {
  HTTPClient http;

  String updateEndpoint = String(incidentsEndpoint) + "/" + String(incidentId);
  http.begin(updateEndpoint);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization",  jwtToken);

  StaticJsonDocument<200> doc;
  doc["title"] = "Bloco 1 - Geophone Sensor";
  doc["description"] = "Vibration level detected";
  doc["severity"] = getSeverity(vibrationLevel);
  doc["leak"] = (vibrationLevel != "No occurrence");
  String requestBody;
  serializeJson(doc, requestBody);

  Serial.println("JSON update request body:");
  Serial.println(requestBody);

  int httpResponseCode = http.PUT(requestBody);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Response:");
    Serial.println(response);
  } else {
    Serial.print("Error on HTTP request: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

String getSeverity(String vibrationLevel) {
  if (vibrationLevel == "High") {
    return "Alto";
  } else if (vibrationLevel == "Medium") {
    return "Médio";
  } else if (vibrationLevel == "Low") {
    return "Baixo";
  } else {
    return "Nenhuma ocorrência";
  }
}
