Import("env")
env.Replace(PROGNAME="%s_%s_%s" % (env.GetProjectOption("custom_prog_name"), env.GetProjectOption("custom_prog_version"), env.GetBuildType()))
env.Append(CPPDEFINES=[("PROG_NAME", env.StringifyMacro(env.GetProjectOption("custom_prog_name")))])
env.Append(CPPDEFINES=[("PROG_VERSION", env.StringifyMacro(env.GetProjectOption("custom_prog_version")))])