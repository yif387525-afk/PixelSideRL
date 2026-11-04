# Room_Combat_05 — 正式战斗房模板规格

| 字段 | 内容 |
|------|------|
| Type / Tag | Combat / Mid·Late |
| Owner | 廊道 |
| 状态 | **规格定稿候选** · COMMENT 交审 · 待 UE umap |
| 尺寸 | 设计宽 **2240**（3.5 屏）· **夹击走廊** |
| 意图 | Late 压力：前后/上下夹击；走廊分段 + 中段掩体打断视线 |
| 入口安全区 | 平层 X≈0–320；无进房贴脸 |
| Spawn | Ground×5 + Air×1；W1–W3 |
| 同时存活上限 | ≤6 |
| Exit | Left / Right · Right **`RequireClear`** |
| 清房 | 波次清空 → **三选一** → 开门 |
| 数据 | `Content/Data/Rooms/Room_Combat_05_*.csv` |
| 禁止 | 全长无掩体的纯管道、暗坑、单级 ≥32 必过、进房 Air 贴脸、fake uasset、改 Lab 跳跃/级差 |

**坐标：** 原点 = 左侧入口内侧地面；`+X` 右；`+Y` 上；1 px = 1 uu。  
**手感：** 级差 ≤28；Peek = 沿 X 站掩体对侧。走廊三段：近段 / 中段夹击窗 / 远段。

---

## 1. ASCII 侧视（约 1 字符 ≈ 56 px）

```
Y
28|     [=近台=]          [=远台=]     ·Air·
 0|安==掩==近段====中段夹击====远段====出口=
   +------------------------------------→ X
     0   400  800  1120  1600  2000  2240
```

地面连续无坑；中段掩体打断视线；前后刷点形成夹击。

---

## 2. 几何尺寸表

### 2.1 地面 / 边界

| ID | 类型 | X0 | X1 | TopY | 备注 |
|----|------|----|----|------|------|
| G_main | 地板 | 0 | 2240 | 0 | 走廊主路；**无坑** |
| WALL_L | 边界墙 | -40 | 0 | — | 高 280 |
| WALL_R | 边界墙 | 2240 | 2280 | — | 高 280 |

### 2.2 平台（厚度 16；TopY=28）

| ID | X0 | X1 | TopY | 角色 |
|----|----|----|------|------|
| P_near | 480 | 720 | **28** | 近段仰角/风筝 |
| P_far | 1520 | 1840 | **28** | 远段仰角/风筝 |

无第二层；中段 X≈800–1400 地面开阔作夹击窗。

### 2.3 短掩体（挡弹；Pawn 可穿；打断视线）

| ID | 底心 (X,Y) | 高 | 宽 | 作用 |
|----|------------|----|----|------|
| CV_A | (360, 0) | 44 | 24 | 近段 Peek |
| CV_B | (1040, 0) | 48 | 28 | **中段夹击窗** 主打断 |
| CV_C | (1400, 0) | 44 | 24 | 中远 Peek |
| CV_D | (1960, 0) | 40 | 24 | 出口前 Peek |

禁止拆除中段掩体做成纯管道。

---

## 3. 出口

| ID | Tag | (X,Y) | 规则 |
|----|-----|-------|------|
| Exit_Left | Exit.Left | (-40, 0) | 回退 |
| Exit_Right | Exit.Right | (2160, 0) | **RequireClear**；清房→三选一→开 |

---

## 4. 刷怪点

完整表见 [`Room_Combat_05_Spawns.md`](./Room_Combat_05_Spawns.md)。

| ID | Tag | Wave | (X,Y) | Optional | 备注 |
|----|-----|------|-------|----------|------|
| SP_G1 | Ground | 1 | (560, 0) | false | W1 近 |
| SP_G2 | Ground | 1 | (880, 0) | false | W1 中近 |
| SP_G3 | Ground | 2 | (1280, 0) | false | W2 中远（夹击） |
| SP_G4 | Ground | 2 | (1680, 0) | false | W2 远 |
| SP_G5 | Ground | 3 | (2000, 0) | false | W3 收束 |
| SP_A1 | Air | 2 | (1120, 80) | false | W2 中段上方 |

Air 仅 W2+；不进房贴脸。占位：`Enemy.Grunt.Early` / `Enemy.Flyer.Early`。

---

## 5. 布局锚点

| 锚点 | (X,Y) | 说明 |
|------|-------|------|
| PlayerStart | (48, 0) | 入口安全区 |
| AffixAnchor | (1120, 0) | 三选一参考（中段） |

---

## 6. 数据表

| 文件 | 用途 |
|------|------|
| `Content/Data/Rooms/Room_Combat_05_meta.csv` | 元数据 |
| `Content/Data/Rooms/Room_Combat_05_spawns.csv` | 刷怪 |
| `Content/Data/Rooms/Room_Combat_05_geometry.csv` | 几何/出口 |

`clear_rule=WavesCleared`；`affix_on_clear=true`；`require_clear_exit=Exit.Right`；`max_alive=6`。

---

## 7. 入库验收

- [ ] 走廊分段；中段掩体打断视线；非纯管道  
- [ ] 前后刷点可读夹击；Air 仅 W2+  
- [ ] 级差 ≤28；无暗坑  
- [ ] RequireClear + 三选一；CSV 齐；无 Lab 改动  

---

## 变更

| 日期 | 说明 |
|------|------|
| 2026-11-04 | 细表占位 |
| 2026-11-05 | 正式规格：夹击走廊几何/刷怪/出口/CSV；COMMENT 交审 |
