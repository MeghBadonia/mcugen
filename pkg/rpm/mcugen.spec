Name:           mcugen
Version:        3.0.0
Release:        1%{?dist}
Summary:        Material Color Utilities Generator
License:        MIT
URL:            https://github.com/MeghBadonia/mcugen

%description
Generates complete Material You color schemes from any image or seed color
and applies them to template files using a simple token syntax.

%install
mkdir -p %{buildroot}%{_bindir}
install -m 755 %{_sourcedir}/mcugen %{buildroot}%{_bindir}/mcugen

%files
%{_bindir}/mcugen

%changelog
* Tue Jun 24 2026 Megh Badonia <badoniamegh@gmail.com> - 3.0.0-1
- v3.0.0: modular refactor, check command, named profiles, ANSI swatches, blend, export
