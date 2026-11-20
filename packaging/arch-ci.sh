#!/usr/bin/env bash
set -euo pipefail
: "${PACKAGE_ARCH:?}" "${BUILD_UID:?}" "${BUILD_GID:?}"
[[ "$(uname -m)" == "$PACKAGE_ARCH" ]]
pacman -Syu --noconfirm --needed base-devel git cmake ninja qt6-base desktop-file-utils zstd
# Arch Linux ARM may default to xz; releases use the same zstd format on both architectures.
sed -i "s/^PKGEXT=.*/PKGEXT='.pkg.tar.zst'/" /etc/makepkg.conf
# The runner owns the checkout; build as the same unprivileged UID for makepkg.
groupadd -g "$BUILD_GID" scrollbuild 2>/dev/null || true
useradd -m -u "$BUILD_UID" -g "$BUILD_GID" scrollbuild
su scrollbuild -s /bin/bash -c 'git config --global --add safe.directory /work && cd /work && ./packaging/build-package.sh'
cd /work/dist
shopt -s nullglob
packages=(./*.pkg.tar.zst)
bundles=(./*-linux-"$PACKAGE_ARCH".tar.gz)
printf "Package files: %s; bundle files: %s\n" "${packages[*]}" "${bundles[*]}"
[[ ${#packages[@]} == 1 && ${#bundles[@]} == 1 ]]
sha256sum "${packages[@]#./}" "${bundles[@]#./}" > "SHA256SUMS-arch-$PACKAGE_ARCH"
