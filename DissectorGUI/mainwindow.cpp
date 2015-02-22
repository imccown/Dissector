#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTreeView>
#include <QList>
#include <QTableView>
#include <QHostAddress>
#include <assert.h>
#include <Dissector.h>
#include <QStack>
#include <DissectorHelpers.h>
#include <QMessageBox>
#include <QTextEdit>
#include "shaderdebugwindow.h"
#include <QPalette>
#include <QInputDialog>
#include "textureviewerfast.h"
#include "meshviewer.h"
#include "launchdialog.h"
#include <QFileInfo>
#include <QProcess>
#include <Qsettings>
#include <QClipboard>
#include <QGlobal.h>
#include <QTime>
#include "shaderAnalyzer.h"

#ifdef SendMessage
#undef SendMessage
#endif

static const QBrush skHighlightBrush( QColor( 255, 43, 60, 255 ) );

enum ItemDataEnums
{
    E_ItemType = Qt::UserRole + 1,
    E_ItemEditable,
    E_ItemEdited,
};

static const unsigned int skWriteBufferSize = 1024*1024;
static const unsigned int skReadBufferSize = 16*1024*1024;

char greetMessage[] = "HELO";
char commandMessage[] = "COMD";
char byeMessage[] = "LATR";

QList<QStandardItem *> prepareRow(const QString &first,
                                  const QString &second)
{
    QList<QStandardItem *> rowItems;
    rowItems << new QStandardItem(first);
    rowItems.back()->setFlags(rowItems.back()->flags() & ~Qt::ItemIsEditable);
    rowItems << new QStandardItem(second);
    rowItems.back()->setFlags(rowItems.back()->flags() & ~Qt::ItemIsEditable);
    return rowItems;
}

bool EnterEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if( keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return )
        {
            QModelIndex idx = mWindow->mStatesview->currentIndex();
            if( idx.column() == 0 )
                idx = idx.sibling( idx.row(), idx.column() + 1 );

            mWindow->slotRenderStateDoubleClicked( idx );
            mWindow->mStatesview->edit( idx );
            return true;
        }
    }

    return QObject::eventFilter(obj, event);
}


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    QTime time = QTime::currentTime();
    qsrand((uint)time.msec());

    mConnectionEstablished = false;
    ui->setupUi(this);
    mSocket = new QTcpSocket;

    mWriteBuffer = new char[skWriteBufferSize];
    mWriteBufferSize = 0;
    mWritesPending = false;

    mReadBuffer = new char[skReadBufferSize];
    mReadBufferSize = 0;

    mCurrentMessage = NULL;

    mDrawModel = new QStandardItemModel;
    mStatesModel = new QStandardItemModel;

    mStatesModel->setColumnCount( 2 );

    mDrawview = findChild<QTreeView*>("DrawCallView");
    mStatesview = findChild<QTreeView*>("StatesView");
    mStatesview->installEventFilter( new EnterEventFilter(this) );

    mDrawview->setContextMenuPolicy( Qt::CustomContextMenu );
    mStatesview->setContextMenuPolicy( Qt::CustomContextMenu );

    connect( ui->actionConnect, SIGNAL(triggered()),
             this, SLOT(slotConnectClicked()) );
    connect( ui->actionCapture, SIGNAL(triggered()),
             this, SLOT(slotCaptureClicked()) );
    connect( ui->actionLaunch, SIGNAL(triggered()),
             this, SLOT(slotLaunchClicked()) );
    connect( ui->actionWireframe_Off, SIGNAL(triggered()),
             this, SLOT(slotWireframeOffClicked()) );

    mDrawview->setModel( mDrawModel );
    auto selModel = mDrawview->selectionModel();
    connect( selModel, SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
             this, SLOT(slotDrawViewSelectionChanged(QModelIndex,QModelIndex)) );

    connect( mStatesModel, SIGNAL(itemChanged(QStandardItem*)),
             this, SLOT(slotRenderStateModification(QStandardItem*)) );

    connect( mStatesview, SIGNAL(doubleClicked(QModelIndex)),
             this, SLOT(slotRenderStateDoubleClicked(QModelIndex)) );

    connect( mStatesview, SIGNAL(customContextMenuRequested(QPoint)),
             this, SLOT(slotContextMenuRenderState(QPoint)) );

    connect( mDrawview, SIGNAL(customContextMenuRequested(QPoint)),
             this, SLOT(slotContextMenuDraws(QPoint)) );



    mStatesview->setIconSize( QSize( 128, 128 ) );
    mStatesview->setModel( mStatesModel );

    QStringList columnNames;
    columnNames << "Name" << "Value";
    mStatesModel->setHorizontalHeaderLabels( columnNames );

    columnNames.clear();
    columnNames << "Event";
    mDrawModel->setHorizontalHeaderLabels( columnNames );

    mConnectAction = findChild<QAction*>( "actionConnect" );
    mCaptureAction = findChild<QAction*>( "actionCapture" );
    mWireFrameAction = ui->actionWireframe_Off;
    mCaptureActive = false;

    mCurrentEventNum = 0;
    mIgnoreRSEdits = false;
    mCompareWindow = NULL;

    mShowWireframe = false;
}

MainWindow::~MainWindow()
{
    if( mCurrentMessage )
    {
        delete[] mCurrentMessage;
        mCurrentMessage = NULL;
    }

    delete ui;
    delete mDrawModel;
    delete mSocket;
    delete[] mWriteBuffer;
}

void MainWindow::ReadSettings()
{
    QSettings settings;
    restoreGeometry(settings.value("mainwindow/geometry").toByteArray());
    restoreState(settings.value("mainwindow/windowState").toByteArray());

    {
        QVariantList columnList =settings.value("statesview/columns").toList();
        int column = 0;
        for( auto iter = columnList.begin(); iter != columnList.end(); ++iter, ++column )
        {
            mStatesview->setColumnWidth( column, iter->toInt() );
        }
    }

    {
        QSplitter* splitter = findChild<QSplitter*>("splitterhoriz");
        if( splitter )
        {
            splitter->restoreState( settings.value("mainwindow/splitterstate").toByteArray() );
        }
    }
}

