<p align="center">
  <img src="images/ARMOR_BANNER.svg" alt="Bannière d'ARMOR-NODE-RADAR" width="100%">
</p>

# 📡 ARMOR-NODE-RADAR

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
  <img src="https://img.shields.io/badge/License-GPL%203.0--or--later-blue.svg" alt="GPL 3.0 ou ultérieure">
  <img src="https://img.shields.io/badge/Language-C%2B%2B17-00599c.svg" alt="Langage">
  <img src="https://img.shields.io/badge/Target-ESP32--S3-e7352c.svg" alt="Target">
  <img src="https://img.shields.io/badge/Tests-7371%20checks-2ea44f.svg" alt="Tests">
  <img src="https://img.shields.io/badge/Version-0.8.0.7-0f6cbd.svg" alt="Version">
  <img src="https://img.shields.io/badge/Maturity-established-2EA44F.svg" alt="Maturité">
</p>

---

**Vérification d'honnêteté - ce qui fonctionne aujourd'hui :** Le nœud radar est le seul nœud qui existe en matériel : son firmware tourne sur une vraie carte Waveshare ESP32-S3-ETH avec un radar et rapporte au banc de test CM5, et son panneau web et ses mises à jour par radio y sont utilisés. Le cœur indépendant du matériel (4,633 vérifications dans 14 programmes de test : les décodeurs du LD2450, du LD2461 et de quatre capteurs de présence, le canal de commande du LD2450, les réglages et leurs contrôles, la table des broches, utilisateurs et sessions, le plan réseau, le sérialiseur de messages dont la sortie est acceptée par ARMOR-COMMON, et les contrôles de version et de signature) est testé sur un ordinateur, le **panneau web** a été exercé dans un vrai navigateur contre un nœud factice et l'**image du firmware se compile** dans le conteneur ESP-IDF 5.5.5. Ce qui n'est **pas** encore prouvé : les décodeurs du LD2461 et des capteurs de présence ne correspondent qu'aux exemples détaillés de leurs manuels, sans module derrière ; le contrôle de signature des mises à jour et l'appairage Bluetooth avec PIN sont prouvés sur ordinateur avec de vraies signatures mais pas encore sur la carte ; le canal de commande du radar n'est pas vérifié contre un module ; et le certificat HTTPS du panneau (fait par le nœud lui-même) n'est pas essayé dans tous les navigateurs.

---

## 📋 En un coup d'œil

| | |
|---|---|
| Version | `0.8.0.7` |
| Maturité | Établi |
| Rôle | Nœud de terrain radar pour ESP32-S3 (famille : Field Nodes, parent : ARMOR-COMMON) |
| Technologie | C++17, ESP-IDF 5.5.5, ESP32-S3 |
| Tests | 7371 vérifications dans 14 programmes hôte et 6 contrôles Python (`ctest`) |
| Cartes | Waveshare ESP32-S3-ETH (Ethernet) et ESP32-S3-WROOM-1 N16R8 (Wi-Fi seul) |
| Capteurs | Jusqu'à 3 radars (LD2450, LD2461) ou capteurs de présence par nœud, capteur de lumière VEML7700 |
| Fonctionne sur | Waveshare ESP32-S3-ETH ou ESP32-S3-WROOM-1 N16R8 ; tests sur hôte sur n'importe quel ordinateur |
| Sur matériel réel | Tourne sur une vraie carte Waveshare ESP32-S3-ETH avec un radar, en rapportant au banc de test CM5 |
| Licence | GPL-3.0-or-later |

