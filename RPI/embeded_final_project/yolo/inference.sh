#!/bin/bash

source "/home/user/yolov8/bin/activate"

# 清空先前資料
rm -rf "/home/user/embeded_final_project/yolo/runs/detect/predict/"

yolo predict task=detect model="/home/user/embeded_final_project/yolo/trained_weight/yolov8n/best.pt" source="/home/user/embeded_final_project/yolo/test_recognition/images/" save_crop=True imgsz=800,600