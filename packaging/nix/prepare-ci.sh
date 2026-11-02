#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
output_dir=$(realpath -m "${1:?Usage: prepare-ci.sh OUTPUT_DIRECTORY}")
mkdir -p "$output_dir"
version=$(sed -n 's/^project(Scroll VERSION \([^ ]*\) .*/\1/p' CMakeLists.txt)
[[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]
if [[ "${GITHUB_REF_TYPE:-}" == tag ]]; then
  [[ "${GITHUB_REF_NAME:?}" == "v$version" ]]
else
  version="$version.r$(git rev-list --count HEAD).g$(git rev-parse --short=8 HEAD)"
fi
git ls-files -z -- CMakeLists.txt LICENSE README.md src tests resources packaging/scroll.desktop flake.nix flake.lock packaging/nix | \
  tar --null --no-recursion --sort=name --mtime=@0 --owner=0 --group=0 --numeric-owner \
  --transform="s,^,Scroll-$version/," -cf - -T - | gzip -n > "$output_dir/scroll-$version.tar.gz"
if [[ -n "${GITHUB_OUTPUT:-}" ]]; then
  printf 'version=%s\n' "$version" >> "$GITHUB_OUTPUT"
fi
