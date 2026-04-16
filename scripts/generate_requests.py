import random
import csv
import os

# 1. 自动定位项目根目录 (根据脚本位置向上找一级)
# __file__ 是当前脚本路径，dirname 取其目录(scripts)，再 dirname 取上级(根目录)
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)

# 2. 设定目标数据存放路径
TARGET_DIR = os.path.join(PROJECT_ROOT, "data", "sfc_requests")
FILENAME = os.path.join(TARGET_DIR, "batch_01.csv")

# 3. 确保目标文件夹存在
os.makedirs(TARGET_DIR, exist_ok=True)

# 4. 实验参数：500 个任务是论文实验的“黄金标配”
NUM_REQUESTS = 500
MAX_NODE_ID = 20  # 匹配你的拓扑规模

print(f"📦 正在准备将数据注入结构化目录...")
print(f"📍 目标路径: {FILENAME}")

try:
    with open(FILENAME, mode='w', newline='') as file:
        writer = csv.writer(file)
        # 写入符合你 request_reader.h 解析逻辑的表头
        writer.writerow(['task_id', 'source_id', 'dest_id', 'reliability', 'max_hops', 'cpu', 'bandwidth'])
        
        for i in range(1, NUM_REQUESTS + 1):
            src = random.randint(0, MAX_NODE_ID)
            dst = random.randint(0, MAX_NODE_ID)
            while src == dst:
                dst = random.randint(0, MAX_NODE_ID)
            
            # 🌟 调整为中小型地图的合理约束范围
            rel = round(random.uniform(0.40, 0.85), 2)  # 可靠性放宽，备用链路绕远时容易跌破0.7
            hops = random.randint(15, 30)               # 最大跳数放宽，给主备隔离留足绕路空间
            cpu = round(random.uniform(5.0, 15.0), 1)   
            bw = round(random.uniform(20.0, 60.0), 1)
            
            writer.writerow([i, src, dst, rel, hops, cpu, bw])
            
    print(f"✅ 成功！500 条 SFC 请求已同步至 【数据层】。")
except Exception as e:
    print(f"❌ 写入失败，请检查文件夹权限: {e}")