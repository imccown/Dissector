#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName("Dissector");
    QCoreApplication::setApplicationName("DissectorGUI");

    MainWindow w;
    w.ReadSettings();
    w.show();

    return a.exec();
}
