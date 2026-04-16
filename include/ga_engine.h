#pragma once

#include <vector>
#include <algorithm>
#include <random>
#include <set>
#include <iostream>
#include "topology.h"

// =========================================================
// 🌟 业务定义：用户的虚拟网络请求 (SFC Request)
// =========================================================
struct SFC_Request {
    double required_reliability; 
    int max_hops;                
    double required_cpu;         
    double required_bandwidth;   
};

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
        for (const auto& node : path_nodes) total_rel *= node.reliability;
        return total_rel;
    }

    static bool isValid(const std::vector<PhysicalNode>& path, 
                        const std::vector<PhysicalLink>& links, 
                        const SFC_Request& req) {
        if (path.size() < 2) return false; 
        if (path.size() - 1 > req.max_hops) return false; 

        for (const auto& node : path) {
            if (node.cpu_capacity < req.required_cpu) return false;
        }

        double rel = calculatePathReliability(path, links);
        if (rel < req.required_reliability) return false;

        return true;
    }

    static double calculateFitness(const std::vector<PhysicalNode>& path, const SFC_Request& req) {
        double score = 0.0;
        for (const auto& node : path) {
            score += (node.cpu_capacity - req.required_cpu);
        }
        return score;
    }

    // ---------------------------------------------------------
    // 2. 染色体定义与种群初始化
    // ---------------------------------------------------------
    struct Chromosome {
        std::vector<PhysicalNode> primary_path;  
        std::vector<PhysicalNode> backup_path;   
        double fitness;                          
    };

    static bool generateRandomChromosome(const std::vector<PhysicalNode>& nodes,
                                         const std::vector<PhysicalLink>& links,
                                         const SFC_Request& req,
                                         int src_id, int dst_id,
                                         Chromosome& out_chrom) {
        // 1. 找主路径
        std::vector<int> empty_blacklist; 
        std::vector<PhysicalNode> p_path = randomWalk(nodes, links, src_id, dst_id, empty_blacklist, req.max_hops);
        if (p_path.empty() || !isValid(p_path, links, req)) return false; 

        // 构建黑名单 将主路径里的中间节点全部剥离出来，装进blacklist数组
        std::vector<int> blacklist;
        for (size_t i = 1; i < p_path.size() - 1; ++i) {
            blacklist.push_back(p_path[i].id);
        }

        // 第二步：带着黑名单，找备用路径！
        // 下层寻路算法一旦看到 blacklist 里的节点，绝对不会踏入一步，从而实现物理隔离
        std::vector<PhysicalNode> b_path = randomWalk(nodes, links, src_id, dst_id, blacklist, req.max_hops);
        if (b_path.empty() || !isValid(b_path, links, req)) return false; 

        // 第三步：组装成一条合法的染色体
        out_chrom.primary_path = p_path;
        out_chrom.backup_path = b_path;
        out_chrom.fitness = calculateFitness(p_path, req) + calculateFitness(b_path, req); 
        
        return true;
    }

    static std::vector<Chromosome> initPopulation(int pop_size,
                                                  const std::vector<PhysicalNode>& nodes,
                                                  const std::vector<PhysicalLink>& links,
                                                  const SFC_Request& req,
                                                  int src_id, int dst_id) {
        std::vector<Chromosome> population;
        int max_retries = pop_size * 50; 
        int attempts = 0;

        while (population.size() < pop_size && attempts < max_retries) {
            Chromosome new_chrom;
            if (generateRandomChromosome(nodes, links, req, src_id, dst_id, new_chrom)) {
                population.push_back(new_chrom);
            }
            attempts++;
        }
        return population;
    }

    // ---------------------------------------------------------
    // 3. 终极主循环：启动遗传算法进行多代进化
    // ---------------------------------------------------------
    static Chromosome Run(const std::vector<PhysicalNode>& nodes,
                          const std::vector<PhysicalLink>& links,
                          const SFC_Request& req,
                          int src_id, int dst_id,
                          int population_size = 20, 
                          int generations = 50) {
        
        std::cout << "\n🚀 [GA引擎启动] 开始执行拓扑感知遗传算法..." << std::endl;
        std::cout << "▶ 种群规模: " << population_size << " | 进化代数: " << generations << std::endl;

        std::mt19937 rng(42); 

        std::vector<Chromosome> population = initPopulation(population_size, nodes, links, req, src_id, dst_id);
        if (population.empty()) {
            std::cerr << "❌ 致命错误：初始化失败，无法在当前网络中找到合法路径！" << std::endl;
            return Chromosome(); 
        }

        Chromosome best_overall = population[0];

        for (int gen = 0; gen < generations; ++gen) {
            std::vector<Chromosome> next_generation;

            // 精英保留
            Chromosome current_best = population[0];
            for (const auto& ind : population) {
                if (ind.fitness > current_best.fitness) current_best = ind;
                if (ind.fitness > best_overall.fitness) best_overall = ind; 
            }
            next_generation.push_back(current_best);

            // 繁衍
            while (next_generation.size() < population_size) {
                // 锦标赛选择
                std::uniform_int_distribution<int> dist(0, population.size() - 1);
                Chromosome parent = population[dist(rng)];
                for(int i = 0; i < 2; ++i) {
                    Chromosome competitor = population[dist(rng)];
                    if(competitor.fitness > parent.fitness) parent = competitor;
                }

                Chromosome child = parent; 

                // 拓扑感知变异 (30% 概率)
                std::uniform_real_distribution<double> prob(0.0, 1.0);
                if (prob(rng) < 0.30) { 
                    Chromosome mutated_child;
                    if(generateRandomChromosome(nodes, links, req, src_id, dst_id, mutated_child)) {
                        child = mutated_child; 
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

        if (dfsRandom(src_id, dst_id, nodes, links, visited, path, rng, max_hops)) {
            return path;
        }
        return {}; 
    }

    static bool dfsRandom(int current, int target,
                          const std::vector<PhysicalNode>& nodes,
                          const std::vector<PhysicalLink>& links,
                          std::set<int>& visited,
                          std::vector<PhysicalNode>& path,
                          std::mt19937& rng,
                          int max_hops) {
        if (current == target) return true; 

        // 核心优化： 深度剪枝
        // 如果当前走过的路程已经达到了最大允许跳数，直接回头！
        if (path.size() - 1 > max_hops) return false;

        std::vector<int> valid_neighbors;
        for (const auto& link : links) {
            if (link.source_id == current && visited.find(link.dest_id) == visited.end()) {
                valid_neighbors.push_back(link.dest_id);
            } else if (link.dest_id == current && visited.find(link.source_id) == visited.end()) {
                valid_neighbors.push_back(link.source_id);
            }
        }

        std::shuffle(valid_neighbors.begin(), valid_neighbors.end(), rng);

        for (int next_node_id : valid_neighbors) {
            visited.insert(next_node_id);
            for (const auto& n : nodes) {
                if (n.id == next_node_id) {
                    path.push_back(n);
                    break;
                }
            }

            if (dfsRandom(next_node_id, target, nodes, links, visited, path, rng, max_hops)) {
                return true; 
            }

            path.pop_back();
            visited.erase(next_node_id);
        }

        return false;
    }
};