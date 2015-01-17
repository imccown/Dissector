#ifndef SHADERDEBUGWINDOW_H
#define SHADERDEBUGWINDOW_H
#include <Dissector.h>

#include <QDialog>
#include <QAction>
#include <QStandardItemModel>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QPushButton>
class MainWindow;

namespace Ui {
class ShaderDebugWindow;
}

class ShaderDebugWindow : public QDialog
{
    Q_OBJECT

public:
    explicit ShaderDebugWindow(QWidget *parent = 0);
    ~ShaderDebugWindow();

    void ProcessInputData( MainWindow* iMain, char* iData, size_t iDataSize );

    void ReadSettings();
    void WriteSettings();

private slots:
       void on_stepButton_clicked();
       void on_stepBackButton_clicked();
       void on_restartButton_clicked();

       void on_pushButton_clicked(bool checked);

private:
    virtual void closeEvent ( QCloseEvent * e ){ WriteSettings(); QDialog::closeEvent( e ); delete this; }

public:
    Ui::ShaderDebugWindow *ui;

    void GotoCurrentLine();
    void GotoStep( int iStep );
    void RefreshVariables();
    void CreateAnalyzeListing();

    void CreateStepList( QVector<QString>& iFileLineList, QVector<int>& oStepList );

    struct VariableEntry
    {
        int mType;
        int mColumns;
        int mRows;
        int mVariableBlockOffset;
        QString mName;
        QStandardItem *mListItem;
    };

    struct DebugStep
    {
        int mFilename;
        int mLine;
        QList< Dissector::ShaderDebugVariableUpdate > mUpdates;
    };

    struct VariableReport
    {
        VariableEntry* mVar;
        QVector< QVector< float > > mValues;
    };

    void GetVariableReports( QVector<VariableReport>& oVars );

    QVector< QString > mFilenames;
    QVector< VariableEntry > mVariables;
    QVector< DebugStep > mSteps;

    QList<QString> mListing;
    QAction mStepForward, mStepBack, mStepIn, mRestart;

    unsigned int mFirstValidStep;
    unsigned int mCurrentFile;
    int mCurrentStep;

    char* mDataBlock;
    char* mValidDataBlock;
    unsigned int mDataBlockSize;

    QPushButton*        mCompareButton;
    QStandardItemModel  mVarsModel;
    MainWindow*         mMainWindow;

    bool                mHighlightValid;
    QTextBlock          mHighlightedBlock;
    QTextCharFormat     mDefaultTextFormat;
};

#endif // SHADERDEBUGWINDOW_H
