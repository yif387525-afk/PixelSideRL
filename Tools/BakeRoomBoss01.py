# -*- coding: utf-8 -*-
"""Bake a real Room_Boss_01.umap via Unreal Editor Python (UE 5.8).

Boss dual-phase arena (width 2560). Geometry mirrors Combat_06 greybox math
(Cube100 / SlabDepth40 / Ground24 / Platform16) with Boss_01 CSV layout.
HAS APixelRoomDirector (RoomId=Room_Boss_01, bBuildGreyboxFromCsv=False).
NO APixelShopRoomHost, NO hand-placed enemies / damage volumes, NO AffixAnchor,
NO fake uassets. Exit.FromAnte RequireClear=false; Exit.Settle RequireClear=true
(BossDefeated unlock). SP_Boss is a TargetPoint marker only (Tag Spawn.Boss).
"""
from __future__ import annotations

import traceback

import unreal

CUBE = 100.0
SLAB_DEPTH = 40.0
GROUND_THICKNESS = 24.0
PLATFORM_THICKNESS = 16.0
SPAWN_CLEARANCE = 2.0
CAPSULE_HALF = 19.0  # PixelSideCombat::CapsuleHalfHeight

ASSET_PATH = "/Game/Maps/Rooms/Room_Boss_01"
MESH_PATH = "/Engine/BasicShapes/Cube.Cube"
MAT_PATH = "/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"

FLOOR_COLOR = unreal.LinearColor(0.027, 0.048, 0.087, 1.0)  # approx #2E3D52 sRGB->lin
WALL_COLOR = unreal.LinearColor(0.043, 0.059, 0.090, 1.0)   # approx #3A4555


def log(msg: str) -> None:
    unreal.log("[BakeBoss01] " + msg)


def fail(msg: str) -> None:
    unreal.log_error("[BakeBoss01] " + msg)
    raise RuntimeError(msg)


def load_mesh():
    mesh = unreal.load_asset(MESH_PATH)
    if not mesh:
        mesh = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube")
    if not mesh:
        fail("Could not load BasicShapes Cube mesh")
    return mesh


def load_class(path: str):
    cls = unreal.load_class(None, path)
    if not cls:
        fail("Could not load class " + path)
    return cls


def try_load_class(path: str):
    try:
        return unreal.load_class(None, path)
    except Exception:
        return None


def get_editor_world():
    try:
        sub = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
        if sub:
            w = sub.get_editor_world()
            if w:
                return w
    except Exception:
        pass
    try:
        return unreal.EditorLevelLibrary.get_editor_world()
    except Exception:
        pass
    fail("No editor world")


def new_or_open_level(path: str) -> None:
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        log("Existing asset found; deleting before bake: " + path)
        unreal.EditorAssetLibrary.delete_asset(path)

    try:
        les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        if les:
            ok = les.new_level(path)
            if ok:
                log("LevelEditorSubsystem.new_level OK: " + path)
                return
            log("LevelEditorSubsystem.new_level returned False; trying AssetTools")
    except Exception as e:
        log("LevelEditorSubsystem.new_level failed: " + str(e))

    try:
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        factory = unreal.WorldFactory()
        package_path = "/Game/Maps/Rooms"
        asset_name = "Room_Boss_01"
        created = asset_tools.create_asset(asset_name, package_path, unreal.World, factory)
        if not created:
            fail("AssetTools.create_asset returned None")
        log("Created World asset via AssetTools")
        try:
            les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
            if les:
                les.load_level(path)
                return
        except Exception:
            pass
        unreal.EditorLevelLibrary.load_level(path)
        return
    except Exception as e:
        log("AssetTools path failed: " + str(e))

    try:
        unreal.EditorLevelLibrary.new_level(path)
        log("EditorLevelLibrary.new_level OK")
        return
    except Exception as e:
        fail("All level-create paths failed: " + str(e))


