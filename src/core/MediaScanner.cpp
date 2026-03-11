#include "core/MediaScanner.h"
#include <QDir>
#include <QUuid>
#include <QFileInfo>
#include <QMediaPlayer>
#include <QEventLoop>
#include "core/MediaInfo.h"
#include <QMediaMetaData>
#include <QSize>
#include <QTimer>

MediaScanner::MediaScanner(const QString &folderPath, QObject *parent)
    : QObject(parent), QRunnable(), m_folderPath(folderPath)
{
    setAutoDelete(true);
}

void MediaScanner::run()
{
    QDir dir(m_folderPath);
    if (!dir.exists()) {
        emit scanError("扫描路径不存在：" + m_folderPath);
        return;
    }

    QStringList filters = {"*.mp4", "*.avi", "*.mkv", "*.mp3", "*.wav", "*.flac", "*.aac"};
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files | QDir::NoDotAndDotDot);

    int total = fileList.size();
    int current = 0;
    QList<MediaInfo> mediaList;

    for (const QFileInfo &fileInfo : fileList) {
        current++;
        emit scanProgress((current * 100) / total);

        MediaInfo media = extractMediaInfo(fileInfo.absoluteFilePath());
        if (!media.id.isEmpty()) {
            mediaList.append(media);
        }
    }

    emit scanFinished(mediaList);
}

bool MediaScanner::isMediaFile(const QString &filePath)
{
    QString suffix = QFileInfo(filePath).suffix().toLower();
    QStringList supportFormats = {"mp4", "avi", "mkv", "mp3", "wav", "flac", "aac"};
    return supportFormats.contains(suffix);
}

MediaInfo MediaScanner::extractMediaInfo(const QString &filePath)
{
    MediaInfo media;
    QFileInfo fileInfo(filePath);

    media.id = QUuid::createUuid().toString().remove("{").remove("}");
    media.name = fileInfo.baseName();
    media.path = fileInfo.absoluteFilePath();
    media.format = fileInfo.suffix().toLower();
    media.size = fileInfo.size();
    media.addTime = QDateTime::currentDateTime();

    QMediaPlayer player;
    player.setSource(QUrl::fromLocalFile(filePath));

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.start(10000);

    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(&player, &QMediaPlayer::mediaStatusChanged, &loop, &QEventLoop::quit);
    loop.exec();

    if (!timer.isActive()) {
        return MediaInfo();
    }

    if (player.mediaStatus() == QMediaPlayer::LoadedMedia) {
        media.duration = player.duration() / 1000;

        QMediaMetaData metaData = player.metaData();
        QVariant resolution = metaData.value(QMediaMetaData::Resolution);

        if (resolution.isValid()) {
            QSize size = resolution.toSize();
            media.resolution = QString("%1x%2").arg(size.width()).arg(size.height());
            media.mediaType = Video;
        } else {
            media.resolution = "";
            media.mediaType = Audio;
        }
    } else {
        media = MediaInfo();
    }

    player.stop();
    return media;
}