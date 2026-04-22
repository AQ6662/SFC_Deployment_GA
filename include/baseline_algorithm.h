#ifndef BASELINE_ALGORITHM_H
#define BASELINE_ALGORITHM_H

#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include "topology.h"
#include "ga_engine.h"
#include "sfc_model.h"

class BaselineAlgorithm {
private:
    // 核心底层寻路引擎：广度优先搜索 (BFS)
    // 参数 exclude_nodes 用于实现“黑名单”机制，避开已经用过的节点
    static std::vector<PhysicalNode> bfs(
        const std::vector<PhysicalNode>& nodes, 
        const std::vector<PhysicalLink>& links, 
        int src_id, 
        int dst_id, 
        const std::unordered_set<int>& exclude_nodes) 
    {
        // 1. 构建邻接表，加速搜索
        std::unordered_map<int, std::vector<int>> adj;
        for (const auto& link : links) {
            // 注意：请确认你的 PhysicalLink 结构体里这两个变量叫什么，如果是 from/to，请自行修改
            adj[link.source_id].push_back(link.dest_id); 
            adj[link.dest_id].push_back(link.source_id); // 假设是双向网络
        }

        std::queue<int> q;
        std::unordered_map<int, int> parent; // 记录路径的来历
        std::unordered_set<int> visited;

        q.push(src_id);
        visited.insert(src_id);

        bool found = false;

        // 2. 开始向外扩散搜索
        while (!q.empty()) {
            int curr = q.front();
            q.pop();

            if (curr == dst_id) {
                found = true;
                break;
            }

            for (int neighbor : adj[curr]) {
                // 如果邻居没被访问过，且不在黑名单里
                if (visited.find(neighbor) == visited.end() && 
                    exclude_nodes.find(neighbor) == exclude_nodes.end()) {
                    visited.insert(neighbor);
                    parent[neighbor] = curr;
                    q.push(neighbor);
                }
            }
        }

        std::vector<PhysicalNode> final_path;
        if (!found) return final_path; // 没找到路，返回空

        // 3. 回溯生成路径节点 ID 序列
        int curr = dst_id;
        std::vector<int> path_ids;
        while (curr != src_id) {
            path_ids.push_back(curr);
            curr = parent[curr];
        }
        path_ids.push_back(src_id);
        std::reverse(path_ids.begin(), path_ids.end()); // 翻转，变成正向的 src -> ... -> dst

        // 4. 将 ID 转换为完整的 PhysicalNode 对象
        for (int id : path_ids) {
            for (const auto& node : nodes) {
                if (node.id == id) {
                    final_path.push_back(node);
                    break;
                }
            }
        }

        return final_path;
    }

public:
    static GAEngine::Chromosome Run(
        const std::vector<PhysicalNode>& physical_nodes, 
        const std::vector<PhysicalLink>& physical_links, 
        const SFC_Request& req, 
        int source_id, 
        int dest_id) 
    {
        GAEngine::Chromosome sol;

        // ==========================================
        // ⚔️ 真正的寻找“主备隔离”路径战役开始
        // ==========================================

        // 第一步：毫无保留，直接寻找全局最短的主路径 (不设置任何黑名单)
        std::vector<PhysicalNode> p_path_nodes = bfs(physical_nodes, physical_links, source_id, dest_id, {});

        // 如果连主路径都找不到，直接失败
        if (p_path_nodes.empty()) {
            sol.primary_path.clear();
            sol.backup_path.clear();
            sol.fitness = 0.0;
            return sol;
        }

        // 第二步：提取主路径中的“中间节点”，加入黑名单
        std::unordered_set<int> exclude_nodes;
        for (size_t i = 1; i < p_path_nodes.size() - 1; ++i) { // 避开 source 和 dest
            exclude_nodes.insert(p_path_nodes[i].id);
        }

        // 第三步：在残缺的地图上，艰难寻找备用路径 (物理节点绝对隔离)
        std::vector<PhysicalNode> b_path_nodes = bfs(physical_nodes, physical_links, source_id, dest_id, exclude_nodes);

        // 如果被主路截断后，找不到备用路径了，任务失败
        if (b_path_nodes.empty()) {
            sol.primary_path.clear();
            sol.backup_path.clear();
            sol.fitness = 0.0;
            return sol;
        }

        // ==========================================
        // ⚖️ 将双路结果提交给 GA 裁判所进行 QoS 盲审
        // ==========================================

        bool is_p_valid = GAEngine::isValid(p_path_nodes, physical_links, req);
        bool is_b_valid = GAEngine::isValid(b_path_nodes, physical_links, req);

        if (is_p_valid && is_b_valid) {
            sol.primary_path = p_path_nodes;
            sol.backup_path = b_path_nodes;
            // 包含4 个参数（主路、备路、全局物理链路、甲方请求）
            sol.fitness = GAEngine::calculateFitness(p_path_nodes, b_path_nodes, physical_links, req); 
        } else {
            sol.primary_path.clear();
            sol.backup_path.clear();
            sol.fitness = 0.0;
        }

        return sol;
    }
};

#endif // BASELINE_ALGORITHM_H