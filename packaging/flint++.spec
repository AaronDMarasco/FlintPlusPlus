Name: flint++
Version: %{RPM_VERSION}
Release: 1%{?COMMIT_TAG}%{?dist}
Summary: Flint++ is cross-platform lint program for C++
License: Boost
Packager: Aaron D. Marasco <github-flintplusplus@marascos.net>

BuildRequires: coreutils gcc-c++ make
Source: %{name}.tar

%description
Flint++ is cross-platform, zero-dependency port of flint, a lint program for C++ developed and used at Facebook.

%{?RPM_HASH:ReleaseID: %{RPM_HASH}}
%prep
%setup -q

%build
%set_build_flags
cd flint
%make_build

%check
cd flint
%{__make} check

%install
%{__install} -m 644 -D --target-directory %{buildroot}/%{_mandir}/man1/ %{name}.1
cd flint
%make_install
# tree -a %%{buildroot}

%files
%{_bindir}/%{name}
%license LICENSE
%{_mandir}/man1/%{name}.1*
%doc README.md "The Lint API Guide.md"
