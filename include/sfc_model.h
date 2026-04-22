// =========================================================
// 🌟 业务定义：用户的虚拟网络请求 (SFC Request)
// =========================================================

#ifndef SFC_MODEL_H   // 🌟 加上这一行
#define SFC_MODEL_H   // 🌟 加上这一行

struct SFC_Request {

    int id;
    int source;
    int destination;

    double required_reliability; // QoS 限制
    int max_hops;                // QoS 限制
    double required_cpu;         // CPU 算力要求
    double required_bandwidth;   
};

#endif