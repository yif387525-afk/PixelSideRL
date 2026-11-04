# Room_Combat_05 — 刷怪点坐标表（相对入口）

**坐标系：** 原点 = 左侧入口内侧地面 `(0, 0)`（1 设计 px = 1 uu）。PlayerStart 在 `(48, 0)`。宽 **2240**；右出口约 `(2160, 0)`。  
**同时存活：** ≤6  
**波次：** W1 近段 → W2 中远夹击+Air → W3 收束

## 刷怪点

| ID | Tag | WaveGroup | (X, Y) | Facing | Optional | 备注 |
|----|-----|-----------|--------|--------|----------|------|
| SP_G1 | Ground | 1 | (560, 0) | Left | false | W1 近 |
| SP_G2 | Ground | 1 | (880, 0) | Left | false | W1 中近 |
| SP_G3 | Ground | 2 | (1280, 0) | Left | false | W2 中远夹击 |
| SP_G4 | Ground | 2 | (1680, 0) | Left | false | W2 远 |
| SP_G5 | Ground | 3 | (2000, 0) | Left | false | W3 收束 |
| SP_A1 | Air | 2 | (1120, 80) | Left | false | W2 中段上方 |

占位：`Enemy.Grunt.Early` / `Enemy.Flyer.Early`。

## 布局锚点

| 锚点 | (X, Y) | 说明 |
|------|--------|------|
| PlayerStart | (48, 0) | 安全区 |
| Exit_Left | (-40, 0) | 入口 |
| Exit_Right | (2160, 0) | RequireClear |
| P_near | (480–720, 28) | 近台 |
| P_far | (1520–1840, 28) | 远台 |
| AffixAnchor | (1120, 0) | 三选一 |

## 变更

| 日期 | 说明 |
|------|------|
| 2026-11-05 | 初版相对入口表 |
