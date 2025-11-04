#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

/* ======================================== Variables pour le réseau. */
const char* ssid = "Redmi Note 12"; // nom de réseau WiFi.
const char* password = "12435687"; // mot de passe WiFi.
/* ======================================== Variables pour les tokens du bot Telegram. */
String BOTtoken = "7068820528:AAFGBE6s5YOQ8qK9t0oP89CRUYbGwSzacIQ"; 
String CHAT_ID = "7187814049";
/* ======================================== Initialisation de WiFiClientSecure. */
WiFiClientSecure clientTCP;
/* ======================================== Initialisation de UniversalTelegramBot. */
UniversalTelegramBot bot(BOTtoken, clientTCP);
/* ======================================== Définition des GPIO de la caméra sur l'ESP32 Cam. */
// CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM    -1
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM    21
#define SIOD_GPIO_NUM    26
#define SIOC_GPIO_NUM    27

#define Y9_GPIO_NUM      35
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      36
#define Y5_GPIO_NUM      19
#define Y4_GPIO_NUM      18
#define Y3_GPIO_NUM       5
#define Y2_GPIO_NUM       4
#define VSYNC_GPIO_NUM   25
#define HREF_GPIO_NUM    23
#define PCLK_GPIO_NUM    22

/* ======================================== Définit HIGH et LOW avec ON et OFF */
#define ON HIGH
#define OFF LOW
#define FLASH_LED_PIN   4           //PIN du flash LED (GPIO 4)
#define PIR_SENSOR_PIN  12          //PIN du capteur PIR (GPIO 12)
#define BUZZER_PIN   13             // PIN du buzzer (GPIO 13)

#define EEPROM_SIZE     2           //le nombre d'octets à accéder

// Vérifie les nouveaux messages toutes les 1 seconde (1000 ms).
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;
/* ======================================== Variables pour millis (pour stabiliser le capteur PIR). */
int countdown_interval_to_stabilize_PIR_Sensor = 1000;
unsigned long lastTime_countdown_Ran;
byte countdown_to_stabilize_PIR_Sensor = 30;

bool sendPhoto = false;             // Variables pour déclencher l'envoi de photos.

bool PIR_Sensor_is_stable = false;  // Variable pour indiquer que le temps de stabilisation du capteur PIR est terminé.

bool boolPIRState = false;

/* __________ Fonction pour parser une chaîne */
String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;
  
  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
/* __________ Sous-routine pour envoyer des messages de retour lorsque les photos sont envoyées avec succès ou échouent à être envoyées sur Telegram. */
void FB_MSG_is_photo_send_successfully (bool state) {
  String send_feedback_message = "";
  if(state == false) {
    send_feedback_message += "Depuis l'ESP32-CAM :\n\n";
    send_feedback_message += "Échec de l'envoi de la photo par l'ESP32-CAM.\n";
    send_feedback_message += "Suggestion :\n";
    send_feedback_message += "- Veuillez réessayer.\n";
    send_feedback_message += "- Réinitialisez l'ESP32-CAM.\n";
    send_feedback_message += "- Changez FRAMESIZE (voir le menu déroulant de la taille des cadres dans void configInitCamera).\n";
    Serial.print(send_feedback_message);
    send_feedback_message += "\n\n";
    send_feedback_message += "/start : pour voir toutes les commandes.";
    bot.sendMessage(CHAT_ID, send_feedback_message, "");
  } else {
    if(boolPIRState == true) {
      Serial.println("Photo envoyée avec succès.");
      send_feedback_message += "Depuis l'ESP32-CAM :\n\n";
      send_feedback_message += "Le capteur PIR détecte des objets et des mouvements.\n";
      send_feedback_message += "Photo envoyée avec succès.\n\n";
      send_feedback_message += "/start : pour voir toutes les commandes.";
      bot.sendMessage(CHAT_ID, send_feedback_message, ""); 
    }
    if(sendPhoto == true) {
      Serial.println("Photo envoyée avec succès.");
      send_feedback_message += "Depuis l'ESP32-CAM :\n\n";
      send_feedback_message += "Photo envoyée avec succès.\n\n";
      send_feedback_message += "/start : pour voir toutes les commandes.";
      bot.sendMessage(CHAT_ID, send_feedback_message, ""); 
    }
  }
}
/* __________ Fonction pour lire la valeur du capteur PIR (HIGH/1 OU LOW/0) */
bool PIR_State() {
  bool PRS = digitalRead(PIR_SENSOR_PIN);
  return PRS;
}
/* __________ Sous-routine pour allumer ou éteindre le flash LED. */
void LEDFlash_State(bool ledState) {
  digitalWrite(FLASH_LED_PIN, ledState);
}
/* __________ Sous-routine pour configurer et sauvegarder les paramètres dans l'EEPROM pour le mode "capture de photos avec capteur PIR". */
void enable_capture_Photo_with_PIR(bool state) {
  EEPROM.write(1, state);
  EEPROM.commit();
  delay(50);
}
/* __________ Fonction pour lire les paramètres dans l'EEPROM pour le mode "capture de photos avec capteur PIR".*/
bool capture_Photo_with_PIR_state() {
  bool capture_Photo_with_PIR = EEPROM.read(1);
  return capture_Photo_with_PIR;
}
/* __________ Sous-routine pour la configuration de la caméra. */
void configInitCamera(){
  camera_config_t config;
 config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG;

  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA; //--> FRAMESIZE_ + UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA
    config.jpeg_quality = 10;  
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;  
    config.fb_count = 1;
  }

  /* ---------------------------------------- init de la caméra. */
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Échec de l'initialisation de la caméra avec l'erreur 0x%x", err);
    Serial.println();
    Serial.println("Redémarrer l'ESP32 Cam");
    delay(1000);
    ESP.restart();
  }
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_SXGA); 
}
/* __________ */

