= 逆引きGoテスト

== たかがテスト、されどテスト

自分はソフトウェアエンジニア、ないしgolangを初めて約1年になります。プログラミングとしては多少触れていたのですが、
プロダクションレベルの品質は何かということを考えるにあたってテストコードがいかに大事かということが分かってきました。

テストコードそのものは機能開発などと異なり、事業価値に直接貢献するものではありません。
しかしながら、継続的にサービスを改善して拡大していく過程においてテストは極めて大きな影響をもたらします。

golangはテストが非常に書きやすい(個人談)ですが、実際には色々なテストのパターンが存在したり、
そもそもテストをどうやって書けばいいのかというのはずっと悩みでした。本章ではこのあたりのテストの書き方についてユースケースに基づいて書き方を洗っていきます。
実際の業務や趣味でgolangを書く際の一助に慣れればと思います。

※ 本記事ではテストの書き方のバリエーションにフォーカスしたいので、一部エラーハンドリングすべき部分を行っていない箇所があります。ご了承ください。

==  テストコードの思想

=== そもそもなぜテストが必要か
テストコードが大事な理由は古今東西色々なところで言われています。が、あえて自分で咀嚼してみると、大きく下記の2点かと思います。
* テスト工程で出力すべきバグを早期に発見し、工程上の手戻りを減らすため
* コードを改善・書き換える際にそのコードが元のコードに対して良いか悪いかを判断するため

自分の個人プロジェクトレベルならまだ良いですが、いろいろな人が数多く携わり、肥大化していったコードに対して、
特定の場所のコードを書き換える場合の影響範囲を完全に特定することは至難の業です。

テストコードを書いておけば、コードの最低限の品質が担保できますし、良いテストコードを書くことができれば
プロジェクトの中でも同じようなテストコードの書き方をしていくことで全体的なレベルアップにも繋がります。

=== テストケースが満たすべき内容とは
テストをどの程度書くべきかという話は色々な人が触れているので、ここではそこまで言及はしません。
ただ、自分がよく業務でテストを書く際は以下を意識しています
* 必要十分な最小のケースを記載する
* テストする関数が影響する範囲だけテストする
* 可読性を意識する。Goの場合は特にケースごとのI/Oとロジックを分離するように書く
* 変にテストの変数を使い回さない

テストの目的はコードの改善速度や不具合検出率を上げるため、コードの良し悪しを判断するためというのが前項で触れた部分です。
そのため、過度にテストのケースを書くことや、テストそのものが長くなりすぎて何をしているか分からないコードは作ったところでそのテストそのものがメンテナンス性に欠けてしまいます。

もちろん、リリース最優先の場合などはテストを後回しにする場合もあります。
ただ、後でテストコードを書くといっても、そのタイミングが後回しにされて結局立ち消えになるケースが多いです。
うまく動いているように見えるコードのテストコード＜＜次のエンハンス、となりがちです。

もしテストが後回しになる場合は、予めスケジュールに組み込む、テストを書く工数を捻出する予定を直近のプロジェクト後に検討しておく、など
仕組みとしてカバーしておくことをオススメします。

=== テストの網羅性についての調査
テストがどれくらい網羅性があるかをチェックする場合はテストカバレッジを調査すれば良いでしょう。
goの場合はこのカバレッジを簡単に調べることができます。

//source[console]{
$ go test -cover sample(パッケージ名)
//}

テストカバレッジの分析も行うことができます。
//source[console]{
$ go test -coverprofile=cover.out sample(パッケージ名)
//}

また、以下のようにhtml形式で出力することもできます。
//source[console]{
$ go tool cover -html=profile
//}

== 通常のテスト

=== 基本的な書き方
まずは基本的なあテストの書き方です。"hello!"とだけ返す関数のテストを考えてみます。

//source[hello.go]{
func Hello()string{
    return "hello!"
}
//}

テストのファイルはテストされるファイルに_testをつけたものが良いです。
また、テスト関数は頭にtestの文字を付けておく事が通例です。

//source[hello_test.go]{
import (
    "testing"
)

