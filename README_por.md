<p align="center">
  <img src="images/ARMOR_BANNER.svg" alt="ARMOR-NODE-RADAR banner" width="100%">
</p>

# 📡 ARMOR-NODE-RADAR

<p align="center">
  <a href="README.md">🇺🇸 English</a> |
  <a href="README_spa.md">🇪🇸 Español</a> |
  <a href="README_fra.md">🇫🇷 Français</a> |
  <a href="README_ita.md">🇮🇹 Italiano</a> |
  <a href="README_deu.md">🇩🇪 Deutsch</a> |
  <a href="README_zho.md">🇨🇳 简体中文</a> |
  <a href="README_jpn.md">🇯🇵 日本語</a> |
  🇵🇹 <b>Português</b>
</p>

### Firmware de nó de campo (Waveshare ESP32-S3-ETH, três radares ou sensores de presença, Ethernet e Wi-Fi) com o seu próprio painel web, e o seu núcleo testado em anfitrião

<p align="center">
  <img src="https://img.shields.io/badge/License-GPL%203.0-blue.svg" alt="GPL 3.0">
  <img src="https://img.shields.io/badge/Language-C%2B%2B17-00599c.svg" alt="Language">
  <img src="https://img.shields.io/badge/Target-ESP32--S3-e7352c.svg" alt="Target">
  <img src="https://img.shields.io/badge/Version-0.8.0.1-0f6cbd.svg" alt="Version">
  <img src="https://img.shields.io/badge/Maturity-established-2EA44F.svg" alt="Maturity">
</p>

---

**Verificação de honestidade - o que funciona hoje:** O nó radar é o único nó que existe como hardware: o seu firmware corre numa placa real Waveshare ESP32-S3-ETH com um radar e reporta à bancada de testes do CM5, e o seu painel web e as atualizações pelo ar são usados nela. O núcleo independente do hardware (4,633 verificações em 13 programas de teste: os descodificadores do LD2450, do LD2461 e de quatro sensores de presença, o canal de comandos do LD2450, as definições e as suas verificações, a tabela de pinos, utilizadores e sessões, o plano de rede, o serializador de mensagens cuja saída o ARMOR-COMMON aceita, e as verificações de versão e assinatura) é testado num computador, o **painel web** foi exercitado num navegador real contra um nó de substituição e a **imagem do firmware compila** no contentor ESP-IDF 5.5.5. O que **ainda não** está provado: os descodificadores do LD2461 e dos sensores de presença só coincidem com os exemplos resolvidos dos seus manuais, sem um módulo por trás; a verificação da assinatura das atualizações e o emparelhamento Bluetooth com PIN estão provados no computador com assinaturas reais mas ainda não na placa; o canal de comandos do radar não foi confrontado com um módulo; e o certificado HTTPS do painel (feito pelo próprio nó) não foi experimentado em todos os navegadores.

---

## 📋 Num relance

| | |
|---|---|
| Versão | `0.8.0.1` |
| Maturidade | Estabelecido |
| Testes | 4 633 verificações em 13 programas (`ctest`) |
| Tecnologia | ESP-IDF C++ |
| Em hardware real | Corre numa placa real Waveshare ESP32-S3-ETH com um radar, reportando à bancada de testes do CM5 |

## 🚦 Estado do projeto

Cada linha é uma parte deste repositório e a luz diz até onde chega hoje. É escrita com o que o repositório consegue demonstrar (testes, compilações, execuções), não com o que está previsto.

🟢 Feito e verificado · 🟡 Funciona, com limites conhecidos · 🟠 Feito a meio ou ainda por verificar · 🔴 Por fazer ou a não funcionar

