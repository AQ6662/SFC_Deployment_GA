#include <iostream>
#include <vector>
#include <string>
#include <iomanip> // 用于对齐输出格式
#include "../include/topology.h"
#include "../include/topology_reader.h"
#include "../include/ga_engine.h"
#include "../include/request_reader.h"
#include "../include/baseline_algorithm.h" //  引入对照组

using namespace std;

int main() {
    cout << "=========================================================" << endl;
    cout << "🚀 基于遗传算法的 SFC 高可靠部署批处理引擎 (Thesis Edition)" << endl;
    cout << "=========================================================" << endl;

    vector<PhysicalNode> physical_nodes;
    vector<PhysicalLink> physical_links;
    
    // 1. 读取物理网络拓扑
    string gml_filepath = "data/topology_zoo/Atmnet.gml";
    cout << "[系统] 正在初始化底层物理网络: " << gml_filepath << " ..." << endl;
    TopologyReader::readGML(gml_filepath, physical_nodes, physical_links);

    if (physical_nodes.empty()) {
        cerr << "❌ 致命错误：拓扑文件加载失败，请检查路径。" << endl;
        return -1;
    }
    cout << "✅ 网络初始化完成！当前激活物理节点: " << physical_nodes.size() << " 个。\n" << endl;

    // 2. 读取批量业务请求数据集
    string csv_filepath = "data/sfc_requests/batch_01.csv";
    cout << "[系统] 正在加载虚拟业务请求集: " << csv_filepath << " ..." << endl;
    vector<SFC_Task> task_list = RequestReader::readCSV(csv_filepath);
    
    if (task_list.empty()) {
        cerr << "❌ 错误：未读取到任何有效请求，请检查 CSV 文件是否创建正确。" << endl;
        return -1;
    }
    cout << "✅ 成功解析批处理任务，共进入队列: " << task_list.size() << " 个请求。\n" << endl;

    // 3. 开启流水线，逐个处理请求
    int success_count = 0;
    double total_fitness = 0.0;

    for (const auto& task : task_list) {
        cout << "---------------------------------------------------------" << endl;
        cout << "📦 [调度开始] 任务 ID: " << setw(3) << setfill('0') << task.task_id 
             << " | 轨迹: Node " << task.source_id << " -> Node " << task.dest_id << endl;
        cout << "▶ 约束参数 -> 最小可靠性: " << task.req.required_reliability 
             << " | 最大跳数: " << task.req.max_hops 
             << " | CPU需求: " << task.req.required_cpu << endl;
        
        // 调用 GA 引擎 (种群: 30, 进化代数: 50)
        GAEngine::Chromosome best_solution = GAEngine::Run(
            physical_nodes, physical_links, task.req, task.source_id, task.dest_id, 30, 50
        );

        if (!best_solution.primary_path.empty()) {
            cout << "\n🟢 [部署成功] 主备不相交链路已锁定！" << endl;
            cout << "   ↳ 主用链路: ";
            for (const auto& n : best_solution.primary_path) cout << n.id << " -> "; cout << "终点\n";
            cout << "   ↳ 备用链路: ";
            for (const auto& n : best_solution.backup_path) cout << n.id << " -> "; cout << "终点\n";
            
            cout << "   ⭐ 方案适应度得分 (Fitness): " << best_solution.fitness << endl;
            
            success_count++;
            total_fitness += best_solution.fitness;
        } else {
            cout << "\n🔴 [部署失败] 在当前网络约束与地图限制下，无合法物理隔离解。" << endl;
        }
    }

    // 4. 输出最终实验统计结果
    cout << "\n=========================================================" << endl;
    cout << "📊 批处理实验报告 (Evaluation Report)" << endl;
    cout << "=========================================================" << endl;
    cout << "▶ 总共下发请求数 : " << task_list.size() << endl;
    cout << "▶ 成功部署请求数 : " << success_count << endl;
    cout << "▶ 算法整体成功率 : " << (task_list.empty() ? 0.0 : (double)success_count / task_list.size() * 100.0) << " %" << endl;
    if (success_count > 0) {
        cout << "▶ 成功方案平均得分: " << (total_fitness / success_count) << endl;
    }
    cout << "=========================================================" << endl;

    // 对照组实验情况 贪心算法与改进后遗传算法的情况
    int ga_success = 0;
    int baseline_success = 0;

    for (const auto& task : task_list) {
        cout << "---------------------------------------------------------" << endl;
        cout << "🔍 [对比实验] 任务 ID: " << task.task_id << endl;
        
        // 1. 运行你的【改进遗传算法】
        GAEngine::Chromosome ga_res = GAEngine::Run(physical_nodes, physical_links, task.req, task.source_id, task.dest_id);
        
        // 2. 运行【基准最短路算法】
        GAEngine::Chromosome base_res = BaselineAlgorithm::Run(physical_nodes, physical_links, task.req, task.source_id, task.dest_id);

        if (!ga_res.primary_path.empty()) ga_success++;
        if (!base_res.primary_path.empty()) baseline_success++;

        cout << "   结果汇报 -> 改进 GA: " << (ga_res.primary_path.empty() ? "❌失败" : "✅成功")
             << " | 基准算法: " << (base_res.primary_path.empty() ? "❌失败" : "✅成功") << endl;
    }

    // 输出论文最需要的对比结论
    cout << "\n================ 论文实验结论 (Conclusion) ================" << endl;
    cout << "算法名称            成功部署数      部署成功率" << endl;
    cout << "---------------------------------------------------------" << endl;
    cout << "基准最短路算法      " << baseline_success << "               " << (double)baseline_success/task_list.size()*100 << "%" << endl;
    cout << "改进遗传算法(本文)  " << ga_success << "               " << (double)ga_success/task_list.size()*100 << "%" << endl;
    cout << "=========================================================" << endl;

    return 0;
}