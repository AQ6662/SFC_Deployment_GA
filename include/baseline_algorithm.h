#pragma once
#include <vector>
#include <queue>
#include <set>
#include "topology.h"
#include "ga_engine.h"

class BaselineAlgorithm {
public:
    static GAEngine::Chromosome Run(const std::vector<PhysicalNode>& nodes,
                                    const std::vector<PhysicalLink>& links,
                                    const SFC_Request& req,
                                    int src_id, int dst_id) {
        
        GAEngine::Chromosome solution;
        
        // 1. 寻找主路径（无黑名单）
        std::vector<int> empty_bl;
        solution.primary_path = findShortestPath(nodes, links, src_id, dst_id, empty_bl);
        
        if (solution.primary_path.empty()) return solution; // 主路都找不到，直接失败

        // 2. 🌟 提取主路径的中间节点，生成黑名单（强迫基准算法也必须守规矩！）
        std::vector<int> blacklist;
        for (size_t i = 1; i < solution.primary_path.size() - 1; ++i) {
            blacklist.push_back(solution.primary_path[i].id);
        }

        // 3. 带着黑名单去寻找备用路径
        solution.backup_path = findShortestPath(nodes, links, src_id, dst_id, blacklist);

        // 只有两条路都找到了，且真的不相交，才算成功
        if (!solution.primary_path.empty() && !solution.backup_path.empty()) {
            // 用GAEngine的 isVaild 裁判函数严格审判主备路径
            if(GAEngine::isValid(solution.primary_path, links, req)&& GAEngine::isValid(solution.backup_path, links, req)) {
            // 只有完全符合max_hops和bandwidth要求的路径才算有效
            solution.fitness = GAEngine::calculateFitness(solution.primary_path, req);
            }else{
                // 如果跳数超标或者可靠性太低，直接判定为失败
                solution.primary_path.clear();
                solution.backup_path.clear();
            }

        return solution;
    }

private:
    // 带有黑名单机制的 BFS 最短路搜索
    static std::vector<PhysicalNode> findShortestPath(const std::vector<PhysicalNode>& nodes,
                                                      const std::vector<PhysicalLink>& links,
                                                      int start, int end,
                                                      const std::vector<int>& blacklist) {
        std::queue<std::vector<int>> q;
        q.push({start});
        
        std::set<int> visited;
        for (int bad_id : blacklist) visited.insert(bad_id); // 提前把黑名单节点标记为“已访问”
        visited.insert(start);

        while (!q.empty()) {
            std::vector<int> curr_path = q.front();
            q.pop();
            int curr_node = curr_path.back();

            if (curr_node == end) {
                std::vector<PhysicalNode> result;
                for (int id : curr_path) {
                    for (const auto& n : nodes) if (n.id == id) result.push_back(n);
                }
                return result;
            }

            for (const auto& link : links) {
                int next = -1;
                if (link.source_id == curr_node) next = link.dest_id;
                else if (link.dest_id == curr_node) next = link.source_id;

                if (next != -1 && visited.find(next) == visited.end()) {
                    visited.insert(next);
                    std::vector<int> next_path = curr_path;
                    next_path.push_back(next);
                    q.push(next_path);
                }
            }
        }
        return {}; // 找不到路返回空
    }
};