Name: u-boot
Version: 1.3.4
Release: 0
Summary: bootloader for Embedded boards based on ARM processor
Group: System Environment/Kernel
License: GPL
ExclusiveArch: %{arm}
URL: http://sourceforge.net/projects/u-boot
Source0: %{name}-%{version}.tar.bz2
Source1001: packaging/u-boot.manifest 

%description
bootloader for Embedded boards based on ARM processor

%package -n u-boot-trats2
Summary: A bootloader for Embedded system
Group: System Environment/Kernel

%description -n u-boot-trats2
A boot loader for embedded systems.
Das U-Boot is a cross-platform bootloader for embedded systems,
used as the default boot loader by several board vendors.  It is
intended to be easy to port and to debug, and runs on many
supported architectures, including PPC, ARM, MIPS, x86, m68k, NIOS,
and Microblaze.

%package -n u-boot-tools
Summary: Companion tools for Das U-Boot bootloader
Group: System Environment/Kernel

%description -n u-boot-tools
This package includes the mkimage program, which allows generation of U-Boot
images in various formats, and the fw_printenv and fw_setenv programs to read
and modify U-Boot's environment.


%ifarch %{arm}
%global use_mmc_storage 1
%endif


%prep
%setup -q

%build
cp %{SOURCE1001} .
make distclean
make exynos_trats2_config

make %{?_smp_mflags} HOSTCC="gcc $RPM_OPT_FLAGS" HOSTSTRIP=/bin/true tools

%if 1%{?use_mmc_storage}
make %{?_smp_mflags} HOSTCC="gcc $RPM_OPT_FLAGS" CONFIG_ENV_IS_IN_MMC=y env
%else
make %{?_smp_mflags} HOSTCC="gcc $RPM_OPT_FLAGS" env
%endif

make %{?_smp_mflags} EXTRAVERSION=`echo %{vcs} | sed 's/.*u-boot.*#\(.\{9\}\).*/-g\1-TIZEN.org/'`

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}%{_bindir}
install -p -m 0755 tools/mkimage %{buildroot}%{_bindir}
install -p -m 0755 tools/env/fw_printenv %{buildroot}%{_bindir}
( cd %{buildroot}%{_bindir}; ln -sf fw_printenv fw_setenv )

# u-boot installation
mkdir -p %{buildroot}/var/tmp/u-boot
install -d %{buildroot}/var/tmp/u-boot
install -m 755 u-boot.bin %{buildroot}/var/tmp/u-boot
install -m 755 u-boot-mmc.bin %{buildroot}/var/tmp/u-boot
install -m 755 params.bin %{buildroot}/var/tmp/u-boot

%clean

%files -n u-boot-trats2
%manifest u_boot.manifest
%defattr(-,root,root,-)
/var/tmp/u-boot

%files -n u-boot-tools
%manifest u-boot.manifest
%defattr(-,root,root,-)
%{_bindir}/mkimage
%{_bindir}/fw_printenv
%{_bindir}/fw_setenv
