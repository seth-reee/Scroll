# qOmaedit

qOmaedit is a small Qt 6 Widgets text editor for scripts and other plain-text files on Omarchy. It uses QPlainTextEdit, keeps one text document per tab, and paints highlighting and line numbers only around the viewport. No QML runtime or Qt Quick scene graph is needed.

Repository: [github.com/seth-reee/qOmaedit](https://github.com/seth-reee/qOmaedit)

## Features

- Open, create, and safely save UTF-8 text files.
- Opens files passed from the desktop file manager or command line; multiple files open in tabs.
- Close saved tabs directly from the close control beside each tab name.
- Uses the system/platform file picker for Open and Save As; Open defaults to an all-files filter.
- Standard `Ctrl+N`, `Ctrl+O`, `Ctrl+S`, and `Ctrl+F` shortcuts.
- Compact header Menu for New, Open, Save, Find and replace, and About.
- About dialog with version, repository link, and license information.
- Find next, replace, and replace all.
- Independent undo history, cursor position, selection, and scrolling in each tab. Replace All is a single undoable operation.
- Automatic scrolling to keep the insertion point visible while typing.
- Line-number gutter that keeps source-line numbers aligned when visual wrapping is enabled; wrapped continuation rows are left blank.
- Footer `Wrap: On/Off` control. Wrapping is display-only: it never inserts newlines, so copied and saved text is unchanged.
- Optional visual guides before wrapped continuation lines, controlled in Settings and never written to the file.
- Mouse-wheel scrolling and draggable vertical/horizontal scrollbars. The horizontal bar hides while wrapping is on.
- Lightweight highlighting for common shell, Python, Lua, and JavaScript-like comments, strings, numbers, and keywords.
- Exceptionally long physical lines (over 32,768 characters) remain editable but skip syntax coloring to bound formatting work. Large files with normal-length lines remain highlighted.
- Live Omarchy palette support. qOmaedit reads `~/.local/state/omarchy/current/theme/colors.toml` and updates its colors after a theme switch without displaying the theme name.

## Build and run

On Arch/Omarchy, install the build tools and Qt packages if necessary:

```bash
sudo pacman -S --needed cmake ninja qt6-base
```

Configure, build, and run:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/qomaedit
```

The build output is intentionally ignored by Git. If `build/` does not exist, rerun the configure command above before building.

Run the regression tests after building:

```bash
ctest --test-dir build --output-on-failure
```

Tests exercise file safety, unsaved-window protection, save failures, find/replace,
per-tab undo, automatic scrolling, and bounded highlighting with a 20,000-line document.
They run headlessly with Qt's offscreen platform. Use `-DBUILD_TESTING=OFF` when
configuring a build that does not need the tests or the Qt Test component.

For a repeatable Linux memory measurement of the compiled app:

```bash
cmake --build build --target profile_memory
./build/profile_memory ./build/qomaedit
```

This optional Linux probe uses fresh processes, isolated settings, syntax
highlighting, and generated JavaScript files of up to 100,000 lines. It reports
RSS/PSS after an eight-second warmup and samples CPU for one second. Active
processes are flagged so that you can rerun with a longer `--settle-ms`.
The default uses Qt's offscreen backend; a live desktop changes memory usage.

For a comparison with installed KWrite and gedit on the same desktop:

```bash
./build/profile_memory --desktop --compare --repeats 2 ./build/qomaedit
```

This opens temporary test windows and closes only the processes it launches.
User editor settings are isolated; no installation or desktop settings are changed.
See [REVIEW.md](REVIEW.md) for measured results and limitations.

## AppImage build

An x86_64 AppImage recipe is provided in `packaging/build-appimage.sh` and has been tested in a Debian Trixie Distrobox. Install the Qt 6 development packages, `cmake`, `ninja-build`, and `patchelf` in the build container, then place the current x86_64 LinuxDeploy and LinuxDeploy Qt plugin AppImages in `.appimage-tools/`.

Run:

```bash
./packaging/build-appimage.sh
```

The finished portable application is written to `dist/qOmaedit-x86_64.AppImage`. It includes native Wayland support for Omarchy/Hyprland and an X11 fallback.

## Current scope

qOmaedit supports multiple open documents through its tab bar. Richer language-specific highlighting and editor features are natural future additions.

## License

qOmaedit is licensed under the MIT License.

You are free to use, modify, distribute, and use qOmaedit commercially, provided that the original copyright and license notices are preserved.

See [LICENSE](LICENSE) for details.

## Contributing

Contributions are welcome. If you improve qOmaedit and would like to help the project, please open a pull request upstream. You are not required to publish or contribute your modifications; the MIT License only requires that copyright and license notices are preserved.