func testHello(t *testing.T) {
    t.Helper() 
    actual := Hello()
    expected = "hello!"

    if actual != expected {
        t.Errorf("result does not match. actual: %v, expected: %v", actual, expected)
    }
}
//}

このテストコードでは、期待されているあたいかどうかをチェックして異なる場合はエラーを出力するようにしています。

go1.9からt.Helper()が追加されるようになりました。t.Helper()を一行書いておくとどの行でテストが落ちたかわかるようになります。
go1.9以降を利用している場合は入れておいて損することは基本ないので、テストには必ず足しておきましょう。

=== 複数のケースを扱う場合
続いて、複数のケースのテストを行う場合を見てみましょう。
例としてステップ関数を取り上げてみます(0より大きいと1、0より小さいと-1、0のときは0を出力する関数)

//source[step.go]{
func Step(n int) int {
    if n > 0 {
        return 1
    } else if n < 0 {
        return -1
    } 
    // n == 0
    return 0
}
//}

複数テストケースがある場合に、同じロジックを繰り返し書くのは冗長です。
この場合、入力を構造体として渡すテーブル駆動テストを利用しましょう。

//source[step_test.go]{
import (
    "testing"
)

func testStep(t *testing.T) {
    t.Helper() 

    // ### Given ###
    cases := []struct{
        name string
        n int
        expected int
    }{
        {
            name: "入力が正",
            n: 10,
            expected: 1,
        },
        {
            name: "入力が負",
            n: -10,
            expected: -1,
        },
        {
            name: "入力が0",
            n: 0,
            expected: 0,
        },
    }
    for _, c := range cases {
        t.Run(c.name, func(t * testing.T){
            // ### When ###
            actual := Step(c.n)

            // ### Then ###
            if actual != c.expected {
                t.Errorf("result does not match. actual: %v, expected: %v", actual, c.expected)
            }
        })
    }
}
//}

このテスト方式を利用することで、入力とそれに対する期待値の部分とテストのロジック部分が分離されるためかなり見通しが良くなります。

=== エラーの種類を確認したい場合 

エラーハンドリングについては色々なパターンが考えられます。
一般的にやりがちなのがエラー文で比較することですが、これを行った場合、エラー文が変わった場合にテストが落ちてしまい頑強性がなくなるため避けたほうが無難です。  
例えば、外部ライブラリを利用してエラーハンドリングをエラー文で判定していた場合、ライブラリアップデートでエラー文が微妙に変わったらそれだけでテストが失敗してしまいます。

そこで、ハンドリングが必要なエラーには型を持たせ、それを判定することで必要なエラーハンドリングを行います。

//source[tmp.go]{
type temporary interface {
	Temporary() bool
}

func IsTemporary(err error) bool {
	te, ok := err.(temporary)
	return ok && te.Temporary()
}

type TempError struct {
	Code int
}

func (t TempError) Temporary() bool { return true }

func (t TempError) Error() string {
	return "this is temporary error."
}

func newTempError(code int) error {
	t := TempError{Code: code}
	return t
}

func main() {
	err := newTempError(100)
	fmt.Printf("%+v\n", err)
	
	if v, ok := err.(TempError); ok {
		fmt.Printf("%v", v.Code)
	}
}
//}

上記の例だと、Temporary Errorを独自定義して、そのコード値(今回の場合は100)を持たせ、最後に型アサーションを行って独自エラーのコードを抜いています。
こうした独自エラー型は実際はDBのエラーハンドリングやhttpリクエストのエラーハンドリングの際に利用します。

例えば、DB周りのコード値のチェックを行いたい場合などは下記のようにコード値判定をすることで、実際にそのコード値が出ているかどうかなどのテストが出来ます。
//source[data_test.go]{

type Data struct{
    ID int
    Data string
}

func testData(t *testing.T) {
    t.Helper() 
    data := Data{
        ID: 1,
        Data: "hello",
    }
    err := InsertPosgre(data) // Insertするための関数
    if v, ok := err.(*pq.Error); ok {
		if v.Code != expected {
            t.Errorf("result does not match. actual: %v, expected: %v", v.Code, expected)
        }
	}
}
//}


