# GDD — 像素横版射击 Roguelike（MVP）

**工程：** PixelSideRL（UE 5.8+）  
**对齐：** `UE_PROJECT_SPEC.md`  
**日活可运行目标：** `Maps/Run_Test` 能开一局并结算。

---

## 1. 一句话与核心循环

**一句话：** 侧视像素枪战 + 短局构筑；手感优先于数值膨胀。

**核心循环：**
Hub 出发 → 房间图推进（战斗 / 精英 / 事件 / 商店 / 休息）→ 层末 Boss → 结算回 Hub → 少量永久成长 → 再开。

| 指标 | MVP 目标 |
|------|----------|
| 单局时长 | 15–20 分钟（中位数） |
| 角色 / 枪 / 词条 | 1 · 3 · 8–12 |
| 敌 + Boss | 3–5 + 1 |
| 房间模板库存 | 8–12 |
| 永久成长 | 轻量（详见 `RoguelikeMeta.md`） |

---

## 2. 局内循环（Run Loop）

### 2.1 状态机

```
[Hub]
  │ StartRun
  ▼
[GenerateRunGraph] ──► [EnterRoom]
  │                        │
  │                        ├─ Combat/Elite ──► Clear ──► AffixChoice(必出) ──► OpenDoor
  │                        ├─ Event        ──► Resolve ─────────────────────► OpenDoor
  │                        ├─ Shop         ──► Buy/Skip ────────────────────► OpenDoor
  │                        └─ Rest/Altar   ──► Heal|Buff 二选一 ─────────────► OpenDoor
  │                                             │
  │                        ◄── SelectNextNode ◄─┘（地图 UI 预览类型，不预览词条）
  │
  ├─ 层内节点未尽 ──► [EnterRoom]
  ├─ 层末 ──► [BossRoom] ──► AffixChoice(高稀有) ──► 下层 or 结算
  │
  ├─ 玩家倒地 ──► [RunFail] ──► [RunSummary] ──► Hub（仅永久货币）
  └─ 层2 Boss 击杀 ──► [RunWin] ──► [RunSummary] ──► Hub
```

**规则：** 不允许回退已清房；门仅在清房 / 事件结算后开启。

### 2.2 层结构（定稿）

| 项 | 值 |
|----|-----|
| 层数 | 固定 **2 层** |
| 每层战斗节点 | **5**（含可选支路） |
| 每层层末 | **1 Boss** |
| 整局可进房间 | 约 **10–12**（含商店 / 事件 / 休息） |
| 模板库存 | 匹配 8–12 房间模板（见 `RoomPipeline.md`） |

### 2.3 房间类型与权重（MVP）

| 类型 | 权重 | 说明 |
|------|------|------|
| 普通战斗 | 55% | 清房必出三选一（普通池） |
| 精英 | 15% | 更高掉落；清房必出三选一（精英池，含 1 次「更好池」） |
| 事件 | 10% | 风险收益；无战斗或轻战斗 |
| 商店 | 10% | 只花 Scrap |
| 休息 / 祭坛 | 10% | 回血 **或** 小强化，二选一 |

节点推进：清房 → 开门 → 选下一节点（`WBP_RunMap` 预览类型图标，不预览具体词条 / 敌人名单）。

### 2.4 资源与生命

| 资源 | 规则 |
|------|------|
| **Scrap（局内）** | 商店唯一通货；击杀掉落 + 房清奖励。拾取可表现为 Coin，并入局内 Scrap；结算按剩余 Scrap×`ScrapToCore` 折 Core |
| **Core（永久）** | 结算入账 `FMetaSaveData.Core`；Hub 永久节点消费。与 `DA_Economy` / `RoguelikeMeta.md` 一致 |
| **HP** | 倒地 = 本局失败 |
| **护甲** | 可选词条提供；受击优先耗护甲层 |

失败结算：只保留永久货币 **Core**；局内 Scrap / Coin / 词条 / 临时枪全部清空。

### 2.5 三选一触发点

1. 每清完一个**普通 / 精英战斗房** → 必出 1 次三选一（精英用更好稀有度偏置）。
2. **Boss 战后** → 1 次高稀有度三选一。
3. **商店**可购「随机三选一券」（贵）。

