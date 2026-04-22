#pragma once

#include <vector>
#include <algorithm>
#include <random>
#include <set>
#include <unordered_set>
#include <iostream>
#include "topology.h"
#include "sfc_model.h"
#include <queue>
#include <unordered_map>

// =========================================================
// 🌟 核心算法引擎：拓扑感知遗传算法
// =========================================================
class GAEngine {
public:
    // ---------------------------------------------------------
    // 1. 论文规则落地：数学公式与约束条件
    // ---------------------------------------------------------
    static double calculatePathReliability(const std::vector<PhysicalNode>& path_nodes,
                                           const std::vector<PhysicalLink>& path_links) {
        if (path_nodes.empty()) return 0.0;
        double total_rel = 1.0;
        // 按照双参数结构计算节点可靠性
        for (const auto& node : path_nodes) total_rel *= node.reliability;
        return total_rel;
    }

    static bool isValid(const std::vector<PhysicalNode>& path, 
                        const std::vector<PhysicalLink>& links, 
                        const SFC_Request& req) {
        if (path.empty()) return false;
        if (path.size() < 2) return false; 
        if (path.size() - 1 > req.max_hops) return false; 

        for (const auto& node : path) {
            if (node.cpu_capacity < req.required_cpu) return false;
        }

        double rel = calculatePathReliability(path, links);
        if (rel < req.required_reliability) return false;

        return true;
    }

    static double calculateFitness(const std::vector<PhysicalNode>& primary, 
                                   const std::vector<PhysicalNode>& backup,
                                   const std::vector<PhysicalLink>& links,
                                   const SFC_Request& req) {
        if(primary.empty() || backup.empty()) return 0.0;

        // 维度1： 计算CPU的负载均衡得分
        double cpu_score = 0.0;
        // 计算主路径的CPU宽裕度
        for(const auto& node : primary){
            cpu_score += (node.cpu_capacity - req.required_cpu);
        }
        // 计算备用路径的CPU宽裕度
        for(const auto& node : backup){
            cpu_score += (node.cpu_capacity - req.required_cpu);
        }
        // 防止出现负分导致算法崩溃，加一个绝对值或保底值
        if(cpu_score <= 0) cpu_score = 1.0;

        // 维度2 & 3: 计算跳数惩罚与可靠性奖励
        int total_hops = (primary.size() - 1) + (backup.size() - 1);
        double rel_p = calculatePathReliability(primary,links);
        double rel_b = calculatePathReliability(backup,links);

        double final_score = (cpu_score * cpu_score) / (std::sqrt(total_hops + 1.0)) * (rel_p * rel_b);

        return final_score;
    }

    // ---------------------------------------------------------
    // 2. 染色体定义与种群初始化
    // ---------------------------------------------------------
    struct Chromosome {
        std::vector<PhysicalNode> primary_path;  
        std::vector<PhysicalNode> backup_path;   
        double fitness;    
        Chromosome() : fitness(0.0) {}                      
    };

    static bool generateRandomChromosome(const std::vector<PhysicalNode>& nodes,
                                         const std::vector<PhysicalLink>& links,
                                         const SFC_Request& req,
                                         int src_id, int dst_id,
                                         Chromosome& out_chrom, std::mt19937& rng) {
        // 1. 找主路径
        std::unordered_set<int> empty_blacklist; 
        std::vector<PhysicalNode> p_path = dfsRandom(src_id, dst_id, nodes, links, empty_blacklist, rng);
        // 验证合法性
        if (p_path.empty() || !isValid(p_path, links, req)) return false; 

        // 构建黑名单 将主路径里的中间节点全部剥离出来，装进blacklist数组
        std::unordered_set<int> blacklist;
        for (size_t i = 1; i < p_path.size() - 1; ++i) {
            blacklist.insert(p_path[i].id);
        }

        // 第二步：带着黑名单，找备用路径！
        // 下层寻路算法一旦看到 blacklist 里的节点，绝对不会踏入一步，从而实现物理隔离
        std::vector<PhysicalNode> b_path = dfsRandom(src_id, dst_id, nodes, links, blacklist, rng);
        if (b_path.empty() || !isValid(b_path, links, req)) return false; 

        // 第三步：组装成一条合法的染色体
        out_chrom.primary_path = p_path;
        out_chrom.backup_path = b_path;
        out_chrom.fitness = calculateFitness(p_path, b_path, links, req) + calculateFitness(b_path, b_path, links, req); 
        
        return true;
    }

