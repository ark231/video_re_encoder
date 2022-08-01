#ifndef LISTDIALOG_H
#define LISTDIALOG_H

#include <QDialog>
#include <QStringList>

namespace Ui {
class ListDialog;
}

class ListDialog : public QDialog {
    Q_OBJECT

   public:
    explicit ListDialog(QWidget *parent = nullptr);
    ~ListDialog();

    static QStringList get_texts(QWidget *parent, const QString &title, const QString &label, const QStringList &texts,
                                 bool *ok = nullptr, Qt::WindowFlags flags = Qt::WindowFlags(),
                                 Qt::InputMethodHints input_method_hints = Qt::ImhNone);

   private:
    Ui::ListDialog *ui_;
};

#endif  // LISTDIALOG_H