/* __________ Sous-routines pour gérer ce qu'il faut faire après l'arrivée d'un nouveau message. */
void handleNewMessages(int numNewMessages) {
  Serial.print("Gérer de nouveaux messages : ");
  Serial.println(numNewMessages);

  /* ---------------------------------------- "Boucle For" pour vérifier le contenu du message nouvellement reçu. */
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Utilisateur non autorisé", "");
      Serial.println("Utilisateur non autorisé");
      Serial.println("------------");
      continue;
    }
    String text = bot.messages[i].text;
    Serial.println(text);
   
    String send_feedback_message = "";
    String from_name = bot.messages[i].from_name;
    if (text == "/start") {
      send_feedback_message += "Depuis l'ESP32-CAM :\n\n";
      send_feedback_message += "Bienvenue, " + from_name + "\n";
      send_feedback_message += "Utilisez les commandes suivantes pour interagir avec l'ESP32-CAM.\n\n";
      send_feedback_message += "/capture_photo : prend une nouvelle photo\n\n";
      send_feedback_message += "Paramètres :\n";
      send_feedback_message += "/enable_capture_Photo_with_PIR : prend une nouvelle photo avec le capteur PIR\n";
      send_feedback_message += "/disable_capture_Photo_with_PIR : prend une nouvelle photo sans le capteur PIR\n\n";
      send_feedback_message += "Statut des paramètres :\n";
     
      if(capture_Photo_with_PIR_state() == ON) {
        send_feedback_message += "- Capture Photo Avec PIR = ON\n";
      }
      if(capture_Photo_with_PIR_state() == OFF) {
        send_feedback_message += "- Capture Photo Avec PIR = OFF\n";
      }
      if(PIR_Sensor_is_stable == false) {
        send_feedback_message += "\nStatut du capteur PIR :\n";
        send_feedback_message += "Le capteur PIR est en cours de stabilisation.\n";
        send_feedback_message += "Le temps de stabilisation est de " + String(countdown_to_stabilize_PIR_Sensor) + " secondes. Veuillez attendre.\n";
      }
      bot.sendMessage(CHAT_ID, send_feedback_message, "");
      Serial.println("------------");
    }
    
    // La condition si la commande reçue est "/capture_photo".
    if (text == "/capture_photo") {
      sendPhoto = true;
      Serial.println("Nouvelle demande de photo");
    }
    
   

    // La condition si la commande reçue est "/enable_capture_Photo_with_PIR".
    if (text == "/enable_capture_Photo_with_PIR") {
      enable_capture_Photo_with_PIR(ON);
      send_feedback_message += "Depuis l'ESP32-CAM :\n\n";
      if(capture_Photo_with_PIR_state() == ON) {
        Serial.println("Capture Photo Avec PIR = ON");
        send_feedback_message += "Capture Photo Avec PIR = ON\n\n";
        botRequestDelay = 20000;
      } else {
        Serial.println("Échec de la configuration. Essayez à nouveau.");
        send_feedback_message += "Échec de la configuration. Essayez à nouveau.\n\n"; 
      }
      Serial.println("------------");
      send_feedback_message += "/start : pour voir toutes les commandes.";
      bot.sendMessage(CHAT_ID, send_feedback_message, "");
    }

    // La condition si la commande reçue est "/disable_capture_Photo_with_PIR".
    if (text == "/disable_capture_Photo_with_PIR") {
      enable_capture_Photo_with_PIR(OFF);
      send_feedback_message += "Depuis l'ESP32-CAM :\n\n";
      if(capture_Photo_with_PIR_state() == OFF) {
        Serial.println("Capture Photo Avec PIR = OFF");
        send_feedback_message += "Capture Photo Avec PIR = OFF\n\n";
        botRequestDelay = 1000;
      } else {
        Serial.println("Échec de la configuration. Essayez à nouveau.");
        send_feedback_message += "Échec de la configuration. Essayez à nouveau.\n\n"; 
      }
      Serial.println("------------");
      send_feedback_message += "/start : pour voir toutes les commandes.";
      bot.sendMessage(CHAT_ID, send_feedback_message, "");
    }
  }
}
 //fonction allumage buzzer
  void allumerBUZZER(int buzzer) {
    int i ;
    //Première fréquence
   for(i=0;i<80;i++)
   {
    digitalWrite(buzzer,HIGH); // Buzzer émet du son
    delay(1);//pause d'une milliseconde
    digitalWrite(buzzer,LOW); // buzzer éteint
    delay(1);//pause d'une milliseconde
    }
    //Deuxième fréquence
     for(i=0;i<100;i++)
      {
        digitalWrite(buzzer,HIGH); // buzzer émet du son
        delay(2);//pause de deux millisecondes
        digitalWrite(buzzer,LOW); // buzzer éteint
        delay(2);//pause de deux millisecondes
      }
}

