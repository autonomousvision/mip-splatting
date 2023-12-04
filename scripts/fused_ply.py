# create fused ply from a mip-splatting model
import os
import GPUtil
from concurrent.futures import ThreadPoolExecutor
import queue
import time

scenes = ["ship", "drums", "ficus", "hotdog", "lego", "materials", "mic", "chair"]

model_dir = "benchmark_nerf_synthetic_ours_stmt"

for scene in scenes:
    cmd = f"python create_fused_ply.py -m {model_dir}/{scene} --output_ply fused/{scene}_fused.ply"
    print(cmd)
    os.system(cmd)
    

# mip-nerf 360
scenes = ["bicycle", "bonsai", "counter", "flowers", "garden", "stump", "treehill", "kitchen", "room"]

model_dir = "benchmark_360v2_ours"
for scene in scenes:
    cmd = f"python create_fused_ply.py -m {model_dir}/{scene} --output_ply fused/{scene}_fused.ply"
    print(cmd)
    os.system(cmd)
