#include <Geode/Geode.hpp>

using namespace geode::prelude;

enum Mode {
	None,
	Alternate,
	ClickLimit,
	SpeedMult,
	Flipped,
	Ghost,
	Accuracy,
};

cocos2d::enumKeyCodes lastKey = KEY_A;
int clickAmount = 0;
bool playedEnd = false;

Mode currentMode = Mode::ClickLimit;
int clickLimit = 30;

#include <Geode/modify/CCKeyboardDispatcher.hpp>
class $modify(AlternateCCKD, CCKeyboardDispatcher) {
	bool dispatchKeyboardMSG(cocos2d::enumKeyCodes key, bool down, bool repeat) {
		if (!down || !PlayLayer::get() || key == KEY_Escape || currentMode != Mode::Alternate) return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, repeat);

		if (key != lastKey && down) {
			lastKey = key;
			return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, repeat);
		}

		return true;
	}
};

#include <Geode/modify/CCScheduler.hpp>
class $modify(SpeedMultCCS, CCScheduler) {
	void update(float dt) {
		if (PlayLayer::get() && currentMode == Mode::SpeedMult) dt *= 1.2f;
		CCScheduler::update(dt);
	}
};

#include <Geode/modify/PlayLayer.hpp>
class $modify(FlippedPL, PlayLayer) {
	void resetLevel() {
		PlayLayer::resetLevel();
		if (currentMode == Mode::Flipped) toggleFlipped(true, true);
	}
};

#include <Geode/modify/PlayLayer.hpp>
class $modify(GhostPL, PlayLayer) {
	void resetLevel() {
		PlayLayer::resetLevel();
		if (m_player1 && currentMode == Mode::Ghost) m_player1->setVisible(false);
		if (m_player2 && currentMode == Mode::Ghost) m_player2->setVisible(false);
	}
};

#include <Geode/modify/PlayLayer.hpp>
class $modify(ClickLimitPL, PlayLayer) {
	void resetLevel() {
		PlayLayer::resetLevel();
		clickAmount = 0;
	}
};

#include <Geode/modify/PlayerObject.hpp>
class $modify(ClickLimitPO, PlayerObject) {
	void pushButton(PlayerButton btn) {
		if (!PlayLayer::get() || currentMode != Mode::ClickLimit) return PlayerObject::pushButton(btn);

		if (clickAmount < (clickLimit * 2) && !playedEnd) { // its triggering twice and i couldn't tell you why
			PlayerObject::pushButton(btn);
			clickAmount++;
		}
	}
};

#include <Geode/modify/PlayLayer.hpp>
class $modify(PlayLayer) {
	void resetLevel() {
		PlayLayer::resetLevel();
		playedEnd = false;
	}

	void postUpdate(float dt) {
		PlayLayer::postUpdate(dt);
		if (getCurrentPercentInt() > 40.f && !playedEnd) {
			playedEnd = true;
			playEndAnimationToPos({m_player1->getPositionX() - 20.f, m_player1->getPositionY()});
		}
	}

	void destroyPlayer(PlayerObject* pl, GameObject* go) {
		bool oldAutoRetr = GameManager::get()->getGameVariable("0026"); // 0026 = auto retry (just so i can spawn the retry layer)
		GameManager::get()->setGameVariable("0026", false);
		PlayLayer::destroyPlayer(pl, go);
		GameManager::get()->setGameVariable("0026", oldAutoRetr);
	}
};

#include <Geode/modify/EndLevelLayer.hpp>
class $modify(EndLevelLayer) {
	void customSetup() {
		EndLevelLayer::customSetup();
		if (playedEnd) {
			CCNode* main = getChildByID("main-layer");
			if (CCNode* node = main->getChildByID("attempts-label")) node->setVisible(false);
			if (CCNode* node = main->getChildByID("jumps-label")) node->setVisible(false);
			if (CCNode* node = main->getChildByID("time-label")) node->setVisible(false);

			CCLabelBMFont* label = CCLabelBMFont::create("S Rank Achieved!", "goldFont.fnt");
			label->setPosition(main->getChildByID("jumps-label")->getPosition());
			main->addChild(label);
		}
	}
};

#include <Geode/modify/UILayer.hpp>
class $modify(ChallengeUI, UILayer) {
	struct Fields {
		CCLabelBMFont* label = nullptr;
	};

	bool init(GJBaseGameLayer* gjbgl) {
		if (!UILayer::init(gjbgl)) return false;

		m_fields->label = CCLabelBMFont::create("No Challenge Active", "bigFont.fnt");
		m_fields->label->setPosition(CCDirector::get()->getWinSize().width / 2.f, 10.f);
		m_fields->label->setOpacity(100);
		m_fields->label->setScale(0.5f);
		addChild(m_fields->label, 100);

		this->schedule(schedule_selector(ChallengeUI::updateChallengeLabel));

		return true;
	}

	void updateChallengeLabel(float dt) {
		switch (currentMode) {
			case Mode::ClickLimit:
				m_fields->label->setString(fmt::format("Current Challenge: Click Limit ({}/{})", (int)(clickAmount / 2.f), clickLimit).c_str());
			default:
				break;
		}
	}
};

#include <Geode/modify/RetryLevelLayer.hpp>
class $modify(RetryLevelLayer) {
	void customSetup() {
		RetryLevelLayer::customSetup();

		if (currentMode != Mode::None) {
			if (CCNode* node = getChildOfType<CCLabelBMFont>(m_mainLayer, 3)) node->setVisible(false); // someone didn't do this layer's IDs properly!!!
			if (CCNode* node = getChildOfType<CCLabelBMFont>(m_mainLayer, 4)) node->setVisible(false);

			CCLabelBMFont* label = CCLabelBMFont::create("Challenge Failed!", "goldFont.fnt");
			label->setPosition(getChildOfType<CCLabelBMFont>(m_mainLayer, 4)->getPosition());
			m_mainLayer->addChild(label);
		}
	}
};