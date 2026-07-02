# Choix d'implémentation et retour d'expérience

## Contexte — le firmware de référence sensor.community

Le firmware de référence de la communauté est **airrohr** (aussi appelé luftdaten firmware).
Il repose sur un ESP8266 ou ESP32 connecté directement au WiFi, qui lit ses capteurs
et envoie un POST JSON à `api.sensor.community` toutes les 145 secondes.

Contraintes du firmware airrohr :
- Nécessite une prise secteur à proximité du capteur (pas de batterie)
- Nécessite un réseau WiFi à portée du capteur
- Node et connectivité Internet sont au même endroit

SensorSensei adopte une **architecture découplée node/gateway** pour lever ces contraintes.

---

## 1. Architecture : node LoRa + gateway WiFi vs node WiFi direct

### Choix retenu

Séparer le node (batterie + LoRa) du gateway (USB + WiFi). Le node se contente
d'émettre un paquet LoRa ; c'est le gateway qui gère la connectivité Internet.

### Justification

| Critère              | airrohr (WiFi direct)       | SensorSensei (LoRa + gateway)        |
|----------------------|-----------------------------|--------------------------------------|
| Alimentation node    | Secteur obligatoire         | Batterie LiPo possible               |
| Portée               | ~50 m (WiFi intérieur)      | ~2 km en extérieur (LoRa SF9)        |
| Contrainte réseau    | WiFi à portée du capteur    | WiFi uniquement au niveau du gateway |
| Nb de nodes          | 1 connexion WiFi par node   | N nodes → 1 seul gateway             |
| Consommation TX      | ~200 mA (WiFi)              | ~40 mA (LoRa TX)                     |

Un seul gateway peut servir plusieurs nodes. Le capteur peut être placé à l'extérieur,
en hauteur, ou dans une zone sans WiFi, tant qu'il est dans la portée LoRa du gateway.

### Retour d'expérience

Un seul node a été testé lors du développement. Les tests se sont limités à environ 5–10 mètres en intérieur — les données étaient bien reçues et transmises à sensor.community sans perte de paquets. Aucun problème de communication LoRa n'a été rencontré sur cette distance.

---

## 2. Protocole radio : LoRa (RadioLib) vs WiFi

### Choix retenu

LoRa 868 MHz avec RadioLib, SF9/BW125/CR4-7.

### Justification

**Pourquoi LoRa plutôt que WiFi pour le node :**
- Consommation TX ~5× inférieure au WiFi (40 mA vs 200 mA)
- Portée ~40× supérieure en extérieur
- Le stack WiFi ESP32 prend ~300 ms à se réinitialiser après deep sleep — la radio LoRa
  est opérationnelle en quelques dizaines de ms
- Pas besoin de gérer DHCP, DNS, TLS côté node

**Paramètres LoRa choisis (SF9/125 kHz/CR4-7) :**
- SF9 : bon compromis portée (~2 km) / time-on-air (~300 ms pour 10 bytes)
- SF12 aurait donné plus de portée mais un time-on-air de ~2.5 s, ce qui réduit
  l'autonomie et augmente le risque de collision si plusieurs nodes émettent
- Sync word 0x12 : réseau privé, évite les collisions avec les nœuds TTN (0x34)

### Retour d'expérience

Aucun test de portée longue distance n'a été réalisé. Les tests se sont limités à une dizaine de mètres en intérieur pour vérifier que les données étaient bien reçues, ce qui était le cas. La communication LoRa a fonctionné très rapidement dès la première mise en place, sans nécessiter d'ajustements des paramètres SF/BW.

La trame LoRa a ensuite été durcie avec une version, un `seq`, des `flags`, un CRC16 et un jitter aléatoire avant émission. Il n'y a pas d'ACK applicatif : le lien reste unidirectionnel et la détection de pertes se fait côté gateway via le `seq`.

---

## 3. Bibliothèque LoRa : RadioLib vs arduino-LoRa

### Choix retenu

RadioLib (jgromes/RadioLib ^6.6.0).

### Justification

| Critère                  | arduino-LoRa        | RadioLib                     |
|--------------------------|---------------------|------------------------------|
| Maintenance              | Peu active          | Activement maintenue (2024)  |
| Compatibilité SX1262     | Non                 | Oui                          |
| Mode réception async     | Polling uniquement  | Interruption DIO1 (ISR)      |
| API deep sleep           | Basique             | `radio.sleep()` propre       |
| Gestion des erreurs      | Limitée             | Codes d'erreur détaillés     |

La Heltec V3 embarque un **SX1262**, non supporté par arduino-LoRa. RadioLib supporte
les deux puces (SX1276 côté node, SX1262 côté gateway) avec la même API.

La réception par interruption ISR sur DIO1 (gateway) évite le busy-polling et permet
au gateway de traiter d'autres tâches entre deux paquets.

