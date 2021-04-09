#include "Global.h"
#include <Modem.h>
#include <QSerialPort>
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
bool Modem::initialize()
{
  PF();
  const QList<QByteArray> atCommands = commands();
  D("Commands:" << atCommands);
  if (atCommands.isEmpty())
    return true;

  QSerialPort servicePort(portService());
  if (!(servicePort.setBaudRate(baudRate()) && servicePort.setDataBits(QSerialPort::Data8) &&
        servicePort.setFlowControl(QSerialPort::HardwareControl) && servicePort.setParity(QSerialPort::NoParity) &&
        servicePort.setStopBits(QSerialPort::OneStop)))
    return false;

  if (!servicePort.open(QIODevice::ReadWrite))
  {
    D("Cannot open modem serial port:" << portService());
    return false;
  }

  auto modemCommand = [&servicePort, this](const QByteArray &command) -> bool {
    if (!(servicePort.isOpen() && ((2 + command.length() == servicePort.write(command + "\r\n")) &&
                                   servicePort.waitForBytesWritten() && servicePort.waitForReadyRead())))
      return false;

    QByteArray data = servicePort.readAll().simplified();
    servicePort.waitForReadyRead();
    data.append(" " + servicePort.readAll().simplified());
    if (data.isEmpty())
      return false;
    data.replace("\r\n", " ");
    parseResponse(data, command);
    return true;
  };

  for (const auto &command : atCommands)
  {
    if (!modemCommand(command))
    {
      servicePort.close();
      return false;
    }
  }
  servicePort.close();
  return true;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QByteArray Modem::baseChatConfiguration(const QByteArray &phone, const QString &accessPoint,
                                        const QList<QByteArray> &chatATCommands) const
{
  QByteArray command = "\"";
  command.append("/usr/sbin/chat -v -s -S");
  if (!accessPoint.isEmpty())
    command.append(" -T " + accessPoint.toLatin1());
  command.append(" ABORT 'NO CARRIER'");
  command.append(" ABORT 'NO DIALTONE'");
  command.append(" ABORT ERROR");
  command.append(" ABORT 'NO ANSWER'");
  command.append(" ABORT BUSY");
  command.append(" TIMEOUT 5");
  for (const QByteArray &item : chatATCommands)
    command.append(" " + item);
  command.append(" OK ATD" + phone);
  command.append(" CONNECT \\c");
  command.append('\"');
  return command;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QByteArray Modem::chatConfiguration(const QByteArray &phone, const QString &accessPoint) const
{
  return baseChatConfiguration(phone, accessPoint, QList<QByteArray>());
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QString toString(Modem::State::Network::registration status)
{
  switch (status)
  {
    case Modem::State::Network::registration::NotRegistered: return "not registered";
    case Modem::State::Network::registration::RegisteredHomeNetwork: return "registered, home network";
    case Modem::State::Network::registration::Searching: return "not registered, searching";
    case Modem::State::Network::registration::RegistrationDenied: return "registration denied";
    case Modem::State::Network::registration::Unknown: return "unknown";
    case Modem::State::Network::registration::RegisteredRoaming: return "registered, roaming";
  }
  return QString();
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QString toString(Modem::State::Network::gprs status)
{
  switch (status)
  {
    case Modem::State::Network::gprs::NotRegistered: return "not registered";
    case Modem::State::Network::gprs::RegisteredHomeNetwork: return "registered, home network";
    case Modem::State::Network::gprs::Searching: return "not registered, trying to attach or searching";
    case Modem::State::Network::gprs::RegistrationDenied: return "registration denied";
    case Modem::State::Network::gprs::Unknown: return "unknown";
    case Modem::State::Network::gprs::RegisteredRoaming: return "registered, roaming";
  }
  return QString();
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QStringList toStringList(const Modem::State &state)
{
  return {("Modem Manufacturer: " + state.modem.Manufacturer),
          ("Modem Model: " + state.modem.Model),
          ("Modem Revision: " + state.modem.Revision),
          ("Modem IMEI: " + state.modem.IMEI),
          ("SIM ICCID: " + state.sim.ICCID),
          ("SIM Operator: " + state.sim.Operator),
          ("Registration: " + toString(state.network.Registration)),
          ("GPRS: " + toString(state.network.GPRS)),
          ("Internet PID: " + QString::number(state.internet.PID)),
          ("Internet Interface: " + state.internet.Interface),
          ("Internet LocalAddress: " + state.internet.LocalAddress),
          ("Internet RemoteAddress: " + state.internet.RemoteAddress),
          ("Internet PrimaryDNS: " + state.internet.PrimaryDNS),
          ("Internet SecondaryDNS: " + state.internet.SecondaryDNS)};
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QString toString(Modem::CommandStatus status)
{
  switch (status)
  {
    case Modem::CommandStatus::None: return "None";
    case Modem::CommandStatus::Error: return "Error";
    case Modem::CommandStatus::Success: return "Success";
  }
  return "Unknown";
}
