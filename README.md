# 基于改进遗传算法的高可靠性服务功能链部署研究
.
├── CMakeLists.txt
├── data
│   ├── sfc_requests
│   └── topology_zoo
├── include
│   ├── baseline_algorithm.h
│   ├── ga_engine.h
│   ├── request_reader.h
│   ├── sfc_model.h
│   ├── topology.h
│   └── topology_reader.h
├── output
├── README.md
├── scripts
│   ├── generate_requests.py
│   ├── parse_gml.py
│   └── plot_results.py
└── src
    ├── ga_engine.cpp
    ├── main.cpp
    └── topology.cpp


## 🚀 核心改进点
- **拓扑感知初始化**：在基因生成阶段引入黑名单隔离机制，确保主备路径 100% 物理隔离。
- **动态深度剪枝**：针对中大型网络（如 Atmnet），优化 DFS 搜索深度，防止计算爆炸。

事件记录

2025.4.13

PART ONE
1.完成了“底层系统建模”： 你没有照抄别人的代码，而是从零开始，用 C++ 面向对象的思想，把论文里的“物理节点”、“光纤链路”以及最核心的“可靠性计算公式（连乘原理）”全部转化为了代码结构。

2.构建了“约束校验引擎”： 你把论文里复杂的数学约束条件（C1可靠性、C2跳数、C4计算资源），写成了精准的 C++ isValid 拦截器。

3. 实现了“真实拓扑感知”： 也是最了不起的一步。你脱离了“人造假数据”的低级趣味，写了一个解析器，成功读取并加载了国际标准的 Topology Zoo 骨干网数据。