def spawn_actor(cls, location, rotation=None):
    if rotation is None:
        rotation = unreal.Rotator(0, 0, 0)
    try:
        eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        if eas:
            return eas.spawn_actor_from_class(cls, location, rotation)
    except Exception:
        pass
    return unreal.EditorLevelLibrary.spawn_actor_from_class(cls, location, rotation)


def set_label(actor, label: str, extra_tags=None) -> None:
    try:
        actor.set_actor_label(label, True)
    except TypeError:
        actor.set_actor_label(label)
    try:
        tags = list(actor.tags)
        want = ["PixelSideRoom", label]
        if extra_tags:
            want.extend(extra_tags)
        existing = [str(t) for t in tags]
        for t in want:
            if t not in existing:
                tags.append(t)
                existing.append(t)
        actor.set_editor_property("tags", tags)
    except Exception:
        pass


def apply_mesh_and_collision(actor, mesh, color, pass_through_pawn: bool) -> None:
    smc = actor.static_mesh_component
    if not smc:
        fail("StaticMeshActor missing component")
    smc.set_mobility(unreal.ComponentMobility.MOVABLE)
    smc.set_static_mesh(mesh)
    smc.set_cast_shadow(False)
    qap = unreal.CollisionEnabled.QUERY_AND_PHYSICS
    if pass_through_pawn:
        smc.set_collision_enabled(qap)
        smc.set_collision_object_type(unreal.CollisionChannel.ECC_WORLD_STATIC)
        smc.set_collision_response_to_all_channels(unreal.CollisionResponseType.ECR_BLOCK)
        smc.set_collision_response_to_channel(
            unreal.CollisionChannel.ECC_CAMERA, unreal.CollisionResponseType.ECR_IGNORE
        )
        smc.set_collision_response_to_channel(
            unreal.CollisionChannel.ECC_PAWN, unreal.CollisionResponseType.ECR_IGNORE
        )
    else:
        smc.set_collision_enabled(qap)
        smc.set_collision_profile_name("BlockAll")
    try:
        base = unreal.load_asset(MAT_PATH)
        if base:
            mid = None
            if hasattr(unreal, "MaterialEditingLibrary"):
                mel = unreal.MaterialEditingLibrary
                for meth in ("create_dynamic_material_instance", "create_material_instance"):
                    if hasattr(mel, meth):
                        mid = getattr(mel, meth)(actor, base)
                        break
            if mid:
                mid.set_vector_parameter_value("Color", color)
                smc.set_material(0, mid)
    except Exception as e:
        log("Tint skipped: " + str(e))


def spawn_block(label, center, size_x, size_z, color, pass_through_pawn, mesh):
    scale = unreal.Vector(size_x / CUBE, SLAB_DEPTH / CUBE, size_z / CUBE)
    actor = spawn_actor(unreal.StaticMeshActor, center)
    if not actor:
        fail("Failed to spawn block " + label)
    set_label(actor, label)
    actor.set_actor_scale3d(scale)
    apply_mesh_and_collision(actor, mesh, color, pass_through_pawn)
    actor.set_actor_location(center, False, True)
    log(
        "Block %s center=(%.1f,%.1f,%.1f) scale=(%.3f,%.3f,%.3f) pass=%s"
        % (label, center.x, center.y, center.z, scale.x, scale.y, scale.z, pass_through_pawn)
    )
    return actor


def set_prop(obj, names, value) -> bool:
    last = None
    for n in names:
        try:
            obj.set_editor_property(n, value)
            return True
        except Exception as e:
            last = e
    if last:
        log("set_prop failed for %s: %s" % (names, last))
    return False


def spawn_marker(label: str, x: float, z: float = 0.0, extra_tags=None):
    """Optional anchor marker (no collision / no uasset content / no damage volume)."""
    loc = unreal.Vector(x, 0.0, z)
    marker = None
    for cls_name in ("TargetPoint", "Note"):
        try:
            cls = getattr(unreal, cls_name, None)
            if cls:
                marker = spawn_actor(cls, loc)
                if marker:
                    break
        except Exception:
            pass
    if not marker:
        try:
            marker = spawn_actor(unreal.Actor, loc)
        except Exception as e:
            log("Marker %s skipped: %s" % (label, e))
            return None
    if marker:
        set_label(marker, label, extra_tags=extra_tags)
        log("Marker %s at (%.1f,0,%.1f) extras=%s" % (label, x, z, extra_tags))
    return marker


