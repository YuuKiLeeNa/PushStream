#include "testOpenGL.h"
#include <QtWidgets/QApplication>
#include "utilfun.h"

int main(int argc, char *argv[])
{
    initLog();

    QApplication a(argc, argv);
    testOpenGL w;
    w.show();
    return a.exec();
}
