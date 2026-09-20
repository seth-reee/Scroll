# Qomaedit

Qomaedit is a small Qt 6 / Qt Quick text editor for scripts and other plain-text files on Omarchy. It is deliberately focused: fast startup, familiar editor basics, and a UI that silently follows the active desktop theme.

## Features

- Open, create, and safely save UTF-8 text files.
- Close saved tabs directly from the close control beside each tab name.
- Uses the system/platform file picker for Open and Save As.
- Standard `Ctrl+N`, `Ctrl+O`, `Ctrl+S`, and `Ctrl+F` shortcuts.
- Find next, replace, and replace all.
- Line-number gutter that keeps source-line numbers aligned when visual wrapping is enabled; wrapped continuation rows are left blank.
- Footer `Wrap: On/Off` control. Wrapping is display-only: it never inserts newlines, so copied and saved text is unchanged.
- Optional extra visual spacing for wrapped lines, controlled in Settings and never written to the file.
- Mouse-wheel scrolling and draggable vertical/horizontal scrollbars. The horizontal bar hides while wrapping is on.
- Lightweight highlighting for common shell, Python, Lua, and JavaScript-like comments, strings, numbers, and keywords.
- Live Omarchy palette support. Qomaedit reads `~/.local/state/omarchy/current/theme/colors.toml` and updates its colors after a theme switch without displaying the theme name.

## Build and run

On Arch/Omarchy, install the build tools and Qt packages if necessary:

```bash
sudo pacman -S --needed cmake ninja qt6-base qt6-declarative
```

Configure, build, and run:

```bash
cmake -S . -B build -G Ninja
cmake --build build
./build/qomaedit
```

The build output is intentionally ignored by Git. If `build/` does not exist, rerun the configure command above before building.

## Current scope

Qomaedit is a single-document editor. Tabs, syntax-aware language detection, and richer language support are natural future additions.
