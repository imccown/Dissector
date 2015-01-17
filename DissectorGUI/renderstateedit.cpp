#include "renderstateedit.h"
#include "ui_renderstateedit.h"

RenderStateEdit::RenderStateEdit(QWidget *parent) :
    QDialog(parent, Qt::FramelessWindowHint),
    ui(new Ui::RenderStateEdit)
{
    ui->setupUi(this);
    mListModel = new QStandardItemModel;
    mListView = findChild<QListView*>("listView");
    mListModel->setColumnCount( 1 );
    mListView->setModel( mListModel );
    mItemChosen = false;
}

RenderStateEdit::~RenderStateEdit()
{
    delete ui;
}

void RenderStateEdit::on_listView_clicked(const QModelIndex &index)
{
    QStandardItem* item = mListModel->itemFromIndex(index);
    mItemChosen = true;
    mRvalue = item->data().toUInt();
    mFinalString = item->text();
    close();
}

void RenderStateEdit::on_listView_activated(const QModelIndex &index)
{
    QStandardItem* item = mListModel->itemFromIndex(index);
    mItemChosen = true;
    mRvalue = item->data().toUInt();
    mFinalString = item->text();
    close();
}
