#include "modbustcpclient.h"
#include "ui_modbustcpclient.h"

#include <QtNetwork/QHostInfo>
#include <QtNetwork/QNetworkInterface>

ModbusClient::ModbusClient(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::ModbusClient),
      m_pTcpSocket(new QTcpSocket(this)), protocolID(0) {
  ui->setupUi(this);

  setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

  transID = 0;

  m_cln = 0;
  m_row = 4;
  m_commandSize = 4;

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

  ui->sbFuncCode->setMaximum(UINT16_MAX);

  for (int i = 0; i < 4; i++) {
    QSpinBox *sb =
        this->findChild<QSpinBox *>("sbCommand" + QString::number(i));
    if (sb)
      sb->setMaximum(UINT16_MAX);
  }

  //  m_responseStream.setDevice(m_pTcpSocket);

  //  ui->leRequest->setFocus();
  //  ui->leResponse->setDisabled(true);

  ui->pbQuit->setDisabled(true);
  ui->pbSendRequest->setDisabled(true);
  ui->pbSendRequest->setToolTip(
      "Для отправки запроса необходимо подключиться к серверу");

  ui->rbUi->toggle();

  connect(ui->cbHostName, &QComboBox::editTextChanged, this,
          &ModbusClient::enableSendDataButton);
  connect(ui->lePort, &QLineEdit::textChanged, this,
          &ModbusClient::enableSendDataButton);
  connect(ui->pbSendRequest, &QAbstractButton::clicked, this,
          &ModbusClient::requestNewData);
  connect(m_pTcpSocket, &QTcpSocket::readyRead, this,
          &ModbusClient::readResponse);
  connect(m_pTcpSocket, &QTcpSocket::connected, this,
          &ModbusClient::slotConnected);
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
  // Increment trans ID
  transID++;

  QByteArray block;
  block.clear();

  // Create dataStream to send data into socket
  QDataStream out(&block, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_12);

  // set unitId, FuncCode
  out << uint16_t(transID) << uint16_t(protocolID) << qint16(0)
      << uint8_t(ui->sbUnitId->value()) << uint16_t(ui->sbFuncCode->value());
  // set command
  for (int i = 0; i < m_commandSize; i++) {
    QSpinBox *sb =
        this->findChild<QSpinBox *>("sbCommand" + QString::number(i));
    if (sb) {
      out << uint16_t(sb->value());
    }
  }
  // set commandSize
  out.device()->seek(4);
  out << quint16(block.size() - sizeof(qint16) * 3);

  // write data in socket
  m_pTcpSocket->write(block);
}

/////////////////
/// \brief ModbusClient::readResponse
///
void ModbusClient::readResponse() {
  QDataStream response(m_pTcpSocket);

  if (response.status() == QDataStream::Ok) {
    //    QString nextFortune;
    //    response >> nextFortune;
    //    ui->leResponse->setText(nextFortune);

    for (;;) {
      if (m_serverMessageSize == 0) {
        if (m_pTcpSocket->bytesAvailable() < 2) {
          break;
        }
        response >> m_serverMessageSize;
      }
      if (m_pTcpSocket->bytesAvailable() < m_serverMessageSize) {
        break;
      }
      QString str;
      response >> str;
      ui->leResponse->setText(str);

      m_serverMessageSize = 0;
      break;
    }

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
    QMessageBox::information(this, tr("ModBus client"),
                             tr("The host was not found. Please check the "
                                "host name and port settings."));
    break;
  case QAbstractSocket::ConnectionRefusedError:
    QMessageBox::information(this, tr("ModBus client"),
                             tr("The connection was refused by the peer. "
                                "Make sure the server is running, "
                                "and check that the host name and port "
                                "settings are correct."));
    break;
  default:
    QMessageBox::information(this, tr("ModBus client"),
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
    connect(m_pTcpSocket, &QTcpSocket::connected, this,
            &ModbusClient::slotConnected);
    connect(m_pTcpSocket, &QTcpSocket::disconnected, this,
            &ModbusClient::slotDisconnected);
    connect(m_pTcpSocket, &QTcpSocket::errorOccurred, this,
            &ModbusClient::displayError);
  }
  m_pTcpSocket->connectToHost(ui->cbHostName->currentText(),
                              ui->lePort->text().toInt());
  ui->pbConnect->setDisabled(true);
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

void ModbusClient::slotConnected() {
  ui->pbConnect->setStyleSheet("background-color: rgb(199, 255, 199)");
  ui->pbQuit->setDisabled(false);
  ui->pbSendRequest->setDisabled(false);
}

void ModbusClient::slotDisconnected() {
  m_pTcpSocket->deleteLater();
  m_pTcpSocket = nullptr;
}

void ModbusClient::on_pbAddByte_clicked() {
  if (m_commandSize < 126) {
    QSpinBox *sb = new QSpinBox(this);

    sb->setMaximum(UINT16_MAX);
    sb->setObjectName("sbCommand" + QString::number(m_commandSize));
    ui->glReqiestContainer->addWidget(sb, m_row, m_cln);

    m_commandSize = m_commandSize + 1;
    ui->lbCommand->setText(
        QString("Команда (%1/252  байт):").arg(m_commandSize * 2));

    m_cln = m_cln + 1;
    if (m_cln == 4) {
      m_row = m_row + 1;
      m_cln = 0;
    }
  }
}

void ModbusClient::on_pbDelByte_clicked() {
  if (m_commandSize != 0) {
    QSpinBox *sb = this->findChild<QSpinBox *>(
        "sbCommand" + QString::number(m_commandSize - 1));
    if (sb) {
      sb->deleteLater();
      m_commandSize = m_commandSize - 1;

      ui->lbCommand->setText(
          QString("Команда (%1/252  байт):").arg(m_commandSize * 2));

      if (m_cln == 0) {
        m_cln = 3;
        m_row = m_row - 1;
      } else {
        m_cln = m_cln - 1;
      }
    }
  }
}

void ModbusClient::on_rbUi_toggled(bool checked) {
  if (checked) {
    ui->formGroupBox->setDisabled(false);
    ui->gridGroupBox->setDisabled(true);
  } else {
    ui->formGroupBox->setDisabled(true);
    ui->gridGroupBox->setDisabled(false);
  }
}

void ModbusClient::on_rbCustom_toggled(bool checked) {
  if (checked) {
    ui->gridGroupBox->setDisabled(false);
    ui->formGroupBox->setDisabled(true);
  } else {
    ui->gridGroupBox->setDisabled(true);
    ui->formGroupBox->setDisabled(false);
  }
}
