#include "ui/nacl/oort_instance.h"
#include <stdexcept>
#include "ppapi/cpp/input_event.h"
#include "ppapi/cpp/point.h"
#include "ui/gui.h"
#include "common/log.h"

using namespace Oort;

void OortInstance::InputInit() {
  if (RequestFilteringInputEvents(PP_INPUTEVENT_CLASS_KEYBOARD|PP_INPUTEVENT_CLASS_WHEEL)) {
    throw std::runtime_error("failed to request filtering input events");
  }

  if (RequestInputEvents(PP_INPUTEVENT_CLASS_MOUSE)) {
    throw std::runtime_error("failed to request input events");
  }
}

static uint32_t convert_key(uint32_t keycode) {
  switch (keycode) {
  case 32: return ' ';
  case 13: return '\n';
  case 71: return 'g';
  case 87: return 'w';
  case 83: return 's';
  case 65: return 'a';
  case 68: return 'd';
  case 90: return 'z';
  case 88: return 'x';
  case 66: return 'b';
  case 86: return 'v';
  case 89: return 'y';
  case 78: return 'n';
  case 221: return ']';
  case 219: return '[';
  }
  log("unexpected keycode %d", keycode);
  return 0;
}

bool OortInstance::HandleInputEvent(const pp::InputEvent &event) {
  if (!gui) {
    return false;
  }

  PP_InputEvent_Type type = event.GetType();
  if (type == PP_INPUTEVENT_TYPE_KEYDOWN) {
    auto key_event = static_cast<pp::KeyboardInputEvent>(event);
    auto keycode = key_event.GetKeyCode();
    if (keys_down.count(keycode) == 0) {
      keys_down.insert(keycode);
      gui->handle_keydown(convert_key(keycode));
    }
    return true;
  } else if (type == PP_INPUTEVENT_TYPE_KEYUP) {
    auto key_event = static_cast<pp::KeyboardInputEvent>(event);
    auto keycode = key_event.GetKeyCode();
    keys_down.erase(keycode);
    gui->handle_keyup(convert_key(keycode));
    return true;
  } else if (type == PP_INPUTEVENT_TYPE_MOUSEDOWN) {
    auto mouse_event = static_cast<pp::MouseInputEvent>(event);
    auto pos = mouse_event.GetPosition();
    auto button = mouse_event.GetButton();
    int real_button = -1;
    if (button == PP_INPUTEVENT_MOUSEBUTTON_LEFT) {
      real_button = 1;
    }
    gui->handle_mousebuttondown(real_button, pos.x(), pos.y());
    return true;
  } else if (type == PP_INPUTEVENT_TYPE_WHEEL) {
    // TODO smooth scrolling
    auto wheel_event = static_cast<pp::WheelInputEvent>(event);
    gui->handle_scroll(wheel_event.GetDelta().y() > 0);
    return true;
  } else if (type == PP_INPUTEVENT_TYPE_MOUSEMOVE) {
    auto mouse_event = static_cast<pp::MouseInputEvent>(event);
    auto pos = mouse_event.GetPosition();
    gui->update_mouse_position(pos.x(), pos.y());
    return true;
  } else {
    return false;
  }
}
