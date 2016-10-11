#ifndef VEINLOGGER_GLOBAL_H
#define VEINLOGGER_GLOBAL_H

#include <QtCore/qglobal.h>
#include <vh_logging.h>

#if defined(VEINLOGGER_LIBRARY)
#  define VEINLOGGERSHARED_EXPORT Q_DECL_EXPORT
#else
#  define VEINLOGGERSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // VEINLOGGER_GLOBAL_H
