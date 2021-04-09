#ifndef MODEMCONNECTIONAUTOMATOR_H
#define MODEMCONNECTIONAUTOMATOR_H

#include <ModemConnectionManager.h>
#include <ModemConnectionManager_global.h>

class QUdevDevice;
class MODEMCONNECTIONMANAGER_EXPORT ModemConnectionAutomator : public QObject
{
  Q_OBJECT
public:
  explicit ModemConnectionAutomator(const QString &path = QString(), QObject *parent = nullptr);
  ~ModemConnectionAutomator();
  const Modem::State &state() const;

Q_SIGNALS:
  void stateChanged(const Modem::State state);

public Q_SLOTS:
  bool connection();
  void disconnection();
  void modemHardReset();

private Q_SLOTS:
  void onStarted();
  void onStateChanged(const Modem::State &state);

private:
  ModemConnectionAutomator(const ModemConnectionAutomator &) = delete;
  ModemConnectionAutomator(ModemConnectionAutomator &&) = delete;
  ModemConnectionAutomator &operator=(const ModemConnectionAutomator &) = delete;
  ModemConnectionAutomator &operator=(ModemConnectionAutomator &&) = delete;

  QThread *_thread = nullptr;
  ModemConnectionManager *_mcm = nullptr;
  Modem::State _state;
};

#endif // MODEMCONNECTIONAUTOMATOR_H
