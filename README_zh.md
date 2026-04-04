# MemoPreview

**MemoPreview** —— 一款为 SuperMemo 补足数学公式展示与 LaTeX 预览能力的 Qt6 辅助工具

SuperMemo 在增量学习和知识复习上的能力毋庸置疑，但一旦内容里出现较多数学公式、推导过程或排版要求较高的笔记，编辑与展示往往就需要依赖额外的折中方案。MemoPreview 正是为这个问题而写。

它持续监控 SuperMemo 窗口内容，自动同步到 LaTeX 工作区，并借助 `latexmk` 完成增量编译和 PDF 预览。这样一来，你可以继续把 SuperMemo 作为知识组织与复习的核心，而把公式渲染、版式控制和输出预览交给 LaTeX。若配合 [Sumatra PDF](https://www.sumatrapdfreader.org/free-pdf-reader)，整体体验会更流畅。

---

## 为什么需要 MemoPreview

- **面向公式密集型内容**
  - 当笔记中包含分式、矩阵、对齐公式、长推导或自定义命令时，纯文本或临时性排版很快就会变得吃力。MemoPreview 让这部分工作回到 LaTeX 最擅长的轨道上。

- **不是替代 SuperMemo，而是补足它**
  - MemoPreview 并不改变你原有的 SuperMemo 工作流，而是在保留其复习与组织能力的前提下，补上一条稳定的数学内容预览链路。

- **把零散折中方案变成连续工作流**
  - 无需在多个工具之间反复复制、手动编译、来回检查结果。内容同步、LaTeX 编译和 PDF 刷新都可以在后台持续进行。

---

## 功能特性

- **SuperMemo 内容实时同步**  
  监控 SuperMemo IE 控件内容，变化时自动更新工作区下的 `main.tex` 文件。

- **以 LaTeX 为核心的公式渲染能力**  
  将捕获到的内容接入 LaTeX 工作区，适合处理数学公式、结构化排版以及更规范的笔记输出。

- **自动增量编译与即时预览**  
  利用 `latexmk -pvc` 持续编译 LaTeX 文档并刷新 PDF 预览，尽量减少中断感。

- **紧凑界面，快捷操作**  
  基于 Qt6 Widgets 的简洁窗口，低负担、易上手，并支持多种快捷键。

- **编译状态可视化**  
  显示“编译中”、“成功”、“失败”等状态，并支持一键显示编译日志。

- **灵活设置**  
  - 可自定义 latexmk 路径、LaTeX 模板、工作区目录（支持一键打开）。
  - 模板可自由添加、修改、删除，方便在不同笔记风格和排版方案之间切换。
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

4. **按原来的 SuperMemo 习惯继续工作**
    - 选择要监控的 SuperMemo 窗口与 LaTeX 模板后，MemoPreview 会在你编辑与复习时持续保持 `main.tex` 和 PDF 预览同步更新。

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
