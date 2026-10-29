#include <WiFi.h>
#include <WebServer.h>

// ---------- Broches ----------
const int LDR    = 34;
const int BOUTON = 4;
const int BUZZER = 18;
const int LED1   = 27;
const int LED2   = 25;
const int LED3   = 26;

// ---------- Parametres ----------
const int SEUIL_BAS   = 33;
const int SEUIL_HAUT  = 66;
const int SEUIL_NOIR  = 33;

const bool MODULE_INVERSE = true;

#define WIFI_SSID "test"
#define WIFI_PASS "12345678"

WebServer server(80);

// ---------- State ----------
bool systemeActif = true;       // no longer volatile: handled in loop()
int  pourcentage  = 0;
int  niveau       = 0;
unsigned long dernierAffichage = 0;
unsigned long dernierDebug     = 0;

// ---------- Web ----------
void pageWeb() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta http-equiv='refresh' content='2'>";
  html += "<title>Indicateur lumineux</title></head><body>";
  html += "<h1>Indicateur de niveau lumineux</h1>";
  html += "<p>Systeme : <b>" + String(systemeActif ? "ACTIF" : "ARRETE") + "</b></p>";
  html += "<p>Obscurite : <b>" + String(pourcentage) + " %</b></p>";
  html += "<p>Niveau : <b>" + String(niveau) + " / 3</b></p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

// ---------- Button polling + debounce ----------
void lireBouton() {
  static int dernierEtatLu = HIGH;
  static int etatStable = HIGH;
  static unsigned long dernierChangement = 0;

  int lecture = digitalRead(BOUTON);

  if (lecture != dernierEtatLu) {
    dernierChangement = millis();
    dernierEtatLu = lecture;
  }

  if (millis() - dernierChangement >= 50) {
    if (lecture != etatStable) {
      etatStable = lecture;

      // INPUT_PULLUP: pressed = LOW
      if (etatStable == LOW) {
        systemeActif = !systemeActif;
        Serial.println(systemeActif ? ">>> Systeme ACTIF" : ">>> Systeme ARRETE");
      }
    }
  }
}

// ---------- System ON ----------
void systemeOn() {
  int lecture = analogRead(LDR);
  pourcentage = map(lecture, 0, 4095, 0, 100);
  if (MODULE_INVERSE) pourcentage = 100 - pourcentage;

  if (pourcentage < SEUIL_BAS)       niveau = 1;
  else if (pourcentage < SEUIL_HAUT) niveau = 2;
  else                               niveau = 3;

  digitalWrite(LED1, niveau >= 1);
  digitalWrite(LED2, niveau >= 2);
  digitalWrite(LED3, niveau >= 3);

  if (niveau == 1) tone(BUZZER, 1000);
  else             noTone(BUZZER);
}

// ---------- System OFF ----------
void systemeOff() {
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
  digitalWrite(LED3, LOW);
  noTone(BUZZER);
  niveau = 0;
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(BOUTON, INPUT_PULLUP);
  analogReadResolution(12);

  // No attachInterrupt — button handled by polling in loop()

  Serial.print("Connexion WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS, 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.println(" connecte !");
  Serial.print("Adresse IP : ");
  Serial.println(WiFi.localIP());

  server.on("/", pageWeb);
  server.begin();
}

// ---------- Loop ----------
void loop() {
  lireBouton();              // <-- button handled here
  server.handleClient();

  if (systemeActif) {
    systemeOn();
  } else {
    systemeOff();
  }

  // Periodic status print (500 ms)
  if (millis() - dernierAffichage >= 500) {
    dernierAffichage = millis();
    Serial.print("Obscurite : ");
    Serial.print(pourcentage);
    Serial.print(" %  |  Niveau : ");
    Serial.print(niveau);
    Serial.print("/3  |  ");
    Serial.println(systemeActif ? "ACTIF" : "ARRETE");
  }

  // Throttled button debug (200 ms) — remove once button works
  if (millis() - dernierDebug >= 200) {
    dernierDebug = millis();
    Serial.print("BOUTON = ");
    Serial.println(digitalRead(BOUTON));
  }
}
