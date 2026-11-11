# -*- coding: utf-8 -*-
"""Bake a real Hub.umap via Unreal Editor Python (UE 5.8).

Content/Maps/Hub.umap — one PlayerStart. No Lab geometry, no APixelRoomDirector,
no shop host, no widget uasset, no directional light. Combat lighting stays on
APixelSideGameMode::EnsureSideSun for Room_Combat_01. This map does not call it.

Run inside the UE 5.8.4 editor Python console, from the project root:

    py Tools/BakeHub.py

Or headless, after the C++ module is compiled:

    UnrealEditor-Cmd PixelSideRL.uproject -ExecutePythonScript=Tools/BakeHub.py -stdout -unattended

Do not hand-write a .umap. Commit the editor-saved file with Git LFS.
"""
from __future__ import annotations

import traceback

import unreal

ASSET_PATH = "/Game/Maps/Hub"
PACKAGE_PATH = "/Game/Maps"
ASSET_NAME = "Hub"
CAPSULE_HALF = 19.0
SPAWN_CLEARANCE = 2.0


def log(msg: str) -> None:
    unreal.log("[BakeHub] " + msg)


def fail(msg: str) -> None:
    unreal.log_error("[BakeHub] " + msg)
    raise RuntimeError(msg)


def get_editor_world():
    try:
        sub = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
        if sub:
            world = sub.get_editor_world()
            if world:
                return world
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
    except Exception as exc:
        log("LevelEditorSubsystem.new_level failed: " + str(exc))

    try:
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        factory = unreal.WorldFactory()
        created = asset_tools.create_asset(ASSET_NAME, PACKAGE_PATH, unreal.World, factory)
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
    except Exception as exc:
        log("AssetTools path failed: " + str(exc))

    try:
        unreal.EditorLevelLibrary.new_level(path)
        log("EditorLevelLibrary.new_level OK")
        return
    except Exception as exc:
        fail("All level-create paths failed: " + str(exc))


def spawn_actor(cls, location, rotation=None):
    if rotation is None:
        rotation = unreal.Rotator(0, 0, 0)
    try:
        eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        if eas:
            spawned = eas.spawn_actor_from_class(cls, location, rotation)
            if spawned:
                return spawned
    except Exception:
        pass
    return unreal.EditorLevelLibrary.spawn_actor_from_class(cls, location, rotation)


def set_label(actor, label: str) -> None:
    try:
        actor.set_actor_label(label, True)
    except TypeError:
        actor.set_actor_label(label)


def set_prop(obj, names, value) -> bool:
    last = None
    for name in names:
        try:
            obj.set_editor_property(name, value)
            return True
        except Exception as exc:
            last = exc
    if last:
        log("set_prop failed for %s: %s" % (names, last))
    return False


def class_name_of(actor) -> str:
    try:
        return actor.get_class().get_name()
    except Exception:
        return ""


def bake() -> None:
    log("START bake Hub")
    new_or_open_level(ASSET_PATH)
    world = get_editor_world()

    try:
        gm_cls = unreal.load_class(None, "/Script/PixelSideRL.PixelSideGameMode")
        if gm_cls:
            ws = world.get_world_settings()
            set_prop(ws, ["default_game_mode", "DefaultGameMode"], gm_cls)
            log("World GameMode set to PixelSideGameMode")
        else:
            log("PixelSideGameMode class missing; project default still applies after compile")
    except Exception as exc:
        log("WorldSettings GameMode set skipped: " + str(exc))

    ps_z = CAPSULE_HALF + SPAWN_CLEARANCE
    player_start = spawn_actor(unreal.PlayerStart, unreal.Vector(0.0, 0.0, ps_z))
    if not player_start:
        fail("Failed to spawn PlayerStart")
    set_label(player_start, "PlayerStart")
    log("PlayerStart at (0,0,%.1f). No floor, no director, no light." % ps_z)

    saved = False
    try:
        les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        if les:
            saved = bool(les.save_current_level())
            log("LevelEditorSubsystem.save_current_level -> %s" % saved)
    except Exception as exc:
        log("LES save failed: " + str(exc))
    if not saved:
        try:
            unreal.EditorLevelLibrary.save_current_level()
            saved = True
            log("EditorLevelLibrary.save_current_level OK")
        except Exception as exc:
            log("ELL save failed: " + str(exc))
    try:
        unreal.EditorAssetLibrary.save_asset(ASSET_PATH, only_if_is_dirty=False)
        log("EditorAssetLibrary.save_asset OK")
        saved = True
    except Exception as exc:
        log("save_asset failed: " + str(exc))
    if not saved:
        fail("Hub level did not save")

    verify(get_editor_world())
    log("PASS bake Hub")


def verify(world) -> None:
    actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)
    player_starts = 0
    forbidden = []
    for actor in actors:
        name = class_name_of(actor)
        if name == "PlayerStart":
            player_starts += 1
        if name in ("PixelRoomDirector", "PixelShopRoomHost", "StaticMeshActor", "DirectionalLight"):
            forbidden.append(name)
        label = ""
        try:
            label = actor.get_actor_label()
        except Exception:
            label = ""
        if label in ("RoomDirector", "ShopRoomHost"):
            forbidden.append(label)
    if player_starts != 1:
        fail("expected 1 PlayerStart, got %d" % player_starts)
    if forbidden:
        fail("Hub must not contain %s" % forbidden)
    log("verify: 1 PlayerStart, no director, no mesh, no directional light")


def main():
    try:
        bake()
    except Exception:
        unreal.log_error("[BakeHub] TRACEBACK:\n" + traceback.format_exc())
        raise


if __name__ == "__main__":
    main()
