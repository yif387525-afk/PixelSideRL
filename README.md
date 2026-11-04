# PixelSideRL

Pixel side-scroller shooter roguelike for Unreal Engine 5.8+ (suggested lock 5.8.4). Short runs move through a room graph (combat, elite, event, shop, rest), with in-run affix builds and a light permanent meta. Target look is pixel art at a logical 640×360, nearest-neighbor scaled, with an orthographic camera.

This is a practice project. The repository holds the design seed plus a playable combat P0 slice: side-view move (coyote / jump buffer), peashooter, and damageables. Open `PixelSideRL.uproject` in UE 5.8+ (suggested lock 5.8.4). Local sandbox steps, including the 5-frame Jump flipbook, are in [Docs/Combat_P0_LabSetup.md](Docs/Combat_P0_LabSetup.md).

## Read first

- [Docs/UE_PROJECT_SPEC.md](Docs/UE_PROJECT_SPEC.md) — engine version, folder layout, naming, and MVP scope
- [Docs/GDD.md](Docs/GDD.md) — gameplay, run loop, and economy

## What’s in the tree

- `Docs/` — GDD, combat feel, roguelike meta, room pipeline, art bible, and UI notes
- `Content/` — art PNGs under `Content/Art/`, affix, meta, and shop JSON, and room specs under `Content/Maps/Rooms/` (including `Room_Combat_01_Spawns.md`)
- `Source/PixelSideRL/` — game module: affix contracts (read-only from combat) and the combat P0 classes
- `Scripts/validate_affix_stub.py` — checks the affix MVP table, the P0.6 subset, and that stubs stay deprecated
- `Scripts/validate_shop_da.py` — checks `DA_ShopInventory` / `DA_ShopPriceCurve` against RoguelikeMeta §5 and Room_Shop_01 slots
- Room shop open/close: player `AC_ShopPresenter` calls `OpenShop` / `OpenShopDemo` / `CloseShop` (viewport Z 80). See [Content/Maps/Rooms/Room_Shop_01.md](Content/Maps/Rooms/Room_Shop_01.md). F11 still opens the Lab demo directly (DemoScrap 128→98) and does not spend RunScrap. `OpenShop` on the shell reads `GetRunScrap` and spends with `AddRunScrap` after a successful buy or reroll. The presenter itself still only forwards open/close.
- `Scripts/test_affix_p05.cpp` — host check for GetStat folds and the three-choice pool (no Unreal)
- `Scripts/test_event_ops.cpp` — host check for event ops on `DA_Event_AbandonedCache` (no Unreal)
- `Scripts/test_room_combat01.cpp` — host check for the Room_Combat_01 wave clock (no Unreal)

## License

Apache License 2.0. See [LICENSE](LICENSE).
