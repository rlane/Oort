// Copyright 2011 Rich Lane
#ifndef OORT_SIM_ENTITY_H_
#define OORT_SIM_ENTITY_H_

#include <memory>
#include <stdint.h>
#include "glm/glm.hpp"

class b2Body;

namespace Oort {

class Game;
class Team;
class ContactListener;
class Entity;

void assert_contact(const Entity &a, const Entity &b);

class Entity {
 public:
  Game *game;
  std::shared_ptr<Team> team;
  bool dead;
  float mass;
  uint32_t creator_id;

  Entity(Game *game, std::shared_ptr<Team> team, uint32_t creator_id);
  ~Entity();

  virtual void tick();
  virtual bool is_weapon() const;
  virtual bool should_collide(const Entity &e) const = 0;
  virtual uint32_t get_id() const { return (uint32_t)-1; }

  void set_position(glm::vec2 p);
  glm::vec2 get_position() const;

  void set_velocity(glm::vec2 v);
  glm::vec2 get_velocity() const;

  void set_heading(float angle);
  float get_heading() const;

  void set_angular_velocity(float w);
  float get_angular_velocity() const;

  b2Body *get_body() { return body; }

  Entity(const Entity&) = delete;
  Entity& operator=(const Entity&) = delete;

 protected:
  b2Body *body;
  friend void Oort::assert_contact(const Entity &a, const Entity &b);
};

}

#endif
