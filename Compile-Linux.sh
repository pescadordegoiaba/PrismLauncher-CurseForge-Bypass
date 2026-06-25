#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT}/build-linux"
INSTALL_DIR="${BUILD_DIR}/package"
JOBS="${JOBS:-$(nproc)}"

log() {
    printf '\033[1;32m[Linux]\033[0m %s\n' "$*"
}

log "Configuring native Linux build..."
cmake -S "${ROOT}" -B "${BUILD_DIR}" \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_CRACKED_MODE=ON \
    -DBUILD_TESTING=OFF

log "Building with ${JOBS} jobs..."
cmake --build "${BUILD_DIR}" --parallel "${JOBS}"

log "Installing runtime package to ${INSTALL_DIR}..."
rm -rf "${INSTALL_DIR}"
cmake --install "${BUILD_DIR}" --prefix "${INSTALL_DIR}"

log "Done."
log "Linux executable/package: ${INSTALL_DIR}"
log "Main binary is usually: ${INSTALL_DIR}/bin/prismlauncher"
