#ifndef RENDERSTATETEXTEDIT_H
#define RENDERSTATETEXTEDIT_H

#include <QDialog>

namespace Ui {
class RenderStateTextEdit;
}

class RenderStateTextEdit : public QDialog
{
    Q_OBJECT

public:
    explicit RenderStateTextEdit(QWidget *parent = 0);
    ~RenderStateTextEdit();

private:
    Ui::RenderStateTextEdit *ui;
};

#endif // RENDERSTATETEXTEDIT_H
