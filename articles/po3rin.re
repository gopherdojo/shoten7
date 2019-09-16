= Go + Gonum による行列計算のエッセンス

== はじめに

白ヤギコーポレーションでバックエンドエンジニアをしている@po3rin@<fn>{po3rin}です。仕事ではGoをメインにサーバーサイドの開発をしています。Goで行列なんて扱えるのかと思うかもしれませんが、Go製の数値計算パッケージである@<em>{Gonum}の中に@<em>{mat}パッケージという行列計算を行うための機能が提供されています。今回はGo+Gonumによる行列計算のエッセンスを紹介します。
//footnote[po3rin][@<href>{https://twitter.com/po3rin}]

== Gonumによる行列の生成と内部実装

ここからはGonumのドキュメント@<fn>{godoc}を見ながらだと理解が進みやすいと思います。まずは行列の作り方を見てみましょう。

//footnote[godoc][@<href>{https://godoc.org/gonum.org/v1/gonum/mat}]

//list[gen][行列の生成][go]{
package main

import (
    "fmt"

    "gonum.org/v1/gonum/mat"
)

func main() {
    A := mat.NewDense(3, 4, []float64{
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
    })
    // |1  2  3  4|
    // |5  6  7  8|
    // |9 10 11 12|

    // ...
}
//}

@<list>{gen}で@<code>{mat.NewDense}を使って@<code>{*mat.Dense}という行列のデータ構造をもつ構造体が返ってきます。@<code>{mat.Dense}のデータ構造をみてみましょう。

//list[inner][mat.Denseの定義][go]{
// gonum/mat/dense.go

type Dense struct {
    mat blas64.General

    capRows, capCols int
}

func NewDense(r, c int, data []float64) *Dense {
    if r < 0 || c < 0 {
        panic("mat: negative dimension")
    }
    if data != nil && r*c != len(data) {
        panic(ErrShape)
    }
    if data == nil {
        data = make([]float64, r*c)
    }
    return &Dense{
        mat: blas64.General{
            Rows:   r,
            Cols:   c,
            Stride: c,
            Data:   data,
        },
        capRows: r,
        capCols: c,
    }
}
//}

@<code>{mat.NewDense}を見れば分かりやすいですが、Denseの正体は要素の値の一覧である@<code>{[]float64}に行数、列数、ストライドなどの情報を保持しているだけです。また、要素を全部0で初期化したい場合は@<code>{mat.NewDense}コードから@<code>{mat.NewDense}の第三引数に@<code>{nil}を渡せばよいことが分かります。mat.NewDense@<code>{Dense.mat}の型である@<code>{blas64.General}については後ほど解説します。そして@<code>{*mat.Dense}は@<code>{mat.Matrix}というインターフェースを実装しています。

//list[matrix][mat.Matrixインターフェース][go]{
// Matrix is the basic matrix interface type.
type Matrix interface {
    Dims() (r, c int) // 行数と列数を返す
    At(i, j int) float64 // 指定した要素の値を返す
    T() Matrix // 転置行列を返す
}
//}

@<code>{mat.Dense}以外で@<code>{Matrix}インターフェースを実装するものとしては@<code>{mat.VecDense}（ベクトルを表現するデータ構造）や@<code>{mat.TriDense}（三角行列を表現するデータ構造）などさまざまな構造体があります。@<code>{Matrix}インターフェースとして振る舞いを定義することで、データ構造の違いを意識せずにコード内で利用できるようになっています。

== Gonumで簡単な行列計算をやってみよう

=== 行列のPrint

早速Gonumによる行列計算の例をみていきたいところですが。今後の行列のデバッグのために@<list>{print}のように@<code>{mat.Formatted}関数を使って行列の構造を確認できるようにしておくと便利です。@<code>{mat.Formatted}関数は@<em>{Functional Option Pattern}で実装されており、すでに用意されているオプション関数を使って出力する行列をフォーマットできます。Functional Option PatternとはGoでよく使われるデザインパターンの１つなので、気になる方はRob Pike氏の「Self-referential functions and the design of options」@<fn>{fop}という記事を読んでみましょう。

