#include "textureviewer.h"
#include "ui_textureviewer.h"
#include "mainwindow.h"
#include <QScrollBar>
#include <QGraphicsItem>
#include <QMenu>
#include <QFileDialog>

/*
bool WheelEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    if( event->type() == QEvent::MouseButtonPress )
    {
        QMouseEvent* ev = (QMouseEvent*)event;
        if(ev->button() == Qt::RightButton )
        {
            mView->slotContextMenu( ev->pos() );
        }
    }
    else if( event->type() == QEvent::Wheel )
    {
        if( QApplication::keyboardModifiers().testFlag( Qt::ControlModifier ) )
        {
            mView->processWheel( (QWheelEvent*)event );
            return true;
        }
    }

    return QObject::eventFilter(obj, event);
}
*/

TextureViewer::TextureViewer(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TextureViewer),
    mMainWindow( NULL ),
    mLastScale( 1.f ),
    mScale( 1.f )
{
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint |
                   Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint );
    ui->setupUi(this);
    mViewer = findChild<QLabel*>("label");
    mScrollArea = findChild<QScrollArea*>("scrollArea");
    //mScrollArea->viewport()->installEventFilter( new WheelEventFilter( this ) );

    connect( this, SIGNAL(customContextMenuRequested(QPoint)),
             this, SLOT(slotContextMenu(QPoint)) );

    connect( mViewer, SIGNAL(customContextMenuRequested(QPoint)),
             this, SLOT(slotContextMenu(QPoint)) );

    connect( mScrollArea, SIGNAL(customContextMenuRequested(QPoint)),
             this, SLOT(slotContextMenu(QPoint)) );
}

TextureViewer::~TextureViewer()
{
    if( mMainWindow )
    {
        //mMainWindow->TextureWindowClosed( this );
    }
    delete ui;
}

void adjustScrollBar(QScrollBar *scrollBar, float factor)
{
    scrollBar->setValue(int(factor * scrollBar->value()
                            + ((factor - 1) * scrollBar->pageStep()/2)));
}

void TextureViewer::UpdateScale()
{
    QSize size = mPixmap.size();

    mScrollArea->widget()->resize( size * mScale );
    mViewer->setPixmap( mPixmap.scaled( size * mScale,
                        Qt::IgnoreAspectRatio, Qt::FastTransformation ) );
    mViewer->resize( size * mScale );
    adjustScrollBar( mScrollArea->horizontalScrollBar(), (mScale / mLastScale) );
    adjustScrollBar( mScrollArea->verticalScrollBar(), (mScale / mLastScale) );
}

bool TextureViewer::processWheel( QWheelEvent * event )
{
    if( event->delta() > 0 ) {
        mLastScale = mScale;
        mScale *= 1.1f;
        UpdateScale();
    } else if( event->delta() < 0 ) {
        mLastScale = mScale;
        mScale *= .9f;
        UpdateScale();
    } else {
        return false;
    }

    return true;
}

void TextureViewer::closeEvent ( QCloseEvent * e )
{
    QDialog::closeEvent( e );
    delete this;
}

void TextureViewer::slotContextMenu(QPoint iPoint)
{
    QMenu* menu = new QMenu( this );

    if( mDebuggable )
    {
        QList< QVariant > varlist;
        varlist.push_back( QVariant( 0 ) );
        QAction* action = new QAction( "Debug Pixel", this );
        action->setData( varlist );
        menu->addAction( action );
    }
    {
        QList< QVariant > varlist;
        varlist.push_back( QVariant( 1 ) );
        QAction* action = new QAction( "Save Image", this );
        varlist.push_back( QVariant( iPoint ) );
        action->setData( varlist );
        menu->addAction( action );
    }

    connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(slotContextAction(QAction*)));
    menu->popup( mapToGlobal( iPoint ) );
}

void TextureViewer::slotContextAction(QAction* iAction)
{
    QList< QVariant > varlist = iAction->data().toList();
    QList< QVariant >::iterator iter = varlist.begin();
    int type = (iter++)->toInt();
    switch( type )
    {
    case( 0 ): // Debug pixel
    {
        //TODO: Find the actual pixel offset.
        QPoint windowPoint = iter->toPoint();
        //mMainWindow->DebugPixel( 127, 127 );
    }break;

    case( 1 ): // SaveImage
    {
        QString filename = QFileDialog::getSaveFileName( this, "Save Image", QString(), QString("*.png") );
        mPixmap.save( filename );
    }break;

    }
}
