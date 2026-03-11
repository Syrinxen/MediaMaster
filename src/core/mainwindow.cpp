#include "ui_mainwindow.h"
#include "core/mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QListWidgetItem>
#include <QAudioOutput>
#include <QMenu>
#include <QUuid>
#include <QTimer>
#include <QEventLoop>
#include <QMediaMetaData>
#include <QVideoWidget>
#include <QInputDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_threadPool(QThreadPool::globalInstance())
    , m_mediaPlayer(new QMediaPlayer())
{
    m_mediaPlayer->setParent(this);

    QAudioOutput *audioOutput = new QAudioOutput(this);
    audioOutput->setVolume(1.0);
    m_mediaPlayer->setAudioOutput(audioOutput);

    ui->setupUi(this);
    setWindowTitle("MediaMaster - 多媒体影音管理系统");
    ui->progressBar->hide();
    ui->labStatus->setText("已就绪");

    m_videoWidget = new QVideoWidget(this);
    m_mediaPlayer->setVideoOutput(m_videoWidget);

    QVBoxLayout *mainLayout = ui->verticalLayout;

    QLayoutItem *listItem = mainLayout->takeAt(2);
    mainLayout->insertWidget(2, m_videoWidget, 1);
    mainLayout->insertItem(3, listItem);

    m_videoWidget->setVisible(false);

    m_videoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_videoWidget->setMinimumHeight(200);

    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged,
            this, &MainWindow::onPlaybackStateChanged);
    connect(m_mediaPlayer, &QMediaPlayer::mediaStatusChanged,
            this, &MainWindow::onMediaStatusChanged);
    connect(ui->listWidgetMedia, &QListWidget::currentItemChanged,
        this, &MainWindow::on_listWidgetMedia_currentItemChanged);
    ui->listWidgetMedia->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->listWidgetMedia, &QListWidget::customContextMenuRequested,
            this, &MainWindow::on_listWidgetMedia_customContextMenuRequested);

    connect(m_mediaPlayer, &QMediaPlayer::durationChanged,
        this, &MainWindow::on_durationChanged);
    connect(m_mediaPlayer, &QMediaPlayer::positionChanged,
        this, &MainWindow::on_positionChanged);

    connect(ui->sliderProgress, &QSlider::sliderMoved,
        this, &MainWindow::on_sliderProgress_sliderMoved);
}

void MainWindow::on_durationChanged(qint64 duration)
{
    ui->sliderProgress->setRange(0, (int)duration);
    ui->sliderProgress->setEnabled(duration > 0);

    int seconds = duration / 1000;
    int minutes = seconds / 60;
    seconds %= 60;
    QString totalStr = QString::asprintf("%02d:%02d", minutes, seconds);

    QString currentText = ui->labelTime->text();
    currentText = currentText.section('/', 0, 0).trimmed();
    ui->labelTime->setText(currentText + " / " + totalStr);
}

void MainWindow::on_positionChanged(qint64 position)
{
    if (!ui->sliderProgress->isSliderDown()) {
        ui->sliderProgress->setValue((int)position);
    }

    int seconds = position / 1000;
    int minutes = seconds / 60;
    seconds %= 60;
    QString currentStr = QString::asprintf("%02d:%02d", minutes, seconds);

    int totalSec = m_mediaPlayer->duration() / 1000;
    int totalMin = totalSec / 60;
    totalSec %= 60;
    QString totalStr = QString::asprintf("%02d:%02d", totalMin, totalSec);

    ui->labelTime->setText(currentStr + " / " + totalStr);
}

void MainWindow::on_sliderProgress_sliderMoved(int value)
{
    m_mediaPlayer->setPosition(value);
}

