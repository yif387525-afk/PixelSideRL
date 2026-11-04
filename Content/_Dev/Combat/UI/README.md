# Content/_Dev/Combat/UI — WBP_DebugCombatHUD

Lab / `_Dev` 只读调试 HUD。**不要**提交 `WBP_*.uasset` 或任何假的二进制 Widget。正式 `WBP_HUD` **不要**引用这里的控件。

没有蓝图资产时也能显示：非 Shipping 且 `APixelSideCharacter.bEnableCombatDebugHUD` 为真（默认开）时，玩家会挂上 C++ 父类，并在左上角自己建出下表同名控件。Shipping 强制关掉，不会进正式包逻辑。

| 项 | 值 |
|----|----|
| 资产名（以后在编辑器里建） | `WBP_DebugCombatHUD` |
| 路径 | `Content/_Dev/Combat/UI/WBP_DebugCombatHUD` |
| C++ 父类 | `UPixelDebugCombatHUD`（类选择器显示名 `WBP_DebugCombatHUD`） |
| 头文件 | `Source/PixelSideRL/Public/UI/PixelDebugCombatHUD.h` |
| 只读门面 | `BI_CombatDebugRead` → `UCombatDebugRead`，由 `UCombatDebugReadComponent`（显示名 `AC_CombatDebugRead`）转发 |
| 画布 | 逻辑 640×360，锚在左上 **(8, 40)**，安全区 8px |
| 刷新 | 0.05s 定时器，只读 |

## 编辑器里做子蓝图（有 `.uproject` 之后，可选）

C++ 父类已经能在 `Lab_CombatP0` 显示。只有要换像素字体或手摆位置时才建 WBP。

1. 编译 `PixelSideRL` 模块（已依赖 UMG / Slate / SlateCore）。
2. 内容浏览器进入 `Content/_Dev/Combat/UI/`（没有就按这个路径建文件夹）。
3. 右键 → User Interface → Widget Blueprint。
4. 命名 **`WBP_DebugCombatHUD`**。父类选 **`PixelDebugCombatHUD`**（显示名 `WBP_DebugCombatHUD`）。不要选 `User Widget`。
5. 根用 Canvas Panel。调试块放左上，槽位 Position **X=8, Y=40**，锚点左上，不要拉成全屏（全屏会挡住手感）。字色 `#F2F2F2`。等宽或像素字体即可。
6. 按下表 **Is Variable**，名字必须逐字一致，这样 `BindWidgetOptional` 才能接到 C++。名字不对时 C++ **不会**拆掉你的根控件，只打一条日志。
7. 图表留空。不要调用 `ApplyDamage`、`TryFire` / `Fire`，不要 `SetWeaponData`，不要改 DA。
8. 挂到 Lab：在 `BP_PlayerSideScroller`（父类 `PixelSideCharacter`）的 **PixelSide \| Debug** 里，`bEnableCombatDebugHUD` 保持勾选，`CombatDebugHUDClass` 指到这个 WBP。留空则继续用 C++ 自建布局。
9. 关卡 `Lab_CombatP0`（或任意由 `APixelSideGameMode` 生成的沙盒）用 Dev 玩家 Play。正式局内 HUD 不要 Add 这个 Widget。
10. 若改由 Dev PlayerController 自己挂：先把角色上的 `bEnableCombatDebugHUD` 关掉，再调用 `UPixelDebugCombatHUD::MountForPlayer(PC, Pawn, WBP)`。避免叠两个。

## 控件（与线框同名）

| 控件名 | 类型 | 显示 | 只读来源 |
|--------|------|------|----------|
| `Txt_HP` | Text Block | `HP  24 / 30` | `GetCurrentHealth` / `GetMaxHealth`（`BI_CombatDebugRead.GetHealth`） |
| `Bar_HP` | Progress Bar | 血量比 | `Health / MaxHealth` |
| `Txt_IFrame` | Text Block | `IFR 0.42s` | `GetIFrameRemaining`。大于 0 时约 2Hz 闪 |
| `Txt_Weapon` | Text Block | `WPN Peashooter` | `GetWeaponId`（`GetWeaponName`） |
| `Txt_CD` | Text Block | `CD  0.05 / 0.12` | 剩余 `GetFireCooldownRemaining` / 分母 `GetResolvedFireCooldown` |
| `Txt_LastDmg` | Text Block | `DMG -8 @ 12.3s` | `GetLastDamageTaken` + `GetLastDamageWorldTime`。0.5s 内 `#FF3B3B`，其后变暗 |
| `Txt_Alive` | Text Block，可选 | `DEAD` | `IsAlive` 为 false 时显示，活着则折叠 |

CD 分母是实际冷却长度：`DA.FireInterval / GetStat(FireRate)`（`UWeaponControllerComponent::GetResolvedFireCooldown`）。P0 的 FireRate 默认为 1，所以分母等于 DA 的 `FireInterval`（豌豆默认 0.12s，线框里的 0.14 只是示意图）。没有武器间隔时分母用 `1/FireRateBase`。

`TeamId` 在 `BI_CombatDebugRead.GetTeamId` 上，线框没有单独一行，HUD 不画它。

## 禁止

- 调用 `ApplyDamage`、`TryFire`、`SetWantsToFire`、`SetWeaponData`、`SetMaxHealth`
- 改 `UDA_Weapon` 上的任何字段
- 把本 Widget 嵌进 `WBP_HUD` 或正式关卡
- 提交 `.uasset`

数据从 `APixelSideCharacter::GetDamageable()` / `GetWeaponController()` 读。没有这两个访问器时，再 `FindComponentByClass`。`AC_CombatDebugRead` 不保存第二套血量或冷却。
