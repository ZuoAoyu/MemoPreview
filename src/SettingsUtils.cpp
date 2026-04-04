#include "SettingsUtils.h"
#include "Config.h"

#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>

QString SettingsUtils::findInPath(const QString &exeName)
{
    QStringList paths = QProcessEnvironment::systemEnvironment().value("PATH").split(';', Qt::SkipEmptyParts);
    for (const QString& dir : paths) {
        QFileInfo fi(dir + "/" + exeName);
#ifdef Q_OS_WIN
        // 尝试查找.exe
        QFileInfo fiExe(dir + "/" + exeName + ".exe");
        if (fiExe.exists() && fiExe.isExecutable())
            return fiExe.absoluteFilePath();
#endif
        if (fi.exists() && fi.isExecutable())
            return fi.absoluteFilePath();
    }
    return QString();
}

void SettingsUtils::ensureInitialSettings()
{
    QSettings settings{SOFTWARE_NAME, SOFTWARE_NAME};

    // latexmk 路径
    if (!settings.contains("latexmkPath") || settings.value("latexmkPath").toString().isEmpty()) {
        QString latexmkPath = findInPath("latexmk");
        if (!latexmkPath.isEmpty())
            settings.setValue("latexmkPath", latexmkPath);
    }

    // 工作区目录
    if (!settings.contains("workspacePath") || settings.value("workspacePath").toString().isEmpty()) {
        // 用户文档目录下新建 Workspace
        QString docDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        QString defaultWs = docDir + "/MemoPreviewWorkspace";
        QDir dir;
        if (!dir.exists(defaultWs)) {
            dir.mkpath(defaultWs);
        }
        settings.setValue("workspacePath", defaultWs);
    }

    // latexmk 参数
    if (!settings.contains("latexmkArgs") || settings.value("latexmkArgs").toString().isEmpty()) {
        settings.setValue("latexmkArgs", "-pdf -pvc -interaction=nonstopmode -outdir=build");
    }

    // 默认模板
    QStringList templates = settings.value("templates").toStringList();
    if (templates.isEmpty()) {
        QString defaultTemplateTitle = "默认";
        QString defaultTemplateContent =
            R"(\documentclass{article}
\usepackage{amsmath,amssymb}
\providecommand{\hl}[1]{\textbf{#1}}
\providecommand{\hc}[1]{\textbf{\textit{#1}}}
\begin{document}

%CONTENT%

\end{document}
)";
        templates << defaultTemplateTitle;
        settings.setValue("templates", templates);
        settings.setValue("templateContent/" + defaultTemplateTitle, defaultTemplateContent);
    }

    // 创建 main.tex 文件
    QString ws = settings.value("workspacePath").toString();
    if (!ws.isEmpty()) {
        QString mainTexPath = QDir(ws).filePath("main.tex");
        if (!QFile::exists(mainTexPath)) {
            QFile f(mainTexPath);
            if (f.open(QIODevice::WriteOnly)) {
                QString content =
                    R"(\documentclass{article}
\usepackage{amsmath,amssymb}
\providecommand{\hl}[1]{\textbf{#1}}
\providecommand{\hc}[1]{\textbf{\textit{#1}}}
\begin{document}

Welcome to MemoPreview!

\end{document}
)";
                f.write(content.toUtf8());
                f.close();
            }
        }
    }
}
