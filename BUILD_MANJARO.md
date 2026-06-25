# Build Prism Launcher (Cracked/Offline) on Manjaro / Arch

This guide assumes you are on **Manjaro** (or Arch) and have the fixed modular cracked code from this workspace.

**Important**: You must run the `pacman` commands yourself with proper privileges. The agent cannot use `sudo`.

## 1. Install build dependencies (Manjaro / Arch)

```bash
# Core build tools
sudo pacman -S --needed base-devel cmake ninja git

# Qt6 (required)
sudo pacman -S --needed qt6-base qt6-tools qt6-svg qt6-networkauth qt6-5compat

# Compression & data libs (Prism uses these)
sudo pacman -S --needed quazip-qt6 tomlplusplus zstd

# Optional but recommended for full feature set
sudo pacman -S --needed openal libpulse java-runtime-common
```

**Extra notes for Manjaro**:
- If you use an AUR helper (yay, paru, etc.), you can install `quazip-qt6` and `tomlplusplus` from AUR if the official repos are behind.
- For a completely clean build, many Arch users also install `vcpkg` and let Prism fetch some libs, but system packages above are usually sufficient.

After installing, verify:

```bash
qmake6 --version   # or just check that /usr/lib/qt6 exists
```

## 2. Get the full source

```bash
cd ~
git clone --depth 1 --recurse-submodules https://github.com/Diegiwg/PrismLauncher-Cracked.git prism-cracked-src
cd prism-cracked-src
```

## 3. Overlay the improved modular cracked code

Copy the files from this workspace (the fixed version you received):

```bash
# From the directory containing this BUILD_MANJARO.md (the agent workspace)
cp -r /path/to/this/workspace/launcher/cracked          prism-cracked-src/launcher/
cp -r /path/to/this/workspace/launcher/ui/dialogs/OfflineSkinDialog.* prism-cracked-src/launcher/ui/dialogs/

# Also copy the improved docs if you want
cp /path/to/this/workspace/README.md                    prism-cracked-src/README.md
cp /path/to/this/workspace/BUILD_MANJARO.md             prism-cracked-src/
cp /path/to/this/workspace/docs/cracked-integration-examples.md prism-cracked-src/docs/
```

Edit `prism-cracked-src/launcher/CMakeLists.txt` and add the cracked module (see the section in `docs/cracked-integration-examples.md`).

## 4. Configure + Build (with cracked mode enabled)

```bash
cd ~/prism-cracked-src

cmake -S . -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_CRACKED_MODE=ON \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DLauncher_APP_BINARY_NAME=prismlauncher-offline

ninja -C build -j$(nproc)
```

## 5. Install

```bash
# System-wide (you will need privileges)
sudo ninja -C build install

# Or install to a user prefix (no sudo)
cmake --install build --prefix "$HOME/.local"
```

After install you should have `prismlauncher-offline` (or the name you chose).

## 6. Run the new tests (optional but recommended)

```bash
cd build
ctest -R Cracked --output-on-failure
```

## Troubleshooting on Manjaro

- Missing `quazip` or `tomlplusplus` → install from AUR if needed.
- Qt6 not found → make sure `qt6-base` is installed and `CMAKE_PREFIX_PATH` includes `/usr/lib/qt6` if necessary.
- "createOffline" does not exist → follow the comments in `CrackedAccountManager.cpp`. The exact factory method name sometimes needs a one-line adjustment for your exact Prism commit.

You now have a clean, maintainable, feature-rich Prism Launcher Offline build on Manjaro.
