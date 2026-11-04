# Room_BossAnte_01 — 正式 Boss 前厅模板规格

| 字段 | 内容 |
|------|------|
| Type / Tag | BossAnte / BossGate |
| Owner | 廊道 |
| 状态 | **骨架规格定稿候选** · COMMENT 交审 · 待 UE umap |
| 尺寸 | 设计宽 **1280**（2 屏）· 平层主路 |
| 意图 | 换气、Boss 预告、锁门演出前缓冲；主路径水平清晰 |
| 入口安全区 | 全房默认安全；出生 X≈0–200 |
| Spawn | **默认无**；可选 ≤2 无伤害引导（`Optional=true`，默认可关） |
| 同时存活上限 | ≤2（仅引导开启时） |
| Exit | Left 回退不锁；Right 前进 → `Room_Boss_01`（可短演出后开） |
| 清房 | **无** `RequireClear`；**无**三选一；**无**商店 |
| 同批 | 与 `Room_Boss_01` 同批骨架（本 PR 仅前厅） |
| 数据 | `Content/Data/Rooms/Room_BossAnte_01_*.csv` |
| 禁止 | 战斗波次、致死陷阱、暗坑、商店、三选一、fake uasset、改 Lab 跳跃/级差、Combat/Elite 直连 Boss |

**坐标：** 原点 = 左侧入口内侧地面；`+X` 右；`+Y` 上；1 px = 1 uu。  
**手感：** 无必过级差；氛围平台若有则 TopY≤28。详见 `Docs/RoomPipeline.md` §7.1。

---

## 1. ASCII 侧视（约 1 字符 ≈ 40 px）

```
Y
28|                 [==门楣台==]
 0|==安全换气========氛围=======Boss门==
   +--------------------------------→ X
     0    320   640   960      1280
```

主路径地面连续无坑；右端为进 Boss 的大门（`Exit.BossGate`）。

---

## 2. 几何尺寸表

### 2.1 地面 / 边界

| ID | 类型 | X0 | X1 | TopY | 备注 |
|----|------|----|----|------|------|
| G_main | 地板 | 0 | 1280 | 0 | 主路平层；**无坑** |
| WALL_L | 边界墙 | -40 | 0 | — | 高 280 |
| WALL_R | 边界墙 | 1280 | 1320 | — | 高 280；门洞另见出口 |

### 2.2 氛围平台（可选，厚度 16；TopY=28）

| ID | X0 | X1 | TopY | 角色 |
|----|----|----|------|------|
| P_lintel | 960 | 1160 | **28** | 门楣侧短台；装饰/走位，非必过 |

无第二层；无战斗压制台。

### 2.3 短掩体（可选氛围；挡弹；Pawn 可穿）

| ID | 底心 (X,Y) | 高 | 宽 | 作用 |
|----|------------|----|----|------|
| CV_A | (480, 0) | 40 | 24 | 氛围柱/残垣；非战斗 Peek 必需 |

---

## 3. 出口

| ID | Tag | (X,Y) | 规则 |
|----|-----|-------|------|
| Exit_Left | Exit.Left | (-40, 0) | 回退；不锁 |
| Exit_BossGate | Exit.BossGate | (1200, 0) | 前进进 `Room_Boss_01`；**无** RequireClear；可播短预告演出后再开 |

拼局规则：Combat/Elite **禁止**直连 Boss；进 Boss **必须**经本前厅。

---

## 4. 刷怪点（可选引导）

完整表见 [`Room_BossAnte_01_Spawns.md`](./Room_BossAnte_01_Spawns.md)。

| ID | Tag | Wave | (X,Y) | Optional | 备注 |
|----|-----|------|-------|----------|------|
| SP_G1 | Guide | 0 | (560, 0) | **true** | 无伤害引导；默认关 |
| SP_G2 | Guide | 0 | (880, 0) | **true** | 无伤害引导；默认关 |

无战斗 WaveGroup；无 `Enemy.Grunt` / Flyer 战斗刷点。

---

## 5. 布局锚点

| 锚点 | (X,Y) | 说明 |
|------|-------|------|
| PlayerStart | (48, 0) | 入口安全区 |
| BossPreview | (1100, 0) | Boss 预告镜头/门扉参考 |
| AffixAnchor | — | **不使用**（无三选一） |

---

## 6. 数据表

| 文件 | 用途 |
|------|------|
| `Content/Data/Rooms/Room_BossAnte_01_meta.csv` | 元数据 |
| `Content/Data/Rooms/Room_BossAnte_01_spawns.csv` | 可选引导点 |
| `Content/Data/Rooms/Room_BossAnte_01_geometry.csv` | 几何/出口 |

列约定同 Combat 房：`Floor`/`Platform` 用 `x0`–`x1`；点位 `x0`=X、`x1`=Y。`clear_rule=None`；`affix_on_clear=false`；`require_clear_exit` 空。

---

## 7. 入库验收

- [ ] 无战斗波次；引导点默认 Optional  
- [ ] 前进门 Tag=`Exit.BossGate` → Boss_01；无 RequireClear / 无三选一  
- [ ] 地面连续无坑；无致死陷阱  
- [ ] CSV 齐；登记表更新；无 Lab 改动；无 fake uasset  

---

## 变更

| 日期 | 说明 |
|------|------|
| 2026-11-04 | 细表占位 |
| 2026-11-05 | 骨架正式规格：几何/可选引导/BossGate 出口/CSV；COMMENT 交审 |
