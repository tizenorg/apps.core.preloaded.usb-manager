Name:       usb-server
Summary:    USB server
Version:    0.0.11
Release:    1
Group:      framework-system
License:    APLv2
Source0:    usb-server-%{version}.tar.gz
Source1:    usb-server.manifest

BuildRequires:  cmake
BuildRequires:  libattr-devel
BuildRequires:  gettext-devel
BuildRequires:  pkgconfig(appcore-common)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(heynoti)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(devman)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(syspopup-caller)
BuildRequires:  pkgconfig(pmapi)
BuildRequires:  pkgconfig(appsvc)
BuildRequires:  pkgconfig(libusb-1.0)
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
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

%post 

vconftool set -t int memory/usb/mass_storage_status "0" -u 0 -i -f
vconftool set -t int memory/usb/accessory_status "0" -u 5000 -i -f
vconftool set -t int db/usb/keep_ethernet "0" -f
vconftool set -t int memory/usb/libusb_status "0" -f

heynotitool set device_usb_accessory

mkdir -p /etc/udev/rules.d
if ! [ -L /etc/udev/rules.d/91-usb-server.rules ]; then
	ln -s /usr/share/usb-server/udev-rules/91-usb-server.rules /etc/udev/rules.d/91-usb-server.rules
fi


%files
%manifest usb-server.manifest
%defattr(540,root,root,-)
/usr/bin/start_dr.sh
/usr/bin/usb-server
%attr(440,root,root) /usr/share/locale/*/LC_MESSAGES/usb-server.mo
%attr(440,root,root) /usr/share/usb-server/udev-rules/91-usb-server.rules
/usr/bin/set_usb_debug.sh
