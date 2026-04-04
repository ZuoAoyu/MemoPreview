# MemoPreview

English / [简体中文](./README_zh.md)

**MemoPreview** — A Qt6-based companion for SuperMemo that brings LaTeX-based formula rendering and live PDF preview into your review workflow

SuperMemo remains one of the most powerful tools for incremental learning, but mathematical content often requires extra work: formulas are not a first-class editing and rendering experience, and serious typesetting usually depends on external tools or workarounds. MemoPreview was built to close that gap.

It monitors the content of SuperMemo windows, synchronizes it into a LaTeX workspace, and uses `latexmk` for continuous compilation and PDF preview. In practice, this means you can keep using SuperMemo for collection management and review, while relying on LaTeX for clear, stable presentation of formulas and structured notes. For the smoothest preview experience, [Sumatra PDF](https://www.sumatrapdfreader.org/free-pdf-reader) is highly recommended.

---

## Why MemoPreview

- **Designed for formula-heavy material**
  - When your knowledge base contains fractions, matrices, derivations, aligned equations, or custom notation, plain text and ad-hoc formatting quickly become limiting. MemoPreview lets LaTeX handle what LaTeX is best at.

- **Keeps SuperMemo in the loop**
  - This is not a replacement for SuperMemo. It is a focused companion tool that extends an existing SuperMemo workflow with a reliable math-preview pipeline.

- **Turns workaround-heavy steps into a continuous workflow**
  - Instead of repeatedly copying content into external editors, compiling manually, and checking the result, MemoPreview keeps synchronization and compilation running in the background.

---

## Features

- **Live Sync from SuperMemo**
  - Monitors changes in the SuperMemo IE control and updates `main.tex` in the workspace automatically.

- **LaTeX-first Formula Rendering**
  - Converts the captured content into a LaTeX workspace, so formulas and structured layouts can be rendered with the full LaTeX toolchain instead of relying on in-place formatting tricks.

- **Incremental Compilation and Preview**
  - Uses `latexmk -pvc` to continuously compile LaTeX documents and refresh PDF previews with minimal interruption.

- **Compact, Efficient UI**
  - A simple Qt6 Widgets-based interface with keyboard-friendly operation and low overhead.

- **Compilation Status Visualization**
  - Real-time display of compilation status (in progress, success, failure), with one-click access to logs.

- **Flexible Settings**
  - Customizable latexmk path, LaTeX templates, and workspace directory (with one-click open).
  - Template management: add, edit, or delete as needed, so different note styles or formula layouts can be switched quickly.
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

4. **Work the way you already do in SuperMemo**
    - Select the SuperMemo window to monitor, choose a LaTeX template, and let MemoPreview keep `main.tex` and the compiled PDF in sync while you edit and review.

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
