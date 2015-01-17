#include "meshviewer.h"
#include <QApplication>
#include <QWindow>
#include <QOpenGLContext>
#include <QOpenGLPaintDevice>
#include <QOpenGLShaderProgram>
#include <QPainter>
#include "mainwindow.h"
#include <qmath.h>
#include <Dissector.h>
#include <QSettings>

static const char *vertexShaderSource =
    "attribute highp vec4 posAttr;\n"
    "varying lowp vec4 col;\n"
    "uniform highp mat4 matrix;\n"
    "uniform lowp vec4 colUniform;\n"
    "void main() {\n"
    "   col = colUniform;\n"
    "   gl_Position = matrix * posAttr;\n"
    "}\n";

static const char *vertexNormalShaderSource =
    "attribute highp vec4 posAttr;\n"
    "attribute highp vec4 normalAttr;\n"
    "varying lowp vec4 col;\n"
    "uniform highp mat4 matrix;\n"
    "uniform highp mat4 viewmatrix;\n"
    "uniform lowp vec4 colUniform;\n"
    "void main() {\n"
    "   float normCol = clamp( (viewmatrix * normalAttr).z, 0.0, 0.75 );\n"
    "   col = colUniform;\n"
    "   col.x = .25 + normCol;\n"
    "   col.y = .25 + normCol;\n"
    "   col.z = .25 + normCol;\n"
    "   col.w = 1.0;\n"
    "   col = col * col;\n"
    "   gl_Position = matrix * posAttr;\n"
    "}\n";

static const char *fragmentShaderSource =
    "varying lowp vec4 col;\n"
    "void main() {\n"
    "   gl_FragColor = col;\n"
    "}\n";


