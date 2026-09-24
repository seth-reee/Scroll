#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_dir"

for tool in git makepkg cmake ninja desktop-file-validate bsdtar tar gzip sha256sum; do
  command -v "$tool" >/dev/null || { echo "Missing build tool: $tool" >&2; exit 1; }
done
if (( EUID == 0 )); then
  echo 'Run the package builder as your normal user, not root.' >&2
  exit 1
fi
if [[ -n "$(git status --porcelain)" ]]; then
  echo 'Commit source changes first: packages are built from the current Git commit.' >&2
  exit 1
fi
if [[ "$(uname -m)" != x86_64 ]]; then
  echo 'This package recipe currently targets Arch Linux x86_64.' >&2
  exit 1
fi

version="$(sed -n 's/^project(Scroll VERSION \([^ ]*\) .*/\1/p' CMakeLists.txt)"
[[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || { echo 'Cannot determine project version.' >&2; exit 1; }
output_dir="$project_dir/dist"
mkdir -p "$output_dir"
work_dir="$(mktemp -d -t scroll-package.XXXXXXXX)"
# Only this newly-created temporary build directory is removed on exit.
trap 'rm -rf -- "$work_dir"' EXIT
export SOURCE_DATE_EPOCH="$(git log -1 --format=%ct)"
export CMAKE_BUILD_PARALLEL_LEVEL="${CMAKE_BUILD_PARALLEL_LEVEL:-4}"
export PKGDEST="$output_dir"

source_name="scroll-$version.tar.gz"
git archive --format=tar --prefix="scroll-$version/" HEAD | gzip -n > "$output_dir/$source_name"
source_sha="$(sha256sum "$output_dir/$source_name")"
source_sha="${source_sha%% *}"
sed -e "s/@VERSION@/$version/g" -e "s/@SOURCE_SHA256@/$source_sha/g" \
  packaging/PKGBUILD.in > "$work_dir/PKGBUILD"
cp "$output_dir/$source_name" "$work_dir/$source_name"
cp "$work_dir/PKGBUILD" "$output_dir/PKGBUILD"

cd "$work_dir"
makepkg --force
makepkg --printsrcinfo > "$output_dir/.SRCINFO"
mapfile -t packages < <(makepkg --packagelist)
[[ ${#packages[@]} == 1 && -f "${packages[0]}" ]] || { echo 'Expected exactly one built package.' >&2; exit 1; }

bundle_name="scroll-$version-linux-x86_64"
mkdir "$work_dir/$bundle_name"
bsdtar -xf "${packages[0]}" -C "$work_dir/$bundle_name" usr
install -m644 "$project_dir/packaging/INSTALL.txt" "$work_dir/$bundle_name/INSTALL.txt"
tar --sort=name --mtime="@$SOURCE_DATE_EPOCH" --owner=0 --group=0 --numeric-owner \
  -czf "$output_dir/$bundle_name.tar.gz" -C "$work_dir" "$bundle_name"

cd "$output_dir"
sha256sum "$source_name" "$bundle_name.tar.gz" "$(basename "${packages[0]}")" PKGBUILD > SHA256SUMS
printf 'Built and tested packages in %s\n' "$output_dir"
