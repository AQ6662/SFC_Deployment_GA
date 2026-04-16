#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <random> // 用于生成随机的 CPU、带宽等资源
#include "topology.h"

class TopologyReader {
public:
    /**
     * @brief 一键读取 GML 文件，并转化为 C++ 的节点和链路对象
     */
    static void readGML(const std::string& filepath, 
                        std::vector<PhysicalNode>& nodes, 
                        std::vector<PhysicalLink>& links) {
        
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "❌ 错误：无法打开文件 " << filepath << std::endl;
            return;
        }

        std::string line;
        bool in_node = false, in_edge = false;
        int current_id = -1, current_source = -1, current_target = -1;

        // --- 核心学术假设：配置随机资源生成器 ---
        // 保证每次跑出来的环境一样，方便复现，所以用固定种子(42)
        std::mt19937 gen(42); 
        std::uniform_real_distribution<> cpu_dist(50.0, 100.0);   // CPU 随机在 50~100 核
        std::uniform_real_distribution<> mem_dist(64.0, 128.0);   // 内存 64~128 GB
        std::uniform_real_distribution<> rel_dist(0.95, 0.999);   // 节点可靠性 95%~99.9%
        std::uniform_real_distribution<> bw_dist(500.0, 1000.0);  // 光纤带宽 500~1000 Mbps
        std::uniform_real_distribution<> delay_dist(2.0, 10.0);   // 传输延迟 2~10 毫秒

        // 逐行解析文件
        while (std::getline(file, line)) {
            // 判断当前读到的是节点还是链路
            if (line.find("node [") != std::string::npos) {
                in_node = true; in_edge = false;
            } else if (line.find("edge [") != std::string::npos) {
                in_node = false; in_edge = true;
            } else if (line.find("]") != std::string::npos) {
                // 一个大括号闭合了，把收集到的数据保存进 vector
                if (in_node && current_id != -1) {
                    PhysicalNode n = {current_id, cpu_dist(gen), mem_dist(gen), rel_dist(gen)};
                    nodes.push_back(n);
                    current_id = -1; in_node = false;
                } else if (in_edge && current_source != -1 && current_target != -1) {
                    PhysicalLink l = {current_source, current_target, bw_dist(gen), delay_dist(gen), rel_dist(gen)};
                    links.push_back(l);
                    current_source = -1; current_target = -1; in_edge = false;
                }
            }

            // 提取核心 ID 数据
            if (in_node && line.find("id ") != std::string::npos) {
                std::istringstream iss(line);
                std::string dummy;
                iss >> dummy >> current_id; // 把 "id" 丢掉，保留后面的数字
            } else if (in_edge) {
                if (line.find("source ") != std::string::npos) {
                    std::istringstream iss(line);
                    std::string dummy;
                    iss >> dummy >> current_source;
                } else if (line.find("target ") != std::string::npos) {
                    std::istringstream iss(line);
                    std::string dummy;
                    iss >> dummy >> current_target;
                }
            }
        }
        file.close();
        std::cout << "✅ 成功解析拓扑！包含节点数: " << nodes.size() 
                  << "，链路数: " << links.size() << std::endl;
    }
};