#ifndef MEDIASCANNER_H
#define MEDIASCANNER_H

#include <QRunnable>
#include <QObject>
#include "MediaInfo.h"

class MediaScanner : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit MediaScanner(const QString &folderPath, QObject *parent = nullptr);
    void run() override;

signals:
    void scanProgress(int progress);
    void scanFinished(QList<MediaInfo> mediaList);
    void scanError(QString errorMsg);

private:
    QString m_folderPath;

    bool isMediaFile(const QString &filePath);
    MediaInfo extractMediaInfo(const QString &filePath);
};

#endif