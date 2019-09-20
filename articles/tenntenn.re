#@# textlint-disable
= 静的解析を使ったISUCON用ツールの開発

== はじめに

株式会社メルペイのバックエンドエンジニアの@tenntenn@<fn>{tenntenn}です。
本稿では、筆者がISUCON@<fn>{isucon}という競技のために開発したツールについて解説します。

//footnote[tenntenn][@<href>{https://tenntenn.dev/}]
//footnote[isucon][@<href>{http://isucon.net/}]

== ISUCONに向けた準備

=== ISUCONとは

ISUCONとは、1人〜3人のチームでお題となるWebサービスを決められたレギュレーションの中で
限界まで高速化を図るチューニングバトルです。
予選と本戦があり、予選はオンラインで行われます。

チューニング行うために、次のような作業を行うでしょう。

 * パーフォーマンスを計測する
 * ソースコードを読む
 * インフラの構成を把握する
 * 改善案を検討する

この中には、ソースコードに決まったコード片を差し込んでいくような単純作業も含みます。
ISUCON当日は時間は貴重なため、単純作業よりも改善策の検討や実装に時間を使いたいと考えるでしょう。
本稿では、単純作業に費やす時間をできるだけ減らすために、
静的解析を用いたツールを開発する方法について解説します。

=== ISUCONに向けた準備

筆者がISUCON9の予選に参加した際、事前に次のような準備を行いました。

 * 過去問を解いて練習する
 * 予選を行う場所の環境を整える
 * 当日使うツール類を準備する

過去に開催されたISUCONの予選や本戦の問題は、Web上で公開されています。
そのため、それらを用いて練習を行います。
その際に、チームメンバーの役割を決めておくとよいでしょう。
筆者はGoが得意なのでGoのコードの改善を行う役割を担当しました。

予選はオンラインで行われます。
そのため、チームで集まって競技に挑む場合は場所を用意する必要があります。
チームメンバーが所属する企業や学校の会議室や教室を使ってすることが多いでしょう。
一度現地に集まって、当日と同じように環境を組み立てておくことで当日になって慌てなくて済みます。
筆者の場合は椅子やディスプレイ、ケーブル類などを準備して足りないものがないかチェックしています。

ISUCON当日はできるだけ無駄な時間を使いたくありません。
そのため、事前に使うであろうツールの準備しておくとよいでしょう。
たとえば、Fabric@<fn>{fabric}のようなリモートでコマンドが実行可能なタスクランターを用意したり、
競技に特化したツールを作っておいたりします。

//footnote[fabric][@<href>{http://www.fabfile.org/}]

ISUCON用のツールはさまざまですが、
本稿では競技時になるべく汎用に使える静的解析を用いたツールについて紹介します。

== 静的解析を用いる理由

=== 静的解析とは

静的解析とは、プログラムを実行せずに解析を行う手法です。
ソースコードを入力データとし、バグの発見やコード生成などを行います。

静的解析は@<img>{static-analysis-flow}のような流れで行われます。

//image[static-analysis-flow][静的解析の流れ]{
静的解析の流れ
//}

各フェーズで得られる情報が異なります。
それぞれのフェーズでは次のような情報を得ることができます。

 * 字句解析
  * どの部分が数値なのか識別子なのかなど
 * 構文解析
  * 文法上の構造
 * 型チェック
  * 型や識別子の定義場所、使用場所
 * 静的単一代入形式への変換
  * 特定の値に対する処理
  * フロー
 * ポインタ解析
  * 特定のポインタがどの変数を指し示すのか
  * 動的なものを含んだコールグラフ

これらの情報を用いてバグの発見やコード生成を行います。

=== 静的解析とGo

Goでは次のような理由から容易に静的解析を行える環境が整っています。

 * 静的型付け
 * 文法がシンプル
 * 標準パッケージで静的解析の機能を提供

Goが開発されたモチベーション1つに静的解析しやすい言語を作りたい@<fn>{goatgoogle}というものがありました。
そのため、Goは静的解析がしやすいように、言語が設計されています。

//footnote[goatgoogle][@<href>{https://talks.golang.org/2012/splash.article}]

