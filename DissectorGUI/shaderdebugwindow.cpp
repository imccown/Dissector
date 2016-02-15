#include "shaderdebugwindow.h"
#include "ui_shaderdebugwindow.h"
#include <QFile>
#include <QTreeView>
#include <QTextBlock>
#include <DissectorHelpers.h>
#include <QKeySequence>
#include <QListView>
#include <QSettings>
#include "mainwindow.h"
#include <limits>
#include <QFileDialog>

ShaderDebugWindow::ShaderDebugWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ShaderDebugWindow),
    mDataBlock( NULL ),
    mValidDataBlock( NULL ),
    mFirstValidStep( 0 ),
    mCurrentFile( -1 ),
    mCurrentStep( -1 ),
    mHighlightValid( false ),
    mStepForward( "Step Forward", this ),
    mStepBack( "Step Back", this ),
    mStepIn( "Step In", this ),
    mRestart( "Restart Simulation", this )
{
    ui->setupUi(this);
    mStepForward.setShortcut( QKeySequence( Qt::Key_F10 ) );
    mStepBack.setShortcut( QKeySequence( Qt::Key_F11) );
    //mStepIn.setShortcut( QKeySequence( Qt::Key_F11 ) );
    mRestart.setShortcut( QKeySequence( Qt::Key_F5 ) );

    connect( &mStepForward, SIGNAL(triggered()), this, SLOT(on_stepButton_clicked()) );
    connect( &mStepBack, SIGNAL(triggered()), this, SLOT(on_stepBackButton_clicked()) );
    connect( &mRestart, SIGNAL(triggered()), this, SLOT(on_restartButton_clicked()) );

    QTextEdit* text = findChild<QTextEdit*>( "textEdit" );

    mDefaultTextFormat = text->currentCharFormat();
    addAction( &mStepForward );
    addAction( &mStepBack );
    addAction( &mStepIn );
    addAction( &mRestart );

    if( text )
    {
        addAction( &mStepForward );
        addAction( &mStepBack );
        addAction( &mStepIn );
        addAction( &mRestart );
    }

    QTreeView* listview = findChild<QTreeView*>("treeView_2");
    mVarsModel.setColumnCount( 2 );
    listview->setModel( &mVarsModel );

    QStringList columnNames;
    columnNames << "Name" << "Value";
    mVarsModel.setHorizontalHeaderLabels( columnNames );

    mCompareButton = findChild<QPushButton*>("pushButton");

    ReadSettings();
}

ShaderDebugWindow::~ShaderDebugWindow()
{
    if( mMainWindow )
        mMainWindow->UnmarkShaderForCompare( this );

    delete ui;

    if( mDataBlock )
    {
        delete[] mDataBlock;
    }

    if( mValidDataBlock )
    {
        delete[] mValidDataBlock;
    }
}