UI：`WBP_AffixChoice`；暂停战斗；选 1 加入 `AC_AffixInventory`。算法与词条表见 `RoguelikeMeta.md`。

### 2.6 武器

- 开局自带 **1** 把（默认 Peashooter，可被 Meta 微调）。
- 局内可捡到另外 2 把枪的**临时强化或替换**（MVP 三枪均可获得）。
- 同时只装备 **1 主武器**；换枪在房间间隙或快捷键（`IA_SwapWeapon`）。
- 真相源：`Content/Data/Weapons/DA_Weapon_*`。

### 2.7 失败态

| 失败态 | 处理 |
|--------|------|
| 玩家 HP ≤ 0 | 进入 `RunFail` → 结算 → Hub |
| 卡死 / 掉出世界 | 软重置当前房（保留 HP/词条，记一次 Debug 计数） |
| 三选一 UI 未选退出 | 禁止；必须选 1 张才能继续 |

---

## 3. 战斗支柱（侧视射击）

### 3.1 移动与瞄准

| 能力 | MVP |
|------|-----|
| 跑 | 必做 |
| 跳 | 必做 |
| 蹲 / 滑 | 可选（Post-MVP 优先滑） |
| 瞄准 | 鼠标 / 右摇杆自由瞄准；侧视弹道 |
| 相机 | 正交；主轴水平滚动（规范强制） |

组件建议：`AC_SideMove`、`AC_AimSide`、`AC_WeaponController`。

### 3.2 MVP 三枪

每枪 1 句定位 + 建议 `DA_Weapon_*` 字段：`Damage`, `FireRate`, `Spread`, `Mag`, `Reload`, `ProjectileSpeed`, `Knockback`。

| Id / DA | 名称 | 定位 | 建议初值（可调） |
|---------|------|------|------------------|
| `DA_Weapon_Peashooter` | Peashooter 豆射手 | 基础全自动，低伤高射速，教学枪 | Dmg 6 · FR 8/s · Spread 3° · Mag 30 · Reload 1.2s · ProjSpd 1200 · KB 50 |
| `DA_Weapon_Scattergun` | Scattergun 散射 | 近中距爆发，弹药少 | Dmg 4×6 pellets · FR 1.2/s · Spread 18° · Mag 6 · Reload 2.0s · ProjSpd 900 · KB 180 |
| `DA_Weapon_RailPierce` | Rail Pierce 穿透轨 | 低射速高伤穿透，需预判 | Dmg 28 · FR 0.7/s · Spread 0° · Mag 4 · Reload 1.8s · ProjSpd 2200 · Pierce 1 · KB 120 |

### 3.3 受击原则

- 受击无敌帧、击退、受击闪白。
- 具体帧数 / 曲线 / 屏幕震动 → `CombatFeel.md`；此处只定「必须有可读反馈，禁止无反馈秒杀」。

---

## 4. 敌人与 Boss（MVP）

难度不靠纯加血：优先 **密度、弹幕阅读、混合编队**。数据：`Content/Data/Enemies/DA_Enemy_*`，强度乘区读 `DT_DifficultyCurve`。

| Id | 名称 | 行为与威胁（一句话） |
|----|------|----------------------|
| `DA_Enemy_Grunt` | Grunt 步兵 | 水平射慢弹，压走位 |
| `DA_Enemy_Hopper` | Hopper 跳虫 | 近身跳跃扑杀，逼近战反应 |
| `DA_Enemy_Drone` | Drone 浮游 | 斜向游走点射，占空域 |
| `DA_Enemy_ShieldCap` | ShieldCap 盾兵 | 正面挡弹，需绕后或爆破（Scattergun 克制） |
| `DA_Enemy_ForgeWarden` | Boss **Forge Warden 锻关守卫** | 两阶段：弹幕墙 + 召唤 Grunt；破盾窗口；场地约 1.5 屏宽 |

**编队原则：** 热身单种 → 成型 2 种混合 → 终局混合 + 盾兵。精英房提高 `EliteChance` 与掉落。

