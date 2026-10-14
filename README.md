# VAO – Voice Auto Optimizer

OBS Studio 用の音声フィルタです。**画面に出る文章を読むだけで、マイクの音量・雑音・声の聞きやすさを自動で整えます。**
音量が小さくて聞いてもらえない配信をなくすことを、いちばんの目的にしています。

*An audio filter for OBS Studio. Read a short script aloud and VAO automatically sets up your microphone level, noise reduction and tone. (Japanese UI; English UI texts are included.)*

## できること

ハイパス → AI ノイズ抑制 → ノイズゲート → EQ → コンプレッサー → ディエッサー → 出力ゲイン → リミッター の順に、
声を整えます。各処理は個別にオン・オフでき、設定は「すべて自動設定」ボタン 1 つで決まります（約 26 秒の測定）。

- 音量が小さい声を、聞きやすい大きさまで持ち上げます（リミッターで音割れも防ぎます）
- ファン・空調などの雑音を、AI（RNNoise 内蔵）で減らします
- キーボードやマウスの音を測って、声がないあいだは静かにします
- EQ は「クリア」「あたたかい」「ラジオ」など、好みの音色（味付け）を選べます
- OBS だけで完結します。更新の通知も、外部への通信もありません

## ダウンロードと使い方

1. [Releases](https://github.com/retzrs-ops/VAO/releases) から、最新の **`VAO-<版>-windows-x64-Setup.exe`** をダウンロードします。
2. OBS Studio を終了してから、ダウンロードしたファイルをダブルクリックし、画面の案内に従って「インストール」→「完了」を押します（言語の選択が出たら「OK」）。
   - 青い警告「WindowsによってPCが保護されました」が出たら、「詳細情報」→「実行」を押してください
     （ソフトに電子署名が付いていないための警告です）。
3. OBS を起動し、マイクの音声ソースを右クリック →「フィルタ」→「音声フィルタ」の「＋」から「**VAO Voice**」を追加します。
4. 「**すべて自動設定（測定する）**」を押し、画面の案内に従います（黙る → キーボード → マウス → 文章を読む → 大きい声）。

くわしい使い方・設定項目・トラブルシューティングは、[Readme.txt](Readme.txt) をご覧ください
（作った理由と設計の考え方も書いてあります）。

## 動作環境

- Windows 10 / 11（64bit）
- OBS Studio（動作確認: 32.2.2）
- OBS の音声サンプルレートが 48 kHz（AI ノイズ抑制は 48 kHz のときだけ動きます）

## ライセンス

[GPL-2.0 以降](LICENSE)。AI ノイズ抑制には [RNNoise](https://github.com/xiph/rnnoise)（BSD-3-Clause、[licenses](licenses)）を使っています。

(c) 2026 Retzn
