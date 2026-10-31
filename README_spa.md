<p align="center">
  <img src="images/ARMOR_BANNER.svg" alt="Banner de ARMOR-NODE-RADAR" width="100%">
</p>

# 📡 ARMOR-NODE-RADAR

<p align="center">
  <a href="README.md">🇺🇸 English</a> |
  🇪🇸 <b>Español</b> |
  <a href="README_fra.md">🇫🇷 Français</a> |
  <a href="README_ita.md">🇮🇹 Italiano</a> |
  <a href="README_deu.md">🇩🇪 Deutsch</a> |
  <a href="README_zho.md">🇨🇳 简体中文</a> |
  <a href="README_jpn.md">🇯🇵 日本語</a> |
  <a href="README_por.md">🇵🇹 Português</a>
</p>

### Firmware del nodo de campo (Waveshare ESP32-S3-ETH, tres radares o sensores de presencia, Ethernet y Wi-Fi) con su propio panel web, y su núcleo probado en el ordenador

<p align="center">
  <img src="https://img.shields.io/badge/License-GPL%203.0--or--later-blue.svg" alt="GPL 3.0 o posterior">
  <img src="https://img.shields.io/badge/Language-C%2B%2B17-00599c.svg" alt="Lenguaje">
  <img src="https://img.shields.io/badge/Target-ESP32--S3-e7352c.svg" alt="Target">
  <img src="https://img.shields.io/badge/Tests-7371%20checks-2ea44f.svg" alt="Pruebas">
  <img src="https://img.shields.io/badge/Version-0.8.0.7-0f6cbd.svg" alt="Versión">
  <img src="https://img.shields.io/badge/Maturity-established-2EA44F.svg" alt="Madurez">
</p>

---

**Comprobación de honestidad - qué funciona hoy:** El nodo radar es el único nodo que existe como hardware: su firmware corre en una placa real Waveshare ESP32-S3-ETH con un radar y reporta al banco de pruebas de la CM5, y su panel web y las actualizaciones por aire se usan en ella. El núcleo independiente del hardware (4,633 comprobaciones en 14 programas de test: los decodificadores del LD2450, el LD2461 y cuatro sensores de presencia, el canal de órdenes del LD2450, los ajustes y sus comprobaciones, la tabla de pines, usuarios y sesiones, el plan de red, el serializador de mensajes cuya salida acepta ARMOR-COMMON, y las comprobaciones de versión y firma) se prueba en un ordenador, el **panel web** se ejercitó en un navegador real contra un nodo de pega y la **imagen del firmware compila** en el contenedor ESP-IDF 5.5.5. Lo que **todavía no** está probado: los decodificadores del LD2461 y de los sensores de presencia solo coinciden con los ejemplos resueltos de sus manuales, sin un módulo detrás; la comprobación de firma de las actualizaciones y el emparejamiento Bluetooth con PIN están probados en ordenador con firmas reales pero aún no en la placa; el canal de órdenes del radar no se ha contrastado con un módulo; y el certificado HTTPS del panel (lo hace el propio nodo) no se ha probado en todos los navegadores.

---

## 📋 De un vistazo

| | |
|---|---|
| Versión | `0.8.0.7` |
| Madurez | Establecido |
| Función | Nodo de campo radar para ESP32-S3 (familia: Field Nodes, padre: ARMOR-COMMON) |
| Tecnología | C++17, ESP-IDF 5.5.5, ESP32-S3 |
| Pruebas | 7371 comprobaciones en 14 programas de host y 6 de Python (`ctest`) |
| Placas | Waveshare ESP32-S3-ETH (Ethernet) y ESP32-S3-WROOM-1 N16R8 (solo Wi-Fi) |
| Sensores | Hasta 3 radares (LD2450, LD2461) o sensores de presencia por nodo, sensor de luz VEML7700 |
| Funciona en | Waveshare ESP32-S3-ETH o ESP32-S3-WROOM-1 N16R8; pruebas de host en cualquier ordenador |
| En hardware real | Corre en una placa real Waveshare ESP32-S3-ETH con un radar, reportando al banco de pruebas de la CM5 |
| Licencia | GPL-3.0-or-later |

