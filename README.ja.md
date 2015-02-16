sfq - Simple File-based Queue
===
名前の通りシンプルなキューです。  
(探し方が悪いのかもしれないけど) 常駐プロセスやＤＢが必要なく、プロセス/スレッドを排他制御してくれる  
都合の良いものが見つからなかったので仕方なく作りました。  
  
主な目的は incrond で起動するプロセスのコントロールなので、ジョブキューとして利用できます。  
シェルで実行しているコマンドをキューに登録するイメージです。  
  

![how to use](http://sfq.iret.co.jp/sfq-how-to-use.png)
  

[Development Environment]
* OS) CentOS 6/7 64bit

[Build]
* 1) yum -y install jansson-devel libcap-devel libuuid-devel
* 2) git clone (get source tree)
* 3) make (use sfq/Makefile)

[Binaries]
* Library) sfq/lib/lib*.so
* Exe) sfq/bin/sfqc-*

[Set Path & DLL-Path]
* export LD_LIBRARY_PATH="** sfq-dir **/lib:${LD_LIBRARY_PATH}"
* export PATH="** sfq-dir **/bin:${PATH}"

[Run) Queue Type-1]

通常のファイル・キューです。push して pop/shift します。  
当初はこの仕組みでデータの登録/取得を行うつもりでしたが、プロセスの生成について考えるのが面倒に  
なったので、後述の Type-2 の機能を入れました。  

for example
* 1) sfqc-init ("noname" directory is made in "/var/tmp")
* 2) sfqc-pusht -v aaa
* 3) sfqc-pusht -v bbb
* 4) sfqc-pusht -v ccc (added 3 record)
* 5) sfqc-list (print records)
* 6) sfqc-shift (you got "aaa")
* 7) sfqc-pop (you got "ccc")
* 8) sfqc-clear (clear all records)

[Run) Queue Type-2]

最初の説明にあるように、実行したい処理をキューに登録(push) すると順次実行されるようになります。  
"-B n" として渡しているのはジョブキューの数なので、複数プロセスに分割して実行もできます。  

for example
* 1) rm -rf /var/tmp/noname (delete the queue that was made before)
* 2) sfqc-init -B 1
* 3) sfqc-pusht -v 'date > /tmp/aaa.txt' (you get to tell the current time in "/tmp/aaa.txt")
... text("date > /tmp/aaa.txt") is sent automatically to the standard input of "/bin/sh"

[Command Options]
...

[ライブラリについて]

当初バッチから利用することを想定していたのでコマンドを作りましたが、ワルノリして php, java から  
呼び出せるようにしました。  

eclipse でのアプリ開発時にも使えるかと思ったので、リモートのサーバに xinetd 経由で push/pop できる  
ようにもしていますが、セキュリティとか考慮しないものになっています。

[その他]

コマンドラインオプションもいろいろあるのでそのうち書きますが、興味のある人がいれば直接連絡してください。


## License
* MIT  
    * see MIT-LICENSE.txt

