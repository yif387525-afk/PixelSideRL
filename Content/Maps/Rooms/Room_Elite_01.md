# Room_Elite_01 — 正式精英房骨架规格

| 字段 | 内容 |
|------|------|
| Type / Tag | Elite / Mid |
| Owner | 廊道 |
| 状态 | **骨架规格定稿候选** · COMMENT 交审 · 待 UE umap |
| 尺寸 | 设计宽 **1920**（3 屏）· **中央竞技场** |
| 意图 | 精英主威胁 + 少量杂兵铺场；近战压迫线 + 风筝线各一条 |
| 入口安全区 | 平层 X≈0–280；精英点远离入口 |
| Spawn | Ground×4 + Elite×1；W1 杂兵 → W2 精英登场 |
| 同时存活上限 | ≤8（含精英） |
| Exit | Left / Right · Right **`RequireClear`** |
| 清房 | 精英+杂兵清空 → **三选一** → 开门 |
| 数据 | `Content/Data/Rooms/Room_Elite_01_*.csv` |
| 禁止 | 多精英、精英与出口门重叠、进房精英贴脸、暗坑、单级 ≥32 必过、fake uasset、改 Lab 跳跃/级差、直连 Boss |

**坐标：** 原点 = 左侧入口内侧地面；`+X` 右；`+Y` 上；1 px = 1 uu。  
**手感：** 级差 ≤28；Peek = 沿 X 站掩体对侧。波次：W1 杂兵铺场 → W2 精英登场（`Spawn.Elite`）。

---

## 1. ASCII 侧视（约 1 字符 ≈ 50 px）

```
Y
28|      [=侧台L=]              [=侧台R=]
 0|安==掩==中央开阔竞技场====掩====出口=
   +--------------------------------→ X
     0   400  640   960  1280  1600 1920
              ★Elite(中后)
```

中央开阔；两侧短台 + 掩体；精英出生中后场可读，非贴脸。

---

## 2. 几何尺寸表

### 2.1 地面 / 边界

| ID | 类型 | X0 | X1 | TopY | 备注 |
|----|------|----|----|------|------|
| G_main | 地板 | 0 | 1920 | 0 | 中央连续；**无坑** |
| WALL_L | 边界墙 | -40 | 0 | — | 高 280 |
| WALL_R | 边界墙 | 1920 | 1960 | — | 高 280 |

### 2.2 平台（厚度 16；TopY=28）

| ID | X0 | X1 | TopY | 角色 |
|----|----|----|------|------|
| P_L | 360 | 560 | **28** | 左侧风筝/仰角 |
| P_R | 1360 | 1600 | **28** | 右侧风筝/仰角 |

无第二层；中路 X≈600–1300 地面开阔可冲。

### 2.3 短掩体（挡弹；Pawn 可穿）

| ID | 底心 (X,Y) | 高 | 宽 | 作用 |
|----|------------|----|----|------|
| CV_A | (280, 0) | 44 | 24 | 入口侧 Peek |
| CV_B | (880, 0) | 40 | 24 | 中央侧 Peek |
| CV_C | (1480, 0) | 44 | 24 | 远场 Peek |

---

## 3. 出口

| ID | Tag | (X,Y) | 规则 |
|----|-----|-------|------|
| Exit_Left | Exit.Left | (-40, 0) | 回退 |
| Exit_Right | Exit.Right | (1840, 0) | **RequireClear**；清房→三选一→开 |

---

## 4. 刷怪点

完整表见 [`Room_Elite_01_Spawns.md`](./Room_Elite_01_Spawns.md)。

| ID | Tag | Wave | (X,Y) | Optional | 备注 |
|----|-----|------|-------|----------|------|
| SP_G1 | Ground | 1 | (480, 0) | false | W1 左 |
| SP_G2 | Ground | 1 | (720, 0) | false | W1 中左 |
| SP_G3 | Ground | 1 | (1200, 0) | false | W1 中右 |
| SP_G4 | Ground | 1 | (1520, 0) | false | W1 右 |
| SP_E1 | Elite | 2 | (1100, 0) | false | **唯一**精英；中后场 |

禁止第二精英点；精英点不得与 Exit_Right 重叠。

---

## 5. 布局锚点

| 锚点 | (X,Y) | 说明 |
|------|-------|------|
| PlayerStart | (48, 0) | 入口安全区 |
| AffixAnchor | (960, 0) | 三选一参考 |

---

## 6. 数据表

| 文件 | 用途 |
|------|------|
| `Content/Data/Rooms/Room_Elite_01_meta.csv` | 元数据 |
| `Content/Data/Rooms/Room_Elite_01_spawns.csv` | 刷怪 |
| `Content/Data/Rooms/Room_Elite_01_geometry.csv` | 几何/出口 |

`clear_rule=WavesCleared`；`affix_on_clear=true`；`require_clear_exit=Exit.Right`；`max_alive=8`。

占位：Ground=`Enemy.Grunt.Early`；Elite=`Enemy.Elite.Mid`。

---

## 7. 入库验收

- [ ] 中央开阔 + 两侧台/掩体；无暗坑  
- [ ] 唯一 Elite 点中后场可读、非贴脸、不压出口  
- [ ] W1 杂兵 → W2 精英；同时 ≤8  
- [ ] RequireClear + 三选一；CSV 齐；无 Lab 改动  

---

## 变更

| 日期 | 说明 |
|------|------|
| 2026-11-04 | 细表占位 |
| 2026-11-05 | 骨架正式规格：中央竞技场几何/刷怪/出口/CSV；COMMENT 交审 |
