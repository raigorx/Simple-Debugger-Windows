add_rules("mode.debug", "mode.release", "mode.asan", "mode.check", "mode.tsan", "mode.lsan" ,"mode.ubsan","mode.valgrind")
add_rules("plugin.vsxmake.autoupdate")
add_rules("plugin.compile_commands.autoupdate")

set_languages("c17", "c++20")

set_warnings("allextra")

add_cxxflags("cl::/Za")
add_defines("UNICODE")

target("debugee")
  set_kind("binary")
  add_files("debugee.c")
  add_links("user32")
  on_run(function (target)
    os.execv(".\\" .. target:targetfile(), {}, {detach = true})
  end)

target("debugger")
  set_kind("binary")
  add_files("debugger.c")
  on_run(function (target)
    os.execv(".\\" .. target:targetfile() .. " debugee.exe", {}, {detach = false})
  end)