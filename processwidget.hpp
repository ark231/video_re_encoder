#ifndef PROCESSWIDGET_HPP
#define PROCESSWIDGET_HPP

#include <QProcess>
#include <QString>
#include <QStringLiteral>
#include <QThread>
#include <QWidget>

namespace Ui {
class ProcessWidget;
}

class ProcessWidget : public QWidget {
    Q_OBJECT

   public:
    explicit ProcessWidget(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
    ~ProcessWidget();
    /**
     * @brief start process with arguments
     *
     * @param command
     * @param arguments
     * @param is_final if this is true, close button is enabled when program finishes.
     */
    void start(const QString &command, const QStringList &arguments, bool is_final = true);
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
     * @return QString content of stdout textarea
     */
    QString get_stdout();
    /**
     * @brief Get the content of stderr textarea. This function does not block.
     * @warning If this function is called while process is running, returned value will be incomplete.
     *
     * @return QString content of stderr textarea
     */
    QString get_stderr();
    void clear_stdout();
    void clear_stderr();
    QString program();
    QStringList arguments();

   signals:
    void finished_success();
    void finished_failure();

   private:
    Ui::ProcessWidget *ui;  // NOLINT(readability-identifier-naming)
    QThread thread_;
    QProcess process_;
    bool is_first_ = true;
   signals:
    void start_process(const QString &command, const QStringList &arguments, QIODeviceBase::OpenMode);
    void sigkill();
   private slots:
    void update_label_on_start();
    void update_label_on_finish(int exit_code, QProcess::ExitStatus exit_status);
    void update_stdout();
    void update_stderr();
    void kill_process();
    void enable_closing();
    void do_close();
    void show_error(QProcess::ProcessError error);
};

#endif  // PROCESSWIDGET_HPP