//footnote[fop][@<href>{https://commandcenter.blogspot.com/2014/01/self-referential-functions-and-design.html}]

//list[print][mat.Formattedを使ったデバッグ用関数の実装][go]{
func matPrint(X mat.Matrix) {
    fa := mat.Formatted(X, mat.Prefix(""), mat.Squeeze())
    fmt.Printf("%v\n", fa)
}
//}

@<list>{print}を使って@<list>{gen}を書き直して@<list>{printwith}のようにすると行列がprintできます。

//list[printwith][行列の生成][go]{
package main

import (
    "fmt"

    "gonum.org/v1/gonum/mat"
)

func main() {
   A := mat.NewDense(3, 4, []float64{
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
    })
    matPrint(A)
    // |1  2  3  4|
    // |5  6  7  8|
    // |9 10 11 12|
}
//}

=== 行列の基本演算

@<code>{gonum/mat}をはじめて使う人のために行列の計算の方法を少し紹介します。まずは基本の足し算引き算です。GoにはPythonのように演算子オーバーロードがないので行列の演算も関数やメソッドを介して行います。

//list[add][行列の足し引き][go]{
func main() {
    A := mat.NewDense(3, 4, []float64{
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
    })

    //行列の足し算
    B := mat.NewDense(3, 4, nil)
    B.Add(A, A)
    matPrint(B)
    // | 2   4   6   8|
    // |10  12  14  16|
    // |18  20  22  24|

    //行列の引き算
    C := mat.NewDense(3, 4, nil)
    C.Sub(A, A)
    matPrint(C)
    // |0  0  0  0|
    // |0  0  0  0|
    // |0  0  0  0|
}
//}

@<code>{mat.Dense}に生えているメソッドを使います。レシーバの値に計算の結果が格納されます。当然他のデータ構造には別のメソッドが生えています。ベクトルを表す@<code>{mat.VecDense}には@<code>{AddVec}というメソッドが生えています。もちろん行列の定数倍、要素同士の積、内積も計算できます。

//list[scale][行列の定数倍][go]{
func main() {
    A := mat.NewDense(3, 4, []float64{
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
    })

    // 要素の定数倍
    C = mat.NewDense(3, 4, nil)
    C.Scale(2, A)
    matPrint(&C)
    // | 2   4   6   8|
    // |10  12  14  16|
    // |18  20  22  24|

    // 要素同士の掛け算
    C = mat.NewDense(3, 4, nil)
    C.MulElem(A, A)
    matPrint(C)
    // | 1    4    9   16|
    // |25   36   49   64|
    // |81  100  121  144|

    // 内積
    C = mat.NewDense(3, 3, nil)
    C.Product(A, A.T()) // (T()は転置行列を返します)
    matPrint(C)
    // | 30   70  110|
    // | 70  174  278|
    // |110  278  446|
}
//}

その他の各種演算用のメソッドが用意されています。是非@<em>{gonum/mat}のドキュメント@<fn>{doc}をのぞいてみてください。

//footnote[doc][@<href>{https://godoc.org/gonum.org/v1/gonum/mat}]

=== 要素の出し入れ

要素の出し入れもメソッドで定義されています。

//list[io][要素の出し入れ][go]{
func main() {
    A := mat.NewDense(3, 4, []float64{
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
    })

    // (0,2)の値を取得
    a := A.At(0, 2)
    println("A[0, 2]: ", a)
    // A[0, 2]:  +3.000000e+000

    // (0,2)に-1.5をセット
    A.Set(0, 2, -1.5)
    matPrint(A)
    // |1   2  -1.5   4|
    // |5   6     7   8|
    // |9  10    11  12|
}
//}

