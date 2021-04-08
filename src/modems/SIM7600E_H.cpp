#include "SIM7600E_H.h"
#include <Global.h>
#include <QProcess>
#include <QRegularExpression>
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QByteArray SIM7600E_H::name() const
{
  return "SIM7600E_H";
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
bool SIM7600E_H::reset()
{
  PF();
  QProcess::execute("/usr/sbin/i2cset -f -y -m 0x40 0 0x20 1 0x40");
  QThread::usleep(13250);
  QProcess::execute("/usr/sbin/i2cset -f -y -m 0x40 0 0x20 1 0x0");
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QList<QByteArray> SIM7600E_H::commands() const
{
  QList<QByteArray> res;
  res.append("ATI");
  res.append("AT+CICCID");
  res.append("AT+CSPN?");
  return res;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
bool SIM7600E_H::parseResponse(const QByteArray &data)
{
  int index = data.indexOf(' ');
  if (-1 == index)
    return false;

  DF(data);
  const QByteArray &cmd = data.left(index);

  if ("ATI" == cmd)
  {
    QRegularExpression rx("Manufacturer:\\s+(.*)\\s+Model:\\s+(.*)\\s+Revision:\\s+(.*)\\s+IMEI:\\s+(.*)\\s+\\+GCAP");
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
    QRegularExpression rx("\\+ICCID:\\s+(.*)\\s+OK");
    QRegularExpressionMatch match = rx.match(data);
    if (!match.isValid() || !match.hasMatch())
    {
      rx.setPattern("(\\+CME\\s+ERROR:\\s+(.*)|ERROR)");
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
    QRegularExpression rx("\\+CSPN:\\s+\"(.*)\",[0-1]\\s+OK");
    QRegularExpressionMatch match = rx.match(data);
    if (!match.isValid() || !match.hasMatch())
    {
      rx.setPattern("(\\+CME\\s+ERROR:\\s+(.*)|ERROR)");
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
    QRegularExpression rx("\\+CREG:\\s+([0-9],[0-9])\\s+OK");
    QRegularExpressionMatch match = rx.match(data);
    if (!match.isValid() || !match.hasMatch())
      return false;
    state.network.Registration = State::Network::registration(match.captured(1).rightRef(1).toInt());
    return true;
  }
  else if ("AT+CGREG?" == cmd)
  {
    QRegularExpression rx("\\+CGREG:\\s+([0-9],[0-9])\\s+OK");
    QRegularExpressionMatch match = rx.match(data);
    if (!match.isValid() || !match.hasMatch())
      return false;
    state.network.GPRS = State::Network::gprs(match.captured(1).rightRef(1).toInt());
    return true;
  }
  return false;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
quint32 SIM7600E_H::baudRate() const
{
  return 115200;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QByteArray SIM7600E_H::portConnection() const
{
  return "/dev/ttyUSB2";
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QByteArray SIM7600E_H::portService() const
{
  return "/dev/ttyUSB3";
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QByteArray SIM7600E_H::portGps() const
{
  return "/dev/ttyUSB4";
}
