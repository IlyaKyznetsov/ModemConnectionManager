extern "C"
{
#include <libudev.h>
}
#include "QUdev.h"
#include "Global.h"
#include <QSocketNotifier>
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QStringList toStringListUdevDevice(udev_device *device)
{
  DF(device);
  QStringList out;
  struct udev_list_entry *entry_device = NULL;

  udev_list_entry_foreach(entry_device, udev_device_get_properties_list_entry(device))
  {
    const char *const p_name = udev_list_entry_get_name(entry_device);
    out.append(
        QString("PROP: %1=%2")
            .arg(QString::fromLatin1(p_name), QString::fromLatin1(udev_device_get_property_value(device, p_name))));
  }

  udev_list_entry_foreach(entry_device, udev_device_get_sysattr_list_entry(device))
  {
    const char *const p_name = udev_list_entry_get_name(entry_device);
    out.append(
        QString("ATTR: %1=%2")
            .arg(QString::fromLatin1(p_name), QString::fromLatin1(udev_device_get_sysattr_value(device, p_name))));
  }

  return out;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
bool QUdev::updateQUdevDevice(udev_device *udevDevice, QUdevDevice &item) const
{
  typedef const char *(*udev_device_get)(struct udev_device *);
  const static std::map<std::string, udev_device_get> map_udev_device_get = {
      {"ACTION", udev_device_get_action},       {"DEVPATH", udev_device_get_devpath},
      {"SUBSYSTEM", udev_device_get_subsystem}, {"DEVTYPE", udev_device_get_devtype},
      {"SYSPATH", udev_device_get_syspath},     {"SYSNAME", udev_device_get_sysname},
      {"SYSNUM", udev_device_get_sysnum},       {"DEVNODE", udev_device_get_devnode},
      {"DRIVER", udev_device_get_driver}};

  const static std::map<std::string, QUdevDevice::UdevType> map_types = {{"remove", QUdevDevice::UdevType::Remove},
                                                                     {"unbind", QUdevDevice::UdevType::Unbind},
                                                                     {"change", QUdevDevice::UdevType::Change},
                                                                     {"bind", QUdevDevice::UdevType::Bind},
                                                                     {"add", QUdevDevice::UdevType::Add}};
  bool isUpdated = false;
  if (!udevDevice)
  {
    if (QUdevDevice::UdevType::None != item._type)
    {
      item._type = QUdevDevice::UdevType::None;
      isUpdated = true;
    }
    return isUpdated;
  }

  QUdevDevice::UdevType eventType = QUdevDevice::UdevType::Exist;
  const char *ptr_value = NULL;
  ptr_value = udev_device_get_action(udevDevice);
  if (ptr_value)
  {
    const auto &type_iterator = map_types.find(ptr_value);
    if (map_types.cend() != type_iterator)
      eventType = type_iterator->second;
    else
      W("Unknown event type:" + QString(ptr_value));
  }
  if (item._type != eventType)
  {
    item._type = eventType;
    isUpdated = true;
  }

  for (auto &attr : item._attributes)
  {
    const auto &func_iterator = map_udev_device_get.find(attr.first);
    if (map_udev_device_get.cend() == func_iterator)
      ptr_value = udev_device_get_sysattr_value(udevDevice, attr.first.c_str());
    else
      ptr_value = func_iterator->second(udevDevice);

    if (ptr_value && strcmp(ptr_value, attr.second.c_str()))
    {
      attr.second = std::string(ptr_value);
      isUpdated = true;
    }

    if (!ptr_value)
    {
      W(QString("Device %1 not have attribute %2")
            .arg(QString::fromStdString(item._name), QString::fromStdString(attr.first)));
    }
  }
  return isUpdated;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QUdevMonitoringDevice *QUdev::findMonitoringQUdevDevice(udev_device *udevDevice)
{
  for (QUdevMonitoringDevice &item : _monitoringDevices)
  {
    bool skip = false;
    for (const auto &prop : item._properties)
    {
      const char *value_ptr = udev_device_get_property_value(udevDevice, prop.first.c_str());
      if (!value_ptr || strcmp(value_ptr, prop.second.c_str()))
      {
        skip = true;
        break;
      }
    }
    if (!skip)
      return &item;
  }
  return 0;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QUdev &QUdev::instance()
{
  static QUdev qudev;
  return qudev;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void QUdev::addMonitoringDevice(const QUdevMonitoringDevice &device)
{
  DF(device.name());
  if (_isMonitoring.load())
  {
    D("udev monitoring is already running");
    return;
  }
  _monitoringDevices.append(device);
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void QUdev::addPollingDevice(const QUdevPollingDevice &device)
{
  DF(device.name());
  if (_isEnumerating.load())
  {
    D("udev enumerating is already running");
    return;
  }
  _pollingDevices.append(device);
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QString QUdev::getSyspathDeviceValueOwner(const QString &syspathDevice, const QString &name,
                                          const QString &defaultValue)
{
  PF();
  D(syspathDevice << name << defaultValue);

  struct udev_device *udevDevice = udev_device_new_from_syspath(_udev, syspathDevice.toStdString().c_str());
  if (udevDevice)
  {
    const char *const p_value = udev_device_get_sysattr_value(udevDevice, name.toStdString().c_str());
    if (!p_value)
    {
      udev_device_unref(udevDevice);
      return defaultValue;
    }

    const QString ret = QString::fromLatin1(p_value);
    udev_device_unref(udevDevice);
    D("--- out:" << ret);
    return ret;
  }
  return defaultValue;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
bool QUdev::setSyspathDeviceValueOwner(const QString &syspathDevice, const QString &name, const QString &newValue)
{
  PF();
  D(syspathDevice << name << newValue);
  bool isOk = false;
  struct udev_device *udevDevice = udev_device_new_from_syspath(_udev, syspathDevice.toStdString().c_str());
  if (udevDevice)
  {

    QByteArray data = newValue.toLatin1();
    if (0 == udev_device_set_sysattr_value(udevDevice, name.toStdString().c_str(), data.data()))
      isOk = true;
    udev_device_unref(udevDevice);
  }
  return isOk;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void QUdev::enumerateAllDevices()
{
  PF();
  _mutex.lock();
  QMetaObject::invokeMethod(this, "enumerateAllDevicesOwner", Qt::QueuedConnection);
  DF("--- wait for started");
  _condition.wait(&_mutex);
  _mutex.unlock();
  DF("--- started");
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void QUdev::runMonitoring(const QString &type)
{
  DF(!_isMonitoring);
  if (_isMonitoring.load())
    return;

  _isMonitoring.store(true);
  QMetaObject::invokeMethod(this, "runMonitoringOwner", Qt::QueuedConnection, Q_ARG(QString, type));
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void QUdev::runPolling(int msec)
{
  DF(!_isEnumerating);
  if (_isEnumerating.load())
    return;
  _isEnumerating.store(true);

  QMetaObject::invokeMethod(this, "runPollingOwner", Qt::QueuedConnection, Q_ARG(int, msec));
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QString QUdev::getSyspathDeviceValue(const QString &syspathDevice, const QString &name, const QString &defaultValue)
{
  QString result;
  QMetaObject::invokeMethod(this, "getSyspathDeviceValueOwner", Qt::BlockingQueuedConnection,
                            Q_RETURN_ARG(QString, result), Q_ARG(QString, syspathDevice), Q_ARG(QString, name),
                            Q_ARG(QString, defaultValue));
  return result;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
bool QUdev::setSyspathDeviceValue(const QString &syspathDevice, const QString &name, const QString &newValue)
{
  bool result(false);
  QMetaObject::invokeMethod(this, "setSyspathDeviceValueOwner", Qt::BlockingQueuedConnection,
                            Q_RETURN_ARG(bool, result), Q_ARG(QString, syspathDevice), Q_ARG(QString, name),
                            Q_ARG(QString, newValue));
  return result;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QUdev::QUdev()
    : QObject(nullptr),
      _udev(nullptr),
      _monitor(nullptr),
      _socketNotifier(nullptr),
      _isMonitoring(false),
      _isEnumerating(false)
{
  DF(this);
  _mutex.lock();
  connect(&_thread, &QThread::started, this, &QUdev::postConstructOwner, Qt::DirectConnection);
  connect(&_thread, &QThread::finished, this, &QUdev::preDestroyOwner, Qt::DirectConnection);
  moveToThread(&_thread);
  _thread.start();
  DF("--- wait for started");
  _condition.wait(&_mutex);
  _mutex.unlock();
  DF("--- started");
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QUdev::~QUdev()
{
  DF(this);
  _thread.quit();
  _thread.wait();
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void QUdev::runMonitoringOwner(const QString &type)
{
  PF();
  // create udev_monitor object ("udev"|"kernel")
  if (NULL == (_monitor = udev_monitor_new_from_netlink(_udev, type.toLatin1().data())))
    throw global::Exception("Cannot create udev monitor");
  D("Create udev monitor:" << type);

  for (const QUdevMonitoringDevice &item : _monitoringDevices)
  {

    const auto it_subsystem = item._properties.find("SUBSYSTEM");
    const auto it_devtype = item._properties.find("DEVTYPE");
    const char *p_subsystem = (it_subsystem == item._properties.cend() ? NULL : it_subsystem->second.data());
    const char *p_devtype = (it_devtype == item._properties.cend() ? NULL : it_devtype->second.data());
    if (p_subsystem || p_devtype)
    {
      if (0 != udev_monitor_filter_add_match_subsystem_devtype(_monitor, p_subsystem, p_devtype))
      {
        throw global::Exception(
            QString("Cannot add udev monitor filter subsystem: %1 devtype: %2").arg(p_subsystem, p_devtype));
      }
    }
    D(QString("Add udev monitor filter subsystem: %1 devtype: %2").arg(p_subsystem, p_devtype));
  }

  if (0 != udev_monitor_enable_receiving(_monitor))
    throw global::Exception("Cannot receiving udev monitor");

  int fd = udev_monitor_get_fd(_monitor);
  if (fd < 0)
    throw global::Exception("Cannot get udev monitor socket descriptor");

  _socketNotifier.reset(new QSocketNotifier(fd, QSocketNotifier::Read));
  connect(_socketNotifier.data(), &QSocketNotifier::activated, this, &QUdev::udevMonitoringEventPrivate,
          Qt::DirectConnection);
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void QUdev::runPollingOwner(int msec)
{
  DF(msec);
  _timer.reset(new QTimer());
  connect(_timer.data(), &QTimer::timeout, this, &QUdev::polling);
  _timer->setSingleShot(false);
  _timer->start(msec);
  polling();
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void QUdev::enumerateAllDevicesOwner()
{
  PF();
  struct udev_enumerate *_udev_enumerate = NULL;

  // create enumerate object
  if (NULL == (_udev_enumerate = udev_enumerate_new(_udev)))
    throw global::Exception("Cannot create udev enumerate");

  // scan enumerate ( with filters )
  if (0 != udev_enumerate_scan_devices(_udev_enumerate))
  {
    _udev_enumerate = udev_enumerate_unref(_udev_enumerate);
    throw global::Exception("Cannot udev enumerate scan devices");
  }

  /* fillup device list */
  struct udev_list_entry *_udev_devices_list = udev_enumerate_get_list_entry(_udev_enumerate);
  if (!_udev_devices_list)
  {
    throw global::Exception("Cannot udev enumerate get devices list");
  }

  struct udev_list_entry *_udev_current_device = NULL;
  udev_list_entry_foreach(_udev_current_device, _udev_devices_list)
  {
    const char *const p_name = udev_list_entry_get_name(_udev_current_device);
    udev_device *udevDevice = udev_device_new_from_syspath(_udev, p_name);

    if (udevDevice)
    {
      // Write to log info about device
      // D(toStringListUdevDevice(udevDevice));

      // Try initialization polling device
      QVector<QUdevPollingDevice>::iterator it_item =
          std::find_if(_pollingDevices.begin(), _pollingDevices.end(),
                       [&p_name](QUdevPollingDevice &it) { return !strcmp(it._syspath.c_str(), p_name); });
      if (it_item != _pollingDevices.cend())
      {
        //        D("Polling device before init:" << it_item->toString());
        if (updateQUdevDevice(udevDevice, *it_item))
        {
          //          D("Polling device after init:" << it_item->toString());
          emit qudevDeviceEvent(*static_cast<QUdevDevice *>(it_item));
        }
      }

      // Try initialization monitoring device
      QUdevMonitoringDevice *p_item = findMonitoringQUdevDevice(udevDevice);
      if (p_item)
      {
        //        D("Monitoring device before init:" << p_item->toString());
        if (updateQUdevDevice(udevDevice, *p_item))
        {
          //          D("Monitoring device after init:" << p_item->toString());
          emit qudevDeviceEvent(*static_cast<QUdevDevice *>(p_item));
        }
      }
    }

    /* free dev */
    udev_device_unref(udevDevice);
  }

  _udev_enumerate = udev_enumerate_unref(_udev_enumerate);

  _condition.wakeAll();
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void QUdev::postConstructOwner()
{
  DF(this);
  disconnect(&_thread, &QThread::started, this, &QUdev::postConstructOwner);
  // create udev static object for udev_monitor
  if (NULL == _udev)
  {
    // create udev object
    if (NULL == (_udev = udev_new()))
      throw global::Exception("Cannot create udev");
  }

  _condition.wakeAll();
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void QUdev::polling()
{
  // get info devices by syspath
  for (QUdevPollingDevice &item : _pollingDevices)
  {
    struct udev_device *udevDevice = udev_device_new_from_syspath(_udev, item._syspath.c_str());
    if (updateQUdevDevice(udevDevice, item))
      emit qudevDeviceEvent(static_cast<QUdevDevice>(item));
    udevDevice = udev_device_unref(udevDevice);
  }
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void QUdev::udevMonitoringEventPrivate(int socket)
{
  Q_UNUSED(socket);
  struct udev_device *udevDevice = udev_monitor_receive_device(_monitor);
//  DF(udevDevice);
  if (!udevDevice)
    return;

  QUdevMonitoringDevice *p_item = findMonitoringQUdevDevice(udevDevice);
  if (p_item && updateQUdevDevice(udevDevice, *p_item))
  {
    emit qudevDeviceEvent(*static_cast<QUdevDevice *>(p_item));
  }

  udevDevice = udev_device_unref(udevDevice);
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void QUdev::preDestroyOwner()
{
  PF();
  if (!_timer.isNull())
  {
    _timer->stop();
    _timer.reset();
  }

  if (!_socketNotifier.isNull())
  {
    _socketNotifier->setEnabled(false);
    _socketNotifier.reset();
  }

  if (NULL != _monitor)
    _monitor = udev_monitor_unref(_monitor);

  if (NULL != _udev)
    _udev = udev_unref(_udev);

  _condition.wakeAll();
}
