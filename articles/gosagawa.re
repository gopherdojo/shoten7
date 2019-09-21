= Goのエラーハンドリングを深掘りする

== 始めに

アプリケーションを実装するにあたり、エラーハンドリングは非常に重要です。しかし、エラー管理がデフォルトでは非常にシンプルです。
カスタマイズをしなければ不便であるところは欠点でありつつもの、逆にしっかりとカスタマイズをすれば非常に強力なツールとなりえます。
この記事ではpkg/errors@<fn>{errors}ライブラリや、GopherCon 2019のセッション"Handling go errors"@<fn>{handling_go_errors}、
そしてAthensプロジェクト@<fn>{athens}でのエラーハンドリングの実例を通して、実用にみあったエラーハンドリング処理の書き方をお伝えできればと思っています。

//footnote[errors][@<href>{https://godoc.org/github.com/pkg/errors}]
//footnote[handling_go_errors][@<href>{https://www.youtube.com/watch?v=4WIhhzTTd0Y&list=PL2ntRZ1ySWBdDyspRTNBIKES1Y-P__59_&index=29&t=0s}]
//footnote[athens][@<href>{https://github.com/gomods/athens}]

== エラーハンドリングで満たしたい要件

#@# textlint-disable

Goのエラーハンドリングのよいところでもありたいへんなところでもあるのは、カスタマイズしやすいが設計する必要があるというではないでしょうか。
何もせず、そのまま@<code>{return err}を繰り返すだけだとエラーのテキスト情報のみしか受け渡すことができません。

#@# textlint-enable

では、実際の業務でエラーハンドリングに求められる要件はどのようなことがあるでしょうか。

Webアプリケーションを例に取った時、よくある要件として以下があげられます。

 * エラーを階層構造でもち、オリジナルのエラーと最後にラップしたエラー含めていずれの階層のエラーも取得できる。
 * 問題の発生箇所のファイル、関数名、行数を確認できる。
 * エラーの種類やログレベル、エラーコード、レスポンスコードなどを保持できる。
 * アプリケーション固有の情報を保持できる。

それぞれみていきましょう。

 * エラーを階層構造でもち、オリジナルのエラーと最後にラップしたエラー含めていずれの階層のエラーも取得できる。

簡単なプログラムでない限り、エラーは階層化されていくでしょう。たとえば、ログイン処理でユーザー取得の際DB接続に失敗したとすると、
DB接続エラー、ユーザー取得エラー、ログインエラーと階層化されます。こうなった時にログインエラーだけわかっても何が原因かわかりませんし、
DB接続エラーだけみてもそれがどのような影響をおよぼすのかがわからないため必要になります。

 * 問題の発生箇所のファイル、行数を確認できる。

エラーが発生したら、発生箇所のコードを追うことになるのでほしい情報です。なお、階層化された場合は階層化されたエラーそれぞれの発生箇所を追えるのが望ましいです。

 * エラーの種類やログレベル、エラーコード、レスポンスコードなどを保持できる。

#@# textlint-disable

エラーハンドリングをやる目的のひとつとして、エラーによって処理を分けたいということがあるでしょう。
エラーの種類によって特定の処理を行う、ログレベル（例:Debug, Info, Warning）を判定してログを出し分ける、レスポンスを返す際エラーコードを付与する、レスポンスコード（例:200, 404, 500）を変更するなどこれらがパラメータとして必要になるのはよくあるパターンでしょう。

#@# textlint-enable

 * アプリケーション固有の情報を保持できる。