| Área | Estado | O que significa aqui |
|---|---|---|
| Software | 🟢 | O firmware compila no contentor ESP-IDF 5.5.5 e funciona numa placa ESP32-S3-ETH real. |
| Testes | 🟢 | Passam 4.633 verificações em 13 programas de anfitrião (`ctest`): os descodificadores, as definições, a tabela de pinos, os utilizadores, o plano de rede, o serializador de mensagens e as verificações de versão. |
| Protocolos e dados | 🟡 | O LD2450 funciona com um radar real; os descodificadores do LD2461 e dos quatro sensores de presença só coincidem com os exemplos dos seus manuais, e o canal de comandos do radar não foi verificado com um módulo. |
| Interface | 🟡 | O painel web está em uso na placa e foi exercitado num navegador real contra um nó simulado; o seu certificado HTTPS, criado pelo próprio nó, não foi experimentado em todos os navegadores. |
| Hardware | 🟡 | É o único nó que existe como hardware: uma placa Waveshare ESP32-S3-ETH real com um radar reporta à bancada de testes CM5; não há mais nenhum módulo de sensor ligado. |
| Segurança | 🟡 | O emparelhamento por PIN Bluetooth e a verificação da assinatura das atualizações estão provados num computador com assinaturas reais, mas ainda não na placa; não houve revisão independente. |
| Atualizações | 🟡 | As atualizações pelo ar estão em uso na placa; a verificação das atualizações assinadas ainda não foi executada lá. |
| Integração | 🟢 | As mensagens da placa chegam ao servidor da bancada de testes CM5, e essas mesmas mensagens são aceites pelos esquemas partilhados e pelos vetores de conformidade do ARMOR-COMMON. |
| Documentação | 🟢 | Oito idiomas e o registo de alterações estão atualizados. |

## 🎯 Visão geral

