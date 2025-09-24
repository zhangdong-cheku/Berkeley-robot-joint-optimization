# 各起始角度误差分布（箱线图+散点）
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# 设置中文字体
plt.rcParams['font.sans-serif'] = ['SimHei', 'Microsoft YaHei', 'DejaVu Sans']
plt.rcParams['axes.unicode_minus'] = False

# 读取数据
df = pd.read_excel('data.xlsx', sheet_name='Sheet1')

# 按起始角度分组统计
means = df.groupby('起始角度')['百分表测量误差'].mean()

# 创建双图布局
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(20, 8))
fig.suptitle('电机重复定位精度分析 - 箱线图+散点图（详细说明）', fontsize=16, fontweight='bold')

# 1. 各起始角度分组箱线图+散点
box_data = [df[df['起始角度'] == angle]['百分表测量误差'] for angle in means.index]
box_plot = ax1.boxplot(box_data, labels=[f"{angle}°" for angle in means.index], 
                      patch_artist=True, boxprops=dict(facecolor='lightgreen'))

# 添加散点显示所有数据点
scatter_plots = []
for i, angle in enumerate(means.index):
    y = df[df['起始角度'] == angle]['百分表测量误差']
    x = np.random.normal(i+1, 0.04, size=len(y))
    scatter = ax1.scatter(x, y, alpha=0.6, s=30, color='blue', edgecolors='black', linewidth=0.5)
    if i == 0:  # 只添加一次图例
        scatter_plots.append(scatter)

ax1.set_title('各起始角度误差分布（分组分析）\n📊 箱线图说明：\n• 箱体：25%-75%数据范围\n• 中线：中位数\n• 须线：非异常值范围\n• 圆点：异常值', 
              fontsize=12, pad=20)
ax1.set_xlabel('起始角度 (°)')
ax1.set_ylabel('百分表测量误差 (mm)')
ax1.grid(True, alpha=0.3)
ax1.tick_params(axis='x', rotation=45)

# 添加图例说明
ax1.legend([scatter_plots[0], box_plot['boxes'][0]], 
          ['散点: 单个实验数据', '箱线: 统计分布'], 
          loc='upper right', framealpha=0.9)

# 2. 总体数据箱线图+散点
box_plot_total = ax2.boxplot([df['百分表测量误差']], labels=['全部数据'], 
                           patch_artist=True, boxprops=dict(facecolor='lightcoral'))

# 添加总体数据的散点（按起始角度用不同颜色）
colors = plt.cm.Set3(np.linspace(0, 1, len(means.index)))
scatter_legends = []
for i, angle in enumerate(means.index):
    y = df[df['起始角度'] == angle]['百分表测量误差']
    x = np.random.normal(1, 0.04, size=len(y))
    scatter = ax2.scatter(x, y, alpha=0.7, s=35, color=colors[i], 
                         edgecolors='black', linewidth=0.5, label=f'{angle}°')
    scatter_legends.append(scatter)

ax2.set_title('总体误差分布（综合视图）\n🎯 分析要点：\n• 箱体宽度反映数据集中程度\n• 中位数位置显示偏差趋势\n• 散点分布展示数据离散性', 
              fontsize=12, pad=20)
ax2.set_xlabel('数据分组')
ax2.set_ylabel('百分表测量误差 (mm)')
ax2.grid(True, alpha=0.3)

# 添加详细的图例
ax2.legend(handles=scatter_legends, title='起始角度', 
           bbox_to_anchor=(1.05, 1), loc='upper left', framealpha=0.9)

# 在图表右侧外部添加统计摘要文本框（避免遮挡图表）


# 在图表右下角添加统计摘要文本框
stats_text = f'''📈 总体统计摘要:
• 样本总数: {len(df)}
• 平均误差: {df['百分表测量误差'].mean():.3f} mm
• 标准差: {df['百分表测量误差'].std():.3f} mm
• 变异系数: {(df['百分表测量误差'].std()/df['百分表测量误差'].mean()*100):.1f}%
• 数据范围: {df['百分表测量误差'].min():.3f} - {df['百分表测量误差'].max():.3f} mm'''

fig.text(0.98, 0.02, stats_text, fontsize=10, verticalalignment='bottom',
         horizontalalignment='right',
         bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.8))

plt.tight_layout()
plt.subplots_adjust(bottom=0.15, right=0.85)  # 为底部和右侧留出空间
plt.show()



# ... existing code ...