import socket
import subprocess
import select
import time

# 記得還要去capture_image/capture_image.cpp 修改ESP32cam網址的網址

# 設定STM32要連接的IP和端口(在RPI內使用ifconfig)
STM32_IP = '172.20.10.3'
STM32_PORT = 12345  # 要與STM32內自己設定的port相同

# 創建socket對象
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# 設置SO_REUSEADDR選項
server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

server_socket.bind((STM32_IP, STM32_PORT))
server_socket.listen(1)

print(f"伺服器啟動，等待連接在 {STM32_IP}:{STM32_PORT}")

while True:
    # 接受來自STM32的連接
    client_socket, client_address = server_socket.accept()
    print(f"連接來自 {client_address}")

    try:
        # 使用select等待數據，超時設置為1秒
        ready_to_read, _, _ = select.select([client_socket], [], [], 1)

        if ready_to_read:
            # 接收資料
            data = client_socket.recv(1024).decode()
            if not data:
                print("wait new data")
                continue  # 繼續等待新的數據

            # 假設接收到的訊號是特定字串，比如"TRIGGER"
            if data == "TRIGGER_ENTER":
                print("接收到觸發訊號，開始執行命令")

                # 執行capture_image程式
                print("開始執行 capture_image")
                subprocess.run(["/home/user/embeded_final_project/capture_image/capture_image"], check=True)
                print("完成 capture_image")

                # 執行yolo的inference.sh腳本
                print("開始執行 yolo")
                # subprocess.run(["/home/user/embeded_final_project/yolo/inference.sh"], check=True)
                subprocess.run(["python", "/home/user/embeded_final_project/yolo/test.py"])
                print("完成 yolo")

                # 執行recognize.py腳本，並取得結果
                print("開始執行 recognize.py")
                # subprocess.run(["python", "/home/user/embeded_final_project/recognize/recognize.py"], check=True)
                result = subprocess.run(["python", "/home/user/embeded_final_project/recognize/enter_recognize.py"], capture_output=True, text=True, check=True)
                print(f"recognize.py 結果: {result.stdout}")
                print("完成 recognize.py")

                # 回傳執行完成訊號
                client_socket.sendall("DONE ".encode())

                # 回傳recognize.py的結果
                client_socket.sendall(result.stdout.encode())
                print("DONE \n")

            elif data == "TRIGGER_EXIT":
                print("接收到觸發訊號，開始執行命令")

                # 執行capture_image程式
                print("開始執行 capture_image")
                subprocess.run(["/home/user/embeded_final_project/capture_image/capture_image"], check=True)
                print("完成 capture_image")

                # 執行yolo的inference.sh腳本
                print("開始執行 yolo")
                # subprocess.run(["/home/user/embeded_final_project/yolo/inference.sh"], check=True)
                subprocess.run(["python", "/home/user/embeded_final_project/yolo/test.py"])
                print("完成 yolo")

                # 執行recognize.py腳本，並取得結果
                print("開始執行 recognize.py")
                # subprocess.run(["python", "/home/user/embeded_final_project/recognize/recognize.py"], check=True)
                result = subprocess.run(["python", "/home/user/embeded_final_project/recognize/exit_recognize.py"], capture_output=True, text=True, check=True)
                print(f"recognize.py 結果: {result.stdout}")
                print("完成 recognize.py")

                # 回傳執行完成訊號
                client_socket.sendall("DONE ".encode())

                # 回傳recognize.py的結果
                client_socket.sendall(result.stdout.encode())
                print("DONE \n")
            

            else:
                print("socket 收到未知訊號\n")

        else:
            # 等待期間進入低功耗模式
            time.sleep(0.1)  # 可調整休眠時間
    except subprocess.CalledProcessError as e:
        print(f"子程序執行錯誤: {e}")
    except socket.error as e:
        print(f"Socket error: {e}")
        break
    finally:
        # 關閉當前連接，並等待下一個連接
        client_socket.close()
        print("連接已關閉，等待下一個連接...")

# 關閉伺服器
server_socket.close()
