Name:       usb-server
Summary:    USB server
Version:    0.0.24
Release:    1
Group:      framework/system
License:    Apache License, Version 2.0
Source0:    usb-server-%{version}.tar.gz
Source1:    usb-server.manifest
Source2:    usb-server.rule

BuildRequires:  cmake
BuildRequires:  libattr-devel
BuildRequires:  gettext-devel
BuildRequires:  pkgconfig(appcore-common)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(devman)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(syspopup-caller)
BuildRequires:  pkgconfig(pmapi)
BuildRequires:  pkgconfig(appsvc)
BuildRequires:  pkgconfig(libusb-1.0)
BuildRequires:  pkgconfig(notification)
BuildRequires:  pkgconfig(libudev)
BuildRequires:  pkgconfig(edbus)
BuildRequires:  pkgconfig(capi-system-usb-accessory)

%description
Description: USB server


%prep
%setup -q

%if 0%{?simulator}
cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix} -DSIMULATOR=yes
%else
cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix} -DSIMULATOR=no
%endif


%build
cp %{SOURCE1} .
cp %{SOURCE2} .
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

%post 

vconftool set -t int memory/usb/mass_storage_status "0" -u 0 -i -f
vconftool set -t int memory/usb/accessory_status "0" -u 5000 -i -f
vconftool set -t int memory/usb/cur_mode "0" -u 0 -i -f
vconftool set -t int db/usb/sel_mode "1" -f


%files
%manifest usb-server.manifest
%defattr(540,root,root,-)
/usr/bin/start_dr.sh
/usr/bin/usb-server
%attr(440,root,root) /usr/share/locale/*/LC_MESSAGES/usb-server.mo
/usr/bin/direct_set_debug.sh
/usr/bin/set_usb_debug.sh
%attr(440,app,app) /usr/share/usb-server/data/usb_icon.png
%attr(440,root,root) /opt/etc/smack/accesses.d/usb-server.rule