### Retour d'expérience

Aucun problème rencontré avec RadioLib. L'intégration s'est faite rapidement sur les deux cartes (SX1276 et SX1262) avec la même API.

---

## 4. Gestion de l'énergie : deep sleep avec AXP2101

### Choix retenu

Deep sleep ESP32 avec réveil par timer toutes les 5 minutes. Coupure du SX1276
via le PMIC AXP2101 (rail ALDO3) pendant le sommeil.

### Justification

**Pourquoi tout mettre dans `setup()` :**
Le deep sleep redémarre l'ESP32 depuis le début à chaque réveil. Placer la logique
dans `setup()` (et non `loop()`) garantit un cycle propre : init → mesure → TX → sleep,
sans état résiduel entre deux cycles.

**Pourquoi couper ALDO3 (SX1276) :**
Le SX1276 en mode sleep logiciel consomme encore ~1 µA. En coupant le rail ALDO3
via le PMIC, la consommation tombe à 0 pendant le deep sleep. Sur 5 minutes de cycle
dont ~3 secondes actives, c'est la différence entre quelques jours et plusieurs semaines
d'autonomie.

**Pourquoi couper ALDO4 (GPS) :**
Le T-Beam embarque un module GPS (non utilisé dans ce projet) qui consomme ~30 mA
en continu. Le désactiver systématiquement au démarrage évite de vider la batterie
sans raison.

**Pourquoi stocker le device_id en RTC RAM :**
Lire le MAC depuis l'eFuse implique un accès au bloc eFuse de l'ESP32 (opération
relativement lente). La RTC RAM survit au deep sleep et permet de ne faire cette
lecture qu'une seule fois (premier boot), puis de la réutiliser à chaque réveil.

**Intervalle de 5 minutes :**
sensor.community n'accepte qu'une mise à jour par station toutes les 145 secondes
(~2.5 min). 5 minutes donne une marge confortable et réduit le duty cycle radio,
respectant ainsi la réglementation EU (< 1% duty cycle sur 868 MHz).

### Estimation d'autonomie (théorique)

| Phase          | Durée     | Courant estimé | Énergie         |
|----------------|-----------|----------------|-----------------|
| Actif (mesure + TX) | ~3 s | ~120 mA        | ~100 µAh/cycle  |
| Deep sleep     | ~297 s    | ~15 µA         | ~1.2 µAh/cycle  |
| **Total/cycle**| 300 s     |                | **~101 µAh**    |

Avec une batterie LiPo 2000 mAh : ~20 000 cycles ≈ **~69 jours** (théorique,
sans compter l'autodécharge et l'efficacité du régulateur).

### Retour d'expérience

Pas de mesure d'autonomie réalisée sur la durée du projet. Le deep sleep fonctionne correctement — le node se réveille, effectue sa mesure et se rendort sans problème observé.

---

## 5. Format de payload : binaire 10 bytes vs JSON

### Choix retenu

Payload métier binaire big-endian de 10 bytes, encapsulé dans une trame LoRa de 16 bytes (voir `shared/payload.h` et `shared/lora_protocol.h`).

### Justification

Le firmware airrohr envoie du JSON directement sur WiFi — la taille du payload
n'a pas d'importance. Sur LoRa, chaque byte supplémentaire allonge le time-on-air
et consomme de la batterie.

| Format           | Taille    | Time-on-air SF9/125kHz |
|------------------|-----------|------------------------|
| JSON complet     | ~180 bytes | ~2.5 s                |
| Payload binaire  | 10 bytes   | ~300 ms               |
| Trame LoRa       | 16 bytes   | ~450 ms               |

10 bytes de payload métier restent bien sous la limite pratique de 51 bytes pour SF9/125 kHz.
La trame complète de 16 bytes laisse encore une marge confortable pour ajouter des champs
de protocole sans exploser le time-on-air.

Les floats IEEE 754 (4 bytes chacun) sont évités : température encodée en int16 ×100
(2 bytes), pression en uint16 entier (2 bytes), PM2.5 en uint16 ×10 (2 bytes).
La précision perdue est négligeable pour ces usages (0.01°C, 1 hPa, 0.1 µg/m³).

### Retour d'expérience