void ShaderDebugWindow::ProcessInputData( MainWindow* iMain, char* iData, size_t iDataSize )
{
    mMainWindow = iMain;
    iDataSize;

    char* iter = iData;
    Dissector::ShaderDebugHeader hdr;
    hdr = GetBufferData< Dissector::ShaderDebugHeader >( iter );

    for( int ii = 0; ii < hdr.mNumFilenames; ++ii )
    {
        int size = GetBufferData< int >( iter );
        QString str = QString::fromLocal8Bit( iter, size );
        iter += size;
        mFilenames.push_back( str );

        // Only the base filename is a full path, need to try to complete other filenames from that path
        if( str.length() >= 2 && str[1] == ':' ) {
            std::string path = str.toLocal8Bit();
            size_t findpos = path.rfind( '\\' );
            if( findpos != std::string::npos ) {
                path = path.substr( 0, findpos+1 );
                mIncludeDirectories.push_back( path );
                mIncludeDirectories.push_back( path + "..\\" );
                mIncludeDirectories.push_back( path + "Include\\" );
                mIncludeDirectories.push_back( path + "..\\Include\\" );
            }
        }
    }

    for( int ii = 0; ii < hdr.mNumVariables; ++ii )
    {
        Dissector::ShaderDebugVariable var;
        var = GetBufferData< Dissector::ShaderDebugVariable >( iter );
        int nameLen = GetBufferData< int >( iter );

        QString str = QString::fromLocal8Bit( iter, nameLen );
        iter += nameLen;

        VariableEntry vare;
        vare.mType = var.mType;
        vare.mColumns = var.mColumns;
        vare.mRows = var.mRows;
        vare.mVariableBlockOffset = var.mVariableBlockOffset;
        vare.mName = str;

        QList<QStandardItem *> rowItems;
        rowItems << new QStandardItem(str);
        QStandardItem *editableItem = new QStandardItem("a");
        rowItems << editableItem;

        mVarsModel.appendRow( rowItems );

        vare.mListItem = editableItem;
        mVariables.push_back( vare );
    }

    for( int ii = 0; ii < hdr.mNumSteps; ++ii )
    {
        Dissector::ShaderDebugStep step =
                GetBufferData< Dissector::ShaderDebugStep > ( iter );

        DebugStep dstep;
        dstep.mFilename = step.mFilename;
        dstep.mLine = step.mLine;

        for( int jj = 0; jj < step.mNumUpdates; ++jj )
        {
            Dissector::ShaderDebugVariableUpdate vu =
                    GetBufferData< Dissector::ShaderDebugVariableUpdate >( iter );

            dstep.mUpdates.push_back( vu );
        }

        mSteps.push_back( dstep );
    }

    mDataBlock = new char[hdr.mVariableBlockSize];
    mValidDataBlock = new char[hdr.mVariableBlockSize];
    memset( mDataBlock, 0, hdr.mVariableBlockSize );
    memset( mValidDataBlock, 0, hdr.mVariableBlockSize );
    mDataBlockSize = hdr.mVariableBlockSize;

    mFirstValidStep = 0;
    for( auto iter = mSteps.begin(); iter != mSteps.end(); ++iter )
    {
        if( iter->mFilename >= 0 )
        {
            break;
        }
        mFirstValidStep++;
    }

    GotoStep( mFirstValidStep );
    CreateAnalyzeListing();
}

void ShaderDebugWindow::GotoCurrentLine()
{
    if( mCurrentStep >= (int)mSteps.size() )
        return;

    DebugStep& step = mSteps[ mCurrentStep ];
    QTextEdit* text = findChild<QTextEdit*>( "textEdit" );
    // TODO: Multiple tabs or something
    if( text )
    {
        if( step.mFilename < mFilenames.size() && step.mFilename != mCurrentFile )
        {
            char buffer[1024];
            size_t readSize = 1;
            text->clear();

            QTextCursor cursor = text->textCursor();
            cursor.setCharFormat( mDefaultTextFormat );
            cursor.clearSelection();
            text->setTextCursor( cursor );

            FILE* f;
            fopen_s( &f, mFilenames[ step.mFilename ].toLocal8Bit(), "r" );
            int includeIter = 0;
            while( !f && includeIter < mIncludeDirectories.size() ) {
                std::string filename = (mIncludeDirectories[includeIter] + mFilenames[ step.mFilename ].toLocal8Bit().data() );
                fopen_s( &f, filename.c_str(), "r" );
                if( f ) {
                    mFilenames[ step.mFilename ] = QString( filename.c_str() );
                    break;
                }
                includeIter++;
            }
            if( !f ) {
                std::string str = mFilenames[ step.mFilename ].toLocal8Bit().data();
                size_t pos = str.rfind( '\\' );
                if( pos != std::string::npos ) {
                    str = str.substr( pos );
                }
                str = str + "(" + str + ")";
                // Not in any include directories. Pop up a dialog.
                QFileDialog fd( 0, QString("Missing file. Locating \"%1\"").arg( mFilenames[ step.mFilename ] ),
                               mIncludeDirectories[0].c_str(), QString( str.c_str() ) ) ;
                fd.setFileMode( QFileDialog::ExistingFile );
                if( fd.exec() ) {
                    QString newFile;
                    fd.selectFile( newFile );
                    newFile = fd.selectedFiles()[0];
                    fopen_s( &f, newFile.toLocal8Bit().data(), "r" );
                    if( f ) {
                        mFilenames[ step.mFilename ] = newFile;
                    }
                }
            }
            if( f ) {

                while( readSize )
                {
                    readSize = fread( buffer, 1, 1023, f );
                    buffer[readSize] = 0;
                    text->insertPlainText( QString( buffer ) );
                }
                fclose( f );
            } else {
                text->insertPlainText( QString( "Couldn't Load File %1" ).arg( mFilenames[ step.mFilename ] ) );
            }

            text->setReadOnly( true );
            mCurrentFile = step.mFilename;
            mHighlightValid = false;
            mDefaultTextFormat = text->currentCharFormat();
        }

        if( step.mLine < unsigned short(-1) )
        {
            if( mHighlightValid )
            {
                QTextCursor cursor = text->textCursor();
                cursor.setPosition( mHighlightedBlock.position() );
                cursor.select( QTextCursor::LineUnderCursor );
                cursor.setCharFormat( mDefaultTextFormat );
                cursor.clearSelection();
            }

            QTextCharFormat fmt;

            QPalette pl = text->palette();
            fmt.setBackground( pl.color( QPalette::Highlight ) );
            fmt.setForeground( pl.color( QPalette::HighlightedText ) );

            mHighlightedBlock = text->document()->findBlockByLineNumber( step.mLine-1 );
            QTextCursor cursor = text->textCursor();
            cursor.setPosition( mHighlightedBlock.position() );
            cursor.select( QTextCursor::LineUnderCursor );
            cursor.setCharFormat( fmt );
            cursor.clearSelection();
            text->setTextCursor( cursor );
            text->ensureCursorVisible();

            mHighlightValid = true;
        }
    }
}

