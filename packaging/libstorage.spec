Name:       libstorage
Summary:    Library to get storage information
Version:    0.1.0
Release:    0
Group:      System/Libraries
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1:    %{name}.manifest
BuildRequires:  cmake
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(capi-base-common)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(libtzplatform-config)

%description
development package of library to get storage

%package devel
Summary:	Get storage information (devel)
Group:		Development/Libraries
Requires:	%{name} = %{version}-%{release}

%description devel
Library to get storage information (devel)


%prep
%setup -q
cp %{SOURCE1} .

%build
%cmake . \
		-DTZ_SYS_RO_APP=%{TZ_SYS_RO_APP} \
		-DTZ_SYS_RO_ETC=%{TZ_SYS_RO_ETC} \
		-DTZ_SYS_MEDIA=%{TZ_SYS_MEDIA}

make %{?jobs:-j%jobs}

%install
%make_install

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%{_libdir}/*.so.*
%{_sysconfdir}/storage/libstorage.conf
%license LICENSE
%manifest %{name}.manifest

%files devel
%{_includedir}/storage/*.h
%{_libdir}/*.so
%{_libdir}/pkgconfig/*.pc
