# qOmaedit 0.2.0: memory and reliability review

## Implementation

The Qt Quick/QML interface has been replaced with Qt Widgets and QPlainTextEdit.
The application now links only Qt Core, Gui, and Widgets directly. Each tab owns
one text document, undo history, cursor, selection, and scroll position. Typing
does not serialize the file into a second string/model.

Line numbers are painted only for visible source blocks. Highlighting uses
display-only extra selections for visible blocks, with cached regular expressions
and cached token ranges. Scrolling replaces those selections, so formatting does
not accumulate across the file or enter its undo history. Physical lines longer
than 32,768 characters skip syntax coloring but remain editable. Coloring remains
generic and single-line, as in the previous editor.

Saving streams UTF-8 in roughly 64 KiB chunks through QSaveFile. Failed writes
leave the tab dirty and open. UTF-8 BOMs and uniform CRLF endings survive a
round trip; mixed LF/CRLF input is normalized to CRLF. Invalid/truncated UTF-8
and NUL-containing files are rejected. Duplicate paths and symlink aliases select
the existing tab; Save As cannot overwrite another open tab's path.

Unsaved-tab protection, find/replace, native file dialogs, wrap guides, settings,
and Omarchy colors remain. Replace All is one undoable operation. Scrolling to
the insertion point is handled by QPlainTextEdit. Hidden tabs remain accessible
through Next/Previous Tab commands and shortcuts.

## Desktop measurements

Measured on this machine under Wayland, Qt 6.11.2, KWrite (kate) 26.08.0 and gedit
50.0. Each sample used a fresh process and isolated settings, a generated `.js`
file with 74-byte source lines, an eight-second warmup, and a one-second CPU
sample. Two runs were made per size; comparator order was rotated. The final
qOmaedit build was remeasured after the last highlighting/UI changes. Values
below are rounded means, in MiB.

| Source lines | Old qOmaedit RSS | New qOmaedit RSS | KWrite RSS | gedit RSS |
| --- | ---: | ---: | ---: | ---: |
| Empty | 123 | 44 | 82 | 55 |
| 1,000 (74 KB) | 129 | 43 | 82 | 57 |
| 20,000 (1.48 MB) | 217 | 54 | 86 | 73 |
| 100,000 (7.4 MB) | 548 | 107 | 104 | 135 |

| Source lines | New qOmaedit PSS | KWrite PSS | gedit PSS |
| --- | ---: | ---: | ---: |
| Empty | 21 | 51 | 31 |
| 1,000 | 21 | 52 | 33 |
| 20,000 | 32 | 56 | 49 |
| 100,000 | 85 | 74 | 111 |

RSS counts resident shared library pages in every process; PSS apportions them.
These are process-memory snapshots, not peak allocations or total GPU/system
memory. Default window sizes/styles and installed library sharing affect results;
one-MiB differences between small samples are noise. The old Qt Quick build was
still CPU-busy at the 100,000-line eight-second sample. One gedit sample also had
background activity. The final widget build sampled 0–1% of one core in every
case, including 0% for both 100,000-line samples.

The rewrite reduces measured empty-file RSS by about 64% and 100,000-line RSS by
about 80%. It uses less memory than both comparators for the smaller files. KWrite
still uses slightly less RSS and about 11 MiB less PSS for the largest file;
qOmaedit is not universally smaller than KWrite.

The 20,000-line/30-key input-and-event-processing test measured a mean of 1.12 ms
(maximum 4.68 ms) with the final highlighting implementation, versus the previous
Qt Quick sample of 27.66 ms. These are synthetic single-run samples and exclude
end-to-end display latency. No file content is omitted or truncated to get these
results; highlighting was enabled.

Reproduce the desktop comparison (opens temporary windows):

```bash
cmake --build build --target profile_memory
./build/profile_memory --desktop --compare --repeats 2 ./build/qomaedit
```

Use `--settle-ms 30000` for a longer warmup. Omit `--desktop --compare` for a
headless qOmaedit-only measurement. Do not compare headless results directly to
native desktop measurements.

## Validation and remaining limits

- Release build and all 15 regression cases pass (including six file round-trip
  variants). Tests cover file errors, malformed input, per-tab undo, save/close,
  find/replace and single-step undo, large-file typing, bounded highlighting,
  long wrapped lines, and automatic scrolling.
- The same suite passes AddressSanitizer/UndefinedBehaviorSanitizer checks.
  Leak detection was disabled for this run; it is not evidence of leak freedom.
- The widget UI was rendered and visually inspected. Native Wayland launches
  and file loading were exercised by the memory benchmark. Interactive native
  file-picker behavior and Qt 6.5 still need platform testing.
- Native Arch packaging installs the executable, desktop entry, icon, and license
  through CMake. The package builder runs the tests and generates a pacman package,
  manual binary tarball, and checksummed source recipe; see README.md.
- Loading still temporarily holds the input bytes and decoded text before
  QPlainTextEdit takes ownership of its copy. Huge single lines and larger files
  can still be expensive; this is not a memory-mapped editor.
- External changes on disk are not detected before saving.
- Theme watching still tracks existing files rather than their parent
  directories, so some missing-file/symlink replacement cases need improvement.