void ShaderDebugWindow::GotoStep( int iStep )
{
    int destStep = std::min<unsigned int>( iStep, mSteps.size()-1 );
    destStep = std::max( 0, destStep );
    if( destStep == mCurrentStep )
        return;

    mCurrentStep = std::max( 0, mCurrentStep );
    int startStep = destStep < mCurrentStep ? 0 : mCurrentStep;
    if( destStep < mCurrentStep )
    {
        startStep = 0;
        memset( mDataBlock, 0, mDataBlockSize );
    }

    for( int ii = startStep; ii < destStep; ++ii )
    {
        QList< Dissector::ShaderDebugVariableUpdate >& updates = mSteps[ii].mUpdates;

        for( QList< Dissector::ShaderDebugVariableUpdate >::iterator iter = updates.begin();
             iter != updates.end(); ++iter )
        {
            // Because the QTCreator debugger sucks.
            Dissector::ShaderDebugVariableUpdate& data = *iter;
            for( int jj = 0; jj < 4; ++jj )
            {
                if( iter->mUpdateOffsets[ jj ] != -1 )
                {
                    *(unsigned int*)&mDataBlock[ data.mDataBlockOffset +
                            sizeof(int) * data.mUpdateOffsets[jj] ] =
                            data.mDataInt[jj];

                    *(unsigned int*)&mValidDataBlock[ data.mDataBlockOffset +
                            sizeof(int) * data.mUpdateOffsets[jj] ] = 1;
                }
            }
        }
    }

    mCurrentStep = destStep;
    RefreshVariables();
    GotoCurrentLine();
}

void ShaderDebugWindow::RefreshVariables()
{
    for( auto iter = mVariables.begin(); iter != mVariables.end(); ++iter )
    {
        QString str;
        for( int ii = 0; ii < iter->mColumns; ++ii )
        {
            if( ii )
            {
                str +=", ";
            }
            unsigned int valid = *(unsigned int*)&mValidDataBlock[
                iter->mVariableBlockOffset + ii * sizeof(float) ];

            if( valid )
            {
                QString value;
                value.sprintf( "%f", *(float*)&mDataBlock[
                           iter->mVariableBlockOffset + ii * sizeof(float) ] );
                str += value;
            }
            else
            {
                str += "_";
            }
        }

        iter->mListItem->setText( str );
    }
}

