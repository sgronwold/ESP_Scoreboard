#include <WiFi.h>
#include <HTTPClient.h>

// we have a very long json file so we need to up the recursion limit
#define ARDUINOJSON_DEFAULT_NESTING_LIMIT 65535

#include <ArduinoJson.h>

const char* ssid = "valpo-media";
const char* password = "brownandgold";

// begin lcd stuff

#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);

// end lcd stuff

char buffer[100];
HTTPClient http;

void nhl_displayScore(String tricode);

void setup() {
  Serial.begin(115200);

  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());

  WiFi.mode(WIFI_STA);  //Optional
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }

  Serial.println("\nConnected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());

  lcd.init();
  lcd.backlight();
}

void loop() {
  nhl_displayScore("ANA");
  delay(5000);
}

void nhl_displayScore(String tricode) {
  strcpy(buffer, "http://site.api.espn.com/apis/site/v2/sports/hockey/nhl/scoreboard");

  http.begin(buffer);
  int responseCode = http.GET();

  char* response = (char*)calloc(2000000, sizeof(char));
  
  WiFiClient client = http.getStream();

  while (client.available()){ 
    int nextch = client.read();

    Serial.print(nextch);
  }



  http.end();

  Serial.printf("Response code: %d\n", responseCode);

  JsonDocument json;
  deserializeJson(json, response);

  free(response);

  uint8_t gameIndex;
  for (gameIndex = 0; gameIndex < 20; gameIndex++) {
    String homeTricode = json["events"][gameIndex]["competitions"][0]["competitors"][0]["team"]["abbreviation"];
    if (homeTricode.equals(tricode)) break;

    String awayTricode = json["events"][gameIndex]["competitions"][0]["competitors"][1]["team"]["abbreviation"];
    if (awayTricode.equals(tricode)) break;
  }

  // if our team doesn't play then we return
  if (gameIndex == 20) {
    lcd.clear();
    return;
  }

  String shortName = json["events"][gameIndex]["shortName"];

  uint8_t awayScore = json["events"][gameIndex]["competitions"][0]["competitors"][1]["score"]["value"];
  uint8_t homeScore = json["events"][gameIndex]["competitions"][0]["competitors"][0]["score"]["value"];

  char* displayScore = (char*)calloc(10, sizeof(char));
  sprintf(displayScore, "%hu - %hu", awayScore, homeScore);

  String status = json["events"][gameIndex]["competitions"][0]["status"]["type"]["shortDetail"];

  lcd.clear();

  lcd.setCursor((20 - shortName.length()) / 2, 0);
  lcd.print(shortName);

  lcd.setCursor((20 - strlen(displayScore)) / 2, 1);
  lcd.print(displayScore);
  free(displayScore);

  lcd.setCursor((20 - status.length()) / 2, 2);
  lcd.print(status);
}
