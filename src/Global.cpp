#include "Global.h"

#include "Global.h"
#include <QFile>
#include <QTextStream>
#include <iostream>

void messageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
  static QFile log("/tmp/out.log");
  if (!log.isOpen())
    log.open(QIODevice::WriteOnly | QIODevice::Text);
  Q_UNUSED(context);
  QByteArray localMsg = msg.toLocal8Bit();
  QTextStream out(&log);
  switch (type)
  {
    case QtDebugMsg:
    case QtInfoMsg:
    case QtWarningMsg:
    case QtCriticalMsg:
    case QtFatalMsg: out << localMsg.constData() << '\n'; std::cout << localMsg.constData() << std::endl;
  }
}

QString toString(const bool x)
{
  return (x ? "true" : "false");
}

#include <QDir>
#include <QRegularExpression>

QList<pid_t> pids(const QString &name)
{
  DF(name);
  QList<pid_t> res;
  const QString shortName = name.left(15);
  const QStringList procs = QDir("/proc").entryList({}, QDir::Dirs).filter(QRegularExpression("^[0-9]+$"));
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
        res.append(pid);
    }
  }
  return res;
}
