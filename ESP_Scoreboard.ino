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
  nhl_displayScore("SJ");
  delay(5000);
  nhl_displayScore("CHI");
  delay(5000);
  mlb_displayScore("CHC");
  delay(5000);
}

void nhl_displayScore(String tricode) {
  // get the gameid for given tricode
  ("http://site.api.espn.com/apis/site/v2/sports/hockey/nhl/teams/"+tricode).toCharArray(buffer, sizeof(buffer)/sizeof(buffer[0]));

  http.begin(buffer);
  int responseCode = http.GET();
  
  Serial.printf("Response code: %d\n", responseCode);

  char* response = (char*) malloc((http.getSize()+2) * sizeof(char));

  http.getString().toCharArray(response, http.getSize()+2);

  http.end();


  JsonDocument json;
  deserializeJson(json, response);

  char* gameid = (char*) malloc(20 * sizeof(char));

  String gameid_str = json["team"]["nextEvent"][0]["id"];
  gameid_str.toCharArray(gameid,20);

  free(response);

  sprintf(buffer, "http://site.api.espn.com/apis/site/v2/sports/hockey/nhl/scoreboard/%s", gameid);
  free(gameid);


  // now we've gotten the game id
  http.begin(buffer);
  responseCode = http.GET();
  
  Serial.printf("Response code: %d\n", responseCode);

  response = (char*) malloc((http.getSize()+2) * sizeof(char));

  http.getString().toCharArray(response, http.getSize()+2);

  http.end();


  json;
  deserializeJson(json, response);

  free(response);


  // henceforth the game stored in our json is the one we want :)

  String shortName = json["shortName"];

  String awayScore = json["competitions"][0]["competitors"][1]["score"];
  String homeScore = json["competitions"][0]["competitors"][0]["score"];
  String score = awayScore + " - " + homeScore;

  String status = json["competitions"][0]["status"]["type"]["shortDetail"];

  lcd.clear();

  lcd.setCursor((20 - shortName.length()) / 2, 0);
  lcd.print(shortName);

  lcd.setCursor((20 - score.length()) / 2, 1);
  lcd.print(score);

  lcd.setCursor((20 - status.length()) / 2, 2);
  lcd.print(status);
}




void mlb_displayScore(String tricode) {
  // get the gameid for given tricode
  ("http://site.api.espn.com/apis/site/v2/sports/baseball/mlb/teams/"+tricode).toCharArray(buffer, sizeof(buffer)/sizeof(buffer[0]));

  http.begin(buffer);
  int responseCode = http.GET();
  
  Serial.printf("Response code: %d\n", responseCode);

  char* response = (char*) malloc((http.getSize()+2) * sizeof(char));

  http.getString().toCharArray(response, http.getSize()+2);

  http.end();


  JsonDocument json;
  deserializeJson(json, response);

  char* gameid = (char*) malloc(20 * sizeof(char));

  String gameid_str = json["team"]["nextEvent"][0]["id"];
  gameid_str.toCharArray(gameid,20);

  free(response);

  sprintf(buffer, "http://site.api.espn.com/apis/site/v2/sports/baseball/mlb/scoreboard/%s", gameid);
  free(gameid);


  // now we've gotten the game id
  http.begin(buffer);
  responseCode = http.GET();
  
  Serial.printf("Response code: %d\n", responseCode);

  response = (char*) malloc((http.getSize()+2) * sizeof(char));

  http.getString().toCharArray(response, http.getSize()+2);

  http.end();



  deserializeJson(json, response);

  free(response);


  // henceforth the game stored in our json is the one we want :)

  String shortName = json["shortName"];

  String awayScore = json["competitions"][0]["competitors"][1]["score"];
  String homeScore = json["competitions"][0]["competitors"][0]["score"];
  String score = awayScore + " - " + homeScore;

  String status = json["competitions"][0]["status"]["type"]["shortDetail"];

  uint8_t balls = 3;
  uint8_t strikes = 2;
  uint8_t outs = 2;

  uint8_t manOnFirst = 1;
  uint8_t manOnSecond = 1;
  uint8_t manOnThird = 1;

  lcd.clear();

  lcd.setCursor(10 + (10 - shortName.length()) / 2, 0);
  lcd.print(shortName);

  lcd.setCursor(10 + (10 - score.length()) / 2, 1);
  lcd.print(score);

  lcd.setCursor(10 + (10 - status.length()) / 2, 2);
  lcd.print(status);

  // print num of balls
  lcd.setCursor(0,0);
  lcd.print("B ");
  for(uint8_t i = 0; i < balls; i++) {
    lcd.print("X");
  }

  // print num of strikes
  lcd.setCursor(0,1);
  lcd.print("S ");
  for(uint8_t i = 0; i < strikes; i++) {
    lcd.print("X");
  }

  // print num of outs
  lcd.setCursor(0,2);
  lcd.print("O ");
  for(uint8_t i = 0; i < outs; i++) {
    lcd.print("X");
  }

  // print home plate
  lcd.setCursor(7,3);
  lcd.print("*");

  // print first base
  lcd.setCursor(8,2);
  if(manOnFirst) lcd.print("*");
  else lcd.print("O");

  // print second base
  lcd.setCursor(7,1);
  if(manOnSecond) lcd.print("*");
  else lcd.print("O");

  // print third base
  lcd.setCursor(6,2);
  if(manOnThird) lcd.print("*");
  else lcd.print("O");
}
