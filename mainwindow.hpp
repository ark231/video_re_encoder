#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAudioOutput>
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

    void open_video();

    void save_result();

   private:
    Ui::MainWindow *ui;  // NOLINT(readability-identifier-naming)
    ProcessWidget *process_ = nullptr;
    QVector<std::tuple<QString, double, QString>> filename_duration_chaptername_tuples_;
    std::tuple<QString, double, QString> current_filename_duration_chaptername_tuple_;
    std::optional<QString> chaptername_plugin_ = std::nullopt;
    static constexpr auto NO_PLUGIN = "do not use any plugins";
    QUrl result_path_;
    int current_index_ = 0;
    void start_saving(QUrl result_path, QString plugin);
    void probe_for_duration();
    void register_duration();
    void create_chaptername();
    void register_chaptername();
    void confirm_chaptername();
    void chaptername_confirmed();
    void concatenate_videos();
    void add_chapters();
};
#endif  // MAINWINDOW_H
