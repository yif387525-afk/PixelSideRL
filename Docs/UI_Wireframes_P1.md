# P1 线框说明：WBP_Shop · WBP_RunSummary

**状态：** 可交互线框（占位像素），不接真实战斗 / Meta 数据  
**画布：** 640×360 · 最近邻 · 安全区 8px  
**临时色：** 与 [UI_Wireframes_P0.md](./UI_Wireframes_P0.md) 文末一致  
**落库：** `Content/Blueprints/UI/`（仓库就绪后）

对齐主管拍板：商店只花 **Scrap（仅局内）**；结算展示 **Core（仅局外永久）** + 构筑列表。  
> Meta 存档/API 已统一为 Core（见 `MetaSaveInterface.md`）；局内 Scrap 结余经结算折算进 Core。

---

## 1. WBP_Shop

**用途：** 商店房打开；**暂停时间**；购买或跳过 → 离开开门。  
**通货：** Scrap（唯一）。  
**货架：** MVP **4 格**（可扩到 6；布局按 2×2，扩成 2×3）。

C++ 壳不按上面的 2×2 做。`Room_Shop_01` 与 `DA_ShopInventory` 是 **3** 格：武器合约 / 回复 / 三选一券。Lab 键是 **F11**（`F10` 是事件壳）。见 `Content/Blueprints/UI/README.md`。

### 1.1 布局（640×360）

```
┌──────────────────────────────────────────────────────────────┐
│ ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓ 半透明遮罩 ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓ │
│                                                              │
│   商店                              Scrap 128                │  顶栏
│                                                              │
│      ┌──────────┐  ┌──────────┐                              │
│      │ 商品 A   │  │ 商品 B   │                              │  2×2
│      │ ICON     │  │ ICON     │                              │  每格 ~128×100
│      │ 名 / 价  │  │ 名 / 价  │                              │
│      └──────────┘  └──────────┘                              │
│      ┌──────────┐  ┌──────────┐                              │
│      │ 商品 C   │  │ 商品 D   │                              │
│      └──────────┘  └──────────┘                              │
│                                                              │
│   [刷新 25]              [离开]                              │  底栏
│   ← → 选格 · Enter 买 · Esc 离开                             │
└──────────────────────────────────────────────────────────────┘
```

| 区域 | 锚点 / 尺寸 | 说明 |
|------|-------------|------|
| `Txt_Title` | 左上 (16,12) | `"商店"` |
| `Txt_Scrap` | 右上 | `"Scrap 000"`，货币色 `#FFD54A` |
| `UG_Slots` | 居中偏上，约 (80,48) | UniformGrid 2×2；格间距 12 |
| `WBP_ShopSlot` ×4 | 每格 128×100 | 见下 |
| `Btn_Reroll` | 左下 (16,320) | `"刷新"` + 价格；买不起灰化 |
| `Btn_Leave` | 右下 | `"离开"`；Esc 同效 |
| `Txt_Hint` | 底中 | 操作提示 |

### 1.2 `WBP_ShopSlot` 控件

| 控件名 | 类型 | 尺寸 | 绑定（占位） |
|--------|------|------|--------------|
| `Border_Frame` | Border | 128×100 | 焦点亮边 / 买不起暗红边 |
| `Btn_Buy` | Button | 铺满 | 点击购买 |
| `Img_Icon` | Image | 24×24 | 商品图标占位 |
| `Txt_Name` | TextBlock | 一行 | 商品名 |
| `Txt_Kind` | TextBlock | 小字 | Affix / Heal / Voucher |
| `Txt_Price` | TextBlock | | `"25 Scrap"`；不足时红色 |
| `Img_SoldOut` | Image | 可选斜杠 | Sold 时显示 |

### 1.3 MVP 商品类型（展示层）

