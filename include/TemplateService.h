#ifndef TEMPLATESERVICE_H
#define TEMPLATESERVICE_H

#include <QMap>
#include <QString>
#include <QStringList>

struct TemplateSnapshot {
    QMap<QString, QString> contentMap;
    QStringList titles;
    QString currentTitle;
};

class TemplateService
{
public:
    TemplateSnapshot loadFromSettings() const;
    void saveLastSelectedTitle(const QString& title) const;
};

#endif // TEMPLATESERVICE_H