bool MeshViewEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    if( event->type() == QEvent::MouseMove )
    {
        QMouseEvent* ev = (QMouseEvent*)event;
        if( mLeftMouseDown )
        {
            QPoint pos = ev->globalPos();
            QPoint delta = pos - mDragPosition;
            mDragPosition = pos;

            mView->scrollDelta( delta );
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

MeshViewer::MeshViewer(QWindow *parent)
    : QWindow(parent)
    , mUpdatePending(false)
    , mAnimating(false)
    , mContext(0)
    , mDevice(0)
    , mMesh( NULL )
    , mIndices( NULL )
    , mNumPrims( 3 )
    , mPrimType( GL_TRIANGLES )
{
    setSurfaceType(QWindow::OpenGLSurface);
    mCenter[0] = mCenter[1] = mCenter[2] = 0.f;
    mBaseDistance[0] = mBaseDistance[1] = 0.f;
    mBaseDistance[2] = .5f;
    mDistanceScale = 1.f;
    mRotationZ = 0.f;
    mRotationY = 0.f;

    MeshViewEventFilter* evFilter = new MeshViewEventFilter( this );
    evFilter->mLeftMouseDown = false;
    installEventFilter( evFilter );

    mIgnoreVisChanges = false;
}

MeshViewer::~MeshViewer()
{
    delete mDevice;
    if( mMesh )
        delete[] mMesh;

    if( mIndices )
        delete[] mIndices;

    if( mMainWindow )
        mMainWindow->MeshWindowClosed( this );
}

void MeshViewer::windowClosed()
{
    delete this;
}

void MeshViewer::render(QPainter *painter)
{
    Q_UNUSED(painter);
}

void MeshViewer::initialize()
{
    mShader = new QOpenGLShaderProgram(this);
    mShader->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    mShader->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    mShader->link();
    mPosAttr = mShader->attributeLocation("posAttr");
    mColUniform = mShader->uniformLocation("colUniform");
    mMatrixUniform = mShader->uniformLocation("matrix");


    mShaderNorm = new QOpenGLShaderProgram(this);
    if( !mShaderNorm->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexNormalShaderSource) )
    {
        QString err = mShaderNorm->log();
        err;
    }
    mShaderNorm->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    mShaderNorm->link();
    mNormPosAttr = mShaderNorm->attributeLocation("posAttr");
    mNormNormalAttr = mShaderNorm->attributeLocation("normalAttr");
    mNormColUniform = mShaderNorm->uniformLocation("colUniform");
    mNormMatrixUniform = mShaderNorm->uniformLocation("matrix");
    mNormViewMatrixUniform = mShaderNorm->uniformLocation("viewmatrix");
}

void MeshViewer::render()
{
    if (!mDevice)
        mDevice = new QOpenGLPaintDevice;

    if( mPrimType < 0 )
        return;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    mDevice->setSize(size());

    QPainter painter(mDevice);
    render(&painter);
    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);
    glCullFace( GL_FRONT_AND_BACK );
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    QOpenGLShaderProgram* shader = mNormals ? mShaderNorm : mShader;
    shader->bind();

    QVector3D delta = mBaseDistance * mDistanceScale;
    QVector3D upVec( 0.f, 1.f, 0.f );

    // rotate about x
    {
        float s = sinf( mRotationZ );
        float c = cosf( mRotationZ );
        float z = delta.z();
        float y = delta.y();
        delta.setY( y*c-z*s );
        delta.setZ( y*s+z*c );
    }
    {
        float s = sinf( mRotationZ );
        float c = cosf( mRotationZ );
        float z = upVec.z();
        float y = upVec.y();
        upVec.setY( y*c-z*s );
        upVec.setZ( y*s+z*c );
    }

    // rotate about Y
    {
        float s = sinf( mRotationY );
        float c = cosf( mRotationY );
        float x = delta.x();
        float z = delta.z();
        delta.setX( z * s + x * c );
        delta.setZ( z * c - x * s );
    }
    {
        float s = sinf( mRotationY );
        float c = cosf( mRotationY );
        float x = upVec.x();
        float z = upVec.z();
        upVec.setX( z * s + x * c );
        upVec.setZ( z * c - x * s );
    }

    QMatrix4x4 proj,view;
    proj.perspective(45.0f, 4.0f/3.0f, 0.1f, 100000.0f);
    view.lookAt( mCenter + delta,
                   mCenter,
                   upVec );
    QMatrix4x4 matrix = proj * view;

    QColor color;
    color.setRgb( 255, 255, 0, 255 );

    shader->setUniformValue(mNormals ? mMatrixUniform : mNormMatrixUniform,
                             matrix);
    shader->setUniformValue(mNormals ? mNormColUniform : mColUniform, color);

    if( mNormals )
    {
        glVertexAttribPointer(mNormNormalAttr, 4, GL_FLOAT, GL_FALSE, 0, mNormals );
        shader->setUniformValue(mNormViewMatrixUniform, view);
    }

    GLfloat vertices[] = {
        0.0f, 0.707f, 0.f, 1.f,
        -0.5f, -0.5f, 0.f, 1.f,
        0.5f, -0.5f, 0.f, 1.f
    };

    GLuint posAttr = mNormals ? mNormPosAttr : mPosAttr;
    if( mMesh )
        glVertexAttribPointer(posAttr, 4, GL_FLOAT, GL_FALSE, 0, mMesh);
    else
        glVertexAttribPointer(posAttr, 4, GL_FLOAT, GL_FALSE, 0, vertices);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    // Arguments make these sound like they want number of primitives. They lie.
    if( mNumIndices )
        glDrawElements( mPrimType, 3*mNumIndices, GL_UNSIGNED_INT, mIndices );
    else
        glDrawArrays(mPrimType, 0, 3*mNumPrims);

    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);

    shader->release();
}

void MeshViewer::renderLater()
{
    if (!mUpdatePending) {
        mUpdatePending = true;
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    }
}

bool MeshViewer::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::UpdateRequest:
        mUpdatePending = false;
        renderNow();
        return true;
    case QEvent::Close:
    {
        WriteSettings();
        bool rvalue = QWindow::event(event);
        if( mMainWindow )
            mMainWindow->MeshWindowClosed( this );
        return rvalue;
    }   break;
    default:
        return QWindow::event(event);
    }
}

void MeshViewer::exposeEvent(QExposeEvent *event)
{
    Q_UNUSED(event);

    if (isExposed())
        renderNow();
}

