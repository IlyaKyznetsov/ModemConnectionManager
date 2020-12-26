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
