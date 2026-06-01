import socket
import threading
import tkinter as tk
from tkinter import messagebox
import json
import os

class TCPServerGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("TCP Server")
        self.root.geometry("300x150")
        
        self.server_socket = None
        self.client_socket = None
        self.is_waiting = False
        self.config_file = "tcp_server_config.json"
        
        port_frame = tk.Frame(root)
        port_frame.pack(pady=20)
        
        tk.Label(port_frame, text="ポート番号:", font=("Arial", 12)).pack(side=tk.LEFT, padx=5)
        self.port_entry = tk.Entry(port_frame, width=10, font=("Arial", 12))
        
        saved_port = self.load_port()
        self.port_entry.insert(0, str(saved_port))
        self.port_entry.pack(side=tk.LEFT, padx=5)
        
        # Openボタン
        self.open_button = tk.Button(root, text="Open", command=self.start_waiting, 
                                      bg="lightblue", font=("Arial", 16), 
                                      width=10, height=2)
        self.open_button.pack(pady=20)
    
    def load_port(self):
        """保存されたポート番号を読み込む"""
        try:
            if os.path.exists(self.config_file):
                with open(self.config_file, 'r') as f:
                    config = json.load(f)
                    return config.get('port', 9002)
        except Exception:
            pass
        return 9002
    
    def save_port(self, port):
        """ポート番号を保存"""
        try:
            with open(self.config_file, 'w') as f:
                json.dump({'port': port}, f)
        except Exception:
            pass
    
    def start_waiting(self):
        """Openボタンが押された時の処理 - 待機状態に入る"""
        try:
            port = int(self.port_entry.get())
            if port < 1024 or port > 65535:
                messagebox.showerror("エラー", "ポート番号は1024-65535の範囲で指定してください")
                return
        except ValueError:
            messagebox.showerror("エラー", "有効なポート番号を入力してください")
            return
        
        self.save_port(port)
        
        self.port_entry.config(state=tk.DISABLED)
        
        self.open_button.config(state=tk.DISABLED, text="待機中...")
        
        try:
            self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.server_socket.bind(('127.0.0.1', port))
            self.server_socket.listen(1)
            self.is_waiting = True
            
            threading.Thread(target=self.wait_and_respond, daemon=True).start()
            
        except Exception as e:
            messagebox.showerror("エラー", f"サーバーの起動に失敗しました: {e}")
            self.reset_ui()
    
    def wait_and_respond(self):
        """クライアントの接続を待ち、メッセージを受信したらwrite\nを返す"""
        while self.is_waiting:
            try:
                self.client_socket, client_address = self.server_socket.accept()
                
                while self.is_waiting:
                    data = self.client_socket.recv(1024)
                    
                    if not data:
                        break
                    
                    response = "Write\n"
                    self.client_socket.send(response.encode('utf-8'))
                
            except Exception as e:
                if self.is_waiting:
                    pass
            finally:
                if self.client_socket:
                    self.client_socket.close()
                    self.client_socket = None
        
        # サーバーソケットをクローズ
        if self.server_socket:
            self.server_socket.close()
            self.server_socket = None
    
    def reset_ui(self):
        """UIを初期状態に戻す"""
        self.port_entry.config(state=tk.NORMAL)
        self.open_button.config(state=tk.NORMAL, text="Open")
    
    def stop_waiting(self):
        """待機状態を停止"""
        self.is_waiting = False
        
        if self.client_socket:
            self.client_socket.close()
            self.client_socket = None
        
        if self.server_socket:
            self.server_socket.close()
            self.server_socket = None
        
        self.reset_ui()
    
    def on_closing(self):
        """ウィンドウを閉じる時の処理"""
        self.stop_waiting()
        self.root.destroy()

def main():
    root = tk.Tk()
    app = TCPServerGUI(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()

if __name__ == '__main__':
    main()