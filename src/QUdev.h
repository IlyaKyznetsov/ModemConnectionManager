#ifndef QUDEV_H
#define QUDEV_H
extern "C" struct udev;
extern "C" struct udev_monitor;
#include "Global.h"
#include <QMap>
#include <QMutex>
#include <QObject>
#include <QScopedPointer>
#include <QSocketNotifier>
#include <QThread>
#include <QTimer>
#include <QUdevDevice.h>
#include <QVector>
#include <QWaitCondition>

class QTUDEV_EXPORT QUdev : public QObject
{
  Q_OBJECT
public:
  static QUdev &instance();
  void addMonitoringDevice(const QUdevMonitoringDevice &device);
  void addPollingDevice(const QUdevPollingDevice &device);
  void enumerateAllDevices();
  void runMonitoring(const QString &type = "udev");
  void runPolling(int msec = 1000);

  QString getSyspathDeviceValue(const QString &syspathDevice, const QString &name,
                                const QString &defaultValue = QString());
  bool setSyspathDeviceValue(const QString &syspathDevice, const QString &name, const QString &newValue);

private:
  explicit QUdev();
  ~QUdev();
  QUdev(const QUdev &) = delete;
  QUdev(QUdev &&) = delete;
  QUdev &operator=(const QUdev &) = delete;
  QUdev &operator=(QUdev &&) = delete;
  Q_INVOKABLE void runMonitoringOwner(const QString &type);
  Q_INVOKABLE void runPollingOwner(int msec);
  Q_INVOKABLE void enumerateAllDevicesOwner();
  Q_INVOKABLE QString getSyspathDeviceValueOwner(const QString &syspathDevice, const QString &name,
                                                 const QString &defaultValue = QString());
  Q_INVOKABLE bool setSyspathDeviceValueOwner(const QString &syspathDevice, const QString &name,
                                              const QString &newValue);
  inline bool updateQUdevDevice(struct udev_device *udevDevice, QUdevDevice &item) const;
  QUdevMonitoringDevice *findMonitoringQUdevDevice(struct udev_device *udevDevice);

  struct udev *_udev;
  struct udev_monitor *_monitor;
  QScopedPointer<QSocketNotifier> _socketNotifier;
  QVector<QUdevMonitoringDevice> _monitoringDevices;
  QVector<QUdevPollingDevice> _pollingDevices;

  QThread _thread;
  QMutex _mutex;
  QWaitCondition _condition;
  QScopedPointer<QTimer> _timer;
  std::atomic_bool _isMonitoring;
  std::atomic_bool _isEnumerating;

private Q_SLOTS:
  void postConstructOwner();
  void polling();
  void udevMonitoringEventPrivate(int socket);
  void preDestroyOwner();

Q_SIGNALS:
  void qudevDeviceEvent(const QUdevDevice &event);
};

#endif // QUDEV_H
