Import("env")
import os

# Allow selecting app-specific source/include/lib from root by env options.

project_dir = env["PROJECT_DIR"]


def _as_list(opt_value):
    if not opt_value:
        return []
    if isinstance(opt_value, (list, tuple)):
        return list(opt_value)
    # comma-separated string
    return [p.strip() for p in str(opt_value).split(",") if p.strip()]


app_src = env.GetProjectOption("app_src")
if app_src:
    abs_src = os.path.join(project_dir, app_src)
    # Correct key is PROJECT_SRC_DIR (with underscores)
    env.Replace(PROJECT_SRC_DIR=abs_src)

app_inc = env.GetProjectOption("app_inc")
for inc in _as_list(app_inc):
    env.Append(CPPPATH=[os.path.join(project_dir, inc)])

app_lib = env.GetProjectOption("app_lib")
for libdir in _as_list(app_lib):
    abs_lib = os.path.join(project_dir, libdir)
    # Help PIO discover private libs
    env.Append(LIBSOURCE_DIRS=[abs_lib])
    # Also make sure the compiler can find headers within these libs
    env.Append(CPPPATH=[abs_lib])
