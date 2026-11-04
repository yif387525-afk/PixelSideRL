# UI 规范与 Widget 清单（MVP）

**索引：** [UI_Index.md](./UI_Index.md)（全部线框路径与依赖字段）  
**范围：** HUD、三选一、事件选择、商店、结算、据点。不写战斗逻辑。  
**命名：** Widget `WBP_`；路径 `Content/Blueprints/UI/`，贴图 `Content/Art/UI/`。  
**分辨率：** 逻辑 640×360，最近邻放大；UI 对齐整数像素。  

**线框详稿：**  
- P0：[UI_Wireframes_P0.md](./UI_Wireframes_P0.md) — HUD · AffixChoice  
- Event：[UI_Wireframes_Event.md](./UI_Wireframes_Event.md) — EventChoice  
- P1：[UI_Wireframes_P1.md](./UI_Wireframes_P1.md) — Shop · RunSummary  
- Hub：[UI_Wireframes_Hub.md](./UI_Wireframes_Hub.md) — Hub · Hub_Upgrade  
- MainMenu：[UI_Wireframes_MainMenu.md](./UI_Wireframes_MainMenu.md) — Entry  
- Debug：[UI_Wireframes_DebugCombatHUD.md](./UI_Wireframes_DebugCombatHUD.md) — `_Dev` 专用  

## 货币（主管拍板 2026-11-04）

| 名称 | 范围 | 用途 |
|------|------|------|
| **Scrap** | **仅局内** | 掉落/房清；商店唯一通货；结算时结余可折算进 Core |
| **Core** | **仅局外永久** | 结算发放；Hub 买永久节点；存档字段与 Meta API 一律用 Core（旧称 Scrap 已废） |

UI 文案禁止把局外货币写成 Scrap。

## 对接边界

| 界面 | 读数据来源 | 写回/事件 | 不负责 |
|------|------------|-----------|--------|
| HUD | 生命/弹药/**Scrap**/武器/词条图标 | 无玩法写入 | 受击判定、弹道 |
| 三选一 | 词条 DA（经 `GetAffixCardView`） | `ConfirmAffixChoice` | 词条数值公式 |
| 事件 | `DA_Event_AbandonedCache` 展示字段（Lab 硬编码同文） | `OnEventChoiceSelected` / `OnEventSkipped` / `OnEventClosed` | Scrap、伤害、治疗、三选一 |
| 商店 | 商品 + 价格 + Scrap | 购买/刷新/离开 | 掉落生成 |
| 结算 | 统计 + 构筑 + **Core** | 确认 → Hub | Core 公式 / 存档 |
| 据点 | 永久节点、**Core** | `Purchase(NodeId)` → Meta | 存档序列化 |

## MVP Widget 清单

| Widget | 用途 | 优先级 | 状态 |
|--------|------|--------|------|
| `WBP_HUD` | 血量、武器、弹药、Scrap、词条一排 | P0 | ✅ C++ 父类 + 配方，无 uasset。战斗数仍是 Demo 设置器 |
| `WBP_AffixChoice` / `WBP_AffixCard` | 三选一 | P0 | ✅ C++ 父类 + 配方，无 uasset。关掉 native 文本占位 |
| `WBP_EventChoice` | 事件选项 + 跳过 | P0 | ✅ C++ 父类 + 配方，无 uasset。确认选项调用 `ResolveEventChoice`，Skip 不调用，不 `SetGamePaused`。Lab `F10` |
| `WBP_Shop` / `WBP_ShopSlot` | 货架、刷新、离开 | P1 | ✅ C++ 父类 + 配方，3 格读 `DA_Shop*`，无 uasset。扣款仍是 DemoScrap。Lab `F11` |
| `WBP_RunSummary` | 胜负、时长、击杀、构筑、Core | P1 | ✅ C++ 父类 + 配方，无 uasset。只展示传入的 `CoreGained`。不写 Core、不算公式、不写存档。无 Lab 键 |
| `WBP_Hub` / `WBP_Hub_Upgrade` | 据点与永久成长 | P1 | ✅ 线框 |
| `WBP_MainMenu` | Entry 开始/设置占位/退出 | P1 | ✅ 线框 |
| `WBP_RunMap` | 节点类型预览 | 后置 | ⏸ 等廊道 |
| `WBP_Pause` | 暂停 | P2 | 待做 |
| `WBP_DebugCombatHUD` | Combat P0 验收只读 | _Dev | ✅ C++ 父类 + 配方，不进正式包 |

## 交互约定

1. 三选一 / 商店 / 结算打开时暂停或脱离局内。事件选择只停移动和开火，不 `SetGamePaused`，门不锁。结算壳没有子系统暂停 API，`Present` / `OpenWin` / `OpenFail` 默认 `SetGamePaused`（与商店 `OpenShopDemo` 相同）；世界已经暂停时不抢所有权，确认后也不解开。本壳不改 `WBP_HUD`、不 Hide HUD。深底盖住画面。流程以后再藏 HUD。  
2. 商店可跳过；三选一不可 Esc 跳过（GDD）。事件可以跳过（SkipHint，无奖励）。  
3. 选中 1px 亮边；稀有度临时色见 P0。  
4. 安全区 8px；整数像素对齐。  
5. 线框阶段：占位数据 + 高对比临时色；不接真战斗。  
6. `_Dev` 调试 HUD 不进正式局内 HUD。

## 词条系统对接（冻结）

- 卡面四字段：`Name` / `Desc` / `Rarity` / `Icon` ← `GetAffixCardView`  
- 三选一：听 `OnAffixChoiceOffered`，确认 `ConfirmAffixChoice`  
- HUD 图标：`GetOwnedAffixes`（Icon + AffixId）  
- 详：`AffixSubsystem_API.md` §0 · §8  

## 主管确认

- [x] 仓库未建：先线框，再合入 Content  
- [x] 顺序：HUD → AffixChoice → Shop → RunSummary → Hub  
- [x] P0 / P1 验收通过  
- [x] 货币：Scrap=局内，Core=局外  
- [x] Hub 线框
- [x] MainMenu 线框（设置占位）
- [ ] RunMap（等廊道）
- [x] UI_Index 总索引（仓库前待命）
