Oort
====

Introduction
------------

Oort is a programming game currently in development. Two or more space fleets,
each ship individually controlled by the player's Lua code, battle in
2-dimensional space with Newtonian physics.

- [Code](https://github.com/rlane/Oort)
- [Bug tracker](https://github.com/rlane/Oort/issues)
- [Reference AI](https://github.com/rlane/Oort/blob/master/ais/reference-classic.lua)

Compilation
-----------

Requires SDL 1.2, Lua 5.1, Box2D, GLEW, and Ruby. On Ubuntu these can be installed with:

```
sudo apt-get install libsdl1.2-dev liblua5.1-0-dev libbox2d-dev libglew-dev ruby
```

Compile with:

```
./bootstrap && ./configure && make
```

Run a battle with the reference AI:

```
./oort_sdl -s scenarios/basic.json --ai ais/reference.lua --ai ais/reference.lua
```

Gameplay
--------

Oort is a programming game, which means that after the simulation has begun the
players have no control over the outcome. Instead, you play by writing a
program (AI) that all ships on your team will individually execute.

### Victory condition

The team with the last ship alive (not counting missiles) is the winner. Ships
beyond a certain radius from the origin are not counted.

### AI

Every ship in the game is controlled by a Lua program that calls functions
provided by Oort to thrust, fire, etc. Each ship is given a timeslice per tick
and preempted when its time is up. Execution resumes where it left off on the
next tick. The ships run in independent Lua VMs and do not share any data. All
coordination must be accomplished using ship orders and the radio (which is not
yet implemented). The amount of memory that can be allocated per ship is
limited to 1 megabyte. See the ais/ directory in the distribution for sample
AI.

#### Oort API

- `position()` - returns `(x,y)`.

- `position_vec()` - returns the position vector.

- `velocity()` - returns `(vx,vy)`.

- `velocity_vec()` - returns the velocity vector.

- `thrust_main(acc)` - set the main thruster to produce the given acceleration.

- `thrust_lateral(acc)` - set the lateral thrusters to produce the given acceleration.

- `thrust_angular(acc)` - set the angular acceleration.

- `fire(name, angle)` - fire the gun named `name` in the given direction.

- `sensor_contacts()` - returns a table of all sensor contacts. See the "Sensors" section for full details.

- `sensor_contact(id)` - given an id from from `sensor_contacts`, return just that contact.

- `spawn(class, orders)` - spawn a ship.

- `yield()` - deschedule the program until the next tick.

- `explode()` - self-destruct.

- `debug_line(x1, y1, x2, y2)` - Draw a line in world coordinates for visual debugging.

- `clear_debug_lines()` - Erase all debug lines.

- `time()` - returns the time in seconds since the beginning of the simulation.

- `orders` - a string global containing the orders for this ship as set by the
spawn() function.

- `class` - a string global containing this ship's class name.

- `team` - a string global containing this ship's team's name.

- `ships` - a global table containing all the properties of each ship class,
keyed by class name.

- `scenario_radius` - a global number informing the AI how far it can go from
the origin before it is ignored when checking for victory.

- `tick_length` - length in seconds of a simulator tick (currently 1/32).

The standard `math`, `table`, and `string` libraries are provided. A library of
useful utility functions (`lib.lua`) is also included in the global
environment. The utility functions include a standard missile AI which is
useful for beginning players.

### Ships

The available ship classes are specified by the ships.lua file. This table is
available to the AI in the `ships` global. The properties of existing ships can
be changed by editing this file, but adding new ships requires adding
corresponding code in the renderer.

- Carrier: Large ship with huge energy and reaction mass reserves. Accelerates
very slowly. No guns besides a point-defense laser, but can spawn
fighters, torpedos, and missiles.

- Assault frigate: Medium-sized ship with a powerful main gun, dual smaller
  guns, and a point-defense laser. Can spawn missiles. Effective against
  carriers, frigates, or small groups of fighters.

- Ion cannon frigate: Medium-size ship with a very powerful beam weapon. The
beam can only shoot straight ahead, so this ship is not effective against fighters
or fast-moving frigates.

- Fighter: Small, highly manueverable ship. Has a 144 degree coverage main gun
and can spawn missiles. Effective against fighters, and in groups can
destroy larger ships.

- Missile: Extremely manueverable and carries a small warhead. Easily destroyed
by point-defense lasers.

- Torpedo: Slower, heavily armored missile. Carries a large warhead and can
survive defensive lasers.

#### Energy

Every ship has an energy supply with a certain recharge rate and a limited
capacity. Energy is used to fire guns, spawn ships, and thrust. If a ship
attempts an action without having the required energy it is ignored. The ship
classes vary in their energy characteristics; for example, carriers have a
large energy supply that regenerates quickly while missiles have a small energy
supply that does not regenerate at all.

#### Thrust

Each ship has three sets of thrusters: main, lateral, and angular. The main
thrusters operate parallel to the ship's heading while the lateral thrusters
are perpendicular. The maximum accelerations for each thruster are defined by
the ship's class. The engines will continue applying the given acceleration
until it is changed by a call to a thrust function or the ship's energy runs
out.

#### Spawning

A ship can call the spawn() function to create a new ship. This costs a large
amount of energy that depends on the specified class. Missiles are just another
class of ships in Oort, so to launch a missile simply spawn it with orders of
where to go. The classes of ships that can be spawned are controlled by the
'spawnable' fields in ships.lua.

#### Guns

Each ship has zero or more guns determined by its class. The guns are named,
and you pass this name to the fire() function. Guns have varying bullet
masses, bullet velocities, bullet lifetimes, reload times, and energy
costs to fire. The gun's bullet velocity is added to the ship velocity.
Whenever a bullet impacts a ship it does damage equal to its kinetic
energy relative to the ship. Some ships also have simpler hit-scan beam
weapons, which are often used to shoot down incoming missiles or fighters.

#### Hull

Hull strength varies among ship classes. When a bullet damages a ship its
relative kinetic energy is subtracted from the ship's hull strength. If a
ship's hull strength reaches zero it is destroyed.

#### Self-destruct

The explode() function causes the ship to self-destruct. An explosion is
created and the ship is destroyed. The damage done by the explosion varies with
the ship class and decreases with distance. Calling explode() should be rare
for most ship classes, but it is the typical behavior for missiles.

#### Sensors

The `sensor_contacts()` function returns a table of all the ships detected by
this ship's sensors. Each contact is an object with the methods `id`, `team`,
`class`, `position`, and `velocity`. Most of these are self explanatory. The `id`
field is an opaque string that can be passed to the `sensor_contact()` function
to return just the information for the given ship, which is significantly more
efficient.

### Determinism

The simulation is designed to be deterministic. Given the same scenario, AI,
and random seed it should play out in exactly the same way every time. There
are some limitations to this: programs that exceed their timeslice or run out
of memory will not behave identically across different Lua VMs (standard Lua vs
LuaJIT). They should still be deterministic given the same VM.

Graphical simulator
-------------------

The "oort_sdl" binary renders the battle with OpenGL. The simulation speed is
limited to real time (32 hz).

### Controls

Zoom: scroll wheel, or 'z' and 'x'

Toggle all debug graphics: 'y'

Pause: space

Single-step: enter

Toggle FPS display: 'v'

You can click on a ship to "pick" it. Data about the currently picked ship is
shown in the lower-left corner of the display, and any debug lines this ship
has drawn will be shown.

Non-graphical simulator
-----------------------

The "oort\_headless" binary runs the simulation and outputs which team won. It
isn't framerate-limited and so can run much more quickly. A future task is to
have oort\_headless output a recording of the battle that can be replayed in a
graphical viewer later.

Contributing
------------

Fork the project on GitHub and send me a merge request.
