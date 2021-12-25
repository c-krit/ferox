<div align="center">

<img src="https://raw.githubusercontent.com/c-krit/ferox/main/examples/res/images/logo.png" alt="c-krit/ferox"><br>

[![version badge](https://img.shields.io/github/v/release/c-krit/ferox?include_prereleases)](https://github.com/c-krit/ferox/releases)
[![codefactor badge](https://www.codefactor.io/repository/github/c-krit/ferox/badge)](https://www.codefactor.io/repository/github/c-krit/ferox)
[![code-size badge](https://img.shields.io/github/languages/code-size/c-krit/ferox?color=brightgreen)](https://github.com/c-krit/ferox)
[![license badge](https://img.shields.io/github/license/c-krit/ferox)](https://github.com/c-krit/ferox/blob/main/LICENSE)

`ferox`는 C언어로 작성된 2차원 충돌 감지 및 물리 시뮬레이션 라이브러리입니다.

**이 프로젝트는 아직 개발 초기 단계에 있으므로 주의하여 사용하시기 바랍니다.**

[개발 문서](https://github.com/c-krit/ferox/wiki) &mdash;
**예제 파일** &mdash;
[필수 조건](#필수-조건)

</div>

## 예제 파일

이 디렉토리는 `ferox`의 예제 소스 파일이 저장되어 있는 곳입니다.

아래 명령어를 사용하면 예제 파일을 빌드할 수 있습니다 (Windows 플랫폼을 대상으로 빌드하려면 `make` 대신 `make TARGET_OS=WINDOWS`을 사용하세요):

```console
$ cd examples
$ make
```

---

### `arrow.c`

<img src="res/images/arrow.gif" width="640" alt="arrow.c">

### `bouncy.c`

<img src="res/images/bouncy.gif" width="640" alt="bouncy.c">

### `bricks.c`

<img src="res/images/bricks.gif" width="640" alt="bricks.c">

### `bullet.c`

<img src="res/images/bullet.gif" width="640" alt="bullet.c">

### `jump.c`

<img src="res/images/jump.gif" width="640" alt="jump.c">

### `laser.c`

<img src="res/images/laser.gif" width="640" alt="laser.c">

## License

MIT License