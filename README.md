Cheerp: A C++ compiler for the Web
==================================

Please report bugs on GitHub:
https://github.com/leaningtech/cheerp-meta/issues

Cheerp GLES implementation installation
---------------------------------------

```
make -C webgles install INSTALL_PREFIX=/opt/cheerp CHEERP_PREFIX=/opt/cheerp
```

Cheerp helper library for Wasm
---------------------------------------

```
make -C wasm install INSTALL_PREFIX=/opt/cheerp CHEERP_PREFIX=/opt/cheerp
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

Cheerp syscall library installation (only generic js version)
---------------------------------------

```
cd system
cmake -B build_genericjs -DCMAKE_INSTALL_PREFIX=/opt/cheerp -DCMAKE_TOOLCHAIN_FILE=/opt/cheerp/share/cmake/Modules/CheerpToolchain.cmake .
make
make install
cd ..
```

Cheerp syscall library installation (only asm.js version)
---------------------------------------

```
cmake -B build_asmjs -DCMAKE_INSTALL_PREFIX=/opt/cheerp -DCMAKE_TOOLCHAIN_FILE=/opt/cheerp/share/cmake/Modules/CheerpWasmToolchain.cmake .
make
make install
cd ..
```