また、標準パッケージの1つである@<code>{go}パッケージが静的解析の機能を提供しています。
@<code>{go}パッケージには@<table>{gopackage}に示したようなサブパッケージがあります。
それぞれの機能についてはここでは触れませんが、ひととおりの機能は標準の@<code>{go}パッケージで十分です。

//table[gopackage][goパッケージのサブパッケージ]{
サブパッケージ名	役割
==============================
@<code>{go/ast}	抽象構文木（AST）を提供
@<code>{go/build}	パッケージに関する情報を集める
@<code>{go/constant}	定数に関する型を提供
@<code>{go/doc}	ドキュメントをASTから取り出す
@<code>{go/format}	コードフォーマッタ機能を提供
@<code>{go/importer}	コンパイラに適したImporterを提供
@<code>{go/parser}	構文解析 機能を提供
@<code>{go/printer}	AST 表示機能を提供
@<code>{go/scanner}	字句解析 機能を提供
@<code>{go/token}	トークンに関する型を提供
@<code>{go/types}	型チェックに関する機能を提供
//}

また、準標準の@<code>{golang.org/x/tools/go}パッケージ以下でも@<table>{xgopackage}に示したような機能が提供されています。
標準パッケージでは補えないコアな機能があり、本書でも@<code>{analysis}パッケージと@<code>{ssa}パッケージについては解説を行います。

//table[xgopackage][golang.org/x/tools/goパッケージのサブパッケージ]{
サブパッケージ名	役割
==============================
@<code>{analysis}	静的解析ツールをモジュール化するパッケージ
@<code>{ast}	抽象構文木関連のユーティリティ
@<code>{callgraph}	コールグラフ関連
@<code>{cfg}	コントロールフローグラフ関連
@<code>{expect}	構造化されたコメントを処理する
@<code>{packages}	@<code>{go/build}に代わるパッケージ
@<code>{pointer}	ポインタ解析
@<code>{ssa}	Static Single Assignment（SSA）関連
@<code>{types}	型情報関連
//}

=== 静的解析を用いる理由

ISUCONのツールに静的解析を用いる理由は次の2つです。

 * 汎用的なツールを作りやすい
 * エディタや統合開発環境（IDE）の機能だけでは対応できない

ISUCONの問題で提供されるソースコードは、競技当日まで公開されません。
そのため、どんなソースコードが提供されても対応できるツールを作る必要があります。
ソースコードを単なる文字列として扱うようなツールを作ってしまうと、エッジケースが多すぎて、
とても汎用的とは呼べないでしょう。

静的解析ではソースコードを抽象構文木や型情報などGoのプログラムとして解釈するため、
汎用的な処理を書くことが可能になります。
たとえば、ソースコード中の文字列を取得したい場合、@<code>{""}でくくられた部分を取り出せば良さそうです。
しかし、Goでは@<code>{``}でくくられたものも文字列として扱われるため、そう簡単にはいきません。
一方、静的解析を用いることで、どちらも文字列リテラルとして解釈できます。

静的解析ツールはエディタやIDEにも付属しています。
たとえば、身近なところではコード補完や定義ジャンプです。
ソースコードを編集する上でこらの静的解析ツールはISUCONでも役に立つでしょう。
しかし、ISUCON特有の処理を行ないたい場合は、専用の静的解析ツールを自作する必要があります。
たとえば、ソースコードに特定の処理を埋め込んでいったり、ソースコードからSQL文を取り出したりする場合です。

本稿では静的解析を用いたISUCON用のツールとしてmeasuregenとsqlstrの2つを紹介します。
また、この2つのツールが静的解析をどのように用いて作られているか解説します。

== 計測用関数をソースコードに埋め込む

=== measure: 計測用ライブラリ

ISUCONでは、ボトルネックがどこにあるかパフォーマンスを計測することによって知る必要あります。
measure@<fn>{measure}は、指定した範囲の処理時間、実行回数などの合計値、平均値、最大・最小値などを計測できるライブラリです。

//footnote[measure][@<href>{https://github.com/najeira/measure}]

たとえば、@<code>{foo}関数のパフォーマンスを計測したい場合、
@<list>{measureexample1}のように記述します。
関数の先頭で@<code>{measure.Start}関数を呼び、関数終了時に@<code>{Stop}メソッドを呼び出します。
@<code>{defer}文を用いることで一文で書くことができます。

