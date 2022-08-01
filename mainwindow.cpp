#include "mainwindow.hpp"

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/ranges.h>

#include <QApplication>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPair>
#include <QPushButton>
#include <QStandardPaths>
#include <QStringList>
#include <QUrl>
#include <QVBoxLayout>
#include <QVector>
#include <chrono>
#include <ciso646>

#include "./ui_mainwindow.h"
#include "listdialog.hpp"
#include "processwidget.hpp"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    ui->pushButton_save->setEnabled(false);
    connect(ui->actionopen, &QAction::triggered, this, &MainWindow::open_video);
    connect(ui->pushButton_save, &QPushButton::pressed, this, &MainWindow::save_result);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::open_video() {
    auto videodirs = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation);
    auto filenames = QFileDialog::getOpenFileNames(this, tr("open video file"), videodirs.isEmpty() ? "" : videodirs[0],
                                                   tr("Videos (*.mp4)"));
    ui->listWidget_filenames->addItems(filenames);
    ui->pushButton_save->setEnabled(true);
}

void MainWindow::probe_for_duration() {
    QStringList ffprobe_arguments{"-hide_banner", "-show_entries", "format=duration", "-of", "compact", "-v", "quiet"};
    QString filename = ui->listWidget_filenames->item(current_index_)->text();
    process_->start("ffprobe", ffprobe_arguments + QStringList{filename}, false);
    connect(process_, &ProcessWidget::finished_success, this, &MainWindow::register_duration);
}
void MainWindow::register_duration() {
    QString filename = ui->listWidget_filenames->item(current_index_)->text();
    QRegularExpression ffprobe_extractor(R"(format\|duration=([0-9.]+))");
    auto match = ffprobe_extractor.match(process_->get_stdout());
    process_->clear_stdout();
    if (not match.hasMatch()) {
        QMessageBox::critical(
            this, tr("ffprobe parse error"),
            tr("failed to parse result of ffprobe\narguments:%1").arg(process_->arguments().join(" ")));
        return;
    }
    bool ok;
    double duration = match.captured(1).toDouble(&ok);
    if (not ok) {
        QMessageBox::critical(this, tr("ffprobe parse error"),
                              tr("failed to parse duration [%1]").arg(match.captured(1)));
        return;
    }
    current_filename_duration_chaptername_tuple_ = {filename, duration, ""};
    disconnect(process_, &ProcessWidget::finished_success, this, &MainWindow::register_duration);
    create_chaptername();
}
void MainWindow::create_chaptername() {
    QString filename = QUrl::fromLocalFile(ui->listWidget_filenames->item(current_index_)->text()).fileName();
    if (chaptername_plugin_.has_value()) {
        process_->start(chaptername_plugin_.value(), {filename}, false);
        connect(process_, &ProcessWidget::finished_success, this, &MainWindow::register_chaptername);
    } else {
        std::get<2>(current_filename_duration_chaptername_tuple_) = filename;
        register_chaptername();
    }
}
void MainWindow::register_chaptername() {
    if (chaptername_plugin_.has_value()) {
        std::get<2>(current_filename_duration_chaptername_tuple_) = process_->get_stdout();
        disconnect(process_, &ProcessWidget::finished_success, this, &MainWindow::register_chaptername);
    }
    filename_duration_chaptername_tuples_.push_back(current_filename_duration_chaptername_tuple_);
    if (current_index_ == ui->listWidget_filenames->count() - 1) {
        confirm_chaptername();
    } else {
        current_index_++;
        probe_for_duration();
    }
}
void MainWindow::confirm_chaptername() {
    bool confirmed;
    QStringList created_chapternames;
    for (const auto &[filename, duration, chaptername] : filename_duration_chaptername_tuples_) {
        created_chapternames << chaptername;
    }
    QStringList confirmed_chapternames =
        listDialog::get_texts(nullptr, tr("confirm chapternames"),
                              tr("Chapter names of result video will be texts below.The texts are editable."),
                              created_chapternames, &confirmed);
    if (confirmed) {
        for (int i = 0; i < filename_duration_chaptername_tuples_.size(); i++) {
            std::get<2>(filename_duration_chaptername_tuples_[i]) = confirmed_chapternames[i];
        }
        concatenate_videos();
    }
}
void MainWindow::concatenate_videos() {
    std::string test_str = "";
    for (const auto &[filename, duration, chaptername] : filename_duration_chaptername_tuples_) {
        test_str += fmt::format("{}:{}:{}\n", filename.toStdString(), duration, chaptername.toStdString());
    }
    QMessageBox::information(this, tr("test"), QString::fromStdString(test_str));
}
void MainWindow::add_chapters() {}
void MainWindow::start_saving(QUrl result_path) {
    process_ = new ProcessWidget(this, Qt::Window);
    process_->setWindowModality(Qt::WindowModal);
    process_->setAttribute(Qt::WA_DeleteOnClose, true);
    process_->show();

    filename_duration_chaptername_tuples_.clear();
    current_index_ = 0;
    result_path_ = result_path;
    probe_for_duration();
}
void MainWindow::save_result() {
    if (ui->listWidget_filenames->count() == 0) {
        return;
    }
    auto source_filepath = QUrl::fromLocalFile(ui->listWidget_filenames->item(0)->text());
    bool confirmed = false;
    QString save_filename;
    QUrl save_filepath;
    do {
        save_filename = QInputDialog::getText(this, tr("savefile name"), tr("enter file name of result"),
                                              QLineEdit::Normal, source_filepath.fileName(), &confirmed);
        save_filepath = QUrl(source_filepath.toString(QUrl::RemoveFilename) + save_filename);
        if (not confirmed) {
            return;
        }
        if (save_filename.isEmpty()) {
            auto button = QMessageBox::warning(this, tr("empty filename"), tr("filename cannot be empty"),
                                               QMessageBox::Retry | QMessageBox::Abort, QMessageBox::Retry);
            if (button == QMessageBox::Abort) {
                return;
            }
        } else if (save_filename == source_filepath.fileName() || QFile::exists(save_filepath.toLocalFile())) {
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
                    break;
                default:
                    Q_UNREACHABLE();
            }
        } else {
            break;
        }
    } while (save_filename.isEmpty());

    start_saving(save_filepath);
}