# 直方图（Histogram）

# 展示“百分表测量误差”的分布频率，判断误差是否集中在某个范围。
#展示“百分表测量误差”的分布频率，判断误差是否集中在某个范围。
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

plt.rcParams['font.sans-serif'] = ['SimHei']
plt.rcParams['axes.unicode_minus'] = False

df = pd.read_excel('data.xlsx', sheet_name='Sheet1')

# 绘制直方图
plt.figure(figsize=(8, 6))
counts, bins, patches = plt.hist(df['百分表测量误差'], bins=10, color='skyblue', edgecolor='black')

# 在每个柱子上标注该区间内的最终角度范围
# 在每个柱子上标注该区间内的最终角度范围（横向放置）
for i in range(len(bins)-1):
    mask = (df['百分表测量误差'] >= bins[i]) & (df['百分表测量误差'] < bins[i+1])
    angles = df.loc[mask, '最终角度']
    if not angles.empty:
        label = f"{angles.min():.1f}°~{angles.max():.1f}°"
        plt.text((bins[i]+bins[i+1])/2, counts[i], label, ha='center', va='bottom', fontsize=8, rotation=0)  # rotation=0 横向

plt.title('百分表测量误差分布直方图')
plt.xlabel('百分表测量误差 (mm)')
plt.ylabel('频数')
plt.grid(True)
plt.tight_layout()
plt.show()