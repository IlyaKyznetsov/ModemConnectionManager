#include "ModemConnectionManager.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QTimer>
#include <QUdev.h>
#include <QtSerialPort/QSerialPort>
#include <modems/SIM7600E_H.h>
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

    if ("Connection Settings" == group)
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
  return (_modem ? _modem->state : Modem::State());
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
ModemConnectionManager::ModemConnectionManager(const QString &path, QObject *parent) : QObject(parent)
{
  DF(path);
  qRegisterMetaType<Modem::State>("Modem::State");
  pppdterm();

  // Read configuration
  QSettings::Format format = QSettings::registerFormat("ModemConnectionManager", readSettings, nullptr);
  const QString rpath = (!path.isEmpty() && QFile::exists(path) ? path : "://ModemConnectionManager.conf");
  QSettings settings(rpath, format, this);
  // settings.setIniCodec("UTF-8");
  settings.beginGroup("Pppd Settings");
  const QStringList options = settings.value("options").toStringList();
  for (const QString &item : options)
    _options += ' ' + item.toLatin1();
  settings.endGroup();
  settings.beginGroup("Connection Settings");
  _reconnectionHope = _connectionHopes = settings.value("ResetConnectionHopes", 0).toInt();
  const int reconnectionTimeout = settings.value("ReconnectTimeout", 0).toInt();
  _phone = settings.value("Phone").toByteArray();
  if (_phone.isEmpty())
    throw global::Exception("[Connection Settings] Phone not exist");
  _user = settings.value("User").toByteArray();
  const QByteArray password = settings.value("Password").toByteArray();
  _accessPoint = settings.value("AccessPoint").toByteArray();
  settings.endGroup();

  // Create secrets:
  const int count = 2;
  const QString files[count] = {"/etc/ppp/chap-secrets", "/etc/ppp/pap-secrets"};

  if (!_user.isEmpty() && !password.isEmpty())
  {
    for (int i = 0; i != count; ++i)
    {
      QFile file(files[i]);
      if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        throw global::Exception("Cannot write secrets files");
      file.write(_user + '\t' + '*' + '\t' + password);
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

  // QUdev
  auto &udev = QUdev::instance();
  connect(&udev, &QUdev::qudevDeviceEvent, this, &ModemConnectionManager::_qudevDeviceEvent, Qt::QueuedConnection);
  udev.addMonitoringDevice(QUdevMonitoringDevice(
      "SIM7600E_H", {{"SUBSYSTEM", "tty"}, {"ID_VENDOR_ID", "1e0e"}, {"ID_MODEL_ID", "9001"}}, {"DEVPATH"}));
  udev.enumerateAllDevices();
  udev.runMonitoring();
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
  if (!_modem)
    return false;
  DF("Started" << _pppdCommand);
  disconnection();

  if (_reconnectionTimer && _pppd->signalsBlocked())
    _pppd->blockSignals(false);
  if (_pppdCommand.isEmpty())
    return false;

  _pppd->start(_pppdCommand);
  bool isStarted = _pppd->waitForStarted(5000);
  const int pid = (isStarted ? _pppd->processId() : -1);
  if (pid != _modem->state.internet.PID)
  {
    _modem->state.internet.PID = pid;
    emit stateChanged(_modem->state);
  }

  if (_connectionHopes)
  {
    if (!_reconnectionHope)
      reset();
    else
      --_reconnectionHope;

    D("ConnectionHope:" << _reconnectionHope);
  }
  return isStarted;
  DF("Finished");
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void ModemConnectionManager::disconnection()
{
  DF(_pppd);

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

  if (_modem)
  {
    clearState(_modem->state, false);
    emit stateChanged(_modem->state);
  }
  else
    emit stateChanged(Modem::State());
  DF("Finished");
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
bool ModemConnectionManager::reset()
{
  PF();
  if (_connectionHopes)
    _reconnectionHope = _connectionHopes;
  bool isOk = _modem->reset();
  emit stateChanged(Modem::State());
  return isOk;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void ModemConnectionManager::_qudevDeviceEvent(const QUdevDevice &event)
{
  const auto type = event.type();
  const auto name = event.name();
  switch (type)
  {
    // Ignored events
    case QUdevDevice::Unknown:
    case QUdevDevice::None:
    case QUdevDevice::Change: return;
    default:
    {
      // Modem exist
      if (_modem)
      {
        switch (type)
        {
          case QUdevDevice::Bind:
          case QUdevDevice::Add:
          case QUdevDevice::Exist: return;
          case QUdevDevice::Remove:
          case QUdevDevice::Unbind:
          {
            DF(toString(type) + ':' + name);
            // Modem remove
            if (_modem->name() == name)
            {
              disconnection();
              _modem.reset();
              _pppdCommand.clear();
              emit stateChanged(Modem::State());
            }
            return;
          }
          break;
          default: return;
        }
      }
      // Modem not exist
      else
      {
        switch (type)
        {
          case QUdevDevice::Remove:
          case QUdevDevice::Unbind: return;
          case QUdevDevice::Bind:
          case QUdevDevice::Add:
          case QUdevDevice::Exist:
          {
            DF(toString(type) + ':' + name);
            // Modem add
            if ("SIM7600E_H" == name)
            {
              _modem.reset(new SIM7600E_H());
              bool isOk = _modem->initialize();
              emit stateChanged(_modem->state);
              D("Modem initialize:" << isOk);
              if (!isOk)
                reset();
              else
              {
                const QByteArray port = _modem->portConnection();
                _pppdCommand = "/usr/sbin/pppd " + port.mid(port.lastIndexOf('/') + 1) + " " +
                               QByteArray::number(_modem->baudRate()) + " " + _options + " connect " +
                               _modem->chatConfiguration(_phone, _accessPoint);
                if (!_user.isEmpty())
                  _pppdCommand += " user " + _user;

                isOk = connection();
                D("Internet connection:" << isOk);
              }
            }
            return;
          }
          break;
          default: return;
        }
      }
    }
  }
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void ModemConnectionManager::_pppdOutput()
{
  PF();

  QByteArray data = _pppd->readAllStandardOutput().simplified();
  if (data.isEmpty())
    return;
  data.replace("^M", "");

  // Parse AT Commands responses
  Modem::CommandStatus commandStatus = _modem->parseResponse(data);
  D("Modem response:" << toString(commandStatus) << data);
  bool isStateChanged = commandStatus != Modem::CommandStatus::None;

  // Parse Pppd output
  QRegularExpression rx("Using interface\\s+(\\S+)\\s+");
  QRegularExpressionMatch match = rx.match(data);
  match = rx.match(data);
  if (match.isValid() && match.hasMatch())
  {
    _modem->state.internet.Interface = match.captured(1);
    isStateChanged = true;
  }

  const QString ip4 =
      "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4]["
      "0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)";

  rx.setPattern(QString("remote\\s+IP\\s+address\\s+(%1)").arg(ip4));
  match = rx.match(data);
  if (match.isValid() && match.hasMatch())
  {
    _modem->state.internet.RemoteAddress = match.captured(1);
    isStateChanged = true;
  }

  rx.setPattern(QString("local\\s+IP\\s+address\\s+(%1)").arg(ip4));
  match = rx.match(data);
  if (match.isValid() && match.hasMatch())
  {
    _modem->state.internet.LocalAddress = match.captured(1);
    isStateChanged = true;
  }

  rx.setPattern(QString("primary\\s+DNS\\s+address\\s+(%1)").arg(ip4));
  match = rx.match(data);
  if (match.isValid() && match.hasMatch())
  {
    _modem->state.internet.PrimaryDNS = match.captured(1);
    isStateChanged = true;
  }

  rx.setPattern(QString("secondary\\s+DNS\\s+address\\s+(%1)").arg(ip4));
  match = rx.match(data);
  if (match.isValid() && match.hasMatch())
  {
    _modem->state.internet.SecondaryDNS = match.captured(1);
    isStateChanged = true;
  }

  if (isStateChanged)
    emit stateChanged(_modem->state);
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void ModemConnectionManager::_pppdFinished(int exitCode, int exitStatus)
{
  Q_UNUSED(exitStatus);
  //  const QProcess::ExitStatus status = QProcess::ExitStatus(exitStatus);
  //  if (6 == exitCode)
  //    modemHardReset();

  clearState(_modem->state, false);
  emit stateChanged(_modem->state);

  DF(exitCode << errorString(exitCode));
  if (_reconnectionTimer && _pppd)
  {
    D("Try reconnect after: " << _reconnectionTimer->interval() / 1000 << "secs");
    _reconnectionTimer->start();
  }
}
