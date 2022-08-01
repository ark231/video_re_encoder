#ifndef LISTDIALOG_H
#define LISTDIALOG_H

#include <QDialog>
#include <QStringList>

namespace Ui {
class listDialog;
}

class listDialog : public QDialog {
    Q_OBJECT

   public:
    explicit listDialog(QWidget *parent = nullptr);
    ~listDialog();

    static QStringList get_texts(QWidget *parent, const QString &title, const QString &label, const QStringList &texts,
                                 bool *ok = nullptr, Qt::WindowFlags flags = Qt::WindowFlags(),
                                 Qt::InputMethodHints inputMethodHints = Qt::ImhNone);

   private:
    Ui::listDialog *ui;
};

#endif  // LISTDIALOG_H
