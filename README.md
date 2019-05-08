# Simple-Linux-Shell
2019 CSE4009 System Programming Term Project

### A Simple Linux Shell
본 설계 프로젝트에서는 간단한 리눅스 명령어 해석기(일명: smsh)를 스스로 만들어 본다.

설계 및 구현해야 할 smsh는 다음과 같은 명령어를 처리할 수 있어야 한다.

* foreground and background execution (&)
* multiple commands separated by semicolons
* history command
* shell redirection (>, >>, >|, <)
* shell pipe (ls –la | more)
* Multiple pipe (ls | grep “^d” | more)
* cd command


함께 제공되는 Main loop 파일을 참조할 것

### Redirection example (>과 >|의 차이점)
~~~
>는 덮어쓰기가 불가능하게 설정되어 있다면, 오류를 반환합니다.
>|는 덮어쓰기가 불가능하게 설정되어 있더라도 해당 파일을 덮어쓸 수 있습니다.
"set -o noclobber"는 덮어쓰기를 금지하는 명령어 입니다.
"set -o noclobber"로 설정어 있다고 가정하고 구현하시면 됩니다.
~~~
