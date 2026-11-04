# Combat P0 — 调试 HUD 需求（给界面）

**优先级:** 可选，但利于无美术阶段验收枪感  
**消费方:** 弹芯（Combat P0 验收）  
**实现方:** 界面（`WBP_`）  
**约束:** 不写战斗逻辑；只读 `BI_Damageable` / 调试接口；仅进 `Content/_Dev/` 或开发开关，**不进正式局内 HUD**

---

## 1. 目标

在 `Lab_CombatP0` / `_Dev` 沙盒关一眼看出：血量、无敌帧、开火冷却、当前武器、最近一次伤害，方便验收 `Combat_P0_Checklist` §1。

---

## 2. Widget

| 名称 | 路径 |
|------|------|
| `WBP_DebugCombatHUD` | `Content/Blueprints/UI/` 或 `Content/_Dev/Combat/UI/`（推荐后者） |

挂载：由 `BP_PlayerSideScroller`（或 Dev PlayerController）在 **非 Shipping** / `bEnableCombatDebugHUD` 为真时 Create。正式 `WBP_` 局内 HUD 不引用本 Widget。

---

## 3. 只读数据（界面拉取，Combat 提供）

弹芯在玩家侧暴露 **只读** 函数或接口 `BI_CombatDebugRead`（可放 `_Dev`）：

| 字段 | 类型 | 来源 |
|------|------|------|
| `Health` / `MaxHealth` | float | `AC_Damageable` |
| `bAlive` | bool | 同上 |
| `IFrameRemaining` | float 秒 | 同上 |
| `TeamId` | enum | 同上 |
| `WeaponName` | Name/String | `AC_WeaponController` ← DA |
| `FireCooldownRemaining` | float 秒 | 同上 |
| `FireInterval` | float | DA 绝对间隔；HUD 显示冷却可用 `FireInterval/GetStat(FireRate)` |
| `LastDamageTaken` | float | 最近一次 `ApplyDamage` 的 Damage |
| `LastDamageTime` | float（游戏时间） | 用于「刚受伤」高亮 |

界面 **禁止** 调用 `ApplyDamage` / `Fire`；禁止改 DA。

---

## 4. 布局建议（像素风、可读优先）

左上角固定，字号按 640×360 逻辑分辨率可读：

```
HP  24 / 30   [████████░░]
IFR 0.42s     (闪烁提示)
WPN Peashooter
CD  0.05 / 0.14
DMG -8 @ 12.3s
```

- 血条可用简单 ProgressBar 或字符块。  
- `IFrameRemaining > 0` 时整行或角色名闪烁。  
- `LastDamageTaken` 在 0.5 s 内用醒目色，之后变暗。

---

## 5. 验收配合

| Combat 验收项 | HUD 需可见 |
|---------------|------------|
| 木桩挨打扣血 | 若选中/显示木桩：其 HP；或玩家打桩时 World 飘字由 Combat 做，HUD 可不显示木桩 |
| 玩家受击 I-Frame | `IFR` 倒计时 |
| 开火冷却 | `CD` 与 `FireInterval`（实际冷却 = FireInterval/FireRateMult） |
| 武器切换（P0 仅豌豆） | `WPN` 名称正确 |

P0 **不要求** 伤害飘字 UI；若界面有余力可另做 `WBP_DebugDmgNumber`，非阻塞。

---

## 6. 对接节奏

1. 界面按本表出 `WBP_DebugCombatHUD` 线框 + 绑定槽。  
2. 弹芯仓库就绪后实现 `BI_CombatDebugRead` 并挂上。  
3. 两边在 `Lab_CombatP0` 对一下即可勾 P0-10/11。

有字段冲突以 `CombatFeel.md` / `Combat_P0_Checklist.md` 为准，抄送小yan。

---

## 7. 仓库落地

只读门面与调试 Widget 已进 C++，**没有**提交 `WBP_*.uasset`。

| 项 | 位置 |
|----|------|
| `BI_CombatDebugRead` | `UCombatDebugRead`，`UCombatDebugReadComponent`（`AC_CombatDebugRead`）转发玩家上的 `UDamageableComponent` / `UWeaponControllerComponent` |
| HUD 父类 | `UPixelDebugCombatHUD`。非 Shipping 且 `bEnableCombatDebugHUD` 时由 `APixelSideCharacter` 创建。类空则自建线框控件 |
| 编辑器子蓝图 | `Content/_Dev/Combat/UI/README.md`：父类、路径、控件名、挂到 `Lab_CombatP0` |
| 正式 HUD | 不引用本 Widget。Shipping 不创建 |

CD 行分母用 `GetResolvedFireCooldown`（`FireInterval / GetStat(FireRate)`）。FireRate 默认为 1 时与 DA `FireInterval` 相同；没有间隔时用 `1/FireRateBase`。界面仍然禁止 `ApplyDamage` / `Fire` / 改 DA。

---

## 变更记录

| 日期 | 作者 | 说明 |
|------|------|------|
| 2026-11-04 | 弹芯 | 首版：调试 HUD 只读字段与布局 |
| 2026-11-04 | 界面 | C++ 父类 `UPixelDebugCombatHUD` + 只读 `BI_CombatDebugRead`；WBP 仅配方，无 uasset |
