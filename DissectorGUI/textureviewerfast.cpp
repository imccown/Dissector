#include "textureviewerfast.h"
#include "ui_textureviewerfast.h"
#include "mainwindow.h"
#include <QGraphicsItem>
#include <QMenu>
#include <QFileDialog>
#include <QMouseEvent>
#include <QScrollBar>
#include <QSettings>
#include <QRgb>

bool WheelEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    if( event->type() == QEvent::HoverMove )
    {
        QHoverEvent* ev = (QHoverEvent*)event;
        mView->UpdateLocalPos( ev->pos() );
    }
    if( event->type() == QEvent::MouseMove )
    {
        QMouseEvent* ev = (QMouseEvent*)event;
        mView->UpdateLocalPos( ev->pos() );
        if( mLeftMouseDown )
        {
            QPoint pos = ev->globalPos();
            QPoint delta = pos - mDragPosition;
            mDragPosition = pos;

            mView->ScrollDelta( delta );
        }
    }

    if( event->type() == QEvent::MouseButtonRelease )
    {
        QMouseEvent* ev = (QMouseEvent*)event;
        if(ev->button() == Qt::LeftButton )
        {
            mLeftMouseDown = false;
        }
    }
    else if( event->type() == QEvent::MouseButtonPress )
    {
        QMouseEvent* ev = (QMouseEvent*)event;
        if(ev->button() == Qt::LeftButton )
        {
            mLeftMouseDown = true;
            mDragPosition = ev->globalPos();
        }
        else if(ev->button() == Qt::RightButton )
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

TextureViewerFast::TextureViewerFast(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TextureViewerFast),
    mMainWindow( NULL ),
    mScale( 1.f ),
    mToggleAlpha( "Toggle Alpha", this )
{
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint |
                   Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint );
    ui->setupUi(this);

    connect( this, SIGNAL(customContextMenuRequested(QPoint)),
             this, SLOT(slotContextMenu(QPoint)) );

    mPixMapItem.setTransformationMode( Qt::FastTransformation );
    mPixMapItemOpaque.setTransformationMode( Qt::FastTransformation );

    mGraphicsView = findChild<QGraphicsView*>("graphicsView");
    mGraphicsView->setScene( &mScene );
    mGraphicsView->viewport()->setMouseTracking( true );
    WheelEventFilter* evFilter = new WheelEventFilter( this );
    evFilter->mLeftMouseDown = false;
    mGraphicsView->viewport()->installEventFilter( evFilter );

    mStatusBar = findChild<QLabel*>( "labelPixel" );

    mShowingAlpha = true;
    SetGraphicsItem();

    ReadSettings();

    mToggleAlpha.setShortcut( QKeySequence( Qt::Key_A ) );
    mGraphicsView->viewport()->addAction( &mToggleAlpha );

    connect( &mToggleAlpha, SIGNAL(triggered()), this, SLOT(on_actionToggleAlpha_triggered()) );
}

TextureViewerFast::~TextureViewerFast()
{
    if( mMainWindow )
    {
        mMainWindow->TextureWindowClosed( this );
    }
    delete ui;
}

void TextureViewerFast::SetGraphicsItem()
{
    mScene.removeItem( &mPixMapItem );
    mScene.removeItem( &mPixMapItemOpaque );
    mScene.addItem( mShowingAlpha ? &mPixMapItem : &mPixMapItemOpaque );
}

void TextureViewerFast::UpdateScale()
{
    QTransform mat;
    mat.scale( mScale, mScale );
    mGraphicsView->setTransform( mat );
}

void TextureViewerFast::ScrollDelta( const QPoint& iDelta )
{
    int horiz = mGraphicsView->horizontalScrollBar()->value() - iDelta.x();
    int vert = mGraphicsView->verticalScrollBar()->value() - iDelta.y();
    mGraphicsView->horizontalScrollBar()->setValue( horiz );
    mGraphicsView->verticalScrollBar()->setValue( vert );
}

void TextureViewerFast::SetPixmap( const QPixmap& iPixmap )
{
    mPixMapItem.setPixmap(iPixmap);
    mImage = iPixmap.toImage();
    QImage imageOpaque = mImage;
    for( int xx = 0, count =  imageOpaque.width(); xx < count; ++xx )
    {
        for( int yy = 0, count =  imageOpaque.height(); yy < count; ++yy )
        {
            QRgb rgb = mImage.pixel( xx, yy );
            imageOpaque.setPixel( xx, yy, qRgba( qRed( rgb ), qGreen( rgb ), qBlue( rgb ), 0xFF ) );
        }
    }

    mPixMapItemOpaque.setPixmap( QPixmap::fromImage( imageOpaque ) );


    UpdateScale();
}

