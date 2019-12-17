Name: cheerp-libs
Version: 2.0
Release:        1%{?dist}
Summary: A C++ compiler for the Web, libraries

License:  GPLv2
URL: https://leaningtech.com/cheerp
Source0: %{NAME}-%{VERSION}.tar.gz

BuildRequires: cmake make cheerp-llvm-clang = %{VERSION} cheerp-utils = %{VERSION} cheerp-newlib = %{VERSION} cheerp-libcxx = %{VERSION} cheerp-libcxxabi = %{VERSION}
Requires: cheerp-llvm-clang = %{VERSION} cheerp-utils = %{VERSION} cheerp-newlib = %{VERSION} cheerp-libcxx = %{VERSION} cheerp-libcxxabi = %{VERSION}

%description
Cheerp is a tool to bring C++ programming to the Web. It can generate a seamless
combination of JavaScript, WebAssembly and Asm.js from a single C++ codebase.

%define debug_package %{nil}

%prep
%autosetup


%build
make -C webgles CHEERP_PREFIX=/opt/cheerp
make -C wasm CHEERP_PREFIX=/opt/cheerp
make -C stdlibs CHEERP_PREFIX=/opt/cheerp

%install
mkdir -p %{buildroot}/opt/cheerp/lib/genericjs
mkdir -p %{buildroot}/opt/cheerp/lib/asmjs
make -C webgles install INSTALL_PREFIX=%{buildroot}/opt/cheerp CHEERP_PREFIX=/opt/cheerp
make -C wasm install INSTALL_PREFIX=%{buildroot}/opt/cheerp CHEERP_PREFIX=/opt/cheerp
make -C stdlibs install INSTALL_PREFIX=%{buildroot}/opt/cheerp CHEERP_PREFIX=/opt/cheerp

%clean
rm -rf $RPM_BUILD_ROOT

%files
/opt/cheerp/

%changelog
* Tue Dec 10 2019 Yuri Iozzelli <yuri@leaningtech.com>
- First RPM version
