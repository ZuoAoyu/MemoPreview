# MemoPreview

English / [简体中文](./README_zh.md)

**MemoPreview** — A Qt6-based tool for syncing SuperMemo content to LaTeX and incrementally compiling PDFs

MemoPreview monitors the content of SuperMemo windows, automatically syncs to a LaTeX workspace, and uses latexmk for live incremental compilation and PDF preview. It's highly recommended to use [Sumatra PDF](https://www.sumatrapdfreader.org/free-pdf-reader) for fast preview integration.

---

## Features

- **Live Sync**
  - Monitors changes in the SuperMemo IE control and updates `main.tex` in the workspace automatically.

- **Incremental Compilation**
  - Uses `latexmk -pvc` to continuously compile LaTeX documents and refresh PDF previews with no extra wait.

- **Compact, Efficient UI**
  - Simple Qt6 Widgets-based interface, supports various keyboard shortcuts for productivity.

- **Compilation Status Visualization**
  - Real-time display of compilation status (in progress, success, failure), with one-click access to logs.

- **Flexible Settings**
  - Customizable latexmk path, LaTeX templates, and workspace directory (with one-click open).
  - Template management: add, edit, or delete as needed.
  - Detects multiple SuperMemo processes; allows user to select which to monitor.

- **Robust & Reliable**
  - Compilation runs in an independent process/thread, automatically restarts on failure, debounces rapid changes for stability.

---

## Requirements

- [SuperMemo](https://www.supermemo.com/) (content source)
- [TeX Live](https://www.tug.org/texlive/) (LaTeX system, including latexmk)
- [Sumatra PDF](https://www.sumatrapdfreader.org/free-pdf-reader) (recommended for PDF preview)
- Windows 10 or later

---

## Getting Started

1. **Download**
    - Go to the [GitHub Releases page](https://github.com/ZuoAoyu/MemoPreview/releases) and download the pre-built MemoPreview executable (no compilation needed).

2. **Set Up Dependencies**
    - Make sure SuperMemo and TeX Live are installed and `latexmk` is available in your PATH.
    - For best experience, install Sumatra PDF to view compiled PDFs.

3. **Run MemoPreview**
    - Unzip the downloaded archive and run `MemoPreview.exe` (or the executable for your platform).
    - On first launch, use the Settings menu to specify the paths for latexmk, templates, and workspace if needed.

---

## License & Legal Notice

MemoPreview uses the Qt dynamic libraries under the [LGPL license](https://www.gnu.org/licenses/lgpl-3.0.html).  
This project is released under the MIT License.  
The software uses third-party dependencies. See the LICENSE file and Qt's official documentation for details.

---

## Related Links

- [SuperMemo Official](https://www.supermemo.com/)
- [TeX Live Official](https://www.tug.org/texlive/)
- [Sumatra PDF Official](https://www.sumatrapdfreader.org/free-pdf-reader)
- [Qt License Documentation](https://doc.qt.io/qt-6/lgpl.html)
