#ifndef MEDIAINFO_H
#define MEDIAINFO_H

#include <QString>
#include <QDateTime>

enum MediaType {
    Audio = 0,
    Video = 1
};

struct MediaInfo {
    QString id;
    QString name;
    QString path;
    QString format;
    qint64 size;
    int duration;
    QString resolution;
    MediaType mediaType;
    int playCount;
    QDateTime addTime;
    bool isCollected;

    bool operator==(const MediaInfo &other) const {
        return id == other.id;
    }

    MediaInfo() : size(0), duration(0), playCount(0),
        mediaType(Audio), isCollected(false) {}
};

#endif