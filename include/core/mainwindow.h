#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QVideoWidget>
#include <QMainWindow>
#include <QThreadPool>
#include <QMediaPlayer>
#include <QListWidgetItem>
#include "core/MediaInfo.h"
#include "core/MediaScanner.h"
#include "core/MediaConverter.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnSelectFolder_clicked();
    void on_btnPlay_clicked();

    void onScanProgress(int progress);
    void onScanFinished(QList<MediaInfo> mediaList);

    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);

    void on_btnConvert_clicked();
    void onConvertProgress(int progress);
    void onConvertFinished(QString outputPath);
    void onConvertError(QString errorMsg);

    void on_listWidgetMedia_doubleClicked(const QModelIndex &index);
    void on_listWidgetMedia_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
    void on_listWidgetMedia_customContextMenuRequested(const QPoint &pos);

    void on_durationChanged(qint64 duration);
    void on_positionChanged(qint64 position);
    void on_sliderProgress_sliderMoved(int value);
private:
    QVideoWidget *m_videoWidget;
    Ui::MainWindow *ui;
    QThreadPool *m_threadPool;
    QMediaPlayer *m_mediaPlayer;
    QList<MediaInfo> m_mediaList;
    MediaInfo m_selectedMedia;
    bool m_isScanning = false;
    bool m_isConverting = false;
    QString formatFileSize(qint64 bytes) const;
};

#endif