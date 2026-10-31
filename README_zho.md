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
  🇨🇳 <b>简体中文</b> |
  <a href="README_jpn.md">🇯🇵 日本語</a> |
  <a href="README_por.md">🇵🇹 Português</a>
</p>

### 现场节点固件（Waveshare ESP32-S3-ETH，三个雷达或存在传感器，以太网和 Wi-Fi），自带网页面板，并有在电脑上测试过的核心

<p align="center">
  <img src="https://img.shields.io/badge/License-GPL%203.0--or--later-blue.svg" alt="GPL 3.0 or later">
  <img src="https://img.shields.io/badge/Language-C%2B%2B17-00599c.svg" alt="Language">
  <img src="https://img.shields.io/badge/Target-ESP32--S3-e7352c.svg" alt="Target">
  <img src="https://img.shields.io/badge/Tests-7371%20checks-2ea44f.svg" alt="Tests">
  <img src="https://img.shields.io/badge/Version-0.8.0.7-0f6cbd.svg" alt="Version">
  <img src="https://img.shields.io/badge/Maturity-established-2EA44F.svg" alt="Maturity">
</p>

---

**诚实性检查 - 今天真正能运行的部分：** 雷达节点是目前唯一真正存在的硬件节点：它的固件运行在带雷达的真实 Waveshare ESP32-S3-ETH 板上，向 CM5 测试台报告，其网页面板和无线更新也在该板上使用。与硬件无关的核心（14 个测试程序中的 7,371 项检查：LD2450、LD2461 和四种存在传感器的解码器、LD2450 命令通道、设置及其检查、引脚表、用户和会话、网络方案、ARMOR-COMMON 接受其输出的消息序列化器，以及版本和签名检查）在计算机上测试，**网页面板**已在真实浏览器中对照替身节点验证，**固件镜像**可在 ESP-IDF 5.5.5 容器中构建。**尚未**证明的是：LD2461 和存在传感器的解码器只与其手册中的示例吻合，背后没有真实模块；更新的签名检查和带 PIN 的蓝牙配对已在计算机上用真实签名验证，但尚未在板上验证；雷达命令通道没有对照真实模块检查；面板的 HTTPS 证书（由节点自己生成）没有在所有浏览器中试过。

---

## 📋 速览

| | |
|---|---|
| 版本 | `0.8.0.7` |
| 成熟度 | 成熟 |
| 角色 | ESP32-S3 雷达现场节点（家族：Field Nodes，上级：ARMOR-COMMON） |
| 技术栈 | C++17, ESP-IDF 5.5.5, ESP32-S3 |
| 测试 | 7371 项检查（14 个主机程序和 6 项 Python 检查，`ctest`） |
| 电路板 | Waveshare ESP32-S3-ETH（以太网）和 ESP32-S3-WROOM-1 N16R8（仅 Wi-Fi） |
| 传感器 | 每个节点最多 3 台雷达（LD2450、LD2461）或存在传感器，以及 VEML7700 光传感器 |
| 运行位置 | Waveshare ESP32-S3-ETH 或 ESP32-S3-WROOM-1 N16R8；主机测试可在任何计算机上运行 |
| 真实硬件 | 运行在带雷达的真实 Waveshare ESP32-S3-ETH 板上，向 CM5 测试台报告 |
| 许可证 | GPL-3.0-or-later |

