#ifndef MODBUSTCPCLIENT_H
#define MODBUSTCPCLIENT_H

#include <QMainWindow>
#include <QMessageBox>
#include <QTcpSocket>

QT_BEGIN_NAMESPACE
namespace Ui {
class ModbusClient;
}
QT_END_NAMESPACE

class ModbusClient : public QMainWindow {
  Q_OBJECT

public:
  ModbusClient(QWidget *parent = nullptr);
  ~ModbusClient();

private:
  Ui::ModbusClient *ui;

  QTcpSocket *m_pTcpSocket = nullptr;
  //  QDataStream m_responseStream;
  QString m_response;

  qint16 m_serverMessageSize;

  uint16_t m_commandSize;
  int m_cln;
  int m_row;

private:
  QByteArray createCommand();

private slots:
  void requestNewData();
  void readResponse();
  void displayError(QAbstractSocket::SocketError socketError);
  void enableSendDataButton();
  void on_pbConnect_clicked();
  void on_pbQuit_clicked();
  void slotDisconnected();
  void slotConnected();
  void on_pbAddByte_clicked();
  void on_pbDelByte_clicked();
};
#endif // MODBUSTCPCLIENT_H
