# Content/Maps/Rooms

单房间模板规格细表（md 占位）；正式 `umap` 经 UE 创建后提交（Git LFS）。  
规则真相源：`Docs/RoomPipeline.md`（含 §0 已拍板）。

## 建图顺序

`Combat_01/02` → `Boss_01` 骨架（含 `BossAnte_01`）→ `Elite_01` → 其余 Combat → Event/Shop

## 文件一览

| 文件 | Type | 优先级 |
|------|------|--------|
| Room_Combat_01.md | Combat Early | P0 |
| Room_Combat_01_Spawns.md | Combat_01 刷怪点像素表 | P0 |
| Room_Combat_02.md | Combat Early | P0 |
| Room_BossAnte_01.md | BossAnte | P0（与 Boss 同批） |
| Room_Boss_01.md | Boss | P0 |
| Room_Elite_01.md | Elite | P1 |
| Room_Combat_03.md | Combat Mid | P2 |
| Room_Combat_04.md | Combat Mid | P2 |
| Room_Combat_05.md | Combat Mid/Late | P2 |
| Room_Combat_06.md | Combat Late | P2 |
| Room_Event_01.md | Event 支路 | P3 |
| Room_Shop_01.md | Shop 支路 | P3 |
