local my_class = class
local my_ship = ships[my_class]

_time = 0

lib()
vector()

function time()
	return _time
end

function position_vec()
	local x, y = sys_position()
	return vec(x, y)
end

function velocity_vec()
	local vx, vy = sys_velocity()
	return vec(vx, vy)
end

function NYI()
	error("NYI")
end

logbuf_head = 0
logbuf_max = 128
logbuf_lines = {}

function log(...)
	local line = string.format(...)
	logbuf_head = logbuf_head + 1
	local del = logbuf_head - logbuf_max
	if logbuf_lines[del] then logbuf_lines[del] = nil end
	logbuf_lines[logbuf_head] = line
end

function copy_table(t, t2)
	for k,v in pairs(t) do
		t2[k] = v
	end
	return t2
end

function stub_reaction_mass()
	return my_ship.reaction_mass
end

function stub_energy()
	return my_ship.energy.max
end

sandbox_api = {
	-- functions
	thrust_main = sys_thrust_main,
	thrust_lateral = sys_thrust_lateral,
	thrust_angular = sys_thrust_angular,
	position = sys_position,
	position_vec = position_vec,
	heading = sys_heading,
	velocity = sys_velocity,
	velocity_vec = velocity_vec,
	angular_velocity = sys_angular_velocity,
	reaction_mass = stub_reaction_mass,
	energy = stub_energy,
	fire = sys_fire,
	check_gun_ready = sys_check_gun_ready,
	yield = coroutine.yield,
	sensor_contacts = sys_sensor_contacts,
	sensor_contact = sys_sensor_contact,
	send = sys_send,
	recv = sys_recv,
	spawn = NYI,
	explode = sys_explode,
	debug_line = sys_debug_line,
	clear_debug_lines = sys_clear_debug_lines,
	time = time,
	log = log,

	-- values
	id = id,
	--hex_id = hex_id,
	--orders = orders,
	class = class,
	team = team,
	scenario_radius = scenario_radius,
	tick_length = tick_length,
	ships = copy_table(ships, {}),
}

function copy_table(t, t2)
	for k,v in pairs(t) do
		t2[k] = v
	end
	return t2
end

function debug_count_hook()
	if debug_preemption then
		print("preempted", team, class, hex_id)
		print(debug.traceback())
	end
end

function sandbox(f)
	local env = {
		error = error,
		assert = assert,
		ipairs = ipairs,
		next = next,
		pairs = pairs,
		print = print,
		select = select,
		tonumber = tonumber,
		tostring = tostring,
		type = type,
		unpack = unpack,

		vec = vec,
	}

	env._G = env
	env.math = copy_table(math, {})
	-- NYI
	--env.math.random = sys_random
	--env.math.randomseed = nil
	env.table = copy_table(table, {})
	env.string = copy_table(string, {})
	env.string.dump = nil

	copy_table(sandbox_api, env)

	if setfenv then
		setfenv(lib, env)
	else
		debug.setupvalue(lib, 1, env)
	end
	lib()

	strict(env._G)

	if setfenv then
		setfenv(f, env)
	else
		debug.setupvalue(f, 1, env)
	end
	return f
end