//list[measureexample1][measureを使った例1][go]{
import "github.com/najeira/measure"

func foo() {
  defer measure.Start("foo").Stop()
  // (略)
}
//}

関数単位ではなく、特定の範囲を計測したい場合は@<list>{measureexample2}のように記述します。

//list[measureexample2][measureを使った例2][go]{
// （略）
m := measure.Start("foo")
// 計測したい部分
m.Stop()
// （略）
//}

計測した結果は@<list>{measurestats}のように取得できます。
HTTPハンドラを作って計測用のエンドポイントを用意するか、
@<code>{http.HandleFunc}関数で@<code>{measure.HandleStats}を用いるとよいでしょう。

//list[measurestats][計測結果の取得][go]{
stats := measure.GetStats()
stats.SortDesc("sum") // 合計でソート

// CSV形式で出力する
for _, s := range stats {
  fmt.Fprintf(w, "%s,%d,%f,%f,%f,%f,%f,%f\n",
    s.Key, s.Count, s.Sum, s.Min, s.Max, s.Avg, s.Rate, s.P95)
}
//}

=== measuregen: 自動で計測用関数を埋め込む

measureは非常に便利なライブラリですが、計測したい範囲で
@<code>{Start}関数と@<code>{Stop}メソッドを呼び出す必要があります。
関数ごとにパフォーマンスを計測したい場合は、すべての関数にこの処理を埋め込んでいかなければいけません。
単純な作業ではありますが、関数が多い場合には貴重な時間を消費してしまいます。

筆者が開発したmeasuregen@<fn>{measuregen}は、この作業を自動で行ってくれるツールです。
静的解析によって関数の先頭に@<code>{defer measure.Start("foo").Stop()}のような文を自動で追加していきます。

//footnote[measuregen][@<href>{https://github.com/tenntenn/isucontools/tree/master/cmd/measuregen}]

measuregenは標準入力から入力されたソースコードに対して、すべての関数の先頭に計測用の処理を差し込み、
標準出力に生成したソースコードを出力します。
そのため、次のようにコマンドを実行することによってmain.goに対して処理を行うことができます。

//cmd{
$ cat main.go | measuregen > main_new.go
$ mv main_new.go main.go
//}

measuregenは冪等性を担保しているため、次のようにmeasuregenが出力ソースコードに
再度measuregenをかけても何もおきません。

//cmd{
$ cat main.go | measuregen | measuregen
//}

そのため、競技途中でリファクタリングによって新しい関数が増えた場合でも再度measuregenをかけると
新しい関数にのみ計測用の処理が差し込まれます。

=== measuregenのしくみ

measuregenでは次のような処理を行うことで、関数の先頭に処理を差し込んでいます。

 * ソースコードに対して構文解析を行い抽象構文木を得る
 * 抽象構文木から関数定義に対応するノードを取得する
 * 関数定義を表すノードの関数本体部分に@<code>{defer}文を差し込む

ソースコードから抽象構文木を得るためには、@<list>{parse}のように
@<code>{go/parser}パッケージの@<code>{ParseFile}関数を用います。
この場合、第1戻り値を代入した変数@<code>{f}が標準入力で与えたファイルを抽象構文木で表したものになります。

//list[parse][構文解析の例][go]{
fset := token.NewFileSet()
f, err := parser.ParseFile(fset, "main.go", os.Stdin, parser.ParseComments)
if err != nil {
  log.Fatal("Error:", err)
}
//}

得られた抽象構文木は@<code>{inspector}@<fn>{inspector}パッケージ用いて探索を行うことができます。
@<code>{(*inspector.Inspector).WithStack}は指定した抽象構文木を探索するために用います。

//footnote[inspector][@<href>{https://golang.org/x/tools/go/ast/inspector}]

第1引数に処理するノードを限定するフィルタ、
第2引数に探索用の関数を指定します。
探索用の関数は、抽象構文木に対して深さ優先探索を行う際に各ノードに対して行われる関数です。
第1引数に処理対象のノード、第2引数に探索の"ゆき"または"かえり"かを表す値、第3引数にここまで通った親ノードが渡されます。

