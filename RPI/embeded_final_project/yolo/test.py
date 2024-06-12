import os
import cv2
import numpy as np
import time
import shutil

# 加載ONNX模型
model_path = "/home/user/embeded_final_project/yolo/trained_weight/yolov8n/best.onnx"
model = cv2.dnn.readNetFromONNX(model_path)

# 設置輸入圖像目錄和輸出目錄
source_dir = "/home/user/embeded_final_project/capture_image/output_capture_images/"
# source_dir = "/home/user/embeded_final_project/yolo/test_recognition/images/"
output_dir = "/home/user/embeded_final_project/yolo/cropped_images/"

# 清空输出目录
if os.path.exists(output_dir):
    shutil.rmtree(output_dir)
os.makedirs(output_dir, exist_ok=True)

# 設置圖像大小
imgsz = (640, 640)

# 設置類別名稱
names = ["plate"]

# 非極大值抑制（NMS）函數
def nms(boxes, scores, iou_threshold):
    indices = cv2.dnn.NMSBoxes(boxes, scores, score_threshold=0.5, nms_threshold=iou_threshold)
    return indices

# 遍歷輸入目錄中的所有圖像
for img_name in os.listdir(source_dir):
    img_path = os.path.join(source_dir, img_name)
    img = cv2.imread(img_path)
    height, width, _ = img.shape
    length = max((height, width))
    image = np.zeros((length, length, 3), np.uint8)
    image[0:height, 0:width] = img
    scale = length / 640

    # 記錄開始時間
    start_time = time.monotonic()

    # 進行推論
    blob = cv2.dnn.blobFromImage(image, scalefactor=1 / 255, size=(640, 640), swapRB=True)
    model.setInput(blob)
    outputs = model.forward()

    # 處理輸出
    outputs = np.array([cv2.transpose(outputs[0])])
    rows = outputs.shape[1]

    boxes = []
    scores = []
    class_ids = []
    output = outputs[0]
    for i in range(rows):
        classes_scores = output[i][4:]
        minScore, maxScore, minClassLoc, (x, maxClassIndex) = cv2.minMaxLoc(classes_scores)
        if maxScore >= 0.25:
            box = [output[i][0] - 0.5 * output[i][2], output[i][1] - 0.5 * output[i][3],
                   output[i][2], output[i][3]]
            boxes.append(box)
            scores.append(maxScore)
            class_ids.append(maxClassIndex)

    result_boxes = cv2.dnn.NMSBoxes(boxes, scores, 0.25, 0.45, 0.5)
    for index in result_boxes:
        box = boxes[index]
        box_out = [round(box[0]*scale), round(box[1]*scale),
                   round((box[0] + box[2])*scale), round((box[1] + box[3])*scale)]
        print("Rect:", names[class_ids[index]], scores[index], box_out)

        # 保存裁剪後的圖像
        x1, y1, x2, y2 = box_out
        cropped_image = img[y1:y2, x1:x2]  # 裁剪圖像
        output_path = os.path.join(output_dir, f'{img_name}_cropped_{index}.jpg')
        cv2.imwrite(output_path, cropped_image)  # 保存裁剪後的圖像

    # 記錄結束時間
    end_time = time.monotonic()
    processing_time = end_time - start_time

    # 打印檔名和處理時間
    print(f"Processed {img_name} in {processing_time:.2f} seconds\n")

print(f"裁剪後的圖像已保存到 {output_dir}")