#include "Global.h"
#include <Modem.h>
#include <QProcess>
#include <QRegularExpression>

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
bool Modem::modemReset_SIM7600E_H()
{
  QProcess reset;
  reset.start("/usr/sbin/i2cset -f -y -m 0x40 0 0x20 1 0x40");
  bool isOk = reset.waitForStarted();
  if (!isOk)
    return false;
  isOk = reset.waitForFinished();
  if (!isOk)
    return false;
  QThread::usleep(13250);
  reset.start("/usr/sbin/i2cset -f -y -m 0x40 0 0x20 1 0x0");
  if (!isOk)
    return false;
  isOk = reset.waitForFinished();
  if (!isOk)
    return false;
  QThread::usleep(13250);
  return true;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
bool Modem::modemResponseParser_SIM7600E_H(const QByteArray &data)
{
  int index = data.indexOf(' ');
  if (-1 == index)
    return false;
  //  DF(data);
  const QByteArray &cmd = data.left(index);

  if ("ATI" == cmd)
  {
    QRegularExpression rx("Manufacturer: (.*) Model: (.*) Revision: (.*) IMEI: (.*) \\+GCAP");
    QRegularExpressionMatch match = rx.match(data);
    if (!match.isValid() || !match.hasMatch())
      return false;
    state.modem.Manufacturer = match.captured(1);
    state.modem.Model = match.captured(2);
    state.modem.Revision = match.captured(3);
    state.modem.IMEI = match.captured(4);
    return true;
  }
  else if ("AT+CICCID" == cmd)
  {
    QRegularExpression rx("\\+ICCID: (.*)  OK");
    QRegularExpressionMatch match = rx.match(data);
    if (!match.isValid() || !match.hasMatch())
    {
      rx.setPattern("(\\+CME ERROR: (.*)|ERROR)");
      QRegularExpressionMatch match = rx.match(data);
      if (!match.isValid() || !match.hasMatch())
        return false;
      state.sim.status = false;
      state.sim.ICCID = match.captured(2);
      return true;
    }
    state.sim.ICCID = match.captured(1);
    return true;
  }
  else if ("AT+CSPN?" == cmd)
  {
    QRegularExpression rx("\\+CSPN: \"(.*)\",[0-1]  OK");
    QRegularExpressionMatch match = rx.match(data);
    if (!match.isValid() || !match.hasMatch())
    {
      rx.setPattern("(\\+CME ERROR: (.*)|ERROR)");
      QRegularExpressionMatch match = rx.match(data);
      if (!match.isValid() || !match.hasMatch())
        return false;
      state.sim.status = false;
      state.sim.Operator = match.captured(2);
      return true;
    }
    state.sim.Operator = match.captured(1);
    return true;
  }
  else if ("AT+CREG?" == cmd)
  {
    QRegularExpression rx("\\+CREG: ([0-9],[0-9])  OK");
    QRegularExpressionMatch match = rx.match(data);
    if (!match.isValid() || !match.hasMatch())
      return false;
    state.network.Registration = State::Network::registration(match.captured(1).right(1).toInt());
    return true;
  }
  else if ("AT+CGREG?" == cmd)
  {
    QRegularExpression rx("\\+CGREG: ([0-9],[0-9])  OK");
    QRegularExpressionMatch match = rx.match(data);
    if (!match.isValid() || !match.hasMatch())
      return false;
    state.network.GPRS = State::Network::gprs(match.captured(1).right(1).toInt());
    return true;
  }
  return false;
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
    case Modem::State::Network::gprs::Searching:
      return "not registered, trying to attach or searching";
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
