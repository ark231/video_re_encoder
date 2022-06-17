#include "mainwindow.hpp"

#include <fmt/chrono.h>
#include <fmt/core.h>

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QFontMetricsF>
#include <QMediaMetaData>
#include <QMessageBox>
#include <QStandardPaths>
#include <QUrl>
#include <chrono>

#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    player_.setVideoOutput(ui->videoWidget);
    connect(ui->pushButton_play_pause, &QPushButton::pressed, this, &MainWindow::play_pause_button_pressed);
    connect(ui->horizontalSlider_current_pos, &QSlider::valueChanged, [=](int position) {
        std::chrono::seconds new_position(position);
        this->update_video_position_slider(new_position);
        this->change_video_position(new_position);
    });
    connect(ui->timeEdit_current_time, &QTimeEdit::userTimeChanged, [=](QTime time) {
        auto new_position =
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::milliseconds(time.msecsSinceStartOfDay()));
        this->update_video_position_display(new_position);
        this->change_video_position(new_position);
    });
    connect(ui->timeEdit_start_time, &QTimeEdit::userTimeChanged, this, &MainWindow::start_time_edit_changed);
    connect(ui->timeEdit_end_time, &QTimeEdit::userTimeChanged, this, &MainWindow::end_time_edit_changed);
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

void MainWindow::start_time_edit_changed(const QTime &time) {}

void MainWindow::end_time_edit_changed(const QTime &time) {}

void MainWindow::open_video() {
    auto videodirs = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation);
    qDebug() << videodirs;
    qDebug() << QUrl(videodirs.isEmpty() ? QUrl() : QUrl(QDir(videodirs[0]).path()));
    auto fileurl = QFileDialog::getOpenFileUrl(this, tr("open video file"),
                                               QUrl(videodirs.isEmpty() ? QUrl() : QUrl(videodirs[0])));
    player_.setSource(fileurl);
}
void MainWindow::update_duration(int newduration) {
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::seconds;
    using namespace std::literals::chrono_literals;
    seconds length = duration_cast<seconds>(milliseconds(newduration));
    if (length != 0s) {
        ui->horizontalSlider_current_pos->setMaximum(length.count());
    }
}

void MainWindow::show_videoerror(QMediaPlayer::Error, const QString &error_string) {
    QMessageBox::critical(this, tr("Error!"), error_string);
    player_.setSource(QUrl());
}

void MainWindow::save_result() {}
void MainWindow::change_curent_time_slider_tracking_state(bool state) {
    ui->horizontalSlider_current_pos->setTracking(state);
}
