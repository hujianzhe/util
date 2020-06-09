add_rules("mode.debug", "mode.release")

if is_plat("windows") then 
    if is_mode("release") then
        add_cxflags("-MT") 
    elseif is_mode("debug") then
        add_cxflags("-MTd") 
    end
    add_ldflags("-nodefaultlib:msvcrt.lib")
end

target("util_static")
    set_kind("static")
    add_files("src/**.c")
    add_headerfiles("inc/(**.h)", {prefixdir = "util"})

target("util_dynamic")
    set_kind("shared")
    add_files("src/**.c")
    add_headerfiles("inc/(**.h)", {prefixdir = "util"})
