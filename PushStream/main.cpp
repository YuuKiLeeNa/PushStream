#include "PushStream.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    initLog();
    QApplication a(argc, argv);
    PushStream w;
    w.show();
    return a.exec();
}
