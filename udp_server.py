import socket
import os

CONFIG_FILE = 'udp_server_config.txt'

def load_config():
    """設定ファイルから設定を読み込む"""
    if os.path.exists(CONFIG_FILE):
        try:
            with open(CONFIG_FILE, 'r') as f:
                lines = f.readlines()
                if len(lines) >= 2:
                    host = lines[0].strip()
                    port = int(lines[1].strip())
                    if 1024 <= port <= 65535:
                        return host, port
        except:
            pass
    return '127.0.0.1', 9001

def save_config(host, port):
    """設定ファイルに設定を保存"""
    with open(CONFIG_FILE, 'w') as f:
        f.write(f"{host}\n")
        f.write(f"{port}\n")

def main():
    # 設定の読み込み
    default_host, default_port = load_config()
    
    print("=== UDP Server Configuration ===")
    
    # IPアドレスの入力
    host_input = input(f"IPアドレスを入力 (default: {default_host}): ").strip()
    if host_input:
        host = host_input
    else:
        host = default_host
    
    # ポート番号の入力
    port_input = input(f"ポート番号を入力 (default: {default_port}): ").strip()
    if port_input:
        try:
            port = int(port_input)
            if port < 1024 or port > 65535:
                print("ポート番号は1024-65535の範囲で指定してください。デフォルト値を使用します。")
                port = default_port
        except ValueError:
            print("無効なポート番号です。デフォルト値を使用します。")
            port = default_port
    else:
        port = default_port
    
    # 設定を保存
    save_config(host, port)
    
    # UDPソケットの作成
    try:
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        server_socket.bind((host, port))
        
        print(f"\nサーバーが起動しました。{host}:{port} で待受中...")
        print("終了するには Ctrl+C を押してください。\n")
        
        while True:
            # データ受信を待機
            data, client_address = server_socket.recvfrom(1024)
            
            # 受信データをUTF-8でデコード
            try:
                message = data.decode('utf-8')
            except:
                message = data.decode('utf-8', errors='ignore')
            
            # 受信情報の表示
            print(f"[{client_address[0]}:{client_address[1]}] {message}")
    
    except OSError as e:
        print(f"\nエラー: {e}")
        print("別のプログラムが同じポートを使用している可能性があります。")
    except KeyboardInterrupt:
        print("\n\nサーバーを終了します。")
    finally:
        # ソケットのクローズ
        if 'server_socket' in locals():
            server_socket.close()

if __name__ == '__main__':
    main()