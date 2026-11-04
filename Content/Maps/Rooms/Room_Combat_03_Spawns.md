# Room_Combat_03 — 刷怪点坐标表（相对入口）

**坐标系：** 原点 = 左侧入口内侧地面 `(0, 0)`（1 设计 px = 1 uu）。PlayerStart 在 `(48, 0)`，不是原点。宽 **1920**；右出口约 `(1840, 0)`。  
**同时存活：** ≤6  
**波次：** W1 Ground → W2 Ground+Air →（可选）W3 Ground

## 刷怪点

| ID | Tag | WaveGroup | (X, Y) | Facing | Optional | 备注 |
|----|-----|-----------|--------|--------|----------|------|
| SP_G1 | Ground | 1 | (520, 0) | Left | false | W1 近 |
| SP_G2 | Ground | 1 | (720, 0) | Left | false | W1 中路 |
| SP_G3 | Ground | 2 | (1280, 0) | Left | false | W2 主台下 |
| SP_A1 | Air | 2 | (460, 72) | Left | false | W2 次台上 |
| SP_A2 | Air | 2 | (1180, 100) | Left | false | W2 主台上 |
| SP_G4 | Ground | 3 | (1560, 0) | Left | **true** | 可选 W3 |

## 布局锚点

| 锚点 | (X, Y) | 说明 |
|------|--------|------|
| PlayerStart | (48, 0) | 安全区 |
| Exit_Left | (-40, 0) | 入口 |
| Exit_Right | (1840, 0) | RequireClear |
| P_sec | (360–580, 28) | 次台 |
| P_step | (820–940, 28) | 踏板 |
| P_main | (980–1380, 56) | 主台 |

## 变更

| 日期 | 说明 |
|------|------|
| 2026-11-05 | 初版相对入口表 |
