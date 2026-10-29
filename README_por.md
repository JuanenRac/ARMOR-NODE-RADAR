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
  <img src="https://img.shields.io/badge/License-GPL%203.0--or--later-blue.svg" alt="GPL 3.0 or later">
  <img src="https://img.shields.io/badge/Language-C%2B%2B17-00599c.svg" alt="Language">
  <img src="https://img.shields.io/badge/Target-ESP32--S3-e7352c.svg" alt="Target">
  <img src="https://img.shields.io/badge/Tests-7371%20checks-2ea44f.svg" alt="Tests">
  <img src="https://img.shields.io/badge/Version-0.8.0.5-0f6cbd.svg" alt="Version">
  <img src="https://img.shields.io/badge/Maturity-established-2EA44F.svg" alt="Maturity">
</p>

---

**Verificação de honestidade - o que funciona hoje:** O nó radar é o único nó que existe como hardware: o seu firmware corre numa placa real Waveshare ESP32-S3-ETH com um radar e reporta à bancada de testes do CM5, e o seu painel web e as atualizações pelo ar são usados nela. O núcleo independente do hardware (4,633 verificações em 14 programas de teste: os descodificadores do LD2450, do LD2461 e de quatro sensores de presença, o canal de comandos do LD2450, as definições e as suas verificações, a tabela de pinos, utilizadores e sessões, o plano de rede, o serializador de mensagens cuja saída o ARMOR-COMMON aceita, e as verificações de versão e assinatura) é testado num computador, o **painel web** foi exercitado num navegador real contra um nó de substituição e a **imagem do firmware compila** no contentor ESP-IDF 5.5.5. O que **ainda não** está provado: os descodificadores do LD2461 e dos sensores de presença só coincidem com os exemplos resolvidos dos seus manuais, sem um módulo por trás; a verificação da assinatura das atualizações e o emparelhamento Bluetooth com PIN estão provados no computador com assinaturas reais mas ainda não na placa; o canal de comandos do radar não foi confrontado com um módulo; e o certificado HTTPS do painel (feito pelo próprio nó) não foi experimentado em todos os navegadores.

---

## 📋 Num relance

| | |
|---|---|
| Versão | `0.8.0.5` |
| Maturidade | Estabelecido |
| Função | Nó de campo radar para ESP32-S3 (família: Field Nodes, pai: ARMOR-COMMON) |
| Tecnologia | C++17, ESP-IDF 5.5.5, ESP32-S3 |
| Testes | 7371 verificações em 14 programas de anfitrião e 6 em Python (`ctest`) |
| Placas | Waveshare ESP32-S3-ETH (Ethernet) e ESP32-S3-WROOM-1 N16R8 (só Wi-Fi) |
| Sensores | Até 3 radares (LD2450, LD2461) ou sensores de presença por nó, sensor de luz VEML7700 |
| Funciona em | Waveshare ESP32-S3-ETH ou ESP32-S3-WROOM-1 N16R8; testes de anfitrião em qualquer computador |
| Em hardware real | Corre numa placa real Waveshare ESP32-S3-ETH com um radar, reportando à bancada de testes do CM5 |
| Licença | GPL-3.0-or-later |

