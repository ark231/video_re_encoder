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
#include <QFileInfo>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QMetaEnum>
#include <QPair>
#include <QPushButton>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QStringList>
#include <QTextStream>
#include <QUrl>
#include <QVBoxLayout>
#include <QVector>
#include <chrono>
#include <ciso646>
#include <functional>

#include "./ui_mainwindow.h"
#include "listdialog.hpp"
#include "processwidget.hpp"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui_(new Ui::MainWindow) {
    ui_->setupUi(this);
    ui_->pushButton_save->setEnabled(false);
    connect(ui_->actionopen, &QAction::triggered, this, &MainWindow::open_video_);
    connect(ui_->pushButton_save, &QPushButton::pressed, this, &MainWindow::save_result_);
    connect(ui_->actiondefault_extractor, &QAction::triggered, this, &MainWindow::select_default_chaptername_plugin_);
    connect(ui_->actionsavefile_name_generator, &QAction::triggered, this, &MainWindow::select_savefile_name_plugin_);
    QDir settings_dir(QApplication::applicationDirPath() + "/settings");
    if (QDir().mkpath(settings_dir.absolutePath())) {  // QDir::mkpath() returns true even when path already exists
        settings_ = new QSettings(settings_dir.filePath("settings.ini"), QSettings::IniFormat);
    }
}

MainWindow::~MainWindow() {
    delete ui_;
    if (settings_ != nullptr) {
        settings_->deleteLater();
    }
    if (tmpdir_ != nullptr) {
        delete tmpdir_;
    }
}

void MainWindow::open_video_() {
    auto videodirs = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation);
    auto filenames = QFileDialog::getOpenFileNames(this, tr("open video file"), videodirs.isEmpty() ? "" : videodirs[0],
                                                   tr("Videos (*.mp4)"));
    ui_->listWidget_filenames->addItems(filenames);
    ui_->pushButton_save->setEnabled(true);
}
namespace impl_ {
constexpr Qt::ConnectionType ONESHOT_AUTO_CONNECTION =
    static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::SingleShotConnection);
class OnTrue {
    std::function<void(void)> func_;

   public:
    OnTrue(std::function<void(void)> func) : func_(func) {}
    void operator()(bool arg) {
        if (arg) {
            this->func_();
        }
    }
};
}  // namespace impl_