void MainWindow::WriteSettings()
{
    QSettings settings;
    settings.setValue("mainwindow/geometry",    saveGeometry());
    settings.setValue("mainwindow/windowState", saveState());

    {
        QVariantList columnList;
        for( int ii = 0; ii < mStatesview->model()->columnCount(); ++ii )
        {
            columnList.append( mStatesview->columnWidth( ii ) );
        }
        settings.setValue("statesview/columns", columnList);
    }

    {
        QSplitter* splitter = findChild<QSplitter*>("splitterhoriz");
        if( splitter )
        {
            settings.setValue("mainwindow/splitterstate", splitter->saveState());
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    WriteSettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::CreateTextureWindow( unsigned int iType, bool iDebuggable )
{
    for( auto iter = mTextureWindows.begin(); iter != mTextureWindows.end(); ++iter )
    {
        if( (*iter)->GetTextureType() == iType )
        {
            return; // Don't open two views to the same texture.
        }
    }

   TextureViewerFast* win = new TextureViewerFast( this );
   win->SetTextureType( iType );
   mTextureWindows.push_back( win );
   win->SetMainWindow( this );
   win->SetDebuggable( iDebuggable );
   win->show();
   RequestTexture( mCurrentEventNum, NULL, iType );
}

void MainWindow::TextureWindowClosed( TextureViewerFast* iWindow )
{
    mTextureWindows.removeOne( iWindow );
}

void MainWindow::MeshWindowClosed( MeshViewer* iWindow )
{
    mMeshWindows.removeOne( iWindow );
}

void MainWindow::slotLaunchClicked()
{
    LaunchDialog ld;
    if( ld.exec() )
    {
        QSettings sets;
        QFileInfo infoExe( ld.mEditExe->toPlainText() );
        QFileInfo infoDir( ld.mEditDir->toPlainText() );

        sets.setValue( "launcher/exename", ld.mEditExe->toPlainText() );
        sets.setValue( "launcher/dirname", ld.mEditDir->toPlainText() );
        sets.setValue( "launcher/exeargs", ld.mEditArgs->toPlainText() );

        bool exeGood = infoExe.exists();
        bool dirGood = infoDir.exists() && infoDir.isDir();
        if( exeGood && dirGood )
        {
            QString cmd = QCoreApplication::applicationDirPath() +
                    QString( "/DissectorInjector.exe -e \"%1\" -d \"%2\" -a \"%3\"" ).arg(
                        ld.mEditExe->toPlainText() ).arg( ld.mEditDir->toPlainText() ).arg(
                        ld.mEditArgs->toPlainText() );
            system( cmd.toLocal8Bit() );
        }
        else
        {
            if( !exeGood )
                QMessageBox::information( this, "Launch Failure", "Invalid executable file" );
            else if( !dirGood )
                QMessageBox::information( this, "Launch Failure", "Invalid working directory" );
        }
    }
}

void MainWindow::slotConnectClicked()
{
    if( !mSocket->isOpen() )
    {
        connect( mSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
                 this, SLOT(slotConnectionStateChanged(QAbstractSocket::SocketState)) );
        connect( mSocket, SIGNAL(bytesWritten(qint64)),
                 this, SLOT(slotBytesWritten(qint64)) );
        connect( mSocket, SIGNAL(readyRead()),
                 this, SLOT(slotDataReady()) );
        mSocket->connectToHost( QHostAddress::LocalHost, 2534 );
        mConnectAction->setText( "Connecting..." );
        SetCaptureActive( false );
    }
}

void MainWindow::slotCaptureClicked()
{
    if( mConnectionEstablished )
    {
        if( mCaptureActive )
        {
            SendCommand( Dissector::CMD_RESUME, NULL, 0 );
            SetCaptureActive( false );
        }
        else
        {
            SendCommand( Dissector::CMD_TRIGGERCAPTURE, NULL, 0 );
        }
    }
}

void MainWindow::slotDrawViewSelectionChanged(const QModelIndex & current,
                                  const QModelIndex & previous)
{
    previous; // unused
    auto data = current.data( Qt::UserRole + 1 );
    int drawCallId = data.toInt();

    GotoDrawCall( drawCallId );
}

void MainWindow::RefreshGUIOptions()
{
    if( mConnectionEstablished )
    {
        Dissector::GUIOptions opts;
        opts.mShowWireFrame = mShowWireframe;
        SendCommand( Dissector::CMD_SETGUIOPTIONS, (const char*)&opts, sizeof(Dissector::GUIOptions) );
    }
}

void MainWindow::slotWireframeOffClicked()
{
    mShowWireframe = !mShowWireframe;
    if( mShowWireframe )
        mWireFrameAction->setText( "Wireframe On" );
    else
        mWireFrameAction->setText( "Wireframe Off" );

    RefreshGUIOptions();
}

void MainWindow::GotoDrawCall( int drawCallId )
{
    mCurrentEventNum = drawCallId;

    SendCommand( Dissector::CMD_GOTOCALL, (const char*)&drawCallId, sizeof(int) );
    SendCommand( Dissector::CMD_GETEVENTINFO, (const char*)&drawCallId, sizeof(int) );
    SendCommand( Dissector::CMD_CANCELOUTSTANDINGTHUMBNAILS, (const char*)&drawCallId, sizeof(int) );
}

void MainWindow::WriteSocketData()
{
    qint64 dataWritten = mSocket->write( mWriteBuffer, mWriteBufferSize );
    mWriteBufferSize -= dataWritten;
    mWritesPending = mWriteBufferSize > 0;
}

void MainWindow::slotBytesWritten( qint64 iBytesWritten )
{
    (void)iBytesWritten;
    if( mWritesPending )
    {
        WriteSocketData();
    }
}

char* FindValue( unsigned int iValue, char* iIter, char* iIterEnd )
{
    char* valueIter = (char*)&iValue;
    char* valueIterEnd = valueIter + sizeof(unsigned int);

    while( iIter != iIterEnd )
    {
        if( *iIter == *valueIter )
        {
            ++valueIter;
            if( valueIter >= valueIterEnd )
            {
                ++iIter;
                return iIter;
            }
        }
        else
        {
            valueIter = (char*)&iValue;
        }
        ++iIter;
    }

    return NULL;
}

void MainWindow::slotDataReady()
{
    int readSize;
    do
    {
        readSize = mSocket->read( mReadBuffer + mReadBufferSize,
                                  skReadBufferSize - mReadBufferSize );
        mReadBufferSize += readSize;

        while( mReadBufferSize )
        {
            if( !mCurrentMessage )
            {
                // Try to find the message header.
                char* val = FindValue( Dissector::PACKET_BEGIN, mReadBuffer, mReadBuffer+mReadBufferSize );

                if( !val )
                    break;

                // Found a value, allocate our current message.
                mCurrentMessageSize = *(unsigned int*)val;
                val += 4;
                mCurrentMessage = new char[mCurrentMessageSize];

                int byteOffset = val - mReadBuffer;
                int bytesToCopy = std::min( mCurrentMessageSize,
                                       mReadBufferSize - (byteOffset) );
                memcpy( mCurrentMessage, val, bytesToCopy );
                mCurrentMessageIter = bytesToCopy;
                if( mCurrentMessageIter == mCurrentMessageSize )
                {
                    HandleMessage();
                }

                int newSize = mReadBufferSize - ( byteOffset + bytesToCopy );
                memmove( mReadBuffer, val + bytesToCopy, newSize );
                mReadBufferSize = newSize;
            }
            else
            {
                int bytesToCopy = std::min( mCurrentMessageSize - mCurrentMessageIter,
                                       mReadBufferSize );

                memcpy( mCurrentMessage + mCurrentMessageIter, mReadBuffer, bytesToCopy );
                mCurrentMessageIter += bytesToCopy;
                if( mCurrentMessageIter >= mCurrentMessageSize )
                {
                    HandleMessage();
                }

                int newSize = mReadBufferSize - bytesToCopy;
                memmove( mReadBuffer, mReadBuffer + bytesToCopy, newSize );
                mReadBufferSize = newSize;
            }
        }
    } while( readSize != 0 );
}

void MainWindow::slotConnectionStateChanged(QAbstractSocket::SocketState iState)
{
    if( iState == QAbstractSocket::ConnectedState )
    {
        if( !mConnectionEstablished )
        {
            mConnectAction->setText( "Connected" );
            mConnectionEstablished = true;
            SendMessage( greetMessage );
            RefreshGUIOptions();

            SendCommand( Dissector::CMD_GETEVENTLIST, NULL, 0 );
            SendCommand( Dissector::CMD_GETSTATETYPES, NULL, 0 );
            SendCommand( Dissector::CMD_GETENUMTYPES, NULL, 0 );
        }
    }
    else if( iState == QAbstractSocket::UnconnectedState )
    {
        mConnectAction->setText( "Connect" );
        mConnectionEstablished = false;
        mReadBufferSize = 0;
        mThumbnailCache.clear();
        mWritesPending = false;
        mWriteBufferSize = 0;
        mCurrentMessage = 0;
        mCurrentMessageIter = 0;
        mCurrentMessageSize = 0;
        mCurrentEventInfo = 0;
        mCurrentEventInfoSize = 0;
        mCurrentEventNum = 0;
        mRenderStateTypes.clear();
        mEnumTypes.clear();
        mThumbnailCache.clear();
        mThumbnailRequests.clear();
        mTextureRequests.clear();
        //mDrawModel->clear();
        //mStatesModel->clear();
        mSocket->close();
    }
}

void MainWindow::DebugPixel( int iX, int iY )
{
    char debugData[ sizeof(unsigned int) * 5 ];
    char* dataIter = debugData;
    StoreBufferData<unsigned int>( Dissector::ST_PIXEL, dataIter );
    StoreBufferData<unsigned int>( mCurrentEventNum, dataIter );
    StoreBufferData<unsigned int>( iX, dataIter );
    StoreBufferData<unsigned int>( iY, dataIter );
    StoreBufferData<unsigned int>( 0, dataIter );

    SendCommand( Dissector::CMD_DEBUGSHADER, debugData, sizeof(debugData) );
}

void MainWindow::WhyDidntPixelRender( int iX, int iY )
{
    char debugData[ sizeof(unsigned int) + sizeof(Dissector::PixelLocation) ];
    char* dataIter = debugData;

    StoreBufferData<unsigned int>( mCurrentEventNum, dataIter );
    StoreBufferData<unsigned int>( iX, dataIter );
    StoreBufferData<unsigned int>( iY, dataIter );
    StoreBufferData<unsigned int>( 0, dataIter );

    SendCommand( Dissector::CMD_TESTPIXELFAILURE, debugData, sizeof(debugData) );
}

void MainWindow::DebugVertex( unsigned int iVertex )
{
    char debugData[ sizeof(unsigned int) * 5 ];
    char* dataIter = debugData;
    StoreBufferData<unsigned int>( Dissector::ST_VERTEX, dataIter );
    StoreBufferData<unsigned int>( mCurrentEventNum, dataIter );
    StoreBufferData<unsigned int>( iVertex, dataIter );

    SendCommand( Dissector::CMD_DEBUGSHADER, debugData, sizeof(debugData) );
}

void MainWindow::slotRenderStateModification( QStandardItem* item )
{
    if( mIgnoreRSEdits )
        return;

    //QStandardItem* item = mStatesModel->itemFromIndex(topLeft);
    unsigned int editable = item->data( E_ItemEditable ).toUInt();
    if( editable )
    {
        unsigned int type = item->data( E_ItemType ).toUInt();
        RenderStateType rstype = mRenderStateTypes[type];
        bool ok;

        unsigned int data[4];
        unsigned int dataLen = 1;
        QString str = item->text();
        char buffer[128];
        switch( rstype.mVisualizerType )
        {
        // Edit this directly in line.
        case(Dissector::RSTF_VISUALIZE_INT):
            *((int*)data) = str.toInt(&ok);
            sprintf_s(buffer, "%d", *((int*)data));
            break;
        case(Dissector::RSTF_VISUALIZE_UINT):
            *((unsigned int*)data) = str.toUInt(&ok);
            sprintf_s(buffer, "%u", *((unsigned int*)data));
            break;
        case(Dissector::RSTF_VISUALIZE_FLOAT):
            *((float*)data) = str.toFloat(&ok);
            sprintf_s(buffer, "%f", *((float*)data));
            break;
        case(Dissector::RSTF_VISUALIZE_HEX):
        {
            if( str.length() > 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X' ) )
            {
                sscanf_s(&str.toLatin1().data()[2], "%x", data );
                sprintf_s(buffer, "0x%x", *((unsigned int*)data) );
                ok = true;
            }
        }break;
        case(Dissector::RSTF_VISUALIZE_FLOAT4):
        case(Dissector::RSTF_VISUALIZE_UINT4):
        case(Dissector::RSTF_VISUALIZE_INT4):
        case(Dissector::RSTF_VISUALIZE_BOOL4):
        {
            ok = false;
            QString strTemp = str;
            QString::iterator iter = std::find( strTemp.begin(), strTemp.end(), ',' );

            dataLen = 4;
            for( int ii = 0; ii < 4; ++ii )
            {
                QString strData = strTemp.left( iter - strTemp.begin() );

                if( rstype.mVisualizerType == Dissector::RSTF_VISUALIZE_FLOAT4)
                    ((float*)data)[ii] = strData.toFloat( &ok );
                else if( rstype.mVisualizerType == Dissector::RSTF_VISUALIZE_UINT4)
                    ((unsigned int*)data)[ii] = strData.toUInt( &ok );
                else if( rstype.mVisualizerType == Dissector::RSTF_VISUALIZE_INT4)
                    ((int*)data)[ii] = strData.toInt( &ok );
                else if( rstype.mVisualizerType == Dissector::RSTF_VISUALIZE_BOOL4)
                {
                    ((int*)data)[ii] = strData.toUInt( &ok );
                    ok = ok && ( ((int*)data)[ii] == 0 || ((int*)data)[ii] == 1 );
                }

                if( !ok )
                    break;
                strTemp = strTemp.mid( 1 + (iter - strTemp.begin()) );

                iter = std::find( strTemp.begin(), strTemp.end(), ',' );
            }

            if( ok )
            {
                if( rstype.mVisualizerType == Dissector::RSTF_VISUALIZE_FLOAT4)
                    sprintf_s(buffer, "%f, %f, %f, %f", ((float*)data)[0], ((float*)data)[1],
                              ((float*)data)[2], ((float*)data)[3] );
                else if( rstype.mVisualizerType == Dissector::RSTF_VISUALIZE_UINT4)
                    sprintf_s(buffer, "%u, %u, %u, %u", ((unsigned int*)data)[0], ((unsigned int*)data)[1],
                              ((unsigned int*)data)[2], ((unsigned int*)data)[3] );
                else if( rstype.mVisualizerType == Dissector::RSTF_VISUALIZE_INT4)
                    sprintf_s(buffer, "%d, %d, %d, %d", ((int*)data)[0], ((int*)data)[1],
                              ((int*)data)[2], ((int*)data)[3] );
                else if( rstype.mVisualizerType == Dissector::RSTF_VISUALIZE_BOOL4)
                    sprintf_s(buffer, "%u, %u, %u, %u", ((unsigned int*)data)[0], ((unsigned int*)data)[1],
                              ((unsigned int*)data)[2], ((unsigned int*)data)[3] );
            }

        }break;

        default:
            ok = false;
        }

        if( ok )
        {
            char message[sizeof(int) + sizeof(Dissector::RenderState) + sizeof(unsigned int)*4];
            char* bufIter = message;
            mIgnoreRSEdits = true;
            item->setText( QString(buffer) );
            item->setForeground( skHighlightBrush );
            item->setData( 1, E_ItemEdited );
            mIgnoreRSEdits = false;

            StoreBufferData( mCurrentEventNum, bufIter );
            Dissector::RenderState rs;
            rs.mType = rstype.mType;
            rs.mFlags = Dissector::RSF_EditedByTool;
            rs.mSize = sizeof(int) * dataLen;
            StoreBufferData( rs, bufIter );
            for( unsigned int ii = 0; ii < dataLen; ++ii )
            {
                StoreBufferData< unsigned int >( data[ii], bufIter );
            }
            SendCommand( Dissector::CMD_OVERRIDERENDERSTATE, message, sizeof(message) );
        }
        else
        {
            mIgnoreRSEdits = true;
            item->setText( mSavedRSEntryData );
            mIgnoreRSEdits = false;
        }

        mIgnoreRSEdits = true;
        item->setData( 0, E_ItemEditable );
        mIgnoreRSEdits = false;
    }
}

void MainWindow::slotRenderStateDoubleClicked( const QModelIndex & index )
{
    QModelIndex idx = index;
    if( idx.column() == 0 )
    {
        idx = idx.sibling( idx.row(), 1 );
        if( !idx.isValid() )
            return;
    }

    QStandardItem* item = mStatesModel->itemFromIndex(idx);
    unsigned int type = item->data( E_ItemType ).toUInt();
    RenderStateType rstype = mRenderStateTypes[type];

    switch( rstype.mVisualizerType )
    {
    // Edit this directly in line.
    case(Dissector::RSTF_VISUALIZE_INT):
    case(Dissector::RSTF_VISUALIZE_UINT):
    case(Dissector::RSTF_VISUALIZE_FLOAT):
    case(Dissector::RSTF_VISUALIZE_HEX):
    case(Dissector::RSTF_VISUALIZE_FLOAT4):
    case(Dissector::RSTF_VISUALIZE_UINT4):
    case(Dissector::RSTF_VISUALIZE_INT4):
    case(Dissector::RSTF_VISUALIZE_BOOL4):
       mSavedRSEntryData = item->text();
       mIgnoreRSEdits = true;
       item->setData( 1, E_ItemEditable );
       mIgnoreRSEdits = false;
       break;

    // Popup a dialogue for these
    case(Dissector::RSTF_VISUALIZE_BOOL):
    {
        QMenu* menu = new QMenu(this);
        QList< QVariant > dataParams;
        dataParams.push_back( idx );
        dataParams.push_back( 1 ); // True
        QAction* action = new QAction( "True", this );
        action->setData( dataParams );
        menu->addAction( action );

        action = new QAction( "False", this );
        dataParams.removeLast();
        dataParams.push_back( 0 ); // False
        action->setData( dataParams );
        menu->addAction( action );

        connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(slotEditRenderStateEnum(QAction*)));
        menu->popup( QCursor::pos() );

        item->setEditable(false); // Prevent it from being edited normally.
    } break;

    case(Dissector::RSTF_VISUALIZE_RESOURCE_TEXTURE):
    case(Dissector::RSTF_VISUALIZE_RESOURCE_RENDERTARGET):
    case(Dissector::RSTF_VISUALIZE_RESOURCE_DEPTHTARGET):
        CreateTextureWindow( rstype.mType,
            rstype.mVisualizerType == Dissector::RSTF_VISUALIZE_RESOURCE_RENDERTARGET );
        item->setEditable(false);
        break;

    // Not supported yet
    case(Dissector::RSTF_VISUALIZE_GROUP_START):
    case(Dissector::RSTF_VISUALIZE_GROUP_END):
    case(Dissector::RSTF_VISUALIZE_RESOURCE_PIXEL_SHADER):
    case(Dissector::RSTF_VISUALIZE_RESOURCE_VERTEX_SHADER):
    case(Dissector::RSTF_VISUALIZE_RESOURCE_GEOMETRY_SHADER):
    case(Dissector::RSTF_VISUALIZE_RESOURCE_HULL_SHADER):
    case(Dissector::RSTF_VISUALIZE_RESOURCE_DOMAIN_SHADER):
    case(Dissector::RSTF_VISUALIZE_RESOURCE_COMPUTE_SHADER):
    case(Dissector::RSTF_VISUALIZE_RESOURCE_BUFFER):
    case(Dissector::RSTF_VISUALIZE_RESOURCE_VERTEXTYPE):
        item->setEditable(false);
        break;

    // Enum. Popup a dialog box
    default:
        auto enumType = mEnumTypes.find( rstype.mVisualizerType );
        if( enumType != mEnumTypes.end() )
        {
            QMenu* menu = new QMenu(this);

            for( auto iter = enumType->begin(); iter != enumType->end(); ++iter )
            {
                QAction* action = new QAction( iter.value(), this );
                QList< QVariant > dataParams;
                dataParams.push_back( idx );
                dataParams.push_back( iter.key() );
                action->setData( dataParams );
                menu->addAction( action );
            }

            connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(slotEditRenderStateEnum(QAction*)));
            menu->popup( QCursor::pos() );

            item->setEditable(false); // Prevent it from being edited normally.
        }
        break;
    }
}

void MainWindow::slotContextMenuDraws(QPoint iPoint)
{
    QModelIndex idx = mDrawview->indexAt( iPoint );
    QMenu* menu = new QMenu( this );

    {
        QList< QVariant > varlist;
        varlist.push_back( QVariant( 0 ) );
        varlist.push_back( idx );
        QAction* action = new QAction( "View Mesh", this );
        action->setData( varlist );
        menu->addAction( action );
    }

    {
        QList< QVariant > varlist;
        varlist.push_back( QVariant( 1 ) );
        varlist.push_back( idx );
        QAction* action = new QAction( "Debug Vertex...", this );
        action->setData( varlist );
        menu->addAction( action );
    }

    if( idx.isValid() )
    {
        QList< QVariant > varlist;
        varlist.push_back( QVariant( 2 ) );
        varlist.push_back( idx );

        QAction* action = new QAction( "Revert Event's Changed States", this );
        action->setData( varlist );
        menu->addAction( action );
    }

    {
        QList< QVariant > varlist;
        varlist.push_back( QVariant( 3 ) );
        QAction* action = new QAction( "Revert All Changed States", this );
        action->setData( varlist );
        menu->addAction( action );
    }

    connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(slotDrawContextAction(QAction*)));
    menu->popup( mDrawview->viewport()->mapToGlobal( iPoint ) );
}

void MainWindow::slotDrawContextAction(QAction* iAction)
{
    QList< QVariant > varlist = iAction->data().toList();
    QList< QVariant >::iterator iter = varlist.begin();
    int type = (iter++)->toInt();
    switch( type )
    {
    case( 0 ): // "View Mesh"
    {
        QModelIndex idx = (iter++)->toModelIndex();
        QStandardItem* item = mDrawModel->itemFromIndex(idx);
        int eventId = item->data().toInt();
        char message[2*sizeof(int)];
        char* bufIter = message;
        StoreBufferData< int >( eventId, bufIter );
        StoreBufferData< int >( -1, bufIter );
        SendCommand( Dissector::CMD_GETMESH, message, sizeof(message) );

        QSurfaceFormat format;
        format.setSamples(16);
        format.setDepthBufferSize(24);

        MeshViewer* mv = new MeshViewer;
        mv->setMainWindow( this );
        mv->setFormat( format );
        mv->resize( 800, 600 );
        mv->show();
        mv->ReadSettings();
        mv->SetupStateSave();
        mv->mEventID = eventId;
        mMeshWindows.push_back( mv );
    }break;
    case( 1 ): // "Debug Vertex..."
    {
        bool ok;
        int vert = QInputDialog::getInt( this, "Debug Vertex", "Vertex Number", 0, 0, INT_MAX, 1, &ok );
        if( ok )
        {
            DebugVertex( vert );
        }
    }break;

    case( 2 ): // "Revert Event's Changed States"
    {
        QModelIndex idx = (iter++)->toModelIndex();
        QStandardItem* item = mDrawModel->itemFromIndex(idx);
        int eventId = item->data().toInt();
        char message[2*sizeof(int)];
        char* bufIter = message;
        StoreBufferData< int >( eventId, bufIter );
        StoreBufferData< int >( -1, bufIter );
        SendCommand( Dissector::CMD_CLEARRENDERSTATEOVERRIDE, message, sizeof(message) );
    }break;
    case( 3 ): // "Revert All Changed States"
    {
        char message[2*sizeof(int)];
        char* bufIter = message;
        StoreBufferData< int >( -1, bufIter );
        StoreBufferData< int >( -1, bufIter );
        SendCommand( Dissector::CMD_CLEARRENDERSTATEOVERRIDE, message, sizeof(message) );
    }break;
    }
}

void MainWindow::slotContextMenuRenderState(QPoint iPoint)
{
    QModelIndex idx = mStatesview->indexAt( iPoint );
    QMenu* menu = new QMenu( this );

    if( idx.column() == 0 )
        idx = idx.sibling( idx.row(), 1 );

    {
        QList< QVariant > varlist;
        varlist.push_back( QVariant( 0 ) );
        varlist.push_back( idx );
        QAction* action = new QAction( "Edit", this );
        action->setData( varlist );
        menu->addAction( action );
    }

    if( idx.isValid() )
    {
        QStandardItem* item = mStatesModel->itemFromIndex(idx);
        int edited = item->data( E_ItemEdited ).toInt();
        if( edited )
        {
            QList< QVariant > varlist;
            varlist.push_back( QVariant( 1 ) );
            varlist.push_back( idx );

            QAction* action = new QAction( "Revert Edit", this );
            action->setData( varlist );
            menu->addAction( action );
        }
    }

    {
        QList< QVariant > varlist;
        varlist.push_back( QVariant( 2 ) );
        QAction* action = new QAction( "Revert Event's Changed States", this );
        action->setData( varlist );
        menu->addAction( action );
    }

    {
        QList< QVariant > varlist;
        varlist.push_back( QVariant( 3 ) );
        QAction* action = new QAction( "Copy Event States to Clipboard", this );
        action->setData( varlist );
        menu->addAction( action );
    }

    connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(slotRenderStateContextAction(QAction*)));
    menu->popup( mStatesview->viewport()->mapToGlobal( iPoint ) );
}