    // 🌟 内部工具：纯净版确定性 BFS（模仿基准算法的行为）
    static std::vector<PhysicalNode> bfsDeterministic(int src_id, int dst_id,
                                                      const std::vector<PhysicalNode>& nodes,
                                                      const std::vector<PhysicalLink>& links,
                                                      const std::unordered_set<int>& exclude_nodes) {
        std::vector<PhysicalNode> path;
        std::unordered_map<int, std::vector<int>> adj;
        for (const auto& link : links) {
            adj[link.source_id].push_back(link.dest_id);
            adj[link.dest_id].push_back(link.source_id);
        }
        std::queue<int> q;
        std::unordered_map<int, int> parent;
        std::unordered_set<int> visited;

        q.push(src_id);
        visited.insert(src_id);
        bool found = false;

        while (!q.empty()) {
            int curr = q.front(); q.pop();
            if (curr == dst_id) { found = true; break; }

            for (int nxt : adj[curr]) {
                if (visited.find(nxt) == visited.end() && exclude_nodes.find(nxt) == exclude_nodes.end()) {
                    visited.insert(nxt);
                    parent[nxt] = curr;
                    q.push(nxt);
                }
            }
        }

        if (found) {
            int curr_node = dst_id;
            std::vector<int> path_ids;
            while (curr_node != src_id) {
                path_ids.push_back(curr_node);
                curr_node = parent[curr_node];
            }
            path_ids.push_back(src_id);
            std::reverse(path_ids.begin(), path_ids.end());

            for (int id : path_ids) {
                for (const auto& node : nodes) {
                    if (node.id == id) { path.push_back(node); break; }
                }
            }
        }
        return path;
    }

