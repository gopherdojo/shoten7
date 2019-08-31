# shoten7
[![CircleCI](https://circleci.com/gh/gopherdojo/shoten7.svg?style=svg)][circleci]

[circleci]:https://circleci.com/gh/gopherdojo/shoten7

このリポジトリはRe:VIEWを使って技術書典7用の文書を書くリポジトリです。Re:VIEWバージョン3.0を使っています。


 * [B5紙面サンプル（PDF）](https://github.com/TechBooster/ReVIEW-Template/tree/master/pdf-sample/TechBooster-Template-B5.pdf)
 * [A5紙面サンプル（PDF）](https://github.com/TechBooster/ReVIEW-Template/tree/master/pdf-sample/TechBooster-Template-A5.pdf)
 * [B5紙面電子書籍サンプル（PDF）](https://github.com/TechBooster/ReVIEW-Template/tree/master/pdf-sample/TechBooster-Template-ebook.pdf)

## 使い方は？
提供されている一般的な使い方は[テンプレートの使い方](./TEMPLATE_README.md)を参照してください。


### このリポジトリだけの使い方
`make`コマンドがあれば以下のコマンドを利用できます。


```bash
lint            Execute textlint
fixlint         Fix textlint error
build           Build PDF in Docker
```

また、エディタにtextlintに対応したプラグインがあればコマンドを実行しなくてもtextlintが表示できます。

- VSCode: [テキスト校正くん - Visual Studio Marketplace](https://marketplace.visualstudio.com/items?itemName=ICS.japanese-proofreading)

PRを作成すると、GoogleドライブにビルドしたPDFが保存されます。

https://drive.google.com/drive/folders/1VZJKEebgRK7yNZrtlHrSw_q0fewCdF4T

アクセス権限がない場合はSlackで@tenntennまでお知らせください。権限付与します。


## Lintのルールがおかしい・この用語はLintで怒られないようにしてほしい
校正の補助としてtextlintを使っています。ルールの調整は以下のファイルを編集してください。

- [.textlintrc](https://github.com/gopherdojo/shoten7/blob/master/.textlintrc)

単語や正規表現を用いた許可ルールの追加は以下のファイルを編集してください。


- [allow.yml](https://github.com/gopherdojo/shoten7/blob/master/allow.yml)

どちらかのファイルを編集した際は、その変更だけ先に`master`ブランチに入れておいてもらえると、他の人も同じルールで執筆ができます。


