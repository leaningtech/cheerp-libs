Cheerp: A C++ compiler for the Web
==================================

Please report bugs on launchpad:
https://bugs.launchpad.net/cheerp

Cheerp GLES implementation installation
---------------------------------------

```
make -C webgles install INSTALL_PREFIX=/opt/cheerp CHEERP_PREFIX=/opt/cheerp
```

Cheerp PreExecuted standard C++ library installation (both generic and asm.js version)
----------------------------------------------------

```
make -C stdlibs install INSTALL_PREFIX=/opt/cheerp CHEERP_PREFIX=/opt/cheerp
```

Cheerp PreExecuted standard C++ library installation (only generic js version)
----------------------------------------------------

```
make -C stdlibs install_genericjs INSTALL_PREFIX=/opt/cheerp CHEERP_PREFIX=/opt/cheerp
```

Cheerp PreExecuted standard C++ library installation (only asm.js version)
----------------------------------------------------

```
make -C stdlibs install_asmjs INSTALL_PREFIX=/opt/cheerp CHEERP_PREFIX=/opt/cheerp
```

