<p align="center">
  <img src="images/ARMOR_BANNER.svg" alt="ARMOR-RADAR banner" width="100%">
</p>

# 📡 ARMOR-RADAR

<p align="center">
  <a href="README.md">🇺🇸 English</a> |
  <a href="README_spa.md">🇪🇸 Español</a> |
  🇫🇷 <b>Français</b> |
  <a href="README_ita.md">🇮🇹 Italiano</a> |
  <a href="README_deu.md">🇩🇪 Deutsch</a> |
  <a href="README_zho.md">🇨🇳 简体中文</a> |
  <a href="README_jpn.md">🇯🇵 日本語</a> |
  <a href="README_por.md">🇵🇹 Português</a>
</p>

### Firmware de nœud de terrain (Waveshare ESP32-S3-ETH, trois radars ou capteurs de présence, Ethernet et Wi-Fi) avec son propre panneau web, et son cœur testé sur ordinateur

<p align="center">
  <img src="https://img.shields.io/badge/License-GPL%203.0-blue.svg" alt="GPL 3.0">
  <img src="https://img.shields.io/badge/Language-C%2B%2B17-00599c.svg" alt="Language">
  <img src="https://img.shields.io/badge/Target-ESP32--S3-e7352c.svg" alt="Target">
  <img src="https://img.shields.io/badge/Version-0.7.9-0f6cbd.svg" alt="Version">
  <img src="https://img.shields.io/badge/Maturity-established-2EA44F.svg" alt="Maturity">
</p>

---

**Vérification d'honnêteté - ce qui fonctionne aujourd'hui:** Le nœud radar est le seul nœud qui existe en matériel : son firmware tourne sur une vraie carte Waveshare ESP32-S3-ETH avec un radar et rapporte au banc de test CM5, et son panneau web et ses mises à jour par radio y sont utilisés. Le cœur indépendant du matériel (2,266 vérifications dans 9 programmes de test : les décodeurs du LD2450, du LD2461 et de quatre capteurs de présence, le canal de commande du LD2450, les réglages et leurs contrôles, la table des broches, utilisateurs et sessions, le plan réseau, le sérialiseur de messages dont la sortie est acceptée par ARMOR-COMMON, et les contrôles de version et de signature) est testé sur un ordinateur, le **panneau web** a été exercé dans un vrai navigateur contre un nœud factice et l'**image du firmware se compile** dans le conteneur ESP-IDF 5.4. Ce qui n'est **pas** encore prouvé : les décodeurs du LD2461 et des capteurs de présence ne correspondent qu'aux exemples détaillés de leurs manuels, sans module derrière ; le contrôle de signature des mises à jour et l'appairage Bluetooth avec PIN sont prouvés sur ordinateur avec de vraies signatures mais pas encore sur la carte ; le canal de commande du radar n'est pas vérifié contre un module ; et le certificat HTTPS du panneau (fait par le nœud lui-même) n'est pas essayé dans tous les navigateurs.

---

## 📋 En un coup d'œil

| | |
|---|---|
| Version | `0.7.9` |
| Maturité | Établi |
| Tests | 2 266 vérifications dans 9 programmes (`ctest`) |
| Technologie | ESP-IDF C++ |
| Sur matériel réel | Tourne sur une vraie carte Waveshare ESP32-S3-ETH avec un radar, en rapportant au banc de test CM5 |

## 🎯 Présentation