* **Duas placas, um só firmware:** a Waveshare ESP32-S3-ETH (Ethernet, a predefinida) e um ESP32-S3-WROOM-1 N16R8 sem Ethernet (só Wi-Fi: a sua configuração pede a rede Wi-Fi a que se juntar e mantém sempre a sua própria rede). A imagem escolhe-se ao compilar (`tools/build_node.sh generic s3-eth` ou `generic s3-wifi`); a tabela de pinos e a forma de entrada seguem a placa, o painel oculta o que a placa não tem, e uma imagem só serve para a sua própria placa. Ambas compilam e estão testadas em anfitrião; nenhuma correu numa placa.
* **Um nó cobre até 270 graus:** cada Waveshare ESP32-S3-ETH lê até três radares LD2450 nas suas três UART, montados a 75 graus de distância, por Ethernet com fio (W5500) com DHCP ou um endereço fixo, alimentado por PoE ou USB. O *Adicionar um nó de 270°* do Studio cria os três radares já ligados ao nó.
* **Um painel web em cada nó,** com o aspeto do Studio e os seus idiomas, embutido no firmware: resumo, rede, Wi-Fi, broker, radares, pinos, utilizadores, atualização de firmware e registo. Um nó sem utilizador abre o Wi-Fi `ARMOR-SETUP-xxxxxx` e cria o seu primeiro administrador com um código de configuração; as palavras-passe são PBKDF2 com sal, as sessões são tokens aleatórios, e cada definição vive na flash do nó, por isso uma imagem serve todos os nós e nenhuma palavra-passe está compilada ([o painel](docs/NODE_PANEL.md)).
* **Um só Wi-Fi a partir de vários nós:** cada nó pode oferecer um ponto de acesso ligado à sua porta Ethernet; dê aos nós o mesmo nome e palavra-passe e o canal em automático (1, 6 ou 11 pelo MAC) e os telemóveis e sensores Wi-Fi veem uma só rede com um só servidor DHCP. Não é uma malha de rádio: cada nó mantém o seu cabo. Um nó pode, em vez disso, juntar-se ao Wi-Fi de um router (com uma pesquisa de redes), e um sem cabo pode configurar-se por **Bluetooth** a partir da aplicação Android.
* **Pinos para o servidor:** qualquer pino livre torna-se uma entrada, uma saída (com um estado seguro quando se perde o broker), PWM ou uma leitura analógica, e aparece como um dispositivo do servidor, por isso um relé ou um contacto não precisa de firmware novo. Os pinos reservados da placa nunca se oferecem.
* **Atualizações pelo ar com retrocesso** a partir do painel (duas ranhuras na flash de 16 MB), e uma **ligação do Studio** ao painel de cada nó, a partir do endereço que o nó publica.
* **O painel sobre HTTPS:** o nó faz o seu próprio certificado (autoassinado, guardado na flash) e serve o painel na porta 443 além da 80, ou só na 443; o cookie de sessão fica marcado como Secure e a impressão digital do certificado mostra-se para comparar com o aviso do navegador. **Identidades de rasto estáveis:** os alvos de cada trama associam-se aos seguidos antes, de modo que uma pessoa mantém o mesmo id de rasto, uma trama perdida não cintila e as posições suavizam-se um pouco.
* **Seis modelos de sensor, um por porta:** uma porta leva um LD2450 ou um LD2461 (seguidores que alimentam o perímetro, com o campo próprio do modelo no Studio) ou um sensor de presença (LD2410, LD2412, LD2410S, MR24HPC1) que se torna um dispositivo do servidor e publica presença e distância. Escolhido no painel; o que cada um é, o seu protocolo e o que não foi verificado estão em `docs/SENSORS.md`.
* **Descodificador, saúde e configuração do LD2450:** um enquadrador que ressincroniza encontra tramas num fluxo ruidoso; cada trama de 30 bytes dá até três alvos que se tornam rastos do contrato; a consola e o painel dizem por radar se reporta, está em silêncio ou ilegível. O painel também pode ler a versão do módulo, escolher um ou três alvos e definir zonas de deteção (um protocolo não verificado contra um módulo).
* **Luz ambiente e mensagens exatas ao contrato:** o VEML7700 com gama automática; JSON de telemetria, saúde e informação que seguem os esquemas publicados e recusam escrever o que for inválido, com instantes de relógio de parede (SNTP) e uma última vontade MQTT. A telemetria retém-se enquanto nenhum radar reporta, nunca um «tudo em ordem» vazio.
* **Uma imagem para todas as placas, como um produto de rede:** o firmware é o mesmo e o MAC distingue as placas (`armor-` e seis dígitos até lhes darem nome); `adopt_node.py` dá a um nó acabado de gravar o seu administrador, a sua identidade no broker e as definições partilhadas da frota pela rede, com um código de configuração calculado a partir do seu MAC, por isso 5 nós ou 27 dão o mesmo trabalho. **Ferramentas de bancada:** gravação por USB-C, um nó substituto para trabalhar no painel, e um conversor de um registo de tramas em bruto para um ficheiro de teste ([arranque de bancada](docs/BENCH_BRINGUP.md)).
* **Atualizações pelo ar assinadas:** o nó só instala do GitHub uma imagem cujo SHA-256 coincide e que leva uma assinatura Ed25519 válida do responsável (a chave pública está no firmware, a privada nunca sai do responsável); um ficheiro carregado à mão por um administrador no painel continua a ser aceite. As páginas e mensagens do painel estão em oito idiomas.
* **Emparelhamento Bluetooth com PIN:** o PIN de seis dígitos deriva do código de configuração, é impresso na consola USB e mostrado a um administrador no painel; o telemóvel tem de o introduzir antes de poder escrever as definições do nó.
* **Só a casa pode usar o painel:** o painel responde por omissão à rede privada (ou às redes que o dono indicar, como uma VPN), recusa uma lista que deixaria de fora quem a guarda, conta as palavras-passe erradas por pessoa e por endereço e funciona atrás do proxy do servidor para ser alcançado de fora de casa sem publicar o nó.
* **Ligação cifrada opcional ao broker (TLS):** desativada por predefinição, por isso o nó fala MQTT sem cifra como antes. Um interruptor e uma caixa para o certificado da autoridade própria da instalação (PEM, no máximo 2 KB) na página Broker fazem-no ligar-se com `mqtts://`, confiar só nessa autoridade e verificar que o certificado do broker foi feito para o endereço alcançado; o que não for um único certificado PEM é recusado. Coberto por testes no computador; o handshake ainda não foi executado numa placa.
* **Relatório de saúde:** cerca de uma vez por minuto o nó publica há quanto tempo está ligado e porque reiniciou da última vez, a memória livre, a ligação e o sinal Wi-Fi, quantas vezes se ligou ao broker e o último erro que registou (`armor/health/<nó>`, o contrato `node_health` do ARMOR-COMMON). O código que o constrói é partilhado por todos os nós e testado num computador; nunca correu numa placa.
* **Controlo a partir do servidor:** o Studio pode reiniciar o nó de forma limpa, pô-lo em pausa até uma hora (retoma sozinho), pôr o seu trabalho em espera (sem fim, mantida nos reinícios, o nó continua ligado e a responder), fazê-lo mostrar-se, pedir um instantâneo de diagnóstico e marcá-lo em manutenção (`armor/control/<nó>/...`, o contrato `node_control` do ARMOR-COMMON). Cada ordem tem dois passos com um token de uso único que pertence a essa operação, só o servidor as pode escrever no broker, e não existe reposição de fábrica, apagamento nem sono profundo. Enquanto o nó está em pausa ou em espera, não publica alvos nem presença (o batimento continua a dizer que está lá). O código é partilhado por todos os nós e testado num computador; ainda não correu numa placa.
* **Um motor de perímetro no nó (desligado por predefinição):** seis zonas de deteção e três de exclusão, cada uma um retângulo que pode ser rodado por um ângulo, três linhas de passagem que contam as travessias por sentido, três percursos de entrada, imunidade a animais e deteção de ocultação. Funciona junto dos sensores, por isso continua a funcionar sem acesso ao broker, guarda os seus eventos enquanto o broker está ausente e publica-os em `armor/radar/<nó>/events`; o editor de zonas está no painel do nó. Nada disto foi ainda experimentado com um sensor numa placa.
* **Quem é cada sensor, e o seu Bluetooth próprio fechado:** ao arrancar, o nó lê a versão de firmware e o endereço Bluetooth de cada sensor, fecha por predefinição o Bluetooth próprio dos sensores (não pede PIN) e pode ceder a linha série de um sensor à ferramenta de PC de um administrador durante alguns minutos (uma ponte série transparente). `GET /metrics` para administradores e uma exportação e importação de tudo ou nada das definições completam o conjunto.
* **Diagnóstico sem cabo:** depois de um arranque que não foi limpo (um bloqueio, um watchdog, uma quebra de alimentação, uma reserva de memória falhada), o relatório de saúde diz o motivo durante dez minutos; uma falha deixa o seu core dump para ver, descarregar ou apagar a partir do painel; as definições guardadas são copiadas antes de cada instalação de firmware e recuperadas se uma atualização as danificar; e um administrador pode pedir ao nó, através do servidor, um pacote de diagnóstico ou o seu registo com um token de utilização única.
* **Adoção por HTTPS com impressão digital fixada:** `tools/adopt_node.py` aceita `https://<endereço>` com `--fingerprint` (o SHA-256 do certificado do nó, verificado na própria ligação que leva o pedido) ou `--show-fingerprint` para a ler; avisa do HTTP em claro e de um ficheiro de frota com IP fixo ou nome de anfitrião, e o painel também avisa ao importar um ficheiro assim.
* **O leitor de sensores espera por eventos em vez de sondar:** a tarefa de leitura dorme nas filas de eventos das linhas série e esvazia tudo o que chegou, esvazia a linha se houve excesso e conta excessos e erros por linha; `system.reader_idle_ms` (100; 0 = a sondagem de 10 ms de antes) fixa a espera, e se uma fila falhar volta sozinha à sondagem.
* **Selagem opcional dos segredos guardados:** desativada por predefinição (`security.seal_secrets`, ou `nodes.nvs_encryption` do servidor). Ativada, as palavras-passe, os tokens, as chaves, os PIN e os hashes de palavras-passe que o nó guarda ficam selados com ChaCha20-Poly1305 e tudo o resto continua legível; ativá-la ou desativá-la reescreve-os sem perder nada, mesmo que o nó pare a meio. Não se queima nenhum eFuse nem se cifra a flash: a chave está numa partição do mesmo chip, pelo que **não** protege contra quem tenha a placa, e um nó instalado antes dessa partição mantém os segredos como estavam até ser gravado com a nova tabela de partições.

