#include "shaderAnalyzer.h"
#include <QTemporaryFile>
#include <QDir>
#include <QGlobal.h>
#include <QVector>
#include <QString>
#include "ShaderDebugWindow.h"
#include <math.h>
#include <QMessageBox>
#include <QHash>

namespace ShaderAnalyzer
{
    bool FindDiffs( const QList<QString>& iListing0,
                    const QList<QString>& iListing1,
                    QVector<QString>& oExecutedLines0,
                    QVector<QString>& oExecutedLines1 )
    {
        QTemporaryFile tempFiles[2];
        if( !tempFiles[0].open() || !tempFiles[1].open() )
            return false;

        const QList<QString>* listings[2] = { &iListing0, &iListing1 };
        for( int ii = 0; ii < 2; ++ii )
        {
            for( auto iter = listings[ii]->begin(); iter != listings[ii]->end(); ++iter )
            {
                QByteArray arr = iter->toLocal8Bit();
                tempFiles[ii].write( arr.constData(), arr.size() );
                tempFiles[ii].write( "\n", 1 );
            }
            tempFiles[ii].close();
        }

        QString diffOutputName = QDir::tempPath() + "//dissectordiff_" + QString("%1").arg(qrand()) + ".txt";

        QString command = QString( "diff.exe -y %1 %2 > %3" ).arg( tempFiles[0].fileName() ).arg( tempFiles[1].fileName() ).
                arg( diffOutputName );

        system( command.toLocal8Bit() );

        QFile fin( diffOutputName );
        if( !fin.open( QIODevice::ReadOnly | QIODevice::Text ) )
            return false;

        while( !fin.atEnd() )
        {
            QByteArray line = fin.readLine();
            QString str(line);
            QStringList list;
            list = str.split(QRegExp("\\s+"),QString::SkipEmptyParts);
            if( list.size() == 2 )
            {
                oExecutedLines0.push_back( list.at( 0 ) );
                oExecutedLines1.push_back( list.at( 1 ) );
            }
            else if( list.size() == 3 )
            {
                QString divider = str.at( 1 );
                if( divider == "|" )
                {
                    oExecutedLines0.push_back( list.at( 0 ) );
                    oExecutedLines1.push_back( "" );

                    oExecutedLines0.push_back( "" );
                    oExecutedLines1.push_back( list.at( 2 ) );
                }
                else if( divider == "<" )
                {
                    oExecutedLines0.push_back( list.at( 0 ) );
                    oExecutedLines1.push_back( "" );
                }
                else if( divider == ">" )
                {
                    oExecutedLines0.push_back( "" );
                    oExecutedLines1.push_back( list.at( 2 ) );
                }
                list.first();
            }

        }

        return true;
    }

    struct VarAnalysis
    {
        VarAnalysis()
        {
            mTypeMismatch = false;
            mRowMismatch = false;
            mColumnMismatch = false;
        }

        QString mName;

        bool    mTypeMismatch;
        bool    mRowMismatch;
        bool    mColumnMismatch;

        QVector< QVector< float > > mLastDiff;
        QVector< QVector< float > > mBiggestDiff;
        QVector< QVector< float > > mLastPct;
        QVector< QVector< float > > mBiggestPct;
    };

