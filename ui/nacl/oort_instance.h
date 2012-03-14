#ifndef OORT_UI_NACL_INSTANCE_H
#define OORT_UI_NACL_INSTANCE_H

#include <unordered_set>
#include <memory>
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/graphics_3d.h"
#include "ppapi/cpp/size.h"

namespace Oort {

struct GUI;
class Game;

class OortInstance : public pp::Instance {
public:
  std::shared_ptr<Game> game;
  GUI *gui;
  pp::Graphics3D gl_context;
  std::unordered_set<uint32_t> keys_down;
  pp::Size size;

  static void static_swap_callback(void* user_data, int32_t result);
  void swap_callback();
  void schedule_swap();

  explicit OortInstance(PP_Instance instance);
  void InputInit();
  void GraphicsInit();

  virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]);
  virtual void DidChangeView(const pp::Rect& position, const pp::Rect& clip);
  virtual void HandleMessage(const pp::Var& message);
  virtual bool HandleInputEvent(const pp::InputEvent &event);
};

};

#endif
