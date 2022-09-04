#ifndef PROCESSWIDGET_HPP
#define PROCESSWIDGET_HPP

#include <QProcess>
#include <QString>
#include <QStringLiteral>
#include <QThread>
#include <QWidget>
#include <functional>
#include <optional>

namespace Ui {
class ProcessWidget;
}

class QTextEdit;

class ProcessWidget : public QWidget {
    Q_OBJECT

   public:
    explicit ProcessWidget(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
    ~ProcessWidget();
    class ProgressParams {
        bool is_active_;
        std::function<int(QStringView, QStringView)> calc_progress_;

       public:
        int min;
        int max;
        ProgressParams(int min = 0, int max = 100,
                       std::optional<decltype(calc_progress_)> calc_progress = std::nullopt) {
            this->min = min;
            this->max = max;
            is_active_ = calc_progress.has_value();
            if (is_active_) {
                calc_progress_ = calc_progress.value();
            }
        }
        bool is_active() { return is_active_; }
        int calc_progress(QStringView stdout_text, QStringView stderr_text) {
            Q_ASSERT(this->is_active());
            return calc_progress_(stdout_text, stderr_text);
        }
    };
    /**
     * @brief start process with arguments
     *
     * @param command
     * @param arguments
     * @param is_final if this is true, close button is enabled when program finishes.
     * @param progress_params parameters for progress bar
     */
    void start(const QString &command, const QStringList &arguments, bool is_final = true,
               ProgressParams progress_params = ProgressParams());
    /**
     * @brief if QProcess::waitForStarted() returned false, show error message
     *
     * @param timeout_msec
     */
    bool wait_for_started_with_check(int timeout_msec = -1);
    /**
     * @brief if QProcess::waitForFinished() returned false, show error message
     *
     * @param timeout_msec
     */
    bool wait_for_finished_with_check(int timeout_msec = -1);
    /**
     * @brief Get the content of stdout textarea. This function does not block.
     * @warning If this function is called while process is running, returned value will be incomplete.
     *
     * @param index index of command whose command will be returned. negative value means latest command
     * @return QString content of stdout textarea
     */
    QString get_stdout(int index = -1);
    /**
     * @brief Get the content of stderr textarea. This function does not block.
     * @warning If this function is called while process is running, returned value will be incomplete.
     *
     * @param index index of command whose command will be returned. negative value means latest command
     * @return QString content of stderr textarea
     */
    QString get_stderr(int index = -1);
    void clear_stdout(int index = -1);
    void clear_stderr(int index = -1);
    QString program();
    QStringList arguments();

   signals:
    void finished(bool is_success);

   private:
    Ui::ProcessWidget *ui_;
    QThread thread_;
    QProcess *process_ = nullptr;
    int current_stdout_tab_idx_ = -1;
    int current_stderr_tab_idx_ = -1;
    ProgressParams current_progress_params_;
   signals:
    void start_process(const QString &command, const QStringList &arguments, QIODeviceBase::OpenMode);
    void sigkill();
   private slots:
    void update_label_on_start_();
    void update_label_on_finish_(int exit_code, QProcess::ExitStatus exit_status);
    void update_stdout_();
    void update_stderr_();
    void kill_process_();
    void enable_closing_();
    void do_close_();
    void show_error_(QProcess::ProcessError error);

   private:
    QTextEdit *stdout_textedit_of_(int idx);
    QTextEdit *stderr_textedit_of_(int idx);
};

#endif  // PROCESSWIDGET_HPP