void StateRowToString( QString& oString, QStandardItem* i1, QStandardItem* i2 )
{
    oString += i1->text() + "\t" + i2->text() + "\n";
}

void MainWindow::slotRenderStateContextAction(QAction* iAction)
{
    QList< QVariant > varlist = iAction->data().toList();
    QList< QVariant >::iterator iter = varlist.begin();
    int type = (iter++)->toInt();
    switch( type )
    {
    case( 0 ): // "Edit"
    {
        QModelIndex idx = (iter++)->toModelIndex();
        slotRenderStateDoubleClicked( idx );
        mStatesview->edit( idx );
    }break;
    case( 1 ): // "Revert Edit"
    {
        QModelIndex idx = (iter++)->toModelIndex();
        QStandardItem* item = mStatesModel->itemFromIndex(idx);
        unsigned int type = item->data( E_ItemType ).toUInt();
        RenderStateType rstype = mRenderStateTypes[type];

        char message[2*sizeof(int)];
        char* bufIter = message;
        StoreBufferData< int >( mCurrentEventNum, bufIter );
        StoreBufferData< int >( rstype.mType, bufIter );
        SendCommand( Dissector::CMD_CLEARRENDERSTATEOVERRIDE, message, sizeof(message) );
    }break;
    case( 2 ): // "Revert Event's Changed States"
    {
        char message[2*sizeof(int)];
        char* bufIter = message;
        StoreBufferData< int >( mCurrentEventNum, bufIter );
        StoreBufferData< int >( -1, bufIter );
        SendCommand( Dissector::CMD_CLEARRENDERSTATEOVERRIDE, message, sizeof(message) );
    }break;
    case( 3 ): // "Copy Event States to Clipboard"
    {
        QString outdata;
        QStandardItem *root = mStatesModel->invisibleRootItem();
        int num = root->rowCount();
        for( int ii = 0; ii < num; ++ii )
        {
            QStandardItem* base = root->child( ii, 0 );
            int baseCount = base->rowCount();
            if( baseCount )
            {
                outdata += base->text() + "\n";
                for( int jj = 0; jj < baseCount; ++jj )
                {
                    QStandardItem* val0 = base->child( jj, 0 );
                    QStandardItem* val1 = base->child( jj, 1 );
                    outdata += '\t';
                    StateRowToString( outdata, val0, val1 );
                }
            }
            else
            {
                QStandardItem* val1 = root->child( ii, 1 );
                StateRowToString( outdata, base, val1 );
            }
        }

        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(outdata);
    }break;
    }
}