時として、エラーは特定のシチュエーションのみでしか発生しないことがあります。たとえ、特定のユーザーのみ発生するエラーがあった場合、ユーザー情報をエラーと一緒に保持しておくとデバッグが容易になります。
エラーメッセージ内に持たせることもできるのですがエラーの可読性を落とす可能性がありますし、システムで何か処理をしたい場合は特に別に持っていることが望ましいです。
また、ログとして記録するエラーと実際にユーザーに表示されるエラーは異なるものになるでしょう。たとえば、記録したいエラーが「ユーザー（%v）:ポイント不足で購入不可」、表示させたいエラーが「ポイント不足で購入ができません。現在の保持ポイント:%v」というような形だった場合、ユーザーIDと所持ポイントを持っておくと、エラー処理時に出力が容易になります。

== 階層化できていないエラーハンドリング

業務で最初に設計したエラーハンドリングのロジックは次のようなものでした。

//list[firstcode][業務で最初に作成したエラーコード][go]{
import (
	"github.com/pkg/errors"
)

type applicationError struct {
	msg        string 
	errorCode  string
	parameters []interface{}
}

func (e *applicationError) Error() string {

	if e == nil {
		return ""
	}

	return e.msg
}

// NewApplicationError ApplicationErrorを作成する
func NewApplicationError(
    errorCode string,
    msg string,
    parameters ...interface{},
) error {

	err := &applicationError{
		msg:        msg,
		errorCode:  errorCode,
		parameters: parameters,
	}

	return errors.WithStack(err)
}

// WrapApplicationError 元のエラーをラップしたApplicationErrorを作成する
func WrapApplicationError(
    cause error,
    errorCode string, 
    msg string,
    parameters ...interface{},
) error {

	err := &applicationError{
		msg:        concatErrorMessage(cause, msg),
		errorCode:  errorCode,
		parameters: parameters,
	}

	return errors.WithStack(err)
}

func concatErrorMessage(cause error, msg string) string {

	if cause == nil {
		return msg
	}

	return msg + ": " + cause.Error()
}
//}

@<code>{pkg/errors}を利用することによって、スタックトレースは付与できるようになっています。
ただ、@<code>{pkg/errors}のように階層化したエラーのオリジナルを取得することはできません。
Wrapをした際にメッセージ以外のオリジナル情報は失われてしまいます。
アプリケーションのエラーコードがオリジナルのエラーによって決定されれば単純に@<code>{pkg/errors}と@<code>{Cause}を利用すればよいのですが、
上位の階層でエラーコードを変えるということを実現するため、このような挙動になっています。

それではエラーを階層化するためにはどうすればよいでしょうか。
それを理解するために@<code>{pkg/errors}でどのようにエラーを構造管理していくかを追っていきます。

== pkg/errorsの動作とできることを理解する

@<code>{pkg/errors}は非常にシンプルなライブラリで、ファイルも数ファイルのみでコードも読みやすいです。実際に読んでみると次の機能に集約されることがわかります。

 * エラー内容を階層化して保存する。
 * スタックトレースを取得する。

それぞれの機能は次のように実現されています。

 * エラー内容を階層化して保存する。

エラーは図に示すような構造体となって階層状に保存されていきます。

@<code>{WithMessage}は@<code>{Cause()}という@<code>{cause}を返す関数を持ちます。@<code>{Cause()}関数をもつ@<code>{causer}インタフェースを定義し、@<code>{err}が@<code>{causer}に変換できるかを再帰処理で調べていきます。
オリジナルのエラーは@<code>{Cause()}関数を持たないため、それでオリジナルのエラーを判別できます。

#@# textlint-disable

//list[Cause][階層化したエラーのオリジナルを取得する関数][go]{
func Cause (err error) error {
	type causer interface {
		Cause() error
	}

	for err != nil {
		cause, ok := err.(causer)
		if !ok {
			break
		}
		err = cause.Cause()
	}
	return err
}
//}

#@# textlint-enable

 * スタックトレースを取得する

スタックトレースは、ファイル名と関数名、行数で構成されていますが
それぞれruntimeパッケージを利用することで取得できます。
シンプルにruntimeパッケージを使って情報を取得するには次のような関数になります。

