#include "SIM7600E_H.h"
#include <QProcess>
#include <QRegularExpression>

SIM7600E_H::SIM7600E_H()
{
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
bool SIM7600E_H::reset()
{
  QProcess reset;
  reset.start("/usr/sbin/i2cset -f -y -m 0x40 0 0x20 1 0x40");
  bool isOk = reset.waitForStarted();
  if (!isOk)
    return false;
  isOk = reset.waitForFinished();
  if (!isOk)
    return false;
  //    QThread::usleep(13250);
  reset.start("/usr/sbin/i2cset -f -y -m 0x40 0 0x20 1 0x0");
  if (!isOk)
    return false;
  isOk = reset.waitForFinished();
  if (!isOk)
    return false;
  //    QThread::usleep(13250);
  return true;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
bool SIM7600E_H::parseResponse(const QByteArray &data)
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
    state.network.Registration = State::Network::registration(match.captured(1).rightRef(1).toInt());
    return true;
  }
  else if ("AT+CGREG?" == cmd)
  {
    QRegularExpression rx("\\+CGREG: ([0-9],[0-9])  OK");
    QRegularExpressionMatch match = rx.match(data);
    if (!match.isValid() || !match.hasMatch())
      return false;
    state.network.GPRS = State::Network::gprs(match.captured(1).rightRef(1).toInt());
    return true;
  }
  return false;
}