void MainWindow::slotEditRenderStateEnum(QAction* iAction)
{
    QVariant var = iAction->data();
    QList< QVariant > varList = var.toList();
    QList< QVariant >::iterator iter = varList.begin();
    if( iter == varList.end() )
        return;

    QModelIndex index = iter->toModelIndex(); // dataParams.push_back( index );
    ++iter;
    unsigned int value = iter->toUInt();

    QStandardItem* item = mStatesModel->itemFromIndex(index);
    unsigned int type = item->data( E_ItemType ).toUInt();
    RenderStateType rstype = mRenderStateTypes[type];

    item->setText( iAction->text() );
    item->setForeground( skHighlightBrush );
    item->setData( 1, E_ItemEdited );

    char message[sizeof(int) + sizeof(Dissector::RenderState) + sizeof(int)];
    char* bufIter = message;
    StoreBufferData( mCurrentEventNum, bufIter );
    Dissector::RenderState rs;
    rs.mType = rstype.mType;
    rs.mFlags = Dissector::RSF_EditedByTool;
    rs.mSize = sizeof(int);
    StoreBufferData( rs, bufIter );
    StoreBufferData< unsigned int >( value, bufIter );
    SendCommand( Dissector::CMD_OVERRIDERENDERSTATE, message, sizeof(message) );
}