    void AnalyzeShaders( ShaderDebugWindow* iWin0, ShaderDebugWindow* iWin1 )
    {
        QString output;
        QVector<QString> exes[2];
        QVector<int>     steps[2];
        FindDiffs( iWin0->mListing, iWin1->mListing, exes[0], exes[1] );
        iWin0->CreateStepList( exes[0], steps[0] );
        iWin1->CreateStepList( exes[1], steps[1] );

        int stepCount = std::min( steps[0].size(), steps[1].size() );
        if( stepCount <= 0 )
            return;

        QHash< QString, VarAnalysis > varHistory;

        for( int ii = 0; ii < stepCount; ++ii )
        {
            iWin0->GotoStep( steps[0][ii] );
            iWin1->GotoStep( steps[1][ii] );

            QVector<ShaderDebugWindow::VariableReport> vars[2];
            iWin0->GetVariableReports( vars[0] );
            iWin1->GetVariableReports( vars[1] );
            for( int jj = 0, count = vars[0].size(); jj < count; ++jj )
            {
                ShaderDebugWindow::VariableReport& var0 = vars[0][jj];
                // Find the other variable by name
                for( int kk = 0, count = vars[1].size(); kk < count; ++kk )
                {
                    ShaderDebugWindow::VariableReport& var1 = vars[1][kk];
                    if( QString::compare( var0.mVar->mName, var1.mVar->mName ) == 0 )
                    {
                        VarAnalysis& hist = varHistory[ var0.mVar->mName ];

                        QString line;
                        if( !hist.mTypeMismatch && var0.mVar->mType != var1.mVar->mType )
                        {
                            hist.mTypeMismatch = true;
                            line += QString( "Var %1 has different type %2 and %3\n" ).arg( var0.mVar->mName ).
                                    arg( var0.mVar->mType ).arg( var1.mVar->mType );
                        }

                        if( !hist.mColumnMismatch && var0.mVar->mColumns != var1.mVar->mColumns )
                        {
                            hist.mColumnMismatch = true;
                            line += QString( "Var %1 has different number of columns %2 and %3\n" ).arg( var0.mVar->mName ).
                                    arg( var0.mVar->mColumns ).arg( var1.mVar->mColumns );
                        }

                        if( !hist.mRowMismatch && var0.mVar->mRows != var1.mVar->mRows )
                        {
                            hist.mRowMismatch = true;
                            line += QString( "Var %1 has different number of rows %2 and %3\n" ).arg( var0.mVar->mName ).
                                    arg( var0.mVar->mRows ).arg( var1.mVar->mRows );
                        }

                        int rows = std::min( var0.mVar->mRows, var1.mVar->mRows );
                        int columns = std::min( var0.mVar->mColumns, var1.mVar->mColumns );

                        for( int mm = 0; mm < 4; ++mm )
                        {
                            QVector< QVector< float > >& arr = (&hist.mLastDiff)[mm];
                            if( arr.size() != rows )
                                arr.resize( rows );

                            for( int rr = 0; rr < rows; ++rr )
                            {
                                if( arr[rr].size() != columns )
                                {
                                    arr[rr].resize( columns );
                                    for( int cc = 0; cc < columns; ++cc )
                                    {
                                        arr[rr][cc] = 0.f;
                                    }
                                }
                            }
                        }

                        for( int rr = 0; rr < rows; ++rr )
                        {
                            for( int cc = 0; cc < columns; ++cc )
                            {
                                float val0 = var0.mValues[rr][cc];
                                float val1 = var1.mValues[rr][cc];

                                float absdiff = abs( val0 - val1 );
                                float pctdiff = absdiff/abs(val0);

                                hist.mLastDiff[rr][cc] = absdiff;
                                if( absdiff > hist.mBiggestDiff[rr][cc] )
                                    hist.mBiggestDiff[rr][cc] = absdiff;

                                hist.mLastPct[rr][cc] = pctdiff;
                                if( pctdiff > hist.mBiggestPct[rr][cc] )
                                {
                                    hist.mBiggestPct[rr][cc] = pctdiff;
                                    if( pctdiff > .05f ) // TODO: Make this an option or something.
                                        line += QString( "Var %1[%2][%3] differs by %4 (%5%)\n" ).arg( var0.mVar->mName ).
                                                arg( rr ).arg( cc ).arg( absdiff ).arg( 100.f * pctdiff );
                                }
                            }
                        }

                        if( !line.isEmpty() )
                        {
                            QString f0 = exes[0][ii];
                            QString f1 = exes[1][ii];
                            if( QString::compare( f0, f1 ) == 0 )
                            {
                                line = QString("At File %1:\n").arg( f0 ) + line;
                            }
                            else if( f0.isEmpty() )
                            {
                                line = QString("At File %1:\n").arg( f1 ) + line;
                            }
                            else if( f1.isEmpty() )
                            {
                                line = QString("At File %1:\n").arg( f0 ) + line;
                            }
                            else
                            {
                                line = QString("At Files %1 %2:\n").arg( f0 ).arg( f1 ) + line;
                            }

                            output += line;
                        }

                        break;
                    }
                }
            }
        }

        if( output.isEmpty() )
        {
            output = "Found no significant differences.";
        }

        // TODO: A fancy window or something here instead of a message box.
        QMessageBox msgbox;
        msgbox.setText( output );
        msgbox.setModal( false );
        msgbox.exec();
    }

};
