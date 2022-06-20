#include "mainwindow.hpp"

#include <fmt/chrono.h>
#include <fmt/core.h>

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFontMetricsF>
#include <QInputDialog>
#include <QMediaMetaData>
#include <QMessageBox>
#include <QProcess>
#include <QStandardPaths>
#include <QStringList>
#include <QUrl>
#include <chrono>

#include "./ui_mainwindow.h"
#include "processwidget.hpp"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    player_.setVideoOutput(ui->videoWidget);
    ui->rangeSlider_selected->SetOption(RangeSlider::DoubleHandles);
    ui->pushButton_save->setEnabled(false);
    connect(ui->pushButton_play_pause, &QPushButton::pressed, this, &MainWindow::play_pause_button_pressed);
    connect(ui->horizontalSlider_current_pos, &QSlider::valueChanged, [=](int position) {
        std::chrono::seconds new_position(position);
        this->update_video_position_display(new_position);
        this->change_video_position(new_position);
    });
    connect(ui->timeEdit_current_time, &QTimeEdit::userTimeChanged, [=](QTime time) {
        auto new_position =
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::milliseconds(time.msecsSinceStartOfDay()));
        this->update_video_position_slider(new_position);
        this->change_video_position(new_position);
    });
    connect(ui->timeEdit_start_time, &QTimeEdit::userTimeChanged, [=](QTime time) {
        using namespace std::literals::chrono_literals;
        auto new_position =
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::milliseconds(time.msecsSinceStartOfDay()));
        this->update_start_time_slider(new_position);
    });
    connect(ui->timeEdit_end_time, &QTimeEdit::userTimeChanged, [=](QTime time) {
        using namespace std::literals::chrono_literals;
        auto new_position =
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::milliseconds(time.msecsSinceStartOfDay()));
        this->update_end_time_slider(new_position);
    });
    connect(ui->rangeSlider_selected, &RangeSlider::lowerValueChanged, [=](int position) {
        std::chrono::seconds new_position(position);
        this->update_start_time_display(new_position);
    });
    connect(ui->rangeSlider_selected, &RangeSlider::upperValueChanged, [=](int position) {
        std::chrono::seconds new_position(position);
        this->update_end_time_display(new_position);
    });
    connect(ui->actionopen, &QAction::triggered, this, &MainWindow::open_video);
    connect(ui->pushButton_save, &QPushButton::pressed, this, &MainWindow::save_result);
    connect(&player_, &QMediaPlayer::errorOccurred, this, &MainWindow::show_videoerror);
    connect(&player_, &QMediaPlayer::durationChanged, this, &MainWindow::update_duration);
    connect(&player_, &QMediaPlayer::positionChanged, [=](qint64 position) {
        if (ui->horizontalSlider_current_pos->isSliderDown()) {
            return;
        }
        using std::chrono::duration_cast;
        using std::chrono::milliseconds;
        using std::chrono::seconds;
        auto new_position = duration_cast<seconds>(milliseconds(position));
        auto current_position = seconds(this->ui->horizontalSlider_current_pos->value());
        if (new_position != current_position) {
            this->update_video_position_display(new_position);
            this->update_video_position_slider(new_position);
        }
    });
    connect(ui->actionenable_tracking_of_current_time_slider, &QAction::triggered, this,
            &MainWindow::change_curent_time_slider_tracking_state);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::play_pause_button_pressed() {
    if (ui->pushButton_play_pause->text() == tr("play")) {
        player_.play();
        ui->pushButton_play_pause->setText(tr("pause"));
    } else {
        player_.pause();
        ui->pushButton_play_pause->setText(tr("play"));
    }
}

void MainWindow::update_video_position_display(std::chrono::seconds newposition) {
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::seconds;
    auto current_position =
        duration_cast<seconds>(milliseconds(this->ui->timeEdit_current_time->time().msecsSinceStartOfDay()));
    if (newposition != current_position) {
        ui->timeEdit_current_time->setTime(
            QTime::fromMSecsSinceStartOfDay(duration_cast<milliseconds>(newposition).count()));
        qDebug() << fmt::format("@{} {}", __func__, newposition).c_str();
    }
}
void MainWindow::update_video_position_slider(std::chrono::seconds newposition) {
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::seconds;
    auto current_position = seconds(ui->horizontalSlider_current_pos->value());
    if (newposition != current_position) {
        ui->horizontalSlider_current_pos->setValue(newposition.count());
        qDebug() << fmt::format("@{} {}", __func__, newposition).c_str();
    }
}
void MainWindow::change_video_position(std::chrono::seconds newposition) {
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::seconds;
    auto current_position = duration_cast<seconds>(milliseconds(player_.position()));
    if (newposition != current_position) {
        player_.setPosition(duration_cast<milliseconds>(newposition).count());
        qDebug() << fmt::format("@{} {}", __func__, newposition).c_str();
    }
}

