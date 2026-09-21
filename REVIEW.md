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
