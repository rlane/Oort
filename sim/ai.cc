// Copyright 2011 Rich Lane
#include "sim/ai.h"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/normal_distribution.hpp>

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include "glm/glm.hpp"
#include "glm/gtx/vector_angle.hpp"
#include <Box2D/Box2D.h>

#include "sim/game.h"
#include "sim/ship.h"
#include "sim/ship_class.h"
#include "sim/team.h"
#include "sim/bullet.h"
#include "common/log.h"
#include "common/resources.h"

using glm::vec2;

namespace Oort {

AI::AI(Ship &ship) : ship(ship) {}
void AI::tick() {}

CxxAI::CxxAI(Ship &ship) : AI(ship) {}
void CxxAI::tick() {}

static void push_vector(lua_State *L, vec2 v) {
  lua_createtable(L, 2, 0);

  lua_pushnumber(L, v.x);
  lua_rawseti(L, -2, 1);

  lua_pushnumber(L, v.y);
  lua_rawseti(L, -2, 2);

  lua_getglobal(L, "vector_metatable"); // XXX optimize
  lua_setmetatable(L, -2);
}

struct LuaSensorContact { 
  const uint32_t magic;
  const uint32_t id;
  const Team &team;
  const ShipClass &klass;
  const vec2 p;
  const vec2 v;

  static const uint32_t RKEY = 0xAABBCC01;

  static int api_id(lua_State *L) {
    auto c = cast(L, 1);
    lua_pushnumber(L, c->id);
    return 1;
  }

  static int api_team(lua_State *L) {
    auto c = cast(L, 1);
    lua_pushstring(L, c->team.name.c_str());
    return 1;
  }

  static int api_class(lua_State *L) {
    auto c = cast(L, 1);
    lua_pushstring(L, c->klass.name.c_str());
    return 1;
  }

  static int api_position(lua_State *L) {
    auto c = cast(L, 1);
    lua_pushnumber(L, c->p.x);
    lua_pushnumber(L, c->p.y);
    return 2;
  }

  static int api_position_vec(lua_State *L) {
    auto c = cast(L, 1);
    push_vector(L, c->p);
    return 1;
  }

  static int api_velocity(lua_State *L) {
    auto c = cast(L, 1);
    lua_pushnumber(L, c->v.x);
    lua_pushnumber(L, c->v.y);
    return 2;
  }

  static int api_velocity_vec(lua_State *L) {
    auto c = cast(L, 1);
    push_vector(L, c->v);
    return 1;
  }

  static void register_api(lua_State *L) {
    lua_pushlightuserdata(L, (void*)RKEY);
    lua_createtable(L, 0, 1);

    lua_pushstring(L, "__index");
    lua_createtable(L, 0, 7);

    lua_pushstring(L, "id");
    lua_pushcfunction(L, api_id);
    lua_settable(L, -3);

    lua_pushstring(L, "team");
    lua_pushcfunction(L, api_team);
    lua_settable(L, -3);

    lua_pushstring(L, "class");
    lua_pushcfunction(L, api_class);
    lua_settable(L, -3);

    lua_pushstring(L, "position");
    lua_pushcfunction(L, api_position);
    lua_settable(L, -3);

    lua_pushstring(L, "position_vec");
    lua_pushcfunction(L, api_position_vec);
    lua_settable(L, -3);

    lua_pushstring(L, "velocity");
    lua_pushcfunction(L, api_velocity);
    lua_settable(L, -3);

    lua_pushstring(L, "velocity_vec");
    lua_pushcfunction(L, api_velocity_vec);
    lua_settable(L, -3);

    lua_settable(L, -3);
    lua_settable(L, LUA_REGISTRYINDEX);
  }

  static void push_metatable(lua_State *L) {
    lua_pushlightuserdata(L, (void*)RKEY);
    lua_gettable(L, LUA_REGISTRYINDEX);
  }

  static void push(lua_State *L, const Ship &ship, int metatable_idx) {
    void *ud = lua_newuserdata(L, sizeof(LuaSensorContact));
    lua_pushvalue(L, metatable_idx);
    lua_setmetatable(L, -2);
    new (ud) LuaSensorContact(ship);
  }

