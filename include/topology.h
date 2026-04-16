#pragma once  // 防止头文件重复包含


// 物理节点结构体  (代表 Topology Zoo 里的一个城市或机房)
struct PhysicalNode{
    int id;  // 节点唯一标识符
    double cpu_capacity;  // CPU容量
    double memory_capacity;  // 内存容量

    double reliability;  // 节点正常工作的概率
};

// 物理链路结构体 (代表 Topology Zoo 里的两条物理节点之间的连接)
struct PhysicalLink{
    int source_id;  // 起始节点ID
    int dest_id;    // 终点节点ID
    double bandwidth;  // 链路带宽
    double delay;    // 链路传输延迟

    double reliability;  // 链路正常工作的概率
};