void MeshViewer::renderNow()
{
    if (!isExposed())
        return;

    bool needsInitialize = false;

    if (!mContext) {
        mContext = new QOpenGLContext(this);
        QSurfaceFormat format = requestedFormat();
        mContext->setFormat( format );
        mContext->create();

        needsInitialize = true;
    }

    mContext->makeCurrent(this);

    if (needsInitialize) {
        initializeOpenGLFunctions();
        initialize();
    }

    render();

    mContext->swapBuffers(this);

    if (mAnimating)
        renderLater();
}

void MeshViewer::setAnimating(bool animating)
{
    mAnimating = animating;

    if (animating)
        renderLater();
}

void MeshViewer::setMesh( int iType, unsigned int iNumPrims, float* iMesh,
                          int numVerts, unsigned int* iIndices, int numIndices )
{
    float center[3] = { 0, 0, 0 };
    float* iter = iMesh;
    float divNum = 1;

    for( int ii = 0; ii < numVerts; ++ii, iter += 4, divNum += 1.f )
    {
        center[0] += (iter[0] - center[0]) / divNum;
        center[1] += (iter[1] - center[1]) / divNum;
        center[2] += (iter[2] - center[2]) / divNum;
    }

    float extents[3] = { 0, 0, 0 };
    iter = iMesh;
    for( int ii = 0; ii < numVerts; ++ii, iter += 4, divNum += 1.f )
    {
        extents[0] = std::max( extents[0], abs(iter[0] - center[0]) );
        extents[1] = std::max( extents[1], abs(iter[1] - center[1]) );
        extents[2] = std::max( extents[2], abs(iter[2] - center[2]) );
    }

    for( int ii = 0; ii < 3; ++ii )
    {
        mCenter[ii] = center[ii];
    }

    int maxIndex = 0, midIndex = 1, minIndex = 2;
    if( extents[maxIndex] < extents[midIndex] )
        std::swap( maxIndex, midIndex );
    if( extents[midIndex] < extents[minIndex] )
        std::swap( minIndex, midIndex );
    if( extents[maxIndex] < extents[midIndex] )
        std::swap( maxIndex, midIndex );

    mBaseDistance[2] = 2.f * extents[maxIndex];
    mBaseDistance[1] = 0.f;
    mBaseDistance[0] = 0.f;

    if( maxIndex == 1 )
    {
        mRotationZ = 0.f;
        mRotationY = 0.f;
    }
    else if( maxIndex == 0 )
    {
        mRotationZ = 0.f;
        mRotationY = 0.f;
    }
    else
    {
        mRotationZ = 0.f;
        mRotationY = float(M_PI_2);
    }

    switch( iType )
    {
    case(Dissector::PrimitiveType::PointList): mPrimType = GL_POINTS; break;
    case(Dissector::PrimitiveType::LineList): mPrimType = GL_LINES; break;
    case(Dissector::PrimitiveType::LineStrip): mPrimType = GL_LINE_STRIP; break;
    case(Dissector::PrimitiveType::TriangleList): mPrimType = GL_TRIANGLES; break;
    case(Dissector::PrimitiveType::TriangleStrip): mPrimType = GL_TRIANGLE_STRIP; break;
    default:
        mPrimType = -1;
    }

    mNumPrims = iNumPrims;

    if( mPrimType == GL_TRIANGLES || mPrimType == GL_TRIANGLE_STRIP )
    {
        // Add normals for meshes so visualization is better
        int stride = mPrimType == GL_TRIANGLE_STRIP ? 1 : 3;
        mPrimType = GL_TRIANGLES;
        mNumIndices = 0;
        mIndices = NULL;
        mMesh = new float[ mNumPrims * 3 * 4 ];
        float* writeIter = mMesh;
        if( numIndices )
        {
            unsigned int* indexIter = iIndices;
            unsigned int* indexEnd  = iIndices + (numIndices-2);
            for( ; indexIter < indexEnd; writeIter+=12, indexIter+=stride )
            {
                memcpy( &writeIter[0], &iMesh[ indexIter[0] * 4 ], sizeof(float)*4);
                memcpy( &writeIter[4], &iMesh[ indexIter[1] * 4 ], sizeof(float)*4);
                memcpy( &writeIter[8], &iMesh[ indexIter[2] * 4 ], sizeof(float)*4);
            }
        }
        else
        {
            float* readIter  = iMesh;
            float* readEnd   = iMesh + ((numVerts-2)*4);
            stride *= 4;
            for( ; readIter < readEnd; writeIter+=12, readIter+=stride )
            {
                memcpy( writeIter, readIter, sizeof(float)*12 );
            }
        }

        mNormals = new float[ mNumPrims * 3 * 4 ];
        float* normalIter = mNormals;
        float* vertIter = mMesh;
        for( unsigned int ii = 0; ii < mNumPrims; ++ii, vertIter+=12 )
        {
            // Face normals.
            float vec1[3], vec2[3];
            vec1[0] = vertIter[4] - vertIter[0];
            vec1[1] = vertIter[5] - vertIter[1];
            vec1[2] = vertIter[6] - vertIter[2];

            vec2[0] = vertIter[8]  - vertIter[0];
            vec2[1] = vertIter[9]  - vertIter[1];
            vec2[2] = vertIter[10] - vertIter[2];

            float normal[3];
            normal[0] = vec1[1] * vec2[2] - vec1[2] * vec2[1];//yz -zy
            normal[1] = vec1[2] * vec2[0] - vec1[0] * vec2[2];//zx - xz
            normal[2] = vec1[0] * vec2[1] - vec1[1] * vec2[0];//xy - yx
            float mag = normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2];
            if( mag <= 0.f )
                mag = 1.f;
            else
                mag = sqrtf( mag );
            normal[0] /= mag;
            normal[1] /= mag;
            normal[2] /= mag;
            for( int jj = 0; jj < 3; ++jj )
            {
                *normalIter++ = normal[0];
                *normalIter++ = normal[1];
                *normalIter++ = normal[2];
                *normalIter++ = 0.f;
            }
        }
    }
    else
    {
        mNormals = NULL;
        mMesh = new float[ numVerts * 4 ];
        memcpy( mMesh, iMesh, numVerts * sizeof(float) * 4 );
        mNumVerts = numVerts;

        if( numIndices )
        {
            mIndices = new unsigned int[ numIndices ];
            memcpy( mIndices, iIndices, sizeof(unsigned int) * numIndices );
        }
        else
        {
            mIndices = NULL;
        }
        mNumIndices = numIndices;
    }

    renderLater();
}

