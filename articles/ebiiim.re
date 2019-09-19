= GoでBluetooth Low Energy

GoでBLEを触ってみたので、必要な知識をまとめました。
BLEとGATTの概要を確認した後、
Gobotフレームワークを用いて心拍数センサから
取得したデータを表示するプログラムを作成することで
GoでBLEを扱う方法の一例を確認します。
Raspberry Piなど、GoとBluetoothの両方が使用できる端末があればいろいろ遊べそうです。

== BLE（Bluetooth Low Energy）

Bluetooth Low Energy（以降BLEとします）とは、
Bluetooth 4.0以降の機能で、少ない消費電力で動作する通信モードです。
従来のBluetooth（Bluetooth Classic）との互換性はありませんが、
次に示す優れたスペックによりIoTを支える技術として広く定着しています。

 * ボタン電池で2年間動作可能
 * 低レイテンシ（数ms）で通信可能
 * 実用上無制限の同時接続台数（Bluetooth Classicは7台）

BLEにおけるデバイス間の通信方法は、ブロードキャスト（1対多）とコネクション（1対1）の2種類があります。
ペアリングして行う一般的な通信はコネクションです。
ペアリング後のアプリケーションデータの送受信方式は、
Generic Attribute Profile（以降GATTとします）で定義されます。

GATTでは、通信を開始する側をクライアントと、クライアントからの要求に応答する側をサーバと呼びます。
たとえば、本稿ではGoのプログラムを用いて心拍数センサからデータを取得しますが、
この場合はGoのプログラムを動作させる機器がクライアントで心拍数センサがサーバです。

=== GATT（Generic Attribute Profile）

GATTは、次に示す概念により、クライアントによるデータの読み書きとサーバによるプッシュ通知のためのデータモデルを定義します。

==== キャラクタリスティック（Characteristic）

UUIDで区別される@<fn>{uuid}、データをやりとりするための仕様です。
例を示します。

 * Battery Level（0x2A19）: バッテリー残量を伝達するためのデータ形式
 * Heart Rate Measurement（0x2A37）: 心拍数を伝達するためのデータ形式

==== サービス（Service）

単一または複数のキャラクタリスティックで構成される、機能を提供する仕様です。
例を示します。

 * BAS（Battery Service）: Battery Level（0x2A19）を実装すること
 * HRS（Heart Rate Service）: Heart Rate Measurement（0x2A37）、Body Sensor Location（0x2A38）（任意）、Heart Rate Control Point（0x2A39）を実装すること

==== プロファイル（Profile）

単一または複数のサービスで構成される、ユースケースを提供する仕様です。
例を示します。

 * HRP（Heart Rate Profile）
 ** クライアントはHRSとDIS@<fn>{dis}を実装すること
 ** サーバはHRSとDIS（任意）を実装すること

//footnote[dis][Device Information Service]
//footnote[uuid][Bluetooth SIGが策定したキャラクタリスティックは、（通信量を減らすために）短縮表記したUUIDを用いることができる]

前述を含めたいくつかのキャラクタリスティック、サービス、プロファイルをBluetooth SIGが策定しています@<fn>{sig1}@<fn>{sig2}。
これによりBLEデバイス間の互換性を高めています。

//footnote[sig1][サービスおよびプロファイル: @<href>{https://www.bluetooth.com/ja-jp/specifications/gatt/}]
//footnote[sig2][キャラクタリスティック: @<href>{https://www.bluetooth.com/ja-jp/specifications/gatt/characteristics/}]

== GoでBLE

それではGoでBLEに触れてみましょう。

=== GoのBLEライブラリ

Goで書かれたオープンソースのBLEライブラリがいくつかあります。
本稿ではGobotを使用しますが、検討したものを示します。

