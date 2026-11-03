#pragma once

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QStringList>

namespace traffic {

inline QString resolve_scenario_path(const QString& requested_path = {}) {
    if (!requested_path.isEmpty()) {
        return QFileInfo(requested_path).absoluteFilePath();
    }

    const QString app_directory = QCoreApplication::applicationDirPath();
    const QStringList candidates{
        QDir(app_directory).filePath(QStringLiteral("config/scenarios.json")),
        QDir(app_directory).filePath(QStringLiteral("../config/scenarios.json")),
        QDir::current().filePath(QStringLiteral("config/scenarios.json"))
    };
    for (const QString& candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return QFileInfo(candidate).absoluteFilePath();
        }
    }
    return QDir(app_directory).filePath(QStringLiteral("config/scenarios.json"));
}

} // namespace traffic
