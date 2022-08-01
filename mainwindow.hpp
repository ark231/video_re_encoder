#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAudioOutput>
#include <QDir>
#include <QMainWindow>
#include <QMediaPlayer>
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

   private:
    Ui::MainWindow *ui_;
    ProcessWidget *process_ = nullptr;
    QVector<std::tuple<QString, double, QString>> filename_duration_chaptername_tuples_;
    std::tuple<QString, double, QString> current_filename_duration_chaptername_tuple_;
    std::optional<QString> chaptername_plugin_ = std::nullopt;
    static constexpr auto NO_PLUGIN = "do not use any plugins";
    QUrl result_path_;
    int current_index_ = 0;

    QDir chaptername_plugins_dir_();
    QStringList search_chapternames_plugins_();
    void start_saving_(QUrl result_path, QString plugin);
    void probe_for_duration_();
    void register_duration_();
    void create_chaptername_();
    void register_chaptername_();
    void confirm_chaptername_();
    void chaptername_confirmed_();
    void concatenate_videos_();
    void add_chapters_();
};
#endif  // MAINWINDOW_H
