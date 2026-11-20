# <img src="resources/scroll.png" width="48" height="48" alt=""> Scroll

A lightweight text editor for scripts and plain-text files on [Omarchy](https://omarchy.org/), built with Qt 6 Widgets. Scroll follows your desktop theme and keeps everyday editing simple.

> [!IMPORTANT]
> **Package rename:** Future packages use `scroll-editor` instead of `scroll`. The application is still called **Scroll** and launches with `scroll`; your settings stay in `~/.config/Scroll/Scroll.conf`. The new package declares that it replaces the old `scroll` package. Existing release downloads retain their original `scroll-*` filenames; use the filename you downloaded when installing.

## Features

- **Tabbed editing** with independent undo history, cursor position, and scrolling for each document.
- **Find and replace**, with case matching, forward and backward search, and Replace All as a single undoable action.
- **Language-aware syntax highlighting** for common script, source, and data formats, with an option to turn it off.
- **Line numbers and optional word wrapping**, with visual guides for wrapped lines. Wrapping never changes the file contents.
- **Live theme updates** when you switch your Omarchy theme.
- **UTF-8 file support**, unsaved-change prompts, external-change warnings, and desktop file-manager integration.

## Installation

Native packages can be built for **Omarchy on x86_64 or ARM64** and use the system Qt libraries. They also work on Arch Linux x86_64 and Arch Linux ARM aarch64. Install the package whose filename matches your machine's architecture.

**ARM64 status:** The ARM64 package built and passed automated tests under QEMU, but remains untested on a real ARM64 Omarchy desktop.

Download a `.pkg.tar.zst` package from [Releases](https://github.com/seth-reee/Scroll/releases), then install it from your download directory:

```bash
sudo pacman -U ./scroll-editor-*.pkg.tar.zst
```

For native Wayland support:

```bash
sudo pacman -S --needed qt6-wayland
```

Launch **Scroll** from your application menu, or open files from a terminal:

```bash
scroll script.sh notes.md
```

The `scroll-editor` package includes the application, desktop entry, icon, and license. It replaces the previous `scroll` and `qomaedit` packages. The application name remains Scroll and the terminal command remains `scroll`. To associate a file type with Scroll, use your file manager's **Open With** settings. Scroll saves preferences in `~/.config/Scroll/Scroll.conf`.

For manual installation from a binary tarball, follow the included [installation instructions](packaging/INSTALL.txt). These binaries require Arch's system libraries.

## Usage

Use the **Menu** button for file actions, Find and Replace, and About. Open **Settings** to adjust editor preferences, or use the **Wrap** button in the footer to toggle word wrapping.

| Shortcut | Action |
| --- | --- |
| `Ctrl+N` | New document |
| `Ctrl+O` | Open files |
| `Ctrl+S` | Save |
| `Ctrl+F` | Find and replace |
| `Ctrl+G` | Go to line |
| `Ctrl+W` | Close tab |
| `Ctrl+Shift+S` | Save As |
| `Ctrl+Tab` / `Ctrl+Shift+Tab` | Switch tabs |

Settings include a syntax highlighting switch, tab width, and an option to insert spaces when pressing Tab. The status bar shows the cursor position, encoding, and line endings.

Syntax highlighting uses lightweight, language-specific single-line rules rather than a full language parser; multiline strings and comments are not tracked across lines. Lines longer than 32,768 characters remain editable but skip highlighting to keep formatting work bounded.

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

## NixOS and Nix-based Omarchy forks

The flake provides `scroll` and `default` packages for `x86_64-linux` and
`aarch64-linux`, with pinned nixpkgs dependencies. From a checkout:

```sh
nix build .#scroll
nix run .#scroll
```

For a declarative NixOS install, add this input to your system flake:

```nix
inputs.scroll.url = "github:seth-reee/Scroll";
```

Pass `inputs` using `specialArgs = { inherit inputs; };`, then add to a module:

```nix
{ inputs, pkgs, ... }: {
  environment.systemPackages = [
    inputs.scroll.packages.${pkgs.stdenv.hostPlatform.system}.scroll
  ];
}
```

For Home Manager, use the same package in `home.packages` and provide `inputs`
through `extraSpecialArgs`. Update your system's flake lock to get new releases.

Scroll uses its built-in colors if no Omarchy theme is present.
CI publishes Nix artifacts for branch builds and adds Nix release assets on version tags.
The existing manual Arch packaging flow is unchanged.

CI builds and tests Arch and Nix packages for both architectures on native runners. Branch builds are available as workflow artifacts; pushing a matching version tag publishes the packages to GitHub Releases. Download the matching
`scroll-VERSION-nix-SYSTEM.nar.xz`, `.store-path`, and `SHA256SUMS-nix-SYSTEM`
files from a release (replace VERSION and SYSTEM), then import and install:

```sh
sha256sum -c SHA256SUMS-nix-SYSTEM
xz -dc scroll-VERSION-nix-SYSTEM.nar.xz | sudo nix-store --import
nix profile install "$(cat scroll-VERSION-nix-SYSTEM.store-path)"
```

Nix must already be installed. Omit `sudo` for single-user Nix installations.
The exports include the full runtime closure; a declarative flake installation
is preferred on NixOS. Omarchy itself is optional.