@<href>{https://play.golang.org/p/CN09KGDu6UC,https://play.golang.org/p/CN09KGDu6UC}

//list[runtime][ランタイムパッケージを使用した情報取得][go]{
package main

import (
	"fmt"
	"runtime"
)

func main() {

    const depth = 32
    var pcs [depth]uintptr
    n := runtime.Callers（0, pcs[:]）

    var st []uintptr = pcs[0:n]
    for _, f := range st {
        fn := runtime.FuncForPC（f）
        file, LINE := fn.FileLine（f）
        fmt.Println（fn.Name（）,file, LINE）
    }
}
//}

これを動かすと次のようになります。

//list[runtimeresult][実行結果]{
runtime.Callers /usr/local/go/src/runtime/extern.go 211
main.main /tmp/sandbox099384991/prog.go 12
runtime.main /usr/local/go/src/runtime/proc.go 212
runtime.goexit /usr/local/go/src/runtime/asm_amd64p32.s 523
//}

しっかり@<code>{runtime.Caller}を呼んだ場所と、@<code>{runtime.Caller}を呼んでいる関数が取得できます。
@<code>{pkg/errors}でも同様にruntimeパッケージを利用して、情報を取得し整形してスタックトレースとしています。

以上が@<code>{pkg/errors}の挙動となりますが、これを踏まえると階層化しているデータはメッセージのみなのでそれ以外の情報を持たせるようにし、
スタックトレースを取る機能は@<code>{pkg/error}を使わずにruntimeパッケージをそのまま使うことにより実現ができそうです。

== xerrors、Go 1.13以降のerrorsについて
@<code>{xerrors}@<fn>{xerrors}については@<code>{pkg/errors}を使ったやり方から公式でも検討された経緯もあり、スタックトレースを取得したりエラーをラップしたりできます。
AsやIs、UnWrapなどの便利な関数も利用できるようになり公式な@<code>{pkg/errors}の上記互換となります。
実際にGo 1.13では@<code>{xerrors}が@<code>{errors}として採用され、スタックトレース表示の機能を除きエラーのラップやIsメソッド、Asメソッドが利用できるようになっています。

//footnote[xerrors][@<href>{https://godoc.org/golang.org/x/xerrors}]

xerrorsについては、次の解説が詳しいです。
@<href>{https://qiita.com/sonatard/items/9c9faf79ac03c20f4ae1,Goの新しいerrors パッケージ xerrors}

なお@<code>{pkg/errors}ではNewもしくはWrap関数で必ずスタックトレースが付与されていたものの、
xerrorsは明示的にWrapして正しく構文を記述しないとスタックフレームを積んでくれないため注意が必要です。

なお、構文ミスによる記述漏れを防ぐやり方は次の記事が参考になります。
@<href>{https://qiita.com/sachaos/items/f6b3e66931d7f73dd68d,Go で静的解析して Go 1.13 から標準の xerrors とうまく付き合っていく}

== error handling in goで述べられていた解決策

GopherCon 2019では、pkg/errorsやxerrors使わずにエラーを階層化して管理する手法が述べられていました。
ここからは実際のサンプルを@<href>{https://github.com/gosagawa/go-error-handling-sample/}にも載せていますので合わせて確認ください。

エラー用の構造体とその処理を定義すると次のようになります。

//list[error][エラー構造体][go]{
package errors

import "errors"

// Op オペレーション
type Op string

// Kind エラー種別
type Kind string

// Error エラー構造体
type Error struct {
	Op   Op    // operation
	Kind Kind  // category of errors
	Err  error // the wrapped error
}

func （e *Error） Error() string {
	return e.Err.Error()
}

// E エラーをラップする
func E（args ...interface{}） error {
	e := &Error{}
	for _, arg := range args {
		switch arg := arg.（type） {
		case Op:
			e.Op = arg
		case error:
			e.Err = arg
		case string:
			e.Err = errors.New（arg）
		case Kind:
			e.Kind = arg
		default:
			panic（"bad call to E"）
		}
	}
	return e
}

#@# textlint-disable
// Ops スタックトレース取得
func Ops（e *Error） []Op {
	res := []Op{e.Op}
	subErr, ok := e.Err.（*Error）
	if !ok {
		return res
	}
	res = append（res, Ops（subErr）...）
	return res
}
//}
#@# textlint-enable

実際に利用すると次のようになります。

//emlist[][]{
// Validate ユーザをバリデート
func Validate(ID int) error {
	const op errors.Op = "login.Validate"
	user, err := db.LookUpUser(ID)
	if err != nil {
		return errors.E(op, err)
	}
	if !user.IsValid() {
		return errors.E(op, "User Invalid")
	}
	return nil
}
//}

特徴的なのはスタックトレースとしてruntimeから出力されたものを使うのではなく、エラー処理時に設定するという点です。これによりスタックトレースを簡略化することに成功しています。
Err構造体には項目を増やすことができ、また内部にerrorインタフェースを内包しているので、階層化されたエラーを失わないまま保持できるところもpkg/errorsと異なる部分です。

ただ、個人的な感想としてはopを毎回指定しなければ行けないのが冗長ではと思っています。
runtimeで呼び出し元の関数名や行数は取得できるのでopを毎回定義するのではなく、runtimeを利用するという方法でもよいのではないでしょうか。

== Athensのエラーハンドリング

GopherCon 2019で発表されたAthensというプロジェクト@<fn>{athens}でもerror handling in goで述べられている方法と同様のやり方が取られています。@<fn>{athens_errorhandling}
それもそのはずで、Athensでのエラーハンドリング部分はerror handling in goの発表者と同じ方によって書かれています。
error handling in goで発表された内容の詳細な実例として見ることができるでしょう。

//emlist[エラー構造体（/pkg/errors/errors.go）][go]{
// Error is an Athens system error.
// It carries information and behavior
// as to what caused this error so that
// callers can implement logic around it.
type Error struct {
	// Kind categories Athens errors into a smaller
	// subset of errors. This way we can generalize
	// what an error really is: such as "not found",
	// "bad request", etc. The official categories
	// are HTTP status code but the ones we use are
	// imported into this package.
	Kind     int
	Op       Op
	Module   M
	Version  V
	Err      error
	Severity logrus.Level
}
//}

//footnote[athens][@<href>{https://github.com/gomods/athens}]
//footnote[athens_errorhandling][@<href>{https://github.com/gomods/athens/blob/master/pkg/errors/errors.go}]

== まとめ

エラーハンドリングを設計する場合、必要な要件を整理するのが最初に必要となります。
@<code>{xerrors}もしくは@<code>{errors}を利用して要件を満たせるのであれば、標準のものをシンプルに利用していくという点でよいやり方となるでしょう。
すでにpkg/errorsを利用している場合は@<code>{xerrors}や@<code>{errors}に乗換えるのもよい選択肢でしょう。
上記で満たせない要件、たとえスタックトレースを見やすくしたりカスタマイズをしたりしたいのであれば階層化されたエラーを独自実装していくのがよいではないでしょうか。

== 参考

[1] @<href>{https://qiita.com/sonatard/items/9c9faf79ac03c20f4ae1, Goの新しいerrors パッケージ xerrors}

[2] @<href>{https://qiita.com/sachaos/items/f6b3e66931d7f73dd68d, Go で静的解析して Go 1.13 から標準の xerrors とうまく付き合っていく}

[3] @<href>{https://www.youtube.com/watch?v=4WIhhzTTd0Y&list=PL2ntRZ1ySWBdDyspRTNBIKES1Y-P__59_&index=29&t=0s, GopherCon 2019: Marwan Sulaiman - Handling Go Errors}

[4] @<href>{https://about.sourcegraph.com/go/gophercon-2019-handling-go-errors,GopherCon 2019 - Handling Go errors}

