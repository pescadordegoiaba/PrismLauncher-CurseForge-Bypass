#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT}/build-win10"
PACKAGE_DIR="${BUILD_DIR}/package"
SYSROOT="${ROOT}/.msys2-win64"
MINGW_PREFIX="${SYSROOT}/mingw64"
PACMAN_CONF="${ROOT}/msys2-pacman.conf"
ZIP_PATH="${BUILD_DIR}/PrismLauncher-Win10.zip"
JOBS="${JOBS:-$(nproc)}"

MSYS2_PACKAGES=(
    mingw-w64-x86_64-extra-cmake-modules
    mingw-w64-x86_64-qt6-base
    mingw-w64-x86_64-qt6-tools
    mingw-w64-x86_64-qt6-svg
    mingw-w64-x86_64-qt6-networkauth
    mingw-w64-x86_64-zlib
    mingw-w64-x86_64-libarchive
    mingw-w64-x86_64-qrencode
    mingw-w64-x86_64-cmark
    mingw-w64-x86_64-tomlplusplus
    mingw-w64-x86_64-vulkan-headers
    mingw-w64-x86_64-pkgconf
)

log() {
    printf '\033[1;32m[Windows]\033[0m %s\n' "$*"
}

require_tool() {
    if ! command -v "$1" >/dev/null 2>&1; then
        printf 'Missing required tool: %s\n' "$1" >&2
        exit 1
    fi
}

write_pacman_conf() {
    cat > "${PACMAN_CONF}" <<EOF
[options]
Architecture = x86_64
CacheDir = ${SYSROOT}/var/cache/pacman/pkg
SigLevel = Never
LocalFileSigLevel = Never
DisableDownloadTimeout

[mingw64]
Server = https://repo.msys2.org/mingw/mingw64

[msys]
Server = https://repo.msys2.org/msys/x86_64
EOF
}

install_sysroot() {
    mkdir -p "${SYSROOT}/var/lib/pacman" "${SYSROOT}/var/cache/pacman/pkg" "${SYSROOT}/etc/pacman.d"
    write_pacman_conf

    log "Installing/updating local MSYS2 Windows sysroot..."
    fakeroot pacman \
        --config "${PACMAN_CONF}" \
        --root "${SYSROOT}" \
        --dbpath "${SYSROOT}/var/lib/pacman" \
        --cachedir "${SYSROOT}/var/cache/pacman/pkg" \
        -Sy --needed --noconfirm "${MSYS2_PACKAGES[@]}"
}

deploy_runtime() {
    local queue seen item dll name lower src dest dir

    mkdir -p "${PACKAGE_DIR}"
    cmake --install "${BUILD_DIR}" --prefix "${PACKAGE_DIR}" || true

    queue="${PACKAGE_DIR}/prismlauncher.exe ${PACKAGE_DIR}/prismlauncher_filelink.exe"
    seen=""
    while [[ -n "${queue}" ]]; do
        item="${queue%% *}"
        if [[ "${queue}" == "${item}" ]]; then
            queue=""
        else
            queue="${queue#* }"
        fi

        case " ${seen} " in
            *" ${item} "*) continue ;;
        esac
        seen="${seen} ${item}"

        while IFS= read -r dll; do
            name="${dll#*DLL Name: }"
            lower="$(printf '%s' "${name}" | tr '[:upper:]' '[:lower:]')"
            case "${lower}" in
                kernel32.dll|user32.dll|gdi32.dll|advapi32.dll|shell32.dll|ole32.dll|oleaut32.dll|uuid.dll|ws2_32.dll|winmm.dll|comdlg32.dll|comctl32.dll|shlwapi.dll|version.dll|imm32.dll|dwmapi.dll|uxtheme.dll|mpr.dll|netapi32.dll|userenv.dll|wtsapi32.dll|dnsapi.dll|iphlpapi.dll|bcrypt.dll|crypt32.dll|secur32.dll|setupapi.dll|authz.dll|ntdll.dll|d2d1.dll|d3d9.dll|d3d11.dll|d3d12.dll|dwrite.dll|dxgi.dll|winhttp.dll|rpcrt4.dll|usp10.dll|shcore.dll|ncrypt.dll|api-ms-win-*.dll|ext-ms-*.dll|msvcrt.dll)
                    continue
                    ;;
            esac

            src="$(find "${MINGW_PREFIX}/bin" /usr/x86_64-w64-mingw32/bin -maxdepth 1 -iname "${name}" -print -quit 2>/dev/null || true)"
            if [[ -n "${src}" ]]; then
                dest="${PACKAGE_DIR}/$(basename "${src}")"
                if [[ ! -f "${dest}" ]]; then
                    cp "${src}" "${dest}"
                    queue="${queue} ${dest}"
                fi
            else
                printf 'Missing non-system DLL %s needed by %s\n' "${name}" "${item}" >&2
            fi
        done < <(x86_64-w64-mingw32-objdump -p "${item}" 2>/dev/null | rg 'DLL Name' || true)
    done

    for dir in platforms imageformats iconengines styles tls; do
        if [[ -d "${MINGW_PREFIX}/share/qt6/plugins/${dir}" ]]; then
            mkdir -p "${PACKAGE_DIR}/${dir}"
            cp -u "${MINGW_PREFIX}/share/qt6/plugins/${dir}"/*.dll "${PACKAGE_DIR}/${dir}/" 2>/dev/null || true
        fi
    done

    printf '[Paths]\nPlugins=.\n' > "${PACKAGE_DIR}/qt.conf"
}

require_tool cmake
require_tool ninja
require_tool fakeroot
require_tool pacman
require_tool x86_64-w64-mingw32-gcc
require_tool x86_64-w64-mingw32-g++
require_tool x86_64-w64-mingw32-windres
require_tool x86_64-w64-mingw32-objdump
require_tool rg

if [[ ! -f "${MINGW_PREFIX}/lib/cmake/Qt6/Qt6Config.cmake" ]]; then
    install_sysroot
fi

export PKG_CONFIG_LIBDIR="${MINGW_PREFIX}/lib/pkgconfig"
export PKG_CONFIG_SYSROOT_DIR="${SYSROOT}"
export PATH="${MINGW_PREFIX}/bin:/usr/bin:${PATH}"

log "Configuring Windows 10 x86_64 build..."
cmake -S "${ROOT}" -B "${BUILD_DIR}" \
    -G Ninja \
    -DCMAKE_SYSTEM_NAME=Windows \
    -DCMAKE_SYSTEM_PROCESSOR=x86_64 \
    -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
    -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
    -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres \
    -DCMAKE_FIND_ROOT_PATH="${MINGW_PREFIX}" \
    -DCMAKE_PREFIX_PATH="${MINGW_PREFIX}" \
    -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=BOTH \
    -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
    -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
    -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY \
    -DQT_HOST_PATH=/usr \
    -DQt6HostInfo_DIR=/usr/lib/cmake/Qt6HostInfo \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_CRACKED_MODE=ON \
    -DBUILD_TESTING=OFF

log "Building Windows executable with ${JOBS} jobs..."
cmake --build "${BUILD_DIR}" --parallel "${JOBS}"

log "Deploying Windows runtime package..."
deploy_runtime

log "Creating zip package..."
rm -f "${ZIP_PATH}"
cmake -E tar cfv "${ZIP_PATH}" --format=zip -- "${PACKAGE_DIR}" >/dev/null

log "Done."
log "Windows package folder: ${PACKAGE_DIR}"
log "Windows zip: ${ZIP_PATH}"
