#ifndef TESTINSERTSPIESTOJSON_H
#define TESTINSERTSPIESTOJSON_H

#include <QByteArray>
#include <QSignalSpy>

class TestInsertSpiesToJson
{
public:
    static QByteArray spiesToJsonAndClear(QSignalSpy &spyDbEntitiesInserted, QSignalSpy &spyDbComponentsInserted);
    static QJsonArray spyEntitiesToJson(const QSignalSpy &spyDbEntitiesInserted);
    static QJsonArray spyComponentsToJson(const QSignalSpy &spyDbComponentsInserted);
};

#endif // TESTINSERTSPIESTOJSON_H