void MainWindow::SendCommand( int iType, const char* iData, int iDataSize )
{
    assert( (mWriteBufferSize + iDataSize + 8) < skWriteBufferSize );
    *(unsigned int*)(mWriteBuffer + mWriteBufferSize) = Dissector::PACKET_BEGIN;
    mWriteBufferSize += 4;
    int comSize = strlen(commandMessage);

    int totalSize = comSize + sizeof(int) + sizeof(int) + iDataSize;
    memcpy( mWriteBuffer + mWriteBufferSize, &totalSize, sizeof(unsigned int) );
    mWriteBufferSize += sizeof(unsigned int);

    memcpy( mWriteBuffer + mWriteBufferSize, commandMessage, comSize );
    mWriteBufferSize += comSize;

    memcpy( mWriteBuffer + mWriteBufferSize, &iType, sizeof(int) );
    mWriteBufferSize += sizeof(int);

    memcpy( mWriteBuffer + mWriteBufferSize, &iDataSize, sizeof(int) );
    mWriteBufferSize += sizeof(int);

    if( iData && iDataSize )
    {
        memcpy( mWriteBuffer + mWriteBufferSize, iData, iDataSize );
        mWriteBufferSize += iDataSize;
    }

    if( mConnectionEstablished && !mWritesPending )
    {
        WriteSocketData();
    }

}

void MainWindow::SendMessage( const char* iMessage )
{
    SendMessage( iMessage, strlen(iMessage) );
}

void MainWindow::SendMessage( const char* iBuffer, unsigned int iDataSize )
{
    assert( (mWriteBufferSize + iDataSize + 8) < skWriteBufferSize );
    *(unsigned int*)(mWriteBuffer + mWriteBufferSize) = Dissector::PACKET_BEGIN;
    mWriteBufferSize += 4;
    memcpy( mWriteBuffer + mWriteBufferSize, &iDataSize, sizeof(unsigned int) );
    mWriteBufferSize += sizeof(unsigned int);
    memcpy( mWriteBuffer + mWriteBufferSize, iBuffer, iDataSize );
    mWriteBufferSize += iDataSize;

    if( mConnectionEstablished && !mWritesPending )
    {
        WriteSocketData();
    }
}

void MainWindow::RegisterRenderStateTypes()
{
    char* dataIter = (char*)mCurrentMessage;
    char* endIter = dataIter + mCurrentMessageSize;
    dataIter += sizeof(int); // Skip event id

    mRenderStateTypes.clear();
    QVariantList columnList;
    for( int ii = 0; ii < mStatesview->model()->columnCount(); ++ii )
    {
        columnList.append( mStatesview->columnWidth( ii ) );
    }

    mStatesModel->clear();
    mStatesModel->setColumnCount( 2 );

    QStringList columnNames;
    columnNames << "Name" << "Value";
    mStatesModel->setHorizontalHeaderLabels( columnNames );

    int column = 0;
    for( auto iter = columnList.begin(); iter != columnList.end(); ++iter, ++column )
    {
        mStatesview->setColumnWidth( column, iter->toInt() );
    }

    QStandardItem *root = mStatesModel->invisibleRootItem();
    QStack< QStandardItem* > groupList;

    groupList.push( root );

    while( dataIter < endIter )
    {
        RenderStateType dat;
        dat.mType = GetBufferData<unsigned int>( dataIter );
        dat.mFlags = GetBufferData<unsigned int>( dataIter );
        dat.mVisualizerType = GetBufferData<unsigned int>( dataIter );
        dat.mEditableItem = NULL;

        unsigned int len;
        len = GetBufferData<unsigned int>( dataIter );

        if( len )
        {
            dat.mName.resize( len );
            for( unsigned int jj = 0; jj < len; ++jj )
            {
                dat.mName[jj] = dataIter[jj];
            }

            dataIter += len;
        }

        if( dat.mType == -1 )
        {
            if( dat.mVisualizerType == Dissector::RSTF_VISUALIZE_GROUP_START )
            {
                QList<QStandardItem *> rowItems;
                QStandardItem* item = new QStandardItem(dat.mName);
                item->setEditable(false);
                rowItems << item;
                groupList.top()->appendRow( rowItems );
                groupList.push( rowItems.first() );
            }
            else if( dat.mVisualizerType == Dissector::RSTF_VISUALIZE_GROUP_END )
            {
                groupList.pop();
            }
        }
        else
        {
            QList<QStandardItem *> rowItems;
            QStandardItem* item = new QStandardItem(dat.mName);
            item->setEditable(false);
            item->setData( dat.mType, E_ItemType );
            rowItems << item;

            QStandardItem *editableItem = new QStandardItem("");
            dat.mEditableItem = editableItem;
            editableItem->setEditable(true);
            editableItem->setData( dat.mType, E_ItemType );
            rowItems << editableItem;
            groupList.top()->appendRow( rowItems );

            mRenderStateTypes[ dat.mType ] = dat;
        }
    }
}