/* __________ Sous-routine pour le processus de prise et d'envoi de photos. */
String sendPhotoTelegram() {
  const char* myDomain = "api.telegram.org";
  String getAll = "";
  String getBody = "";
  /* ---------------------------------------- Le processus de prise de photos. */
  Serial.println("Prise d'une photo...");
  delay(1500);
  /* ::::::::::::::::: Prise d'une photo. */ 
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Échec de la capture de la caméra");
    Serial.println("Redémarrer l'ESP32 Cam");
    delay(1000);
    ESP.restart();
    return "Échec de la capture de la caméra";
  }  
  /* ---------------------------------------- Le processus d'envoi de photos. */
  Serial.println("Connexion à " + String(myDomain));

  if (clientTCP.connect(myDomain, 443)) {
    Serial.println("Connexion réussie");
    Serial.print("Envoi des photos");
    
    String head = "--Esp32Cam\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n";
    head += CHAT_ID; 
    head += "\r\n--Esp32Cam\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--Esp32Cam--\r\n";
    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;
  
    clientTCP.println("POST /bot"+BOTtoken+"/sendPhoto HTTP/1.1");
    clientTCP.println("Host: " + String(myDomain));
    clientTCP.println("Content-Length: " + String(totalLen));
    clientTCP.println("Content-Type: multipart/form-data; boundary=Esp32Cam");
    clientTCP.println();
    clientTCP.print(head);
  
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    
    for (size_t n=0;n<fbLen;n=n+1024) {
      if (n+1024<fbLen) {
        clientTCP.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen%1024>0) {
        size_t remainder = fbLen%1024;
        clientTCP.write(fbBuf, remainder);
      }
    }  
    
    clientTCP.print(tail);
    
    esp_camera_fb_return(fb);
    
    int waitTime = 10000;   //--> délai d'expiration de 10 secondes (Pour envoyer des photos.)
    long startTimer = millis();
    boolean state = false;
    
    while ((startTimer + waitTime) > millis()){
      Serial.print(".");
      delay(100);      
      while (clientTCP.available()) {
        char c = clientTCP.read();
        if (state==true) getBody += String(c);        
        if (c == '\n') {
          if (getAll.length()==0) state=true; 
          getAll = "";
        } 
        else if (c != '\r')
          getAll += String(c);
        startTimer = millis();
      }
      if (getBody.length()>0) break;
    }
    clientTCP.stop();
    Serial.println(getBody);
    // Si la photo est réussie ou a échoué à être envoyée, un message de retour sera envoyé sur Telegram.
    if(getBody.length() > 0) {
      String send_status = "";
      send_status = getValue(getBody, ',', 0);
      send_status = send_status.substring(6);
      
      if(send_status == "true") {
        FB_MSG_is_photo_send_successfully(true); 
      }
      if(send_status == "false") {
        FB_MSG_is_photo_send_successfully(false); 
      }
    }
    if(getBody.length() == 0) FB_MSG_is_photo_send_successfully(false);
  }
  else {
    getBody="Connexion à api.telegram.org échouée.";
    Serial.println("Connexion à api.telegram.org échouée.");
  }
  Serial.println("------------");
  return getBody;
  
}
void setup(){
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println();
  Serial.println("------------");
  EEPROM.begin(EEPROM_SIZE);
  /*Écrit des paramètres dans l'EEPROM. */  
  enable_capture_Photo_with_PIR(OFF);
  delay(500);
  Serial.println("Statut des paramètres :");
  if(capture_Photo_with_PIR_state() == ON) {
    Serial.println("- Capture Photo Avec PIR = ON");
    botRequestDelay = 20000;
  }
  if(capture_Photo_with_PIR_state() == OFF) {
    Serial.println("- Capture Photo Avec PIR = OFF");
    botRequestDelay = 1000;
  }
  pinMode(FLASH_LED_PIN, OUTPUT);
  LEDFlash_State(OFF);
  Serial.println();
  Serial.println("Démarre la configuration et l'initialisation de la caméra...");
  configInitCamera();
  Serial.println("Configuration et initialisation de la caméra réussies.");
  Serial.println();
  /* ---------------------------------------- Connectez-vous au Wi-Fi. */
  WiFi.mode(WIFI_STA);
  Serial.print("Connexion à ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  clientTCP.setCACert(TELEGRAM_CERTIFICATE_ROOT); 
  int connecting_process_timed_out = 20; //20 = 20 secondes.
  connecting_process_timed_out = connecting_process_timed_out * 2;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    LEDFlash_State(ON);
    delay(250);
    LEDFlash_State(OFF);
    delay(250);
    if(connecting_process_timed_out > 0) connecting_process_timed_out--;
    if(connecting_process_timed_out == 0) {
      delay(1000);
      ESP.restart();
    }
  }
  LEDFlash_State(OFF);
  Serial.println();
  Serial.print("Connecté avec succès à ");
  Serial.println(ssid);
  Serial.print("Adresse IP de l'ESP32-CAM : ");
  Serial.println(WiFi.localIP());
  Serial.println();
  Serial.println("Le capteur PIR est en cours de stabilisation.");
  Serial.printf("Le temps de stabilisation est de %d secondes. Veuillez attendre.\n", countdown_to_stabilize_PIR_Sensor);
  
  Serial.println("------------");
  Serial.println();
 
  //initialisation du buzzer
  pinMode(BUZZER_PIN,OUTPUT);//On initialise la broche du buzzer en sortie
}