スライシングももちろん可能です。スライシングは行列から指定の箇所を行列として抽出する操作です。

//list[sliceing][行列のスライシング][go]{
func main() {
    A := mat.NewDense(3, 4, []float64{
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
    })

    // 指定した範囲を行列として取得
    S := A.Slice(0, 3, 0, 3)
    matPrint(S)
    // |1   2   3|
    // |5   6   7|
    // |9  10  11|
}
//}

=== 行列の結合

行列の結合においては@<code>{Stack}メソッドや@<code>{Augment}が用意されています。

//list[stack][Stackを使った行列の結合][go]{
func main() {
    a := mat.NewDense(3, 2, []float64{
        1, 2,
        3, 4,
        5, 6,
    })
    b := mat.NewDense(3, 2, []float64{
        7, 8,
        9, 10,
        11, 12,
    })

    c := mat.NewDense(6, 2, nil)
    c.Stack(a, b)
    matPrint(c)
    /*
    | 1   2|
    | 3   4|
    | 5   6|
    | 7   8|
    | 9  10|
    |11  12|
    */

    d := mat.NewDense(3, 4, nil)
    d.Augment(a, b)
    matPrint(d)
    /*
    |1  2   7   8|
    |3  4   9  10|
    |5  6  11  12|
    */
}
//}

=== 各要素に任意の操作を行う

@<code>{Apply}メソッドは要素に任意の操作を行いたい場合に便利です。

//list[apply][Applyのサンプル][go]{
func main() {
    A := mat.NewDense(3, 4, []float64{
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
    })

    // 各要素の値に行番号、列番号を足す
    sumOfIndices := func(i, j int, v float64) float64 {
        return float64(i+j) + v
    }

    var B mat.Dense
    B.Apply(sumOfIndices, A)
    matPrint(B)
    // | 1   3   5   7|
    // | 6   8  10  12|
    // |11  13  15  17|
}
//}

@<code>{Apply}は各要素に任意の操作を実行できます。第一引数には@<code>{func(i int, j int, v float64) float64}という型の関数を渡します。@<code>{i},@<code>{j}は行番号、列番号で、@<code>{v}はその要素の値です。この3つの値を使って任意の操作を行います。そして返り値が行列に新しくセットされます。@<code>{gonum/mat}に用意されていない複雑な計算をする時に便利です。

== Go+Gonumによる行列計算の実装レシピ

ここではGonumでは提供されていないけど、Gonumで行列を本格的に扱おうとすると必要になってくる処理をピックアップして実装の方法をご紹介します。

=== 行列とベクトルの足し算

Pythonでは演算子オーバーロードがあるので@<code>{+}でそのまま行列とベクトルの足し算ができますが、Gonumの@<code>{Add}メソッドでは同じサイズの行列しか計算できません。そこで行列とベクトルの足し算をする関数を別で用意する必要があります。

//list[addmatvec][行列とベクトルの足し算][go]{
func AddMatVec(x *mat.Dense, v *mat.VecDense) *mat.Dense {
	r, c := x.Dims()
	result := mat.NewDense(r, c, nil)

	for i := 0; i < r; i++ {
		vd := mat.NewVecDense(c, nil)
		rv := x.RowView(i)
		vd.AddVec(rv, v)
		result.SetRow(i, vd.RawVector().Data)
	}

	return result
}
//}

@<list>{addmatvec}ではまだ紹介していないGonumの機能を使っています。@<code>{RowView}は引数で指定した行を@<code>{mat.Vector}インターフェースとして抜き出します。@<code>{mat.Vector}(@<list>{vec})はベクトルとしての振る舞いが定義されています。

//list[vec][ベクトルの振る舞いをもつVectorインターフェース][go]{
type Vector interface {
    Matrix
    AtVec(int) float64
    Len() int
}
//}

