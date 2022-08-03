#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAudioOutput>
#include <QDir>
#include <QMainWindow>
#include <QMap>
#include <QMediaPlayer>
#include <QSettings>
#include <QTemporaryDir>
#include <QUrl>
#include <chrono>
#include <optional>
#include <tuple>

#include "processwidget.hpp"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

   private slots:

    void open_video_();

    void save_result_();

    void select_default_chaptername_plugin_();

   private:
    Ui::MainWindow *ui_;
    ProcessWidget *process_ = nullptr;  // deleted on close
    QSettings *settings_ = nullptr;
    QVector<std::tuple<QString, double, QString>> filename_duration_chaptername_tuples_;
    std::tuple<QString, double, QString> current_filename_duration_chaptername_tuple_;
    std::optional<QString> chaptername_plugin_ = std::nullopt;
    static constexpr auto NO_PLUGIN = "do not use any plugins";
    QUrl result_path_;
    struct {
        QString concatenated;
        QString metadata;
    } tmpfile_paths_;
    int current_index_ = 0;
    QTemporaryDir *tmpdir_ = nullptr;

    QDir chaptername_plugins_dir_();
    QStringList search_chapternames_plugins_();
    QStringList chapternames_plugins_();
    int default_chapternames_plugin_index_();
    void start_saving_(QUrl result_path, QString plugin);
    void probe_for_duration_();
    void register_duration_();
    void create_chaptername_();
    void register_chaptername_();
    void confirm_chaptername_();
    void chaptername_confirmed_();
    void concatenate_videos_();
    void retrieve_metadata_();
    void add_chapters_();
    void cleanup_after_saving_();
};
#endif  // MAINWINDOW_H
