// Copyright 2011 Rich Lane
#include <memory>
#include "glm/glm.hpp"

namespace Oort {

class Game;
class Test;

class GUI {
public:
	static GUI *create(std::shared_ptr<Game> game, Test *test);

	virtual void start() = 0;
	virtual void stop() = 0;
	virtual void handle_keydown(int keycode) = 0;
	virtual void handle_keyup(int sym) = 0;
	virtual void handle_mousebuttondown(int button, int x, int y) = 0;
	virtual void handle_scroll(bool up) = 0;
	virtual void handle_resize(int w, int h) = 0;
	virtual void update_mouse_position(int x, int y) = 0;
	virtual void zoom(float d) = 0;
	virtual void pan(glm::vec2 d) = 0;
	virtual void render() = 0;
	virtual bool is_running() = 0;
};

}