def bake() -> None:
    log("START bake Room_Boss_01")
    mesh = load_mesh()
    exit_cls = load_class("/Script/PixelSideRL.PixelRoomExit")
    director_cls = try_load_class("/Script/PixelSideRL.PixelRoomDirector")
    gm_cls = load_class("/Script/PixelSideRL.PixelSideGameMode")
    # Explicitly do NOT load/spawn PixelShopRoomHost.

    new_or_open_level(ASSET_PATH)
    world = get_editor_world()
    log("Editor world: " + str(world))

    try:
        ws = world.get_world_settings()
        set_prop(ws, ["default_game_mode", "DefaultGameMode"], gm_cls)
        log("World GameMode set to PixelSideGameMode")
    except Exception as e:
        log("WorldSettings GameMode set skipped: " + str(e))

    # Geometry — Room_Boss_01: width 2560; continuous G_main; no pit;
    # P_sideL/R@28; P_p2@56; CV_A/B/C PassThroughPawn
    spawn_block(
        "G_main",
        unreal.Vector(1280.0, 0.0, 0.0 - GROUND_THICKNESS * 0.5),
        2560.0,
        GROUND_THICKNESS,
        FLOOR_COLOR,
        False,
        mesh,
    )
    wall_h = 280.0
    wall_total = wall_h + GROUND_THICKNESS
    wall_cz = (wall_h - GROUND_THICKNESS) * 0.5
    spawn_block(
        "WALL_L",
        unreal.Vector(-20.0, 0.0, wall_cz),
        40.0,
        wall_total,
        WALL_COLOR,
        False,
        mesh,
    )
    spawn_block(
        "WALL_R",
        unreal.Vector(2580.0, 0.0, wall_cz),
        40.0,
        wall_total,
        WALL_COLOR,
        False,
        mesh,
    )
    # P_sideL X400-640 TopY28; P_sideR X1760-2080 TopY28; P_p2 X1120-1520 TopY56
    spawn_block(
        "P_sideL",
        unreal.Vector(520.0, 0.0, 28.0 - PLATFORM_THICKNESS * 0.5),
        240.0,
        PLATFORM_THICKNESS,
        FLOOR_COLOR,
        False,
        mesh,
    )
    spawn_block(
        "P_sideR",
        unreal.Vector(1920.0, 0.0, 28.0 - PLATFORM_THICKNESS * 0.5),
        320.0,
        PLATFORM_THICKNESS,
        FLOOR_COLOR,
        False,
        mesh,
    )
    spawn_block(
        "P_p2",
        unreal.Vector(1320.0, 0.0, 56.0 - PLATFORM_THICKNESS * 0.5),
        400.0,
        PLATFORM_THICKNESS,
        FLOOR_COLOR,
        False,
        mesh,
    )
    # Cover PassThroughPawn
    spawn_block(
        "CV_A",
        unreal.Vector(720.0, 0.0, 0.0 + 44.0 * 0.5),
        24.0,
        44.0,
        WALL_COLOR,
        True,
        mesh,
    )
    spawn_block(
        "CV_B",
        unreal.Vector(1440.0, 0.0, 0.0 + 40.0 * 0.5),
        24.0,
        40.0,
        WALL_COLOR,
        True,
        mesh,
    )
    spawn_block(
        "CV_C",
        unreal.Vector(2160.0, 0.0, 0.0 + 44.0 * 0.5),
        24.0,
        44.0,
        WALL_COLOR,
        True,
        mesh,
    )

    # Exits — VolumeRelativeLocation stays Zero; C++ auto-offsets left trigger.
    for label, x, exit_id, require_clear in (
        ("Exit_FromAnte", -40.0, "Exit.FromAnte", False),
        ("Exit_Settle", 2480.0, "Exit.Settle", True),
    ):
        loc = unreal.Vector(x, 0.0, 0.0)
        exit_actor = spawn_actor(exit_cls, loc)
        if not exit_actor:
            fail("Failed to spawn " + label)
        set_label(exit_actor, label)
        if not set_prop(exit_actor, ["exit_id", "ExitId"], unreal.Name(exit_id)):
            fail("Could not set ExitId on " + label)
        if not set_prop(
            exit_actor, ["require_clear", "b_require_clear", "bRequireClear"], require_clear
        ):
            fail("Could not set bRequireClear on " + label)
        set_prop(
            exit_actor,
            ["volume_relative_location", "VolumeRelativeLocation"],
            unreal.Vector(0.0, 0.0, 0.0),
        )
        log("Exit %s id=%s require_clear=%s" % (label, exit_id, require_clear))

    # PlayerStart — feet at (80,0,0); capsule center Half+SpawnClearance
    ps_z = CAPSULE_HALF + SPAWN_CLEARANCE
    ps = spawn_actor(unreal.PlayerStart, unreal.Vector(80.0, 0.0, ps_z))
    if not ps:
        fail("Failed to spawn PlayerStart")
    set_label(ps, "PlayerStart")
    log("PlayerStart at (80,0,%.1f)" % ps_z)

    # Markers — hazard placeholders / intro cam / unique Boss spawn point (NO enemies)
    spawn_marker("Haz_A", 960.0, 0.0)
    spawn_marker("Haz_B", 1680.0, 0.0)
    spawn_marker("CameraIntro", 640.0, 80.0)
    spawn_marker("SP_Boss", 1280.0, 0.0, extra_tags=["Spawn.Boss"])

    # Director (spec: hang if class exists)
    if director_cls:
        director = spawn_actor(director_cls, unreal.Vector(0.0, 0.0, 0.0))
        if not director:
            fail("Failed to spawn RoomDirector")
        set_label(director, "RoomDirector")
        if not set_prop(director, ["room_id", "RoomId"], unreal.Name("Room_Boss_01")):
            fail("Could not set RoomId")
        if not set_prop(
            director,
            ["build_greybox_from_csv", "b_build_greybox_from_csv", "bBuildGreyboxFromCsv"],
            False,
        ):
            fail("Could not set bBuildGreyboxFromCsv=False")
        set_prop(director, ["auto_start", "b_auto_start", "bAutoStart"], True)
        set_prop(
            director,
            ["place_player_at_anchor", "b_place_player_at_anchor", "bPlacePlayerAtAnchor"],
            True,
        )
        set_prop(director, ["first_wave_delay", "FirstWaveDelay"], 0.75)
        log("Director RoomId=Room_Boss_01 bBuildGreyboxFromCsv=False")
    else:
        log("WARN PixelRoomDirector class missing; geometry+exits+markers only")

    # Optional DirectionalLight
    try:
        existing = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.DirectionalLight)
    except Exception:
        existing = []
    if not existing:
        sun_rot = unreal.Rotator(-40.0, -90.0, 0.0)
        sun = spawn_actor(unreal.DirectionalLight, unreal.Vector(0.0, 0.0, 0.0), sun_rot)
        if sun:
            set_label(sun, "DirectionalLight_Room")
            try:
                light = sun.light_component
                if light:
                    light.set_mobility(unreal.ComponentMobility.MOVABLE)
                    light.set_intensity(10.0)
            except Exception as e:
                log("Light tweak skipped: " + str(e))
            log("Spawned DirectionalLight")
    else:
        log("DirectionalLight already present; skip")

    # Save
    saved = False
    try:
        les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        if les:
            saved = bool(les.save_current_level())
            log("LevelEditorSubsystem.save_current_level -> %s" % saved)
    except Exception as e:
        log("LES save failed: " + str(e))
    if not saved:
        try:
            unreal.EditorLevelLibrary.save_current_level()
            saved = True
            log("EditorLevelLibrary.save_current_level OK")
        except Exception as e:
            log("ELL save failed: " + str(e))
    try:
        unreal.EditorAssetLibrary.save_asset(ASSET_PATH, only_if_is_dirty=False)
        log("EditorAssetLibrary.save_asset OK")
        saved = True
    except Exception as e:
        log("save_asset failed: " + str(e))

    if not saved:
        fail("Level save failed")

    if not unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
        fail("Asset missing after save: " + ASSET_PATH)

    log("DONE bake OK: " + ASSET_PATH)


