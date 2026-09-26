# Maintainer: zyrophix <175893402+xeitghu@users.noreply.github.com>
#
# This package is built from this repository rather than the AUR: roller has no
# .desktop entry and is only ever started from a keybind, so nobody searches a
# helper for it, and an in-tree PKGBUILD avoids keeping a second copy in sync.
# Install with:  git clone <this repo> && cd roller && makepkg -si

pkgname=roller
# Placeholder only: pkgver() below replaces it on every build. makepkg refuses
# to start if this is empty.
pkgver=0.1.0
pkgrel=1
pkgdesc='Coverflow wallpaper picker for Hyprland and other wlr-layer-shell desktops'
arch=('x86_64')
url="https://github.com/zyrophix/roller"
license=('MIT')
source=("git+${url}.git#branch=main")
b2sums=('SKIP')

# layer-shell-qt belongs in both lists, and not only because the linker wants
# it. The project calls find_package(LayerShellQt QUIET), so a build without it
# succeeds and produces a binary that then refuses to start at runtime. A
# packaging-time dependency is the only place that gets caught before the user
# runs anything.
depends=('qt6-base' 'qt6-declarative' 'layer-shell-qt')
makedepends=('cmake' 'gcc' 'qt6-base' 'qt6-declarative' 'layer-shell-qt')
optdepends=(
  'awww: animated transitions, and the only backend tested on a real desktop'
  'hyprpaper: Hyprland native backend'
  'waypaper: waypaper backend'
  'swaybg: swaybg backend'
  'mpvpaper: video wallpapers'
  'ffmpeg: poster frames for video wallpapers'
)

build() {
    # The installed binary keeps a $ORIGIN runpath. Qt sets it in
    # qt_add_executable so it can find its plugins when relocated, and neither
    # QT_NO_INSTALL_RPATH nor CMAKE_SKIP_INSTALL_RPATH overrides it. Verified to
    # resolve cleanly, so it is left as Qt intends.
    cmake -B build -S "$srcdir/roller" \
        -DCMAKE_BUILD_TYPE=None \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DBUILD_TESTING=ON
    cmake --build build
}

check() {
    ctest --test-dir build --output-on-failure
}

package() {
    # Installs the binary only: the QML module is compiled into the executable,
    # and the install rule in CMakeLists.txt does not touch the test target.
    DESTDIR="$pkgdir" cmake --install build
}

# Derived from git, so tagging a release is the only step needed to publish a
# new package version. Building exactly on the tag reports the bare version;
# past it, the commit distance and hash say so instead of pretending to be the
# release. Before the first tag exists, fall back to a bare commit count.
pkgver() {
    cd "$srcdir/roller"
    local tag distance
    tag=$(git describe --tags --abbrev=0 2>/dev/null) || tag=""
    if [[ -z $tag ]]; then
        printf 'r%s.g%s\n' "$(git rev-list --count HEAD)" "$(git rev-parse --short=7 HEAD)"
        return
    fi
    distance=$(git rev-list --count "${tag}..HEAD")
    if [[ $distance -eq 0 ]]; then
        printf '%s\n' "${tag#v}"
    else
        printf '%s.r%s.g%s\n' "${tag#v}" "$distance" "$(git rev-parse --short=7 HEAD)"
    fi
}