void MainWindow::PopulateEventList()
{
    char* dataIter = (char*)mCurrentMessage;
    char* endIter = dataIter + mCurrentMessageSize;
    dataIter += sizeof(int); // Skip event id
    int drawCallSelection = GetBufferData<int>( dataIter );
    QModelIndex selIndex;
    bool selIndexValid = false;

    mDrawModel->clear();
    QStringList columnNames;
    columnNames << "Event";
    mDrawModel->setHorizontalHeaderLabels( columnNames );

    QStandardItem *root = mDrawModel->invisibleRootItem();
    QStack< QStandardItem* > groupList;

    groupList.push( root );

    int eventCounter = 0;
    bool active = dataIter < endIter;
    if( active && !mCaptureActive )
    {
        SetCaptureActive( true );
    }

    while( dataIter < endIter )
    {
        int eventType = GetBufferData<int>( dataIter );
        int stringLength = GetBufferData<int>( dataIter );

        char* str = NULL;
        if( stringLength )
        {
            str = dataIter;
            dataIter += stringLength;
        }

        if( eventType == -1 )
        {
            QList<QStandardItem *> rowItems;
            rowItems << new QStandardItem(str);
            rowItems.back()->setFlags(rowItems.back()->flags() & ~Qt::ItemIsEditable);
            rowItems.first()->setData( eventCounter );
            groupList.top()->appendRow( rowItems );
            groupList.push( rowItems.first() );
        }
        else if( eventType == -2 )
        {
            if( groupList.size() > 1 ) // Don't pop your top or we have nowhere to put stuff.
                groupList.pop();
        }
        else
        {
            QList<QStandardItem *> rowItems;
            rowItems << new QStandardItem( QString( "%1. %2").arg( eventCounter ).arg( str ) );
            rowItems.back()->setFlags(rowItems.back()->flags() & ~Qt::ItemIsEditable);
            rowItems.first()->setData( eventCounter );
            groupList.top()->appendRow( rowItems );
            if( eventCounter == drawCallSelection )
            {
                selIndex = rowItems.first()->index();
                selIndexValid = true;

                mDrawview->selectionModel()->select( selIndex, QItemSelectionModel::Rows );
            }
        }

        ++eventCounter;
    }

    mDrawview->expandAll();
    if( selIndexValid )
    {
        mDrawview->selectionModel()->clear();
        mDrawview->setCurrentIndex( selIndex );
        mDrawview->scrollTo(selIndex);
        GotoDrawCall( drawCallSelection );
    }
}

void MainWindow::PopulateEventInfo()
{
    char* dataIter = (char*)mCurrentMessage;
    char* endIter = dataIter + mCurrentMessageSize;
    dataIter += sizeof(int); // Event id.

    int eventNum = GetBufferData<int>( dataIter );

    if( eventNum != mCurrentEventNum )
    {
        return;
    }

    while( dataIter < endIter )
    {
        Dissector::RenderState* rs = (Dissector::RenderState*)dataIter;
        char* eventData = dataIter + sizeof(Dissector::RenderState);
        int   eventSize = rs->mSize;

        dataIter += sizeof(Dissector::RenderState);
        dataIter += eventSize;

        RenderStateType& rstype = mRenderStateTypes[ rs->mType ];
        QStandardItem* editable = rstype.mEditableItem;

        editable->setData( 0, E_ItemEditable );

        // Highlight edited items.
        if( rs->mFlags & Dissector::RSF_EditedByTool )
        {
            editable->setForeground( skHighlightBrush );
            editable->setData( 1, E_ItemEdited );

        }
        else
        {
            editable->setForeground( palette().windowText() );
            editable->setData( 0, E_ItemEdited );
        }

        QString value;
        switch( rstype.mVisualizerType )
        {
        case(Dissector::RSTF_VISUALIZE_BOOL):
            value.sprintf( "%s", *(int*)eventData ? "True" : "False" );
            editable->setText( value );
            break;
        case(Dissector::RSTF_VISUALIZE_HEX):
            value.sprintf( "0x%x", *(unsigned int*)eventData );
            editable->setText( value );
            break;
        case(Dissector::RSTF_VISUALIZE_INT):
            value.sprintf( "%d", *(int*)eventData );
            editable->setText( value );
            break;
        case(Dissector::RSTF_VISUALIZE_UINT):
            value.sprintf( "%u", *(unsigned int*)eventData );
            editable->setText( value );
            break;
        case(Dissector::RSTF_VISUALIZE_FLOAT):
            value.sprintf( "%f", *(float*)eventData );
            editable->setText( value );
            break;
        case(Dissector::RSTF_VISUALIZE_FLOAT4):
            value.sprintf( "%f, %f, %f, %f",
                           ((float*)eventData)[0],
                           ((float*)eventData)[1],
                           ((float*)eventData)[2],
                           ((float*)eventData)[3] );
            editable->setText( value );
            break;
        case(Dissector::RSTF_VISUALIZE_UINT4):
            value.sprintf( "%u, %u, %u, %u",
                           ((unsigned int*)eventData)[0],
                           ((unsigned int*)eventData)[1],
                           ((unsigned int*)eventData)[2],
                           ((unsigned int*)eventData)[3] );
            editable->setText( value );
            break;
        case(Dissector::RSTF_VISUALIZE_BOOL4):
            value.sprintf( "%u, %u, %u, %u",
                           ((bool*)eventData)[0],
                           ((bool*)eventData)[1],
                           ((bool*)eventData)[2],
                           ((bool*)eventData)[3] );
            editable->setText( value );
            break;

        case(Dissector::RSTF_VISUALIZE_GROUP_START):
        case(Dissector::RSTF_VISUALIZE_GROUP_END):
            value = "oops";
            editable->setText( value );
            break;

        case(Dissector::RSTF_VISUALIZE_RESOURCE_TEXTURE):
        case(Dissector::RSTF_VISUALIZE_RESOURCE_RENDERTARGET):
        {
            editable->setIcon( QIcon() );
            unsigned __int64 eventptr;
            if( rs->mSize == 8 ) {
                eventptr = *(unsigned __int64*)eventData;
                value.sprintf( "0x%llu", eventptr );
            } else {
                eventptr = *(unsigned int*)eventData;
                value.sprintf( "0x%x", eventptr );
            }

            RequestThumbnail( eventNum, eventptr, rstype.mVisualizerType, editable );

            editable->setText( value );
        }   break;

        case(Dissector::RSTF_VISUALIZE_RESOURCE_PIXEL_SHADER):
        case(Dissector::RSTF_VISUALIZE_RESOURCE_VERTEX_SHADER):
        case(Dissector::RSTF_VISUALIZE_RESOURCE_GEOMETRY_SHADER):
        case(Dissector::RSTF_VISUALIZE_RESOURCE_HULL_SHADER):
        case(Dissector::RSTF_VISUALIZE_RESOURCE_DOMAIN_SHADER):
        case(Dissector::RSTF_VISUALIZE_RESOURCE_COMPUTE_SHADER):
        case(Dissector::RSTF_VISUALIZE_RESOURCE_BUFFER):
        case(Dissector::RSTF_VISUALIZE_RESOURCE_VERTEXTYPE):
            if( rs->mSize == 8 )
                value.sprintf( "0x%llu", *(unsigned __int64*)eventData );
            else
                value.sprintf( "0x%x", *(unsigned int*)eventData );
            editable->setText( value );
            break;
        default:
            auto enumType = mEnumTypes.find( rstype.mVisualizerType );
            if( enumType != mEnumTypes.end() )
            {
                auto enumStringIter = enumType->find( *(int*)eventData );
                if( enumStringIter != enumType->end() )
                {
                    QString str = *enumStringIter;
                    editable->setText( str );
                }
            }
            break;
        }
    }
}

