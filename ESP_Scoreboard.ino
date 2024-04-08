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

char buffer[200];


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
  HTTPClient http;
  // get preferred teams
  strcpy(buffer, "https://sgronwold.xyz/api/preferred-teams.json");
  http.begin(buffer);
  http.GET();

  char* payload = (char*)malloc(http.getSize() + 1 * sizeof(char));

  http.getString().toCharArray(payload, http.getSize() + 1);

  JsonDocument json;
  deserializeJson(json, payload);
  free(payload);

  http.end();

  char* team = (char*)malloc(20 * sizeof(char));

  // display preferred teams
  JsonArray teamsList = json["ncaab"];
  for (int i = 0; i < teamsList.size(); i++) {
    String NCAAB_SCOREBOARD_URL = "http://site.api.espn.com/apis/site/v2/sports/basketball/mens-college-basketball/scoreboard/";
    String NCAAB_TEAMS_URL = "http://site.api.espn.com/apis/site/v2/sports/basketball/mens-college-basketball/teams/";
    displayScore(0, NCAAB_TEAMS_URL, NCAAB_SCOREBOARD_URL, teamsList[i].as<String>());
    delay(5000);
  }

  teamsList = json["nhl"];
  for (int i = 0; i < teamsList.size(); i++) {
    String NHL_SCOREBOARD_URL = "http://site.api.espn.com/apis/site/v2/sports/hockey/nhl/scoreboard/";
    String NHL_TEAMS_URL = "http://site.api.espn.com/apis/site/v2/sports/hockey/nhl/teams/";
    displayScore(0, NHL_TEAMS_URL, NHL_SCOREBOARD_URL, teamsList[i].as<String>());
    delay(5000);
  }

  teamsList = json["mlb"];
  for (int i = 0; i < teamsList.size(); i++) {
    String MLB_SCOREBOARD_URL = "http://site.api.espn.com/apis/site/v2/sports/baseball/mlb/scoreboard/";
    String MLB_TEAMS_URL = "http://site.api.espn.com/apis/site/v2/sports/baseball/mlb/teams/";
    displayScore(1, MLB_TEAMS_URL, MLB_SCOREBOARD_URL, teamsList[i].as<String>());
    delay(5000);
  }

  free(team);
}

void displayScore(uint8_t useBaseballFormatting, String teamsURL, String scoreboardURL, String tricode) {
  HTTPClient http;
  
  // get the gameid for given tricode)
  Serial.println("Requesting the URL " + teamsURL+tricode);

  http.begin(teamsURL+tricode);

  int responseCode = http.GET();

  Serial.printf("Response code: %d\n", responseCode);

  char* response = (char*)malloc((http.getSize() + 2) * sizeof(char));

  http.getString().toCharArray(response, http.getSize() + 2);

  http.end();


  JsonDocument json;
  deserializeJson(json, response);

  String gameid_str = json["team"]["nextEvent"][0]["id"];

  Serial.println("Found gameid " + gameid_str);

  free(response);

  Serial.println("Requesting the URL " + scoreboardURL+gameid_str);


  // now we've gotten the game id
  http.begin(scoreboardURL+gameid_str);
  responseCode = http.GET();

  Serial.printf("Response code: %d\n", responseCode);

  response = (char*)malloc((http.getSize() + 2) * sizeof(char));

  http.getString().toCharArray(response, http.getSize() + 2);

  http.end();


  deserializeJson(json, response);

  free(response);


  // henceforth the game stored in our json is the one we want :)

  String shortName = json["shortName"];

  String awayScore = json["competitions"][0]["competitors"][1]["score"];
  String homeScore = json["competitions"][0]["competitors"][0]["score"];
  String score = awayScore + " - " + homeScore;

  String status = json["competitions"][0]["status"]["type"]["shortDetail"];
  String gameStatus = json["competitions"][0]["status"]["type"]["name"];

  uint8_t balls = 0;
  uint8_t strikes = 0;
  uint8_t outs = 0;
  uint8_t manOnFirst = 0;
  uint8_t manOnSecond = 0;
  uint8_t manOnThird = 0;


  lcd.clear();
  if (useBaseballFormatting && !gameStatus.equals("STATUS_SCHEDULED")) {

    lcd.setCursor(10 + (10 - shortName.length()) / 2, 0);
    lcd.print(shortName);

    lcd.setCursor(10 + (10 - score.length()) / 2, 1);
    lcd.print(score);

    lcd.setCursor(10 + (10 - status.length()) / 2, 2);
    lcd.print(status);

    // print num of balls
    lcd.setCursor(0, 0);
    lcd.print("B ");
    for (uint8_t i = 0; i < balls; i++) {
      lcd.print("X");
    }

    // print num of strikes
    lcd.setCursor(0, 1);
    lcd.print("S ");
    for (uint8_t i = 0; i < strikes; i++) {
      lcd.print("X");
    }

    // print num of outs
    lcd.setCursor(0, 2);
    lcd.print("O ");
    for (uint8_t i = 0; i < outs; i++) {
      lcd.print("X");
    }

    // print home plate
    lcd.setCursor(7, 3);
    lcd.print("*");

    // print first base
    lcd.setCursor(8, 2);
    if (manOnFirst) lcd.print("*");
    else lcd.print("O");

    // print second base
    lcd.setCursor(7, 1);
    if (manOnSecond) lcd.print("*");
    else lcd.print("O");

    // print third base
    lcd.setCursor(6, 2);
    if (manOnThird) lcd.print("*");
    else lcd.print("O");
  } else {

    lcd.setCursor((20 - shortName.length()) / 2, 0);
    lcd.print(shortName);

    lcd.setCursor((20 - score.length()) / 2, 1);
    lcd.print(score);

    lcd.setCursor((20 - status.length()) / 2, 2);
    lcd.print(status);
  }
}