#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

#include <cmath>
#include <cstdint>
#include <unordered_set>

using namespace geode::prelude;

namespace {
    constexpr float GRID = 30.f;
    constexpr int BLOCK_ID = 1; // plain square block, used as the deco building piece

    int64_t cellKey(int x, int y) {
        return (static_cast<int64_t>(x) << 32) | static_cast<uint32_t>(y);
    }

    int cellOf(float v) {
        return static_cast<int>(std::floor(v / GRID));
    }
}

class $modify(AutoDesignEditorUI, EditorUI) {
    struct Fields {
        int placed = 0;
        int cap = 2500;
    };

    bool init(LevelEditorLayer* lel) {
        if (!EditorUI::init(lel)) return false;

        // Black box button
        auto bg = cocos2d::extension::CCScale9Sprite::create("square02_001.png", {0, 0, 80, 80});
        bg->setContentSize({44.f, 44.f});
        bg->setColor({0, 0, 0});
        bg->setOpacity(230);

        auto label = CCLabelBMFont::create("AUTO", "bigFont.fnt");
        label->setScale(0.42f);
        label->setPosition(bg->getContentSize() / 2.f);
        bg->addChild(label);

        auto btn = CCMenuItemSpriteExtra::create(
            bg, this, menu_selector(AutoDesignEditorUI::onAutoDesign)
        );
        btn->setID("auto-design-button"_spr);

        // Put it in the editor's undo menu (right side of the editor)
        CCNode* menu = this->getChildByID("undo-menu");
        if (!menu) menu = this->getChildByID("editor-buttons-menu");

        if (menu) {
            menu->addChild(btn);
            menu->updateLayout();
        } else {
            // fallback: own menu in the top-right corner
            auto fallback = CCMenu::create();
            auto win = CCDirector::get()->getWinSize();
            fallback->setPosition({win.width - 40.f, win.height - 120.f});
            fallback->addChild(btn);
            this->addChild(fallback, 100);
        }

        return true;
    }

    // Create one deco piece. Returns true if placed.
    bool placePiece(float x, float y, float sx, float sy) {
        auto f = m_fields.self();
        if (f->placed >= f->cap) return false;

        auto obj = m_editorLayer->createObject(BLOCK_ID, {x, y}, true);
        if (!obj) return false;

        if (Mod::get()->getSettingValue<bool>("no-touch")) {
            obj->m_isNoTouch = true;
        }
        obj->updateCustomScaleX(sx);
        obj->updateCustomScaleY(sy);

        int64_t channel = Mod::get()->getSettingValue<int64_t>("color-channel");
        if (channel > 0 && obj->m_baseColor) {
            obj->m_baseColor->m_colorID = static_cast<int>(channel);
        }

        f->placed++;
        return true;
    }

    void onAutoDesign(CCObject*) {
        auto f = m_fields.self();
        f->placed = 0;
        f->cap = static_cast<int>(Mod::get()->getSettingValue<int64_t>("max-objects"));

        CCArray* selected = this->getSelectedObjects();
        if (!selected || selected->count() == 0) {
            Notification::create("Select some blocks first!", NotificationIcon::Warning)->show();
            return;
        }

        // 1. Gather solid blocks into a grid-cell set
        std::unordered_set<int64_t> solid;
        std::vector<std::pair<int, int>> cells;

        for (auto obj : CCArrayExt<GameObject*>(selected)) {
            if (!obj) continue;
            if (obj->m_objectType != GameObjectType::Solid) continue;
            auto p = obj->getPosition();
            int cx = cellOf(p.x);
            int cy = cellOf(p.y);
            if (solid.insert(cellKey(cx, cy)).second) {
                cells.emplace_back(cx, cy);
            }
        }

        if (cells.empty()) {
            Notification::create("No solid blocks in selection", NotificationIcon::Warning)->show();
            return;
        }

        bool doOutline = Mod::get()->getSettingValue<bool>("outline");
        bool doCorners = Mod::get()->getSettingValue<bool>("corners");
        bool doInner   = Mod::get()->getSettingValue<bool>("inner-detail");
        float t = static_cast<float>(Mod::get()->getSettingValue<int64_t>("thickness"));
        float ts = t / GRID; // scale for a thin bar

        auto has = [&](int x, int y) { return solid.count(cellKey(x, y)) > 0; };

        // 2. Decorate each cell
        for (auto [cx, cy] : cells) {
            float x = cx * GRID + GRID / 2.f;
            float y = cy * GRID + GRID / 2.f;
            float half = GRID / 2.f;

            bool top    = !has(cx, cy + 1);
            bool bottom = !has(cx, cy - 1);
            bool left   = !has(cx - 1, cy);
            bool right  = !has(cx + 1, cy);

            if (doOutline) {
                if (top)    placePiece(x, y + half + t / 2.f, 1.f, ts);
                if (bottom) placePiece(x, y - half - t / 2.f, 1.f, ts);
                if (left)   placePiece(x - half - t / 2.f, y, ts, 1.f);
                if (right)  placePiece(x + half + t / 2.f, y, ts, 1.f);
            }

            if (doCorners) {
                float off = half + t / 2.f;
                float cs = (t * 1.6f) / GRID;
                if (top && left)     placePiece(x - off, y + off, cs, cs);
                if (top && right)    placePiece(x + off, y + off, cs, cs);
                if (bottom && left)  placePiece(x - off, y - off, cs, cs);
                if (bottom && right) placePiece(x + off, y - off, cs, cs);
            }

            if (doInner) {
                // small inset square, drawn on top of the block (created later = on top)
                placePiece(x, y, 0.45f, 0.45f);
            }
        }

        auto msg = fmt::format("Auto Design: placed {} deco objects", f->placed);
        if (f->placed >= f->cap) msg += " (cap reached)";
        Notification::create(msg, NotificationIcon::Success)->show();

        this->updateButtons();
    }
};
