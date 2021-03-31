#include "QUdevDevice.h"
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QUdevDevice::QUdevDevice(const QString &name, const std::initializer_list<QString> &attributes)
    : _type(QUdevDevice::UdevType::Unknown), _name(name.toStdString())
{
  for (const QString &name : attributes)
    _attributes.insert(std::pair<std::string, std::string>(name.toStdString(), ""));
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QUdevDevice::QUdevDevice(const QString &name, const QStringList &attributes)
    : _type(QUdevDevice::UdevType::Unknown), _name(name.toStdString())
{
  for (const QString &name : attributes)
    _attributes.insert(std::pair<std::string, std::string>(name.toStdString(), ""));
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QUdevDevice::UdevType QUdevDevice::type() const
{
  return _type;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QString QUdevDevice::name() const
{
  return QString::fromStdString(_name);
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QString QUdevDevice::getValue(const QString &name, const QString &defaultValue) const
{
  const auto &it = _attributes.find(name.toStdString());
  return (it == _attributes.cend() ? defaultValue : QString::fromStdString(it->second));
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QString QUdevDevice::toString() const
{
  QString out = QString("name=%1 | type=%2").arg(name(), ::toString(type()));
  if (!_attributes.empty())
  {
    out.append(" | attributes:");
    for (auto item : _attributes)
    {
      out.append(QString(" %1=%2 |").arg(QString::fromStdString(item.first), QString::fromStdString(item.second)));
    }
  }
  return out;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QUdevMonitoringDevice::QUdevMonitoringDevice(const QString &name,
                                             const std::initializer_list<std::pair<QString, QString>> &properties,
                                             const std::initializer_list<QString> &attributes)
    : QUdevDevice(name, attributes)
{
  for (const std::pair<QString, QString> &prop : properties)
    _properties.insert(std::pair<std::string, std::string>(prop.first.toStdString(), prop.second.toStdString()));
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QUdevMonitoringDevice::QUdevMonitoringDevice(const QString &name, const QList<QPair<QString, QString>> &properties,
                                             const QStringList &attributes)
    : QUdevDevice(name, attributes)
{
  for (const QPair<QString, QString> &prop : properties)
    _properties.insert(std::pair<std::string, std::string>(prop.first.toStdString(), prop.second.toStdString()));
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QUdevPollingDevice::QUdevPollingDevice(const QString &name, const QString &syspath,
                                       const std::initializer_list<QString> &attributes)
    : QUdevDevice(name, attributes), _syspath(syspath.toStdString())
{
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QUdevPollingDevice::QUdevPollingDevice(const QString &name, const QString &syspath, const QStringList &attributes)
    : QUdevDevice(name, attributes), _syspath(syspath.toStdString())
{
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QString toString(const QUdevDevice::UdevType type, const QString &defaultValue)
{
  const static std::map<QUdevDevice::UdevType, QString> map_types = {
      {QUdevDevice::UdevType::Unknown, "unknown"}, {QUdevDevice::UdevType::None, "none"},
      {QUdevDevice::UdevType::Remove, "remove"},   {QUdevDevice::UdevType::Unbind, "unbind"},
      {QUdevDevice::UdevType::Change, "change"},   {QUdevDevice::UdevType::Bind, "bind"},
      {QUdevDevice::UdevType::Add, "add"},         {QUdevDevice::UdevType::Exist, "exist"}};
  const auto &type_iterator = map_types.find(type);
  return (map_types.cend() == type_iterator ? defaultValue : type_iterator->second);
}
