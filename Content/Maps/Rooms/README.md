# Content/Maps/Rooms

单房间模板规格细表（md 占位）；正式 `umap` 经 UE 创建后提交（Git LFS）。  
规则真相源：`Docs/RoomPipeline.md`（含 §0 已拍板）。

`Room_Combat_01` 已写正式几何 + 刷怪 + 出口（`Room_Combat_01.md` / `Room_Combat_01_Spawns.md`），机读表在 `Content/Data/Rooms/Room_Combat_01_{meta,spawns,geometry}.csv`。P0.7 由 `APixelRoomDirector` 读这些 CSV（验收：进房 → 两波 → 三选一 → 右门开）。尚无 umap / uasset。

`Room_Combat_02` 同样已有正式几何 + 刷怪 + 出口（`Room_Combat_02.md` / `Room_Combat_02_Spawns.md`），机读表在 `Content/Data/Rooms/Room_Combat_02_{meta,spawns,geometry}.csv`。尚无 umap / uasset。一层平台 TopY=28，无坑。

`Room_Combat_03` 已有正式几何 + 刷怪 + 出口（`Room_Combat_03.md` / `Room_Combat_03_Spawns.md`），机读表在 `Content/Data/Rooms/Room_Combat_03_{meta,spawns,geometry}.csv`。尚无 umap / uasset。Mid 双高台：次台/踏板 TopY=28，主台 TopY=56，中路可冲，无坑。

`Room_BossAnte_01` 已有骨架几何 + 可选引导 + BossGate 出口（`Room_BossAnte_01.md` / `Room_BossAnte_01_Spawns.md`），机读表在 `Content/Data/Rooms/Room_BossAnte_01_{meta,spawns,geometry}.csv`。尚无 umap / uasset。平层宽 1280，无坑，无战斗波，无三选一。

`Room_Boss_01` 已有骨架几何 + 刷怪 + 出口（`Room_Boss_01.md` / `Room_Boss_01_Spawns.md`），机读表在 `Content/Data/Rooms/Room_Boss_01_{meta,spawns,geometry}.csv`。尚无 umap / uasset。双阶段宽 2560，仅经前厅进入，Boss 死亡后进结算，无三选一。

`Room_Elite_01` 已有骨架几何 + 刷怪 + 出口（`Room_Elite_01.md` / `Room_Elite_01_Spawns.md`），机读表在 `Content/Data/Rooms/Room_Elite_01_{meta,spawns,geometry}.csv`。尚无 umap / uasset。中央竞技场宽 1920，两侧台 TopY=28，无坑。

`Room_Elite_02` 已有骨架几何 + 刷怪 + 出口（`Room_Elite_02.md` / `Room_Elite_02_Spawns.md`），机读表在 `Content/Data/Rooms/Room_Elite_02_{meta,spawns,geometry}.csv`。尚无 umap / uasset。高台压制，宽 1920，精英台 TopY=28，与 Elite_01 进池轮换。

`Room_Combat_04` 已有正式几何 + 刷怪 + 出口（`Room_Combat_04.md` / `Room_Combat_04_Spawns.md`），机读表在 `Content/Data/Rooms/Room_Combat_04_{meta,spawns,geometry}.csv`。尚无 umap / uasset。坑 720–960 宽 240，常驻桥 TopY=28，落坑软回收。

## 建图顺序

`Combat_01/02` → `Boss_01` 骨架（含 `BossAnte_01`）→ `Elite_01` → 其余 Combat → Event/Shop

## 文件一览

| 文件 | Type | 优先级 |
|------|------|--------|
| Room_Combat_01.md | Combat Early · 几何/出口定稿候选 | P0 |
| Room_Combat_01_Spawns.md | Combat_01 刷怪点（相对入口） | P0 |
| Room_Combat_02.md | Combat Early · 一层平台定稿候选 | P0 |
| Room_Combat_02_Spawns.md | Combat_02 刷怪点（相对入口） | P0 |
| Room_BossAnte_01.md | BossAnte · 骨架定稿候选 | P0（与 Boss 同批） |
| Room_BossAnte_01_Spawns.md | BossAnte 可选引导点（相对入口） | P0（与 Boss 同批） |
| Room_Boss_01.md | Boss · 骨架定稿候选 | P0 |
| Room_Boss_01_Spawns.md | Boss_01 刷怪点（相对入口） | P0 |
| Room_Elite_01.md | Elite · 骨架定稿候选 | P1 |
| Room_Elite_01_Spawns.md | Elite_01 刷怪点（相对入口） | P1 |
| Room_Elite_02.md | Elite · 高台压制定稿候选 | P1 |
| Room_Elite_02_Spawns.md | Elite_02 刷怪点（相对入口） | P1 |
| Room_Combat_03.md | Combat Mid · 双高台定稿候选 | P2 |
| Room_Combat_03_Spawns.md | Combat_03 刷怪点（相对入口） | P2 |
| Room_Combat_04.md | Combat Mid · 坑+桥定稿候选 | P2 |
| Room_Combat_04_Spawns.md | Combat_04 刷怪点（相对入口） | P2 |
| Room_Combat_05.md | Combat Mid/Late | P2 |
| Room_Combat_06.md | Combat Late | P2 |
| Room_Event_01.md | Event 支路 | P3 |
| Room_Shop_01.md | Shop 支路 | P3 |
