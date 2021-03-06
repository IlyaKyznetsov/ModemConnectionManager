#include "ModemConnectionAutomator.h"
#include "Global.h"

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
    _mcm->disconnection(false);
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
  _mcm->disconnection(false);
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void ModemConnectionAutomator::modemHardReset()
{
  PF();
  if (QThread::currentThread() != _thread)
  {
    bool result(false);
    QMetaObject::invokeMethod(_mcm, "modemHardReset", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, result));
    return;
  }
  _mcm->reset();
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void ModemConnectionAutomator::onStarted()
{
  PF();
  qRegisterMetaType<Modem::State>("Modem::State");
  connect(_mcm, &ModemConnectionManager::stateChanged, this, &ModemConnectionAutomator::onStateChanged);
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void ModemConnectionAutomator::onStateChanged(const Modem::State &state)
{
  PF();
  _state = state;
  emit stateChanged(_state);
}
