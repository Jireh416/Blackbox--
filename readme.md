# 15³ 情緒黑盒 (Emotional Black Box)

這是一個具備「習慣化抗性」的藝術互動硬體原型專案。透過 Arduino UNO Q 整合電容觸控、動態伺服馬達機構、RGB 光效、音效與實體白霧，賦予一個絕對平坦的 15x15x15cm 黑盒獨特且會疲乏的「情緒」。

## 專案結構
- `src/main/main.ino`: 核心狀態機與硬體控制程式碼。
- `docs/`: 包含硬體規格、接線指南與專案地圖。
- `source/`: 供使用者存放參考資料與 Datasheet。

## 依賴函式庫 (Dependencies)
在編譯 `main.ino` 之前，請確保已在 Arduino IDE 內安裝以下函式庫：
- `Adafruit MPR121` (電容觸控)
- `Adafruit PWM Servo Driver Library` (PCA9685 馬達驅動)
- `FastLED` (WS2812B 燈條驅動)
- `DFRobotDFPlayerMini` (MP3 播放器)
- `SoftwareSerial` (內建)

## 狀態機與互動邏輯
系統具備五個嚴謹階段的體驗流：
1. **Phase 0 (休眠)**: 閉合、微弱紅暗暈、深沉心跳。
2. **Phase 1 (甦醒)**: 1mm 縫隙、暗紅呼吸光、底噪。
3. **Phase 2 (躁動)**: 不規則起伏、橘紅漸快呼吸光、震動麻刺。
4. **Phase 3 (高潮)**: 滿開、深紅飽和脈動光、強烈共鳴震動、密集心跳。
5. **Phase 4 (匱乏)**: 降下閉合、冷藍光、實體白霧噴發、餘震。

## 編譯與燒錄
1. 連接 Arduino UNO Q。
2. 確保 Arduino IDE 安裝對應的核心 (Board Core)。
3. 編譯 `src/main/main.ino` 並上傳。
