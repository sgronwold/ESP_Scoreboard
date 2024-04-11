#include <WiFi.h>
#include <HTTPClient.h>

// we have a very long json file so we need to up the recursion limit
#define ARDUINOJSON_DEFAULT_NESTING_LIMIT 65535

// we also need to allow commments in our json
#define ARDUINOJSON_ENABLE_COMMENTS 1

#include <ArduinoJson.h>

const char* ssid = "valpo-media";
const char* password = "brownandgold";

enum LEAGUE {
  NHL,
  NFL,
  NCAAB,
  NBA,
  MLB
};

// begin lcd stuff

#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);

// end lcd stuff

char buffer[1000];


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
  strcpy(buffer, "https://script.google.com/macros/s/AKfycbyoKpL1jsUBxsbQjMmXjrb1ST6RlBLxSuEget8oTJ9bp1OIoRBBRmo-OcUIBGs1BroV/exec");
  http.begin(buffer);

  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

  int responseCode = http.GET();

  Serial.printf("Response code %d\n", responseCode);

  String payload = http.getString();

  JsonDocument json;
  deserializeJson(json, payload);

  http.end();

  char* team = (char*)malloc(20 * sizeof(char));

  // display preferred teams
  JsonArray teamsList = json["ncaab"];
  for (int i = 0; i < teamsList.size(); i++) {
    String SCOREBOARD_URL = "http://site.api.espn.com/apis/site/v2/sports/basketball/mens-college-basketball/scoreboard/";
    String TEAMS_URL = "http://site.api.espn.com/apis/site/v2/sports/basketball/mens-college-basketball/teams/";
    displayScore(NCAAB, TEAMS_URL, SCOREBOARD_URL, teamsList[i].as<String>());
    delay(5000);
  }

  teamsList = json["nba"];
  for (int i = 0; i < teamsList.size(); i++) {
    String SCOREBOARD_URL = "http://site.api.espn.com/apis/site/v2/sports/basketball/nba/scoreboard/";
    String TEAMS_URL = "http://site.api.espn.com/apis/site/v2/sports/basketball/nba/teams/";
    displayScore(NBA, TEAMS_URL, SCOREBOARD_URL, teamsList[i].as<String>());
    delay(5000);
  }

  teamsList = json["nhl"];
  for (int i = 0; i < teamsList.size(); i++) {
    String SCOREBOARD_URL = "http://site.api.espn.com/apis/site/v2/sports/hockey/nhl/scoreboard/";
    String TEAMS_URL = "http://site.api.espn.com/apis/site/v2/sports/hockey/nhl/teams/";
    displayScore(NHL, TEAMS_URL, SCOREBOARD_URL, teamsList[i].as<String>());
    delay(5000);
  }

  teamsList = json["mlb"];
  for (int i = 0; i < teamsList.size(); i++) {
    String SCOREBOARD_URL = "http://site.api.espn.com/apis/site/v2/sports/baseball/mlb/scoreboard/";
    String TEAMS_URL = "http://site.api.espn.com/apis/site/v2/sports/baseball/mlb/teams/";
    displayScore(MLB, TEAMS_URL, SCOREBOARD_URL, teamsList[i].as<String>());
    delay(5000);
  }

  free(team);
}

