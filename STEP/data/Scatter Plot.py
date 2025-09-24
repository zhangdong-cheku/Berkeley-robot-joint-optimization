# 散点图（Scatter Plot）

# 用于展示“最终角度”与“目标角度”的偏差分布。
# 横轴：起始角度，纵轴：最终角度或偏差（最终角度-目标角度）。
# 在电机重复定位精度实验中，偏差越小越好。
# 偏差表示最终角度与目标角度的差异，偏差越小说明电机定位越准确，重复性越高。

#在散点图中，“均值±标准差”表示：

#均值：对于每一个起始角度，把所有实验的偏差（最终角度-目标角度）求平均，得到该起始角度下的平均偏差。
#标准差：衡量这些偏差的波动程度。标准差越大，说明实验结果的离散性越强，重复定位精度越差。
#在图上，红色点表示每个起始角度的平均偏差，误差棒（上下的线）表示该平均值上下一个标准差的范围，反映了数据的波动性。这样可以直观看出不同起始角度下定位误差的平均水平和稳定性。
import pandas as pd
import matplotlib.pyplot as plt

# 设置中文字体
plt.rcParams['font.sans-serif'] = ['SimHei']
plt.rcParams['axes.unicode_minus'] = False

# 读取数据
df = pd.read_excel('data.xlsx', sheet_name='Sheet1')

# 计算偏差
df['偏差'] = df['最终角度'] - df['目标角度']

plt.figure(figsize=(10, 7))

# 先绘制误差线和均值点，后绘制散点图，确保散点图在最上层
grouped = df.groupby('起始角度')['偏差']
means = grouped.mean()
stds = grouped.std()
plt.errorbar(means.index, means.values, yerr=stds.values, fmt='o', color='red', capsize=5, label='均值±标准差', zorder=2)

# 再绘制所有实验的散点
plt.scatter(df['起始角度'], df['偏差'], color='blue', alpha=0.7, label='每次实验偏差', s=40, zorder=3)

plt.title('最终角度与目标角度的偏差分布（含均值和波动）')
plt.xlabel('起始角度 (°)')
plt.ylabel('偏差 (°)')
plt.grid(True, zorder=1)
plt.legend()
plt.tight_layout()
plt.show()