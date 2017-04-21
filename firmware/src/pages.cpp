#include "application.h"
#include "Adafruit_SSD1306.h"
#include "buttons.h"
#include "pages.h"

// =================  Page  ====================
Page::Page(Page* prev, void (*draw)())
{
  SetPrevious(prev);
  _draw = draw;
}

void Page::SetPrevious(Page* prev)
{
  if (prev != NULL) prev->_next = this;
  _prev = prev;
}

void Page::Draw() { _draw(); }

void Page::setActive() { }

Page* Page::_onAction() { return this; }

Page* Page::HandleButton(uint8_t button)
{
  return button == BUTTON_RED ? _prev : button == BUTTON_YELLOW ? _next : _onAction();
}

// =================  Settings Page  ====================
SettingsPage::SettingsPage(Page* prev, Adafruit_SSD1306* lcd, MenuItem actions[], uint8_t action_count)
    : Page(prev, NULL) {
  _lcd = lcd;
  _actions = actions;
  _action_count = action_count;
}

void SettingsPage::setActive()
{
  _cur = -1;
}

Page* SettingsPage::HandleButton(uint8_t button)
{
  _lcd->clearDisplay();
  if (_cur < 0) return Page::HandleButton(button);
  switch (button) {
    case BUTTON_RED:
      _cur = -1;
      break;
    case BUTTON_YELLOW:
      _cur = (_cur + 1) % _action_count;
      break;
    case BUTTON_GREEN:
      if (_actions[_cur].action != NULL) _actions[_cur].action();
      _cur = -1;
      return NULL;
  }
  return this;
}

Page* SettingsPage::_onAction() { _cur = 0; return this; }

void SettingsPage::Draw()
{
  _lcd->setTextColor(WHITE);
  _lcd->setTextSize(1);
  _lcd->setCursor(0,16);
  if (_cur < 0) {
    _lcd->println("Enter for Settings");
    return;
  }

  MenuItem* s = _actions;
  for (int i=0;i<_action_count;i++)
  {
    String name = (s->name != NULL ? s->name : s->getName());
    _lcd->println((_cur == i ? "> " : "  ") + name);
    s++;
  }
}
