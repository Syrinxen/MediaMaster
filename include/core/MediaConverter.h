#ifndef MEDIACONVERTER_H
#define MEDIACONVERTER_H

#include <QObject>
#include <QRunnable>
#include <QProcess>
#include "MediaInfo.h"

class MediaConverter : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit MediaConverter(const MediaInfo &media, const QString &targetFormat, QObject *parent = nullptr);
    void run() override;

signals:
    void convertProgress(int progress);
    void convertFinished(QString outputPath);
    void convertError(QString errorMsg);

private:
    MediaInfo m_media;
    QString m_targetFormat;
    qint64 m_totalDuration;

    void parseFFmpegOutput(const QString &output);
};

#endif