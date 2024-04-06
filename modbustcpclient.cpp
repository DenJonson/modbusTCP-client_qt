#include "modbustcpclient.h"
#include "ui_modbustcpclient.h"

#include <QtNetwork/QHostInfo>
#include <QtNetwork/QNetworkInterface>

ModbusClient::ModbusClient(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::ModbusClient),
      m_pTcpSocket(new QTcpSocket(this)) {
  ui->setupUi(this);

  setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

  ui->cbHostName->setEditable(true);
  // find out name of this machine
  QString name = QHostInfo::localHostName();
  if (!name.isEmpty()) {
    ui->cbHostName->addItem(name);
    QString domain = QHostInfo::localDomainName();
    if (!domain.isEmpty())
      ui->cbHostName->addItem(name + QChar('.') + domain);
  }
  if (name != QLatin1String("localhost"))
    ui->cbHostName->addItem(QString("localhost"));
  // find out IP addresses of this machine
  QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
  // add non-localhost addresses
  for (int i = 0; i < ipAddressesList.size(); ++i) {
    if (!ipAddressesList.at(i).isLoopback())
      ui->cbHostName->addItem(ipAddressesList.at(i).toString());
  }
  // add localhost addresses
  for (int i = 0; i < ipAddressesList.size(); ++i) {
    if (ipAddressesList.at(i).isLoopback())
      ui->cbHostName->addItem(ipAddressesList.at(i).toString());
  }

  ui->lePort->setValidator(new QIntValidator(1, 65535, this));
  ui->lePort->setText("502");

  //  m_responseStream.setDevice(m_pTcpSocket);

  ui->leRequest->setFocus();
  ui->leResponse->setDisabled(true);

  ui->pbQuit->setDisabled(true);
  ui->pbSendRequest->setDisabled(true);
  ui->pbSendRequest->setToolTip(
      "Для отправки запроса необходимо подключиться к серверу");

  connect(ui->cbHostName, &QComboBox::editTextChanged, this,
          &ModbusClient::enableSendDataButton);
  connect(ui->lePort, &QLineEdit::textChanged, this,
          &ModbusClient::enableSendDataButton);
  connect(ui->pbSendRequest, &QAbstractButton::clicked, this,
          &ModbusClient::requestNewData);
  connect(m_pTcpSocket, &QTcpSocket::readyRead, this,
          &ModbusClient::readResponse);
  connect(m_pTcpSocket, &QTcpSocket::disconnected, this,
          &ModbusClient::slotDisconnected);
  connect(m_pTcpSocket, &QTcpSocket::errorOccurred, this,
          &ModbusClient::displayError);
}

ModbusClient::~ModbusClient() { delete ui; }

///////////
/// \brief ModbusClient::requestNewData
///
void ModbusClient::requestNewData() {
  QByteArray block;

  // Create dataStream to send data into socket
  QDataStream out(&block, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_12);

  // write data in byteArray through the dataStream
  out << ui->leRequest->text();

  // write data in socket
  m_pTcpSocket->write(block);
}

/////////////////
/// \brief ModbusClient::readResponse
///
void ModbusClient::readResponse() {
  QDataStream response(m_pTcpSocket);

  if (response.status() == QDataStream::Ok) {
    QString nextFortune;
    response >> nextFortune;
    ui->leResponse->setText(nextFortune);
  } else {
    ui->leResponse->setText(QString::number(response.status()));
  }

  ui->pbSendRequest->setEnabled(true);
}

//////////////////
/// \brief ModbusClient::displayError
/// \param socketError
///
void ModbusClient::displayError(QAbstractSocket::SocketError socketError) {
  switch (socketError) {
  case QAbstractSocket::RemoteHostClosedError:
    break;
  case QAbstractSocket::HostNotFoundError:
    QMessageBox::information(this, tr("Fortune Client"),
                             tr("The host was not found. Please check the "
                                "host name and port settings."));
    break;
  case QAbstractSocket::ConnectionRefusedError:
    QMessageBox::information(this, tr("Fortune Client"),
                             tr("The connection was refused by the peer. "
                                "Make sure the fortune server is running, "
                                "and check that the host name and port "
                                "settings are correct."));
    break;
  default:
    QMessageBox::information(this, tr("Fortune Client"),
                             tr("The following error occurred: %1.")
                                 .arg(m_pTcpSocket->errorString()));
  }
  on_pbQuit_clicked();
}

void ModbusClient::enableSendDataButton() {
  ui->pbSendRequest->setEnabled(!ui->cbHostName->currentText().isEmpty() &&
                                !ui->lePort->text().isEmpty());
}

void ModbusClient::on_pbConnect_clicked() {
  if (m_pTcpSocket == nullptr) {
    m_pTcpSocket = new QTcpSocket(this);
    connect(m_pTcpSocket, &QTcpSocket::readyRead, this,
            &ModbusClient::readResponse);
    connect(m_pTcpSocket, &QTcpSocket::disconnected, this,
            &ModbusClient::slotDisconnected);
    connect(m_pTcpSocket, &QTcpSocket::errorOccurred, this,
            &ModbusClient::displayError);
  }
  m_pTcpSocket->connectToHost(ui->cbHostName->currentText(),
                              ui->lePort->text().toInt());
  ui->pbConnect->setDisabled(true);
  ui->pbConnect->setStyleSheet("background-color: rgb(199, 255, 199)");
  ui->pbQuit->setDisabled(false);
  ui->pbSendRequest->setDisabled(false);
  ui->pbSendRequest->setToolTip("Нажмите для отправки запроса серверу");
}

void ModbusClient::on_pbQuit_clicked() {
  m_pTcpSocket->abort();
  ui->pbConnect->setDisabled(false);
  ui->pbConnect->setStyleSheet("background-color: rgb(255, 199, 199);");
  ui->pbQuit->setDisabled(true);
  ui->pbSendRequest->setDisabled(true);
  ui->pbSendRequest->setToolTip(
      "Для отправки запроса необходимо подключиться к серверу");
}

void ModbusClient::slotDisconnected() {
  m_pTcpSocket->deleteLater();
  m_pTcpSocket = nullptr;
}