    static std::vector<Chromosome> initPopulation(
                                                int size, const std::vector<PhysicalNode>& nodes, 
                                                const std::vector<PhysicalLink>& links,
                                                const SFC_Request& req, 
                                                int src_id, int dst_id, 
                                                std::mt19937& rng)
        {
        std::vector<Chromosome> pop;
        
        // 1. 纯净随机探索：给足极大的重试次数，强迫使用 CPU 感知引擎找不同的路！
        int attempts = 0;
        int max_random_attempts = size * 20; 
        
        while (pop.size() < size && attempts < max_random_attempts) {
            Chromosome chrom;
            // 内部调用的是我们带有 cpu_penalty 的 dfsRandom
            // 它会自动嗅探哪里拥堵，从第 1 个请求开始就主动分散流量！
            if (generateRandomChromosome(nodes, links, req, src_id, dst_id, chrom, rng)) {
                pop.push_back(chrom);
            }
            attempts++;
        }

        // 2. 如果因为图太稀疏，实在找不满 50 个（比如找到了 15 个绝佳方案）
        // 绝不借用基准算法！而是把这 15 个优质方案交替填满种群，保证它们都是“绕行血脉”
        if (!pop.empty() && pop.size() < size) {
            int valid_count = pop.size();
            int i = 0;
            while (pop.size() < size) {
                pop.push_back(pop[i % valid_count]);
                i++;
            }
        }

        // 3. 终极兜底（只有在网络彻底卡死，GA 连 1 条路都找不到时，才给 1 个基准解）
        if (pop.empty()) {
            Chromosome seed_chrom;
            std::vector<PhysicalNode> p_path = bfsDeterministic(src_id, dst_id, nodes, links, {});
            if (!p_path.empty()) {
                std::unordered_set<int> exclude;
                for (size_t i = 1; i < p_path.size() - 1; ++i) exclude.insert(p_path[i].id);
                std::vector<PhysicalNode> b_path = bfsDeterministic(src_id, dst_id, nodes, links, exclude);
                if (!b_path.empty() && isValid(p_path, links, req) && isValid(b_path, links, req)) {
                    seed_chrom.primary_path = p_path;
                    seed_chrom.backup_path = b_path;
                    seed_chrom.fitness = calculateFitness(p_path, b_path, links, req);
                    pop.push_back(seed_chrom); 
                }
            }
        }

        return pop;

    }
    // ---------------------------------------------------------
    // 3. 终极主循环：启动遗传算法进行多代进化
    // ---------------------------------------------------------
    static Chromosome Run(const std::vector<PhysicalNode>& nodes,
                          const std::vector<PhysicalLink>& links,
                          const SFC_Request& req,
                          int src_id, int dst_id,
                          int population_size = 20, 
                          int generations = 50,
                          double MUTATION_RATE = 0.25) {
        
        std::cout << "\n🚀 [GA引擎启动] 开始执行拓扑感知遗传算法..." << std::endl;
        std::cout << "▶ 种群规模: " << population_size << " | 进化代数: " << generations << std::endl;

        // 🌟 修复 1：解除随机种子封印，使用真正的物理随机数！
        std::random_device rd;
        std::mt19937 rng(rd()); 

        std::vector<Chromosome> population = initPopulation(population_size, nodes, links, req, src_id, dst_id, rng);
        if (population.empty()) {
            std::cerr << "❌ 致命错误：初始化失败，无法在当前网络中找到合法路径！" << std::endl;
            return Chromosome(); 
        }

        Chromosome best_overall = population[0];

        for (int gen = 0; gen < generations; ++gen) {
            std::vector<Chromosome> next_generation;

            // 精英保留（逻辑完美，保留不变）
            Chromosome current_best = population[0];
            for (const auto& ind : population) {
                if (ind.fitness > current_best.fitness) current_best = ind;
                if (ind.fitness > best_overall.fitness) best_overall = ind; 
            }
            next_generation.push_back(current_best);

            // 繁衍
            while (next_generation.size() < population_size) {
                // 锦标赛选择（逻辑完美，保留不变）
                std::uniform_int_distribution<int> dist(0, population.size() - 1);
                Chromosome parent = population[dist(rng)];
                // 把锦标赛规模从 3 降到 2，稍微给普通个体一点活路，防止精英过早克隆霸屏
                for(int i = 0; i < 1; ++i) { 
                    Chromosome competitor = population[dist(rng)];
                    if(competitor.fitness > parent.fitness) parent = competitor;
                }

                Chromosome child = parent; 

                // 🌟 修复 2 & 3：处理变异逻辑
                std::uniform_real_distribution<double> prob(0.0, 1.0);
                if (prob(rng) < MUTATION_RATE) { 
                    Chromosome mutated_child;
                    if(generateRandomChromosome(nodes, links, req, src_id, dst_id, mutated_child, rng)) {
                        // 【治标方案】: 如果你目前只有生成全新路径的方法，我们勉强用它。
                        // 但是，为了让它有一丝机会活下来，必须确保它的 fitness 被正确计算了！
                        
                        // 强制调用你的裁判函数，给这个随机新生儿打分！
                        mutated_child.fitness = calculateFitness(mutated_child.primary_path, mutated_child.backup_path, links, req); 
                        
                        // 只有当这个随机产生的新路，不是一条“垃圾路”（比如分数>0），才允许它替换掉父母
                        if (mutated_child.fitness > 0) {
                            child = mutated_child; 
                        }
                    }
                }
                
                next_generation.push_back(child);
            }
            
            population = next_generation; 

            if ((gen + 1) % 10 == 0) {
                std::cout << "[进化日志] 第 " << (gen + 1) << " 代完成 | 当前最高适应度得分: " 
                          << current_best.fitness << std::endl;
            }
        }

        std::cout << "\n🏆 进化彻底完成！找到全局最优解。历史最高得分: " << best_overall.fitness << std::endl;
        return best_overall;
    }

private:
    // ---------------------------------------------------------
    // 4. 底层寻路引擎：带有黑名单屏蔽的随机化 DFS
    // ---------------------------------------------------------
    static std::vector<PhysicalNode> randomWalk(const std::vector<PhysicalNode>& nodes,
                                                const std::vector<PhysicalLink>& links,
                                                int src_id, int dst_id,
                                                const std::vector<int>& blacklist,
                                                int max_hops) {
        std::vector<PhysicalNode> path;
        std::set<int> visited;
        
        for (int bad_id : blacklist) visited.insert(bad_id);

        for (const auto& n : nodes) {
            if (n.id == src_id) {
                path.push_back(n);
                visited.insert(src_id);
                break;
            }
        }

        static std::mt19937 rng(std::random_device{}());

        std::vector<PhysicalNode> temp_path = dfsRandom(src_id, dst_id, nodes, links, {}, rng);
        if (!temp_path.empty()) {
            return path;
        }
        return {}; 
    }

