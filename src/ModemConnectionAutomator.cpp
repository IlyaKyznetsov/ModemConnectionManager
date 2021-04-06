#include "ModemConnectionAutomator.h"
#include "Global.h"
#include <QUdev.h>
#include <modems/SIM7600E_H.h>

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
ModemConnectionAutomator::ModemConnectionAutomator(const QString &path, QObject *parent)
    : QObject(parent), _thread(new QThread(this))
{
  PF();
  connect(_thread, &QThread::started, this, &ModemConnectionAutomator::onStarted);
  _mcm = new ModemConnectionManager(path);
  _mcm->moveToThread(_thread);
  _thread->start();
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
ModemConnectionAutomator::~ModemConnectionAutomator()
{
  PF();
  if (_mcm)
  {
    _mcm->disconnection();
    _mcm->deleteLater();
  }
  _thread->quit();
  _thread->wait();
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
const Modem::State &ModemConnectionAutomator::state() const
{
  PF();
  return _state;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
bool ModemConnectionAutomator::connection()
{
  PF();
  if (QThread::currentThread() != _thread)
  {
    bool result(false);
    QMetaObject::invokeMethod(_mcm, "connection", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, result));
    return result;
  }
  return _mcm->connection();
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void ModemConnectionAutomator::disconnection()
{
  PF();
  if (QThread::currentThread() != _thread)
  {
    QMetaObject::invokeMethod(_mcm, "disconnection", Qt::BlockingQueuedConnection);
    return;
  }
  _mcm->disconnection();
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
bool ModemConnectionAutomator::modemHardReset()
{
  PF();
  if (QThread::currentThread() != _thread)
  {
    bool result(false);
    QMetaObject::invokeMethod(_mcm, "modemHardReset", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, result));
    return result;
  }
  return _mcm->reset();
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void ModemConnectionAutomator::onStarted()
{
  PF();
  qRegisterMetaType<Modem::State>("Modem::State");
  connect(_mcm, &ModemConnectionManager::stateChanged, this, &ModemConnectionAutomator::onStateChanged);
  auto &udev = QUdev::instance();
  connect(&udev, &QUdev::qudevDeviceEvent, this, &ModemConnectionAutomator::qudevDeviceEvent, Qt::QueuedConnection);
  udev.addMonitoringDevice(QUdevMonitoringDevice(
      "SIM7600E", {{"SUBSYSTEM", "tty"}, {"ID_VENDOR_ID", "1e0e"}, {"ID_MODEL_ID", "9001"}}, {"DEVPATH"}));
  udev.enumerateAllDevices();
  udev.runMonitoring();
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void ModemConnectionAutomator::onStateChanged(const Modem::State &state)
{
  PF();
  _state = state;
  emit stateChanged(_state);
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void ModemConnectionAutomator::qudevDeviceEvent(const QUdevDevice &event)
{
  const auto type = event.type();
  const auto name = event.name();
  switch (type)
  {
    case QUdevDevice::Unknown:
    case QUdevDevice::None:
    case QUdevDevice::Change: return;
    case QUdevDevice::Remove:
    case QUdevDevice::Unbind:
    {
      const QByteArray modem = _mcm->modemName();
      if (name == modem)
      {
        DF(toString(type) + ':' + name);
        D(event.toString());
        if (_mcm)
        {
          _mcm->disconnection();
          _mcm->deleteLater();
        }
        _thread->quit();
        _thread->wait();
      }
    }
    break;
    case QUdevDevice::Bind:
    case QUdevDevice::Add:
    case QUdevDevice::Exist:
    {
      if (name == "SIM7600E" && _mcm->modemName().isEmpty())
      {
        DF(toString(type) + ':' + name);
        D(event.toString());
        Modem *modem = new SIM7600E_H;
        D(modem);
        if (modem->initialize())
        {
          _mcm->setModem(modem);
          connection();
        }
        else
        {
          _mcm->setModem();
          Q_EMIT stateChanged(modem->state);
          D(modem->reset());
        }
      }
    }
    break;
  }
}
