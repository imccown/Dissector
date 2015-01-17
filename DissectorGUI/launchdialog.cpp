#include "launchdialog.h"
#include "ui_launchdialog.h"
#include <QFileDialog>
#include <QSettings>

LaunchDialog::LaunchDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LaunchDialog)
{
    QSettings sets;
    ui->setupUi(this);

    mEditExe  = findChild<QPlainTextEdit*>("EditExe");
    mEditDir  = findChild<QPlainTextEdit*>("EditDir");
    mEditArgs = findChild<QPlainTextEdit*>("EditArgs");

    mEditExe->setPlainText( sets.value( "launcher/exename" ).toString() );
    mEditDir->setPlainText( sets.value( "launcher/dirname" ).toString() );
    mEditArgs->setPlainText( sets.value( "launcher/exeargs" ).toString() );

    ReadSettings();
}

LaunchDialog::~LaunchDialog()
{
    WriteSettings();
    delete ui;
}

void LaunchDialog::on_BrowseDir_pressed()
{
    QString dirName = QFileDialog::getExistingDirectory( this, tr("Working Directory") );

    if( !dirName.isEmpty() )
    {
        mEditDir->setPlainText( dirName );
    }
}

void LaunchDialog::on_BrowseExe_pressed()
{
    QString oldFilename = mEditExe->toPlainText();
    QFileInfo infoOld(oldFilename);
    QString exeDir = infoOld.absoluteDir().canonicalPath();
    QString oldDirectory = mEditDir->toPlainText();


    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                     exeDir,
                                                     tr("Files (*.exe)"));
    if( !fileName.isEmpty() )
    {
        mEditExe->setPlainText( fileName );
        QFileInfo infoNew(fileName);

        QString newDir = infoNew.absoluteDir().canonicalPath();
        if( oldDirectory.isEmpty() || (infoOld.isExecutable() &&
                QString::compare( exeDir, oldDirectory, Qt::CaseInsensitive ) == 0) )
        {
            mEditDir->setPlainText( newDir );
        }
    }
}

void LaunchDialog::ReadSettings()
{
    QSettings settings;
    restoreGeometry(settings.value("launcher/geometry").toByteArray());
    if( settings.value("launcher/maximized", false).toBool() )
    {
       move( settings.value("launcher/topleft").toPoint() );
    }
}

void LaunchDialog::WriteSettings()
{
    QSettings settings;
    settings.setValue("launcher/geometry",    saveGeometry());
    settings.setValue("launcher/maximized",   isMaximized());
    settings.setValue("launcher/topleft",     parentWidget()->mapToGlobal( geometry().topLeft() ) );

}
