#include "ModemCommander.h"
#include <QSerialPort>

#include "Global.h"

ModemCommander::ModemCommander(const QByteArray &servicePort, const qint32 baud, QObject *parent)
    : QObject(parent), _serialPort(new QSerialPort(servicePort, this))
{

  if (!(_serialPort->setBaudRate(baud) && _serialPort->setDataBits(QSerialPort::Data8) &&
        _serialPort->setFlowControl(QSerialPort::HardwareControl) && _serialPort->setParity(QSerialPort::NoParity) &&
        _serialPort->setStopBits(QSerialPort::OneStop)))
    throw global::Exception("Cannot create SerialPort");

  if (!_serialPort->open(QIODevice::ReadWrite))
    throw global::Exception("Cannot open SerialPort: " + servicePort);
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
ModemCommander::~ModemCommander()
{
  _serialPort->close();
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
bool ModemCommander::sendCommand(const QByteArray &ATCommand)
{
  bool isOk = (2 + ATCommand.length() == _serialPort->write(ATCommand + "\r\n")) &&
              _serialPort->waitForBytesWritten() && _serialPort->waitForReadyRead();
  if (!isOk)
    return false;
  QByteArray data = _serialPort->readAll().simplified();
  if (ATCommand != data)
    return false;
  _serialPort->waitForReadyRead();
  data.append(" " + _serialPort->readAll().simplified());
  if (data.isEmpty())
    return false;
  data.replace("\r\n", " ");
  isOk = _SIM7600E_H.modemResponseParser_SIM7600E_H(data);

  if (isOk)
  {
    const auto items = toStringList(_SIM7600E_H.state);
    for (const auto &item : items)
    {
      D(item);
    }
  }
  return true;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
const Modem &ModemCommander::modem() const
{
  return _SIM7600E_H;
}
