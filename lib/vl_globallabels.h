#ifndef VL_GLOBALLABELS_H
#define VL_GLOBALLABELS_H

#include <QStringList>

class VLGlobalLabels {
public:
    const static QStringList noStoreComponents() { return QStringList()
        << QStringLiteral("EntityName")
        << QStringLiteral("INF_ModuleInterface"); }
};

#endif // VL_GLOBALLABELS_H
