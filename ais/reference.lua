while true do
	drive_towards(100, 0, 0)
	if check_gun_ready(0) then
		local a = lead_vec(position_vec(), vec(0, 0), velocity_vec(), vec(0, 0), 3000, 10)
		if a then
			fire_gun(0, a)
		end
	end
	yield()
end
