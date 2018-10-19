#pragma once
#include "bnActivity.h"
#include "bnSegue.h"
#include "bnTimer.h"
#include "bnEngine.h"
#include <SFML/Graphics.hpp>
#include <stack>

class ActivityManager {
  friend class Segue;

private:
  std::stack<Activity*> activities;
  sf::RenderTexture* surface;

public:
  ActivityManager() {
    surface = new sf::RenderTexture();
    surface->create((unsigned int)ENGINE.view.getSize().x, (unsigned int)ENGINE.view.getSize().y); 
  }

  ~ActivityManager() {
    while (!activities.empty()) {
      Activity* activity = activities.top();
      activity->OnEnd();
      delete activity;
      activities.pop();
    }

    delete surface;
  }

  template<typename T, typename DurationType=Duration<&sf::milliseconds, 500>>
  class Segue {
  public:
    template<typename U>
    static void To() {
      bool hasLast = (SceneManager::GetInstance().activities.size() > 0);
      Activity* last = hasLast ? SceneManager::GetInstance().activities.top() : nullptr;
      Activity* next = new U();
      ::Segue* effect = new T(DurationType::value(), last, next);

      effect->OnStart();

      activities.push(effect);
    }
  };

  template<typename T>
  class JumpTo {
    JumpTo() {
      bool hasLast = (SceneManager::GetInstance().activities.size() > 0);
      Activity* last = hasLast ? SceneManager::GetInstance().activities.top() : nullptr;
      Activity* next = new U();
      last->OnEnd();

      activities.pop();
      delete last;

      next->OnStart();
      activities.push(effect);
    }
  };

private:
  void Update(double _elapsed) {
    if (activities.size() == 0)
      return;

    activities.top()->OnUpdate(_elapsed);
  }

  void Draw(sf::RenderWindow& window) {
    if (activities.size() == 0)
      return;

    activities.top()->OnDraw(*surface);

    surface->display();

    // Capture buffer in a drawable context
    sf::Sprite post(surface->getTexture());

    // drawbuffer on top of the scene
    window.draw(post);

    // show final result
    window.display();

    // Prepare buffer for next cycle
    surface->clear(sf::Color::Transparent);
  }

};