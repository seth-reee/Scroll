# Qomaedit

A small, native-feeling script editor for Omarchy, written with Qt 6 and Qt Quick.

## What it does

- Opens and safely saves UTF-8 text files.
- Provides New, Open, and Save actions plus standard keyboard shortcuts.
- Uses the active Omarchy palette from `~/.local/state/omarchy/current/theme/colors.toml`.
- Watches the palette and updates its UI after `omarchy theme set …`.

Syntax highlighting, line numbers, tabs, and find/replace are intentionally the next milestone; this first cut establishes the application and theming foundation.

## Build and run

```bash
cmake -S . -B build -G Ninja
cmake --build build
./build/qomaedit
```

Requires the `qt6-base` and `qt6-declarative` Arch packages.

## GitHub

This machine’s GitHub CLI authentication is currently expired. After running `gh auth login`, create and publish a repository with:

```bash
git init
git add .
git commit -m "Initial Qomaedit prototype"
gh repo create Qomaedit --public --source=. --push
```
