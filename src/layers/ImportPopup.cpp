#include "ImportPopup.h"

ImportPopup* ImportPopup::create(CCArray* selectedObj) {
    auto ret = new ImportPopup();
    if (ret && ret->init(m_popupSize.width, m_popupSize.height)) {
        ret->autorelease();
        if (!ret->setupPopup(selectedObj)) {
            return nullptr;
        }
        return ret;
    }
    delete ret;
    return nullptr;
}

bool ImportPopup::setupPopup(CCArray* selectedObj) {
    this->m_centerObj = CCArrayExt<GameObject*>(selectedObj)[0];

    this->m_countLabel = CCLabelBMFont::create("Objects: 0", "bigFont.fnt");
    this->m_countLabel->setPosition({m_popupSize.width / 2, m_popupSize.height / 2 - 65.f});
    this->m_countLabel->setVisible(false);
    this->m_countLabel->setID("count-label");
    this->m_countLabel->setScale(0.4f);

    auto drawLabel = CCLabelBMFont::create("Scale:", "bigFont.fnt");
    drawLabel->setPosition({m_popupSize.width / 2 + 65.f, m_popupSize.height / 2 + 15.f});
    drawLabel->setVisible(false);
    drawLabel->setID("draw-scale-label");
    drawLabel->setScale(0.5f);

    auto zLayerLabel = CCLabelBMFont::create("Z-Layer\nOffset:", "bigFont.fnt");
    zLayerLabel->setPosition({m_popupSize.width / 2 - 65.f, m_popupSize.height / 2 + 15.f});
    zLayerLabel->setVisible(false);
    zLayerLabel->setID("zlayer-label");
    zLayerLabel->setScale(0.325f);

    this->m_fileLabel = CCLabelBMFont::create("", "bigFont.fnt");
    this->m_fileLabel->setColor({0, 255, 0});
    this->m_fileLabel->setPosition({m_popupSize.width / 2, m_popupSize.height / 2 + 60.f});
    this->m_fileLabel->setScale(0.45f);
    this->m_fileLabel->setID("file-label");
    this->m_fileLabel->setVisible(false);

    auto importJsonSpr = ButtonSprite::create("Select File");
    auto importJsonBtn = CCMenuItemSpriteExtra::create(
        importJsonSpr, this, menu_selector(ImportPopup::importJSON)
    );
    importJsonBtn->setID("import-btn");
    importJsonBtn->setPosition(importJsonBtn->getPosition() + m_popupSize / 2);

    auto changeJsonSpr = ButtonSprite::create("Change File");
    auto changeJsonBtn = CCMenuItemSpriteExtra::create(
        changeJsonSpr, this, menu_selector(ImportPopup::importJSON)
    );
    changeJsonBtn->setVisible(false);
    changeJsonBtn->setPosition({m_popupSize.width / 2, m_popupSize.height / 2 + 90.f});
    changeJsonBtn->setID("change-btn");

    auto parseSpr = ButtonSprite::create("Create");
    auto parseBtn = CCMenuItemSpriteExtra::create(
        parseSpr, this, menu_selector(ImportPopup::checkAlert)
    );
    parseBtn->setPosition({m_popupSize.width / 2, m_popupSize.height / 2 - 95.f});
    parseBtn->setVisible(false);
    parseBtn->setID("convert-btn");

    // Scale input
    this->m_drawScaleInput = TextInput::create(50.f, "Float", "bigFont.fnt");
    this->m_drawScaleInput->setCommonFilter(CommonFilter::Float);
    this->m_drawScaleInput->setMaxCharCount(5);
    this->m_drawScaleInput->setString("1");
    this->m_drawScaleInput->setID("draw-input");
    this->m_drawScaleInput->setPosition({m_popupSize.width / 2 + 65.f, m_popupSize.height / 2 - 15.f});
    this->m_drawScaleInput->setVisible(false);
    this->m_drawScaleInput->setPlaceholderScale(0.5f);
    this->m_drawScaleInput->setFontScale(0.6f);
    this->m_drawScaleInput->setCallback([this](std::string const& value) {
        this->onDrawScaleChanged(value);
    });

    // Z-layer input
    this->m_zLayerInput = TextInput::create(50.f, "Int", "bigFont.fnt");
    this->m_zLayerInput->setCommonFilter(CommonFilter::Int);
    this->m_zLayerInput->setMaxCharCount(5);
    this->m_zLayerInput->setString("0");
    this->m_zLayerInput->setID("zlayer-input");
    this->m_zLayerInput->setPosition({m_popupSize.width / 2 - 65.f, m_popupSize.height / 2 - 15.f});
    this->m_zLayerInput->setVisible(false);
    this->m_zLayerInput->setPlaceholderScale(0.6f);
    this->m_zLayerInput->setFontScale(0.6f);
    this->m_zLayerInput->setCallback([this](std::string const& value) {
        this->onZLayerChanged(value);
    });

    this->m_mainLayer->addChild(this->m_fileLabel);
    this->m_mainLayer->addChild(this->m_countLabel);
    this->m_mainLayer->addChild(drawLabel);
    this->m_mainLayer->addChild(zLayerLabel);

    this->m_buttonMenu->addChild(importJsonBtn);
    this->m_buttonMenu->addChild(changeJsonBtn);
    this->m_buttonMenu->addChild(parseBtn);
    this->m_buttonMenu->addChild(this->m_drawScaleInput);
    this->m_buttonMenu->addChild(this->m_zLayerInput);
    return true;
}

