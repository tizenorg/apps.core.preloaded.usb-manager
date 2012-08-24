Name:       usb-server
Summary:    USB server
Version: 	0.0.1
Release:    5
Group:      TO_BE/FILLED_IN
License:    TO_BE/FILLED_IN
Source0:    usb-server-%{version}.tar.gz

BuildRequires:  cmake
BuildRequires:  libattr-devel
BuildRequires:  gettext-devel
BuildRequires:  pkgconfig(appcore-common)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(heynoti)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(devman)
BuildRequires:  pkgconfig(iniparser)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(syspopup-caller)
BuildRequires:  pkgconfig(appsvc)

%description
Description: USB server


%prep
%setup -q
cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix}

%build
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install


%post 

vconftool set -t int memory/usb/mass_storage_status "0" -u 0 -i -f
vconftool set -t int memory/usb/accessory_status "0" -u 5000 -i -f

heynotitool set device_usb_accessory

mkdir -p /etc/udev/rules.d
if ! [ -L /etc/udev/rules.d/91-usb-server.rules ]; then
	ln -s /usr/share/usb-server/udev-rules/91-usb-server.rules /etc/udev/rules.d/91-usb-server.rules
fi


%files
%defattr(540,root,root,-)
/usr/bin/start_dr.sh
/usr/bin/usb-server
%attr(440,root,root) /usr/share/locale/*/LC_MESSAGES/usb-server.mo
%attr(440,root,root) /usr/share/usb-server/udev-rules/91-usb-server.rules
