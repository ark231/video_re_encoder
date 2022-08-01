#include "listdialog.hpp"

#include "ui_listdialog.h"

ListDialog::ListDialog(QWidget *parent) : QDialog(parent), ui_(new Ui::ListDialog) {
    ui_->setupUi(this);
    connect(ui_->listWidget, &QListWidget::itemActivated, ui_->listWidget, &QListWidget::editItem);
}

ListDialog::~ListDialog() { delete ui_; }

QStringList ListDialog::get_texts(QWidget *parent, const QString &title, const QString &label, const QStringList &texts,
                                  bool *ok, Qt::WindowFlags flags, Qt::InputMethodHints input_method_hints) {
    ListDialog dialog(parent);
    dialog.setWindowFlags(flags);
    dialog.setInputMethodHints(input_method_hints);
    dialog.setWindowTitle(title);
    dialog.ui_->label->setText(label);
    dialog.ui_->listWidget->addItems(texts);
    for (int i = 0; i < dialog.ui_->listWidget->count(); i++) {
        dialog.ui_->listWidget->item(i)->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled);
    }
    QStringList result;
    switch (dialog.exec()) {
        case QDialog::Accepted:
            *ok = true;
            for (int i = 0; i < dialog.ui_->listWidget->count(); i++) {
                result << dialog.ui_->listWidget->item(i)->text();
            }
            break;
        case QDialog::Rejected:
            *ok = false;
            break;
    }
    return result;
}