<p align="center">
  <img src="images/ARMOR_BANNER.svg" alt="ARMOR-RADAR banner" width="100%">
</p>

# 📡 ARMOR-RADAR

<p align="center">
  <a href="README.md">🇺🇸 English</a> |
  <a href="README_spa.md">🇪🇸 Español</a> |
  <a href="README_fra.md">🇫🇷 Français</a> |
  🇮🇹 <b>Italiano</b> |
  <a href="README_deu.md">🇩🇪 Deutsch</a> |
  <a href="README_zho.md">🇨🇳 简体中文</a> |
  <a href="README_jpn.md">🇯🇵 日本語</a> |
  <a href="README_por.md">🇵🇹 Português</a>
</p>

### Firmware del nodo di campo (Waveshare ESP32-S3-ETH, tre radar o sensori di presenza, Ethernet e Wi-Fi) con un proprio pannello web e il nucleo testato su computer

<p align="center">
  <img src="https://img.shields.io/badge/License-GPL%203.0-blue.svg" alt="GPL 3.0">
  <img src="https://img.shields.io/badge/Language-C%2B%2B17-00599c.svg" alt="Language">
  <img src="https://img.shields.io/badge/Target-ESP32--S3-e7352c.svg" alt="Target">
  <img src="https://img.shields.io/badge/Version-0.7.0-0f6cbd.svg" alt="Version">
  <img src="https://img.shields.io/badge/Maturity-established-2EA44F.svg" alt="Maturity">
</p>

---

