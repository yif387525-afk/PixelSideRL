# PixelSideRL

Pixel side-scroller shooter roguelike for Unreal Engine 5.8+ (suggested lock 5.8.4). Short runs move through a room graph (combat, elite, event, shop, rest), with in-run affix builds and a light permanent meta. Target look is pixel art at a logical 640×360, nearest-neighbor scaled, with an orthographic camera.

This is a practice project. The repository holds the design seed: documentation, pixel-art stubs, JSON data contracts, room specs, and C++ header contracts. Create the Unreal project locally next in UE 5.8+ (suggested lock 5.8.4) (`PixelSideRL.uproject`, module `Build.cs`, and generated project files). `Source/` currently contains affix interface headers only.

## Read first

- [Docs/UE_PROJECT_SPEC.md](Docs/UE_PROJECT_SPEC.md) — engine version, folder layout, naming, and MVP scope
- [Docs/GDD.md](Docs/GDD.md) — gameplay, run loop, and economy

## What’s in the tree

- `Docs/` — GDD, combat feel, roguelike meta, room pipeline, art bible, and UI notes
- `Content/` — art PNGs under `Content/Art/`, affix and meta JSON stubs, and room specs under `Content/Maps/Rooms/` (including `Room_Combat_01_Spawns.md`)
- `Source/PixelSideRL/` — C++ contracts for affix types and the affix subsystem
- `Scripts/validate_affix_stub.py` — checks the affix MVP table against the stub schema

## License

Apache License 2.0. See [LICENSE](LICENSE).
