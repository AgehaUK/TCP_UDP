import socket

def main():
    # IPアドレスとポート番号の設定
    server_host = '127.0.0.1'
    server_port = 9001
    
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    
    try:
        while True:
            message = input("メッセージを入力してください (終了は'exit'): ")
            
            #ループを終了
            if message == 'exit':
                print("プログラムを終了します。")
                break
            
            # 文字列をUTF-8のバイト列にエンコード
            data = message.encode('utf-8')
            
            # サーバーにデータを送信
            client_socket.sendto(data, (server_host, server_port))
    
    except KeyboardInterrupt:
        print("\nプログラムを終了します。")
    finally:
        # ソケットのクローズ
        client_socket.close()

if __name__ == '__main__':
    main()