=== 構造体の比較 
構造体を比較する場合は、普通に比較してもよいのですが、構造体が複雑な場合はどこが原因でテストが落ちているのか不透明になりがちです。
go-cmpを使うと、どこで構造体が間違っているのかが分かり非常に便利です。

//source[person.go]{
type Person struct {
    Name string
    Age int
}

// 年齢を1つ進める
func(p *Person) incrementAge() {
    p.Age++
}

func NewPerson(name string, age int) Person {
    return Person{
        Name: name,
        Age: age,
    }
}
//}

//source[person_test.go]{
import (
    "testing"
    "github.com/google/go-cmp/cmp"
)

func testPerson(t *testing.T) {
    t.Helper() 
    actual := NewPerson("Taro", 22)
    actual.incrementAge()

    expected = Person{
        Name: "Taro",
        Age: 23,
    }

    if diff := cmp.Diff(actual, expected); diff != ""{
        t.Errorf("diff: %s", diff)
    }

}
//}

ちなみにreflect.DeepEqualを使う方法もあるのですが、time.Time型が入った比較などをする場合にはこれだと駄目なので
(Monotonic clockか何かで厳密な秒数が一致してもテストが落ちることがあるみたいです)、go-cmpを使った方がベターです。

=== テスト時にconfigを書き換える
CI環境やローカル環境で読み込む環境変数が変わる場合に切り替えが面倒な場合があります。
そのようなケースではconfigを直接書き換えるという方法もあります。

//source[make_url].go]{
func makeURL()string{
    cfg := config.GetConfig() // configで環境変数を呼び出す
    domain := cfg.Server.Domain
    return path.Join(domain, "./show")
}
//}

//source[make_url_test.go]{
import (
    "testing"
)

func testURL(t *testing.T) {
    t.Helper() 
    
    cfg := config.GetConfig() 
    orig := cfg.Server.Domain
    defer func(){cfg.Server.Domain = orig}()

    domain := "http://example.com"
    cfg.Server.Domain = domain

    actual := makeURL()
    expected = "http://example.com/show"

    if actual != expected {
        t.Errorf("result does not match. actual: %v, expected: %v", actual, expected)
    }
}
//}


== I/Oが関係するテスト

=== テストが行いやすい書き方

前項における「テストする関数が影響する範囲だけテストする」という部分だけもう少し補足します。
これは、関数内で別の関数を呼び出す場合や、DB/APIとの通信などの外部とのやり取りを行う場合にテスト範囲がその関数だけに收まるようにするという考え方です。

例えば、下記のようなケースでテストを行う場合を考えてみましょう
//source[transform.go]{
func Transform(i int) (string, error){
    d, err := GetData(i)
    if err != nil {
        return "", err
    }

    ret, err := Convert(d)
    if err != nil {
        return "", err
    }

    return ret, nil
}
//}

何かDBから値を取得して、加工するメソッドです。
この場合、これをそのままテストしようとすると、入力iを色々切り替えて、期待している値やエラーが返ってくるかをチェックすることになります。
しかし、もしGetDataに問題があった場合、GetDataでエラーは出ているのに返り値はTransformからとなります。  
つまり、Transformでエラーが出たことは分かるが、本当にGetDataが悪いのか、Transformのどこかで書き方をミスっているのかが判断しづらくなります。

そのため、GetDataやConvertといった関数は本来的にはTransform関数と切り離して考えるべきです。
つまり、GetDataについてテスト→Convertについてテスト→GetDataとConvertが期待値(正常系・異常系)を返してくれる前提でTransform関数をテスト
という手順を踏むことで、問題が生じた場合も影響範囲の切り分けが容易になります。

では、どのようにこうした関数の切り分けを行えばよいでしょうか。

=== モックによるテスト TODO
前述した通り、もし関数が別の関数に依存している場合、頭から入力値を入れるテストのみにしてしまうと、テスト結果が依存する関数に引きずられてしまうため、
テストが失敗した場合の問題の切り分けなどが難しくなりあまり好ましくありません。
そこで、依存する関数については期待される動作をするモックを作ることで、テストをしたい関数の範囲だけテストをすることが出来ます。

