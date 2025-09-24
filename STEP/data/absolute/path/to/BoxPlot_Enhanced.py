# 增强版箱线图（Box Plot）
# 用于分析"百分表测量误差"的分布情况，包含详细统计信息标注

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# 设置中文字体
plt.rcParams['font.sans-serif'] = ['SimHei']
plt.rcParams['axes.unicode_minus'] = False

# 读取数据
df = pd.read_excel('data.xlsx', sheet_name='Sheet1')

# 计算统计量
error_data = df['百分表测量误差']
stats = {
    '中位数': np.median(error_data),
    '平均值': np.mean(error_data),
    '标准差': np.std(error_data),
    '最小值': np.min(error_data),
    '最大值': np.max(error_data),
    'Q1': np.percentile(error_data, 25),
    'Q3': np.percentile(error_data, 75),
    'IQR': np.percentile(error_data, 75) - np.percentile(error_data, 25)
}

# 创建图形
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 10))

# 主箱线图
box_plot = ax1.boxplot(error_data, vert=True, patch_artist=True, 
                      boxprops=dict(facecolor='lightblue', alpha=0.7),
                      medianprops=dict(color='red', linewidth=2),
                      whiskerprops=dict(color='black', linestyle='--'),
                      capprops=dict(color='black'),
                      flierprops=dict(marker='o', markersize=6, alpha=0.5))

ax1.set_title('百分表测量误差分布箱线图（增强版）', fontsize=14, fontweight='bold')
ax1.set_ylabel('百分表测量误差 (mm)', fontsize=12)
ax1.grid(True, alpha=0.3)

# 添加统计信息标注
stats_text = f"统计信息:\n" \
             f"样本数: {len(error_data)}\n" \
             f"中位数: {stats['中位数']:.3f} mm\n" \
             f"平均值: {stats['平均值']:.3f} mm\n" \
             f"标准差: {stats['标准差']:.3f} mm\n" \
             f"Q1: {stats['Q1']:.3f} mm\n" \
             f"Q3: {stats['Q3']:.3f} mm\n" \
             f"IQR: {stats['IQR']:.3f} mm\n" \
             f"范围: {stats['最小值']:.3f} - {stats['最大值']:.3f} mm"

ax1.text(1.2, np.max(error_data), stats_text, 
         bbox=dict(boxstyle="round,pad=0.5", facecolor="wheat", alpha=0.8),
         fontsize=10, verticalalignment='top')

# 添加异常值标注（如果有）
fliers = box_plot['fliers'][0].get_ydata()
if len(fliers) > 0:
    ax1.text(0.8, np.max(fliers), f"异常值: {len(fliers)}个", 
             fontsize=9, color='red', ha='right')

# 第二子图：按起始角度分组的箱线图（如果数据量足够）
if len(df['起始角度'].unique()) > 1:
    grouped_data = [df[df['起始角度'] == angle]['百分表测量误差'] for angle in df['起始角度'].unique()]
    ax2.boxplot(grouped_data, labels=[f"{angle}°" for angle in df['起始角度'].unique()])
    ax2.set_title('按起始角度分组的误差分布')
    ax2.set_xlabel('起始角度 (°)')
    ax2.set_ylabel('百分表测量误差 (mm)')
    ax2.grid(True, alpha=0.3)
    ax2.tick_params(axis='x', rotation=45)
else:
    ax2.axis('off')
    ax2.text(0.5, 0.5, '数据不足：需要多个起始角度数据进行分组分析', 
             ha='center', va='center', transform=ax2.transAxes)

plt.tight_layout()
plt.show()

# 输出详细统计报告
print("="*50)
print("百分表测量误差统计分析报告")
print("="*50)
print(f"数据样本数: {len(error_data)}")
print(f"误差范围: {stats['最小值']:.3f} mm - {stats['最大值']:.3f} mm")
print(f"平均值: {stats['平均值']:.3f} mm")
print(f"中位数: {stats['中位数']:.3f} mm")
print(f"标准差: {stats['标准差']:.3f} mm")
print(f"四分位距(IQR): {stats['IQR']:.3f} mm")
print(f"异常值数量: {len(fliers)}")
if len(fliers) > 0:
    print(f"异常值: {fliers}")
print("="*50)