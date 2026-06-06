import os
from pathlib import Path
from shutil import copyfile

Import("env")

project_dir = Path(env.subst("$PROJECT_DIR")).resolve()
candidate_roots = []

if os.environ.get("PWD"):
    candidate_roots.append(Path(os.environ["PWD"]).resolve())

candidate_roots.append(Path.cwd().resolve())

project_lib_dir = project_dir / "lib"
if project_lib_dir.exists():
    candidate_roots.extend(path.resolve() for path in project_lib_dir.iterdir())

source_settings = next(
    (
        root / "examples" / "WatchFaces" / "Basic" / "settings.h"
        for root in candidate_roots
        if (root / "examples" / "WatchFaces" / "Basic" / "settings.h").exists()
    ),
    None,
)
project_src_dir = Path(env.subst("$PROJECT_SRC_DIR")).resolve()
ci_basic_sketch = project_src_dir / "Basic.ino"
ci_settings = project_src_dir / "settings.h"

if ci_basic_sketch.exists() and source_settings is not None and not ci_settings.exists():
    copyfile(source_settings, ci_settings)
