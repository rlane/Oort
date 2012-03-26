// Copyright 2011 Rich Lane
#include "ui/gui.h"

#include <stdio.h>
#include <time.h>
#include <sys/timeb.h>
#include <memory>
#include <vector>
#include <string>
#include <sstream>
#include <boost/format.hpp>

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/gtx/vector_angle.hpp"

#include "sim/game.h"
#include "sim/test.h"
#include "sim/math_util.h"
#include "sim/ship.h"
#include "sim/ship_class.h"
#include "sim/team.h"
#include "renderer/renderer.h"
#include "renderer/physics_debug_renderer.h"

namespace Oort {

class GUIImpl : public GUI {
 public:
  enum class State {
    RUNNING,
    FINISHED
  };

  static constexpr float zoom_const = 2.0;
  static constexpr float pan_const = 0.6;
  static constexpr float timewarp_const = 5.0f/4;

  enum State state;
  bool running;
  bool paused;
  bool single_step;
  bool render_physics_debug;
  bool show_fps;
  float view_radius;
  float zoom_rate;
  glm::vec2 view_center;
  glm::vec2 view_speed;
  uint32_t picked_id;
  std::shared_ptr<Game> game;
  Test *test;
  std::unique_ptr<Renderer> renderer;
  std::unique_ptr<PhysicsDebugRenderer> physics_debug_renderer;
  int screen_width, screen_height;
  uint64_t last_tick_time;
  float last_time_delta;
  uint64_t render_time;
  uint64_t tick_time;
  uint64_t snapshot_time;
  glm::vec2 mouse_position;
  Team *winner;
  float tps_target;

  pthread_mutex_t tick_mutex;
  pthread_mutex_t render_mutex;
  pthread_cond_t snapshot_cond;
  pthread_t ticker;
  pthread_t snapshotter;
  FramerateCounter framerate;
  FramerateCounter tickrate;

  GUIImpl(std::shared_ptr<Game> game, Test *test)
    : state(State::RUNNING),
      running(true),
      paused(false),
      single_step(false),
      render_physics_debug(false),
      show_fps(false),
      view_radius(3000.0f),
      zoom_rate(0),
      view_center(0, 0),
      view_speed(0, 0),
      picked_id(INVALID_SHIP_ID),
      game(game),
      test(test),
      screen_width(0),
      screen_height(0),
      last_tick_time(0),
      last_time_delta(0),
      render_time(0),
      tick_time(0),
      snapshot_time(0),
      mouse_position(0, 0),
      winner(NULL),
      tps_target(32.0f) {
    pthread_mutex_init(&tick_mutex, NULL);
    pthread_mutex_init(&render_mutex, NULL);
    pthread_cond_init(&snapshot_cond, NULL);

    renderer = std::unique_ptr<Renderer>(new Renderer());
    renderer->snapshot(*game);
    last_tick_time = microseconds();

    physics_debug_renderer =
      std::unique_ptr<PhysicsDebugRenderer>(new PhysicsDebugRenderer());
    physics_debug_renderer->SetFlags(b2Draw::e_shapeBit);
    game->world->SetDebugDraw(physics_debug_renderer.get());

    if (test) {
      paused = true;
    }
  }

  void start() {
    pthread_create(&ticker, NULL, GUIImpl::static_ticker_func, this);
    pthread_create(&snapshotter, NULL, GUIImpl::static_snapshotter_func, this);
  }

  void stop() {
    running = false;
    pthread_join(ticker, NULL);
    pthread_join(snapshotter, NULL);
  }

  void handle_keydown(int keycode) {
    switch (keycode) {
    case 27:
      stop();
      break;
    case ' ':
      paused = !paused;
      break;
    case '\n':
      paused = false;
      single_step = true;
      break;
    case 'g':
      render_physics_debug = !render_physics_debug;
      break;
    case 'w':
      view_speed.y += pan_const;
      break;
    case 's':
      view_speed.y -= pan_const;
      break;
    case 'a':
      view_speed.x -= pan_const;
      break;
    case 'd':
      view_speed.x += pan_const;
      break;
    case 'z':
      zoom_rate -= zoom_const;
      break;
    case 'x':
      zoom_rate += zoom_const;
      break;
    case 'b':
      show_fps = renderer->benchmark = !renderer->benchmark;
      break;
    case 'v':
      show_fps = !show_fps;
      break;
    case 'y':
      renderer->render_all_debug_lines = !renderer->render_all_debug_lines;
      break;
    case 'n':
      log("%0.2f fps", framerate.hz);
      log("%0.2f tps", tickrate.hz);
      log("");
      log("Simulation performance:");
      log("physics: %s", game->physics_perf.summary().c_str());
      log("ai:      %s", game->ai_perf.summary().c_str());
      log("");
      log("Renderer performance:");
      renderer->dump_perf();
      log("");
      log("Physics:");
      game->physics_perf.dump();
      log("");
      break;
    case ']':
      tps_target *= timewarp_const;
      break;
    case '[':
      tps_target *= 1.0/timewarp_const;
      break;
    default:
      break;
    }
  }