void MainWindow::create_savefile_name_() {
    QString filename = QUrl::fromLocalFile(ui_->listWidget_filenames->item(current_index_)->text()).fileName();
    if (settings_->contains("savefile_name_plugin") && settings_->value("savefile_name_plugin") != NO_PLUGIN) {
        process_->start(
            PYTHON,
            {savefile_name_plugins_dir_().absoluteFilePath(settings_->value("savefile_name_plugin").toString()),
             filename},
            false);
        connect(process_, &ProcessWidget::finished, this, impl_::OnTrue([=] { this->confirm_savefile_name_(); }),
                impl_::ONESHOT_AUTO_CONNECTION);
    } else {
        confirm_savefile_name_();
    }
}
void MainWindow::confirm_savefile_name_() {
    auto source_filepath = QUrl::fromLocalFile(ui_->listWidget_filenames->item(0)->text());
    QString default_savefile_name = source_filepath.fileName();
    if (settings_->contains("savefile_name_plugin") && settings_->value("savefile_name_plugin") != NO_PLUGIN) {
        default_savefile_name = process_->get_stdout();
    }
    bool confirmed = false;
    QString save_filename;
    QUrl save_filepath;
    do {
        save_filename = QInputDialog::getText(this, tr("savefile name"), tr("enter file name of result"),
                                              QLineEdit::Normal, default_savefile_name, &confirmed);
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
    result_path_ = save_filepath;
    confirm_chaptername_plugin_();
}
void MainWindow::confirm_chaptername_plugin_() {
    QDir plugin_dir = chaptername_plugins_dir_();
    QString plugin = NO_PLUGIN;
    bool confirmed;
    if (plugin_dir.exists()) {
        plugin = QInputDialog::getItem(
            this, tr("select plugin"), tr("chapter name plugins are found. Select one you want to use."),
            chapternames_plugins_(), default_chapternames_plugin_index_(), false, &confirmed);
        if (not confirmed) {
            return;
        }
    }
    if (plugin == NO_PLUGIN) {
        chaptername_plugin_ = std::nullopt;
    } else {
        chaptername_plugin_ = chaptername_plugins_dir_().absoluteFilePath(plugin);
    }
    probe_for_duration_();
}
void MainWindow::probe_for_duration_() {
    QStringList ffprobe_arguments{"-hide_banner", "-show_entries", "format=duration", "-of",
                                  "compact" /*, "-v", "quiet"*/};
    QString filename = ui_->listWidget_filenames->item(current_index_)->text();
    process_->start("ffprobe", ffprobe_arguments + QStringList{filename}, false);
    connect(process_, &ProcessWidget::finished, this, impl_::OnTrue([=] { this->register_duration_(); }),
            impl_::ONESHOT_AUTO_CONNECTION);
}
void MainWindow::register_duration_() {
    QString filename = ui_->listWidget_filenames->item(current_index_)->text();
    QRegularExpression ffprobe_extractor(R"(format\|duration=([0-9.]+))");
    auto match = ffprobe_extractor.match(process_->get_stdout());
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
    create_chaptername_();
}
void MainWindow::create_chaptername_() {
    QString filename = QUrl::fromLocalFile(ui_->listWidget_filenames->item(current_index_)->text()).fileName();
    if (chaptername_plugin_.has_value()) {
        process_->start(PYTHON,
                        {chaptername_plugin_.value(), filename,
                         QString::number(std::get<1>(current_filename_duration_chaptername_tuple_), 'g', 10)},
                        false);
        connect(process_, &ProcessWidget::finished, this, impl_::OnTrue([=] { this->register_chaptername_(); }),
                impl_::ONESHOT_AUTO_CONNECTION);
    } else {
        std::get<2>(current_filename_duration_chaptername_tuple_) = filename;
        register_chaptername_();
    }
}
void MainWindow::register_chaptername_() {
    if (chaptername_plugin_.has_value()) {
        std::get<2>(current_filename_duration_chaptername_tuple_) = process_->get_stdout().remove('\n');
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
namespace impl_ {
int decode_ffmpeg(QStringView, QStringView new_stderr) {
    QRegularExpression time_pattern(R"(time=(?<hours>\d\d):(?<minutes>\d\d):(?<seconds>\d\d).(?<centiseconds>\d\d))");
    auto match = time_pattern.match(new_stderr);
    if (not match.hasMatch()) {
        return -1;
    }
    using std::chrono::duration_cast;
    using std::chrono::hours;
    using std::chrono::minutes;
    using std::chrono::seconds;
    using centiseconds = std::chrono::duration<int, std::centi>;
    using std::chrono::milliseconds;
    return duration_cast<milliseconds>(
               hours(match.captured("hours").toInt()) + minutes(match.captured("minutes").toInt()) +
               seconds(match.captured("seconds").toInt()) + centiseconds(match.captured("centiseconds").toInt()))
        .count();
}
}  // namespace impl_
void MainWindow::concatenate_videos_() {
    tmpdir_ = new QTemporaryDir();
    if (not tmpdir_->isValid()) {
        QMessageBox::critical(this, tr("temporary directory error"),
                              tr("failed to create temporary directory \n%1").arg(tmpdir_->errorString()));
        return;
    }
    QFile concat_file(tmpdir_->filePath("concat.txt"));
    if (not concat_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(
            this, tr("file open error"),
            tr("failed to open file [%1]. QFile::error(): %2").arg(concat_file.fileName()).arg(concat_file.error()));
        return;
    }
    using std::chrono::duration_cast;
    using milliseconds = std::chrono::duration<int, std::milli>;
    using seconds = std::chrono::duration<double>;
    using namespace std::chrono_literals;
    QTextStream concat_file_stream(&concat_file);
    total_length_ = 0ms;
    for (const auto &[filename, duration, chaptername] : filename_duration_chaptername_tuples_) {
        concat_file_stream << "file '" << filename << "'\n";
        total_length_ += duration_cast<milliseconds>(seconds(duration));
    }
    concat_file.close();
    QStringList arguments;
    tmpfile_paths_.concatenated = tmpdir_->filePath("concatenated." + QFileInfo(result_path_.toLocalFile()).suffix());
    // clang-format off
    arguments << "-f" << "concat"
              << "-safe" << "0"
              << "-i" << concat_file.fileName() 
              << "-c" << "copy" 
              << tmpfile_paths_.concatenated;
    // clang-format on
    process_->start("ffmpeg", arguments, false, {0, total_length_.count(), impl_::decode_ffmpeg});
    connect(process_, &ProcessWidget::finished, this, impl_::OnTrue([=] { this->retrieve_metadata_(); }),
            impl_::ONESHOT_AUTO_CONNECTION);
}
void MainWindow::retrieve_metadata_() {
    QStringList arguments;
    tmpfile_paths_.metadata = tmpdir_->filePath("metadata.ini");
    // clang-format off
    arguments << "-i" << tmpfile_paths_.concatenated 
              << "-f" << "ffmetadata" 
              << tmpfile_paths_.metadata;
    // clang-format on
    process_->start("ffmpeg", arguments, false);
    connect(process_, &ProcessWidget::finished, this, impl_::OnTrue([=] { this->add_chapters_(); }),
            impl_::ONESHOT_AUTO_CONNECTION);
}
void MainWindow::add_chapters_() {
    QFile metadata_file(tmpfile_paths_.metadata);
    if (not metadata_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QMessageBox::critical(this, tr("file open error"),
                              tr("failed to open file [%1]. QFile::error(): %2")
                                  .arg(metadata_file.fileName())
                                  .arg(metadata_file.error()));
        return;
    }
    QTextStream metadata_stream(&metadata_file);
    metadata_stream << Qt::endl;
    using namespace std::chrono_literals;
    using micro_seconds = std::chrono::duration<double, std::micro>;
    using std::chrono::duration_cast;
    auto start = 0.0s;
    for (const auto &[filename, raw_duration, chaptername] : filename_duration_chaptername_tuples_) {
        std::chrono::duration<double> duration(raw_duration);
        metadata_stream << "[CHAPTER]" << Qt::endl;
        metadata_stream << "TIMEBASE=1/1000000" << Qt::endl;
        metadata_stream << "START=" << static_cast<quint64>(duration_cast<micro_seconds>(start).count()) << Qt::endl;
        metadata_stream << "END=" << static_cast<quint64>(duration_cast<micro_seconds>(start + duration).count())
                        << Qt::endl;
        metadata_stream << "TITLE=" << chaptername << Qt::endl;
        start += duration;
    }
    metadata_file.close();

    QStringList arguments;
    // clang-format off
    arguments << "-i" << tmpfile_paths_.concatenated
              << "-i" << tmpfile_paths_.metadata
              << "-map_metadata" << "1"
              << "-map_chapters" << "1"
              << "-c" << "copy" 
              << result_path_.toLocalFile();
    // clang-format on

    // process_->start(
    //     "py", {"-c", "import time;[print(i,flush=True) or time.sleep(1) for i in range(120)]",
    //     tmpfile_paths_.metadata}, true);

    process_->start("ffmpeg", arguments, true, {0, total_length_.count(), impl_::decode_ffmpeg});
    connect(process_, &ProcessWidget::finished, this, impl_::OnTrue([=] { this->cleanup_after_saving_(); }),
            impl_::ONESHOT_AUTO_CONNECTION);
}
void MainWindow::cleanup_after_saving_() {
    delete tmpdir_;
    tmpdir_ = nullptr;
}
void MainWindow::start_saving_() {
    process_ = new ProcessWidget(this, Qt::Window);
    process_->setWindowModality(Qt::WindowModal);
    process_->setAttribute(Qt::WA_DeleteOnClose, true);
    process_->show();

    filename_duration_chaptername_tuples_.clear();
    current_index_ = 0;
    create_savefile_name_();
}
void MainWindow::save_result_() {
    if (ui_->listWidget_filenames->count() == 0) {
        return;
    }
    start_saving_();
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

int MainWindow::savefile_name_plugin_index_() {
    int result = 0;
    if (settings_->contains("savefile_name_plugin")) {
        int raw_idx = savefile_name_plugins_().indexOf(settings_->value("savefile_name_plugin"));
        result = qMax(raw_idx, 0);
    }

    return result;
}
QDir MainWindow::savefile_name_plugins_dir_() {
    return QDir(QApplication::applicationDirPath() + "/plugins/savefile_name");
}
QStringList MainWindow::search_savefile_name_plugins_() {
    return savefile_name_plugins_dir_().entryList({"*.py"}, QDir::Files);
}

QStringList MainWindow::savefile_name_plugins_() { return QStringList{NO_PLUGIN} + search_savefile_name_plugins_(); }

void MainWindow::select_savefile_name_plugin_() {
    QString plugin = NO_PLUGIN;
    bool confirmed;
    plugin = QInputDialog::getItem(this, tr("select plugin"), tr("Select savefile name plugin."),
                                   savefile_name_plugins_(), savefile_name_plugin_index_(), false, &confirmed);
    if (confirmed && settings_ != nullptr) {
        settings_->setValue("savefile_name_plugin", plugin);
    }
}