void MainWindow::PopulateEnumInfo()
{
    mEnumTypes.clear();

    char* dataIter = (char*)mCurrentMessage;
    char* endIter = dataIter + mCurrentMessageSize;
    dataIter += sizeof(int); // Event id.

    while( dataIter < endIter )
    {
        QHash< int, QString > item;

        unsigned int count = GetBufferData<unsigned int>( dataIter );
        unsigned int id = GetBufferData<unsigned int>( dataIter );

        for( unsigned int ii = 0; ii < count; ++ii )
        {
            unsigned int value = GetBufferData<unsigned int>( dataIter );
            unsigned int strsize = GetBufferData<unsigned int>( dataIter );

            QString str;
            str.resize( strsize );
            for( unsigned int jj = 0; jj < strsize; ++jj )
            {
                str[jj] = *dataIter;
                ++dataIter;
            }

            item[ value ] = str;
        }

        mEnumTypes[ id ] = item;
    }

}

void MainWindow::RequestTexture( int iEventNum, unsigned __int64 iEventData, unsigned int iType )
{
    iEventData;
    unsigned __int64 id = iType;//*(unsigned int*)&iEventData;
    if( !id )
        return;

    bool idRequestPending = false;
    for( auto listIter = mTextureRequests.begin();
         listIter != mTextureRequests.end(); ++listIter )
    {
        if( listIter->mId == id && listIter->mEventNum == iEventNum )
        {
            idRequestPending = true;
            return;
        }
    }

    if( !idRequestPending )
    {
        mTextureRequests.push_back( ThumbnailPendingRequest( id, iEventNum, NULL ) );
    }

    char data[ sizeof(__int64) + sizeof(int) + sizeof(unsigned int) ];
    char* dataIter = data;

    StoreBufferData( id, dataIter );
    StoreBufferData( iEventNum, dataIter );
    StoreBufferData( iType, dataIter );

    SendCommand( Dissector::CMD_GETIMAGE, data, sizeof(data) );
}

void MainWindow::RequestThumbnail( int iEventNum, unsigned __int64 iEventData, unsigned int iType, QStandardItem* ioDestination )
{
    unsigned __int64 id = iEventData;
    if( !id )
        return;

    for( auto iter = mThumbnailCache.begin(); iter != mThumbnailCache.end(); ++iter )
    {
        if( iter->mId == id && iter->mEventNum == iEventNum )
        {
            ioDestination->setIcon( iter->mIcon );
            return;
        }
    }

    {
        ioDestination->setIcon( QIcon() );
        bool idRequestPending = false;
        for( auto listIter = mThumbnailRequests.begin();
             listIter != mThumbnailRequests.end(); ++listIter )
        {
            if( listIter->mId == id )
            {
                idRequestPending = true;
                if( listIter->mDestination == ioDestination )
                {
                    return;
                }
            }
        }

        if( !idRequestPending )
        {
            mThumbnailRequests.push_back( ThumbnailPendingRequest( id, iEventNum, ioDestination ) );
        }

        char data[ sizeof(__int64) + sizeof(int) + sizeof(unsigned int) ];
        char* dataIter = data;

        StoreBufferData( id, dataIter );
        StoreBufferData( iEventNum, dataIter );
        StoreBufferData( iType, dataIter );

        SendCommand( Dissector::CMD_GETTHUMBNAIL, data, sizeof(data) );
    }
    ioDestination;
}

void MainWindow::AcceptImage()
{
    char* dataIter = (char*)mCurrentMessage;
    char* endIter = dataIter + mCurrentMessageSize;
    dataIter += sizeof(int); // Event id.

    __int64 id = GetBufferData<__int64>( dataIter );
    unsigned int rstype = GetBufferData<unsigned int>( dataIter );
    unsigned int sizeX = GetBufferData<unsigned int>( dataIter );
    unsigned int sizeY = GetBufferData<unsigned int>( dataIter );
    unsigned int pitch = GetBufferData<unsigned int>( dataIter );
    unsigned int pixelType = GetBufferData<unsigned int>( dataIter );

    endIter;

    for( auto iter = mTextureWindows.begin(); iter != mTextureWindows.end(); ++iter )
    {
        if( (*iter)->GetTextureType() == rstype )
        {
            (*iter)->SetImageData( Dissector::PixelTypes(pixelType), sizeX, sizeY, pitch, dataIter );
        }
    }

    for( auto listIter = mTextureRequests.begin();
         listIter != mTextureRequests.end(); )
    {
        if( listIter->mId == id )
        {
            listIter = mTextureRequests.erase( listIter );
        }
        else
        {
            ++listIter;
        }
    }
}

void MainWindow::AcceptThumbnail()
{
    char* dataIter = (char*)mCurrentMessage;
    char* endIter = dataIter + mCurrentMessageSize;
    dataIter += sizeof(int); // Event id.

    __int64 id = GetBufferData<__int64>( dataIter );
    unsigned int visualizer = GetBufferData<unsigned int>( dataIter );
    unsigned int sizeX = GetBufferData<unsigned int>( dataIter );
    unsigned int sizeY = GetBufferData<unsigned int>( dataIter );
    unsigned int pitch = GetBufferData<unsigned int>( dataIter );
    int eventNum = GetBufferData<int>( dataIter );

    visualizer;
    endIter;

    QImage image( (uchar*)dataIter, sizeX, sizeY, pitch,
                  QImage::Format_ARGB32 );

    bool found = false;
    auto iter = mThumbnailCache.begin();
    for( ; iter != mThumbnailCache.end(); ++iter )
    {
        if( iter->mId == id && iter->mEventNum == eventNum )
        {
            found = true;
            break;
        }
    }

    if( !found )
    {
        ThumbnailData newData;
        newData.mEventNum = -1;
        newData.mId = id;
        newData.mType = visualizer;
        iter = mThumbnailCache.insert( mThumbnailCache.begin(), newData );
    }

    iter->mIcon = QIcon( QPixmap::fromImage( image ) );

    for( auto listIter = mThumbnailRequests.begin();
         listIter != mThumbnailRequests.end(); )
    {
        if( listIter->mId == id && listIter->mEventNum == eventNum )
        {
            listIter->mDestination->setIcon( iter->mIcon );
            listIter = mThumbnailRequests.erase( listIter );
        }
        else
        {
            ++listIter;
        }
    }
}

void MainWindow::AcceptMesh()
{
    char* dataIter = (char*)mCurrentMessage;
    dataIter += sizeof(int); // sub size
    unsigned int eventId = GetBufferData<unsigned int>( dataIter );
    unsigned int primtype = GetBufferData<unsigned int>( dataIter );
    unsigned int numprims = GetBufferData<unsigned int>( dataIter );
    unsigned int vertSize = GetBufferData<unsigned int>( dataIter );
    unsigned int indexSize = GetBufferData<unsigned int>( dataIter );
    unsigned int numVerts = vertSize / (4*sizeof(float));
    unsigned int numIndices = indexSize / (sizeof(unsigned int));

    float* verts = (float*)dataIter;
    unsigned int* indices = (unsigned int*)(dataIter + vertSize);

    unsigned int maxi = 0;
    for( unsigned int ii = 0; ii < numIndices; ++ii )
    {
        maxi = maxi < indices[ii] ? indices[ii] : maxi;
    }

    for( auto listIter = mMeshWindows.begin();
         listIter != mMeshWindows.end(); ++listIter )
    {
        if( (*listIter)->mEventID == eventId )
        {
            (*listIter)->setMesh( primtype, numprims, verts, numVerts,
                                  indices, numIndices );
        }
    }
}