void MainWindow::update_start_time_display(std::chrono::seconds time) {
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::seconds;
    auto current_position =
        duration_cast<seconds>(milliseconds(this->ui->timeEdit_start_time->time().msecsSinceStartOfDay()));
    if (time != current_position) {
        ui->timeEdit_start_time->setTime(QTime::fromMSecsSinceStartOfDay(duration_cast<milliseconds>(time).count()));
        qDebug() << fmt::format("@{} {}", __func__, time).c_str();
    }
}
void MainWindow::update_start_time_slider(std::chrono::seconds time) {
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::seconds;
    auto current_position = seconds(ui->rangeSlider_selected->GetLowerValue());
    if (time != current_position) {
        ui->rangeSlider_selected->setLowerValue(time.count());
        qDebug() << fmt::format("@{} {}", __func__, time).c_str();
    }
}

void MainWindow::update_end_time_display(std::chrono::seconds time) {
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::seconds;
    auto current_position =
        duration_cast<seconds>(milliseconds(this->ui->timeEdit_end_time->time().msecsSinceStartOfDay()));
    if (time != current_position) {
        ui->timeEdit_end_time->setTime(QTime::fromMSecsSinceStartOfDay(duration_cast<milliseconds>(time).count()));
        qDebug() << fmt::format("@{} {}", __func__, time).c_str();
    }
}
void MainWindow::update_end_time_slider(std::chrono::seconds time) {
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::seconds;
    auto current_position = seconds(ui->rangeSlider_selected->GetUpperValue());
    if (time != current_position) {
        ui->rangeSlider_selected->setUpperValue(time.count());
        qDebug() << fmt::format("@{} {}", __func__, time).c_str();
    }
}

void MainWindow::open_video() {
    auto videodirs = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation);
    qDebug() << videodirs;
    qDebug() << QUrl(videodirs.isEmpty() ? QUrl() : QUrl(QDir(videodirs[0]).path()));
    auto fileurl = QFileDialog::getOpenFileUrl(this, tr("open video file"),
                                               QUrl(videodirs.isEmpty() ? QUrl() : QUrl(videodirs[0])));
    player_.setSource(fileurl);
    ui->pushButton_save->setEnabled(true);
}
void MainWindow::update_duration(int newduration) {
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::seconds;
    using namespace std::literals::chrono_literals;
    seconds length = duration_cast<seconds>(milliseconds(newduration));
    if (length != 0s) {
        ui->horizontalSlider_current_pos->setMaximum(length.count());
        ui->rangeSlider_selected->setMaximum(length.count());
        ui->rangeSlider_selected->setLowerValue(0);
        ui->rangeSlider_selected->setUpperValue(length.count());
    }
}

void MainWindow::show_videoerror(QMediaPlayer::Error, const QString &error_string) {
    QMessageBox::critical(this, tr("Error!"), error_string);
    player_.setSource(QUrl());
    ui->pushButton_save->setEnabled(false);
}

void MainWindow::save_result() {
    auto source_filename = player_.source().fileName();
    bool confirmed = false;
    QString save_filename;
    auto source_filepath = player_.source().toLocalFile();
    QString save_filepath;
    bool overwrite = false;
    do {
        save_filename = QInputDialog::getText(this, tr("savefile name"), tr("enter file name of result"),
                                              QLineEdit::Normal, source_filename, &confirmed);
        save_filepath = QUrl(player_.source().toString(QUrl::RemoveFilename) + save_filename).toLocalFile();
        if (not confirmed) {
            return;
        }
        if (save_filename.isEmpty()) {
            auto button = QMessageBox::warning(this, tr("empty filename"), tr("filename cannot be empty"),
                                               QMessageBox::Retry | QMessageBox::Abort, QMessageBox::Retry);
            if (button == QMessageBox::Abort) {
                return;
            }
        } else if (save_filename == source_filename || QFile::exists(save_filepath)) {
            auto button =
                QMessageBox::warning(this, tr("overwrite source"),
                                     tr("provided filename already exists. Are you sure you want to OVERWRITE it?"),
                                     QMessageBox::Retry | QMessageBox::Abort | QMessageBox::Yes, QMessageBox::Retry);
            switch (button) {
                case QMessageBox::Retry:
                    save_filename = "";
                    break;
                case QMessageBox::Abort:
                    return;
                case QMessageBox::Yes:
                    overwrite = true;
                    break;
                default:
                    Q_UNREACHABLE();
            }
        } else {
            break;
        }
    } while (save_filename.isEmpty());
    using std::chrono::seconds;
    seconds range_start(ui->rangeSlider_selected->GetLowerValue());
    seconds range_end(ui->rangeSlider_selected->GetUpperValue());
    QStringList arguments{"-ss",        QString::number(range_start.count()),
                          "-to",        QString::number(range_end.count()),
                          "-i",         source_filepath,
                          "-c",         "copy",
                          save_filepath};
    ProcessWidget *ffmpeg_window = new ProcessWidget(this, Qt::Window);
    ffmpeg_window->setWindowModality(Qt::ApplicationModal);
    ffmpeg_window->setAttribute(Qt::WA_DeleteOnClose, true);
    ffmpeg_window->show();
    ffmpeg_window->start("ffmpeg", arguments);
}

void MainWindow::change_curent_time_slider_tracking_state(bool state) {
    ui->horizontalSlider_current_pos->setTracking(state);
}
