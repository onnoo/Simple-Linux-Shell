### 4월 30일
~~~
UNIX는 모두 File로 관리한다.
redirection / pipe의 경우, stdin을 입력받고 close()와 dup()을 통해 선점할 수 있다.
History : File 관리 측면에서 생각
~~~

### 5월 2일
~~~
stdin에 내가 지정한 buffer보다 크게 들어오는 입력의 경우,
1024 byte 이상의 입력은 들어오지 않는다고 가정한다.
~~~

### 5월 7일
~~~
history 기능은 옵션없이 구현한다.
more, ln, la 등 명령어는 기존 bin 파일에 있는 프로그램으로 사용한다.
기존 프로그램을 사용하기 위해선 환경변수 PATH를 받아와야 함.
유용한 명령어 : sudo find / -name
책 속 예제들을 참고하면 수월하다.
>와 >|의 차이?
~~~