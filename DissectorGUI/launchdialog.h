#ifndef LAUNCHDIALOG_H
#define LAUNCHDIALOG_H

#include <QDialog>
#include <QPlainTextEdit>

namespace Ui {
class LaunchDialog;
}

class LaunchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LaunchDialog(QWidget *parent = 0);
    ~LaunchDialog();

    void ReadSettings();
    void WriteSettings();

private slots:
    void on_BrowseDir_pressed();
    void on_BrowseExe_pressed();

public:
    Ui::LaunchDialog *ui;
    QPlainTextEdit   *mEditExe;
    QPlainTextEdit   *mEditDir;
    QPlainTextEdit   *mEditArgs;
};

#endif // LAUNCHDIALOG_H
