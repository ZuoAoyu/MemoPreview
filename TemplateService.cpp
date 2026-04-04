#include "TemplateService.h"

#include <QSettings>
#include "Config.h"

TemplateSnapshot TemplateService::loadFromSettings() const
{
    QSettings settings{SOFTWARE_NAME, SOFTWARE_NAME};
    TemplateSnapshot snapshot;
    snapshot.titles = settings.value("templates").toStringList();
    const QString lastTemplate = settings.value("lastTemplateTitle").toString();

    for (const auto& title : snapshot.titles) {
        const QString content = settings.value("templateContent/" + title, "").toString();
        snapshot.contentMap[title] = content;
    }

    if (snapshot.contentMap.contains(lastTemplate)) {
        snapshot.currentTitle = lastTemplate;
    } else if (!snapshot.contentMap.isEmpty()) {
        snapshot.currentTitle = snapshot.contentMap.firstKey();
        settings.setValue("lastTemplateTitle", snapshot.currentTitle);
    } else {
        settings.remove("lastTemplateTitle");
    }

    return snapshot;
}

void TemplateService::saveLastSelectedTitle(const QString& title) const
{
    QSettings settings{SOFTWARE_NAME, SOFTWARE_NAME};
    settings.setValue("lastTemplateTitle", title);
}