**Controllo di onestà - cosa funziona oggi:** Il nodo radar è l'unico nodo che esiste come hardware: il suo firmware gira su una vera scheda Waveshare ESP32-S3-ETH con un radar e riferisce al banco di prova CM5, e il suo pannello web e gli aggiornamenti via etere vi sono in uso. Il nucleo indipendente dall'hardware (955 controlli in 5 programmi di test: i decodificatori dell'LD2450, dell'LD2461 e di quattro sensori di presenza, il canale di comando dell'LD2450, le impostazioni e i loro controlli, la tabella dei pin, utenti e sessioni, il piano di rete, il serializzatore di messaggi la cui uscita è accettata da ARMOR-COMMON, e i controlli di versione e firma) è provato su un computer, il **pannello web** è stato esercitato in un vero browser contro un nodo sostitutivo e l'**immagine del firmware si compila** nel contenitore ESP-IDF 5.4. Ciò che **non** è ancora provato: i decodificatori dell'LD2461 e dei sensori di presenza corrispondono solo agli esempi svolti dei loro manuali, senza un modulo dietro; il controllo della firma degli aggiornamenti e l'abbinamento Bluetooth con PIN sono provati su computer con firme vere ma non ancora sulla scheda; il canale di comando del radar non è verificato con un modulo; e il certificato HTTPS del pannello (fatto dal nodo stesso) non è provato in tutti i browser.

---

## 📋 In breve

| | |
|---|---|
| Versione | `0.7.0` |
| Maturità | Consolidato |
| Test | 955 checks in 5 programs (`ctest`) |
| Tecnologia | ESP-IDF C++ |
| Su hardware reale | Gira su una vera scheda Waveshare ESP32-S3-ETH con un radar, riferendo al banco di prova CM5 |

## 🎯 Panoramica

* **Due schede, un solo firmware:** la Waveshare ESP32-S3-ETH (Ethernet, predefinita) e una ESP32-S3-WROOM-1 N16R8 senza Ethernet (solo Wi-Fi: la configurazione chiede la rete Wi-Fi a cui unirsi e il nodo tiene sempre la propria rete). L'immagine si sceglie in compilazione (`tools/build_node.sh generic s3-eth` o `generic s3-wifi`); la tabella dei pin e l'accesso seguono la scheda, il pannello nasconde ciò che la scheda non ha e un'immagine vale solo per la sua scheda. Entrambe compilano e sono testate sul computer; nessuna ha girato su una scheda.
* **Due nodi da 270 gradi:** ogni Waveshare ESP32-S3-ETH legge fino a tre radar LD2450 sulle sue tre UART, montati a 75 gradi l'uno dall'altro, via Ethernet cablata (W5500) con DHCP o indirizzo fisso, alimentato con PoE o USB. *Add a 270° node* di Studio crea i tre radar già associati al nodo.
* **Un pannello web su ogni nodo,** con l'aspetto di Studio e in otto lingue, incorporato nel firmware: panoramica, rete, Wi-Fi, broker, radar, pin, utenti, aggiornamento del firmware e registro. Un nodo senza utenti apre il Wi-Fi `ARMOR-SETUP-xxxxxx` e crea il suo primo amministratore con un codice di configurazione; le password sono PBKDF2 con sale, le sessioni sono token casuali e ogni impostazione è nella flash del nodo, quindi un'unica immagine serve tutti i nodi e nessuna password è compilata ([il pannello](docs/NODE_PANEL.md)).
* **Un solo Wi-Fi da più nodi:** ogni nodo può offrire un access point collegato alla sua porta Ethernet; con lo stesso nome e password e il canale in automatico (1, 6 o 11 secondo la MAC), telefoni e sensori Wi-Fi vedono una sola rete con un solo server DHCP. Non è una rete mesh radio: ogni nodo mantiene il suo cavo. Un nodo può invece unirsi al Wi-Fi di un router (con ricerca delle reti), e uno senza cavo può essere configurato via **Bluetooth** dall'app Android.
* **Pin per il server:** qualsiasi pin libero diventa un ingresso, un'uscita (con uno stato sicuro se il broker si perde), PWM o una lettura analogica, e appare come dispositivo del server: un relè o un contatto non richiede un nuovo firmware. I pin riservati della scheda non vengono mai offerti.
* **Aggiornamenti via radio (OTA) con ripristino** dal pannello (due slot sulla flash da 16 MB) e un **collegamento da Studio** al pannello di ogni nodo, dall'indirizzo che il nodo pubblica.
* **Il pannello in HTTPS:** il nodo crea il proprio certificato (autofirmato, conservato in flash) e serve il pannello sulla porta 443 oltre che sulla 80, o solo sulla 443; il cookie di sessione è marcato Secure e l'impronta del certificato viene mostrata per confrontarla con l'avviso del browser. **Identità di traccia stabili:** i bersagli di ogni frame sono associati a quelli seguiti prima, così una persona mantiene lo stesso id, un frame perso non fa sfarfallare e le posizioni sono un po' levigate.
* **Sei modelli di sensore, uno per porta:** una porta porta un LD2450 o un LD2461 (tracker che alimentano il perimetro, con il campo proprio del modello in Studio) o un sensore di presenza (LD2410, LD2412, LD2410S, MR24HPC1) che diventa un dispositivo del server e pubblica presenza e distanza. Si sceglie nel pannello; cos'è ciascuno, il suo protocollo e cosa non è stato verificato sono in `docs/SENSORS.md`.
* **Decodificatore, salute e configurazione dell'LD2450:** un framer che si risincronizza trova i frame in un flusso rumoroso; ogni frame da 30 byte dà fino a tre bersagli che diventano tracce del contratto; la console e il pannello dicono per radar se trasmette, tace o manda dati illeggibili. Il pannello può anche leggere la versione del modulo, scegliere uno o tre bersagli e impostare zone di rilevamento (un protocollo non verificato su un modulo).
* **Luce ambiente e messaggi esatti rispetto al contratto:** il VEML7700 con range automatico; JSON di telemetria, salute e informazioni che segue gli schemi pubblicati e rifiuta di scrivere qualsiasi cosa non valida, con timestamp di orologio reale (SNTP) e un last will MQTT. La telemetria è trattenuta finché nessun radar trasmette, mai un «tutto tranquillo» vuoto.
* **Un'immagine per tutte le schede, come un prodotto di rete:** il firmware è lo stesso e la MAC distingue le schede (`armor-` e sei cifre finché non ricevono un nome); `adopt_node.py` dà a un nodo appena flashato il suo amministratore, la sua identità di broker e le impostazioni comuni della flotta via rete, con un codice di configurazione calcolato dalla sua MAC: 5 nodi o 27 sono lo stesso lavoro. **Strumenti da banco:** flash via USB-C, un nodo finto per lavorare sul pannello e un convertitore da un registro di frame grezzi a un fixture di test ([messa in servizio da banco](docs/BENCH_BRINGUP.md)).
* **Aggiornamenti via etere firmati:** il nodo installa da GitHub solo un'immagine il cui SHA-256 coincide e che porta una firma Ed25519 valida del responsabile (la chiave pubblica è nel firmware, quella privata non lascia mai il responsabile); un file caricato a mano da un amministratore nel pannello resta accettato. Le pagine e i messaggi del pannello sono in otto lingue.
* **Abbinamento Bluetooth con PIN:** il PIN a sei cifre deriva dal codice di configurazione, è stampato sulla console USB e mostrato a un amministratore nel pannello; il telefono deve inserirlo prima di poter scrivere le impostazioni del nodo.
* **Solo la casa può usare il pannello:** il pannello risponde per impostazione predefinita alla rete privata (o alle reti indicate dal proprietario, come una VPN), rifiuta un elenco che escluderebbe chi lo salva, conta le password errate per persona e per indirizzo e funziona dietro il proxy del server per essere raggiunto da fuori casa senza pubblicare il nodo.
* **Collegamento cifrato facoltativo con il broker (TLS):** disattivato per impostazione predefinita, quindi il nodo parla MQTT non cifrato come prima. Un interruttore e una casella per il certificato dell'autorità propria dell'installazione (PEM, al massimo 2 KB) nella pagina Broker lo fanno collegare con `mqtts://`, fidarsi solo di quell'autorità e verificare che il certificato del broker sia stato fatto per l'indirizzo raggiunto; ciò che non è un solo certificato PEM viene rifiutato. Coperto da test sul computer; l'handshake non è ancora stato eseguito su una scheda.
* **Rapporto di salute:** circa una volta al minuto il nodo pubblica da quanto è acceso e perché si è riavviato l'ultima volta, la memoria libera, il collegamento e il segnale Wi-Fi, quante volte si è connesso al broker e l'ultimo errore registrato (`armor/health/<nodo>`, il contratto `node_health` di ARMOR-COMMON). Il codice che lo costruisce è condiviso da tutti i nodi e testato su un computer; non è mai girato su una scheda.

## 🖼️ Schermate

Catturate a 1920×1080 su un banco di prova, con dati di esempio e senza alcuna installazione reale dietro.

<table>
<tr><td width="50%" valign="top"><img src="images/screenshots/overview.png" alt="Riepilogo: lo stato del nodo in breve - rete, broker, firmware e ciò che riporta." width="100%"><br><sub>Riepilogo: lo stato del nodo in breve - rete, broker, firmware e ciò che riporta.</sub></td><td width="50%" valign="top"><img src="images/screenshots/radars.png" alt="Radar: ogni radar del nodo, il suo modello, i suoi pin e la sua calibrazione." width="100%"><br><sub>Radar: ogni radar del nodo, il suo modello, i suoi pin e la sua calibrazione.</sub></td></tr>
<tr><td width="50%" valign="top"><img src="images/screenshots/pins.png" alt="Pin: cosa fa ogni pin, verificato perché due funzioni non ne condividano mai uno." width="100%"><br><sub>Pin: cosa fa ogni pin, verificato perché due funzioni non ne condividano mai uno.</sub></td><td width="50%" valign="top"><img src="images/screenshots/network.png" alt="Rete: l'indirizzamento Ethernet o Wi-Fi del nodo, impostato dal suo pannello web." width="100%"><br><sub>Rete: l'indirizzamento Ethernet o Wi-Fi del nodo, impostato dal suo pannello web.</sub></td></tr>
</table>

## 🧩 Ruolo in A.R.M.O.R.

ARMOR-RADAR sono gli occhi del perimetro. Ogni nodo legge fino a tre radar o sensori di presenza e pubblica tracce, luce e salute verso [ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER) via MQTT; [ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO) le disegna sulla planimetria del sito e [ARMOR-SERVER-AI](https://github.com/JuanenRac/ARMOR-SERVER-AI) le pesa rispetto alle telecamere. Il suo firmware condivide una base con gli altri nodi (il pannello, la configurazione Bluetooth, gli aggiornamenti firmati, l'accesso e l'orologio vengono dalla base firmware di [ARMOR-COMMON](https://github.com/JuanenRac/ARMOR-COMMON)), così ciò che impari su un nodo vale per tutti.

## 📂 Struttura del repository

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

## 🛠️ Ambiente di sviluppo

**Requisiti:** a C++17 compiler for the host tests, the ESP-IDF 5.4 container to build the image, a Waveshare ESP32-S3-ETH or an ESP32-S3-WROOM-1 N16R8

```bash
cmake -S tests -B build/host && cmake --build build/host
build/host/test_core && build/host/test_node && build/host/test_sensors && build/host/test_board_wifi && build/host/test_release   # 955 checks, -Werror
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

I test su computer richiedono un compilatore C++17 (Linux, WSL, MSYS2). Vedi la [messa in servizio da banco](docs/BENCH_BRINGUP.md), il [pannello](docs/NODE_PANEL.md) e il [confine hardware](docs/HARDWARE_BOUNDARY.md).

## 🔗 Progetti correlati

**A.R.M.O.R.** (Autonomous Radar & Multimodal Observation Range) è un sistema di sicurezza perimetrale fatto di repository indipendenti. Ognuno ha la propria versione, i propri test e il proprio README; ecco la famiglia:

* **[ARMOR-COMMON](https://github.com/JuanenRac/ARMOR-COMMON)** - Contratti dei messaggi, validatori, vettori di conformità e tipi generati
* **ARMOR-RADAR** (questo repository) - Firmware del nodo di campo per ESP32-S3 con tre radar e un proprio pannello web
* **[ARMOR-SOLAR](https://github.com/JuanenRac/ARMOR-SOLAR)** - Protocolli di inverter e batterie solari e messaggi di un nodo gateway
* **[ARMOR-ELECTRICAL](https://github.com/JuanenRac/ARMOR-ELECTRICAL)** - Nodo elettrico: contatori, il messaggio delle letture della rete e le regole di manovra
* **[ARMOR-ALARM](https://github.com/JuanenRac/ARMOR-ALARM)** - Nodo e centrale d'allarme: zone, inserimento, ritardi, sirena e PIN, con il server o senza
* **[ARMOR-HMI](https://github.com/JuanenRac/ARMOR-HMI)** - Pannello touch: lo stato del sistema su uno schermo a parete, attivare e riconoscere gli allarmi, e la casa dell'assistente vocale
* **[ARMOR-NETWORK](https://github.com/JuanenRac/ARMOR-NETWORK)** - La rete locale: i suoi dispositivi, internet e ciò che cambia
* **[ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER)** - Coordinatore centrale: telemetria, allarmi, dispositivi, letture solari e telecamere
* **[ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO)** - Console web: telecamere, radar, allarmi, energia solare e progettista del sito 2D/3D
* **[ARMOR-ANDROID-CONTROL](https://github.com/JuanenRac/ARMOR-ANDROID-CONTROL)** - Client Android dell'operatore con radar 2D/3D in tempo reale
* **[ARMOR-SERVER-AI](https://github.com/JuanenRac/ARMOR-SERVER-AI)** - Politica di inferenza visiva che spiega le sue decisioni e non agisce mai
* **[ARMOR-VISION](https://github.com/JuanenRac/ARMOR-VISION)** - Zone delle telecamere, maschere e regole di rilevamento per la Jetson Orin NX
* **[ARMOR-VOICE-AI](https://github.com/JuanenRac/ARMOR-VOICE-AI)** - Intenti vocali offline con una conferma impossibile da falsificare
* **[ARMOR-HARDWARE](https://github.com/JuanenRac/ARMOR-HARDWARE)** - Contenitori, elettronica e matrice di accettazione da banco
* **[ARMOR-DEVOPS](https://github.com/JuanenRac/ARMOR-DEVOPS)** - Distribuzione, banco di prova CM5, backup e TLS
* **[ARMOR-SIMULATOR](https://github.com/JuanenRac/ARMOR-SIMULATOR)** - Simulatore di telemetria offline con guasti ripetibili
* **[ARMOR-UPDATER](https://github.com/JuanenRac/ARMOR-UPDATER)** - Rileva, installa e aggiorna i repository stessi dell'ecosistema
* **[ARMOR-DOCS](https://github.com/JuanenRac/ARMOR-DOCS)** - Architettura, base di sicurezza e matrice delle capacità

## 📚 Documentazione e comunità

Dove leggere di più:

* [Matrice delle capacità: cosa è provato e cosa no](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/CAPABILITY_MATRIX.md)
* [Catalogo dei progetti: versioni e dipendenze tra i repository](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/PROJECT_CATALOG.md)
* [Cronologia delle modifiche di questo repository](CHANGELOG.md)
* [Licenza (GPL-3.0-or-later)](LICENSE)
* Domande, idee e segnalazioni: electrohobby3d@gmail.com

## 👤 AUTORE

**JuanenRac (Electro Hobby 3D)** · electrohobby3d@gmail.com

## 📜 LICENZA

GPL-3.0-or-later - vedi [LICENSE](LICENSE).