  static LuaSensorContact *cast(lua_State *L, int narg) {
    auto c = static_cast<LuaSensorContact*>(lua_touserdata(L, narg));
    if (c != NULL && c->magic != RKEY) c = NULL;
    if (c == NULL) luaL_argerror(L, narg, "sensor contact expected");
    return c;
  }

  LuaSensorContact(const Ship &ship)
    : magic(RKEY),
      id(ship.id),
      team(*ship.team),
      klass(ship.klass),
      p(ship.get_position()),
      v(ship.get_velocity())
  {
  }
};

LuaAIFactory::LuaAIFactory(std::string filename, std::string code)
  : AIFactory(),
    filename(filename),
    code(code)
{
}

std::unique_ptr<AI> LuaAIFactory::instantiate(Ship &ship) {
  return std::unique_ptr<AI>(new LuaAI(ship, filename, code));
}

static void *AI_RKEY = (void*)0xAABBCC02;

LuaAI::LuaAI(Ship &ship, std::string filename, std::string code)
  : AI(ship), dead(false)
{
  G = luaL_newstate();
  if (!G) {
    throw std::runtime_error("Failed to create Lua state");
  }

  lua_pushlightuserdata(G, AI_RKEY);
  lua_pushlightuserdata(G, this);
  lua_settable(G, LUA_REGISTRYINDEX);

  luaL_openlibs(G);
  register_api();

  std::vector<std::string> libs = { "lib", "strict", "vector" };
  BOOST_FOREACH(auto &lib, libs) {
    std::string filename = lib + ".lua";
    auto lib_code = load_resource(filename);
    luaL_loadbuffer(G, lib_code.c_str(), lib_code.length(), filename.c_str());
    lua_setglobal(G, lib.c_str());
  }

  std::vector<std::string> libs2 = { "ships", "runtime" };
  BOOST_FOREACH(auto &lib, libs2) {
    std::string filename = lib + ".lua";
    auto lib_code = load_resource(filename);
    luaL_loadbuffer(G, lib_code.c_str(), lib_code.length(), filename.c_str());
    lua_call(G, 0, 0);
  }

  L = lua_newthread(G);

  lua_getglobal(L, "sandbox");

  if (luaL_loadbuffer(L, code.c_str(), code.length(), filename.c_str())) {
    throw std::runtime_error("Failed to load Lua AI"); // XXX message
  }

  lua_call(L, 1, 1);
}

LuaAI::~LuaAI() {
  if (G) {
    lua_close(G);
  }
}

void LuaAI::tick() {
  if (dead) {
    return;
  }

  lua_pushnumber(G, ship.game->time);
  lua_setglobal(G, "_time");
  auto result = lua_resume(L, 0);
  if (result == 0) {
    throw std::runtime_error("AI exited");
  } else if (result == LUA_YIELD) {
  } else {
    log("ship %#x error %s", ship.id, lua_tostring(L, -1));
    log("backtrace:");
    lua_Debug ar;
    for (int i = 0; lua_getstack(L, i, &ar); i++) {
      if (!lua_getinfo(L, "nSl", &ar)) {
        log("  %d: error", i);
      } else {
        log("  %d: %s %s %s @ %s:%d", i, ar.what, ar.namewhat, ar.name, ar.short_src, ar.currentline);
      }
    }
    dead = true;
  }
}

static LuaAI &lua_ai(lua_State *L) {
  lua_pushlightuserdata(L, AI_RKEY);
  lua_gettable(L, LUA_REGISTRYINDEX);
  void *v = lua_touserdata(L, -1);
  lua_pop(L, 1);
  return *(LuaAI*)v;
}

int api_position(lua_State *L) {
  auto &ship = lua_ai(L).ship;
  auto p = ship.get_position();
  lua_pushnumber(L, p.x);
  lua_pushnumber(L, p.y);
  return 2;
}

int api_velocity(lua_State *L) {
  auto &ship = lua_ai(L).ship;
  auto v = ship.get_velocity();
  lua_pushnumber(L, v.x);
  lua_pushnumber(L, v.y);
  return 2;
}

int api_heading(lua_State *L) {
  auto &ship = lua_ai(L).ship;
  lua_pushnumber(L, ship.get_heading());
  return 1;
}

int api_angular_velocity(lua_State *L) {
  auto &ship = lua_ai(L).ship;
  lua_pushnumber(L, ship.get_angular_velocity());
  return 1;
}

int api_acc_main(lua_State *L) {
  auto &ship = lua_ai(L).ship;
  float a = luaL_checknumber(L, 1);
  ship.acc_main(a);
  return 0;
}

int api_acc_lateral(lua_State *L) {
  auto &ship = lua_ai(L).ship;
  float a = luaL_checknumber(L, 1);
  ship.acc_lateral(a);
  return 0;
}

int api_acc_angular(lua_State *L) {
  auto &ship = lua_ai(L).ship;
  float a = luaL_checknumber(L, 1);
  ship.acc_angular(a);
  return 0;
}

static int find_gun(const Ship &ship, const char *name) {
  for (unsigned int i = 0; i < ship.klass.guns.size(); i++) {
    if (!strcmp(name, ship.klass.guns[i].name.c_str())) {
      return i;
    }
  }
  return -1;
}

static int find_beam(const Ship &ship, const char *name) {
  for (unsigned int i = 0; i < ship.klass.beams.size(); i++) {
    if (!strcmp(name, ship.klass.beams[i].name.c_str())) {
      return i;
    }
  }
  return -1;
}

int api_fire(lua_State *L) {
  auto &ship = lua_ai(L).ship;
  const char *name = luaL_checkstring(L, 1);
  float a = luaL_checknumber(L, 2);
  int idx;

  if ((idx = find_gun(ship, name)) != -1) {
    ship.fire_gun(idx, a);
  } else if ((idx = find_beam(ship, name)) != -1) {
    ship.fire_beam(idx, a);
  } else {
    return luaL_argerror(L, 1, "no such gun");
  }

  return 0;
}

int api_check_gun_ready(lua_State *L) {
  auto &ship = lua_ai(L).ship;
  const char *name = luaL_checkstring(L, 1);
  int idx = find_gun(ship, name);
  if (idx == -1) {
    return luaL_argerror(L, 1, "no such gun");
  }
  lua_pushboolean(L, ship.gun_ready(idx));
  return 1;
}

void push_sensor_contact(lua_State *L, const std::shared_ptr<Ship> contact) {
  lua_createtable(L, 0, 4);
  int table_idx = lua_gettop(L);
  auto p = contact->get_position();

  lua_pushliteral(L, "id");
  lua_pushnumber(L, (double)contact->id);
  lua_settable(L, table_idx);

  lua_pushliteral(L, "x");
  lua_pushnumber(L, p.x);
  lua_settable(L, table_idx);

  lua_pushliteral(L, "y");
  lua_pushnumber(L, p.y);
  lua_settable(L, table_idx);

  lua_pushliteral(L, "team");
  lua_pushstring(L, contact->team->name.c_str());
  lua_settable(L, table_idx);
}

// TODO parse query
int api_sensor_contacts(lua_State *L) {
  auto &ship = lua_ai(L).ship;
  lua_newtable(L);
  int table_idx = lua_gettop(L);
  int i = 1;
  LuaSensorContact::push_metatable(L);
  int mt_idx = lua_gettop(L);
  int query_argnum = 1;

  float has_distance_lt = false;
  float distance_lt = 0;
  bool has_enemy = false;
  bool enemy = false;
  int limit = 10000;

  if (lua_istable(L, query_argnum)) {
    lua_getfield(L, query_argnum, "distance_lt");
    if (lua_isnumber(L, -1)) {
      has_distance_lt = true;
      distance_lt = lua_tonumber(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, query_argnum, "enemy");
    if (lua_isboolean(L, -1)) {
      has_enemy = true;
      enemy = lua_toboolean(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, query_argnum, "limit");
    if (lua_isnumber(L, -1)) {
      limit = lua_tointeger(L, -1);
    }
    lua_pop(L, 1);
  } else if (lua_isnil(L, query_argnum)) {
  } else {
    luaL_argerror(L, query_argnum, "expected table or nil");
  }

  BOOST_FOREACH(auto &contact, ship.game->ships) {
    if (i > limit) break;
    if (has_enemy && (contact->team != ship.team) != enemy) continue;
    if (has_distance_lt &&
        glm::distance(contact->get_position(), ship.get_position()) > distance_lt) continue;
    LuaSensorContact::push(L, *contact, mt_idx);
    lua_rawseti(L, table_idx, i);
    i += 1;
  }
  lua_pop(L, 1);
  return 1;
}

int api_sensor_contact(lua_State *L) {
  auto &ship = lua_ai(L).ship;
  uint32_t id = luaL_checkinteger(L, 1);
  LuaSensorContact::push_metatable(L);
  int mt_idx = lua_gettop(L);
  BOOST_FOREACH(auto &contact, ship.game->ships) {
    if (contact->id == id) {
      LuaSensorContact::push(L, *contact, mt_idx);
      return 1;
    }
  }
  return 0;
}

// NYI
int api_send(lua_State *L) {
  return 0;
}

// NYI
int api_recv(lua_State *L) {
  return 0;
}

int api_spawn(lua_State *L) {
  auto &ship = lua_ai(L).ship;
  const char *klass_name = luaL_checkstring(L, 1);
  const char *orders = luaL_optstring(L, 2, "");
  auto &klass = ShipClass::lookup(klass_name); // XXX handle failure
  auto new_ship = std::make_shared<Ship>(ship.game, klass, ship.team, ship.id, orders);
  new_ship->set_position(ship.get_position());
  new_ship->set_heading(ship.get_heading());
  new_ship->set_velocity(ship.get_velocity());
  ship.game->ships.push_back(new_ship);
  return 0;
}

int api_explode(lua_State *L) {
  auto &ship = lua_ai(L).ship;
  ship.explode();
  lua_yield(L, 0);
  return 0;
}

// XXX limit number of lines
int api_debug_line(lua_State *L) {
  auto &ship = lua_ai(L).ship;
  float x1 = (float) luaL_checknumber(L, 1);
  float y1 = (float) luaL_checknumber(L, 2);
  float x2 = (float) luaL_checknumber(L, 3);
  float y2 = (float) luaL_checknumber(L, 4);
  ship.debug_line(vec2(x1, y1), vec2(x2, y2));
  return 0;
}

int api_clear_debug_lines(lua_State *L) {
  auto &ship = lua_ai(L).ship;
  ship.clear_debug_lines();
  return 0;
}

void LuaAI::register_api() {
  lua_register(G, "sys_position", api_position);
  lua_register(G, "sys_velocity", api_velocity);
  lua_register(G, "sys_heading", api_heading);
  lua_register(G, "sys_angular_velocity", api_angular_velocity);
  lua_register(G, "sys_thrust_main", api_acc_main);
  lua_register(G, "sys_thrust_lateral", api_acc_lateral);
  lua_register(G, "sys_thrust_angular", api_acc_angular);
  lua_register(G, "sys_fire", api_fire);
  lua_register(G, "sys_check_gun_ready", api_check_gun_ready);
  lua_register(G, "sys_sensor_contacts", api_sensor_contacts);
  lua_register(G, "sys_sensor_contact", api_sensor_contact);
  lua_register(G, "sys_send", api_send);
  lua_register(G, "sys_recv", api_recv);
  lua_register(G, "sys_spawn", api_spawn);
  lua_register(G, "sys_explode", api_explode);
  lua_register(G, "sys_debug_line", api_debug_line);
  lua_register(G, "sys_clear_debug_lines", api_clear_debug_lines);

  lua_pushnumber(G, ship.id);
  lua_setglobal(G, "id");

  lua_pushstring(G, ship.orders.c_str());
  lua_setglobal(G, "orders");

  lua_pushstring(G, ship.klass.name.c_str());
  lua_setglobal(G, "class");

  lua_pushstring(G, ship.team->name.c_str());
  lua_setglobal(G, "team");

  lua_pushnumber(G, ship.game->radius);
  lua_setglobal(G, "scenario_radius");

  lua_pushnumber(G, Game::tick_length);
  lua_setglobal(G, "tick_length");

  LuaSensorContact::register_api(G);
}

}