golangの場合は、ある関数をモックするためにgo-mockというライブラリがあります。
このライブラリを利用することで、必要なインターフェースを満たすmockを下記のコマンドで自動生成することができます。

例えば、前述したTransfrom関数を下記のように書き換えましょう
//source[transform.go]{
type Data struct {
    ID int
    x int
    y int
    z int
}

type Out struct {
    distance_xy int
    distance_yz int
    distance_xz int
}

type DataRepositoryInterface interface {
    GetData(i int) (Data, error)
    Convert(d Data) (Out, error)
}

type dataRepository struct {}

func(d dataRepository) GetData(i int) (Data, error){
    ...
}

func(d dataRepository) Convert(i int) (d Data) (Out, error){
    ...
}

type dataService struct {
    dataRepo DataRepositoryInterface
}

func newDataService() dataService {
    return dataService{
		dataRepo:    &dataRepository{},
	}

}

func(s dataService) Transform(i int) (string, error){
    
    d, err := s.dataRepo.GetData(i)
    if err != nil {
        return "", err
    }

    ret, err := s.dataRepo.Convert(d)
    if err != nil {
        return "", err
    }

    return ret, nil
}
//}

やたら長くなりましたが、見るべきはTransformがdataServiceに紐付いていること、GetData並びにConvertがDataRepositoryに
紐付いていること、そしてdataServiceを呼ぶ際にDataRepositoryInterfaceというインターフェースをstructに与えているところになります。

このように、インターフェース型を活用することで、例えばdataRepoの部分を、本当のdataRepositoryではなく、
期待値を返してくれるモックに差し替えることで、関数ごとの責任範囲を分解してテストを行うことができるようになります。

このモック、go-mockというライブラリを利用することで下記のようにCLIでモックを自動生成することができるようになります。
//source[console]{
$ mkdir mock
$ mockgen -source transform.go DataRepositoryInterface >> mock/transform.go
//}

//source[transform.go]{
import (
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/(...)/mock_transform" // 各自のパス
)

func testTransform(t *testing.T) {
    t.Helper() 
    
    mockCtrl := gomock.NewController(t)
	defer mockCtrl.Finish()

    m := mock_transform.NewMockDataRepositoryInterface(mockCtrl)

    // 期待値をここで入力する
    m.EXPECT().GetData(mock.Any()).Return(Data{
        x: 1,
        y: 2,
        z: 3,
    }, nil) // 入力に関係なく値を返却する
    m.EXPECT().GetData(mock.Any()).Return(Out{
            distance_xy: 1,
            distance_yz: 1,
            distance_xz: 2,
    }, 
    nil) 

    s := dataService{
        dataRepo: m,
    }

    // ### When ###
    i := 1
    actual, err := s.Transform(i)
    expected := Out{
        distance_xy: 1,
        distance_yz: 1,
        distance_xz: 2,
    }

    // ### Then ###
    if err != nil {
        t.Errorf("result should be nil. err: %v", err)
    }
    if diff := cmp.Diff(actual, expected); diff != ""{
        t.Errorf("diff: %s", diff)
    }
    
}
//}

=== http通信に対するテスト TODO
http通信を行う場合、httpもmockを立てて、そこに対してリクエストを送ることで


http通信を非同期で行うmockを自ら書いてテストする方法もあります。
が、この場合はサーバーの立ち上げ時間などでテストが失敗してしまうケースがあるため、なるべく最初の方法を利用したほうがベターです。

//source[transform.go]{
import (
	"github.com/jarcoal/httpmock"
	"io/ioutil"
	"net/http"
	"testing"
)

func TestHttpMockSample(t *testing.T) {
	httpmock.Activate()
	defer httpmock.DeactivateAndReset()

	httpmock.RegisterResponder(
		"GET",
		"http://www.google.co.jp",
		httpmock.NewStringResponder(200, "this is a mock."))

	res, _ := http.Get("http://www.google.co.jp")
	body, _ := ioutil.ReadAll(res.Body)
	defer res.Body.Close()

	expected := "this is a mock."
	ret := string(body)
	if ret != expected {
		t.Errorf("expected %s, but got %s\n", expected, ret)
	}
}
}

