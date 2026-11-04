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

### 2.8 事件房（支路 · MVP）

**规则（与 `RoomPipeline` 对齐）：** 无战斗、门不锁、无三选一；交互完成或走门跳过均可立刻离开。与 Shop **同 Run 最多共占 1 条支路**。

**默认模板：** `Room_Event_01` → `Interact_Main` 挂 `DA_Event_AbandonedCache`（`Content/Data/Events/`）。

| 字段 | 定稿 |
|------|------|
| EventId | `AbandonedCache` |
| 标题 | 废弃补给箱 |
| 正文 | 封条已经裂开。小心撬开能摸到 Scrap；硬拽更快，但箱缘会割手。也可以直接走开。 |
| SkipHint | 走开（无奖励） |

| ChoiceId | 按钮 | 效果（有序） |
|----------|------|--------------|
| `CarefulLoot` | 小心撬开 | `GrantScrap 25` |
| `ForceLoot` | 硬拽出来 | `DamageSelf 15` → `GrantScrap 50`（DamageSelf 后 CurrentHP 下限 **1**，事件不可直接致死） |

**Op 白名单（MVP）：** `GrantScrap` / `DamageSelf` / `HealSelf` / `None`。不发词条、不改 Core。  
**UI：** `WBP_EventChoice`（或等价）；打开期间可暂停移动开火；门仍可走。C++ 壳见 `Docs/UI_Wireframes_Event.md`：Lab 用 `F10` 打开，只关移动/开火输入，不 `SetGamePaused`。确认选项调用一次 `ResolveEventChoice`（壳不自己加减 Scrap / HP）；Skip / Esc 不调用。`F8` / `F9` 仍是直调 Resolve。`F11` 仍是商店。扩池只追加 `DT_Event_MVP` 行，不改已锁数值。

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

- 局内商店花 **Scrap**；拾取可为 Coin 并入局内 Scrap。勿与 Core 混存。Lab 商店壳是 **F11**（非 Shipping），只改本地 DemoScrap（128，买回复后 98），不调用 `AddRunScrap`。`F10` 仍是事件壳，不拿来开商店。非 Demo 的 `OpenShop` 读 `GetRunScrap`，买/刷成功再 `AddRunScrap(-Price)` / `AddRunScrap(-RerollCost)`。流程开/关仍是 `AC_ShopPresenter`，该组件自己不扣款。
- MVP 永久：5 节点（开局 HP / 开局 Scrap / 豆射手微幅等），**不改变局内词条公式**。明细 → `RoguelikeMeta.md` §7。

结算 UI：`WBP_RunSummary`（时长、击杀、构筑列表、入账 Core）。

---


---

## 6.5 近端落地顺序（主管夜间裁定 · 2026-11-05）

**手感本版签收：** `Docs/CombatFeel.md` 当前参数表（移动 / 三枪骨架 / 受击 / FireInterval÷FireRate 乘区）视为 **已签收**，后续改表须走变更记录，不得 silently 漂移。

| 序号 | 里程碑 | Owner 建议 | 可玩定义（退出条件） |
|------|--------|------------|----------------------|
| **P0.5** | **Grunt 最小敌** | 弹芯 + 廊道刷点 | `DA_Enemy_Grunt`：水平慢弹、可被豆射手击杀、受击闪白/死亡；Lab 或 `Run_Test` 单房可刷 ≥1 只 |
| **P0.6** | **三选一最小闭环** | 词条官 + 界面 + 织局验收 | 清房（或 Debug 键）→ 暂停 → `WBP_AffixChoice` 出 3 张（读 `DT_Affix_MVP`）→ 选 1 → 写入 `AC_AffixInventory` → Modifier 生效（至少 RapidFire 射速可感知）→ 恢复控制 |
| **P0.7** | **第一战斗房** | 廊道 + 弹芯 | 1 个普通战斗房模板：刷 Grunt 编队 → 清房开门 → **必出** 一次三选一 → 不可回退 |
| **P0.8** | **Hub → Run_Test 开局** | Meta/流程 + 界面 | `Hub` 按「开始」进 `Run_Test`（或正式 Run 图）；带入 Meta 开局上下文；死亡/通关可回 Hub 结算（Core 可先占位） |

**禁止并行抢跑：** P0.6 不得阻塞在完整商店/永久树；P0.7 不得上精英/Boss；P0.8 不要求完整 2 层。

## 7. MVP 验收清单

- [ ] 能从 Hub 进入 `Run_Test` 并开局
- [ ] 打完 2 层 + 两层 Boss（或至少层2 Boss）并可胜利结算
- [ ] 三选一单局至少触发 **8** 次
- [ ] 3 枪（Peashooter / Scattergun / Rail）局内都能拿到并装备
- [ ] 死亡有失败结算，仅保留 Core
- [ ] 中位时长落在 **15–20 分钟**
- [ ] 武器 / 敌人 / 词条均由 `DA_` / `DT_` 驱动，蓝图只读表
- [ ] 不允许回退已清房；倒地即失败


### 7.1 近端里程碑验收（先于完整 MVP）

- [ ] **手感签收：** `CombatFeel.md` 本版已引用；Peashooter 走跳开火与受击 I-Frame 符合表内观感（弹芯自测 + 织局抽查）
- [ ] **P0.5 Grunt：** 至少 1 只 Grunt 可追击/射击、可被击杀；死亡有反馈；数据在 `DA_Enemy_Grunt`
- [x] **P0.6 三选一最小闭环：** 单次三选一全链路可跑通；选中 RapidFire 后射速明显加快；Unique 不会二次出现（可用 Debug 连抽验证）· 策划签收 2026-11-05（PR #29；Mac Play 明早补）
- [ ] **P0.7 第一战斗房：** 一间普通房清房 → 必出三选一 → 开门；无回退。步骤：进房 → 两波 → 三选一 → 右门开。
- [ ] **P0.8 Hub/Run_Test 开局：** Hub 一键开局进入可玩图；失败或主动退出能回 Hub（结算 UI 可极简）

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
| 事件 Op | `Content/Data/Events/DA_Event_*` + `UEventSubsystem::ResolveEventChoice` |

---

## 变更记录

| 日期 | 作者 | 说明 |
|------|------|------|
| 2026-11-04 | 织局 | 首版：局内循环、难度曲线、敌人/三枪、结算 Core |
| 2026-11-05 | 织局 | **签收** `CombatFeel.md` 本版；近端顺序锁定 **P0.5 Grunt → P0.6 三选一最小闭环 → P0.7 第一战斗房 → P0.8 Hub/Run_Test 开局**；补 §6.5 与 §7.1 验收 |
| 2026-11-05 | 织局 | §2.8 Event：`DA_Event_AbandonedCache` 挂 `Room_Event_01`；CarefulLoot/+25 Scrap · ForceLoot/−15HP+50 Scrap；跳过无奖 |
| 2026-11-05 | 词条官 | 事件 Op 运行时：`ResolveEventChoice` 按序执行四 Op；局内字段 `RunScrap`；DamageSelf 下限 1，不改 Core / 手感 |
| 2026-11-05 | 界面 | 事件壳确认选项调用一次 `ResolveEventChoice`。Skip / Esc 不调用。不 `SetGamePaused`。F6–F11 不动 |
| 2026-11-05 | 界面 | 商店两路钱包。F11 / `OpenShopDemo` 仍是 DemoScrap 128→98。`OpenShop` 读 `GetRunScrap`，买/刷成功 `AddRunScrap`。不改 F6–F11，不改 `AC_ShopPresenter` 开/关 |
