#ifndef SYSTEMUTILS_H
#define SYSTEMUTILS_H

#include <QString>
#include <signal.h>

bool pkill(const QString &name, const int signo);
#endif // SYSTEMUTILS_H
