# Reliability and resource review

## Fixed in this pass

- Window close now checks every tab for unsaved changes and offers Save,
  Discard, or Cancel, continuing through remaining modified tabs.
- A failed Save As no longer discards a tab awaiting Save and Close.
- Edits from Find and Replace synchronize even when the editor lacks focus.
  Find preserves its selection; Replace removes the matched text, and Replace
  All no longer severs a binding needed for subsequent tab switches.
- The editor explicitly treats input as plain text, including HTML-looking files.
- Saving tests whether a tab has a path, rather than comparing its filename to
  `Untitled`, which is also a valid filename.
- Opening rejects invalid UTF-8 and NUL-containing files, checks read errors,
  and saving checks the full write before committing the atomic save.
- UTF-8 BOMs and uniform CRLF line endings survive load/save. Mixed LF/CRLF
  files are normalized to CRLF when saved.
- Reopening a path (including symlink aliases) selects the existing tab instead
  of creating competing copies. Save As rejects paths open in another tab.
- The gutter only exposes rows around the viewport, instead of instantiating
  two QML objects for every visual line in the entire document.
- Typing only updates tab labels when modified state changes. Footer counts
  use the text document's block count and editor length instead of splitting
  the entire text on each edit.
- Highlighting reuses compiled regular expressions. String/comment formatting
  takes precedence over keywords and numbers.

## Validation

Release build and headless C++/QML regression tests on Qt 6.11.2. Tests cover
invalid input, failed writes, BOM/CRLF round trips, duplicate paths, tab
notifications, find/replace without focus, repeated tab switches, unsaved
background tabs, failed Save and Close, sequential shutdown, and gutters with
20,001 source lines and long wrapped lines. The large-file test asserts fewer
than 100 gutter rows at the top and after scrolling. This is an object-count
bound, not an end-to-end memory or startup benchmark.

## Remaining opportunities

- The current single TextArea still serializes its entire contents into a
  QString on every edit. Per-tab QTextDocuments would reduce copying and also
  preserve per-tab undo history, selections, and cursor positions.
- External file changes are not detected before saving. A future conflict
  workflow should offer reload, overwrite, or Save As.
- Theme watching only tracks existing files. Watching the parent directory
  would improve recovery when theme files are absent at startup or a theme
  symlink is replaced without changing `theme.name`.
- Highlighting remains generic rather than language-specific and does not
  parse multiline strings/comments.
- The AppImage recipe assumes Debian x86_64 Qt paths. Portable discovery and
  package validation are separate packaging work; the AppImage was not rebuilt
  during this pass.
- Native dialog behavior on a live Wayland desktop and the minimum supported
  Qt 6.5 version still need platform testing.

## Scrolling and memory follow-up

The editor now uses `TextArea.flickable`, letting Qt manage content dimensions
and keep the cursor visible while typing, navigating, and wrapping. Regression
coverage checks added newlines, horizontal scrolling, wrapping, and that manually
scrolling upward does not snap back to the cursor.

Text synchronization now guards its return signal instead of repeatedly reading
and comparing the entire editor text. A synthetic 20,000-line/30-key test measured
mean input/event processing of 33.72 ms before this change and 27.66 ms afterward
on the development machine. These are single-run samples, not display latency
measurements or guarantees. Whole-document serialization remains a bottleneck.

The opt-in `profile_memory` target launches the real Release executable in fresh
processes with isolated settings and highlighting enabled. On Qt 6.11.2, using
offscreen/software rendering and 74-byte generated source lines, observed memory
was approximately:

| Source lines | File size | RSS (MiB) | PSS (MiB) |
| --- | --- | --- | --- |
| Empty | 0 | 70 | 36 |
| 1,000 | 74 KB | 75 | 41 |
| 20,000 | 1.48 MB | 161 | 127 |
| 100,000 | 7.4 MB | 523 | 489 |

RSS includes shared library pages; PSS apportions those pages among processes.
These are snapshots, not peak allocations or total GPU memory. The 100,000-line
run did not meet the probe's idle threshold within its observation window and
used about 5% of one CPU core in the subsequent one-second sample. The smaller
runs sampled 0–1%. The scrolling integration did not materially change memory
use. Large-file memory and responsiveness still need further work; these results
do not establish that the editor is optimally lightweight.
