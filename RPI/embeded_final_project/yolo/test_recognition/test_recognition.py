import os
import platform
import pylab as plt
import cv2
import numpy as np
import pytesseract
from PIL import Image, ImageFont, ImageDraw
from ultralytics import YOLO

def text(img, text, xy=(0, 0), color=(0, 0, 0), size=20):
    pil = Image.fromarray(img)
    s = platform.system()
    if s == "Linux":
        font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", size)
    elif s == "Darwin":
        font = ImageFont.truetype('....', size)
    else:
        font = ImageFont.truetype('simsun.ttc', size)
    ImageDraw.Draw(pil).text(xy, text, font=font, fill=color)
    return np.asarray(pil)

# 指定使用模型
model = YOLO("/home/user/embeded_final_project/yolo/trained_weight/yolov8n/best.pt")
# 新增輸出路徑
input_path = "./images"
# input_path = "/home/user/embeded_final_project/capture_image/output_capture_images/"
# input_path = "/home/user/embeded_final_project/yolo/runs/detect/predict/crops/plate/"
# 新增輸出路徑
output_path = "./predict"
plt.figure(figsize=(12, 9))

for i, file in enumerate(os.listdir(input_path)[:6]):
    full = os.path.join(input_path, file)
    print(full)
    img = cv2.imdecode(np.fromfile(full, dtype=np.uint8), cv2.IMREAD_COLOR)
    img = img[:, :, ::-1].copy()

    results = model.predict(img, save=False)
    boxes = results[0].boxes.xyxy
    for box in boxes:
        x1 = int(box[0])
        y1 = int(box[1])
        x2 = int(box[2])
        y2 = int(box[3])
        cv2.rectangle(cv2.UMat(img), (x1, y1), (x2, y2), (0, 255, 0), 2)  # 將 img 轉換為 UMat 型態

        tmp = cv2.cvtColor(img[y1:y2, x1:x2].copy(), cv2.COLOR_RGB2GRAY)
        license = pytesseract.image_to_string(tmp, lang='eng', config='--psm 11')
        img = text(img, license, (x1, y1 - 20), (0, 255, 0), 25)
        print(license)

    # 構建新的輸出檔名
    base_name = os.path.splitext(file)[0]  # 獲取不帶副檔名的檔名部分
    output_file = f"{output_path}/output_{base_name}.jpg"  # 構建新的輸出檔名
    plt.imshow(img)
    plt.savefig(output_file)  # 保存圖像到新的輸出檔名

    print(f"Saved output to: {output_file}")