抜き出したベクトルと引数で受け取ったベクトルを@<code>{AddVec}で足します。最終的に@<code>{SetRow}を使ってベクトルを結果として返す行列の行に格納します。@<code>{SetRow}の定義をみてみましょう。

//list[setrow][func (*Dense) SetRow][go]{
func (m *Dense) SetRow(i int, src []float64)
//}

@<code>{SetRow}関数では第一引数で指定した行に@<code>{[]float64}を格納します。その為、@<code>{mat.VecDense}から全要素が格納された@<code>{[]float64}を取得する必要があります。そこで@<code>{RawVector}メソッドで@<code>{blas64.Vector}を取得し、全要素が格納された@<code>{[]float64}を格納している@<code>{Data}フィールドにアクセスしています。ちなみに行列の場合は@<code>{RawMatrix}メソッドが用意されています。ここは少し複雑なので各定義を@<list>{raw}にまとめます。

//list[raw][RawVector][go]{
// in gonum/mat/vector.go
func (v *VecDense) RawVector() blas64.Vector

// in gonum/mat/matrix.go
type RawVectorer interface {
    RawVector() blas64.Vector
}

// in gonum/blas/blas64/blas64.go
type Vector struct {
    N    int
    Data []float64
    Inc  int
}
//}

=== 列ごとの合計値を計算してベクトルにする

列ごとの合計値を計算してベクトルにする処理です。これはニューラルネットワークを実装する際に必要でした。

//list[sumcol][列ごとの合計値を計算してベクトルにする][go]{
func SumCol(x *mat.Dense) *mat.VecDense {
	_, c := x.Dims()
	result := mat.NewVecDense(c, nil)

	for i := 0; i < c; i++ {
		result.SetVec(i, mat.Sum(x.ColView(i)))
	}

	return result
}
//}

@<list>{sumcol}では@<code>{ColView}メソッドを使って指定した列のVectorを取得しています。@<code>{mat.Sum}関数は@<code>{Matrix}インターフェースを受けて全要素の合計値を返します。

//list[sum][全要素の合計値を返すSum関数][go]{
func Sum(a Matrix) float64
//}

@<code>{Matrix}インターフェースを受け取るので当然@<code>{mat.Dense}や@<code>{mat.VecDense}を受け取れます。合計値を@<code>{SetVec}を使ってベクトルに格納しています。

=== 行列から行を間引く

@<code>{Slice}は範囲指定で行列を生成しますが、とびとびで行を指定してそれを抽出して行列を作りたい場合は自分で実装する必要があります。

//list[thin][行列から行を間引く][go]{
func ThinRow(x mat.Dense, targets []int) *mat.Dense {
	_, c := x.Dims()
	result := mat.NewDense(len(targets), c, nil)

	for i, v := range targets {
		result.SetRow(i, x.RawRowView(v))
	}
	return result
}
//}

@<list>{thin}では引数に抽出したい行を@<code>{[]int}を指定します。@<code>{RawRowView}では指定した行の要素から形成される@<code>{[]float64}を生成します。

=== シグモイド関数

シグモイド関数はデープラーニングで活性化関数として使われたりします。

//image[Sigmoid][シグモイド関数の式][scale=0.5]{
//}

この式を実装してみましょう。

//list[sigmoid][シグモイド関数の実装][go]{
func Sigmoid(x mat.Matrix) mat.Matrix {
	sigmoid := func(i, j int, v float64) float64 {
		return 1 / (1 + math.Exp(-v))
	}

	r, c := x.Dims()
	result := mat.NewDense(r, c, nil)
	result.Apply(sigmoid, x)
	return result
}
//}

@<list>{sigmoid}のように要素ごとに関数を適用したい場合は@<code>{Apply}が便利です。

== 行列のテスト

行列計算を実装したらテストを実装したくなります。ここでも@<code>{gonum/mat}パッケージが便利な関数を提供しています。@<code>{mat.EqualApprox}を使うと誤差を許容した行列のテストができます。例としてシグモイド関数をテストします。

