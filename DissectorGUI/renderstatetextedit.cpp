#include "renderstatetextedit.h"
#include "ui_renderstatetextedit.h"

RenderStateTextEdit::RenderStateTextEdit(QWidget *parent) :
    QDialog(parent, Qt::FramelessWindowHint),
    ui(new Ui::RenderStateTextEdit)
{
    ui->setupUi(this);
}

RenderStateTextEdit::~RenderStateTextEdit()
{
    delete ui;
}