void MainWindow::on_listWidgetMedia_customContextMenuRequested(const QPoint &pos)
{
    QListWidgetItem *item = ui->listWidgetMedia->itemAt(pos);
    if (!item) return;

    ui->listWidgetMedia->setCurrentItem(item);

    QMenu menu(this);
    QAction *playAction = menu.addAction("播放");
    QAction *renameAction = menu.addAction("重命名");
    QAction *deleteAction = menu.addAction("删除");

    QAction *selectedAction = menu.exec(ui->listWidgetMedia->viewport()->mapToGlobal(pos));

    if (selectedAction == playAction) {
        on_btnPlay_clicked();
    } else if (selectedAction == renameAction) {
        QString mediaId = ui->listWidgetMedia->currentItem()->data(Qt::UserRole).toString();
        MediaInfo *mediaPtr = nullptr;
        for (auto &media : m_mediaList) {
            if (media.id == mediaId) {
                mediaPtr = &media;
                break;
            }
        }
        if (!mediaPtr) return;

        QString oldPath = mediaPtr->path;
        QFileInfo fileInfo(oldPath);
        QString oldBaseName = fileInfo.baseName();
        QString suffix = fileInfo.suffix();

        if (m_mediaPlayer->source() == QUrl::fromLocalFile(oldPath) &&
            m_mediaPlayer->playbackState() != QMediaPlayer::StoppedState) {
            QMessageBox::warning(this, "文件正在播放",
                                 "该文件正在播放，请先停止播放再重命名。");
            return;
        }

        bool ok;
        QString newBaseName = QInputDialog::getText(this, "重命名文件",
                                                     "请输入新的文件名（不含扩展名）：",
                                                     QLineEdit::Normal,
                                                     oldBaseName, &ok);
        if (!ok || newBaseName.isEmpty() || newBaseName == oldBaseName)
            return;

        QString newPath = fileInfo.absolutePath() + "/" + newBaseName + "." + suffix;

        if (QFile::exists(newPath)) {
            QMessageBox::StandardButton reply = QMessageBox::question(this, "文件已存在",
                QString("目标文件已存在：\n%1\n是否覆盖？").arg(newPath),
                QMessageBox::Yes | QMessageBox::No);
            if (reply != QMessageBox::Yes) return;
            if (!QFile::remove(newPath)) {
                QMessageBox::critical(this, "错误", "无法删除已存在的目标文件。");
                return;
            }
        }

        if (QFile::rename(oldPath, newPath)) {
            mediaPtr->name = newBaseName;
            mediaPtr->path = newPath;

            QListWidgetItem *item = ui->listWidgetMedia->currentItem();
            QString itemText = QString("[%1] %2.%3 | 时长：%4秒 | 大小：%5")
                                   .arg(mediaPtr->mediaType == Video ? "视频" : "音频")
                                   .arg(mediaPtr->name)
                                   .arg(mediaPtr->format)
                                   .arg(mediaPtr->duration)
                                   .arg(formatFileSize(mediaPtr->size));
            item->setText(itemText);

            ui->labStatus->setText(QString("文件已重命名为：%1").arg(newBaseName + "." + suffix));
        } else {
            QMessageBox::critical(this, "重命名失败", "无法重命名文件，可能文件正在使用或权限不足。");
        }
    } else if (selectedAction == deleteAction) {
        QString mediaId = ui->listWidgetMedia->currentItem()->data(Qt::UserRole).toString();
        QString filePath;
        for (const MediaInfo &media : m_mediaList) {
            if (media.id == mediaId) {
                filePath = media.path;
                break;
            }
        }

        if (filePath.isEmpty()) return;

        QMessageBox::StandardButton reply = QMessageBox::question(this, "确认删除",
            QString("确定要永久删除文件吗？\n%1").arg(filePath),
            QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) return;

        QFile file(filePath);
        if (file.remove()) {
            delete ui->listWidgetMedia->currentItem();

            for (int i = 0; i < m_mediaList.size(); ++i) {
                if (m_mediaList[i].id == mediaId) {
                    m_mediaList.removeAt(i);
                    break;
                }
            }

            ui->labStatus->setText(QString("文件已删除，剩余 %1 个文件").arg(m_mediaList.size()));
        } else {
            QMessageBox::critical(this, "删除失败", "无法删除文件：" + file.errorString());
        }
    }
}

QString MainWindow::formatFileSize(qint64 bytes) const {
    if (bytes < 1024) return QString::number(bytes) + " B";
    if (bytes < 1024 * 1024) return QString::number(bytes / 1024.0, 'f', 1) + " KB";
    if (bytes < 1024 * 1024 * 1024) return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + " MB";
    return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
}

void MainWindow::on_listWidgetMedia_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current) {
        ui->comboConvertFormat->clear();
        ui->comboConvertFormat->addItem("MP4");
        ui->comboConvertFormat->addItem("MP3");
        ui->comboConvertFormat->addItem("AVI");
        ui->comboConvertFormat->addItem("MKV");
        ui->comboConvertFormat->addItem("WAV");
        return;
    }

    QString mediaId = current->data(Qt::UserRole).toString();
    for (const MediaInfo &media : m_mediaList) {
        if (media.id == mediaId) {
            ui->comboConvertFormat->clear();
            if (media.mediaType == Audio) {
                ui->comboConvertFormat->addItem("MP3");
                ui->comboConvertFormat->addItem("WAV");
            } else {
                ui->comboConvertFormat->addItem("MP4");
                ui->comboConvertFormat->addItem("MP3");
                ui->comboConvertFormat->addItem("AVI");
                ui->comboConvertFormat->addItem("MKV");
                ui->comboConvertFormat->addItem("WAV");
            }
            break;
        }
    }
}

MainWindow::~MainWindow()
{
    m_threadPool->waitForDone();
    delete ui;
}

