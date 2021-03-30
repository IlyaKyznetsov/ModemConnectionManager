#include "ModemConnectionManager.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QTimer>
#include <QtSerialPort/QSerialPort>

#include <signal.h>

#include "Global.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
static void pppdterm()
{
  const QStringList procs = QDir("/proc").entryList({}, QDir::Dirs).filter(QRegularExpression("^[0-9]+$"));
  for (const QString &item : procs)
  {
    bool isPid;
    QFile comm("/proc/" + item + "/comm");
    pid_t pid = item.toInt(&isPid);
    if (comm.exists() && isPid)
    {
      comm.open(QIODevice::ReadOnly);
      QString processName = comm.readAll().simplified();
      comm.close();
      if (processName == "pppd")
        kill(pid, SIGTERM);
    }
  }
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QString errorString(const int pppdCode)
{
  switch (pppdCode)
  {

    case 0:
      return "Pppd has detached, or otherwise the connection was successfully established and terminated at the "
             "peer's request.";
    case 1:
      return "An immediately fatal error of some kind occurred, such as an essential system call failing, or "
             "running out of virtual memory.";
    case 2:
      return "An error was detected in processing the options given, such as two mutually exclusive options being "
             "used.";
    case 3: return "Pppd is not setuid-root and the invoking user is not root.";
    case 4:
      return "The kernel does not support PPP, for example, the PPP kernel driver is not included or cannot be loaded.";
    case 5: return "Pppd terminated because it was sent a SIGINT, SIGTERM or SIGHUP signal.";
    case 6: return "The serial port could not be locked.";
    case 7: return "The serial port could not be opened.";
    case 8: return "The connect script failed (returned a non-zero exit status).";
    case 9: return "The command specified as the argument to the pty option could not be run.";
    case 10:
      return "The PPP negotiation failed, that is, it didn't reach the point where at least one network protocol "
             "(e.g. IP) was running.";
    case 11: return "The peer system failed (or refused) to authenticate itself.";
    case 12: return "The link was established successfully and terminated because it was idle.";
    case 13: return "The link was established successfully and terminated because the connect time limit was reached.";
    case 14: return "Callback was negotiated and an incoming call should arrive shortly.";
    case 15: return "The link was terminated because the peer is not responding to echo requests.";
    case 16: return "The link was terminated by the modem hanging up.";
    case 17: return "The PPP negotiation failed because serial loopback was detected.";
    case 18: return "The init script failed (returned a non-zero exit status).";
    case 19: return "We failed to authenticate ourselves to the peer.";
  }
  return "Unknown error";
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
static bool readSettings(QIODevice &device, QSettings::SettingsMap &map)
{
  PF();
  QString group;
  for (QByteArray line; !(line = device.readLine().simplified()).isEmpty();)
  {
    if (*line.begin() == '#')
      continue;

    if (*line.begin() == '[' && *(line.end() - 1) == ']')
    {
      group = line.mid(1, line.length() - 2);
      continue;
    }

    if ("Modem Configuration" == group || "Connection Settings" == group)
    {
      int pos = line.indexOf('=');
      if (-1 != pos)
        map.insert(group + '/' + line.left(pos).simplified(), line.mid(pos + 1).simplified());
    }
    else if ("Pppd Settings" == group)
    {
      const QString key = group + "/options";
      if (!map.contains(key))
        map.insert(key, QStringList());
      QStringList value = map.value(key).toStringList();
      value.append(line);
      map.insert(key, value);
    }
  }
  return true;
}
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
inline void clearState(Modem::State &state, bool all)
{
  state.internet.PID = 0;
  if (state.network.Registration != Modem::State::Network::registration::NotRegistered)
    state.network.Registration = Modem::State::Network::registration::Unknown;
  if (state.network.GPRS != Modem::State::Network::gprs::NotRegistered)
    state.network.GPRS = Modem::State::Network::gprs::Unknown;
  state.internet.Interface = state.internet.LocalAddress = state.internet.RemoteAddress = state.internet.PrimaryDNS =
      state.internet.SecondaryDNS = QString();
  if (all)
  {
    state.network.Registration = Modem::State::Network::registration::NotRegistered;
    state.network.GPRS = Modem::State::Network::gprs::NotRegistered;
    state.modem.Manufacturer = state.modem.Model = state.modem.Revision = state.modem.IMEI = state.sim.ICCID =
        state.sim.Operator = QString();
  }
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
Modem::State ModemConnectionManager::state() const
{
  return _modem.state;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
ModemConnectionManager::ModemConnectionManager(const QString &path, QObject *parent) : QObject(parent)
{
  DF(path);
  qRegisterMetaType<Modem::State>("Modem::State");

  pppdterm();

  _pppdCommand = "/usr/sbin/pppd";
  auto pppdCommandAppend = [this](const QByteArray &data) { _pppdCommand += ' ' + data; };

  // Read configuration
  QSettings::Format format = QSettings::registerFormat("ModemConnectionManager", readSettings, nullptr);
  const QString rpath = (!path.isEmpty() && QFile::exists(path) ? path : "://ModemConnectionManager.conf");
  QSettings settings(rpath, format, this);
  // settings.setIniCodec("UTF-8");

  settings.beginGroup("Modem Configuration");
  const QByteArray modem = settings.value("Modem").toByteArray();
  D(modem);
  if (modem.isEmpty())
    throw global::Exception("[Modem Configuration] Modem not exist");
  pppdCommandAppend(modem.mid(modem.lastIndexOf('/') + 1));

  const QByteArray baud = settings.value("Baud").toByteArray();
  if (baud.isEmpty())
    throw global::Exception("[Modem Configuration] Baud not exist");
  pppdCommandAppend(baud);
  _modemResetCommand = settings.value("Reset").toByteArray();
  if (_modemResetCommand.isEmpty())
  {
    QFile file("://ModemHardRestarting.sh");
    file.open(QIODevice::ReadOnly);
    const QByteArray data = file.readAll();
    file.close();
    file.setFileName("/tmp/ModemHardRestarting.sh");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
      throw global::Exception("Not exist Modem hard restarting command");
    file.write(data);
    file.setPermissions(QFileDevice::ReadUser | QFileDevice::WriteUser | QFileDevice::ExeUser | QFileDevice::ReadGroup |
                        QFileDevice::ExeGroup | QFileDevice::ExeOther);
    file.close();
    _modemResetCommand = "/tmp/ModemHardRestarting.sh";
  }
  settings.endGroup();
  settings.beginGroup("Pppd Settings");
  const QStringList options = settings.value("options").toStringList();
  for (const QString &item : options)
    pppdCommandAppend(item.toLatin1());
  settings.endGroup();
  settings.beginGroup("Connection Settings");
  _reconnectionHope = _connectionHopes = settings.value("ResetConnectionHopes", 0).toInt();
  const int reconnectionTimeout = settings.value("ReconnectTimeout", 0).toInt();
  const QByteArray phone = settings.value("Phone").toByteArray();
  if (phone.isEmpty())
    throw global::Exception("[Connection Settings] Phone not exist");
  const QByteArray user = settings.value("User").toByteArray();
  const QByteArray password = settings.value("Password").toByteArray();
  const QByteArray accessPoint = settings.value("AccessPoint").toByteArray();
  settings.endGroup();
  pppdCommandAppend("connect");
  pppdCommandAppend(modemChatConfiguration_SIM7600E_H(phone, accessPoint));

  // Create secrets:
  const int count = 2;
  const QString files[count] = {"/etc/ppp/chap-secrets", "/etc/ppp/pap-secrets"};

  if (!user.isEmpty() && !password.isEmpty())
  {
    pppdCommandAppend("user");
    pppdCommandAppend(user);
    for (int i = 0; i != count; ++i)
    {
      QFile file(files[i]);
      if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        throw global::Exception("Cannot write secrets files");
      file.write(user + '\t' + '*' + '\t' + password);
      file.setPermissions(QFileDevice::ReadUser | QFileDevice::WriteUser);
      file.close();
    }
  }

  if (_connectionHopes > 0 && reconnectionTimeout > 0)
  {
    _reconnectionTimer = new QTimer(this);
    if (!_reconnectionTimer)
      throw global::Exception("Cannot create reconnection timer");
    _reconnectionTimer->setSingleShot(true);
    _reconnectionTimer->setInterval(reconnectionTimeout * 1000);
    connect(_reconnectionTimer, &QTimer::timeout, this, &ModemConnectionManager::connection);
  }

  _pppd = new QProcess(this);
  if (!_pppd)
    throw global::Exception("Cannot create pppd");
  _pppd->setProcessChannelMode(QProcess::MergedChannels);
  connect(_pppd, &QProcess::readyReadStandardOutput, this, &ModemConnectionManager::_pppdOutput);
  connect(_pppd, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
          &ModemConnectionManager::_pppdFinished);
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QByteArray ModemConnectionManager::modemChatConfiguration_SIM7600E_H(const QByteArray &phone,
                                                                     const QString &accessPoint) const
{
  QByteArray command = "\"";
  command.append("/usr/sbin/chat -v -s -S");
  if (!accessPoint.isEmpty())
    command.append(" -T " + accessPoint);
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

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
ModemConnectionManager::~ModemConnectionManager()
{
  PF();
  if (_reconnectionTimer)
  {
    _reconnectionTimer->disconnect(this);
    if (_reconnectionTimer->isActive())
      _reconnectionTimer->stop();
  }

  _pppd->disconnect(this);
  _pppd->terminate();
  if (!_pppd->waitForFinished())
    _pppd->kill();
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
bool ModemConnectionManager::connection()
{
  DF(_pppd->program());
  disconnection();
  if (_reconnectionTimer && _pppd->signalsBlocked())
    _pppd->blockSignals(false);
  _pppd->start(_pppdCommand);
  bool isStarted = _pppd->waitForStarted(5000);
  const int pid = (isStarted ? _pppd->processId() : -1);
  if (pid != _modem.state.internet.PID)
  {
    _modem.state.internet.PID = pid;
    emit stateChanged(_modem.state);
  }
  if (_connectionHopes)
  {
    if (!_reconnectionHope)
      modemHardReset();
    else
      --_reconnectionHope;

    D("ConnectionHope:" << _reconnectionHope);
  }
  return isStarted;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void ModemConnectionManager::disconnection()
{
  PF();

  if (_reconnectionTimer)
  {
    if (!_pppd->signalsBlocked())
      _pppd->blockSignals(true);

    if (_reconnectionTimer->isActive())
      _reconnectionTimer->stop();
  }

  if (QProcess::NotRunning != _pppd->state())
  {
    _pppd->terminate();
    if (!_pppd->waitForFinished())
      throw global::Exception("Cannot terminate pppd");
  }

  clearState(_modem.state, false);
  emit stateChanged(_modem.state);
}

bool ModemConnectionManager::modemHardReset()
{
  PF();
  if (_connectionHopes)
    _reconnectionHope = _connectionHopes;

  QProcess process;
  process.start(_modemResetCommand, QStringList());
  bool isOk = process.waitForStarted(5000);
  if (isOk)
  {
    clearState(_modem.state, true);
    emit stateChanged(_modem.state);
    isOk = process.waitForFinished();
  }
  return isOk;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void ModemConnectionManager::_pppdOutput()
{
  PF();

  QByteArray data = _pppd->readAllStandardOutput().simplified();
  if (data.isEmpty())
    return;
  data.replace("^M", "");

  D(data);

  // Parse AT Commands responses
  bool isStateChanged = modemResponseParser_SIM7600E_H(data);

  // Parse Pppd output
  QRegularExpression rx("Using interface (\\S+) ");
  QRegularExpressionMatch match = rx.match(data);
  match = rx.match(data);
  if (match.isValid() && match.hasMatch())
  {
    _modem.state.internet.Interface = match.captured(1);
    isStateChanged = true;
  }

  const QString ip4 =
      "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4]["
      "0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)";

  rx.setPattern(QString("remote IP address (%1)").arg(ip4));
  match = rx.match(data);
  if (match.isValid() && match.hasMatch())
  {
    _modem.state.internet.RemoteAddress = match.captured(1);
    isStateChanged = true;
  }

  rx.setPattern(QString("local IP address (%1)").arg(ip4));
  match = rx.match(data);
  if (match.isValid() && match.hasMatch())
  {
    _modem.state.internet.LocalAddress = match.captured(1);
    isStateChanged = true;
  }

  rx.setPattern(QString("primary DNS address (%1)").arg(ip4));
  match = rx.match(data);
  if (match.isValid() && match.hasMatch())
  {
    _modem.state.internet.PrimaryDNS = match.captured(1);
    isStateChanged = true;
  }

  rx.setPattern(QString("secondary DNS address (%1)").arg(ip4));
  match = rx.match(data);
  if (match.isValid() && match.hasMatch())
  {
    _modem.state.internet.SecondaryDNS = match.captured(1);
    isStateChanged = true;
  }

  if (isStateChanged)
    emit stateChanged(_modem.state);

  return;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void ModemConnectionManager::_pppdFinished(int exitCode, int exitStatus)
{
  Q_UNUSED(exitStatus);
  //  const QProcess::ExitStatus status = QProcess::ExitStatus(exitStatus);
  //  if (6 == exitCode)
  //    modemHardReset();

  clearState(_modem.state, false);
  emit stateChanged(_modem.state);

  DF(exitCode << errorString(exitCode));
  if (_reconnectionTimer && _pppd)
  {
    D("Try reconnect after: " << _reconnectionTimer->interval() / 1000 << "secs");
    _reconnectionTimer->start();
  }
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
bool ModemConnectionManager::modemResponseParser_SIM7600E_H(const QByteArray &data)
{
  int index = data.indexOf(' ');
  if (-1 == index)
    return false;
  const QByteArray &cmd = data.left(index);
  if ("ATI" == cmd)
  {
    QRegularExpression rx("Manufacturer: (.*) Model: (.*) Revision: (.*) IMEI: (.*) \\+GCAP");
    QRegularExpressionMatch match = rx.match(data);
    if (!match.isValid() || !match.hasMatch())
      return false;
    _modem.state.modem.Manufacturer = match.captured(1);
    _modem.state.modem.Model = match.captured(2);
    _modem.state.modem.Revision = match.captured(3);
    _modem.state.modem.IMEI = match.captured(4);
    return true;
  }
  else if ("AT+CICCID" == cmd)
  {
    QRegularExpression rx("\\+ICCID: (.*)  OK");
    QRegularExpressionMatch match = rx.match(data);
    if (!match.isValid() || !match.hasMatch())
      return false;
    _modem.state.sim.ICCID = match.captured(1);
    return true;
  }
  else if ("AT+CSPN?" == cmd)
  {
    QRegularExpression rx("\\+CSPN: \"(.*)\",[0-1]  OK");
    QRegularExpressionMatch match = rx.match(data);
    if (!match.isValid() || !match.hasMatch())
      return false;
    _modem.state.sim.Operator = match.captured(1);
    return true;
  }
  else if ("AT+CREG?" == cmd)
  {
    QRegularExpression rx("\\+CREG: ([0-9],[0-9])  OK");
    QRegularExpressionMatch match = rx.match(data);
    if (!match.isValid() || !match.hasMatch())
      return false;
    _modem.state.network.Registration = Modem::State::Network::registration(match.captured(1).right(1).toInt());
    return true;
  }
  else if ("AT+CGREG?" == cmd)
  {
    QRegularExpression rx("\\+CGREG: ([0-9],[0-9])  OK");
    QRegularExpressionMatch match = rx.match(data);
    if (!match.isValid() || !match.hasMatch())
      return false;
    _modem.state.network.GPRS = Modem::State::Network::gprs(match.captured(1).right(1).toInt());
    return true;
  }
  return false;
}
