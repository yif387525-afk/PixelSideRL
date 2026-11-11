# -*- coding: utf-8 -*-
"""Surgical fix: official-room CV_* collision -> PassThroughPawn. Do not change geometry."""
from __future__ import annotations

import traceback

import unreal

PROFILE = "PassThroughPawn"
EPS = 0.05
LOG_PREFIX = "[FixCoverCollision] "

ROOMS = [
    ("Room_Combat_01", "/Game/Maps/Rooms/Room_Combat_01", ("CV_A", "CV_B")),
    ("Room_Combat_02", "/Game/Maps/Rooms/Room_Combat_02", ("CV_A", "CV_B")),
    ("Room_Combat_03", "/Game/Maps/Rooms/Room_Combat_03", ("CV_A", "CV_B", "CV_C")),
    ("Room_Combat_04", "/Game/Maps/Rooms/Room_Combat_04", ("CV_A", "CV_B", "CV_C")),
    ("Room_Combat_05", "/Game/Maps/Rooms/Room_Combat_05", ("CV_A", "CV_B", "CV_C", "CV_D")),
    ("Room_Combat_06", "/Game/Maps/Rooms/Room_Combat_06", ("CV_A", "CV_B", "CV_C")),
    ("Room_Elite_01", "/Game/Maps/Rooms/Room_Elite_01", ("CV_A", "CV_B", "CV_C")),
    ("Room_Elite_02", "/Game/Maps/Rooms/Room_Elite_02", ("CV_A", "CV_B", "CV_C", "CV_D")),
    ("Room_Event_01", "/Game/Maps/Rooms/Room_Event_01", ("CV_A",)),
    ("Room_Shop_01", "/Game/Maps/Rooms/Room_Shop_01", ("CV_counter",)),
    ("Room_BossAnte_01", "/Game/Maps/Rooms/Room_BossAnte_01", ("CV_A",)),
    ("Room_Boss_01", "/Game/Maps/Rooms/Room_Boss_01", ("CV_A", "CV_B", "CV_C")),
]


def log(msg: str) -> None:
    unreal.log(LOG_PREFIX + msg)


def log_error(msg: str) -> None:
    unreal.log_error(LOG_PREFIX + msg)


def is_cv_label(lab: str) -> bool:
    return bool(lab) and lab.startswith("CV_")


def load_level(path: str) -> None:
    try:
        les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        if les:
            if not les.load_level(path):
                raise RuntimeError("LevelEditorSubsystem.load_level returned False")
            return
    except Exception as e:
        log("LES load failed: %s" % e)
    unreal.EditorLevelLibrary.load_level(path)


def save_level(path: str) -> None:
    saved = False
    try:
        les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        if les:
            saved = bool(les.save_current_level())
            log("LevelEditorSubsystem.save_current_level -> %s" % saved)
    except Exception as e:
        log("LES save failed: %s" % e)
    if not saved:
        try:
            unreal.EditorLevelLibrary.save_current_level()
            saved = True
            log("EditorLevelLibrary.save_current_level OK")
        except Exception as e:
            log("ELL save failed: %s" % e)
    try:
        unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
        log("EditorAssetLibrary.save_asset OK")
        saved = True
    except Exception as e:
        log("save_asset failed: %s" % e)
    if not saved:
        raise RuntimeError("Level save failed: " + path)


def all_level_actors():
    eas = None
    try:
        eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    except Exception:
        pass
    if eas:
        return list(eas.get_all_level_actors())
    return list(unreal.EditorLevelLibrary.get_all_level_actors())


def actor_label(actor) -> str:
    try:
        return actor.get_actor_label()
    except Exception:
        try:
            return str(actor.get_name())
        except Exception:
            return ""


def vec_tuple(v):
    return (float(v.x), float(v.y), float(v.z))


def nearly_equal(a, b, eps=EPS) -> bool:
    return abs(a[0] - b[0]) <= eps and abs(a[1] - b[1]) <= eps and abs(a[2] - b[2]) <= eps


def channel_enum(name: str):
    cc = unreal.CollisionChannel
    for attr in (name, "ECC_" + name, name.upper(), "ECC_" + name.upper()):
        if hasattr(cc, attr):
            return getattr(cc, attr)
    raise RuntimeError("no CollisionChannel." + name)


def profile_name(smc) -> str:
    try:
        return str(smc.get_collision_profile_name())
    except Exception:
        try:
            return str(smc.get_editor_property("collision_profile_name"))
        except Exception as e:
            return "ERR:" + str(e)


def response_str(smc, channel) -> str:
    try:
        return str(smc.get_collision_response_to_channel(channel))
    except Exception as e:
        return "ERR:" + str(e)


def is_ignore(s: str) -> bool:
    return "IGNORE" in s.upper()


def is_block(s: str) -> bool:
    return "BLOCK" in s.upper()


def collect_cv_actors():
    found = {}
    extras = []
    for actor in all_level_actors():
        lab = actor_label(actor)
        if not is_cv_label(lab):
            continue
        smc = getattr(actor, "static_mesh_component", None)
        if not smc:
            try:
                smc = actor.get_component_by_class(unreal.StaticMeshComponent)
            except Exception:
                smc = None
        rec = {
            "actor": actor,
            "label": lab,
            "smc": smc,
            "loc": vec_tuple(actor.get_actor_location()),
            "scale": vec_tuple(actor.get_actor_scale3d()),
        }
        if lab not in found:
            found[lab] = rec
        else:
            extras.append(lab)
            # keep first; still record duplicate as extra label hit
    return found, extras


