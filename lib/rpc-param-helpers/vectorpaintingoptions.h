#ifndef VECTORPAINTINGOPTIONS_H
#define VECTORPAINTINGOPTIONS_H

#include <vectorsettings.h>
#include <QString>
#include <QColor>
#include <QJsonObject>
#include <QMap>

class VectorPaintingOptions
{
public:
    VectorPaintingOptions();
    bool parseParams(const QVariantMap &param);
    void calculateNominalAndMinValues(QJsonObject loggedValues);
    QColor getPhaseColor(int idx) const;
    QString getPhaseLabel(int idx) const;
    VectorSettingsUser::VectorStandard getVectorStandard() const;
    VectorSettingsUser::VectorType getVectorType() const;
    VectorSettingsLengths::VectorNominals getNominalSelection() const;
    float getNomVoltage() const;
    float getNomCurrent() const;
    float getMinVoltage() const;
    float getMinCurrent() const;
    VectorSettingsLayout::VectorStyle getVectorStyle() const;

    static const QColor m_defaultColor;
    static const QMap<int, QString> m_defaultLabels;
    static const VectorSettingsUser::VectorStandard m_defaultVectorStandard;
    static const VectorSettingsUser::VectorType m_defaultVectorType;
    static const VectorSettingsLengths::VectorNominals m_defaultNominalSelection;
    static const float m_defaultLengthValue;
    static const VectorSettingsLayout::VectorStyle m_defaultStyle;

private:
    void extractColors(const QJsonObject &inputJson);
    void extractLabels(const QJsonObject &inputJson);
    void extractUserSettings(const QJsonObject &inputJson);
    void extractNominalSelection(const QJsonObject &inputJson);
    void extractStyle(const QJsonObject &inputJson);

    static const QMap<QString, int> m_channelNamesIndexMap;
    QVector<QColor> m_colors;
    QVector<QString> m_labels;
    VectorSettingsUser m_userSettings;
    VectorSettingsLengths m_lengthSettings;
    VectorSettingsLayout m_layoutSettings;
};

#endif // VECTORPAINTINGOPTIONS_H
