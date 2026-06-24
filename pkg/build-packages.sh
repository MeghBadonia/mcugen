#!/usr/bin/env bash
set -euo pipefail

VERSION="3.0.0"
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
chmod 755 "$DEB_STAGE"
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
PKG_NAME="mcugen-${VERSION}-1-${ARCH}.pkg.tar.zst"
ARCH_BUILD="$(mktemp -d)"
ARCH_PKG="$ARCH_BUILD/pkg"
mkdir -p "$ARCH_PKG/usr/bin"
install -m755 "$BINARY" "$ARCH_PKG/usr/bin/mcugen"

INSTALLED_SIZE=$(du -sb "$ARCH_PKG" | cut -f1)
BUILDDATE=$(date +%s)

cat > "$ARCH_BUILD/.PKGINFO" <<EOF
pkgname = mcugen
pkgver = ${VERSION}-1
pkgdesc = Material Color Utilities Generator
url = https://github.com/MeghBadonia/mcugen
builddate = ${BUILDDATE}
packager = Megh Badonia <badoniamegh@gmail.com>
size = ${INSTALLED_SIZE}
arch = ${ARCH}
license = MIT
EOF

PKG_OUT="$DIST/$PKG_NAME"

# .MTREE must be gzip-compressed (as in real makepkg output)
bsdtar -czf "$ARCH_BUILD/.MTREE" --format=mtree \
    --options='!all,use-set,type,uid,gid,mode,time,size,md5,sha256,link' \
    --uid 0 --gid 0 --uname root --gname root \
    -C "$ARCH_PKG" .

# Final archive: root:root ownership, no leading ./ on payload paths
fakeroot bsdtar --no-fflags -cf - \
    --uid 0 --gid 0 --uname root --gname root \
    -C "$ARCH_BUILD" .PKGINFO .MTREE \
    -C "$ARCH_PKG" usr \
    | zstd -T0 -19 --force -o "$PKG_OUT"

rm -rf "$ARCH_BUILD"
echo "    -> $PKG_OUT"

echo ""
echo "==> All packages built in $DIST/"
ls -lh "$DIST/"
