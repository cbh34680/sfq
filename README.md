sfq
===

Simple File-based Queue

[Development Environment]
* OS) CentOS 7 64bit

[Build]
* 1) git clone (get source tree)
* 2) make (use sfq/Makefile)

[Binaries]
* Library) sfq/lib/lib*.so
* Exe) sfq/bin/sfqc-*

[Set Path & DLL-Path]
* export LD_LIBRARY_PATH="** sfq-dir **/lib:${LD_LIBRARY_PATH}"
* export PATH="** sfq-dir **/bin:${PATH}"

[Run) Queue Type-1]

Queue is static. you can add data (by sfqc-pushc command) and you can get data (by sfqc-shift command)

for example
* 1) sfqc-init ("noname" directory is made in "/var/tmp")
* 2) sfqc-pushc -t aaa
* 3) sfqc-pushc -t bbb
* 4) sfqc-pushc -t ccc (added 3 record)
* 5) sfqc-list (print records)
* 6) sfqc-shift (you got "aaa")
* 7) sfqc-pop (you got "ccc")
* 8) sfqc-clear (clear all records)

[Run) Queue Type-2]

Queue is dynamic. you can add data (by command) but data is automatically retrieved

for example
* 1) rm -rf /var/tmp/noname (delete the queue that was made before)
* 2) sfqc-init -P 1
* 3) sfqc-pushc -t 'date > /tmp/aaa.txt' (you get to tell the current time in "/tmp/aaa.txt")
... text("date > /tmp/aaa.txt") is sent automatically to the standard input of "/bin/sh"

[Command Options]
*
*
*

## License
* MIT  
    * see LICENSE

