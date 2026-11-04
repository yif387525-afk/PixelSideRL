# Room_Boss_01 — 刷怪点坐标表（相对入口）

**坐标系：** 原点 = 左侧入口内侧地面 `(0, 0)`（1 设计 px = 1 uu）。PlayerStart 在 `(80, 0)`。宽 **2560**；结算出口约 `(2480, 0)`。  
**同时存活：** Boss 1 + 杂兵 ≤3  
**波次：** WaveGroup 0 = Boss 常驻；1 = P1 可选；2 = P2 可选

## 刷怪点

| ID | Tag | WaveGroup | (X, Y) | Facing | Optional | 备注 |
|----|-----|-----------|--------|--------|----------|------|
| SP_Boss | Boss | 0 | (1280, 0) | Left | false | 唯一 Boss 本体 |
| SP_A1 | Add | 1 | (640, 0) | Right | **true** | P1 左杂兵 |
| SP_A2 | Add | 1 | (1920, 0) | Left | **true** | P1 右杂兵 |
| SP_A3 | Add | 2 | (1280, 72) | Left | **true** | P2 空中杂兵 |

占位：`Enemy.Boss.MVP`；Add Ground=`Enemy.Grunt.Early`；Add Air=`Enemy.Flyer.Early`。

## 布局锚点

| 锚点 | (X, Y) | 说明 |
|------|--------|------|
| PlayerStart | (80, 0) | 自前厅进入 |
| Exit_FromAnte | (-40, 0) | Intro 后锁 |
| Exit_Settle | (2480, 0) | BossDefeated 解锁 |
| P_sideL | (400–640, 28) | 左侧台 |
| P_sideR | (1760–2080, 28) | 右侧台 |
| P_p2 | (1120–1520, 56) | P2 中央台 |
| Haz_A | (960, 0) | 转场危害 |
| Haz_B | (1680, 0) | 转场危害 |

## 变更

| 日期 | 说明 |
|------|------|
| 2026-11-05 | 初版 Boss + 可选杂兵表 |
