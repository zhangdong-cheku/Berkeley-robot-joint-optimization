#折线图

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

# 设置中文字体
plt.rcParams['font.sans-serif'] = ['SimHei']
plt.rcParams['axes.unicode_minus'] = False

# 检查文件是否存在
data_path = 'data.xlsx'
if not os.path.exists(data_path):
    raise FileNotFoundError(f"未找到数据文件: {data_path}")

# 读取数据
df = pd.read_excel(data_path, sheet_name='Sheet1')

# 检查必要的列是否存在
required_columns = ['起始角度', '目标角度', '最终角度']
for col in required_columns:
    if col not in df.columns:
        raise ValueError(f"数据表缺少必要列: {col}")

# 计算偏差
df['偏差'] = df['最终角度'] - df['目标角度']

plt.figure(figsize=(12, 7))
exp_numbers = df.index + 1
start_angles = df['起始角度'].astype(str)

# 每十组数据取一个起始角度作为标签，其余为空
x_labels = []
for i in range(len(df)):
    if i % 10 == 0:
        x_labels.append(f"{exp_numbers[i]}\n{start_angles[i]}°")
    else:
        x_labels.append("")

plt.plot(exp_numbers, df['偏差'], marker='o', color='green', label='每次实验偏差')
plt.xticks(exp_numbers, x_labels, rotation=0)
plt.title('实验编号与偏差的变化趋势（每十组对应一个起始角度）')
plt.xlabel('实验编号（下方为每十组的起始角度）')
plt.ylabel('偏差 (°)')
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.show()

# ----------- 散点图（含均值±标准差） -----------
# plt.figure(figsize=(10, 7))
# grouped = df.groupby('起始角度')['偏差']
# means = grouped.mean()
# stds = grouped.std()
# plt.errorbar(means.index, means.values, yerr=stds.values, fmt='o', color='red', capsize=5, label='均值±标准差', zorder=2)
# plt.scatter(df['起始角度'], df['偏差'], color='blue', alpha=0.7, label='每次实验偏差', s=40, zorder=3)
# plt.title('最终角度与目标角度的偏差分布（含均值和波动）')
# plt.xlabel('起始角度 (°)')
# plt.ylabel('偏差 (°)')
# plt.grid(True, zorder=1)
# plt.legend()
# plt.tight_layout()
# plt.show()



