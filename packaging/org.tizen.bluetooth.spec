%define PREFIX /usr/apps/org.tizen.bluetooth
Name: org.tizen.bluetooth
Version:    0.0.45
Release:    0
Summary: Tizen W BT headset connection application
URL: http://slp-source.sec.samsung.net
Source: %{name}-%{version}.tar.gz
License: Flora-1.1
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
BuildRequires: pkgconfig(edje)
#BUildRequires: pkgconfig()

BuildRequires: efl-assist-devel
BuildRequires: edje-bin, embryo-bin

%description
W BT headset connection application

%prep
%setup -q

%build
export CFLAGS="${CFLAGS} -fPIC -fvisibility=hidden"
#CFLAGS+=" -fvisibility=hidden"; export CFLAGS
CXXFLAGS+=" -fvisibility=hidden"; export CXXFLAGS
FFLAGS+=" -fvisibility=hidden"; export FFLAGS

#%if 0%{?tizen_build_binary_release_type_eng}
export CFLAGS+=" -DTIZEN_ENGINEER_MODE"
export CXXFLAGS+=" -DTIZEN_ENGINEER_MODE"
export FFLAGS+=" -DTIZEN_ENGINEER_MODE"
#%endif

export CFLAGS+=" -DTELEPHONY_DISABLED"
export CXXFLAGS+=" -DTELEPHONY_DISABLED"
export FFLAGS+=" -DTELEPHONY_DISABLED"

%cmake \
	-DTELEPHONY_DISABLED=YES \

LDFLAGS+="-Wl,--rpath=%{PREFIX}/lib -Wl,--as-needed -Wl,--hash-style=both"; export LDFLAGS
cmake . -DCMAKE_INSTALL_PREFIX=%{PREFIX}
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}

%make_install
mkdir -p %{buildroot}/opt/usr/apps/org.tizen.bluetooth
install -D -m 0644 LICENSE %{buildroot}%{_datadir}/license/org.tizen.bluetooth

%post
/sbin/ldconfig
/usr/bin/signing-client/app-sign.sh /usr/apps/org.tizen.bluetooth

%files
%manifest org.tizen.bluetooth.manifest
/etc/smack/accesses.d/org.tizen.bluetooth.efl

%defattr(-,root,root,-)
%attr(-,inhouse,inhouse)
%{PREFIX}/bin/*
%{PREFIX}/res/*
/usr/share/packages/*
#/usr/share/icons/default/small/*
/usr/share/packages/%{name}.xml
%{_datadir}/license/org.tizen.bluetooth
/usr/apps/org.tizen.bluetooth/shared/res/locale
/usr/apps/org.tizen.bluetooth/shared/res/tables/org.tizen.bluetooth_ChangeableColorTable.xml
/usr/apps/org.tizen.bluetooth/shared/res/tables/org.tizen.bluetooth_FontInfoTable.xml

