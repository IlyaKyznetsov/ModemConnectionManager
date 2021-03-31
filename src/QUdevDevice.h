#ifndef QUDEVDEVICE_H
#define QUDEVDEVICE_H
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>
#include <map>
#include <string>

class QUdevPollingDevice;
class QUdev;

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
class QUdevDevice
{
public:
  enum UdevType
  {
    Unknown = 0,
    None,
    Remove,
    Unbind,
    Change,
    Bind,
    Add,
    Exist
  };

  QUdevDevice() = default;
  QUdevDevice(const QUdevDevice &) = default;
  QUdevDevice(const QString &name, const std::initializer_list<QString> &attributes = {});
  QUdevDevice(const QString &name, const QStringList &attributes);
  QUdevDevice &operator=(const QUdevDevice &) = default;

  UdevType type() const;
  QString name() const;
  QString getValue(const QString &name, const QString &defaultValue = QString()) const;

  QString toString() const;

private:
  UdevType _type;
  /*const*/ std::string _name;
  std::map<std::string, std::string> _attributes;
  friend class QUdev;
};

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
class QUdevMonitoringDevice : public QUdevDevice
{
public:
  QUdevMonitoringDevice() = default;
  QUdevMonitoringDevice(const QUdevMonitoringDevice &) = default;
  QUdevMonitoringDevice(const QString &name, const std::initializer_list<std::pair<QString, QString>> &properties,
                        const std::initializer_list<QString> &attributes = {});
  QUdevMonitoringDevice(const QString &name, const QList<QPair<QString, QString>> &properties,
                        const QStringList &attributes);

private:
  std::map<std::string, std::string> _properties;
  friend class QUdev;
};

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
class QUdevPollingDevice : public QUdevDevice
{
public:
  QUdevPollingDevice() = default;
  QUdevPollingDevice(const QUdevPollingDevice &) = default;
  QUdevPollingDevice(const QString &name, const QString &syspath, const std::initializer_list<QString> &attributes);
  QUdevPollingDevice(const QString &name, const QString &syspath, const QStringList &attributes);

private:
  /*const*/ std::string _syspath;
  friend class QUdev;
};

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QString toString(const QUdevDevice::UdevType type, const QString &defaultValue = QString());

#include <qmetatype.h>
Q_DECLARE_METATYPE(QUdevDevice)

#endif // QUDEVDEVICE_H
