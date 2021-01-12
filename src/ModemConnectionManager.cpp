#include "ModemConnectionManager.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QTimer>

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
QString toString(ModemConnectionManager::State::Network::registration status)
{
  switch (status)
  {
    case ModemConnectionManager::State::Network::registration::NotRegistered: return "not registered";
    case ModemConnectionManager::State::Network::registration::RegisteredHomeNetwork: return "registered, home network";
    case ModemConnectionManager::State::Network::registration::Searching: return "not registered, searching";
    case ModemConnectionManager::State::Network::registration::RegistrationDenied: return "registration denied";
    case ModemConnectionManager::State::Network::registration::Unknown: return "unknown";
    case ModemConnectionManager::State::Network::registration::RegisteredRoaming: return "registered, roaming";
  }
  return QString();
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QString toString(ModemConnectionManager::State::Network::gprs status)
{
  switch (status)
  {
    case ModemConnectionManager::State::Network::gprs::NotRegistered: return "not registered";
    case ModemConnectionManager::State::Network::gprs::RegisteredHomeNetwork: return "registered, home network";
    case ModemConnectionManager::State::Network::gprs::Searching:
      return "not registered, trying to attach or searching";
    case ModemConnectionManager::State::Network::gprs::RegistrationDenied: return "registration denied";
    case ModemConnectionManager::State::Network::gprs::Unknown: return "unknown";
    case ModemConnectionManager::State::Network::gprs::RegisteredRoaming: return "registered, roaming";
  }
  return QString();
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QStringList toStringList(const ModemConnectionManager::State &state)
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
    else if ("Modem Commands" == group)
    {
      const QString key = group + "/chat";
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
inline void clearState(ModemConnectionManager::State &state, bool all)
{
  state.internet.PID = 0;
  if (state.network.Registration != ModemConnectionManager::State::Network::registration::NotRegistered)
    state.network.Registration = ModemConnectionManager::State::Network::registration::Unknown;
  if (state.network.GPRS != ModemConnectionManager::State::Network::gprs::NotRegistered)
    state.network.GPRS = ModemConnectionManager::State::Network::gprs::Unknown;
  state.internet.Interface = state.internet.LocalAddress = state.internet.RemoteAddress = state.internet.PrimaryDNS =
      state.internet.SecondaryDNS = QString();
  if (all)
  {
    state.network.Registration = ModemConnectionManager::State::Network::registration::NotRegistered;
    state.network.GPRS = ModemConnectionManager::State::Network::gprs::NotRegistered;
    state.modem.Manufacturer = state.modem.Model = state.modem.Revision = state.modem.IMEI = state.sim.ICCID =
        state.sim.Operator = QString();
  }
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
ModemConnectionManager::State ModemConnectionManager::state() const
{
  return _state;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
ModemConnectionManager::ModemConnectionManager(const QString &path, QObject *parent) : QObject(parent)
{
  DF(path);
  qRegisterMetaType<ModemConnectionManager::State>("ModemConnectionManager::State");

  pppdterm();

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
  const QByteArray baud = settings.value("Baud").toByteArray();
  if (baud.isEmpty())
    throw global::Exception("[Modem Configuration] Baud not exist");
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
  QStringList options = settings.value("options").toStringList();
  settings.endGroup();
  settings.beginGroup("Modem Commands");
  const QStringList chat = settings.value("chat").toStringList();
  D(chat);
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

  QList<QByteArray> data;

  // Create chat configuration:
  const QByteArray chatPath = "/tmp/ModemConnection.chat";
  QFile file(chatPath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    throw global::Exception("Cannot create chat");

  data.append("ECHO OFF");
  data.append("ABORT \'NO CARRIER\'");
  data.append("ABORT \'NO DIALTONE\'");
  data.append("ABORT \'ERROR\'");
  data.append("ABORT \'NO ANSWER\'");
  data.append("ABORT \'BUSY\'");
  data.append("TIMEOUT 5");
  for (const QString &item : chat)
  {
    QStringList group(item.split("' '"));
    D(group);
    int pos = -1;
    QByteArray cmd;
    QByteArray check;
    QString rx;
    for (QString elm : group)
    {
      ++pos;
      auto size = elm.size();
      if (0 == size)
        continue;
      if ('\'' == elm[size - 1])
      {
        elm.remove(size - 1, 1);
        --size;
        if (0 == size)
          continue;
      }
      if ('\'' == elm[0])
      {
        elm.remove(0, 1);
        --size;
        if (0 == size)
          continue;
      }

      switch (pos)
      {
        case 0: cmd = elm.toLatin1(); break;
        case 1: check = elm.toLatin1(); break;
        case 2: rx = elm; break;
        default: throw global::Exception("Bad [Modem Commands] format");
      }

      if (!cmd.isEmpty() && !check.isEmpty())
        data.append(check + " \'" + cmd + "\'");
    }
  }
  data.append("OK \'ATD" + phone + "\'");
  data.append("CONNECT \\c");
  file.write(data.join('\n'));
  file.close();

  // Create peer configuration:
  file.setFileName("/etc/ppp/peers/ModemConnection");
  QStringList pppdArguments{"call", "ModemConnection"};
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    throw global::Exception("Cannot create peer");
  // -rwxr-xr-x
  file.setPermissions(QFileDevice::ReadUser | QFileDevice::WriteUser | QFileDevice::ExeUser | QFileDevice::ReadGroup |
                      QFileDevice::ExeGroup | QFileDevice::ExeOther);
  data = {modem.mid(modem.lastIndexOf('/') + 1), baud};
  data.append("connect \'chat -v -s -S -f " + chatPath + (accessPoint.isEmpty() ? "\'" : " -T " + accessPoint + "\'"));

  for (const QString &item : options)
  {
    data.append(item.toLatin1());
  }
  file.write(data.join('\n'));
  file.close();

  // Create secrets:
  const int count = 2;
  const QString files[count] = {"/etc/ppp/chap-secrets", "/etc/ppp/pap-secrets"};

  if (!user.isEmpty() && !password.isEmpty())
  {
    pppdArguments.append({"user", user});
    for (int i = 0; i != count; ++i)
    {
      file.setFileName(files[i]);
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
  _pppd->setProgram("/usr/sbin/pppd");
  _pppd->setArguments(pppdArguments);
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
  if (_pppd)
  {
    _pppd->disconnect(this);
    _pppd->terminate();
    if (!_pppd->waitForFinished(5000))
      _pppd->kill();
  }
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
bool ModemConnectionManager::connection()
{
  PF();
  disconnection();
  _pppd->start();
  bool isStarted = _pppd->waitForStarted(5000);
  const int pid = (isStarted ? _pppd->processId() : -1);
  if (pid != _state.internet.PID)
  {
    _state.internet.PID = pid;
    emit stateChanged(_state);
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

  if (_reconnectionTimer && _reconnectionTimer->isActive())
    _reconnectionTimer->stop();

  if (QProcess::NotRunning != _pppd->state())
  {
    _pppd->terminate();
    if (!_pppd->waitForFinished(5000))
    {
      _pppd->kill();
      _pppd->waitForFinished(-1);
    }
  }

  clearState(_state, false);
  emit stateChanged(_state);
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
    clearState(_state, true);
    emit stateChanged(_state);
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
  bool isStateChanged = SIM7600E_H(data);

  // Parse Pppd output
  QRegularExpression rx("Using interface (\\S+) ");
  QRegularExpressionMatch match = rx.match(data);
  match = rx.match(data);
  if (match.isValid() && match.hasMatch())
  {
    _state.internet.Interface = match.captured(1);
    isStateChanged = true;
  }

  const QString ip4 =
      "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4]["
      "0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)";

  rx.setPattern(QString("remote IP address (%1)").arg(ip4));
  match = rx.match(data);
  if (match.isValid() && match.hasMatch())
  {
    _state.internet.RemoteAddress = match.captured(1);
    isStateChanged = true;
  }

  rx.setPattern(QString("local IP address (%1)").arg(ip4));
  match = rx.match(data);
  if (match.isValid() && match.hasMatch())
  {
    _state.internet.LocalAddress = match.captured(1);
    isStateChanged = true;
  }

  rx.setPattern(QString("primary DNS address (%1)").arg(ip4));
  match = rx.match(data);
  if (match.isValid() && match.hasMatch())
  {
    _state.internet.PrimaryDNS = match.captured(1);
    isStateChanged = true;
  }

  rx.setPattern(QString("secondary DNS address (%1)").arg(ip4));
  match = rx.match(data);
  if (match.isValid() && match.hasMatch())
  {
    _state.internet.SecondaryDNS = match.captured(1);
    isStateChanged = true;
  }

  if (isStateChanged)
    emit stateChanged(_state);

  return;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void ModemConnectionManager::_pppdFinished(int exitCode, int exitStatus)
{
  Q_UNUSED(exitStatus);
  //  const QProcess::ExitStatus status = QProcess::ExitStatus(exitStatus);
  //  if (6 == exitCode)
  //    modemHardReset();

  clearState(_state, false);
  emit stateChanged(_state);

  DF(exitCode << errorString(exitCode));
  if (_reconnectionTimer && _pppd)
  {
    D("Try reconnect after: " << _reconnectionTimer->interval() / 1000 << "secs");
    _reconnectionTimer->start();
  }
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
bool ModemConnectionManager::SIM7600E_H(const QByteArray &data)
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
    _state.modem.Manufacturer = match.captured(1);
    _state.modem.Model = match.captured(2);
    _state.modem.Revision = match.captured(3);
    _state.modem.IMEI = match.captured(4);
    return true;
  }
  else if ("AT+CICCID" == cmd)
  {
    QRegularExpression rx("\\+ICCID: (.*)  OK");
    QRegularExpressionMatch match = rx.match(data);
    if (!match.isValid() || !match.hasMatch())
      return false;
    _state.sim.ICCID = match.captured(1);
    return true;
  }
  else if ("AT+CSPN?" == cmd)
  {
    QRegularExpression rx("\\+CSPN: \"(.*)\",[0-1]  OK");
    QRegularExpressionMatch match = rx.match(data);
    if (!match.isValid() || !match.hasMatch())
      return false;
    _state.sim.Operator = match.captured(1);
    return true;
  }
  else if ("AT+CREG?" == cmd)
  {
    QRegularExpression rx("\\+CREG: ([0-9],[0-9])  OK");
    QRegularExpressionMatch match = rx.match(data);
    if (!match.isValid() || !match.hasMatch())
      return false;
    _state.network.Registration = State::Network::registration(match.captured(1).right(1).toInt());
    return true;
  }
  else if ("AT+CGREG?" == cmd)
  {
    QRegularExpression rx("\\+CGREG: ([0-9],[0-9])  OK");
    QRegularExpressionMatch match = rx.match(data);
    if (!match.isValid() || !match.hasMatch())
      return false;
    _state.network.GPRS = State::Network::gprs(match.captured(1).right(1).toInt());
    return true;
  }
  return false;
}
