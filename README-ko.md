<div align="center">

<img src="https://raw.githubusercontent.com/c-krit/ferox/main/examples/res/images/logo.png" alt="c-krit/ferox"><br>

![version badge](https://img.shields.io/github/v/release/c-krit/ferox?include_prereleases)
![code-size badge](https://img.shields.io/github/languages/code-size/c-krit/ferox?color=brightgreen)
![license badge](https://img.shields.io/github/license/c-krit/ferox)

`ferox`는 C언어로 작성된 2차원 충돌 감지 및 물리 시뮬레이션 라이브러리입니다.

**이 프로젝트는 아직 개발 초기 단계에 있으므로 주의하여 사용하시기 바랍니다.**

[개발 문서](https://github.com/c-krit/ferox/wiki) &mdash;
[예제 파일](https://github.com/c-krit/ferox/tree/main/examples) &mdash;
[필수 조건](#필수-조건)

</div>

## 주요 기능

<img src="https://raw.githubusercontent.com/c-krit/ferox/main/examples/res/images/bricks.gif" width="640" alt="Bricks!">

**이 프로젝트는 제가 물리 엔진의 구조를 이해하기 위해 만들었으며, 새로운 기능이 자주 추가되지 않을 수도 있습니다.**

- Broad-phase collision detection with spatial hashing algorithm
- Narrow-phase collision detection with SAT (Separating Axis Theorem)
- 'Sequential Impulse' iterative constraint solver
- Semi-implicit (symplectic) Euler integrator
- Support for collision event callbacks

## 필수 조건

- GCC 9.4.0 이상의 버전
- GNU Make 4.1 이상의 버전
- Git 2.17.1 이상의 버전

```console
$ sudo apt install build-essential git
```

### 선택 조건

이 프로젝트에 포함된 예제 프로그램을 빌드하시려면 [raylib 4.0.0 이상의 버전](https://github.com/raysan5/raylib)이 필요합니다.

## 빌드 방법

이 프로젝트는 빌드 자동화를 위해 [GNU Make](https://www.gnu.org/software/make)를 사용합니다.

```console
$ git clone https://github.com/c-krit/ferox
$ cd ferox
$ make
```

아래 명령어를 사용하면 `raylib.h` 헤더 파일과 raylib에 의존하는 디버그 함수를 제외하고 프로젝트를 빌드할 수 있습니다.

```console
$ make BUILD=STANDALONE
```

### Windows 플랫폼을 대상으로 크로스 컴파일하기

`RAYLIB_PATH` 변수에 raylib의 저장소 경로를 입력하고 `TARGET_OS=WINDOWS`를 입력하여 컴파일하면 Windows 플랫폼을 대상으로 프로젝트를 빌드할 수 있습니다.

```console
$ make RAYLIB_PATH=../raylib TARGET_OS=WINDOWS
```

## 참고 문헌

- [Apple Inc. (2021), SKPhysicsBody | Apple Developer Documentation](https://developer.apple.com/documentation/spritekit/skphysicsbody)
- [Bostock, Mike. (2018, January 23), Sutherland–Hodgman Clipping](https://observablehq.com/@mbostock/sutherland-hodgman-clipping)
- [dyn4j. (2011, November 11), Contact Points Using Clipping](http://www.dyn4j.org/2011/11/contact-points-using-clipping)
- [dyn4j. (2010, January 01), SAT (Separating Axis Theorem)](http://dyn4j.org/2010/01/sat)
- [Catto, Erin. (2006), Fast and Simple Physics using Sequential Impulses](https://box2d.org/files/ErinCatto_SequentialImpulses_GDC2006.pdf)
- [Catto, Erin. (2005), Iterative Dynamics with Temporal Coherence](https://box2d.org/files/ErinCatto_IterativeDynamics_GDC2005.pdf)
- [Chou, Ming-Lun. (2014, January 7), Game Physics: Stability – Warm Starting](http://allenchou.net/2014/01/game-physics-stability-warm-starting/)
- [Coumans, Erwin. (2010, July 26), Collision Detection: Contact Generation and GPU Acceleration](https://sgvr.kaist.ac.kr/~sungeui/Collision_tutorial/Erwin.pdf)
- [Hastings, Erin & Mesit, Jaruwan. (2005). Optimization of large-scale, real-time simulations by spatial hashing](http://www.cs.ucf.edu/~jmesit/publications/scsc%202005.pdf)
- [Fiedler, Glenn. (2013, February 24), Collision Response and Coulomb Friction](https://gafferongames.com/post/collision_response_and_coulomb_friction)
- [Fiedler, Glenn. (2004, June 10), Fix Your Timestep!](https://gafferongames.com/post/fix_your_timestep)
- [Fiedler, Glenn. (2004, June 01), Integration Basics](https://gafferongames.com/post/integration_basics)
- [Gaul, Randy. (2013, April 06), How to Create a Custom Physics Engine](https://gamedevelopment.tutsplus.com/series/how-to-create-a-custom-physics-engine--gamedev-12715)
- [Gaul, Randy. (2013, July 16), Separating Axis Test (SAT) and Support Points in 2D](http://www.randygaul.net/wp-content/uploads/2013/07/SATandSupportPoints.pdf)
- [Gregorius, Dirk. (2013), The Separating Axis Test between Convex Polyhedra](https://storage.googleapis.com/google-code-archive-downloads/v2/code.google.com/box2d/DGregorius_GDC2013.zip)
- [Macdonald, Tristam. (2009, October 01), Spatial Hashing](https://www.gamedev.net/tutorials/programming/general-and-gameplay-programming/spatial-hashing-r2697/)
- [Manzke, Michael. (2016, February 22), Multiple Contact Resolution](https://www.scss.tcd.ie/~manzkem/CS7057/cs7057-1516-10-MultipleContacts-mm.pdf)
- [NVIDIA Corporation. (2017, May 12), NVIDIA PhysX 3.4.0 API Documentation](https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/guide/Manual/Index.html)
- [Rees, Gareth. (2019, February 09), How do you detect where two line segments intersect?](https://stackoverflow.com/a/565282)
- [Scratchapixel. (n.d.), A Minimal Ray-Tracer: Rendering Simple Shapes (Sphere, Cube, Disk, Plane, etc.)](https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection)
- [Tonge, Richard. (2013), Iterative Rigid Body Solvers](https://storage.googleapis.com/google-code-archive-downloads/v2/code.google.com/box2d/Tonge_Richard_PhysicsForGame.pdf)
- [Unity Technologies. (2018, April 04), Unity Manual: Physics Reference 2D](https://docs.unity3d.com/Manual/Physics2DReference.html)
- [Virtual Method Studio. (2017, November 21), Physics 101 #3: Solvers](http://blog.virtualmethodstudio.com/2017/11/physics-101-3-solvers)

## 라이선스

MIT 라이선스