def verify_reloaded() -> None:
    log("START verify_reloaded")
    try:
        les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        if les:
            if not les.load_level(ASSET_PATH):
                fail("verify load_level failed")
        else:
            unreal.EditorLevelLibrary.load_level(ASSET_PATH)
    except Exception as e:
        fail("verify load failed: " + str(e))

    eas = None
    try:
        eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    except Exception:
        pass
    actors = eas.get_all_level_actors() if eas else unreal.EditorLevelLibrary.get_all_level_actors()

    by_label = {}
    shop_hits = []
    forbidden = []
    for a in actors:
        try:
            lab = a.get_actor_label()
        except Exception:
            lab = str(a.get_name())
        by_label.setdefault(lab, []).append(a)
        cname = a.get_class().get_name()
        loc = a.get_actor_location()
        log("Actor label=%s class=%s loc=(%.1f,%.1f,%.1f)" % (lab, cname, loc.x, loc.y, loc.z))
        if "PixelShopRoomHost" in cname or lab == "ShopRoomHost":
            shop_hits.append(lab)
        if "AffixAnchor" in lab:
            forbidden.append(lab)
        if lab.startswith("SP_A") or "Enemy" in cname or "Damage" in cname:
            forbidden.append("%s/%s" % (lab, cname))

    missing = []
    for lab in (
        "G_main", "WALL_L", "WALL_R",
        "P_sideL", "P_sideR", "P_p2",
        "CV_A", "CV_B", "CV_C",
        "Exit_FromAnte", "Exit_Settle",
        "PlayerStart", "SP_Boss",
        "RoomDirector",
    ):
        if lab not in by_label:
            if lab == "PlayerStart":
                found = False
                for k, arr in by_label.items():
                    if "PlayerStart" in k or arr[0].get_class().get_name() == "PlayerStart":
                        found = True
                        log("PlayerStart matched as label=%s" % k)
                        break
                if not found:
                    missing.append(lab)
            elif lab == "RoomDirector":
                # soft if class absent at bake time
                has_dir = any("PixelRoomDirector" in a.get_class().get_name() for a in actors)
                if not has_dir:
                    log("WARN RoomDirector missing (class may be unavailable)")
            elif lab == "SP_Boss":
                missing.append(lab)
            else:
                missing.append(lab)

    if shop_hits:
        missing.append("UNEXPECTED_ShopHost:" + ",".join(shop_hits))
    if forbidden:
        missing.append("UNEXPECTED_Forbidden:" + ",".join(forbidden))

    def gp(obj, names):
        for n in names:
            try:
                return obj.get_editor_property(n)
            except Exception:
                pass
        return None

    director = None
    for lab, arr in by_label.items():
        cname = arr[0].get_class().get_name()
        if "PixelRoomDirector" in cname or lab == "RoomDirector":
            director = arr[0]
            break
    if not director:
        for a in actors:
            if "PixelRoomDirector" in a.get_class().get_name():
                director = a
                break
    if director:
        room_id = gp(director, ["room_id", "RoomId"])
        grey = gp(director, ["build_greybox_from_csv", "b_build_greybox_from_csv", "bBuildGreyboxFromCsv"])
        auto = gp(director, ["auto_start", "b_auto_start", "bAutoStart"])
        place = gp(director, ["place_player_at_anchor", "b_place_player_at_anchor", "bPlacePlayerAtAnchor"])
        fwd = gp(director, ["first_wave_delay", "FirstWaveDelay"])
        log(
            "Director RoomId=%r bBuildGreyboxFromCsv=%r bAutoStart=%r bPlacePlayerAtAnchor=%r FirstWaveDelay=%r"
            % (room_id, grey, auto, place, fwd)
        )
        if str(room_id) != "Room_Boss_01":
            missing.append("RoomId!=Room_Boss_01")
        if grey is not False:
            missing.append("bBuildGreyboxFromCsv!=False")
        if auto is not True:
            missing.append("bAutoStart!=True")
        if fwd is not None and abs(float(fwd) - 0.75) > 1e-3:
            missing.append("FirstWaveDelay!=0.75")
    else:
        missing.append("DirectorMissing")

    for lab, want_id, want_req, want_x in (
        ("Exit_FromAnte", "Exit.FromAnte", False, -40.0),
        ("Exit_Settle", "Exit.Settle", True, 2480.0),
    ):
        if lab not in by_label:
            continue
        ex = by_label[lab][0]
        eid = gp(ex, ["exit_id", "ExitId"])
        req = gp(ex, ["require_clear", "b_require_clear", "bRequireClear"])
        vol = gp(ex, ["volume_relative_location", "VolumeRelativeLocation"])
        loc = ex.get_actor_location()
        log(
            "Exit %s ExitId=%r bRequireClear=%r VolumeRel=%r loc=(%.1f,%.1f,%.1f)"
            % (lab, eid, req, vol, loc.x, loc.y, loc.z)
        )
        if str(eid) != want_id:
            missing.append("%s ExitId" % lab)
        if bool(req) != want_req:
            missing.append("%s bRequireClear" % lab)
        if abs(loc.x - want_x) > 0.5:
            missing.append("%s X" % lab)

    if "PlayerStart" in by_label:
        ps = by_label["PlayerStart"][0]
        loc = ps.get_actor_location()
        log("PlayerStart loc=(%.1f,%.1f,%.1f)" % (loc.x, loc.y, loc.z))
        if abs(loc.x - 80.0) > 0.5:
            missing.append("PlayerStart X")
        if abs(loc.z - 21.0) > 1.0:
            missing.append("PlayerStart Z")

    if "SP_Boss" in by_label:
        sp = by_label["SP_Boss"][0]
        loc = sp.get_actor_location()
        tags = [str(t) for t in (sp.tags or [])]
        log("SP_Boss loc=(%.1f,%.1f,%.1f) tags=%s" % (loc.x, loc.y, loc.z, tags))
        if abs(loc.x - 1280.0) > 0.5:
            missing.append("SP_Boss X")
        if "Spawn.Boss" not in tags:
            missing.append("SP_Boss missing Tag Spawn.Boss")

    for lab, want_x, want_z in (
        ("P_sideL", 520.0, 20.0),
        ("P_sideR", 1920.0, 20.0),
        ("P_p2", 1320.0, 48.0),
        ("CV_A", 720.0, 22.0),
        ("CV_B", 1440.0, 20.0),
        ("CV_C", 2160.0, 22.0),
    ):
        if lab in by_label:
            loc = by_label[lab][0].get_actor_location()
            if abs(loc.x - want_x) > 1.0 or abs(loc.z - want_z) > 1.0:
                missing.append("%s pos" % lab)
            log("%s loc=(%.1f,%.1f,%.1f)" % (lab, loc.x, loc.y, loc.z))

    if missing:
        fail("verify failed: " + ", ".join(missing))
    log("PASS verify_reloaded")


def main():
    try:
        bake()
        verify_reloaded()
    except Exception:
        unreal.log_error("[BakeBoss01] TRACEBACK:\n" + traceback.format_exc())
        raise


if __name__ == "__main__":
    main()
