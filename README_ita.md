<p align="center">
  <img src="images/ARMOR_BANNER.svg" alt="Banner di ARMOR-NODE-RADAR" width="100%">
</p>

# 📡 ARMOR-NODE-RADAR

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
  <img src="https://img.shields.io/badge/License-GPL%203.0--or--later-blue.svg" alt="GPL 3.0 o successiva">
  <img src="https://img.shields.io/badge/Language-C%2B%2B17-00599c.svg" alt="Linguaggio">
  <img src="https://img.shields.io/badge/Target-ESP32--S3-e7352c.svg" alt="Target">
  <img src="https://img.shields.io/badge/Tests-7371%20checks-2ea44f.svg" alt="Test">
  <img src="https://img.shields.io/badge/Version-0.8.0.7-0f6cbd.svg" alt="Versione">
  <img src="https://img.shields.io/badge/Maturity-established-2EA44F.svg" alt="Maturità">
</p>

---

**Controllo di onestà - cosa funziona oggi:** Il nodo radar è l'unico nodo che esiste come hardware: il suo firmware gira su una vera scheda Waveshare ESP32-S3-ETH con un radar e riferisce al banco di prova CM5, e il suo pannello web e gli aggiornamenti via etere vi sono in uso. Il nucleo indipendente dall'hardware (4,633 controlli in 14 programmi di test: i decodificatori dell'LD2450, dell'LD2461 e di quattro sensori di presenza, il canale di comando dell'LD2450, le impostazioni e i loro controlli, la tabella dei pin, utenti e sessioni, il piano di rete, il serializzatore di messaggi la cui uscita è accettata da ARMOR-COMMON, e i controlli di versione e firma) è provato su un computer, il **pannello web** è stato esercitato in un vero browser contro un nodo sostitutivo e l'**immagine del firmware si compila** nel contenitore ESP-IDF 5.5.5. Ciò che **non** è ancora provato: i decodificatori dell'LD2461 e dei sensori di presenza corrispondono solo agli esempi svolti dei loro manuali, senza un modulo dietro; il controllo della firma degli aggiornamenti e l'abbinamento Bluetooth con PIN sono provati su computer con firme vere ma non ancora sulla scheda; il canale di comando del radar non è verificato con un modulo; e il certificato HTTPS del pannello (fatto dal nodo stesso) non è provato in tutti i browser.

---

## 📋 In breve

| | |
|---|---|
| Versione | `0.8.0.7` |
| Maturità | Consolidato |
| Ruolo | Nodo di campo radar per ESP32-S3 (famiglia: Field Nodes, padre: ARMOR-COMMON) |
| Tecnologia | C++17, ESP-IDF 5.5.5, ESP32-S3 |
| Test | 7371 verifiche in 14 programmi host e 6 verifiche Python (`ctest`) |
| Schede | Waveshare ESP32-S3-ETH (Ethernet) e ESP32-S3-WROOM-1 N16R8 (solo Wi-Fi) |
| Sensori | Fino a 3 radar (LD2450, LD2461) o sensori di presenza per nodo, sensore di luce VEML7700 |
| Gira su | Waveshare ESP32-S3-ETH o ESP32-S3-WROOM-1 N16R8; test su host su qualsiasi computer |
| Su hardware reale | Gira su una vera scheda Waveshare ESP32-S3-ETH con un radar, riferendo al banco di prova CM5 |
| Licenza | GPL-3.0-or-later |

**Vai a:** [Panoramica](#-panoramica) · [Architettura](#-architettura) · [Sicurezza](#-sicurezza-e-protezione) · [Interfacce](#-interfacce) · [Configurazione](#-configurazione) · [Avvio rapido](#-avvio-rapido) · [Test](#-test) · [Problemi](#-risoluzione-dei-problemi) · [Struttura](#-struttura-del-repository) · [Correlati](#-progetti-correlati)

## 🚦 Stato del progetto

Ogni riga è una parte di questo repository e il semaforo dice a che punto è oggi. È scritta in base a ciò che il repository può mostrare (test, compilazioni, esecuzioni), non a ciò che è previsto.

🟢 Fatto e verificato · 🟡 Funziona, con limiti noti · 🟠 Fatto a metà o non ancora verificato · 🔴 Non fatto o non funzionante

| Area | Stato | Cosa significa qui |
|---|---|---|
| Software | 🟢 | Il firmware compila nel contenitore ESP-IDF 5.5.5 e gira su una scheda ESP32-S3-ETH reale. |
| Test | 🟢 | Superano 7.371 controlli in 14 programmi host (`ctest`): i decodificatori, le impostazioni, la tabella dei pin, gli utenti, il piano di rete, il serializzatore dei messaggi e i controlli di versione. |
| Protocolli e dati | 🟡 | L'LD2450 funziona con un radar reale; i decodificatori dell'LD2461 e dei quattro sensori di presenza corrispondono solo agli esempi dei loro manuali e il canale di comando del radar non è stato verificato con un modulo. |
| Interfaccia | 🟡 | Il pannello web è in uso sulla scheda e provato in un browser reale contro un nodo simulato; il suo certificato HTTPS, creato dal nodo stesso, non è stato provato in tutti i browser. |
| Hardware | 🟡 | È l'unico nodo che esiste come hardware: una scheda Waveshare ESP32-S3-ETH reale con un radar riferisce al banco di prova CM5; nessun altro modulo sensore è collegato. |
| Sicurezza | 🟡 | L'abbinamento con PIN Bluetooth e il controllo della firma degli aggiornamenti sono provati su un computer con firme reali ma non ancora sulla scheda; non c'è stata una revisione indipendente. |
| Aggiornamenti | 🟡 | Gli aggiornamenti via etere sono in uso sulla scheda; il controllo degli aggiornamenti firmati non vi è ancora stato eseguito. |
| Integrazione | 🟡 | I messaggi della scheda arrivano al server del banco di prova CM5 e gli stessi messaggi sono accettati dagli schemi condivisi e dai vettori di conformità di ARMOR-COMMON. Visto solo sul banco di prova, non in un'installazione. |
| Documentazione | 🟢 | Otto lingue e il registro delle modifiche sono aggiornati. |

## 🎯 Panoramica

* **Due schede, un solo firmware:** la Waveshare ESP32-S3-ETH (Ethernet, predefinita) e una ESP32-S3-WROOM-1 N16R8 senza Ethernet (solo Wi-Fi: la configurazione chiede la rete Wi-Fi a cui unirsi e il nodo tiene sempre la propria rete). L'immagine si sceglie in compilazione (`tools/build_node.sh generic s3-eth` o `generic s3-wifi`); la tabella dei pin e l'accesso seguono la scheda, il pannello nasconde ciò che la scheda non ha e un'immagine vale solo per la sua scheda. Entrambe compilano e sono testate sul computer; nessuna ha girato su una scheda.
* **Un nodo copre fino a 270 gradi:** ogni Waveshare ESP32-S3-ETH legge fino a tre radar LD2450 sulle sue tre UART, montati a 75 gradi l'uno dall'altro, via Ethernet cablata (W5500) con DHCP o indirizzo fisso, alimentato con PoE o USB. *Add a 270° node* di Studio crea i tre radar già associati al nodo.
* **Un pannello web su ogni nodo,** con l'aspetto di Studio e in otto lingue, incorporato nel firmware: panoramica, rete, Wi-Fi, broker, radar, pin, utenti, aggiornamento del firmware e registro. Un nodo senza utenti apre il Wi-Fi `ARMOR-SETUP-xxxxxx` e crea il suo primo amministratore con un codice di configurazione; le password sono PBKDF2 con sale, le sessioni sono token casuali e ogni impostazione è nella flash del nodo, quindi un'unica immagine serve tutti i nodi e nessuna password è compilata ([il pannello](docs/NODE_PANEL.md)).
* **Un solo Wi-Fi da più nodi:** ogni nodo può offrire un access point collegato alla sua porta Ethernet; con lo stesso nome e password e il canale in automatico (1, 6 o 11 secondo la MAC), telefoni e sensori Wi-Fi vedono una sola rete con un solo server DHCP. Non è una rete mesh radio: ogni nodo mantiene il suo cavo. Un nodo può invece unirsi al Wi-Fi di un router (con ricerca delle reti), e uno senza cavo può essere configurato via **Bluetooth** dall'app Android.
* **Pin per il server:** qualsiasi pin libero diventa un ingresso, un'uscita (con uno stato sicuro se il broker si perde), PWM o una lettura analogica, e appare come dispositivo del server: un relè o un contatto non richiede un nuovo firmware. I pin riservati della scheda non vengono mai offerti.
* **Aggiornamenti via radio (OTA) con ripristino** dal pannello (due slot sulla flash da 16 MB) e un **collegamento da Studio** al pannello di ogni nodo, dall'indirizzo che il nodo pubblica.
* **Il pannello in HTTPS:** il nodo crea il proprio certificato (autofirmato, conservato in flash) e serve il pannello sulla porta 443 oltre che sulla 80, o solo sulla 443; il cookie di sessione è marcato Secure e l'impronta del certificato viene mostrata per confrontarla con l'avviso del browser.
* **Identità di traccia stabili:** i bersagli di ogni frame sono associati a quelli seguiti prima, così una persona mantiene lo stesso id, un frame perso non fa sfarfallare e le posizioni sono un po' levigate.
* **Sei modelli di sensore, uno per porta:** una porta porta un LD2450 o un LD2461 (tracker che alimentano il perimetro, con il campo proprio del modello in Studio) o un sensore di presenza (LD2410, LD2412, LD2410S, MR24HPC1) che diventa un dispositivo del server e pubblica presenza e distanza. Si sceglie nel pannello; cos'è ciascuno, il suo protocollo e cosa non è stato verificato sono in `docs/SENSORS.md`.
* **Decodificatore, salute e configurazione dell'LD2450:** un framer che si risincronizza trova i frame in un flusso rumoroso; ogni frame da 30 byte dà fino a tre bersagli che diventano tracce del contratto; la console e il pannello dicono per radar se trasmette, tace o manda dati illeggibili. Il pannello può anche leggere la versione del modulo, scegliere uno o tre bersagli e impostare zone di rilevamento (un protocollo non verificato su un modulo).
* **Luce ambiente e messaggi esatti rispetto al contratto:** il VEML7700 con range automatico; JSON di telemetria, salute e informazioni che segue gli schemi pubblicati e rifiuta di scrivere qualsiasi cosa non valida, con timestamp di orologio reale (SNTP) e un last will MQTT. La telemetria è trattenuta finché nessun radar trasmette, mai un «tutto tranquillo» vuoto.
* **Un'immagine per tutte le schede, come un prodotto di rete:** il firmware è lo stesso e la MAC distingue le schede (`armor-` e sei cifre finché non ricevono un nome); `adopt_node.py` dà a un nodo appena flashato il suo amministratore, la sua identità di broker e le impostazioni comuni della flotta via rete, con un codice di configurazione calcolato dalla sua MAC: 5 nodi o 27 sono lo stesso lavoro.
* **Strumenti da banco:** flash via USB-C, un nodo finto per lavorare sul pannello e un convertitore da un registro di frame grezzi a un fixture di test ([messa in servizio da banco](docs/BENCH_BRINGUP.md)).
* **Aggiornamenti via etere firmati:** il nodo installa da GitHub solo un'immagine il cui SHA-256 coincide e che porta una firma Ed25519 valida del responsabile (la chiave pubblica è nel firmware, quella privata non lascia mai il responsabile); un file caricato a mano da un amministratore nel pannello resta accettato. Le pagine e i messaggi del pannello sono in otto lingue.
* **Abbinamento Bluetooth con PIN:** il PIN a sei cifre deriva dal codice di configurazione, è stampato sulla console USB e mostrato a un amministratore nel pannello; il telefono deve inserirlo prima di poter scrivere le impostazioni del nodo.
* **Solo la casa può usare il pannello:** il pannello risponde per impostazione predefinita alla rete privata (o alle reti indicate dal proprietario, come una VPN), rifiuta un elenco che escluderebbe chi lo salva, conta le password errate per persona e per indirizzo e funziona dietro il proxy del server per essere raggiunto da fuori casa senza pubblicare il nodo.
* **Collegamento cifrato facoltativo con il broker (TLS):** disattivato per impostazione predefinita, quindi il nodo parla MQTT non cifrato come prima. Un interruttore e una casella per il certificato dell'autorità propria dell'installazione (PEM, al massimo 2 KB) nella pagina Broker lo fanno collegare con `mqtts://`, fidarsi solo di quell'autorità e verificare che il certificato del broker sia stato fatto per l'indirizzo raggiunto; ciò che non è un solo certificato PEM viene rifiutato. Coperto da test sul computer; l'handshake non è ancora stato eseguito su una scheda.
* **Rapporto di salute:** circa una volta al minuto il nodo pubblica da quanto è acceso e perché si è riavviato l'ultima volta, la memoria libera, il collegamento e il segnale Wi-Fi, quante volte si è connesso al broker e l'ultimo errore registrato (`armor/health/<nodo>`, il contratto `node_health` di ARMOR-COMMON). Il codice che lo costruisce è condiviso da tutti i nodi e testato su un computer; non è mai girato su una scheda.
* **Controllo dal server:** Studio può riavviare il nodo in modo pulito, metterlo in pausa fino a un'ora (riprende da solo), mettere il suo lavoro in standby (senza fine, conservato ai riavvii, il nodo resta connesso e continua a rispondere), farlo mostrare, chiedere un'istantanea diagnostica e segnarlo in manutenzione (`armor/control/<nodo>/...`, il contratto `node_control` di ARMOR-COMMON). Ogni ordine è in due passi con un token monouso legato a quell'operazione, solo il server può scriverli sul broker e non esistono ripristino di fabbrica, cancellazione né sonno profondo. Finché il nodo è in pausa o in standby non pubblica bersagli né presenza (il battito continua a dire che c'è). Il codice è condiviso da tutti i nodi e provato su un computer; non ha ancora girato su una scheda.
* **Un motore di perimetro sul nodo (spento per impostazione predefinita):** sei zone di rilevamento e tre di esclusione, ciascuna un rettangolo ruotabile di un angolo, tre linee di attraversamento che contano i passaggi per direzione, tre percorsi d'ingresso, immunità agli animali domestici e rilevamento del mascheramento. Funziona accanto ai sensori, quindi continua senza accesso al broker, conserva i suoi eventi finché il broker è assente e li pubblica su `armor/radar/<nodo>/events`; l'editor delle zone è nel pannello del nodo. Nulla di tutto ciò è ancora stato provato con un sensore su una scheda.
* **Chi è ciascun sensore, e il suo Bluetooth proprio chiuso:** all'avvio il nodo legge la versione del firmware e l'indirizzo Bluetooth di ogni sensore, chiude per impostazione predefinita il Bluetooth dei sensori (non richiede PIN) e può affidare la linea seriale di un sensore allo strumento PC di un amministratore per qualche minuto (un ponte seriale trasparente). `GET /metrics` per gli amministratori e un'esportazione e importazione tutto o niente delle impostazioni completano il quadro.
* **Diagnostica senza cavo:** dopo un avvio non pulito (un blocco, un watchdog, un calo di alimentazione, un'allocazione fallita) il rapporto di salute ne indica la causa per dieci minuti; un arresto anomalo lascia il suo core dump da vedere, scaricare o cancellare dal pannello; le impostazioni salvate vengono copiate prima di ogni installazione del firmware e recuperate se un aggiornamento le danneggia; e un amministratore può chiedere al nodo, tramite il server, un pacchetto di diagnostica o il suo registro con un token monouso.
* **Adozione via HTTPS con impronta fissata:** `tools/adopt_node.py` accetta `https://<indirizzo>` con `--fingerprint` (lo SHA-256 del certificato del nodo, verificato sulla stessa connessione che porta la richiesta) o `--show-fingerprint` per leggerla; avverte del HTTP in chiaro e di un file di flotta con IP fisso o nome host, e anche il pannello avverte all'importazione di un file così.
* **Il lettore dei sensori attende eventi invece di interrogare:** il task di lettura dorme sulle code di eventi delle linee seriali e svuota tutto ciò che è arrivato, svuota la linea in caso di overflow e conta overflow ed errori per linea; `system.reader_idle_ms` (100; 0 = il polling da 10 ms di prima) fissa l'attesa, e una coda che si guasta riporta da sola al polling.
* **Sigillatura facoltativa dei segreti salvati:** disattivata per impostazione predefinita (`security.seal_secrets`, o `nodes.nvs_encryption` del server). Attiva, le password, i token, le chiavi, i PIN e gli hash delle password che il nodo salva restano sigillati con ChaCha20-Poly1305 e tutto il resto resta leggibile; attivarla o disattivarla li riscrive senza perdere nulla, anche se il nodo si ferma a metà. Nessun eFuse viene bruciato e la flash non è cifrata: la chiave si trova in una partizione dello stesso chip, quindi **non** protegge da chi ha la scheda, e un nodo installato prima di quella partizione mantiene i suoi segreti com'erano finché non viene flashato con la nuova tabella delle partizioni.

## 🧩 Ruolo in A.R.M.O.R.

ARMOR-NODE-RADAR sono gli occhi del perimetro. Ogni nodo legge fino a tre radar o sensori di presenza e pubblica tracce, luce e salute verso [ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER) via MQTT; [ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO) le disegna sulla planimetria del sito e [ARMOR-SERVER-AI](https://github.com/JuanenRac/ARMOR-SERVER-AI) le pesa rispetto alle telecamere. Il suo firmware condivide una base con gli altri nodi (il pannello, la configurazione Bluetooth, gli aggiornamenti firmati, l'accesso e l'orologio vengono dalla base firmware di [ARMOR-COMMON](https://github.com/JuanenRac/ARMOR-COMMON)), così ciò che impari su un nodo vale per tutti.

| Nome | Come | A cosa serve |
|---|---|---|
| ARMOR-SERVER | MQTT `armor/node/<id>/telemetry|health|info|command` | Riceve i messaggi del radar (oggi il banco CM5) e comanda il nodo |
| ARMOR-STUDIO | collegamento al pannello | Apre il pannello del nodo dall'indirizzo che il nodo pubblica |
| ARMOR-ANDROID-CONTROL | Bluetooth | Configura il nodo sul campo |
| ARMOR-COMMON | contratti, base firmware | I messaggi sono verificati con i suoi schemi; il firmware comune è copiato da lì |

## 🏛️ Architettura

<p align="center">
  <img src="images/architecture.svg" alt="Architettura di ARMOR-NODE-RADAR: chi lo chiama, cosa contiene e da cosa dipende" width="100%">
</p>

## 🖼️ Schermate

Catturate a 1920×1080 su un banco di prova, con dati di esempio e senza alcuna installazione reale dietro.

<table>
<tr><td width="50%" valign="top"><img src="images/screenshots/overview.png" alt="Riepilogo: lo stato del nodo in breve - rete, broker, firmware e ciò che riporta." width="100%"><br><sub>Riepilogo: lo stato del nodo in breve - rete, broker, firmware e ciò che riporta.</sub></td><td width="50%" valign="top"><img src="images/screenshots/radars.png" alt="Radar: ogni radar del nodo, il suo modello, i suoi pin e la sua calibrazione." width="100%"><br><sub>Radar: ogni radar del nodo, il suo modello, i suoi pin e la sua calibrazione.</sub></td></tr>
<tr><td width="50%" valign="top"><img src="images/screenshots/pins.png" alt="Pin: cosa fa ogni pin, verificato perché due funzioni non ne condividano mai uno." width="100%"><br><sub>Pin: cosa fa ogni pin, verificato perché due funzioni non ne condividano mai uno.</sub></td><td width="50%" valign="top"><img src="images/screenshots/network.png" alt="Rete: l'indirizzamento Ethernet o Wi-Fi del nodo, impostato dal suo pannello web." width="100%"><br><sub>Rete: l'indirizzamento Ethernet o Wi-Fi del nodo, impostato dal suo pannello web.</sub></td></tr>
</table>

## 🔒 Sicurezza e protezione

| Minaccia | Difesa |
|---|---|
| Password in chiaro o compilate | PBKDF2-HMAC-SHA256 con 10 000 round; nessuna password nel firmware (un nodo senza utenti entra in configurazione); al massimo 4 utenti (admin o visualizzatore) e da 8 a 64 caratteri; al massimo 6 sessioni, 30 minuti di inattività |
| Prendere un nodo appena programmato | La configurazione richiede un codice di avvio: quello della build (`CONFIG_ARMOR_SETUP_CODE`) o uno casuale tenuto in flash e mostrato sulla console USB; con un segreto di flotta è `HMAC-SHA256(segreto, MAC)`; è anche la chiave del Wi-Fi di configurazione (WPA2, mai in bridge) |
| Il pannello raggiunto da Internet | Risponde di default alle reti private di una casa (10/8, 172.16/12, 192.168/16, 169.254/16, 127/8 e 100.64/10 dove vivono alcune VPN); l'impostazione `web.admin_sources` elenca altri indirizzi e reti IPv4 (8 voci, 120 caratteri); tutto il resto è rifiutato tranne l'identità pubblica `GET /api/v1/session`; una lista che escluderebbe chi la salva è rifiutata |
| Il pannello in chiaro su HTTP | HTTPS sulla 443 con un certificato che il nodo crea da sé (autofirmato, in flash, impronta SHA-256 visibile per confrontarla); il pannello può essere http, https o entrambi; il cookie di sessione è sempre HttpOnly e SameSite=Strict (e Secure su TLS) |
| Un firmware falso da GitHub | Un'immagine si installa solo con uno SHA-256 coincidente e una firma Ed25519 valida fatta con la chiave del manutentore (`no_checksum`, `no_signature`, `bad_signature`, `checksum_mismatch`, `wrong_firmware`, `not_newer`, `version_mismatch` la rifiutano); un caricamento manuale da un amministratore non è verificato: un limite esplicito |
| Dirottamento della configurazione Bluetooth | Un PIN a 6 cifre derivato dal codice di configurazione, visibile sulla console USB e a un amministratore; il telefono deve digitarlo prima di scrivere le impostazioni; la finestra dura `CONFIG_ARMOR_BLE_SETUP_MINUTES` (10 di default, da 1 a 120) |
| Un ordine remoto che fa danni | Due passi con un token monouso legato a quell'operazione; solo il server può scrivere quei topic nel broker; una pausa dura al massimo un'ora; non ci sono ripristino di fabbrica, cancellazione né sonno profondo da ordine remoto |
| Segreti memorizzati letti da chi ha la scheda | Opzionale (spento di default, interruttore `security.seal_secrets` o `nodes.nvs_encryption` del server): ChaCha20-Poly1305 per password, token, chiavi, PIN e hash. Non cifra la flash né brucia un eFuse: la chiave è in una partizione dello stesso chip, quindi non protegge da chi ha la scheda |
| Il collegamento al broker in chiaro | TLS opzionale (`mqtts://`) con l'autorità propria del proprietario (PEM fino a 2 KB) e verifica del nome; spento di default. L'handshake è girato solo nei test su host, non su una scheda |
| Il Bluetooth proprio di un sensore | Di default il nodo chiude il Bluetooth di ogni sensore LD (senza PIN); un ponte seriale trasparente verso un computer si apre solo per pochi minuti e da un amministratore |
| Adottare un nodo su una rete non fidata | `tools/adopt_node.py https://<indirizzo> --fingerprint <SHA-256>` verifica il certificato nella stessa connessione; avvisa per l'HTTP in chiaro e per i file di flotta con indirizzo fisso |
| Un pin pericoloso | I pin riservati della scheda non vengono mai offerti; un'uscita ha uno stato sicuro se si perde il broker; due funzioni non possono condividere un pin (verificato) |
| Un firmware pubblico con segreto di flotta | `CONFIG_ARMOR_FLEET_SECRET` è vuoto di default: la build pubblica non porta alcun segreto |

Leggi [SECURITY.md](SECURITY.md) per segnalare una vulnerabilità. Non c'è stata una revisione indipendente.

## 🔌 Interfacce

| Parte | Cosa imposta |
|---|---|
| MQTT | `armor/node/<id>/{telemetry,health,info,command}`, `armor/health/<id>` (il suo last will è `{"node_id":"..","online":false}`), `armor/control/<id>/{command,result,maintenance}` e `armor/control/all/command`, `armor/device/<id>/<pin>/{state,set}`, `armor/radar/<id>/events`, `armor/radar/<id>/control/armed` |
| Pannello (HTTP 80, HTTPS 443) | Percorsi `session`, `login`, `logout`, `setup`, `status`, `config` (+ export, import), `wifi/scan`, `time`, `radars` (+ command), `perimeter` (+ armed, learn, zone), `pins` (+ command), `bridge` (+ open, close, io), `ota` (+ check, install, progress, copy, switch), `account`, `ble-pin`, `log`, `coredump`, `factory-reset`, `reboot`; `/metrics` per gli amministratori e il WebSocket `/ws/radar-map`; `tests/check_routes.py` verifica che ogni percorso chiamato dal pannello esista |
| Testi del pannello | 76 codici di errore con testo in otto lingue (832 righe in `panel/text.js`), verificati da `tests/check_panel_text.py` |
| Hardware | 3 UART per LD2450, LD2461, LD2410, LD2412, LD2410S e MR24HPC1 (RX 16, 17, 18; TX 15, 21, 38); W5500 su SPI (MOSI 11, MISO 12, SCLK 13, CS 14, INT 10, RST 9, 20 MHz); VEML7700 su I2C (GPIO 1 e 2); pulsante BOOT |

## ⚙️ Configurazione

| Variabile | Predefinito | Cosa imposta |
|---|---|---|
| `CONFIG_ARMOR_NODE_ID` | `""` | Il nome; vuoto dà `armor-` e gli ultimi tre byte del MAC |
| `CONFIG_ARMOR_SETUP_CODE`, `CONFIG_ARMOR_FLEET_SECRET` | `""` | Il codice di configurazione della build (vuoto: casuale) e il segreto di flotta (16+ caratteri; mai nel repository) |
| `CONFIG_ARMOR_RADAR_COUNT` | 3 (0-3) | Quanti radar sono collegati |
| `CONFIG_ARMOR_TRACK_STABILISER` | y | Mantenere l'identità di una traccia da un fotogramma all'altro |
| `CONFIG_ARMOR_MQTT_URI`, `_USERNAME`, `_PASSWORD` | `""` | Le prime impostazioni del broker (modificabili nel pannello) |
| `CONFIG_ARMOR_HEARTBEAT_S` | 10 (2-300) | Secondi tra i battiti di salute |
| `CONFIG_ARMOR_TELEMETRY_PERIOD_MS` | 200 (200-5000) | Al massimo 5 messaggi al secondo |
| `CONFIG_ARMOR_BLE_SETUP_MINUTES` | 10 (1-120) | Quanto resta aperta la configurazione Bluetooth |
| `CONFIG_ARMOR_BOARD_S3_ETH` / `_S3_WIFI` | eth | La scheda: Waveshare ESP32-S3-ETH o ESP32-S3-WROOM-1 N16R8 (solo Wi-Fi) |
| `CONFIG_ARMOR_BOOT_FACTORY_RESET` | y | Tieni BOOT 8 s nei primi 30 s per ricominciare |
| `system.login_limit_per_minute` | 30 (0-120) | Password errate al minuto prima che il pannello blocchi l'accesso a tutti (0 = nessun limite globale); si legge e si cambia da Studio (un pulsante su ogni nodo) o con `POST /api/v1/admin/nodes/settings`, dove l'accesso al pannello è usato una volta e mai conservato |

Le impostazioni regolate dal vivo sono nel pannello e salvate in flash (ad esempio `system.reader_idle_ms`, `security.seal_secrets`, il limite di accessi falliti al minuto (30, da 0 a 120) e le zone del motore di perimetro); i valori predefiniti del firmware sono in `sdkconfig.defaults` e `sdkconfig.board.*`. La flash da 16 MB contiene `nvs` (64 KB), `otadata`, `phy_init`, due slot da 3 MB `ota_0` e `ota_1`, `coredump` (128 KB) e `nvs_keys` (4 KB) (`partitions.csv`).

## 🚀 Avvio rapido

**Requisiti:** un compilatore C++17 per i test su host; il container ESP-IDF 5.5.5 per costruire l'immagine; una Waveshare ESP32-S3-ETH o una ESP32-S3-WROOM-1 N16R8.

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

I test su computer richiedono un compilatore C++17 (Linux, WSL, MSYS2). Vedi la [messa in servizio da banco](docs/BENCH_BRINGUP.md), il [pannello](docs/NODE_PANEL.md) e il [confine hardware](docs/HARDWARE_BOUNDARY.md).

## 🧪 Test

```bash
cmake -S tests -B build/host && cmake --build build/host && ctest --test-dir build/host
build/host/emit_samples | python tests/check_contract.py
python -I tools/armor_ci_validate.py
```

14 programmi di test su host e 6 verifiche Python girano con `ctest`; 7371 verifiche contate dai programmi host, compilati con `-Werror`.

| Parte | Cosa copre |
|---|---|
| `test_core`, `test_sensors`, `test_family`, `test_uart_wake` | I decodificatori (LD2450, LD2461, quattro sensori di presenza), il delimitatore di frame, il canale dei comandi, le famiglie di sensori e il risveglio della UART |
| `test_node`, `test_base`, `test_shared`, `test_board_wifi` | Le impostazioni e le loro verifiche, la tabella dei pin, utenti e sessioni, il piano di rete, la base condivisa e la scheda Wi-Fi |
| `test_logic`, `test_perimeter`, `test_control`, `test_diag` | La logica dei pin, il motore di perimetro, gli ordini di controllo in due passi e la diagnostica |
| `test_mqtt_tls`, `test_release` | Il collegamento TLS al broker (solo host) e le verifiche di versione e firma |
| `check_panel_text`, `check_task_safety`, `check_save_paths`, `check_ble_session`, `check_routes`, `check_bridge_client` | Python: i testi del pannello in otto lingue (832 righe), i task, i percorsi di salvataggio, la sessione Bluetooth sotto il suo lock, i percorsi e il client del ponte seriale |

Il pannello web è anche provato in un browser reale contro un nodo sostitutivo (`tools/panel_mock.mjs`). I decodificatori LD2461 e di presenza coincidono solo con gli esempi dei loro manuali.

## 🏗️ Compilazione e rilascio

`tools/build_node.sh generic` costruisce UNA immagine per ogni scheda Waveshare nel container ESP-IDF 5.5.5, e `tools/flash.bat` la scrive via USB-C; lo stesso firmware è costruito per la scheda solo Wi-Fi con `s3-wifi`. Le versioni portano l'immagine, il suo SHA-256 e la sua firma Ed25519, una per scheda.

## 🩺 Risoluzione dei problemi

| Vedi | Perché | Fai |
|---|---|---|
| Hai perso la password o il nodo non entra più in rete | Un nodo in configurazione senza indirizzo dopo 90 s apre un proprio Wi-Fi `ARMOR-SETUP-xxxxxx` (apri `http://192.168.4.1/`) | Per ricominciare tieni premuto BOOT per 8 secondi nei primi 30 secondi dopo l'accensione (cancella impostazioni e utenti), o usa *Ripristino di fabbrica* nel pannello |
| Un aggiornamento da GitHub è rifiutato | La versione non ha checksum o firma, non corrispondono (`e_no_checksum`, `e_no_signature`, `e_checksum_mismatch`, `e_bad_signature`), non ha un'immagine per questa scheda (`e_no_asset_for_board`) o è di un altro progetto (`e_wrong_firmware`) | Il nodo non è cambiato; riprova o lascia che un amministratore carichi il file a mano |
| I comandi al radar rispondono `no_answer`, `refused` o `disabled` | Il filo TX del radar non è collegato, il modulo si è bloccato o il nodo chiude prima il suo Bluetooth | Controlla il cablaggio (vedi la messa in servizio al banco), riavvia il modulo dal pannello |
| Il nodo non mostra il sensore 3 o lascia dormire un radar | Il modulo ha smesso di parlare (silenzio senza frame errati) | Usa *riavvia modulo* nella pagina dei radar, poi chiudi il Bluetooth del sensore |

## 📂 Struttura del repository

<p align="center">
  <img src="images/repository-structure.svg" alt="Struttura del repository di ARMOR-NODE-RADAR" width="100%">
</p>

<details>
<summary>Versione testuale dell'albero</summary>

```text
ARMOR-NODE-RADAR/
├── main/                                il firmware: un componente ESP-IDF
├── panel/                               il pannello web incorporato nel firmware
├── tests/                               test su host e verifiche Python
├── docs/                                7 guide per il banco e il pannello
├── tools/                               strumenti di build, flash, adozione, flotta e verifica
├── .github/                             workflow CI e modelli per issue e pull request
├── images/                              banner, icona e le due immagini di questo README
├── build.sh build.bat                   build tramite il flusso di progetto condiviso
├── test.sh test.bat                     eseguire i test tramite il flusso condiviso
├── run.sh run.bat                       avvio tramite il flusso di progetto condiviso
├── armor.project.json                   manifesto: versione, ruolo, maturità, comando di build
├── CHANGELOG.md                         cosa è cambiato in ogni versione
├── SECURITY.md SUPPORT.md LICENSE       politica di sicurezza, supporto e licenza
├── CONTRIBUTING.md CODE_OF_CONDUCT.md   come contribuire e il codice di condotta
├── CMakeLists.txt                       il progetto ESP-IDF
├── partitions.csv                       la tabella della flash
├── sdkconfig.defaults                   le impostazioni del firmware
├── sdkconfig.board.s3-eth               la scheda Ethernet
├── sdkconfig.board.s3-wifi              la scheda Wi-Fi
├── secrets/                             esempi di file di flotta e nodo
├── build-test.sh build-test.bat         eseguire i test tramite il flusso condiviso
└── .gitattributes .gitignore            impostazioni di Git
```

</details>

## 🗺️ Roadmap e limiti noti

**Limiti noti oggi**

* I decodificatori LD2461 e di presenza coincidono solo con gli esempi dei loro manuali, senza modulo dietro; il canale dei comandi è stato verificato solo con l'LD2450.
* La verifica della firma degli aggiornamenti e l'abbinamento Bluetooth con PIN sono dimostrati su un computer con firme reali ma non ancora sulla scheda; l'handshake TLS con il broker non è girato su una scheda.
* Il rapporto di salute e il controllo dal server non sono girati sulla scheda; il motore di perimetro non è mai girato con un sensore; il certificato HTTPS proprio del pannello non è provato in tutti i browser; la scheda Wi-Fi non esiste fisicamente (esiste solo quella Ethernet).

**Prossimi passi**

* Far girare il motore di perimetro, il rapporto di salute e gli ordini di controllo con sensori reali e il banco CM5; provare il collegamento TLS al broker e l'aggiornamento firmato sulla scheda.

## 🌍 Lingue

Il README e i testi esistono in inglese, spagnolo, francese, italiano, tedesco, cinese, giapponese e portoghese europeo.

## 🔗 Progetti correlati

**A.R.M.O.R.** (Autonomous Radar & Multimodal Observation Range) è un sistema di sicurezza perimetrale fatto di repository indipendenti. Ognuno ha la propria versione, i propri test e il proprio README; ecco la famiglia:

* **[ARMOR-COMMON](https://github.com/JuanenRac/ARMOR-COMMON)** - Contratti dei messaggi, validatori, vettori di conformità e tipi generati
* **ARMOR-NODE-RADAR** (questo repository) - Firmware del nodo di campo per ESP32-S3 con tre radar e un proprio pannello web
* **[ARMOR-NODE-SOLAR](https://github.com/JuanenRac/ARMOR-NODE-SOLAR)** - Protocolli di inverter e batterie solari e messaggi di un nodo gateway
* **[ARMOR-NODE-ELECTRICAL](https://github.com/JuanenRac/ARMOR-NODE-ELECTRICAL)** - Nodo elettrico: contatori, il messaggio delle letture della rete e le regole di manovra
* **[ARMOR-NODE-ALARM](https://github.com/JuanenRac/ARMOR-NODE-ALARM)** - Nodo e centrale d'allarme: zone, inserimento, ritardi, sirena e PIN, con il server o senza
* **[ARMOR-NODE-HMI](https://github.com/JuanenRac/ARMOR-NODE-HMI)** - Pannello touch: lo stato del sistema su uno schermo a parete, attivare e riconoscere gli allarmi, e la casa dell'assistente vocale
* **[ARMOR-NETWORK](https://github.com/JuanenRac/ARMOR-NETWORK)** - La rete locale: i suoi dispositivi, internet e ciò che cambia
* **[ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER)** - Coordinatore centrale: telemetria, allarmi, dispositivi, letture solari e telecamere
* **[ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO)** - Console web: telecamere, radar, allarmi, energia solare e progettista del sito 2D/3D
* **[ARMOR-ANDROID-CONTROL](https://github.com/JuanenRac/ARMOR-ANDROID-CONTROL)** - Client Android dell'operatore con radar 2D/3D in tempo reale
* **[ARMOR-IOS-CONTROL](https://github.com/JuanenRac/ARMOR-IOS-CONTROL)** - Client operatore per iPhone
* **[ARMOR-SERVER-AI](https://github.com/JuanenRac/ARMOR-SERVER-AI)** - Politica di inferenza visiva che spiega le sue decisioni e non agisce mai
* **[ARMOR-VISION](https://github.com/JuanenRac/ARMOR-VISION)** - Zone delle telecamere, maschere e regole di rilevamento per la Jetson Orin NX
* **[ARMOR-VOICE-AI](https://github.com/JuanenRac/ARMOR-VOICE-AI)** - Intenti vocali offline con una conferma impossibile da falsificare
* **[ARMOR-HARDWARE](https://github.com/JuanenRac/ARMOR-HARDWARE)** - Contenitori, elettronica e matrice di accettazione da banco
* **[ARMOR-DEVOPS](https://github.com/JuanenRac/ARMOR-DEVOPS)** - Distribuzione, banco di prova CM5, backup e TLS
* **[ARMOR-SIMULATOR](https://github.com/JuanenRac/ARMOR-SIMULATOR)** - Simulatore di telemetria offline con guasti ripetibili
* **[ARMOR-UPDATER](https://github.com/JuanenRac/ARMOR-UPDATER)** - Rileva, installa e aggiorna i repository stessi dell'ecosistema
* **[ARMOR-DOCS](https://github.com/JuanenRac/ARMOR-DOCS)** - Architettura, base di sicurezza e matrice delle capacità

## 📚 Documentazione

* [Pannello del nodo](docs/NODE_PANEL.md): primo avvio, utenti, rete, le pagine del pannello
* [Codice di configurazione](docs/SETUP_CODE.md) e [provisioning Bluetooth](docs/BLE_PROVISIONING.md)
* [Sensori](docs/SENSORS.md): i sei modelli e i loro decodificatori
* [Messa in servizio al banco](docs/BENCH_BRINGUP.md), [prove al banco](docs/BENCH_TESTS.md) e [limite dell'hardware](docs/HARDWARE_BOUNDARY.md)
* [Matrice delle capacità: cosa è provato e cosa no](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/CAPABILITY_MATRIX.md)
* [Catalogo dei progetti: versioni e dipendenze tra i repository](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/PROJECT_CATALOG.md)
* [Cronologia delle modifiche di questo repository](CHANGELOG.md)

## 🤝 Contribuire e comunità

* Prima di una pull request esegui `ctest --test-dir build/host` and `python -I tools/armor_ci_validate.py`: deve passare. Vedi [CONTRIBUTING.md](CONTRIBUTING.md) e il [codice di condotta](CODE_OF_CONDUCT.md).
* Il README esiste in otto lingue ed è generato dalle sorgenti in ARMOR-DOCS: una modifica al testo si fa lì.
* Una modifica a un messaggio tra componenti parte da ARMOR-COMMON.
* Non descrivere come funzionante su una scheda ciò che è stato solo compilato.
* Segnala una vulnerabilità in privato: [SECURITY.md](SECURITY.md). Domande, idee e segnalazioni: electrohobby3d@gmail.com ([SUPPORT.md](SUPPORT.md)).

## 👤 AUTORE

**JuanenRac (Electro Hobby 3D)** · electrohobby3d@gmail.com

## 📜 LICENZA

GPL-3.0-or-later - vedi [LICENSE](LICENSE).
