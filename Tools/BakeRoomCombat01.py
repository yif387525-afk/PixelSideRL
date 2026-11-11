# -*- coding: utf-8 -*-
"""Bake a real Room_Combat_01.umap via Unreal Editor Python (UE 5.8).
Mirrors APixelRoomDirector::BuildGreybox geometry math; greybox flag left OFF.
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

ASSET_PATH = "/Game/Maps/Rooms/Room_Combat_01"
MESH_PATH = "/Engine/BasicShapes/Cube.Cube"
MAT_PATH = "/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"

FLOOR_COLOR = unreal.LinearColor(0.027, 0.048, 0.087, 1.0)  # approx #2E3D52 sRGB->lin
WALL_COLOR = unreal.LinearColor(0.043, 0.059, 0.090, 1.0)   # approx #3A4555


def log(msg: str) -> None:
    unreal.log("[BakeCombat01] " + msg)


def fail(msg: str) -> None:
    unreal.log_error("[BakeCombat01] " + msg)
    raise RuntimeError(msg)


def load_mesh():
    mesh = unreal.load_asset(MESH_PATH)
    if not mesh:
        # try without .Cube suffix
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
    # Delete existing asset if present so we bake clean.
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        log("Existing asset found; deleting before bake: " + path)
        unreal.EditorAssetLibrary.delete_asset(path)

    # Prefer LevelEditorSubsystem (UE5)
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
        asset_name = "Room_Combat_01"
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

    # Last resort: EditorLevelLibrary
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
        # Named project profile PassThroughPawn (Config/DefaultEngine.ini).
        # WorldStatic, QueryAndPhysics; Pawn/Camera Ignore; Visibility/WorldDynamic Block.
        # Set the profile name; do not leave Engine Cube/BlockAll and only tweak channels,
        # or the umap serializes without PassThroughPawn and pawns cannot pass.
        smc.set_collision_enabled(qap)
        smc.set_collision_profile_name("PassThroughPawn")
    else:
        smc.set_collision_enabled(qap)
        smc.set_collision_profile_name("BlockAll")
    # Tint optional
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


def bake() -> None:
    log("START bake Room_Combat_01")
    mesh = load_mesh()
    exit_cls = load_class("/Script/PixelSideRL.PixelRoomExit")
    director_cls = load_class("/Script/PixelSideRL.PixelRoomDirector")
    gm_cls = load_class("/Script/PixelSideRL.PixelSideGameMode")

    new_or_open_level(ASSET_PATH)
    world = get_editor_world()
    log("Editor world: " + str(world))

    # World GameMode
    try:
        ws = world.get_world_settings()
        set_prop(ws, ["default_game_mode", "DefaultGameMode"], gm_cls)
        log("World GameMode set to PixelSideGameMode")
    except Exception as e:
        log("WorldSettings GameMode set skipped: " + str(e))

    # Geometry (BuildGreybox mirror)
    # G_main Floor X0..1280 TopY=0
    spawn_block(
        "G_main",
        unreal.Vector(640.0, 0.0, 0.0 - GROUND_THICKNESS * 0.5),
        1280.0,
        GROUND_THICKNESS,
        FLOOR_COLOR,
        False,
        mesh,
    )
    # WALL_L X -40..0 wall top 280
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
    # P_kite
    spawn_block(
        "P_kite",
        unreal.Vector(700.0, 0.0, 28.0 - PLATFORM_THICKNESS * 0.5),
        160.0,
        PLATFORM_THICKNESS,
        FLOOR_COLOR,
        False,
        mesh,
    )
    # Cover
    spawn_block(
        "CV_A",
        unreal.Vector(640.0, 0.0, 0.0 + 44.0 * 0.5),
        24.0,
        44.0,
        WALL_COLOR,
        True,
        mesh,
    )
    spawn_block(
        "CV_B",
        unreal.Vector(880.0, 0.0, 0.0 + 40.0 * 0.5),
        24.0,
        40.0,
        WALL_COLOR,
        True,
        mesh,
    )

    # Exits — VolumeRelativeLocation stays Zero; C++ auto-offsets left trigger.
    for label, x, exit_id, require_clear in (
        ("Exit_Left", -40.0, "Exit.Left", False),
        ("Exit_Right", 1200.0, "Exit.Right", True),
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

    # PlayerStart — feet at Z=0; capsule center Half+SpawnClearance like PlacePlayer
    ps_z = CAPSULE_HALF + SPAWN_CLEARANCE
    ps = spawn_actor(unreal.PlayerStart, unreal.Vector(40.0, 0.0, ps_z))
    if not ps:
        fail("Failed to spawn PlayerStart")
    set_label(ps, "PlayerStart")
    log("PlayerStart at (40,0,%.1f)" % ps_z)

    # Director
    director = spawn_actor(director_cls, unreal.Vector(0.0, 0.0, 0.0))
    if not director:
        fail("Failed to spawn RoomDirector")
    set_label(director, "RoomDirector")
    if not set_prop(director, ["room_id", "RoomId"], unreal.Name("Room_Combat_01")):
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
    log("Director RoomId=Room_Combat_01 bBuildGreyboxFromCsv=False")

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

    # Verify asset exists
    if not unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
        fail("Asset missing after save: " + ASSET_PATH)

    log("DONE bake OK: " + ASSET_PATH)


def main():
    try:
        bake()
    except Exception:
        unreal.log_error("[BakeCombat01] TRACEBACK:\n" + traceback.format_exc())
        raise


if __name__ == "__main__":
    main()
