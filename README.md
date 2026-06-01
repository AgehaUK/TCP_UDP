# TCP_UDP

TCP / UDP 間でメッセージを中継・送受信するためのプログラム集です。
Windows 向けのネイティブな中継プログラム（C / C++ / Winsock）と、動作確認用の
Python クライアント・サーバー（GUI / CLI）で構成されています。

## 構成

### 中継プログラム（C++ / Winsock）

ネットワーク上の TCP と UDP を相互に橋渡しするブリッジです。設定は同じフォルダ内の
`.ini` ファイルから読み込み、ファイルが無ければデフォルト設定の雛形を自動生成します。

| ファイル | 役割 |
|---|---|
| `fm_TCP_to_UDP.cpp` | TCP サーバーへ接続し、受信したメッセージを UDP（ブロードキャスト可）へ転送。切断時は自動再接続（10秒間隔）。 |
| `fm_UDP_to_TCP.cpp` | UDP を受信し、接続中のすべての TCP クライアント（最大64）へブロードキャスト。 |
| `fm_UDP_to_TCP_multi.cpp` | `fm_UDP_to_TCP` の拡張版。TCP 受信 / UDP 受信 / UDP 転送を併用するマルチ構成。 |

設定ファイル例（`fm_TCP_to_UDP_config.ini`）:

```ini
[Setting]
udp_port=41449
tcp_port=51234
udp_ipaddr=192.168.101.255
tcp_ipaddr=192.168.100.18
```

### UDP 送受信ツール（C / Win32 GUI）

Win32 API でウィンドウを持つ、単体の UDP 送受信ツールです。

| ファイル | 役割 |
|---|---|
| `udp_sender.c` | 宛先 IP / ポートとメッセージを入力して UDP 送信する GUI ツール。 |
| `udp_receiver.c` | 指定ポートで待ち受け、受信した UDP メッセージを表示する GUI ツール。 |
| `udp_sender_config.txt` | 送信側の設定（1行目: 宛先IP、2行目: ポート）。 |
| `udp_receiver_config.txt` | 受信側の設定（`ポート IPアドレス`）。 |

### 動作確認用 Python スクリプト

| ファイル | 役割 |
|---|---|
| `tcp_server.py` | tkinter GUI の TCP サーバー。ポートを指定して待ち受け。 |
| `tcp_client.py` | TCP クライアント（CLI）。`127.0.0.1` の指定ポートへ接続。 |
| `udp_server.py` | UDP サーバー。`udp_server_config.txt` から設定を読み込み。 |
| `udp_client.py` | UDP クライアント（CLI）。標準入力のメッセージを送信。 |

## ビルド・実行

### C / C++（Windows）

Winsock（`ws2_32.lib`）を使用します。各ソースは `#pragma comment(lib, "ws2_32.lib")`
を含むため、ライブラリの明示リンクは不要です。

MSVC（Developer Command Prompt）の場合:

```bat
cl fm_TCP_to_UDP.cpp
cl fm_UDP_to_TCP.cpp
cl udp_sender.c
cl udp_receiver.c
```

初回はビルドした実行ファイルを起動すると設定ファイル（`.ini` / `.txt`）が生成されます。
設定を編集してから再度起動してください。

### Python

Python 3 が必要です（標準ライブラリのみ使用、`tcp_server.py` は tkinter を利用）。

```bash
python tcp_server.py
python udp_server.py
python udp_client.py
```

## 備考

- 中継プログラム群（`.cpp` / `.c`）は **Windows / Winsock 専用** です。
- コンパイル成果物（`.obj`）はリポジトリに含めていません（`.gitignore` で除外）。
- 設定ファイル内の IP アドレス・ポートは利用環境に合わせて編集してください。
