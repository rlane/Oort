#include "test/testcase.h"
#include "sim/model.h"
#include <boost/bind.hpp>
#include "glm/gtx/rotate_vector.hpp"

class BenchmarkBulletAI : public CxxAI {
public:
  BenchmarkBulletAI(Ship &ship) : CxxAI(ship) {}

  void tick() {
    ship.fire_gun(0, ship.get_heading());
  }
};

class BenchmarkBulletTest : public SimpleTest {
public:
  BenchmarkBulletTest()
    : SimpleTest(Scenario(),
                 {  })
  {
    const int n = 256;
    float r = 2000;
    auto ai_factory = CxxAI::factory<BenchmarkBulletAI>();
    auto team = make_shared<Team>("green", ai_factory, vec3(0, 1, 0));
    for (int i = 0; i < n; i++) {
      auto ship = make_shared<Ship>(&*game, *fighter, team);
      auto h = 2*Oort::pi*i/n;
      auto p = glm::rotate(vec2(r, 0), h);
      ship->set_position(p);
      ship->set_heading(Oort::pi + h);
      game->ships.push_back(ship);
    }
  }

  void after_tick() {
    if (game->ticks == 128) {
      finished = true;

      log("Physics:");
      game->physics_perf.dump();
      int missed = 0;
      for (int i = 1; i < PerfHistogram::NUM_BUCKETS; i++) {
        missed += game->physics_perf.buckets[i];
      }
      log("missed target: %d", missed);
    }
  }
} test;