def apply_profile(actor, smc) -> None:
    qap = unreal.CollisionEnabled.QUERY_AND_PHYSICS
    smc.set_collision_enabled(qap)
    smc.set_collision_profile_name(PROFILE)
    # Do not tweak channels after setting the named profile, or it becomes Custom
    # and the profile name is lost on serialize.
    try:
        smc.modify()
    except Exception:
        pass
    try:
        actor.modify()
    except Exception:
        pass


def check_collision(smc, problems, prefix):
    pname = profile_name(smc)
    if pname != PROFILE:
        problems.append("%s profile=%s want=%s" % (prefix, pname, PROFILE))
    pawn = response_str(smc, channel_enum("PAWN"))
    cam = response_str(smc, channel_enum("CAMERA"))
    vis = response_str(smc, channel_enum("VISIBILITY"))
    wd = response_str(smc, channel_enum("WORLD_DYNAMIC"))
    if not is_ignore(pawn):
        problems.append("%s Pawn=%s want=Ignore" % (prefix, pawn))
    if not is_ignore(cam):
        problems.append("%s Camera=%s want=Ignore" % (prefix, cam))
    if not is_block(vis):
        problems.append("%s Visibility=%s want=Block" % (prefix, vis))
    if not is_block(wd):
        problems.append("%s WorldDynamic=%s want=Block" % (prefix, wd))
    log(
        "%s profile=%s Pawn=%s Camera=%s Visibility=%s WorldDynamic=%s"
        % (prefix, pname, pawn, cam, vis, wd)
    )


def fix_room(room_name, asset_path, expected):
    log("START room %s path=%s" % (room_name, asset_path))
    if not unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        raise RuntimeError("asset missing: " + asset_path)
    load_level(asset_path)

    found, dupes = collect_cv_actors()
    problems = []
    if dupes:
        problems.append("duplicate CV labels: " + ",".join(dupes))

    snapshots = {}
    all_labels = set(found.keys())
    unexpected = sorted(lab for lab in all_labels if lab not in expected)
    missing = [lab for lab in expected if lab not in found]
    if missing:
        problems.append("missing expected CV: " + ",".join(missing))
    if unexpected:
        log("UNEXPECTED CV prefix (will still fix): " + ",".join(unexpected))

    for lab, rec in sorted(found.items()):
        smc = rec["smc"]
        if not smc:
            problems.append("%s has no StaticMeshComponent" % lab)
            continue
        before = profile_name(smc)
        log(
            "BEFORE %s profile=%s loc=(%.3f,%.3f,%.3f) scale=(%.3f,%.3f,%.3f)"
            % (lab, before, rec["loc"][0], rec["loc"][1], rec["loc"][2], rec["scale"][0], rec["scale"][1], rec["scale"][2])
        )
        snapshots[lab] = rec
        apply_profile(rec["actor"], smc)
        after_loc = vec_tuple(rec["actor"].get_actor_location())
        after_scale = vec_tuple(rec["actor"].get_actor_scale3d())
        if not nearly_equal(after_loc, rec["loc"]):
            problems.append("%s loc changed during apply %s -> %s" % (lab, rec["loc"], after_loc))
        if not nearly_equal(after_scale, rec["scale"]):
            problems.append("%s scale changed during apply %s -> %s" % (lab, rec["scale"], after_scale))

    if problems:
        raise RuntimeError("; ".join(problems))

    save_level(asset_path)
    log("RELOAD %s" % asset_path)
    load_level(asset_path)

    found2, dupes2 = collect_cv_actors()
    problems = []
    if dupes2:
        problems.append("duplicate CV labels after reload: " + ",".join(dupes2))
    for lab in expected:
        if lab not in found2:
            problems.append("missing after reload: " + lab)
            continue
        rec2 = found2[lab]
        snap = snapshots.get(lab)
        if snap:
            loc2 = rec2["loc"]
            scale2 = rec2["scale"]
            if not nearly_equal(loc2, snap["loc"]):
                problems.append("%s loc changed after reload %s -> %s" % (lab, snap["loc"], loc2))
            if not nearly_equal(scale2, snap["scale"]):
                problems.append("%s scale changed after reload %s -> %s" % (lab, snap["scale"], scale2))
        smc = rec2["smc"]
        if not smc:
            problems.append("%s no SMC after reload" % lab)
            continue
        check_collision(smc, problems, lab)

    for lab in sorted(found2.keys()):
        if lab in expected:
            continue
        log("checking unexpected CV after reload: " + lab)
        rec2 = found2[lab]
        if rec2["smc"]:
            check_collision(rec2["smc"], problems, lab)

    if problems:
        raise RuntimeError("; ".join(problems))
    fixed = ",".join(expected)
    extra = (", extra=" + ",".join(unexpected)) if unexpected else ""
    log("ROOM_PASS %s cv=[%s]%s" % (room_name, fixed, extra))
    return list(expected) + list(unexpected)


def main() -> None:
    log("START official cover collision fix; profile=%s" % PROFILE)
    failures = []
    summary = []
    for room_name, asset_path, expected in ROOMS:
        try:
            labels = fix_room(room_name, asset_path, expected)
            summary.append("%s:%s" % (room_name, ",".join(labels)))
        except Exception as e:
            log_error("ROOM_FAIL %s: %s" % (room_name, e))
            log_error("TRACEBACK:\n" + traceback.format_exc())
            failures.append("%s: %s" % (room_name, e))
    if failures:
        log("FAIL overall: " + " | ".join(failures))
        raise RuntimeError("FAIL overall: " + " | ".join(failures))
    log("SUMMARY " + " ; ".join(summary))
    log("PASS all rooms")


if __name__ == "__main__":
    try:
        main()
    except Exception:
        log_error("TRACEBACK:\n" + traceback.format_exc())
        raise
