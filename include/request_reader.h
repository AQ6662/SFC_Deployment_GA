#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include "ga_engine.h"

// 定义一个完整的任务包 (包含起点、终点和 SFC 需求)
struct SFC_Task {
    int task_id;
    int source_id;
    int dest_id;
    SFC_Request req;
};

class RequestReader {
public:
    static std::vector<SFC_Task> readCSV(const std::string& filepath) {
        std::vector<SFC_Task> tasks;
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "❌ 无法打开请求数据文件: " << filepath << std::endl;
            return tasks;
        }

        std::string line;
        // 丢弃第一行表头
        std::getline(file, line); 
        
        // 逐行解析数据
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string token;
            SFC_Task t;
            
            try {
                std::getline(ss, token, ','); t.task_id = std::stoi(token);
                std::getline(ss, token, ','); t.source_id = std::stoi(token);
                std::getline(ss, token, ','); t.dest_id = std::stoi(token);
                std::getline(ss, token, ','); t.req.required_reliability = std::stod(token);
                std::getline(ss, token, ','); t.req.max_hops = std::stoi(token);
                std::getline(ss, token, ','); t.req.required_cpu = std::stod(token);
                std::getline(ss, token, ','); t.req.required_bandwidth = std::stod(token);
                tasks.push_back(t);
            } catch (const std::exception& e) {
                std::cerr << "⚠️ 警告: 行解析失败，跳过该行: " << line << std::endl;
            }
        }
        return tasks;
    }
};