import shutil
import subprocess
from pathlib import Path


def find_bin():
    for name in ("awww", "swww"):
        p = shutil.which(name)
        if p:
            return p
    return None


def find_daemon():
    for name in ("awww-daemon", "swww-daemon"):
        p = shutil.which(name)
        if p:
            return p
    return None


def is_daemon_running():
    # check via pgrep or awww query
    import shutil as _sh

    if _sh.which("pgrep"):
        try:
            r = subprocess.run(["pgrep", "-x", "awww-daemon"], capture_output=True)
            if r.returncode == 0:
                return True
            r = subprocess.run(["pgrep", "-x", "swww-daemon"], capture_output=True)
            if r.returncode == 0:
                return True
        except Exception:
            pass
    # fallback: try query
    bin_path = find_bin()
    if bin_path:
        try:
            r = subprocess.run([bin_path, "query"], capture_output=True, timeout=2)
            return r.returncode == 0
        except Exception:
            return False
    return False


def apply_wallpaper(path: Path, transition_type="grow", transition_pos="0.5,0.5", transition_duration=1.2, transition_fps=60, outputs=None, extra_args=None):
    bin_path = find_bin()
    if not bin_path:
        raise RuntimeError("awww/swww not found in PATH")
    path = Path(path).expanduser()
    if not path.is_file():
        raise FileNotFoundError(str(path))

    cmd = [bin_path, "img", str(path)]

    if outputs:
        cmd += ["-o", ",".join(outputs)]

    # transition — all configurable, defaults are user's choice: grow 0.5,0.5 1.2 60
    if transition_type:
        cmd += ["--transition-type", str(transition_type)]
    if transition_pos:
        cmd += ["--transition-pos", str(transition_pos)]
    if transition_duration is not None:
        cmd += ["--transition-duration", str(transition_duration)]
    if transition_fps:
        cmd += ["--transition-fps", str(transition_fps)]

    if extra_args:
        cmd += list(extra_args)

    # run async
    subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return cmd
