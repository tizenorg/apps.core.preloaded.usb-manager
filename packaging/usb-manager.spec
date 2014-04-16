Name:       usb-server
Summary:    USB server
Version:    0.0.24
Release:    0
Group:      Application Framework/Utilities
License:    Apache-2.0
Source0:    usb-server-%{version}.tar.gz
Source1:    usb-server.manifest

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
BuildRequires:  pkgconfig(libtzplatform-config)

%description
Description: USB server


%prep
%setup -q

%if 0%{?simulator}
%cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix} -DSYS_CONF_DIR=%{TZ_SYS_ETC} -DSIMULATOR=yes
%else
%cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix} -DSYS_CONF_DIR=%{TZ_SYS_ETC} -DSIMULATOR=no
%endif


%build
cp %{SOURCE1} .
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

%post 
USER_GROUP_ID=$(getent group %{TZ_SYS_USER_GROUP} | cut -d: -f3)
vconftool set -t int memory/usb/mass_storage_status "0" -u 0 -i -f
vconftool set -t int memory/usb/accessory_status "0" -g $USER_GROUP_ID -i -f
vconftool set -t int memory/usb/cur_mode "0" -u 0 -i -f
vconftool set -t int db/usb/sel_mode "1" -f


%files
%manifest %{name}.manifest
%defattr(540,root,root,-)
%{_bindir}/start_dr.sh
%{_bindir}/usb-server
%attr(440,root,root) %{_datadir}/locale/*/LC_MESSAGES/usb-server.mo
%{_bindir}/direct_set_debug.sh
%{_bindir}/set_usb_debug.sh
%attr(440,root,%{TZ_SYS_USER_GROUP}) %{_datadir}/usb-server/data/usb_icon.png
