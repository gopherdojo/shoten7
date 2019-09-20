#@# author: ebiiim <mail@ebiiim.com>

= GoでBluetooth Low Energy

GoでBluetooth Low Energy（以降BLEとします）を触ってみたので、必要な知識をまとめました。
BLEとGATTの概要を確認した後、
Gobotフレームワークを用いて心拍数センサから
取得したデータを表示するプログラムを作成することで
GoでBLEを扱う方法の一例を確認します。
Raspberry Piなど、GoとBluetoothの両方が使用できる端末があればいろいろ遊べそうです。

== Bluetooth Low Energy

BLEとは、
Bluetooth 4.0以降の機能で、少ない消費電力で動作する通信モードです。
Bluetooth Classic（従来のBluetooth）との互換性はありませんが、
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

=== Generic Attribute Profile

GATTは、次に示す概念により、クライアントによるデータの読み書きとサーバによる通知のためのデータモデルを定義します。

==== キャラクタリスティック（Characteristic）

UUIDで識別される@<fn>{uuid}、データをやりとりするための仕様です。
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
//footnote[uuid][Bluetooth SIGが策定したキャラクタリスティックは、（通信量の削減を目的とした）短縮表記したUUIDを用いることができる]

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

BLEのpure goによる実装です。
LinuxとmacOSに対応しています。
ググるとブログ記事がそこそこヒットしますが、
ライブラリは現在メンテナンスされていないようです。
Linuxでは正常に動作します。macOSでは、APIの変更に追いついていないため、使うことが困難です。

 * Star: 800+
 * Last commit: October 2015
 * License: BSD-3-Clause
 * Repository: @<href>{https://github.com/paypal/gatt}

==== currantlabs/ble
こちらもBLEのpure goによる実装です。
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
DISのキャラクタリスティックからデバイス情報を取得する
@<code>{examples/ble_device_info.go}を実行します（@<list>{gobot2}）。

サンプルコードでは、引数にフレンドリ名@<fn>{fname}またはBDアドレス@<fn>{bda}を与えることでデバイスの特定を行います。
実行する前に、BLEテストツール@<fn>{lb}などで使用したいBLEデバイスの情報を確認しておくことを推奨します。

//footnote[fname][スマートフォンやPCのBluetooth接続画面に表示される名前]
//footnote[bda][Bluetoothデバイスを識別するためのIEEE 802に準拠したアドレス（MACアドレスと同じ構造）]
//footnote[lb][iOSまたはAndroidで動作するLightBlue Explorerが便利]

//list[gobot2][サンプルコードの実行][bash]{
$ cd ${GOPATH}/src/gobot.io/x/gobot
$ go build examples/ble_device_info.go
$ sudo ./ble_device_info "フレンドリ名またはBDアドレス"
//}

@<list>{gobot3}のような出力が得られるでしょう。
なお、私の環境では@<code>{examples/ble_device_info.go}の33行目にある
@<code>{info.GetPnPId}メソッドでパニックが発生した@<fn>{pnpid}ので、33行目をコメントアウトした状態で実行しています。

//footnote[pnpid][使用したBLEデバイスがPnP ID（0x2A50）キャラクタリスティックに対応していないため]

ログの@<code>{Model number}や@<code>{Firmware rev}は、
それぞれModel Number String（0x2A24）キャラクタリスティックとFirmware Revision String（0x2A26）キャラクタリスティックから取得したデータです。
GATTのキャラクタリスティックはこのようにデータの仕様を細かく定義しています。

//list[gobot3][サンプルコードの実行結果][plain]{
$ sudo ./ble_device_info "Echowell BH 123456"
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

== 心拍数モニタを作る

サンプルコードの動作を確認したので、次は実際のコードを書きます。
心拍数センサから心拍数を取得するために、HRSを一部実装します。

=== GATTクライアント

@<code>{examples/ble_device_info.go}と@<code>{examples/ble_battery.go}を参考にGATTクライアントを実装します（@<list>{client}）。
これは、BASとHRSが使用できることを想定したプログラムです。
バッテリー残量の取得（l.26）を実施した後、センサ位置の取得（l.29）と心拍数通知の受け付け開始（l.32）を行います。

//listnum[client][examples/ble_heart_rate.go][go]{
package main

import (
	"fmt"
	"os"

	"gobot.io/x/gobot"
	"gobot.io/x/gobot/platforms/ble"
)

func main() {
	bleAdaptor := ble.NewClientAdaptor(os.Args[1])
	info := ble.NewDeviceInformationDriver(bleAdaptor)
	battery := ble.NewBatteryDriver(bleAdaptor)
	heartRate := ble.NewHeartRateDriver(bleAdaptor)

	work := func() {
		// info
		fmt.Println("=== Device Information ===")
		fmt.Println("Model number:", info.GetModelNumber())
		fmt.Println("Firmware rev:", info.GetFirmwareRevision())
		fmt.Println("Hardware rev:", info.GetHardwareRevision())
		fmt.Println("Manufacturer name:", info.GetManufacturerName())
		// battery
		fmt.Println("=== Battery Level ===")
		fmt.Println("Battery level:", battery.GetBatteryLevel())
		// heartRate
		fmt.Println("=== Body Sensor Location ===")
		loc, _ := heartRate.GetBodySensorLocation()
		fmt.Println("Body sensor location:", loc)
		fmt.Println("=== Heart Rate ===")
		heartRate.SubscribeHeartRate()
	}

	robot := gobot.NewRobot("bleBot",
		[]gobot.Connection{bleAdaptor},
		[]gobot.Device{battery, heartRate},
		work,
	)

	robot.Start()
}
//}

@<code>{examples/ble_battery.go}で確認したように、BASドライバはすでに用意されています。
そのため、このコードを実行するためには、次の項目を実装したHRSドライバを用意する必要があります。

 1. @<code>{HeartRateDriver}型
 2. @<code>{NewHeartRateDriver}関数
 3. @<code>{HeartRateDriver.GetBodySensorLocation}メソッド
 4. @<code>{HeartRateDriver.SubscribeHeartRate}メソッド

1と2は既存のコードと同じように実装するだけですが、
3と4はGATTの仕様を読みながら実装していく必要がありそうです。

=== HRSドライバ

@<code>{platforms/ble/ble_device_info.go}のDISドライバと
@<code>{platforms/ble/ble_battery.go}のBASドライバを参考に
HRSドライバのひな型を実装します（@<list>{hrs1}）。

//listnum[hrs1][platforms/ble/heart_rate_driver.go（ひな型）][go]{
package ble

import (
	"encoding/binary"
	"fmt"
	"os"
	"time"

	"gobot.io/x/gobot"
)

type HeartRateDriver struct {
	name       string
	connection gobot.Connection
	gobot.Eventer
}

func NewHeartRateDriver(a BLEConnector) *HeartRateDriver {
	n := &HeartRateDriver{
		name:       gobot.DefaultName("Heart Rate"),
		connection: a,
		Eventer:    gobot.NewEventer(),
	}
	return n
}

func (b *HeartRateDriver) Name() string { return b.name }

func (b *HeartRateDriver) SetName(n string) { b.name = n }

func (b *HeartRateDriver) Connection() gobot.Connection {
    return b.connection
}

func (b *HeartRateDriver) Start() (err error) { return }

func (b *HeartRateDriver) Halt() (err error) { return }

func (b *HeartRateDriver) adaptor() BLEConnector {
	return b.Connection().(BLEConnector)
}

// TODO: HeartRateDriver.GetBodySensorLocation()
// TODO: HeartRateDriver.SubscribeHeartRate()
//}

これで、残りは@<code>{HeartRateDriver.GetBodySensorLocation}メソッド
と@<code>{HeartRateDriver.SubscribeHeartRate}メソッドです。
それらは、それぞれHRSで実装することが求められているキャラクタリスティックの
Body Sensor Location（0x2A38）とHeart Rate Measurement（0x2A37）を使用することを想定します。
HRSは、消費カロリーの値をリセットするためのHeart Rate Control Point（0x2A39）の実装も必要としますが、
本稿では省略します@<fn>{hrcp}。

//footnote[hrcp][とても簡単]

=== GATTキャラクタリスティック

HRPの各キャラクタリスティックのための定数を定義します（@<list>{hrs}）。

//listnum[hrs][heart_rate_driver.go（キャラクタリスティックの定義）][go]{
// HRS(Heart Rate Service) characteristics
const (
	cUUIDHeartRateMeasurement  = "2a37"
	cUUIDBodySensorLocation    = "2a38"
	cUUIDHeartRateControlPoint = "2a39"
)
//}

=== Body Sensor Location（0x2A38）

@<code>{HeartRateDriver.GetBodySensorLocation}メソッドのために、
Bluetooth SIGの公式サイトからBody Sensor Location（0x2A38）の仕様を確認します（@<list>{xmlbsl}）。

//list[xmlbsl][org.bluetooth.characteristic.body_sensor_location.xml（抜粋）][go]{
<Field name="Body Sensor Location">
    <Requirement>Mandatory</Requirement>
    <Format>8bit</Format>
        <Enumerations>
            <Enumeration key="0" value="Other"/>
            <Enumeration key="1" value="Chest"/>
            <Enumeration key="2" value="Wrist"/>
            <Enumeration key="3" value="Finger"/>
            <Enumeration key="4" value="Hand"/>
            <Enumeration key="5" value="Ear Lobe"/>
            <Enumeration key="6" value="Foot"/>
            <ReservedForFutureUse start="7" end="255"/>
        </Enumerations>
</Field>
//}

8bitのデータが1個あり、その値がセンサの位置を示すようです。
この仕様をGoのコードに書き起こします（@<list>{bsl}）。

//listnum[bsl][heart_rate_driver.go（Body Sensor Locationの定義）][go]{
// BodySensorLocation value
var mBodySensorLocation = map[uint8]string{
	0: "Other",
	1: "Chest",
	2: "Wrist",
	3: "Finger",
	4: "Hand",
	5: "Ear Lobe",
	6: "Foot",
}
//}

キャラクタリスティックの値を読み、Body Sensor Locationの値をstringで返す
@<code>{HeartRateDriver.GetBodySensorLocation}メソッドを実装します（@<list>{getbsl}）。
値の読み出しには@<code>{BLEConnecter.ReadCharacteristic}メソッドを使用します。

//listnum[getbsl][heart_rate_driver.go（Body Sensor Locationの取得）][go]{
func (b *HeartRateDriver) GetBodySensorLocation() (string, error) {
	c, err := b.adaptor().ReadCharacteristic(cUUIDBodySensorLocation)
	if err != nil {
		return "", err
	}
	val := uint8(c[0])
	ret := mBodySensorLocation[val]
	if ret == "" {
		return "", fmt.Errorf("undefined location %v", val)
	}
	return ret, nil
}
//}

先ほど用意したクライアントのコード（@<code>{examples/ble_heart_rate.go}）を実行してみましょう@<fn>{co}。
@<list>{out1}のような出力が得られるでしょう。

//footnote[co][実装していない部分はコメントアウトすること]

//list[out1][examples/ble_heart_rate.goの実行結果（1）][plain]{
$ sudo ./ble_heart_rate "Echowell BH 123456"
2019/09/19 18:29:20 Initializing connections...
2019/09/19 18:29:20 Initializing connection BLEClient-6F2E8C08 ...
2019/09/19 18:29:20 Initializing devices...
2019/09/19 18:29:20 Initializing device Battery-141B8478 ...
2019/09/19 18:29:20 Initializing device Heart Rate-5C9C877 ...
2019/09/19 18:29:20 Robot bleBot initialized.
2019/09/19 18:29:20 Starting Robot bleBot ...
2019/09/19 18:29:20 Starting connections...
2019/09/19 18:29:20 Starting connection BLEClient-6F2E8C08...
2019/09/19 18:29:22 Starting devices...
2019/09/19 18:29:22 Starting device Battery-141B8478...
2019/09/19 18:29:22 Starting device Heart Rate-5C9C877...
2019/09/19 18:29:22 Starting work...
=== Battery Level ===
Battery level: 86
=== Body Sensor Location ===
Body sensor location: Chest
//}

今回用意したセンサは胸につけることを想定しているため、
Body Sensor Locationの値は常に1（Chest）が得られます。
これで、Body Sensor Location（0x2A38）キャラクタリスティックの実装が完了しました。

=== Heart Rate Measurement（0x2A37）

同様に、キャラクタリスティックの通知の受け付けを開始し、通知を受けたらデータを標準出力に書き出す
@<code>{HeartRateDriver.SubscribeHeartRate}メソッドを実装します（@<list>{gethr_proto}）。
通知の受け付けには@<code>{BLEConnecter.Subscribe}メソッドを使用します。

//listnum[gethr_proto][heart_rate_driver.go（心拍数計測データの取得）][go]{
func (b *HeartRateDriver) SubscribeHeartRate() error {
	err := b.adaptor().Subscribe(cUUIDHeartRateMeasurement,
		func(data []byte, _ error) {
            fmt.Println(time.Now().Format("15:04:05"), c)
		})
	return err
}
//}

クライアントのコード（@<code>{examples/ble_heart_rate.go}）を実行すると、
@<list>{out2}のような出力が得られるでしょう。

//list[out2][examples/ble_heart_rate.goの実行結果（2）（抜粋）][go]{
=== Heart Rate ===
18:25:39 [22 74 134 127 131 108]
18:25:40 [22 71 56 131 124 111]
18:25:41 [22 69 221 134 117 114]
//}

#@# textlint-disable

データを解析していないのでただのバイト列ですが、
心拍にあわせて出力されているので、あと一歩ですね。

#@# textlint-enable

それでは、先ほどと同様にBluetooth SIGの公式サイトからHeart Rate Measurement（0x2A37）の仕様を確認します。
長いので、概要のみ掲載します。なお、データのバイトオーダはリトルエンディアンと決められています。

 * 最初の1バイトは各種フラグを示した値
 ** 最下位bitは心拍数のデータサイズを示す
 ** 下位2bit目と下位3bit目はセンサのステータスを示す
 ** 下位4bit目は消費カロリーデータの有無を示す
 ** 下位5bit目はRR-Intervalデータの有無を示す
 * 各種フラグに続いて心拍数の値
 ** 長さはフラグで指定される
 ** 1バイトまたは2バイトの符号なし整数
 * 心拍数に続いて消費カロリーの値
 ** 消費カロリーのフラグが1の場合のみ
 * 消費カロリーに続いてRR-Intervalの値
 ** RR-Intervalのフラグが1の場合のみ

まず、各種フラグをGoのコードに書き起こします@<fn>{spec}（@<list>{hrmflags}@<fn>{binlit}）。

//footnote[spec][XMLからコードを生成できたら...]
//footnote[binlit][接頭辞@<code>{0b}または@<code>{0B}から始まる2進数リテラルはGo 1.13で追加された]

//listnum[hrmflags][heart_rate_driver.go（Heart Rate Measurementフラグの定義）][go]{
// HeartRateMeasurement flags
var mHeartRateFormat = map[uint8]string{
	0b0: "UINT8",
	0b1: "UINT16",
}
var mSensorContactStatus = map[uint8]string{
	0b00: "not supported",
	0b01: "not supported",
	0b10: "supported but contact is not detected",
	0b11: "supported and contact is detected",
}
var mEnergyExpandedStatus = map[uint8]string{
	0b0: "not present",
	0b1: "present",
}
var mRRInterval = map[uint8]string{
	0b0: "not present",
	0b1: "present (one or more)",
}
//}

続いて、1バイトのデータから各種フラグを取り出す関数を実装します（@<list>{hrmparse}）。
バイトから特定ビットの値を取り出すにはシフト演算とAND演算を用います（@<code>{parseHeartRateFlags}関数）。

//list[hrmparse][heart_rate_driver.go（Heart Rate Measurementフラグの解析）][go]{
type HeartRateFlags struct {
	heartRateFormat      uint8
	sensorContactStatus  uint8
	energyExpendedStatus uint8
	rrInterval           uint8
}

func parseHeartRateFlags(flags byte) HeartRateFlags {
	var hrf HeartRateFlags
	hrf.heartRateFormat = flags & 0b1
	hrf.sensorContactStatus = flags >> 1 & 0b11
	hrf.energyExpendedStatus = flags >> 3 & 0b1
	hrf.rrInterval = flags >> 4 & 0b1
	return hrf
}

func (hrf HeartRateFlags) String() string {
        return fmt.Sprintf("HeartRateFormat: %v\n",
                mHeartRateFormat[hrf.heartRateFormat]) +
                fmt.Sprintf("SensorContactStatus: %v\n",
                        mSensorContactStatus[hrf.sensorContactStatus]) +
                fmt.Sprintf("EnergyExpandedStatus: %v\n",
                        mEnergyExpandedStatus[hrf.energyExpendedStatus]) +
                fmt.Sprintf("RR-Interval: %v",
                        mRRInterval[hrf.rrInterval])
}
//}

そして、@<code>{HeartRateDriver.SubscribeHeartRate}メソッドにデータ解析のためのコードを追加します（@<list>{gethr}）。

//listnum[gethr][heart_rate_driver.go（心拍数の取得）][go]{
func (b *HeartRateDriver) SubscribeHeartRate() error {
	err := b.adaptor().Subscribe(cUUIDHeartRateMeasurement,
		func(data []byte, e error) {
			if e != nil {
				fmt.Fprintf(os.Stderr, "err: %v", e)
				return
			}
			hr, hrf, e := ParseHeartRate(data)
			if e != nil {
				fmt.Fprintf(os.Stderr, "err: %v", e)
				return
			}
			fmt.Println(hrf)
			fmt.Printf("HeartRate: %v\n", hr)
			fmt.Println("--------------------")
		})
	return err
}
//}

最後に、クライアントのコード（@<code>{examples/ble_heart_rate.go}）を実行します。
@<list>{out3}のように、解析されたデータが得られるでしょう。

//listnum[out3][examples/ble_heart_rate.goの実行結果（3）（抜粋）][go]{
=== Heart Rate ===
18:29:22
HeartRateFormat: UINT8
SensorContactStatus: supported and contact is detected
EnergyExpandedStatus: not present
RR-Interval: present (one or more)
HeartRate: 66
--------------------
18:29:23
HeartRateFormat: UINT8
SensorContactStatus: supported and contact is detected
EnergyExpandedStatus: not present
RR-Interval: present (one or more)
HeartRate: 67
--------------------
//}

心拍数が正しく取得できています。
これで、Heart Rate Measurement（0x2A37）キャラクタリスティックの実装が完了しました。
