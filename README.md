# 📷 Caméra de surveillance ESP32-CAM avec alertes Telegram

Système de surveillance connecté basé sur un **ESP32-CAM** et un **capteur de mouvement PIR**. Lorsqu'un mouvement est détecté, la caméra prend une photo, l'envoie instantanément sur **Telegram** et déclenche un **buzzer**. Le système se pilote à distance depuis l'application Telegram grâce à un bot.

Ce projet montre comment l'IoT peut renforcer la sécurité d'un domicile ou d'un local professionnel avec du matériel peu coûteux.

## ✨ Fonctionnalités

- 📸 **Photo à la demande** : la commande `/capture_photo` prend une photo et l'envoie dans la conversation.
- 🚶 **Détection de mouvement** : en mode PIR, chaque mouvement détecté déclenche une photo et une alerte.
- 🔔 **Alarme sonore** : un buzzer retentit à chaque détection.
- 🔒 **Accès restreint** : seul l'identifiant de chat autorisé peut piloter la caméra ; les autres utilisateurs sont refusés.
- 💾 **Paramètres en EEPROM** : l'état du mode PIR est enregistré dans la mémoire de l'ESP32.
- 📶 **Robustesse** : redémarrage automatique si le Wi-Fi ne se connecte pas en 20 secondes ou si la caméra ne s'initialise pas.
- ⏱️ **Stabilisation du capteur** : période de 30 secondes au démarrage pour éviter les fausses détections, avec notification Telegram une fois terminée.

## 🛠️ Matériel

| Composant | Rôle |
|---|---|
| ESP32-CAM (avec PSRAM recommandée) | Microcontrôleur, Wi-Fi et caméra OV2640 |
| Capteur PIR (HC-SR501 ou équivalent) | Détection de mouvement |
| Buzzer actif ou passif | Alarme sonore |
| Programmateur USB-TTL (FTDI) ou carte ESP32-CAM-MB | Téléversement du programme |
| Alimentation 5 V stable (≥ 2 A conseillés) | Alimentation du montage |

### Câblage

| Composant | Broche ESP32-CAM |
|---|---|
| Sortie du capteur PIR | GPIO 12 |
| Buzzer (+) | GPIO 13 |
| LED flash (intégrée) | GPIO 4 |
| VCC du PIR et du buzzer | 5 V |
| GND | GND |

> ⚠️ GPIO 12 est une broche de démarrage de l'ESP32. Si l'ESP32-CAM refuse de démarrer quand le PIR est branché, débranche le capteur pendant le démarrage ou utilise une autre broche libre.

## 💻 Logiciel et bibliothèques

- [Arduino IDE](https://www.arduino.cc/en/software) avec le support des cartes **ESP32** (gestionnaire de cartes Espressif)
- [UniversalTelegramBot](https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot)
- [ArduinoJson](https://arduinojson.org/)
- `esp_camera`, `WiFi`, `WiFiClientSecure` et `EEPROM` (inclus avec le support ESP32)

## 🚀 Installation

### 1. Créer le bot Telegram

1. Dans Telegram, ouvre une conversation avec **@BotFather** et envoie `/newbot`.
2. Choisis un nom ; BotFather te donne un **token**.
3. Ouvre une conversation avec **@userinfobot** pour obtenir ton **ID de chat**.

### 2. Configurer le programme

Renseigne tes identifiants en haut de `sys.ino` :

```cpp
const char* ssid     = "NOM_DU_WIFI";
const char* password = "MOT_DE_PASSE_WIFI";
String BOTtoken      = "TOKEN_DU_BOT";
String CHAT_ID       = "TON_ID_DE_CHAT";
```

> 🔐 **Ne publie jamais ces valeurs sur GitHub.** Le plus sûr est de les placer dans un fichier `secrets.h` inclus par `sys.ino` et ajouté au `.gitignore`.

### 3. Téléverser

1. Dans l'Arduino IDE, sélectionne la carte **AI Thinker ESP32-CAM** et le bon port.
2. Relie GPIO 0 à GND (mode programmation), puis appuie sur RESET.
3. Téléverse le programme, retire le pont GPIO 0 / GND et appuie de nouveau sur RESET.
4. Ouvre le moniteur série à **115200 bauds** pour suivre la connexion.

## 📱 Commandes du bot

| Commande | Action |
|---|---|
| `/start` | Affiche les commandes disponibles et l'état des paramètres |
| `/capture_photo` | Prend une photo et l'envoie |
| `/enable_capture_Photo_with_PIR` | Active la détection de mouvement |
| `/disable_capture_Photo_with_PIR` | Désactive la détection de mouvement |

## ⚙️ Fonctionnement

```
Démarrage → connexion Wi-Fi → initialisation caméra → stabilisation PIR (30 s)
                                                              │
        ┌─────────────────────────────────────────────────────┤
        ▼                                                     ▼
Commande Telegram reçue                          Mouvement détecté (mode PIR actif)
        │                                                     │
        ▼                                                     ▼
Photo prise → envoi à Telegram          Photo prise → envoi à Telegram → buzzer
```

- La résolution est choisie automatiquement : **UXGA** si la PSRAM est présente, **SVGA** sinon.
- Le bot vérifie les nouveaux messages toutes les secondes, ou toutes les 20 secondes en mode PIR pour laisser la priorité à la détection.

## 🧭 Limites connues et pistes d'amélioration

- Le brochage de la caméra défini dans le code correspond à l'**ESP32 WROVER-KIT** ; il doit être remplacé par celui de l'**AI-Thinker** si tu utilises ce modèle.
- Le mode PIR est remis à `OFF` à chaque démarrage, ce qui annule l'intérêt de l'EEPROM.
- Aucun délai entre deux détections : un mouvement continu envoie des photos en rafale.
- Pistes : enregistrement sur carte microSD, flux vidéo en direct, plages horaires d'activation, boîtier imprimé en 3D.

## 👤 Auteur

**Edem Agbagno** — [github.com/edemAgbagno](https://github.com/edemAgbagno)
