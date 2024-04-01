#ifndef MODBUSCLIENT_H
#define MODBUSCLIENT_H

#include <QMainWindow>
#include <QtNetwork/QTcpSocket>
#include <QMessageBox>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class ModbusClient; }
QT_END_NAMESPACE

class ModbusClient : public QMainWindow
{
    Q_OBJECT

public:
    ModbusClient(QWidget *parent = nullptr);
    ~ModbusClient();

private:
    Ui::ModbusClient *ui;

    QTcpSocket *m_pTcpSocket = nullptr;
    QDataStream m_responseStream;
    QString m_response;

private slots:
    void requestNewData();
    void readResponse();
    void displayError(QAbstractSocket::SocketError socketError);
    void enableSendDataButton();
};
#endif // MODBUSCLIENT_H
