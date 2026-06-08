#!/bin/bash

# YCSB workloadb MongoDB 基准测试复现
# 环境：Docker 限制 10G 内存，WiredTiger cache 4.5G

WORK=$(pwd)
YCSB=$(pwd)/ycsb-0.17.0

# ============================================================
# 0. 清理旧容器
# ============================================================
if docker ps -a --format '{{.Names}}' | grep -q '^mongo-ycsb$'; then
  echo "⚠ 发现旧容器 mongo-ycsb，正在删除..."
  docker rm -f mongo-ycsb
fi

# ============================================================
# 1. 创建 MongoDB 数据目录（确保在 NVMe 分区上）
# ============================================================
sudo rm -fr $WORK/data/db
mkdir -p $WORK/data/db
echo "✓ 数据目录已创建: $WORK/data/db"

# ============================================================
# 2. 启动 MongoDB Docker 容器（10G cgroup 内存限制）
# ============================================================
docker run -d \
  --name mongo-ycsb \
  --memory=10g \
  --memory-swap=10g \
  --cpus=8 \
  -v $WORK/data/db:/data/db:z \
  -p 27017:27017 \
  mongo \
  mongod --wiredTigerCacheSizeGB 4.5 \
         --wiredTigerJournalCompressor none \
         --wiredTigerCollectionBlockCompressor none

echo "✓ MongoDB 容器已就绪"

# ============================================================
# 3. YCSB 阶段
# ============================================================
cd $YCSB

./bin/ycsb.sh load mongodb \
  -P workloads/workloadb \
  -p recordcount=20000000 \
  -p operationcount=6000000 \
  -threads 32 \
  &> $WORK/ycsb_load_wb_20M.log

echo "✓ YCSB Load 完成"

for i in {1..3}; do
  ./bin/ycsb.sh run mongodb \
    -P workloads/workloadb \
    -p recordcount=20000000 \
    -p operationcount=6000000 \
    -threads 32 \
    &> $WORK/ycsb_run_wb_run${i}.log

  echo "✓ YCSB 第 ${i} 次测试完成"
done

cd - &> /dev/null

# ============================================================
# 4. 停止并清理容器
# ============================================================
docker stop mongo-ycsb
docker rm mongo-ycsb
echo "✓ MongoDB 容器已清理"

# ============================================================
# 5. 提取结果
# ============================================================
for i in {1..3}; do
  echo "Run ${i} :"
  grep -E "Throughput|AverageLatency" $WORK/ycsb_run_wb_run${i}.log
done

cat /proc/vmstat | grep -E "pgpgin|pgpgout|workingset_refault"