@<list>{inspector}のように、ノードの種類によって型スイッチで処理を分岐させます。
measuregenでは、関数定義を表すノードに対して処理行うため、
@<code>{*ast.FuncDecl}型と@<code>{*ast.FuncLit}型で表されるノードを対象としています。
@<code>{*ast.FuncDecl}型は名前付きの関数定義を表し、@<code>{*ast.FuncLit}型は関数リテラル、つまり無名関数を表します。

//list[inspector][抽象構文木の探索][go]{
// 関数定義だけを対象にする
filter := []ast.Node{ new(ast.FuncDecl), new(ast.FuncLit) }
inspector.New([]*ast.File{f}).WithStack(
  filter, func(n ast.Node, push bool, stack []ast.Node) bool {
    if !push { return true }
    switch n := n.(type) {
    case *ast.FuncDecl:
      // 名前付き関数の定義を処理
    case *ast.FuncLit:
      // 無名関数の定義を処理
    case *ast.FuncLit:
    }
    return true
})
//}

名前付き関数を表す@<code>{*ast.FuncDecl}型のノードは@<list>{FuncDecl}のように処理されます。
まず@<code>{defer}文で差し込みたい関数呼び出しを文字列で表現したものを
@<code>{parser.ParseExpr}関数で式を表す抽象構文木のノードに変換します。
@<code>{measure.Start}関数の第1引数には計測対象の名前を指定するため、@<code>{n.Name.Name}で関数名を取得しています。

そして、@<code>{defer}文を表す@<code>{*ast.DeferStmt}型の値を生成し、
関数本体を表す@<code>{n.Body}フィールドが持つ文のリストの先頭に差し込んでいきます。
その際、すでに先頭に該当の処理が差し込まれていないか@<code>{hasMeasure}関数でチェックしています。

//list[FuncDecl][名前付き関数の処理][go]{
exprStr := fmt.Sprintf(`measure.Start("%s").Stop()`, n.Name.Name)
expr, err := parser.ParseExpr(exprStr)
if err != nil {
  log.Fatal("Error:", err)
}
deferStmt := &ast.DeferStmt{Call: expr.(*ast.CallExpr)}

if hasMeasure(n.Body.List, deferStmt) {
  return true
}

n.Body.List = append([]ast.Stmt{deferStmt}, n.Body.List...)
//}

同様に無名関数を表す@<code>{*ast.FuncLit}型のノードに対しても処理を行います。
@<list>{FuncLit}では無名関数からは関数名が取れないため、
親の関数名に連番を付与した文字列を@<code>{measure.Start}関数の引数に渡しています。

//list[FuncLit][無名関数の処理][go]{
name := "NONAME"
if parent := findParent(stack); parent != nil {
  closures[parent]++ // closuresはmap[*ast.FuncDecl]int型
  name = fmt.Sprintf("%s-%d", parent.Name.Name, closures[parent])
}
exprStr := fmt.Sprintf(`measure.Start("%s").Stop()`, name)
expr, err := parser.ParseExpr(exprStr)
if err != nil {
  log.Fatal("Error:", err)
}

deferStmt := &ast.DeferStmt{Call: expr.(*ast.CallExpr)}

if hasMeasure(n.Body.List, deferStmt) {
  return true
}

n.Body.List = append([]ast.Stmt{deferStmt}, n.Body.List...)
//}

関数の先頭に計測用の関数呼び出しを差し込んでいくだけではビルドがとおりません。
@<list>{addImport}のようにファイルに@<code>{measure}パッケージを
インポートする処理を追加していく必要があります。

//list[addImport][インポート文を追加する関数][go]{
func addImport(f *ast.File) {

  // すでにインポートしているか調べる
  for _, im := range f.Imports {
    path, err := strconv.Unquote(im.Path.Value)
    if err != nil {
      continue
    }

    // already imported
    if path == importPATH {
      return
    }
  }

  // ast.FileのImportsフィールドに追加
  importSpec := &ast.ImportSpec{
    Name: ast.NewIdent("measure"),
    Path: &ast.BasicLit{
      Kind:  token.STRING,
      Value: strconv.Quote(importPATH),
    },
  }
  f.Imports = append(f.Imports, importSpec)

  // 他のimport文がある場合は最後に追加する
  var lastGenDecl *ast.GenDecl
  for _, decl := range f.Decls {
    genDecl, ok := decl.(*ast.GenDecl)
    if !ok || genDecl.Tok != token.IMPORT {
      continue
    }
    lastGenDecl = genDecl
  }
  if lastGenDecl != nil {
    lastGenDecl.Specs = append(lastGenDecl.Specs, importSpec)
    return
  }

  // import文が1つもない場合は新しく作る
  f.Decls = append([]ast.Decl{&ast.GenDecl{
    Tok:   token.IMPORT,
    Specs: []ast.Spec{importSpec},
  }}, f.Decls...)
}

