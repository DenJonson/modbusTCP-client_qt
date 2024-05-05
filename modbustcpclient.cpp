#include <QScreen>
#include <QtNetwork/QHostInfo>
#include <QtNetwork/QNetworkInterface>

#include "modbustcpclient.h"
#include "ui_modbustcpclient.h"

ModbusClient::ModbusClient(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::ModbusClient),
      m_pTcpSocket(new QTcpSocket(this)), m_protocolID(0) {
  ui->setupUi(this);

  setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

  m_transID = 0;

  m_cln = 0;
  m_row = 4;
  m_commandSize = 4;

  ui->formGroupBox_4->setHidden(true);
  ui->formGroupBox_3->setHidden(true);
  ui->formGroupBox_2->setHidden(true);
  ui->groupBox->setHidden(true);
  ui->label_20->setHidden(true);
  ui->sbUnitId->setHidden(true);

  QScreen *screen = QGuiApplication::primaryScreen();
  setGeometry(0, 0, 0, 0);
  move(screen->geometry().center().x() - (520 / 2),
       screen->geometry().center().y() - (467 / 2));

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

  ui->leUnitIdOut->setInputMask(UI_INPUT_MASK_8);
  ui->leUnitIdOut->setText("00");
  ui->leFuncCode->setInputMask(UI_INPUT_MASK_8);
  ui->leFuncCode->setText("00");

  ui->leCommand0->setInputMask(UI_INPUT_MASK_16);
  ui->leCommand0->setText("0000");
  ui->leCommand1->setInputMask(UI_INPUT_MASK_16);
  ui->leCommand1->setText("0000");
  ui->leCommand2->setInputMask(UI_INPUT_MASK_16);
  ui->leCommand2->setText("0000");
  ui->leCommand3->setInputMask(UI_INPUT_MASK_16);
  ui->leCommand3->setText("0000");

  for (int i = 0; i < 4; i++) {
    QSpinBox *sb =
        this->findChild<QSpinBox *>("sbCommand" + QString::number(i));
    if (sb)
      sb->setMaximum(UINT16_MAX);
  }

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
  m_transID++;

  QByteArray block;
  block.clear();

  // Create dataStream to send data into socket
  QDataStream out(&block, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_12);

  uint8_t unitId = ui->leUnitIdOut->text().toInt(nullptr, 16);

  // set unitId, FuncCode
  out << uint16_t(m_transID) << uint16_t(m_protocolID) << qint16(0) << unitId
      << uint8_t(ui->leFuncCode->text().toInt(nullptr, 16));
  // set command
  for (int i = 0; i < m_commandSize; i++) {
    QLineEdit *le =
        this->findChild<QLineEdit *>("leCommand" + QString::number(i));
    if (le) {
      bool ok;
      uint16_t commPart = le->text().split(" ").join("").toUInt(&ok, 16);
      out << uint16_t(commPart);
    }
  }
  // set commandSize
  out.device()->seek(4);
  out << quint16(block.size() + 2 - sizeof(qint16) * 3);

  qint16 CRC = qChecksum(block, block.size());

  uint8_t buff[sizeof(qint16)];
  memcpy(&buff, &CRC, sizeof(qint16));

  for (uint64_t i = 0; i < sizeof(qint16); i++) {
    block.append(buff[i]);
  }

  // write data in socket
  m_pTcpSocket->write(block);
}

/////////////////
/// \brief ModbusClient::readResponse
///
void ModbusClient::readResponse() {
  mbTcpInitTrans_t initData;
  initData.transLen = 0;
  initData.protocolId = 0;
  initData.transId = 0;

  QTcpSocket *socket = (QTcpSocket *)sender();
  QDataStream response(socket);

  if (response.status() == QDataStream::Ok) {
    for (;;) {
      if (initData.transLen == 0) {
        if (m_pTcpSocket->bytesAvailable() < 6) {
          break;
        }
        response >> initData.transId;
        response >> initData.protocolId;
        response >> initData.transLen;
      }
      if (m_pTcpSocket->bytesAvailable() < initData.transLen) {
        qDebug() << m_pTcpSocket->bytesAvailable();
        break;
      }

      QByteArray message;
      QString str;
      int size = m_pTcpSocket->bytesAvailable();
      for (int i = 0; i < size; i++) {
        char byte;
        m_pTcpSocket->read(&byte, sizeof(char));
        message.append(uint8_t(byte));
      }

      str += convertStrToCompleteHex(
          QString::number(initData.transId, 16).toUpper(), sizeof(uint16_t));
      str += convertStrToCompleteHex(
          QString::number(initData.protocolId, 16).toUpper(), sizeof(uint16_t));
      str += convertStrToCompleteHex(
          QString::number(initData.transLen, 16).toUpper(), sizeof(uint16_t));

      foreach (char ch, message) {
        str += convertStrToCompleteHex(
            QString::number(uint8_t(ch), 16).toUpper(), sizeof(char));
      }
      QString mask = ">HH ";
      for (int i = 2; i <= str.size() / 2; i++) {
        mask += "HH ";
      }
      ui->leResponse->setInputMask(mask);
      ui->leResponse->setText(str);

      m_serverMessageSize = 0;
      break;
    }

  } else {
    ui->leResponse->setText(QString::number(response.status()));
  }

  ui->pbSendRequest->setEnabled(true);
}

QString ModbusClient::convertStrToCompleteHex(QString str, int size) {
  QString complete;
  if (str.size() < size * 2) {
    int iter = size - str.size() / 2;
    for (int i = 0; i < iter; i++) {
      if (i == iter - 1) {
        if (str.size() % 2 > 0) {
          complete += "0" + str;
        } else {
          complete += str;
        }
      } else {
        complete += "00";
      }
    }
    return complete;
  }
  return str;
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
    QLineEdit *le = new QLineEdit(this);

    le->setInputMask(UI_INPUT_MASK_16);
    le->setText("0000");
    le->setObjectName("leCommand" + QString::number(m_commandSize));
    ui->glReqiestContainer->addWidget(le, m_row, m_cln);

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
    QLineEdit *le = this->findChild<QLineEdit *>(
        "leCommand" + QString::number(m_commandSize - 1));
    if (le) {
      le->deleteLater();
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