void ImportPopup::importJSON(CCObject* sender) {
    file::FilePickOptions options {
        .filters = { file::FilePickOptions::Filter {
            .description = "Geometrize JSON Output",
            .files = { "*.json" },
        }}
    };

    // Geode 6 uses async::spawn with a future instead of EventListener + Task
    this->m_pickTask = async::spawn(
        file::pick(file::PickMode::OpenFile, options),
        [this](file::PickResult result) {
            this->onFilePicked(std::move(result));
        }
    );
}

void ImportPopup::onFilePicked(file::PickResult result) {
    if (!result.isOk()) {
        Notification::create("Failed to open file", NotificationIcon::Error)->show();
        return;
    }

    auto maybePath = std::move(result).unwrap();
    if (!maybePath.has_value()) {
        Notification::create("File selection cancelled", NotificationIcon::Error)->show();
        return;
    }

    auto path = maybePath.value();
    if (!path.string().ends_with(".json")) {
        Notification::create("Wrong file format. It must be a .json file!", NotificationIcon::Error)->show();
        return;
    }

    // Reads the file as text
    auto text = file::readString(path);
    if (text.isErr()) {
        Notification::create("Failed to read the file", NotificationIcon::Error)->show();
        return;
    }

    // Parses the JSON
    auto parsed = matjson::parse(text.unwrap());
    if (parsed.isErr()) {
        Notification::create("Failed to parse JSON!", NotificationIcon::Error)->show();
        return;
    }
    auto json = std::move(parsed).unwrap();

    // The file can either be a bare array or an object with a "shapes" array
    matjson::Value shapesValue = json;
    if (json.isObject() && json.contains("shapes")) {
        shapesValue = json["shapes"];
    }
    auto shapesResult = shapesValue.asArray();
    if (shapesResult.isErr()) {
        Notification::create("Failed to parse \"shapes\" key as array!", NotificationIcon::Error)->show();
        return;
    }
    auto shapes = shapesResult.unwrap();

    this->m_shapes.clear();
    for (auto& shape : shapes) {
        this->m_shapes.push_back(shape);
    }

    // Counts the objects that will actually be placed
    this->m_objsCount = 0;
    for (auto shape : this->m_shapes) {
        if (this->isValidShape(shape)) {
            this->m_objsCount++;
        }
    }

    auto countText = fmt::format("Objects: {}", this->m_objsCount);
    auto fileText = fmt::format("File: {}", path.filename().string());
    this->m_countLabel->setString(countText.c_str());
    this->m_fileLabel->setString(fileText.c_str());

    this->m_buttonMenu->getChildByID("import-btn")->setVisible(false);
    this->m_buttonMenu->getChildByID("change-btn")->setVisible(true);
    this->m_buttonMenu->getChildByID("convert-btn")->setVisible(true);
    this->m_mainLayer->getChildByID("draw-scale-label")->setVisible(true);
    this->m_mainLayer->getChildByID("zlayer-label")->setVisible(true);
    this->m_fileLabel->setVisible(true);
    this->m_countLabel->setVisible(true);
    this->m_zLayerInput->setVisible(true);
    this->m_drawScaleInput->setVisible(true);

    Notification::create("File is imported", NotificationIcon::Success)->show();
}

// A shape is valid if it has a known type and a positive score
bool ImportPopup::isValidShape(matjson::Value shape) const {
    auto type = static_cast<int>(shape["type"].asDouble().unwrapOr(-1.0));
    auto score = shape["score"].asDouble().unwrapOr(0.0);
    if (score <= 0) {
        return false;
    }
    return this->m_validTypes.contains(type);
}

void ImportPopup::onDrawScaleChanged(std::string const& value) {
    auto num = utils::numFromString<float>(value);
    if (num.isErr() || num.unwrap() <= 0.f) {
        this->m_drawScaleInput->setTextColor({255, 0, 0, 255});
        return;
    }
    this->m_drawScaleInput->setTextColor({255, 255, 255, 255});
    this->m_drawScale = num.unwrap();
}