  void handle_keyup(int sym) {
    switch (sym) {
    case 'w':
      view_speed.y -= pan_const;
      break;
    case 's':
      view_speed.y += pan_const;
      break;
    case 'a':
      view_speed.x += pan_const;
      break;
    case 'd':
      view_speed.x -= pan_const;
      break;
    case 'z':
      zoom_rate += zoom_const;
      break;
    case 'x':
      zoom_rate -= zoom_const;
      break;
    default:
      break;
    }
  }

  class PickCallback : public b2QueryCallback {
  public:
    b2Vec2 center;
    uint32_t found_id;

    PickCallback(b2Vec2 center)
    : center(center),
      found_id(INVALID_SHIP_ID) {}

    bool ReportFixture(b2Fixture *fixture) {
      auto body = fixture->GetBody();
      auto entity = static_cast<Entity*>(body->GetUserData());
      auto ship = dynamic_cast<Ship*>(entity);
      if (ship && !ship->dead && fixture->TestPoint(center)) {
        found_id = ship->id;
      }
      return true;
    }
  };

  void handle_mousebuttondown(int button, int x, int y) {
    if (button == 1) {
      auto c = screen2world(glm::vec2(x, y));
      glm::vec2 size(1, 1);
      b2AABB aabb;
      aabb.lowerBound = n2b(c - size);
      aabb.upperBound = n2b(c + size);
      PickCallback picker(n2b(c));
      game->world->QueryAABB(&picker, aabb);
      if (picker.found_id != INVALID_SHIP_ID) {
        picked_id = picker.found_id;
      } else {
        picked_id = INVALID_SHIP_ID;
      }
    }
  }

  void handle_scroll(bool up) {
    zoom(up ? -0.1 : 0.1);
  }

  void handle_resize(int w, int h) {
    screen_width = w;
    screen_height = h;
    glViewport(0, 0, w, h);
    renderer->reshape(w, h);
    physics_debug_renderer->reshape(w, h);
  }

  glm::vec2 screen2world(glm::vec2 screen_pos) {
    screen_pos -= glm::vec2(screen_width, screen_height) * 0.5f;
    screen_pos.y = -screen_pos.y;
    return view_center + screen_pos * (2*view_radius/screen_width);
  }

  void update_mouse_position(int x, int y) {
    mouse_position = glm::vec2(x, y);
  }

  void zoom(float d) {
    if (d < 0) {
      auto p = screen2world(mouse_position);
      auto dp = -d * (p - view_center);
      view_center += dp;
    }
    view_radius *= (1 + d);
  }

  void pan(glm::vec2 d) {
    view_center += d*view_radius;
  }

