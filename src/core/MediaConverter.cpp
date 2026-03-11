#include "core/MediaConverter.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>

MediaConverter::MediaConverter(const MediaInfo &media, const QString &targetFormat, QObject *parent)
    : QObject(parent), QRunnable(), m_media(media), m_targetFormat(targetFormat)
{
    setAutoDelete(true);
    m_totalDuration = media.duration * 1000;
}

void MediaConverter::run()
{
    // 获取源文件信息
    QFileInfo srcFileInfo(m_media.path);
    // 获取输出目录路径
    QString outputDir = srcFileInfo.absolutePath();
    // 生成输出文件名
    QString outputName = srcFileInfo.baseName() + "_converted." + m_targetFormat;
    // 生成完整的输出文件路径
    QString outputPath = QDir(outputDir).filePath(outputName);

    // 默认音频和视频编码器设置为“copy”
    QString audioCodec = "copy";
    QString videoCodec = "copy";
    // 将目标格式转换为小写
    QString fmt = m_targetFormat.toLower();

    // 根据目标格式设置相应的音频和视频编码器
    if (fmt == "mp3") {
        audioCodec = "libmp3lame";
    } else if (fmt == "wav") {
        audioCodec = "pcm_s16le";
    } else if (fmt == "aac" || fmt == "m4a") {
        audioCodec = "aac";
    } else if (fmt == "mp4") {
        audioCodec = "aac";
        videoCodec = (m_media.mediaType == Video) ? "libx264" : "none";
    } else if (fmt == "avi") {
        audioCodec = "libmp3lame";
        videoCodec = (m_media.mediaType == Video) ? "libxvid" : "none";
    } else if (fmt == "mkv") {
        audioCodec = "aac";
        videoCodec = (m_media.mediaType == Video) ? "libx264" : "none";
    }

    // 构建FFmpeg命令行参数列表
    QStringList ffmpegArgs;
    // 添加输入文件参数
    ffmpegArgs << "-i" << m_media.path;

    // 如果媒体类型是视频且视频编码器不为空，则添加视频编码器参数
    if (m_media.mediaType == Video && videoCodec != "none" && !videoCodec.isEmpty()) {
        ffmpegArgs << "-c:v" << videoCodec;
    }
    // 添加音频编码器参数
    ffmpegArgs << "-c:a" << audioCodec;
    // 添加覆盖输出文件的参数和输出文件路径
    ffmpegArgs << "-y" << outputPath;

    // 创建QProcess对象用于运行FFmpeg进程
    QProcess ffmpegProcess;
    // 获取FFmpeg可执行文件的路径
    QString ffmpegPath = QCoreApplication::applicationDirPath() + "/ffmpeg.exe";
    // 启动FFmpeg进程并传递参数
    ffmpegProcess.start(ffmpegPath, ffmpegArgs);

    // 检查FFmpeg进程是否成功启动
    if (!ffmpegProcess.waitForStarted()) {
        // 发射转换错误信号
        emit convertError("FFmpeg启动失败：" + ffmpegProcess.errorString());
        return;
    }
    // 连接标准输出读取信号到槽函数
    connect(&ffmpegProcess, &QProcess::readyReadStandardOutput, this, [&]() {
        QByteArray data = ffmpegProcess.readAllStandardOutput();
        parseFFmpegOutput(QString::fromUtf8(data));
    });
    // 连接标准错误输出读取信号到槽函数
    connect(&ffmpegProcess, &QProcess::readyReadStandardError, this, [&]() {
        QByteArray data = ffmpegProcess.readAllStandardError();
        parseFFmpegOutput(QString::fromUtf8(data));
    });

    // 等待FFmpeg进程完成
    ffmpegProcess.waitForFinished(-1);

    // 检查FFmpeg进程退出码，如果不为0表示出错
    if (ffmpegProcess.exitCode() != 0) {
        QString errorOutput = QString::fromUtf8(ffmpegProcess.readAllStandardError());
        // 发射转换错误信号
        emit convertError("转换失败：" + errorOutput);
    }

    // 发射转换完成信号
    emit convertFinished(outputPath);
}

void MediaConverter::parseFFmpegOutput(const QString &output)
{
    if (m_totalDuration <= 0) return;

    QRegularExpression re(R"(\s(\d{2}:\d{2}:\d{2}\.\d{2}))");
    QRegularExpressionMatchIterator it = re.globalMatch(output);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString timeStr = match.captured(1);

        QStringList parts = timeStr.split(':');
        if (parts.size() != 3) continue;

        int hours = parts[0].toInt();
        int minutes = parts[1].toInt();
        double seconds = parts[2].toDouble();
        qint64 currentMs = (hours * 3600 + minutes * 60 + seconds) * 1000;

        int progress = qMin(100, (int)(currentMs * 100 / m_totalDuration));
        emit convertProgress(progress);
    }
}