* **Deux cartes, un firmware :** la Waveshare ESP32-S3-ETH (Ethernet, par défaut) et une ESP32-S3-WROOM-1 N16R8 sans Ethernet (Wi-Fi seul : sa configuration demande le réseau Wi-Fi à rejoindre et le nœud garde toujours son propre réseau). L'image se choisit à la compilation (`tools/build_node.sh generic s3-eth` ou `generic s3-wifi`) ; la table des broches et l'accès suivent la carte, le panneau masque ce que la carte n'a pas, et une image ne vaut que pour sa carte. Les deux compilent et sont testées sur ordinateur ; aucune n'a tourné sur une carte.
* **Un nœud couvre jusqu'à 270 degrés :** chaque Waveshare ESP32-S3-ETH lit jusqu'à trois radars LD2450 sur ses trois UART, montés à 75 degrés l'un de l'autre, par Ethernet filaire (W5500) en DHCP ou adresse fixe, alimenté en PoE ou USB. *Add a 270° node* de Studio crée les trois radars déjà associés au nœud.
* **Un panneau web sur chaque nœud,** à l'apparence de Studio et en huit langues, intégré au firmware : vue d'ensemble, réseau, Wi-Fi, broker, radars, broches, utilisateurs, mise à jour du firmware et journal. Un nœud sans utilisateur ouvre le Wi-Fi `ARMOR-SETUP-xxxxxx` et crée son premier administrateur avec un code d'installation ; les mots de passe sont en PBKDF2 salé, les sessions sont des jetons aléatoires et tous les réglages sont dans la flash du nœud, si bien qu'une image sert tous les nœuds et qu'aucun mot de passe n'est compilé ([le panneau](docs/NODE_PANEL.md)).
* **Un seul Wi-Fi à partir de plusieurs nœuds :** chaque nœud peut offrir un point d'accès relié à son port Ethernet ; avec le même nom et mot de passe et le canal en automatique (1, 6 ou 11 selon la MAC), téléphones et capteurs Wi-Fi voient un seul réseau avec un seul serveur DHCP. Ce n'est pas un maillage radio : chaque nœud garde son câble. Un nœud peut aussi rejoindre le Wi-Fi d'un routeur (avec recherche de réseaux), et un nœud sans câble peut être configuré en **Bluetooth** depuis l'application Android.
* **Broches pour le serveur :** toute broche libre devient une entrée, une sortie (avec un état sûr si le broker est perdu), du PWM ou une lecture analogique, et apparaît comme un appareil du serveur : un relais ou un contact n'exige aucun nouveau firmware. Les broches réservées de la carte ne sont jamais proposées.
* **Mises à jour par radio (OTA) avec retour arrière** depuis le panneau (deux emplacements sur la flash de 16 Mo), et un **lien depuis Studio** vers le panneau de chaque nœud, à partir de l'adresse que le nœud publie.
* **Le panneau en HTTPS :** le nœud fabrique son propre certificat (auto-signé, conservé en flash) et sert le panneau sur le port 443 en plus du 80, ou seulement sur le 443 ; le cookie de session est marqué Secure et l'empreinte du certificat s'affiche pour la comparer à l'avertissement du navigateur. **Identités de pistes stables :** les cibles de chaque trame sont associées à celles suivies auparavant ; une personne garde donc le même identifiant, une trame perdue ne fait pas clignoter et les positions sont un peu lissées.
* **Six modèles de capteurs, un par port :** un port porte un LD2450 ou un LD2461 (traqueurs qui alimentent le périmètre, avec le champ propre du modèle dans Studio) ou un capteur de présence (LD2410, LD2412, LD2410S, MR24HPC1) qui devient un appareil du serveur et publie présence et distance. Choisi dans le panneau ; ce qu'est chacun, son protocole et ce qui n'a pas été vérifié sont dans `docs/SENSORS.md`.
* **Décodeur, santé et configuration du LD2450 :** un découpeur qui se resynchronise trouve les trames dans un flux bruité ; chaque trame de 30 octets donne jusqu'à trois cibles qui deviennent des pistes du contrat ; la console et le panneau indiquent par radar s'il émet, se tait ou envoie des données illisibles. Le panneau peut aussi lire la version du module, choisir une ou trois cibles et définir des zones de détection (un protocole non vérifié face à un module).
* **Lumière ambiante et messages conformes au contrat :** le VEML7700 avec plage automatique ; JSON de télémétrie, de santé et d'information qui suit les schémas publiés et refuse d'écrire quoi que ce soit d'invalide, avec des horodatages d'horloge murale (SNTP) et un testament MQTT. La télémétrie est retenue tant qu'aucun radar n'émet, jamais un « tout est calme » vide.
* **Une image pour toutes les cartes, comme un produit réseau :** le firmware est le même et l'adresse MAC distingue les cartes (`armor-` et six chiffres jusqu'au nommage) ; `adopt_node.py` donne à un nœud fraîchement flashé son administrateur, son identité de broker et les réglages communs de la flotte par le réseau, avec un code d'installation calculé depuis sa MAC : 5 nœuds ou 27, c'est le même travail. **Outils de banc :** flashage par USB-C, un nœud simulé pour travailler sur le panneau et un convertisseur d'un journal de trames brutes en jeu de test ([mise en service sur banc](docs/BENCH_BRINGUP.md)).
* **Mises à jour par radio signées :** le nœud n'installe depuis GitHub qu'une image dont le SHA-256 correspond et qui porte une signature Ed25519 valide du responsable (la clé publique est dans le firmware, la privée ne quitte jamais le responsable) ; un fichier envoyé à la main par un administrateur dans le panneau reste accepté. Les pages et messages du panneau sont en huit langues.
* **Appairage Bluetooth avec un PIN :** le PIN à six chiffres est dérivé du code de configuration, imprimé sur la console USB et montré à un administrateur dans le panneau ; le téléphone doit le saisir avant de pouvoir écrire les réglages du nœud.
* **Seule la maison peut utiliser le panneau :** le panneau répond par défaut au réseau privé (ou aux réseaux que son propriétaire indique, comme un VPN), refuse une liste qui exclurait celui qui l'enregistre, compte les mauvais mots de passe par personne et par adresse, et fonctionne derrière le proxy du serveur pour être atteint depuis l'extérieur sans publier le nœud.
* **Liaison chiffrée facultative avec le broker (TLS) :** désactivée par défaut, le nœud parle donc MQTT non chiffré comme avant. Un interrupteur et une zone pour le certificat de l'autorité propre à l'installation (PEM, 2 Ko au plus) dans la page Broker lui font se connecter en `mqtts://`, ne faire confiance qu'à cette autorité et vérifier que le certificat du broker a été établi pour l'adresse atteinte ; ce qui n'est pas un seul certificat PEM est refusé. Couvert par des tests sur ordinateur ; la poignée de main n'a pas encore été exécutée sur une carte.
* **Rapport de santé :** environ une fois par minute le nœud publie depuis combien de temps il tourne et pourquoi il a redémarré la dernière fois, sa mémoire libre, sa liaison et son signal Wi-Fi, combien de fois il s'est connecté au broker et la dernière erreur notée (`armor/health/<nœud>`, le contrat `node_health` d'ARMOR-COMMON). Le code qui le construit est partagé par tous les nœuds et testé sur un ordinateur ; il n'a pas tourné sur une carte.
* **Commande depuis le serveur :** Studio peut redémarrer le nœud proprement, le mettre en pause jusqu'à une heure (il reprend seul), mettre son travail en veille (sans fin, conservée aux redémarrages, le nœud reste connecté et continue de répondre), le faire se signaler, demander un instantané de diagnostic et le marquer en maintenance (`armor/control/<nœud>/...`, le contrat `node_control` d'ARMOR-COMMON). Chaque ordre se fait en deux temps avec un jeton à usage unique propre à cette opération, seul le serveur peut les écrire sur le broker, et il n'existe ni réinitialisation d'usine, ni effacement, ni sommeil profond. Tant que le nœud est en pause ou en veille, il ne publie ni cibles ni présence (le battement dit encore qu'il est là). Le code est partagé par tous les nœuds et testé sur un ordinateur ; il n'a pas tourné sur une carte.
* **Un moteur de périmètre sur le nœud (désactivé par défaut) :** six zones de détection et trois zones d'exclusion, chacune un rectangle qu'on peut tourner d'un angle, trois lignes de franchissement qui comptent les passages par sens, trois chemins d'entrée, immunité aux animaux et détection de masquage. Il tourne à côté des capteurs, donc il fonctionne sans accès au broker, garde ses événements tant que le broker est absent et les publie sur `armor/radar/<nœud>/events` ; l'éditeur de zones est dans le panneau du nœud. Rien de tout cela n'a encore tourné avec un capteur sur une carte.
* **Qui est chaque capteur, et son propre Bluetooth fermé :** au démarrage, le nœud lit la version du micrologiciel et l'adresse Bluetooth de chaque capteur, ferme par défaut le Bluetooth propre des capteurs (il ne demande aucun code PIN) et peut confier la ligne série d'un capteur à l'outil PC d'un administrateur pendant quelques minutes (un pont série transparent). `GET /metrics` pour les administrateurs et un export et import tout ou rien des réglages complètent l'ensemble.
* **Diagnostic sans câble :** après un démarrage qui n'était pas propre (plantage, chien de garde, chute d'alimentation, allocation échouée), le rapport de santé en donne la raison pendant dix minutes ; un plantage laisse son vidage mémoire (core dump) à consulter, télécharger ou effacer depuis le panneau ; les réglages enregistrés sont copiés avant chaque installation de micrologiciel et récupérés si une mise à jour les abîme ; et un administrateur peut demander au nœud, via le serveur, un paquet de diagnostic ou son journal avec un jeton à usage unique.

