#include "modbusclient.h"
#include "ui_modbusclient.h"

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

  m_responseStream.setDevice(m_pTcpSocket);
  m_responseStream.setVersion(QDataStream::Qt_5_15);

  connect(ui->cbHostName, &QComboBox::editTextChanged, this,
          &ModbusClient::enableSendDataButton);
  connect(ui->lePort, &QLineEdit::textChanged, this,
          &ModbusClient::enableSendDataButton);
  connect(ui->pbSendRequest, &QAbstractButton::clicked, this,
          &ModbusClient::requestNewData);
  connect(ui->pbQuit, &QAbstractButton::clicked, this, &QWidget::close);
  connect(m_pTcpSocket, &QIODevice::readyRead, this,
          &ModbusClient::readResponse);
  connect(m_pTcpSocket, &QAbstractSocket::errorOccurred, this,
          &ModbusClient::displayError);

  ui->lePort->setFocus();
}

ModbusClient::~ModbusClient() { delete ui; }

void ModbusClient::requestNewData() {
  QByteArray block;

  QString request = "1237";

  QDataStream out(&block, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_10);

  out << request;

  ui->pbSendRequest->setEnabled(false);
  m_pTcpSocket->abort();
  m_pTcpSocket->write(block);
  m_pTcpSocket->connectToHost(ui->cbHostName->currentText(),
                              ui->lePort->text().toInt());
}

void ModbusClient::readResponse() {
  m_responseStream.startTransaction();

  QString nextFortune;
  m_responseStream >> nextFortune;

  if (!m_responseStream.commitTransaction())
    return;

  if (nextFortune == m_response) {
    QTimer::singleShot(0, this, &ModbusClient::requestNewData);
    return;
  }

  m_response = nextFortune;
  ui->lbResponse->setText(m_response);
  ui->pbSendRequest->setEnabled(true);
}

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

  ui->pbSendRequest->setEnabled(true);
}

void ModbusClient::enableSendDataButton() {
  ui->pbSendRequest->setEnabled(!ui->cbHostName->currentText().isEmpty() &&
                                !ui->lePort->text().isEmpty());
}
