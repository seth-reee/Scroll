# qOmaedit

qOmaedit is a small Qt 6 / Qt Quick text editor for scripts and other plain-text files on Omarchy. It is deliberately focused: fast startup, familiar editor basics, and a UI that silently follows the active desktop theme.

Repository: [github.com/seth-reee/qOmaedit](https://github.com/seth-reee/qOmaedit)

## Features

- Open, create, and safely save UTF-8 text files.
- Close saved tabs directly from the close control beside each tab name.
- Uses the system/platform file picker for Open and Save As; Open defaults to an all-files filter.
- Standard `Ctrl+N`, `Ctrl+O`, `Ctrl+S`, and `Ctrl+F` shortcuts.
- Compact header Menu for New, Open, Save, Find and replace, and About.
- About dialog with version, repository link, and license information.
- Find next, replace, and replace all.
- Line-number gutter that keeps source-line numbers aligned when visual wrapping is enabled; wrapped continuation rows are left blank.
- Footer `Wrap: On/Off` control. Wrapping is display-only: it never inserts newlines, so copied and saved text is unchanged.
- Optional visual guides before wrapped continuation lines, controlled in Settings and never written to the file.
- Mouse-wheel scrolling and draggable vertical/horizontal scrollbars. The horizontal bar hides while wrapping is on.
- Lightweight highlighting for common shell, Python, Lua, and JavaScript-like comments, strings, numbers, and keywords.
- Live Omarchy palette support. qOmaedit reads `~/.local/state/omarchy/current/theme/colors.toml` and updates its colors after a theme switch without displaying the theme name.

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

qOmaedit supports multiple open documents through its tab bar. Richer language-specific highlighting and editor features are natural future additions.

## License

qOmaedit is licensed under the MIT License.

You are free to use, modify, distribute, and use qOmaedit commercially, provided that the original copyright and license notices are preserved.

See [LICENSE](LICENSE) for details.

## Contributing

Contributions are welcome. If you improve qOmaedit and would like to help the project, please open a pull request upstream. You are not required to publish or contribute your modifications; the MIT License only requires that copyright and license notices are preserved.
