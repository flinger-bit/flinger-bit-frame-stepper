#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include <algorithm>

using namespace geode::prelude;
using cocos2d::CCDirector;
using cocos2d::CCEvent;
using cocos2d::CCLayer;
using cocos2d::CCLayerColor;
using cocos2d::CCLabelBMFont;
using cocos2d::CCPoint;
using cocos2d::CCRect;
using cocos2d::CCTouch;

namespace {
    constexpr char const* kSettingEnabled = "enable-frame-stepper";
    constexpr char const* kSettingIntervalMs = "step-interval-ms";
    constexpr int kOverlayTag = 0xFB17;

    struct FrameStepState {
        bool holding = false;
        bool pulse = false;
        float accumulator = 0.f;
    };

    FrameStepState g_state;
}

class StepOverlayLayer final : public CCLayer {
public:
    static StepOverlayLayer* create() {
        auto ret = new StepOverlayLayer();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

protected:
    bool init() override {
        if (!CCLayer::init())
            return false;

        this->setTouchEnabled(true);
        this->scheduleUpdate();

        auto const size = CCDirector::sharedDirector()->getWinSize();

        constexpr float width = 132.f;
        constexpr float height = 46.f;
        constexpr float margin = 14.f;

        m_hitbox = CCRect(
            size.width - width - margin,
            size.height - height - margin,
            width,
            height
        );

        auto panel = CCLayerColor::create(cocos2d::ccc4(0, 0, 0, 150), width, height);
        panel->setAnchorPoint({ 0.f, 0.f });
        panel->setPosition({ m_hitbox.origin.x, m_hitbox.origin.y });
        this->addChild(panel, 1);

        auto label = CCLabelBMFont::create("FRAME STEP", "bigFont.fnt");
        label->setScale(0.35f);
        label->setPosition({ width / 2.f, height / 2.f + 4.f });
        panel->addChild(label);

        auto hint = CCLabelBMFont::create("HOLD", "goldFont.fnt");
        hint->setScale(0.25f);
        hint->setPosition({ width / 2.f, 11.f });
        panel->addChild(hint);

        return true;
    }

    void update(float dt) override {
        if (!Mod::get()->getSettingValue<bool>(kSettingEnabled)) {
            g_state.holding = false;
            g_state.pulse = false;
            g_state.accumulator = 0.f;
            return;
        }

        if (!g_state.holding)
            return;

        auto intervalMs = Mod::get()->getSettingValue<int64_t>(kSettingIntervalMs);
        intervalMs = std::clamp<int64_t>(intervalMs, 1, 1000);
        auto intervalSeconds = static_cast<float>(intervalMs) / 1000.f;

        g_state.accumulator += dt;
        while (g_state.accumulator >= intervalSeconds) {
            g_state.accumulator -= intervalSeconds;
            g_state.pulse = true;
        }
    }

    bool ccTouchBegan(CCTouch* touch, CCEvent*) override {
        if (!Mod::get()->getSettingValue<bool>(kSettingEnabled))
            return false;

        auto const point = touch->getLocation();
        if (!this->isInside(point))
            return false;

        g_state.holding = true;
        g_state.accumulator = 0.f;
        g_state.pulse = true;
        return true;
    }

    void ccTouchEnded(CCTouch*, CCEvent*) override {
        g_state.holding = false;
        g_state.accumulator = 0.f;
    }

    void ccTouchCancelled(CCTouch*, CCEvent*) override {
        g_state.holding = false;
        g_state.accumulator = 0.f;
    }

    void registerWithTouchDispatcher() override {
        CCDirector::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, 0, true);
    }

private:
    bool isInside(CCPoint const& point) const {
        return point.x >= m_hitbox.origin.x
            && point.x <= m_hitbox.origin.x + m_hitbox.size.width
            && point.y >= m_hitbox.origin.y
            && point.y <= m_hitbox.origin.y + m_hitbox.size.height;
    }

    CCRect m_hitbox;
};

class $modify(FlingerBitPlayLayer, PlayLayer) {
    void onEnter() {
        PlayLayer::onEnter();

        if (this->getChildByTag(kOverlayTag) == nullptr) {
            auto overlay = StepOverlayLayer::create();
            overlay->setTag(kOverlayTag);
            overlay->setZOrder(10'000);
            this->addChild(overlay, 10'000);
        }
    }

    void update(float dt) {
        if (!Mod::get()->getSettingValue<bool>(kSettingEnabled)) {
            PlayLayer::update(dt);
            return;
        }

        if (!g_state.pulse)
            return;

        g_state.pulse = false;
        PlayLayer::update(dt);
    }
};
