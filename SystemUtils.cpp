#include "SystemUtils.h"

#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QStringList>
#include <unistd.h>

#include "Global.h"

bool pkill(const QString &name, const int signo)
{
  DF(name << QString::number(signo));
  const QString shortName = name.left(15);
  const QStringList procs = QDir("/proc").entryList({}, QDir::Dirs).filter(QRegularExpression("^[0-9]+$"));
  bool isOk = false;
  for (const QString &item : procs)
  {
    QFile comm("/proc/" + item + "/comm");
    bool isPid;
    pid_t pid = item.toInt(&isPid);
    if (comm.exists() && isPid)
    {
      comm.open(QIODevice::ReadOnly);
      QString processName = comm.readAll().simplified();
      comm.close();
      if (processName == shortName)
        isOk = (0 == kill(pid, signo));
    }
  }
  return isOk;
}
