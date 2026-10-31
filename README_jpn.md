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
  🇯🇵 <b>日本語</b> |
  <a href="README_por.md">🇵🇹 Português</a>
</p>

### フィールドノードのファームウェア（Waveshare ESP32-S3-ETH、レーダーまたは人感センサー 3 基、イーサネットと Wi-Fi）。独自の Web パネルと、コンピューターでテスト済みのコア付き

<p align="center">
  <img src="https://img.shields.io/badge/License-GPL%203.0--or--later-blue.svg" alt="GPL 3.0 or later">
  <img src="https://img.shields.io/badge/Language-C%2B%2B17-00599c.svg" alt="Language">
  <img src="https://img.shields.io/badge/Target-ESP32--S3-e7352c.svg" alt="Target">
  <img src="https://img.shields.io/badge/Tests-7371%20checks-2ea44f.svg" alt="Tests">
  <img src="https://img.shields.io/badge/Version-0.8.0.7-0f6cbd.svg" alt="Version">
  <img src="https://img.shields.io/badge/Maturity-established-2EA44F.svg" alt="Maturity">
</p>

---

**正直さのチェック - 今日動いているもの:** レーダーノードはハードウェアとして実在する唯一のノードです。そのファームウェアはレーダー付きの実際の Waveshare ESP32-S3-ETH 基板で動作して CM5 テストベンチに報告し、Web パネルと無線アップデートもその基板で使われています。ハードウェアに依存しないコア（5 つのテストプログラムで 950 項目：LD2450・LD2461 と 4 種の人感センサーのデコーダー、LD2450 のコマンドチャネル、設定とその検査、ピン表、ユーザーとセッション、ネットワーク計画、ARMOR-COMMON が出力を受け付けるメッセージシリアライザー、バージョンと署名の検査）はコンピューター上でテストされ、**Web パネル**は代替ノードに対して実際のブラウザーで動かし、**ファームウェアイメージ**は ESP-IDF 5.5.5 コンテナーでビルドできます。**まだ**証明されていないもの：LD2461 と人感センサーのデコーダーはマニュアルの計算例にしか一致せず、実モジュールがありません。アップデートの署名検査と PIN 付き Bluetooth ペアリングは実際の署名でコンピューター上で証明済みですが、基板上ではまだです。レーダーのコマンドチャネルはモジュールで確認していません。パネルの HTTPS 証明書（ノード自身が作る）はすべてのブラウザーで試していません。

---

## 📋 ひと目で

| | |
|---|---|
| バージョン | `0.8.0.7` |
| 成熟度 | 確立 |
| 役割 | ESP32-S3 用レーダーフィールドノード（ファミリー：Field Nodes、親：ARMOR-COMMON） |
| 技術 | C++17, ESP-IDF 5.5.5, ESP32-S3 |
| テスト | 7371 件の検査（ホスト用プログラム 14 本と Python 検査 6 個、`ctest`） |
| 基板 | Waveshare ESP32-S3-ETH（Ethernet）と ESP32-S3-WROOM-1 N16R8（Wi-Fi のみ） |
| センサー | ノードあたり最大 3 台のレーダー（LD2450、LD2461）または人感センサー、光センサー VEML7700 |
| 動作環境 | Waveshare ESP32-S3-ETH または ESP32-S3-WROOM-1 N16R8、ホストテストは任意のコンピューター |
| 実機での確認 | レーダー付きの実際の Waveshare ESP32-S3-ETH 基板で動作し、CM5 テストベンチに報告 |
| ライセンス | GPL-3.0-or-later |

