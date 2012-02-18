function find_target()
	for i, t in ipairs(sensor_contacts({})) do
		if t.team ~= team then
			return t
		end
	end
	return nil
end

while true do
	print(time())
	clear_debug_lines()
	local target = find_target()
	if target then
		debug_square(target.x, target.y, 20)
		drive_towards(100, target.x, target.y)
		if check_gun_ready(0) then
			local a = lead_vec(position_vec(), vec(target.x, target.y), velocity_vec(), vec(0, 0), 3000, 10)
			if a then
				fire_gun(0, a)
			end
		end
	else
		drive_towards(100, 0, 0)
	end
	yield()
end