## 🖼️ Capturas de ecrã

Capturadas a 1920×1080 numa bancada de testes, com dados de exemplo e sem qualquer instalação real por trás.

<table>
<tr><td width="50%" valign="top"><img src="images/screenshots/overview.png" alt="Resumo: o estado do nó num relance - rede, broker, firmware e o que reporta." width="100%"><br><sub>Resumo: o estado do nó num relance - rede, broker, firmware e o que reporta.</sub></td><td width="50%" valign="top"><img src="images/screenshots/radars.png" alt="Radares: cada radar do nó, o seu modelo, os seus pinos e a sua calibração." width="100%"><br><sub>Radares: cada radar do nó, o seu modelo, os seus pinos e a sua calibração.</sub></td></tr>
<tr><td width="50%" valign="top"><img src="images/screenshots/pins.png" alt="Pinos: o que cada pino faz, verificado para que duas funções nunca partilhem um." width="100%"><br><sub>Pinos: o que cada pino faz, verificado para que duas funções nunca partilhem um.</sub></td><td width="50%" valign="top"><img src="images/screenshots/network.png" alt="Rede: o endereçamento Ethernet ou Wi-Fi do nó, definido no seu próprio painel web." width="100%"><br><sub>Rede: o endereçamento Ethernet ou Wi-Fi do nó, definido no seu próprio painel web.</sub></td></tr>
</table>