Pas de problème d'encodage observé. Les données arrivent correctement sur sensor.community. La configuration de la station a néanmoins demandé du temps : le format attendu par l'API (notamment la distinction pin 1 / pin 11 et l'unité de pression en Pa et non en hPa) n'est pas trivial à retrouver dans la documentation sensor.community.

---

## 6. Capteur de poussière : GP2Y1010AU0F vs SDS011

### Choix retenu

GP2Y1010AU0F (Sharp/Waveshare) — capteur optique analogique.

### Justification

| Critère          | SDS011 (référence airrohr) | GP2Y1010AU0F            |
|------------------|---------------------------|-------------------------|
| Interface        | UART                       | Analogique (ADC)        |
| Consommation     | ~70 mA actif               | ~20 mA actif            |
| Prix             | ~15–20 €                   | ~3–5 €                  |
| Précision        | Bonne (laser)              | Correcte (IR)           |
| Distintion PM10/PM2.5 | Oui                  | Non (même valeur)       |
| Compatibilité deep sleep | Démarrage lent (fan) | Rapide (~1.5 s warmup) |

Le GP2Y1010 ne distingue pas PM10 et PM2.5. Le gateway envoie la même valeur pour
les deux champs (`P1` et `P2`) dans le format SDS011 de sensor.community.
C'est un compromis acceptable pour un projet DIY sur batterie.

L'ADC 12 bits de l'ESP32 présente une non-linéarité connue (~3–4% d'erreur).
Une moyenne glissante sur 5 échantillons (`DUST_FILTER_SIZE`) réduit le bruit ADC.

### Retour d'expérience

Le GP2Y1010 s'est révélé le point le plus difficile du projet. Un des capteurs testés ne fonctionnait pas normalement, ce qui a compliqué le développement. Même avec un capteur fonctionnel, les valeurs PM2.5 varient beaucoup trop en pratique — le signal ADC est bruité et la moyenne glissante ne suffit pas à stabiliser les mesures. Ce capteur est acceptable pour détecter de grandes tendances mais pas pour des valeurs précises.

---

## 7. Pattern gatekeeper : validation avant transmission

### Choix retenu

Toute payload LoRa reçue par le gateway passe par un **gatekeeper** avant d'être
transmise à sensor.community. Le gatekeeper applique deux vérifications :
1. Whitelist des `device_id` autorisés
2. Plages de valeurs physiquement plausibles (limites opérationnelles des capteurs)

### Justification

Le firmware airrohr n'a pas ce problème : le node est directement sur WiFi, il
contrôle lui-même ce qu'il envoie. Dans une architecture LoRa, le gateway reçoit
des paquets radio qui peuvent venir de **n'importe qui** sur la même fréquence
et le même sync word.

Sans gatekeeper :
- Un autre node mal configuré (ou malveillant) partageant la fréquence 868 MHz
  et le sync word `0x12` pourrait injecter des fausses données dans la station
  sensor.community du gateway.
- Un capteur mal câblé ou en train de lâcher enverrait des valeurs aberrantes
  (0 hPa, 999°C) qui pollueraient les données communautaires.

Le gatekeeper est **le seul endroit** où ces deux vérifications sont faites.
Le node n'a pas à s'en préoccuper, et sensor.community reçoit uniquement
des données valides et autorisées.

### Mode dev vs prod

```cpp
// gatekeeper.h — whitelist vide = accepte tout (mode dev)
static const uint32_t AUTHORIZED_DEVICES[] = {
    // 0x1234ABCD,
};
```

Si `AUTHORIZED_DEVICES` est vide, le gatekeeper laisse passer tous les nodes.
Ce comportement est intentionnel pour faciliter le développement (pas besoin
de connaître le device_id à l'avance). En production, remplir la liste avec
les IDs des T-Beams enregistrés.

### Retour d'expérience

La whitelist est restée vide en mode dev tout au long du projet (1 seul node testé). Les plages de validation se sont révélées utiles indirectement : les valeurs aberrantes du capteur de poussière défaillant auraient pu polluer sensor.community sans ce filtre.

---

## 8. Ce qui reste à améliorer

**Qualité du code :** la qualité du code ne nous satisfait pas entièrement. Le projet a évolué par itérations rapides et certaines parties mériteraient une refactorisation pour être plus propres et maintenables.

**Capteur de son (SPH0645) :** l'intégration du microphone I2S a posé des difficultés et n'a pas abouti à une implémentation stable. Ce capteur a finalement été écarté du firmware final. L'ajout d'un niveau sonore dans la payload reste un objectif non atteint.

**Stabilité du capteur de poussière :** les valeurs PM2.5 varient trop pour être réellement exploitables. Un SDS011 (UART, laser) donnerait des mesures beaucoup plus fiables au prix d'une consommation plus élevée.

Pistes techniques identifiées :
- Ajouter un accusé de réception (ACK) du gateway vers le node si l'on passe à un lien bidirectionnel
- Intégrer l'humidité (BME280 à la place du BMP280) — sensor.community accepte ce champ
- Chiffrement du payload (AES-128) pour éviter l'usurpation de device_id
- Remplir la whitelist `AUTHORIZED_DEVICES` en production
- Calibration de l'ADC ESP32 avec `esp_adc_cal` pour améliorer la précision PM2.5