    static std::vector<PhysicalNode> dfsRandom(int src_id, int dst_id,
                          const std::vector<PhysicalNode>& nodes,
                          const std::vector<PhysicalLink>& links,
                          const std::unordered_set<int>& exclude_nodes,
                          std::mt19937& rng) {
        std::vector<PhysicalNode> path;
        std::unordered_map<int, std::vector<int>> adj;
        
        for (const auto& link : links) {
            adj[link.source_id].push_back(link.dest_id);
            adj[link.dest_id].push_back(link.source_id); 
        }

        // 🌟 提取全网的 CPU 容量，作为智能避障的雷达
        std::unordered_map<int, double> cpu_map;
        for (const auto& n : nodes) {
            cpu_map[n.id] = n.cpu_capacity;
        }

        using PDI = std::pair<double, int>;
        std::priority_queue<PDI, std::vector<PDI>, std::greater<PDI>> pq;
        std::unordered_map<int, double> dist;
        std::unordered_map<int, int> parent;

        for (const auto& node : nodes) dist[node.id] = 1e12; // 初始距离无限大
        dist[src_id] = 0.0;
        pq.push({0.0, src_id});

        std::uniform_real_distribution<double> dist_noise(0.0, 2.0); // 基因扰动因子
        bool found = false;

        while (!pq.empty()) {
            double d = pq.top().first;
            int u = pq.top().second;
            pq.pop();

            if (u == dst_id) { found = true; break; }
            if (d > dist[u]) continue;

            for (int v : adj[u]) {
                if (exclude_nodes.find(v) != exclude_nodes.end()) continue;

                // ==========================================
                // 🧠 核心智能权重公式
                // ==========================================
                double base_hop_cost = 10.0; // 基础跳数成本 (极力压缩跳数)
                
                // CPU 容量越少，拥堵惩罚越大 (防止负数或0导致除以0报错)
                double safe_cpu = cpu_map[v] > 0.1 ? cpu_map[v] : 0.1; 
                double cpu_penalty = 100.0 / safe_cpu; 
                
                double noise = dist_noise(rng); // 增加随机性，避免每次走一模一样的路

                // 综合代价：跳数少、CPU多、带点随机的节点，代价最小！
                double cost = base_hop_cost + cpu_penalty + noise;

                if (dist[u] + cost < dist[v]) {
                    dist[v] = dist[u] + cost;
                    parent[v] = u;
                    pq.push({dist[v], v});
                }
            }
        }

        if (found) {
            int curr = dst_id;
            std::vector<int> path_ids;
            while (curr != src_id) {
                path_ids.push_back(curr);
                curr = parent[curr];
            }
            path_ids.push_back(src_id);
            std::reverse(path_ids.begin(), path_ids.end());

            for (int id : path_ids) {
                for (const auto& node : nodes) {
                    if (node.id == id) { path.push_back(node); break; }
                }
            }
        }
        return path;
    }         
};