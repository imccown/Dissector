#ifndef MESHVIEWER_H
#define MESHVIEWER_H

#include <QWindow>
#include <QOpenGLFunctions>
#include <QCloseEvent>
#include <QVector3D>
class QOpenGLShaderProgram;
class QOpenGLPaintDevice;
class MainWindow;

class MeshViewer : public QWindow, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit MeshViewer(QWindow *parent = 0);
    ~MeshViewer();

    virtual void render(QPainter *painter);
    virtual void render();

    virtual void initialize();

    void setAnimating(bool animating);
    void setMesh( int iType, unsigned int iNumprims, float* iMesh, int numVerts,
                  unsigned int* iIndices, int numIndices );

    void setMainWindow( MainWindow* iMainWindow ){ mMainWindow = iMainWindow; }

    int     mEventID;

    void ReadSettings();
    void WriteSettings();
    void SetupStateSave();

public slots:
    void renderLater();
    void renderNow();

    void windowClosed();

    void visChanged(QWindow::Visibility visibility);

protected:
    bool event(QEvent *event);

    void exposeEvent(QExposeEvent *event);
    void scrollDelta( const QPoint& iDelta );
    bool processWheel(QWheelEvent *);

private:
    MainWindow* mMainWindow;

    bool mUpdatePending;
    bool mAnimating;

    QOpenGLContext *mContext;
    QOpenGLPaintDevice *mDevice;

    QOpenGLShaderProgram *mShader;
    GLuint mPosAttr;
    GLuint mNormalAttr;
    GLuint mColUniform;
    GLuint mMatrixUniform;
    GLuint mViewMatrixUniform;

    QOpenGLShaderProgram *mShaderNorm;
    GLuint mNormPosAttr;
    GLuint mNormNormalAttr;
    GLuint mNormColUniform;
    GLuint mNormMatrixUniform;
    GLuint mNormViewMatrixUniform;

    int    mPrimType;

    float mDistanceScale;
    float mRotationZ, mRotationY;

    unsigned int mNumPrims;

    float*  mMesh;
    float*  mNormals;
    int     mNumVerts;

    unsigned int* mIndices;
    int     mNumIndices;
    bool   mIgnoreVisChanges;

    QVector3D   mCenter, mBaseDistance;
    friend class MeshViewEventFilter;
};

class MeshViewEventFilter : public QObject
{
    Q_OBJECT
public:
    MeshViewEventFilter(): mLeftMouseDown( false ) {}
    MeshViewEventFilter( MeshViewer* iView ){ mView = iView; }

    MeshViewer* mView;
protected:
    bool eventFilter(QObject *obj, QEvent *event);

protected:
    bool mLeftMouseDown;
    QPoint mDragPosition;
    friend class MeshViewer;
};

#endif // MESHVIEWER_H
