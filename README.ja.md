sfq - Simple File-based Queue
===
���O�̒ʂ�V���v���ȃL���[�ł��B  
(�T�����������̂�������Ȃ�����) �풓�v���Z�X��c�a���K�v�Ȃ��A�v���Z�X/�X���b�h��r�����䂵�Ă����  
�s���̗ǂ����̂�������Ȃ������̂Ŏd���Ȃ����܂����B  
  
��ȖړI�� incrond �ŋN������v���Z�X�̃R���g���[���Ȃ̂ŁA�W���u�L���[�Ƃ��ė��p�ł��܂��B  
�V�F���Ŏ��s���Ă���R�}���h���L���[�ɓo�^����C���[�W�ł��B  


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

�ʏ�̃t�@�C���E�L���[�ł��Bpush ���� pop/shift ���܂��B  
�����͂��̎d�g�݂Ńf�[�^�̓o�^/�擾���s������ł������A�v���Z�X�̐����ɂ��čl����̂��ʓ|��  
�Ȃ����̂ŁA��q�� Type-2 �̋@�\�����܂����B  

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

�ŏ��̐����ɂ���悤�ɁA���s�������������L���[�ɓo�^(push) ����Ə������s�����悤�ɂȂ�܂��B  
"-B n" �Ƃ��ēn���Ă���̂̓W���u�L���[�̐��Ȃ̂ŁA�����v���Z�X�ɕ������Ď��s���ł��܂��B  

for example
* 1) rm -rf /var/tmp/noname (delete the queue that was made before)
* 2) sfqc-init -B 1
* 3) sfqc-pusht -v 'date > /tmp/aaa.txt' (you get to tell the current time in "/tmp/aaa.txt")
... text("date > /tmp/aaa.txt") is sent automatically to the standard input of "/bin/sh"

[Command Options]
...

[���C�u�����ɂ���]

�����o�b�`���痘�p���邱�Ƃ�z�肵�Ă����̂ŃR�}���h�����܂������A�����m������ php, java ����  
�Ăяo����悤�ɂ��܂����B  

eclipse �ł̃A�v���J�����ɂ��g���邩�Ǝv�����̂ŁA�����[�g�̃T�[�o�� xinetd �o�R�� push/pop �ł���  
�悤�ɂ����Ă��܂����A�Z�L�����e�B�Ƃ��l�����Ȃ����̂ɂȂ��Ă��܂��B

[���̑�]

�R�}���h���C���I�v�V���������낢�날��̂ł��̂��������܂����A�����̂���l������Β��ژA�����Ă��������B


## License
* MIT  
    * see MIT-LICENSE.txt

