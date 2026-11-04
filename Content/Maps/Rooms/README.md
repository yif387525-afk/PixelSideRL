# Content/Maps/Rooms

单房间模板规格细表（md 占位）；正式 `umap` 经 UE 创建后提交（Git LFS）。  
规则真相源：`Docs/RoomPipeline.md`（含 §0 已拍板）。

`Room_Combat_01` 已写正式几何 + 刷怪 + 出口（`Room_Combat_01.md` / `Room_Combat_01_Spawns.md`），机读表在 `Content/Data/Rooms/Room_Combat_01_{meta,spawns,geometry}.csv`。尚无 umap / uasset。

`Room_Combat_02` 同样已有正式几何 + 刷怪 + 出口（`Room_Combat_02.md` / `Room_Combat_02_Spawns.md`），机读表在 `Content/Data/Rooms/Room_Combat_02_{meta,spawns,geometry}.csv`。尚无 umap / uasset。一层平台 TopY=28，无坑。

## 建图顺序

`Combat_01/02` → `Boss_01` 骨架（含 `BossAnte_01`）→ `Elite_01` → 其余 Combat → Event/Shop

## 文件一览

| 文件 | Type | 优先级 |
|------|------|--------|
| Room_Combat_01.md | Combat Early · 几何/出口定稿候选 | P0 |
| Room_Combat_01_Spawns.md | Combat_01 刷怪点（相对入口） | P0 |
| Room_Combat_02.md | Combat Early · 一层平台定稿候选 | P0 |
| Room_Combat_02_Spawns.md | Combat_02 刷怪点（相对入口） | P0 |
| Room_BossAnte_01.md | BossAnte | P0（与 Boss 同批） |
| Room_Boss_01.md | Boss | P0 |
| Room_Elite_01.md | Elite | P1 |
| Room_Combat_03.md | Combat Mid | P2 |
| Room_Combat_04.md | Combat Mid | P2 |
| Room_Combat_05.md | Combat Mid/Late | P2 |
| Room_Combat_06.md | Combat Late | P2 |
| Room_Event_01.md | Event 支路 | P3 |
| Room_Shop_01.md | Shop 支路 | P3 |
