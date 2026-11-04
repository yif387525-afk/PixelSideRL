# Room_Boss_01 — 正式 Boss 房骨架规格

| 字段 | 内容 |
|------|------|
| Type / Tag | Boss / Boss |
| Owner | 廊道 |
| 状态 | **骨架规格定稿候选** · COMMENT 交审 · 待 UE umap |
| 尺寸 | 设计宽 **2560**（4 屏）· **双阶段**场地 |
| 意图 | Intro→P1→转场→P2→Outro；总时长约 2.5–3.5 min（不含前厅） |
| 入口 | **仅**自 `Room_BossAnte_01`（`Exit.BossGate`）；进房后锁左门 |
| Spawn | `Spawn.Boss`×1；可选杂兵点（同时 ≤3） |
| 同时存活上限 | Boss 1 + 杂兵 ≤3 |
| Exit | 结算出口 `Exit.Settle`；**Boss 死亡后解锁** |
| 清房 | Boss 死亡 → 结算（**无**三选一） |
| 同批 | 前厅 `Room_BossAnte_01` 已合；本 PR 仅 Boss 房骨架 |
| 数据 | `Content/Data/Rooms/Room_Boss_01_*.csv` |
| 禁止 | 跳过前厅直连、无法躲避死区、杂兵盖过 Boss 可读性、三选一、商店、fake uasset、改 Lab 跳跃/级差 |

**坐标：** 原点 = 左侧入口内侧地面；`+X` 右；`+Y` 上；1 px = 1 uu。  
**手感：** 级差 ≤28；Peek = 沿 X 站掩体对侧。详见 `Docs/RoomPipeline.md` §7.2。

---

## 1. 节奏（关卡侧）

| 阶段 | 时长（约） | 关卡侧要求 |
|------|------------|------------|
| Intro | 3–5s | 锁左门、镜头可读、**无伤害** |
| P1 | 40–60s | 场地**半开放**；杂兵点仅辅助（可选） |
| 转场 | 5–8s | 平台/危害启用或侧通路关闭；明确可读 telegraph |
| P2 | 50–80s | 全场地危害或夹击点激活；**禁止**无法躲避死区 |
| Outro | — | Boss 死亡 → 解锁 `Exit.Settle` |

失败回收：玩家死亡回 Hub / 结算；**不**中途改房间几何。

---

## 2. ASCII 侧视（约 1 字符 ≈ 80 px）

```
Y
56|              [====P2台====]
28|   [=侧台L=]              [=侧台R=]
 0|门=安全==半开放主场========远场====结算=
   +------------------------------------→ X
     0   480  960   1600   2080    2560
```

P1：中路开阔 + 两侧短台可选；转场后 `Haz_A/B` 与 `P_p2` 启用，侧通路可读关闭。

---

## 3. 几何尺寸表

### 3.1 地面 / 边界

| ID | 类型 | X0 | X1 | TopY | 备注 |
|----|------|----|----|------|------|
| G_main | 地板 | 0 | 2560 | 0 | 主场连续；**无暗坑** |
| WALL_L | 边界墙 | -40 | 0 | — | 高 280；Intro 后入口锁 |
| WALL_R | 边界墙 | 2560 | 2600 | — | 高 280 |

### 3.2 平台（厚度 16；级差 ≤28）

| ID | X0 | X1 | TopY | 阶段 | 角色 |
|----|----|----|------|------|------|
| P_sideL | 400 | 640 | **28** | P1+ | 左侧半开放高台 |
| P_sideR | 1760 | 2080 | **28** | P1+ | 右侧半开放高台 |
| P_p2 | 1120 | 1520 | **56** | **P2** | 转场后启用中央高台（经侧台 +28→+56） |

跳接：地面→侧台 (+28)→`P_p2`（再 +28）。无单级 ≥32 必过。

### 3.3 短掩体（挡弹；Pawn 可穿）

| ID | 底心 (X,Y) | 高 | 宽 | 作用 |
|----|------------|----|----|------|
| CV_A | (720, 0) | 44 | 24 | P1 近场 Peek |
| CV_B | (1440, 0) | 40 | 24 | 中场 Peek |
| CV_C | (2160, 0) | 44 | 24 | 远场 / 结算前 Peek |

### 3.4 转场危害锚点（骨架占位；P2 启用）

| ID | 底心 (X,Y) | 规则 | 备注 |
|----|------------|------|------|
| Haz_A | (960, 0) | TelegraphThenActive | 转场可读；禁止持续无窗死区 |
| Haz_B | (1680, 0) | TelegraphThenActive | 夹击点；须有躲避窗 |

---

## 4. 出口

| ID | Tag | (X,Y) | 规则 |
|----|-----|-------|------|
| Exit_FromAnte | Exit.FromAnte | (-40, 0) | 仅来自前厅；Intro 后锁，不可回退战斗中 |
| Exit_Settle | Exit.Settle | (2480, 0) | **BossDefeated** 后解锁；进结算；无三选一 |

---

## 5. 刷怪点

完整表见 [`Room_Boss_01_Spawns.md`](./Room_Boss_01_Spawns.md)。

| ID | Tag | Wave | (X,Y) | Optional | 备注 |
|----|-----|------|-------|----------|------|
| SP_Boss | Boss | 0 | (1280, 0) | false | **唯一** Boss 本体点 |
| SP_A1 | Add | 1 | (640, 0) | **true** | P1 可选杂兵 |
| SP_A2 | Add | 1 | (1920, 0) | **true** | P1 可选杂兵 |
| SP_A3 | Add | 2 | (1280, 72) | **true** | P2 可选空中杂兵 |

杂兵同时 ≤3；限频；不得盖过 Boss 可读性。占位：`Enemy.Boss.MVP` / `Enemy.Grunt.Early` / `Enemy.Flyer.Early`。

---

## 6. 布局锚点

| 锚点 | (X,Y) | 说明 |
|------|-------|------|
| PlayerStart | (80, 0) | 自前厅进入落点 |
| CameraIntro | (640, 80) | Intro 镜头参考 |
| AffixAnchor | — | **不使用**（无三选一） |

---

## 7. 数据表

| 文件 | 用途 |
|------|------|
| `Content/Data/Rooms/Room_Boss_01_meta.csv` | 元数据 |
| `Content/Data/Rooms/Room_Boss_01_spawns.csv` | Boss + 可选杂兵 |
| `Content/Data/Rooms/Room_Boss_01_geometry.csv` | 几何/出口/危害占位 |

`clear_rule=BossDefeated`；`affix_on_clear=false`；`require_clear_exit=Exit.Settle`。

---

## 8. 入库验收

- [ ] 仅经 BossAnte 进入；Intro 锁左门  
- [ ] `Spawn.Boss` 唯一；杂兵 Optional 且同时 ≤3  
- [ ] 转场 telegraph 可读；P2 无无法躲避死区  
- [ ] Boss 死后开 `Exit.Settle`；无三选一  
- [ ] CSV 齐；登记表更新；无 Lab / Combat / BossAnte 改动  

---

## 变更

| 日期 | 说明 |
|------|------|
| 2026-11-04 | 细表占位 |
| 2026-11-05 | 骨架正式规格：双阶段几何/刷怪/出口/危害占位/CSV；COMMENT 交审 |
