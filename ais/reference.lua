local my_class = class
local my_ship = ships[my_class]

function find_target()
	for i, t in ipairs(sensor_contacts({})) do
		if t:team() ~= team then
			return t
		end
	end
	return nil
end

local target_id = nil
while true do
	clear_debug_lines()

	local target = nil
	if target_id then
		target = sensor_contact(target_id)
	end

	if not target then
		target = find_target()
		if target then target_id = target:id() end
		print("new target ", target_id)
	end

	if target then
		local tp = target:position_vec()
		debug_square(tp.x, tp.y, 20)
		drive_towards(100, tp.x, tp.y)

		if class == "ion_cannon_frigate" then
			local p = position_vec()
			if p:distance(tp) < my_ship.guns.main.length then
				fire("main", heading())
			end
		else
			if check_gun_ready("main") then
				local a = lead_vec(position_vec(), target:position_vec(), velocity_vec(), target:velocity_vec(), 3000, 10)
				if a then
					fire("main", a)
				end
			end
		end
	else
		drive_towards(100, 0, 0)
	end
	yield()
end