## 🖼️ Captures d'écran

Capturées en 1920×1080 sur un banc d'essai, avec des données d'exemple et sans installation réelle derrière.

<table>
<tr><td width="50%" valign="top"><img src="images/screenshots/overview.png" alt="Résumé : l'état du nœud en un coup d'œil - réseau, broker, firmware et ce qu'il remonte." width="100%"><br><sub>Résumé : l'état du nœud en un coup d'œil - réseau, broker, firmware et ce qu'il remonte.</sub></td><td width="50%" valign="top"><img src="images/screenshots/radars.png" alt="Radars : chaque radar du nœud, son modèle, ses broches et son étalonnage." width="100%"><br><sub>Radars : chaque radar du nœud, son modèle, ses broches et son étalonnage.</sub></td></tr>
<tr><td width="50%" valign="top"><img src="images/screenshots/pins.png" alt="Broches : ce que fait chaque broche, vérifié pour que deux fonctions n'en partagent jamais une." width="100%"><br><sub>Broches : ce que fait chaque broche, vérifié pour que deux fonctions n'en partagent jamais une.</sub></td><td width="50%" valign="top"><img src="images/screenshots/network.png" alt="Réseau : l'adressage Ethernet ou Wi-Fi du nœud, réglé depuis son propre panneau web." width="100%"><br><sub>Réseau : l'adressage Ethernet ou Wi-Fi du nœud, réglé depuis son propre panneau web.</sub></td></tr>
</table>

