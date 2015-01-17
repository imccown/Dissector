#ifndef TEXTUREVIEWER_H
#define TEXTUREVIEWER_H

#include <QDialog>
#include <QLabel>
#include <QScrollArea>
class MainWindow;

namespace Ui {
class TextureViewer;
}

class TextureViewer : public QDialog
{
    Q_OBJECT

public:
    explicit TextureViewer(QWidget *parent = 0);
    ~TextureViewer();

    enum TextureTypes
    {
        CURRENT_RT = -1,
    };
    unsigned int GetTextureType(){ return mTextureType; }
    void SetTextureType( unsigned int iType ){ mTextureType = iType; }
    void SetMainWindow( MainWindow* iMainWindow ){ mMainWindow = iMainWindow; }
    void SetPixmap( const QPixmap& iPixmap ){ mPixmap = iPixmap; UpdateScale(); }
    void SetDebuggable( bool iDebuggable ){ mDebuggable = iDebuggable; }
    bool processWheel(QWheelEvent *);

private slots:
    void slotContextMenu(QPoint iPoint);
    void slotContextAction(QAction* iAction);

private:
    void UpdateScale();
    virtual void closeEvent ( QCloseEvent * e );
    Ui::TextureViewer *ui;
    MainWindow* mMainWindow;
    QLabel*     mViewer;
    QScrollArea* mScrollArea;
    unsigned int mTextureType;
    QPixmap     mPixmap;
    bool        mDebuggable;

    float        mLastScale;
    float        mScale;

    friend class WheelEventFilter;
};
/*
class WheelEventFilter : public QObject
{
    Q_OBJECT
public:
    WheelEventFilter( TextureViewer* iView ){ mView = iView; }

    TextureViewer* mView;
protected:
    bool eventFilter(QObject *obj, QEvent *event);
};
*/
#endif // TEXTUREVIEWER_H