//list[test][行列のテスト][go]{
func TestSigmoid(t *testing.T) {
    tests := []struct {
        name  string
        input mat.Matrix
        want  mat.Matrix
    }{
        {
            name:  "2*2",
            input: mat.NewDense(2, 2, []float64{2, 2, 2, 2}),
            want:  mat.NewDense(2, 2, []float64{
                0.88079707797788, 0.88079707797788,
                0.88079707797788, 0.88079707797788,
            }),
        },
    }

    for _, tt := range tests {
        tt := tt
        t.Run(tt.name, func(t *testing.T) {
            if got := Sigmoid(tt.input); !mat.EqualApprox(got, tt.want, 1e-14) {
                t.Fatalf("want = %d, got = %d", tt.want, got)
            }
        })
    }
}
//}

こレデ行列のサイズと値が等しいかをチェックできます。@<code>{mat.EqualApprox}は第３引数は任意の許容誤差を指定できます。実際にテストケースの@<code>{want}は@<code>{0.88079707797788}と設定していますが、実際の計算結果の値は@<code>{0.8807970779778823}です。しかし、mat.EqualApproxに許容誤差を指定しているのでこのテストはPASSします。もちろん完全な一致を確認したい場合は@<code>{mat.Equal}関数が使えます。


== Gonumで行列を扱う際のエラーハンドリング

@<code>{gonum/mat}では内部で@<code>{panic}を呼んでいます。たとえば@<code>{Slice}メソッドでは内部のいたる所で@<code>{panic}を呼んでいます。

//list[slice][Sliceメソッドのソースコード][go]{
func (m *Dense) Slice(i, k, j, l int) Matrix {
	mr, mc := m.Caps()
	if i < 0 || mr <= i || j < 0 || mc <= j || k < i || mr < k || l < j || mc < l {
		if i == k || j == l {
			panic(ErrZeroLength)
		}
		panic(ErrIndexOutOfRange)
	}
	t := *m
	t.mat.Data = t.mat.Data[i*t.mat.Stride+j : (k-1)*t.mat.Stride+l]
	t.mat.Rows = k - i
	t.mat.Cols = l - j
	t.capRows -= i
	t.capCols -= j
	return &t
}
//}

しかし、@<code>{panic}ではなくちゃんとエラーハンドリングしたい場合があります。そんな時の為に@<code>{mat.Maybe}が用意されています。使い方をみてみましょう。

//list[maybe][Maybeを使ったエラーハンドリング][go]{
func main() {
    a := mat.NewDense(3, 2, []float64{
		1, 2,
		3, 4,
		5, 6,
	})
	b := mat.NewDense(3, 2, []float64{
		7, 8,
		9, 10,
		11, 12,
	})

	err := mat.Maybe(func() {
		a.Product(a, b)
	})
	if err != nil {
		log.Fatal(err)
	}
}
//}

このコードは内部で@<code>{panic}がおきます。しかし、@<code>{Maybe}でラップしているのでGonumで定義された@<code>{ErrorStack}という独自型のエラーが返ってきます。内部で@<code>{string}としてスタックトレースも保持してくれます。

//list[err][ErrorStackの実装][go]{
type ErrorStack struct {
	Err error

	// StackTrace is the stack trace
	// recovered by Maybe, MaybeFloat
	// or MaybeComplex.
	StackTrace string
}
//}

必要に応じて適宜使っていきましょう。

== Gonumで使うBLASを切り替えてみよう

=== Gonumで使うBLASを切り替える方法

最初に見たとおり、Matrixインターフェースの実態としてDenseがあります。

//list[dense][mat.Denseの定義][go]{
type Dense struct {
    mat blas64.General

    capRows, capCols int
}

