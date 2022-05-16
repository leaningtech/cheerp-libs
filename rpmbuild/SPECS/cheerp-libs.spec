Name: cheerp-libs
Version: 2.5
Release:        1%{?dist}
Summary: A C++ compiler for the Web, libraries

License:  GPLv2
URL: https://leaningtech.com/cheerp
Source0: %{NAME}_%{VERSION}.orig.tar.gz

BuildRequires: cmake make cheerp-llvm-clang = %{VERSION} cheerp-utils = %{VERSION} cheerp-musl = %{VERSION} cheerp-libcxx-libcxxabi = %{VERSION}
Requires: cheerp-llvm-clang = %{VERSION} cheerp-utils = %{VERSION} cheerp-musl = %{VERSION} cheerp-libcxx-libcxxabi = %{VERSION}

%description
Cheerp is a tool to bring C++ programming to the Web. It can generate a seamless
combination of JavaScript, WebAssembly and Asm.js from a single C++ codebase.

%define debug_package %{nil}

%prep
%autosetup

cmake -S system -B system/build_genericjs -DCMAKE_INSTALL_PREFIX=%{buildroot}/opt/cheerp -DCMAKE_TOOLCHAIN_FILE=/opt/cheerp/share/cmake/Modules/CheerpToolchain.cmake .
cmake -S system -B system/build_asmjs -DCMAKE_INSTALL_PREFIX=%{buildroot}/opt/cheerp -DCMAKE_TOOLCHAIN_FILE=/opt/cheerp/share/cmake/Modules/CheerpWasmToolchain.cmake .

%build
make -C webgles CHEERP_PREFIX=/opt/cheerp
make -C wasm CHEERP_PREFIX=/opt/cheerp
make -C stdlibs CHEERP_PREFIX=/opt/cheerp
make -C system/build_genericjs CHEERP_PREFIX=/opt/cheerp
make -C system/build_asmjs CHEERP_PREFIX=/opt/cheerp

%install
mkdir -p %{buildroot}/opt/cheerp/lib/genericjs
mkdir -p %{buildroot}/opt/cheerp/lib/asmjs
make -C webgles install INSTALL_PREFIX=%{buildroot}/opt/cheerp CHEERP_PREFIX=/opt/cheerp
make -C wasm install INSTALL_PREFIX=%{buildroot}/opt/cheerp CHEERP_PREFIX=/opt/cheerp
make -C stdlibs install INSTALL_PREFIX=%{buildroot}/opt/cheerp CHEERP_PREFIX=/opt/cheerp
make -C system/build_genericjs install
make -C system/build_asmjs install

%clean
rm -rf $RPM_BUILD_ROOT

%files
/opt/cheerp/

%changelog
* Tue Dec 10 2019 Yuri Iozzelli <yuri@leaningtech.com>
- First RPM version
