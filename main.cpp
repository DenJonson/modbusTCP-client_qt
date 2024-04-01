#include "modbusclient.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ModbusClient w;
    w.show();
    return a.exec();
}