=== 標準出力に対するテスト
標準出力をテストする場合は、出力先を変数に格納するヘルパー関数を用意します。  

//source[helper.go]{
func captureStdout(f func()) string {
    r, w, _ := os.Pipe()

    stdout := os.Stdout
    os.Stdout = w

    f()

    os.Stdout = stdout
    w.Close()

    var buf bytes.Buffer
    io.Copy(&buf, r)

    return buf.String()
}
//}

上記のように、関数を呼び出す際に出力先をbytes.Bufferに切り替え、それをstringにして返しています。  
実際にrun()関数の標準出力をテストする場合は下記の様に書くことができます。

//source[run_test.go]{
func TestRun(t *testing.T) {
    var code int
    expected := "expected output"

    actual := captureStdout(func() {
        code = run()
    })

    if code != ExitCodeOK {
        t.Errorf("Unexpected exit code: %d", code)
    }

    if actual != expected {
        t.Errorf("result does not match. actual: %v, expected: %v", actual, expected)
    }
}
//}

=== CLIへのテスト

CLIは一見テストが行いづらそうに思えますが、main関数をそのままrunで囲ってしまうことでテストを分離することが可能です。

//source[main.go]{
func main() {
	code := run(os.Args[1:])
	return code
}

const (
    ExitCodeOK = 0
    ExitCodeError = 1
)

func run(args []string) int {
    // オプション引数のパース
    var version bool
    flags := flag.NewFlagSet(os.Args[0], flag.ContinueOnError)
    flags.BoolVar(&version, "version", false, "Print version information and quit")

    if err := flags.Parse(args[1:]); err != nil
        return ExitCodeError
    }

    fmt.Println("Do awesome workn")

    return ExitCodeOK
}
//}

=== ファイル入力のテスト

ファイル入力を行う場合は、packageのtestdata配下に読み込みたいテストファイルを置き、それを読み込んでテストを行うと良いです。

//source[testdata/test.csv]{
id,name
1,Taro
2,Jiro
//}

//source[read.go]{
type User struct {
	ID   int    `csv:"id"`
	Name string `csv:"name"`
}

func read(p string) []User {
	var users []User
	b, _ := ioutil.ReadFile(p)
	_ = csvutil.Unmarshal(b, &users)
	return users
}
//}

//source[read_test.go]{
func TestRead(t *testing.T) {
	// ### Given ###
	p := "./testdata/test.csv"
	expected := []User{
		User{
			ID:   1,
			Name: "Taro",
		},
		User{
			ID:   2,
			Name: "Jiro",
		},
	}

	// ### When ###
	actual := read(p)

	// ### Then ###
	if diff := cmp.Diff(actual, expected); diff != "" {
		t.Errorf("diff: %s", diff)
	}
}
//}


=== ファイル出力のテスト

ファイル出力を行う場合は、テストの中でtemporaryのディレクトリを作成します。
次に出力先を書き換え、そこに出力し、中身を読み込むことで単体テストの範囲でファイルの出力内容について確認することが出来ます。

//source[run.go]{
func run(p string, txt string) { 
	file, err := os.Create(p)
	if err != nil {
		panic(err)
	}
	defer file.Close()
	file.Write(([]byte)(txt))
}
//}

//source[run_test.go]{
func TestRun(t *testing.T) {
	// ### Given ###
	tmp, _ := ioutil.TempFile("", "_")
	defer func() {
		os.Remove(tmp.Name())
	}()
    expected := "Hello, World!!"

	// ### When ###
	run(tmp.Name(), expected)

	// ### Then ###
	in, _ := os.Open(tmp.Name())
	defer in.Close()
	bs, err := ioutil.ReadAll(in)
	if err != nil {
		t.Errorf("failed to read: %v", err)
	}
	actual := string(bs)

	if actual != expected {
		t.Errorf("result does not match. actual: %v, expected: %v", actual, expected)
	}
}
//}

== 参考
http://yukihir0.hatenablog.jp/entry/2015/07/05/154626
https://blog.y-yuki.net/entry/2017/05/08/000000
https://swet.dena.com/entry/2018/01/29/141707

