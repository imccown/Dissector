#ifndef TEXTUREVIEWER_H
#define TEXTUREVIEWER_H

#include <QDialog>
#include <QLabel>
#include <QScrollArea>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QAction>
#include <Dissector.h>

class MainWindow;

namespace Ui {
class TextureViewerFast;
}

class TextureViewerFast : public QDialog
{
    Q_OBJECT

public:
    explicit TextureViewerFast(QWidget *parent = 0);
    ~TextureViewerFast();

    enum TextureTypes
    {
        CURRENT_RT = -1,
    };
    unsigned int GetTextureType(){ return mTextureType; }
    void SetTextureType( unsigned int iType ){ mTextureType = iType; }
    void SetMainWindow( MainWindow* iMainWindow ){ mMainWindow = iMainWindow; }
    //void SetPixmap( const QPixmap& iPixmap );
    void SetImage( const QImage& iImage );
    void SetDebuggable( bool iDebuggable ){ mDebuggable = iDebuggable; }
    bool processWheel(QWheelEvent *);

    void ScrollDelta( const QPoint& iDelta );

    void SetImageData( Dissector::PixelTypes iType, unsigned int iSizeX, unsigned int iSizeY, unsigned int iPitch, void* iData );

    void ReadSettings();
    void WriteSettings();

private slots:
    void slotContextMenu(QPoint iPoint);
    void slotContextAction(QAction* iAction);

    void on_actionToggleAlpha_triggered();

private:
    void UpdateScale();
    virtual void closeEvent ( QCloseEvent * e );
    QPoint TextureViewerFast::GetTexturePos( QPoint iPoint );
    void UpdateLocalPos( QPoint localPos );
    void SetGraphicsItem();
    void PrepareFloatImage();

private: // Data
    Ui::TextureViewerFast *ui;
    MainWindow* mMainWindow;

    QGraphicsView* mGraphicsView;
    QGraphicsScene mScene;
    QGraphicsPixmapItem mPixMapItem;
    QGraphicsPixmapItem mPixMapItemOpaque;
    QImage  mImage;

    QScrollArea* mScrollArea;
    QLabel*      mStatusBar;
    unsigned int mTextureType;
    bool         mDebuggable;

    float        mScale;

    bool         mShowingAlpha;
    QAction      mToggleAlpha;

    unsigned int            mImageSizeX;
    unsigned int            mImageSizeY;
    Dissector::PixelTypes   mPixelType;
    char*                   mImageData;

    friend class WheelEventFilter;
};

class WheelEventFilter : public QObject
{
    Q_OBJECT
public:
    WheelEventFilter(): mLeftMouseDown( false ) {}
    WheelEventFilter( TextureViewerFast* iView ){ mView = iView; }

    TextureViewerFast* mView;
protected:
    bool eventFilter(QObject *obj, QEvent *event);

protected:
    bool mLeftMouseDown;
    QPoint mDragPosition;
    friend class TextureViewerFast;
};

#endif // TEXTUREVIEWER_H
