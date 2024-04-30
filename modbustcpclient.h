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

#define UI_INPUT_MASK_8 ">HH"
#define UI_INPUT_MASK_16 ">HH HH"

class ModbusClient : public QMainWindow {
  Q_OBJECT

public:
  ModbusClient(QWidget *parent = nullptr);
  ~ModbusClient();

private:
  QString convertStrToCompleteHex(QString str, int size);

private:
  Ui::ModbusClient *ui;

  typedef struct {
    uint16_t transId;
    uint16_t protocolId;
    uint16_t transLen;
  } mbTcpInitTrans_t;

  QTcpSocket *m_pTcpSocket = nullptr;
  QString m_response;

  qint16 m_serverMessageSize;

  uint16_t m_commandSize;
  int m_cln;
  int m_row;

  uint16_t m_transID;
  uint16_t const m_protocolID;

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