**Ir para:** [Visão geral](#-visão-geral) · [Arquitetura](#-arquitetura) · [Segurança](#-segurança-e-proteção) · [Interfaces](#-interfaces) · [Configuração](#-configuração) · [Início rápido](#-início-rápido) · [Testes](#-testes) · [Problemas](#-resolução-de-problemas) · [Estrutura](#-estrutura-do-repositório) · [Relacionados](#-projetos-relacionados)

## 🚦 Estado do projeto

Cada linha é uma parte deste repositório e a luz diz até onde chega hoje. É escrita com o que o repositório consegue demonstrar (testes, compilações, execuções), não com o que está previsto.

🟢 Feito e verificado · 🟡 Funciona, com limites conhecidos · 🟠 Feito a meio ou ainda por verificar · 🔴 Por fazer ou a não funcionar

| Área | Estado | O que significa aqui |
|---|---|---|
| Software | 🟢 | O firmware compila no contentor ESP-IDF 5.5.5 e funciona numa placa ESP32-S3-ETH real. |
| Testes | 🟢 | Passam 7.371 verificações em 14 programas de anfitrião (`ctest`): os descodificadores, as definições, a tabela de pinos, os utilizadores, o plano de rede, o serializador de mensagens e as verificações de versão. |
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
* **O painel sobre HTTPS:** o nó faz o seu próprio certificado (autoassinado, guardado na flash) e serve o painel na porta 443 além da 80, ou só na 443; o cookie de sessão fica marcado como Secure e a impressão digital do certificado mostra-se para comparar com o aviso do navegador.
* **Identidades de rasto estáveis:** os alvos de cada trama associam-se aos seguidos antes, de modo que uma pessoa mantém o mesmo id de rasto, uma trama perdida não cintila e as posições suavizam-se um pouco.
* **Seis modelos de sensor, um por porta:** uma porta leva um LD2450 ou um LD2461 (seguidores que alimentam o perímetro, com o campo próprio do modelo no Studio) ou um sensor de presença (LD2410, LD2412, LD2410S, MR24HPC1) que se torna um dispositivo do servidor e publica presença e distância. Escolhido no painel; o que cada um é, o seu protocolo e o que não foi verificado estão em `docs/SENSORS.md`.
* **Descodificador, saúde e configuração do LD2450:** um enquadrador que ressincroniza encontra tramas num fluxo ruidoso; cada trama de 30 bytes dá até três alvos que se tornam rastos do contrato; a consola e o painel dizem por radar se reporta, está em silêncio ou ilegível. O painel também pode ler a versão do módulo, escolher um ou três alvos e definir zonas de deteção (um protocolo não verificado contra um módulo).
* **Luz ambiente e mensagens exatas ao contrato:** o VEML7700 com gama automática; JSON de telemetria, saúde e informação que seguem os esquemas publicados e recusam escrever o que for inválido, com instantes de relógio de parede (SNTP) e uma última vontade MQTT. A telemetria retém-se enquanto nenhum radar reporta, nunca um «tudo em ordem» vazio.
* **Uma imagem para todas as placas, como um produto de rede:** o firmware é o mesmo e o MAC distingue as placas (`armor-` e seis dígitos até lhes darem nome); `adopt_node.py` dá a um nó acabado de gravar o seu administrador, a sua identidade no broker e as definições partilhadas da frota pela rede, com um código de configuração calculado a partir do seu MAC, por isso 5 nós ou 27 dão o mesmo trabalho.
* **Ferramentas de bancada:** gravação por USB-C, um nó substituto para trabalhar no painel, e um conversor de um registo de tramas em bruto para um ficheiro de teste ([arranque de bancada](docs/BENCH_BRINGUP.md)).
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

## 🧩 Papel no A.R.M.O.R.

O ARMOR-NODE-RADAR são os olhos do perímetro. Cada nó lê até três radares ou sensores de presença e publica pistas, luz e saúde para o [ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER) por MQTT; o [ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO) desenha-os na planta do local e o [ARMOR-SERVER-AI](https://github.com/JuanenRac/ARMOR-SERVER-AI) pesa-os face às câmaras. O seu firmware partilha uma base com os outros nós (o painel, a configuração por Bluetooth, as atualizações assinadas, o acesso e o relógio vêm da base de firmware do [ARMOR-COMMON](https://github.com/JuanenRac/ARMOR-COMMON)), por isso o que se aprende num nó vale em todos.

| Nome | Como | Para quê |
|---|---|---|
| ARMOR-SERVER | MQTT `armor/node/<id>/telemetry|health|info|command` | Recebe as mensagens do radar (hoje a bancada CM5) e comanda o nó |
| ARMOR-STUDIO | ligação ao painel | Abre o painel do nó a partir do endereço que o nó publica |
| ARMOR-ANDROID-CONTROL | Bluetooth | Configura o nó no terreno |
| ARMOR-COMMON | contratos, base de firmware | As mensagens são verificadas com os seus esquemas; o firmware comum é copiado dele |

## 🏛️ Arquitetura

<p align="center">
  <img src="images/architecture.svg" alt="Arquitetura de ARMOR-NODE-RADAR: quem o chama, o que contém e de que depende" width="100%">
</p>

## 🖼️ Capturas de ecrã

Capturadas a 1920×1080 numa bancada de testes, com dados de exemplo e sem qualquer instalação real por trás.

<table>
<tr><td width="50%" valign="top"><img src="images/screenshots/overview.png" alt="Resumo: o estado do nó num relance - rede, broker, firmware e o que reporta." width="100%"><br><sub>Resumo: o estado do nó num relance - rede, broker, firmware e o que reporta.</sub></td><td width="50%" valign="top"><img src="images/screenshots/radars.png" alt="Radares: cada radar do nó, o seu modelo, os seus pinos e a sua calibração." width="100%"><br><sub>Radares: cada radar do nó, o seu modelo, os seus pinos e a sua calibração.</sub></td></tr>
<tr><td width="50%" valign="top"><img src="images/screenshots/pins.png" alt="Pinos: o que cada pino faz, verificado para que duas funções nunca partilhem um." width="100%"><br><sub>Pinos: o que cada pino faz, verificado para que duas funções nunca partilhem um.</sub></td><td width="50%" valign="top"><img src="images/screenshots/network.png" alt="Rede: o endereçamento Ethernet ou Wi-Fi do nó, definido no seu próprio painel web." width="100%"><br><sub>Rede: o endereçamento Ethernet ou Wi-Fi do nó, definido no seu próprio painel web.</sub></td></tr>
</table>

## 🔒 Segurança e proteção

| Ameaça | Defesa |
|---|---|
| Palavras-passe em claro ou compiladas | PBKDF2-HMAC-SHA256 com 10 000 rondas; nenhuma palavra-passe no firmware (um nó sem utilizador entra em configuração); no máximo 4 utilizadores (admin ou visualizador) e de 8 a 64 caracteres; no máximo 6 sessões, 30 minutos de inatividade |
| Tomar um nó acabado de gravar | A configuração exige um código de arranque: o da compilação (`CONFIG_ARMOR_SETUP_CODE`) ou um aleatório guardado na flash e mostrado na consola USB; com segredo de frota é `HMAC-SHA256(segredo, MAC)`; é também a chave do Wi-Fi de configuração (WPA2, nunca em ponte) |
| O painel alcançado a partir da internet | Responde por omissão às redes privadas de uma casa (10/8, 172.16/12, 192.168/16, 169.254/16, 127/8 e 100.64/10 onde vivem algumas VPN); a definição `web.admin_sources` lista outros endereços e redes IPv4 (8 entradas, 120 caracteres); tudo o que está fora é recusado exceto a identidade pública `GET /api/v1/session`; uma lista que deixaria de fora quem a guarda é rejeitada |
| O painel em claro por HTTP | HTTPS no 443 com um certificado que o nó cria sozinho (autoassinado, na flash, impressão digital SHA-256 visível para comparar); o painel pode ser http, https ou ambos; o cookie de sessão é sempre HttpOnly e SameSite=Strict (e Secure por TLS) |
| Um firmware falso do GitHub | Uma imagem só se instala com um SHA-256 coincidente e uma assinatura Ed25519 válida feita com a chave do mantenedor (`no_checksum`, `no_signature`, `bad_signature`, `checksum_mismatch`, `wrong_firmware`, `not_newer`, `version_mismatch` recusam-na); um envio manual por um administrador não é verificado: um limite explícito |
| Sequestro da configuração Bluetooth | Um PIN de 6 dígitos derivado do código de configuração, visível na consola USB e ao administrador; o telemóvel tem de o introduzir antes de escrever definições; a janela dura `CONFIG_ARMOR_BLE_SETUP_MINUTES` (10 por omissão, de 1 a 120) |
| Uma ordem remota que faz mal | Dois passos com um token de uso único ligado a essa operação; só o servidor pode escrever esses tópicos no broker; uma pausa dura uma hora no máximo; não há reposição de fábrica, apagamento nem sono profundo por ordem remota |
| Segredos guardados lidos por quem tem a placa | Opcional (desligado por omissão, interruptor `security.seal_secrets` ou `nodes.nvs_encryption` do servidor): ChaCha20-Poly1305 para palavras-passe, tokens, chaves, PIN e hashes. Não cifra a flash nem queima um eFuse: a chave está numa partição do mesmo chip, por isso não protege contra quem tem a placa |
| A ligação ao broker em claro | TLS opcional (`mqtts://`) com a autoridade própria do dono (PEM até 2 KB) e verificação do nome; desligado por omissão. O handshake só correu em testes de anfitrião, não numa placa |
| O Bluetooth próprio de um sensor | Por omissão o nó fecha o Bluetooth de cada sensor LD (sem PIN); uma ponte série transparente para um computador só se abre por alguns minutos e por um administrador |
| Adotar um nó numa rede não fiável | `tools/adopt_node.py https://<endereço> --fingerprint <SHA-256>` verifica o certificado na mesma ligação; avisa sobre o HTTP simples e sobre ficheiros de frota com endereço fixo |
| Um pino perigoso | Os pinos reservados da placa nunca são oferecidos; uma saída tem estado seguro se o broker se perder; duas funções não podem partilhar um pino (verificado) |
| Um firmware público com segredo de frota | `CONFIG_ARMOR_FLEET_SECRET` está vazio por omissão: a compilação pública não leva segredo |

Leia [SECURITY.md](SECURITY.md) para comunicar uma vulnerabilidade. Não houve revisão independente.

## 🔌 Interfaces

| Parte | O que define |
|---|---|
| MQTT | `armor/node/<id>/{telemetry,health,info,command}`, `armor/health/<id>` (o seu testamento é `{"node_id":"..","online":false}`), `armor/control/<id>/{command,result,maintenance}` e `armor/control/all/command`, `armor/device/<id>/<pin>/{state,set}`, `armor/radar/<id>/events`, `armor/radar/<id>/control/armed` |
| Painel (HTTP 80, HTTPS 443) | Rotas `session`, `login`, `logout`, `setup`, `status`, `config` (+ export, import), `wifi/scan`, `time`, `radars` (+ command), `perimeter` (+ armed, learn, zone), `pins` (+ command), `bridge` (+ open, close, io), `ota` (+ check, install, progress, copy, switch), `account`, `ble-pin`, `log`, `coredump`, `factory-reset`, `reboot`; `/metrics` para administradores e o WebSocket `/ws/radar-map`; `tests/check_routes.py` verifica que toda a rota que o painel chama existe |
| Textos do painel | 76 códigos de erro com texto em oito línguas (832 linhas em `panel/text.js`), verificados por `tests/check_panel_text.py` |
| Hardware | 3 UART para LD2450, LD2461, LD2410, LD2412, LD2410S e MR24HPC1 (RX 16, 17, 18; TX 15, 21, 38); W5500 por SPI (MOSI 11, MISO 12, SCLK 13, CS 14, INT 10, RST 9, 20 MHz); VEML7700 em I2C (GPIO 1 e 2); botão BOOT |

## ⚙️ Configuração

| Variável | Predefinição | O que define |
|---|---|---|
| `CONFIG_ARMOR_NODE_ID` | `""` | O nome; vazio dá `armor-` e os três últimos bytes do MAC |
| `CONFIG_ARMOR_SETUP_CODE`, `CONFIG_ARMOR_FLEET_SECRET` | `""` | O código de configuração da compilação (vazio: aleatório) e o segredo de frota (16+ caracteres; nunca no repositório) |
| `CONFIG_ARMOR_RADAR_COUNT` | 3 (0-3) | Quantos radares estão ligados |
| `CONFIG_ARMOR_TRACK_STABILISER` | y | Manter a identidade de uma pista de imagem para imagem |
| `CONFIG_ARMOR_MQTT_URI`, `_USERNAME`, `_PASSWORD` | `""` | As primeiras definições do broker (alteráveis no painel) |
| `CONFIG_ARMOR_HEARTBEAT_S` | 10 (2-300) | Segundos entre batimentos de saúde |
| `CONFIG_ARMOR_TELEMETRY_PERIOD_MS` | 200 (200-5000) | No máximo 5 mensagens por segundo |
| `CONFIG_ARMOR_BLE_SETUP_MINUTES` | 10 (1-120) | Quanto tempo a configuração Bluetooth fica aberta |
| `CONFIG_ARMOR_BOARD_S3_ETH` / `_S3_WIFI` | eth | A placa: Waveshare ESP32-S3-ETH ou ESP32-S3-WROOM-1 N16R8 (só Wi-Fi) |
| `CONFIG_ARMOR_BOOT_FACTORY_RESET` | y | Mantenha BOOT 8 s nos primeiros 30 s para recomeçar |

As definições afinadas em direto estão no painel e guardam-se na flash (por exemplo `system.reader_idle_ms`, `security.seal_secrets`, o limite de falhas de início de sessão por minuto (30, de 0 a 120) e as zonas do motor de perímetro); os valores por omissão do firmware estão em `sdkconfig.defaults` e `sdkconfig.board.*`. A flash de 16 MB contém `nvs` (64 KB), `otadata`, `phy_init`, duas ranhuras de 3 MB `ota_0` e `ota_1`, `coredump` (128 KB) e `nvs_keys` (4 KB) (`partitions.csv`).

## 🚀 Início rápido

**Requisitos:** um compilador C++17 para os testes de anfitrião; o contentor ESP-IDF 5.5.5 para compilar a imagem; uma Waveshare ESP32-S3-ETH ou uma ESP32-S3-WROOM-1 N16R8.

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

Os testes de anfitrião precisam de qualquer compilador C++17 (Linux, WSL, MSYS2). Veja o [arranque de bancada](docs/BENCH_BRINGUP.md), o [painel](docs/NODE_PANEL.md) e a [fronteira de hardware](docs/HARDWARE_BOUNDARY.md).

## 🧪 Testes

```bash
cmake -S tests -B build/host && cmake --build build/host && ctest --test-dir build/host
build/host/emit_samples | python tests/check_contract.py
python -I tools/armor_ci_validate.py
```

14 programas de teste de anfitrião e 6 verificações Python correm com `ctest`; 7371 verificações contadas pelos programas de anfitrião, compiladas com `-Werror`.

| Parte | O que cobre |
|---|---|
| `test_core`, `test_sensors`, `test_family`, `test_uart_wake` | Os descodificadores (LD2450, LD2461, quatro sensores de presença), o delimitador de tramas, o canal de comandos, as famílias de sensores e o despertar da UART |
| `test_node`, `test_base`, `test_shared`, `test_board_wifi` | As definições e as suas verificações, a tabela de pinos, utilizadores e sessões, o plano de rede, a base partilhada e a placa Wi-Fi |
| `test_logic`, `test_perimeter`, `test_control`, `test_diag` | A lógica dos pinos, o motor de perímetro, as ordens de controlo em dois passos e o diagnóstico |
| `test_mqtt_tls`, `test_release` | A ligação TLS ao broker (só anfitrião) e as verificações de versão e assinatura |
| `check_panel_text`, `check_task_safety`, `check_save_paths`, `check_ble_session`, `check_routes`, `check_bridge_client` | Python: os textos do painel em oito línguas (832 linhas), as tarefas, os caminhos de gravação, a sessão Bluetooth sob o seu bloqueio, as rotas e o cliente da ponte série |

O painel web também é exercitado num navegador real contra um nó de substituição (`tools/panel_mock.mjs`). Os descodificadores LD2461 e de presença só coincidem com os exemplos dos seus manuais.

## 🏗️ Compilação e publicação

`tools/build_node.sh generic` compila UMA imagem para cada placa Waveshare no contentor ESP-IDF 5.5.5, e `tools/flash.bat` grava-a por USB-C; o mesmo firmware compila-se para a placa só Wi-Fi com `s3-wifi`. As versões levam a imagem, o seu SHA-256 e a sua assinatura Ed25519, uma por placa.

## 🩺 Resolução de problemas

| Vê | Porque | Faça |
|---|---|---|
| Perdeu a palavra-passe ou o nó já não se liga à rede | Um nó em configuração e sem endereço após 90 s abre o seu próprio Wi-Fi `ARMOR-SETUP-xxxxxx` (abra `http://192.168.4.1/`) | Para recomeçar mantenha BOOT 8 segundos nos primeiros 30 segundos após ligar (apaga definições e utilizadores), ou use *Reposição de fábrica* no painel |
| Uma atualização do GitHub é recusada | A versão não traz soma de verificação nem assinatura, não coincidem (`e_no_checksum`, `e_no_signature`, `e_checksum_mismatch`, `e_bad_signature`), não traz imagem para esta placa (`e_no_asset_for_board`) ou é de outro projeto (`e_wrong_firmware`) | O nó não mudou; tente de novo ou deixe um administrador enviar o ficheiro à mão |
| Os comandos ao radar respondem `no_answer`, `refused` ou `disabled` | O fio TX do radar não está ligado, o módulo bloqueou ou o nó fecha primeiro o seu Bluetooth | Verifique a cablagem (veja o arranque em bancada), reinicie o módulo no painel |
| O nó não mostra o sensor 3 ou deixa um radar a dormir | O módulo deixou de falar (silêncio sem tramas más) | Use *reiniciar módulo* na página de radares e depois feche o Bluetooth do próprio sensor |

## 📂 Estrutura do repositório

<p align="center">
  <img src="images/repository-structure.svg" alt="Estrutura do repositório de ARMOR-NODE-RADAR" width="100%">
</p>

<details>
<summary>Versão em texto da árvore</summary>

```text
ARMOR-NODE-RADAR/
├── main/                                o firmware: um componente ESP-IDF
├── panel/                               o painel web embebido no firmware
├── tests/                               testes de anfitrião e verificações Python
├── docs/                                7 guias da bancada e do painel
├── tools/                               ferramentas de compilação, gravação, adoção, frota e verificação
├── .github/                             fluxo de CI e modelos de issues e pull requests
├── images/                              faixa, ícone e as duas imagens deste README
├── build.sh build.bat                   compilar pelo fluxo de projeto partilhado
├── test.sh test.bat                     executar os testes pelo fluxo partilhado
├── run.sh run.bat                       arrancar pelo fluxo de projeto partilhado
├── armor.project.json                   manifesto: versão, função, maturidade, comando de build
├── CHANGELOG.md                         o que mudou em cada versão
├── SECURITY.md SUPPORT.md LICENSE       política de segurança, apoio e licença
├── CONTRIBUTING.md CODE_OF_CONDUCT.md   como contribuir e o código de conduta
├── CMakeLists.txt                       o projeto ESP-IDF
├── partitions.csv                       a tabela da flash
├── sdkconfig.defaults                   as definições do firmware
├── sdkconfig.board.s3-eth               a placa Ethernet
├── sdkconfig.board.s3-wifi              a placa Wi-Fi
├── secrets/                             exemplos de ficheiros de frota e nó
├── build-test.sh build-test.bat         executar os testes pelo fluxo partilhado
└── .gitattributes .gitignore            definições do Git
```

</details>

## 🗺️ Roteiro e limites conhecidos

**Limites conhecidos hoje**

* Os descodificadores LD2461 e de presença só coincidem com os exemplos dos seus manuais, sem módulo atrás; o canal de comandos só foi verificado com o LD2450.
* A verificação da assinatura das atualizações e o emparelhamento Bluetooth com PIN estão provados num computador com assinaturas reais mas ainda não na placa; o handshake TLS com o broker não correu numa placa.
* O relatório de saúde e o controlo a partir do servidor não correram na placa; o motor de perímetro nunca correu com um sensor; o certificado HTTPS próprio do painel não foi experimentado em todos os navegadores; a placa Wi-Fi não existe fisicamente (só existe a Ethernet).

**A seguir**

* Correr o motor de perímetro, o relatório de saúde e as ordens de controlo com sensores reais e a bancada CM5; experimentar a ligação TLS ao broker e a atualização assinada na placa.

## 🌍 Idiomas

O README e os textos existem em inglês, espanhol, francês, italiano, alemão, chinês, japonês e português europeu.

## 🔗 Projetos relacionados

O **A.R.M.O.R.** (Autonomous Radar & Multimodal Observation Range) é um sistema de segurança perimetral feito de repositórios independentes. Cada um tem a sua própria versão, os seus próprios testes e o seu próprio README; esta é a família:

* **[ARMOR-COMMON](https://github.com/JuanenRac/ARMOR-COMMON)** - Contratos de mensagens, validadores, vetores de conformidade e tipos gerados
* **ARMOR-NODE-RADAR** (este repositório) - Firmware de nó de campo para ESP32-S3 com três radares e o seu próprio painel web
* **[ARMOR-NODE-SOLAR](https://github.com/JuanenRac/ARMOR-NODE-SOLAR)** - Protocolos de inversores e baterias solares e as mensagens de um nó porta de ligação
* **[ARMOR-NODE-ELECTRICAL](https://github.com/JuanenRac/ARMOR-NODE-ELECTRICAL)** - Nó elétrico: contadores, a mensagem das leituras da rede e as regras de comutação
* **[ARMOR-NODE-ALARM](https://github.com/JuanenRac/ARMOR-NODE-ALARM)** - Nó e painel de alarme: zonas, armar, atrasos, sirene e PIN, com o servidor ou sem ele
* **[ARMOR-NODE-HMI](https://github.com/JuanenRac/ARMOR-NODE-HMI)** - Painel tátil: o estado do sistema num ecrã de parede, armar e reconhecer, e a casa do assistente de voz
* **[ARMOR-NETWORK](https://github.com/JuanenRac/ARMOR-NETWORK)** - A rede local: os seus dispositivos, a internet e o que muda
* **[ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER)** - Coordenador central: telemetria, alarmes, dispositivos, leituras solares e câmaras
* **[ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO)** - Consola web: câmaras, radar, alarmes, energia solar e o desenhador 2D/3D do local
* **[ARMOR-ANDROID-CONTROL](https://github.com/JuanenRac/ARMOR-ANDROID-CONTROL)** - Cliente de operador Android com radar 2D/3D em direto
* **[ARMOR-IOS-CONTROL](https://github.com/JuanenRac/ARMOR-IOS-CONTROL)** - Cliente de operador para iPhone
* **[ARMOR-SERVER-AI](https://github.com/JuanenRac/ARMOR-SERVER-AI)** - Política de inferência visual que explica as suas decisões e nunca atua
* **[ARMOR-VISION](https://github.com/JuanenRac/ARMOR-VISION)** - Zonas de câmara, máscaras e regras de deteção para o Jetson Orin NX
* **[ARMOR-VOICE-AI](https://github.com/JuanenRac/ARMOR-VOICE-AI)** - Intenções de voz offline com uma confirmação que não pode ser falsificada
* **[ARMOR-HARDWARE](https://github.com/JuanenRac/ARMOR-HARDWARE)** - Caixas, eletrónica e a matriz de aceitação em bancada
* **[ARMOR-DEVOPS](https://github.com/JuanenRac/ARMOR-DEVOPS)** - Implantação, a bancada de testes do CM5, cópias de segurança e TLS
* **[ARMOR-SIMULATOR](https://github.com/JuanenRac/ARMOR-SIMULATOR)** - Simulador de telemetria offline com falhas repetíveis
* **[ARMOR-UPDATER](https://github.com/JuanenRac/ARMOR-UPDATER)** - Deteta, instala e atualiza os repositórios do próprio ecossistema
* **[ARMOR-DOCS](https://github.com/JuanenRac/ARMOR-DOCS)** - Arquitetura, base de segurança e a matriz de capacidades

## 📚 Documentação

* [Painel do nó](docs/NODE_PANEL.md): primeiro arranque, utilizadores, rede, as páginas do painel
* [Código de configuração](docs/SETUP_CODE.md) e [provisionamento Bluetooth](docs/BLE_PROVISIONING.md)
* [Sensores](docs/SENSORS.md): os seis modelos e os seus descodificadores
* [Arranque em bancada](docs/BENCH_BRINGUP.md), [testes de bancada](docs/BENCH_TESTS.md) e [limite do hardware](docs/HARDWARE_BOUNDARY.md)
* [Matriz de capacidades: o que está demonstrado e o que não está](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/CAPABILITY_MATRIX.md)
* [Catálogo de projetos: versões e como os repositórios dependem uns dos outros](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/PROJECT_CATALOG.md)
* [Registo de alterações deste repositório](CHANGELOG.md)

## 🤝 Contribuir e comunidade

* Antes de um pull request, execute `ctest --test-dir build/host` and `python -I tools/armor_ci_validate.py`: tem de passar. Consulte [CONTRIBUTING.md](CONTRIBUTING.md) e o [código de conduta](CODE_OF_CONDUCT.md).
* O README existe em oito idiomas e é gerado a partir das fontes do ARMOR-DOCS: uma alteração ao texto faz-se lá.
* Uma alteração a uma mensagem entre componentes começa no ARMOR-COMMON.
* Não descreva como a funcionar numa placa aquilo que só foi compilado.
* Comunique uma vulnerabilidade em privado: [SECURITY.md](SECURITY.md). Perguntas, ideias e relatórios: electrohobby3d@gmail.com ([SUPPORT.md](SUPPORT.md)).

## 👤 AUTOR

**JuanenRac (Electro Hobby 3D)** · electrohobby3d@gmail.com

## 📜 LICENÇA

GPL-3.0-or-later - consulte [LICENSE](LICENSE).
