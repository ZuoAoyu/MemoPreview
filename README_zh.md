# MemoPreview

**MemoPreview** —— 一款基于 Qt6 的 SuperMemo 内容同步与 LaTeX 增量编译工具

MemoPreview 监控 SuperMemo 窗口的内容变化，自动同步至 LaTeX 工作区并利用 latexmk 实现增量编译，持续刷新 PDF 预览。适合配合 [Sumatra PDF](https://www.sumatrapdfreader.org/free-pdf-reader) 实现极致的即时预览体验。

---

## 功能特性

- **实时同步**  
  监控 SuperMemo IE 控件内容，变化时自动更新工作区下的 `main.tex` 文件。

- **自动增量编译**  
  利用 `latexmk -pvc` 实现 LaTeX 文档的持续编译与自动预览，节省等待时间。

- **紧凑界面，快捷操作**  
  基于 Qt6 Widgets 的简洁窗口，支持多种快捷键。

- **编译状态可视化**  
  显示“编译中”、“成功”、“失败”等状态，并支持一键显示编译日志。

- **灵活设置**  
  - 可自定义 latexmk 路径、LaTeX 模板、工作区目录（支持一键打开）。
  - 模板可自由添加、修改、删除。
  - 支持检测和选择多个 SuperMemo 进程。

- **高可靠性**  
  编译进程独立运行，异常时自动重启，防抖触发编译请求，保持稳定。

---

## 依赖环境

- [SuperMemo](https://www.supermemo.com/)（内容源）
- [TeX Live](https://www.tug.org/texlive/)（LaTeX 环境，包括 latexmk）
- [Sumatra PDF](https://www.sumatrapdfreader.org/free-pdf-reader)（建议用于高效预览）
- Windows 10 及以上

---

## 使用方法

1. **下载安装**
    - 前往 [GitHub Releases 页面](https://github.com/ZuoAoyu/MemoPreview/releases) 下载已编译好的 MemoPreview 程序（无需自行编译）。

2. **配置依赖**
    - 确保已安装 SuperMemo、TeX Live，并将 latexmk 加入环境变量。
    - 建议安装并使用 Sumatra PDF 打开工作区下生成的 PDF。

3. **运行 MemoPreview**
    - 解压下载的程序，直接运行 `MemoPreview.exe`（或相应平台的可执行文件）。
    - 首次运行可在设置中指定 latexmk 路径、模板及工作区目录。

---

## License & 法律声明

MemoPreview 使用 Qt 动态库并遵循 Qt 的 [LGPL 许可协议](https://www.gnu.org/licenses/lgpl-3.0.html)。  
本项目代码采用 MIT License 发布。  
本软件使用了第三方依赖，详情见 LICENSE 文件和 Qt 官方说明。

---

## 相关链接

- [SuperMemo 官网](https://www.supermemo.com/)
- [TeX Live 官网](https://www.tug.org/texlive/)
- [Sumatra PDF 官网](https://www.sumatrapdfreader.org/free-pdf-reader)
- [Qt 官方 License 说明](https://doc.qt.io/qt-6/lgpl.html)
