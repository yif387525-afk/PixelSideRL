# _Dev 线框：WBP_DebugCombatHUD

**来源需求：** [Combat_P0_DebugHUD.md](./Combat_P0_DebugHUD.md)（弹芯）  
**约束：** 仅 `_Dev` / 非 Shipping / `bEnableCombatDebugHUD`；**不进**正式 `WBP_HUD`；不写战斗逻辑。  
**路径建议：** `Content/_Dev/Combat/UI/WBP_DebugCombatHUD`  
**落地：** C++ 父类 `UPixelDebugCombatHUD`（`Source/PixelSideRL/Public/UI/PixelDebugCombatHUD.h`）。只读门面 `UCombatDebugRead`（显示名 `BI_CombatDebugRead`）由 `UCombatDebugReadComponent` 转发，不另写战斗。蓝图配方 `Content/_Dev/Combat/UI/README.md`。仓库不提交 `WBP_*.uasset`；没有资产时父类自建下表同名控件。

---

## 布局（640×360，左上，安全区 8px）

```
HP  24 / 30   [████████░░]
IFR 0.42s
WPN Peashooter
FR  1.000
CD  0.05 / 0.120
DMG -8 @ 12.3s
```

锚点 (8, 40) 起（避开正式 HUD 顶栏若同屏预览）；等宽/像素字体；字色 `#F2F2F2`，底半透黑条可选。

## 控件与绑定槽

| 控件 | 类型 | 绑定（只读 `BI_CombatDebugRead`） |
|------|------|----------------------------------|
| `Txt_HP` | TextBlock | `Health` / `MaxHealth` → `"HP  24 / 30"` |
| `Bar_HP` | ProgressBar | `Health/MaxHealth` |
| `Txt_IFrame` | TextBlock | `IFrameRemaining` → `"IFR 0.42s"`；>0 时 2Hz 闪烁 |
| `Txt_Weapon` | TextBlock | `WeaponName` |
| `Txt_FireRate` | TextBlock | `GetFireRate()` → `GetStat(FireRate)`，显示 `FR  1.000`；RapidFire ≈ `1.120` |
| `Txt_CD` | TextBlock | `FireCooldownRemaining` / `GetResolvedFireCooldown()` |
| `Txt_LastDmg` | TextBlock | `LastDamageTaken` + `LastDamageTime`；0.5s 内 `#FF3B3B`，其后变暗 |
| `Txt_Alive`（可选） | TextBlock | `bAlive` 为 false 时显示 `DEAD` |

Tick：Timer 0.05s 或 Bind to 玩家；**禁止** `ApplyDamage` / `Fire`。

## 状态

| 状态 | 表现 |
|------|------|
| `Hidden` | Shipping 或开关关 → Collapse |
| `Visible` | 开发开关开 |
| `InIFrame` | IFR 行闪烁 |
| `RecentHit` | DMG 行醒目 ≤0.5s |

## 对接

1. 界面：按上表出 Widget + 绑定槽（本期线框完成）。  
2. 弹芯：实现 `BI_CombatDebugRead` 后挂玩家。  
3. 同关 `Lab_CombatP0` 对验收。

P0 **不做** `WBP_DebugDmgNumber`（非阻塞）。

## 落地（C++，无 uasset）

线框与上表仍然有效。运行时父类是 `UPixelDebugCombatHUD`，挂在 `APixelSideCharacter` 上，开关 `bEnableCombatDebugHUD`（Shipping 强制关）。

| 线框 | 代码 |
|------|------|
| `WBP_DebugCombatHUD` | 编辑器子蓝图，父类 `UPixelDebugCombatHUD`。路径 `Content/_Dev/Combat/UI/`。步骤见该目录 README |
| `BI_CombatDebugRead` | `UCombatDebugRead` + `UCombatDebugReadComponent`（`AC_CombatDebugRead`）。血量/武器走原组件；`GetFireRate` 走 `AC_StatModifiers::GetStat` |
| `Txt_*` / `Bar_HP` | `BindWidgetOptional`，名字与上表一致。没有设计师根节点时 C++ 按 (8, 40) 自建 |
| CD 分母 | `GetResolvedFireCooldown` = `DA.FireInterval / GetStat(FireRate)`。FireRate 为 1 时等于 DA 间隔。没有间隔时用 `1/FireRateBase` |
| 刷新 | 定时器 0.05s。不调用 `ApplyDamage` / `Fire`，不改 DA |

`Hidden`：Shipping 或 `bEnableCombatDebugHUD` 关闭 → Collapse。正式 `WBP_HUD` 不引用本 Widget。