**失败态：** AI 卡住超 8s 强制重新寻路 / 落点；Boss 阶段切换失败则回退到阶段 1 并记日志。

---

## 5. 难度曲线

| 阶段 | 房间序号（约） | 敌人强度系数 | 编队规则 | 掉落 / 三选一 |
|------|----------------|--------------|----------|----------------|
| 热身 | 层1 房1–2 | 0.85–1.0 | 单种杂兵 | 普通池 |
| 成型 | 层1 房3–5 | 1.0–1.15 | 2 种混合 | 普通 + 少量稀有 |
| 层末 | 层1 Boss | 1.2 | Boss 机制 | 稀有权重 ↑ |
| 加压 | 层2 房1–3 | 1.15–1.35 | 精英更容易刷出 | 稀有 ↑ |
| 终局 | 层2 房4–5 + Boss | 1.4–1.6 | 混合 + 盾兵 | Boss 后史诗权重 |

**玩家战力预期**
- 第 3 次三选一后应出现可识别「雏形构筑」。
- 层2 应感到**构筑差异大于数值碾压**。

**失败点设计**
- 层1 Boss 前应有一次「血线警告」（低血 HUD / 休息房权重微升，二选一即可）。
- 主要挫败来自走位 / 弹幕阅读，而非随机秒杀。

**可调参入口：** `DT_DifficultyCurve`  
字段建议：`StageId`, `EnemyPowerMul`, `EliteChance`, `AffixRarityBias`（Common/Rare/Epic 权重向量）。

---

## 6. 结算与永久成长（轻量）

- 胜利 / 失败都回 Hub（`Maps/Hub`）。
- 结算发 **Core（永久）**，参数来自 `DA_Economy`（定稿见 `RoguelikeMeta.md` §6）：

```
Core = floor(
    ( BaseClearBonus * (Cleared ? 1.0 : DeathMult)
    + RoomsCleared * PerRoom
    + BossKilled * BossBonus
    + RunScrapRemain * ScrapToCore
    ) * MetaCoreMult
)
Core = min(Core, SoftCapPerRun)
```

- 局内商店花 **Scrap**；拾取可为 Coin 并入局内 Scrap。勿与 Core 混存。
- MVP 永久：5 节点（开局 HP / 开局 Scrap / 豆射手微幅等），**不改变局内词条公式**。明细 → `RoguelikeMeta.md` §7。

结算 UI：`WBP_RunSummary`（时长、击杀、构筑列表、入账 Core）。

---

## 7. MVP 验收清单

- [ ] 能从 Hub 进入 `Run_Test` 并开局
- [ ] 打完 2 层 + 两层 Boss（或至少层2 Boss）并可胜利结算
- [ ] 三选一单局至少触发 **8** 次
- [ ] 3 枪（Peashooter / Scattergun / Rail）局内都能拿到并装备
- [ ] 死亡有失败结算，仅保留 Core
- [ ] 中位时长落在 **15–20 分钟**
- [ ] 武器 / 敌人 / 词条均由 `DA_` / `DT_` 驱动，蓝图只读表
- [ ] 不允许回退已清房；倒地即失败

---

## 8. 非目标（MVP 不做）

- 多人 / 联机
- 复杂剧情与过场
- 超过 2 层
- 宠物 / 召唤物常驻
- 枪械改造树（仅临时替换 / 合约）
- 随机地图几何（只用房间模板拼）
- 直接用永久成长复制局内核心词条效果（暴击、穿透等）

---

## 数值 / 蓝图入口速查

| 系统 | 入口 |
|------|------|
| 难度 | `DT_DifficultyCurve` |
| 武器 | `Content/Data/Weapons/DA_Weapon_*` |
| 敌人 | `Content/Data/Enemies/DA_Enemy_*` |
| 词条 | `Content/Data/Affixes/` + `RoguelikeMeta.md` |
| 房间 | `Content/Data/Rooms/` + `Blueprints/Rooms/` |
| 局内 Buff | `AC_AffixInventory` / `Blueprints/Roguelike/` |
| 永久 | `Blueprints/Meta/` |
