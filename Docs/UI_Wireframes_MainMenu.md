# 线框说明：WBP_MainMenu

**状态：** 可交互线框，不接存档 / 设置真逻辑  
**画布：** 640×360 · 最近邻 · 安全区 8px  
**挂载：** `Maps/Entry.umap`  
**排期：** 主管指定本项；`WBP_RunMap` 等廊道拼局可视化需求更清晰再做  

---

## 1. 布局

```
┌──────────────────────────────────────────────────────────────┐
│                                                              │
│                     PixelSideRL                              │  标题
│                  像素横版射击 Roguelike                        │  副标（可关）
│                                                              │
│                   [ 开始游戏 ]                               │  → Hub 或直进 Run（流程定）
│                   [ 设置     ]                               │  → 占位面板
│                   [ 退出     ]                               │
│                                                              │
│              v0.1 MVP · 开发中                               │  角标
└──────────────────────────────────────────────────────────────┘
```

| 控件 | 类型 | 说明 |
|------|------|------|
| `Txt_GameTitle` | TextBlock | `"PixelSideRL"` |
| `Txt_Subtitle` | TextBlock | 可选短标语 |
| `Btn_Start` | Button | `"开始游戏"` |
| `Btn_Settings` | Button | `"设置"` |
| `Btn_Quit` | Button | `"退出"` |
| `Txt_Version` | TextBlock | `"v0.1 MVP"` |
| `Panel_Settings` | Overlay/Border | **占位**：默认 Hidden |
| `Btn_SettingsClose` | Button | 关设置面板 |
| `Txt_SettingsStub` | TextBlock | `"设置占位：音量 / 键位 稍后"` |
| `Slider_MasterVol`（占位） | Slider | Disabled 或仅本地预览，不写 ini |
| `Chk_Fullscreen`（占位） | CheckBox | 同上 |

焦点默认 `Btn_Start`；↑↓ / Tab 循环三主按钮。

---

## 2. 流程约定（UI 只发事件）

| 按钮 | 事件 | 建议流向（流程侧） |
|------|------|-------------------|
| 开始游戏 | `OnStartGame` | Entry → **Hub**（永久成长后再开局）；若无 Hub 关卡则临时 → `Run_Test` |
| 设置 | `OnOpenSettings` | 显示 `Panel_Settings`；不暂停世界（菜单关） |
| 退出 | `OnQuitGame` | `QuitGame`；编辑器则仅 Print |

UI **不**直接 OpenLevel；由 GameInstance / Entry PC 听事件。

---

## 3. 状态

| 状态 | 表现 |
|------|------|
| `Main` | 三按钮可见；设置面板 Hidden |
| `SettingsOpen` | 设置面板 Visible；主按钮可 SelfHitTestInvisible 或仍可点关闭 |
| `Starting` | Start 短锁，防双开 |
| `Hidden` | 离开 Entry 后 Collapse |

---

## 4. 输入

| 输入 | 行为 |
|------|------|
| ↑ ↓ | 主菜单按钮焦点 |
| Enter / 点击 | 激活焦点 |
| Esc | SettingsOpen → 关面板；Main → 忽略（或同退出需二次确认：MVP **忽略**） |

---

## 5. 与 Hub 衔接

- 正常路径：`MainMenu.OnStartGame` → 加载 `Hub` → `WBP_Hub`。  
- Hub「返回标题」→ 回 Entry → 再显本 Widget。  
- 不在主菜单花 **Core/Scrap**（无商店）。

---

## 6. 交付清单

| 项 | 状态 |
|----|------|
| 控件 / 状态 / 输入 | ✅ |
| 设置占位面板 | ✅（无持久化） |
| UE 蓝图壳 | ⏳ 仓库就绪后 |
| RunMap | ⏸ 等廊道 |