こうして改変した抽象構文木をソースコードに戻すには、@<code>{go/format}パッケージの@<code>{Node}関数を用います。
標準出力に出力したい場合は、@<code>{format.Node(os.Stdout, fset, f)}のように記述します。

== ソースコードからSQLを取り出す

=== sqlstr: ソースコードからSQL取り出す

ISUCONでは、ソースコード中に書かれたSQLを読みクエリを改善することが多いです。
SQLはソースコード中に散らばっているため、漏れなく探すのに時間が取られます。

そこで筆者はsqlstrというツールを作ることによって、ソースコード中のSQLを抜き出すことにしました。
SQLといっても厳密に何がSQLなのかを判定するのは難しいため、
ここでは簡単のため@<code>{SELECT}や@<code>{UPDATE}などで始まっているSQLっぽい文字列を抜き出すことにします。

sqlstrは次のように用います。
プログラム引数に対象のソースコードを渡すか、標準入力にソースコードを渡します。
そうするとソースコード中からSQLっぽい文字列を見つけて、標準出力にファイル上の位置や関数名ともに出力します。

//cmd{
$ sqlstr main.go
main.go:5:14
    SELECT * FROM items

main.go:8:15 in main
    SELECT * FROM items
//}

=== sqlstrのしくみ

sqlstrは抽象構文木の中から文字列リテラルを表す
@<code>{*ast.BasicLit}型のノードを探している訳ではありません。
文字列定数といっても@<code>{"SELECT * " + "FROM items"}のような定数式になる可能性があります。

そのためsqlstrでは、まず抽象構文木中の式にあたる@<code>{ast.Expr}型のノードを定数式として評価を試みます。
@<code>{v + "ABC"}のように文字列定数として評価できないような式の場合は無視します。

式の評価には、@<list>{eval}のような関数を用います。
@<code>{eval}関数は引数でもらった式を一度文字列のソースコードにし、
@<code>{types.Eval}関数によって定数式として評価を行います。

@<code>{types.Eval}関数は定数として評価できない式が渡された場合は、エラーを返します。
エラーが返ってきた場合には、その式は無視して次の式の評価に移ります。
定数式として評価できた場合も、その結果が文字列である場合のみSQLっぽい文字列かどうかの判断をします。

//list[eval][式の評価][go]{
func eval(expr ast.Expr) (types.TypeAndValue, error) {
  fset := token.NewFileSet()
  var buf bytes.Buffer
  if err := printer.Fprint(&buf, fset, expr); err != nil {
    return types.TypeAndValue{}, err
  }
  pkg := types.NewPackage("main", "main")
  return types.Eval(fset, pkg, token.NoPos, buf.String())
}
//}

このように、抽象構文木を取得する構文解析では定数式の評価を行うことはできません。
一方、次のフェーズの型チェックまで行うと定数式の評価も行えますが、
識別子の解決や型情報の取得など不要な処理も行ってしまいます。
そのため、sqlstrでは型チェックのすべてを行わずに
定数式の評価だけを@<code>{types.Eval}関数を用いて行っています。

== おわりに

本稿では静的解析を用いてISUCON向けのツールを作る方法について解説を行いました。
静的解析を用いれば本稿で扱ったmeasuregenやsqlstr以外にも便利なツールが作れるでしょう。
たとえば、筆者が次に作ろうと考えているのは、ソースコード中のSQLっぽい文字列にコメントを差し込んでいくツールを作ろうと考えています。
つまり、@<code>{"SELECT * FROM items"}のような文字列があった場合に、
@<code>{"/* main() */ SELECT * FROM items"}のように変更するツールです。
RDBを監視する上でそのクエリがどこで発行されたのか知る助けになるはずです。

読者のみなさんもぜひ静的解析を用いて何かツールを作ってみてください。
#@# textlint-enable
