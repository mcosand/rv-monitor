#include "application.h"
#include "Adafruit_SSD1306.h"
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

Page* Page::_onAction() { return this; }

Page* Page::HandleButton(uint8_t button)
{
  return button == 0 ? _prev : button == 1 ? _next : _onAction();
}

// =================  Settings Page  ====================
SettingsPage::SettingsPage(Page* prev, Adafruit_SSD1306* lcd, String names[], uint8_t name_count)
    : Page(prev, NULL) {
  _lcd = lcd;
  _names = names;
  _name_count = name_count;
}

Page* SettingsPage::HandleButton(uint8_t button)
{
  _lcd->clearDisplay();
  if (_cur < 0) return Page::HandleButton(button);
  switch (button) {
    case 0:
      _cur = -1;
      break;
    case 1:
      _cur = (_cur + 1) % _name_count;
      break;
    case 2:
      _cur = -1;
      return NULL;
  }
  return this;
}

Page* SettingsPage::_onAction() { _cur = 0; return this; }

void SettingsPage::Draw()
{
  _lcd->setTextColor(WHITE);
  _lcd->setTextSize(2);
  if (_cur < 0) {
    _lcd->setCursor(0,16);
    _lcd->println("Enter for Settings");
    return;
  }

  _lcd->setCursor(0,0);
  String* s = _names;
  for (int i=0;i<_name_count;i++)
  {
    _lcd->println((_cur == i ? ">" : " ") + *s++);
  }
}