**移動先：** [概要](#-概要) · [アーキテクチャ](#-アーキテクチャ) · [セキュリティ](#-セキュリティと安全) · [インターフェース](#-インターフェース) · [設定](#-設定) · [クイックスタート](#-クイックスタート) · [テスト](#-テスト) · [トラブルシューティング](#-トラブルシューティング) · [構成](#-リポジトリの構成) · [関連](#-関連プロジェクト)

## 🚦 プロジェクトの状況

各行はこのリポジトリの一部分で、信号は今日どこまで進んでいるかを示します。計画ではなく、リポジトリが示せるもの（テスト、ビルド、実行）に基づいて書かれています。

🟢 完了・検証済み · 🟡 動作する（既知の制限あり） · 🟠 一部のみ、または未検証 · 🔴 未実装または動作しない

| 領域 | 状態 | ここでの意味 |
|---|---|---|
| ソフトウェア | 🟢 | ファームウェアは ESP-IDF 5.5.5 コンテナでビルドでき、実機の ESP32-S3-ETH ボードで動作しています。 |
| テスト | 🟢 | 14 のホストプログラムで 7,371 件のチェックが通ります（`ctest`）：デコーダー、設定、ピン表、ユーザー、ネットワーク計画、メッセージのシリアライザー、バージョン確認。 |
| プロトコルとデータ | 🟡 | LD2450 は実機のレーダーで動作しています。LD2461 と 4 種類の人感センサーのデコーダーはマニュアルの例と一致するだけで、レーダーのコマンドチャネルはモジュールでは未確認です。 |
| インターフェース | 🟡 | Web パネルはボード上で使われており、実ブラウザーで模擬ノードに対しても試しています。ノード自身が作る HTTPS 証明書は、すべてのブラウザーでは試していません。 |
| ハードウェア | 🟡 | ハードウェアとして存在する唯一のノードです。実機の Waveshare ESP32-S3-ETH ボードとレーダーが CM5 テストベンチに報告しています。他のセンサーモジュールは接続していません。 |
| セキュリティ | 🟡 | Bluetooth の PIN ペアリングとアップデート署名の確認は、本物の署名を使ってコンピューター上で確認済みですが、ボード上ではまだです。独立したレビューもありません。 |
| アップデート | 🟡 | 無線アップデートはボード上で使われています。署名付きアップデートの確認はまだ実機では実行していません。 |
| 連携 | 🟡 | ボードのメッセージは CM5 テストベンチのサーバーに届き、同じメッセージは ARMOR-COMMON の共有スキーマと適合ベクターにも受理されます。実際の設置ではなく、テストベンチでのみ確認しています。 |
| ドキュメント | 🟢 | 8 言語と変更履歴は最新です。 |

## 🎯 概要

* **2 種類のボード、1 つのファームウェア：** Waveshare ESP32-S3-ETH（Ethernet、標準）と、Ethernet のない ESP32-S3-WROOM-1 N16R8（Wi-Fi のみ：セットアップで参加する Wi-Fi ネットワークを尋ね、ノードは常に独自のネットワークも保ちます）。イメージはビルド時に選びます（`tools/build_node.sh generic s3-eth` または `generic s3-wifi`）。ピン表とアクセス方法はボードに従い、パネルはボードにないものを隠し、イメージは対応するボード専用です。どちらもビルドでき、コンピューター上でテスト済みですが、実機では動かしていません。
* **1 つのノードで最大 270 度をカバー：** 各 Waveshare ESP32-S3-ETH は 3 本の UART で最大 3 基の LD2450 レーダーを読み取ります。レーダーは 75 度ずつずらして取り付け、有線イーサネット（W5500）で DHCP または固定アドレス、給電は PoE か USB です。Studio の *Add a 270° node* は、ノードに割り当て済みの 3 基のレーダーを作成します。
* **すべてのノードに Web パネル：** Studio と同じ見た目で 8 言語に対応し、ファームウェアに組み込まれています。概要、ネットワーク、Wi-Fi、ブローカー、レーダー、ピン、ユーザー、ファームウェア更新、ログ。ユーザーのいないノードは Wi-Fi `ARMOR-SETUP-xxxxxx` を開き、セットアップコードで最初の管理者を作成します。パスワードはソルト付き PBKDF2、セッションはランダムなトークン、すべての設定はノードのフラッシュにあるので、1 つのイメージがすべてのノードに使え、パスワードはコンパイルされません（[パネル](docs/NODE_PANEL.md)）。
* **複数のノードで 1 つの Wi-Fi：** 各ノードはイーサネットポートにつながるアクセスポイントを提供できます。同じ名前とパスワードにしてチャネルを自動（MAC により 1、6、11）にすると、スマートフォンと Wi-Fi センサーには 1 つの DHCP サーバーを持つ 1 つのネットワークに見えます。無線メッシュではなく、各ノードはケーブルを保ちます。代わりにルーターの Wi-Fi に参加でき（ネットワーク検索付き）、ケーブルのないノードは Android アプリから **Bluetooth** で設定できます。
* **サーバー用のピン：** 空いているピンは入力、出力（ブローカーを失ったときの安全な状態付き）、PWM、アナログ読み取りになり、サーバーのデバイスとして現れるので、リレーや接点に新しいファームウェアは要りません。ボードの予約ピンは提示されません。
* **ロールバック付きのオーバーザエア更新**をパネルから行えます（16 MB のフラッシュに 2 スロット）。また Studio から各ノードのパネルへ、ノードが公開するアドレスを使う**リンク**があります。
* **パネルの HTTPS：** ノードは自分の証明書（自己署名、フラッシュに保存）を作り、80 番に加えて 443 番、または 443 番のみでパネルを提供します。セッションクッキーは Secure が付き、ブラウザーの警告と照合できるよう証明書のフィンガープリントを表示します。
* **安定したトラック ID：** 各フレームのターゲットは以前追跡していたものと対応づけられるため、人は同じトラック ID を保ち、フレーム欠落でちらつかず、位置は少し平滑化されます。
* **センサーモデルは 6 種類、ポートごとに 1 つ：** ポートには LD2450 または LD2461（周辺を担うトラッカーで、Studio にモデル固有の視野あり）か、人感センサー（LD2410、LD2412、LD2410S、MR24HPC1）を接続でき、後者はサーバーのデバイスになって在・不在と距離を公開します。パネルで選びます。各センサーの説明、プロトコル、未検証の点は `docs/SENSORS.md` にあります。
* **LD2450 のデコーダー、状態、設定：** 再同期するフレーム検出器がノイズの多いストリームからフレームを見つけ、30 バイトの各フレームから最大 3 つのターゲットが契約のトラックになります。コンソールとパネルはレーダーごとに、報告中か、沈黙か、文字化けかを示します。パネルはモジュールのバージョンを読み、1 つまたは 3 つのターゲットを選び、検出ゾーンを設定することもできます（モジュールでは未検証のプロトコル）。
* **周囲光と契約に厳密なメッセージ：** 自動レンジの VEML7700。公開スキーマに従い、無効なものは書き出さないテレメトリ・ヘルス・情報の JSON、壁時計のタイムスタンプ（SNTP）、MQTT の遺言。どのレーダーも報告していない間はテレメトリを控え、空の「異常なし」は決して送りません。
* **ネットワーク製品のように、全ボード共通の 1 つのイメージ：** ファームウェアは同一で、MAC がボードを区別します（名前を付けるまでは `armor-` と 6 桁）。`adopt_node.py` は、書き込んだばかりのノードにネットワーク経由で管理者、ブローカーの ID、フリート共通の設定を与えます。セットアップコードは MAC から計算されるので、5 台でも 27 台でも作業は同じです。
* **ベンチツール：** USB-C での書き込み、パネル開発用の代役ノード、生フレームのログをテスト用フィクスチャに変換するコンバーター（[ベンチでの立ち上げ](docs/BENCH_BRINGUP.md)）。
* **署名付きの無線アップデート：**ノードが GitHub からインストールするのは、SHA-256 が一致し、管理者の有効な Ed25519 署名を持つイメージだけです（公開鍵はファームウェア内、秘密鍵は管理者の手元から出ません）。管理者がパネルで手動アップロードしたファイルは引き続き受け付けます。パネルのページとメッセージは 8 言語です。
* **PIN 付きの Bluetooth ペアリング：**6 桁の PIN は設定コードから導出され、USB コンソールに表示され、管理者にはパネルにも表示されます。スマートフォンはこれを入力して初めてノードの設定を書き込めます。
* **パネルを使えるのは家だけ：**パネルは既定でプライベートネットワーク（または所有者が指定した VPN などのネットワーク）にだけ応答し、保存する本人を締め出す一覧は拒否します。誤ったパスワードは人ごと・アドレスごとに数え、サーバーのプロキシの背後でも動くので、ノードを公開せずに家の外から使えます。
* **ブローカーとの暗号化通信(TLS、任意):** 初期状態ではオフで、ノードはこれまでどおり暗号化なしの MQTT で通信します。ブローカーページのスイッチと、設備独自の認証局の証明書(PEM、最大 2 KB)の入力欄により、`mqtts://` で接続し、その認証局だけを信頼し、到達したアドレス向けに発行された証明書かを確認します。PEM 証明書 1 つでないものは拒否されます。ホストテストで検証済みで、ハンドシェイクはまだ基板では実行していません。
* **状態報告:** ノードは約 1 分ごとに、稼働時間と直前の再起動の理由、空きメモリ、回線と Wi-Fi 信号、ブローカーへの接続回数、最後に記録したエラーを公開します（`armor/health/<ノード>`、ARMOR-COMMON の `node_health` 契約）。作成するコードは全ノードで共有され、コンピューター上でテストされていますが、ボード上ではまだ動作していません。
* **サーバーからの制御：** Studio はノードをきれいに再起動し、最大 1 時間まで一時停止し（自動的に再開）、作業を待機にし（期限なし、再起動後も維持、ノードは接続したまま応答を続けます）、自分を表示させ、診断のスナップショットを求め、メンテナンス中として印を付けることができます（`armor/control/<ノード>/...`、ARMOR-COMMON の `node_control` 契約）。命令はすべて、その操作専用の使い捨てトークンによる 2 段階で、ブローカーに書き込めるのはサーバーだけです。工場出荷時リセットも消去もディープスリープもありません。一時停止中または待機中のノードは、ターゲットも在室も発行しません（ハートビートは送り続けます）。 このコードは全ノードで共有され、コンピューター上でテスト済みですが、実機ではまだ動作していません。
* **ノード上の境界エンジン（既定ではオフ）：** 6つの検知ゾーンと3つの除外ゾーン（それぞれ角度で回転できる長方形）、方向別に通過を数える3本のトリップワイヤー、3つの侵入経路、ペット除外、マスキング検知。センサーのそばで動作するため、ブローカーに接続できなくても機能し、ブローカー不在の間はイベントを保持して `armor/radar/<ノード>/events` に発行します。ゾーンエディターはノードのパネルにあります。これらはまだセンサーを接続した実機では動かしていません。
* **各センサーの正体と、センサー自身のBluetoothの停止：** 起動時にノードは各センサーのファームウェアバージョンとBluetoothアドレスを読み取り、センサー自身のBluetooth（PINを要求しません）を既定で閉じ、1台のセンサーのシリアル回線を数分間、管理者のPCツールに渡せます（透過シリアルブリッジ）。管理者向けの `GET /metrics` と、設定のエクスポートおよび全か無かのインポートがこれを補います。
* **ケーブルなしの診断：** 正常でない起動（クラッシュ、ウォッチドッグ、電源の電圧低下、メモリ確保の失敗）の後、ヘルスレポートが10分間その原因を伝えます。クラッシュするとコアダンプが残り、パネルから確認、ダウンロード、削除できます。保存された設定はファームウェアのインストールごとにコピーされ、更新で壊れた場合は復元されます。管理者はサーバー経由で、使い捨てトークンを使って、ノードに診断パッケージまたはログを要求できます。
* **証明書フィンガープリントを固定した HTTPS での登録:** `tools/adopt_node.py` は `https://<アドレス>` を受け付け、`--fingerprint`(ノード証明書の SHA-256。リクエストを運ぶ接続そのものの上で確認)または読み取り用の `--show-fingerprint` を使えます。平文の HTTP や、固定アドレス・ホスト名を含む構成ファイルには警告し、そのようなファイルをパネルに読み込むときも警告します。
* **センサーの読み取りはポーリングではなくイベント待ち:** 読み取りタスクはシリアル回線のイベントキューで待機し、届いた分をすべて取り出し、あふれた回線は空にして、回線ごとにあふれとエラーを数えます。`system.reader_idle_ms`(100。0 なら従来の 10 ms ポーリング)で待ち時間を決め、キューが失敗した場合は自動的にポーリングへ戻ります。
* **保存された秘密情報の任意の封印:** 既定ではオフです(`security.seal_secrets`、またはサーバーの `nodes.nvs_encryption`)。オンにすると、ノードが保存するパスワード、トークン、鍵、PIN、パスワードのハッシュが ChaCha20-Poly1305 で封印され、それ以外は読めるままです。オン/オフの切り替えは何も失わずに書き直し、途中で止まっても大丈夫です。eFuse は焼かれず、フラッシュも暗号化されません。鍵は同じチップのパーティションにあるため、基板を手にした人に対する保護には**なりません**。このパーティションがない時期にインストールされたノードは、新しいパーティションテーブルで書き込むまで従来のままです。

## 🧩 A.R.M.O.R. での役割

ARMOR-NODE-RADAR は境界の目です。各ノードは最大 3 台のレーダーまたは人感センサーを読み取り、軌跡・照度・状態を MQTT で [ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER) に送ります。[ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO) はそれをサイト図に描き、[ARMOR-SERVER-AI](https://github.com/JuanenRac/ARMOR-SERVER-AI) はカメラと照らし合わせます。ファームウェアは他のノードと土台を共有し（パネル、Bluetooth 設定、署名付きアップデート、サインイン、時計は [ARMOR-COMMON](https://github.com/JuanenRac/ARMOR-COMMON) のファームウェア基盤から）、1 つのノードで覚えたことがすべてに通用します。

| 名前 | 方法 | 用途 |
|---|---|---|
| ARMOR-SERVER | MQTT `armor/node/<id>/telemetry|health|info|command` | レーダーのメッセージを受け取り（現在は CM5 テストベンチ）、ノードに命令する |
| ARMOR-STUDIO | パネルへのリンク | ノードが公開するアドレスからノードのパネルを開く |
| ARMOR-ANDROID-CONTROL | Bluetooth | 現場でノードを設定 |
| ARMOR-COMMON | コントラクト、ファームウェア基盤 | メッセージはそのスキーマで検証し、共通ファームウェアはここからコピー |

## 🏛️ アーキテクチャ

<p align="center">
  <img src="images/architecture.svg" alt="ARMOR-NODE-RADAR のアーキテクチャ：呼び出し元、内部、依存先" width="100%">
</p>

## 🖼️ スクリーンショット

テストベンチで 1920×1080 で撮影。サンプルデータを使用しており、実際の設備は接続されていません。

<table>
<tr><td width="50%" valign="top"><img src="images/screenshots/overview.png" alt="概要：ネットワーク、ブローカー、ファームウェア、報告内容など、ノードの状態をひと目で確認できます。" width="100%"><br><sub>概要：ネットワーク、ブローカー、ファームウェア、報告内容など、ノードの状態をひと目で確認できます。</sub></td><td width="50%" valign="top"><img src="images/screenshots/radars.png" alt="レーダー：ノードの各レーダーの型式、ピン、キャリブレーション。" width="100%"><br><sub>レーダー：ノードの各レーダーの型式、ピン、キャリブレーション。</sub></td></tr>
<tr><td width="50%" valign="top"><img src="images/screenshots/pins.png" alt="ピン：各ピンの役割。2 つの機能が同じピンを共有しないよう検査されます。" width="100%"><br><sub>ピン：各ピンの役割。2 つの機能が同じピンを共有しないよう検査されます。</sub></td><td width="50%" valign="top"><img src="images/screenshots/network.png" alt="ネットワーク：ノードの Ethernet または Wi-Fi のアドレス設定を、専用の Web パネルから行います。" width="100%"><br><sub>ネットワーク：ノードの Ethernet または Wi-Fi のアドレス設定を、専用の Web パネルから行います。</sub></td></tr>
</table>

## 🔒 セキュリティと安全

| 脅威 | 対策 |
|---|---|
| 平文またはコンパイル済みのパスワード | PBKDF2-HMAC-SHA256 10 000 ラウンド。ファームウェアにパスワードはなく（ユーザーのないノードはセットアップに入る）、ユーザーは最大 4 人（管理者または閲覧者）、8〜64 文字、セッションは最大 6、無操作 30 分 |
| 書き込み直後のノードの乗っ取り | セットアップには起動コードが必要です。ビルドのもの（`CONFIG_ARMOR_SETUP_CODE`）か、フラッシュに保存して USB コンソールに表示するランダムなもの。フリートシークレットがあれば `HMAC-SHA256(シークレット, MAC)`。これはセットアップ用 Wi-Fi（WPA2、ブリッジなし）の鍵でもあります |
| インターネットからのパネルへのアクセス | 初期設定では家庭のプライベートネットワーク（10/8、172.16/12、192.168/16、169.254/16、127/8、一部の VPN が使う 100.64/10）にだけ応答します。設定 `web.admin_sources` で他の IPv4 アドレスやネットワークを追加でき（8 件、120 文字）、それ以外は公開の `GET /api/v1/session` を除き拒否します。保存した本人を締め出すリストは拒否されます |
| HTTP で平文のパネル | 443 番ポートで HTTPS。証明書はノード自身が作成（自己署名、フラッシュ保存、比較用に SHA-256 フィンガープリントを表示）。パネルは http、https、または両方を選べ、セッション Cookie は常に HttpOnly と SameSite=Strict（TLS では Secure も） |
| GitHub からの偽ファームウェア | SHA-256 が一致し、メンテナーの鍵による有効な Ed25519 署名があるイメージだけをインストールします（`no_checksum`、`no_signature`、`bad_signature`、`checksum_mismatch`、`wrong_firmware`、`not_newer`、`version_mismatch` は拒否）。管理者の手動アップロードは検証されません（明示した制限） |
| Bluetooth セットアップの乗っ取り | セットアップコードから導出した 6 桁の PIN を USB コンソールと管理者に表示し、設定を書き込む前にスマホで入力させます。セットアップの時間枠は `CONFIG_ARMOR_BLE_SETUP_MINUTES`（初期 10、1〜120） |
| 害を及ぼす遠隔命令 | その操作に結び付いた使い捨てトークンによる 2 段階。ブローカーのそのトピックに書き込めるのはサーバーだけで、一時停止は最長 1 時間。遠隔命令による工場出荷状態への復元、消去、ディープスリープはありません |
| 基板を持つ人による保存済みシークレットの読み取り | 任意（初期はオフ、スイッチ `security.seal_secrets` またはサーバーの `nodes.nvs_encryption`）：パスワード、トークン、鍵、PIN、ハッシュを ChaCha20-Poly1305 で保護。フラッシュ全体の暗号化や eFuse の焼き込みはしません。鍵は同じチップの別パーティションにあるため、基板を手にした人には防御になりません |
| ブローカーとの平文通信 | 任意の TLS（`mqtts://`）。所有者自身の認証局（PEM 2 KB まで）と名前の検証を使い、初期はオフです。ハンドシェイクはホストのテストでしか動いておらず、基板では未実行です |
| センサー自身の Bluetooth | 既定ではノードがすべての LD センサーの Bluetooth を閉じます（PIN なし）。コンピューターへの透過シリアルブリッジは管理者が数分間だけ開けます |
| 信頼できないネットワークでのノードの採用 | `tools/adopt_node.py https://<アドレス> --fingerprint <SHA-256>` は同じ接続で証明書を検証し、平文 HTTP や固定アドレスのフリートファイルを警告します |
| 危険なピン | 基板の予約ピンは提示されず、ブローカーを失うと出力は安全な状態になり、2 つの機能が同じピンを共有することはできません（検査済み） |
| フリートシークレットを含む公開ファームウェア | `CONFIG_ARMOR_FLEET_SECRET` は初期で空で、公開ビルドにシークレットは含まれません |

脆弱性の報告は [SECURITY.md](SECURITY.md) を参照してください。独立したレビューはありません。

## 🔌 インターフェース

| 部分 | 設定内容 |
|---|---|
| MQTT | `armor/node/<id>/{telemetry,health,info,command}`、`armor/health/<id>`（遺言は `{"node_id":"..","online":false}`）、`armor/control/<id>/{command,result,maintenance}` と `armor/control/all/command`、`armor/device/<id>/<pin>/{state,set}`、`armor/radar/<id>/events`、`armor/radar/<id>/control/armed` |
| パネル（HTTP 80、HTTPS 443） | ルート `session`、`login`、`logout`、`setup`、`status`、`config`（+ export、import）、`wifi/scan`、`time`、`radars`（+ command）、`perimeter`（+ armed、learn、zone）、`pins`（+ command）、`bridge`（+ open、close、io）、`ota`（+ check、install、progress、copy、switch）、`account`、`ble-pin`、`log`、`coredump`、`factory-reset`、`reboot`。管理者向けの `/metrics` と WebSocket `/ws/radar-map`。`tests/check_routes.py` がパネルの呼ぶルートがすべて存在することを検査します |
| パネルの文言 | 8 言語の文言つきエラーコード 76 個（`panel/text.js` に 832 行）。`tests/check_panel_text.py` が検査 |
| ハードウェア | LD2450、LD2461、LD2410、LD2412、LD2410S、MR24HPC1 用の UART 3 本（RX 16、17、18、TX 15、21、38）、SPI の W5500（MOSI 11、MISO 12、SCLK 13、CS 14、INT 10、RST 9、20 MHz）、I2C の VEML7700（GPIO 1 と 2）、BOOT ボタン |

## ⚙️ 設定

| 変数 | 既定値 | 設定内容 |
|---|---|---|
| `CONFIG_ARMOR_NODE_ID` | `""` | 名前。空なら `armor-` と MAC の下位 3 バイト |
| `CONFIG_ARMOR_SETUP_CODE`, `CONFIG_ARMOR_FLEET_SECRET` | `""` | ビルドのセットアップコード（空ならランダム）とフリートシークレット（16 文字以上、リポジトリには置かない） |
| `CONFIG_ARMOR_RADAR_COUNT` | 3 (0-3) | 接続されているレーダーの数 |
| `CONFIG_ARMOR_TRACK_STABILISER` | y | フレーム間でトラックの ID を保つ |
| `CONFIG_ARMOR_MQTT_URI`, `_USERNAME`, `_PASSWORD` | `""` | 最初のブローカー設定（パネルで変更可能） |
| `CONFIG_ARMOR_HEARTBEAT_S` | 10 (2-300) | ヘルスビートの間隔（秒） |
| `CONFIG_ARMOR_TELEMETRY_PERIOD_MS` | 200 (200-5000) | 毎秒最大 5 メッセージ |
| `CONFIG_ARMOR_BLE_SETUP_MINUTES` | 10 (1-120) | Bluetooth セットアップを開けておく時間 |
| `CONFIG_ARMOR_BOARD_S3_ETH` / `_S3_WIFI` | eth | 基板：Waveshare ESP32-S3-ETH または ESP32-S3-WROOM-1 N16R8（Wi-Fi のみ） |
| `CONFIG_ARMOR_BOOT_FACTORY_RESET` | y | 最初の 30 秒以内に BOOT を 8 秒押すと初期化 |

ライブで調整する設定はパネルにあり、フラッシュに保存されます（例：`system.reader_idle_ms`、`security.seal_secrets`、1 分あたりのログイン失敗上限（30、0〜120）、境界エンジンのゾーン）。ファームウェアの既定値は `sdkconfig.defaults` と `sdkconfig.board.*` にあります。16 MB のフラッシュには `nvs`（64 KB）、`otadata`、`phy_init`、3 MB のスロット `ota_0` と `ota_1`、`coredump`（128 KB）、`nvs_keys`（4 KB）があります（`partitions.csv`）。

## 🚀 クイックスタート

**必要なもの:** ホストテスト用の C++17 コンパイラ、イメージのビルド用の ESP-IDF 5.5.5 コンテナ、Waveshare ESP32-S3-ETH または ESP32-S3-WROOM-1 N16R8。

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

ホストテストには C++17 コンパイラー（Linux、WSL、MSYS2）が必要です。[ベンチでの立ち上げ](docs/BENCH_BRINGUP.md)、[パネル](docs/NODE_PANEL.md)、[ハードウェアの境界](docs/HARDWARE_BOUNDARY.md)を参照。

## 🧪 テスト

```bash
cmake -S tests -B build/host && cmake --build build/host && ctest --test-dir build/host
build/host/emit_samples | python tests/check_contract.py
python -I tools/armor_ci_validate.py
```

ホスト用テストプログラム 14 個と Python の検査 6 個を `ctest` で実行します。ホストのプログラムが数えた検査は 7371 件で、`-Werror` でビルドします。

| 部分 | 対象 |
|---|---|
| `test_core`, `test_sensors`, `test_family`, `test_uart_wake` | デコーダー（LD2450、LD2461、人感センサー 4 種）、フレーム分割、コマンドチャネル、センサーファミリー、UART のウェイク |
| `test_node`, `test_base`, `test_shared`, `test_board_wifi` | 設定とその検査、ピンテーブル、ユーザーとセッション、ネットワーク計画、共通基盤、Wi-Fi 基板 |
| `test_logic`, `test_perimeter`, `test_control`, `test_diag` | ピンのロジック、境界エンジン、2 段階の制御命令、診断 |
| `test_mqtt_tls`, `test_release` | ブローカーへの TLS 接続（ホストのみ）とリリース・署名の検査 |
| `check_panel_text`, `check_task_safety`, `check_save_paths`, `check_ble_session`, `check_routes`, `check_bridge_client` | Python：8 言語のパネル文言（832 行）、タスク、保存パス、ロック下の Bluetooth セッション、ルート、シリアルブリッジのクライアント |

Web パネルは、代役ノード（`tools/panel_mock.mjs`）を相手に実際のブラウザでも検証します。LD2461 と人感センサーのデコーダーはマニュアルの例とだけ一致しています。

## 🏗️ ビルドとリリース

`tools/build_node.sh generic` は ESP-IDF 5.5.5 コンテナで全 Waveshare 基板共通の 1 つのイメージをビルドし、`tools/flash.bat` が USB-C で書き込みます。Wi-Fi のみの基板向けには `s3-wifi` で同じファームウェアをビルドします。リリースにはイメージ、その SHA-256 と Ed25519 署名が基板ごとに付きます。

## 🩺 トラブルシューティング

| 症状 | 原因 | 対処 |
|---|---|---|
| パスワードを忘れた、またはノードがネットワークに参加しない | セットアップ中でアドレスがないノードは 90 秒後に独自の Wi-Fi `ARMOR-SETUP-xxxxxx` を開きます（`http://192.168.4.1/` を開く） | 最初からやり直すには、電源投入後 30 秒以内に BOOT を 8 秒押し続ける（設定とユーザーを消去）か、パネルの*工場出荷状態に戻す*を使います |
| GitHub からの更新が拒否される | リリースにチェックサムや署名がない、一致しない（`e_no_checksum`、`e_no_signature`、`e_checksum_mismatch`、`e_bad_signature`）、この基板用のイメージがない（`e_no_asset_for_board`）、または別プロジェクトのもの（`e_wrong_firmware`） | ノードは変更されていません。再試行するか、管理者が手動でファイルをアップロード |
| レーダーのコマンドが `no_answer`、`refused`、`disabled` を返す | レーダーの TX 線が未接続、モジュールがフリーズ、またはノードが先に Bluetooth を閉じている | 配線（ベンチ立ち上げの手順）を確認し、パネルからモジュールを再起動 |
| センサー 3 が表示されない、またはレーダーが眠ったままになる | モジュールが応答しなくなった（不正フレームなしの無音） | レーダーページの*モジュール再起動*を使い、その後センサー自身の Bluetooth を閉じる |

## 📂 リポジトリの構成

<p align="center">
  <img src="images/repository-structure.svg" alt="ARMOR-NODE-RADAR のリポジトリ構造" width="100%">
</p>

<details>
<summary>ツリーのテキスト版</summary>

```text
ARMOR-NODE-RADAR/
├── main/                                ファームウェア（ESP-IDF コンポーネント）
├── panel/                               ファームウェアに埋め込まれた Web パネル
├── tests/                               ホストテストと Python 検査
├── docs/                                ベンチとパネルの 7 つのガイド
├── tools/                               ビルド・書き込み・採用・フリート・検査のツール
├── .github/                             CI ワークフローと Issue・Pull Request のテンプレート
├── images/                              バナー、アイコン、この README の 2 つの図
├── build.sh build.bat                   共通のプロジェクトワークフローでビルド
├── test.sh test.bat                     共通のワークフローでテストを実行
├── run.sh run.bat                       共通のプロジェクトワークフローで起動
├── armor.project.json                   マニフェスト：バージョン、役割、成熟度、ビルドコマンド
├── CHANGELOG.md                         各バージョンの変更点
├── SECURITY.md SUPPORT.md LICENSE       セキュリティポリシー、サポート、ライセンス
├── CONTRIBUTING.md CODE_OF_CONDUCT.md   貢献方法と行動規範
├── CMakeLists.txt                       ESP-IDF プロジェクト
├── partitions.csv                       フラッシュのテーブル
├── sdkconfig.defaults                   ファームウェアの設定
├── sdkconfig.board.s3-eth               Ethernet 基板
├── sdkconfig.board.s3-wifi              Wi-Fi 基板
├── secrets/                             フリートとノードのファイルの例
├── build-test.sh build-test.bat         共通のワークフローでテストを実行
└── .gitattributes .gitignore            Git の設定
```

</details>

## 🗺️ ロードマップと既知の制限

**現時点の既知の制限**

* LD2461 と人感センサーのデコーダーはマニュアルの例とだけ一致し、実モジュールはありません。コマンドチャネルは LD2450 でしか確認していません。
* 更新の署名検証と Bluetooth の PIN ペアリングは実際の署名でコンピューター上では確認済みですが、基板ではまだです。ブローカーとの TLS ハンドシェイクは基板で未実行です。
* ヘルスレポートとサーバーからの制御は基板で未実行、境界エンジンはセンサーとは一度も動いておらず、パネル自身の HTTPS 証明書はすべてのブラウザで未検証、Wi-Fi 基板の実機はありません（Ethernet 基板のみ）。

**今後**

* 実センサーと CM5 テストベンチで境界エンジン、ヘルスレポート、制御命令を動かし、ブローカーへの TLS 接続と署名付き更新を基板で試す。

## 🌍 言語

README とテキストは、英語、スペイン語、フランス語、イタリア語、ドイツ語、中国語、日本語、ヨーロッパポルトガル語で提供されています。

## 🔗 関連プロジェクト

**A.R.M.O.R.**（Autonomous Radar & Multimodal Observation Range）は、独立したリポジトリで構成される境界セキュリティシステムです。各リポジトリは独自のバージョン、テスト、README を持ちます。ファミリーは次のとおりです：

* **[ARMOR-COMMON](https://github.com/JuanenRac/ARMOR-COMMON)** - メッセージのコントラクト、バリデーター、適合ベクトル、生成された型
* **ARMOR-NODE-RADAR**（このリポジトリ） - 3 つのレーダーと独自の Web パネルを備えた ESP32-S3 向けフィールドノードのファームウェア
* **[ARMOR-NODE-SOLAR](https://github.com/JuanenRac/ARMOR-NODE-SOLAR)** - ソーラーインバーターとバッテリーのプロトコル、ゲートウェイノードのメッセージ
* **[ARMOR-NODE-ELECTRICAL](https://github.com/JuanenRac/ARMOR-NODE-ELECTRICAL)** - 電気ノード：メーター、系統の読み取り値のメッセージ、切り替えのルール
* **[ARMOR-NODE-ALARM](https://github.com/JuanenRac/ARMOR-NODE-ALARM)** - 警報ノードとパネル：ゾーン、警戒、遅延、サイレン、PIN。サーバーありでもなしでも動作
* **[ARMOR-NODE-HMI](https://github.com/JuanenRac/ARMOR-NODE-HMI)** - タッチパネル：壁面画面でのシステムの状態、警戒と確認応答、そして音声アシスタントの置き場
* **[ARMOR-NETWORK](https://github.com/JuanenRac/ARMOR-NETWORK)** - ローカルネットワーク：そのデバイス、インターネット、そして変化
* **[ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER)** - 中央コーディネーター：テレメトリ、警報、デバイス、ソーラーの読み取り値、カメラ
* **[ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO)** - Web コンソール：カメラ、レーダー、警報、太陽光エネルギー、2D/3D のサイトデザイナー
* **[ARMOR-ANDROID-CONTROL](https://github.com/JuanenRac/ARMOR-ANDROID-CONTROL)** - ライブの 2D/3D レーダーを備えた Android オペレータークライアント
* **[ARMOR-IOS-CONTROL](https://github.com/JuanenRac/ARMOR-IOS-CONTROL)** - iPhone オペレータークライアント
* **[ARMOR-SERVER-AI](https://github.com/JuanenRac/ARMOR-SERVER-AI)** - 判断を説明し、決して作動させない視覚推論ポリシー
* **[ARMOR-VISION](https://github.com/JuanenRac/ARMOR-VISION)** - Jetson Orin NX 向けのカメラゾーン、マスク、検出ルール
* **[ARMOR-VOICE-AI](https://github.com/JuanenRac/ARMOR-VOICE-AI)** - 偽造できない確認つきのオフライン音声インテント
* **[ARMOR-HARDWARE](https://github.com/JuanenRac/ARMOR-HARDWARE)** - 筐体、電子回路、ベンチ受け入れマトリクス
* **[ARMOR-DEVOPS](https://github.com/JuanenRac/ARMOR-DEVOPS)** - デプロイ、CM5 テストベンチ、バックアップ、TLS
* **[ARMOR-SIMULATOR](https://github.com/JuanenRac/ARMOR-SIMULATOR)** - 再現可能な障害を持つオフラインのテレメトリシミュレーター
* **[ARMOR-UPDATER](https://github.com/JuanenRac/ARMOR-UPDATER)** - エコシステム自身のリポジトリを検出、インストール、更新
* **[ARMOR-DOCS](https://github.com/JuanenRac/ARMOR-DOCS)** - アーキテクチャ、セキュリティのベースライン、機能マトリクス

## 📚 ドキュメント

* [ノードのパネル](docs/NODE_PANEL.md)：初回起動、ユーザー、ネットワーク、パネルのページ
* [セットアップコード](docs/SETUP_CODE.md)と [Bluetooth プロビジョニング](docs/BLE_PROVISIONING.md)
* [センサー](docs/SENSORS.md)：6 つのモデルとデコーダー
* [ベンチ立ち上げ](docs/BENCH_BRINGUP.md)、[ベンチテスト](docs/BENCH_TESTS.md)、[ハードウェアの境界](docs/HARDWARE_BOUNDARY.md)
* [機能マトリクス：何が証明され、何が証明されていないか](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/CAPABILITY_MATRIX.md)
* [プロジェクトカタログ：バージョンと、リポジトリ同士の依存関係](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/PROJECT_CATALOG.md)
* [このリポジトリの変更履歴](CHANGELOG.md)

## 🤝 コントリビュートとコミュニティ

* Pull Request の前に `ctest --test-dir build/host` and `python -I tools/armor_ci_validate.py` を実行してください。通る必要があります。[CONTRIBUTING.md](CONTRIBUTING.md) と[行動規範](CODE_OF_CONDUCT.md)を参照してください。
* README は 8 言語で提供され、ARMOR-DOCS のソースから生成されます。文面の変更はそちらで行います。
* コンポーネント間のメッセージの変更は ARMOR-COMMON から始めます。
* ビルドしただけのものを、ボード上で動いているかのように書かないでください。
* 脆弱性は非公開で報告してください：[SECURITY.md](SECURITY.md)。質問、アイデア、報告：electrohobby3d@gmail.com（[SUPPORT.md](SUPPORT.md)）。

## 👤 作者

**JuanenRac (Electro Hobby 3D)** · electrohobby3d@gmail.com

## 📜 ライセンス

GPL-3.0-or-later - [LICENSE](LICENSE) を参照してください。