| Kind | 说明 | 价（Demo） | 购后 |
|------|------|------------|------|
| `Affix` | 直接获得指定词条；卡面字段同契约 `GetAffixCardView`（Name/Desc/Rarity/Icon） | 30 / 55 / 90 | 槽 Sold |
| `Heal` | 回复 HP（如 30%） | 20 | 可保留或 Sold（本期 Sold） |
| `Voucher` | 「随机三选一券」；购后关闭商店并打开 `WBP_AffixChoice` | 75 | Sold + 链式打开三选一 |

刷新：重roll 未售格；价格 Demo **25 Scrap**；Scrap 不足则按钮 Disabled。

### 1.4 占位货架 Demo

| Slot | Kind | Name | Price | Rarity |
|------|------|------|-------|--------|
| 0 | Affix | 豌豆连射 | 30 | Common |
| 1 | Heal | 急救凝胶 | 20 | — |
| 2 | Affix | 爆裂弹芯 | 55 | Rare |
| 3 | Voucher | 随机强化券 | 75 | — |

`DemoScrap` 初始 **128**（够买多项，便于点测）。

### 1.5 状态

| 状态 | 对象 | 表现 |
|------|------|------|
| `Closed` | 根 | Collapsed；恢复时间 |
| `Open` | 根 | Visible；暂停；焦点 Slot_0 |
| `Slot_Idle` | 槽 | 普通边框 |
| `Slot_Focused` | 槽 | 1px 白亮边 |
| `Slot_CantAfford` | 槽 | 价格红字；确认时短抖 2px（不扣款） |
| `Slot_Sold` | 槽 | 灰化 + Sold 标记；不可再选 |
| `Reroll_Disabled` | 刷新钮 | Scrap < 刷新价 或 全 Sold |
| `Buying` | 根 | 锁输入 ~0.1s → 扣 Scrap → 应用效果 |

### 1.6 输入

| 输入 | 行为 |
|------|------|
| ← → ↑ ↓ | 焦点在 2×2 格间移动 |
| Enter / 点击槽 | 尝试购买焦点格 |
| R（可选）/ 点刷新 | 付费刷新 |
| Esc / 点离开 | `OnShopClosed` → 开门离店（**允许不买**） |

### 1.7 对外事件（预留）

- `OnBuyItem(SlotIndex, ItemId)`  
- `OnReroll()`  
- `OnShopClosed()`（离开）  
- `OnRequestAffixChoiceFromVoucher()`（购券后由 Game 打开三选一；UI 不写战斗）

HUD：商店打开期间 HUD 可保留 Scrap 数字同步；本期 Demo 与 `Txt_Scrap` 双向占位即可。

---

## 2. WBP_RunSummary

**用途：** 胜利或失败后全屏结算；**一屏看懂**本局结果与 Core；确认 → Hub。  
**不负责：** Core 公式计算（只展示系统给的数）。

### 2.1 布局（640×360）

```
┌──────────────────────────────────────────────────────────────┐
│                                                              │
│              胜利 / 失败                                      │  大标题
│              12:34 · 击杀 86                                  │  副标
│                                                              │
│   ┌─ 构筑 ─────────────────────────────┐                     │
│   │ [ic]名  [ic]名  [ic]名  [ic]名 …   │  词条 wrap，最多两行 │
│   └────────────────────────────────────┘                     │
│                                                              │
│   层进度  2/2     Boss  1                                     │
│   Scrap 结余  40  → 折算已计入下方                            │
│                                                              │
│              获得 Core  +42                                   │  强调
│                                                              │
│                    [ 回据点 ]                                 │
└──────────────────────────────────────────────────────────────┘
```

| 控件 | 类型 | 说明 |
|------|------|------|
| `Txt_Outcome` | TextBlock | `"胜利"` 绿 / `"失败"` 红 |
| `Txt_Time` | TextBlock | `"12:34"` 本局时长 |
| `Txt_Kills` | TextBlock | `"击杀 86"` |
| `WB_Build` | WrapBox / HB | 已选词条图标+短名；空则 `"（无词条）"` |
| `Txt_Floor` | TextBlock | `"层进度 2/2"` |
| `Txt_BossKills` | TextBlock | `"Boss 1"` |
| `Txt_ScrapLeft` | TextBlock | `"Scrap 结余 40"`（失败局仍可显示本局曾持有/结余供透明） |
| `Txt_CoreGain` | TextBlock | `"获得 Core +42"`；字号最大 |
| `Txt_CoreTip` | TextBlock | 失败时小字：`"局内 Scrap / 词条 / 临时枪已清空"` |
| `Btn_Continue` | Button | `"回据点"`；Enter 同效 |
| `Img_Dim` | Image | 可选深底，无游戏穿透 |

