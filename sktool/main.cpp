#include <QApplication>
#include "sktool.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Sktool sktool;
    sktool.show();
    return app.exec();
}
