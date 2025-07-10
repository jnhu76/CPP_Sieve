import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

def generate_report():
    """
    读取汇总的CSV数据，生成Markdown报告和PNG图表。
    """
    try:
        # 读取数据
        df = pd.read_csv("all_results.csv")
        print("Data loaded successfully:")
        print(df)

        # --- 1. 生成Markdown摘要报告 ---
        summary = df.pivot_table(
            index=['Version', 'Threads'], 
            columns='OS', 
            values='Time',
            aggfunc='mean'
        ).round(3)

        with open("summary_report.md", "w") as f:
            f.write("# Sieve Benchmark Summary\n\n")
            f.write("Execution time in seconds (average).\n\n")
            f.write(summary.to_markdown())

        print("\nSummary report 'summary_report.md' generated.")

        # --- 2. 生成图表 ---
        plt.style.use('seaborn-v0_8_whitegrid')
        fig, ax = plt.subplots(figsize=(16, 9))

        # 使用seaborn创建分组条形图
        sns.barplot(data=df, x='Version', y='Time', hue='OS', 
                    order=['mutex', 'spinlock', 'atomic', 'unsafe'],
                    ax=ax, errorbar=None)
        
        # 为每个线程数创建子分组
        # 这个部分较为复杂，一个更简单直观的方式是为每个线程数创建一张图
        # 这里我们简化处理，在图表中展示所有线程数的平均值，或者为每个线程数分别作图
        # 为简单起见，我们对所有线程数的数据进行分组展示
        
        df['x_label'] = df['Version'] + ' (' + df['Threads'].astype(str) + ' thr)'
        df_sorted = df.sort_values(by=['Version', 'Threads'])
        
        plt.figure(figsize=(20, 10))
        sns.barplot(data=df_sorted, x='x_label', y='Time', hue='OS')
        
        plt.title('Sieve Benchmark Performance Comparison', fontsize=20)
        plt.xlabel('Version (Threads)', fontsize=14)
        plt.ylabel('Execution Time (seconds)', fontsize=14)
        plt.xticks(rotation=45, ha='right')
        plt.legend(title='Operating System', fontsize=12)
        plt.tight_layout()
        
        # 保存图表
        plt.savefig("results_chart.png", dpi=150)
        print("Chart 'results_chart.png' generated.")

    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    generate_report()