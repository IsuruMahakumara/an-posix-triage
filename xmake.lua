set_languages("cxx20")
set_optimize("fastest")

target("engine")
    set_kind("binary")
    add_files("Engine.cpp")
    add_includedirs(".")

target("triage")
    set_kind("binary")
    add_files("Triage.cpp")
    add_includedirs(".")
