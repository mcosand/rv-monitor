#include "application.h"
#include "Adafruit_SSD1306.h"

class Page {
 public:
  Page(Page* prev, void(*draw)());
  virtual Page* HandleButton(uint8_t button);
  virtual void Draw();

  void SetPrevious(Page* prev);
 protected:
  Page* _prev;
  Page* _next;
  void (*_draw)();
  virtual Page* _onAction();
};

class SettingsPage : public Page {
  public:
    virtual Page* HandleButton(uint8_t button);
    virtual void Draw();

   SettingsPage(Page* prev, Adafruit_SSD1306* lcd, String names[], uint8_t name_count);
   void SetPrevious(Page* prev);
  protected:
   Page* _prev;
   Page* _next;

   Adafruit_SSD1306* _lcd;
   String* _names;
   uint8_t _name_count;
   int8_t _cur = -1;

   virtual Page* _onAction();
};
