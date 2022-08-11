#include "processwidget.hpp"

#include <QMessageBox>
#include <QProcess>
#include <QTextEdit>
#include <QTextStream>

#include "ui_processwidget.h"

ProcessWidget::ProcessWidget(QWidget *parent, Qt::WindowFlags flags)
    : QWidget(parent, flags), ui_(new Ui::ProcessWidget) {
    ui_->setupUi(this);
    ui_->label_status->setText(tr("Executing nothing."));
    thread_.start();
}

ProcessWidget::~ProcessWidget() {
    delete ui_;
    thread_.quit();
    thread_.wait();
}

void ProcessWidget::start(const QString &command, const QStringList &arguments, bool is_final) {
    if (process_ != nullptr) {
        process_->deleteLater();
    }
    process_ = new QProcess;
    process_->moveToThread(&thread_);
    connect(this, &ProcessWidget::start_process, process_,
            qOverload<const QString &, const QStringList &, QIODeviceBase::OpenMode>(&QProcess::start));
    connect(process_, &QProcess::readyReadStandardOutput, this, &ProcessWidget::update_stdout_);
    connect(process_, &QProcess::readyReadStandardError, this, &ProcessWidget::update_stderr_);
    connect(process_, &QProcess::started, this, &ProcessWidget::update_label_on_start_);
    connect(process_, &QProcess::errorOccurred, this, &ProcessWidget::show_error_);
    connect(process_, &QProcess::finished, this, &ProcessWidget::update_label_on_finish_);
    connect(this, &ProcessWidget::sigkill, process_, &QProcess::close, Qt::BlockingQueuedConnection);
    connect(ui_->pushButton_close, &QPushButton::clicked, this, &ProcessWidget::do_close_);
    connect(ui_->pushButton_kill, &QPushButton::clicked, this, &ProcessWidget::kill_process_);
    if (is_final) {
        connect(process_, &QProcess::finished, this, &ProcessWidget::enable_closing_);
    }

    auto arguments_content = new QWidget;
    auto arguments_layout = new QGridLayout(arguments_content);
    auto arguments_textedit = new QTextEdit(arguments_content);
    arguments_textedit->setReadOnly(true);
    arguments_textedit->setLineWrapMode(QTextEdit::NoWrap);
    arguments_textedit->setWordWrapMode(QTextOption::NoWrap);
    QString arguments_quoted;
    QTextStream arguments_stream(&arguments_quoted);
    for (const auto &argument : arguments) {
        if (argument.contains(" ")) {
            arguments_stream << QStringLiteral(R"("%1")").arg(argument);
        } else {
            arguments_stream << argument;
        }
        arguments_stream << " ";
    }
    arguments_textedit->append(arguments_quoted);
    arguments_layout->addWidget(arguments_textedit);
    auto current_arguments_tab_idx = ui_->tab_arguments_commands->addTab(arguments_content, command);
    ui_->tab_arguments_commands->setCurrentIndex(current_arguments_tab_idx);

    auto stdout_content = new QWidget;
    auto stdout_layout = new QGridLayout(stdout_content);
    auto stdout_textedit = new QTextEdit(stdout_content);
    stdout_textedit->setReadOnly(true);
    stdout_layout->addWidget(stdout_textedit);
    current_stdout_tab_idx_ = ui_->tab_stdout_commands->addTab(stdout_content, command);
    ui_->tab_stdout_commands->setCurrentIndex(current_stdout_tab_idx_);

    auto stderr_content = new QWidget;
    auto stderr_layout = new QGridLayout(stderr_content);
    auto stderr_textedit = new QTextEdit(stdout_content);
    stderr_textedit->setReadOnly(true);
    stderr_layout->addWidget(stderr_textedit);
    current_stderr_tab_idx_ = ui_->tab_stderr_commands->addTab(stderr_content, command);
    ui_->tab_stderr_commands->setCurrentIndex(current_stderr_tab_idx_);

    ui_->label_status->setText(tr("Starting %1").arg(command));

    emit start_process(command, arguments, QIODeviceBase::ReadWrite);
}
QString ProcessWidget::get_stdout(int index) {
    return stdout_textedit_of_(index < 0 ? current_stdout_tab_idx_ : index)->toPlainText();
}
QString ProcessWidget::get_stderr(int index) {
    return stderr_textedit_of_(index < 0 ? current_stderr_tab_idx_ : index)->toPlainText();
}
void ProcessWidget::clear_stdout(int index) {
    stdout_textedit_of_(index < 0 ? current_stdout_tab_idx_ : index)->clear();
}
void ProcessWidget::clear_stderr(int index) {
    stderr_textedit_of_(index < 0 ? current_stderr_tab_idx_ : index)->clear();
}
void ProcessWidget::update_label_on_start_() {
    ui_->label_status->setText(tr("Executing %1 (pid=%2)").arg(process_->program()).arg(process_->processId()));
}
void ProcessWidget::update_label_on_finish_(int exit_code, QProcess::ExitStatus exit_status) {
    switch (exit_status) {
        case QProcess::NormalExit:
            ui_->label_status->setText(
                tr("Execution of %1 has finished with exit code %2.").arg(process_->program()).arg(exit_code));
            emit finished(true);
            break;
        case QProcess::CrashExit:
            ui_->label_status->setText(tr("Execution of %1 has crashed.").arg(process_->program()));
            emit finished(false);
            break;
        default:
            Q_UNREACHABLE();
    }
}
bool ProcessWidget::wait_for_started_with_check(int timeout_msec) {
    if (not process_->waitForStarted(timeout_msec)) {
        QMessageBox::critical(this, tr("failed to start process"), tr("failed to start %1").arg(process_->program()));
        return false;
    }

    return true;
}
bool ProcessWidget::wait_for_finished_with_check(int timeout_msec) {
    if (not process_->waitForFinished(timeout_msec)) {
        QMessageBox::critical(this, tr("process failed"), tr("execution of %1 failed").arg(process_->program()));
        return false;
    }

    return true;
}
QString ProcessWidget::program() { return process_->program(); }
QStringList ProcessWidget::arguments() { return process_->arguments(); };
void ProcessWidget::update_stdout_() {
    process_->setReadChannel(QProcess::StandardOutput);
    auto textedit = stdout_textedit_of_(current_stdout_tab_idx_);
    auto cursor = textedit->textCursor();
    textedit->moveCursor(QTextCursor::End);
    textedit->insertPlainText(QString::fromUtf8(process_->readAll()));  // NOTE: from utf8!!!
    textedit->setTextCursor(cursor);
}
void ProcessWidget::update_stderr_() {
    process_->setReadChannel(QProcess::StandardError);
    auto textedit = stderr_textedit_of_(current_stderr_tab_idx_);
    auto cursor = textedit->textCursor();
    textedit->moveCursor(QTextCursor::End);
    textedit->insertPlainText(QString::fromUtf8(process_->readAll()));  // NOTE: from utf8!!!
    textedit->setTextCursor(cursor);
}
void ProcessWidget::kill_process_() {
    emit sigkill();  // this call blocks
}
void ProcessWidget::enable_closing_() {
    ui_->pushButton_close->setEnabled(true);
    ui_->pushButton_kill->setEnabled(false);
}
void ProcessWidget::show_error_(QProcess::ProcessError) {
    QMessageBox::critical(this, tr("error"), tr("error: %1").arg(process_->errorString()));
}
void ProcessWidget::do_close_() {
    thread_.quit();
    thread_.wait();
    close();
}
QTextEdit *ProcessWidget::stdout_textedit_of_(int idx) {
    return qobject_cast<QTextEdit *>(ui_->tab_stdout_commands->widget(idx)->layout()->itemAt(0)->widget());
}
QTextEdit *ProcessWidget::stderr_textedit_of_(int idx) {
    return qobject_cast<QTextEdit *>(ui_->tab_stderr_commands->widget(idx)->layout()->itemAt(0)->widget());
}
