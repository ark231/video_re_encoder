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

void ProcessWidget::start(const QString &command, const QStringList &arguments, bool is_final) {
    if (is_first_) {
        process_.moveToThread(&thread_);
        connect(this, &ProcessWidget::start_process, &process_,
                qOverload<const QString &, const QStringList &, QIODeviceBase::OpenMode>(&QProcess::start));
        connect(&process_, &QProcess::readyReadStandardOutput, this, &ProcessWidget::update_stdout);
        connect(&process_, &QProcess::readyReadStandardError, this, &ProcessWidget::update_stderr);
        connect(&process_, &QProcess::started, this, &ProcessWidget::update_label_on_start);
        connect(&process_, &QProcess::errorOccurred, this, &ProcessWidget::show_error);
        connect(&process_, &QProcess::finished, this, &ProcessWidget::update_label_on_finish);
        connect(this, &ProcessWidget::sigkill, &process_, &QProcess::close, Qt::BlockingQueuedConnection);
        connect(ui->pushButton_close, &QPushButton::clicked, this, &ProcessWidget::do_close);
        connect(ui->pushButton_kill, &QPushButton::clicked, this, &ProcessWidget::kill_process);
    }
    if (is_final) {
        connect(&process_, &QProcess::finished, this, &ProcessWidget::enable_closing);
    }
    if (is_first_) {
        is_first_ = false;
        thread_.start();
    }
    emit start_process(command, arguments, QIODeviceBase::ReadWrite);
}
QString ProcessWidget::get_stdout() { return ui->textEdit_stdout->toPlainText(); }
QString ProcessWidget::get_stderr() { return ui->textEdit_stderr->toPlainText(); }
void ProcessWidget::clear_stdout() { ui->textEdit_stdout->clear(); }
void ProcessWidget::clear_stderr() { ui->textEdit_stderr->clear(); }
void ProcessWidget::update_label_on_start() {
    ui->label_status->setText(tr("Executing %1 (pid=%2)").arg(process_.program()).arg(process_.processId()));
}
void ProcessWidget::update_label_on_finish(int exit_code, QProcess::ExitStatus exit_status) {
    switch (exit_status) {
        case QProcess::NormalExit:
            ui->label_status->setText(
                tr("Execution of %1 has finished with exit code %2.").arg(process_.program()).arg(exit_code));
            emit finished_success();
            break;
        case QProcess::CrashExit:
            ui->label_status->setText(tr("Execution of %1 has crashed.").arg(process_.program()));
            emit finished_failure();
            break;
        default:
            Q_UNREACHABLE();
    }
}
bool ProcessWidget::wait_for_started_with_check(int timeout_msec) {
    if (not process_.waitForStarted(timeout_msec)) {
        QMessageBox::critical(this, tr("failed to start process"), tr("failed to start %1").arg(process_.program()));
        return false;
    }
    return true;
}
bool ProcessWidget::wait_for_finished_with_check(int timeout_msec) {
    if (not process_.waitForFinished(timeout_msec)) {
        QMessageBox::critical(this, tr("process failed"), tr("execution of %1 failed").arg(process_.program()));
        return false;
    }
    return true;
}
QString ProcessWidget::program() { return process_.program(); }
QStringList ProcessWidget::arguments() { return process_.arguments(); };
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