  void render() {
    Timer timer;

    zoom(zoom_rate/framerate.hz);
    pan(view_speed/framerate.hz);

    std::ostringstream tmp;
    if (paused) {
      tmp << "(paused) tick " << game->ticks;
    } else {
      tmp << "tick " << game->ticks;
    }
    renderer->text(screen_width-tmp.str().length()*9-3, screen_height-10,
                   tmp.str());

    if (test) {
      if (state == State::RUNNING) {
        renderer->text(screen_width-120, screen_height-22, "test running");
      } else if (state == State::FINISHED) {
        renderer->text(screen_width-120, screen_height-22, "test finished");
      }
    } else {
      if (state == State::FINISHED) {
        if (winner != NULL) {
          using boost::str;
          using boost::format;
          renderer->text(screen_width/2-85, screen_height/3,
                         str(format("%s team wins") % winner->name.c_str()));
        } else {
          renderer->text(screen_width/2-15, screen_height/3, "tie");
        }
      }
    }

    if (picked_id != INVALID_SHIP_ID) {
      auto ship = game->lookup_ship(picked_id);
      if (ship && !ship->dead) {
        const int x = 15;
        const int dy = 12;
        const int y = screen_height - 5*dy - 3;
        std::ostringstream tmp;
        auto p = ship->get_position();
        auto h = ship->get_heading();
        auto v = ship->get_velocity();
        tmp << ship->klass.name << " " << ship->id;
        renderer->text(x, y+0*dy, tmp.str());
        tmp.str("");
        tmp << "position: (" << p.x << "," << p.y << ")";
        renderer->text(x, y+1*dy, tmp.str());
        tmp.str("");
        tmp << "heading: " << h;
        renderer->text(x, y+2*dy, tmp.str());
        tmp.str("");
        tmp << "velocity: (" << v.x << "," << v.y << ")";
        renderer->text(x, y+3*dy, tmp.str());
        tmp.str("");
        tmp << "hull: " << ship->hull;
        renderer->text(x, y+4*dy, tmp.str());
        tmp.str("");
      } else {
        picked_id = INVALID_SHIP_ID;
      }
    }

    if (show_fps) {
      using boost::str;
      using boost::format;
      renderer->text(screen_width-180, 10,
                     str(format("     fps: %0.2f") % framerate.hz));
      renderer->text(screen_width-180, 20,
                     str(format("     tps: %0.2f") % tickrate.hz));
    }

    if (renderer->benchmark) {
      using boost::str;
      using boost::format;
      renderer->text(screen_width-180, 30,
                     str(format("  render: %0.2f ms") % (render_time/1e3)));
      renderer->text(screen_width-180, 40,
                     str(format("    tick: %0.2f ms") % (tick_time/1e3)));
      renderer->text(screen_width-180, 50,
                     str(format("snapshot: %0.2f ms") % (snapshot_time/1e3)));
    }

    {
      pthread_mutex_lock(&render_mutex);

      if (render_physics_debug) {
        auto p = screen2world(mouse_position);
        std::ostringstream tmp;
        tmp << "mouse position: " << glm::to_string(p);
        renderer->text(5, 9, tmp.str());
      }

      float time_delta = float(microseconds() - last_tick_time)/(1000*1000);
      if (time_delta > 0.1f) time_delta = 0;
      renderer->render(view_radius, view_center,
                       paused ? last_time_delta : time_delta);
      if (!paused) {
        last_time_delta = time_delta;
      }

      if (render_physics_debug) {
        physics_debug_renderer->begin_render(view_radius, view_center);
        for (const b2Body *body = game->world->GetBodyList();
             body; body = body->GetNext()) {
          physics_debug_renderer->DrawPoint(body->GetWorldCenter(), 2,
                                            b2Color(0.9, 0.4, 0.3));
          physics_debug_renderer->DrawPoint(body->GetPosition(), 2,
                                            b2Color(0.3, 0.4, 0.9));
        }
        physics_debug_renderer->DrawCircle(b2Vec2(0, 0),
                                           game->radius/Oort::SCALE,
                                           b2Color(0.6, 0.8, 0.6));
        game->world->DrawDebugData();
        physics_debug_renderer->end_render();
      }

      pthread_mutex_unlock(&render_mutex);
    }

    framerate.update();
    render_time = timer.elapsed();
  }

  static void *static_ticker_func(void *arg) {
    auto gui = static_cast<GUIImpl*>(arg);
    gui->ticker_func();
    return NULL;
  }

  void ticker_func() {
    while (running) {
      Timer timer;
      uint64_t target_time = 1e6/tps_target;

      if (!paused) {
        if (single_step) {
          single_step = false;
          paused = true;
        }

        if (state == State::RUNNING) {
          if (test) {
            if (test->finished) {
              printf("Test finished\n");
              state = State::FINISHED;
            }
          } else {
            if (game->check_victory(winner)) {
              if (winner == NULL) {
                printf("Tie\n");
              } else {
                printf("%s team wins\n", winner->name.c_str());
              }
              state = State::FINISHED;
            }
          }
        }

        {
          pthread_mutex_lock(&tick_mutex);

          game->tick();

          if (test) {
            test->after_tick();
          }

          pthread_mutex_unlock(&tick_mutex);
        }

        pthread_cond_broadcast(&snapshot_cond);
      }

      tickrate.update();
      last_tick_time = microseconds();
      tick_time = timer.elapsed();
      int remaining = int(target_time - tick_time);
      usleep(glm::max(remaining, 1000));
    }

    pthread_cond_broadcast(&snapshot_cond);
  }

  static void *static_snapshotter_func(void *arg) {
    auto gui = static_cast<GUIImpl*>(arg);
    gui->snapshotter_func();
    return NULL;
  }

  void snapshotter_func() {
    pthread_mutex_lock(&render_mutex);

    while (pthread_cond_wait(&snapshot_cond, &render_mutex) == 0 && running) {
      pthread_mutex_lock(&tick_mutex);
      Timer timer;
      renderer->snapshot(*game);
      snapshot_time = timer.elapsed();
      pthread_mutex_unlock(&tick_mutex);
    }

    pthread_mutex_unlock(&render_mutex);
  }

  bool is_running() {
    return running;
  }
};

GUI *GUI::create(std::shared_ptr<Game> game, Test *test) {
  return new GUIImpl(game, test);
}

}