**Aller à :** [Présentation](#-présentation) · [Architecture](#-architecture) · [Sécurité](#-sécurité-et-protection) · [Interfaces](#-interfaces) · [Configuration](#-configuration) · [Démarrage rapide](#-démarrage-rapide) · [Tests](#-tests) · [Dépannage](#-dépannage) · [Structure](#-structure-du-dépôt) · [Liés](#-projets-liés)

## 🚦 État du projet

Chaque ligne est une partie de ce dépôt et le feu dit où elle en est aujourd'hui. Elle est écrite d'après ce que le dépôt peut montrer (tests, compilations, exécutions), pas d'après ce qui est prévu.

🟢 Fait et vérifié · 🟡 Fonctionne, avec des limites connues · 🟠 À moitié fait ou pas encore vérifié · 🔴 Pas fait ou ne fonctionne pas

| Domaine | État | Ce que cela veut dire ici |
|---|---|---|
| Logiciel | 🟢 | Le firmware compile dans le conteneur ESP-IDF 5.5.5 et tourne sur une vraie carte ESP32-S3-ETH. |
| Tests | 🟢 | 7 371 vérifications dans 14 programmes d'hôte réussissent (`ctest`) : les décodeurs, les réglages, la table des broches, les utilisateurs, le plan réseau, le sérialiseur de messages et les contrôles de version. |
| Protocoles et données | 🟡 | Le LD2450 tourne avec un vrai radar ; les décodeurs du LD2461 et des quatre capteurs de présence ne correspondent qu'aux exemples de leurs notices, et le canal de commandes du radar n'a pas été vérifié avec un module. |
| Interface | 🟡 | Le panneau web est utilisé sur la carte et essayé dans un vrai navigateur contre un nœud simulé ; son certificat HTTPS, créé par le nœud lui-même, n'a pas été essayé dans tous les navigateurs. |
| Matériel | 🟡 | C'est le seul nœud qui existe en matériel : une vraie carte Waveshare ESP32-S3-ETH avec un radar rend compte au banc d'essai CM5 ; aucun autre module de capteur n'est branché. |
| Sécurité | 🟡 | L'appairage par PIN Bluetooth et la vérification de la signature des mises à jour sont prouvés sur un ordinateur avec de vraies signatures mais pas encore sur la carte ; il n'y a pas eu de revue indépendante. |
| Mises à jour | 🟡 | Les mises à jour par voie hertzienne sont utilisées sur la carte ; la vérification des mises à jour signées n'y a pas encore tourné. |
| Intégration | 🟡 | Les messages de la carte arrivent au serveur du banc d'essai CM5, et ces mêmes messages sont acceptés par les schémas partagés et les vecteurs de conformité d'ARMOR-COMMON. Vu uniquement sur le banc d'essai, pas dans une installation. |
| Documentation | 🟢 | Huit langues et le journal des modifications sont à jour. |

## 🎯 Présentation

* **Deux cartes, un firmware :** la Waveshare ESP32-S3-ETH (Ethernet, par défaut) et une ESP32-S3-WROOM-1 N16R8 sans Ethernet (Wi-Fi seul : sa configuration demande le réseau Wi-Fi à rejoindre et le nœud garde toujours son propre réseau). L'image se choisit à la compilation (`tools/build_node.sh generic s3-eth` ou `generic s3-wifi`) ; la table des broches et l'accès suivent la carte, le panneau masque ce que la carte n'a pas, et une image ne vaut que pour sa carte. Les deux compilent et sont testées sur ordinateur ; aucune n'a tourné sur une carte.
* **Un nœud couvre jusqu'à 270 degrés :** chaque Waveshare ESP32-S3-ETH lit jusqu'à trois radars LD2450 sur ses trois UART, montés à 75 degrés l'un de l'autre, par Ethernet filaire (W5500) en DHCP ou adresse fixe, alimenté en PoE ou USB. *Add a 270° node* de Studio crée les trois radars déjà associés au nœud.
* **Un panneau web sur chaque nœud,** à l'apparence de Studio et en huit langues, intégré au firmware : vue d'ensemble, réseau, Wi-Fi, broker, radars, broches, utilisateurs, mise à jour du firmware et journal. Un nœud sans utilisateur ouvre le Wi-Fi `ARMOR-SETUP-xxxxxx` et crée son premier administrateur avec un code d'installation ; les mots de passe sont en PBKDF2 salé, les sessions sont des jetons aléatoires et tous les réglages sont dans la flash du nœud, si bien qu'une image sert tous les nœuds et qu'aucun mot de passe n'est compilé ([le panneau](docs/NODE_PANEL.md)).
* **Un seul Wi-Fi à partir de plusieurs nœuds :** chaque nœud peut offrir un point d'accès relié à son port Ethernet ; avec le même nom et mot de passe et le canal en automatique (1, 6 ou 11 selon la MAC), téléphones et capteurs Wi-Fi voient un seul réseau avec un seul serveur DHCP. Ce n'est pas un maillage radio : chaque nœud garde son câble. Un nœud peut aussi rejoindre le Wi-Fi d'un routeur (avec recherche de réseaux), et un nœud sans câble peut être configuré en **Bluetooth** depuis l'application Android.
* **Broches pour le serveur :** toute broche libre devient une entrée, une sortie (avec un état sûr si le broker est perdu), du PWM ou une lecture analogique, et apparaît comme un appareil du serveur : un relais ou un contact n'exige aucun nouveau firmware. Les broches réservées de la carte ne sont jamais proposées.
* **Mises à jour par radio (OTA) avec retour arrière** depuis le panneau (deux emplacements sur la flash de 16 Mo), et un **lien depuis Studio** vers le panneau de chaque nœud, à partir de l'adresse que le nœud publie.
* **Le panneau en HTTPS :** le nœud fabrique son propre certificat (auto-signé, conservé en flash) et sert le panneau sur le port 443 en plus du 80, ou seulement sur le 443 ; le cookie de session est marqué Secure et l'empreinte du certificat s'affiche pour la comparer à l'avertissement du navigateur.
* **Identités de pistes stables :** les cibles de chaque trame sont associées à celles suivies auparavant ; une personne garde donc le même identifiant, une trame perdue ne fait pas clignoter et les positions sont un peu lissées.
* **Six modèles de capteurs, un par port :** un port porte un LD2450 ou un LD2461 (traqueurs qui alimentent le périmètre, avec le champ propre du modèle dans Studio) ou un capteur de présence (LD2410, LD2412, LD2410S, MR24HPC1) qui devient un appareil du serveur et publie présence et distance. Choisi dans le panneau ; ce qu'est chacun, son protocole et ce qui n'a pas été vérifié sont dans `docs/SENSORS.md`.
* **Décodeur, santé et configuration du LD2450 :** un découpeur qui se resynchronise trouve les trames dans un flux bruité ; chaque trame de 30 octets donne jusqu'à trois cibles qui deviennent des pistes du contrat ; la console et le panneau indiquent par radar s'il émet, se tait ou envoie des données illisibles. Le panneau peut aussi lire la version du module, choisir une ou trois cibles et définir des zones de détection (un protocole non vérifié face à un module).
* **Lumière ambiante et messages conformes au contrat :** le VEML7700 avec plage automatique ; JSON de télémétrie, de santé et d'information qui suit les schémas publiés et refuse d'écrire quoi que ce soit d'invalide, avec des horodatages d'horloge murale (SNTP) et un testament MQTT. La télémétrie est retenue tant qu'aucun radar n'émet, jamais un « tout est calme » vide.
* **Une image pour toutes les cartes, comme un produit réseau :** le firmware est le même et l'adresse MAC distingue les cartes (`armor-` et six chiffres jusqu'au nommage) ; `adopt_node.py` donne à un nœud fraîchement flashé son administrateur, son identité de broker et les réglages communs de la flotte par le réseau, avec un code d'installation calculé depuis sa MAC : 5 nœuds ou 27, c'est le même travail.
* **Outils de banc :** flashage par USB-C, un nœud simulé pour travailler sur le panneau et un convertisseur d'un journal de trames brutes en jeu de test ([mise en service sur banc](docs/BENCH_BRINGUP.md)).
* **Mises à jour par radio signées :** le nœud n'installe depuis GitHub qu'une image dont le SHA-256 correspond et qui porte une signature Ed25519 valide du responsable (la clé publique est dans le firmware, la privée ne quitte jamais le responsable) ; un fichier envoyé à la main par un administrateur dans le panneau reste accepté. Les pages et messages du panneau sont en huit langues.
* **Appairage Bluetooth avec un PIN :** le PIN à six chiffres est dérivé du code de configuration, imprimé sur la console USB et montré à un administrateur dans le panneau ; le téléphone doit le saisir avant de pouvoir écrire les réglages du nœud.
* **Seule la maison peut utiliser le panneau :** le panneau répond par défaut au réseau privé (ou aux réseaux que son propriétaire indique, comme un VPN), refuse une liste qui exclurait celui qui l'enregistre, compte les mauvais mots de passe par personne et par adresse, et fonctionne derrière le proxy du serveur pour être atteint depuis l'extérieur sans publier le nœud.
* **Liaison chiffrée facultative avec le broker (TLS) :** désactivée par défaut, le nœud parle donc MQTT non chiffré comme avant. Un interrupteur et une zone pour le certificat de l'autorité propre à l'installation (PEM, 2 Ko au plus) dans la page Broker lui font se connecter en `mqtts://`, ne faire confiance qu'à cette autorité et vérifier que le certificat du broker a été établi pour l'adresse atteinte ; ce qui n'est pas un seul certificat PEM est refusé. Couvert par des tests sur ordinateur ; la poignée de main n'a pas encore été exécutée sur une carte.
* **Rapport de santé :** environ une fois par minute le nœud publie depuis combien de temps il tourne et pourquoi il a redémarré la dernière fois, sa mémoire libre, sa liaison et son signal Wi-Fi, combien de fois il s'est connecté au broker et la dernière erreur notée (`armor/health/<nœud>`, le contrat `node_health` d'ARMOR-COMMON). Le code qui le construit est partagé par tous les nœuds et testé sur un ordinateur ; il n'a pas tourné sur une carte.
* **Commande depuis le serveur :** Studio peut redémarrer le nœud proprement, le mettre en pause jusqu'à une heure (il reprend seul), mettre son travail en veille (sans fin, conservée aux redémarrages, le nœud reste connecté et continue de répondre), le faire se signaler, demander un instantané de diagnostic et le marquer en maintenance (`armor/control/<nœud>/...`, le contrat `node_control` d'ARMOR-COMMON). Chaque ordre se fait en deux temps avec un jeton à usage unique propre à cette opération, seul le serveur peut les écrire sur le broker, et il n'existe ni réinitialisation d'usine, ni effacement, ni sommeil profond. Tant que le nœud est en pause ou en veille, il ne publie ni cibles ni présence (le battement dit encore qu'il est là). Le code est partagé par tous les nœuds et testé sur un ordinateur ; il n'a pas tourné sur une carte.
* **Un moteur de périmètre sur le nœud (désactivé par défaut) :** six zones de détection et trois zones d'exclusion, chacune un rectangle qu'on peut tourner d'un angle, trois lignes de franchissement qui comptent les passages par sens, trois chemins d'entrée, immunité aux animaux et détection de masquage. Il tourne à côté des capteurs, donc il fonctionne sans accès au broker, garde ses événements tant que le broker est absent et les publie sur `armor/radar/<nœud>/events` ; l'éditeur de zones est dans le panneau du nœud. Rien de tout cela n'a encore tourné avec un capteur sur une carte.
* **Qui est chaque capteur, et son propre Bluetooth fermé :** au démarrage, le nœud lit la version du micrologiciel et l'adresse Bluetooth de chaque capteur, ferme par défaut le Bluetooth propre des capteurs (il ne demande aucun code PIN) et peut confier la ligne série d'un capteur à l'outil PC d'un administrateur pendant quelques minutes (un pont série transparent). `GET /metrics` pour les administrateurs et un export et import tout ou rien des réglages complètent l'ensemble.
* **Diagnostic sans câble :** après un démarrage qui n'était pas propre (plantage, chien de garde, chute d'alimentation, allocation échouée), le rapport de santé en donne la raison pendant dix minutes ; un plantage laisse son vidage mémoire (core dump) à consulter, télécharger ou effacer depuis le panneau ; les réglages enregistrés sont copiés avant chaque installation de micrologiciel et récupérés si une mise à jour les abîme ; et un administrateur peut demander au nœud, via le serveur, un paquet de diagnostic ou son journal avec un jeton à usage unique.
* **Adoption en HTTPS avec empreinte épinglée :** `tools/adopt_node.py` accepte `https://<adresse>` avec `--fingerprint` (le SHA-256 du certificat du nœud, vérifié sur la connexion même qui porte la requête) ou `--show-fingerprint` pour la lire ; il avertit du HTTP en clair et d'un fichier de flotte avec une IP fixe ou un nom d'hôte, et le panneau avertit aussi à l'import d'un tel fichier.
* **Le lecteur de capteurs attend des événements au lieu d'interroger :** la tâche de lecture dort sur les files d'événements des lignes série et vide tout ce qui est arrivé, vide la ligne en cas de débordement et compte débordements et erreurs par ligne ; `system.reader_idle_ms` (100 ; 0 = l'ancienne interrogation de 10 ms) fixe l'attente, et une file défaillante ramène seule à l'interrogation.
* **Scellement facultatif des secrets enregistrés :** désactivé par défaut (`security.seal_secrets`, ou `nodes.nvs_encryption` du serveur). Activé, les mots de passe, jetons, clés, codes PIN et empreintes de mots de passe que le nœud enregistre sont scellés avec ChaCha20-Poly1305 et tout le reste reste lisible ; l'activer ou le désactiver les réécrit sans rien perdre, même si le nœud s'arrête en route. Aucun eFuse n'est brûlé et la flash n'est pas chiffrée : la clé est dans une partition de la même puce, donc cela ne protège **pas** contre quelqu'un qui tient la carte, et un nœud installé avant cette partition garde ses secrets tels quels jusqu'à ce qu'il soit flashé avec la nouvelle table de partitions.

## 🧩 Rôle dans A.R.M.O.R.

ARMOR-NODE-RADAR est l'œil du périmètre. Chaque nœud lit jusqu'à trois radars ou capteurs de présence et publie pistes, lumière et santé vers [ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER) en MQTT ; [ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO) les dessine sur le plan du site et [ARMOR-SERVER-AI](https://github.com/JuanenRac/ARMOR-SERVER-AI) les pèse face aux caméras. Son firmware partage une base avec les autres nœuds (le panneau, la configuration Bluetooth, les mises à jour signées, la connexion et l'horloge viennent de la base firmware d'[ARMOR-COMMON](https://github.com/JuanenRac/ARMOR-COMMON)), si bien que ce qu'on apprend sur un nœud vaut pour tous.

| Nom | Comment | À quoi ça sert |
|---|---|---|
| ARMOR-SERVER | MQTT `armor/node/<id>/telemetry|health|info|command` | Reçoit les messages du radar (aujourd'hui le banc CM5) et commande le nœud |
| ARMOR-STUDIO | lien vers le panneau | Ouvre le panneau du nœud à l'adresse que le nœud publie |
| ARMOR-ANDROID-CONTROL | Bluetooth | Configure le nœud sur le terrain |
| ARMOR-COMMON | contrats, base de firmware | Les messages sont vérifiés avec ses schémas ; le firmware commun en est copié |

## 🏛️ Architecture

<p align="center">
  <img src="images/architecture.svg" alt="Architecture de ARMOR-NODE-RADAR : qui l'appelle, ce qu'il contient et de quoi il dépend" width="100%">
</p>

## 🖼️ Captures d'écran

Capturées en 1920×1080 sur un banc d'essai, avec des données d'exemple et sans installation réelle derrière.

<table>
<tr><td width="50%" valign="top"><img src="images/screenshots/overview.png" alt="Résumé : l'état du nœud en un coup d'œil - réseau, broker, firmware et ce qu'il remonte." width="100%"><br><sub>Résumé : l'état du nœud en un coup d'œil - réseau, broker, firmware et ce qu'il remonte.</sub></td><td width="50%" valign="top"><img src="images/screenshots/radars.png" alt="Radars : chaque radar du nœud, son modèle, ses broches et son étalonnage." width="100%"><br><sub>Radars : chaque radar du nœud, son modèle, ses broches et son étalonnage.</sub></td></tr>
<tr><td width="50%" valign="top"><img src="images/screenshots/pins.png" alt="Broches : ce que fait chaque broche, vérifié pour que deux fonctions n'en partagent jamais une." width="100%"><br><sub>Broches : ce que fait chaque broche, vérifié pour que deux fonctions n'en partagent jamais une.</sub></td><td width="50%" valign="top"><img src="images/screenshots/network.png" alt="Réseau : l'adressage Ethernet ou Wi-Fi du nœud, réglé depuis son propre panneau web." width="100%"><br><sub>Réseau : l'adressage Ethernet ou Wi-Fi du nœud, réglé depuis son propre panneau web.</sub></td></tr>
</table>

## 🔒 Sécurité et protection

| Menace | Défense |
|---|---|
| Mots de passe en clair ou compilés | PBKDF2-HMAC-SHA256 avec 10 000 tours ; aucun mot de passe dans le firmware (un nœud sans utilisateur passe en configuration) ; au plus 4 utilisateurs (admin ou lecteur) et de 8 à 64 caractères ; au plus 6 sessions, 30 minutes d'inactivité |
| Prendre un nœud tout juste flashé | La configuration exige un code de démarrage : celui du build (`CONFIG_ARMOR_SETUP_CODE`) ou un code aléatoire gardé en flash et affiché sur la console USB ; avec un secret de flotte c'est `HMAC-SHA256(secret, MAC)` ; c'est aussi la clé du Wi-Fi de configuration (WPA2, jamais ponté) |
| Le panneau atteint depuis Internet | Il répond par défaut aux réseaux privés d'une maison (10/8, 172.16/12, 192.168/16, 169.254/16, 127/8 et 100.64/10 où vivent certains VPN) ; le réglage `web.admin_sources` liste d'autres adresses et réseaux IPv4 (8 entrées, 120 caractères) ; tout le reste est refusé sauf l'identité publique `GET /api/v1/session` ; une liste qui exclurait celui qui l'enregistre est rejetée |
| Le panneau en clair en HTTP | HTTPS sur le 443 avec un certificat que le nœud fabrique lui-même (autosigné, en flash, empreinte SHA-256 visible pour comparer) ; le panneau peut être http, https ou les deux ; le cookie de session est toujours HttpOnly et SameSite=Strict (et Secure en TLS) |
| Un faux firmware depuis GitHub | Une image n'est installée qu'avec un SHA-256 concordant et une signature Ed25519 valide faite avec la clé du mainteneur (`no_checksum`, `no_signature`, `bad_signature`, `checksum_mismatch`, `wrong_firmware`, `not_newer`, `version_mismatch` la refusent) ; un envoi manuel par un administrateur n'est pas vérifié : une limite explicite |
| Détournement de la configuration Bluetooth | Un PIN à 6 chiffres dérivé du code de configuration, visible sur la console USB et pour un administrateur ; le téléphone doit le saisir avant d'écrire des réglages ; la fenêtre dure `CONFIG_ARMOR_BLE_SETUP_MINUTES` (10 par défaut, de 1 à 120) |
| Un ordre distant qui fait du mal | Deux étapes avec un jeton à usage unique lié à cette opération ; seul le serveur peut écrire ces topics dans le broker ; une pause dure une heure au plus ; il n'y a ni réinitialisation d'usine, ni effacement, ni sommeil profond par ordre distant |
| Secrets stockés lus par qui a la carte | Optionnel (désactivé par défaut, interrupteur `security.seal_secrets` ou `nodes.nvs_encryption` du serveur) : ChaCha20-Poly1305 pour mots de passe, jetons, clés, PIN et hachages. Il ne chiffre pas la flash et ne grille aucun eFuse : la clé est dans une partition de la même puce, il ne protège donc pas contre qui tient la carte |
| Le lien avec le broker en clair | TLS optionnel (`mqtts://`) avec l'autorité propre du propriétaire (PEM jusqu'à 2 Ko) et vérification du nom ; désactivé par défaut. Le handshake n'a tourné qu'en tests sur hôte, pas sur une carte |
| Le Bluetooth propre d'un capteur | Par défaut le nœud ferme le Bluetooth de chaque capteur LD (sans PIN) ; un pont série transparent vers un ordinateur ne s'ouvre que quelques minutes et par un administrateur |
| Adopter un nœud sur un réseau non fiable | `tools/adopt_node.py https://<adresse> --fingerprint <SHA-256>` vérifie le certificat dans la même connexion ; il prévient pour le HTTP en clair et les fichiers de flotte à adresse fixe |
| Une broche dangereuse | Les broches réservées de la carte ne sont jamais proposées ; une sortie a un état sûr si le broker est perdu ; deux fonctions ne peuvent pas partager une broche (vérifié) |
| Un firmware public avec secret de flotte | `CONFIG_ARMOR_FLEET_SECRET` est vide par défaut : le build public ne porte aucun secret |

Lisez [SECURITY.md](SECURITY.md) pour signaler une vulnérabilité. Il n'y a pas eu de revue indépendante.

## 🔌 Interfaces

| Partie | Ce qu'elle règle |
|---|---|
| MQTT | `armor/node/<id>/{telemetry,health,info,command}`, `armor/health/<id>` (son testament est `{"node_id":"..","online":false}`), `armor/control/<id>/{command,result,maintenance}` et `armor/control/all/command`, `armor/device/<id>/<pin>/{state,set}`, `armor/radar/<id>/events`, `armor/radar/<id>/control/armed` |
| Panneau (HTTP 80, HTTPS 443) | Routes `session`, `login`, `logout`, `setup`, `status`, `config` (+ export, import), `wifi/scan`, `time`, `radars` (+ command), `perimeter` (+ armed, learn, zone), `pins` (+ command), `bridge` (+ open, close, io), `ota` (+ check, install, progress, copy, switch), `account`, `ble-pin`, `log`, `coredump`, `factory-reset`, `reboot` ; `/metrics` pour les administrateurs et le WebSocket `/ws/radar-map` ; `tests/check_routes.py` vérifie que toute route appelée par le panneau existe |
| Textes du panneau | 76 codes d'erreur avec texte en huit langues (832 lignes dans `panel/text.js`), vérifiés par `tests/check_panel_text.py` |
| Matériel | 3 UART pour LD2450, LD2461, LD2410, LD2412, LD2410S et MR24HPC1 (RX 16, 17, 18 ; TX 15, 21, 38) ; W5500 en SPI (MOSI 11, MISO 12, SCLK 13, CS 14, INT 10, RST 9, 20 MHz) ; VEML7700 en I2C (GPIO 1 et 2) ; bouton BOOT |

## ⚙️ Configuration

| Variable | Par défaut | Ce qu'elle règle |
|---|---|---|
| `CONFIG_ARMOR_NODE_ID` | `""` | Le nom ; vide donne `armor-` et les trois derniers octets de la MAC |
| `CONFIG_ARMOR_SETUP_CODE`, `CONFIG_ARMOR_FLEET_SECRET` | `""` | Le code de configuration du build (vide : aléatoire) et le secret de flotte (16+ caractères ; jamais dans le dépôt) |
| `CONFIG_ARMOR_RADAR_COUNT` | 3 (0-3) | Combien de radars sont connectés |
| `CONFIG_ARMOR_TRACK_STABILISER` | y | Garder l'identité d'une piste d'une image à l'autre |
| `CONFIG_ARMOR_MQTT_URI`, `_USERNAME`, `_PASSWORD` | `""` | Les premiers réglages du broker (modifiables dans le panneau) |
| `CONFIG_ARMOR_HEARTBEAT_S` | 10 (2-300) | Secondes entre battements de santé |
| `CONFIG_ARMOR_TELEMETRY_PERIOD_MS` | 200 (200-5000) | Au plus 5 messages par seconde |
| `CONFIG_ARMOR_BLE_SETUP_MINUTES` | 10 (1-120) | Combien de temps la configuration Bluetooth reste ouverte |
| `CONFIG_ARMOR_BOARD_S3_ETH` / `_S3_WIFI` | eth | La carte : Waveshare ESP32-S3-ETH ou ESP32-S3-WROOM-1 N16R8 (Wi-Fi seulement) |
| `CONFIG_ARMOR_BOOT_FACTORY_RESET` | y | Maintenez BOOT 8 s dans les 30 premières s pour recommencer |
| `system.login_limit_per_minute` | 30 (0-120) | Mots de passe erronés par minute avant que le panneau verrouille la connexion pour tous (0 = pas de limite globale) ; on le lit et le change depuis Studio (un bouton sur chaque nœud) ou avec `POST /api/v1/admin/nodes/settings`, l'identifiant du panneau étant utilisé une fois et jamais conservé |

Les réglages ajustés en direct sont dans le panneau et gardés en flash (par exemple `system.reader_idle_ms`, `security.seal_secrets`, la limite d'échecs de connexion par minute (30, de 0 à 120) et les zones du moteur de périmètre) ; les valeurs par défaut du firmware sont dans `sdkconfig.defaults` et `sdkconfig.board.*`. La flash de 16 Mo contient `nvs` (64 Ko), `otadata`, `phy_init`, deux emplacements de 3 Mo `ota_0` et `ota_1`, `coredump` (128 Ko) et `nvs_keys` (4 Ko) (`partitions.csv`).

## 🚀 Démarrage rapide

**Prérequis:** un compilateur C++17 pour les tests sur hôte ; le conteneur ESP-IDF 5.5.5 pour construire l'image ; une Waveshare ESP32-S3-ETH ou une ESP32-S3-WROOM-1 N16R8.

```bash
cmake -S tests -B build/host && cmake --build build/host && ctest --test-dir build/host   # host tests, -Werror
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

## 🧪 Tests

```bash
cmake -S tests -B build/host && cmake --build build/host && ctest --test-dir build/host
build/host/emit_samples | python tests/check_contract.py
python -I tools/armor_ci_validate.py
```

14 programmes de test sur hôte et 6 contrôles Python tournent sous `ctest` ; 7371 vérifications comptées par les programmes hôte, compilés avec `-Werror`.

| Partie | Ce qu'il couvre |
|---|---|
| `test_core`, `test_sensors`, `test_family`, `test_uart_wake` | Les décodeurs (LD2450, LD2461, quatre capteurs de présence), le délimiteur de trames, le canal de commandes, les familles de capteurs et le réveil de l'UART |
| `test_node`, `test_base`, `test_shared`, `test_board_wifi` | Les réglages et leurs contrôles, la table des broches, utilisateurs et sessions, le plan réseau, la base partagée et la carte Wi-Fi |
| `test_logic`, `test_perimeter`, `test_control`, `test_diag` | La logique des broches, le moteur de périmètre, les ordres de contrôle en deux étapes et le diagnostic |
| `test_mqtt_tls`, `test_release` | Le lien TLS avec le broker (hôte seulement) et les contrôles de version et de signature |
| `check_panel_text`, `check_task_safety`, `check_save_paths`, `check_ble_session`, `check_routes`, `check_bridge_client` | Python : les textes du panneau en huit langues (832 lignes), les tâches, les chemins de sauvegarde, la session Bluetooth sous son verrou, les routes et le client du pont série |

Le panneau web est aussi exercé dans un vrai navigateur contre un nœud de substitution (`tools/panel_mock.mjs`). Les décodeurs LD2461 et de présence ne correspondent qu'aux exemples de leurs manuels.

## 🏗️ Compilation et publication

`tools/build_node.sh generic` construit UNE image pour chaque carte Waveshare dans le conteneur ESP-IDF 5.5.5, et `tools/flash.bat` l'écrit par USB-C ; le même firmware est construit pour la carte Wi-Fi seul avec `s3-wifi`. Les versions portent l'image, son SHA-256 et sa signature Ed25519, une par carte.

## 🩺 Dépannage

| Vous voyez | Parce que | Faites |
|---|---|---|
| Vous avez perdu le mot de passe ou le nœud ne rejoint plus le réseau | Un nœud en configuration sans adresse après 90 s ouvre son propre Wi-Fi `ARMOR-SETUP-xxxxxx` (ouvrez `http://192.168.4.1/`) | Pour recommencer, maintenez BOOT 8 secondes dans les 30 premières secondes après la mise sous tension (efface réglages et utilisateurs), ou utilisez *Réinitialisation d'usine* dans le panneau |
| Une mise à jour depuis GitHub est refusée | La version n'a ni somme de contrôle ni signature, elles ne concordent pas (`e_no_checksum`, `e_no_signature`, `e_checksum_mismatch`, `e_bad_signature`), n'a pas d'image pour cette carte (`e_no_asset_for_board`) ou est d'un autre projet (`e_wrong_firmware`) | Le nœud n'a pas changé ; réessayez ou laissez un administrateur envoyer le fichier à la main |
| Les commandes du radar répondent `no_answer`, `refused` ou `disabled` | Le fil TX du radar n'est pas connecté, le module s'est bloqué ou le nœud ferme d'abord son Bluetooth | Vérifiez le câblage (voir la mise en service sur banc), redémarrez le module depuis le panneau |
| Le nœud ne montre pas le capteur 3 ou laisse un radar dormir | Le module a cessé de parler (silence sans trame mauvaise) | Utilisez *redémarrer le module* dans la page des radars, puis fermez le Bluetooth du capteur |

## 📂 Structure du dépôt

<p align="center">
  <img src="images/repository-structure.svg" alt="Structure du dépôt de ARMOR-NODE-RADAR" width="100%">
</p>

<details>
<summary>Version texte de l'arborescence</summary>

```text
ARMOR-NODE-RADAR/
├── main/                                le firmware : un composant ESP-IDF
├── panel/                               le panneau web intégré au firmware
├── tests/                               tests sur hôte et contrôles Python
├── docs/                                7 guides du banc et du panneau
├── tools/                               outils de build, flashage, adoption, flotte et contrôle
├── .github/                             workflow CI et modèles d'issues et de pull requests
├── images/                              bannière, icône et les deux images de ce README
├── build.sh build.bat                   build par le flux de projet partagé
├── test.sh test.bat                     lancer les tests par le flux partagé
├── run.sh run.bat                       démarrer par le flux de projet partagé
├── armor.project.json                   manifeste : version, rôle, maturité, commande de build
├── CHANGELOG.md                         ce qui a changé à chaque version
├── SECURITY.md SUPPORT.md LICENSE       politique de sécurité, support et licence
├── CONTRIBUTING.md CODE_OF_CONDUCT.md   comment contribuer et le code de conduite
├── CMakeLists.txt                       le projet ESP-IDF
├── partitions.csv                       la table de la flash
├── sdkconfig.defaults                   les réglages du firmware
├── sdkconfig.board.s3-eth               la carte Ethernet
├── sdkconfig.board.s3-wifi              la carte Wi-Fi
├── secrets/                             exemples de fichiers de flotte et de nœud
├── build-test.sh build-test.bat         lancer les tests par le flux partagé
└── .gitattributes .gitignore            réglages Git
```

</details>

## 🗺️ Feuille de route et limites connues

**Limites connues aujourd'hui**

* Les décodeurs LD2461 et de présence ne correspondent qu'aux exemples de leurs manuels, sans module derrière ; le canal de commandes n'a été vérifié qu'avec le LD2450.
* La vérification de signature des mises à jour et l'appariement Bluetooth par PIN sont prouvés sur ordinateur avec de vraies signatures mais pas encore sur la carte ; le handshake TLS avec le broker n'a pas tourné sur une carte.
* Le rapport de santé et le contrôle depuis le serveur n'ont pas tourné sur la carte ; le moteur de périmètre n'a jamais tourné avec un capteur ; le certificat HTTPS propre du panneau n'est pas essayé dans tous les navigateurs ; la carte Wi-Fi n'existe pas physiquement (seule la carte Ethernet existe).

**Ensuite**

* Faire tourner le moteur de périmètre, le rapport de santé et les ordres de contrôle avec de vrais capteurs et le banc CM5 ; essayer le lien TLS avec le broker et la mise à jour signée sur la carte.

## 🌍 Langues

Le README et les textes existent en anglais, espagnol, français, italien, allemand, chinois, japonais et portugais européen.

## 🔗 Projets liés

**A.R.M.O.R.** (Autonomous Radar & Multimodal Observation Range) est un système de sécurité périmétrique composé de dépôts indépendants. Chacun a sa propre version, ses propres tests et son propre README ; voici la famille :

* **[ARMOR-COMMON](https://github.com/JuanenRac/ARMOR-COMMON)** - Contrats de messages, validateurs, vecteurs de conformité et types générés
* **ARMOR-NODE-RADAR** (ce dépôt) - Firmware du nœud de terrain pour ESP32-S3 avec trois radars et son propre panneau web
* **[ARMOR-NODE-SOLAR](https://github.com/JuanenRac/ARMOR-NODE-SOLAR)** - Protocoles des onduleurs et batteries solaires et messages d'un nœud passerelle
* **[ARMOR-NODE-ELECTRICAL](https://github.com/JuanenRac/ARMOR-NODE-ELECTRICAL)** - Nœud électrique : compteurs, le message des mesures du réseau et les règles de commutation
* **[ARMOR-NODE-ALARM](https://github.com/JuanenRac/ARMOR-NODE-ALARM)** - Nœud et centrale d'alarme : zones, armement, temporisations, sirène et PIN, avec ou sans le serveur
* **[ARMOR-NODE-HMI](https://github.com/JuanenRac/ARMOR-NODE-HMI)** - Panneau tactile : l'état du système sur un écran mural, armer et acquitter, et la maison de l'assistant vocal
* **[ARMOR-NETWORK](https://github.com/JuanenRac/ARMOR-NETWORK)** - Le réseau local : ses appareils, internet et ce qui change
* **[ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER)** - Coordinateur central : télémétrie, alarmes, appareils, relevés solaires et caméras
* **[ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO)** - Console web : caméras, radar, alarmes, énergie solaire et concepteur de site 2D/3D
* **[ARMOR-ANDROID-CONTROL](https://github.com/JuanenRac/ARMOR-ANDROID-CONTROL)** - Client Android de l'opérateur avec radar 2D/3D en direct
* **[ARMOR-IOS-CONTROL](https://github.com/JuanenRac/ARMOR-IOS-CONTROL)** - Client opérateur iPhone
* **[ARMOR-SERVER-AI](https://github.com/JuanenRac/ARMOR-SERVER-AI)** - Politique d'inférence visuelle qui explique ses décisions et n'agit jamais
* **[ARMOR-VISION](https://github.com/JuanenRac/ARMOR-VISION)** - Zones de caméra, masques et règles de détection pour la Jetson Orin NX
* **[ARMOR-VOICE-AI](https://github.com/JuanenRac/ARMOR-VOICE-AI)** - Intentions vocales hors ligne avec une confirmation impossible à falsifier
* **[ARMOR-HARDWARE](https://github.com/JuanenRac/ARMOR-HARDWARE)** - Boîtiers, électronique et matrice d'acceptation sur banc
* **[ARMOR-DEVOPS](https://github.com/JuanenRac/ARMOR-DEVOPS)** - Déploiement, banc d'essai CM5, sauvegarde et TLS
* **[ARMOR-SIMULATOR](https://github.com/JuanenRac/ARMOR-SIMULATOR)** - Simulateur de télémétrie hors ligne avec des pannes reproductibles
* **[ARMOR-UPDATER](https://github.com/JuanenRac/ARMOR-UPDATER)** - Détecte, installe et met à jour les propres dépôts de l'écosystème
* **[ARMOR-DOCS](https://github.com/JuanenRac/ARMOR-DOCS)** - Architecture, base de sécurité et matrice des capacités

## 📚 Documentation

* [Panneau du nœud](docs/NODE_PANEL.md) : premier démarrage, utilisateurs, réseau, les pages du panneau
* [Code de configuration](docs/SETUP_CODE.md) et [mise en service Bluetooth](docs/BLE_PROVISIONING.md)
* [Capteurs](docs/SENSORS.md) : les six modèles et leurs décodeurs
* [Mise en service sur banc](docs/BENCH_BRINGUP.md), [essais de banc](docs/BENCH_TESTS.md) et [limite du matériel](docs/HARDWARE_BOUNDARY.md)
* [Matrice des capacités : ce qui est prouvé et ce qui ne l'est pas](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/CAPABILITY_MATRIX.md)
* [Catalogue des projets : versions et dépendances entre les dépôts](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/PROJECT_CATALOG.md)
* [Historique des modifications de ce dépôt](CHANGELOG.md)

## 🤝 Contribuer et communauté

* Avant une pull request, lancez `ctest --test-dir build/host` and `python -I tools/armor_ci_validate.py` : il doit réussir. Voir [CONTRIBUTING.md](CONTRIBUTING.md) et le [code de conduite](CODE_OF_CONDUCT.md).
* Le README existe en huit langues et est généré à partir des sources d'ARMOR-DOCS : une modification du texte se fait là-bas.
* Une modification d'un message entre composants commence dans ARMOR-COMMON.
* Ne décrivez pas comme fonctionnant sur une carte ce qui a seulement été compilé.
* Signalez une vulnérabilité en privé : [SECURITY.md](SECURITY.md). Questions, idées et rapports : electrohobby3d@gmail.com ([SUPPORT.md](SUPPORT.md)).

## 👤 AUTEUR

**JuanenRac (Electro Hobby 3D)** · electrohobby3d@gmail.com

## 📜 LICENCE

GPL-3.0-or-later - voir [LICENSE](LICENSE).
