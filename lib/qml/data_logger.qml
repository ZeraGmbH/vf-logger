import QtQuick 2.0
import VeinEntity 1.0
import VeinLogger 1.0

VeinLogger {
    initializeValues: true;
    sessionName: loggerEntity.sessionName;
    transactionName: loggerEntity.transactionName;
    guiContext: loggerEntity.guiContext;


    readonly property QtObject systemEntity: VeinEntity.getEntity("_System");
    readonly property QtObject loggerEntity: VeinEntity.getEntity("_LoggingSystem");
    readonly property string sysSession: systemEntity.Session
    onSysSessionChanged: {
        session = systemEntity.Session;
        loggerEntity.availableContentSets = getAvailableContentSets();
    }

    // next candidate for C++
    readonly property var sysContentSets: loggerEntity.currentContentSets;
    onSysContentSetsChanged: {
        // update VeinLogger property contentSet
        contentSets = loggerEntity.currentContentSets;

        var loggedComponentsFromContentSets = readContentSets();

        clearLoggerEntries();
        loggerEntity.LoggedComponents = loggedComponentsFromContentSets;
    }

    readonly property bool scriptRunning: loggingEnabled
    onScriptRunningChanged: {
        if(scriptRunning === true) {
            console.log("starting logging at", new Date().toLocaleTimeString());
            startLogging();
        }
        else {
            stopLogging();
            console.log("stopped logging at", new Date().toLocaleTimeString());
        }
    }

    Component.onCompleted: console.info("QML Data logger running")
}
