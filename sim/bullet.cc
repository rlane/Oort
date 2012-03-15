// Copyright 2011 Rich Lane
#include "sim/bullet.h"

#include <boost/foreach.hpp>
#include <Box2D/Box2D.h>

#include "sim/ai.h"
#include "sim/team.h"
#include "sim/game.h"
#include "sim/math_util.h"
#include "sim/ship_class.h"
#include "sim/ship.h"
#include "common/log.h"

using glm::vec2;

namespace Oort {

Bullet::Bullet(Game *game,
               std::shared_ptr<Team> team,
               uint32_t creator_id,
               const GunDef &def,
                vec2 p, vec2 v)
  : team(team),
    def(def),
    creation_time(game->time),
    creator_id(creator_id),
    initial_position(p),
    velocity(v),
    dead(false) {
}

float Bullet::damage(const Ship &ship) const {
  float dv = glm::length(ship.get_velocity() - velocity);
  float e = 0.5 * def.mass * dv*dv;
  return e;
}

class BulletRayCastCallback : public b2RayCastCallback {
 public:
  const Bullet &bullet;
  Ship *ship;
  glm::vec2 point;

  BulletRayCastCallback(const Bullet &b)
    : bullet(b),
      ship(NULL) {
  }

  float ReportFixture(b2Fixture* fixture, const b2Vec2& point,
                        const b2Vec2& normal, float32 fraction) {
    auto body = fixture->GetBody();
    auto entity = (Entity*) body->GetUserData();
    auto ship = dynamic_cast<Ship*>(entity);

    if (!ship || ship->get_id() == bullet.creator_id) {
      return -1;
    }

    this->ship = ship;
    this->point = b2n(point);
    return fraction;
  }
};

void Bullet::tick_all(Game &game) {
  BOOST_FOREACH(auto &b, game.bullets) {
    if (game.time > b->creation_time + b->def.ttl) {
      b->dead = true;
    } else {
      BulletRayCastCallback callback(*b);
      auto p1 = n2b(b->get_position(game.time));
      auto p2 = n2b(b->get_position(game.time+Game::tick_length));
      game.world->RayCast(&callback, p1, p2);
      auto ship = callback.ship;
      if (ship) {
        if (ship->team != b->team) {
          auto hit = Hit { ship, NULL, callback.point, b->damage(*ship) };
          game.hits.emplace_back(hit);
        }
        b->dead = true;
      }
    }
  }
}

}