## 🧩 Rôle dans A.R.M.O.R.

ARMOR-RADAR est l'œil du périmètre. Chaque nœud lit jusqu'à trois radars ou capteurs de présence et publie pistes, lumière et santé vers [ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER) en MQTT ; [ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO) les dessine sur le plan du site et [ARMOR-SERVER-AI](https://github.com/JuanenRac/ARMOR-SERVER-AI) les pèse face aux caméras. Son firmware partage une base avec les autres nœuds (le panneau, la configuration Bluetooth, les mises à jour signées, la connexion et l'horloge viennent de la base firmware d'[ARMOR-COMMON](https://github.com/JuanenRac/ARMOR-COMMON)), si bien que ce qu'on apprend sur un nœud vaut pour tous.

## 📂 Structure du dépôt

```text
ARMOR-RADAR/
├── main/
│   ├── app_main.cpp        start-up: settings, radars, pins, network, panel, broker, Bluetooth
│   ├── node_store.cpp      settings and users in flash
│   ├── network.cpp         Ethernet, Wi-Fi access point and bridge, station
│   ├── web_server.cpp      the panel and its JSON API, login, update
│   ├── api_shared.cpp      the operations the panel and Bluetooth share (status, settings, Wi-Fi scan)
│   ├── ble_provision.cpp   configuration from a phone over Bluetooth (NimBLE)
│   ├── radar_manager.cpp   UARTs, frames, health, command channel (armor_radar.cpp, radar_tracks.hpp)
│   ├── gpio_manager.cpp    the pins mapped for the server
│   ├── mqtt_link.cpp       clock, health, telemetry, information, pin topics
│   ├── board_ethernet.cpp, light_sensor.cpp, tls_cert.cpp, log_buffer.cpp, entropy.cpp
│   ├── Kconfig.projbuild   the first settings of a build
│   └── core/               framer, ld2450, ld2450_command, ld2461, presence, sensor_model, node_config, board_pins, network_plan, auth, ble_frame, ble_dispatch, gpio_logic, json, veml7700, telemetry_json...
├── panel/                  index.html, app.js, text.js (8 languages), style.css
├── tests/                  test_core.cpp, test_node.cpp, test_mqtt_tls.cpp, test_sensors.cpp, test_board_wifi.cpp, emit_samples.cpp, check_contract.py, test_tools.py
├── tools/                  build_node.sh, make_fleet.py, adopt_node.py, provision_node.sh, flash.bat, pack_panel.py, panel_mock.mjs, frames_to_fixture.py
├── secrets/                node.conf.example, fleet.example.json (the real files are git-ignored)
├── partitions.csv, sdkconfig.defaults, sdkconfig.board.*
└── docs/                   BENCH_BRINGUP.md, NODE_PANEL.md, BLE_PROVISIONING.md, SENSORS.md, HARDWARE_BOUNDARY.md
```

## 🛠️ Environnement de développement

**Prérequis:** un compilateur C++17 pour les tests sur l'ordinateur, le conteneur ESP-IDF 5.4 pour compiler l'image, un Waveshare ESP32-S3-ETH ou un ESP32-S3-WROOM-1 N16R8

```bash
cmake -S tests -B build/host && cmake --build build/host
build/host/test_core && build/host/test_node && build/host/test_sensors && build/host/test_board_wifi && build/host/test_release   # 2,266 checks, -Werror
build/host/emit_samples | python tests/check_contract.py
node tools/panel_mock.mjs --user admin:adminpass123   # the panel without a board
python tools/make_fleet.py                             # once: the fleet secret and the shared settings
tools/build_node.sh generic                           # ONE image for every Waveshare board (dist/generic-s3-eth.bin), in the ESP-IDF container
tools/build_node.sh generic s3-wifi                   # the same firmware for an ESP32-S3-WROOM-1 N16R8 with no Ethernet
```

```bat
tools\flash.bat generic COM5 monitor                  # each Waveshare board, once, by USB-C (add s3-wifi for the other board)
```

```bash
tools/adopt_node.py 192.168.0.181 --id perimetro-3 --fleet secrets/fleet.json --broker-ssh-host <cm5> --broker-ssh-user <user>
```

Les tests sur ordinateur demandent un compilateur C++17 (Linux, WSL, MSYS2). Voir la [mise en service sur banc](docs/BENCH_BRINGUP.md), le [panneau](docs/NODE_PANEL.md) et la [frontière matérielle](docs/HARDWARE_BOUNDARY.md).

## 🔗 Projets liés

**A.R.M.O.R.** (Autonomous Radar & Multimodal Observation Range) est un système de sécurité périmétrique composé de dépôts indépendants. Chacun a sa propre version, ses propres tests et son propre README ; voici la famille :

* **[ARMOR-COMMON](https://github.com/JuanenRac/ARMOR-COMMON)** - Contrats de messages, validateurs, vecteurs de conformité et types générés
* **ARMOR-RADAR** (ce dépôt) - Firmware du nœud de terrain pour ESP32-S3 avec trois radars et son propre panneau web
* **[ARMOR-SOLAR](https://github.com/JuanenRac/ARMOR-SOLAR)** - Protocoles des onduleurs et batteries solaires et messages d'un nœud passerelle
* **[ARMOR-ELECTRICAL](https://github.com/JuanenRac/ARMOR-ELECTRICAL)** - Nœud électrique : compteurs, le message des mesures du réseau et les règles de commutation
* **[ARMOR-ALARM](https://github.com/JuanenRac/ARMOR-ALARM)** - Nœud et centrale d'alarme : zones, armement, temporisations, sirène et PIN, avec ou sans le serveur
* **[ARMOR-HMI](https://github.com/JuanenRac/ARMOR-HMI)** - Panneau tactile : l'état du système sur un écran mural, armer et acquitter, et la maison de l'assistant vocal
* **[ARMOR-NETWORK](https://github.com/JuanenRac/ARMOR-NETWORK)** - Le réseau local : ses appareils, internet et ce qui change
* **[ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER)** - Coordinateur central : télémétrie, alarmes, appareils, relevés solaires et caméras
* **[ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO)** - Console web : caméras, radar, alarmes, énergie solaire et concepteur de site 2D/3D
* **[ARMOR-ANDROID-CONTROL](https://github.com/JuanenRac/ARMOR-ANDROID-CONTROL)** - Client Android de l'opérateur avec radar 2D/3D en direct
* **[ARMOR-SERVER-AI](https://github.com/JuanenRac/ARMOR-SERVER-AI)** - Politique d'inférence visuelle qui explique ses décisions et n'agit jamais
* **[ARMOR-VISION](https://github.com/JuanenRac/ARMOR-VISION)** - Zones de caméra, masques et règles de détection pour la Jetson Orin NX
* **[ARMOR-VOICE-AI](https://github.com/JuanenRac/ARMOR-VOICE-AI)** - Intentions vocales hors ligne avec une confirmation impossible à falsifier
* **[ARMOR-HARDWARE](https://github.com/JuanenRac/ARMOR-HARDWARE)** - Boîtiers, électronique et matrice d'acceptation sur banc
* **[ARMOR-DEVOPS](https://github.com/JuanenRac/ARMOR-DEVOPS)** - Déploiement, banc d'essai CM5, sauvegarde et TLS
* **[ARMOR-SIMULATOR](https://github.com/JuanenRac/ARMOR-SIMULATOR)** - Simulateur de télémétrie hors ligne avec des pannes reproductibles
* **[ARMOR-UPDATER](https://github.com/JuanenRac/ARMOR-UPDATER)** - Détecte, installe et met à jour les propres dépôts de l'écosystème
* **[ARMOR-DOCS](https://github.com/JuanenRac/ARMOR-DOCS)** - Architecture, base de sécurité et matrice des capacités

## 📚 Documentation et communauté

Pour en savoir plus :

* [Matrice des capacités : ce qui est prouvé et ce qui ne l'est pas](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/CAPABILITY_MATRIX.md)
* [Catalogue des projets : versions et dépendances entre les dépôts](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/PROJECT_CATALOG.md)
* [Historique des modifications de ce dépôt](CHANGELOG.md)
* [Licence (GPL-3.0-or-later)](LICENSE)
* Questions, idées et rapports : electrohobby3d@gmail.com

## 👤 AUTEUR

**JuanenRac (Electro Hobby 3D)** · electrohobby3d@gmail.com

## 📜 LICENCE

GPL-3.0-or-later - voir [LICENSE](LICENSE).
