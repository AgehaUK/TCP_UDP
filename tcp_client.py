import socket
import threading

class TCPClient:
    def __init__(self, port):
        self.host = '127.0.0.1'
        self.port = port
        self.client_socket = None
        self.is_connected = False
    
    def connect(self):
        try:
            self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.client_socket.connect((self.host, self.port))
            self.is_connected = True
            print(f"サーバー (ポート {self.port}) に接続しました")
            
            threading.Thread(target=self.receive_messages, daemon=True).start()
            
            return True
        except Exception as e:
            print(f"接続エラー: {e}")
            return False
    
    def receive_messages(self):
        """サーバーからのメッセージを受信"""
        try:
            while self.is_connected:
                data = self.client_socket.recv(1024)
                if not data:
                    print("サーバーとの接続が切断されました")
                    self.is_connected = False
                    break
                
                message = data.decode('utf-8')
                print(f"\n[サーバーから受信] {message}", end="", flush=True)
                
        except Exception as e:
            if self.is_connected:
                print(f"\n受信エラー: {e}")
            self.is_connected = False
    
    def send_message(self, message):
        """メッセージを送信"""
        try:
            if self.client_socket and self.is_connected:
                self.client_socket.send(message.encode('utf-8'))
                return True
            else:
                print("サーバーに接続されていません")
                return False
        except Exception as e:
            print(f"送信エラー: {e}")
            self.is_connected = False
            return False
    
    def close(self):
        self.is_connected = False
        if self.client_socket:
            self.client_socket.close()
            print("接続を閉じました")

def main():
    
    # ポート番号の入力
    port_str = input("接続先ポート番号 (デフォルト: 9002): ").strip()
    if not port_str:
        port = 9002
    else:
        try:
            port = int(port_str)
            if port < 1024 or port > 65535:
                print("ポート番号は1024-65535の範囲で指定してください")
                return
        except ValueError:
            print("有効なポート番号を入力してください")
            return
    
    # クライアントの作成と接続
    client = TCPClient(port)
    
    if not client.connect():
        return
    
    try:
        # メッセージ送信ループ
        while client.is_connected:
            message = input("メッセージを入力してください (終了は'exit'): ")
            
            if message.lower() == 'exit':
                print("プログラムを終了します")
                break
            
            if message:
                if not client.send_message(message):
                    break
            
    except KeyboardInterrupt:
        print("\nプログラムを終了します")
    finally:
        client.close()

if __name__ == '__main__':
    main()