#@# textlint-disable
==== paypal/gatt
#@# textlint-enable
BLEのPure goによる実装です。
LinuxとmacOSに対応しています。
ググるとブログ記事がそこそこヒットしますが、
ライブラリは現在メンテナンスされていないようです。
Linuxでは正常に動作します。macOSでは、APIの変更に追いついていないため、使うことが困難です。

 * Star: 800+
 * Last commit: October 2015
 * License: BSD-3-Clause
 * Repository: @<href>{https://github.com/paypal/gatt}

==== currantlabs/ble
こちらもBLEのPure goによる実装です。
LinuxとmacOSに対応しています。
こちらもLinuxでは正常に動作します。macOSでも動くはずですが、私の環境ではデバイスの探索がうまくできませんでした。

 * Star: 150+
 * Last commit: December 2017
 * License: BSD-3-Clause
 * Repository: @<href>{https://github.com/currantlabs/ble}

==== muka/go-bluetooth
BlueZ@<fn>{bluez}とD-Busでやりとりするためのライブラリです。
BlueZを前提としているため、今回は検証しませんでした。

//footnote[bluez][Linuxカーネルに採用されているBluetoothプロトコルスタック @<href>{http://www.bluez.org/}]

 * Star: 250+
 * Last commit: September 2019
 * License: Apache-2.0
 * Repository: @<href>{https://github.com/muka/go-bluetooth}

==== Gobot
IoTやロボティクスのためのフレームワークです。
@<code>{gobot.io/x/gobot/platforms/ble}は@<code>{currantlabs/ble}をフォークしたライブラリ@<fn>{goble}を使用しています。
公式サイト@<fn>{gobot}のドキュメントが豊富です。本稿のソースコードはGobotを使用します。

//footnote[goble][@<href>{https://github.com/go-ble/ble}]
//footnote[gobot][@<href>{https://gobot.io/}]

 * Star: 5800+
 * Last commit: May 2019
 * License: Apache-2.0
 * Repository: @<href>{https://github.com/hybridgroup/gobot}

=== Gobotを使う

====[column] 動作確認に用いた環境

本稿の動作確認はすべてRaspberri PiとBLE対応の心拍数センサで行いました。
詳細を示します。

 * コンピュータ: Raspberry Pi 3 Model B+
 * OS: Ubuntu 18.04.2 LTS
 * Go: Go 1.13
 * BLEデバイス: Echowell BH20（HRPとBASに対応した心拍数センサ）

====[/column]

まずはGobotをインストールしましょう（@<list>{gobot1}）。

//list[gobot1][Gobotのインストール][bash]{
$ go get -d -u gobot.io/x/gobot/...
//}

ディレクトリを眺めてみると、次のことがわかります。

 * @<code>{gobot.io/x/gobot/examples/}にサンプルコードがある
 * @<code>{gobot.io/x/gobot/platforms/ble/}にBLE関連のコードがある

では、サンプルコードで動作を確認しましょう。
DISのキャラクタリスティックにアクセスしてデバイス情報を取得する
@<code>{examples/ble_device_info.go}を実行します（@<list>{gobot2}）。

サンプルコードでは、引数にフレンドリ名@<fn>{fname}またはBDアドレス@<fn>{bda}を与えることでデバイスの特定を行います。

//footnote[fname][スマートフォンやPCのBluetooth接続画面に表示される名前]
//footnote[bda][Bluetoothデバイスを識別するためのIEEE 802に準拠したアドレス（MACアドレスと同じ構造）]

//list[gobot2][サンプルコードの実行][bash]{
$ cd ${GOPATH}/src/gobot.io/x/gobot
$ go build examples/ble_device_info.go
$ sudo ./ble_device_info フレンドリ名またはBDアドレス
//}

@<list>{gobot3}のようなログが現れるでしょう。
なお、私の環境では@<code>{examples/ble_device_info.go}の33行目にある
@<code>{info.GetPnPId()}でパニックが発生した@<fn>{pnpid}ので、33行目をコメントアウトした状態で実行しています。

//footnote[pnpid][使用したBLEデバイスがPnP ID（0x2A50）キャラクタリスティックに対応していないため]

ログの@<code>{Model number}や@<code>{Firmware rev}は、
それぞれModel Number String（0x2A24）キャラクタリスティックとFirmware Revision String（0x2A26）キャラクタリスティックから取得したデータです。
GATTのキャラクタリスティックはこのようにデータの仕様を細かく定義しています。

//list[gobot3][サンプルコードの実行結果][plain]{
2019/09/18 23:32:58 Initializing connections...
2019/09/18 23:32:58 Initializing connection BLEClient-72B41CA7 ...
2019/09/18 23:32:58 Initializing devices...
2019/09/18 23:32:58 Initializing device DeviceInformation-3E0CE82B ...
2019/09/18 23:32:58 Robot bleBot initialized.
2019/09/18 23:32:58 Starting Robot bleBot ...
2019/09/18 23:32:58 Starting connections...
2019/09/18 23:32:58 Starting connection BLEClient-72B41CA7...
2019/09/18 23:33:53 Starting devices...
2019/09/18 23:33:53 Starting device DeviceInformation-3E0CE82B...
2019/09/18 23:33:53 Starting work...
Model number: BH20
Firmware rev: QLN10
Hardware rev: 01_00_0000
Manufacturer name: Echowell
//}

同様に、@<code>{examples/ble_battery.go}を実行することでBASからバッテリー残量を取得できます。

