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

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui_(new Ui::MainWindow) {
    ui_->setupUi(this);
    ui_->pushButton_save->setEnabled(false);
    connect(ui_->actionopen, &QAction::triggered, this, &MainWindow::open_video_);
    connect(ui_->pushButton_save, &QPushButton::pressed, this, &MainWindow::save_result_);
    connect(ui_->actiondefault_extractor, &QAction::triggered, this, &MainWindow::select_default_chaptername_plugin_);
    QDir settings_dir(QApplication::applicationDirPath() + "/settings");
    if (QDir().mkpath(settings_dir.absolutePath())) {  // QDir::mkpath() returns true even when path already exists
        settings_ = new QSettings(settings_dir.filePath("settings.ini"), QSettings::IniFormat);
    }
}

MainWindow::~MainWindow() { delete ui_; }

void MainWindow::open_video_() {
    auto videodirs = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation);
    auto filenames = QFileDialog::getOpenFileNames(this, tr("open video file"), videodirs.isEmpty() ? "" : videodirs[0],
                                                   tr("Videos (*.mp4)"));
    ui_->listWidget_filenames->addItems(filenames);
    ui_->pushButton_save->setEnabled(true);
}

void MainWindow::probe_for_duration_() {
    QStringList ffprobe_arguments{"-hide_banner", "-show_entries", "format=duration", "-of", "compact", "-v", "quiet"};
    QString filename = ui_->listWidget_filenames->item(current_index_)->text();
    process_->start("ffprobe", ffprobe_arguments + QStringList{filename}, false);
    connect(process_, &ProcessWidget::finished_success, this, &MainWindow::register_duration_);
}
void MainWindow::register_duration_() {
    QString filename = ui_->listWidget_filenames->item(current_index_)->text();
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
    disconnect(process_, &ProcessWidget::finished_success, this, &MainWindow::register_duration_);
    create_chaptername_();
}
void MainWindow::create_chaptername_() {
    QString filename = QUrl::fromLocalFile(ui_->listWidget_filenames->item(current_index_)->text()).fileName();
    if (chaptername_plugin_.has_value()) {
#ifdef _WIN32
        constexpr auto PYTHON = "py";
#else
        constexpr auto PYTHON = "python";
#endif
        process_->start(PYTHON, {chaptername_plugin_.value(), filename}, false);
        connect(process_, &ProcessWidget::finished_success, this, &MainWindow::register_chaptername_);
    } else {
        std::get<2>(current_filename_duration_chaptername_tuple_) = filename;
        register_chaptername_();
    }
}
void MainWindow::register_chaptername_() {
    if (chaptername_plugin_.has_value()) {
        std::get<2>(current_filename_duration_chaptername_tuple_) = process_->get_stdout().remove('\n');
        process_->clear_stdout();
        disconnect(process_, &ProcessWidget::finished_success, this, &MainWindow::register_chaptername_);
    }
    filename_duration_chaptername_tuples_.push_back(current_filename_duration_chaptername_tuple_);
    if (current_index_ == ui_->listWidget_filenames->count() - 1) {
        confirm_chaptername_();
    } else {
        current_index_++;
        probe_for_duration_();
    }
}
void MainWindow::confirm_chaptername_() {
    bool confirmed;
    QStringList created_chapternames;
    for (const auto &[filename, duration, chaptername] : filename_duration_chaptername_tuples_) {
        created_chapternames << chaptername;
    }
    QStringList confirmed_chapternames =
        ListDialog::get_texts(nullptr, tr("confirm chapternames"),
                              tr("Chapter names of result video will be texts below.The texts are editable."),
                              created_chapternames, &confirmed);
    if (confirmed) {
        for (int i = 0; i < filename_duration_chaptername_tuples_.size(); i++) {
            std::get<2>(filename_duration_chaptername_tuples_[i]) = confirmed_chapternames[i];
        }
        concatenate_videos_();
    }
}
void MainWindow::concatenate_videos_() {
    std::string test_str = "";
    for (const auto &[filename, duration, chaptername] : filename_duration_chaptername_tuples_) {
        test_str += fmt::format("'{}' : '{}' : '{}'\n", filename.toStdString(), duration, chaptername.toStdString());
    }
    QMessageBox::information(this, tr("test"), QString::fromStdString(test_str));
}
void MainWindow::add_chapters_() {}
void MainWindow::start_saving_(QUrl result_path, QString plugin) {
    process_ = new ProcessWidget(this, Qt::Window);
    process_->setWindowModality(Qt::WindowModal);
    process_->setAttribute(Qt::WA_DeleteOnClose, true);
    process_->show();

    filename_duration_chaptername_tuples_.clear();
    current_index_ = 0;
    result_path_ = result_path;
    if (plugin == NO_PLUGIN) {
        chaptername_plugin_ = std::nullopt;
    } else {
        chaptername_plugin_ = chaptername_plugins_dir_().absoluteFilePath(plugin);
    }
    probe_for_duration_();
}
void MainWindow::save_result_() {
    if (ui_->listWidget_filenames->count() == 0) {
        return;
    }
    auto source_filepath = QUrl::fromLocalFile(ui_->listWidget_filenames->item(0)->text());
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

    QDir plugin_dir = chaptername_plugins_dir_();
    QString plugin = NO_PLUGIN;
    if (plugin_dir.exists()) {
        plugin = QInputDialog::getItem(
            this, tr("select plugin"), tr("chapter name plugins are found. Select one you want to use."),
            chapternames_plugins_(), default_chapternames_plugin_index_(), false, &confirmed);
        if (not confirmed) {
            return;
        }
    }

    start_saving_(save_filepath, plugin);
}
int MainWindow::default_chapternames_plugin_index_() {
    int result = 0;
    if (settings_->contains("default_chaptername_plugin")) {
        int raw_idx = chapternames_plugins_().indexOf(settings_->value("default_chaptername_plugin"));
        result = qMax(raw_idx, 0);
    }
    return result;
}
QDir MainWindow::chaptername_plugins_dir_() {
    return QDir(QApplication::applicationDirPath() + "/plugins/chapternames");
}
QStringList MainWindow::search_chapternames_plugins_() {
    return chaptername_plugins_dir_().entryList({"*.py"}, QDir::Files);
}
QStringList MainWindow::chapternames_plugins_() { return QStringList{NO_PLUGIN} + search_chapternames_plugins_(); }
void MainWindow::select_default_chaptername_plugin_() {
    QString plugin = NO_PLUGIN;
    bool confirmed;
    plugin = QInputDialog::getItem(this, tr("select plugin"), tr("Select default chapter name plugin."),
                                   chapternames_plugins_(), default_chapternames_plugin_index_(), false, &confirmed);
    if (confirmed && settings_ != nullptr) {
        settings_->setValue("default_chaptername_plugin", plugin);
    }
}