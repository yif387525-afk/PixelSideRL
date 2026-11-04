# 像素横版射击 Roguelike — Unreal 工程规范

**引擎:** Unreal Engine 5.4+（锁定小版本后全员统一）  
**类型:** 侧视横版射击 + 局内构筑 Roguelike + 少量永久成长  
**渲染:** 像素风（建议逻辑分辨率 640×360，最近邻放大；正交相机）

## 仓库根目录

```
PixelSideRL/
├── README.md
├── Docs/
│   ├── GDD.md                 # 玩法总纲
│   ├── CombatFeel.md          # 枪感参数表
│   ├── RoguelikeMeta.md       # 词条 / 永久成长
│   ├── RoomPipeline.md        # 房间模板与拼局规则
│   └── ArtBible.md            # 像素规范（色板、像素密度、动画帧率）
├── Config/                    # 版本锁定、.editorconfig、.gitattributes
├── Content/                   # 仅通过 UE / 规范路径提交
│   ├── _Dev/                  # 个人沙盒，不进正式包
│   ├── Art/
│   │   ├── Characters/
│   │   ├── Enemies/
│   │   ├── Weapons/
│   │   ├── FX/
│   │   ├── Environments/
│   │   ├── UI/
│   │   └── Palettes/
│   ├── Audio/
│   │   ├── SFX/
│   │   └── Music/
│   ├── Blueprints/
│   │   ├── Characters/
│   │   ├── Combat/
│   │   ├── AI/
│   │   ├── Roguelike/         # 词条、三选一、商店
│   │   ├── Rooms/             # 房间模板、出口、刷怪点
│   │   ├── Meta/              # 永久成长、存档接口
│   │   └── UI/
│   ├── Data/                  # DataAsset / DataTable
│   │   ├── Weapons/
│   │   ├── Affixes/
│   │   ├── Enemies/
│   │   └── Rooms/
│   ├── Input/
│   ├── Maps/
│   │   ├── Entry.umap         # 启动 / 主菜单
│   │   ├── Hub.umap           # 局外据点（永久成长）
│   │   ├── Run_Test.umap      # 拼局测试
│   │   └── Rooms/             # 单房间模板关
│   └── Core/                  # GameInstance、子系统、通用库
├── Source/                    # C++ 模块（手感 / 网络预留 / 工具）
│   └── PixelSideRL/
├── Scripts/                   # 导出、校验、CI
└── Builds/                    # 本地包，默认 gitignore
```

## 命名约定

| 类型 | 前缀/规则 | 例 |
|------|-----------|-----|
| Blueprint | `BP_` | `BP_PlayerSideScroller` |
| Actor Component | `AC_` | `AC_WeaponController` |
| Interface | `BI_` | `BI_Damageable` |
| Enum | `E_` | `E_AffixRarity` |
| Struct | `F` / `S_` | `FAffixDef` |
| DataAsset | `DA_` | `DA_Weapon_Peashooter` |
| DataTable | `DT_` | `DT_EnemyStats` |
| Widget | `WBP_` | `WBP_AffixChoice` |
| Input Action | `IA_` | `IA_Fire` |
| 像素贴图 | `T_` + 用途 | `T_Player_Idle` |
| 材质/MI | `M_` / `MI_` | `MI_PixelSprite_Unlit` |

- 资源名用 **PascalCase**，文件夹用 **PascalCase**。  
- 禁止在 `Content/` 根下散落资源；禁止把临时资产留在正式目录。  
- 词条、武器、敌人 **以 DataAsset 为唯一真相源**，蓝图只读表。

## 像素与相机（强制）

1. 目标像素密度：角色约 **32–48 px 高**（以 ArtBible 为准）。  
2. 贴图过滤：**Nearest**；关闭不必要的抗锯齿对精灵的模糊。  
3. 相机：正交，固定高度；滚动仅水平（可少量垂直跟随）。  
4. 碰撞与显示分离时可允许 1–2 px 手感偏移，但需记在 CombatFeel.md。

## 模块职责边界

- **Combat：** 移动、瞄准（侧视）、开火、弹道/命中、受击、击杀反馈。  
- **Roguelike：** 房间图生成、三选一、商店、局内 Buff 管线。  
- **Meta：** 局外货币、永久节点、解锁，不直接改局内数值公式以外的表现。  
- **Rooms：** 模板 Actor、刷怪点、门/出口、精英标记。  
- **UI：** HUD、选择卡、结算、据点；不写战斗逻辑。

## Git / 协作

- 大文件走 Git LFS（`uasset`/`umap`/`png`/`wav`）。  
- 同一张地图同时只允许一人锁定编辑；优先小房间模板而非巨型关卡。  
- PR 最小单元：一个可玩改动 + 对应 Docs 一行更新。  
- 每日可运行目标：`Run_Test` 能开一局并结算。

## MVP 范围（提醒）

1 角色 · 3 枪 · 8–12 词条 · 3–5 敌 + 1 Boss · 8–12 房间模板 · 15–20 分钟一局 · 少量永久成长。
