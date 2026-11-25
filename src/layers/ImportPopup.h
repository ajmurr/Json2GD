#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/utils/async.hpp>
#include <Geode/utils/file.hpp>
#include <Geode/utils/general.hpp>
#include <set>
#include <sstream>
#include <vector>

using namespace geode::prelude;

class ImportPopup : public geode::Popup {
protected:
    static constexpr int m_circleId = 497;
    static constexpr CCSize m_popupSize = CCSize(385.f, 245.f);
    // Shape types from the Geometrize JSON that get converted
    const std::set<int> m_validTypes = {3, 4, 5, 8, 16, 32};

    int m_zOrder = 0;
    int m_objsCount = 0;
    float m_drawScale = 1.f;
    std::vector<matjson::Value> m_shapes;
    std::ostringstream m_objsString;
    GameObject* m_centerObj = nullptr;
    TextInput* m_zLayerInput = nullptr;
    TextInput* m_drawScaleInput = nullptr;
    CCLabelBMFont* m_fileLabel = nullptr;
    CCLabelBMFont* m_countLabel = nullptr;
    // Keeps the file picker task alive until it finishes
    arc::TaskHandle<void> m_pickTask;

    bool setupPopup(CCArray* selectedObj);
    void importJSON(CCObject* sender);
    void onFilePicked(file::PickResult result);
    void onDrawScaleChanged(std::string const& value);
    void onZLayerChanged(std::string const& value);
    void checkAlert(CCObject* sender);
    void parseAndPlace();
    bool isValidShape(matjson::Value shape) const;
    void rgbToHsv(float fR, float fG, float fB, float& fH, float& fS, float& fV);

public:
    static ImportPopup* create(CCArray* selectedObj);
};
