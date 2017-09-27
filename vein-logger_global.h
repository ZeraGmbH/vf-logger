#ifndef VEINLOGGER_GLOBAL_H
#define VEINLOGGER_GLOBAL_H

#include <QtCore/qglobal.h>
#include <vh_logging.h>

Q_DECLARE_LOGGING_CATEGORY(VEIN_LOGGER)
#if defined(VEINLOGGER_LIBRARY)
#  define VEINLOGGERSHARED_EXPORT Q_DECL_EXPORT
#else
#  define VEINLOGGERSHARED_EXPORT Q_DECL_IMPORT
#endif

#if defined(VEIN_COLORS_SUPPORT)
# define VEIN_DEBUGNAME_LOGGER "\e[0;34m<Vein.Logger>\033[0m"
#else
# define VEIN_DEBUGNAME_EVENT "<Vein.Logger>"
#endif //defined(VEIN_DEBUG_COLOR_SUPPORT)

#endif // VEINLOGGER_GLOBAL_H