void MainWindow::on_btnSelectFolder_clicked()
{
    if (m_isScanning) {
        QMessageBox::information(this, "提示", "正在扫描中，请稍后...");
        return;
    }

    QString folderPath = QFileDialog::getExistingDirectory(this, "选择要扫描的文件夹");
    if (folderPath.isEmpty()) {
        return;
    }

    m_isScanning = true;
    ui->btnSelectFolder->setEnabled(false);
    ui->progressBar->show();
    ui->progressBar->setValue(0);
    ui->labStatus->setText("正在扫描音视频文件...");

    MediaScanner *scanner = new MediaScanner(folderPath, this);
    connect(scanner, &MediaScanner::scanProgress, this, &MainWindow::onScanProgress);
    connect(scanner, &MediaScanner::scanFinished, this, &MainWindow::onScanFinished);
    connect(scanner, &MediaScanner::scanError, this, [=](QString errorMsg) {
        QMessageBox::warning(this, "扫描失败", errorMsg);
        ui->progressBar->hide();
        ui->labStatus->setText("扫描失败：" + errorMsg);
        m_isScanning = false;
        ui->btnSelectFolder->setEnabled(true);
    });
    m_threadPool->start(scanner);
}

void MainWindow::onScanProgress(int progress)
{
    ui->progressBar->setValue(progress);
}

void MainWindow::onScanFinished(QList<MediaInfo> mediaList)
{
    ui->progressBar->hide();
    ui->listWidgetMedia->clear();
    m_mediaList = mediaList;

    if (mediaList.isEmpty()) {
        QMessageBox::information(this, "扫描完成", "未找到支持的音视频文件（mp3/mp4/avi等）");
        ui->labStatus->setText("就绪：未找到音视频文件");
    } else {
        for (const MediaInfo &media : mediaList) {
            QListWidgetItem *item = new QListWidgetItem(ui->listWidgetMedia);
            QString itemText = QString("[%1] %2.%3 | 时长：%4秒 | 大小：%5")
                       .arg(media.mediaType == Video ? "视频" : "音频")
                       .arg(media.name)
                       .arg(media.format)
                       .arg(media.duration)
                       .arg(formatFileSize(media.size));
            item->setText(itemText);
            item->setData(Qt::UserRole, media.id);
            ui->listWidgetMedia->addItem(item);
        }
        ui->labStatus->setText(QString("就绪：共找到 %1 个音视频文件").arg(mediaList.size()));
        QMessageBox::information(this, "扫描完成",
                                 QString("成功扫描到 %1 个音视频文件").arg(mediaList.size()));
    }



    m_isScanning = false;
    ui->btnSelectFolder->setEnabled(true);
}

void MainWindow::on_btnPlay_clicked()
{
    QListWidgetItem *selectedItem = ui->listWidgetMedia->currentItem();
    if (!selectedItem) {
        QMessageBox::warning(this, "播放提示", "请先选中一个音视频文件！");
        return;
    }

    QString mediaId = selectedItem->data(Qt::UserRole).toString();
    MediaInfo selectedMedia;
    bool found = false;
    for (const MediaInfo &media : m_mediaList) {
        if (media.id == mediaId) {
            selectedMedia = media;
            found = true;
            break;
        }
    }
    if (!found) return;

    QMediaPlayer::PlaybackState currentState = m_mediaPlayer->playbackState();
    QUrl currentSource = m_mediaPlayer->source();
    QUrl newSource = QUrl::fromLocalFile(selectedMedia.path);

    if (currentSource == newSource) {
        if (currentState == QMediaPlayer::PlayingState) {
            m_mediaPlayer->pause();
        } else {
            m_mediaPlayer->play();
        }
    } else {
        m_mediaPlayer->setSource(newSource);
        m_mediaPlayer->play();
        ui->labStatus->setText("正在播放：" + selectedMedia.name);
        m_videoWidget->setVisible(selectedMedia.mediaType == Video);
    }
}


void MainWindow::onPlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    if (state == QMediaPlayer::PlayingState) {
        ui->btnPlay->setText("暂停");
    } else if (state == QMediaPlayer::PausedState) {
        ui->btnPlay->setText("播放");
    } else if (state == QMediaPlayer::StoppedState) {
        ui->btnPlay->setText("播放");
        ui->labStatus->setText("播放结束");
        m_videoWidget->setVisible(false);
    }
}


void MainWindow::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::InvalidMedia) {
        QMessageBox::warning(this, "播放错误", "无法播放该媒体文件（格式不支持/文件损坏）");
        ui->labStatus->setText("播放失败：无效的媒体文件");
    }
}


