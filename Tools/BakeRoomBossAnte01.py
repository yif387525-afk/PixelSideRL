# -*- coding: utf-8 -*-
"""Bake a real Room_BossAnte_01.umap via Unreal Editor Python (UE 5.8).

Boss antechamber (BossGate). Geometry mirrors Shop/Event greybox math
(Cube100 / SlabDepth40 / Ground24 / Platform16) with BossAnte_01 CSV layout.
NO APixelRoomDirector, NO APixelShopRoomHost, NO grunts, NO spawn points,
NO damage volumes, NO AffixAnchor, NO fake uassets. Both exits unlocked
(RequireClear=false). Exit.BossGate forwards to Room_Boss_01.
This map only validates geometry, exits, and BossPreview anchor.
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

ASSET_PATH = "/Game/Maps/Rooms/Room_BossAnte_01"
MESH_PATH = "/Engine/BasicShapes/Cube.Cube"
MAT_PATH = "/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"

FLOOR_COLOR = unreal.LinearColor(0.027, 0.048, 0.087, 1.0)  # approx #2E3D52 sRGB->lin
WALL_COLOR = unreal.LinearColor(0.043, 0.059, 0.090, 1.0)   # approx #3A4555


def log(msg: str) -> None:
    unreal.log("[BakeBossAnte01] " + msg)


def fail(msg: str) -> None:
    unreal.log_error("[BakeBossAnte01] " + msg)
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
        asset_name = "Room_BossAnte_01"
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


def set_label(actor, label: str) -> None:
    try:
        actor.set_actor_label(label, True)
    except TypeError:
        actor.set_actor_label(label)
    try:
        tags = actor.tags
        if "PixelSideRoom" not in [str(t) for t in tags]:
            tags.append("PixelSideRoom")
        if label not in [str(t) for t in tags]:
            tags.append(label)
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
        # PixelSideMesh::PassThroughPawn — cover / atmosphere column
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


def spawn_marker(label: str, x: float, z: float = 0.0):
    """Optional anchor marker (no collision / no uasset content)."""
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
        set_label(marker, label)
        log("Marker %s at (%.1f,0,%.1f)" % (label, x, z))
    return marker


def bake() -> None:
    log("START bake Room_BossAnte_01")
    mesh = load_mesh()
    exit_cls = load_class("/Script/PixelSideRL.PixelRoomExit")
    gm_cls = load_class("/Script/PixelSideRL.PixelSideGameMode")
    # Explicitly do NOT load/spawn PixelRoomDirector or PixelShopRoomHost.

    new_or_open_level(ASSET_PATH)
    world = get_editor_world()
    log("Editor world: " + str(world))

    try:
        ws = world.get_world_settings()
        set_prop(ws, ["default_game_mode", "DefaultGameMode"], gm_cls)
        log("World GameMode set to PixelSideGameMode")
    except Exception as e:
        log("WorldSettings GameMode set skipped: " + str(e))

    # Geometry — Room_BossAnte_01_geometry.csv (design Y → UE Z)
    # G_main Floor X0..1280 TopY=0; no pit
    spawn_block(
        "G_main",
        unreal.Vector(640.0, 0.0, 0.0 - GROUND_THICKNESS * 0.5),
        1280.0,
        GROUND_THICKNESS,
        FLOOR_COLOR,
        False,
        mesh,
    )
    # WALL_L X -40..0 / WALL_R X 1280..1320 wall top 280
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
        unreal.Vector(1300.0, 0.0, wall_cz),
        40.0,
        wall_total,
        WALL_COLOR,
        False,
        mesh,
    )
    # P_lintel X 960–1160 TopY=28 (lintel-side atmosphere; not required step)
    spawn_block(
        "P_lintel",
        unreal.Vector(1060.0, 0.0, 28.0 - PLATFORM_THICKNESS * 0.5),
        200.0,
        PLATFORM_THICKNESS,
        FLOOR_COLOR,
        False,
        mesh,
    )
    # CV_A bottom-center (480,0) w24 h40 PassThroughPawn
    spawn_block(
        "CV_A",
        unreal.Vector(480.0, 0.0, 0.0 + 40.0 * 0.5),
        24.0,
        40.0,
        WALL_COLOR,
        True,
        mesh,
    )

    # Exits — both unlocked (RequireClear=false); VolumeRel=0
    # Forward gate Tag = Exit.BossGate → Room_Boss_01
    for label, x, exit_id, require_clear in (
        ("Exit_Left", -40.0, "Exit.Left", False),
        ("Exit_BossGate", 1200.0, "Exit.BossGate", False),
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

    # PlayerStart — feet at (48,0,0); capsule center Z = Half+Clearance
    ps_z = CAPSULE_HALF + SPAWN_CLEARANCE
    ps = spawn_actor(unreal.PlayerStart, unreal.Vector(48.0, 0.0, ps_z))
    if not ps:
        fail("Failed to spawn PlayerStart")
    set_label(ps, "PlayerStart")
    log("PlayerStart at (48,0,%.1f)" % ps_z)

    # BossPreview marker only (no collision / no fake uassets / no AffixAnchor / no spawn points)
    spawn_marker("BossPreview", 1100.0, 0.0)

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
    verify_reloaded()


def verify_reloaded() -> None:
    """Reload saved level and assert geometry + BossGate RequireClear=false + no host."""
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
    if eas:
        actors = eas.get_all_level_actors()
    else:
        actors = unreal.EditorLevelLibrary.get_all_level_actors()

    by_label = {}
    forbidden_hits = []
    for a in actors:
        try:
            lab = a.get_actor_label()
        except Exception:
            lab = str(a.get_name())
        by_label.setdefault(lab, []).append(a)
        cname = a.get_class().get_name()
        loc = a.get_actor_location()
        log("Actor label=%s class=%s loc=(%.1f,%.1f,%.1f)" % (lab, cname, loc.x, loc.y, loc.z))
        if (
            "PixelRoomDirector" in cname
            or lab == "RoomDirector"
            or "PixelShopRoomHost" in cname
            or lab == "ShopRoomHost"
            or "EventRoomHost" in cname
            or lab == "EventRoomHost"
            or "BossAnteHost" in cname
            or lab == "BossAnteHost"
            or "AffixAnchor" in lab
            or lab.startswith("SP_")
        ):
            forbidden_hits.append("%s/%s" % (lab, cname))

    missing = []
    for lab in (
        "G_main",
        "WALL_L",
        "WALL_R",
        "P_lintel",
        "CV_A",
        "Exit_Left",
        "Exit_BossGate",
        "PlayerStart",
        "BossPreview",
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
            elif lab == "BossPreview":
                log("WARN BossPreview missing (non-fatal soft if TargetPoint unavailable)")
            else:
                missing.append(lab)

    if forbidden_hits:
        missing.append("UNEXPECTED_HOST_OR_DIRECTOR:" + ",".join(forbidden_hits))

    def gp(obj, names):
        for n in names:
            try:
                return obj.get_editor_property(n)
            except Exception:
                pass
        return None

    for lab, want_id, want_req in (
        ("Exit_Left", "Exit.Left", False),
        ("Exit_BossGate", "Exit.BossGate", False),
    ):
        if lab not in by_label:
            continue
        ex = by_label[lab][0]
        eid = gp(ex, ["exit_id", "ExitId"])
        req = gp(ex, ["require_clear", "b_require_clear", "bRequireClear"])
        vol = gp(ex, ["volume_relative_location", "VolumeRelativeLocation"])
        log("Exit %s ExitId=%r bRequireClear=%r VolumeRel=%r" % (lab, eid, req, vol))
        if str(eid) != want_id:
            missing.append("%s ExitId want=%s got=%r" % (lab, want_id, eid))
        if bool(req) != want_req:
            missing.append("%s bRequireClear" % lab)

    for lab, arr in by_label.items():
        if "PlayerStart" in lab or arr[0].get_class().get_name() == "PlayerStart":
            loc = arr[0].get_actor_location()
            if abs(loc.x - 48.0) > 1.0:
                missing.append("PlayerStartX!=48")
            log("PlayerStart loc=(%.1f,%.1f,%.1f)" % (loc.x, loc.y, loc.z))
            break

    if "BossPreview" in by_label:
        loc = by_label["BossPreview"][0].get_actor_location()
        if abs(loc.x - 1100.0) > 1.0:
            missing.append("BossPreviewX!=1100")
        log("BossPreview loc=(%.1f,%.1f,%.1f)" % (loc.x, loc.y, loc.z))

    if "P_lintel" in by_label:
        loc = by_label["P_lintel"][0].get_actor_location()
        # center X=1060, Z=20 (TopY28 - half thickness 8)
        if abs(loc.x - 1060.0) > 1.0:
            missing.append("P_lintelX!=1060")
        if abs(loc.z - 20.0) > 1.0:
            missing.append("P_lintelZ!=20")
        log("P_lintel loc=(%.1f,%.1f,%.1f)" % (loc.x, loc.y, loc.z))

    if "CV_A" in by_label:
        loc = by_label["CV_A"][0].get_actor_location()
        if abs(loc.x - 480.0) > 1.0:
            missing.append("CV_AX!=480")
        log("CV_A loc=(%.1f,%.1f,%.1f)" % (loc.x, loc.y, loc.z))

    if missing:
        fail("verify failed: " + str(missing))
    log("PASS verify_reloaded")


def main():
    try:
        bake()
    except Exception:
        unreal.log_error("[BakeBossAnte01] TRACEBACK:\n" + traceback.format_exc())
        raise


if __name__ == "__main__":
    main()
