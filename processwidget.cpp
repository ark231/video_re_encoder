#include "processwidget.hpp"

#include <QMessageBox>
#include <QProcess>
#include <QTextStream>

#include "ui_processwidget.h"

ProcessWidget::ProcessWidget(QWidget *parent, Qt::WindowFlags flags)
    : QWidget(parent, flags), ui(new Ui::ProcessWidget) {
    ui->setupUi(this);
    ui->label_status->setText(tr("Executing nothing."));
}

ProcessWidget::~ProcessWidget() {
    delete ui;
    thread_.quit();
    thread_.wait();
}

void ProcessWidget::start(const QString &command, const QStringList &arguments) {
    process_.moveToThread(&thread_);
    connect(this, &ProcessWidget::start_process, &process_,
            qOverload<const QString &, const QStringList &, QIODeviceBase::OpenMode>(&QProcess::start));
    connect(&process_, &QProcess::readyReadStandardOutput, this, &ProcessWidget::update_stdout);
    connect(&process_, &QProcess::readyReadStandardError, this, &ProcessWidget::update_stderr);
    connect(&process_, &QProcess::started, this, &ProcessWidget::update_label_on_start);
    connect(&process_, &QProcess::errorOccurred, this, &ProcessWidget::show_error);
    connect(&process_, &QProcess::finished, this, &ProcessWidget::update_label_on_finish);
    connect(&process_, &QProcess::finished, this, &ProcessWidget::enable_closing);
    connect(this, &ProcessWidget::sigkill, &process_, &QProcess::close, Qt::BlockingQueuedConnection);
    connect(ui->pushButton_close, &QPushButton::clicked, this, &ProcessWidget::do_close);
    connect(ui->pushButton_kill, &QPushButton::clicked, this, &ProcessWidget::kill_process);
    Q_ASSERT(process_.thread() == &thread_);
    thread_.start();
    emit start_process(command, arguments, QIODeviceBase::ReadWrite);
}
void ProcessWidget::update_label_on_start() {
    ui->label_status->setText(tr("Executing %1 (pid=%2)").arg(process_.program()).arg(process_.processId()));
}
void ProcessWidget::update_label_on_finish(int exit_code, QProcess::ExitStatus exit_status) {
    switch (exit_status) {
        case QProcess::NormalExit:
            ui->label_status->setText(
                tr("Execution of %1 has finished with exit code %2.").arg(process_.program()).arg(exit_code));
            break;
        case QProcess::CrashExit:
            ui->label_status->setText(tr("Execution of %1 has crashed.").arg(process_.program()));
            break;
        default:
            Q_UNREACHABLE();
    }
}
void ProcessWidget::wait_for_finished(QDeadlineTimer deadline) { thread_.wait(deadline); }
void ProcessWidget::update_stdout() {
    process_.setReadChannel(QProcess::StandardOutput);
    while (process_.canReadLine()) {
        ui->textEdit_stdout->append(QString(process_.readLine(0xffffffff)).remove('\n'));
    }
}
void ProcessWidget::update_stderr() {
    process_.setReadChannel(QProcess::StandardError);
    while (process_.canReadLine()) {
        ui->textEdit_stderr->append(QString(process_.readLine(0xffffffff)).remove('\n'));
    }
}
void ProcessWidget::kill_process() {
    emit sigkill();  // this call blocks
}
void ProcessWidget::enable_closing() {
    ui->pushButton_close->setEnabled(true);
    ui->pushButton_kill->setEnabled(false);
}
void ProcessWidget::show_error(QProcess::ProcessError) {
    QMessageBox::critical(this, tr("error"), tr("error: %1").arg(process_.errorString()));
}
void ProcessWidget::do_close() {
    thread_.quit();
    thread_.wait();
    close();
}
