#!/usr/bin/env bash
set -euo pipefail

VERSION="2.0.0"
ARCH="x86_64"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DIST="$ROOT/dist"

mkdir -p "$DIST"

echo "==> Building binary..."
cd "$ROOT"
make clean
make
BINARY="$ROOT/build/mcugen"

# ── .deb ─────────────────────────────────────────────────────────────────────
echo "==> Building .deb..."
DEB_STAGE="$(mktemp -d)"
mkdir -p "$DEB_STAGE/DEBIAN" "$DEB_STAGE/usr/bin"
cp "$ROOT/pkg/deb/DEBIAN/control" "$DEB_STAGE/DEBIAN/control"
install -m755 "$BINARY" "$DEB_STAGE/usr/bin/mcugen"
dpkg-deb --build --root-owner-group "$DEB_STAGE" "$DIST/mcugen_${VERSION}_amd64.deb"
rm -rf "$DEB_STAGE"
echo "    -> $DIST/mcugen_${VERSION}_amd64.deb"

# ── .rpm ─────────────────────────────────────────────────────────────────────
echo "==> Building .rpm..."
RPM_ROOT="$(mktemp -d)"
mkdir -p "$RPM_ROOT"/{BUILD,RPMS,SOURCES,SPECS,SRPMS}
cp "$BINARY" "$RPM_ROOT/SOURCES/mcugen"
cp "$ROOT/pkg/rpm/mcugen.spec" "$RPM_ROOT/SPECS/mcugen.spec"
rpmbuild -bb \
    --define "_topdir $RPM_ROOT" \
    --define "_sourcedir $RPM_ROOT/SOURCES" \
    --target "$ARCH" \
    "$RPM_ROOT/SPECS/mcugen.spec"
find "$RPM_ROOT/RPMS" -name "*.rpm" -exec cp {} "$DIST/" \;
rm -rf "$RPM_ROOT"
echo "    -> $DIST/mcugen-${VERSION}-1.${ARCH}.rpm"

# ── .pkg.tar.zst (Arch) ──────────────────────────────────────────────────────
echo "==> Building .pkg.tar.zst..."
ARCH_STAGE="$(mktemp -d)"
mkdir -p "$ARCH_STAGE/usr/bin"
install -m755 "$BINARY" "$ARCH_STAGE/usr/bin/mcugen"

PKG_NAME="mcugen-${VERSION}-1-${ARCH}.pkg.tar.zst"
MTREE="$(mktemp)"

# Build .MTREE
bsdtar -czf "$MTREE" --format=mtree \
    --options='!all,use-set,type,uid,gid,mode,time,size,md5,sha256,link' \
    -C "$ARCH_STAGE" .

# Build .PKGINFO
INSTALLED_SIZE=$(du -sk "$ARCH_STAGE" | cut -f1)
cat > "$ARCH_STAGE/.PKGINFO" <<EOF
pkgname = mcugen
pkgver = ${VERSION}-1
pkgdesc = Material Color Utilities Generator
url = https://github.com/MeghBadonia/mcugen
builddate = $(date +%s)
packager = Megh Badonia <badoniamegh@gmail.com>
size = ${INSTALLED_SIZE}
arch = ${ARCH}
license = MIT
EOF

cp "$MTREE" "$ARCH_STAGE/.MTREE"
rm "$MTREE"

bsdtar -cf "$DIST/${PKG_NAME%.zst}" -C "$ARCH_STAGE" .
zstd -T0 -19 "$DIST/${PKG_NAME%.zst}" -o "$DIST/$PKG_NAME"
rm -f "$DIST/${PKG_NAME%.zst}"
rm -rf "$ARCH_STAGE"
echo "    -> $DIST/$PKG_NAME"

echo ""
echo "==> All packages built in $DIST/"
ls -lh "$DIST/"