### 2.2 占位 Demo 两套

**Win**

| 字段 | 值 |
|------|-----|
| Outcome | 胜利 |
| Time | 16:20 |
| Kills | 86 |
| Floor | 2/2 |
| BossKills | 2 |
| ScrapRemain | 40 |
| CoreGain | 42 |
| Build | 豌豆连射、爆裂弹芯、吸血涂层 |

**Fail**

| 字段 | 值 |
|------|-----|
| Outcome | 失败 |
| Time | 09:05 |
| Kills | 31 |
| Floor | 1/2 |
| BossKills | 0 |
| ScrapRemain | 12（仅展示） |
| CoreGain | 8 |
| Build | 豌豆连射 |
| Tip | 显示清空说明 |

`_Dev`：`Btn_DemoWin` / `Btn_DemoFail` 切换两套数据（仅线框预览）。

### 2.3 状态

| 状态 | 表现 |
|------|------|
| `Closed` | Collapsed |
| `Open_Win` | 绿标题；无失败 Tip |
| `Open_Fail` | 红标题；显示清空 Tip |
| `Ready` | `Btn_Continue` 可点（数据灌入后） |
| `Leaving` | 锁输入 → `OnContinueToHub` |

无「再看详情」翻页：构筑超两行则缩略名 + 悬停预留（MVP 可只截断 + `…`）。

### 2.4 输入

| 输入 | 行为 |
|------|------|
| Enter / Space / 点击回据点 | 确认去 Hub |
| Esc | 同确认（避免卡死在结算） |

### 2.5 对外事件 / 数据 struct（预留）

```
FRunSummaryView
  bVictory: bool
  DurationSec: int32
  KillCount: int32
  FloorReached: int32   // 1..2
  FloorMax: int32       // 2
  BossKills: int32
  ScrapRemain: int32
  CoreGained: int32
  AffixIds: []          // 只读展示
```

- `OnContinueToHub()` — UI 只发事件，不写存档。

### 2.6 「一屏看懂」检查

- [x] 胜负一眼可辨（色 + 文案）  
- [x] 时长、击杀、构筑同屏  
- [x] Core 获得量视觉最强  
- [x] 失败有清空说明，避免误解「东西还在」  

---

## 3. 与 P0 衔接

| 项 | 约定 |
|----|------|
| HUD 货币文案 | 由 `"币"` 改为 **`Scrap`**（与 GDD 对齐；改 P0 落地时一并替换） |
| 购券 | Shop → 关闭或压栈 → `WBP_AffixChoice` → 选完回房间流（不回商店除非设计改） |
| 暂停 | Shop / AffixChoice / RunSummary 均暂停或脱离局内；HUD 在 Summary 时 Hide |

---

## 4. 交付清单

| 项 | 状态 |
|----|------|
| `WBP_Shop` + `WBP_ShopSlot` 控件 / 状态 / 输入 | ✅ 线框。C++ 壳改为 3 格，读 `DA_Shop*`，不足 GreyOut |
| `WBP_RunSummary` 控件 / 胜负两态 / struct | ✅ |
| Demo 货架与 Win/Fail 占位数据 | ✅ 线框 4 格仍留本文。壳的货架以 #35 表为准 |
| `WBP_Shop` C++ 壳 | ✅ `UPixelShopWidget`。无 uasset。DemoScrap 本地扣；日后 `AddRunScrap(-Price)` |
| `WBP_RunSummary` C++ 壳 | ⏳ 未做 |
| 真 RunScrap 扣款 | ❌ 本壳不调用 `AddRunScrap` |
