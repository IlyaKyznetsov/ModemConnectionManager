#ifndef MODEMCONNECTIONMANAGER_H
#define MODEMCONNECTIONMANAGER_H

#include <Modem.h>
#include <ModemConnectionManager_global.h>
#include <QObject>

class QSerialPort;
class QProcess;
class QTimer;
class QThread;

class MODEMCONNECTIONMANAGER_EXPORT ModemConnectionManager : public QObject
{
  Q_OBJECT
public:
  Modem::State state() const;

  explicit ModemConnectionManager(Modem *modem, const QString &path = QString(), QObject *parent = nullptr);
  ~ModemConnectionManager();

Q_SIGNALS:
  void stateChanged(const Modem::State state);

public Q_SLOTS:
  bool connection();
  void disconnection();
  bool reset();

private Q_SLOTS:
  void _pppdOutput();
  void _pppdFinished(int exitCode, int exitStatus);

private:
  ModemConnectionManager(const ModemConnectionManager &) = delete;
  ModemConnectionManager(ModemConnectionManager &&) = delete;
  ModemConnectionManager &operator=(const ModemConnectionManager &) = delete;
  ModemConnectionManager &operator=(ModemConnectionManager &&) = delete;
  bool modemServicePortOpen();
  bool modemServicePortCommand(const QByteArray &ATCommand);
  void modemServicePortClose();

  int _connectionHopes = 0;
  int _reconnectionHope = 0;
  Modem *_modem = nullptr;
  QSerialPort* _modemServicePort=0;
  QTimer *_reconnectionTimer = nullptr;
  QProcess *_pppd = nullptr;
  QByteArray _pppdCommand;
  QByteArray _modemResetCommand;
};

#endif // MODEMCONNECTIONMANAGER_H
