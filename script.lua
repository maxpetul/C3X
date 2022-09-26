ffi = require ("ffi")
ffi.cdef[[
void pop_up_in_game_error (char const * msg);
]]

function GetMagicNumber ()
	 ffi.C.pop_up_in_game_error ("Testing 1 2 3...");
	 return 7*6*5*4*3*2
end