void ImportPopup::onZLayerChanged(std::string const& value) {
    auto num = utils::numFromString<int>(value);
    if (num.isErr()) {
        this->m_zLayerInput->setTextColor({255, 0, 0, 255});
        return;
    }
    this->m_zLayerInput->setTextColor({255, 255, 255, 255});
    this->m_zOrder = num.unwrap();
}

// Checks does object count is bigger than 5k. If so, it shows a warning
void ImportPopup::checkAlert(CCObject* sender) {
    if (this->m_objsCount > 5000) {
        geode::createQuickPopup(
            "Alert",
            "This will place more than <cy>5000 objects</c>\nAre you sure?",
            "No", "Yes",
            [this](auto, bool btn2) {
                if (btn2) {
                    this->parseAndPlace();
                }
            }
        );
    } else {
        this->parseAndPlace();
    }
}

// Converts the shapes to GD object strings and places them in the editor
void ImportPopup::parseAndPlace() {
    this->m_objsString.str("");
    this->m_objsString.clear();

    for (auto shape : this->m_shapes) {
        if (!this->isValidShape(shape)) {
            continue;
        }

        float posX = this->m_centerObj->getPositionX();
        float posY = this->m_centerObj->getPositionY();
        float scaleX = 1.f, scaleY = 1.f, rotation = 0.f;

        // Shape data: [x, y, width, height, rotation]
        auto data = shape["data"];
        posX = static_cast<float>(data[0].asDouble().unwrapOr(0.0)) * this->m_drawScale
             + this->m_centerObj->getPositionX();
        posY = static_cast<float>(data[1].asDouble().unwrapOr(0.0)) * this->m_drawScale
             + this->m_centerObj->getPositionY();
        scaleX = static_cast<float>(data[2].asDouble().unwrapOr(1.0)) * this->m_drawScale / 4.f * 0.16f;
        scaleY = static_cast<float>(data[3].asDouble().unwrapOr(1.0)) * this->m_drawScale / 4.f * 0.16f;
        rotation = -static_cast<float>(data[4].asDouble().unwrapOr(0.0));

        // Color: [R, G, B, A], alpha is ignored
        float r = static_cast<float>(shape["color"][0].asDouble().unwrapOr(0.0)) / 255.f;
        float g = static_cast<float>(shape["color"][1].asDouble().unwrapOr(0.0)) / 255.f;
        float b = static_cast<float>(shape["color"][2].asDouble().unwrapOr(0.0)) / 255.f;
        float h = 0.f, s = 0.f, v = 0.f;
        this->rgbToHsv(r, g, b, h, s, v);

        this->m_objsString << fmt::format(
            "1,{},2,{},3,{},128,{},129,{},6,{},41,1,42,1,21,1010,22,1010,43,{}a{}a{}a1a1,44,{}a{}a{}a1a1,25,{},372,1;",
            m_circleId, posX, posY, scaleX, scaleY, rotation,
            h, s, v, h, s, v, this->m_zOrder
        );
        this->m_zOrder++;
    }

    if (this->m_objsString.str().empty()) {
        Notification::create("No objects added.", NotificationIcon::Error)->show();
        return this->onClose(nullptr);
    }

    auto activeEditorLayer = LevelEditorLayer::get();
    auto activeEditorUI = activeEditorLayer->m_editorUI;

    activeEditorUI->onDeleteSelected(nullptr);

    auto objectsArray = activeEditorLayer->createObjectsFromString(this->m_objsString.str(), true, true);
    activeEditorUI->flipObjectsY(objectsArray);

    activeEditorLayer->m_undoObjects->addObject(
        UndoObject::createWithArray(objectsArray, UndoCommand::Paste)
    );
    activeEditorUI->selectObjects(objectsArray, true);

    activeEditorUI->updateButtons();
    Notification::create("Successfully converted to gd objects!", NotificationIcon::Success)->show();
    this->onClose(nullptr);
}

void ImportPopup::rgbToHsv(float fR, float fG, float fB, float& fH, float& fS, float& fV) {
    // Taken from https://gist.github.com/fairlight1337/4935ae72bcbcc1ba5c72#file-hsvrgb-cpp-L53
    float fCMax = std::max(std::max(fR, fG), fB);
    float fCMin = std::min(std::min(fR, fG), fB);
    float fDelta = fCMax - fCMin;

    if (fDelta > 0) {
        if (fCMax == fR) {
            fH = 60 * (fmod(((fG - fB) / fDelta), 6));
        } else if (fCMax == fG) {
            fH = 60 * (((fB - fR) / fDelta) + 2);
        } else if (fCMax == fB) {
            fH = 60 * (((fR - fG) / fDelta) + 4);
        }

        fS = fCMax > 0 ? fDelta / fCMax : 0;
        fV = fCMax;
    } else {
        fH = 0;
        fS = 0;
        fV = fCMax;
    }

    if (fH < 0) {
        fH = 360 + fH;
    }
}
