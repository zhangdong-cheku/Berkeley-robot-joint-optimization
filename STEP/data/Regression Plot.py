# 绘制线性回归图（Regression Plot）
# 分析“起始角度”对定位误差的影响，可以画“起始角度”与“百分表测量误差”的散点图，并拟合一条趋势线。

import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np

# 设置中文字体
plt.rcParams['font.sans-serif'] = ['SimHei']
plt.rcParams['axes.unicode_minus'] = False

# 读取数据
df = pd.read_excel('data.xlsx', sheet_name='Sheet1')

# 确保起始角度为数值型
df['起始角度'] = pd.to_numeric(df['起始角度'], errors='coerce')

plt.figure(figsize=(10, 7))
# 先绘制所有实验的散点
plt.scatter(df['起始角度'], df['百分表测量误差'], color='blue', s=40, alpha=0.7, label='每次实验散点')
# 再绘制回归线
sns.regplot(x='起始角度', y='百分表测量误差', data=df, scatter=False, line_kws={'color':'red'}, label='线性趋势')

plt.title('起始角度与百分表测量误差的线性回归分析')
plt.xlabel('起始角度 (°)')
plt.ylabel('百分表测量误差 (mm)')
plt.yticks(np.arange(df['百分表测量误差'].min(), df['百分表测量误差'].max() + 0.5, 0.5))
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.show()