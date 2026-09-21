#pragma once

#include <QString>
#include <QMetaType>

struct GameEntry {
    qint64 appId = 0;
    QString name;
    qint64 playtimeMinutes = 0;
    qint64 rtimeLastPlayed = 0;
    bool installed = false;
    QString boxArtPath; // local cache path; empty if both art sources failed
};

Q_DECLARE_METATYPE(GameEntry)
