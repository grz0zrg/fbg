-- a simple multithreaded example which use 8 cores, each displaying their own rectangle on screen (see glfw_lua.c)

local x = 0
local velx = 8

function draw ()
    background(0, 0, 0)

    fill(255, 0, 0)
    -- 'C_frag_id' is a C variable related to the current fragment (a fragment is a thread related to graphical tasks) 
    rect(x, 64 + C_frag_id * 17, 16, 16)

    x = x + velx
    if x < 1 or x >= C_width - 16
    then
        velx = -velx
    end
end

function compositing ()
 
end