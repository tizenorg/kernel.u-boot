Name: u-boot
Version: 2014.01
Release: 1%{?dist}
Summary: Das U-Boot - Tizen bootloader
Group: System/Kernel
License: GPL-2.0+
ExclusiveArch: %{arm}
URL: https://review.tizen.org/git/?p=kernel/u-boot.git
Source0: %{name}-%{version}.tar.bz2
Source1001: u_boot.manifest

%description
u-boot - Tizen bootloader for Embedded boards based on ARM processor

#TODO: Describe rpm package information depending on board
%package -n u-boot-pkg
Summary: A bootloader for Embedded system
Group: System/Kernel

%description -n u-boot-pkg
A boot loader for embedded systems.
Das U-Boot is a cross-platform bootloader for embedded systems,
used as the default boot loader by several board vendors.  It is
intended to be easy to port and to debug, and runs on many
supported architectures, including PPC, ARM, MIPS, x86, m68k, NIOS,
and Microblaze.

%package -n u-boot-tools
Summary: Companion tools for Das U-Boot bootloader
Group: System/Kernel

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

make mrproper

# Set configuration
make trats2_config

# Build tools
make HOSTCC="gcc $RPM_OPT_FLAGS" HOSTSTRIP=/bin/true tools

%if 1%{?use_mmc_storage}
make HOSTCC="gcc $RPM_OPT_FLAGS" CONFIG_ENV_IS_IN_MMC=y env
%else
make HOSTCC="gcc $RPM_OPT_FLAGS" env
%endif

# Build u-boot
make EXTRAVERSION=`echo %{vcs} | sed 's/.*u-boot#\(.\{9\}\).*/-g\1-TIZEN.org/'`

%install
rm -rf %{buildroot}

# Tools installation
mkdir -p %{buildroot}%{_bindir}
install -p -m 0755 tools/mkimage %{buildroot}%{_bindir}
install -p -m 0755 tools/env/fw_printenv %{buildroot}%{_bindir}
( cd %{buildroot}%{_bindir}; ln -sf fw_printenv fw_setenv )

# u-boot installation
install -d %{buildroot}/boot/u-boot
install -m 755 u-boot.bin %{buildroot}/boot/u-boot
install -m 755 u-boot-mmc.bin %{buildroot}/boot/u-boot

%clean

%files
%manifest u_boot.manifest
%defattr(-,root,root,-)
/boot/u-boot/

%files -n u-boot-tools
%manifest u_boot.manifest
%defattr(-,root,root,-)
%{_bindir}/mkimage
%{_bindir}/fw_printenv
%{_bindir}/fw_setenv
