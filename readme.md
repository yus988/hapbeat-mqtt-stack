# 簡易設定ガイド

## 1) Wi‑Fi 設定（ローカルブローカーを含む全体共通）

- ルート直下の `config.h` を開き、以下を編集します。無い場合は `config.h.template` ファイルを元に作成して下さい。
  - `#define WIFI_SSID "YOUR_SSID"`
  - `#define WIFI_PASSWORD "YOUR_PASSWORD"`
- ローカル MQTT ブローカーは AtomS3 上で動作します。ブローカーの IP は
  ゲートウェイとサブネットに基づき `BROKER_STATIC_HOST_OCTET` を用いて自動算出されます。
  - 推奨値の目安（OCTET は第 4 オクテット）:
    - 一般家庭の /24: 200〜250（DHCP 範囲外の未使用域）
    - 機器多めの環境: 240〜250
    - テザリング等 /28: 10 または 12（有効範囲は通常 2〜14）
  - 衝突時はブローカー画面に警告が表示されるため、別の値に変更してください。

## 2) 色の閾値を変更する（オーバーライド方式）

- ルート直下に `sensor_color_params.h` を作成すると、送信機の色判定しきい値を上書きできます。
- 例（既定値と同じ構造体名を使用）:

```cpp
const ColorThreshold RED_THD = { .rMin = 140, .rMax = 255, .gMin = 0, .gMax = 70, .bMin = 0, .bMax = 70 };
const ColorThreshold BLUE_THD = { .rMin = 30, .rMax = 70, .gMin = 0, .gMax = 90, .bMin = 120, .bMax = 255 };
const ColorThreshold YELLOW_THD = { .rMin = 100, .rMax = 159, .gMin = 50, .gMax = 100, .bMin = 0, .bMax = 60 };
```

- `apps/sender/include/adjustParams.h` が `sensor_color_params.h` の存在を検出して読み込みます。
  ファイルが無い場合は既定値が使われます。

## 3) ビルド・書き込み（PlatformIO）

- ルートの `platformio.ini` で環境を選び、PlatformIO GUI から Build/Upload してください。
- Environments（例）: `receiver-BandWL_V2`, `sender-atom`, `broker-AtomS3`
  - receiver: 信号を受信する機器。基本的には Hapbeat
  - sender: 信号を送信する機器。基本的にはセンサーと接続された M5Stack ATOM LITE
  - broker: 信号を中継するサーバーのような役割：基本的には M5Stack S3

## 4) トラブルシュート（要点）

- 接続不安定: `BROKER_STATIC_HOST_OCTET` が DHCP 範囲内だと衝突しがち。範囲外の未使用値に変更。
- テザリング: /28 のように範囲が狭いので OCTET は 10 or 12 を推奨。
- 受信機が再起動: 初期化順を調整済み。継続する場合はご連絡ください。