void displayScore(LEAGUE league, String teamsURL, String scoreboardURL, String tricode) {
  HTTPClient http;
  
  // get the gameid for given tricode)
  Serial.println("Requesting the URL " + teamsURL+tricode);

  http.begin(teamsURL+tricode);

  int responseCode = http.GET();

  Serial.printf("Response code: %d\n", responseCode);
  if(responseCode != 200) return;

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
  if(responseCode != 200) return;

  response = (char*)malloc((http.getSize() + 2) * sizeof(char));

  http.getString().toCharArray(response, http.getSize() + 2);

  http.end();


  deserializeJson(json, response);

  free(response);


  // henceforth the game stored in our json is the one we want :)

  String shortName = json["shortName"];

  String shortAwayName = json["competitions"][0]["competitors"][1]["team"]["abbreviation"].as<String>();
  while(shortAwayName.length() < 4) shortAwayName += " ";

  String shortHomeName = json["competitions"][0]["competitors"][0]["team"]["abbreviation"].as<String>();
  while(shortHomeName.length() < 4) shortHomeName += " ";

  String longAwayName = json["competitions"][0]["competitors"][1]["team"]["shortDisplayName"].as<String>() + "(" + json["competitions"][0]["competitors"][1]["records"][0]["summary"].as<String>() + ")";
  String longHomeName = json["competitions"][0]["competitors"][0]["team"]["shortDisplayName"].as<String>() + "(" + json["competitions"][0]["competitors"][0]["records"][0]["summary"].as<String>() + ")";
  longAwayName.replace("Golden Knights", "G. Knights");
  longHomeName.replace("Golden Knights", "G. Knights");

  uint8_t awayScore = json["competitions"][0]["competitors"][1]["score"];
  uint8_t homeScore = json["competitions"][0]["competitors"][0]["score"];

  String status = json["competitions"][0]["status"]["type"]["shortDetail"];
  String gameStatus = json["competitions"][0]["status"]["type"]["name"];

  uint8_t balls = json["competitions"][0]["situation"]["balls"];
  uint8_t strikes = json["competitions"][0]["situation"]["strikes"];
  uint8_t outs = json["competitions"][0]["situation"]["outs"];
  uint8_t manOnFirst = json["competitions"][0]["situation"]["onFirst"];
  uint8_t manOnSecond = json["competitions"][0]["situation"]["onSecond"];
  uint8_t manOnThird = json["competitions"][0]["situation"]["onThird"];

  double awaySavePct = json["competitions"][0]["competitors"][1]["statistics"][1]["displayValue"];
  double homeSavePct = json["competitions"][0]["competitors"][0]["statistics"][1]["displayValue"];

  String lastPlayType = json["competitions"][0]["situation"]["lastPlay"]["type"]["abbreviation"];
  JsonArray athletesInvolved = json["competitions"][0]["situation"]["lastPlay"]["athletesInvolved"];
  
  String lastPlayPlayers = "";
  for(int i = 0; i < athletesInvolved.size(); i++) {
    lastPlayPlayers += athletesInvolved[i]["shortName"].as<String>() + ", ";
  }
  lastPlayPlayers = lastPlayPlayers.substring(0, lastPlayPlayers.length()-2);

  String lastPlay = json["competitions"][0]["situation"]["lastPlay"]["text"];

  String atBat = json["competitions"][0]["situation"]["batter"]["athlete"]["shortName"];
  String pitcher = json["competitions"][0]["situation"]["pitcher"]["athlete"]["shortName"];

  uint8_t period = json["competitions"][0]["status"]["period"];
  String clock = json["competitions"][0]["status"]["displayClock"];

  lcd.clear();
  if (league == MLB && gameStatus.equals("STATUS_IN_PROGRESS")) {

    lcd.setCursor(0, 0);
    lcd.print(shortAwayName + " " + awayScore);

    lcd.setCursor(0, 1);
    lcd.print(shortHomeName + " " + homeScore);

    lcd.setCursor(0, 2);
    lcd.print(status);

    // print num of balls
    lcd.setCursor(9, 0);
    lcd.print("B ");
    for (uint8_t i = 0; i < balls; i++) {
      lcd.print("X");
    }

    // print num of strikes
    lcd.setCursor(9, 1);
    lcd.print("S ");
    for (uint8_t i = 0; i < strikes; i++) {
      lcd.print("X");
    }

    // print num of outs
    lcd.setCursor(9, 2);
    lcd.print("O ");
    for (uint8_t i = 0; i < outs; i++) {
      lcd.print("X");
    }

    // print home plate
    lcd.setCursor(18, 2);
    lcd.print("*");

    // print first base
    lcd.setCursor(19, 1);
    if (manOnFirst) lcd.print("*");
    else lcd.print("O");

    // print second base
    lcd.setCursor(18, 0);
    if (manOnSecond) lcd.print("*");
    else lcd.print("O");

    // print third base
    lcd.setCursor(17, 1);
    if (manOnThird) lcd.print("*");
    else lcd.print("O");

    lcd.setCursor(0, 3);

    if((pitcher.length() + atBat.length()) > 19) {
      pitcher = pitcher.substring(pitcher.indexOf(' ')+1);
      atBat = atBat.substring(atBat.indexOf(' ')+1);
    }

    if(!pitcher.equals("null") && !atBat.equals("null")) {
      lcd.print(pitcher);
      lcd.print("|");
      lcd.print(atBat.substring(0, 19-pitcher.length()));
    }
  } else if (league == NHL && gameStatus.equals("STATUS_IN_PROGRESS")) {
    lcd.setCursor(0, 0);
    lcd.print(shortAwayName);

    lcd.setCursor(3, 0);
    sprintf(buffer, "%2hu", awayScore);
    lcd.print(buffer);

    sprintf(buffer, " %.3fSV%%", awaySavePct);
    lcd.print(buffer);

    lcd.setCursor(0, 1);
    lcd.print(shortHomeName);

    lcd.setCursor(3, 1);
    sprintf(buffer, "%2hu", homeScore);
    lcd.print(buffer);

    sprintf(buffer, " %.3fSV%%", homeSavePct);
    lcd.print(buffer);

    lcd.setCursor(15,0);
    sprintf(buffer, "Per%2hu", period);
    lcd.print(buffer);

    lcd.setCursor(15,1);
    lcd.print(clock);

    if(!lastPlay.equals("null")) {
      lcd.setCursor(0, 2);
      lcd.print(lastPlay.substring(0,20));

      lcd.setCursor(0, 3);
      lcd.print(lastPlay.substring(20, 40));
    }
  } else {
    lcd.setCursor((20 - longAwayName.length()) / 2, 0);
    lcd.print(longAwayName);

    lcd.setCursor((20 - longHomeName.length()) / 2, 1);
    lcd.print(longHomeName);

    sprintf(buffer, "%hu - %hu", awayScore, homeScore);
    lcd.setCursor((20 - strlen(buffer)) / 2, 2);
    lcd.print(buffer);

    lcd.setCursor((20 - status.length()) / 2, 3);
    lcd.print(status);
  }
}