void MeshViewer::scrollDelta( const QPoint& iDelta )
{
    float newRotZ = mRotationZ - iDelta.y() * (M_PI/1000.f);

    if( newRotZ > float(M_PI_2) )
        newRotZ = float(M_PI_2);
    else if( newRotZ < -float(M_PI_2) )
        newRotZ = -float(M_PI_2);

    mRotationZ = newRotZ;
    mRotationY -= iDelta.x() * (M_PI/1000.f);
    renderLater();
}

bool MeshViewer::processWheel( QWheelEvent * event )
{
    if( event->delta() > 0 ) {
        mDistanceScale *= .9f;
        renderLater();
    } else if( event->delta() < 0 ) {
        mDistanceScale *= 1.1f;
        renderLater();
    } else {
        return false;
    }

    return true;
}

void MeshViewer::visChanged(QWindow::Visibility visibility)
{
    if( !mIgnoreVisChanges && visibility != Hidden )
    {
        QSettings settings;
        settings.setValue("meshviewer/visiblity", visibility);
    }
}

void MeshViewer::SetupStateSave()
{
    connect( this, SIGNAL(visibilityChanged(QWindow::Visibility)), this, SLOT(visChanged(QWindow::Visibility)) );
}

void MeshViewer::ReadSettings()
{
    mIgnoreVisChanges = true;
    QSettings settings;
    setGeometry( settings.value("meshviewer/geometry").toRect() );
    Visibility vis = Visibility( settings.value("meshviewer/visiblity", QWindow::Windowed ).toUInt());
    setVisibility( vis );
    mIgnoreVisChanges = false;
}

void MeshViewer::WriteSettings()
{
    QSettings settings;
    settings.setValue("meshviewer/geometry",   geometry());
    //settings.setValue("meshviewer/windowstate", windowState());
    settings.setValue("meshviewer/visiblity", visibility());
}