void loop() {
  /* ---------------------------------------- Conditions pour prendre et envoyer des photos. */
  if(sendPhoto) {
    Serial.println("Préparation de la photo...");
    sendPhotoTelegram(); 
    sendPhoto = false; 
  }
  if(millis() > lastTimeBotRan + botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      Serial.println();
      Serial.println("------------");
      Serial.println("réponse reçue");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }

  if(PIR_Sensor_is_stable == false) {
    if(millis() > lastTime_countdown_Ran + countdown_interval_to_stabilize_PIR_Sensor) {
      if(countdown_to_stabilize_PIR_Sensor > 0) countdown_to_stabilize_PIR_Sensor--;
      if(countdown_to_stabilize_PIR_Sensor == 0) {
        PIR_Sensor_is_stable = true;
        Serial.println();
        Serial.println("------------");
        Serial.println("Le temps de stabilisation du capteur PIR est terminé.");
        Serial.println("Le capteur PIR peut déjà fonctionner.");
        Serial.println("------------");
        String send_Status_PIR_Sensor = "";
        send_Status_PIR_Sensor += "Depuis l'ESP32-CAM :\n\n";
        send_Status_PIR_Sensor += "Le temps de stabilisation du capteur PIR est terminé.\n";
        send_Status_PIR_Sensor += "Le capteur PIR peut déjà fonctionner.";
        bot.sendMessage(CHAT_ID, send_Status_PIR_Sensor, "");
      }
      lastTime_countdown_Ran = millis();
    }
  }
  
  if(capture_Photo_with_PIR_state() == ON) {
    if(PIR_State() == true && PIR_Sensor_is_stable == true) {
      Serial.println("------------");
      Serial.println("Le capteur PIR détecte des objets et des mouvements.");
      boolPIRState = true;
      sendPhotoTelegram();
      allumerBUZZER(BUZZER_PIN);

      boolPIRState = false;
    }
  }
}
