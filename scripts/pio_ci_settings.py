from pathlib import Path
from shutil import copyfile

Import("env")

project_dir = Path(env.subst("$PROJECT_DIR")).resolve()
candidate_roots = []

candidate_roots.append(Path.cwd().resolve())

project_lib_dir = project_dir / "lib"
if project_lib_dir.exists():
    candidate_roots.extend(path.resolve() for path in project_lib_dir.iterdir())

project_src_dir = Path(env.subst("$PROJECT_SRC_DIR")).resolve()
ci_sketches = list(project_src_dir.glob("*.ino"))
source_extensions = {".c", ".cpp", ".h", ".hpp", ".ino", ".s", ".S"}
source_sketch_dir = None

for ci_sketch in ci_sketches:
    for root in candidate_roots:
        for source_sketch in (root / "examples").glob(f"**/{ci_sketch.name}"):
            source_sketch_dir = source_sketch.parent
            break
        if source_sketch_dir is not None:
            break
    if source_sketch_dir is not None:
        break

if source_sketch_dir is not None:
    for source_file in source_sketch_dir.iterdir():
        if not source_file.is_file() or source_file.suffix not in source_extensions:
            continue
        target_file = project_src_dir / source_file.name
        if not target_file.exists():
            copyfile(source_file, target_file)