bool TextureViewerFast::processWheel( QWheelEvent * event )
{
    if( event->delta() > 0 ) {
        mScale *= 1.1f;
        UpdateScale();
    } else if( event->delta() < 0 ) {
        mScale *= .9f;
        UpdateScale();
    } else {
        return false;
    }

    return true;
}

void TextureViewerFast::closeEvent ( QCloseEvent * e )
{
    WriteSettings();
    QDialog::closeEvent( e );
    deleteLater();
}

QPoint TextureViewerFast::GetTexturePos( QPoint iPoint )
{
    float x = mGraphicsView->horizontalScrollBar()->value() + iPoint.x();
    float y = mGraphicsView->verticalScrollBar()->value() + iPoint.y();

    x = floorf( x / mScale );
    y = floorf( y / mScale );
    QPoint rvalue( x, y );

    return rvalue;
}

void TextureViewerFast::slotContextMenu(QPoint iPoint)
{
    QMenu* menu = new QMenu( this );

    if( mDebuggable )
    {
        {
            QPoint pos = GetTexturePos( iPoint );
            QList< QVariant > varlist;
            varlist.push_back( QVariant( 0 ) );
            QString str = QString( "Debug Pixel (%1, %2)" ).arg( pos.x() ).arg( pos.y() );
            QAction* action = new QAction( str, this );
            varlist.push_back( QVariant( QPoint( pos.x(), pos.y() ) ) );
            action->setData( varlist );
            menu->addAction( action );
        }

        {
            QPoint pos = GetTexturePos( iPoint );
            QList< QVariant > varlist;
            varlist.push_back( QVariant( 1 ) );
            QString str = QString( "Why Didn't Pixel Render (%1, %2)" ).arg( pos.x() ).arg( pos.y() );
            QAction* action = new QAction( str, this );
            varlist.push_back( QVariant( QPoint( pos.x(), pos.y() ) ) );
            action->setData( varlist );
            menu->addAction( action );
        }
    }

    {
        QList< QVariant > varlist;
        varlist.push_back( QVariant( 2 ) );
        QAction* action = new QAction( "Save Image", this );
        varlist.push_back( QVariant( iPoint ) );
        action->setData( varlist );
        menu->addAction( action );
    }

    connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(slotContextAction(QAction*)));
    menu->popup( mapToGlobal( iPoint ) );
}

void TextureViewerFast::slotContextAction(QAction* iAction)
{
    QList< QVariant > varlist = iAction->data().toList();
    QList< QVariant >::iterator iter = varlist.begin();
    int type = (iter++)->toInt();
    switch( type )
    {
    case( 0 ): // Debug pixel
    {
        QPoint windowPoint = iter->toPoint();
        QPoint point = (iter++)->toPoint();
        mMainWindow->DebugPixel( point.x(), point.y() );
    }break;

    case( 1 ): // Why Didn't Pixel Render
    {
        QPoint windowPoint = iter->toPoint();
        QPoint point = (iter++)->toPoint();
        mMainWindow->WhyDidntPixelRender( point.x(), point.y() );
    }break;

    case( 2 ): // SaveImage
    {
        QString filename = QFileDialog::getSaveFileName( this, "Save Image", QString(), QString("*.png") );
        mPixMapItem.pixmap().save( filename );
    }break;

    }
}

void TextureViewerFast::UpdateLocalPos( QPoint localPos )
{
    QPoint pos = GetTexturePos( localPos );
    QRgb col = mImage.pixel( pos.x(), pos.y() );

    mStatusBar->setText( QString("(%1, %2) -> (%3, %4, %5, %6)").arg( pos.x() ).arg( pos.y() ).
                         arg( qRed(col) ).arg( qGreen(col) ).arg( qBlue(col) ).arg( qAlpha(col) ) );
}

void TextureViewerFast::ReadSettings()
{
    QSettings settings;
    restoreGeometry(settings.value("textureviewer/geometry").toByteArray());
    if( settings.value("textureviewer/maximized", false).toBool() )
    {
       move( settings.value("textureviewer/topleft").toPoint() );
    }
}

void TextureViewerFast::WriteSettings()
{
    QSettings settings;
    settings.setValue("textureviewer/geometry",    saveGeometry());
    settings.setValue("textureviewer/maximized",   isMaximized());
    settings.setValue("textureviewer/topleft",     parentWidget()->mapToGlobal( geometry().topLeft() ) );
}

void TextureViewerFast::on_actionToggleAlpha_triggered()
{
    mShowingAlpha = !mShowingAlpha;
    SetGraphicsItem();
}
