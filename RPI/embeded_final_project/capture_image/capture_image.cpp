#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <opencv2/opencv.hpp>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>

using namespace cv;

#define MAX_IMAGE_SIZE 1024 * 1024 // 假設最大影像大小為1MB
#define OUTPUT_FOLDER "/home/user/embeded_final_project/capture_image/output_capture_images/"
#define esp32cam_url "http://192.168.50.117/capture" // 根據esp32cam的網址調整

struct MemoryStruct
{
    unsigned char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    if (mem->size + realsize > MAX_IMAGE_SIZE)
    {
        realsize = MAX_IMAGE_SIZE - mem->size;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;

    return realsize;
}

void fetch_and_process_image(CURL *curl, int count)
{
    CURLcode res;
    struct MemoryStruct chunk;

    chunk.memory = (unsigned char *)malloc(MAX_IMAGE_SIZE);
    chunk.size = 0;

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK)
    {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }
    else
    {
        printf("response.headers OK!\n");

        // 檢查影像尺寸是否有效
        if (chunk.size > 0)
        {
            printf("response.status OK!\n");

            std::vector<uchar> data(chunk.memory, chunk.memory + chunk.size);
            Mat image = imdecode(data, IMREAD_COLOR);

            if (!image.empty() && image.cols > 0 && image.rows > 0)
            {
                printf("image.shape OK!\n");

                // 顯示影像 // 顯不出來
                // imshow("ESP32 Camera Image", image);

                // 保存影像到指定資料夾
                time_t now = time(NULL);
                struct tm *t = localtime(&now);
                char timestamp[32];
                strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", t);

                char image_filename[128];
                snprintf(image_filename, sizeof(image_filename), OUTPUT_FOLDER "/capture_%s_%d.jpg", timestamp, count);
                imwrite(image_filename, image);
                printf("Image saved as %s\n\n", image_filename);

                waitKey(100); // 等待0.1秒再顯示下一張圖像
            }
            else
            {
                printf("Invalid image data received\n");
            }
        }
        else
        {
            printf("Image size exceeds buffer size\n");
        }
    }

    free(chunk.memory);
}

void clear_output_folder()
{
    struct dirent *entry;
    DIR *dir = opendir(OUTPUT_FOLDER);

    if (dir == NULL)
    {
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        char filepath[256];
        snprintf(filepath, sizeof(filepath), OUTPUT_FOLDER "/%s", entry->d_name);

        // 忽略 "." 和 ".."
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            remove(filepath);
        }
    }
    closedir(dir);
}

int main(void)
{
    CURL *curl;
    CURLcode res;

    // 創建輸出資料夾
    struct stat st = {0};
    if (stat(OUTPUT_FOLDER, &st) == -1)
    {
        mkdir(OUTPUT_FOLDER, 0700);
    }

    // 清空輸出資料夾
    clear_output_folder();

    // 網路初始化
    printf("init network\n");
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl)
    {
        // 連接網路
        printf("connecting esp32cam network\n\n");
        curl_easy_setopt(curl, CURLOPT_URL, esp32cam_url);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        int count = 0;
        while (count < 3) // 設定要連拍幾次
        {
            count++;
            fetch_and_process_image(curl, count);
        }

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();

    return 0;
}
