import csv
import random
import os  # 🌟 新增：引入系统操作模块

# ==========================================
# ⚙️ 核心配置参数 
# ==========================================
NUM_REQUESTS = 500  
NUM_NODES = 21      
FILE_PATH = "data/sfc_requests/batch_01.csv" 

# 🌟 核心防爆盾：自动检测并创建所有缺失的文件夹！
os.makedirs(os.path.dirname(FILE_PATH), exist_ok=True)

# 打开文件准备写入
with open(FILE_PATH, mode='w', newline='', encoding='utf-8') as file:
    writer = csv.writer(file)
    writer.writerow(['id', 'source', 'destination', 'required_reliability', 'max_hops', 'required_cpu', 'required_bandwidth'])

    for req_id in range(NUM_REQUESTS):
        src = random.randint(0, NUM_NODES - 1)
        dst = random.randint(0, NUM_NODES - 1)
        while src == dst:
            dst = random.randint(0, NUM_NODES - 1)

        req_rel = round(random.uniform(0.55, 0.70), 2) 
        max_hops = random.randint(22, 30)              
        req_cpu = round(random.uniform(2.0, 8.0), 1)
        req_bw = round(random.uniform(10.0, 50.0), 1)

        writer.writerow([req_id, src, dst, req_rel, max_hops, req_cpu, req_bw])

print(f"✅ 成功生成 {NUM_REQUESTS} 条业务请求！")
print(f"📁 文件已保存至: {FILE_PATH}")
print(f"🎯 约束条件已放宽，GA 引擎已解除封印！")