#ifndef RENDERSTATEEDIT_H
#define RENDERSTATEEDIT_H

#include <QDialog>
#include <QStandardItemModel>
#include <QListView>

namespace Ui {
class RenderStateEdit;
}

class RenderStateEdit : public QDialog
{
    Q_OBJECT

public:
    explicit RenderStateEdit(QWidget *parent = 0);
    ~RenderStateEdit();

    QStandardItemModel* mListModel;
    QListView*          mListView;

    bool                mItemChosen;
    unsigned int        mRvalue;
    QString             mFinalString;

private slots:
    void on_listView_clicked(const QModelIndex &index);

    void on_listView_activated(const QModelIndex &index);

private:
    Ui::RenderStateEdit *ui;
};

#endif // RENDERSTATEEDIT_H
