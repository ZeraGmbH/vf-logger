#include "vectorpaintingoptions.h"
#include <vectorconstants.h>
#include <QJsonDocument>
#include <QJsonArray>
#include <cmath>

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

static constexpr const char* RangeModule = "1020";

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

bool VectorPaintingOptions::convertJsonParams(const QVariantMap &param)
{
    QJsonObject json = QJsonObject::fromVariantMap(param);
    extractColors(json);
    extractLabels(json);
    extractUserSettings(json);
    extractNominalSelection(json);
    extractStyle(json);
    return true;
}

void VectorPaintingOptions::calculateNominalAndMinValues(QJsonObject loggedValues)
{
    QJsonObject rangeValues = loggedValues.value(RangeModule).toObject();

    float maxVal = 0.0;
    for(int channel = 1; channel <= 3; channel++) {
        float newVal = rangeValues.value("INF_Channel" + QString::number(channel) + "ActREJ").toDouble() / rangeValues.value("INF_PreScalingInfoGroup0").toDouble();
        if(newVal > maxVal)
            maxVal = newVal;
    }
    float maxRejectionU = maxVal * sqrt(2);
    m_lengthSettings.setNomVoltage(maxRejectionU);
    float minVoltage = (m_lengthSettings.getNominalSelection() == VectorSettingsLengths::VectorNominals::MAXIMUM) ?
                           (maxRejectionU * 0.009) : (maxRejectionU * 0.05);
    m_lengthSettings.setMinVoltage(minVoltage);

    maxVal = 0.0;
    for(int channel = 4; channel <= 6; channel++) {
        float newVal = rangeValues.value("INF_Channel" + QString::number(channel) + "ActREJ").toDouble() / rangeValues.value("INF_PreScalingInfoGroup1").toDouble();
        if(newVal > maxVal)
            maxVal = newVal;
    }
    float maxRejectionI = maxVal * sqrt(2);
    m_lengthSettings.setNomCurrent(maxRejectionI);
    float minCurrent = (m_lengthSettings.getNominalSelection() == VectorSettingsLengths::VectorNominals::MAXIMUM) ?
                           (maxRejectionI * 0.009) : (maxRejectionI * 0.05);
    m_lengthSettings.setMinCurrent(minCurrent);
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
    if(inputJson.contains("colors") && inputJson.value("colors").isObject()) {
        QJsonObject colors = inputJson.value("colors").toObject();
        for(QString &channel: colors.keys()) {
            if(m_channelNamesIndexMap.contains(channel))
                m_colors[m_channelNamesIndexMap.value(channel)] = colors.value(channel).toString();
        }
    }
}

void VectorPaintingOptions::extractLabels(const QJsonObject &inputJson)
{
    if(inputJson.contains("labels") && inputJson.value("labels").isObject()) {
        QJsonObject labels = inputJson.value("labels").toObject();
        for(QString &channel: labels.keys()) {
            if(m_channelNamesIndexMap.contains(channel))
                m_labels[m_channelNamesIndexMap.value(channel)] = labels.value(channel).toString();
        }
    }
}

void VectorPaintingOptions::extractUserSettings(const QJsonObject &inputJson)
{
    if(inputJson.contains("vector_standard") && inputJson.value("vector_standard").isString()) {
        int standard = inputJson.value("vector_standard").toString().toInt();
        switch(static_cast<VectorSettingsUser::VectorStandard>(standard)) {
        case VectorSettingsUser::VectorStandard::DIN:
            m_userSettings.setVectorStandard(VectorSettingsUser::VectorStandard::DIN);
            break;
        case VectorSettingsUser::VectorStandard::IEC:
            m_userSettings.setVectorStandard(VectorSettingsUser::VectorStandard::IEC);
            break;
        case VectorSettingsUser::VectorStandard::ANSI:
            m_userSettings.setVectorStandard(VectorSettingsUser::VectorStandard::ANSI);
            break;
        default:
            m_userSettings.setVectorStandard(m_defaultVectorStandard);
            break;
        }
    }

    if(inputJson.contains("vector_type") && inputJson.value("vector_standard").isString()) {
        int type = inputJson.value("vector_type").toString().toInt();
        switch(static_cast<VectorSettingsUser::VectorType>(type)) {
        case VectorSettingsUser::VectorType::STAR:
            m_userSettings.setVectorType(VectorSettingsUser::VectorType::STAR);
            break;
        case VectorSettingsUser::VectorType::TRIANGLE:
            m_userSettings.setVectorType(VectorSettingsUser::VectorType::TRIANGLE);
            break;
        case VectorSettingsUser::VectorType::THREE_PHASE:
            m_userSettings.setVectorType(VectorSettingsUser::VectorType::THREE_PHASE);
            break;
        default:
            m_userSettings.setVectorType(m_defaultVectorType);
            break;
        }
    }
}

void VectorPaintingOptions::extractNominalSelection(const QJsonObject &inputJson)
{
    if(inputJson.contains("vector_circlecmode") && inputJson.value("vector_circlecmode").isString()) {
        int mode = inputJson.value("vector_circlecmode").toString().toInt();
        switch(static_cast<VectorSettingsLengths::VectorNominals>(mode)) {
        case VectorSettingsLengths::VectorNominals::NOMINAL:
            m_lengthSettings.setNominalSelection(VectorSettingsLengths::VectorNominals::NOMINAL);
            break;
        case VectorSettingsLengths::VectorNominals::MAXIMUM:
            m_lengthSettings.setNominalSelection(VectorSettingsLengths::VectorNominals::MAXIMUM);
            break;
        default:
            m_lengthSettings.setNominalSelection(m_defaultNominalSelection);
            break;
        }
    }
}

void VectorPaintingOptions::extractStyle(const QJsonObject &inputJson)
{
    if(inputJson.contains("vector_style") && inputJson.value("vector_style").isString()) {
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
