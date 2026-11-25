# Vibeyrino — personal Leafyrino fork (see README.md).
_gitrepo='https://github.com/RED-404/vibeyrino.git'

pkgname=vibeyrino-git
_pkgname=vibeyrino
pkgver=0.0.0
pkgrel=1
pkgdesc='Personal Chatterino/Leafyrino fork: Kick/YouTube multistream, native Qt UI, Gruvbox'
arch=('x86_64' 'aarch64')
url='https://github.com/RED-404/vibeyrino'
license=('MIT')
install=${pkgname}.install
depends=(
  'boost-libs'
  'hunspell'
  'libavif'
  'libnotify'
  'openssl'
  'qt6-base'
  'qt6-charts'
  'qt6-imageformats'
  'qt6-svg'
  'qtkeychain-qt6'
)
makedepends=(
  'boost'
  'cmake'
  'git'
  'ninja'
  'pkgconf'
  'rapidjson'
)
optdepends=(
  'qt6-wayland: Wayland support'
  'streamlink: pipe Twitch/Kick streams to a player'
  'pulseaudio: audio output'
  'pipewire-pulse: audio output'
)
provides=('vibeyrino')
conflicts=('vibeyrino' 'leafyrino' 'leafyrino-git')
source=(
  "git+${_gitrepo}#branch=leafyrino"
  'git+https://github.com/Chatterino/libcommuni.git#branch=chatterino-cmake'
  'git+https://github.com/pajlada/settings.git'
  'git+https://github.com/pajlada/signals.git'
  'git+https://github.com/pajlada/serialize.git'
  'git+https://github.com/Neargye/magic_enum.git'
  'git+https://github.com/mackron/miniaudio.git'
  'git+https://github.com/lua/lua.git#branch=v5.4'
  'git+https://github.com/ThePhD/sol2.git'
  'git+https://github.com/Chatterino/certify.git'
  'git+https://github.com/martinmoene/expected-lite.git'
  'git+https://github.com/HowardHinnant/date.git'
  'git+https://github.com/fmtlib/fmt.git'
)
sha256sums=(
  'SKIP'
  'SKIP'
  'SKIP'
  'SKIP'
  'SKIP'
  'SKIP'
  'SKIP'
  'SKIP'
  'SKIP'
  'SKIP'
  'SKIP'
  'SKIP'
  'SKIP'
)

pkgver() {
  cd "${srcdir}/${_pkgname}"
  local tag
  tag=$(git tag -l 'v2.*' --sort=-v:refname | grep -vE 'beta|alpha' | head -1)
  if [[ -n "$tag" ]]; then
    git describe --long --tags --match "$tag" |
      sed 's/^v//;s/\([^-]*-g\)/r\1/;s/-/./g'
  else
    printf "r%s.g%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short=7 HEAD)"
  fi
}

prepare() {
  cd "${srcdir}/${_pkgname}"

  git config submodule.cmake/sanitizers-cmake.update 'none'
  git config submodule.lib/WinToast.update 'none'
  git config submodule.lib/rapidjson.update 'none'
  git config submodule.lib/qtkeychain.update 'none'
  git config submodule.lib/googletest.update 'none'
  git config submodule.lib/kimageformats.update 'none'
  git config submodule.tools/crash-handler.update 'none'

  git submodule init
  git config submodule.lib/libcommuni.url "${srcdir}/libcommuni"
  git config submodule.lib/settings.url "${srcdir}/settings"
  git config submodule.lib/signals.url "${srcdir}/signals"
  git config submodule.lib/serialize.url "${srcdir}/serialize"
  git config submodule.lib/magic_enum.url "${srcdir}/magic_enum"
  git config submodule.lib/miniaudio.url "${srcdir}/miniaudio"
  git config submodule.lib/lua/src.url "${srcdir}/lua"
  git config submodule.lib/sol2.url "${srcdir}/sol2"
  git config submodule.lib/certify.url "${srcdir}/certify"
  git config submodule.lib/expected-lite.url "${srcdir}/expected-lite"
  git config submodule.lib/twitch-eventsub-ws/lib/date.url "${srcdir}/date"
  git config submodule.lib/twitch-eventsub-ws/lib/fmt.url "${srcdir}/fmt"
  git -c protocol.file.allow=always submodule update
}

build() {
  cd "${srcdir}/${_pkgname}"

  export CFLAGS+=" -ffile-prefix-map=${srcdir}=. -fmacro-prefix-map=${srcdir}=."
  export CXXFLAGS+=" -ffile-prefix-map=${srcdir}=. -fmacro-prefix-map=${srcdir}=."

  rm -rf build

  local -a cmake_options=(
    -S .
    -B build
    -G Ninja
    -DCMAKE_BUILD_TYPE=None
    -DCMAKE_INSTALL_PREFIX=/usr
    -DBUILD_TESTS=OFF
    -DBUILD_BENCHMARKS=OFF
    -DBUILD_WITH_CRASHPAD=OFF
    -DBUILD_WITH_QTKEYCHAIN=ON
    -DUSE_SYSTEM_QTKEYCHAIN=ON
    -DUSE_PRECOMPILED_HEADERS=OFF
    -DCHATTERINO_NO_AVIF_PLUGIN=ON
    -DCHATTERINO_PLUGINS=ON
    -DCHATTERINO_SPELLCHECK=ON
    -DCHATTERINO_UPDATER=OFF
    -DSKIP_JSON_GENERATION=ON
  )

  if [[ ${CXXFLAGS} == *'-flto'* ]]; then
    cmake_options+=(-DCHATTERINO_LTO=ON)
  fi

  cmake "${cmake_options[@]}"
  cmake --build build
}

package() {
  cd "${srcdir}/${_pkgname}"

  DESTDIR="${pkgdir}" cmake --install build
  install -Dm644 LICENSE "${pkgdir}/usr/share/licenses/${pkgname}/LICENSE"
}