@<list>{dense}のフィールドを見ると@<code>{blas.General}とあります。このblasとは何でしょうか。@<em>{BLAS}とは、Basic Linear Algebra Subprogramsの略です。つまり基本的な線形演算のライブラリを意味しています。BLASにはさまざまな種類があり OpenBLAS や ATLAS などが存在します。各BLASの説明は記事「Numpyに使われるBLASによって計算速度が変わるらしい」@<fn>{blas}がまとまっています。

//footnote[blas][@<href>{https://echomist.com/python-blas-performance/}]


Gonumでは使うBLASを簡単に切り替えれます。下記は@<em>{gonum/blas/blas64/blas64.go}のコードの抜粋です。@<code>{Use}という関数で使うBLASをすぐに切り替えれます。

//list[useblas][blas64.Useのコード][go]{
package blas64

var blas64 blas.Float64 = gonum.Implementation{}

// Use sets the BLAS float64 implementation to be used by subsequent BLAS calls.
// The default implementation is
// gonum.org/v1/gonum/blas/gonum.Implementation.
func Use(b blas.Float64) {
    blas64 = b
}
//}

たとえばDot関数では内部ではblas64.Ddotを呼び出しています。当然@<code>{blas64.Ddot}はインターフェースで定義されたメソッドなのでblas64の実体ごとに違う処理を呼べるというわけです。

//list[blas][Dot関数内部コード][go]{
// Dot computes the dot product of the two vectors:
//  \sum_i x[i]*y[i].
func Dot(x, y Vector) float64 {
    if x.N != y.N {
        panic(badLength)
    }
    return blas64.Ddot(x.N, x.Data, x.Inc, y.Data, y.Inc)
}
//}

=== OpenBLASを使った行列演算のベンチマーク

早速ベンチマークを撮ってみましょう。今回はOpenBLASとGonumのデフォルトで使われているBLAS実装を比較します。まずはOpenBLASをインストールします。Macであれば@<list>{install}でインストールできます。

//list[install][OpenBLASのインストール][go]{
$ brew install openblas
//}

ではベンチマークを書きましょう。

//list[bench][GonumのBLAS実装とOpenBLASの比較][go]{
package main_test

import (
	"math/rand"
	"testing"

	"gonum.org/v1/gonum/blas/blas64"
	"gonum.org/v1/gonum/blas/gonum"
	"gonum.org/v1/gonum/mat"
	"gonum.org/v1/netlib/blas/netlib"
)

func makeRandVec(num int) *mat.VecDense {
	data := make([]float64, num)
	for i := range data {
		data[i] = rand.NormFloat64()
	}
	return mat.NewVecDense(num, data)
}

func BenchmarkOpenBLAS(b *testing.B) {
	blas64.Use(netlib.Implementation{})
	v := makeRandVec(10000)
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_ = mat.Dot(v, v)
	}
}

func BenchmarkGonumBLAS(b *testing.B) {
	blas64.Use(gonum.Implementation{})
	v := makeRandVec(10000)
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_ = mat.Dot(v, v)
	}
}
//}

@<code>{blas64.Use}で使うBLASをベンチマークごとに切り替えれいます。@<list>{bench}の@<code>{makeRandVec}ではテスト用にランダムな要素をもつ行列を生成します。ベンチマークを実行するときは@<code>{CGO_LDFLAGS}フラッグでOpenBLASのインストール先のパスを指定する必要があります。

//list[run][ベンチマークの実行][go]{
$ go version
go version go1.13 darwin/amd64

$ CGO_LDFLAGS="-L/usr/local/opt/openblas/lib -lopenblas" go test -bench=.
goos: darwin
goarch: amd64
pkg: github.com/po3rin/trygonum
BenchmarkOpenBLAS-12     	  726909	      1481 ns/op
BenchmarkGonumBLAS-12    	  448212	      2575 ns/op
PASS
//}

今回のケースでは1.7倍ほどOpenBLASの方が早くなっています。もちろん全パターンでOpenBLASが早い訳ではないので、適宜ベンチマークをとっていきましょう。

== まとめ