**跳转到：** [概述](#-概述) · [架构](#-架构) · [安全](#-安全与防护) · [接口](#-接口) · [配置](#-配置) · [快速开始](#-快速开始) · [测试](#-测试) · [故障排除](#-故障排除) · [结构](#-仓库结构) · [相关项目](#-相关项目)

## 🚦 项目状态

每一行是本仓库的一个部分，灯号表示它今天做到了哪一步。内容依据仓库能够展示的证据（测试、构建、运行），而不是计划。

🟢 已完成并验证 · 🟡 可用，有已知限制 · 🟠 部分完成或尚未验证 · 🔴 未完成或不可用

| 领域 | 状态 | 在这里的含义 |
|---|---|---|
| 软件 | 🟢 | 固件可在 ESP-IDF 5.5.5 容器中构建，并在真实的 ESP32-S3-ETH 板上运行。 |
| 测试 | 🟢 | 14 个主机程序中的 7,371 项检查通过（`ctest`）：解码器、设置、引脚表、用户、网络方案、消息序列化器和版本检查。 |
| 协议与数据 | 🟡 | LD2450 已在真实雷达上运行；LD2461 和四种人体存在传感器的解码器只与其手册中的示例一致，雷达命令通道尚未对照模块检查。 |
| 界面 | 🟡 | 网页面板已在板子上使用，并在真实浏览器中对模拟节点做过测试；由节点自己生成的 HTTPS 证书尚未在所有浏览器中试过。 |
| 硬件 | 🟡 | 这是唯一实际存在的硬件节点：一块真实的 Waveshare ESP32-S3-ETH 板带着雷达向 CM5 测试台上报；没有连接其他传感器模块。 |
| 安全 | 🟡 | 蓝牙 PIN 配对和更新签名检查已在电脑上用真实签名证明，但尚未在板子上验证；没有经过独立审查。 |
| 更新 | 🟡 | 空中更新已在板子上使用；签名更新的检查尚未在板子上运行过。 |
| 集成 | 🟡 | 板子的消息送达 CM5 测试台上的服务器，同样的消息也被 ARMOR-COMMON 的共享模式和一致性向量接受。仅在测试台上见过，尚未在实际安装中使用。 |
| 文档 | 🟢 | 八种语言和更新日志均为最新。 |

## 🎯 概述

* **两种开发板，一套固件：** Waveshare ESP32-S3-ETH（以太网，默认）和不带以太网的 ESP32-S3-WROOM-1 N16R8（仅 Wi-Fi：设置时会询问要加入的 Wi-Fi 网络，节点始终保留自己的网络）。镜像在构建时选择（`tools/build_node.sh generic s3-eth` 或 `generic s3-wifi`）；引脚表和接入方式随开发板而定，面板会隐藏开发板没有的功能，镜像只适用于对应的开发板。两者都能构建并已在电脑上测试，但都没在开发板上运行过。
* **一个节点最多覆盖 270 度：** 每块 Waveshare ESP32-S3-ETH 在其三个 UART 上读取最多三个 LD2450 雷达，彼此相隔 75 度安装，通过有线以太网（W5500）使用 DHCP 或固定地址，由 PoE 或 USB 供电。Studio 的 *Add a 270° node* 会创建已与节点关联的三个雷达。
* **每个节点自带网页面板，** 外观与 Studio 一致，支持八种语言，内嵌于固件：概览、网络、Wi-Fi、代理、雷达、引脚、用户、固件更新和日志。没有用户的节点会开启 Wi-Fi `ARMOR-SETUP-xxxxxx`，并用设置码创建第一个管理员；密码使用加盐 PBKDF2，会话是随机令牌，所有设置都保存在节点的闪存中，因此一个镜像适用于所有节点，且不会把任何密码编译进去（[面板](docs/NODE_PANEL.md)）。
* **多个节点组成一个 Wi-Fi：** 每个节点可提供接到其以太网口的接入点；给节点设置相同的名称和密码，信道设为自动（按 MAC 取 1、6 或 11），手机和 Wi-Fi 传感器就会看到一个网络和一个 DHCP 服务器。这不是无线 mesh：每个节点仍保留自己的网线。节点也可以改为加入路由器的 Wi-Fi（可搜索网络），没有网线的节点可通过 Android 应用用**蓝牙**配置。
* **为服务器提供引脚：** 任何空闲引脚都可成为输入、输出（代理丢失时有安全状态）、PWM 或模拟读数，并显示为服务器的一个设备，因此继电器或触点无需新固件。开发板的保留引脚永远不会被提供。
* **通过面板进行带回滚的空中更新（OTA）**（16 MB 闪存上有两个槽位），以及从 Studio 到每个节点面板的**链接**，地址来自节点发布的信息。
* **通过 HTTPS 访问面板：** 节点生成自己的证书（自签名，保存在闪存中），除 80 端口外也在 443 端口提供面板，或只使用 443；会话 Cookie 标为 Secure，并显示证书指纹以便与浏览器的警告比对。
* **稳定的轨迹身份：** 每帧的目标会与之前跟踪的目标匹配，因此一个人保持同一轨迹编号，丢帧不会闪烁，位置也会略微平滑。
* **六种传感器型号，每个端口一种：** 端口可接 LD2450 或 LD2461（为周界提供数据的跟踪器，Studio 中有该型号自己的探测范围），或存在传感器（LD2410、LD2412、LD2410S、MR24HPC1），后者成为服务器的设备并发布存在与距离。在面板中选择；每种传感器是什么、其协议以及哪些未经验证，见 `docs/SENSORS.md`。
* **LD2450 解码器、健康状态与配置：** 可重新同步的分帧器能在有噪声的数据流中找到帧；每个 30 字节的帧给出最多三个目标，成为契约轨迹；控制台和面板会按雷达说明它是在上报、静默还是数据混乱。面板还能读取模块版本、选择一个或三个目标并设置检测区域（这一协议尚未对照模块检查）。
* **环境光与严格符合契约的消息：** 带自动量程的 VEML7700；遵循已发布模式的遥测、健康和信息 JSON，拒绝写入任何无效内容，带墙上时钟时间戳（SNTP）和 MQTT 遗嘱。没有雷达上报时会扣留遥测，绝不发出空的“一切正常”。
* **一个镜像适用于所有开发板，像网络产品一样：** 固件相同，由 MAC 区分开发板（命名前为 `armor-` 加六位数字）；`adopt_node.py` 通过网络给刚刷好的节点分配管理员、代理身份和整个节点群共用的设置，设置码由其 MAC 计算得出，因此 5 个节点和 27 个节点工作量相同。
* **台架工具：** 通过 USB-C 刷写、用于开发面板的替身节点，以及把原始帧日志转换成测试夹具的转换器（[台架调试](docs/BENCH_BRINGUP.md)）。
* **签名的无线更新：**节点只会从 GitHub 安装 SHA-256 匹配且带有维护者有效 Ed25519 签名的镜像（公钥在固件中，私钥从不离开维护者）；管理员在面板中手动上传的文件仍然被接受。面板的页面和消息有八种语言。
* **带 PIN 的蓝牙配对：**六位 PIN 由设置码派生，打印在 USB 控制台上并向管理员显示在面板中；手机必须输入它才能写入节点的设置。
* **只有家中才能使用面板：**面板默认只响应私有网络（或所有者列出的网络，如 VPN），拒绝会把保存者自己挡在外面的列表，按人和按地址统计错误密码，并可在服务器代理之后工作，从而无需公开节点就能在家外访问。
* **可选的与代理加密连接(TLS):** 默认关闭,因此节点仍像以前一样使用未加密的 MQTT。代理页面中的一个开关和一个用于填写本安装自有证书颁发机构证书(PEM,最大 2 KB)的框,会让节点用 `mqtts://` 连接、只信任该机构,并检查代理证书是否为其所到达的地址而签发;不是单个 PEM 证书的内容会被拒绝。已有主机测试覆盖;握手尚未在电路板上运行。
* **健康报告：** 节点大约每分钟发布一次：已运行多久、上次为何重启、可用内存、连接与 Wi-Fi 信号、连接代理的次数以及记录的最后一个错误（`armor/health/<节点>`，即 ARMOR-COMMON 的 `node_health` 契约）。生成它的代码由所有节点共享并在计算机上测试；尚未在开发板上运行。
* **由服务器控制：** Studio 可以干净地重启节点、将其暂停至多一小时（自动恢复）、让其工作进入待机（无期限，重启后仍保持，节点保持连接并继续应答）、让它显示自己、请求诊断快照，并把它标记为维护中（`armor/control/<节点>/...`，ARMOR-COMMON 的 `node_control` 契约）。每条命令分两步，使用只属于该操作的一次性令牌，只有服务器可以在代理上写入，并且没有恢复出厂设置、擦除或深度睡眠。节点处于暂停或待机时，不发布目标和人员存在信息（心跳仍表明它在线）。 该代码由所有节点共用，并已在计算机上测试；尚未在电路板上运行过。
* **节点上的周界引擎（默认关闭）：** 六个检测区和三个排除区，每个都是可按角度旋转的矩形，三条按方向统计穿越次数的绊线，三条进入路径，宠物免疫和遮挡检测。它与传感器一起在节点上运行，因此在无法连接 broker 时仍然工作，broker 不在时保存事件，并发布到 `armor/radar/<节点>/events`；区域编辑器在节点的面板中。以上功能尚未在带传感器的实际电路板上运行过。
* **每个传感器是谁，以及关闭其自带蓝牙：** 启动时，节点读取每个传感器的固件版本和蓝牙地址，默认关闭传感器自带的蓝牙（它不要求 PIN），并可在几分钟内把一个传感器的串口线交给管理员的电脑工具（透明串口桥）。面向管理员的 `GET /metrics`，以及设置的导出和全有或全无的导入，使之更加完整。
* **无需线缆的诊断：** 在一次不干净的启动之后（崩溃、看门狗、电源跌落、内存分配失败），健康报告会在十分钟内说明原因；崩溃会留下核心转储（core dump），可在面板中查看、下载或删除；已保存的设置会在每次固件安装前被复制，并在更新损坏它们时恢复；管理员还可以通过服务器，用一次性令牌向节点索取诊断包或其日志。
* **通过 HTTPS 并固定证书指纹进行接入:** `tools/adopt_node.py` 接受 `https://<地址>`,配合 `--fingerprint`(节点证书的 SHA-256,在承载请求的同一连接上校验)或 `--show-fingerprint` 读取指纹;对明文 HTTP 以及带固定地址或主机名的设备清单文件会给出警告,面板在导入这类文件时也会警告。
* **传感器读取任务改为等待事件而不是轮询:** 读取任务在串口线路的事件队列上休眠,并一次取走所有到达的数据,在溢出时清空该线路,并按线路统计溢出和错误;`system.reader_idle_ms`(100;0 = 之前的 10 ms 轮询)设定等待时间,某个队列出错时会自动退回轮询。
* **可选的已存储机密封存:** 默认关闭(`security.seal_secrets`,或服务器的 `nodes.nvs_encryption`)。开启后,节点保存的密码、令牌、密钥、PIN 和密码哈希会用 ChaCha20-Poly1305 封存,其余内容仍可读取;开启或关闭时会无损重写,即使节点中途停止也不会丢失。不会烧写 eFuse,闪存也不加密:密钥在同一芯片的一个分区中,因此**不能**防范持有电路板的人;在该分区出现之前安装的节点会保持原样,直到用新的分区表重新烧录。

## 🧩 在 A.R.M.O.R. 中的角色

ARMOR-NODE-RADAR 是周界的眼睛。每个节点读取最多三个雷达或存在传感器，并通过 MQTT 向 [ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER) 发布轨迹、光照和健康状态；[ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO) 把它们画在场地平面图上，[ARMOR-SERVER-AI](https://github.com/JuanenRac/ARMOR-SERVER-AI) 把它们与摄像头对照权衡。它的固件与其他节点共用一个基础（面板、蓝牙设置、签名更新、登录和时钟都来自 [ARMOR-COMMON](https://github.com/JuanenRac/ARMOR-COMMON) 的固件基础），所以在一个节点上学到的对所有节点都适用。

| 名称 | 方式 | 用途 |
|---|---|---|
| ARMOR-SERVER | MQTT `armor/node/<id>/telemetry|health|info|command` | 接收雷达消息（目前是 CM5 测试台）并向节点下达命令 |
| ARMOR-STUDIO | 面板链接 | 通过节点发布的地址打开节点面板 |
| ARMOR-ANDROID-CONTROL | Bluetooth | 在现场设置节点 |
| ARMOR-COMMON | 合约、固件基础 | 消息按其模式校验；公共固件由此复制 |

## 🏛️ 架构

<p align="center">
  <img src="images/architecture.svg" alt="ARMOR-NODE-RADAR 的架构：谁调用它、它包含什么、它依赖什么" width="100%">
</p>

## 🖼️ 屏幕截图

在测试台上以 1920×1080 截取，使用示例数据，背后没有真实的安装。

<table>
<tr><td width="50%" valign="top"><img src="images/screenshots/overview.png" alt="概览：节点状态一目了然——网络、代理、固件及其上报内容。" width="100%"><br><sub>概览：节点状态一目了然——网络、代理、固件及其上报内容。</sub></td><td width="50%" valign="top"><img src="images/screenshots/radars.png" alt="雷达：节点上的每个雷达，包括型号、引脚和校准。" width="100%"><br><sub>雷达：节点上的每个雷达，包括型号、引脚和校准。</sub></td></tr>
<tr><td width="50%" valign="top"><img src="images/screenshots/pins.png" alt="引脚：每个引脚的用途，并检查两个功能不会共用同一引脚。" width="100%"><br><sub>引脚：每个引脚的用途，并检查两个功能不会共用同一引脚。</sub></td><td width="50%" valign="top"><img src="images/screenshots/network.png" alt="网络：节点的以太网或 Wi-Fi 地址设置，在其自带的网页面板中配置。" width="100%"><br><sub>网络：节点的以太网或 Wi-Fi 地址设置，在其自带的网页面板中配置。</sub></td></tr>
</table>

## 🔒 安全与防护

| 威胁 | 防御 |
|---|---|
| 明文或编译进固件的密码 | PBKDF2-HMAC-SHA256，10 000 轮；固件中没有密码（没有用户的节点会进入设置）；最多 4 个用户（管理员或查看者），8 到 64 个字符；最多 6 个会话，空闲 30 分钟 |
| 接管刚刷写的节点 | 设置需要启动码：构建时的码（`CONFIG_ARMOR_SETUP_CODE`），或保存在闪存中并显示在 USB 控制台上的随机码；有机群密钥时为 `HMAC-SHA256(密钥, MAC)`；它也是设置用 Wi-Fi 的密钥（WPA2，绝不桥接） |
| 从互联网访问面板 | 默认只响应家庭的私有网络（10/8、172.16/12、192.168/16、169.254/16、127/8，以及某些 VPN 使用的 100.64/10）；设置 `web.admin_sources` 可列出其他 IPv4 地址和网络（8 项，120 个字符）；除公开身份 `GET /api/v1/session` 外，其余一律拒绝；会把保存者自己锁在外面的列表会被拒绝 |
| 通过 HTTP 明文访问面板 | 在 443 端口提供 HTTPS，证书由节点自己生成（自签名、存于闪存，显示 SHA-256 指纹以便比对）；面板可设为 http、https 或两者；会话 Cookie 始终为 HttpOnly 和 SameSite=Strict（TLS 下还有 Secure） |
| 来自 GitHub 的伪造固件 | 只有 SHA-256 匹配且带有维护者密钥有效 Ed25519 签名的镜像才会被安装（`no_checksum`、`no_signature`、`bad_signature`、`checksum_mismatch`、`wrong_firmware`、`not_newer`、`version_mismatch` 会拒绝）；管理员手动上传不做校验：这是明确的限制 |
| 劫持蓝牙设置 | 由设置码派生的 6 位 PIN，显示在 USB 控制台并对管理员可见；手机必须先输入才能写入设置；设置窗口时长为 `CONFIG_ARMOR_BLE_SETUP_MINUTES`（默认 10，范围 1 到 120） |
| 造成危害的远程命令 | 两步操作，使用与该操作绑定的一次性令牌；只有服务器能向代理写入这些主题；暂停最长一小时；远程命令没有恢复出厂、擦除和深度睡眠 |
| 拿到电路板的人读取存储的机密 | 可选（默认关闭，开关为 `security.seal_secrets` 或服务器的 `nodes.nvs_encryption`）：用 ChaCha20-Poly1305 保护密码、令牌、密钥、PIN 和哈希。它不加密闪存也不烧写 eFuse：密钥位于同一芯片的分区中，因此无法防御拿到电路板的人 |
| 与代理的明文连接 | 可选 TLS（`mqtts://`），使用所有者自己的证书颁发机构（PEM 最大 2 KB）并检查名称；默认关闭。握手只在主机测试中运行过，没有在电路板上运行 |
| 传感器自带的蓝牙 | 默认情况下节点会关闭每个 LD 传感器的蓝牙（无需 PIN）；通向计算机的透明串口桥仅由管理员开启几分钟 |
| 在不可信网络上接入节点 | `tools/adopt_node.py https://<地址> --fingerprint <SHA-256>` 在同一连接中校验证书；对明文 HTTP 和使用固定地址的机群文件给出警告 |
| 危险的引脚 | 电路板的保留引脚从不提供；代理丢失时输出进入安全状态；两个功能不能共用一个引脚（已检查） |
| 带有机群密钥的公开固件 | `CONFIG_ARMOR_FLEET_SECRET` 默认为空：公开构建不含任何密钥 |

报告漏洞请阅读 [SECURITY.md](SECURITY.md)。没有经过独立审查。

## 🔌 接口

| 部分 | 作用 |
|---|---|
| MQTT | `armor/node/<id>/{telemetry,health,info,command}`、`armor/health/<id>`（遗嘱为 `{"node_id":"..","online":false}`）、`armor/control/<id>/{command,result,maintenance}` 和 `armor/control/all/command`、`armor/device/<id>/<pin>/{state,set}`、`armor/radar/<id>/events`、`armor/radar/<id>/control/armed` |
| 面板（HTTP 80、HTTPS 443） | 路由 `session`、`login`、`logout`、`setup`、`status`、`config`（+ export、import）、`wifi/scan`、`time`、`radars`（+ command）、`perimeter`（+ armed、learn、zone）、`pins`（+ command）、`bridge`（+ open、close、io）、`ota`（+ check、install、progress、copy、switch）、`account`、`ble-pin`、`log`、`coredump`、`factory-reset`、`reboot`；面向管理员的 `/metrics` 和 WebSocket `/ws/radar-map`；`tests/check_routes.py` 检查面板调用的每条路由都存在 |
| 面板文字 | 76 个带八种语言文字的错误码（`panel/text.js` 中 832 行），由 `tests/check_panel_text.py` 检查 |
| 硬件 | 用于 LD2450、LD2461、LD2410、LD2412、LD2410S 和 MR24HPC1 的 3 个 UART（RX 16、17、18；TX 15、21、38）；SPI 接口的 W5500（MOSI 11、MISO 12、SCLK 13、CS 14、INT 10、RST 9、20 MHz）；I2C 上的 VEML7700（GPIO 1 和 2）；BOOT 按钮 |

## ⚙️ 配置

| 变量 | 默认值 | 作用 |
|---|---|---|
| `CONFIG_ARMOR_NODE_ID` | `""` | 名称；为空则为 `armor-` 加 MAC 的最后三个字节 |
| `CONFIG_ARMOR_SETUP_CODE`, `CONFIG_ARMOR_FLEET_SECRET` | `""` | 构建的设置码（为空则随机）和机群密钥（16 个字符以上，绝不放入仓库） |
| `CONFIG_ARMOR_RADAR_COUNT` | 3 (0-3) | 已连接的雷达数量 |
| `CONFIG_ARMOR_TRACK_STABILISER` | y | 在帧与帧之间保持轨迹身份 |
| `CONFIG_ARMOR_MQTT_URI`, `_USERNAME`, `_PASSWORD` | `""` | 最初的代理设置（可在面板中修改） |
| `CONFIG_ARMOR_HEARTBEAT_S` | 10 (2-300) | 健康心跳间隔（秒） |
| `CONFIG_ARMOR_TELEMETRY_PERIOD_MS` | 200 (200-5000) | 每秒最多 5 条消息 |
| `CONFIG_ARMOR_BLE_SETUP_MINUTES` | 10 (1-120) | 蓝牙设置保持开启的时间 |
| `CONFIG_ARMOR_BOARD_S3_ETH` / `_S3_WIFI` | eth | 电路板：Waveshare ESP32-S3-ETH 或 ESP32-S3-WROOM-1 N16R8（仅 Wi-Fi） |
| `CONFIG_ARMOR_BOOT_FACTORY_RESET` | y | 在前 30 秒内按住 BOOT 8 秒可重新开始 |

现场调节的设置在面板中并保存在闪存里（例如 `system.reader_idle_ms`、`security.seal_secrets`、每分钟登录失败上限（30，范围 0 到 120）以及周界引擎的区域）；固件默认值在 `sdkconfig.defaults` 和 `sdkconfig.board.*` 中。16 MB 闪存包含 `nvs`（64 KB）、`otadata`、`phy_init`、两个 3 MB 槽位 `ota_0` 和 `ota_1`、`coredump`（128 KB）和 `nvs_keys`（4 KB）（`partitions.csv`）。

## 🚀 快速开始

**要求:** 用于主机测试的 C++17 编译器；用于构建镜像的 ESP-IDF 5.5.5 容器；Waveshare ESP32-S3-ETH 或 ESP32-S3-WROOM-1 N16R8。

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

主机测试需要任意 C++17 编译器（Linux、WSL、MSYS2）。参见[台架调试](docs/BENCH_BRINGUP.md)、[面板](docs/NODE_PANEL.md)和[硬件边界](docs/HARDWARE_BOUNDARY.md)。

## 🧪 测试

```bash
cmake -S tests -B build/host && cmake --build build/host && ctest --test-dir build/host
build/host/emit_samples | python tests/check_contract.py
python -I tools/armor_ci_validate.py
```

`ctest` 运行 14 个主机测试程序和 6 个 Python 检查；主机程序统计的检查共 7371 项，使用 `-Werror` 构建。

| 部分 | 覆盖内容 |
|---|---|
| `test_core`, `test_sensors`, `test_family`, `test_uart_wake` | 解码器（LD2450、LD2461、四种存在传感器）、帧切分、命令通道、传感器系列和 UART 唤醒 |
| `test_node`, `test_base`, `test_shared`, `test_board_wifi` | 设置及其检查、引脚表、用户与会话、网络方案、共享基础和 Wi-Fi 电路板 |
| `test_logic`, `test_perimeter`, `test_control`, `test_diag` | 引脚逻辑、周界引擎、两步控制命令和诊断 |
| `test_mqtt_tls`, `test_release` | 与代理的 TLS 连接（仅主机）以及发布和签名检查 |
| `check_panel_text`, `check_task_safety`, `check_save_paths`, `check_ble_session`, `check_routes`, `check_bridge_client` | Python：八种语言的面板文字（832 行）、任务、保存路径、受锁保护的蓝牙会话、路由和串口桥客户端 |

网页面板还会在真实浏览器中对着模拟节点（`tools/panel_mock.mjs`）进行验证。LD2461 和存在传感器的解码器只与其手册中的示例相符。

## 🏗️ 构建与发布

`tools/build_node.sh generic` 在 ESP-IDF 5.5.5 容器中为所有 Waveshare 电路板构建同一个镜像，`tools/flash.bat` 通过 USB-C 写入；仅 Wi-Fi 的电路板用 `s3-wifi` 构建同一固件。每个发布版都带有镜像、其 SHA-256 和 Ed25519 签名，每块电路板一份。

## 🩺 故障排除

| 现象 | 原因 | 处理 |
|---|---|---|
| 忘记了密码，或节点不再加入网络 | 处于设置状态且 90 秒后仍无地址的节点会开启自己的 Wi-Fi `ARMOR-SETUP-xxxxxx`（打开 `http://192.168.4.1/`） | 要重新开始，请在上电后 30 秒内按住 BOOT 8 秒（会清除设置和用户），或使用面板中的*恢复出厂设置* |
| 来自 GitHub 的更新被拒绝 | 发布版缺少校验和或签名，或二者不匹配（`e_no_checksum`、`e_no_signature`、`e_checksum_mismatch`、`e_bad_signature`），没有适用于此电路板的镜像（`e_no_asset_for_board`），或属于另一个项目（`e_wrong_firmware`） | 节点未发生任何变化；请重试，或由管理员手动上传文件 |
| 雷达命令返回 `no_answer`、`refused` 或 `disabled` | 雷达的 TX 线未连接、模块卡死，或节点先关闭了其蓝牙 | 检查接线（见台架调试），在面板中重启模块 |
| 节点没有显示传感器 3，或让雷达休眠 | 模块不再发送数据（静默，且没有坏帧） | 在雷达页面使用*重启模块*，然后关闭传感器自带的蓝牙 |

## 📂 仓库结构

<p align="center">
  <img src="images/repository-structure.svg" alt="ARMOR-NODE-RADAR 的仓库结构" width="100%">
</p>

<details>
<summary>目录树的文本版</summary>

```text
ARMOR-NODE-RADAR/
├── main/                                固件：一个 ESP-IDF 组件
├── panel/                               嵌入固件的网页面板
├── tests/                               主机测试和 Python 检查
├── docs/                                7 份台架与面板指南
├── tools/                               构建、刷写、接入、机群和检查工具
├── .github/                             CI 工作流以及 Issue 和 Pull Request 模板
├── images/                              横幅、图标和本 README 的两幅图
├── build.sh build.bat                   通过共享的项目工作流构建
├── test.sh test.bat                     通过共享工作流运行测试
├── run.sh run.bat                       通过共享的项目工作流启动
├── armor.project.json                   清单：版本、角色、成熟度、构建命令
├── CHANGELOG.md                         每个版本的变化
├── SECURITY.md SUPPORT.md LICENSE       安全策略、支持和许可证
├── CONTRIBUTING.md CODE_OF_CONDUCT.md   如何贡献与行为准则
├── CMakeLists.txt                       ESP-IDF 项目
├── partitions.csv                       闪存分区表
├── sdkconfig.defaults                   固件设置
├── sdkconfig.board.s3-eth               以太网电路板
├── sdkconfig.board.s3-wifi              Wi-Fi 电路板
├── secrets/                             机群和节点文件示例
├── build-test.sh build-test.bat         通过共享工作流运行测试
└── .gitattributes .gitignore            Git 设置
```

</details>

## 🗺️ 路线图与已知限制

**目前已知的限制**

* LD2461 和存在传感器的解码器只与手册中的示例相符，没有真实模块；命令通道只对 LD2450 验证过。
* 更新的签名校验和蓝牙 PIN 配对已在计算机上用真实签名验证，但尚未在电路板上验证；与代理的 TLS 握手尚未在电路板上运行。
* 健康报告和服务器控制尚未在电路板上运行；周界引擎从未配合传感器运行；面板自己的 HTTPS 证书未在所有浏览器中试过；Wi-Fi 电路板没有实物（只有以太网电路板）。

**下一步**

* 用真实传感器和 CM5 测试台运行周界引擎、健康报告和控制命令；在电路板上试验与代理的 TLS 连接和签名更新。

## 🌍 语言

README 和文本提供英语、西班牙语、法语、意大利语、德语、汉语、日语和欧洲葡萄牙语版本。

## 🔗 相关项目

**A.R.M.O.R.**（Autonomous Radar & Multimodal Observation Range）是一个由相互独立的仓库组成的周界安全系统。每个仓库都有自己的版本、自己的测试和自己的 README；这是整个系列：

* **[ARMOR-COMMON](https://github.com/JuanenRac/ARMOR-COMMON)** - 消息契约、验证器、一致性向量和生成的类型
* **ARMOR-NODE-RADAR**（本仓库） - 面向 ESP32-S3 的现场节点固件，配三个雷达和自带的网页面板
* **[ARMOR-NODE-SOLAR](https://github.com/JuanenRac/ARMOR-NODE-SOLAR)** - 太阳能逆变器和电池的协议，以及网关节点的消息
* **[ARMOR-NODE-ELECTRICAL](https://github.com/JuanenRac/ARMOR-NODE-ELECTRICAL)** - 电气节点：电表、电网读数的消息和切换规则
* **[ARMOR-NODE-ALARM](https://github.com/JuanenRac/ARMOR-NODE-ALARM)** - 报警节点与面板：防区、布防、延时、警笛和 PIN，可配合服务器也可独立使用
* **[ARMOR-NODE-HMI](https://github.com/JuanenRac/ARMOR-NODE-HMI)** - 触摸面板：壁挂屏幕上的系统状态、布防与确认，以及语音助手的所在
* **[ARMOR-NETWORK](https://github.com/JuanenRac/ARMOR-NETWORK)** - 本地网络：其设备、互联网以及变化
* **[ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER)** - 中央协调器：遥测、报警、设备、太阳能读数和摄像头
* **[ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO)** - 网页控制台：摄像头、雷达、报警、太阳能以及 2D/3D 场地设计器
* **[ARMOR-ANDROID-CONTROL](https://github.com/JuanenRac/ARMOR-ANDROID-CONTROL)** - 带实时 2D/3D 雷达的 Android 操作员客户端
* **[ARMOR-IOS-CONTROL](https://github.com/JuanenRac/ARMOR-IOS-CONTROL)** - iPhone 操作员客户端
* **[ARMOR-SERVER-AI](https://github.com/JuanenRac/ARMOR-SERVER-AI)** - 视觉推理策略，会解释其决策且从不驱动设备
* **[ARMOR-VISION](https://github.com/JuanenRac/ARMOR-VISION)** - 面向 Jetson Orin NX 的摄像头区域、遮罩和检测规则
* **[ARMOR-VOICE-AI](https://github.com/JuanenRac/ARMOR-VOICE-AI)** - 离线语音意图，附带无法伪造的确认
* **[ARMOR-HARDWARE](https://github.com/JuanenRac/ARMOR-HARDWARE)** - 外壳、电子器件和台架验收矩阵
* **[ARMOR-DEVOPS](https://github.com/JuanenRac/ARMOR-DEVOPS)** - 部署、CM5 测试台、备份和 TLS
* **[ARMOR-SIMULATOR](https://github.com/JuanenRac/ARMOR-SIMULATOR)** - 带可重复故障的离线遥测模拟器
* **[ARMOR-UPDATER](https://github.com/JuanenRac/ARMOR-UPDATER)** - 检测、安装和更新生态系统自身的仓库
* **[ARMOR-DOCS](https://github.com/JuanenRac/ARMOR-DOCS)** - 架构、安全基线和能力矩阵

## 📚 文档

* [节点面板](docs/NODE_PANEL.md)：首次启动、用户、网络、面板页面
* [设置码](docs/SETUP_CODE.md)和[蓝牙配置](docs/BLE_PROVISIONING.md)
* [传感器](docs/SENSORS.md)：六种型号及其解码器
* [台架调试](docs/BENCH_BRINGUP.md)、[台架测试](docs/BENCH_TESTS.md)和[硬件边界](docs/HARDWARE_BOUNDARY.md)
* [能力矩阵：哪些已被证明，哪些没有](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/CAPABILITY_MATRIX.md)
* [项目目录：版本以及各仓库之间如何相互依赖](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/PROJECT_CATALOG.md)
* [本仓库的更新日志](CHANGELOG.md)

## 🤝 参与贡献与社区

* 提交 Pull Request 之前请运行 `ctest --test-dir build/host` and `python -I tools/armor_ci_validate.py`：必须通过。参见 [CONTRIBUTING.md](CONTRIBUTING.md) 和[行为准则](CODE_OF_CONDUCT.md)。
* README 提供八种语言，由 ARMOR-DOCS 中的源文件生成：文本的修改在那里进行。
* 组件之间消息的变更从 ARMOR-COMMON 开始。
* 不要把仅仅构建过的东西描述为已在板子上运行。
* 请私下报告漏洞：[SECURITY.md](SECURITY.md)。问题、想法和报告：electrohobby3d@gmail.com（[SUPPORT.md](SUPPORT.md)）。

## 👤 作者

**JuanenRac (Electro Hobby 3D)** · electrohobby3d@gmail.com

## 📜 许可证

GPL-3.0-or-later - 见 [LICENSE](LICENSE)。
