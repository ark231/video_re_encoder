#include "listdialog.hpp"

#include "ui_listdialog.h"

listDialog::listDialog(QWidget *parent) : QDialog(parent), ui(new Ui::listDialog) {
    ui->setupUi(this);
    connect(ui->listWidget, &QListWidget::itemActivated, ui->listWidget, &QListWidget::editItem);
}

listDialog::~listDialog() { delete ui; }

QStringList listDialog::get_texts(QWidget *parent, const QString &title, const QString &label, const QStringList &texts,
                                  bool *ok, Qt::WindowFlags flags, Qt::InputMethodHints inputMethodHints) {
    listDialog dialog(parent);
    dialog.setWindowFlags(flags);
    dialog.setInputMethodHints(inputMethodHints);
    dialog.setWindowTitle(title);
    dialog.ui->label->setText(label);
    dialog.ui->listWidget->addItems(texts);
    for (int i = 0; i < dialog.ui->listWidget->count(); i++) {
        dialog.ui->listWidget->item(i)->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled);
    }
    QStringList result;
    switch (dialog.exec()) {
        case QDialog::Accepted:
            *ok = true;
            for (int i = 0; i < dialog.ui->listWidget->count(); i++) {
                result << dialog.ui->listWidget->item(i)->text();
            }
            break;
        case QDialog::Rejected:
            *ok = false;
            break;
    }
    return result;
}