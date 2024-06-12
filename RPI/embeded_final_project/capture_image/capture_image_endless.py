import requests
import cv2
import numpy as np
import os
from datetime import datetime
# ESP32相機的網址
# esp32_camera_url = 'http://192.168.0.20/capture' # home
esp32_camera_url = 'http://192.168.50.119/capture' # lab

# 設置圖片保存的資料夾
output_folder = 'output_capture_images_endless'
if not os.path.exists(output_folder):
    os.makedirs(output_folder)

# 預先分配一個大緩衝區，用於存儲接收到的影像數據
max_image_size = 1024 * 1024  # 假設最大影像大小為1MB
buffer = bytearray(max_image_size)

count = 0
def capture_image():
    global count  # 使用 global 關鍵字宣告 count 為全域變數

    with requests.Session() as session:  # 使用 requests.Session 建立連線池
        while True:  # 使用 True 來持續執行迴圈
            count += 1
            print("\nloop: {}".format(count))

            try:
                # 發送GET請求獲取影像數據
                response = session.get(esp32_camera_url)
                if response.headers['Content-Type'] == 'image/jpeg':
                    print("response.headers OK!")

                    # 檢查請求是否成功
                    if response.status_code == 200:
                        print("response.status OK!")
                        # 將返回的影像數據存儲到預先分配的緩衝區
                        image_data = response.content
                        if len(image_data) <= max_image_size:
                            buffer[:len(image_data)] = image_data
                            nparr = np.frombuffer(buffer[:len(image_data)], np.uint8)
                            image = cv2.imdecode(nparr, cv2.IMREAD_COLOR)

                            # 檢查影像尺寸是否有效
                            if image is not None and image.shape[0] > 0 and image.shape[1] > 0:
                                print("image.shape OK!")

                                # 顯示影像
                                cv2.imshow('ESP32 Camera Image', image)

                                # 保存影像到指定資料夾
                                timestamp = datetime.now().strftime("%Y%m%d_%H%M%S_%f")
                                image_filename = os.path.join(output_folder, f"capture_{timestamp}.jpg")
                                cv2.imwrite(image_filename, image)
                                print(f"Image saved as {image_filename}")

                                cv2.waitKey(100)  # 等待0.1秒再顯示下一張圖像
                            else:
                                print("Invalid image data received")
                        else:
                            print("Image size exceeds buffer size")
                    else:
                        print(f"Failed to fetch image. Status code: {response.status_code}")
                else:
                    print(f"Unexpected content type: {response.headers['Content-Type']}")

            except requests.exceptions.ConnectionError as e:
                print(f"Connection error: {e}")
            except Exception as e:
                print(f"Error: {e}")

if __name__ == "__main__":
    capture_image()
