#ifndef GLOBAL_H
#define GLOBAL_H

#include <QDateTime>
#include <QDebug>
#include <QException>
#include <QThread>

#define GLOBAL_DEBUG_PREFIX QTime::currentTime().toString("mm:ss.zzz") << QThread::currentThreadId()

#ifdef QT_DEBUG
#define D(x) qInfo() << "D DEBUG:" << GLOBAL_DEBUG_PREFIX << x
#else
#define D(x)
#endif
#define PF() qInfo() << "PF:" << GLOBAL_DEBUG_PREFIX << __PRETTY_FUNCTION__
#define DF(x) qInfo() << "DF:" << GLOBAL_DEBUG_PREFIX << __PRETTY_FUNCTION__ << x
#define I(x) qInfo() << "I:" << GLOBAL_DEBUG_PREFIX << x
#define W(x) qWarning() << "W:" << GLOBAL_DEBUG_PREFIX << __FILE__ << __LINE__ << x
#define C(x) qCritical() << "C:" << GLOBAL_DEBUG_PREFIX << x

void messageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);

namespace global
{
class Exception
{
public:
  explicit Exception(const QString &msg)
  {
    qCritical() << "Exception:" << msg;
    throw std::exception();
  }
};
} // namespace global

QString toString(const bool x);

#endif // GLOBAL_H