## 🧩 Papel no A.R.M.O.R.

O ARMOR-NODE-RADAR são os olhos do perímetro. Cada nó lê até três radares ou sensores de presença e publica pistas, luz e saúde para o [ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER) por MQTT; o [ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO) desenha-os na planta do local e o [ARMOR-SERVER-AI](https://github.com/JuanenRac/ARMOR-SERVER-AI) pesa-os face às câmaras. O seu firmware partilha uma base com os outros nós (o painel, a configuração por Bluetooth, as atualizações assinadas, o acesso e o relógio vêm da base de firmware do [ARMOR-COMMON](https://github.com/JuanenRac/ARMOR-COMMON)), por isso o que se aprende num nó vale em todos.

## 📂 Estrutura do repositório

```text
ARMOR-NODE-RADAR/
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
├── tools/                  build_node.sh, check_sdkconfig.py, make_fleet.py, adopt_node.py, provision_node.sh, flash.bat, pack_panel.py, panel_mock.mjs, frames_to_fixture.py, serial_bridge_client.py, setup_code.html, the shared CI checks (armor_ci_validate.py, armor_project_tool.py, _armor_readme_parity.py)
├── secrets/                node.conf.example, fleet.example.json (the real files are git-ignored)
├── CMakeLists.txt, partitions.csv, sdkconfig.defaults, sdkconfig.board.*   the ESP-IDF project, the flash layout and the settings of each board (s3-eth, s3-wifi)
└── docs/                   BENCH_BRINGUP.md, NODE_PANEL.md, BLE_PROVISIONING.md, SENSORS.md, HARDWARE_BOUNDARY.md, BENCH_TESTS.md, SETUP_CODE.md
```

## 🛠️ Ambiente de desenvolvimento

**Requisitos:** um compilador de C++17 para os testes no computador, o contentor ESP-IDF 5.5.5 para compilar a imagem, um Waveshare ESP32-S3-ETH ou um ESP32-S3-WROOM-1 N16R8

```bash
cmake -S tests -B build/host && cmake --build build/host
build/host/test_core && build/host/test_node && build/host/test_sensors && build/host/test_board_wifi && build/host/test_release   # 4,633 checks, -Werror
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

Os testes de anfitrião precisam de qualquer compilador C++17 (Linux, WSL, MSYS2). Veja o [arranque de bancada](docs/BENCH_BRINGUP.md), o [painel](docs/NODE_PANEL.md) e a [fronteira de hardware](docs/HARDWARE_BOUNDARY.md).

## 🔗 Projetos relacionados

O **A.R.M.O.R.** (Autonomous Radar & Multimodal Observation Range) é um sistema de segurança perimetral feito de repositórios independentes. Cada um tem a sua própria versão, os seus próprios testes e o seu próprio README; esta é a família:

* **[ARMOR-COMMON](https://github.com/JuanenRac/ARMOR-COMMON)** - Contratos de mensagens, validadores, vetores de conformidade e tipos gerados
* **ARMOR-NODE-RADAR** (este repositório) - Firmware de nó de campo para ESP32-S3 com três radares e o seu próprio painel web
* **[ARMOR-NODE-SOLAR](https://github.com/JuanenRac/ARMOR-NODE-SOLAR)** - Protocolos de inversores e baterias solares e as mensagens de um nó de ligação
* **[ARMOR-NODE-ELECTRICAL](https://github.com/JuanenRac/ARMOR-NODE-ELECTRICAL)** - Nó elétrico: contadores, a mensagem das leituras da rede e as regras de comutação
* **[ARMOR-NODE-ALARM](https://github.com/JuanenRac/ARMOR-NODE-ALARM)** - Nó e central de alarme: zonas, armar, atrasos, sirene e PIN, com o servidor ou sem ele
* **[ARMOR-NODE-HMI](https://github.com/JuanenRac/ARMOR-NODE-HMI)** - Painel tátil: o estado do sistema num ecrã de parede, armar e reconhecer, e a casa do assistente de voz
* **[ARMOR-NETWORK](https://github.com/JuanenRac/ARMOR-NETWORK)** - A rede local: os seus dispositivos, a internet e o que muda
* **[ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER)** - Coordenador central: telemetria, alarmes, dispositivos, leituras solares e câmaras
* **[ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO)** - Consola web: câmaras, radar, alarmes, energia solar e o desenhador de local 2D/3D
* **[ARMOR-ANDROID-CONTROL](https://github.com/JuanenRac/ARMOR-ANDROID-CONTROL)** - Cliente Android de operador com um radar 2D/3D em direto
* **[ARMOR-IOS-CONTROL](https://github.com/JuanenRac/ARMOR-IOS-CONTROL)** - Cliente de operador para iPhone
* **[ARMOR-SERVER-AI](https://github.com/JuanenRac/ARMOR-SERVER-AI)** - Política de inferência visual que explica as suas decisões e nunca atua
* **[ARMOR-VISION](https://github.com/JuanenRac/ARMOR-VISION)** - Zonas de câmara, máscaras e regras de deteção para o Jetson Orin NX
* **[ARMOR-VOICE-AI](https://github.com/JuanenRac/ARMOR-VOICE-AI)** - Intenções de voz offline com uma confirmação que não se pode falsificar
* **[ARMOR-HARDWARE](https://github.com/JuanenRac/ARMOR-HARDWARE)** - Caixas, eletrónica e a matriz de aceitação de bancada
* **[ARMOR-DEVOPS](https://github.com/JuanenRac/ARMOR-DEVOPS)** - Implementação, a bancada de testes CM5, cópias de segurança e TLS
* **[ARMOR-SIMULATOR](https://github.com/JuanenRac/ARMOR-SIMULATOR)** - Simulador de telemetria offline com falhas repetíveis
* **[ARMOR-UPDATER](https://github.com/JuanenRac/ARMOR-UPDATER)** - Deteta, instala e atualiza os próprios repositórios do ecossistema
* **[ARMOR-DOCS](https://github.com/JuanenRac/ARMOR-DOCS)** - Arquitetura, linha de base de segurança e a matriz de capacidades

## 📚 Documentação e comunidade

Onde ler mais:

* [Matriz de capacidades: o que está provado e o que não](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/CAPABILITY_MATRIX.md)
* [Catálogo de projetos: versões e como os repositórios dependem uns dos outros](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/PROJECT_CATALOG.md)
* [Registo de alterações deste repositório](CHANGELOG.md)
* [Licença (GPL-3.0-or-later)](LICENSE)
* Perguntas, ideias e relatórios: electrohobby3d@gmail.com

## 👤 AUTOR

**JuanenRac (Electro Hobby 3D)** · electrohobby3d@gmail.com

## 📜 LICENÇA

GPL-3.0-or-later - veja [LICENSE](LICENSE).
