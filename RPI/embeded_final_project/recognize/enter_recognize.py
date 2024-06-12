import cv2
import pytesseract
import os
import shutil
import numpy as np
import collections
import sys
from datetime import datetime

# 設置 input 和 output 路徑
# input_path = "/home/user/embeded_final_project/yolo/runs/detect/predict/crops/plate/"
input_path = "/home/user/embeded_final_project/yolo/cropped_images/"
# input_path = "/home/user/embeded_final_project/recognize/test_input/"
output_path = "/home/user/embeded_final_project/recognize/output_recognize/"
entry_log_path = "/home/user/embeded_final_project/recognize/entry_log.txt"

# 清空 output 資料夾
if os.path.exists(output_path):
    shutil.rmtree(output_path)
os.makedirs(output_path)

# 影像預處理函數 對比度和二值化用了反而更糟
def preprocess_image(img):
    # 轉換為灰度圖像
    img = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    # 去噪
    img = cv2.medianBlur(img, 1) # 要是奇數 能大幅影響辨識出來的結果
    return img

# 更精確的裁切車牌
def refine_crop(image):
    # 用彩色圖片進行處理
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    blurred = cv2.GaussianBlur(gray, (5, 5), 0)
    edged = cv2.Canny(blurred, 0, 255)  # 調整 Canny 邊緣檢測參數

    # 使用形態學操作來去除噪聲和加強邊緣
    kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (3, 3))
    closed = cv2.morphologyEx(edged, cv2.MORPH_CLOSE, kernel)

    contours, _ = cv2.findContours(closed, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
    contours = sorted(contours, key=cv2.contourArea, reverse=True)[:10]

    plate_contour = None
    for contour in contours:
        peri = cv2.arcLength(contour, True)
        approx = cv2.approxPolyDP(contour, 0.02 * peri, True)
        if len(approx) == 4:
            x, y, w, h = cv2.boundingRect(approx)
            aspect_ratio = w / float(h)
            # 這裡假設車牌的長寬比在 1.5 到 3.5 之間，可以根據實際情況調整
            if 1.5 <= aspect_ratio <= 3.5:
                plate_contour = approx
                break

    if plate_contour is None:
        return image

    x, y, w, h = cv2.boundingRect(plate_contour)
    cropped_plate = image[y:y + h, x:x + w]
    return cropped_plate

# 讀取 input_path 中的所有檔案
file_list = os.listdir(input_path)
valid_extensions = ('.jpg', '.jpeg', '.png', '.bmp', '.tiff', '.tif')

# 在遍历每个文件进行处理的循环外部，创建一个空的collections.Counter对象来跟踪每个识别出的车牌号码及其出现的次数
plate_counter = collections.Counter()

# 遍歷每個檔案進行處理
for file in file_list:
    # 確保只處理圖像檔案
    if file.lower().endswith(valid_extensions):
        # 讀取圖片
        img_path = os.path.join(input_path, file)
        img = cv2.imread(img_path)

        if img is None:
            print(f"Failed to load image: {img_path}")
            continue

        # 將影像固定放大至 380x160
        img = cv2.resize(img, (420, 180), interpolation=cv2.INTER_LINEAR)
        
        # 進一步裁切車牌
        refined_img = refine_crop(img)

        # 預處理影像
        binary = preprocess_image(refined_img)

        # 使用Tesseract進行文字識別
        config = '--psm 11 -c tessedit_char_whitelist=0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ'
        text = pytesseract.image_to_string(binary, config=config).strip()

        # 更新计数器
        plate_counter[text] += 1

        # 組合輸出檔名
        base_name = os.path.splitext(file)[0]  # 獲取不帶副檔名的檔名部分

        # 儲存預處理後的影像
        output_image_path = f"{output_path}/preprocessed_{base_name}.png"
        cv2.imwrite(output_image_path, binary)

        # 儲存進一步裁切後的影像
        refined_image_path = f"{output_path}/refined_{base_name}.png"
        cv2.imwrite(refined_image_path, refined_img)

        # 儲存文字結果
        output_file = f"{output_path}/output_{base_name}.txt"  # 構建新的輸出檔名
        with open(output_file, 'w') as f:
            f.write(text)

# 找出出现次数最多的车牌号码
most_common_plate, count = plate_counter.most_common(1)[0]

if len(most_common_plate) != 7:
    print(f"ERROR: recognize '{most_common_plate}' not 7 numbers\n")
else:
    # 獲取當前時間
    current_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    # 記錄車牌號碼和當前時間到文件中
    entry_log_path = "/home/user/embeded_final_project/recognize/entry_log.txt"  # 需要根據實際情況設置路徑
    with open(entry_log_path, 'a') as log_file:
        log_file.write(f"{most_common_plate},{current_time}\n")

    # 最後返回到 main 當中的數值 = 所有 print 和 stdout 的字元，所以最終運作時不要亂print東西
    print(f"success recognize: {most_common_plate}")