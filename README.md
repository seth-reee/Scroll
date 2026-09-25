# <img src="resources/scroll.png" width="48" height="48" alt=""> Scroll

A lightweight text editor for scripts and plain-text files on [Omarchy](https://omarchy.org/), built with Qt 6 Widgets. Scroll follows your desktop theme and keeps everyday editing simple.

## Features

- **Tabbed editing** with independent undo history, cursor position, and scrolling for each document.
- **Find and replace**, including Replace All as a single undoable action.
- **Syntax highlighting** for common shell, Python, Lua, and JavaScript constructs.
- **Line numbers and optional word wrapping**, with visual guides for wrapped lines. Wrapping never changes the file contents.
- **Live theme updates** when you switch your Omarchy theme.
- **UTF-8 file support**, unsaved-change prompts, and desktop file-manager integration.

## Installation

Native packages can be built for **Omarchy on x86_64 or ARM64** and use the system Qt libraries. They also work on Arch Linux x86_64 and Arch Linux ARM aarch64. Install the package whose filename matches your machine's architecture.

**ARM64 status:** The ARM64 package built and passed automated tests under QEMU, but remains untested on a real ARM64 Omarchy desktop.

Download a `.pkg.tar.zst` package from [Releases](https://github.com/seth-reee/Scroll/releases), then install it from your download directory:

```bash
sudo pacman -U ./scroll-*.pkg.tar.zst
```

For native Wayland support:

```bash
sudo pacman -S --needed qt6-wayland
```

Launch **Scroll** from your application menu, or open files from a terminal:

```bash
scroll script.sh notes.md
```

The package includes the application, desktop entry, icon, and license. It replaces the qOmaedit package. To associate a file type with Scroll, use your file manager's **Open With** settings. Scroll saves preferences in `~/.config/Scroll/Scroll.conf`.

For manual installation from a binary tarball, follow the included [installation instructions](packaging/INSTALL.txt). These binaries require Arch's system libraries.

## Usage

Use the **Menu** button for file actions, Find and Replace, and About. Open **Settings** to adjust editor preferences, or use the **Wrap** button in the footer to toggle word wrapping.

| Shortcut | Action |
| --- | --- |
| `Ctrl+N` | New document |
| `Ctrl+O` | Open files |
| `Ctrl+S` | Save |
| `Ctrl+F` | Find and replace |

Syntax highlighting is intentionally lightweight rather than a full language parser. Lines longer than 32,768 characters remain editable but skip highlighting to keep formatting work bounded.

## Development

### Build from source

Install the build dependencies on Arch / Omarchy:

```bash
sudo pacman -S --needed base-devel cmake ninja qt6-base git
```

Clone the repository, then configure and build:

```bash
git clone https://github.com/seth-reee/Scroll.git
cd Scroll
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/scroll
```

The application requires Qt 6.5 or later. Tests are enabled by default; add `-DBUILD_TESTING=OFF` when configuring to build without them.

### Tests

```bash
ctest --test-dir build --output-on-failure
```

The regression suite runs headlessly and covers file handling, unsaved changes, find and replace, per-tab undo, scrolling, and highlighting in large documents.

### Packaging

From a clean, committed checkout, run the package builder as your normal user:

```bash
sudo pacman -S --needed desktop-file-utils
./packaging/build-package.sh
```

The builder runs natively on x86_64 or aarch64, packages the current Git commit, runs the tests, and validates the desktop entry. Outputs are written to `dist/`:

- An Arch Linux package (`.pkg.tar.zst`).
- A binary tarball with manual installation instructions.
- A versioned source archive.
- The package recipe, metadata, and SHA-256 checksums.

Building a package does not install it. If you previously created a development launcher or a symlink in `~/.local/bin`, it may take precedence over the packaged application; see the [installation notes](packaging/INSTALL.txt).

### Performance profiling

An optional Linux probe measures memory and CPU usage with generated files:

```bash
cmake --build build --target profile_memory
./build/profile_memory ./build/scroll
```

The default measurement uses Qt's offscreen backend and isolated settings. To compare with installed KWrite and gedit on your desktop:

```bash
./build/profile_memory --desktop --compare --repeats 2 ./build/scroll
```

The desktop comparison opens temporary test windows. See [REVIEW.md](REVIEW.md) for results, methodology, and limitations.

## Contributing

Bug reports and pull requests are welcome. For code changes, include relevant tests and run the regression suite before submitting.

## License

Scroll is available under the [MIT License](LICENSE).
