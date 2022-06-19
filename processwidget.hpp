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
    void start(const QString &command, const QStringList &arguments);
    void wait_for_finished(QDeadlineTimer deadline = QDeadlineTimer(QDeadlineTimer::Forever));

   private:
    Ui::ProcessWidget *ui;  // NOLINT(readability-identifier-naming)
    QThread thread_;
    QProcess process_;
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
