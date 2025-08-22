#include "vectorpaintingoptions.h"
#include <vectorconstants.h>
#include <QJsonDocument>
#include <QJsonArray>

const QColor VectorPaintingOptions::m_defaultColor = Qt::darkGray;
const QMap<int, QString> VectorPaintingOptions::m_defaultLabels = {
    {0, "UL1"},
    {1, "UL2"},
    {2, "UL3"},
    {3, "IL1"},
    {4, "IL2"},
    {5, "IL3"}
};
const VectorSettingsUser::VectorStandard VectorPaintingOptions::m_defaultVectorStandard = VectorSettingsUser::VectorStandard::DIN;
const VectorSettingsUser::VectorType VectorPaintingOptions::m_defaultVectorType = VectorSettingsUser::VectorType::STAR;
const VectorSettingsLengths::VectorNominals VectorPaintingOptions::m_defaultNominalSelection = VectorSettingsLengths::VectorNominals::MAXIMUM;
const float VectorPaintingOptions::m_defaultLengthValue = 1e-9;
const VectorSettingsLayout::VectorStyle VectorPaintingOptions::m_defaultStyle = VectorSettingsLayout::VectorStyle::ZENUX;

const QMap<QString, int> VectorPaintingOptions::m_channelNamesIndexMap = {
    {"UL1", 0},
    {"UL2", 1},
    {"UL3", 2},
    {"IL1", 3},
    {"IL2", 4},
    {"IL3", 5},
};

VectorPaintingOptions::VectorPaintingOptions() :
    m_colors(VectorConstants::COUNT_VECTORS),
    m_labels(VectorConstants::COUNT_VECTORS)
{
    for(auto &channelColor: m_colors)
        channelColor = m_defaultColor;
    for(int i = 0; i < m_labels.count(); i++)
        m_labels[i] = m_defaultLabels.value(i);
    m_userSettings.setVectorStandard(m_defaultVectorStandard);
    m_userSettings.setVectorType(m_defaultVectorType);
    m_lengthSettings.setNominalSelection(m_defaultNominalSelection);
    m_lengthSettings.setNomVoltage(m_defaultLengthValue);
    m_lengthSettings.setNomCurrent(m_defaultLengthValue);
    m_lengthSettings.setMinVoltage(m_defaultLengthValue);
    m_lengthSettings.setMinCurrent(m_defaultLengthValue);
    m_layoutSettings.setVectorStyle(m_defaultStyle);
}

bool VectorPaintingOptions::convertJsonParams(const QString &param)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(param.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning("VectorPaintingOptions::convertJsonParams input parameter is an invalid json.");
        return false;
    }
    QJsonObject json = doc.object();
    extractColors(json);
    extractLabels(json);
    extractUserSettings(json);
    extractLengthSettings(json);
    extractStyle(json);
    return true;
}

QColor VectorPaintingOptions::getPhaseColor(int idx) const
{
    if(idx >= m_colors.count()) {
        qWarning("VectorPaintingOptions::getPhaseColor idx is out of limit.");
        return m_defaultColor;
    }
    return m_colors[idx];
}

QString VectorPaintingOptions::getPhaseLabel(int idx) const
{
    if(idx >= m_labels.count()) {
        qWarning("VectorPaintingOptions::getPhaseLabel idx is out of limit.");
        return "";
    }
    return m_labels[idx];
}

VectorSettingsUser::VectorStandard VectorPaintingOptions::getVectorStandard() const
{
    return m_userSettings.getVectorStandard();
}

VectorSettingsUser::VectorType VectorPaintingOptions::getVectorType() const
{
    return m_userSettings.getVectorType();
}

VectorSettingsLengths::VectorNominals VectorPaintingOptions::getNominalSelection() const
{
    return m_lengthSettings.getNominalSelection();
}

float VectorPaintingOptions::getNomVoltage() const
{
    return m_lengthSettings.getNomVoltage();
}

float VectorPaintingOptions::getNomCurrent() const
{
    return m_lengthSettings.getNomCurrent();
}

float VectorPaintingOptions::getMinVoltage() const
{
    return m_lengthSettings.getMinVoltage();
}

float VectorPaintingOptions::getMinCurrent() const
{
    return m_lengthSettings.getMinCurrent();
}

VectorSettingsLayout::VectorStyle VectorPaintingOptions::getVectorStyle() const
{
    return m_layoutSettings.getVectorStyle();
}

void VectorPaintingOptions::extractColors(const QJsonObject &inputJson)
{
    if(inputJson.contains("colors")) {
        QJsonObject colors = inputJson.value("colors").toObject();
        for(QString &channel: colors.keys()) {
            if(m_channelNamesIndexMap.contains(channel))
                m_colors[m_channelNamesIndexMap.value(channel)] = colors.value(channel).toString();
        }
    }
}

void VectorPaintingOptions::extractLabels(const QJsonObject &inputJson)
{
    if(inputJson.contains("labels")) {
        QJsonObject labels = inputJson.value("labels").toObject();
        for(QString &channel: labels.keys()) {
            if(m_channelNamesIndexMap.contains(channel))
                m_labels[m_channelNamesIndexMap.value(channel)] = labels.value(channel).toString();
        }
    }
}

void VectorPaintingOptions::extractUserSettings(const QJsonObject &inputJson)
{
    if(inputJson.contains("vector_standard")) {
        int standard = inputJson.value("vector_standard").toString().toInt();
        m_userSettings.setVectorStandard(static_cast<VectorSettingsUser::VectorStandard>(standard));
    }
    if(inputJson.contains("vector_type")) {
        int type = inputJson.value("vector_type").toString().toInt();
        m_userSettings.setVectorType(static_cast<VectorSettingsUser::VectorType>(type));
    }
}

void VectorPaintingOptions::extractLengthSettings(const QJsonObject &inputJson)
{
    if(inputJson.contains("vector_circlecmode")) {
        int mode = inputJson.value("vector_circlecmode").toString().toInt();
       m_lengthSettings.setNominalSelection(static_cast<VectorSettingsLengths::VectorNominals>(mode));
    }
    if(inputJson.contains("lengths")) {
        QJsonObject lengths = inputJson.value("lengths").toObject();
        if(lengths.contains("nomvoltage"))
            m_lengthSettings.setNomVoltage(lengths.value("nomvoltage").toDouble());
        if(lengths.contains("nomCurrent"))
            m_lengthSettings.setNomCurrent(lengths.value("nomCurrent").toDouble());
        if(lengths.contains("minvoltage"))
            m_lengthSettings.setMinVoltage(lengths.value("minvoltage").toDouble());
        if(lengths.contains("minCurrent"))
            m_lengthSettings.setMinCurrent(lengths.value("minCurrent").toDouble());
    }
}

void VectorPaintingOptions::extractStyle(const QJsonObject &inputJson)
{
    if(inputJson.contains("vector_style")) {
        QString styleStr = inputJson.value("vector_style").toString();
        if(styleStr == "ZENUX")
            m_layoutSettings.setVectorStyle(VectorSettingsLayout::VectorStyle::ZENUX);
        else if(styleStr == "WEBSAM")
            m_layoutSettings.setVectorStyle(VectorSettingsLayout::VectorStyle::WEBSAM);
        else if(styleStr == "UNDEFINED")
            m_layoutSettings.setVectorStyle(VectorSettingsLayout::VectorStyle::UNDEFINED);
        else
            m_layoutSettings.setVectorStyle(m_defaultStyle);
    }
}