**Ir a:** [Resumen](#-resumen) · [Arquitectura](#-arquitectura) · [Seguridad](#-seguridad-y-protección) · [Interfaces](#-interfaces) · [Configuración](#-configuración) · [Inicio rápido](#-inicio-rápido) · [Pruebas](#-pruebas) · [Problemas](#-resolución-de-problemas) · [Estructura](#-estructura-del-repositorio) · [Relacionados](#-proyectos-relacionados)

## 🚦 Estado del proyecto

Cada línea es una parte de este repositorio y la luz dice hasta dónde llega hoy. Está escrito con lo que el repositorio puede demostrar (pruebas, compilaciones, ejecuciones), no con lo que está planeado.

🟢 Hecho y verificado · 🟡 Funciona, con límites conocidos · 🟠 Parcial o sin verificar todavía · 🔴 No hecho o no funciona

| Área | Estado | Qué significa aquí |
|---|---|---|
| Software | 🟢 | El firmware compila en el contenedor ESP-IDF 5.5.5 y funciona en una placa ESP32-S3-ETH real. |
| Pruebas | 🟢 | Pasan 7.371 comprobaciones en 14 programas de host (`ctest`): los decodificadores, los ajustes, la tabla de pines, los usuarios, el plan de red, el serializador de mensajes y las comprobaciones de versión. |
| Protocolos y datos | 🟡 | El LD2450 funciona con un radar real; los decodificadores del LD2461 y de los cuatro sensores de presencia solo coinciden con los ejemplos de sus manuales, y el canal de órdenes del radar no se ha comprobado con un módulo. |
| Interfaz | 🟡 | El panel web se usa en la placa y se ejercita en un navegador real contra un nodo simulado; su certificado HTTPS, creado por el propio nodo, no se ha probado en todos los navegadores. |
| Hardware | 🟡 | Es el único nodo que existe como hardware: una placa Waveshare ESP32-S3-ETH real con un radar informa al banco de pruebas CM5; no hay ningún otro módulo de sensor conectado. |
| Seguridad | 🟡 | El emparejamiento con PIN Bluetooth y la comprobación de la firma de las actualizaciones están probados en un ordenador con firmas reales, pero aún no en la placa; no ha habido una revisión independiente. |
| Actualizaciones | 🟡 | Las actualizaciones por aire se usan en la placa; la comprobación de actualizaciones firmadas aún no se ha ejecutado allí. |
| Integración | 🟡 | Los mensajes de la placa llegan al servidor del banco de pruebas CM5, y esos mismos mensajes los aceptan los esquemas compartidos y los vectores de conformidad de ARMOR-COMMON. Solo se ha visto en el banco, no en una instalación. |
| Documentación | 🟢 | Ocho idiomas y el historial de cambios están al día. |

## 🎯 Resumen

* **Dos placas, un firmware:** la Waveshare ESP32-S3-ETH (Ethernet, la de serie) y una ESP32-S3-WROOM-1 N16R8 sin Ethernet (solo Wi-Fi: su configuración pide la red Wi-Fi a la que unirse y siempre conserva su propia red). La imagen se elige al compilar (`tools/build_node.sh generic s3-eth` o `generic s3-wifi`); la tabla de pines y la forma de acceso siguen a la placa, el panel oculta lo que la placa no tiene y una imagen es solo para su placa. Ambas compilan y tienen tests en el ordenador; ninguna ha funcionado en una placa.
* **Un nodo cubre hasta 270 grados:** cada Waveshare ESP32-S3-ETH lee hasta tres radares LD2450 en sus tres UART, montados con 75 grados de separación, por Ethernet cableada (W5500) con DHCP o dirección fija, alimentada por PoE o USB. *Añadir un nodo de 270°* en Studio crea los tres radares ya asignados al nodo.
* **Un panel web en cada nodo,** con el aspecto de Studio y sus ocho idiomas, integrado en el firmware: resumen, red, Wi-Fi, broker, radares, pines, usuarios, actualización del firmware y registro. Un nodo sin usuarios abre el Wi-Fi `ARMOR-SETUP-xxxxxx` y crea su primer administrador con un código de configuración; las contraseñas usan PBKDF2 con sal, las sesiones son tokens aleatorios y todos los ajustes viven en la flash del nodo, así que una imagen sirve para todos los nodos y no se compila ninguna contraseña ([el panel](docs/NODE_PANEL.md)).
* **Un solo Wi-Fi desde muchos nodos:** cada nodo puede ofrecer un punto de acceso unido a su puerto Ethernet; dales el mismo nombre y contraseña y el canal en automático (1, 6 u 11 según la MAC) y los móviles y sensores Wi-Fi ven una red con un servidor DHCP. No es una malla por radio: cada nodo conserva su cable. Un nodo puede en cambio unirse al Wi-Fi de un router (con búsqueda de redes), y uno sin cable se puede configurar por **Bluetooth** desde la app de Android.
* **Pines para el servidor:** cualquier pin libre pasa a ser una entrada, una salida (con estado seguro si se pierde el broker), PWM o una lectura analógica, y aparece como dispositivo del servidor, así que un relé o un contacto no necesita firmware nuevo. Los pines reservados de la placa nunca se ofrecen.
* **Actualizaciones por el aire con vuelta atrás** desde el panel (dos ranuras en la flash de 16 MB), y un **enlace desde Studio** al panel de cada nodo, a partir de la dirección que el nodo publica.
* **El panel por HTTPS:** el nodo crea su propio certificado (autofirmado, guardado en flash) y sirve el panel también por el puerto 443, o solo por él; la cookie de sesión va marcada como Secure y se muestra la huella del certificado para compararla con el aviso del navegador.
* **Identidades de pista estables:** los objetivos de cada trama se asocian a los que ya se seguían, así que una persona conserva su id, una trama perdida no hace parpadear y las posiciones se suavizan un poco.
* **Seis modelos de sensor, uno por puerto:** un puerto lleva un LD2450 o un LD2461 (rastreadores que alimentan el perímetro, con el campo propio del modelo en Studio) o un sensor de presencia (LD2410, LD2412, LD2410S, MR24HPC1) que pasa a ser un dispositivo del servidor y publica presencia y distancia. Se elige en el panel; qué es cada uno, su protocolo y lo que no se verificó está en `docs/SENSORS.md`.
* **Decodificador LD2450, salud y configuración:** un delimitador que se resincroniza encuentra tramas en un flujo ruidoso; cada trama de 30 bytes da hasta tres objetivos que pasan a ser pistas del contrato; la consola y el panel dicen por radar si informa, está en silencio o llega ininteligible. El panel también puede leer la versión del módulo, elegir uno o tres objetivos y fijar zonas de detección (un protocolo sin comprobar con un módulo).
* **Luz ambiente y mensajes exactos al contrato:** el VEML7700 con rango automático; JSON de telemetría, salud e información que sigue los esquemas publicados y se niega a escribir algo inválido, con marcas de tiempo reales (SNTP) y un last will de MQTT. La telemetría se retiene mientras ningún radar informa, nunca un «todo despejado» vacío.
* **Una imagen para todas las placas, como un producto de red:** el firmware es el mismo y la MAC distingue las placas (`armor-` y seis dígitos hasta que se le da nombre); `adopt_node.py` da a un nodo recién grabado su administrador, su identidad en el broker y los ajustes comunes de la flota por la red, con un código de configuración calculado desde su MAC, así que 5 nodos o 27 dan el mismo trabajo.
* **Herramientas de banco:** grabación por USB-C, un nodo simulado para trabajar en el panel y un conversor de un registro de tramas en bruto a un fixture de pruebas ([puesta en marcha](docs/BENCH_BRINGUP.md)).
* **Actualizaciones por aire firmadas:** el nodo solo instala desde GitHub una imagen cuyo SHA-256 coincide y que lleva una firma Ed25519 válida del responsable (la clave pública está en el firmware, la privada nunca sale del responsable); un archivo subido a mano por un administrador en el panel se sigue aceptando. Las páginas y mensajes del panel están en ocho idiomas.
* **Emparejamiento Bluetooth con PIN:** el PIN de seis cifras se deriva del código de configuración, se imprime en la consola USB y se muestra a un administrador en el panel; el móvil tiene que escribirlo antes de poder cambiar los ajustes del nodo.
* **Solo la casa puede usar el panel:** el panel responde por defecto a la red privada (o a las redes que su dueño indique, como una VPN), rechaza una lista que dejaría fuera a quien la guarda, cuenta las contraseñas erróneas por persona y por dirección, y funciona detrás del proxy del servidor para llegar a él desde fuera de casa sin publicar el nodo.
* **Enlace cifrado opcional con el broker (TLS):** desactivado por defecto, así que el nodo habla MQTT sin cifrar como antes. Un interruptor y una caja para el certificado de la autoridad propia de la instalación (PEM, 2 KB como máximo) en la página Broker hacen que se conecte con `mqtts://`, confíe solo en esa autoridad y compruebe que el certificado del broker se hizo para la dirección a la que llegó; lo que no sea un único certificado PEM se rechaza. Cubierto con pruebas en el ordenador; el handshake aún no se ha ejecutado en una placa.
* **Informe de salud:** cada minuto más o menos el nodo publica cuánto lleva encendido y por qué se reinició la última vez, su memoria libre, su enlace y su señal Wi-Fi, cuántas veces se conectó al broker y el último error que anotó (`armor/health/<nodo>`, el contrato `node_health` de ARMOR-COMMON). El código que lo construye lo comparten todos los nodos y está probado en un ordenador; no se ha ejecutado en una placa.
* **Control desde el servidor:** Studio puede reiniciar el nodo limpiamente, pausarlo hasta una hora (se reanuda solo), poner su trabajo en espera (sin fin, se conserva al reiniciar, el nodo sigue conectado y respondiendo), hacer que se muestre, pedirle una instantánea de diagnóstico y marcarlo en mantenimiento (`armor/control/<nodo>/...`, el contrato `node_control` de ARMOR-COMMON). Cada orden son dos pasos con un token de un solo uso que pertenece a esa operación, solo el servidor puede escribirlas en el broker, y no existe restablecimiento de fábrica, borrado ni sueño profundo. Mientras el nodo está en pausa o en espera no publica objetivos ni presencia (el latido sigue diciendo que está). El código es compartido por todos los nodos y se prueba en un ordenador; no ha funcionado en una placa.
* **Un motor de perímetro en el nodo (apagado por defecto):** seis zonas de detección y tres de exclusión, cada una un rectángulo que se puede girar un ángulo, tres líneas de cruce que cuentan los pasos por sentido, tres rutas de entrada, inmunidad a mascotas y detección de enmascaramiento. Funciona junto a los sensores, así que sigue funcionando sin acceso al broker, guarda sus eventos mientras el broker no está y los publica en `armor/radar/<nodo>/events`; el editor de zonas está en el panel del nodo. Nada de esto se ha probado aún con un sensor en una placa.
* **Quién es cada sensor y su Bluetooth propio cerrado:** al arrancar, el nodo lee la versión de firmware y la dirección Bluetooth de cada sensor, cierra por defecto el Bluetooth propio de los sensores (no pide PIN) y puede ceder la línea serie de un sensor a una herramienta de PC de un administrador durante unos minutos (un puente serie transparente). `GET /metrics` para administradores y una exportación e importación de todo o nada de los ajustes lo completan.
* **Diagnóstico sin cable:** tras un arranque que no fue limpio (un cuelgue, un watchdog, una caída de la alimentación, una reserva de memoria fallida) el informe de salud dice el motivo durante diez minutos; un fallo deja su volcado de núcleo (core dump) para verlo, descargarlo o borrarlo desde el panel; los ajustes guardados se copian antes de cada instalación de firmware y se recuperan si una actualización los daña; y un administrador puede pedir al nodo, a través del servidor, un paquete de diagnóstico o su registro con un token de un solo uso.
* **Adopción por HTTPS con huella fijada:** `tools/adopt_node.py` acepta `https://<dirección>` con `--fingerprint` (el SHA-256 del certificado del nodo, comprobado en la misma conexión que lleva la petición) o `--show-fingerprint` para leerla; avisa del HTTP en claro y de un fichero de flota con IP fija o nombre de host, y el panel también avisa al importar un fichero así.
* **El lector de sensores espera eventos en vez de sondear:** la tarea lectora duerme sobre las colas de eventos de las líneas serie y vacía todo lo recibido, vacía la línea si hubo desbordamiento y cuenta desbordamientos y errores por línea; `system.reader_idle_ms` (100; 0 = el sondeo de 10 ms de antes) fija cuánto espera, y si una cola falla vuelve sola al sondeo.
* **Sellado opcional de los secretos guardados:** desactivado por defecto (`security.seal_secrets`, o `nodes.nvs_encryption` del servidor). Activado, las contraseñas, tokens, claves, PIN y hashes de contraseña que guarda el nodo quedan sellados con ChaCha20-Poly1305 y todo lo demás sigue legible; activarlo o desactivarlo los reescribe sin perder nada, aunque el nodo se pare a medias. No se quema ningún eFuse ni se cifra la flash: la clave está en una partición del mismo chip, así que **no** protege frente a quien tenga la placa, y un nodo instalado antes de esa partición conserva sus secretos como estaban hasta que se grabe con la nueva tabla de particiones.

## 🧩 Papel en A.R.M.O.R.

ARMOR-NODE-RADAR son los ojos del perímetro. Cada nodo lee hasta tres radares o sensores de presencia y publica pistas, luz y salud a [ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER) por MQTT; [ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO) las dibuja en el plano del sitio y [ARMOR-SERVER-AI](https://github.com/JuanenRac/ARMOR-SERVER-AI) las sopesa frente a las cámaras. Su firmware comparte una base con los demás nodos (el panel, la configuración por Bluetooth, las actualizaciones firmadas, el acceso y el reloj vienen de la base de firmware de [ARMOR-COMMON](https://github.com/JuanenRac/ARMOR-COMMON)), así que lo que aprendes en un nodo vale en todos.

| Nombre | Cómo | Para qué |
|---|---|---|
| ARMOR-SERVER | MQTT `armor/node/<id>/telemetry|health|info|command` | Recibe los mensajes del radar (hoy el banco CM5) y ordena al nodo |
| ARMOR-STUDIO | enlace al panel | Abre el panel del nodo desde la dirección que el nodo publica |
| ARMOR-ANDROID-CONTROL | Bluetooth | Configura el nodo en el campo |
| ARMOR-COMMON | contratos, base de firmware | Los mensajes se comprueban con sus esquemas; el firmware común se copia de él |

## 🏛️ Arquitectura

<p align="center">
  <img src="images/architecture.svg" alt="Arquitectura de ARMOR-NODE-RADAR: quién lo llama, qué contiene y de qué depende" width="100%">
</p>

## 🖼️ Capturas de pantalla

Capturadas a 1920×1080 en un banco de pruebas, con datos de ejemplo y sin ninguna instalación real detrás.

<table>
<tr><td width="50%" valign="top"><img src="images/screenshots/overview.png" alt="Resumen: el estado del nodo de un vistazo - red, broker, firmware y lo que reporta." width="100%"><br><sub>Resumen: el estado del nodo de un vistazo - red, broker, firmware y lo que reporta.</sub></td><td width="50%" valign="top"><img src="images/screenshots/radars.png" alt="Radares: cada radar del nodo, su modelo, sus pines y su calibración." width="100%"><br><sub>Radares: cada radar del nodo, su modelo, sus pines y su calibración.</sub></td></tr>
<tr><td width="50%" valign="top"><img src="images/screenshots/pins.png" alt="Pines: qué hace cada pin, comprobado para que dos funciones no compartan uno." width="100%"><br><sub>Pines: qué hace cada pin, comprobado para que dos funciones no compartan uno.</sub></td><td width="50%" valign="top"><img src="images/screenshots/network.png" alt="Red: el direccionamiento Ethernet o Wi-Fi del nodo, ajustado desde su propio panel web." width="100%"><br><sub>Red: el direccionamiento Ethernet o Wi-Fi del nodo, ajustado desde su propio panel web.</sub></td></tr>
</table>

## 🔒 Seguridad y protección

| Amenaza | Defensa |
|---|---|
| Contraseñas en claro o compiladas | PBKDF2-HMAC-SHA256 con 10 000 rondas; ninguna contraseña en el firmware (un nodo sin usuario entra en puesta en marcha); máximo 4 usuarios (admin o visor) y de 8 a 64 caracteres; máximo 6 sesiones, 30 minutos de inactividad |
| Tomar un nodo recién grabado | La puesta en marcha exige un código: el de la compilación (`CONFIG_ARMOR_SETUP_CODE`) o uno aleatorio guardado en flash y mostrado en la consola USB; con secreto de flota es `HMAC-SHA256(secreto, MAC)`; también es la clave del Wi-Fi de configuración (WPA2, nunca puenteado) |
| El panel alcanzado desde internet | Responde por defecto a las redes privadas de una casa (10/8, 172.16/12, 192.168/16, 169.254/16, 127/8 y 100.64/10 donde viven algunas VPN); el ajuste `web.admin_sources` lista otras direcciones y redes IPv4 (8 entradas, 120 caracteres); lo de fuera se rechaza salvo la identidad pública `GET /api/v1/session`; se rechaza una lista que dejaría fuera a quien la guarda |
| El panel en claro por HTTP | HTTPS en el 443 con un certificado que el nodo hace él mismo (autofirmado, en flash, huella SHA-256 visible para comparar); el panel puede ser http, https o ambos; la cookie de sesión es siempre HttpOnly y SameSite=Strict (y Secure por TLS) |
| Un firmware falso desde GitHub | Una imagen solo se instala con un SHA-256 que coincide y una firma Ed25519 válida hecha con la clave del mantenedor (`no_checksum`, `no_signature`, `bad_signature`, `checksum_mismatch`, `wrong_firmware`, `not_newer`, `version_mismatch` la rechazan); una subida manual de un administrador no se comprueba: es un límite explícito |
| Secuestro de la puesta en marcha por Bluetooth | Un PIN de 6 dígitos derivado del código de puesta en marcha, visible en la consola USB y para el administrador; el móvil debe introducirlo antes de escribir ajustes; la ventana dura `CONFIG_ARMOR_BLE_SETUP_MINUTES` (10 por defecto, de 1 a 120) |
| Una orden remota que hace daño | Dos pasos con un token de un solo uso ligado a esa operación; solo el servidor puede escribir esos topics en el broker; la pausa dura una hora como máximo; no hay restablecimiento de fábrica, borrado ni sueño profundo por orden remota |
| Secretos guardados leídos por quien tiene la placa | Opcional (apagado por defecto, interruptor `security.seal_secrets` o `nodes.nvs_encryption` del servidor): ChaCha20-Poly1305 para contraseñas, tokens, claves, PIN y hashes. No cifra la flash ni quema un eFuse: la clave está en una partición del mismo chip, así que no protege frente a quien tenga la placa |
| El enlace con el broker en claro | TLS opcional (`mqtts://`) con la autoridad propia del dueño (PEM de hasta 2 KB) y comprobación del nombre; apagado por defecto. El handshake solo se ha ejecutado en pruebas de host, no en una placa |
| El Bluetooth propio de un sensor | Por defecto el nodo cierra el Bluetooth de cada sensor LD (no pide PIN); un puente serie transparente a un ordenador solo se abre unos minutos y por un administrador |
| Adoptar un nodo en una red no fiable | `tools/adopt_node.py https://<dirección> --fingerprint <SHA-256>` comprueba el certificado en la misma conexión; avisa del HTTP plano y de ficheros de flota con dirección fija |
| Un pin peligroso | Los pines reservados de la placa nunca se ofrecen; una salida tiene estado seguro si se pierde el broker; dos funciones no pueden compartir un pin (comprobado) |
| Un firmware público con secreto de flota | `CONFIG_ARMOR_FLEET_SECRET` está vacío por defecto: la compilación pública no lleva secreto |

Lee [SECURITY.md](SECURITY.md) para informar de una vulnerabilidad. No ha habido revisión independiente.

## 🔌 Interfaces

| Parte | Qué define |
|---|---|
| MQTT | `armor/node/<id>/{telemetry,health,info,command}`, `armor/health/<id>` (su última voluntad es `{"node_id":"..","online":false}`), `armor/control/<id>/{command,result,maintenance}` y `armor/control/all/command`, `armor/device/<id>/<pin>/{state,set}`, `armor/radar/<id>/events`, `armor/radar/<id>/control/armed` |
| Panel (HTTP 80, HTTPS 443) | Rutas `session`, `login`, `logout`, `setup`, `status`, `config` (+ export, import), `wifi/scan`, `time`, `radars` (+ command), `perimeter` (+ armed, learn, zone), `pins` (+ command), `bridge` (+ open, close, io), `ota` (+ check, install, progress, copy, switch), `account`, `ble-pin`, `log`, `coredump`, `factory-reset`, `reboot`; `/metrics` para administradores y el WebSocket `/ws/radar-map`; `tests/check_routes.py` comprueba que toda ruta que llama el panel existe |
| Textos del panel | 76 códigos de error con texto en ocho idiomas (832 filas en `panel/text.js`), comprobados por `tests/check_panel_text.py` |
| Hardware | 3 UART para LD2450, LD2461, LD2410, LD2412, LD2410S y MR24HPC1 (RX 16, 17, 18; TX 15, 21, 38); W5500 por SPI (MOSI 11, MISO 12, SCLK 13, CS 14, INT 10, RST 9, 20 MHz); VEML7700 en I2C (GPIO 1 y 2); botón BOOT |

## ⚙️ Configuración

| Variable | Por defecto | Qué define |
|---|---|---|
| `CONFIG_ARMOR_NODE_ID` | `""` | El nombre; vacío da `armor-` y los tres últimos bytes de la MAC |
| `CONFIG_ARMOR_SETUP_CODE`, `CONFIG_ARMOR_FLEET_SECRET` | `""` | El código de puesta en marcha de la compilación (vacío: uno aleatorio) y el secreto de flota (16+ caracteres; nunca en el repositorio) |
| `CONFIG_ARMOR_RADAR_COUNT` | 3 (0-3) | Cuántos radares hay conectados |
| `CONFIG_ARMOR_TRACK_STABILISER` | y | Mantener la identidad de una pista de un cuadro a otro |
| `CONFIG_ARMOR_MQTT_URI`, `_USERNAME`, `_PASSWORD` | `""` | Los primeros ajustes del broker (se cambian en el panel) |
| `CONFIG_ARMOR_HEARTBEAT_S` | 10 (2-300) | Segundos entre latidos de salud |
| `CONFIG_ARMOR_TELEMETRY_PERIOD_MS` | 200 (200-5000) | Como máximo 5 mensajes por segundo |
| `CONFIG_ARMOR_BLE_SETUP_MINUTES` | 10 (1-120) | Cuánto dura abierta la puesta en marcha por Bluetooth |
| `CONFIG_ARMOR_BOARD_S3_ETH` / `_S3_WIFI` | eth | La placa: Waveshare ESP32-S3-ETH o ESP32-S3-WROOM-1 N16R8 (solo Wi-Fi) |
| `CONFIG_ARMOR_BOOT_FACTORY_RESET` | y | Mantén BOOT 8 s en los primeros 30 s para empezar de cero |
| `system.login_limit_per_minute` | 30 (0-120) | Contraseñas erróneas por minuto antes de que el panel bloquee el acceso a todos (0 = sin límite global); se lee y se cambia desde Studio (un botón en cada nodo) o con `POST /api/v1/admin/nodes/settings`, donde el acceso al panel se usa una vez y nunca se guarda |

Los ajustes que se afinan en vivo están en el panel y se guardan en flash (por ejemplo `system.reader_idle_ms`, `security.seal_secrets`, el límite de fallos de acceso por minuto (30, de 0 a 120) y las zonas del motor de perímetro); los valores por defecto del firmware están en `sdkconfig.defaults` y `sdkconfig.board.*`. La flash de 16 MB contiene `nvs` (64 KB), `otadata`, `phy_init`, dos ranuras de 3 MB `ota_0` y `ota_1`, `coredump` (128 KB) y `nvs_keys` (4 KB) (`partitions.csv`).

## 🚀 Inicio rápido

**Requisitos:** un compilador C++17 para las pruebas de host; el contenedor ESP-IDF 5.5.5 para compilar la imagen; una Waveshare ESP32-S3-ETH o una ESP32-S3-WROOM-1 N16R8.

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

Los tests en el ordenador necesitan cualquier compilador C++17 (Linux, WSL, MSYS2). Véanse la [puesta en marcha](docs/BENCH_BRINGUP.md), el [panel](docs/NODE_PANEL.md) y el [límite del hardware](docs/HARDWARE_BOUNDARY.md).

## 🧪 Pruebas

```bash
cmake -S tests -B build/host && cmake --build build/host && ctest --test-dir build/host
build/host/emit_samples | python tests/check_contract.py
python -I tools/armor_ci_validate.py
```

14 programas de prueba de host y 6 comprobaciones en Python se ejecutan con `ctest`; 7371 comprobaciones contadas por los programas de host, compilados con `-Werror`.

| Parte | Qué cubre |
|---|---|
| `test_core`, `test_sensors`, `test_family`, `test_uart_wake` | Los decodificadores (LD2450, LD2461, cuatro sensores de presencia), el delimitador de tramas, el canal de órdenes, las familias de sensores y el despertar de la UART |
| `test_node`, `test_base`, `test_shared`, `test_board_wifi` | Los ajustes y sus comprobaciones, la tabla de pines, usuarios y sesiones, el plan de red, la base compartida y la placa Wi-Fi |
| `test_logic`, `test_perimeter`, `test_control`, `test_diag` | La lógica de pines, el motor de perímetro, las órdenes de control en dos pasos y el diagnóstico |
| `test_mqtt_tls`, `test_release` | El enlace TLS con el broker (solo en host) y las comprobaciones de versión y firma |
| `check_panel_text`, `check_task_safety`, `check_save_paths`, `check_ble_session`, `check_routes`, `check_bridge_client` | Python: los textos del panel en ocho idiomas (832 filas), las tareas, las rutas de guardado, la sesión Bluetooth bajo su bloqueo, las rutas y el cliente del puente serie |

El panel web también se ejercita en un navegador real contra un nodo de pruebas (`tools/panel_mock.mjs`). Los decodificadores LD2461 y de presencia solo coinciden con los ejemplos de sus manuales.

## 🏗️ Compilación y publicación

`tools/build_node.sh generic` construye UNA imagen para cada placa Waveshare en el contenedor ESP-IDF 5.5.5 y `tools/flash.bat` la graba por USB-C; el mismo firmware se compila para la placa solo Wi-Fi con `s3-wifi`. Las versiones llevan la imagen, su SHA-256 y su firma Ed25519, una por placa.

## 🩺 Resolución de problemas

| Ves | Porque | Haz |
|---|---|---|
| Perdiste la contraseña o el nodo ya no se une a la red | Un nodo con puesta en marcha y sin dirección tras 90 s abre su propio Wi-Fi `ARMOR-SETUP-xxxxxx` (abre `http://192.168.4.1/`) | Para empezar de cero mantén BOOT 8 segundos en los primeros 30 segundos tras encender (borra ajustes y usuarios), o usa *Restablecer de fábrica* en el panel |
| Se rechaza una actualización desde GitHub | La versión no trae suma de comprobación ni firma, no coinciden (`e_no_checksum`, `e_no_signature`, `e_checksum_mismatch`, `e_bad_signature`), no trae imagen para esta placa (`e_no_asset_for_board`) o es de otro proyecto (`e_wrong_firmware`) | El nodo no cambió; reintenta o deja que un administrador suba el fichero a mano |
| Las órdenes al radar responden `no_answer`, `refused` o `disabled` | El hilo TX del radar no está conectado, el módulo se colgó o el nodo cierra antes su Bluetooth | Revisa el cableado (mira la puesta en marcha en banco), reinicia el módulo desde el panel |
| El nodo no muestra el sensor 3 o deja dormir un radar | El módulo dejó de hablar (silencio sin tramas malas) | Usa *reiniciar módulo* en la página de radares y luego cierra el Bluetooth del propio sensor |

## 📂 Estructura del repositorio

<p align="center">
  <img src="images/repository-structure.svg" alt="Estructura del repositorio de ARMOR-NODE-RADAR" width="100%">
</p>

<details>
<summary>Versión en texto del árbol</summary>

```text
ARMOR-NODE-RADAR/
├── main/                                el firmware: un componente de ESP-IDF
├── panel/                               el panel web embebido en el firmware
├── tests/                               pruebas de host y comprobaciones Python
├── docs/                                7 guías del banco y del panel
├── tools/                               herramientas de compilación, grabación, adopción, flota y comprobación
├── .github/                             flujo de CI y plantillas de issues y pull requests
├── images/                              banner, icono y las dos imágenes de este README
├── build.sh build.bat                   compilar con el flujo de proyecto compartido
├── test.sh test.bat                     ejecutar las pruebas con el flujo compartido
├── run.sh run.bat                       arrancar con el flujo de proyecto compartido
├── armor.project.json                   manifiesto: versión, rol, madurez, comando de build
├── CHANGELOG.md                         qué cambió en cada versión
├── SECURITY.md SUPPORT.md LICENSE       política de seguridad, soporte y licencia
├── CONTRIBUTING.md CODE_OF_CONDUCT.md   cómo contribuir y el código de conducta
├── CMakeLists.txt                       el proyecto ESP-IDF
├── partitions.csv                       la tabla de la flash
├── sdkconfig.defaults                   los ajustes del firmware
├── sdkconfig.board.s3-eth               la placa Ethernet
├── sdkconfig.board.s3-wifi              la placa Wi-Fi
├── secrets/                             ejemplos de ficheros de flota y nodo
├── build-test.sh build-test.bat         ejecutar las pruebas con el flujo compartido
└── .gitattributes .gitignore            ajustes de Git
```

</details>

## 🗺️ Hoja de ruta y límites conocidos

**Límites conocidos hoy**

* Los decodificadores LD2461 y de presencia solo coinciden con los ejemplos de sus manuales, sin módulo detrás; el canal de órdenes del radar no se ha comprobado con otro módulo que el LD2450.
* La comprobación de la firma de las actualizaciones y el emparejado Bluetooth con PIN están probados en un ordenador con firmas reales pero no aún en la placa; el handshake TLS con el broker no se ha ejecutado en una placa.
* El informe de salud y el control desde el servidor no han corrido en la placa; el motor de perímetro nunca ha corrido con un sensor; el certificado HTTPS propio del panel no se ha probado en todos los navegadores; la placa Wi-Fi no tiene placa física (solo existe la de Ethernet).

**Lo siguiente**

* Ejecutar el motor de perímetro, el informe de salud y las órdenes de control con sensores reales y el banco CM5; probar el enlace TLS con el broker y la actualización firmada en la placa.

## 🌍 Idiomas

El README y los textos existen en inglés, español, francés, italiano, alemán, chino, japonés y portugués de Portugal.

## 🔗 Proyectos relacionados

**A.R.M.O.R.** (Autonomous Radar & Multimodal Observation Range) es un sistema de seguridad perimetral hecho de repositorios independientes. Cada uno tiene su propia versión, sus propias pruebas y su propio README; esta es la familia:

* **[ARMOR-COMMON](https://github.com/JuanenRac/ARMOR-COMMON)** - Contratos de mensajes, validadores, vectores de conformidad y tipos generados
* **ARMOR-NODE-RADAR** (este repositorio) - Firmware de nodo de campo para ESP32-S3 con tres radares y su propio panel web
* **[ARMOR-NODE-SOLAR](https://github.com/JuanenRac/ARMOR-NODE-SOLAR)** - Protocolos de inversores y baterías solares y los mensajes de un nodo pasarela
* **[ARMOR-NODE-ELECTRICAL](https://github.com/JuanenRac/ARMOR-NODE-ELECTRICAL)** - Nodo eléctrico: medidores, el mensaje de las lecturas de la red y las reglas de maniobra
* **[ARMOR-NODE-ALARM](https://github.com/JuanenRac/ARMOR-NODE-ALARM)** - Nodo y panel de alarma: zonas, armado, retardos, sirena y PIN, con el servidor o sin él
* **[ARMOR-NODE-HMI](https://github.com/JuanenRac/ARMOR-NODE-HMI)** - Panel táctil: el estado del sistema en una pantalla de pared, armar y reconocer, y el hogar del asistente de voz
* **[ARMOR-NETWORK](https://github.com/JuanenRac/ARMOR-NETWORK)** - La red local: sus dispositivos, internet y lo que cambia
* **[ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER)** - Coordinador central: telemetría, alarmas, dispositivos, lecturas solares y cámaras
* **[ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO)** - Consola web: cámaras, radar, alarmas, energía solar y el diseñador 2D/3D del emplazamiento
* **[ARMOR-ANDROID-CONTROL](https://github.com/JuanenRac/ARMOR-ANDROID-CONTROL)** - Cliente de operador Android con radar 2D/3D en vivo
* **[ARMOR-IOS-CONTROL](https://github.com/JuanenRac/ARMOR-IOS-CONTROL)** - Cliente de operador para iPhone
* **[ARMOR-SERVER-AI](https://github.com/JuanenRac/ARMOR-SERVER-AI)** - Política de inferencia visual que explica sus decisiones y nunca actúa
* **[ARMOR-VISION](https://github.com/JuanenRac/ARMOR-VISION)** - Zonas de cámara, máscaras y reglas de detección para la Jetson Orin NX
* **[ARMOR-VOICE-AI](https://github.com/JuanenRac/ARMOR-VOICE-AI)** - Intenciones de voz sin conexión con una confirmación que no se puede falsificar
* **[ARMOR-HARDWARE](https://github.com/JuanenRac/ARMOR-HARDWARE)** - Cajas, electrónica y la matriz de aceptación en banco
* **[ARMOR-DEVOPS](https://github.com/JuanenRac/ARMOR-DEVOPS)** - Despliegue, el banco de pruebas de la CM5, copias de seguridad y TLS
* **[ARMOR-SIMULATOR](https://github.com/JuanenRac/ARMOR-SIMULATOR)** - Simulador de telemetría sin conexión con fallos repetibles
* **[ARMOR-UPDATER](https://github.com/JuanenRac/ARMOR-UPDATER)** - Detecta, instala y actualiza los repositorios del propio ecosistema
* **[ARMOR-DOCS](https://github.com/JuanenRac/ARMOR-DOCS)** - Arquitectura, base de seguridad y la matriz de capacidades

## 📚 Documentación

* [Panel del nodo](docs/NODE_PANEL.md): primer arranque, usuarios, red, las páginas del panel
* [Código de puesta en marcha](docs/SETUP_CODE.md) y [alta por Bluetooth](docs/BLE_PROVISIONING.md)
* [Sensores](docs/SENSORS.md): los seis modelos y sus decodificadores
* [Puesta en marcha en banco](docs/BENCH_BRINGUP.md), [pruebas de banco](docs/BENCH_TESTS.md) y [límite del hardware](docs/HARDWARE_BOUNDARY.md)
* [Matriz de capacidades: qué está demostrado y qué no](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/CAPABILITY_MATRIX.md)
* [Catálogo de proyectos: versiones y cómo dependen unos repositorios de otros](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/PROJECT_CATALOG.md)
* [Registro de cambios de este repositorio](CHANGELOG.md)

## 🤝 Contribuir y comunidad

* Antes de un pull request ejecuta `ctest --test-dir build/host` and `python -I tools/armor_ci_validate.py`: debe pasar. Consulta [CONTRIBUTING.md](CONTRIBUTING.md) y el [código de conducta](CODE_OF_CONDUCT.md).
* El README existe en ocho idiomas y se genera desde las fuentes de ARMOR-DOCS: un cambio de texto se hace allí.
* Un cambio en un mensaje entre componentes empieza en ARMOR-COMMON.
* No describas como funcionando en una placa lo que solo se ha compilado.
* Informa de una vulnerabilidad en privado: [SECURITY.md](SECURITY.md). Preguntas, ideas e informes: electrohobby3d@gmail.com ([SUPPORT.md](SUPPORT.md)).

## 👤 AUTOR

**JuanenRac (Electro Hobby 3D)** · electrohobby3d@gmail.com

## 📜 LICENCIA

GPL-3.0-or-later - véase [LICENSE](LICENSE).