void MainWindow::on_btnConvert_clicked()
{
    if (m_isConverting) {
        QMessageBox::information(this, "提示", "已有转换任务在进行中");
        return;
    }

    QListWidgetItem *selectedItem = ui->listWidgetMedia->currentItem();
    if (!selectedItem) {
        QMessageBox::warning(this, "转换提示", "请先选中一个音视频文件！");
        return;
    }

    QString targetFormat = ui->comboConvertFormat->currentText().toLower();
    if (targetFormat.isEmpty()) {
        QMessageBox::warning(this, "转换提示", "请选择目标格式！");
        return;
    }

    QString mediaId = selectedItem->data(Qt::UserRole).toString();
    bool found = false;
    for (const MediaInfo &media : m_mediaList) {
        if (media.id == mediaId) {
            m_selectedMedia = media;
            found = true;
            break;
        }
    }
    if (!found) {
        QMessageBox::warning(this, "错误", "未找到选中媒体信息");
        return;
    }

    m_isConverting = true;
    ui->btnConvert->setEnabled(false);
    ui->comboConvertFormat->setEnabled(false);

    ui->progressBarConvert->show();
    ui->progressBarConvert->setValue(0);
    ui->labStatus->setText("正在转换：" + m_selectedMedia.name);

    MediaConverter *converter = new MediaConverter(m_selectedMedia, targetFormat, this);
    connect(converter, &MediaConverter::convertProgress, this, &MainWindow::onConvertProgress);
    connect(converter, &MediaConverter::convertFinished, this, &MainWindow::onConvertFinished);
    connect(converter, &MediaConverter::convertError, this, &MainWindow::onConvertError);
    m_threadPool->start(converter);
}


void MainWindow::onConvertProgress(int progress)
{
    ui->progressBarConvert->setValue(progress);
}


void MainWindow::onConvertFinished(QString outputPath)
{
    ui->progressBarConvert->hide();
    ui->labStatus->setText("转换完成：" + outputPath);
    QMessageBox::information(this, "转换成功", "文件已转换完成！\n输出路径：" + outputPath);
    m_isConverting = false;
    ui->btnConvert->setEnabled(true);
    ui->comboConvertFormat->setEnabled(true);

    QFileInfo fileInfo(outputPath);
    if (!fileInfo.exists()) {
        return;
    }

    MediaInfo newMedia;
    newMedia.id = QUuid::createUuid().toString().remove("{").remove("}");
    newMedia.name = fileInfo.baseName();
    newMedia.path = fileInfo.absoluteFilePath();
    newMedia.format = fileInfo.suffix().toLower();
    newMedia.size = fileInfo.size();
    newMedia.addTime = QDateTime::currentDateTime();
    newMedia.playCount = 0;
    newMedia.isCollected = false;

    QString fmt = newMedia.format;
    if (fmt == "mp3" || fmt == "wav" || fmt == "flac" || fmt == "aac" || fmt == "ogg") {
        newMedia.mediaType = Audio;
    } else {
        newMedia.mediaType = Video;
    }

    QMediaPlayer tempPlayer;
    tempPlayer.setSource(QUrl::fromLocalFile(outputPath));

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.start(5000);

    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(&tempPlayer, &QMediaPlayer::mediaStatusChanged, &loop, &QEventLoop::quit);
    loop.exec();

    if (timer.isActive()) {
        if (tempPlayer.mediaStatus() == QMediaPlayer::LoadedMedia ||
            tempPlayer.mediaStatus() == QMediaPlayer::BufferedMedia) {
            newMedia.duration = tempPlayer.duration() / 1000;
        }
    } else {
        newMedia.duration = 0;
    }

    if (newMedia.mediaType == Video) {
        QMediaMetaData meta = tempPlayer.metaData();
        QVariant res = meta.value(QMediaMetaData::Resolution);
        if (res.isValid()) {
            QSize size = res.toSize();
            newMedia.resolution = QString("%1x%2").arg(size.width()).arg(size.height());
        } else {
            newMedia.resolution = "";
        }
    }

    m_mediaList.append(newMedia);

    QString itemText = QString("[%1] %2.%3 | 时长：%4秒 | 大小：%5")
                           .arg(newMedia.mediaType == Video ? "视频" : "音频")
                           .arg(newMedia.name)
                           .arg(newMedia.format)
                           .arg(newMedia.duration)
                           .arg(formatFileSize(newMedia.size));

    QListWidgetItem *item = new QListWidgetItem(itemText);
    item->setData(Qt::UserRole, newMedia.id);
    ui->listWidgetMedia->addItem(item);

    ui->labStatus->setText(QString("已添加转换后的文件，共 %1 个文件").arg(m_mediaList.size()));
}

void MainWindow::onConvertError(QString errorMsg)
{
    ui->progressBarConvert->hide();
    ui->labStatus->setText("转换失败：" + errorMsg);
    QMessageBox::warning(this, "转换失败", errorMsg);
    m_isConverting = false;
    ui->btnConvert->setEnabled(true);
    ui->comboConvertFormat->setEnabled(true);
}

void MainWindow::on_listWidgetMedia_doubleClicked(const QModelIndex &index)
{
    on_btnPlay_clicked();
}
