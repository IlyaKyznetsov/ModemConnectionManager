#include "Global.h"
#include <Modem.h>
#include <QSerialPort>
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
bool Modem::status() const
{
  return state.modem.status && state.sim.status && state.network.status && state.internet.status;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
bool Modem::initialize()
{
  PF();
  const QList<QByteArray> atCommands = commands();
  if (atCommands.isEmpty())
    return true;

  QSerialPort servicePort(portService());
  if (!(servicePort.setBaudRate(baudRate()) && servicePort.setDataBits(QSerialPort::Data8) &&
        servicePort.setFlowControl(QSerialPort::HardwareControl) && servicePort.setParity(QSerialPort::NoParity) &&
        servicePort.setStopBits(QSerialPort::OneStop)))
    return false;
  if (!servicePort.open(QIODevice::ReadWrite))
    return false;

  auto modemCommand = [&servicePort, this](const QByteArray &ATCommand) -> bool {
    if (!(servicePort.isOpen() && ((2 + ATCommand.length() == servicePort.write(ATCommand + "\r\n")) &&
                                   servicePort.waitForBytesWritten() && servicePort.waitForReadyRead())))
      return false;

    QByteArray data = servicePort.readAll().simplified();
    if (ATCommand != data)
      return false;
    servicePort.waitForReadyRead();
    data.append(" " + servicePort.readAll().simplified());
    if (data.isEmpty())
      return false;
    data.replace("\r\n", " ");
    parseResponse(data);
    return true;
  };

  for (const auto &command : atCommands)
  {
    D("AT command:"<<command);
    if (!modemCommand(command) || !status())
    {
      servicePort.close();
      return false;
    }
  }
  servicePort.close();
  return true;
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
QByteArray Modem::chatConfiguration(const QByteArray &phone, const QString &accessPoint) const
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
  command.append(" OK-AT-OK ATZ");
  command.append(" OK-AT-OK ATI");
  command.append(" OK-AT-OK AT+CICCID");
  command.append(" OK-AT-OK AT+CSPN?");
  command.append(" OK-AT-OK AT+CREG?");
  command.append(" OK-AT-OK AT+CGREG?");
  command.append(" OK ATD" + phone);
  command.append(" CONNECT \\c");
  command.append('\"');
  return command;
}
