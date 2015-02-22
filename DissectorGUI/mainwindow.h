#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QtNetwork/QTcpSocket>
#include <QHash>
#include <QLabel>
#include <QTreeView>
#include <QKeyEvent>
class TextureViewerFast;
class MeshViewer;
class ShaderDebugWindow;

struct RenderStateType
{
    unsigned int   mType;
    unsigned int   mFlags;
    unsigned int   mVisualizerType;
    unsigned int   mDataSize;
    QString        mName;
    QStandardItem* mEditableItem;
};

struct RenderGroupData
{
    QString         mName;
    bool            mIsStacked;
};

struct ThumbnailData
{
    QIcon           mIcon;
    __int64         mId;
    int             mEventNum;
    unsigned int    mType;
};

struct ThumbnailPendingRequest
{
    ThumbnailPendingRequest(){}
    ThumbnailPendingRequest( unsigned __int64 iId, int iEvent, QStandardItem* iDestination ):
        mId( iId ),
        mEventNum( iEvent ),
        mDestination( iDestination ) {}

    unsigned __int64 mId;
    int             mEventNum;
    QStandardItem*  mDestination;
};

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

       void ReadSettings();
       void WriteSettings();

       void MarkShaderForCompare( ShaderDebugWindow* iWindow );
       void UnmarkShaderForCompare( ShaderDebugWindow* iWindow );

private slots:
       void slotLaunchClicked();
       void slotConnectClicked();
       void slotCaptureClicked();
       void slotWireframeOffClicked();

       void slotConnectionStateChanged( QAbstractSocket::SocketState iState );
       void slotBytesWritten( qint64 iBytesWritten );
       void slotDataReady();

       void slotDrawViewSelectionChanged(const QModelIndex & current,
                                         const QModelIndex & previous);

       void slotRenderStateModification( QStandardItem* item );

       void slotRenderStateDoubleClicked( const QModelIndex & index );

       void slotContextMenuDraws(QPoint iPoint);
       void slotContextMenuRenderState(QPoint iPoint);

       void slotEditRenderStateEnum(QAction*);

       void slotDrawContextAction(QAction* iAction);
       void slotRenderStateContextAction(QAction* iAction);


private:
       void SendCommand( int iType, const char* iData, int iDataSize );
       void SendMessage( const char* iMessage );
       void SendMessage( const char* iBuffer, unsigned int iDataSize );

       void WriteSocketData();

       void HandleMessage();
       void PopulateEventList();
       void PopulateEventInfo();
       void PopulateEnumInfo();
       void AcceptThumbnail();
       void AcceptImage();
       void AcceptMesh();

       void HandleShaderDebug();
       void HandleShaderDebugFailed();
       void HandleCurrentRTUpdate();

       void HandlePixelTestResponse();
       void HandleClearedRenderStateOverrides();

       void RegisterRenderStateTypes();
       void RequestThumbnail( int iEventNum, unsigned __int64, unsigned int iType,
                              QStandardItem* iDestination );

       void RequestTexture( int iEventNum, unsigned __int64, unsigned int iType );

       void CreateTextureWindow( unsigned int iType, bool iDebuggable );
       void TextureWindowClosed( TextureViewerFast* iWindow );
       void MeshWindowClosed( MeshViewer* iWindow );

       void DebugPixel( int iX, int iY );
       void DebugVertex( unsigned int iVertex );
       void WhyDidntPixelRender( int iX, int iY );

       void closeEvent(QCloseEvent *event);
       void SetCaptureActive( bool iEnabled );

       void GotoDrawCall( int drawCallId );

       void RefreshGUIOptions();
private:
    friend class EnterEventFilter;
    friend class TextureViewerFast;
    friend class MeshViewer;

    Ui::MainWindow*     ui;
    QStandardItemModel* mDrawModel;
    QStandardItemModel* mStatesModel;
    QTcpSocket*         mSocket;

    QTreeView*          mStatesview;
    QTreeView*          mDrawview;

    bool                mConnectionEstablished;
    bool                mWritesPending;

    char*               mWriteBuffer;
    unsigned int        mWriteBufferSize;

    char*               mReadBuffer;
    unsigned int        mReadBufferSize;

    char*               mCurrentMessage;
    unsigned int        mCurrentMessageIter;
    unsigned int        mCurrentMessageSize;

    char*               mCurrentEventInfo;
    unsigned int        mCurrentEventInfoSize;

    int                 mCurrentEventNum;
    QString             mSavedRSEntryData;

    QAction*            mConnectAction;
    QAction*            mCaptureAction;
    QAction*            mWireFrameAction;

    bool                mCaptureActive;

    bool                mIgnoreRSEdits;

    QHash< int, RenderStateType> mRenderStateTypes;
    QHash< int, QHash< int, QString > > mEnumTypes;

    QList< ThumbnailData > mThumbnailCache;
    QList< ThumbnailPendingRequest > mThumbnailRequests;

    QList< ThumbnailPendingRequest > mTextureRequests;

    QList< TextureViewerFast* > mTextureWindows;
    QList< MeshViewer* >        mMeshWindows;

    ShaderDebugWindow*          mCompareWindow;

    bool                        mShowWireframe;
};

class EnterEventFilter : public QObject
{
    Q_OBJECT
public:
    EnterEventFilter( MainWindow* iWindow ){ mWindow = iWindow; }

    MainWindow* mWindow;
protected:
    bool eventFilter(QObject *obj, QEvent *event);
};

#endif // MAINWINDOW_H
