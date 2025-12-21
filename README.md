# 🏠 MyHouse Client - Système de Monitoring Domotique

## 📋 Description

Client de monitoring domotique pour M5Stack CoreS3 permettant de surveiller et visualiser en temps réel l'état de différents équipements connectés d'une maison intelligente (lumière, chauffage, porte, température).

Ce projet implémente un **client ESP32** qui communique avec un serveur domotique pour :

- Afficher l'état des équipements sur un écran tactile
- Contrôler des LEDs physiques en fonction des états
- Diffuser l'écran en streaming via HTTP
- S'authentifier de manière sécurisée avec le serveur

## ✨ Fonctionnalités

### 🖥️ Interface Graphique

- **Interface moderne** avec cartes colorées et hiérarchie visuelle
- **Affichage temps réel** de 4 indicateurs :
  - 💡 Lumière (ON/OFF)
  - 🔥 Chauffage (ON/OFF)
  - 🚪 Porte (OPEN/CLOSE)
  - 🌡️ Température (en °C)
- **Indicateurs visuels** avec codes couleur (vert=actif, rouge=erreur, orange=allumé)

### 🔐 Authentification

- Appairage sécurisé avec le serveur via token
- ID unique de l'appareil : `F8C096E350CC`
- Autorisation via header HTTP : `ID:TOKEN`

### 📡 Monitoring Auto-Refresh

- **Watcher automatique** toutes les 3 secondes
- Activation/désactivation via bouton C
- Synchronisation des états entre serveur et client
- Indicateur visuel de rafraîchissement (pastille bleue)

### 💡 Contrôle LEDs Physiques

- **3 LEDs connectées** aux GPIO :
  - Pin 1 : LED Chauffage
  - Pin 3 : LED Lumière
  - Pin 16 : LED Porte
- **Synchronisation** état serveur ↔ LED physique

### 📺 Streaming Écran

- **Serveur Web intégré** sur port 80
- Capture d'écran en temps réel (format BMP)
- Page HTML avec auto-refresh toutes les 2 secondes
- Accessible via navigateur : `http://[IP_CLIENT]/`

## 🔧 Configuration Matérielle

### Matériel Requis

- **M5Stack CoreS3** (ESP32-S3)
- **3 LEDs** + résistances
- **Réseau WiFi** (SSID: `MyHouseOS`)

### Connexions LEDs

```
GPIO 1  → LED Chauffage (+ résistance 220Ω)
GPIO 3  → LED Lumière (+ résistance 220Ω)
GPIO 16 → LED Porte (+ résistance 220Ω)
GND     → Masse commune
```

## ⚙️ Configuration Logicielle

### Bibliothèques Nécessaires

```cpp
#include "M5CoreS3.h"      // Librairie M5Stack CoreS3
#include <WiFi.h>          // Connexion WiFi
#include <ArduinoJson.h>   // Parsing JSON
#include <HTTPClient.h>    // Requêtes HTTP
#include <WebServer.h>     // Serveur Web
```

### Paramètres Réseau

À modifier dans le code selon votre configuration :

```cpp
// WiFi
const char* ssid = "MyHouseOS";
const char* password = "12345678";

// Serveur domotique
const char* serverIP = "192.168.4.2";
const int serverPort = 3000;

// URLs API
const char* authUrl  = "http://192.168.4.1/link";
const char* meteoUrl = "http://192.168.4.1/temp";
const char* lightUrl = "http://192.168.4.2:3000/toggle/light";
const char* heatUrl  = "http://192.168.4.2:3000/toggle/heat";
const char* doorUrl  = "http://192.168.4.2:3000/toggle/door";
```

## 🚀 Installation

### 1. Prérequis

