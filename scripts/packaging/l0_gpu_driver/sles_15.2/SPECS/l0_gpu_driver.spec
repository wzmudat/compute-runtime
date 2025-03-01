# spec file for package intel-level-zero-gpu

#it's changed by external script
%global ver xxx
%global rel xxx
%global build_id xxx

%define gmmlib_sover  11
%define igc_sover 1

%if !0%{?build_type:1}
%define build_type Release
%endif

%define _source_payload w5T16.xzdio
%define _binary_payload w5T16.xzdio

Name: intel-level-zero-gpu
Version: %{ver}
Release: %{rel}%{?dist}
Summary: Intel(R) GPU Driver for oneAPI Level Zero.

Group: System Environment/Libraries
License: MIT
URL: https://github.com/intel/compute-runtime
Source0: %{url}/archive/%{version}/compute-runtime-%{version}.tar.xz
Source1: copyright

ExclusiveArch:  x86_64
BuildRequires:  cmake make gcc-c++
#BuildRequires:  libva-devel
BuildRequires:  libigdgmm%{?name_suffix}-devel
BuildRequires:  libigdfcl%{?name_suffix}-devel

Requires:       libigc%{igc_sover}%{?name_suffix}
Requires:       libigdfcl%{igc_sover}%{?name_suffix}
Requires:       libigdgmm%{gmmlib_sover}%{?name_suffix}

%description
Runtime library providing the ability to use Intel GPUs with the oneAPI Level
Zero programming interface.
Level Zero is the primary low-level interface for language and runtime
libraries. Level Zero offers fine-grain control over accelerators capabilities,
delivering a simplified and low-latency interface to hardware, and efficiently
exposing hardware capabilities to applications.

%debug_package

%prep
%autosetup -p1 -n compute-runtime-%{version}

%build
%cmake .. \
   -DNEO_VERSION_BUILD=%{build_id} \
   -DCMAKE_BUILD_TYPE=%{build_type} \
   -DCMAKE_INSTALL_PREFIX=/usr \
   -DSKIP_UNIT_TESTS=TRUE \
   -DRELEASE_WITH_REGKEYS=TRUE \
   -DL0_INSTALL_UDEV_RULES=1 \
   -DUDEV_RULES_DIR=/etc/udev/rules.d/ \
   -DCMAKE_VERBOSE_MAKEFILE:BOOL=OFF \
   -Wno-dev
%make_build

%install
cd build
%make_install

#Remove OpenCL files before installing
rm -rf %{buildroot}%{_libdir}/intel-opencl/
rm -rf %{buildroot}%{_sysconfdir}/OpenCL/
rm -rf %{buildroot}%{_bindir}/ocloc
rm -rf %{buildroot}%{_libdir}/libocloc.so
rm -rf %{buildroot}%{_includedir}/ocloc_api.h
#Remove debug files
rm -f %{buildroot}/%{_libdir}/intel-opencl/libigdrcl.so.debug
rm -f %{buildroot}/%{_libdir}/libocloc.so.debug
rm -rf %{buildroot}/usr/lib/debug/
#insert license into package
mkdir -p %{buildroot}/usr/share/doc/intel-level-zero-gpu%{?name_suffix}/
cp -pR %{_sourcedir}/copyright %{buildroot}/usr/share/doc/intel-level-zero-gpu%{?name_suffix}/.

%files -n intel-level-zero-gpu%{?name_suffix}
%defattr(-,root,root)
%{_libdir}/libze_intel_gpu.so.*
%{_sharedstatedir}/libze_intel_gpu/pci_bind_status_file
%{_sharedstatedir}/libze_intel_gpu/wedged_file
%{_sysconfdir}/udev/rules.d/99-drm_ze_intel_gpu.rules
/usr/share/doc/intel-level-zero-gpu%{?name_suffix}/copyright
%config(noreplace)

%doc

%changelog
* Tue Apr 28 2020 Jacek Danecki <jacek.danecki@intel.com> - 0.8.16582-1
- Update to 0.8.16582

* Tue Mar 24 2020 Spruit, Neil R <neil.r.spruit@intel.com> - 0.8.0
* Fri Mar 13 2020 Pavel Androniychuk <pavel.androniychuk@intel.com> - 0.4.1
- Spec file init
