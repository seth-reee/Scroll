#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${project_dir}/build-appimage"
app_dir="${project_dir}/AppDir"
dist_dir="${project_dir}/dist"
tools_dir="${project_dir}/.appimage-tools"
linuxdeploy="${tools_dir}/linuxdeploy-x86_64.AppImage"
qt_plugin="${tools_dir}/linuxdeploy-plugin-qt-x86_64.AppImage"

for tool in "${linuxdeploy}" "${qt_plugin}"; do
    if [[ ! -x "${tool}" ]]; then
        echo "Missing ${tool}. Download the linuxdeploy and linuxdeploy-plugin-qt x86_64 AppImages into .appimage-tools/." >&2
        exit 1
    fi
done

cmake -S "${project_dir}" -B "${build_dir}" -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build "${build_dir}" --parallel

rm -rf "${app_dir}"
install -Dm755 "${build_dir}/qomaedit" "${app_dir}/usr/bin/qomaedit"
install -Dm644 "${project_dir}/packaging/qomaedit.desktop" "${app_dir}/usr/share/applications/qomaedit.desktop"
install -Dm644 "${project_dir}/packaging/qomaedit.svg" "${app_dir}/usr/share/icons/hicolor/scalable/apps/qomaedit.svg"
# linuxdeploy's Qt plugin detects X11 automatically. Add the Wayland QPA
# plugins explicitly so qOmaedit launches natively on Hyprland/Omarchy too.
qt_plugins="/usr/lib/x86_64-linux-gnu/qt6/plugins"
qt_libs="/usr/lib/x86_64-linux-gnu"
install -Dm755 "${qt_plugins}/platforms/libqwayland-egl.so" "${app_dir}/usr/plugins/platforms/libqwayland-egl.so"
install -Dm755 "${qt_plugins}/platforms/libqwayland-generic.so" "${app_dir}/usr/plugins/platforms/libqwayland-generic.so"
install -Dm755 "${qt_plugins}/wayland-graphics-integration-client/libqt-plugin-wayland-egl.so" "${app_dir}/usr/plugins/wayland-graphics-integration-client/libqt-plugin-wayland-egl.so"
install -Dm755 "${qt_plugins}/wayland-shell-integration/libxdg-shell.so" "${app_dir}/usr/plugins/wayland-shell-integration/libxdg-shell.so"
# These libraries are loaded only by the explicit Wayland platform plugins,
# so add them before linuxdeploy scans the AppDir for dependencies.
install -Dm755 "${qt_libs}/libQt6WaylandClient.so.6" "${app_dir}/usr/lib/libQt6WaylandClient.so.6"
install -Dm755 "${qt_libs}/libQt6WaylandEglClientHwIntegration.so.6" "${app_dir}/usr/lib/libQt6WaylandEglClientHwIntegration.so.6"
for plugin in \
    "${app_dir}/usr/plugins/platforms/libqwayland-egl.so" \
    "${app_dir}/usr/plugins/platforms/libqwayland-generic.so" \
    "${app_dir}/usr/plugins/wayland-graphics-integration-client/libqt-plugin-wayland-egl.so" \
    "${app_dir}/usr/plugins/wayland-shell-integration/libxdg-shell.so"; do
    patchelf --set-rpath '$ORIGIN/../../lib' "${plugin}"
done

mkdir -p "${dist_dir}"
cd "${dist_dir}"
export LINUXDEPLOY_PLUGIN_QT_QMAKE="$(command -v qmake6)"
export APPIMAGE_EXTRACT_AND_RUN=1
"${linuxdeploy}" --appimage-extract-and-run \
    --appdir "${app_dir}" \
    --executable "${app_dir}/usr/bin/qomaedit" \
    --desktop-file "${project_dir}/packaging/qomaedit.desktop" \
    --icon-file "${project_dir}/packaging/qomaedit.svg" \
    --plugin qt \
    --output appimage