void ShaderDebugWindow::on_stepButton_clicked()
{
    GotoStep( mCurrentStep + 1 );
}

void ShaderDebugWindow::on_stepBackButton_clicked()
{
    unsigned int nextStep = (mCurrentStep > 0) ? mCurrentStep - 1 : 0;
    nextStep = std::max( nextStep, mFirstValidStep );
    GotoStep( nextStep );
}

void ShaderDebugWindow::on_restartButton_clicked()
{
    GotoStep( mFirstValidStep );
}

void ShaderDebugWindow::ReadSettings()
{
    QSettings settings;
    restoreGeometry(settings.value("shaderdebug/geometry").toByteArray());
    if( settings.value("shaderdebug/maximized", false).toBool() )
    {
       move( settings.value("shaderdebug/topleft").toPoint() );
    }
}

void ShaderDebugWindow::WriteSettings()
{
    QSettings settings;
    settings.setValue("shaderdebug/geometry",    saveGeometry());
    settings.setValue("shaderdebug/maximized",   isMaximized());
    settings.setValue("shaderdebug/topleft",     parentWidget()->mapToGlobal( geometry().topLeft() ) );
}

void ShaderDebugWindow::CreateAnalyzeListing()
{
    mListing.clear();
    int size = mSteps.size();
    for( int ii = mFirstValidStep; ii < size; ++ii )
    {
        DebugStep& step = mSteps[ ii ];
        QString line;
        if( step.mFilename >= 0 && step.mFilename < mFilenames.size() )
        {
            line += mFilenames[ step.mFilename ];
            line += QString("").arg(step.mLine);
            mListing.push_back( line );
        }
    }
}

void ShaderDebugWindow::CreateStepList( QVector<QString>& iFileLineList,
                                        QVector<int>& oStepList )
{
    // Converts from the CreateAnaylzeListing format back to step numbers
    int lastStep = mFirstValidStep;
    for( int ii = 0, size = iFileLineList.size(); ii < size; ++ii )
    {
        int matchNum = -1;
        QString line = iFileLineList[ii];
        if( !line.isEmpty() )
        {
            for( int jj = lastStep, size = mSteps.size(); jj < size; ++jj )
            {
                DebugStep& step = mSteps[ jj ];
                if( step.mFilename >= 0 && step.mFilename < mFilenames.size() )
                {
                    QString stepLine = mFilenames[ step.mFilename ];
                    stepLine += QString("").arg(step.mLine);
                    if( QString::compare( stepLine, line ) == 0 )
                    {
                        matchNum = jj;
                        break;
                    }
                }
            }
        }

        if( matchNum != -1 )
            lastStep = matchNum;
        else
            matchNum = lastStep;

        oStepList.push_back( matchNum );
    }
}

void ShaderDebugWindow::GetVariableReports( QVector<VariableReport>& oVars )
{
    for( auto iter = mVariables.begin(); iter != mVariables.end(); ++iter )
    {
        VariableReport rep;
        rep.mVar = &*iter;
        rep.mValues.resize( iter->mRows );
        for( int ii = 0; ii < iter->mRows; ++ii )
        {
            for( int jj = 0; jj < iter->mColumns; ++jj )
            {
                unsigned int valid = *(unsigned int*)&mValidDataBlock[
                    iter->mVariableBlockOffset + jj * sizeof(float) ];

                if( valid )
                {
                    rep.mValues[ii].push_back( *(float*)&mDataBlock[
                                           iter->mVariableBlockOffset + jj * sizeof(float) ] );
                }
                else
                {
                    rep.mValues[ii].push_back( std::numeric_limits<float>::quiet_NaN() );
                }
            }
        }

        oVars.push_back( rep );
    }
}

void ShaderDebugWindow::on_pushButton_clicked(bool checked)
{
    if( !mMainWindow )
        return;

    if( checked )
        mMainWindow->MarkShaderForCompare( this );
    else
        mMainWindow->UnmarkShaderForCompare( this );

}