- [Arduino IDE](https://www.arduino.cc/en/software) ou PlatformIO
- [M5Stack Library](https://github.com/m5stack/M5CoreS3)
- [ArduinoJson](https://arduinojson.org/) (v7+)

### 2. Installation des Bibliothèques

Dans Arduino IDE :

```
Outils → Gérer les bibliothèques
Rechercher et installer :
- M5CoreS3
- ArduinoJson
```

### 3. Configuration

1. Ouvrir `Client.ino`
2. Modifier les paramètres WiFi (SSID/password)
3. Ajuster les URLs selon votre architecture réseau
4. Vérifier l'ID unique de l'appareil

### 4. Upload

1. Connecter le M5Stack CoreS3 via USB
2. Sélectionner le port COM approprié
3. Téléverser le code

## 📱 Utilisation

### Au Démarrage

1. Le M5Stack se connecte au WiFi
2. L'adresse IP du stream s'affiche pendant 4 secondes (notez-la !)
3. L'interface principale apparaît

### Boutons Physiques

| Bouton               | Action                                     |
| -------------------- | ------------------------------------------ |
| **A** (Gauche) | Lancer l'authentification avec le serveur  |
| **C** (Droite) | Activer/Désactiver le watcher automatique |

### Modes de Fonctionnement

#### Mode Pause (Défaut)

- Affichage statique des dernières valeurs
- LEDs éteintes
- Pas de rafraîchissement automatique
- Message : `PAUSE - Appuyez sur C`

#### Mode Watcher (Actif)

- Rafraîchissement automatique toutes les 3 secondes
- LEDs synchronisées avec les états
- Indicateur de refresh (pastille bleue)
- Message : `WATCHER ACTIF (Btn C)`

### Streaming Écran

1. Noter l'IP affichée au démarrage (ex: `192.168.4.5`)
2. Ouvrir un navigateur sur un autre appareil
3. Accéder à `http://[IP_CLIENT]/`
4. L'écran se rafraîchit automatiquement toutes les 2 secondes

## 🌐 API et Communication

### Authentification

**POST** `/link`

```json
Request:
{
  "id": "F8C096E350CC"
}

Response:
{
  "token": "abc123xyz"
}
```

### Requêtes Équipements

Toutes les requêtes incluent le header :

```
Authorization: F8C096E350CC:abc123xyz
```

**GET** `/toggle/light`

```json
{
  "light": "true"
}
```

**GET** `/toggle/heat`

```json
{
  "heat": "false"
}
```

**GET** `/toggle/door`

```json
{
  "door": "true"
}
```

**GET** `/temp`

```json
{
  "temp": "22.5"
}
```

## 🎨 Interface Utilisateur

### Palette de Couleurs

```cpp
Fond principal : #0f172a (Bleu nuit)
Cartes         : #1e293b (Gris ardoise)
Accent         : #6366f1 (Indigo)
Texte          : #f8fafc (Blanc cassé)
Sous-texte     : #94a3b8 (Gris clair)
Succès         : #22c55e (Vert)
Avertissement  : #fb923c (Orange)
Erreur         : #ef4444 (Rouge)
```

### Structure de l'Écran

```
┌─────────────────────────────────┐
│ MyHouse Client      [WiFi OK]   │ Header
├──────────────────┬──────────────┤
│  LUMIERE         │  CHAUFFAGE   │
│    ON/OFF        │    ON/OFF    │ Ligne 1
├──────────────────┼──────────────┤
│  PORTE           │  Température │
│  OPEN/CLOSE      │   22.5 C     │ Ligne 2
├─────────────────────────────────┤
│ WATCHER ACTIF (Btn C) / PAUSE   │ Footer
└─────────────────────────────────┘
```

## 🔄 Architecture et Flux de Données

```
┌─────────────┐      WiFi       ┌──────────────┐
│  M5Stack    │ ←───────────→   │   Serveur    │
│  Client     │   HTTP/JSON     │  Domotique   │
└─────────────┘                 └──────────────┘
      │
      │ GPIO
      ↓
┌─────────────┐
│  3x LEDs    │
│  Physiques  │
└─────────────┘
```

### Cycle de Rafraîchissement (3s)

1. **checkLight()** → Requête GET `/toggle/light` → Mise à jour UI + LED
2. **checkHeat()** → Requête GET `/toggle/heat` → Mise à jour UI + LED
3. **checkDoor()** → Requête GET `/toggle/door` → Mise à jour UI + LED
4. **getMeteoAPI()** → Requête GET `/temp` → Mise à jour UI

## 🐛 Dépannage

### Problème : Pas de connexion WiFi

- Vérifier le SSID et mot de passe
- Vérifier que le réseau est à portée
- Redémarrer le M5Stack

### Problème : Erreurs d'authentification

- Vérifier que le serveur est accessible
- Vérifier l'URL d'authentification
- Appuyer sur le bouton A pour réessayer

### Problème : LEDs ne s'allument pas

- Vérifier les connexions GPIO
- Vérifier les résistances (220Ω recommandé)
- Vérifier que le watcher est actif (bouton C)

### Problème : Stream ne fonctionne pas

- Vérifier l'IP affichée au démarrage
- Vérifier que le client et navigateur sont sur le même réseau
- Essayer d'accéder à `http://[IP]/capture` directement

### Problème : Données non actualisées

- Activer le watcher (bouton C)
- Vérifier la connexion au serveur
- Vérifier les URLs API dans le code

## 📝 Personnalisation

### Changer l'ID de l'Appareil

```cpp
doc["id"] = "VOTRE_NOUVEL_ID";
fullToken = "VOTRE_NOUVEL_ID:" + Token;
```

### Modifier l'Intervalle de Rafraîchissement

```cpp
const unsigned long watcherInterval = 5000; // 5 secondes au lieu de 3
```

### Changer les Pins LEDs

```cpp
const int PIN_LED_HEAT  = 2;  // Nouveau pin
const int PIN_LED_LIGHT = 4;
const int PIN_LED_DOOR  = 5;
```

## 📄 Licence

Projet open source - Libre d'utilisation et de modification.

## 👥 Auteur

Développé pour le M5Stack CoreS3 dans le cadre d'un projet de domotique connectée.

---

**Version :** 1.0
**Date :** Décembre 2025
**Plateforme :** M5Stack CoreS3 (ESP32-S3)
