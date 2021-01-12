#include "Global.h"
#include "ModemConnectionAutomator.h"
#include "ModemConnectionManager.h"
#include <QCoreApplication>
#include <QRegularExpression>

#include <QThread>
#include <QTimer>

int main(int argc, char *argv[])
{
  QCoreApplication a(argc, argv);
  auto appPids = pids(QCoreApplication::applicationName());
  appPids.removeAll(QCoreApplication::applicationPid());
  if (!appPids.isEmpty())
  {
    C(QCoreApplication::applicationName() << "already started pid:" << appPids);
    return 1;
  }

  ModemConnectionAutomator mca("", &a);
  //  mca.connection();

  //  QThread thread(&a);
  //  ModemConnectionManager *m = new ModemConnectionManager();
  //  //  m->connection();
  //  m->moveToThread(&thread);
  //  thread.start();
  //  QObject::connect(&thread, &QThread::started, [&m]() {
  //    D("XXX: Started");
  //    QObject::connect(m, &ModemConnectionManager::stateChanged, [](const ModemConnectionManager::State state) {
  //      D("###############################");
  //      for (const auto &item : toStringList(state))
  //        D(item);
  //    });
  //    m->connection();
  //  });

  //  QObject::connect(&thread, &QThread::finished, []() { D("XXX: Finished"); });
  //  QObject::connect(&thread, &QThread::destroyed, []() { D("XXX: Destroyed"); });

  /*
    // Read configuration
    QObject::connect(&ModemConnectionManager::instance(), &ModemConnectionManager::stateChanged,
                     [](const ModemConnectionManager::State state) { D(toStringList(state)); });
    ModemConnectionManager::instance().connection();
    */
  QTimer test;
  test.setInterval(60000);
  test.setSingleShot(true);
  QObject::connect(&test, &QTimer::timeout, []() {
    D("### Disconnection ###");
    //    ModemConnectionManager::instance().disconnection();
    throw 1;
    //      D("### Hard reset modem ###");
    //      ModemConnectionManager::instance().modemHardReset();
    //      D("### Connection ###");
    //      ModemConnectionManager::instance().connection();
  });
  test.start();
  return a.exec();
}