void MainWindow::HandleShaderDebug()
{
    char* dataIter = (char*)mCurrentMessage;
    dataIter += sizeof(int); // Event id.
    int eventNum      = GetBufferData<int>( dataIter );
    unsigned int loc0 = GetBufferData<unsigned int>( dataIter );
    unsigned int loc1 = GetBufferData<unsigned int>( dataIter );

    ShaderDebugWindow* window = new ShaderDebugWindow( this );
    window->ProcessInputData( this, dataIter, mCurrentMessageSize-(4*sizeof(int)) );
    window->setWindowTitle( QString( "Event %1 (%2,%3)" ).arg( eventNum ).arg( loc0 ).arg( loc1 ) );
    window->show();
}

void MainWindow::HandleShaderDebugFailed()
{
    char* dataIter = (char*)mCurrentMessage;
    dataIter += sizeof(int); // Event type
    int eventNum      = GetBufferData<int>( dataIter );
    unsigned int loc0 = GetBufferData<unsigned int>( dataIter );
    unsigned int loc1 = GetBufferData<unsigned int>( dataIter );

    QMessageBox msgbox;
    msgbox.setText( QString( "Shader debug for event %1 (%2,%3) failed.\n"
                             "Probably because the pixel wasn't rasterized." ).
                    arg( eventNum ).arg( loc0 ).arg( loc1 ) );
    msgbox.setModal( true );
    msgbox.exec();
}

void MainWindow::HandleCurrentRTUpdate()
{
    char* dataIter = (char*)mCurrentMessage;
    char* endIter = dataIter + mCurrentMessageSize;
    endIter;

    dataIter += sizeof(int); // Event id.

    unsigned int sizeX = GetBufferData<unsigned int>( dataIter );
    unsigned int sizeY = GetBufferData<unsigned int>( dataIter );
    unsigned int pitch = GetBufferData<unsigned int>( dataIter );
    unsigned int pixelType = GetBufferData<unsigned int>( dataIter );

    for( auto iter = mTextureWindows.begin(); iter != mTextureWindows.end(); ++iter )
    {
        if( (*iter)->GetTextureType() == TextureViewerFast::CURRENT_RT )
        {
            (*iter)->SetImageData( Dissector::PixelTypes(pixelType), sizeX, sizeY, pitch, dataIter );
        }
    }
}

void MainWindow::HandlePixelTestResponse()
{
    char* dataIter = (char*)mCurrentMessage;
    dataIter += sizeof(int); // Event id.

    unsigned int numItems = GetBufferData<unsigned int>( dataIter );

    QString failmessage;
    for( unsigned int ii = 0; ii < numItems; ++ii )
    {
        unsigned int fail = GetBufferData<unsigned int>( dataIter );
        if( fail & Dissector::PixelTests::RenderTargetsNotEnabled )
        {
            continue;
        }

        QString temp;
        temp.sprintf( "For Render target %d:\n", ii );
        failmessage += temp;

        if( !fail )
        {
            temp.sprintf( "Pixel was written successfully.\n", ii );
            failmessage += temp;
        }

        if( fail & Dissector::PixelTests::TriangleCulled )
        {
            temp.sprintf( "Triangle was culled (back-facing).\n", ii );
            failmessage += temp;
        }

        if( fail & Dissector::PixelTests::Rasterization )
        {
            temp.sprintf( "Pixel is not rasterized by this draw call.\n", ii );
            failmessage += temp;
            continue;
        }

        if( fail & Dissector::PixelTests::ShaderClips )
        {
            temp.sprintf( "Pixel was clipped by a shader instruction.\n", ii );
            failmessage += temp;
        }

        if( fail & Dissector::PixelTests::AlphaTest )
        {
            temp.sprintf( "Pixel failed alpha test.\n", ii );
            failmessage += temp;
        }

        if( fail & Dissector::PixelTests::ClipPlanes )
        {
            temp.sprintf( "Pixel was clipped by device clip planes.\n", ii );
            failmessage += temp;
        }

        if( fail & Dissector::PixelTests::Scissor )
        {
            temp.sprintf( "Pixel failed scissor test.\n", ii );
            failmessage += temp;
        }

        if( fail & Dissector::PixelTests::DepthTest )
        {
            temp.sprintf( "Pixel failed depth test.\n", ii );
            failmessage += temp;
        }

        if( fail & Dissector::PixelTests::Stencil )
        {
            temp.sprintf( "Pixel failed stencil test.\n", ii );
            failmessage += temp;
        }

        if( fail & Dissector::PixelTests::BlendFactor )
        {
            temp.sprintf( "Pixel blend factor is zero (alpha blend makes color invisible).\n", ii );
            failmessage += temp;
        }

        if( fail & Dissector::PixelTests::ColorMatches )
        {
            temp.sprintf( "Pixel color is deceptively close to background color, may be hard to see visually.\n", ii );
            failmessage += temp;
        }

        if( fail & Dissector::PixelTests::ColorWriteMask )
        {
            temp.sprintf( "Color write mask for render target prevents any colors being written.\n", ii );
            failmessage += temp;
        }

        if( fail & Dissector::PixelTests::RenderTargetsBound )
        {
            temp.sprintf( "No render target bound.\n", ii );
            failmessage += temp;
        }
    }

    QMessageBox msgbox;
    msgbox.setText( failmessage );
    msgbox.setModal( false );
    msgbox.exec();
}

void MainWindow::HandleClearedRenderStateOverrides()
{
    QModelIndex idx = mDrawview->selectionModel()->currentIndex();
    QStandardItem* item = mDrawModel->itemFromIndex( idx );
    int drawCallId = item->data().toInt();
    SendCommand( Dissector::CMD_GETEVENTINFO, (const char*)&drawCallId, sizeof(int) );
}

void MainWindow::SetCaptureActive( bool iEnabled )
{
    mCaptureActive = iEnabled;
    mCaptureAction->setText( iEnabled ?  "Resume" : "Capture" );
}

void MainWindow::MarkShaderForCompare( ShaderDebugWindow* iWindow )
{
    if( !iWindow )
        return;

    if( mCompareWindow )
    {
        ShaderAnalyzer::AnalyzeShaders( mCompareWindow, iWindow );
        mCompareWindow->mCompareButton->setChecked( false );
        iWindow->mCompareButton->setChecked( false );
        mCompareWindow = NULL;

        // Uncheck the buttons.
    }
    else
        mCompareWindow = iWindow;
}

void MainWindow::UnmarkShaderForCompare( ShaderDebugWindow* iWindow )
{
    if( mCompareWindow == iWindow )
        mCompareWindow = NULL;
}

void MainWindow::HandleMessage()
{
    int type = *(int*)mCurrentMessage;
    switch( type )
    {
    case( Dissector::RSP_EVENTLIST ): PopulateEventList(); break;
    case( Dissector::RSP_ALLTIMINGS ): break;
    case( Dissector::RSP_GETTIMING ): break;
    case( Dissector::RSP_STATETYPES ): RegisterRenderStateTypes(); break;
    case( Dissector::RSP_EVENTINFO ): PopulateEventInfo(); break;
    case( Dissector::RSP_ENUMTYPES ): PopulateEnumInfo(); break;
    case( Dissector::RSP_THUMBNAIL ): AcceptThumbnail(); break;
    case( Dissector::RSP_IMAGE ):     AcceptImage(); break;
    case( Dissector::RSP_SHADERDEBUGINFO ): HandleShaderDebug(); break;
    case( Dissector::RSP_SHADERDEBUGFAILED ): HandleShaderDebugFailed(); break;
    case( Dissector::RSP_CURRENTRENDERTARGET ): HandleCurrentRTUpdate(); break;
    case( Dissector::RSP_TESTPIXELFAILURE): HandlePixelTestResponse(); break;
    case( Dissector::RSP_CLEARRENDERSTATEOVERRIDE ): HandleClearedRenderStateOverrides(); break;
    case( Dissector::RSP_CAPTURECOMPLETED ):
        SetCaptureActive( true );
        SendCommand( Dissector::CMD_GETEVENTLIST, NULL, 0 );
        break;
    case( Dissector::RSP_MESH ): AcceptMesh(); break;
    }

    delete[] mCurrentMessage;
    mCurrentMessage = NULL;
}
