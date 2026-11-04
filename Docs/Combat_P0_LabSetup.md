# Combat P0 本地沙盒（UE 5.8.4）

C++ 已能在空关里跑：左右走、Coyote/Jump Buffer 跳、急停、豌豆枪、木桩受击、玩家 I-Frame。  
本机工程：`/Users/apple/Documents/Unreal Projects/~Dev/PixelSideRL`  
**Jump 贴图是 5 帧，不是旧的 4 帧。** 用 `Content/Art/Characters/T_Player_Jump.png`（160×40 = 5×32×40）。不要用 `Content/_Dev/Player_v1_archive/T_Player_Jump.png`。

## 1. 拉代码并编译

```bash
cd "/Users/apple/Documents/Unreal Projects/~Dev/PixelSideRL"
git fetch origin
git checkout cursor/combat-p0-playable-beb4   # 合入 main 之后改为 pull main
```

用 **Unreal Editor 5.8.4** 打开该目录下的 `PixelSideRL.uproject`。若提示模块缺失，选编译。需要插件 **Enhanced Input**、**Paper2D**（`.uproject` 已打开）。

第一次 Play 用引擎自带的 `/Engine/Maps/Entry`。`APixelSideGameMode` 会在没有木桩的关卡里铺一条灰盒平台、一个木桩、一座调试炮塔。

| 键 | 作用 |
|----|------|
| A / D，左摇杆 X | 左右 |
| Space，手柄下键 | 跳（Coyote 0.08s，Jump Buffer 0.10s；松跳且上升时竖直速度 ×0.35，短按更矮） |
| 鼠标左键，右扳机 | 豌豆枪连发，朝向 ±X |
| H | 调试受击（非 Shipping） |

验收：急停更干脆；短按跳更矮、按住跳更高；空中可改线；弹是暖黄色小球，撞两端墙消失；打木桩掉血、闪白、击退更明显；炮塔或 H 打到玩家会闪白、击退，并在 0.6s 内闪烁且不再叠伤。

灰盒只在关卡里还没有 `APixelDamageDummy`（或其蓝图子类）时生成。自己摆好木桩后就不会再铺。关掉它：GameMode 上取消 `bSpawnLabSandbox`。

## 2. 做 `Lab_CombatP0`（可选，但建议）

1. 内容浏览器建 `Content/_Dev/Combat/`。
2. 新建空关卡，另存为 `Content/_Dev/Combat/Lab_CombatP0`。
3. World Settings → GameMode Override = `PixelSideGameMode`。不放木桩时，Play 仍会生成灰盒；要自己搭平台就把 `bSpawnLabSandbox` 关掉，并放好地板、`PixelDamageDummy`、`PixelDebugHurtTurret`、Player Start。
4. Project Settings → Maps：Editor Startup / Game Default 指到 `Lab_CombatP0`。
5. 窗口尽量 16:9。相机是正交 640×360，只跟角色 X，高度固定。

## 3. Jump Flipbook：**5 帧**

帧尺寸 **32×40**，脚底中心为 pivot。导入 Filtering = **Nearest**，不要 Mip。

| 文件 | 像素 | 帧数 | 含义 |
|------|------|------|------|
| `T_Player_Idle.png` | 128×40 | **4** | Idle |
| `T_Player_Run.png` | 192×40 | **6** | Run |
| `T_Player_Jump.png` | 160×40 | **5** | 0–1 rise，2 apex，3–4 fall |
| `T_Player_Fire.png` | 96×40 | **3** | 开火 |
| `T_Player_Hurt.png` | 64×40 | **2** | 受击 |

Jump 的做法：

1. 导入 `Content/Art/Characters/T_Player_Jump.png`。
2. 右键 → Sprite Actions → Extract Sprites，或新建 `Paper Flipbook`。
3. 网格 **32×40**，**5** 格，间距 0。源图宽度是 160，不是 128。
4. 每格 pivot = **Bottom Center**。
5. Flipbook 帧率 **12** fps。命名 `PF_Player_Jump`。
6. 角色会按竖直速度刮这 5 帧，不会把 Jump 当成 4 帧循环去播。帧数不是 5 时，输出日志会警告。

Idle / Run / Fire / Hurt 用同一套网格规则，帧数按上表。

蓝图（名字与清单一致，父类用 C++）：

1. `Content/Blueprints/Characters/BP_PlayerSideScroller`，父类 `PixelSideCharacter`。
2. 在 Details 里把五个 Flipbook 赋上。**Jump Flipbook 必须是上面的 5 帧。**
3. 若角色朝向反了，把 `Art Facing Multiplier` 改成 `-1`。精灵组件相对 Yaw 已是 90°，面朝 +Y 相机。
4. World Settings 或 GameMode 的 Default Pawn 改成这个 BP（不改则直接用 C++ 占位方块，手感一样）。

可选数据资产：`Content/Data/Weapons/DA_Weapon_Peashooter`，父类 `DA_Weapon_Peashooter`。新建出来就是 CombatFeel §4.1。不建也行，武器组件会用这份 C++ 默认值。射速乘区默认 1，冷却 = `FireInterval / GetStat(FireRate)`。

`BP_DamageDummy` 父类 `PixelDamageDummy`。`BP_Projectile_Pea` 父类 `ProjectilePea`。不必为了先跑通而建。

## 4. 数值从哪来

| 内容 | 位置 |
|------|------|
| 走、跳、Coyote、Buffer、重力、Jump Cut | `UPixelSideCharacterMovement` 与 `HandleJumpCompleted`，默认 CombatFeel §2 |
| 豌豆枪 | `UDA_Weapon` / `UDA_Weapon_Peashooter`，§4.1 |
| 玩家受击 | `UDamageableComponent` 且 `bUsePlayerHurtProfile`，§6 |
| 常量只写在 | `Source/PixelSideRL/Public/Combat/PixelSideCombatMath.h` |

战斗只读 `AC_StatModifiers.GetStat`。没有该组件时开火回退到 FireRate = 1 和武器 DA 的伤害/击退。不要在战斗蓝图里 Roll 词条。

击退距离用表里的 uu（70 / 145），0.11s 内 ease-out 走完。空中保留 60%。这样不会被地面急停摩擦（4000）收成不到 1 像素。

## 5. 本机验收勾选

- [ ] A/D 能走，松键能急停，跳有离台容错和提前按跳
- [ ] 按住左键连续出黄色豌豆，打到墙消失
- [ ] 木桩扣血、闪白、被轻推
- [ ] H 或炮塔打中玩家：闪白、击退、闪烁，无敌期间不连扣
- [ ] 若挂了 Jump Flipbook，Output Log 没有 “expects 5” 警告，滞空能看到 rise / apex / fall
