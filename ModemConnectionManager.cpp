#include "ModemConnectionManager.h"

#include <signal.h>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QRegularExpression>
#include <QSettings>

#include "Global.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
static bool pkill(const QString &name, const int signo)
{
  DF(name << QString::number(signo));
  const QString shortName = name.left(15);
  const QStringList procs = QDir("/proc").entryList({}, QDir::Dirs).filter(QRegularExpression("^[0-9]+$"));
  bool isOk = false;
  for (const QString &item : procs)
  {
    QFile comm("/proc/" + item + "/comm");
    bool isPid;
    pid_t pid = item.toInt(&isPid);
    if (comm.exists() && isPid)
    {
      comm.open(QIODevice::ReadOnly);
      QString processName = comm.readAll().simplified();
      comm.close();
      if (processName == shortName)
        isOk = (0 == kill(pid, signo));
    }
  }
  return isOk;
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
ModemConnectionManager &ModemConnectionManager::instance(const QString &path)
{
  static ModemConnectionManager obj(path);
  return obj;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
ModemConnectionManager::State ModemConnectionManager::state() const
{
  return _state;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
ModemConnectionManager::ModemConnectionManager(const QString &path, QObject *parent)
    : QObject(parent), _configurationPath(path)
{
  _mutex.lock();
  connect(&_thread, &QThread::started, this, &ModemConnectionManager::_postConstructOwner, Qt::QueuedConnection);
  connect(&_thread, &QThread::finished, this, &ModemConnectionManager::_preDestroyOwner, Qt::QueuedConnection);
  moveToThread(&_thread);
  _thread.start();
  DF("--- wait for started");
  _condition.wait(&_mutex);
  _mutex.unlock();
  DF("--- started");
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
ModemConnectionManager::~ModemConnectionManager()
{
  PF();
  _thread.quit();
  _thread.wait();
}

void ModemConnectionManager::connection()
{
  QMetaObject::invokeMethod(this, "_connection", Qt::QueuedConnection);
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void ModemConnectionManager::disconnection()
{
  _reconnectionHope = _resetConnectionHopes;
  QMetaObject::invokeMethod(this, "_disconnection", Qt::QueuedConnection);
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void ModemConnectionManager::modemHardReset()
{
  _reconnectionHope = _resetConnectionHopes;
  QMetaObject::invokeMethod(this, "_modemHardReset", Qt::QueuedConnection);
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
bool ModemConnectionManager::_connection()
{
  PF();
  _disconnection();

  // Pppd setup and connection
  _reconnectionTimer.reset(new QTimer());
  _pppd.reset(new QProcess());
  if (!_pppd || !_reconnectionTimer)
  {
    _pppd.reset();
    _reconnectionTimer.reset();
    W("Cannot create pppd process");
    return false;
  }

  _reconnectionTimer->setSingleShot(true);
  _reconnectionTimer->setInterval(_reconnectTimeout * 1000);
  connect(_reconnectionTimer.data(), &QTimer::timeout, this, &ModemConnectionManager::connection);

  _pppd->setProcessChannelMode(QProcess::MergedChannels);
  connect(_pppd.data(), &QProcess::readyReadStandardOutput, this, &ModemConnectionManager::_pppdOutput);
  connect(_pppd.data(), &QProcess::readyReadStandardError, this, &ModemConnectionManager::_pppdError);
  connect(_pppd.data(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
          &ModemConnectionManager::_pppdFinished);
  _pppd->start("/usr/sbin/pppd", _pppdArguments);
  bool isStarted = _pppd->waitForStarted(5000);
  const int pid = (isStarted ? _pppd->processId() : -1);
  if (pid != _state.internet.PID)
  {
    _state.internet.PID = pid;
    emit stateChanged(_state);
  }
  if (_resetConnectionHopes)
  {
    if (!_reconnectionHope)
      modemHardReset();
    else
      --_reconnectionHope;
  }
  D("ConnectionHope:" << _reconnectionHope);
  return isStarted;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void ModemConnectionManager::_disconnection()
{
  PF();
  if (_reconnectionTimer)
  {
    if (_reconnectionTimer->isActive())
      _reconnectionTimer->stop();
    _reconnectionTimer.reset();
  }
  if (_pppd)
  {
    if (QProcess::NotRunning != _pppd->state())
    {
      _pppd->terminate();
      _pppd->waitForFinished(-1);
    }
    _pppd.reset();
  }

  clearState(_state, false);
  emit stateChanged(_state);
}

bool ModemConnectionManager::_modemHardReset()
{
  PF();
  QProcess process;
  process.start(_modemResetCommand, QStringList());
  bool isOk = process.waitForStarted(5000);
  if (isOk)
  {
    clearState(_state, true);
    emit stateChanged(_state);
    isOk = process.waitForFinished(-1);
  }
  return isOk;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void ModemConnectionManager::_postConstructOwner()
{
  PF();
  disconnect(&_thread, &QThread::started, this, &ModemConnectionManager::_postConstructOwner);
  pkill("pppd", SIGTERM);
  // Read configuration
  QSettings::Format format = QSettings::registerFormat("ModemConnectionManager", readSettings, nullptr);
  const QString rpath =
      (!_configurationPath.isEmpty() && QFile::exists(_configurationPath) ? _configurationPath
                                                                          : "://ModemConnectionManager.conf");
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
  _reconnectionHope = _resetConnectionHopes = settings.value("ResetConnectionHopes", 0).toInt();
  _reconnectTimeout = settings.value("ReconnectTimeout", 0).toInt();
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
  _pppdArguments.append({"call", "ModemConnection"});
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
    _pppdArguments.append({"user", user});
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

  _condition.wakeAll();
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void ModemConnectionManager::_preDestroyOwner()
{
  PF();
  _reconnectionHope = _resetConnectionHopes;
  _disconnection();
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
void ModemConnectionManager::_pppdError()
{
  PF();
  QByteArray data = _pppd->readAllStandardError().simplified();
  if (data.isEmpty())
    return;
  D("Error:" << data);
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void ModemConnectionManager::_pppdFinished(int exitCode, int exitStatus)
{
  Q_UNUSED(exitStatus);
  //  const QProcess::ExitStatus status = QProcess::ExitStatus(exitStatus);
  QString message;
  switch (exitCode)
  {

    case 0:
      message = "Pppd has detached, or otherwise the connection was successfully established and terminated at the "
                "peer's request.";
      break;
    case 1:
      message = "An immediately fatal error of some kind occurred, such as an essential system call failing, or "
                "running out of virtual memory.";
      break;
    case 2:
      message =
          "An error was detected in processing the options given, such as two mutually exclusive options being used.";
      break;
    case 3: message = "Pppd is not setuid-root and the invoking user is not root."; break;
    case 4:
      message =
          "The kernel does not support PPP, for example, the PPP kernel driver is not included or cannot be loaded.";
      break;
    case 5: message = "Pppd terminated because it was sent a SIGINT, SIGTERM or SIGHUP signal."; break;
    case 6: message = "The serial port could not be locked."; break;
    case 7: message = "The serial port could not be opened."; break;
    case 8: message = "The connect script failed (returned a non-zero exit status)."; break;
    case 9: message = "The command specified as the argument to the pty option could not be run."; break;
    case 10:
      message = "The PPP negotiation failed, that is, it didn't reach the point where at least one network protocol "
                "(e.g. IP) was running.";
      break;
    case 11: message = "The peer system failed (or refused) to authenticate itself."; break;
    case 12: message = "The link was established successfully and terminated because it was idle."; break;
    case 13:
      message = "The link was established successfully and terminated because the connect time limit was reached.";
      break;
    case 14: message = "Callback was negotiated and an incoming call should arrive shortly."; break;
    case 15: message = "The link was terminated because the peer is not responding to echo requests."; break;
    case 16: message = "The link was terminated by the modem hanging up."; break;
    case 17: message = "The PPP negotiation failed because serial loopback was detected."; break;
    case 18: message = "The init script failed (returned a non-zero exit status)."; break;
    case 19: message = "We failed to authenticate ourselves to the peer."; break;
    default: break;
  }

  //  if (6 == exitCode)
  //    modemHardReset();

  clearState(_state, false);
  emit stateChanged(_state);

  DF(exitCode << message);
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
