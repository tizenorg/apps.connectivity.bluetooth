%define PREFIX /usr/apps/org.tizen.bluetooth
Name: org.tizen.bluetooth
Version:    0.0.20
Release:    0
Summary: Tizen W BT connect error application
URL: http://slp-source.sec.samsung.net
Source: %{name}-%{version}.tar.gz
License: Flora Software License, Version 1.1
Group: Samsung/Application

BuildRequires: cmake
BuildRequires: gettext-tools
BuildRequires: efl-assist-devel
BuildRequires: pkgconfig(embryo)
BuildRequires: pkgconfig(ecore)
BuildRequires: pkgconfig(elementary)
BuildRequires: pkgconfig(appcore-efl)
BuildRequires: pkgconfig(appcore-common)
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(capi-appfw-application)
BuildRequires: pkgconfig(vconf)
BuildRequires: pkgconfig(syspopup-caller)
BuildRequires: pkgconfig(capi-network-bluetooth)
BuildRequires: pkgconfig(dbus-glib-1)
BuildRequires: pkgconfig(utilX)
BuildRequires: pkgconfig(deviced)
#BUildRequires: pkgconfig()

BuildRequires: efl-assist-devel
BuildRequires: embryo-bin

%description
W bt connect error popup application

%prep
%setup -q

%build
export CFLAGS="${CFLAGS} -fPIC -fvisibility=hidden"
#CFLAGS+=" -fvisibility=hidden"; export CFLAGS
CXXFLAGS+=" -fvisibility=hidden"; export CXXFLAGS
FFLAGS+=" -fvisibility=hidden"; export FFLAGS

%if 0%{?tizen_build_binary_release_type_eng}
export CFLAGS+=" -DTIZEN_ENGINEER_MODE"
export CXXFLAGS+=" -DTIZEN_ENGINEER_MODE"
export FFLAGS+=" -DTIZEN_ENGINEER_MODE"
%endif

LDFLAGS+="-Wl,--rpath=%{PREFIX}/lib -Wl,--as-needed -Wl,--hash-style=both"; export LDFLAGS
cmake . -DCMAKE_INSTALL_PREFIX=%{PREFIX}
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}

%make_install
install -D -m 0644 LICENSE.Flora %{buildroot}%{_datadir}/license/org.tizen.bluetooth
mkdir -p %{buildroot}/opt/usr/apps/org.tizen.bluetooth

%post
/sbin/ldconfig

%files
%manifest org.tizen.bluetooth.manifest
/etc/smack/accesses2.d/org.tizen.bluetooth.rule

%defattr(-,root,root,-)
%attr(-,inhouse,inhouse)
%{PREFIX}/bin/*
%{PREFIX}/res/images/*.png
/usr/share/packages/*
#/usr/share/icons/default/small/*
/usr/share/packages/%{name}.xml
%{_datadir}/license/org.tizen.bluetooth
